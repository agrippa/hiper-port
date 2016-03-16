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
#include "clang/AST/ASTContext.h"
#include "clang/AST/Stmt.h"
#include "llvm/ADT/ArrayRef.h"

#include <algorithm>
#include <sstream>
#include <iostream>

#include "OMPToHClib.h"
#include "OMPNode.h"

/*
 * NOTE: The one thing to be careful about whenever inserting code in an
 * existing function is that the brace inserter is not run before this pass
 * anymore. That means that extra care must be taken to ensure that converting a
 * single line of code to a multi-line block of code does not change the control
 * flow of the application. This should be kept in mind anywhere that an
 * InsertText, ReplaceText, etc API is called.
 */

extern clang::FunctionDecl *curr_func_decl;

#define ASYNC_SUFFIX "_hclib_async"

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

const clang::Stmt *OMPToHClib::getParent(const clang::Stmt *s) {
    assert(parentMap.find(s) != parentMap.end());
    return parentMap.at(s);
}

std::string OMPToHClib::stmtToString(const clang::Stmt* stmt) {
    std::string s;
    llvm::raw_string_ostream stream(s);
    stmt->printPretty(stream, NULL, Context->getPrintingPolicy());
    stream.flush();
    return s;
}

void OMPToHClib::visitChildren(const clang::Stmt *s, bool firstTraversal) {
    for (clang::Stmt::const_child_iterator i = s->child_begin(),
            e = s->child_end(); i != e; i++) {
        const clang::Stmt *child = *i;
        if (child != NULL) {
            if (firstTraversal) {
                setParent(child, s);
            }
            VisitStmt(child);
        }
    }
}

std::string OMPToHClib::getDeclarationTypeStr(clang::QualType qualType,
        std::string name, std::string soFarBefore, std::string soFarAfter) {
    const clang::Type *type = qualType.getTypePtr();
    assert(type);

    if (const clang::ArrayType *arrayType = type->getAsArrayTypeUnsafe()) {
        if (const clang::ConstantArrayType *constantArrayType = clang::dyn_cast<clang::ConstantArrayType>(arrayType)) {
            std::string sizeStr = constantArrayType->getSize().toString(10, true);
            return getDeclarationTypeStr(constantArrayType->getElementType(),
                    name, soFarBefore, soFarAfter + "[" + sizeStr + "]");
        } else {
            std::cerr << "Unsupported array size modifier " << arrayType->getSizeModifier() << std::endl;
            exit(1);
        }
    } else if (const clang::PointerType *pointerType = type->getAs<clang::PointerType>()) {
        return getDeclarationTypeStr(pointerType->getPointeeType(), name, soFarBefore + "*", soFarAfter);
    } else if (const clang::BuiltinType *builtinType = type->getAs<clang::BuiltinType>()) {
        return qualType.getAsString() + " " + soFarBefore + name + soFarAfter;
    } else if (const clang::TypedefType *typedefType = type->getAs<clang::TypedefType>()) {
        return getDeclarationTypeStr(typedefType->getDecl()->getUnderlyingType(), name, soFarBefore, soFarAfter);
    } else if (const clang::ElaboratedType *elaboratedType = type->getAs<clang::ElaboratedType>()) {
        return getDeclarationTypeStr(elaboratedType->getNamedType(), name, soFarBefore, soFarAfter);
    } else if (const clang::TagType *tagType = type->getAs<clang::TagType>()) {
        if (tagType->getDecl()->getDeclName()) {
            return tagType->getDecl()->getDeclName().getAsString() + " " + soFarBefore + name + soFarAfter;
        } else if (tagType->getDecl()->getTypedefNameForAnonDecl()) {
            return tagType->getDecl()->getTypedefNameForAnonDecl()->getNameAsString() + " " + soFarBefore + name + soFarAfter;
        } else {
            assert(false);
        }
    } else {
        std::cerr << "Unsupported type " << std::string(type->getTypeClassName()) << std::endl;
        exit(1);
    }
}

std::string OMPToHClib::getDeclarationStr(clang::ValueDecl *decl) {
    return getDeclarationTypeStr(decl->getType(), decl->getNameAsString(), "", "") + ";";
}

