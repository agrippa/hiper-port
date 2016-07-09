#ifndef PARALLEL_REGION_INFO_H
#define PARALLEL_REGION_INFO_H

#include "clang/AST/Decl.h"

#include <vector>

class ParallelRegionInfo {
    private:
        /*
         * Variables referenced directly from the initial parallel region, or
         * globals referenced from called functions. Basically, any variables
         * that must be captured and transferred to the device.
         */
        std::vector<const clang::ValueDecl *> referenced;
        /*
         * Variables declared directly inside of the parallel region. Used to
         * verify that if a variable referenced could not be found in the
         * capture list for the parallel region, it was one declared locally.
         */
        std::vector<const clang::Decl *> declaredInsideParallelRegion;
        /*
         * Functions called from this parallel region (including transitively
         * called functions).
         */
        std::vector<const clang::FunctionDecl *> called;
        /*
         * Functions called from this parallel region that could not be
         * immediately resolved (i.e. they had no bodies defined). Some of these
         * are okay (e.g. compiler intrinsics such as sqrt that are supported on
         * CUDA devices), but some may cause a fatal error in generating a CUDA
         * region as they cannot be resolved at compile time for the
         * source-to-source transformation.
         */
        std::vector<const clang::FunctionDecl *> unresolved;

    public:
        ParallelRegionInfo();

        void addReferencedVar(const clang::ValueDecl *var);
        void addDeclaredVar(const clang::Decl *var);
        void foundUnresolvedFunction(const clang::FunctionDecl *unresolved);
        void addCalledFunction(const clang::FunctionDecl *func);

        bool isDeclaredInsideParallelRegion(const clang::Decl *var);

        std::vector<const clang::FunctionDecl *>::iterator called_begin();
        std::vector<const clang::FunctionDecl *>::iterator called_end();

        std::vector<const clang::FunctionDecl *>::iterator unresolved_begin();
        std::vector<const clang::FunctionDecl *>::iterator unresolved_end();

        std::vector<const clang::ValueDecl *>::iterator referenced_begin();
        std::vector<const clang::ValueDecl *>::iterator referenced_end();
};

#endif
