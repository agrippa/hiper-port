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

#include "BraceInserter.h"

void BraceInserter::visitChildren(const clang::Stmt *s) {
    for (clang::Stmt::const_child_iterator i = s->child_begin(),
            e = s->child_end(); i != e; i++) {
        const clang::Stmt *child = *i;
        if (child != NULL) {
            VisitStmt(child);
        }
    }
}

std::string BraceInserter::to_string(const clang::Stmt *stmt) {
    return rewriter->getRewrittenText(stmt->getSourceRange());
}


const clang::FunctionDecl *BraceInserter::getNestedFDecl(const clang::Expr *e) {
    if (e->getType().getTypePtr()->isFunctionPointerType()) {
        const clang::ImplicitCastExpr *cast =
            clang::dyn_cast<clang::ImplicitCastExpr>(e);
        if (cast) {
            const clang::DeclRefExpr *ref =
                clang::dyn_cast<clang::DeclRefExpr>(cast->getSubExpr());
            if (ref) {
                const clang::ValueDecl *decl = ref->getDecl();
                if (const clang::FunctionDecl *fdecl =
                        clang::dyn_cast<clang::FunctionDecl>(decl)) {
                    return fdecl;
                }
            }
        }
    }

    return NULL;
}

void BraceInserter::VisitStmt(const clang::Stmt *s) {
    /*
     * Visit children and do call inserts before adding braces around any
     * children
     */
    visitChildren(s);

    // Insert braces around all if and for statement bodies
    switch(s->getStmtClass()) {
        case clang::Stmt::ForStmtClass: {
            const clang::ForStmt *f = clang::dyn_cast<clang::ForStmt>(s);
            assert(f);

            const clang::Stmt *body = f->getBody();
            assert(body != NULL);

            if (!clang::isa<clang::CompoundStmt>(body)) {
                std::string for_str;
                llvm::raw_string_ostream for_stream(for_str);

                // Any of init, cond, or inc may be NULL if not specified in the source
                std::string init_str = (f->getInit() ? to_string(f->getInit()) : "");
                std::string cond_str = (f->getCond() ? to_string(f->getCond()) : "");
                std::string inc_str = (f->getInc() ? to_string(f->getInc()) : "");
                std::string body_str = to_string(f->getBody());

                for_stream << "for (";

                for_stream << init_str;
                if (f->getInit() == NULL ||
                        f->getInit()->getStmtClass() != clang::Stmt::DeclStmtClass) {
                    for_stream << "; ";
                }

                for_stream << cond_str << "; " << inc_str << ") { " <<
                    body_str << "; }";

                for_stream.flush();

                rewriter->ReplaceText(f->getSourceRange(), for_str);
            }
            break;
        }
        case clang::Stmt::IfStmtClass: {
            const clang::IfStmt *f = clang::dyn_cast<clang::IfStmt>(s);
            assert(f);

            assert(f->getThen() != NULL);

            if (!clang::isa<clang::CompoundStmt>(f->getThen()) ||
                    (f->getElse() != NULL &&
                     !clang::isa<clang::CompoundStmt>(f->getElse()))) {
                std::string if_str;
                llvm::raw_string_ostream if_stream(if_str);

                std::string cond_str = to_string(f->getCond());
                std::string then_str = to_string(f->getThen());

                if_stream << "if (" << cond_str << ") {" << then_str << "; }";

                if (f->getElse() != NULL) {
                    if_stream << " else ";
                    if (!clang::isa<clang::IfStmt>(f->getElse())) {
                        if_stream << " {";
                    }

                    std::string else_str = to_string(f->getElse());
                    if_stream << else_str;
                    if (!clang::isa<clang::IfStmt>(f->getElse())) {
                        if_stream << "; } ";
                    }
                }
                if_stream.flush();

                rewriter->ReplaceText(f->getSourceRange(), if_str);
            }
            break;
        }
        case clang::Stmt::BinaryOperatorClass: {
            /*
             * Check for an assignment statement that stores the address of a
             * static function in a variable that may need to be restored on
             * resume (which is unsupported).
             */
            const clang::BinaryOperator *op =
                clang::dyn_cast<clang::BinaryOperator>(s);
            if (op->isAssignmentOp()) {
                clang::Expr *rhs = op->getRHS();
                if (rhs->getType().getTypePtr()->isFunctionPointerType()) {
                    const clang::FunctionDecl *fdecl = getNestedFDecl(rhs);
                    if (fdecl) {
                        /*
                         * An assignment may come directly from another
                         * function, or it may come from another variable or
                         * function parameter that had a function stored in it.
                         * If it comes from something that isn't a function
                         * declaration, we assume that the storage it is coming
                         * from was assigned to somewhere else and this logic
                         * will catch it there.
                         */
                        if (!fdecl->isExternallyVisible()) {
                            std::string fname = fdecl->getName().str();
                            fprintf(stderr, "ERROR: CHIMES does not support "
                                    "restoring function pointers to 'static' "
                                    "functions. Found an assignment from a pointer "
                                    "to \"%s\".\n", fname.c_str());
                            exit(1);
                        }
                    }
                }
            }
            break;
        }
        case clang::Stmt::CallExprClass: {
            /*
             * Check for function pointers being passed to a function call, and
             * verify that none of them point to static function declarations.
             */
            const clang::CallExpr *call = clang::dyn_cast<clang::CallExpr>(s);
            assert(call);
            for (unsigned a = 0; a < call->getNumArgs(); a++) {
                const clang::Expr *arg = call->getArg(a);
                assert(arg);

                if (arg->getType().getTypePtr()->isFunctionPointerType()) {
                    const clang::FunctionDecl *fdecl = getNestedFDecl(arg);
                    if (fdecl) {
                        if (!fdecl->isExternallyVisible()) {
                            std::string fname = fdecl->getName().str();
                            fprintf(stderr, "ERROR: CHIMES does not support "
                                    "restoring function pointers to 'static' "
                                    "functions. Found a function pointer to \"%s\" "
                                    "being passed as an argument on the stack.\n",
                                    fname.c_str());
                            exit(1);
                        }
                    }
                }
            }
        }
        default:
            break;
    }
}