std::string OMPToHClib::getArraySizeExpr(clang::QualType qualType) {
    const clang::Type *type = qualType.getTypePtr();
    assert(type);

    if (const clang::ArrayType *arrayType = type->getAsArrayTypeUnsafe()) {
        if (const clang::ConstantArrayType *constantArrayType = clang::dyn_cast<clang::ConstantArrayType>(arrayType)) {
            std::string sizeStr = constantArrayType->getSize().toString(10, true);
            return sizeStr + " * (" + getArraySizeExpr(constantArrayType->getElementType()) + ")";
        } else {
            std::cerr << "Unsupported array size modifier " << arrayType->getSizeModifier() << std::endl;
            exit(1);
        }
    } else if (const clang::BuiltinType *builtinType = type->getAs<clang::BuiltinType>()) {
        return "sizeof(" + qualType.getAsString() + ")";
    } else {
        std::cerr << "Unsupported type while getting array size expression " <<
            std::string(type->getTypeClassName()) << std::endl;
        exit(1);
    }
}

std::string OMPToHClib::getUnpackStr(clang::ValueDecl *decl) {
    const clang::Type *type = decl->getType().getTypePtr();
    assert(type);

    std::stringstream ss;
    ss << getDeclarationTypeStr(decl->getType(), decl->getNameAsString(), "",
            "") << "; ";

    if (const clang::ArrayType *arrayType = type->getAsArrayTypeUnsafe()) {
        std::string arraySizeExpr = getArraySizeExpr(decl->getType());
        ss << "memcpy(" << decl->getNameAsString() << ", ctx->" <<
            decl->getNameAsString() << ", " << arraySizeExpr << "); ";
    } else if (type->getAs<clang::PointerType>() ||
            type->getAs<clang::BuiltinType>() ||
            type->getAs<clang::TypedefType>() ||
            type->getAs<clang::ElaboratedType>() ||
            type->getAs<clang::TagType>()) {
        ss << decl->getNameAsString() << " = ctx->" << decl->getNameAsString() << ";";
    } else {
        std::cerr << "Unsupported type " << std::string(type->getTypeClassName()) << std::endl;
        exit(1);
    }

    ss << std::flush;
    return ss.str();
}

std::string OMPToHClib::getCaptureStr(clang::ValueDecl *decl) {
    const clang::Type *type = decl->getType().getTypePtr();
    assert(type);

    std::stringstream ss;

    if (const clang::ArrayType *arrayType = type->getAsArrayTypeUnsafe()) {
        std::string arraySizeExpr = getArraySizeExpr(decl->getType());
        ss << "memcpy(ctx->" << decl->getNameAsString() << ", " <<
            decl->getNameAsString() << ", " << arraySizeExpr << "); ";
    } else if (type->getAs<clang::PointerType>() ||
            type->getAs<clang::BuiltinType>() ||
            type->getAs<clang::TypedefType>() ||
            type->getAs<clang::ElaboratedType>() ||
            type->getAs<clang::TagType>()) {
        ss << "ctx->" << decl->getNameAsString() << " = " << decl->getNameAsString() <<
            ";";
    } else {
        std::cerr << "Unsupported type " << std::string(type->getTypeClassName()) << std::endl;
        exit(1);
    }

    ss << std::flush;
    return ss.str();
}

std::string OMPToHClib::getStructDef(std::string structName,
        std::vector<clang::ValueDecl *> *captured) {
    std::string struct_def = "typedef struct _" + structName + " {\n";
    for (std::vector<clang::ValueDecl *>::iterator ii =
            captured->begin(), ee = captured->end();
            ii != ee; ii++) {
        clang::ValueDecl *curr = *ii;
        struct_def += "    " + getDeclarationStr(curr) + "\n";
    }
    struct_def += " } " + structName + ";\n\n";

    return struct_def;
}

void OMPToHClib::preFunctionVisit(clang::FunctionDecl *func) {
    assert(getCurrentLexicalDepth() == 0);
    addNewScope();

    for (int i = 0; i < func->getNumParams(); i++) {
        clang::ParmVarDecl *param = func->getParamDecl(i);
        addToCurrentScope(param);
    }
}

