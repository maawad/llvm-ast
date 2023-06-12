#include <fstream>
#include <iostream>
#include <string>

#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

// Escape special characters in a label string
std::string escapeLabel(const std::string &label) {
  std::string escapedLabel = label;
  size_t pos = 0;
  while ((pos = escapedLabel.find('"', pos)) != std::string::npos) {
    escapedLabel.insert(pos, "\\");
    pos += 2;
  }
  return escapedLabel;
}

// Define a custom AST visitor to print the AST in DOT format
class ASTDOTVisitor : public RecursiveASTVisitor<ASTDOTVisitor> {
  raw_ostream &Out;
  ASTContext *Context;  // Store the ASTContext

 public:
  explicit ASTDOTVisitor(raw_ostream &Out) : Out(Out) {
    Out << "digraph G {\n";
  }
  void setASTContext(ASTContext &Context) { this->Context = &Context; }
  ~ASTDOTVisitor() { Out << "}\n"; }

  // Get a valid node name by removing special characters
  std::string getValidNodeName(const std::string &nodeName) {
    std::string validNodeName = nodeName;
    std::replace_if(
        validNodeName.begin(), validNodeName.end(),
        [](char c) { return !isalnum(c); }, '_');
    return validNodeName;
  }

  bool VisitCallExpr(CallExpr *callExpr) { return true; }

  bool VisitFunctionDecl(FunctionDecl *func) {
    std::string functionName =
        escapeLabel(func->getNameInfo().getName().getAsString());
    std::string validFunctionName = getValidNodeName(functionName);
    QualType returnType = func->getReturnType();
    std::string returnTypeName = returnType.getAsString();
    std::string escapedReturnTypeName = escapeLabel(returnTypeName);

    Out << "node_" << validFunctionName << " [shape=box, label=\""
        << escapedReturnTypeName << ' ' << functionName << "\"];\n";

    for (const ParmVarDecl *param : func->parameters()) {
      std::string paramName = escapeLabel(param->getNameAsString());
      std::string validParamName = getValidNodeName(paramName);
      Out << "node_" << validParamName << " [shape=box, label=\"" << paramName
          << "\"];\n";
      Out << "node_" << validFunctionName << " -> node_" << validParamName
          << ";\n";
    }

    if (const Stmt *body = func->getBody()) {
      for (const Stmt *stmt : body->children()) {
        if (const DeclStmt *declStmt = dyn_cast<DeclStmt>(stmt)) {
          for (const Decl *decl : declStmt->decls()) {
            if (const VarDecl *varDecl = dyn_cast<VarDecl>(decl)) {
              std::string varName = escapeLabel(varDecl->getNameAsString());
              std::string validVarName = getValidNodeName(varName);
              Out << "node_" << validVarName << " [shape=box, label=\""
                  << varName << "\"];\n";
              Out << "node_" << validFunctionName << " -> node_" << validVarName
                  << ";\n";
            }
          }
        }

        if (const BinaryOperator *binaryOp = dyn_cast<BinaryOperator>(stmt)) {
          if (binaryOp->isAssignmentOp()) {
            // Get the left-hand side (LHS) and right-hand side (RHS) of the
            // assignment
            const Expr *lhs = binaryOp->getLHS();
            const Expr *rhs = binaryOp->getRHS();

            if (const DeclRefExpr *lhsDeclRef = dyn_cast<DeclRefExpr>(lhs)) {
              const ValueDecl *lhsDecl = lhsDeclRef->getDecl();
              std::string lhsName =
                  getValidNodeName(lhsDecl->getNameAsString());

              if (const IntegerLiteral *rhsLiteral =
                      dyn_cast<IntegerLiteral>(rhs)) {
                llvm::APInt rhsValue = rhsLiteral->getValue();
                llvm::SmallString<10> rhsValueStr;
                rhsValue.toStringUnsigned(rhsValueStr, 10);

                Out << "Variable: " << lhsName << " [shape=box];\n";
                Out << "node_" << validFunctionName
                    << " -> Variable: " << lhsName << ";\n";
                Out << "Operator: " << validFunctionName
                    << " [shape=diamond];\n";
                Out << "Constant: " << rhsValueStr << " [shape=box];\n";
                Out << "Operator: " << validFunctionName
                    << " -> Constant: " << rhsValueStr << ";\n";
              }
            }
          } else if (binaryOp->isAdditiveOp()) {
            Expr *lhs = binaryOp->getLHS();
            Expr *rhs = binaryOp->getRHS();

            // Visit the left-hand side and right-hand side expressions
            TraverseStmt(lhs);
            TraverseStmt(rhs);

            std::string lhsName, rhsName;
            if (const DeclRefExpr *lhsDeclRef = dyn_cast<DeclRefExpr>(lhs)) {
              const ValueDecl *lhsDecl = lhsDeclRef->getDecl();
              lhsName = getValidNodeName(lhsDecl->getNameAsString());
            }
            if (const DeclRefExpr *rhsDeclRef = dyn_cast<DeclRefExpr>(rhs)) {
              const ValueDecl *rhsDecl = rhsDeclRef->getDecl();
              rhsName = getValidNodeName(rhsDecl->getNameAsString());
            }

            // Add the operator node and connect it to the left-hand side and
            // right-hand side nodes
            Out << "Operator: " << lhsName << " + " << rhsName
                << " [shape=diamond];\n";
            Out << "node_" << lhsName << " -> Operator: " << lhsName << " + "
                << rhsName << ";\n";
            Out << "node_" << rhsName << " -> Operator: " << lhsName << " + "
                << rhsName << ";\n";
          }
        }
      }
    }

    // Include return value if the function is not void
    if (!returnType->isVoidType()) {
      std::string validReturnName = getValidNodeName(functionName + "_return");
      Out << "node_" << validReturnName << " [shape=box, label=\""
          << escapedReturnTypeName << "\"];\n";
      Out << "node_" << validFunctionName << " -> node_" << validReturnName
          << ";\n";
    }

    return true;
  }
};

// Define a custom AST consumer to process the AST
class MyASTConsumer : public ASTConsumer {
  raw_ostream &Out;

 public:
  explicit MyASTConsumer(raw_ostream &Out) : Out(Out) {}

  void HandleTranslationUnit(ASTContext &context) override {
    ASTDOTVisitor visitor(Out);
    visitor.setASTContext(context);
    visitor.TraverseDecl(context.getTranslationUnitDecl());
  }
};

// Define a front-end action to create the AST consumer
class MyFrontendAction : public ASTFrontendAction {
 public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(
      CompilerInstance &compiler, llvm::StringRef file) override {
    return std::make_unique<MyASTConsumer>(llvm::outs());
  }
};

static cl::OptionCategory MyToolCategory("My tool options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::opt<std::string> fileName();

int main(int argc, const char **argv) {
  // Set up the command-line options
  auto optionsParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  ClangTool tool(optionsParser->getCompilations(),
                 optionsParser->getSourcePathList());

  // Run the tool with the custom front-end action
  return tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
