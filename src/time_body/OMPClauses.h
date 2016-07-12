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
        std::vector<std::string> *getFlattenedArgsList(std::string clause);

        int getNumCollapsedLoops();

        void addClauseArg(std::string clause, std::string arg);

        std::map<std::string, std::vector<SingleClauseArgs *> *>::iterator begin();
        std::map<std::string, std::vector<SingleClauseArgs *> *>::iterator end();
};

#endif
