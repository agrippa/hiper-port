#include "OMPPragma.h"

void OMPPragma::addClause(std::string clauseName,
        std::vector<std::string> clauseArguments) {
    if (clauses.find(clauseName) == clauses.end()) {
        clauses[clauseName] = clauseArguments;
    } else {
        for (std::vector<std::string>::iterator i = clauseArguments.begin(),
                e = clauseArguments.end(); i != e; i++) {
            clauses[clauseName].push_back(*i);
        }
    }
}

