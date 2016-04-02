#ifndef OMP_CLAUSES_H
#define OMP_CLAUSES_H

#include "SingleClauseArgs.h"

#include <map>
#include <vector>

class OMPClauses {
    private:
        std::map<std::string, std::vector<SingleClauseArgs *> *> parsedClauses;

    public:
        OMPClauses();
        OMPClauses(std::string clauses);

        bool hasClause(std::string clause);
        std::string getSingleArg(std::string clause);
        std::vector<SingleClauseArgs *> *getArgs(std::string clause);
};

#endif
