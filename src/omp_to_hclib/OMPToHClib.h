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
#include "ParallelRegionInfo.h"
#include "CUDAFunctorParameters.h"

class OMPToHClib : public clang::ConstStmtVisitor<OMPToHClib> {
    public:
        OMPToHClib(const char *checkForPthread,
                const char *startCriticalSectionId);
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
        std::string stmtToStringWithSharedVars(const clang::Stmt *stmt,
                std::vector<OMPVarInfo> *shared);
        void replaceAllReferencesTo(const clang::Stmt *stmt,
                std::vector<OMPVarInfo> *shared);
        std::string stringForAST(const clang::Stmt *stmt);
        void setParent(const clang::Stmt *child,
                const clang::Stmt *parent);
        const clang::Stmt *getParent(const clang::Stmt *s);

        void preFunctionVisit(clang::FunctionDecl *func);
        void postFunctionVisit(clang::FunctionDecl *func);

        std::string getClosureDecl(std::string closureName,
                bool isForasyncClosure, int forasyncDim,
                bool isFuture, bool &isAcceleratable,
                std::string iterator = "", std::string bodyStr = "",
                const clang::Stmt *body = NULL,
                std::vector<clang::ValueDecl *> *captured = NULL,
                OMPClauses *clauses = NULL,
                CUDAFunctorParameters *functor_params = NULL);
        void traverseFunctorBody(const clang::Stmt *curr,
                ParallelRegionInfo &info, bool beneathFunctionCall);
        std::string getCUDAFunctorDef(std::string closureName,
                std::vector<clang::ValueDecl *> *captured, OMPClauses *clauses,
                std::string iterator, std::string bodyStr,
                const clang::Stmt *body, CUDAFunctorParameters *functor_params);
        bool handleCapturedPointer(const clang::PointerType *pointer,
                const clang::ValueDecl *ref, OMPVarInfo &foundVarInfo,
                const bool isShared, std::stringstream &constructor_sig,
                std::stringstream &constructor_body, std::stringstream &ss,
                CUDAFunctorParameters *functor_params,
                std::vector<OMPVarInfo> *toBeTransferred);
        std::string getClosureDef(std::string closureName,
                bool isForasyncClosure, bool isAsyncClosure,
                std::string contextName,
                std::vector<clang::ValueDecl *> *captured, std::string bodyStr,
                bool isFuture, OMPClauses *clauses,
                bool wrapBodyInFinish, bool waitAtEnd,
                bool isAcceleratable, std::vector<const clang::ValueDecl *> *condVars = NULL);
        std::string getStructDef(std::string structName,
                std::vector<clang::ValueDecl *> *captured, OMPClauses *clauses);
        std::string getContextSetup(PragmaNode *node, std::string structName,
                std::vector<clang::ValueDecl *> *captured, OMPClauses *clauses);

        enum CAPTURE_TYPE getParentCaptureType(PragmaNode *curr,
                std::string varname);

        const clang::Stmt *getBodyFrom(const clang::CallExpr *call,
                std::string lbl);
        const clang::Stmt *getBodyForMarker(const clang::CallExpr *call);
        std::string getPragmaNameForMarker(const clang::CallExpr *call);
        std::string getPragmaArgumentsForMarker(const clang::CallExpr *call);
        OMPClauses *getOMPClausesForMarker(const clang::CallExpr *call);
        std::string getOMPPragmaNameForMarker(const clang::CallExpr *call);

        int getCriticalSectionId() { return criticalSectionId; }
        bool hasShmemCalls() { return anyShmemCalls; }

    protected:
        clang::ASTContext *Context;
        clang::SourceManager *SM;
        clang::Rewriter *rewriter;

        std::map<const clang::Stmt *, const clang::Stmt *> parentMap;

    private:
        std::string getDeclarationTypeStr(clang::QualType qualType,
                std::string name, std::string soFarBefore,
                std::string soFarAfter);
        std::string getDeclarationStr(clang::ValueDecl *decl);
        std::string getCaptureStr(clang::ValueDecl *decl);
        std::string getUnpackStr(clang::ValueDecl *decl);
        std::string getArraySizeExpr(clang::QualType qualType);

        bool canLaunchTasks(const clang::Stmt *body);

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

        clang::Expr *unwrapCasts(clang::Expr *expr);

        std::string getCriticalSectionLockStr(int criticalSectionId);
        std::string getCriticalSectionUnlockStr(int criticalSectionId);

        /*
         * Map from line containing a OMP pragma to its immediate predessor. It
         * is safe to use a line here because no more than one pragma can appear
         * on each line.
         */
        std::map<int, std::vector<clang::ValueDecl *> *> captures;

        std::vector<std::vector<clang::ValueDecl *> *> in_scope;

        bool checkForPthread;

        PragmaNode *pragmaTree;

        int criticalSectionId;

        bool anyShmemCalls = false;

        bool foundOmpToHclibLaunch = false;

        std::vector<std::string> compatiblePthreadAPIs;

        std::set<const clang::DeclRefExpr *> sharedVarsReplaced;
};

#endif
