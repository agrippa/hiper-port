#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Stmt.h"
#include "llvm/ADT/ArrayRef.h"

#include <algorithm>
#include <sstream>
#include <iostream>

#include "TimeBodyPass.h"

#define VERBOSE

/*
 * NOTE: The one thing to be careful about whenever inserting code in an
 * existing function is that the brace inserter is not run before this pass
 * anymore. That means that extra care must be taken to ensure that converting a
 * single line of code to a multi-line block of code does not change the control
 * flow of the application. This should be kept in mind anywhere that an
 * InsertText, ReplaceText, etc API is called.
 *
 * NOTE: In general, each rewriter use is followed by an assertion that it
 * should not fail. Rewrites can fail if the target location is invalid, e.g.
 * the target location is in a macro. So if you start getting these assert
 * failures, it's usually simpler to change the application code.
 */

extern clang::FunctionDecl *curr_func_decl;
extern std::vector<clang::ValueDecl *> globals;
extern int timingScope;

void TimeBodyPass::setParent(const clang::Stmt *child,
        const clang::Stmt *parent) {
    assert(parentMap.find(child) == parentMap.end());
    parentMap[child] = parent;
}

const clang::Stmt *TimeBodyPass::getParent(const clang::Stmt *s) {
    assert(parentMap.find(s) != parentMap.end());
    return parentMap.at(s);
}

std::string TimeBodyPass::stmtToString(const clang::Stmt *stmt) {
    // std::string s;
    // llvm::raw_string_ostream stream(s);
    // stmt->printPretty(stream, NULL, Context->getPrintingPolicy());
    // stream.flush();
    // return s;
    return rewriter->getRewrittenText(stmt->getSourceRange());
}

void TimeBodyPass::visitChildren(const clang::Stmt *s, bool firstTraversal) {
    for (clang::Stmt::const_child_iterator i = s->child_begin(),
            e = s->child_end(); i != e; i++) {
        const clang::Stmt *child = *i;
        if (child != NULL) {
            if (firstTraversal) {
                setParent(child, s);
            }
            VisitStmt(child);
        }
    }
}

void TimeBodyPass::preFunctionVisit(clang::FunctionDecl *func) {
    assert(getCurrentLexicalDepth() == 0);
    addNewScope();

    for (int i = 0; i < func->getNumParams(); i++) {
        clang::ParmVarDecl *param = func->getParamDecl(i);
        addToCurrentScope(param);
    }

    clang::PresumedLoc presumedStart = SM->getPresumedLoc(
            func->getLocStart());
    const int functionStartLine = presumedStart.getLine();
    pragmaTree = new PragmaNode(NULL, func->getBody(),
            new std::vector<clang::ValueDecl *>(), SM);
}

