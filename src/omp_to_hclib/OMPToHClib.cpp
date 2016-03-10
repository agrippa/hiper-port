#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/raw_ostream.h"

#include <sstream>

#include "OMPToHClib.h"

/*
void OMPToHClib::visitChildren(const clang::Stmt *s) {
    for (clang::Stmt::const_child_iterator i = s->child_begin(),
            e = s->child_end(); i != e; i++) {
        const clang::Stmt *child = *i;
        if (child != NULL) {
            setParent(child, s);
            VisitStmt(child);
        }
    }
}

std::string OMPToHClib::get_unique_arg_varname() {
    std::stringstream ss;
    ss << "____chimes_unroll_var_" << arg_var_counter;
    arg_var_counter++;
    return ss.str();
}

std::string OMPToHClib::get_decl_with_same_type(const clang::Expr *e,
        std::string varname) {
    std::stringstream ss;
    std::string type_str = e->getType().getAsString();
    if (type_str.find("(*)") != std::string::npos) {
        assert(type_str.find("(*)") == type_str.rfind("(*)"));
        size_t index = type_str.find("(*)");
        type_str.insert(index + 2, varname);
        ss << " " << type_str;
    } else {
        ss << " " << e->getType().getAsString() << " " <<
            varname;
    }

    return (ss.str());
}

const clang::Stmt *OMPToHClib::getParent(const clang::Stmt *s) {
    assert(parentMap.find(s) != parentMap.end());
    return parentMap.at(s);
}

void OMPToHClib::InsertAtFront(const clang::Stmt *s,
        std::string st) {
    const clang::Stmt *parent = getParent(s);
    while (!clang::isa<clang::CompoundStmt>(parent) &&
            !clang::isa<clang::CaseStmt>(parent)) {
        s = parent;
        parent = getParent(s);
    }
    clang::SourceLocation start = s->getLocStart();
    rewriter->InsertText(start, st, true, true);
}

void OMPToHClib::VisitStmt(const clang::Stmt *s) {

    if (const clang::CallExpr *call = clang::dyn_cast<clang::CallExpr>(s)) {
        std::string prefix = convert(call);
        InsertAtFront(call, prefix);

        const clang::FunctionDecl *callee = call->getDirectCallee();
        if (callee && callee->hasAttrs()) {
            for (clang::AttrVec::const_iterator i = callee->getAttrs().begin(),
                    e = callee->getAttrs().end(); i != e; i++) {
                // TODO
            }
        }
    } else {
        visitChildren(s);
    }
}
*/

void OMPToHClib::setParent(const clang::Stmt *child,
        const clang::Stmt *parent) {
    assert(parentMap.find(child) == parentMap.end());
    parentMap[child] = parent;
}

std::string OMPToHClib::stmtToString(const clang::Stmt* stmt) {
    std::string s;
    llvm::raw_string_ostream stream(s);
    stmt->printPretty(stream, NULL, Context->getPrintingPolicy());
    stream.flush();
    return s;
}

void OMPToHClib::visitChildren(const clang::Stmt *s) {
    for (clang::Stmt::const_child_iterator i = s->child_begin(),
            e = s->child_end(); i != e; i++) {
        const clang::Stmt *child = *i;
        if (child != NULL) {
            setParent(child, s);
            VisitStmt(child);
        }
    }
}


void OMPToHClib::VisitStmt(const clang::Stmt *s) {
    clang::SourceLocation loc = s->getLocStart();
    if (loc.isValid()) {
    }

    visitChildren(s);
}

std::vector<OMPPragma> *OMPToHClib::parseOMPPragmas(const char *ompPragmaFile) {
    std::vector<OMPPragma> *pragmas = new std::vector<OMPPragma>();

    std::ifstream fp;
    fp.open(std::string(ompPragmaFile, std::ios::in));

    std::string line;

    while (getline(fp, line)) {
        size_t end = line.find(' ');
        unsigned line_no = atoi(line.substr(0, end).c_str());
        line = line.substr(end + 1);

        end = line.find(' ');
        unsigned last_line = atoi(line.substr(0, end).c_str());
        line = line.substr(end + 1);

        std::string pragma = line;

        line = line.substr(line.find(' ') + 1);
        assert(line.find("omp") == 0);

        line = line.substr(line.find(' ') + 1);

        // Look for a command like parallel, for, etc.
        end = line.find(' ');

        if (end == std::string::npos) {
            // No clauses, just pragma name
            std::string pragma_name = line;

            pragmas->push_back(OMPPragma(line_no, last_line, line,
                        pragma_name));
        } else {
            std::string pragma_name = line.substr(0, end);

            OMPPragma pragma(line_no, last_line, line, pragma_name);
            std::string clauses = line.substr(end + 1);

            std::vector<std::string> split_clauses;
            std::stringstream acc;
            int paren_depth = 0;
            int start = 0;
            int index = 0;

            while (index < clauses.size()) {
                if (clauses[index] == ' ' && paren_depth == 0) {
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
                    index++;
                }
            }
            if (index != start) {
                split_clauses.push_back(clauses.substr(start));
            }

            for (std::vector<std::string>::iterator i = split_clauses.begin(),
                    e = split_clauses.end(); i != e; i++) {
                std::string clause = *i;

                if (clause.find("(") == std::string::npos) {
                    pragma.addClause(clause, std::vector<std::string>());
                } else {
                    size_t open = clause.find("(");
                    assert(open != std::string::npos);

                    std::string args = clause.substr(open + 1);

                    size_t close = args.find_last_of(")");
                    assert(close != std::string::npos);

                    args = args.substr(0, close);

                    std::string clause_name = clause.substr(0, open);
                    while (clause_name.at(clause_name.size() - 1) == ' ') {
                        clause_name = clause_name.substr(0, clause_name.size() - 1);
                    }

                    std::vector<std::string> clause_args;
                    int args_index = 0;
                    int args_start = 0;
                    while (args_index < args.size()) {
                        if (args[args_index] == ',') {
                            clause_args.push_back(args.substr(args_start,
                                        args_index - args_start));
                            args_index++;
                            while (args_index < args.size() &&
                                    args[args_index] == ' ') {
                                args_index++;
                            }
                            args_start = args_index;
                        } else {
                            args_index++;
                        }
                    }

                    if (args_start != args_index) {
                        clause_args.push_back(args.substr(args_start));
                    }

                    pragma.addClause(clause_name, clause_args);
                }
            }

            pragmas->push_back(pragma);
        }
    }
    fp.close();

    return pragmas;
}

OMPToHClib::OMPToHClib(const char *ompPragmaFile) {
    pragmas = parseOMPPragmas(ompPragmaFile);
}
