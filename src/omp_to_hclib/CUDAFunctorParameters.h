#ifndef CUDA_FUNCTOR_PARAMETERS_H
#define CUDA_FUNCTOR_PARAMETERS_H

#include "clang/AST/Decl.h"

#include "OMPVarInfo.h"

#include <vector>

class CUDAParameter {
    private:
        const clang::ValueDecl *decl;
        OMPVarInfo info;

    public:
        CUDAParameter(const clang::ValueDecl *set_decl, OMPVarInfo set_info);
        CUDAParameter(const CUDAParameter &other);

        const clang::ValueDecl *getDecl();
        OMPVarInfo getInfo();
};

class CUDAFunctorParameters {
    private:
        std::vector<CUDAParameter> parameters;

    public:
        void addParameter(const clang::ValueDecl *decl, OMPVarInfo info);

        std::vector<CUDAParameter>::iterator begin();
        std::vector<CUDAParameter>::iterator end();
};

#endif
