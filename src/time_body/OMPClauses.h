#ifndef OMP_CLAUSES_H
#define OMP_CLAUSES_H

#include "OMPVarInfo.h"
#include "OMPReductionVar.h"
#include "SingleClauseArgs.h"

#include <map>
#include <vector>

class OMPClauses {
    private:
        std::map<std::string, std::vector<SingleClauseArgs *> *> parsedClauses;
        std::string originalClauses;

        bool computeSingleVarInfo(clang::ValueDecl *decl, bool isGlobal,
                std::vector<OMPVarInfo> *vars);

    public:
        OMPClauses();
        OMPClauses(std::string clauses);

        bool hasClause(std::string clause);
        std::string getSingleArg(std::string clause);
        std::vector<SingleClauseArgs *> *getArgs(std::string clause);
        std::vector<std::string> *getFlattenedArgsList(std::string clause);

        int getNumCollapsedLoops();
        std::vector<OMPReductionVar> *getReductions();
        std::vector<OMPVarInfo> *getVarInfo(
                std::vector<clang::ValueDecl *> *locals);
        std::vector<OMPVarInfo> *getSharedVarInfo(
                std::vector<clang::ValueDecl *> *locals);

        std::string getOriginalClauses();

        void addClauseArg(std::string clause, std::string arg);

        std::map<std::string, std::vector<SingleClauseArgs *> *>::iterator begin();
        std::map<std::string, std::vector<SingleClauseArgs *> *>::iterator end();
};

#endif