void TimeBodyPass::postFunctionVisit(clang::FunctionDecl *func) {
    assert(getCurrentLexicalDepth() == 1);
    popScope();

    if (func) {
        std::string fname = func->getNameAsString();
        clang::PresumedLoc presumedStart = SM->getPresumedLoc(
                func->getLocStart());
        const int functionStartLine = presumedStart.getLine();

        pragmaTree->print();

        std::vector<PragmaNode *> *leaves = pragmaTree->getLeaves();

        for (std::vector<PragmaNode *>::iterator i = leaves->begin(),
                e = leaves->end(); i != e; i++) {
            PragmaNode *node = *i;
            const clang::Stmt *body = node->getBody();
            std::string pragmaName = node->getPragmaName();

            std::stringstream ss;
            if (pragmaName == "omp_to_hclib") {
                assert(body);

                if (timingScope & FULL_PROGRAM) {
                    ss << "const unsigned long long full_program_start = " <<
                        "current_time_ns();" << std::endl;
                    ss << stmtToString(body) << " ; " << std::endl;
                    ss << "const unsigned long long full_program_end = " <<
                        "current_time_ns();" << std::endl;
                    ss << "printf(\"full_program \%llu ns\", " <<
                        "full_program_end - full_program_start);" << std::endl;
                } else {
                    ss << stmtToString(body) << std::endl;
                }
            } else if (pragmaName == "omp") {
                std::string ompCmd = node->getPragmaCmd();
                OMPClauses *clauses = getOMPClausesForMarker(node->getMarker());

                if (ompCmd == "parallel" && clauses->hasClause("for") &&
                        timingScope & PARALLEL_FOR) {
                    ss << " { const unsigned long long parallel_for_start = " <<
                        "current_time_ns();" << std::endl;
                    ss << "#pragma omp " << ompCmd << " " <<
                        clauses->getOriginalClauses() << std::endl;
                    ss << stmtToString(body) << " ; " << std::endl;
                    ss << "const unsigned long long parallel_for_end = " <<
                        "current_time_ns();" << std::endl;
                    ss << "printf(\"" << node->getLbl() << " \%llu ns\", " <<
                        "parallel_for_end - parallel_for_start); } " << std::endl;
                } else {
                    ss << "#pragma omp " << ompCmd << " " <<
                        clauses->getOriginalClauses() << std::endl;
                    if (body) {
                        ss << stmtToString(body);
                    }
                }
            } else if (pragmaName == "root") {
                // Nothing to do once we get to the top of the pragma tree
            } else {
                std::cerr << "Unrecognized pragma name \"" << pragmaName <<
                    "\"" << std::endl;
                exit(1);
            }

            std::string replaceWith = ss.str();
            if (replaceWith.size() > 0) {
                const bool failed = rewriter->ReplaceText(clang::SourceRange(
                            node->getStartLoc(), node->getEndLoc()),
                        ss.str());
                assert(!failed);
            }
        }
    }
}

bool TimeBodyPass::isScopeCreatingStmt(const clang::Stmt *s) {
    return clang::isa<clang::CompoundStmt>(s) || clang::isa<clang::ForStmt>(s);
}

int TimeBodyPass::getCurrentLexicalDepth() {
    return in_scope.size();
}

void TimeBodyPass::addNewScope() {
    in_scope.push_back(new std::vector<clang::ValueDecl *>());
}

void TimeBodyPass::popScope() {
    assert(in_scope.size() >= 1);
    in_scope.pop_back();
}

void TimeBodyPass::addToCurrentScope(clang::ValueDecl *d) {
    in_scope[in_scope.size() - 1]->push_back(d);
}

std::vector<clang::ValueDecl *> *TimeBodyPass::visibleDecls() {
    std::vector<std::string> alreadyCaptured;
    std::vector<clang::ValueDecl *> *visible =
        new std::vector<clang::ValueDecl *>();

    for (std::vector<std::vector<clang::ValueDecl *> *>::reverse_iterator i =
            in_scope.rbegin(), e = in_scope.rend(); i != e; i++) {
        std::vector<clang::ValueDecl *> *currentScope = *i;
        for (std::vector<clang::ValueDecl *>::iterator ii =
                currentScope->begin(), ee = currentScope->end(); ii != ee;
                ii++) {
            std::string varname = (*ii)->getNameAsString();
            // Deal with variables with the same name but in nested scopes
            if (std::find(alreadyCaptured.begin(), alreadyCaptured.end(),
                        varname) == alreadyCaptured.end()) {
                visible->push_back(*ii);
            }
            alreadyCaptured.push_back(varname);
        }
    }
    return visible;
}

std::string TimeBodyPass::getPragmaNameForMarker(const clang::CallExpr *call) {
    if (call == NULL) {
        // Root node only
        return "root";
    }
    assert(call->getNumArgs() == 2);
    const clang::Expr *pragmaNameArg = call->getArg(0);
    while (clang::isa<clang::ImplicitCastExpr>(pragmaNameArg)) {
        pragmaNameArg = clang::dyn_cast<clang::ImplicitCastExpr>(pragmaNameArg)->getSubExpr();
    }
    const clang::StringLiteral *literal = clang::dyn_cast<clang::StringLiteral>(pragmaNameArg);
    assert(literal);
    std::string pragmaName = literal->getString().str();
    return pragmaName;
}

