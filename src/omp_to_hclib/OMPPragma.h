#ifndef OMP_PRAGMA_H
#define OMP_PRAGMA_H

#include <string>
#include <map>
#include <vector>

class OMPPragma {
    public:
        OMPPragma(int setLine, int setLastLine,
                std::string setPragma, std::string setPragmaName) :
                line(setLine), lastLine(setLastLine), pragma(setPragma),
                pragmaName(setPragmaName) { }

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

    private:
        int line;
        int lastLine;
        std::string pragma;

        std::string pragmaName;
        std::map<std::string, std::vector<std::string> > clauses;
};

#endif // OMP_PRAGMA_H
