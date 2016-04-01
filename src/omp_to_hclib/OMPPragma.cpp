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
        } else if (clauseName == "default") {
            // Ignore for now
            supportedClause = true;
        } else if (clauseName == "num_threads") {
            // Ignore for now
            supportedClause = true;
        } else if (clauseName == "for") {
            // Ignore, already know we're in a parallel for
            supportedClause = true;
        } else if (clauseName == "reduction") {
            assert(clauseArguments.size() >= 1);
            std::string firstArg = clauseArguments[0];
            assert(firstArg.find(":") != std::string::npos);

            int start_index = 0;
            while (std::isspace(firstArg[start_index])) start_index++;
            int op_end_index = firstArg.find(":") - 1;
            while (std::isspace(firstArg[op_end_index])) op_end_index--;
            int vars_start_index = firstArg.find(":") + 1;
            while (std::isspace(firstArg[vars_start_index])) vars_start_index++;
            int vars_end_index = firstArg.size() - 1;
            while (std::isspace(firstArg[vars_end_index])) vars_end_index--;

            std::string opStr = firstArg.substr(start_index,
                    op_end_index - start_index + 1);
            std::string firstVarStr = firstArg.substr(vars_start_index,
                    vars_end_index - vars_start_index + 1);

            reductions.push_back(OMPReductionVar(opStr, firstVarStr));
            for (int i = 1; i < clauseArguments.size(); i++) {
                reductions.push_back(OMPReductionVar(opStr,
                            clauseArguments[i]));
            }

            supportedClause = true;
        }
    } else if (pragmaName == "simd") {
        // Nothing at the moment
    } else if (pragmaName == "single") {
        if (clauseName == "nowait") {
            supportedClause = true;
        } else if (clauseName == "private") {
            // Ignore for now
            supportedClause = true;
        }
    } else if (pragmaName == "task") {
        if (clauseName == "untied") {
            /*
             * An untied task is one who, on suspend, can be resumed by any
             * thread not just the thread it suspended on. For now we ignore
             * this case, but in the future this could cause problems (e.g. if
             * the task uses thread-local variables and we work steal it onto
             * another thread after a blocking condition is satisfied). This
             * could be implemented by executing the task at a locale.
             */
            supportedClause = true;
        } else if (clauseName == "private") {
            // Ignore for now
            supportedClause = true;
        } else if (clauseName == "firstprivate") {
            // Ignore for now
            supportedClause = true;
        } else if (clauseName == "shared") {
            // Ignore for now
            supportedClause = true;
        } else if (clauseName == "if") {
            // Handled in the actual code generation step in OMPToHClib
            assert(clauseArguments.size() == 1); // only one argument to the if clause
            assert(clauses.find("if") == clauses.end()); // only one if per task
            supportedClause = true;
        } else if (clauseName == "final") {
            /*
             * If the expression passed to final evaluates to true, then the
             * task created is "final" and any of its transitively spawned child
             * tasks are also "final". This essentially turns the spawned task
             * into a sequential block of code regardless of any parallel/task
             * pragmas. We don't really have an equivalent for this in HClib at
             * the moment so for now we ignore this. In the future if we wanted
             * to better support this we could add a flag to hclib_async that
             * turns all spawn calls beneath it into sequential recursive calls.
             */
            supportedClause = true;
        } else if (clauseName == "mergeable") {
            // Ignore for now
            supportedClause = true;
        }
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


bool OMPPragma::expectsSuccessorBlock() {
    return pragmaName != "taskwait" && pragmaName != "barrier";
}
