#ifndef OMP_PRAGMA_H
#define OMP_PRAGMA_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

#include "OMPReductionVar.h"

class OMPPragma {
    public:
        OMPPragma(int setLine, int setLastLine,
                std::string setPragma, std::string setPragmaName) :
                line(setLine), lastLine(setLastLine), pragma(setPragma),
                pragmaName(setPragmaName) {
            if (setPragmaName != "parallel" && setPragmaName != "simd") {
                std::cerr << "Unsupported pragma detected: \"" << setPragma <<
                    "\"" << std::endl;
                exit(1);
            }
        }

        void addClause(std::string clauseName,
                std::vector<std::string> clauseArguments);

        int getLine() { return line; }
        int getLastLine() { return lastLine; }
        void updateLine(int setLine) { line = setLine; }
        std::string getPragma() { return pragma; }

        std::string getPragmaName() { return pragmaName; }
        std::map<std::string, std::vector<std::string> > getClauses() {
            return clauses;
        }
        std::vector<OMPReductionVar> *getReductions() {
            return &reductions;
        }

    private:
        int line;
        int lastLine;
        std::string pragma;

        std::string pragmaName;
        std::map<std::string, std::vector<std::string> > clauses;
        std::vector<OMPReductionVar> reductions;
};

#endif // OMP_PRAGMA_H