clang::Expr *OMPToHClib::unwrapCasts(clang::Expr *expr) {
    while (clang::isa<clang::ImplicitCastExpr>(expr)) {
        expr = clang::dyn_cast<clang::ImplicitCastExpr>(expr)->getSubExpr();
    }
    return expr;
}

std::string OMPToHClib::getCondVarAndLowerBoundFromInit(const clang::Stmt *init,
        const clang::ValueDecl **condVar) {
    if (const clang::BinaryOperator *bin =
            clang::dyn_cast<clang::BinaryOperator>(init)) {
        clang::Expr *lhs = bin->getLHS();
        if (const clang::DeclRefExpr *declref =
                clang::dyn_cast<clang::DeclRefExpr>(lhs)) {
            *condVar = declref->getDecl();
            return stmtToString(bin->getRHS());
        } else {
            std::cerr << "LHS is unsupported class " <<
                std::string(lhs->getStmtClassName()) << std::endl;
            exit(1);
        }
    } else {
        std::cerr << "Unsupported init class " <<
            std::string(init->getStmtClassName()) << std::endl;
        exit(1);
    }
}

std::string OMPToHClib::getUpperBoundFromCond(const clang::Stmt *cond,
        const clang::ValueDecl *condVar) {
    if (const clang::BinaryOperator *bin = clang::dyn_cast<clang::BinaryOperator>(cond)) {
        if (bin->getOpcode() == clang::BO_LT) {
            clang::Expr *lhs = unwrapCasts(bin->getLHS());
            const clang::DeclRefExpr *declref = clang::dyn_cast<clang::DeclRefExpr>(lhs);
            assert(declref);
            const clang::ValueDecl *decl = declref->getDecl();
            assert(condVar == decl);
            return stmtToString(bin->getRHS());
        } else {
            std::cerr << "Unsupported binary operator" << std::endl;
            exit(1);
        }
    } else {
        std::cerr << "Unsupported cond class " <<
            std::string(cond->getStmtClassName()) <<
            std::endl;
        exit(1);
    }
}

std::string OMPToHClib::getStrideFromIncr(const clang::Stmt *inc,
        const clang::ValueDecl *condVar) {
    if (const clang::UnaryOperator *uno = clang::dyn_cast<clang::UnaryOperator>(inc)) {
        clang::Expr *target = unwrapCasts(uno->getSubExpr());
        const clang::DeclRefExpr *declref = clang::dyn_cast<clang::DeclRefExpr>(target);
        assert(declref);
        const clang::ValueDecl *decl = declref->getDecl();
        assert(condVar == decl);

        if (uno->getOpcode() == clang::UO_PostInc ||
                uno->getOpcode() == clang::UO_PreInc) {
            return "1";
        } else if (uno->getOpcode() == clang::UO_PostDec ||
                uno->getOpcode() == clang::UO_PreDec) {
            return "-1";
        } else {
            std::cerr << "Unsupported unary operator" << std::endl;
            exit(1);
        }
    } else if (const clang::BinaryOperator *bin = clang::dyn_cast<clang::BinaryOperator>(inc)) {
        if (bin->getOpcode() == clang::BO_Assign) {
            clang::Expr *target = unwrapCasts(bin->getLHS());
            const clang::DeclRefExpr *declref = clang::dyn_cast<clang::DeclRefExpr>(target);
            assert(declref);
            assert(condVar == declref->getDecl());

            clang::Expr *rhs = unwrapCasts(bin->getRHS());
            if (const clang::BinaryOperator *rhsBin = clang::dyn_cast<clang::BinaryOperator>(rhs)) {
                clang::Expr *lhs = unwrapCasts(rhsBin->getLHS());
                clang::Expr *rhs = unwrapCasts(rhsBin->getRHS());
                clang::Expr *other = NULL;

                clang::DeclRefExpr *declref = NULL;
                if (declref = clang::dyn_cast<clang::DeclRefExpr>(lhs)) {
                    other = rhs;
                } else if (declref = clang::dyn_cast<clang::DeclRefExpr>(rhs)) {
                    other = lhs;
                } else {
                    std::cerr << "Neither of the inputs to the RHS binary op for an incr clause are a decl ref" << std::endl;
                    exit(1);
                }

                assert(condVar == declref->getDecl());

                if (rhsBin->getOpcode() == clang::BO_Add) {
                    if (const clang::IntegerLiteral *literal = clang::dyn_cast<clang::IntegerLiteral>(other)) {
                        return stmtToString(literal);
                    } else {
                        std::cerr << "unsupported other is a " << other->getStmtClassName() << std::endl;
                        exit(1);
                    }
                } else {
                    std::cerr << "Unsupported binary operator on RHS of binary incr clause" << std::endl;
                    exit(1);
                }
            } else {
                std::cerr << "Unsupported RHS to binary incr clause: " << rhs->getStmtClassName() << std::endl;
                exit(1);
            }
        } else {
            std::cerr << "Unsupported binary "
                "operator in increment clause" << std::endl;
            exit(1);
        }
    } else {
        std::cerr << "Unsupported incr class " <<
            std::string(inc->getStmtClassName()) <<
            std::endl;
        exit(1);
    }
}

