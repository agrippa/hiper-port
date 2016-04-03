#ifndef OMP_DEPENDENCIES_H
#define OMP_DEPENDENCIES_H

#include "SingleClauseArgs.h"

#include <string>
#include <vector>

enum DEPENDENCY_TYPE {
    IN = 0, OUT
};

class OMPDependency {
    private:
        enum DEPENDENCY_TYPE type;
        std::string addrStr;
        std::string lengthStr;

    public:
        OMPDependency(std::string addrStr, std::string lengthStr,
                enum DEPENDENCY_TYPE type);

        enum DEPENDENCY_TYPE getType();
        std::string getAddrStr();
        std::string getLengthStr();
};

class OMPDependencies {
    private:
        std::vector<OMPDependency> dependencies;

    public:
        OMPDependencies(std::vector<SingleClauseArgs *> *args);

        std::vector<OMPDependency> *getInDependencies();
        std::vector<OMPDependency> *getOutDependencies();
};

#endif
