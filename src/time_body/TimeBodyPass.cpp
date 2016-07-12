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

#include "OMPClauses.h"
#include "TimeBodyPass.h"

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

std::string TimeBodyPass::stringForAST(const clang::Stmt *stmt) {
    std::string s;
    llvm::raw_string_ostream stream(s);
    stmt->printPretty(stream, NULL, Context->getPrintingPolicy());
    stream.flush();
    return s;
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

std::string TimeBodyPass::getPragmaNameForMarker(const clang::CallExpr *call) {
    assert(call);

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

    return getBodyFrom(call, pragmaName);
}

void TimeBodyPass::VisitStmt(const clang::Stmt *s) {
    clang::SourceLocation start = s->getLocStart();
    clang::SourceLocation end = s->getLocEnd();

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
                    bool handled = false;

                    const clang::Stmt *body = getBodyForMarker(call);
                    std::string pragma_type = getPragmaNameForMarker(call);
                    std::string pragma_cmd = getPragmaArgumentsForMarker(call);

                    if (pragma_type == "omp" && body) {
                        size_t ompPragmaNameEnd = pragma_cmd.find(' ');

                        std::string ompPragma, ompClauses;
                        if (ompPragmaNameEnd == std::string::npos) {
                            // No clauses
                            ompPragma = pragma_cmd;
                            ompClauses = "";
                        } else {
                            ompPragma = pragma_cmd.substr(0, ompPragmaNameEnd);
                            ompClauses = pragma_cmd.substr(ompPragmaNameEnd + 1);
                        }

                        OMPClauses clauses(ompClauses);

                        if (ompPragma == "parallel" &&
                                clauses.hasClause("for")) {
                            clang::SourceLocation start = call->getLocStart();
                            clang::PresumedLoc presumedStart =
                                SM->getPresumedLoc(start);
                            int startLine = presumedStart.getLine();

                            std::stringstream lbl;
                            lbl << "pragma" << startLine << "_omp_parallel" <<
                                std::flush;

                            std::string launchBody = stmtToString(body);

                            std::stringstream ss;
                            ss << "unsigned long long ____hclib_start_time = hclib_current_time_ns();" << std::endl;
                            ss << "#pragma omp parallel " << ompClauses << std::endl;
                            ss << launchBody;
                            ss << " ; unsigned long long ____hclib_end_time = hclib_current_time_ns(); ";
                            ss << "printf(\"" << lbl.str() <<
                                " \%llu ns\\n\", ____hclib_end_time - " <<
                                "____hclib_start_time);";
                            bool failed = rewriter->ReplaceText(
                                    clang::SourceRange(start,
                                        body->getLocEnd()), ss.str());
                            assert(!failed);

                            handled = true;
                        }
                    }
                }
            }
        }
    }

    visitChildren(s);
}