std::string OMPToHClib::getClosureDef(std::string closureName, bool isForasyncClosure,
        std::string contextName, std::vector<clang::ValueDecl *> *captured,
        std::string bodyStr, const clang::ValueDecl *condVar) {
    std::stringstream ss;
    ss << "static void " << closureName << "(void *arg";
    if (isForasyncClosure) {
        ss << ", const int ___iter";
    }
    ss << ") {\n";

    ss << "    " << contextName << " *ctx = (" << contextName << " *)arg;\n";
    for (std::vector<clang::ValueDecl *>::iterator ii = captured->begin(),
            ee = captured->end(); ii != ee; ii++) {
        clang::ValueDecl *curr = *ii;
        ss << "    " << getUnpackStr(curr) << "\n";
    }

    /*
     * Insert a one iteration do-loop around the
     * original body so that continues have the same
     * semantics.
     */
    if (isForasyncClosure) {
        ss << "    " << condVar->getNameAsString() << " = ___iter;\n";
        ss << "    do {\n";
    }
    ss << bodyStr;
    if (isForasyncClosure) {
        ss << "    } while (0);\n";
    }
    ss << "}\n\n";
    return ss.str();
}

std::string OMPToHClib::getContextSetup(std::string structName,
        std::vector<clang::ValueDecl *> *captured) {
    std::stringstream ss;
    ss << structName << " *ctx = (" << structName << " *)malloc(sizeof(" <<
        structName << "));\n";
    for (std::vector<clang::ValueDecl *>::iterator ii = captured->begin(),
            ee = captured->end(); ii != ee; ii++) {
        clang::ValueDecl *curr = *ii;
        ss << getCaptureStr(curr) << "\n";
    }
    return ss.str();
}

