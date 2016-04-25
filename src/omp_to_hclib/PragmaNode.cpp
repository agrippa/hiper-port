#include "PragmaNode.h"

#include <iostream>
#include <sstream>

PragmaNode::PragmaNode(const clang::CallExpr *setMarker,
        const clang::Stmt *setBody, std::vector<clang::ValueDecl *> *setCaptures,
        clang::SourceManager *setSM) {
    marker = setMarker;
    parent = NULL;
    body = setBody;
    captures = setCaptures;
    SM = setSM;
}

const clang::Stmt *PragmaNode::getBody() {
    return body;
}

const clang::CallExpr *PragmaNode::getMarker() {
    return marker;
}

std::vector<clang::ValueDecl *> *PragmaNode::getCaptures() {
    return captures;
}

PragmaNode *PragmaNode::getParent() {
    return parent;
}

std::string PragmaNode::getPragmaName() {
    if (marker == NULL) {
        // Root node only
        return "root";
    }
    assert(marker->getNumArgs() == 2);
    const clang::Expr *pragmaNameArg = marker->getArg(0);
    while (clang::isa<clang::ImplicitCastExpr>(pragmaNameArg)) {
        pragmaNameArg = clang::dyn_cast<clang::ImplicitCastExpr>(
                pragmaNameArg)->getSubExpr();
    }
    const clang::StringLiteral *literal = clang::dyn_cast<clang::StringLiteral>(
            pragmaNameArg);
    assert(literal);
    std::string pragmaName = literal->getString().str();
    return pragmaName;
}

std::string PragmaNode::getPragmaArguments() {
    assert(marker->getNumArgs() == 2);
    const clang::Expr *pragmaArgsArg = marker->getArg(1);
    while (clang::isa<clang::ImplicitCastExpr>(pragmaArgsArg)) {
        pragmaArgsArg = clang::dyn_cast<clang::ImplicitCastExpr>(
                pragmaArgsArg)->getSubExpr();
    }
    const clang::StringLiteral *literal = clang::dyn_cast<clang::StringLiteral>(
            pragmaArgsArg);
    assert(literal);
    return literal->getString().str();
}

std::string PragmaNode::getPragmaCmd() {
    std::string pragmaName = getPragmaName();
    assert(pragmaName == "omp");

    std::string pragmaArgs = getPragmaArguments();
    size_t ompPragmaNameEnd = pragmaArgs.find(' ');
    std::string ompPragma;

    if (ompPragmaNameEnd == std::string::npos) {
        // No clauses
        ompPragma = pragmaArgs;
    } else {
        ompPragma = pragmaArgs.substr(0, ompPragmaNameEnd);
    }
    return ompPragma;
}

PragmaNode *PragmaNode::getParentAccountForFusing() {
    if (getParent()->getPragmaName() == "omp" &&
            (getParent()->getPragmaCmd() == "single" || getParent()->getPragmaCmd() == "master") &&
            getParent()->getParent()->getPragmaCmd() == "parallel") {
        return getParent()->getParent()->getParent();
    } else if (getPragmaName() == "omp" &&
            (getPragmaCmd() == "single" || getPragmaCmd() == "master") &&
            getParent()->getPragmaCmd() == "parallel") {
        return getParent()->getParent();
    } else {
        return getParent();
    }
}

std::vector<PragmaNode *> *PragmaNode::getChildren() {
    return &children;
}

int PragmaNode::nchildren() {
    return children.size();
}

void PragmaNode::setParent(PragmaNode *setParent) {
    parent = setParent;
}

std::string PragmaNode::getLbl() {
    std::stringstream lbl;
    lbl << "pragma" << getStartLine() << "_" << getPragmaName() << "_" <<
        getPragmaCmd() << std::flush;
    return lbl.str();
}

clang::SourceLocation PragmaNode::getStartLoc() {
    if (marker) {
        // Normal pragma
        return marker->getLocStart();
    } else {
        // Root node only
        return body->getLocStart();
    }
}

clang::SourceLocation PragmaNode::getEndLoc() {
    if (body) {
        // parallel, single, launch, etc
        return body->getLocEnd();
    } else {
        // barrier, taskwait, etc
        return marker->getLocEnd();
    }
}

int PragmaNode::getStartLine() {
    clang::PresumedLoc presumedStart = SM->getPresumedLoc(getStartLoc());
    return presumedStart.getLine();
}

int PragmaNode::getEndLine() {
    clang::PresumedLoc presumedEnd = SM->getPresumedLoc(getEndLoc());
    return presumedEnd.getLine();
}

void PragmaNode::addChild(PragmaNode *node) {
    const int childStartLine = node->getStartLine();
    const int childEndLine = node->getEndLine();

    for (int i = 0; i < children.size(); i++) {
        PragmaNode *existingChild = children.at(i);
        if (childStartLine == existingChild->getStartLine()) {
            std::cerr << "There appear to be two OMP pragmas both starting " <<
                "on the same line: " << childStartLine << std::endl;
            exit(1);
        }
        assert(childStartLine != existingChild->getEndLine());

        /*
         * We should traverse the pragmas in an order such that child pragmas
         * are only encountered after their parents.
         */
        assert(!(childStartLine < existingChild->getStartLine() &&
                    childEndLine > existingChild->getEndLine()));

        if (childStartLine > existingChild->getStartLine() &&
                childStartLine < existingChild->getEndLine()) {
            // Should be a child pragma
            if (!(childEndLine > existingChild->getStartLine() &&
                    childEndLine < existingChild->getEndLine())) {
                std::cerr << "Partially overlapping OMP pragmas? existing " <<
                    "child is " << existingChild->getStartLine() << "->" <<
                    existingChild->getEndLine() << ", new child is " <<
                    childStartLine << "->" << childEndLine << std::endl;
                exit(1);
            }
            existingChild->addChild(node);
            return;
        }
    }

    node->setParent(this);
    children.push_back(node);
}

void PragmaNode::getLeavesHelper(std::vector<PragmaNode *> *accum) {
    if (children.size() == 0) {
        accum->push_back(this);
    } else {
        for (int i = 0; i < children.size(); i++) {
            children.at(i)->getLeavesHelper(accum);
        }
    }
}

std::vector<PragmaNode *> *PragmaNode::getLeaves() {
    std::vector<PragmaNode *> *accum = new std::vector<PragmaNode *>();
    getLeavesHelper(accum);
    return accum;
}

void PragmaNode::print() {
    std::cerr << "======= pragma tree =======" << std::endl;
    printHelper(0);
    std::cerr << std::endl;
}

void PragmaNode::printHelper(int depth) {
    for (int i = 0; i < depth * 3; i++) {
        std::cerr << "=";
    }
    std::cerr << "pragma @ lines " << getStartLine() << "->" << getEndLine() <<
        ", parent = " << parent << ", this = " << this << std::endl;
    for (int i = 0; i < children.size(); i++) {
        children[i]->printHelper(depth + 1);
    }
}