std::string TimeBodyPass::getPragmaArgumentsForMarker(const clang::CallExpr *call) {
    assert(call->getNumArgs() == 2);
    const clang::Expr *pragmaArgsArg = call->getArg(1);
    while (clang::isa<clang::ImplicitCastExpr>(pragmaArgsArg)) {
        pragmaArgsArg = clang::dyn_cast<clang::ImplicitCastExpr>(pragmaArgsArg)->getSubExpr();
    }
    const clang::StringLiteral *literal = clang::dyn_cast<clang::StringLiteral>(pragmaArgsArg);
    assert(literal);
    return literal->getString().str();
}

const clang::Stmt *TimeBodyPass::getBodyFrom(const clang::CallExpr *call,
        std::string lbl) {
    const clang::Stmt *parent = getParent(call);
    const clang::Stmt *body = NULL;

    if (const clang::CompoundStmt *compound =
            clang::dyn_cast<clang::CompoundStmt>(parent)) {
        clang::CompoundStmt::const_body_iterator i = compound->body_begin();
        clang::CompoundStmt::const_body_iterator e = compound->body_end();
        while (i != e) {
            const clang::Stmt *curr = *i;
            if (curr == call) {
                i++;
                body = *i;
                break;
            }
            i++;
        }
    } else {
        std::cerr << "Unhandled parent while looking for body: " <<
            parent->getStmtClassName() << " of " << lbl << std::endl;
        exit(1);
    }

    assert(body);
    return body;
}

const clang::Stmt *TimeBodyPass::getBodyForMarker(const clang::CallExpr *call) {
    std::string pragmaName = getPragmaNameForMarker(call);
    std::string pragmaArgs = getPragmaArgumentsForMarker(call);

    if (pragmaName == "omp") {
        size_t ompPragmaNameEnd = pragmaArgs.find(' ');
        std::string ompPragma;

        if (ompPragmaNameEnd == std::string::npos) {
            // No clauses
            ompPragma = pragmaArgs;
        } else {
            ompPragma = pragmaArgs.substr(0, ompPragmaNameEnd);
        }

        if (ompPragma == "taskwait") {
            return NULL;
        } else if (ompPragma == "task" || ompPragma == "critical" ||
                ompPragma == "atomic" || ompPragma == "parallel" ||
                ompPragma == "single" || ompPragma == "simd" ||
                ompPragma == "master") {
            return getBodyFrom(call, ompPragma);
        } else {
            std::cerr << "Unhandled OMP pragma \"" << ompPragma << "\"" << std::endl;
            exit(1);
        }
    } else if (pragmaName == "omp_to_hclib") {
        return getBodyFrom(call, "omp_to_hclib");
    } else {
        std::cerr << "Unhandled pragma name \"" << pragmaName << "\"" <<
            std::endl;
        exit(1);
    }
}