void OMPToHClib::postFunctionVisit(clang::FunctionDecl *func) {
    assert(getCurrentLexicalDepth() == 1);
    popScope();

    if (func) {
        std::string fname = func->getNameAsString();
        clang::PresumedLoc presumedStart = SM->getPresumedLoc(func->getLocStart());
        const int functionStartLine = presumedStart.getLine();

        OMPNode rootNode(NULL, functionStartLine, NULL, NULL, std::string("root"), SM);

        for (std::map<int, const clang::Stmt *>::iterator i = predecessors.begin(),
                e = predecessors.end(); i != e; i++) {
            const int line = i->first;
            const clang::Stmt *pred = i->second;
            assert(successors.find(line) != successors.end());
            assert(captures.find(line) != captures.end());
            const clang::Stmt *succ = successors[line];
            OMPPragma *pragma = getOMPPragmaFor(line);

            if (supportedPragmas.find(pragma->getPragmaName()) == supportedPragmas.end()) {
                std::cerr << "Encountered an unknown pragma \"" <<
                    pragma->getPragmaName() << "\" at line " << line << std::endl;
                exit(1);
            }

            if (pragma->getPragmaName() == "parallel") {
                std::string lbl = fname + std::to_string(line);
                rootNode.addChild(succ, line, pragma, lbl, SM);
            } else if (pragma->getPragmaName() == "simd") {
                // ignore and don't add to pragma tree
            } else {
                std::cerr << "Unhandled supported pragma \"" <<
                    pragma->getPragmaName() << "\"" << std::endl;
                exit(1);
            }
        }

        rootNode.print();

        std::string accumulatedStructDefs = "";
        std::string accumulatedKernelDefs = "";

        if (rootNode.nchildren() > 0) {
            std::vector<OMPNode *> *todo = new std::vector<OMPNode *>();
            std::vector<OMPNode *> *processing = rootNode.getLeaves();

            while (processing->size() != 0) {
                for (std::vector<OMPNode *>::iterator i = processing->begin(),
                        e = processing->end(); i != e; i++) {
                    OMPNode *node = *i;
                    const clang::Stmt *succ = successors[node->getPragmaLine()];
                    const clang::Stmt *pred = predecessors[node->getPragmaLine()];

                    assert(supportedPragmas.find(node->getPragma()->getPragmaName()) != supportedPragmas.end());

                    if (node->getPragma()->getPragmaName() == "parallel") {
                        std::map<std::string, std::vector<std::string> > clauses = node->getPragma()->getClauses();
                        if (clauses.find("for") != clauses.end()) {
                            const std::string structDef = getStructDef(
                                    node->getLbl(),
                                    captures[node->getPragmaLine()]);
                            accumulatedStructDefs = accumulatedStructDefs + structDef;

                            const clang::ForStmt *forLoop = clang::dyn_cast<clang::ForStmt>(node->getBody());
                            if (!forLoop) {
                                std::cerr << "Expected to find for loop " <<
                                    "inside omp parallel for at line " <<
                                    node->getPragmaLine() << " but found a " <<
                                    node->getBody()->getStmtClassName() <<
                                    " instead." << std::endl;
                                std::cerr << stmtToString(node->getBody()) << std::endl;
                                exit(1);
                            }
                            const clang::Stmt *init = forLoop->getInit();
                            const clang::Stmt *cond = forLoop->getCond();
                            const clang::Expr *inc = forLoop->getInc();

                            std::string bodyStr = stmtToString(forLoop->getBody());

                            const clang::ValueDecl *condVar = NULL;
                            std::string lowStr =
                                getCondVarAndLowerBoundFromInit(init,
                                        &condVar);
                            assert(condVar);

                            std::string highStr = getUpperBoundFromCond(
                                    cond, condVar);

                            std::string strideStr = getStrideFromIncr(inc,
                                    condVar);

                            accumulatedKernelDefs += getClosureDef(
                                    node->getLbl() + ASYNC_SUFFIX, true,
                                    node->getLbl(),
                                    captures[node->getPragmaLine()], bodyStr, condVar);

                            std::string contextCreation = "\n" +
                                getContextSetup(node->getLbl(),
                                        captures[node->getPragmaLine()]);

                            contextCreation += "hclib_loop_domain_t domain;\n";
                            contextCreation += "domain.low = " + lowStr + ";\n";
                            contextCreation += "domain.high = " + highStr + ";\n";
                            contextCreation += "domain.stride = " + strideStr + ";\n";
                            contextCreation += "domain.tile = 1;\n";
                            contextCreation += "hclib_future_t *fut = hclib_forasync_future((void *)" +
                                node->getLbl() + ASYNC_SUFFIX + ", ctx, NULL, 1, " +
                                "&domain, FORASYNC_MODE_RECURSIVE);\n";
                            contextCreation += "hclib_future_wait(fut);\n";
                            contextCreation += "free(ctx);\n";
                            // Add braces to ensure we don't change control flow
                            rewriter->ReplaceText(succ->getSourceRange(), " { " + contextCreation + " } ");
                        } else {
                            std::cerr << "Parallel pragma without a for at line " <<
                                node->getPragmaLine() << std::endl;
                            exit(1);
                        }
                    } else {
                        std::cerr << "Unhandled supported pragma \"" <<
                            node->getPragma()->getPragmaName() << "\"" << std::endl;
                        exit(1);
                    }

                    if (std::find(todo->begin(), todo->end(), node->getParent()) == todo->end() && node->getParent()->getLbl() != "root") {
                        todo->push_back(node->getParent());
                    }
                }

                processing = todo;
                todo = new std::vector<OMPNode *>();
            }
        }

        if (accumulatedStructDefs.length() > 0 ||
                accumulatedKernelDefs.length() > 0) {
            rewriter->InsertText(func->getLocStart(),
                    accumulatedStructDefs + accumulatedKernelDefs, true, true);
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

bool OMPToHClib::hasLaunchBody() {
    return launchStartLine > 0;
}

clang::SourceLocation OMPToHClib::getLaunchBodyBeginLoc() {
    assert(firstInsideLaunch);
    return firstInsideLaunch->getLocStart();
}

clang::SourceLocation OMPToHClib::getLaunchBodyEndLoc() {
    assert(lastInsideLaunch);
    return lastInsideLaunch->getLocEnd();
}

std::string OMPToHClib::getLaunchBody() {
    assert(launchStartLine > 0 && launchEndLine > 0);
    assert(firstInsideLaunch);
    assert(lastInsideLaunch);
    assert(launchCaptures);

    assert(getParent(firstInsideLaunch) == getParent(lastInsideLaunch));

    const clang::Stmt *parent = getParent(firstInsideLaunch);
    clang::Stmt::const_child_iterator i = parent->child_begin();
    while (*i != firstInsideLaunch) {
        i++;
    }
    std::stringstream ss;
    while (*i != lastInsideLaunch) {
        ss << stmtToString(*i) << "; ";
        i++;
    }
    ss << stmtToString(lastInsideLaunch) << "; " << std::flush;
    return ss.str();
}

std::vector<clang::ValueDecl *> *OMPToHClib::getLaunchCaptures() {
    assert(launchCaptures);
    return launchCaptures;
}

const clang::FunctionDecl *OMPToHClib::getFunctionContainingLaunch() {
    assert(functionContainingLaunch);
    return functionContainingLaunch;
}

void OMPToHClib::VisitStmt(const clang::Stmt *s) {
    clang::SourceLocation start = s->getLocStart();
    clang::SourceLocation end = s->getLocEnd();

    const int currentScope = getCurrentLexicalDepth();
    if (isScopeCreatingStmt(s)) {
        addNewScope();
    }

    if (start.isValid() && end.isValid() && SM->isInMainFile(end)) {
        clang::PresumedLoc presumedStart = SM->getPresumedLoc(start);
        clang::PresumedLoc presumedEnd = SM->getPresumedLoc(end);

        /*
         * Look for OMP calls and make sure the user has to replace them with
         * something appropriate.
         */
        if (const clang::CallExpr *call = clang::dyn_cast<clang::CallExpr>(s)) {
            if (call->getDirectCallee()) {
                const clang::FunctionDecl *callee = call->getDirectCallee();
                std::string calleeName = callee->getNameAsString();
                if (calleeName.find("omp_") == 0) {
                    std::cerr << "Found OMP function call to \"" <<  calleeName << "\" on line " << presumedStart.getLine() << std::endl;
                    std::cerr << "Please replace or delete, then retry to the translation" << std::endl;
                    exit(1);
                }
            }
        }

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

            if (presumedEnd.getLine() < pragma.getLine()) {
                if (predecessors.find(pragma.getLine()) ==
                        predecessors.end()) {
                    predecessors[pragma.getLine()] = s;
                } else {
                    const clang::Stmt *curr = predecessors[pragma.getLine()];
                    clang::PresumedLoc curr_loc = SM->getPresumedLoc(
                            curr->getLocEnd());
                    if (presumedEnd.getLine() > curr_loc.getLine() ||
                            (presumedEnd.getLine() == curr_loc.getLine() &&
                             presumedEnd.getColumn() > curr_loc.getColumn())) {
                        predecessors[pragma.getLine()] = s;
                    }
                }
            }

            if (presumedStart.getLine() >= pragma.getLine()) {
                if (successors.find(pragma.getLine()) == successors.end()) {
                    successors[pragma.getLine()] = s;
                    captures[pragma.getLine()] = visibleDecls();
                } else {
                    const clang::Stmt *curr = successors[pragma.getLine()];
                    clang::PresumedLoc curr_loc =
                        SM->getPresumedLoc(curr->getLocStart());
                    if (presumedStart.getLine() < curr_loc.getLine() ||
                            (presumedStart.getLine() == curr_loc.getLine() &&
                             presumedStart.getColumn() < curr_loc.getColumn())) {
                        successors[pragma.getLine()] = s;
                        captures[pragma.getLine()] = visibleDecls();
                    }
                }
            }
        }

        /*
         * If the provided file contained launch pragmas for HClib, search for
         * the first and last statements inside these pragmas.
         */
        if (launchStartLine > 0) {
            assert(launchEndLine > 0);

            if (presumedEnd.getLine() < launchEndLine) {
                if (lastInsideLaunch) {
                    size_t latestSoFar = SM->getPresumedLoc(
                            lastInsideLaunch->getLocEnd()).getLine();
                    if (presumedEnd.getLine() > latestSoFar) {
                        lastInsideLaunch = s;
                    }
                } else {
                    lastInsideLaunch = s;
                }
            }

            if (presumedStart.getLine() > launchStartLine) {
                if (firstInsideLaunch) {
                    size_t earliestSoFar = SM->getPresumedLoc(
                            firstInsideLaunch->getLocStart()).getLine();
                    if (presumedStart.getLine() < earliestSoFar) {
                        firstInsideLaunch = s;
                        launchCaptures = visibleDecls();
                        functionContainingLaunch = curr_func_decl;
                    }
                } else {
                    firstInsideLaunch = s;
                    launchCaptures = visibleDecls();
                    functionContainingLaunch = curr_func_decl;
                }
            }
        }

        /*
         * First check if this statement is a variable declaration, and if so
         * add it to the list of visible declarations in the current scope. This
         * step must be done before the block of code below for the case where
         * the last statement before an OMP pragma is a variable declaration. We
         * want to make sure that declaration is included in the capture list of
         * the following pragma.
         */
        if (const clang::DeclStmt *decls = clang::dyn_cast<clang::DeclStmt>(s)) {
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


void OMPToHClib::parseHClibPragmas(const char *filename) {
    std::ifstream fp;
    fp.open(std::string(filename), std::ios::in);
    if (!fp.is_open()) {
        std::cerr << "Failed opening " << std::string(filename) << std::endl;
        exit(1);
    }

    std::string line;

    while (getline(fp, line)) {
        size_t end = line.find(' ');
        std::string firstToken = line.substr(0, end);
        if (firstToken == "BODY") {
            line = line.substr(end + 1);
            end = line.find(' ');
            launchStartLine = atoi(line.substr(0, end).c_str());
            line = line.substr(end + 1);
            end = line.find(' ');
            launchEndLine = atoi(line.substr(0, end).c_str());
            std::cerr << "Parsed HClib body from line " << launchStartLine <<
                " to " << launchEndLine << std::endl;
        } else {
            std::cerr << "Unknown label \"" << firstToken << "\"" << std::endl;
            exit(1);
        }
    }

    fp.close();
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

OMPToHClib::OMPToHClib(const char *ompPragmaFile, const char *ompToHclibPragmaFile) {
    pragmas = parseOMPPragmas(ompPragmaFile);

    supportedPragmas.insert("parallel");
    supportedPragmas.insert("simd"); // ignore

    launchStartLine = -1;
    launchEndLine = -1;
    firstInsideLaunch = NULL;
    lastInsideLaunch = NULL;
    launchCaptures = NULL;
    functionContainingLaunch = NULL;
    parseHClibPragmas(ompToHclibPragmaFile);
}

OMPToHClib::~OMPToHClib() {
}
