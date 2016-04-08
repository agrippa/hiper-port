#ifndef PRAGMA_NODE_H
#define PRAGMA_NODE_H

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

#include <vector>

class PragmaNode {
    private:
        // NULL for function root node
        const clang::CallExpr *marker;
        // NULL for pragmas that don't have bodies, e.g. omp taskwait
        const clang::Stmt *body;
        std::vector<clang::ValueDecl *> *captures;
        // NULL for function root node
        PragmaNode *parent;
        std::vector<PragmaNode *> children;
        clang::SourceManager *SM;

        void printHelper(int depth);
        void getLeavesHelper(std::vector<PragmaNode *> *accum);

    public:
        PragmaNode(const clang::CallExpr *setMarker, const clang::Stmt *setBody,
                std::vector<clang::ValueDecl *> *captures,
                clang::SourceManager *SM);

        void addChild(PragmaNode *node);
        std::vector<PragmaNode *> *getLeaves();

        void print();

        const clang::Stmt *getBody();
        const clang::CallExpr *getMarker();
        std::string getPragmaName();
        std::string getPragmaArguments();
        std::string getPragmaCmd();
        std::vector<clang::ValueDecl *> *getCaptures();
        PragmaNode *getParent();
        int nchildren();
        std::vector<PragmaNode *> *getChildren();
        PragmaNode *getParentAccountForFusing();

        void setParent(PragmaNode *parent);

        clang::SourceLocation getStartLoc();
        clang::SourceLocation getEndLoc();

        int getStartLine();
        int getEndLine();

        std::string getLbl();
};

#endif
