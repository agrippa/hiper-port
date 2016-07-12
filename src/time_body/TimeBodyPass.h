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

#include "PragmaNode.h"
#include "SingleClauseArgs.h"
#include "OMPClauses.h"
#include "OMPReductionVar.h"

#define NONE 0
#define FULL_PROGRAM 1
#define PARALLEL_FOR 2

class TimeBodyPass : public clang::ConstStmtVisitor<TimeBodyPass> {
    public:
        TimeBodyPass();
        ~TimeBodyPass();

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
        void setParent(const clang::Stmt *child,
                const clang::Stmt *parent);
        const clang::Stmt *getParent(const clang::Stmt *s);

        void preFunctionVisit(clang::FunctionDecl *func);
        void postFunctionVisit(clang::FunctionDecl *func);

        const clang::Stmt *getBodyFrom(const clang::CallExpr *call,
                std::string lbl);
        const clang::Stmt *getBodyForMarker(const clang::CallExpr *call);
        std::string getPragmaNameForMarker(const clang::CallExpr *call);
        std::string getPragmaArgumentsForMarker(const clang::CallExpr *call);
        OMPClauses *getOMPClausesForMarker(const clang::CallExpr *call);
        std::string getOMPPragmaNameForMarker(const clang::CallExpr *call);

    protected:
        clang::ASTContext *Context;
        clang::SourceManager *SM;
        clang::Rewriter *rewriter;

        std::map<const clang::Stmt *, const clang::Stmt *> parentMap;

    private:
        bool isScopeCreatingStmt(const clang::Stmt *s);
        int getCurrentLexicalDepth();
        void addNewScope();
        void popScope();
        void addToCurrentScope(clang::ValueDecl *d);
        std::vector<clang::ValueDecl *> *visibleDecls();

        /*
         * Map from line containing a OMP pragma to its immediate predessor. It
         * is safe to use a line here because no more than one pragma can appear
         * on each line.
         */
        std::map<int, std::vector<clang::ValueDecl *> *> captures;

        std::vector<std::vector<clang::ValueDecl *> *> in_scope;

        PragmaNode *pragmaTree;

        bool foundOmpToHclibLaunch = false;

        std::set<const clang::DeclRefExpr *> sharedVarsReplaced;
};

#endif
