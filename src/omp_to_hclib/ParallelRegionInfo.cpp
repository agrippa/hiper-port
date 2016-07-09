#include "ParallelRegionInfo.h"

ParallelRegionInfo::ParallelRegionInfo() {
}

void ParallelRegionInfo::addReferencedVar(const clang::ValueDecl *var) {
    if (std::find(referenced.begin(), referenced.end(), var) == referenced.end()) {
        referenced.push_back(var);
    }
}

void ParallelRegionInfo::foundUnresolvedFunction(
        const clang::FunctionDecl *setUnresolved) {
    unresolved.push_back(setUnresolved);
}

void ParallelRegionInfo::addCalledFunction(const clang::FunctionDecl *func) {
    called.push_back(func);
}

void ParallelRegionInfo::addDeclaredVar(const clang::Decl *var) {
    declaredInsideParallelRegion.push_back(var);
}

bool ParallelRegionInfo::isDeclaredInsideParallelRegion(
        const clang::Decl *var) {
    return (std::find(declaredInsideParallelRegion.begin(),
                declaredInsideParallelRegion.end(), var) !=
            declaredInsideParallelRegion.end());
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

std::vector<const clang::ValueDecl *>::iterator ParallelRegionInfo::referenced_begin() {
    return referenced.begin();
}

std::vector<const clang::ValueDecl *>::iterator ParallelRegionInfo::referenced_end() {
    return referenced.end();
}
