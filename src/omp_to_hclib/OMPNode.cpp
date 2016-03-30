#include "OMPNode.h"

#include <iostream>

// #define VERBOSE

OMPNode::OMPNode(const clang::Stmt *setBody, int setPragmaLine,
        OMPPragma *setPragma, OMPNode *setParent, std::string setLbl,
        clang::SourceManager *SM) {
    body = setBody;
    pragmaLine = setPragmaLine;
    pragma = setPragma;
    parent = setParent;
    lbl = setLbl;

    if (body) {
        clang::PresumedLoc presumedStart = SM->getPresumedLoc(body->getLocStart());
        clang::PresumedLoc presumedEnd = SM->getPresumedLoc(body->getLocEnd());
        startLine = presumedStart.getLine();
        endLine = presumedEnd.getLine();
    } else {
        // body is NULL on root node of function
        startLine = -1;
        endLine = -1;
    }
}

int OMPNode::getStartLine() {
    return startLine;
}

int OMPNode::getEndLine() {
    return endLine;
}

int OMPNode::getPragmaLine() {
    return pragmaLine;
}

OMPPragma *OMPNode::getPragma() {
    return pragma;
}

std::string OMPNode::getLbl() {
    return lbl;
}

const clang::Stmt *OMPNode::getBody() {
    return body;
}

OMPNode *OMPNode::getParent() {
    return parent;
}

std::vector<OMPNode *> *OMPNode::getChildren() {
    return &children;
}

void OMPNode::addChild(const clang::Stmt *stmt, int newPragmaLine,
        OMPPragma *newPragma, std::string newLbl, clang::SourceManager *SM) {
    int childStartLine, childEndLine;
    if (stmt) {
        clang::PresumedLoc presumedStart = SM->getPresumedLoc(
                stmt->getLocStart());
        clang::PresumedLoc presumedEnd = SM->getPresumedLoc(stmt->getLocEnd());
        childStartLine = presumedStart.getLine();
        childEndLine = presumedEnd.getLine();
    } else {
        childStartLine = newPragmaLine;
        childEndLine = newPragmaLine;
    }

    for (int i = 0; i < children.size(); i++) {
        OMPNode *existingChild = children.at(i);
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
                    "child (" << existingChild->getLbl() << ") is " <<
                    existingChild->getStartLine() << "->" <<
                    existingChild->getEndLine() << ", new child (" << lbl <<
                    ") is " << childStartLine << "->" << childEndLine <<
                    std::endl;
                exit(1);
            }
            existingChild->addChild(stmt, newPragmaLine, newPragma, newLbl, SM);
            return;
        }
    }
#ifdef VERBOSE
    std::cerr << "Adding pragma " << newPragma->getPragmaName() <<
        " at line " << newPragmaLine << " as child of " <<
        (pragma ? pragma->getPragmaName() : "root") << " at line " <<
        pragmaLine << std::endl;
#endif
    children.push_back(new OMPNode(stmt, newPragmaLine, newPragma, this, newLbl,
                SM));
}

void OMPNode::print() {
    std::cerr << "======= OMP pragma tree =======" << std::endl;
    printHelper(0);
    std::cerr << std::endl;
}

void OMPNode::printHelper(int depth) {
    for (int i = 0; i < depth * 2; i++) {
        std::cerr << "=";
    }
    std::cerr << (pragma ? pragma->getPragmaName() : "root") << " @ line " <<
        pragmaLine << ", body = (" << startLine << "->" << endLine << "), " <<
        children.size() << " children, lbl = " << lbl << ", parent = " <<
        parent << ", this = " << this << std::endl;
    for (int i = 0; i < children.size(); i++) {
        children[i]->printHelper(depth + 1);
    }
}

int OMPNode::nchildren() {
    return children.size();
}

void OMPNode::getLeavesHelper(std::vector<OMPNode *> *accum) {
    if (nchildren() == 0) {
        accum->push_back(this);
    } else {
        for (int i = 0; i < nchildren(); i++) {
            children.at(i)->getLeavesHelper(accum);
        }
    }
}

std::vector<OMPNode *> *OMPNode::getLeaves() {
    std::vector<OMPNode *> *accum = new std::vector<OMPNode *>();
    getLeavesHelper(accum);
    return accum;
}
