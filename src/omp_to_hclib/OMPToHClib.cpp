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

#include "OMPToHClib.h"
#include "OMPNode.h"

extern clang::FunctionDecl *curr_func_decl;

#define ASYNC_SUFFIX "_hclib_async"

std::vector<OMPPragma> *OMPToHClib::getOMPPragmasFor(
        clang::FunctionDecl *decl, clang::SourceManager &SM) {
    std::vector<OMPPragma> *result = new std::vector<OMPPragma>();
    if (!decl->hasBody()) return result;

    clang::SourceLocation start = decl->getBody()->getLocStart();
    clang::SourceLocation end = decl->getBody()->getLocEnd();

    clang::PresumedLoc startLoc = SM.getPresumedLoc(start);
    clang::PresumedLoc endLoc = SM.getPresumedLoc(end);

    for (std::vector<OMPPragma>::iterator i = pragmas->begin(),
            e = pragmas->end(); i != e; i++) {
        OMPPragma curr = *i;

        bool before = (curr.getLine() < startLoc.getLine());
        bool after = (curr.getLine() > endLoc.getLine());

        if (!before && !after) {
            result->push_back(curr);
        }
    }

    return result;
}

OMPPragma *OMPToHClib::getOMPPragmaFor(int lineNo) {
    for (std::vector<OMPPragma>::iterator i = pragmas->begin(),
            e = pragmas->end(); i != e; i++) {
        OMPPragma *curr = &*i;
        if (curr->getLine() == lineNo) return curr;
    }
    return NULL;
}

void OMPToHClib::setParent(const clang::Stmt *child,
        const clang::Stmt *parent) {
    assert(parentMap.find(child) == parentMap.end());
    parentMap[child] = parent;
}

std::string OMPToHClib::stmtToString(const clang::Stmt* stmt) {
    std::string s;
    llvm::raw_string_ostream stream(s);
    stmt->printPretty(stream, NULL, Context->getPrintingPolicy());
    stream.flush();
    return s;
}

