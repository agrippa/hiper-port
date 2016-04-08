#include "OMPClauses.h"

#include <assert.h>
#include <sstream>
#include <iostream>

extern std::vector<clang::ValueDecl *> globals;

OMPClauses::OMPClauses() {
}

OMPClauses::OMPClauses(std::string clauses) {
    std::vector<std::string> split_clauses;
    std::stringstream acc;
    int paren_depth = 0;
    int brace_depth = 0;
    int start = 0;
    int index = 0;

    while (index < clauses.size()) {
        if (clauses[index] == ' ' && paren_depth == 0 && brace_depth == 0) {
            int seek = index;
            while (seek < clauses.size() && clauses[seek] == ' ') seek++;
            if (seek < clauses.size() && clauses[seek] == '(') {
                index = seek;
            } else {
                split_clauses.push_back(clauses.substr(start,
                            index - start));
                index++;
                while (index < clauses.size() && clauses[index] == ' ') {
                    index++;
                }
                start = index;
            }
        } else {
            if (clauses[index] == '(') paren_depth++;
            else if (clauses[index] == ')') paren_depth--;
            else if (clauses[index] == '[') brace_depth++;
            else if (clauses[index] == ']') brace_depth--;
            index++;
        }
    }
    if (index != start) {
        split_clauses.push_back(clauses.substr(start));
    }

    for (std::vector<std::string>::iterator i = split_clauses.begin(),
            e = split_clauses.end(); i != e; i++) {
        std::string clause = *i;

        std::string clauseName;
        SingleClauseArgs *clauseArgs = NULL;
        if (clause.find("(") == std::string::npos) {
            clauseName = clause;
            clauseArgs = new SingleClauseArgs(clauseName);
        } else {
            size_t end = clause.find("(");
            clauseName = clause.substr(0, end);
            while (clauseName.at(clauseName.size() - 1) == ' ') {
                clauseName = clauseName.substr(0, clauseName.size() - 1);
            }
            std::string withoutOpenParen = clause.substr(end + 1);
            assert(withoutOpenParen[withoutOpenParen.size() - 1] == ')');
            std::string withoutParens = withoutOpenParen.substr(0,
                    withoutOpenParen.size() - 1);
            clauseArgs = new SingleClauseArgs(clauseName, withoutParens);
        }

        if (parsedClauses.find(clauseName) == parsedClauses.end()) {
            parsedClauses.insert(
                    std::pair<std::string, std::vector<SingleClauseArgs *> *>(
                        clauseName, new std::vector<SingleClauseArgs *>()));
        }
        parsedClauses.at(clauseName)->push_back(clauseArgs);
    }
}

bool OMPClauses::hasClause(std::string clause) {
    return parsedClauses.find(clause) != parsedClauses.end();
}

std::string OMPClauses::getSingleArg(std::string clause) {
    assert(hasClause(clause));
    assert(parsedClauses.at(clause)->size() == 1);
    SingleClauseArgs *arg = parsedClauses.at(clause)->at(0);
    return arg->getSingleArg();
}

std::vector<SingleClauseArgs *> *OMPClauses::getArgs(std::string clause) {
    assert(hasClause(clause));
    return parsedClauses.at(clause);
}

std::map<std::string, std::vector<SingleClauseArgs *> *>::iterator
OMPClauses::begin() {
    return parsedClauses.begin();
}

std::map<std::string, std::vector<SingleClauseArgs *> *>::iterator
OMPClauses::end() {
    return parsedClauses.end();
}

std::vector<std::string> *OMPClauses::getFlattenedArgsList(std::string clause) {
    std::vector<std::string> *flattened = new std::vector<std::string>();

    std::vector<SingleClauseArgs *> *allArgs = getArgs(clause);
    for (std::vector<SingleClauseArgs *>::iterator i = allArgs->begin(),
            e = allArgs->end(); i != e; i++) {
        std::vector<std::string> *singleClause = (*i)->getArgs();
        for (std::vector<std::string>::iterator ii = singleClause->begin(),
                ee = singleClause->end(); ii != ee; ii++) {
            flattened->push_back(*ii);
        }
    }
    return flattened;
}

int OMPClauses::getNumCollapsedLoops() {
    assert(hasClause("for"));
    if (hasClause("collapse")) {
        std::string collapseArg = getSingleArg("collapse");
        return atoi(collapseArg.c_str());
    } else {
        return 1;
    }
}

