#include "OMPDependencies.h"

#include <assert.h>
#include <iostream>

OMPDependency::OMPDependency(std::string setAddrStr, std::string setLengthStr,
        enum DEPENDENCY_TYPE setType) {
    addrStr = setAddrStr;
    lengthStr = setLengthStr;
    type = setType;
}

enum DEPENDENCY_TYPE OMPDependency::getType() {
    return type;
}

std::string OMPDependency::getAddrStr() {
    return addrStr;
}

std::string OMPDependency::getLengthStr() {
    return lengthStr;
}

OMPDependencies::OMPDependencies(std::vector<SingleClauseArgs *> *args) {
    // depend(in: dA[0:T.nb*T.nb], dT[0:ib*T.nb]) depend(inout:dC[0:T.nb*T.nb])
    for (std::vector<SingleClauseArgs *>::iterator i = args->begin(),
            e = args->end(); i != e; i++) {
        SingleClauseArgs *singleClause = *i;
        std::vector<std::string> *clauseArgs = singleClause->getArgs();

        std::string direction = clauseArgs->at(0);

        for (int ii = 1; ii < clauseArgs->size(); ii++) {
            std::string var = clauseArgs->at(ii);

            std::string addrStr, lengthStr;
            size_t openBrace = var.find("[");
            if (openBrace == std::string::npos) {
                // Assume is just a raw pointer
                addrStr = var;
                lengthStr = "0";
            } else {
                size_t closeBrace = var.find("]");
                assert(closeBrace != std::string::npos);
                std::string varname = var.substr(0, openBrace);
                std::string inBraces = var.substr(openBrace + 1,
                        closeBrace - openBrace - 1);

                size_t colon = inBraces.find(":");
                assert(colon != std::string::npos);
                std::string offsetStr = inBraces.substr(0, colon);

                addrStr = "(" + varname + ") + (" + offsetStr + ")";
                lengthStr = inBraces.substr(colon + 1);
            }

            if (direction == "in") {
                dependencies.push_back(OMPDependency(addrStr, lengthStr,
                            DEPENDENCY_TYPE::IN));
            } else if (direction == "out") {
                dependencies.push_back(OMPDependency(addrStr, lengthStr,
                            DEPENDENCY_TYPE::OUT));
            } else if (direction == "inout") {
                dependencies.push_back(OMPDependency(addrStr, lengthStr,
                            DEPENDENCY_TYPE::IN));
                dependencies.push_back(OMPDependency(addrStr, lengthStr,
                            DEPENDENCY_TYPE::OUT));
            } else {
                std::cerr << "Invalid direction \"" << direction <<
                    "\" for variable \"" << var << "\"" << std::endl;
                exit(1);
            }
        }
    }
}

static std::vector<OMPDependency> *getDependenciesOfType(
        std::vector<OMPDependency> *l, enum DEPENDENCY_TYPE type) {
    std::vector<OMPDependency> *acc = new std::vector<OMPDependency>();

    for (std::vector<OMPDependency>::iterator i = l->begin(), e = l->end();
            i != e; i++) {
        OMPDependency curr = *i;
        if (curr.getType() == type) {
            acc->push_back(curr);
        }
    }
    return acc;
}

std::vector<OMPDependency> *OMPDependencies::getInDependencies() {
    return getDependenciesOfType(&dependencies, DEPENDENCY_TYPE::IN);
}

std::vector<OMPDependency> *OMPDependencies::getOutDependencies() {
    return getDependenciesOfType(&dependencies, DEPENDENCY_TYPE::OUT);
}
