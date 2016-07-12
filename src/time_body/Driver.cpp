#include <sstream>
#include <string>
#include <iostream>

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

#include "TimeBodyPass.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory ToolingSampleCategory("omp-to-hclib options");
static llvm::cl::opt<std::string> outputFile("o");
static llvm::cl::opt<std::string> enableFullProgram("f");
static llvm::cl::opt<std::string> enableParallelFor("l");

static TimeBodyPass *transform = NULL;
FunctionDecl *curr_func_decl = NULL;
std::vector<ValueDecl *> globals;
std::vector<std::string> discoveredGlobals;
int timingScope = NONE;

class TransformASTConsumer : public ASTConsumer {
public:
  TransformASTConsumer(Rewriter &setR, ASTContext &setContext) :
          R(setR), Context(setContext) { }

  void handleFunctionDecl(FunctionDecl *fdecl) {
      if (fdecl->isThisDeclarationADefinition() &&
              R.getSourceMgr().isInMainFile(fdecl->getLocation())) {

          std::cerr << "Visiting " << fdecl->getNameAsString() << std::endl;

          curr_func_decl = fdecl;
          transform->preFunctionVisit(fdecl);
          transform->Visit(fdecl->getBody());
          transform->postFunctionVisit(fdecl);
      }
  }

  // Override the method that gets called for each parsed top-level
  // declaration.
  bool HandleTopLevelDecl(DeclGroupRef DR) override {
    assert(transform != NULL);
    transform->setRewriter(R);
    transform->setContext(Context);

    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
        Decl *toplevel = *b;

        if (VarDecl *vdecl = clang::dyn_cast<VarDecl>(toplevel)) {
            clang::ValueDecl *asValue = clang::dyn_cast<clang::ValueDecl>(
                    toplevel);
            if (std::find(discoveredGlobals.begin(), discoveredGlobals.end(),
                        asValue->getNameAsString()) == discoveredGlobals.end()) {
                globals.push_back(asValue);
            }
            discoveredGlobals.push_back(asValue->getNameAsString());
        } else if (LinkageSpecDecl *ldecl = clang::dyn_cast<LinkageSpecDecl>(
                    toplevel)) {
            for (DeclContext::decl_iterator di = ldecl->decls_begin(),
                    de = ldecl->decls_end(); di != de; di++) {
                Decl *curr_linkage_decl = *di;
                if (VarDecl *vdecl = clang::dyn_cast<VarDecl>(
                            curr_linkage_decl)) {
                    clang::ValueDecl *asValue =
                        clang::dyn_cast<clang::ValueDecl>(curr_linkage_decl);
                    if (std::find(discoveredGlobals.begin(),
                                discoveredGlobals.end(),
                                asValue->getNameAsString()) ==
                            discoveredGlobals.end()) {
                        globals.push_back(asValue);
                    }
                    discoveredGlobals.push_back(asValue->getNameAsString());
                }
            }
        }
    }

    // For each top-level function defined
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
        Decl *toplevel = *b;

        if (FunctionDecl *fdecl = clang::dyn_cast<FunctionDecl>(toplevel)) {
            handleFunctionDecl(fdecl);
        } else if (FunctionTemplateDecl *tdecl = clang::dyn_cast<FunctionTemplateDecl>(toplevel)) {
            handleFunctionDecl(tdecl->getTemplatedDecl());
        } else if (LinkageSpecDecl *ldecl = clang::dyn_cast<LinkageSpecDecl>(
                    toplevel)) {
            for (DeclContext::decl_iterator di = ldecl->decls_begin(),
                    de = ldecl->decls_end(); di != de; di++) {
                Decl *curr_linkage_decl = *di;
                if (FunctionDecl *fdecl = clang::dyn_cast<FunctionDecl>(
                            curr_linkage_decl)) {
                    handleFunctionDecl(fdecl);
                }
            }
        }
    }

    return true;
  }

private:
  Rewriter &R;
  ASTContext &Context;
};

// For each source file provided to the tool, a new FrontendAction is created.
template <class c> class NumDebugFrontendAction : public ASTFrontendAction {
public:
  NumDebugFrontendAction() {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR == 8
      std::error_code EC;
      out = new llvm::raw_fd_ostream(outputFile.c_str(), EC,
              llvm::sys::fs::OpenFlags::F_None);
#else
      std::string code;
      out = new llvm::raw_fd_ostream(outputFile.c_str(), code,
              llvm::sys::fs::OpenFlags::F_None);
#endif
  }

  void EndSourceFileAction() override {
    SourceManager &SM = rewriter.getSourceMgr();
    rewriter.getEditBuffer(SM.getMainFileID()).write(*out);
    out->close();
  }

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR == 8
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
#else
  clang::ASTConsumer *CreateASTConsumer(
#endif
      CompilerInstance &CI, StringRef file) override {
    rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    clang::ASTConsumer *result = new TransformASTConsumer(rewriter,
        CI.getASTContext());
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR == 8
    return std::unique_ptr<clang::ASTConsumer>(result);
#else
    return result;
#endif
  }

private:
  llvm::raw_fd_ostream *out;
  Rewriter rewriter;
};

static void check_opt(llvm::cl::opt<std::string> &s, const char *msg) {
    if (s.size() == 0) {
        llvm::errs() << std::string(msg) << " is required\n";
        exit(1);
    }
}

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);

  check_opt(outputFile, "Output file");
  // check_opt(enableFullProgram, "Enable full program");
  // check_opt(enableParallelFor, "Enable parallel for");

  if (std::string(enableFullProgram.c_str()) == "enable") {
      timingScope |= FULL_PROGRAM;
  }

  if (std::string(enableParallelFor.c_str()) == "enable") {
      timingScope |= PARALLEL_FOR;
  }

  assert(op.getSourcePathList().size() == 1);

  std::unique_ptr<FrontendActionFactory> factory_ptr = newFrontendActionFactory<
      NumDebugFrontendAction<TransformASTConsumer>>();
  FrontendActionFactory *factory = factory_ptr.get();

  transform = new TimeBodyPass();

  ClangTool *Tool = new ClangTool(op.getCompilations(), op.getSourcePathList());
  int err = Tool->run(factory);
  if (err) {
      /*
       * Right now we ignore clang compilation errors because there are
       * situations where we insert references to undefined variables, variables
       * that will be defined on the next iteration of code generation. Is this
       * safe? Not really.
       */
      // fprintf(stderr, "Error running clang tool: %d\n", err);
      // return err;
  }

  return 0;
}
