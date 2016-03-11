#include "OMPNode.h"

#include <iostream>

OMPNode::OMPNode(const clang::Stmt *setBody, int setPragmaLine, OMPPragma *setPragma,
        clang::SourceManager *SM) {
    body = setBody;
    pragmaLine = setPragmaLine;
    pragma = setPragma;

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

void OMPNode::addChild(const clang::Stmt *stmt, int pragmaLine, OMPPragma *pragma,
        clang::SourceManager *SM) {
    clang::PresumedLoc presumedStart = SM->getPresumedLoc(stmt->getLocStart());
    clang::PresumedLoc presumedEnd = SM->getPresumedLoc(stmt->getLocEnd());
    const int childStartLine = presumedStart.getLine();
    const int childEndLine = presumedEnd.getLine();

    for (int i = 0; i < children.size(); i++) {
        OMPNode &existingChild = children.at(i);
        assert(childStartLine != existingChild.getStartLine());
        assert(childStartLine != existingChild.getEndLine());

        /*
         * We should traverse the pragmas in an order such that child pragmas
         * are only encoutered after their parents.
         */
        assert(!(childStartLine < existingChild.getStartLine() &&
                    childEndLine > existingChild.getEndLine()));

        if (childStartLine > existingChild.getStartLine() &&
                childStartLine < existingChild.getEndLine()) {
            // Should be a child pragma
            assert(childEndLine > existingChild.getStartLine() &&
                    childEndLine < existingChild.getEndLine());
            existingChild.addChild(stmt, pragmaLine, pragma, SM);
            return;
        }
    }
    children.push_back(OMPNode(stmt, pragmaLine, pragma, SM));
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
    std::cerr << "pragma @ line " << pragmaLine << ", " << children.size() <<
        " children" << std::endl;
    for (int i = 0; i < children.size(); i++) {
        children[i].printHelper(depth + 1);
    }
}
