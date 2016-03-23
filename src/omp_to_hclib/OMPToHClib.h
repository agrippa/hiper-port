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

#include "OMPPragma.h"
#include "OMPNode.h"

class OMPToHClib : public clang::ConstStmtVisitor<OMPToHClib> {
    public:
        OMPToHClib(const char *ompPragmaFile, const char *ompToHclibPragmaFile);
        ~OMPToHClib();

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

        void preFunctionVisit(clang::FunctionDecl *func);
        void postFunctionVisit(clang::FunctionDecl *func);

        bool hasLaunchBody();
        std::string getLaunchBody();
        std::vector<clang::ValueDecl *> *getLaunchCaptures();
        const clang::FunctionDecl *getFunctionContainingLaunch();
        clang::SourceLocation getLaunchBodyBeginLoc();
        clang::SourceLocation getLaunchBodyEndLoc();

        std::string getClosureDef(std::string closureName, bool isForasyncClosure,
                std::string contextName, std::vector<clang::ValueDecl *> *captured,
                std::string bodyStr,
                std::vector<OMPReductionVar> *reductions = NULL,
                const clang::ValueDecl *condVar = NULL);
        std::string getStructDef(std::string structName,
                std::vector<clang::ValueDecl *> *captured);
        std::string getContextSetup(std::string structName,
                std::vector<clang::ValueDecl *> *captured,
                std::vector<OMPReductionVar> *reductions);

    protected:
        clang::ASTContext *Context;
        clang::SourceManager *SM;
        clang::Rewriter *rewriter;

        std::map<const clang::Stmt *, const clang::Stmt *> parentMap;

    private:
        std::vector<OMPPragma> *parseOMPPragmas(const char *ompPragmaFile);
        void parseHClibPragmas(const char *filename);
        std::vector<OMPPragma> *getOMPPragmasFor(
                clang::FunctionDecl *decl, clang::SourceManager &SM);
        OMPPragma *getOMPPragmaFor(int lineNo);

        std::string getDeclarationTypeStr(clang::QualType qualType,
                std::string name, std::string soFarBefore, std::string soFarAfter);
        std::string getDeclarationStr(clang::ValueDecl *decl);
        std::string getCaptureStr(clang::ValueDecl *decl);
        std::string getUnpackStr(clang::ValueDecl *decl);
        std::string getArraySizeExpr(clang::QualType qualType);

        bool isScopeCreatingStmt(const clang::Stmt *s);
        int getCurrentLexicalDepth();
        void addNewScope();
        void popScope();
        void addToCurrentScope(clang::ValueDecl *d);
        std::vector<clang::ValueDecl *> *visibleDecls();

        std::string getCondVarAndLowerBoundFromInit(const clang::Stmt *init,
                const clang::ValueDecl **condVar);
        std::string getUpperBoundFromCond(const clang::Stmt *cond,
                const clang::ValueDecl *condVar);
        std::string getStrideFromIncr(const clang::Stmt *inc,
                const clang::ValueDecl *condVar);

        std::string getStructDef(OMPNode *node);

        clang::Expr *unwrapCasts(clang::Expr *expr);

        std::vector<OMPPragma> *pragmas;

        /*
         * Map from line containing a OMP pragma to its immediate predessor. It is
         * safe to use a line here because no more than one pragma can appear on
         * each line.
         */
        std::map<int, const clang::Stmt *> predecessors;
        std::map<int, const clang::Stmt *> successors;
        std::map<int, std::vector<clang::ValueDecl *> *> captures;

        std::set<std::string> supportedPragmas;

        std::vector<std::vector<clang::ValueDecl *> *> in_scope;

        int launchStartLine;
        int launchEndLine;
        const clang::Stmt *firstInsideLaunch;
        const clang::Stmt *lastInsideLaunch;
        std::vector<clang::ValueDecl *> *launchCaptures;
        const clang::FunctionDecl *functionContainingLaunch;
};

#endif
