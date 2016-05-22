#ifndef OMP_TO_HCLIB_H
#define OMP_TO_HCLIB_H

#include <fstream>
#include <set>

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

class MeasureLoadBalancePass : public clang::ConstStmtVisitor<MeasureLoadBalancePass> {
    public:
        MeasureLoadBalancePass() { }
        ~MeasureLoadBalancePass() { }

        void setRewriter(clang::Rewriter &R) {
            rewriter = &R;
            SM = &R.getSourceMgr();
        }
        void setContext(clang::ASTContext &setContext) {
            Context = &setContext;
        }

        void visitChildren(const clang::Stmt *s, bool firstTraversal = true);
        void VisitStmt(const clang::Stmt *s);
        std::string stmtToString(const clang::Stmt* s);
        std::string stringForAST(const clang::Stmt *stmt);
        void setParent(const clang::Stmt *child,
                const clang::Stmt *parent);
        const clang::Stmt *getParent(const clang::Stmt *s);

        void postFunctionVisit(clang::FunctionDecl *func);

    protected:
        clang::ASTContext *Context;
        clang::SourceManager *SM;
        clang::Rewriter *rewriter;

        std::map<const clang::Stmt *, const clang::Stmt *> parentMap;

    private:
        const clang::Stmt *getBodyForMarker(const clang::CallExpr *call);
        std::string getPragmaNameForMarker(const clang::CallExpr *call);
        std::string getPragmaArgumentsForMarker(const clang::CallExpr *call);
        const clang::Stmt *getBodyFrom(const clang::CallExpr *call,
                std::string lbl);
        bool isParallelFor(std::string pragmaArgs);
        std::string getOMPCommandForArgs(std::string args);

        bool foundOmpToHclibLaunch = false;
        /*
         * For simplicity, handle one pragma per invocation. Extremely
         * inefficient, but ensures no changes overlap.
         */
        bool handledAPragma = false;
};

#endif
