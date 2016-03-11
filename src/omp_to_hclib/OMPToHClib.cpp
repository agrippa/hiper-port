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
#include <iostream>

#include "OMPToHClib.h"

extern clang::FunctionDecl *curr_func_decl;

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

std::vector<OMPPragma> *OMPToHClib::getOMPPragmasFor(
        clang::FunctionDecl *decl, clang::SourceManager &SM) {
    std::vector<OMPPragma> *result = new std::vector<OMPPragma>();
    if (!decl->hasBody()) return result;

    clang::SourceLocation start = decl->getBody()->getLocStart();
    clang::SourceLocation end = decl->getBody()->getLocEnd();

    clang::PresumedLoc startLoc = SM.getPresumedLoc(start);
    clang::PresumedLoc endLoc = SM.getPresumedLoc(end);

    for (std::vector<OMPPragma>::iterator i = pragmas->begin(),
            e = pragmas->end(); i != e; i++) {
        OMPPragma curr = *i;

        bool before = (curr.getLine() < startLoc.getLine());
        bool after = (curr.getLine() > endLoc.getLine());

        if (!before && !after) {
            result->push_back(curr);
        }
    }

    return result;
}

OMPPragma *OMPToHClib::getOMPPragmaFor(int lineNo) {
    for (std::vector<OMPPragma>::iterator i = pragmas->begin(),
            e = pragmas->end(); i != e; i++) {
        OMPPragma *curr = &*i;
        if (curr->getLine() == lineNo) return curr;
    }
    return NULL;
}

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

void OMPToHClib::postVisit() {
    if (curr_func_decl) {
        std::string fname = curr_func_decl->getNameAsString();
        clang::PresumedLoc presumedStart = SM->getPresumedLoc(curr_func_decl->getLocStart());
        const int functionStartLine = presumedStart.getLine();

        int countParallelRegions = 0;

        std::cerr << "Post visiting " << fname << std::endl;

        for (std::map<int, const clang::Stmt *>::iterator i = predecessors.begin(),
                e = predecessors.end(); i != e; i++) {
            int line = i->first;
            const clang::Stmt *pred = i->second;
            assert(successors.find(line) != successors.end());
            assert(captures.find(line) != captures.end());
            const clang::Stmt *succ = successors[line];
            OMPPragma *pragma = getOMPPragmaFor(line);

            if (supportedPragmas.find(pragma->getPragmaName()) != supportedPragmas.end()) {
                std::string lbl = fname + std::to_string(countParallelRegions++);
                std::cerr<< "pragma on " << stmtToString(succ) <<
                    " with lbl " << lbl << std::endl;

                structFile << "struct name = " << lbl << std::endl;
                structFile << "declare on line = " << functionStartLine << std::endl;
                for (std::vector<clang::ValueDecl *>::iterator ii =
                        captures[line]->begin(), ee = captures[line]->end();
                        ii != ee; ii++) {
                    clang::ValueDecl *curr = *ii;
                    structFile << curr->getType().getAsString() << " " << curr->getNameAsString() << std::endl;
                }
                structFile << "======" << std::endl;
            }
        }

        successors.clear();
        predecessors.clear();
    }
}

bool OMPToHClib::isScopeCreatingStmt(const clang::Stmt *s) {
    return clang::isa<clang::CompoundStmt>(s) || clang::isa<clang::ForStmt>(s);
}

int OMPToHClib::getCurrentLexicalDepth() {
    return in_scope.size();
}

void OMPToHClib::addNewScope() {
    in_scope.push_back(new std::vector<clang::ValueDecl *>());
}

void OMPToHClib::popScope() {
    assert(in_scope.size() >= 1);
    in_scope.pop_back();
}

void OMPToHClib::addToCurrentScope(clang::ValueDecl *d) {
    in_scope[in_scope.size() - 1]->push_back(d);
}

std::vector<clang::ValueDecl *> *OMPToHClib::visibleDecls() {
    std::vector<clang::ValueDecl *> *visible = new std::vector<clang::ValueDecl *>();
    for (std::vector<std::vector<clang::ValueDecl *> *>::iterator i =
            in_scope.begin(), e = in_scope.end(); i != e; i++) {
        std::vector<clang::ValueDecl *> *currentScope = *i;
        for (std::vector<clang::ValueDecl *>::iterator ii = currentScope->begin(),
                ee = currentScope->end(); ii != ee; ii++) {
            visible->push_back(*ii);
        }
    }
    return visible;
}

