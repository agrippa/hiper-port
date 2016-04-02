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

/*
 * NOTE: The one thing to be careful about whenever inserting code in an
 * existing function is that the brace inserter is not run before this pass
 * anymore. That means that extra care must be taken to ensure that converting a
 * single line of code to a multi-line block of code does not change the control
 * flow of the application. This should be kept in mind anywhere that an
 * InsertText, ReplaceText, etc API is called.
 *
 * NOTE: In general, each rewriter use is followed by an assertion that it
 * should not fail. Rewrites can fail if the target location is invalid, e.g.
 * the target location is in a macro. So if you start getting these assert
 * failures, it's usually simpler to change the application code.
 */

extern clang::FunctionDecl *curr_func_decl;

#define ASYNC_SUFFIX "_hclib_async"

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
    // std::string s;
    // llvm::raw_string_ostream stream(s);
    // stmt->printPretty(stream, NULL, Context->getPrintingPolicy());
    // stream.flush();
    // return s;
    return rewriter->getRewrittenText(stmt->getSourceRange());
}

std::string OMPToHClib::stringForAST(const clang::Stmt *stmt) {
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
        if (const clang::ConstantArrayType *constantArrayType =
                clang::dyn_cast<clang::ConstantArrayType>(arrayType)) {
            std::string sizeStr = constantArrayType->getSize().toString(10,
                    true);
            return getDeclarationTypeStr(constantArrayType->getElementType(),
                    name, soFarBefore, soFarAfter + "[" + sizeStr + "]");
        } else {
            std::cerr << "Unsupported array type " <<
                std::string(type->getTypeClassName()) << " while building " <<
                "declaration for " << name << std::endl;
            exit(1);
        }
    } else if (const clang::PointerType *pointerType =
            type->getAs<clang::PointerType>()) {
        return getDeclarationTypeStr(pointerType->getPointeeType(), name,
                soFarBefore + "*", soFarAfter);
    } else if (const clang::BuiltinType *builtinType =
            type->getAs<clang::BuiltinType>()) {
        return qualType.getAsString() + " " + soFarBefore + name + soFarAfter;
    } else if (const clang::TypedefType *typedefType =
            type->getAs<clang::TypedefType>()) {
        return typedefType->getDecl()->getNameAsString() + " " + soFarBefore +
            name + soFarAfter;
    } else if (const clang::ElaboratedType *elaboratedType =
            type->getAs<clang::ElaboratedType>()) {
        return elaboratedType->getNamedType().getAsString() + " " +
            soFarBefore + name + soFarAfter;
    } else if (const clang::TagType *tagType = type->getAs<clang::TagType>()) {
        clang::TagDecl *decl = tagType->getDecl();

        std::string tagName;
        if (tagType->getDecl()->getTypedefNameForAnonDecl()) {
            tagName = decl->getTypedefNameForAnonDecl()->getNameAsString();
        } else if (tagType->getDecl()->getDeclName()) {
            tagName = decl->getDeclName().getAsString() ;
        } else {
            assert(false);
        }

        return tagName + " " + soFarBefore + name + soFarAfter;
    } else {
        std::cerr << "Unsupported type " <<
            std::string(type->getTypeClassName()) << std::endl;
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
        std::string sizeStr;
        if (const clang::ConstantArrayType *constantArrayType = clang::dyn_cast<clang::ConstantArrayType>(arrayType)) {
            sizeStr = constantArrayType->getSize().toString(10, true);
        } else {
            std::cerr << "Unsupported array type " <<
                std::string(type->getTypeClassName()) << std::endl;
            exit(1);
        }
        return sizeStr + " * (" + getArraySizeExpr(arrayType->getElementType()) + ")";
    } else if (const clang::BuiltinType *builtinType = type->getAs<clang::BuiltinType>()) {
        return "sizeof(" + qualType.getAsString() + ")";
    } else if (const clang::DecayedType *decayedType = type->getAs<clang::DecayedType>()) {
        return getArraySizeExpr(decayedType->getOriginalType());
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

    if (type->getAs<clang::DecayedType>()) {
        type = type->getAs<clang::DecayedType>()->getPointeeType().getTypePtr();
    }

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
        std::vector<clang::ValueDecl *> *captured, bool hasReductions) {
    std::string struct_def = "typedef struct _" + structName + " {\n";

    for (std::vector<clang::ValueDecl *>::iterator ii =
            captured->begin(), ee = captured->end();
            ii != ee; ii++) {
        clang::ValueDecl *curr = *ii;
        struct_def += "    " + getDeclarationStr(curr) + "\n";
    }

    if (hasReductions) {
        struct_def += "    pthread_mutex_t reduction_mutex;\n";
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

    clang::PresumedLoc presumedStart = SM->getPresumedLoc(
            func->getLocStart());
    const int functionStartLine = presumedStart.getLine();
    pragmaTree = new PragmaNode(NULL, func->getBody(),
            new std::vector<clang::ValueDecl *>(), SM);
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
    } else if (const clang::DeclStmt *decl = clang::dyn_cast<clang::DeclStmt>(init)) {
        assert(decl->isSingleDecl());

        const clang::ValueDecl *val = clang::dyn_cast<clang::ValueDecl>(decl->getSingleDecl());
        assert(val);
        *condVar = val;

        const clang::VarDecl *var = clang::dyn_cast<clang::VarDecl>(val);
        assert(var);
        assert(var->hasInit());
        const clang::Expr *initialValue = var->getInit();
        return stmtToString(initialValue);
    } else {
        std::cerr << "Unsupported init class " <<
            std::string(init->getStmtClassName()) << std::endl;
        exit(1);
    }
}

