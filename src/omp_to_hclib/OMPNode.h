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
        const clang::Stmt *body;
        int startLine;
        int endLine;
        int pragmaLine;
        OMPPragma *pragma;

    public:
        OMPNode(const clang::Stmt *setBody, int setPragmaLine, OMPPragma *setPragma,
                clang::SourceManager *SM);

        void addChild(const clang::Stmt *stmt, int pragmaLine, OMPPragma *pragma,
                clang::SourceManager *SM);

        int getStartLine();
        int getEndLine();
        int getPragmaLine();
        OMPPragma *getPragma();
        void print();
        void printHelper(int depth);
};

#endif