void OMPToHClib::visitChildren(const clang::Stmt *s, bool firstTraversal) {
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

std::string OMPToHClib::getStructDef(OMPNode *node) {
    const int pragmaLine = node->getPragmaLine();
    std::string struct_def = "typedef struct _" + node->getLbl() + " {\n";
    for (std::vector<clang::ValueDecl *>::iterator ii =
            captures[pragmaLine]->begin(), ee = captures[pragmaLine]->end();
            ii != ee; ii++) {
        clang::ValueDecl *curr = *ii;
        struct_def += "    " + curr->getType().getAsString() + " " +
            curr->getNameAsString() + ";\n";
    }
    struct_def += " } " + node->getLbl() + ";\n\n";

    return struct_def;
}

void OMPToHClib::preFunctionVisit(clang::FunctionDecl *func) {
    assert(getCurrentLexicalDepth() == 0);
    addNewScope();

    for (int i = 0; i < func->getNumParams(); i++) {
        clang::ParmVarDecl *param = func->getParamDecl(i);
        addToCurrentScope(param);
    }
}

void OMPToHClib::postFunctionVisit(clang::FunctionDecl *func) {
    assert(getCurrentLexicalDepth() == 1);
    popScope();

    if (func) {
        std::string fname = func->getNameAsString();
        clang::PresumedLoc presumedStart = SM->getPresumedLoc(func->getLocStart());
        const int functionStartLine = presumedStart.getLine();

        OMPNode rootNode(NULL, functionStartLine, NULL, NULL, std::string("root"), SM);

        for (std::map<int, const clang::Stmt *>::iterator i = predecessors.begin(),
                e = predecessors.end(); i != e; i++) {
            const int line = i->first;
            const clang::Stmt *pred = i->second;
            assert(successors.find(line) != successors.end());
            assert(captures.find(line) != captures.end());
            const clang::Stmt *succ = successors[line];
            OMPPragma *pragma = getOMPPragmaFor(line);

            std::string lbl = fname + std::to_string(line);
            rootNode.addChild(succ, line, pragma, lbl, SM);
        }

        rootNode.print();

        std::string accumulatedStructDefs = "";
        std::string accumulatedKernelDefs = "";

        if (rootNode.nchildren() > 0) {
            std::vector<OMPNode *> *todo = new std::vector<OMPNode *>();
            std::vector<OMPNode *> *processing = rootNode.getLeaves();

            while (processing->size() != 0) {
                for (std::vector<OMPNode *>::iterator i = processing->begin(),
                        e = processing->end(); i != e; i++) {
                    OMPNode *node = *i;
                    const clang::Stmt *succ = successors[node->getPragmaLine()];
                    const clang::Stmt *pred = predecessors[node->getPragmaLine()];

                    if (supportedPragmas.find(node->getPragma()->getPragmaName()) != supportedPragmas.end()) {
                        if (node->getPragma()->getPragmaName() == "parallel") {
                            std::map<std::string, std::vector<std::string> > clauses = node->getPragma()->getClauses();
                            if (clauses.find("for") != clauses.end()) {
                                const std::string structDef = getStructDef(node);
                                accumulatedStructDefs = accumulatedStructDefs + structDef;

                                const clang::ForStmt *forLoop = clang::dyn_cast<clang::ForStmt>(node->getBody());
                                if (!forLoop) {
                                    std::cerr << "Expected to find for loop inside omp parallel for but found a " << node->getBody()->getStmtClassName() << " instead." << std::endl;
                                    std::cerr << stmtToString(node->getBody()) << std::endl;
                                    exit(1);
                                }
                                const clang::Stmt *init = forLoop->getInit();
                                const clang::Stmt *cond = forLoop->getCond();
                                const clang::Expr *inc = forLoop->getInc();

                                std::string bodyStr = stmtToString(forLoop->getBody());

                                std::string lowStr = "";
                                std::string highStr = "";
                                std::string strideStr = "";

                                accumulatedKernelDefs += "static void " + node->getLbl() + ASYNC_SUFFIX + "(void *arg, const int ___iter) {\n";
                                accumulatedKernelDefs += "    " + node->getLbl() + " *ctx = (" + node->getLbl() + " *)arg;\n";
                                for (std::vector<clang::ValueDecl *>::iterator ii =
                                        captures[node->getPragmaLine()]->begin(), ee = captures[node->getPragmaLine()]->end();
                                        ii != ee; ii++) {
                                    clang::ValueDecl *curr = *ii;
                                    accumulatedKernelDefs += "    " +
                                        curr->getType().getAsString() + " " +
                                        curr->getNameAsString() + " = ctx->" +
                                        curr->getNameAsString() + ";\n";
                                }

                                const clang::ValueDecl *condVar = NULL;

                                if (const clang::BinaryOperator *bin = clang::dyn_cast<clang::BinaryOperator>(init)) {
                                    clang::Expr *lhs = bin->getLHS();
                                    if (const clang::DeclRefExpr *declref = clang::dyn_cast<clang::DeclRefExpr>(lhs)) {
                                        condVar = declref->getDecl();
                                        std::string varname = condVar->getNameAsString();
                                        accumulatedKernelDefs += "    " + varname + " = ___iter;\n";

                                        lowStr = stmtToString(bin->getRHS());
                                    } else {
                                        std::cerr << "LHS is unsupported class " << std::string(lhs->getStmtClassName()) << std::endl;
                                        exit(1);
                                    }
                                } else {
                                    std::cerr << "Unsupported init class " <<
                                        std::string(init->getStmtClassName()) <<
                                        std::endl;
                                    exit(1);
                                }

                                if (const clang::BinaryOperator *bin = clang::dyn_cast<clang::BinaryOperator>(cond)) {
                                    if (bin->getOpcode() == clang::BO_LT) {
                                        clang::Expr *lhs = bin->getLHS();
                                        while (clang::isa<clang::ImplicitCastExpr>(lhs)) {
                                            lhs = clang::dyn_cast<clang::ImplicitCastExpr>(lhs)->getSubExpr();
                                        }
                                        std::cerr << "LHS = " << std::string(lhs->getStmtClassName()) << std::endl;
                                        const clang::DeclRefExpr *declref = clang::dyn_cast<clang::DeclRefExpr>(lhs);
                                        assert(declref);
                                        const clang::ValueDecl *decl = declref->getDecl();
                                        assert(condVar == decl);
                                        highStr = stmtToString(bin->getRHS());
                                    } else {
                                        std::cerr << "Unsupported binary operator" << std::endl;
                                        exit(1);
                                    }
                                } else {
                                    std::cerr << "Unsupported cond class " <<
                                        std::string(cond->getStmtClassName()) <<
                                        std::endl;
                                    exit(1);
                                }

                                if (const clang::UnaryOperator *uno = clang::dyn_cast<clang::UnaryOperator>(inc)) {
                                    if (uno->getOpcode() == clang::UO_PostInc ||
                                            uno->getOpcode() == clang::UO_PreInc) {
                                        strideStr = "1";
                                    } else if (uno->getOpcode() == clang::UO_PostDec ||
                                            uno->getOpcode() == clang::UO_PreDec) {
                                        strideStr = "-1";
                                    } else {
                                        std::cerr << "Unsupported unary operator" << std::endl;
                                        exit(1);
                                    }
                                } else {
                                    std::cerr << "Unsupported incr class " <<
                                        std::string(inc->getStmtClassName()) <<
                                        std::endl;
                                    exit(1);
                                }

                                /*
                                 * Insert a one iteration do-loop around the
                                 * original body so that continues have the same
                                 * semantics.
                                 */
                                accumulatedKernelDefs += "    do {\n";
                                accumulatedKernelDefs += bodyStr;
                                accumulatedKernelDefs += "    } while (0);\n";
                                accumulatedKernelDefs += "}\n\n";

                                std::string contextCreation = "\n" + node->getLbl() +
                                    " *ctx = (" + node->getLbl() + " *)malloc(sizeof(" + node->getLbl() + "));\n";
                                for (std::vector<clang::ValueDecl *>::iterator ii =
                                        captures[node->getPragmaLine()]->begin(), ee = captures[node->getPragmaLine()]->end();
                                        ii != ee; ii++) {
                                    clang::ValueDecl *curr = *ii;
                                    contextCreation += "ctx->" +
                                        curr->getNameAsString() + " = " +
                                        curr->getNameAsString() + ";\n";
                                }
                                contextCreation += "hclib_loop_domain_t domain;\n";
                                contextCreation += "domain.low = " + lowStr + ";\n";
                                contextCreation += "domain.high = " + highStr + ";\n";
                                contextCreation += "domain.stride = " + strideStr + ";\n";
                                contextCreation += "domain.tile = 1;\n";
                                contextCreation += "hclib_future_t *fut = hclib_forasync_future((void *)" +
                                    node->getLbl() + ASYNC_SUFFIX + ", ctx, NULL, 1, " +
                                    "&domain, FORASYNC_MODE_RECURSIVE);\n";
                                contextCreation += "hclib_future_wait(fut);\n";
                                contextCreation += "free(ctx);\n";
                                rewriter->ReplaceText(succ->getSourceRange(), contextCreation);
                                // rewriter->InsertTextAfterToken(pred->getLocEnd(),
                                //         contextCreation);
                            } else {
                                std::cerr << "Parallel pragma without a for at line " <<
                                    node->getPragmaLine() << std::endl;
                                exit(1);
                            }
                        } else if (node->getPragma()->getPragmaName() == "simd") {
                            // ignore
                        } else {
                            std::cerr << "Unhandled supported pragma \"" <<
                                node->getPragma()->getPragmaName() << "\"" << std::endl;
                            exit(1);
                        }
                    } else {
                        std::cerr << "Encountered an unknown pragma \"" <<
                            node->getPragma()->getPragmaName() << "\" at line " << node->getPragmaLine() << std::endl;
                        exit(1);
                    }

                    if (std::find(todo->begin(), todo->end(), node->getParent()) == todo->end() && node->getParent()->getLbl() != "root") {
                        todo->push_back(node->getParent());
                    }
                }

                processing = todo;
                todo = new std::vector<OMPNode *>();
            }
        }

        if (fname == "main") {
            accumulatedStructDefs += "typedef struct _main_ctx {\n";
            accumulatedStructDefs += "  int argc;\n";
            accumulatedStructDefs += "  char **argv;\n";
            accumulatedStructDefs += "} main_ctx;\n\n";

            accumulatedKernelDefs += "static int main_entrypoint(void *arg) {\n";
            accumulatedKernelDefs += "    main_ctx *ctx = (main_ctx *)arg;\n";
            accumulatedKernelDefs += "    int argc = ctx->argc;\n";
            accumulatedKernelDefs += "    char **argv = ctx->argv;\n";
            accumulatedKernelDefs += stmtToString(func->getBody());
            accumulatedKernelDefs += "}\n";
        }

        if (accumulatedStructDefs.length() > 0 ||
                accumulatedKernelDefs.length() > 0) {
            rewriter->InsertText(func->getLocStart(),
                    accumulatedStructDefs + accumulatedKernelDefs, true, true);
        }

        successors.clear();
        predecessors.clear();

        if (fname == "main") {
            std::string launchStr = "";
            launchStr += "{ main_ctx *ctx = (main_ctx *)malloc(sizeof(main_ctx));\n";
            launchStr += "ctx->argc = argc;\n";
            launchStr += "ctx->argv = argv;\n";
            launchStr += std::string("hclib_launch(NULL, NULL, "
                "(void (*)(void*))main_entrypoint, ctx);\n");
            launchStr += "free(ctx); return 0; }\n";
            rewriter->ReplaceText(func->getBody()->getSourceRange(), launchStr);
        }
    }
}

bool OMPToHClib::isScopeCreatingStmt(const clang::Stmt *s) {
    return clang::isa<clang::CompoundStmt>(s) || clang::isa<clang::ForStmt>(s);
}

int OMPToHClib::getCurrentLexicalDepth() {
    return in_scope.size();
}

void OMPToHClib::addNewScope() {
    in_scope.push_back(new std::vector<clang::ValueDecl *>());
}

void OMPToHClib::popScope() {
    assert(in_scope.size() >= 1);
    in_scope.pop_back();
}

void OMPToHClib::addToCurrentScope(clang::ValueDecl *d) {
    in_scope[in_scope.size() - 1]->push_back(d);
}

std::vector<clang::ValueDecl *> *OMPToHClib::visibleDecls() {
    std::vector<clang::ValueDecl *> *visible = new std::vector<clang::ValueDecl *>();
    for (std::vector<std::vector<clang::ValueDecl *> *>::iterator i =
            in_scope.begin(), e = in_scope.end(); i != e; i++) {
        std::vector<clang::ValueDecl *> *currentScope = *i;
        for (std::vector<clang::ValueDecl *>::iterator ii = currentScope->begin(),
                ee = currentScope->end(); ii != ee; ii++) {
            visible->push_back(*ii);
        }
    }
    return visible;
}

void OMPToHClib::VisitStmt(const clang::Stmt *s) {
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
                if (calleeName.find("omp_") == 0) {
                    std::cerr << "Found OMP function call to \"" <<  calleeName << "\" on line " << presumedStart.getLine() << std::endl;
                    std::cerr << "Please replace or delete, then retry to the translation" << std::endl;
                    exit(1);
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

        /*
         * This block of code checks if the current statement is either the
         * first before (predecessors) or after (successors) the line that an
         * OMP pragma is on.
         */
        std::vector<OMPPragma> *omp_pragmas =
            getOMPPragmasFor(curr_func_decl, *SM);
        for (std::vector<OMPPragma>::iterator i = omp_pragmas->begin(),
                e = omp_pragmas->end(); i != e; i++) {
            OMPPragma pragma = *i;

            if (presumedEnd.getLine() < pragma.getLine()) {
                if (predecessors.find(pragma.getLine()) ==
                        predecessors.end()) {
                    predecessors[pragma.getLine()] = s;
                    captures[pragma.getLine()] = visibleDecls();
                } else {
                    const clang::Stmt *curr = predecessors[pragma.getLine()];
                    clang::PresumedLoc curr_loc = SM->getPresumedLoc(
                            curr->getLocEnd());
                    if (presumedEnd.getLine() > curr_loc.getLine() ||
                            (presumedEnd.getLine() == curr_loc.getLine() &&
                             presumedEnd.getColumn() > curr_loc.getColumn())) {
                        predecessors[pragma.getLine()] = s;
                        captures[pragma.getLine()] = visibleDecls();
                    }
                }
            }

            if (presumedStart.getLine() >= pragma.getLine()) {
                if (successors.find(pragma.getLine()) == successors.end()) {
                    successors[pragma.getLine()] = s;
                } else {
                    const clang::Stmt *curr = successors[pragma.getLine()];
                    clang::PresumedLoc curr_loc =
                        SM->getPresumedLoc(curr->getLocStart());
                    if (presumedStart.getLine() < curr_loc.getLine() ||
                            (presumedStart.getLine() == curr_loc.getLine() &&
                             presumedStart.getColumn() < curr_loc.getColumn())) {
                        successors[pragma.getLine()] = s;
                    }
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

std::vector<OMPPragma> *OMPToHClib::parseOMPPragmas(const char *ompPragmaFile) {
    std::vector<OMPPragma> *pragmas = new std::vector<OMPPragma>();

    std::ifstream fp;
    fp.open(std::string(ompPragmaFile), std::ios::in);
    if (!fp.is_open()) {
        std::cerr << "Failed opening " << std::string(ompPragmaFile) << std::endl;
        exit(1);
    }

    std::string line;

    while (getline(fp, line)) {
        size_t end = line.find(' ');
        unsigned line_no = atoi(line.substr(0, end).c_str());
        line = line.substr(end + 1);

        end = line.find(' ');
        unsigned last_line = atoi(line.substr(0, end).c_str());
        line = line.substr(end + 1);

        std::string pragma = line;

        line = line.substr(line.find(' ') + 1);
        assert(line.find("omp") == 0);

        line = line.substr(line.find(' ') + 1);

        // Look for a command like parallel, for, etc.
        end = line.find(' ');

        if (end == std::string::npos) {
            // No clauses, just pragma name
            std::string pragma_name = line;

            pragmas->push_back(OMPPragma(line_no, last_line, line,
                        pragma_name));
        } else {
            std::string pragma_name = line.substr(0, end);

            OMPPragma pragma(line_no, last_line, line, pragma_name);
            std::string clauses = line.substr(end + 1);

            std::vector<std::string> split_clauses;
            std::stringstream acc;
            int paren_depth = 0;
            int start = 0;
            int index = 0;

            while (index < clauses.size()) {
                if (clauses[index] == ' ' && paren_depth == 0) {
                    int seek = index;
                    while (seek < clauses.size() && clauses[seek] == ' ') seek++;
                    if (seek < clauses.size() && clauses[seek] == '(') {
                        index = seek;
                    } else {
                        split_clauses.push_back(clauses.substr(start,
                                    index - start));
                        index++;
                        while (index < clauses.size() && clauses[index] == ' ') {
                            index++;
                        }
                        start = index;
                    }
                } else {
                    if (clauses[index] == '(') paren_depth++;
                    else if (clauses[index] == ')') paren_depth--;
                    index++;
                }
            }
            if (index != start) {
                split_clauses.push_back(clauses.substr(start));
            }

            for (std::vector<std::string>::iterator i = split_clauses.begin(),
                    e = split_clauses.end(); i != e; i++) {
                std::string clause = *i;

                if (clause.find("(") == std::string::npos) {
                    pragma.addClause(clause, std::vector<std::string>());
                } else {
                    size_t open = clause.find("(");
                    assert(open != std::string::npos);

                    std::string args = clause.substr(open + 1);

                    size_t close = args.find_last_of(")");
                    assert(close != std::string::npos);

                    args = args.substr(0, close);

                    std::string clause_name = clause.substr(0, open);
                    while (clause_name.at(clause_name.size() - 1) == ' ') {
                        clause_name = clause_name.substr(0, clause_name.size() - 1);
                    }

                    std::vector<std::string> clause_args;
                    int args_index = 0;
                    int args_start = 0;
                    while (args_index < args.size()) {
                        if (args[args_index] == ',') {
                            clause_args.push_back(args.substr(args_start,
                                        args_index - args_start));
                            args_index++;
                            while (args_index < args.size() &&
                                    args[args_index] == ' ') {
                                args_index++;
                            }
                            args_start = args_index;
                        } else {
                            args_index++;
                        }
                    }

                    if (args_start != args_index) {
                        clause_args.push_back(args.substr(args_start));
                    }

                    pragma.addClause(clause_name, clause_args);
                }
            }

            pragmas->push_back(pragma);
        }
    }
    fp.close();

    std::cerr << "Parsed " << pragmas->size() << " pragmas from " <<
        std::string(ompPragmaFile) << std::endl;

    return pragmas;
}

OMPToHClib::OMPToHClib(const char *ompPragmaFile) {
    pragmas = parseOMPPragmas(ompPragmaFile);

    supportedPragmas.insert("parallel");
    supportedPragmas.insert("simd"); // ignore
}

OMPToHClib::~OMPToHClib() {
}
