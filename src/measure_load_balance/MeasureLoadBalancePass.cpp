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

#include "MeasureLoadBalancePass.h"

void MeasureLoadBalancePass::setParent(const clang::Stmt *child,
        const clang::Stmt *parent) {
    assert(parentMap.find(child) == parentMap.end());
    parentMap[child] = parent;
}

const clang::Stmt *MeasureLoadBalancePass::getParent(const clang::Stmt *s) {
    assert(parentMap.find(s) != parentMap.end());
    return parentMap.at(s);
}

std::string MeasureLoadBalancePass::stmtToString(const clang::Stmt *stmt) {
    // std::string s;
    // llvm::raw_string_ostream stream(s);
    // stmt->printPretty(stream, NULL, Context->getPrintingPolicy());
    // stream.flush();
    // return s;
    return rewriter->getRewrittenText(stmt->getSourceRange());
}

std::string MeasureLoadBalancePass::stringForAST(const clang::Stmt *stmt) {
    std::string s;
    llvm::raw_string_ostream stream(s);
    stmt->printPretty(stream, NULL, Context->getPrintingPolicy());
    stream.flush();
    return s;
}

void MeasureLoadBalancePass::visitChildren(const clang::Stmt *s, bool firstTraversal) {
    for (clang::Stmt::const_child_iterator i = s->child_begin(),
            e = s->child_end(); i != e; i++) {
        const clang::Stmt *child = *i;
        if (child != NULL) {
            if (firstTraversal) {
                setParent(child, s);
            }
            if (!handledAPragma) {
                VisitStmt(child);
            }
        }
    }
}

std::string MeasureLoadBalancePass::getPragmaNameForMarker(const clang::CallExpr *call) {
    assert(call);

    assert(call->getNumArgs() == 3);
    const clang::Expr *pragmaNameArg = call->getArg(0);
    while (clang::isa<clang::ImplicitCastExpr>(pragmaNameArg)) {
        pragmaNameArg = clang::dyn_cast<clang::ImplicitCastExpr>(pragmaNameArg)->getSubExpr();
    }
    const clang::StringLiteral *literal = clang::dyn_cast<clang::StringLiteral>(pragmaNameArg);
    assert(literal);
    std::string pragmaName = literal->getString().str();
    return pragmaName;
}

std::string MeasureLoadBalancePass::getPragmaArgumentsForMarker(const clang::CallExpr *call) {
    assert(call->getNumArgs() == 3);
    const clang::Expr *pragmaArgsArg = call->getArg(1);
    while (clang::isa<clang::ImplicitCastExpr>(pragmaArgsArg)) {
        pragmaArgsArg = clang::dyn_cast<clang::ImplicitCastExpr>(pragmaArgsArg)->getSubExpr();
    }
    const clang::StringLiteral *literal = clang::dyn_cast<clang::StringLiteral>(pragmaArgsArg);
    assert(literal);
    return literal->getString().str();
}

const clang::Stmt *MeasureLoadBalancePass::getBodyFrom(
        const clang::CallExpr *call, std::string lbl) {
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
                assert(i < e);
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

    std::cerr << "body=" << body << " call=" << call << " lbl=" << lbl << std::endl;

    assert(body);
    return body;
}

const clang::Stmt *MeasureLoadBalancePass::getBodyForMarker(const clang::CallExpr *call) {
    std::string pragmaName = getPragmaNameForMarker(call);
    std::string pragmaArgs = getPragmaArgumentsForMarker(call);

    return getBodyFrom(call, pragmaName);
}

bool MeasureLoadBalancePass::isParallelFor(std::string pragmaArgs) {
    int index = 0;
    while (index < pragmaArgs.size()) {
        if (pragmaArgs[index] == ' ') {
            index++;
        } else {
            int paren_depth = 0;
            const int start_token = index;

            while (index < pragmaArgs.size() && (paren_depth > 0 || pragmaArgs[index] != ' ')) {
                if (pragmaArgs[index] == '(') {
                    paren_depth++;
                } else if (pragmaArgs[index] == ')') {
                    paren_depth--;
                }
                index++;
            }

            std::string token = pragmaArgs.substr(start_token, index - start_token);
            if (token == "for") {
                return true;
            }
        }
    }

    return false;
}

