#ifndef PARALLEL_REGION_INFO_H
#define PARALLEL_REGION_INFO_H

#include "clang/AST/Decl.h"

#include <vector>

class ParallelRegionInfo {
    private:
        std::vector<const clang::ValueDecl *> referenced;
        std::vector<const clang::FunctionDecl *> called;
        std::vector<const clang::FunctionDecl *> unresolved;

    public:
        ParallelRegionInfo();

        void addReferencedVar(const clang::ValueDecl *var);
        void foundUnresolvedFunction(const clang::FunctionDecl *unresolved);
        void addCalledFunction(const clang::FunctionDecl *func);

        std::vector<const clang::FunctionDecl *>::iterator called_begin();
        std::vector<const clang::FunctionDecl *>::iterator called_end();

        std::vector<const clang::FunctionDecl *>::iterator unresolved_begin();
        std::vector<const clang::FunctionDecl *>::iterator unresolved_end();
};

#endif
