#ifndef SINGLE_CLAUSE_ARGS
#define SINGLE_CLAUSE_ARGS

#include <vector>
#include <string>

class SingleClauseArgs {
    private:
        std::vector<std::string> args;

    public:
        SingleClauseArgs();
        SingleClauseArgs(std::string args);

        std::string getSingleArg();
        std::vector<std::string> *getArgs();
};

#endif
