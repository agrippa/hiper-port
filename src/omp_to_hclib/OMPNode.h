#ifndef OMP_NODE_H
#define OMP_NODE_H

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

#include "OMPPragma.h"

class OMPNode {
    private:
        std::vector<OMPNode> children;
        OMPNode *parent;
        const clang::Stmt *body;
        int startLine;
        int endLine;
        int pragmaLine;
        OMPPragma *pragma;
        std::string lbl;

        void getLeavesHelper(std::vector<OMPNode *> *accum);

    public:
        OMPNode(const clang::Stmt *setBody, int setPragmaLine,
                OMPPragma *setPragma, OMPNode *setParent, std::string setLbl,
                clang::SourceManager *SM);

        void addChild(const clang::Stmt *stmt, int pragmaLine,
                OMPPragma *pragma, std::string lbl, clang::SourceManager *SM);

        int getStartLine();
        int getEndLine();
        int getPragmaLine();
        OMPPragma *getPragma();
        void print();
        void printHelper(int depth);
        int nchildren();
        std::vector<OMPNode *> *getLeaves();
        std::string getLbl();
        OMPNode *getParent();
        std::vector<OMPNode> *getChildren();
        const clang::Stmt *getBody();
};

#endif
