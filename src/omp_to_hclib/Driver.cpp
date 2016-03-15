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

#include "OMPToHClib.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory ToolingSampleCategory("omp-to-hclib options");
static llvm::cl::opt<std::string> outputFile("o",
        llvm::cl::desc("Output file"), llvm::cl::value_desc("outputFile"));
static llvm::cl::opt<std::string> ompPragmaFile("m",
        llvm::cl::desc("OpenMP file"), llvm::cl::value_desc("ompInfo"));
static llvm::cl::opt<std::string> ompToHclibPragmaFile("s",
        llvm::cl::desc("OMP-to-hclib file"), llvm::cl::value_desc("ompToHclibInfo"));

static OMPToHClib *transform = NULL;
FunctionDecl *curr_func_decl = NULL;

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

    // For each top-level function defined
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
        Decl *toplevel = *b;

        if (FunctionDecl *fdecl = clang::dyn_cast<FunctionDecl>(toplevel)) {
            handleFunctionDecl(fdecl);
        } else if (LinkageSpecDecl *ldecl = clang::dyn_cast<LinkageSpecDecl>(toplevel)) {
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
      std::string code;
      out = new llvm::raw_fd_ostream(outputFile.c_str(), code,
              llvm::sys::fs::OpenFlags::F_None);
  }

  void EndSourceFileAction() override {
    if (transform->hasLaunchBody()) {
        std::string launchBody = transform->getLaunchBody();
        std::string launchStruct = transform->getStructDef(
                "main_entrypoint_ctx", transform->getLaunchCaptures());
        std::string closureFunction = transform->getClosureDef(
                "main_entrypoint", false, "main_entrypoint_ctx",
                transform->getLaunchCaptures(), launchBody);
        std::string contextSetup = transform->getContextSetup(
                "main_entrypoint_ctx", transform->getLaunchCaptures());
        std::string launchStr = contextSetup +
            "hclib_launch(NULL, NULL, main_entrypoint, ctx);\n" +
            "free(ctx);\n";

        rewriter.InsertText(transform->getFunctionContainingLaunch()->getLocStart(),
                launchStruct + closureFunction, true, true);
        rewriter.ReplaceText(SourceRange(transform->getLaunchBodyBeginLoc(),
                    transform->getLaunchBodyEndLoc()), launchStr);
    }

    SourceManager &SM = rewriter.getSourceMgr();
    rewriter.getEditBuffer(SM.getMainFileID()).write(*out);
    out->close();
  }

  clang::ASTConsumer *CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    rewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return new TransformASTConsumer(rewriter, CI.getASTContext());
  }

private:
  llvm::raw_fd_ostream *out;
  Rewriter rewriter;
};

static void check_opt(llvm::cl::opt<std::string> s, const char *msg) {
    if (s.size() == 0) {
        llvm::errs() << std::string(msg) << " is required\n";
        exit(1);
    }
}

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);

  check_opt(outputFile, "Output file");
  check_opt(ompPragmaFile, "OpenMP pragma file");
  check_opt(ompToHclibPragmaFile, "OMP-to-HClib pragma file");

  assert(op.getSourcePathList().size() == 1);

  std::unique_ptr<FrontendActionFactory> factory_ptr = newFrontendActionFactory<
      NumDebugFrontendAction<TransformASTConsumer>>();
  FrontendActionFactory *factory = factory_ptr.get();

  transform = new OMPToHClib(ompPragmaFile.c_str(), ompToHclibPragmaFile.c_str());

  ClangTool *Tool = new ClangTool(op.getCompilations(), op.getSourcePathList());
  int err = Tool->run(factory);
  if (err) {
      fprintf(stderr, "Error running clang tool: %d\n", err);
      return err;
  }

  return 0;
}
