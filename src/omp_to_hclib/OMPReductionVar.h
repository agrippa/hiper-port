#ifndef OMP_REDUCTION_VAR_H
#define OMP_REDUCTION_VAR_H

#include "clang/AST/Decl.h"
#include <string>

class OMPReductionVar {
    private:
        std::string op;
        std::string var;

    public:
        OMPReductionVar(std::string op, std::string var);

        std::string getOp();
        std::string getVar();

        std::string getInitialValue();
        std::string getReductionFunc();
};

#endif
