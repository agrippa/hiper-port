#ifndef SINGLE_CLAUSE_ARGS
#define SINGLE_CLAUSE_ARGS

#include <vector>
#include <string>

class SingleClauseArgs {
    private:
        std::string clause;
        std::vector<std::string> args;

    public:
        SingleClauseArgs(std::string clause);
        SingleClauseArgs(std::string clause, std::string args);

        std::string getSingleArg();
        std::vector<std::string> *getArgs();
        std::string str();
};

#endif
