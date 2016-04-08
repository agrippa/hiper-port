#include "SingleClauseArgs.h"

#include <iostream>
#include <assert.h>
#include <sstream>

int skipWhiteSpace(std::string s, int index) {
    while (index < s.size() && s[index] == ' ') {
        index++;
    }
    return index;
}

int seekPastToken(std::string s, int index) {
    int braceDepth = 0;
    while (index < s.size() && (braceDepth > 0 || (s[index] != ',' &&
            s[index] != ' ' && s[index] != ':'))) {
        if (s[index] == '[') {
            braceDepth++;
        } else if (s[index] == ']') {
            braceDepth--;
        }
        index++;
    }
    return index;
}

SingleClauseArgs::SingleClauseArgs(std::string setClause, std::string argsStr) {
    clause = setClause;

    int index = skipWhiteSpace(argsStr, 0);

    while (index < argsStr.size()) {
        int start = index;
        int end = seekPastToken(argsStr, index);
        std::string token = argsStr.substr(start, end - start);
        args.push_back(token);

        const int delimiterIndex = skipWhiteSpace(argsStr, end);
        assert(delimiterIndex == argsStr.size() ||
                argsStr.at(delimiterIndex) == ',' ||
                argsStr.at(delimiterIndex) == ':');
        index = skipWhiteSpace(argsStr, delimiterIndex + 1);
    }
}

SingleClauseArgs::SingleClauseArgs(std::string setClause) {
    clause = setClause;
}

std::string SingleClauseArgs::getSingleArg() {
    if (args.size() != 1) {
        std::stringstream ss;
        ss << "Expected single arg to \"" << clause << "\" but got " <<
            args.size() << " args:";
        for (std::vector<std::string>::iterator i = args.begin(),
                e = args.end(); i != e; i++) {
            ss << " " << *i;
        }
        std::cerr << ss.str() << std::endl;
        exit(1);
    }
    return args.at(0);
}

std::vector<std::string> *SingleClauseArgs::getArgs() {
    return &args;
}

std::string SingleClauseArgs::str() {
    std::stringstream ss;
    ss << "[";
    for (std::vector<std::string>::iterator i = args.begin(), e = args.end();
            i != e; i++) {
        ss << " \"" << *i << "\"";
    }
    ss << " ]";
    return ss.str();
}
