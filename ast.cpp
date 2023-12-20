#include "ast.hpp"

bool final_code_stdout = false;
bool intermediate_code_stdout = false;
std::string filepath;

llvm::LLVMContext AST::TheContext;
llvm::IRBuilder<> AST::Builder(TheContext);
std::unique_ptr<llvm::Module> AST::TheModule;
std::unique_ptr<llvm::legacy::FunctionPassManager> AST::TheFPM;

llvm::Type *AST::i8;
llvm::Type *AST::i32;
llvm::Type *AST::i64;

std::map<std::string, std::map<std::string, llvm::Value *>> AST::NamedValues;
std::map<std::string, std::map<std::string,std::string> *> AST::FunctionTranslationTablesRealToLocal;
std::map<std::string, std::map<std::string,std::string> *> AST::FunctionTranslationTablesLocalToReal;
