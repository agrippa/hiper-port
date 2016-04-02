#include "SingleClauseArgs.h"

#include <assert.h>

int skipWhiteSpace(std::string s, int index) {
    while (index < s.size() && s[index] == ' ') {
        index++;
    }
    return index;
}

int seekPastToken(std::string s, int index) {
    while (index < s.size() && s[index] != ',' &&
            s[index] != ' ' && s[index] != ':') {
        index++;
    }
    return index;
}

SingleClauseArgs::SingleClauseArgs(std::string argsStr) {
    int index = skipWhiteSpace(argsStr, 0);

    while (index < argsStr.size()) {
        int start = index;
        int end = seekPastToken(argsStr, index);
        std::string token = argsStr.substr(start, end - start);
        args.push_back(token);

        const int delimiterIndex = skipWhiteSpace(argsStr, end);
        assert(delimiterIndex == argsStr.size() ||
                argsStr[delimiterIndex] == ',' ||
                argsStr[delimiterIndex] == ':');
        index = skipWhiteSpace(argsStr, delimiterIndex + 1);
    }
}

SingleClauseArgs::SingleClauseArgs() {
}


std::string SingleClauseArgs::getSingleArg() {
    assert(args.size() == 1);
    return args.at(0);
}

std::vector<std::string> *SingleClauseArgs::getArgs() {
    return &args;
}
