#ifndef BRACE_INSERTER_H
#define BRACE_INSERTER_H

class BraceInserter : public clang::ConstStmtVisitor<BraceInserter> {
    public:
        BraceInserter() { }

        void setRewriter(clang::Rewriter &R) {
            rewriter = &R;
            SM = &R.getSourceMgr();
        }
        void setContext(clang::ASTContext &set_Context) {
            Context = &set_Context;
        }

        void visitChildren(const clang::Stmt *s);
        void VisitStmt(const clang::Stmt *s);
        std::string to_string(const clang::Stmt *stmt);
        const clang::FunctionDecl *getNestedFDecl(const clang::Expr *e);

    protected:
        clang::ASTContext *Context;
        clang::SourceManager *SM;
        clang::Rewriter *rewriter;
};

#endif
