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
        OMPToHClib(const char *ompPragmaFile, const char *structFilename);
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
        void setParent(const clang::Stmt *child,
                const clang::Stmt *parent);

        void preFunctionVisit(clang::FunctionDecl *func);
        void postFunctionVisit(clang::FunctionDecl *func);

    protected:
        clang::ASTContext *Context;
        clang::SourceManager *SM;
        clang::Rewriter *rewriter;

        std::map<const clang::Stmt *, const clang::Stmt *> parentMap;

    private:
        std::vector<OMPPragma> *parseOMPPragmas(const char *ompPragmaFile);
        std::vector<OMPPragma> *getOMPPragmasFor(
                clang::FunctionDecl *decl, clang::SourceManager &SM);
        OMPPragma *getOMPPragmaFor(int lineNo);

        bool isScopeCreatingStmt(const clang::Stmt *s);
        int getCurrentLexicalDepth();
        void addNewScope();
        void popScope();
        void addToCurrentScope(clang::ValueDecl *d);
        std::vector<clang::ValueDecl *> *visibleDecls();

        std::string getStructDef(OMPNode *node);

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

        std::ofstream structFile;
};

#endif
