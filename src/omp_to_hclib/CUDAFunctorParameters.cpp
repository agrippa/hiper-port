#include "CUDAFunctorParameters.h"

CUDAParameter::CUDAParameter(const clang::ValueDecl *set_decl,
        OMPVarInfo set_info) {
    decl = set_decl;
    info = set_info;
}

CUDAParameter::CUDAParameter(const CUDAParameter &other) {
    decl = other.decl;
    info = other.info;
}

const clang::ValueDecl *CUDAParameter::getDecl() {
    return decl;
}

OMPVarInfo CUDAParameter::getInfo() {
    return info;
}

void CUDAFunctorParameters::addParameter(const clang::ValueDecl *decl,
        OMPVarInfo info) {
    parameters.push_back(CUDAParameter(decl, info));
}

std::vector<CUDAParameter>::iterator CUDAFunctorParameters::begin() {
    return parameters.begin();
}

std::vector<CUDAParameter>::iterator CUDAFunctorParameters::end() {
    return parameters.end();
}