void OMPToHClib::VisitStmt(const clang::Stmt *s) {
    clang::SourceLocation start = s->getLocStart();
    clang::SourceLocation end = s->getLocEnd();

    const int currentScope = getCurrentLexicalDepth();
    if (isScopeCreatingStmt(s)) {
        addNewScope();
    }

    if (start.isValid() && end.isValid() && SM->isInMainFile(end)) {
        clang::PresumedLoc presumed_start = SM->getPresumedLoc(start);
        clang::PresumedLoc presumed_end = SM->getPresumedLoc(end);
        /*
         * This block of code checks if the current statement is either the
         * first before (predecessors) or after (successors) the line that an
         * OMP pragma is on.
         */
        std::vector<OMPPragma> *omp_pragmas =
            getOMPPragmasFor(curr_func_decl, *SM);
        for (std::vector<OMPPragma>::iterator i = omp_pragmas->begin(),
                e = omp_pragmas->end(); i != e; i++) {
            OMPPragma pragma = *i;

            if (presumed_end.getLine() < pragma.getLine()) {
                if (predecessors.find(pragma.getLine()) ==
                        predecessors.end()) {
                    predecessors[pragma.getLine()] = s;
                    captures[pragma.getLine()] = visibleDecls();
                } else {
                    const clang::Stmt *curr = predecessors[pragma.getLine()];
                    clang::PresumedLoc curr_loc = SM->getPresumedLoc(
                            curr->getLocEnd());
                    if (presumed_end.getLine() > curr_loc.getLine() ||
                            (presumed_end.getLine() == curr_loc.getLine() &&
                             presumed_end.getColumn() > curr_loc.getColumn())) {
                        predecessors[pragma.getLine()] = s;
                        captures[pragma.getLine()] = visibleDecls();
                    }
                }
            }

            if (presumed_start.getLine() >= pragma.getLine()) {
                if (successors.find(pragma.getLine()) == successors.end()) {
                    successors[pragma.getLine()] = s;
                } else {
                    const clang::Stmt *curr = successors[pragma.getLine()];
                    clang::PresumedLoc curr_loc =
                        SM->getPresumedLoc(curr->getLocStart());
                    if (presumed_start.getLine() < curr_loc.getLine() ||
                            (presumed_start.getLine() == curr_loc.getLine() &&
                             presumed_start.getColumn() < curr_loc.getColumn())) {
                        successors[pragma.getLine()] = s;
                    }
                }
            }
        }

        if (const clang::DeclStmt *decls = clang::dyn_cast<clang::DeclStmt>(s)) {
            std::cerr << "Found declaration " << stmtToString(s) << std::endl;

            for (clang::DeclStmt::const_decl_iterator i = decls->decl_begin(),
                    e = decls->decl_end(); i != e; i++) {
                clang::Decl *decl = *i;
                if (clang::ValueDecl *named = clang::dyn_cast<clang::ValueDecl>(decl)) {
                    addToCurrentScope(named);
                }
            }
        }
    }

    visitChildren(s);

    if (isScopeCreatingStmt(s)) {
        popScope();
    }
    assert(currentScope == getCurrentLexicalDepth());
}

std::vector<OMPPragma> *OMPToHClib::parseOMPPragmas(const char *ompPragmaFile) {
    std::vector<OMPPragma> *pragmas = new std::vector<OMPPragma>();

    std::ifstream fp;
    fp.open(std::string(ompPragmaFile), std::ios::in);
    if (!fp.is_open()) {
        std::cerr << "Failed opening " << std::string(ompPragmaFile) << std::endl;
        exit(1);
    }

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

    std::cerr << "Parsed " << pragmas->size() << " pragmas from " <<
        std::string(ompPragmaFile) << std::endl;

    return pragmas;
}

OMPToHClib::OMPToHClib(const char *ompPragmaFile, const char *structFilename) {
    pragmas = parseOMPPragmas(ompPragmaFile);

    supportedPragmas.insert("parallel");

    structFile.open(std::string(structFilename), std::ios::out);
    assert(structFile.is_open());
}

OMPToHClib::~OMPToHClib() {
    structFile.close();
}
