#include <assert.h>

#include "OMPPragma.h"

void OMPPragma::addClause(std::string clauseName,
        std::vector<std::string> clauseArguments) {
    // Check support
    bool supportedClause = false;
    if (pragmaName == "parallel") {
        if (clauseName == "private") {
            /*
             * For now ignore private, though we could
             * use it in the future to improve the
             * capture list.
             */
            supportedClause = true;
        } else if (clauseName == "shared") {
            // Ignore for now
            supportedClause = true;
        } else if (clauseName == "firstprivate") {
            // Ignore for now
            supportedClause = true;
        } else if (clauseName == "schedule") {
            // Ignore for now
            supportedClause = true;
        } else if (clauseName == "for") {
            // Ignore, already know we're in a parallel for
            supportedClause = true;
        } else if (clauseName == "reduction") {
            assert(clauseArguments.size() == 1);
            std::string arg = clauseArguments[0];
            assert(arg.find(":") != std::string::npos);

            int start_index = 0;
            while (std::isspace(arg[start_index])) start_index++;
            int op_end_index = arg.find(":") - 1;
            while (std::isspace(arg[op_end_index])) op_end_index--;
            int vars_start_index = arg.find(":") + 1;
            while (std::isspace(arg[vars_start_index])) vars_start_index++;
            int vars_end_index = arg.size() - 1;
            while (std::isspace(arg[vars_end_index])) vars_end_index--;

            std::string op_str = arg.substr(start_index, op_end_index - start_index + 1);
            std::string vars_str = arg.substr(vars_start_index, vars_end_index - vars_start_index + 1);
            assert(vars_str.find(",") == std::string::npos); // for now only support single variable lists

            reductions.push_back(OMPReductionVar(op_str, vars_str));

            supportedClause = true;
        }
    } else if (pragmaName == "simd") {
        // Nothing at the moment
    } else {
        std::cerr << "Unsupported pragma \"" << pragmaName << "\"" << std::endl;
        exit(1);
    }

    if (!supportedClause) {
        std::cerr << "Unsupported clause \"" << clauseName <<
            "\" on pragma \"" << pragmaName << "\" with " <<
            clauseArguments.size() << " argument(s)" << std::endl;
        exit(1);
    }

    if (clauses.find(clauseName) == clauses.end()) {
        clauses[clauseName] = clauseArguments;
    } else {
        for (std::vector<std::string>::iterator i = clauseArguments.begin(),
                e = clauseArguments.end(); i != e; i++) {
            clauses[clauseName].push_back(*i);
        }
    }
}