bool OMPClauses::computeSingleVarInfo(clang::ValueDecl *decl,
        bool isGlobal, std::vector<OMPVarInfo> *vars) {
    std::string varname = decl->getNameAsString();
    enum CAPTURE_TYPE type = CAPTURE_TYPE::NONE;

    std::vector<OMPReductionVar> *reductions = getReductions();
    for (std::vector<OMPReductionVar>::iterator i = reductions->begin(),
            e = reductions->end(); i != e && type == CAPTURE_TYPE::NONE; i++) {
        OMPReductionVar var = *i;
        if (var.getVar() == varname) {
            type = CAPTURE_TYPE::PRIVATE;
        }
    }

    if (type == CAPTURE_TYPE::NONE && hasClause("shared")) {
        std::vector<std::string> *args = getFlattenedArgsList("shared");
        for (std::vector<std::string>::iterator i = args->begin(),
                e = args->end(); i != e && type == CAPTURE_TYPE::NONE; i++) {
            if (*i == varname) type = CAPTURE_TYPE::SHARED;
        }
    }

    if (type == CAPTURE_TYPE::NONE && hasClause("private")) {
        std::vector<std::string> *args = getFlattenedArgsList("private");
        for (std::vector<std::string>::iterator i = args->begin(),
                e = args->end(); i != e && type == CAPTURE_TYPE::NONE; i++) {
            if (*i == varname) type = CAPTURE_TYPE::PRIVATE;
        }
    }

    if (type == CAPTURE_TYPE::NONE && hasClause("firstprivate")) {
        std::vector<std::string> *args = getFlattenedArgsList("firstprivate");
        for (std::vector<std::string>::iterator i = args->begin(),
                e = args->end(); i != e && type == CAPTURE_TYPE::NONE; i++) {
            if (*i == varname) type = CAPTURE_TYPE::FIRSTPRIVATE;
        }
    }

    if (type == CAPTURE_TYPE::NONE && hasClause("lastprivate")) {
        std::vector<std::string> *args = getFlattenedArgsList("lastprivate");
        for (std::vector<std::string>::iterator i = args->begin(),
                e = args->end(); i != e && type == CAPTURE_TYPE::NONE; i++) {
            if (*i == varname) type = CAPTURE_TYPE::LASTPRIVATE;
        }
    }

    if (type == CAPTURE_TYPE::NONE) {
        if (hasClause("default") && getSingleArg("default") == "none") {
            /*
             * If this variable is not explicitly listed in a capture clause and
             * the default capture is none, then don't add it to the output
             * list.
             */
            return false;
        } else {
            type = CAPTURE_TYPE::SHARED;
        }
    }

    vars->push_back(OMPVarInfo(decl, type, isGlobal));
    return true;
}

std::vector<OMPVarInfo> *OMPClauses::getVarInfo(
        std::vector<clang::ValueDecl *> *locals) {
    std::vector<OMPVarInfo> *vars = new std::vector<OMPVarInfo>();
    std::vector<std::string> varnames;

    for (std::vector<clang::ValueDecl *>::iterator i = locals->begin(),
            e = locals->end(); i != e; i++) {
        clang::ValueDecl *decl = *i;
        std::string declName = decl->getNameAsString();

        if (std::find(varnames.begin(), varnames.end(), declName) == varnames.end() &&
                computeSingleVarInfo(decl, false, vars)) {
            varnames.push_back(declName);
        }
    }

    for (std::vector<clang::ValueDecl *>::iterator i = globals.begin(),
            e = globals.end(); i != e; i++) {
        clang::ValueDecl *decl = *i;
        std::string declName = decl->getNameAsString();

        if (std::find(varnames.begin(), varnames.end(), declName) == varnames.end() &&
                computeSingleVarInfo(decl, true, vars)) {
            varnames.push_back(declName);
        }
    }
    return vars;
}


std::vector<OMPVarInfo> *OMPClauses::getSharedVarInfo(
        std::vector<clang::ValueDecl *> *locals) {
    std::vector<OMPVarInfo> *vars = getVarInfo(locals);
    std::vector<OMPVarInfo> *sharedOnly = new std::vector<OMPVarInfo>();

    for (std::vector<OMPVarInfo>::iterator i = vars->begin(), e = vars->end();
            i != e; i++) {
        OMPVarInfo info = *i;
        if (info.getType() == CAPTURE_TYPE::SHARED) {
            sharedOnly->push_back(info);
        }
    }

    return sharedOnly;
}

std::vector<OMPReductionVar> *OMPClauses::getReductions() {
    std::vector<OMPReductionVar> *reductions =
        new std::vector<OMPReductionVar>();
    if (hasClause("reduction")) {
        std::vector<SingleClauseArgs *> *args = getArgs("reduction");
        for (std::vector<SingleClauseArgs *>::iterator i = args->begin(),
                e = args->end(); i != e; i++) {
            SingleClauseArgs *single_arg = *i;
            std::vector<std::string> *rawArgs = single_arg->getArgs();

            std::string op = rawArgs->at(0);
            assert(op == "+");
            for (int v = 1; v < rawArgs->size(); v++) {
                reductions->push_back(OMPReductionVar(op, rawArgs->at(v)));
            }
        }
    }
    return reductions;
}

void OMPClauses::addClauseArg(std::string clause, std::string arg) {
    if (!hasClause(clause)) {
        parsedClauses.insert(
                std::pair<std::string, std::vector<SingleClauseArgs *> *>(
                    clause, new std::vector<SingleClauseArgs *>()));
    }
    parsedClauses.at(clause)->push_back(new SingleClauseArgs(clause, arg));
}