std::string OMPToHClib::getUpperBoundFromCond(const clang::Stmt *cond,
        const clang::ValueDecl *condVar) {
    if (const clang::BinaryOperator *bin = clang::dyn_cast<clang::BinaryOperator>(cond)) {
        clang::Expr *lhs = unwrapCasts(bin->getLHS());
        const clang::DeclRefExpr *declref = clang::dyn_cast<clang::DeclRefExpr>(lhs);
        assert(declref);
        const clang::ValueDecl *decl = declref->getDecl();
        assert(condVar == decl);

        if (bin->getOpcode() == clang::BO_LT) {
            return stringForAST(bin->getRHS());
        } else if (bin->getOpcode() == clang::BO_LE) {
            return "(" + stringForAST(bin->getRHS()) + ") + 1"; // assumes integral values?
        } else {
            std::cerr << "Unsupported binary operator " << bin->getOpcode() << std::endl;
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

std::string OMPToHClib::getClosureDecl(std::string closureName,
        bool isForasyncClosure) {
    std::stringstream ss;
    ss << "static void " << closureName << "(void *____arg";
    if (isForasyncClosure) {
        ss << ", const int ___iter";
    }
    ss << ");";
    return ss.str();
}

std::string OMPToHClib::getClosureDef(std::string closureName,
        bool isForasyncClosure, bool isAsyncClosure,
        std::string contextName, std::vector<clang::ValueDecl *> *captured,
        std::string bodyStr, std::vector<OMPReductionVar> *reductions,
        const clang::ValueDecl *condVar) {
    assert(!(isForasyncClosure && isAsyncClosure));

    std::stringstream ss;
    ss << "static void " << closureName << "(void *____arg";
    if (isForasyncClosure) {
        ss << ", const int ___iter";
    }
    ss << ") {\n";

    ss << "    " << contextName << " *ctx = (" << contextName << " *)____arg;\n";
    for (std::vector<clang::ValueDecl *>::iterator ii = captured->begin(),
            ee = captured->end(); ii != ee; ii++) {
        clang::ValueDecl *curr = *ii;
        ss << "    " << getUnpackStr(curr) << "\n";
    }

    if (isAsyncClosure || isForasyncClosure) {
        /*
         * In OMP's task model you can use taskwait to wait on all previously
         * created child tasks of the currently executing task. To model this in
         * HClib, we wrap the body of each async in a finish scope and on
         * encountering a taskwait call hclib_end_finish followed by
         * hclib_start_finish.
         */
        ss << "    hclib_start_finish();\n";
    }

    if (isForasyncClosure) {
        /*
         * Insert a one iteration do-loop around the original body so that
         * continues have the same semantics and in case the condition variable
         * declared below has the same name as a captured variable.
         */
        ss << "    do {\n";

        /*
         * In the case of C++ style loops with the iterator variable declared inside
         * the initialization clause, make sure the condition variable still gets
         * added into the body of the kernel.
         */
        if (std::find(captured->begin(), captured->end(), condVar) == captured->end()) {
            ss << "    " << getDeclarationTypeStr(condVar->getType(),
                    condVar->getNameAsString(), "", "") << "; ";
        }
        ss << "    " << condVar->getNameAsString() << " = ___iter;\n";
    }

    ss << bodyStr << " ; ";

    if (isForasyncClosure) {
        ss << "    } while (0);\n";
        if (!reductions->empty()) {
            ss << "    const int lock_err = pthread_mutex_lock(&ctx->reduction_mutex);\n";
            ss << "    assert(lock_err == 0);\n";

            for (std::vector<OMPReductionVar>::iterator i = reductions->begin(),
                    e = reductions->end(); i != e; i++) {
                OMPReductionVar red = *i;
                std::string varname = red.getVar();

                ss << "    ctx->" << varname << " " << red.getOp() << "= " <<
                    varname << ";\n";
            }

            ss << "    const int unlock_err = pthread_mutex_unlock(&ctx->reduction_mutex);\n";
            ss << "    assert(unlock_err == 0);\n";
        }
    }
    if (isAsyncClosure || isForasyncClosure) {
        ss << "    ; hclib_end_finish();\n";
    }
    ss << "}\n\n";
    return ss.str();
}

std::string OMPToHClib::getContextSetup(std::string structName,
        std::vector<clang::ValueDecl *> *captured,
        std::vector<OMPReductionVar> *reductions) {
    std::stringstream ss;
    ss << structName << " *ctx = (" << structName << " *)malloc(sizeof(" <<
        structName << "));\n";

    for (std::vector<clang::ValueDecl *>::iterator ii = captured->begin(),
            ee = captured->end(); ii != ee; ii++) {
        clang::ValueDecl *curr = *ii;
        ss << getCaptureStr(curr) << "\n";
    }

    if (reductions && !reductions->empty()) {
        for (std::vector<OMPReductionVar>::iterator i = reductions->begin(),
                e = reductions->end(); i != e; i++) {
            OMPReductionVar red = *i;
            // Initialize the reduction target based on the reduction op
            ss << "ctx->" << red.getVar() << " = " << red.getInitialValue() <<
                ";\n";
        }
        ss << "const int init_err = pthread_mutex_init(&ctx->reduction_mutex, NULL);\n";
        ss << "assert(init_err == 0);\n";
    }

    return ss.str();
}

void OMPToHClib::postFunctionVisit(clang::FunctionDecl *func) {
    assert(getCurrentLexicalDepth() == 1);
    popScope();

    if (func) {
        std::string fname = func->getNameAsString();
        clang::PresumedLoc presumedStart = SM->getPresumedLoc(
                func->getLocStart());
        const int functionStartLine = presumedStart.getLine();

        pragmaTree->print();

        std::string accumulatedStructDefs = "";
        std::string accumulatedKernelDecls = "";
        std::string accumulatedKernelDefs = "";

        std::vector<PragmaNode *> *leaves = pragmaTree->getLeaves();

        for (std::vector<PragmaNode *>::iterator i = leaves->begin(),
                e = leaves->end(); i != e; i++) {
            PragmaNode *node = *i;
            std::string pragmaName = getPragmaNameForMarker(node->getMarker());

            if (pragmaName == "omp_to_hclib") {
                if (foundOmpToHclibLaunch) {
                    std::cerr << "Found multiple locations where the " <<
                        "omp_to_hclib pragma is used. This use case is " <<
                        "currently unsupported." << std::endl;
                    exit(1);
                }
                foundOmpToHclibLaunch = true;

                std::string launchBody = stmtToString(node->getBody());
                std::string launchStruct = getStructDef(
                        "main_entrypoint_ctx", node->getCaptures(), false);
                std::string closureFunction = getClosureDef(
                        "main_entrypoint", false, false, "main_entrypoint_ctx",
                        node->getCaptures(), launchBody);
                std::string contextSetup = getContextSetup(
                        "main_entrypoint_ctx", node->getCaptures(), NULL);
                std::string launchStr = contextSetup +
                    "hclib_launch(main_entrypoint, ctx);\n" +
                    "free(ctx);\n";

                bool failed = rewriter->ReplaceText(
                        clang::SourceRange(node->getStartLoc(),
                            node->getEndLoc()), launchStr);
                assert(!failed);

                accumulatedStructDefs += launchStruct;
                accumulatedKernelDecls += closureFunction;
            } else if (pragmaName == "omp") {
                std::string ompCmd = getOMPPragmaNameForMarker(node->getMarker());
                OMPClauses *clauses = getOMPClausesForMarker(node->getMarker());

                if (ompCmd == "critical" || ompCmd == "atomic") {
                    // For now we implement atomic with rather heavyweight locks
                    const clang::Stmt *body = node->getBody();

                    std::stringstream lock_ss;
                    lock_ss << " { const int ____lock_" << criticalSectionId <<
                        "_err = pthread_mutex_lock(&critical_" <<
                        criticalSectionId << "_lock); assert(____lock_" <<
                        criticalSectionId << "_err == 0); ";

                    std::stringstream unlock_ss;
                    unlock_ss << "; const int ____unlock_" <<
                        criticalSectionId << "_err = " <<
                        "pthread_mutex_unlock(&critical_" <<
                        criticalSectionId << "_lock); assert(____unlock_" <<
                        criticalSectionId << "_err); } ";

                    criticalSectionId++;

                    const bool failed = rewriter->ReplaceText(
                            clang::SourceRange(node->getStartLoc(), node->getEndLoc()),
                            lock_ss.str() + stmtToString(body) + unlock_ss.str());
                    assert(!failed);
                } else if (ompCmd == "taskwait") {
                    const bool failed = rewriter->ReplaceText(
                            clang::SourceRange(node->getStartLoc(), node->getEndLoc()),
                            " hclib_end_finish(); hclib_start_finish(); ");
                    assert(!failed);
                } else if (ompCmd == "single") {
                    assert(node->getParent()); // should not be root
                    assert(getPragmaNameForMarker(node->getParent()->getMarker()) == "omp");

                    std::string parentCmd = getOMPPragmaNameForMarker(
                            node->getParent()->getMarker());
                    if (parentCmd != "parallel") {
                        std::cerr << "It appears the \"single\" pragma " <<
                            "on line " << node->getStartLine() <<
                            " does not have a \"parallel\" pragma as " <<
                            "its parent. Instead has \"" << parentCmd <<
                            "\" at line " <<
                            node->getParent()->getStartLine() << "." <<
                            std::endl;
                        node->getParent()->print();
                        exit(1);
                    }

                    const clang::CompoundStmt *parentCmpd =
                        clang::dyn_cast<clang::CompoundStmt>(
                                node->getParent()->getBody());
                    assert(parentCmpd);
                    assert(parentCmpd->size() == 2);
                    clang::CompoundStmt::const_body_iterator bodyIter = parentCmpd->body_begin();
                    assert(*bodyIter == node->getMarker());
                    bodyIter++;
                    assert(*bodyIter == node->getBody());

                    /*
                     * Verify that at least one of these pragmas does not
                     * have the nowait clause.
                     */
                    OMPClauses *parentClauses = getOMPClausesForMarker(
                            node->getParent()->getMarker());
                    assert(!clauses->hasClause("nowait") ||
                            !parentClauses->hasClause("nowait"));

                    // Insert finish
                    const clang::Stmt *body = node->getBody();

                    const bool failed = rewriter->ReplaceText(
                            clang::SourceRange(node->getParent()->getStartLoc(), node->getParent()->getEndLoc()),
                            "hclib_start_finish(); " + stmtToString(body) + " ; hclib_end_finish(); ");
                    assert(!failed);
                } else if (ompCmd == "task") {
                    const clang::Stmt *body = node->getBody();
                    std::string bodyStr = stmtToString(body);

                    accumulatedKernelDecls += getClosureDecl(
                            node->getLbl() + ASYNC_SUFFIX, false);

                    accumulatedKernelDefs += getClosureDef(
                            node->getLbl() + ASYNC_SUFFIX, false, true,
                            node->getLbl(), node->getCaptures(),
                            bodyStr, NULL, NULL);

                    const std::string structDef = getStructDef(
                            node->getLbl(), node->getCaptures(),
                            false);
                    accumulatedStructDefs += structDef;

                    std::stringstream contextCreation;
                    contextCreation << "\n" <<
                        getContextSetup(node->getLbl(), node->getCaptures(),
                                NULL);
                    OMPClauses *clauses = getOMPClausesForMarker(node->getMarker());
                    if (clauses->hasClause("if")) {
                        // We have already asserted there is only one if arg
                        std::string ifArg = clauses->getSingleArg("if");
                        // If the if condition is false, just call the task body sequentially
                        contextCreation << "if (!(" << ifArg << ")) {\n";
                        contextCreation << "    " << node->getLbl() << ASYNC_SUFFIX << "(ctx);\n";
                        contextCreation << "} else {\n";
                    }
                    contextCreation << "hclib_async(" << node->getLbl() <<
                        ASYNC_SUFFIX << ", ctx, NO_FUTURE, NO_PHASER, " <<
                        "ANY_PLACE);\n";
                    if (clauses->hasClause("if")) {
                        contextCreation << "}\n";
                    }

                    // Add braces to ensure we don't change control flow
                    const bool failed = rewriter->ReplaceText(
                            clang::SourceRange(node->getStartLoc(), node->getEndLoc()),
                            " { " + contextCreation.str() + " } ");
                    assert(!failed);
                } else if (ompCmd == "parallel") {
                    OMPClauses *clauses = getOMPClausesForMarker(node->getMarker());

                    if (clauses->hasClause("for")) {

                        const clang::ForStmt *forLoop = clang::dyn_cast<clang::ForStmt>(node->getBody());
                        if (!forLoop) {
                            std::cerr << "Expected to find for loop " <<
                                "inside omp parallel for at line " <<
                                node->getStartLine() << " but found a " <<
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

                        accumulatedKernelDecls += getClosureDecl(
                                node->getLbl() + ASYNC_SUFFIX, true);

                        std::vector<OMPReductionVar> *reductions =
                            getReductions(clauses);

                        accumulatedKernelDefs += getClosureDef(
                                node->getLbl() + ASYNC_SUFFIX, true, false,
                                node->getLbl(), node->getCaptures(),
                                bodyStr, reductions, condVar);

                        const std::string structDef = getStructDef(
                                node->getLbl(), node->getCaptures(),
                                !reductions->empty());
                        accumulatedStructDefs += structDef;

                        std::stringstream contextCreation;
                        contextCreation << "\n" <<
                            getContextSetup(node->getLbl(), node->getCaptures(),
                                    reductions);

                        contextCreation << "hclib_loop_domain_t domain;\n";
                        contextCreation << "domain.low = " << lowStr << ";\n";
                        contextCreation << "domain.high = " << highStr << ";\n";
                        contextCreation << "domain.stride = " << strideStr << ";\n";
                        contextCreation << "domain.tile = 1;\n";
                        contextCreation << "hclib_future_t *fut = hclib_forasync_future((void *)" <<
                            node->getLbl() << ASYNC_SUFFIX << ", ctx, NULL, 1, " <<
                            "&domain, FORASYNC_MODE_RECURSIVE);\n";
                        contextCreation << "hclib_future_wait(fut);\n";
                        contextCreation << "free(ctx);\n";

                        for (std::vector<OMPReductionVar>::iterator i =
                                reductions->begin(), e = reductions->end();
                                i != e; i++) {
                            OMPReductionVar var = *i;
                            contextCreation << var.getVar() << " = " <<
                                "ctx->" << var.getVar() << ";\n";
                        }

                        // Add braces to ensure we don't change control flow
                        const bool failed = rewriter->ReplaceText(
                                clang::SourceRange(node->getStartLoc(), node->getEndLoc()),
                                " { " + contextCreation.str() + " } ");
                        assert(!failed);
                    } else {
                        std::cerr << "Parallel pragma without a for at line " <<
                            node->getStartLine() << std::endl;
                        exit(1);
                    }
                } else if (ompCmd == "simd") {
                    // Do nothing for now, just delete this pragma
                    std::string bodyStr = stmtToString(node->getBody());
                    const bool failed = rewriter->ReplaceText(clang::SourceRange(
                                node->getStartLoc(), node->getEndLoc()), bodyStr);
                    assert(!failed);
                } else {
                    std::cerr << "Unhandled OMP command \"" << ompCmd << "\"" <<
                        std::endl;
                    exit(1);
                }
            } else if (pragmaName == "root") {
                // Nothing to do once we get to the top of the pragma tree
            } else {
                std::cerr << "Unhandled pragma type \"" << pragmaName << "\"" <<
                    std::endl;
                exit(1);
            }
        }

        if (accumulatedStructDefs.length() > 0 ||
                accumulatedKernelDefs.length() > 0) {

            bool failed = rewriter->InsertText(func->getLocStart(),
                    accumulatedStructDefs + accumulatedKernelDecls, true, true);
            assert(!failed);
            failed = rewriter->ReplaceText(func->getLocEnd(), 1,
                    "} " + accumulatedKernelDefs);
            assert(!failed);
        }
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

std::string OMPToHClib::getPragmaNameForMarker(const clang::CallExpr *call) {
    if (call == NULL) {
        // Root node only
        return "root";
    }
    assert(call->getNumArgs() == 2);
    const clang::Expr *pragmaNameArg = call->getArg(0);
    while (clang::isa<clang::ImplicitCastExpr>(pragmaNameArg)) {
        pragmaNameArg = clang::dyn_cast<clang::ImplicitCastExpr>(pragmaNameArg)->getSubExpr();
    }
    const clang::StringLiteral *literal = clang::dyn_cast<clang::StringLiteral>(pragmaNameArg);
    assert(literal);
    std::string pragmaName = literal->getString().str();
    return pragmaName;
}

std::string OMPToHClib::getPragmaArgumentsForMarker(const clang::CallExpr *call) {
    assert(call->getNumArgs() == 2);
    const clang::Expr *pragmaArgsArg = call->getArg(1);
    while (clang::isa<clang::ImplicitCastExpr>(pragmaArgsArg)) {
        pragmaArgsArg = clang::dyn_cast<clang::ImplicitCastExpr>(pragmaArgsArg)->getSubExpr();
    }
    const clang::StringLiteral *literal = clang::dyn_cast<clang::StringLiteral>(pragmaArgsArg);
    assert(literal);
    return literal->getString().str();
}

const clang::Stmt *OMPToHClib::getBodyFrom(const clang::CallExpr *call,
        std::string lbl) {
    const clang::Stmt *parent = getParent(call);
    const clang::Stmt *body = NULL;

    if (const clang::CompoundStmt *compound =
            clang::dyn_cast<clang::CompoundStmt>(parent)) {
        clang::CompoundStmt::const_body_iterator i = compound->body_begin();
        clang::CompoundStmt::const_body_iterator e = compound->body_end();
        while (i != e) {
            const clang::Stmt *curr = *i;
            if (curr == call) {
                i++;
                body = *i;
                break;
            }
            i++;
        }
    } else {
        std::cerr << "Unhandled parent while looking for body: " <<
            parent->getStmtClassName() << " of " << lbl << std::endl;
        exit(1);
    }

    assert(body);
    return body;
}

const clang::Stmt *OMPToHClib::getBodyForMarker(const clang::CallExpr *call) {
    std::string pragmaName = getPragmaNameForMarker(call);
    std::string pragmaArgs = getPragmaArgumentsForMarker(call);

    if (pragmaName == "omp") {
        size_t ompPragmaNameEnd = pragmaArgs.find(' ');
        std::string ompPragma;

        if (ompPragmaNameEnd == std::string::npos) {
            // No clauses
            ompPragma = pragmaArgs;
        } else {
            ompPragma = pragmaArgs.substr(0, ompPragmaNameEnd);
        }

        if (ompPragma == "taskwait") {
            return NULL;
        } else if (ompPragma == "task" || ompPragma == "critical" ||
                ompPragma == "atomic" || ompPragma == "parallel" ||
                ompPragma == "single" || ompPragma == "simd") {
            return getBodyFrom(call, ompPragma);
        } else {
            std::cerr << "Unhandled OMP pragma \"" << ompPragma << "\"" << std::endl;
            exit(1);
        }
    } else if (pragmaName == "omp_to_hclib") {
        return getBodyFrom(call, "omp_to_hclib");
    } else {
        std::cerr << "Unhandled pragma name \"" << pragmaName << "\"" <<
            std::endl;
        exit(1);
    }
}

std::string OMPToHClib::getOMPPragmaNameForMarker(const clang::CallExpr *call) {
    std::string pragmaName = getPragmaNameForMarker(call);
    assert(pragmaName == "omp");

    std::string pragmaArgs = getPragmaArgumentsForMarker(call);
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

OMPClauses *OMPToHClib::getOMPClausesForMarker(const clang::CallExpr *call) {
    std::string pragmaName = getPragmaNameForMarker(call);
    assert(pragmaName == "omp");

    std::string pragmaArgs = getPragmaArgumentsForMarker(call);

    size_t ompPragmaNameEnd = pragmaArgs.find(' ');
    std::string ompPragma;

    if (ompPragmaNameEnd != std::string::npos) {
        // Some clauses, non-empty args
        std::string clauses = pragmaArgs.substr(ompPragmaNameEnd + 1);
        return new OMPClauses(clauses);
    } else {
        return new OMPClauses();
    }
}

std::vector<OMPReductionVar> *OMPToHClib::getReductions(OMPClauses *clauses) {
    std::vector<OMPReductionVar> *reductions = new std::vector<OMPReductionVar>();
    if (clauses->hasClause("reduction")) {
        std::vector<SingleClauseArgs *> *args = clauses->getArgs("reduction");
        for (std::vector<SingleClauseArgs *>::iterator i = args->begin(),
                e = args->end(); i != e; i++) {
            SingleClauseArgs *single_arg = *i;
            std::vector<std::string> *raw_args = single_arg->getArgs();

            std::string op = raw_args->at(0);
            assert(op == "+");
            for (int v = 1; v < raw_args->size(); v++) {
                reductions->push_back(OMPReductionVar(op, raw_args->at(v)));
            }
        }
    }
    return reductions;
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
                bool abort = false;
                if (calleeName.find("omp_") == 0) {
                    std::cerr << "Found OMP function call to \"" <<
                        calleeName << "\" on line " <<
                        presumedStart.getLine() << std::endl;
                    abort = true;
                } else if (checkForPthread && calleeName.find("pthread_") == 0 &&
                        std::find(compatiblePthreadAPIs.begin(),
                            compatiblePthreadAPIs.end(), calleeName) ==
                        compatiblePthreadAPIs.end()) {
                    std::cerr << "Found pthread function call to \"" <<
                        calleeName << "\" on line " <<
                        presumedStart.getLine() << std::endl;
                    abort = true;
                }

                if (abort) {
                    std::cerr << "Please replace or delete, then retry the " <<
                        "translation" << std::endl;
                    exit(1);
                }
            }
        }

        if (const clang::CallExpr *call = clang::dyn_cast<clang::CallExpr>(s)) {
            if (call->getDirectCallee()) {
                const clang::FunctionDecl *callee = call->getDirectCallee();
                std::string calleeName = callee->getNameAsString();

                if (calleeName == "hclib_pragma_marker") {
                    const clang::Stmt *body = getBodyForMarker(call);
                    std::vector<clang::ValueDecl *> *captures = visibleDecls();
                    PragmaNode *node = new PragmaNode(call, body, captures, SM);
                    pragmaTree->addChild(node);
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

OMPToHClib::OMPToHClib(const char *checkForPthreadStr,
        const char *startCriticalSectionId) {

    if (strcmp(checkForPthreadStr, "true") == 0) {
        checkForPthread = true;
    } else if (strcmp(checkForPthreadStr, "false") == 0) {
        checkForPthread = false;
    } else {
        std::cerr << "Invalid value \"" << std::string(checkForPthreadStr) <<
            "\" provided for -n" << std::endl;
        exit(1);
    }

    criticalSectionId = atoi(startCriticalSectionId);

    compatiblePthreadAPIs.push_back("pthread_mutex_init");
    compatiblePthreadAPIs.push_back("pthread_mutex_lock");
    compatiblePthreadAPIs.push_back("pthread_mutex_unlock");
}

OMPToHClib::~OMPToHClib() {
}
