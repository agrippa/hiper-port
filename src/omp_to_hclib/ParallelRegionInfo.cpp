#include "ParallelRegionInfo.h"

ParallelRegionInfo::ParallelRegionInfo() {
}

void ParallelRegionInfo::addReferencedVar(const clang::ValueDecl *var) {
    referenced.push_back(var);
}

void ParallelRegionInfo::foundUnresolvedFunction(
        const clang::FunctionDecl *setUnresolved) {
    unresolved.push_back(setUnresolved);
}

void ParallelRegionInfo::addCalledFunction(const clang::FunctionDecl *func) {
    called.push_back(func);
}

std::vector<const clang::FunctionDecl *>::iterator ParallelRegionInfo::called_begin() {
    return called.begin();
}

std::vector<const clang::FunctionDecl *>::iterator ParallelRegionInfo::called_end() {
    return called.end();
}

std::vector<const clang::FunctionDecl *>::iterator ParallelRegionInfo::unresolved_begin() {
    return unresolved.begin();
}

std::vector<const clang::FunctionDecl *>::iterator ParallelRegionInfo::unresolved_end() {
    return unresolved.end();
}