OMPClauses *TimeBodyPass::getOMPClausesForMarker(const clang::CallExpr *call) {
    std::string pragmaName = getPragmaNameForMarker(call);
    assert(pragmaName == "omp");

    std::string pragmaArgs = getPragmaArgumentsForMarker(call);

    size_t ompPragmaNameEnd = pragmaArgs.find(' ');
    std::string ompPragma;

    OMPClauses *parsed = NULL;
    if (ompPragmaNameEnd != std::string::npos) {
        // Some clauses, non-empty args
        std::string clauses = pragmaArgs.substr(ompPragmaNameEnd + 1);
        parsed = new OMPClauses(clauses);
        ompPragma = pragmaArgs.substr(0, ompPragmaNameEnd);
    } else {
        parsed = new OMPClauses();
        ompPragma = pragmaArgs;
    }

    /*
     * This block of code is just here to make sure that if we run into a new
     * OMP clause we haven't tested before an error message prompts us. Since it
     * is decoupled from the actual handling of these clauses, this isn't a
     * particularly safe check.
     */
    for (std::map<std::string, std::vector<SingleClauseArgs *> *>::iterator i =
            parsed->begin(), e = parsed->end(); i != e; i++) {
        std::string clauseName = i->first;
        bool handledClause = false;

        if (ompPragma == "parallel") {
            if (clauseName == "collapse") {
                assert(parsed->hasClause("for"));
                handledClause = true;
            } else if (clauseName == "for" || clauseName == "private" ||
                    clauseName == "shared" || clauseName == "schedule" ||
                    clauseName == "reduction" || clauseName == "firstprivate" ||
                    clauseName == "num_threads" || clauseName == "default") {
                // Do nothing
                handledClause = true;
            }
        } else if (ompPragma == "task") {
            if (clauseName == "firstprivate" || clauseName == "private" ||
                    clauseName == "shared" || clauseName == "untied" ||
                    clauseName == "default" || clauseName == "if") {
                // Do nothing
                handledClause = true;
            } else if (clauseName == "depend") {
                // Handled during code generation.
                handledClause = true;
            }
        } else if (ompPragma == "single") {
            if (clauseName == "private" || clauseName == "nowait") {
                // Do nothing
                handledClause = true;
            }
        }
        if (!handledClause) {
            std::cerr << "Unchecked clause \"" << clauseName <<
                "\" on OMP pragma \"" << ompPragma << "\"" << std::endl;
            exit(1);
        }
    }

    return parsed;
}

void TimeBodyPass::VisitStmt(const clang::Stmt *s) {
    clang::SourceLocation start = s->getLocStart();
    clang::SourceLocation end = s->getLocEnd();

    const int currentScope = getCurrentLexicalDepth();
    if (isScopeCreatingStmt(s)) {
        addNewScope();
    }

    if (start.isValid() && end.isValid() && SM->isInMainFile(end)) {
        clang::PresumedLoc presumedStart = SM->getPresumedLoc(start);
        clang::PresumedLoc presumedEnd = SM->getPresumedLoc(end);

        /*
         * Look for OMP calls and make sure the user has to replace them with
         * something appropriate.
         */
        if (const clang::CallExpr *call = clang::dyn_cast<clang::CallExpr>(s)) {
            if (call->getDirectCallee()) {
                const clang::FunctionDecl *callee = call->getDirectCallee();
                std::string calleeName = callee->getNameAsString();

                if (calleeName == "hclib_pragma_marker") {
                    const clang::Stmt *body = getBodyForMarker(call);
                    std::vector<clang::ValueDecl *> *captures = visibleDecls();
                    PragmaNode *node = new PragmaNode(call, body, captures, SM);
                    pragmaTree->addChild(node);
                }
            }
        }

        /*
         * First check if this statement is a variable declaration, and if so
         * add it to the list of visible declarations in the current scope. This
         * step must be done before the block of code below for the case where
         * the last statement before an OMP pragma is a variable declaration. We
         * want to make sure that declaration is included in the capture list of
         * the following pragma.
         */
        if (const clang::DeclStmt *decls = clang::dyn_cast<clang::DeclStmt>(s)) {
            for (clang::DeclStmt::const_decl_iterator i = decls->decl_begin(),
                    e = decls->decl_end(); i != e; i++) {
                clang::Decl *decl = *i;
                if (clang::ValueDecl *named = clang::dyn_cast<clang::ValueDecl>(decl)) {
                    addToCurrentScope(named);
                }
            }
        }
    }

    visitChildren(s);

    if (isScopeCreatingStmt(s)) {
        popScope();
    }
    assert(currentScope == getCurrentLexicalDepth());
}

TimeBodyPass::TimeBodyPass() {
}

TimeBodyPass::~TimeBodyPass() {
}