std::string MeasureLoadBalancePass::getOMPCommandForArgs(std::string args) {
    int index = 0;
    while (index < args.length() && args[index] == ' ') {
        index++;
    }
    assert(index != args.length());

    const int start = index;
    while (index < args.length() && args[index] != ' ') {
        index++;
    }
    return args.substr(start, index - start);
}

void MeasureLoadBalancePass::VisitStmt(const clang::Stmt *s) {
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
                bool abort = false;

                if (calleeName == "hclib_pragma_marker") {
                    std::string pragmaName = getPragmaNameForMarker(call);
                    std::string pragmaArgs = getPragmaArgumentsForMarker(call);

                    if (pragmaName != "omp_to_hclib") {
                        std::string ompCmd = getOMPCommandForArgs(pragmaArgs);
                        std::stringstream ss;

                        if (ompCmd == "parallel" && isParallelFor(pragmaArgs)) {
                            const clang::Stmt *body = getBodyForMarker(call);
                            const clang::ForStmt *f = clang::dyn_cast<clang::ForStmt>(body);
                            assert(f);
                            const clang::Stmt *forBody = f->getBody();
                            std::string forBodyStr = stmtToString(forBody);
                            ss << "{ ____num_tasks[omp_get_thread_num()]++;\n";
                            ss << forBodyStr;
                            ss << " ; }\n";

                            bool failed = rewriter->ReplaceText(
                                    clang::SourceRange(forBody->getLocStart(),
                                        forBody->getLocEnd()), ss.str());
                            assert(!failed);
                        } else if (ompCmd == "task") {
                            const clang::Stmt *body = getBodyForMarker(call);
                            std::string launchBody = stmtToString(body);
                            ss << "{ ____num_tasks[omp_get_thread_num()]++;\n";
                            ss << launchBody;
                            ss << " ; }\n";

                            bool failed = rewriter->ReplaceText(
                                    clang::SourceRange(body->getLocStart(),
                                        body->getLocEnd()), ss.str());
                            assert(!failed);
                        } else if (ompCmd == "taskwait" ||
                                ompCmd == "parallel" || ompCmd == "single" ||
                                ompCmd == "master" || ompCmd == "critical" ||
                                ompCmd == "atomic" || ompCmd == "simd") {
                            // Ignore
                            std::cerr << "Ignoring \"" << ompCmd << "\"" <<
                                std::endl;
                        } else {
                            std::cerr << "Unhandled OMP command \"" << ompCmd << "\"" << std::endl;
                            exit(1);
                        }

                        std::string pragma = "#pragma " + pragmaName + " " +
                            pragmaArgs + "\n";
                        bool failed = rewriter->ReplaceText(
                                clang::SourceRange(call->getLocStart(),
                                    call->getLocEnd()), pragma);
                        assert(!failed);

                        handledAPragma = true;
                    } else {
                        const clang::Stmt *body = getBodyForMarker(call);
                        assert(body);
                        std::string bodyStr = stmtToString(body);
                        std::stringstream ss;
                        ss << bodyStr << " ; {\n";
                        ss << "    int __i;\n";
                        ss << "    assert(omp_get_max_threads() <= 32);\n";
                        ss << "    for (__i = 0; __i < omp_get_max_threads(); __i++) {\n";
                        ss << "        fprintf(stderr, \"Thread %d: %d\\n\", __i, ____num_tasks[__i]);\n";
                        ss << "    }\n";
                        ss << "}\n";

                        bool failed = rewriter->ReplaceText(
                                clang::SourceRange(call->getLocStart(),
                                    body->getLocEnd()), ss.str());
                        assert(!failed);
                        handledAPragma = true;
                    }
                }
            }
        }
    }

    visitChildren(s);
}
