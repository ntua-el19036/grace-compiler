#pragma once

#include <iostream>
#include <map>
#include <vector>
#include "symbol.hpp"
#include <memory>
#include <fstream>
#include <ctime>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>

#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>

#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"


#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetMachine.h"

#include "llvm/Support/Host.h"

// Define global flags
extern bool final_code_stdout;
extern bool intermediate_code_stdout;
extern std::string filepath;

extern void yyerror2(const char *msg, int line_number);

class AST
{
public:
  virtual ~AST() {}
  virtual void printOn(std::ostream &out) const = 0;
  virtual void sem() {}

  virtual llvm::Value *codegen() = 0;

  virtual void logToFile(const std::string& message, const std::string& filename = "log.txt") {
    // Get the current time
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);

    // Open the file in append mode
    std::ofstream file(filename, std::ios_base::app);

    if (file.is_open()) {
        // Log the message along with the timestamp
        file << "[" << (now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-'
             << now->tm_mday << ' ' << now->tm_hour << ':' << now->tm_min << ':'
             << now->tm_sec << "] " << message << std::endl;

        // Close the file
        file.close();
    } else {
        // Failed to open the file
        std::cerr << "Error: Unable to open the log file." << std::endl;
    }
  }

  virtual std::string get_translation_real_to_local(std::string real_name) {
    // std::cout << "looking up real " << real_name << std::endl;
    llvm::Function *Current_Function = Builder.GetInsertBlock()->getParent();
    std::string current_function_name = std::string(Current_Function->getName());
    // std::cout << "returns " << (*FunctionTranslationTablesRealToLocal[current_function_name])[real_name] << std::endl;
    return (*FunctionTranslationTablesRealToLocal[current_function_name])[real_name];
  }

  virtual std::string get_translation_local_to_real(std::string local_name) {
    // std::cout << "looking up local" << local_name << std::endl;
    llvm::Function *Current_Function = Builder.GetInsertBlock()->getParent();
    std::string current_function_name = std::string(Current_Function->getName());
    // std::cout << "returns " << (*FunctionTranslationTablesLocalToReal[current_function_name])[local_name] << std::endl;
    return (*FunctionTranslationTablesLocalToReal[current_function_name])[local_name];
  }

  void llvm_compile_and_dump(bool optimize = true)
  {
    // Initialize
    TheModule = std::make_unique<llvm::Module>("grace program", TheContext);
    TheFPM = std::make_unique<llvm::legacy::FunctionPassManager>(TheModule.get());
    NamedValues = std::map<std::string,std::map<std::string, llvm::Value *>>();
    FunctionTranslationTablesRealToLocal = std::map<std::string, std::map<std::string,std::string> *>();
    FunctionTranslationTablesLocalToReal = std::map<std::string, std::map<std::string,std::string> *>();
    // NamedFunctions = std::map<std::string, llvm::Function *>();
    if (optimize)
    {
      TheFPM->add(llvm::createPromoteMemoryToRegisterPass());
      // TheFPM->add(llvm::createInstructionCombiningPass());
      TheFPM->add(llvm::createReassociatePass());
      TheFPM->add(llvm::createGVNPass());
      TheFPM->add(llvm::createCFGSimplificationPass());
    }
    TheFPM->doInitialization();
    // Initialize types
    i8 = llvm::IntegerType::get(TheContext, 8);
    i32 = llvm::IntegerType::get(TheContext, 32);
    i64 = llvm::IntegerType::get(TheContext, 64);


    // Initialize library functions
    init_library();

    llvm::FunctionType *main_type = llvm::FunctionType::get(i32, {}, false);
    llvm::Function *main =
      llvm::Function::Create(main_type, llvm::Function::ExternalLinkage,
                       "main", TheModule.get());
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", main);
    Builder.SetInsertPoint(BB);
    // Emit the program code.
    codegen();
    Builder.CreateRet(c32(0));
    // Verify the IR.
    bool bad = verifyModule(*TheModule, &llvm::errs());
    if (bad) {
      std::cerr << "The IR is bad!" << std::endl;
      TheModule->print(llvm::errs(), nullptr);
      exit(1);
    }
    // Optimize!
    TheFPM->run(*main);

    // Initialize all targets
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();

    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    TheModule->setTargetTriple(TargetTriple);
    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    TheModule->setDataLayout(TheTargetMachine->createDataLayout());

    // Emit assembly code
    std::error_code EC;
    llvm::raw_fd_ostream dest(filepath + ".asm", EC);

    llvm::legacy::PassManager pass;
    auto FileType = llvm::CGFT_AssemblyFile;

    // Print out the IR.
    if (intermediate_code_stdout) {
      TheModule->print(llvm::outs(), nullptr);
    } else if (final_code_stdout) {
      TheTargetMachine->addPassesToEmitFile(pass, llvm::outs(), nullptr, FileType);
    } else {
      TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType);
      llvm::raw_fd_ostream dest(filepath + ".imm", EC);
      TheModule->print(dest, nullptr);
    }

     pass.run(*TheModule);
     dest.flush();

  }

protected:
  static llvm::LLVMContext TheContext;
  static llvm::IRBuilder<> Builder;
  static std::unique_ptr<llvm::Module> TheModule;
  static std::unique_ptr<llvm::legacy::FunctionPassManager> TheFPM;

  static llvm::Type *i8;
  static llvm::Type *i32;
  static llvm::Type *i64; //not used

  static llvm::ConstantInt *c8(char c)
  {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(8, c, true));
  }
  static llvm::ConstantInt *c32(int n)
  {
    return llvm::ConstantInt::get(TheContext, llvm::APInt(32, n, true));
  }

  void init_library() {
    llvm::FunctionType *writeInteger_type =
      llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {i32}, false);
    llvm::Function::Create(writeInteger_type, llvm::Function::ExternalLinkage, "writeInteger", TheModule.get());
    llvm::FunctionType *writeString_type =
      llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {llvm::PointerType::get(i8, 0)}, false);
    llvm::Function::Create(writeString_type, llvm::Function::ExternalLinkage, "writeString", TheModule.get());
    llvm::FunctionType *writeChar_type =
      llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {i8}, false);
    llvm::Function::Create(writeChar_type, llvm::Function::ExternalLinkage, "writeChar", TheModule.get());
    llvm::FunctionType *readInteger_type =
      llvm::FunctionType::get(llvm::Type::getInt32Ty(TheContext), {}, false);
    llvm::Function::Create(readInteger_type, llvm::Function::ExternalLinkage, "readInteger", TheModule.get());
    llvm::FunctionType *readString_type =
      llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {i32, llvm::PointerType::get(i8, 0)}, false);
    llvm::Function::Create(readString_type, llvm::Function::ExternalLinkage, "readString", TheModule.get());
    llvm::FunctionType *readChar_type =
      llvm::FunctionType::get(llvm::Type::getInt8Ty(TheContext), {}, false);
    llvm::Function::Create(readChar_type, llvm::Function::ExternalLinkage, "readChar", TheModule.get());
    llvm::FunctionType *ascii_type =
      llvm::FunctionType::get(llvm::Type::getInt32Ty(TheContext), {i8}, false);
       llvm::Function *ord = llvm::Function::Create(ascii_type, llvm::Function::ExternalLinkage, "ord", TheModule.get());
    llvm::Function *ascii = llvm::Function::Create(ascii_type, llvm::Function::ExternalLinkage, "ascii", TheModule.get());
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(TheContext, "entry", ascii);
    Builder.SetInsertPoint(BB);
    llvm::Value *arg = ascii->arg_begin();
    llvm::Value *result = Builder.CreateCall(ord, arg);  
    Builder.CreateRet(result);
    llvm::FunctionType *chr_type =
      llvm::FunctionType::get(llvm::Type::getInt8Ty(TheContext), {i32}, false);
    llvm::Function::Create(chr_type, llvm::Function::ExternalLinkage, "chr", TheModule.get());
    llvm::FunctionType *strlen_type =
      llvm::FunctionType::get(llvm::Type::getInt32Ty(TheContext), {llvm::PointerType::get(i8, 0)}, false);
    llvm::Function::Create(strlen_type, llvm::Function::ExternalLinkage, "strlen", TheModule.get());
    llvm::FunctionType *strcmp_type =
      llvm::FunctionType::get(llvm::Type::getInt32Ty(TheContext), {llvm::PointerType::get(i8, 0), llvm::PointerType::get(i8, 0)}, false);
    llvm::Function::Create(strcmp_type, llvm::Function::ExternalLinkage, "strcmp", TheModule.get());
    llvm::FunctionType *strcpy_type =
      llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {llvm::PointerType::get(i8, 0), llvm::PointerType::get(i8, 0)}, false);
    llvm::Function::Create(strcpy_type, llvm::Function::ExternalLinkage, "strcpy", TheModule.get());
    llvm::FunctionType *strcat_type =
      llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), {llvm::PointerType::get(i8, 0), llvm::PointerType::get(i8, 0)}, false);
    llvm::Function::Create(strcat_type, llvm::Function::ExternalLinkage, "strcat", TheModule.get());
  }

  static std::map<std::string, std::map<std::string, llvm::Value *>> NamedValues;
  static std::map<std::string, std::map<std::string,std::string> *> FunctionTranslationTablesRealToLocal;
  static std::map<std::string, std::map<std::string,std::string> *> FunctionTranslationTablesLocalToReal;
};

inline std::ostream &operator<<(std::ostream &out, const AST &t)
{
  t.printOn(out);
  return out;
}

inline std::ostream &operator<<(std::ostream &out, DataType t)
{
  switch (t)
  {
  case DataType::TYPE_int:
    out << "int";
    break;
  case DataType::TYPE_char:
    out << "char";
    break;
  case DataType::TYPE_nothing:
    out << "nothing";
    break;
  }
  return out;
}

class Expr : public AST
{
public:
  virtual int eval() const = 0;

  virtual llvm::Value *llvm_get_array_offset(std::vector<llvm::Value *> * indices) {
    return 0;
  }

  virtual int get_int_cosnt_number() {
    return 0;
  }

  virtual llvm::Value *llvm_get_value_ptr(bool isParam = false) {
    return nullptr;
  };

  virtual llvm::Type *get_llvm_type() {
    return nullptr;
  };

  int line_number = 0;

  void type_check(DataType t, std::vector<int> dim = {})
  {
    if (type != t)
    {
      yyerror2("Type mismatch", line_number);
      // yyerror("Type mismatch");
    }
    if (dimensions != dim)
    {
      if (dimensions.empty() || dim.empty())
      {
        yyerror2("Type mismatch, array and non-array", line_number);
      }
      if (dimensions[0] == 0 || dim[0] == 0)
      {
        // the first dimension of either is unknown, so compare the rest
        for (unsigned int i = 1; i < dimensions.size(); i++)
        {
          if (dimensions[i] != dim[i])
          {
            yyerror2("Array dimension mismatch", line_number);
          }
        }
      }
      else
      {
        yyerror2("Array dimension mismatch", line_number);
      }
    }
  }

  DataType get_type() const
  {
    return type;
  }

  EntryKind get_kind() const
  {
    return kind;
  }

  std::vector<int> get_dimensions() const
  {
    return dimensions;
  }

  virtual bool is_rvalue() const
  {
    return false;
  }

protected:
  DataType type;
  EntryKind kind;
  std::vector<int> dimensions;
};

class Stmt : public AST
{
public:
  virtual void run() const = 0;
};
// unused
// class VarDecl: public Stmt {
// public:
//     VarDecl(const std::string &s, DataType t): var(s), datatype(t) {}

//     void printOn(std::ostream &out) const override {
//         out << "VarDecl(" << var << ": " << datatype << ")";
//   }
// private:
//   std::string var;
//   DataType datatype;
// };

// class FunDecl: public Stmt {
// public:
//     FunDecl(const std::string &s, DataType t, std::vector<std::tuple<DataType, std::string>>  p): var(s), rettype(t), params(p) {}

//     void printOn(std::ostream &out) const override {
//         out << "FunDecl(" << var << ": " << rettype << ")";
//   }
// private:
//   std::string var;
//   DataType rettype;
//   std::vector<std::tuple<DataType, std::string>> params;
// };

class IntConst : public Expr
{
public:
  IntConst(int n, int lineno = 0) : num(n) { line_number = lineno; }
  virtual void printOn(std::ostream &out) const override
  {
    out << "IntConst(" << num << ")";
  }
  virtual int eval() const override
  {
    return num;
  }

  virtual int get_int_cosnt_number() override
  {
    return num;
  }

  virtual llvm::Value *codegen() override
  {
    return c32(num);
  }

  virtual void sem() override
  {
    type = DataType::TYPE_int;
  }

  virtual bool is_rvalue() const override
  {
    return true;
  }

private:
  int num;
};

class CharConst : public Expr
{
public:
  CharConst(char c, int lineno = 0) : charval(c) { line_number = lineno; }
  virtual void printOn(std::ostream &out) const override
  {
    out << "CharConst(" << charval << ")";
  }
  virtual int eval() const override
  {
    return charval;
  }

  virtual void sem() override
  {
    type = DataType::TYPE_char;
  }

  virtual llvm::Value *codegen() override
  {
    return c8(charval);
  }

  virtual bool is_rvalue() const override
  {
    return true;
  }

private:
  char charval;
};

class Id : public Expr
{
public:
  Id(std::string *s, int lineno = 0) : var(s) { line_number = lineno; }

  void printOn(std::ostream &out) const override
  {
    out << "Id(" << *var << ")";
  }
  // TODO: implement
  virtual int eval() const override
  {
    return 0;
  }

  virtual void sem() override
  {
    // std::cout<<"looking up "<<*var<<std::endl;
    STEntry *entry = st.lookup(*var);
    if (entry == nullptr)
    {
      yyerror2("Variable not declared", line_number);
    }
    // std::cout<<"entry kind: " << entry->kind << std::endl;
    if (!entry->dimensions.empty())
    {
      dimensions = entry->dimensions;
    }
    if (entry->missingFirstDimension)
    {
      dimensions.insert(dimensions.begin(), 0);
    }
    type = entry->type;
    kind = entry->kind;
  }

  virtual llvm::Value *llvm_get_array_offset(std::vector<llvm::Value *> *indices) override {
    std::string current_function_name = std::string(Builder.GetInsertBlock()->getParent()->getName());
    llvm::Value *alloca = NamedValues[current_function_name][get_translation_real_to_local(*var)];
    if (!alloca) //this won't happen because we check for undeclared variables in sem
    {
      yyerror2("Unknown variable name", line_number);
    }
    return alloca;
  }

  virtual llvm::Value *llvm_get_value_ptr(bool isParam = false) override
  {
    std::string current_function_name = std::string(Builder.GetInsertBlock()->getParent()->getName());
    std::string translation = get_translation_real_to_local(*var);
    AST::logToFile("looking up " + translation + " in " + current_function_name);
    llvm::Value *alloca = NamedValues[current_function_name][translation];
    if (!alloca) //this won't happen because we check for undeclared variables in sem
    {
      yyerror2("Unknown variable name", line_number);
    }
    // TODO: check if this is correct
    if(alloca->getType()->isPointerTy() && alloca->getType()->getPointerElementType()->isPointerTy()) {
      llvm::Value *ptr = Builder.CreateGEP(alloca, c32(0), "outptr");
      return Builder.CreateLoad(ptr, *var);
    }
    if(alloca->getType()->isPointerTy() && alloca->getType()->getPointerElementType()->isArrayTy()) {
      llvm::Value *ptr = Builder.CreateGEP(alloca, c32(0), "firstelementptr");
      return ptr;
    }
    llvm::Value *GEP = Builder.CreateGEP(alloca, c32(0));
    return GEP;
  }

  virtual llvm::Type *get_llvm_type() override {
    std::string current_function_name = std::string(Builder.GetInsertBlock()->getParent()->getName());
    llvm::Value *alloca = NamedValues[current_function_name][*var];
    if (!alloca) //this won't happen because we check for undeclared variables in sem
    {
      yyerror2("Unknown variable name", line_number);
    }
    return alloca->getType();
  }

  virtual llvm::Value *codegen() override
  {
    std::string current_function_name = std::string(Builder.GetInsertBlock()->getParent()->getName());
    llvm::Value *alloca = NamedValues[current_function_name][get_translation_real_to_local(*var)];
    if (!alloca) //this won't happen because we check for undeclared variables in sem
    {
      yyerror2("Unknown variable name", line_number);
    }
     if(alloca->getType()->isPointerTy() && alloca->getType()->getPointerElementType()->isPointerTy()) {
      llvm::Value *outptr = Builder.CreateGEP(alloca, c32(0), "outptr");
      llvm::Value *inptr = Builder.CreateLoad(outptr, *var);
      return Builder.CreateLoad(inptr, *var);
    }
    llvm::Value *ptr = Builder.CreateGEP(alloca, c32(0), "ptr");
    return Builder.CreateLoad(ptr, *var);
  }

private:
  std::string *var;
};

class StringLiteral : public Expr
{
public:
  StringLiteral(std::string *s, int lineno = 0) : stringval(s) { line_number = lineno; }
  ~StringLiteral() { delete stringval; }

  void printOn(std::ostream &out) const override
  {
    out << "StringLiteral(" << *stringval << ")";
  }

  // TODO: implement
  virtual int eval() const override
  {
    return 0;
  }

  virtual void sem() override
  {
    type = DataType::TYPE_char;
    dimensions.push_back(stringval->length() + 1);
  }

  virtual llvm::Value *codegen() override
  {
    llvm::Value *string_ptr = Builder.CreateGlobalString(*stringval, "string");
    return Builder.CreateGEP(string_ptr, std::vector<llvm::Value *>({c32(0), c32(0)}), "stringptr");
    // return Builder.CreateLoad(string_ptr, "stringliteral");
  }

  virtual llvm::Value *llvm_get_value_ptr(bool isParam = false) override
  {
    llvm::Value *string_ptr = Builder.CreateGlobalString(*stringval, "string");
    return Builder.CreateGEP(string_ptr, std::vector<llvm::Value *>({c32(0), c32(0)}), "stringptr");
  }

private:
  std::string *stringval;
};

class ArrayAccess : public Expr
{
public:
  ArrayAccess(Expr *obj, Expr *pos) : object(obj), position(pos) { line_number = obj->line_number; }
  ~ArrayAccess()
  {
    delete object;
    delete position;
  }

  void printOn(std::ostream &out) const override
  {
    out << "ArrayAccess(" << *object << ", " << *position << ")";
  }

  virtual llvm::Value *llvm_get_value_ptr(bool isParam = false) override
  {
    std::vector<llvm::Value *> *indices = new std::vector<llvm::Value *>();
    llvm::Value *base = object->llvm_get_array_offset(indices);
    indices->push_back(position->codegen());
    unsigned long int count = 1;
    llvm::Type *type = base->getType()->getPointerElementType();
    while (type->isArrayTy())
    {
      type = type->getArrayElementType();
      count++;
    }

    if (isParam && !(count == indices->size()))
      indices->insert(indices->begin(), c32(0));

    if (base->getType()->isPointerTy() && base->getType()->getPointerElementType()->isPointerTy())
      base = Builder.CreateLoad(base, "array12312");
    llvm::Value *ptr = Builder.CreateGEP(base, *indices, "elementptr");
    delete indices;
    return ptr;
  }

  // TODO: implement
  virtual int eval() const override
  {
    return 0;
  }

  virtual void sem() override
  {
    object->sem();
    position->sem();
    if (object->get_kind() == EntryKind::FUNCTION)
    {
      yyerror2("Cannot index a function", line_number);
    }
    if (position->get_kind() == EntryKind::FUNCTION)
    {
      yyerror2("Cannot index with a function", line_number);
    }
    position->type_check(DataType::TYPE_int); // maybe not needed
    if (object->get_dimensions().empty())
    {
      yyerror2("Cannot index a non-array", line_number);
    }
    type = object->get_type();
    dimensions = object->get_dimensions();
    dimensions.erase(dimensions.begin());
  }

  virtual llvm::Value *llvm_get_array_offset(std::vector<llvm::Value *> *indices) {
    llvm::Value *result = object->llvm_get_array_offset(indices);
    indices->push_back(position->codegen());
    return result;
  }

  virtual llvm::Value *codegen() override
  {
    std::vector<llvm::Value *> *indices = new std::vector<llvm::Value *>();
    // indices->push_back(c32(0));
    llvm::Value *base = object->llvm_get_array_offset(indices);
    //std::cout << std::endl << "base: " ;
    //base->print(llvm::outs());
    //std::cout << std::endl;
    indices->push_back(position->codegen());
    if(base->getType()->isPointerTy() && base->getType()->getPointerElementType()->isPointerTy())
      base = Builder.CreateLoad(base, "array"); //this is dumb, it loads the whole array, oh well
    llvm::Value *ptr = Builder.CreateGEP(base, *indices, "elementptr");
    delete indices;
    return Builder.CreateLoad(ptr, "element");
  }

private:
  Expr *object;
  Expr *position;
};

class ExpressionList : public Expr
{
public:
  std::vector<Expr *> expressions;

  ExpressionList(Expr *e) : expressions() { expressions.push_back(e); }
  ~ExpressionList()
  {
    for (Expr *e : expressions)
      delete e;
  }

  void add_expression(Expr *e) { expressions.insert(expressions.begin(), e); }
  void printOn(std::ostream &out) const override
  {
    out << "ExpressionList(";
    bool first = true;
    for (const auto &e : expressions)
    {
      if (!first)
        out << ", ";
      first = false;
      out << *e;
    }
    out << ")";
  }

  // TODO: implement
  virtual int eval() const override
  {
    return 0;
  }

  virtual void sem() override
  {
    for (const auto &e : expressions)
    {
      e->sem();
    }
  }

  virtual llvm::Value *codegen() override
  {
    return nullptr;
  }
};

class FunctionCall : public Expr, public Stmt
{
public:
  FunctionCall(std::string *i, ExpressionList *el = nullptr, int lineno = 0) : id(i), args(el)
  {
    line_number = lineno;
  }
  FunctionCall(std::string *i, int lineno = 0) : id(i) { line_number = lineno; }
  ~FunctionCall()
  {
    delete id;
    delete args;
  }

  void printOn(std::ostream &out) const override
  {
    out << "FunctionCall(" << *id;
    if (args != nullptr)
      out << ", " << *args;
    out << ")";
  }

  // TODO: implement
  virtual int eval() const override
  {
    return 0;
  }

  // TODO: implement
  virtual void run() const override
  {
    return;
  }

  virtual bool is_rvalue() const override
  {
    return true;
  }

  bool is_library_function () {
    return strcmp(id->c_str(), "writeInteger") == 0 ||
           strcmp(id->c_str(), "writeString") == 0 ||
           strcmp(id->c_str(), "writeChar") == 0 ||
           strcmp(id->c_str(), "readInteger") == 0 ||
           strcmp(id->c_str(), "readString") == 0 ||
           strcmp(id->c_str(), "readChar") == 0 ||
           strcmp(id->c_str(), "ord") == 0 ||
           strcmp(id->c_str(), "ascii") == 0 ||
           strcmp(id->c_str(), "chr") == 0 ||
           strcmp(id->c_str(), "strlen") == 0 ||
           strcmp(id->c_str(), "strcmp") == 0 ||
           strcmp(id->c_str(), "strcpy") == 0 ||
           strcmp(id->c_str(), "strcat") == 0;
  }

  virtual void sem() override
  {
    STEntry *entry = st.lookup(*id);
    if (entry == nullptr)
    {
      yyerror2("Function not declared", line_number);
    }
    if (entry->kind != EntryKind::FUNCTION)
    {
      yyerror2("Not a function", line_number);
    }
    type = entry->type;
    if (args == nullptr)
    {
      if (!entry->paramTypes.empty())
      {
        yyerror2("No arguments provided", line_number);
      }
    }
    else
    {
      if (entry->paramTypes.size() != args->expressions.size())
      {
        yyerror2("Wrong number of arguments", line_number);
      }
      std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>>::iterator param_it = entry->paramTypes.begin();
      for (const auto &e : args->expressions)
      {
        e->sem();
        if(std::get<1>(*param_it) == PassingType::BY_REFERENCE) {
          if(e->is_rvalue() == true) {
            yyerror2("Cannot pass r-value by reference", line_number);
          }
        }
        std::vector<int> param_dimensions = std::get<2>(*param_it); // copy dimensions vector
        if (std::get<3>(*param_it)) // if missing first dimension
          param_dimensions.insert(param_dimensions.begin(), 0);
        e->type_check(std::get<0>(*param_it), param_dimensions);
        ++param_it;
      }
    }
  }

  virtual llvm::Value *codegen() override
  {
    std::string callee_function_name = std::string("user_") + *id;
    if(this->is_library_function()) {
      callee_function_name = *id;
    }
    llvm::Function *CalleeF = TheModule->getFunction(callee_function_name);
    // llvm::Function *CalleeF = NamedFunctions[*id];
    if(!CalleeF) {
      yyerror2("Unknown function referenced", line_number);
    }
    //std::cout << "CalleeF: " << CalleeF->args()->print() << std::endl;
    //maybe check argsize
    llvm::Function::arg_iterator argIt = CalleeF->arg_begin();
    std::vector<llvm::Value *> ArgV;
    unsigned long int user_param_count = 0;
    if(args != nullptr) {
      user_param_count = args->expressions.size();
    }
    std::string caller_function_name = std::string(Builder.GetInsertBlock()->getParent()->getName());

    // std::map<std::string, llvm::Value *>::iterator it = NamedValues.begin();
    Expr::logToFile("calling: " + callee_function_name + " from: " + caller_function_name);
    Expr::logToFile("user param count: " + std::to_string(user_param_count));
    Expr::logToFile("signature arg count: " + std::to_string(CalleeF->arg_size()));

    for(unsigned i = 0, e = CalleeF->arg_size(); i != e; ++i) {
      if(i >= user_param_count) { /* local variables */
        std::string param_name = std::string(argIt->getName());
        Expr::logToFile("param_name: " + param_name);
        Expr::logToFile("callee_function_name: " + callee_function_name);
        std::map<std::string, std::string> *CalleeFunctionLocalToRealTranslations = FunctionTranslationTablesLocalToReal[callee_function_name];
        if(CalleeFunctionLocalToRealTranslations == nullptr) {
          Expr::logToFile("CalleeFunctionLocalToRealTranslations is null");
        }
        Expr::logToFile("looking up: " + param_name + " in local to real of " + callee_function_name);
        std::string real_param_name = (*CalleeFunctionLocalToRealTranslations)[param_name];
        Expr::logToFile("real_param_name: " + real_param_name);
        std::map<std::string, std::string> *CallerFunctionRealToLocalTranslations = FunctionTranslationTablesRealToLocal[caller_function_name];
        std::string local_param_name = (*CallerFunctionRealToLocalTranslations)[real_param_name];
        Expr::logToFile("local param: " + local_param_name + " real param: " + real_param_name + " param name " + param_name);
        llvm::Value *arg_value = NamedValues[caller_function_name][local_param_name];
        // std::cout << "local param: " << local_param_name << " real param: " << real_param_name << " param name " << param_name << std::endl;
        Expr::logToFile("local param: " + local_param_name + " real param: " + real_param_name + " param name " + param_name);
        if(arg_value->getType()->isPointerTy()) {
          if(arg_value->getType()->getPointerElementType()->isPointerTy()) {
            llvm::Value *ptr = Builder.CreateGEP(arg_value, c32(0), "ptrtolocal");
            ArgV.push_back(Builder.CreateLoad(ptr, param_name));
          }
          else {
            ArgV.push_back(arg_value);
          }
        }
        else {
          ArgV.push_back(arg_value);
        }
        ++argIt; 
        // ++it;
        continue;
      }
      if(argIt->getType()->isPointerTy()) {
        Expr::logToFile("pointer param: " + std::string(argIt->getName()));
        // if(args->expressions[i]->llvm_get_value_ptr()->getType()->isArrayTy()) {
        // }
        llvm::Value * ptr = args->expressions[i]->llvm_get_value_ptr(true);
        // if(argIt->getType()->getPointerElementType()->isPointerTy()) {
        //   ptr = Builder.CreateGEP(ptr, std::vector<llvm::Value *>({c32(0), c32(0)}), "ptrfromptr");
        // }
        ArgV.push_back(ptr);
      } else {
        ArgV.push_back(args->expressions[i]->codegen());
      }
      if(!ArgV.back()) return nullptr;
      ++argIt;
    }
    return Builder.CreateCall(CalleeF, ArgV);
  }

private:
  std::string *id;
  ExpressionList *args;
};

class Negative : public Expr
{
public:
  Negative(Expr *e) : expr(e) { line_number = e->line_number; }
  ~Negative() { delete expr; }
  virtual void printOn(std::ostream &out) const override
  {
    out << "Negative(" << *expr << ")";
  }
  virtual int eval() const override
  {
    return -(expr->eval());
  }

  virtual bool is_rvalue() const override
  {
    return true;
  }

  virtual void sem() override
  {
    expr->sem();
    type = expr->get_type();
    dimensions = expr->get_dimensions();
    if (expr->get_kind() == EntryKind::FUNCTION)
    {
      yyerror2("Cannot negate a function", line_number);
    }
  }

  virtual llvm::Value *codegen() override {
    llvm::Value *V = expr->codegen();
    if(!V) return nullptr;
    return Builder.CreateNeg(V, "negtmp");
  }

private:
  Expr *expr;
};

class BinOp : public Expr
{
public:
  BinOp(Expr *l, char o, Expr *r) : left(l), op(o), right(r) { line_number = l->line_number; }
  ~BinOp()
  {
    delete left;
    delete right;
  }

  virtual void printOn(std::ostream &out) const override
  {
    out << op << "(" << *left << ", " << *right << ")";
  }

  virtual int eval() const override
  {
    switch (op)
    {
    case '+':
      return left->eval() + right->eval();
    case '-':
      return left->eval() - right->eval();
    case '*':
      return left->eval() * right->eval();
    case '/':
      return left->eval() / right->eval();
    case '%':
      return left->eval() % right->eval();
    case '=':
      return left->eval() == right->eval();
    case '#':
      return left->eval() != right->eval();
    case '<':
      return left->eval() < right->eval();
    case '>':
      return left->eval() > right->eval();
    case 'l':
      return left->eval() <= right->eval();
    case 'g':
      return left->eval() >= right->eval();

    case '&':
      return {!(left->eval()) ? false : right->eval()};
    case '|':
      return {(left->eval()) ? true : right->eval()};
    }
    return 0; // this will never be reached
  }

  virtual bool is_rvalue() const override
  {
    return true;
  }

  // TODO: maybe check to see if operands are ints only
  // otherwise we allow char operations
  virtual void sem() override
  {
    left->sem();
    right->sem();
    if (left->get_kind() == EntryKind::FUNCTION)
    {
      yyerror2("left operand cannot be a function", line_number);
    }
    if (right->get_kind() == EntryKind::FUNCTION)
    {
      yyerror2("right operand cannot be a function", line_number);
    }
    left->type_check(right->get_type(), right->get_dimensions());
    type = left->get_type();
    if (left->get_dimensions().empty() || left->get_dimensions().front() == 0)
      dimensions = right->get_dimensions();
    else
      dimensions = left->get_dimensions();
  }

  //for non short-cirtuiting binary operators
  virtual llvm::Value *binop_basic_codegen() {
    llvm::Value *l = left->codegen();
    while(l->getType()->isPointerTy()) {
      llvm::Value *tmp = Builder.CreateGEP(l, c32(0));
      l = Builder.CreateLoad(tmp);
    }
    llvm::Value *r = right->codegen();
    while(r->getType()->isPointerTy()) {
      llvm::Value *tmp = Builder.CreateGEP(r, c32(0));
      r = Builder.CreateLoad(tmp);
    }
    switch (op)
    {
    case '+':
      return Builder.CreateAdd(l, r, "addtmp");
    case '-':
      return Builder.CreateSub(l, r, "subtmp");
    case '*':
      return Builder.CreateMul(l, r, "multmp");
    case '/':
      return Builder.CreateSDiv(l, r, "divtmp");
    case '%':
      return Builder.CreateSRem(l, r, "modtmp");
    case '=':
      return Builder.CreateICmpEQ(l, r, "eqtmp");
    case '#':
      return Builder.CreateICmpNE(l, r, "netmp");
    case '<':
      return Builder.CreateICmpSLT(l, r, "lttmp");
    case '>':
      return Builder.CreateICmpSGT(l, r, "gttmp");
    case 'l':
      return Builder.CreateICmpSLE(l, r, "letmp");
    case 'g':
      return Builder.CreateICmpSGE(l, r, "getmp");
    }
    return nullptr;
  }

  virtual llvm::Value *codegen() override
  {
    switch (op)
    {
    case '&':
      // return Builder.CreateAnd(l, r, "andtmp");
      // short-circuit and
      {
        llvm::Value *l = left->codegen();
        while(l->getType()->isPointerTy()) {
          llvm::Value *tmp = Builder.CreateGEP(l, c32(0));
          l = Builder.CreateLoad(tmp);
        }

        llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(TheContext, "andthen", TheFunction);
        llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(TheContext, "andelse");
        llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(TheContext, "andcont");

        Builder.CreateCondBr(l, ThenBB, ElseBB); //if l is false, go to else
        Builder.SetInsertPoint(ThenBB);

        llvm::Value *r = right->codegen();
        while(r->getType()->isPointerTy()) {
          llvm::Value *tmp = Builder.CreateGEP(r, c32(0));
          r = Builder.CreateLoad(tmp);
        }

        llvm::Value *AndV = Builder.CreateICmpEQ(l, r, "andtmp"); //l is true, return right
        Builder.CreateBr(MergeBB);
        ThenBB = Builder.GetInsertBlock();

        TheFunction->getBasicBlockList().push_back(ElseBB);
        Builder.SetInsertPoint(ElseBB);
        llvm::Value *ElseV = Builder.getFalse(); //l is false, return false
        Builder.CreateBr(MergeBB);
        ElseBB = Builder.GetInsertBlock();

        TheFunction->getBasicBlockList().push_back(MergeBB);
        Builder.SetInsertPoint(MergeBB);
        llvm::PHINode *PN = Builder.CreatePHI(llvm::Type::getInt1Ty(TheContext), 2, "andphi");
        PN->addIncoming(AndV, ThenBB);
        PN->addIncoming(ElseV, ElseBB);
        return PN;
      }
    case '|':
      {
        llvm::Value *l = left->codegen();
        while(l->getType()->isPointerTy()) {
          llvm::Value *tmp = Builder.CreateGEP(l, c32(0));
          l = Builder.CreateLoad(tmp);
        }

        llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(TheContext, "orthen", TheFunction);
        llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(TheContext, "orelse");
        llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(TheContext, "orcont");

        Builder.CreateCondBr(l, ElseBB, ThenBB); //if l is false, go to then
        Builder.SetInsertPoint(ThenBB);

        llvm::Value *r = right->codegen();
        while(r->getType()->isPointerTy()) {
          llvm::Value *tmp = Builder.CreateGEP(r, c32(0));
          r = Builder.CreateLoad(tmp);
        }

        llvm::Value *OrV = Builder.CreateICmpNE(l, r, "ortmp"); //l is false, return right
        Builder.CreateBr(MergeBB);
        ThenBB = Builder.GetInsertBlock();

        TheFunction->getBasicBlockList().push_back(ElseBB);
        Builder.SetInsertPoint(ElseBB);
        llvm::Value *ElseV = Builder.getTrue();  //l is true, return true
        Builder.CreateBr(MergeBB);
        ElseBB = Builder.GetInsertBlock();

        TheFunction->getBasicBlockList().push_back(MergeBB);
        Builder.SetInsertPoint(MergeBB);
        llvm::PHINode *PN = Builder.CreatePHI(llvm::Type::getInt1Ty(TheContext), 2, "orphi");
        PN->addIncoming(OrV, ThenBB);
        PN->addIncoming(ElseV, ElseBB);
        return PN;
      }
    default:
      return binop_basic_codegen();
    }
    return nullptr;
  }

private:
  Expr *left;
  char op;
  Expr *right;
};

class Not : public Expr
{
public:
  Not(Expr *c) : cond(c) {}
  ~Not() { delete cond; }
  virtual void printOn(std::ostream &out) const override
  {
    out << "Not"
        << "(" << *cond << ")";
  }
  virtual int eval() const override
  {
    return !(cond->eval());
  }

  virtual void sem() override
  {
    cond->sem();
    // if(cond->get_kind() == EntryKind::FUNCTION) {
    //   yyerror("Cannot negate a function");
    // }
  }

  virtual bool is_rvalue() const override
  {
    return true;
  }

  virtual llvm::Value *codegen() override {
    llvm::Value *CondV = cond->codegen();
    if(!CondV) return nullptr;
    return Builder.CreateNot(CondV, "nottmp");
  }

private:
  Expr *cond;
};

class Block : public Stmt
{
public:
  Block() : stmt_list() {}
  ~Block()
  {
    for (Stmt *s : stmt_list)
      delete s;
  }
  void append_stmt(Stmt *s) { stmt_list.push_back(s); }
  void printOn(std::ostream &out) const override
  {
    out << "Block(";
    bool first = true;
    for (const auto &s : stmt_list)
    {
      if (!first)
        out << ", ";
      first = false;
      out << *s;
    }
    out << ")";
  }
  void run() const override
  {
    for (Stmt *s : stmt_list)
      s->run();
  }

  virtual void sem() override
  {
    for (Stmt *s : stmt_list)
      s->sem();
  }

  virtual llvm::Value *codegen() override {
    llvm::Value *V = nullptr;
    for (Stmt *s : stmt_list) {
      if(!V) V = s->codegen();
      else s->codegen();
    }
    return c32(0);
  }

private:
  std::vector<Stmt *> stmt_list;
};

class If : public Stmt
{
public:
  If(Expr *c, Stmt *s1, Stmt *s2 = nullptr) : cond(c), stmt1(s1), stmt2(s2) {}
  ~If()
  {
    delete cond;
    delete stmt1;
    delete stmt2;
  }
  void printOn(std::ostream &out) const override
  {
    out << "If(" << *cond << ", " << *stmt1;
    if (stmt2 != nullptr)
      out << ", " << *stmt2;
    out << ")";
  }
  void run() const override
  {
    if (cond->eval())
      stmt1->run();
    else
      stmt2->run();
  }

  virtual void sem() override
  {
    cond->sem();
    stmt1->sem();
    if (stmt2 != nullptr)
      stmt2->sem();
  }

  virtual llvm::Value *codegen() override {
    llvm::Value *CondV = cond->codegen();
    if(!CondV) return nullptr;
    //CondV = Builder.CreateICmpNE(CondV, c32(0), "ifcond");

    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    llvm::BasicBlock *ThenBB = llvm::BasicBlock::Create(TheContext, "then", TheFunction);
    llvm::BasicBlock *ElseBB = llvm::BasicBlock::Create(TheContext, "else");
    llvm::BasicBlock *MergeBB = llvm::BasicBlock::Create(TheContext, "ifcont");

    Builder.CreateCondBr(CondV, ThenBB, ElseBB);

    Builder.SetInsertPoint(ThenBB);
    llvm::Value *ThenV = stmt1->codegen();
    if(!ThenV) return nullptr;

    if(ThenBB->getTerminator() == nullptr)
      Builder.CreateBr(MergeBB);
    ThenBB = Builder.GetInsertBlock();

    TheFunction->getBasicBlockList().push_back(ElseBB);
    Builder.SetInsertPoint(ElseBB);
    if(stmt2 != nullptr) {
      llvm::Value *ElseV = stmt2->codegen();
      if(!ElseV) return nullptr;
    }
    if(ElseBB->getTerminator() == nullptr)
      Builder.CreateBr(MergeBB);
    ElseBB = Builder.GetInsertBlock();

    if(Builder.GetInsertBlock()->getTerminator() == nullptr)
      Builder.CreateBr(MergeBB);
    TheFunction->getBasicBlockList().push_back(MergeBB);
    Builder.SetInsertPoint(MergeBB);
    // llvm::PHINode *PN = Builder.CreatePHI(i32, 2, "iftmp");

    // PN->addIncoming(ThenV, ThenBB);
    // PN->addIncoming(ElseV, ElseBB);

    return MergeBB;
  }

private:
  Expr *cond;
  Stmt *stmt1;
  Stmt *stmt2;
};

class While : public Stmt
{
public:
  While(Expr *c, Stmt *s) : cond(c), stmt(s) {}
  ~While()
  {
    delete cond;
    delete stmt;
  }
  void printOn(std::ostream &out) const override
  {
    out << "While(" << *cond << " do " << *stmt;
    out << ")";
  }
  void run() const override
  {
    while (cond->eval())
      stmt->run();
  }

  virtual void sem() override
  {
    cond->sem();
    stmt->sem();
  }

  virtual llvm::Value* codegen() override {
    llvm::Function *TheFunction = Builder.GetInsertBlock()->getParent();

    llvm::BasicBlock *LoopStartBB = llvm::BasicBlock::Create(TheContext, "loopstart", TheFunction);
    llvm::BasicBlock *LoopInsideBB = llvm::BasicBlock::Create(TheContext, "loopin");
    llvm::BasicBlock *LoopEndBB = llvm::BasicBlock::Create(TheContext, "loopend");

    Builder.CreateBr(LoopStartBB);
    Builder.SetInsertPoint(LoopStartBB);
    llvm::Value *CondV = cond->codegen();
    if(!CondV) return nullptr;

    if(LoopStartBB->getTerminator() == nullptr)
      Builder.CreateCondBr(CondV, LoopInsideBB, LoopEndBB);
    if(Builder.GetInsertBlock()->getTerminator() == nullptr)
      Builder.CreateCondBr(CondV, LoopInsideBB, LoopEndBB);

    TheFunction->getBasicBlockList().push_back(LoopInsideBB);
    Builder.SetInsertPoint(LoopInsideBB);
    llvm::Value *InsideV = stmt->codegen();
    if(!InsideV) return nullptr;
    if(LoopInsideBB->getTerminator() == nullptr)
      Builder.CreateBr(LoopStartBB);
    LoopInsideBB = Builder.GetInsertBlock();

    if(Builder.GetInsertBlock()->getTerminator() == nullptr)
      Builder.CreateBr(LoopStartBB);
    TheFunction->getBasicBlockList().push_back(LoopEndBB);
    Builder.SetInsertPoint(LoopEndBB);
    return LoopEndBB;
  }

private:
  Expr *cond;
  Stmt *stmt;
};

class EmptyStmt : public Stmt
{
public:
  EmptyStmt() {}
  void printOn(std::ostream &out) const override
  {
    out << "EmptyStmt()";
  }
  void run() const override {}

  virtual void sem() override {} // idk if this is needed

  virtual llvm::Value *codegen() override {
    return nullptr;
  }
};

class Assignment : public Stmt
{
public:
  Assignment(Expr *l, Expr *e) : l_value(l), expr(e) {}
  ~Assignment()
  {
    delete l_value;
    delete expr;
  }
  void printOn(std::ostream &out) const override
  {
    out << "Assignment(" << *l_value << ", " << *expr << ")";
  }

  // TODO: implement
  void run() const override
  {
  }

  virtual void sem() override
  {
    l_value->sem();
    expr->sem();
    if (l_value->get_kind() == EntryKind::FUNCTION)
    {
      yyerror2("Cannot assign to a function", l_value->line_number);
    }
    if (expr->get_kind() == EntryKind::FUNCTION)
    {
      yyerror2("Cannot assign a function", expr->line_number);
    }
    // std::cout<<"l_value type: "<<l_value->get_type()<<std::endl;
    // std::cout<<"expr type: "<<expr->get_type()<<std::endl;
    l_value->type_check(expr->get_type(), expr->get_dimensions());
  }

  virtual llvm::Value *codegen() override {
    llvm::Value* lval = l_value->llvm_get_value_ptr(false);
    llvm::Value* rval = expr->codegen();
    Builder.CreateStore(rval, lval);
    return c32(0);
  }

private:
  Expr *l_value;
  Expr *expr;
};

class Return : public Stmt
{
public:
  Return(Expr *e = nullptr) : expr(e) {}
  Return(int lineno = 0) : expr(nullptr) { line_number = lineno; }
  ~Return() { delete expr; }

  int line_number = 0;

  // TODO: implement
  void run() const override
  {
  }

  void printOn(std::ostream &out) const override
  {
    out << "Return(";
    if (expr != nullptr)
      out << *expr;
    out << ")";
  }

  virtual void sem() override
  {
    if (expr == nullptr)
    {
      if (st.get_return_type() != DataType::TYPE_nothing)
      {
        yyerror2("Type mismatch", line_number);
      }
    }
    else
    {
      expr->sem();
      expr->type_check(st.get_return_type());
    }
    st.set_return_exists();
  }

  virtual llvm::Value *codegen() override {
    if(!expr) return Builder.CreateRetVoid();
    return Builder.CreateRet(expr->codegen());
  }

private:
  Expr *expr;
};

class IdList : public AST
{
public:
  std::vector<std::string *> id_list;
  IdList(std::string *i) { append_id(i); }

  void append_id(std::string *id)
  {
    id_list.insert(id_list.begin(), id);
  }

  void printOn(std::ostream &out) const override
  {
    out << "IdList(";
    bool first = true;
    for (const auto &id : id_list)
    {
      if (!first)
        out << ", ";
      first = false;
      out << *id;
    }
    out << ")";
  }

  virtual llvm::Value *codegen() override {
    return nullptr;
  }
};

class ArrayDimension : public AST
{
public:
  bool missingFirstDimension;

  ArrayDimension() : missingFirstDimension(false), dimensions() {}
  // ~ArrayDimension() { for (IntConst *d : dimensions) delete d; }

  void add_dimension(int d)
  {
    dimensions.insert(dimensions.begin(), d);
  }

  void printOn(std::ostream &out) const override
  {
    out << "ArrayDimension(";
    if (missingFirstDimension)
      out << "[]";
    for (const auto &d : dimensions)
    {
      out << "[" << d << "]";
    }
    out << ")";
  }

  std::vector<int> getDimensions() const
  {
    return dimensions;
  }

  virtual llvm::Value *codegen() override {
    return nullptr;
  }

private:
  std::vector<int> dimensions;
};

class VariableType : public AST
{
public:
  VariableType(DataType t, ArrayDimension *d) : datatype(t), dim(d) {}
  ~VariableType() { delete dim; }

  void printOn(std::ostream &out) const override
  {
    out << "VariableType(" << datatype << ", " << *dim << ")";
  }

  // TODO: implement array types
  DataType getDataType() const { return datatype; }

  llvm::Type * get_llvm_variable_type() {
    llvm::Type *base_type = nullptr;
    switch (datatype)
    {
    case DataType::TYPE_int:
      base_type = i32;
      break;
    case DataType::TYPE_char:
      base_type = i8;
      break;
    case DataType::TYPE_nothing:
      base_type = llvm::Type::getVoidTy(TheContext);
      break;
    }
    if(dim->getDimensions().empty()) return base_type;
    std::vector<int> dimensions = dim->getDimensions();
    for(std::vector<int>::reverse_iterator it = dimensions.rbegin(); it != dimensions.rend(); ++it ) {
      base_type = llvm::ArrayType::get(base_type, *it);
    }
    return base_type;
  }

  std::vector<int> getDimensions() const
  {
    return dim->getDimensions();
  }

  bool getMissingFirstDimension() const
  {
    return dim->missingFirstDimension;
  }

  virtual llvm::Value *codegen() override {
    return nullptr;
  }

private:
  DataType datatype;
  ArrayDimension *dim;
};

class FuncParam : public AST
{
public:
  FuncParam(std::string *il, VariableType *t, PassingType pt, int lineno) : id(il), param_type(t), passing_type(pt) {line_number = lineno;}
  ~FuncParam()
  {
    delete id;
    delete param_type;
  }

  int line_number = 0;

  void printOn(std::ostream &out) const override
  {
    out << "FuncParam(" << (bool(passing_type) ? "reference, " : "value, ") << *id << ", " << *param_type << ")";
  }

  std::tuple<DataType, PassingType, std::vector<int>, bool> getParam() const
  {
    return std::make_tuple(param_type->getDataType(), passing_type, param_type->getDimensions(), param_type->getMissingFirstDimension());
  }

  std::string get_param_name() const
  {
    return *id;
  }

  virtual void sem() override
  {
    if(passing_type == PassingType::BY_VALUE && (!param_type->getDimensions().empty() || param_type->getMissingFirstDimension())) {
      yyerror2("Cannot pass array by value", line_number);
    }
    st.insert_param(*id, param_type->getDataType(), passing_type, param_type->getDimensions(), param_type->getMissingFirstDimension(), line_number);
  }

  llvm::Type *get_llvm_type() const {
    DataType type = param_type->getDataType();
    llvm::Type *baseType;
    switch (type)
    {
    case DataType::TYPE_int:
    {
      baseType = i32;
      break;
    }
    case DataType::TYPE_char:
    {
      baseType = i8;
      break;
    }
    case DataType::TYPE_nothing:
      {
        baseType = llvm::Type::getVoidTy(TheContext);
        break;
      }
    }
    if(passing_type == PassingType::BY_REFERENCE) {
      // if not-array, or array with 1 dimension which is unknown at compile time
      if(param_type->getDimensions().empty()) {
        return baseType->getPointerTo();
      }
      std::vector<int> dimensions = param_type->getDimensions();
      std::reverse(dimensions.begin(), dimensions.end());
      if (!(param_type->getMissingFirstDimension()))
        dimensions.pop_back();
      for(unsigned dimension : dimensions) {
        baseType = llvm::ArrayType::get(baseType, dimension);
      }
      return baseType->getPointerTo();
    }
    return baseType;
  }

  virtual llvm::Value *codegen() override {
    return nullptr;
  }

private:
  std::string *id;
  VariableType *param_type;
  PassingType passing_type;
};

class FuncParamList : public AST
{
public:
  std::vector<FuncParam *> param_list;

  void add_param(FuncParam *p)
  {
    param_list.push_back(p);
  }

  FuncParamList(IdList *il, VariableType *fpt, int lineno, PassingType pt = PassingType::BY_VALUE)
  {
    for (const auto &id : il->id_list)
    {
      add_param(new FuncParam(id, fpt, pt, lineno));
    }
  }
  ~FuncParamList()
  {
    for (FuncParam *p : param_list)
      delete p;
  }

  void join(FuncParamList *other)
  {
    param_list.insert(param_list.end(), other->param_list.begin(), other->param_list.end());
  }

  void printOn(std::ostream &out) const override
  {
    out << "FuncParamList(";
    bool first = true;
    for (const auto &p : param_list)
    {
      if (!first)
        out << ", ";
      first = false;
      out << *p;
    }
    out << ")";
  }

  virtual void sem() override
  {
    for (const auto &p : param_list)
    {
      p->sem();
    }
  }

  virtual llvm::Value *codegen() override {
    return nullptr;
  }
};

class Header : public AST
{
public:
  Header(std::string *i, DataType t, int lineno, FuncParamList *p = nullptr) : id(i), returntype(t), paramlist(p) {line_number = lineno;}
  ~Header()
  {
    delete id;
    delete paramlist;
  }

  int line_number = 0;
  int get_line_number() const {return line_number;}

  void printOn(std::ostream &out) const override
  {
    out << "Header(" << *id << ": " << returntype;
    if (paramlist != nullptr)
      out << ", " << *paramlist;
    out << ")";
  }

  bool was_declared() const
  {
    return st.was_declared(*id);
  }

  void declare() {
    if(was_declared()) {
      yyerror2("Function already declared", line_number);
    }
    std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> param_types;
    if (paramlist != nullptr)
    {
      for (const auto &p : paramlist->param_list)
      {
        param_types.push_back(p->getParam());
      }
    }
    st.insert_function_declaration(*id, returntype, param_types, line_number);
  }

  void define() {
    if(!was_declared()) {
      yyerror2("Function not declared", line_number);
    }
    std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> param_types;
    if (paramlist != nullptr)
    {
      for (const auto &p : paramlist->param_list)
      {
        param_types.push_back(p->getParam());
      }
    }
    st.insert_function_definition(*id, returntype, param_types, line_number);
  }

  void define_main(){
    if(paramlist != nullptr) {
      yyerror2("Main function cannot have parameters", line_number);
    }
    if(returntype != DataType::TYPE_nothing) {
      yyerror2("Main function must return nothing", line_number);
    }
    st.insert_function(*id, returntype, std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>>(), line_number);
  }

  virtual void sem() override
  {
    std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> param_types;
    if (paramlist != nullptr)
    {
      for (const auto &p : paramlist->param_list)
      {
        param_types.push_back(p->getParam());
      }
    }
    st.insert_function(*id, returntype, param_types, line_number);
  }

  void register_param_list()
  {
    if (paramlist != nullptr)
    {
      paramlist->sem();
    }
  }

  std::string get_name() const
  {
    return *id;
  }

  DataType get_return_type() const
  {
    return returntype;
  }

  unsigned long int get_params_size() const
  {
    if (paramlist == nullptr)
      return 0;
    return paramlist->param_list.size();
  }

  llvm::FunctionType *get_llvm_function_type() {
    std::string current_function_name = std::string(Builder.GetInsertBlock()->getParent()->getName());
    std::vector<llvm::Type *> locals_params(NamedValues[current_function_name].size());
    int i = 0;
    llvm::Type *local_value_type;
    for(auto &it : NamedValues[current_function_name]) {
      // std::cout << "it.second: " << it.second << std::endl;
      local_value_type = it.second->getType();
      /* This is to stop adding more pointers to the local type */
      if(local_value_type->isPointerTy()){
        if(local_value_type->getPointerElementType()->isPointerTy()){
          locals_params[i++] = local_value_type->getPointerElementType();
        }
        else{
          locals_params[i++] = local_value_type;
        }
      }
    }
    if(paramlist == nullptr) {
      switch(returntype) {
        case DataType::TYPE_int:
          {
            if(locals_params.empty())
              return llvm::FunctionType::get(i32, false);
            else
              return llvm::FunctionType::get(i32, locals_params, false);
          }
        case DataType::TYPE_char:
          {
            if(locals_params.empty())
              return llvm::FunctionType::get(i8, false);
            else
              return llvm::FunctionType::get(i8, locals_params, false);
          }
        case DataType::TYPE_nothing:
          {
            if(locals_params.empty())
              return llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), false);
            else
              return llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), locals_params, false);
          }
      }
    }
    else {
      std::vector<llvm::Type *> llvm_param_types(paramlist->param_list.size());
      for (unsigned long int i = 0; i < paramlist->param_list.size(); i++)
      {
        llvm_param_types[i] = paramlist->param_list[i]->get_llvm_type();
      }
      std::vector<llvm::Type *> argument_types(locals_params.size() + llvm_param_types.size());
      std::copy(llvm_param_types.begin(), llvm_param_types.end(), argument_types.begin());
      std::copy(locals_params.begin(), locals_params.end(), argument_types.begin() + llvm_param_types.size());
      switch(returntype) {
        case DataType::TYPE_int:
          return llvm::FunctionType::get(i32, argument_types, false);
        case DataType::TYPE_char:
          return llvm::FunctionType::get(i8, argument_types, false);
        case DataType::TYPE_nothing:
          return llvm::FunctionType::get(llvm::Type::getVoidTy(TheContext), argument_types, false);
      }
    }
    return nullptr;
  }

  virtual llvm::Function *codegen() override {
    llvm::FunctionType *FT = get_llvm_function_type();
    std::string function_name = std::string("user_") + *id;
    llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, function_name, TheModule.get());
    //set argument names
    unsigned long int i = 0;
    //TODO : UPDATE THIS
    for (auto &Arg : F->args()) {
      if(paramlist == nullptr) {
        Arg.setName(std::string("local") + std::to_string(i++));
        continue;
      }
      if(i >= paramlist->param_list.size()) {
        Arg.setName(std::string("local") + std::to_string(i++));
        continue;
      }
      Arg.setName(paramlist->param_list[i++]->get_param_name());
    }
    return F;
  }

private:
  std::string *id;
  DataType returntype;
  FuncParamList *paramlist;
};

class LocalDefinition : public AST
{
public:
  virtual void printOn(std::ostream &out) const = 0;
  virtual bool isVariableDefinition() const { return false; }
  virtual std::string get_variable_name() const { return ""; }
  virtual llvm::Value *get_init_value() const { return nullptr; }
  virtual llvm::Type *get_llvm_variable_type() const { return nullptr; }
  int line_number = 0;
};

class VariableDefinition : public LocalDefinition
{
public:
  VariableDefinition(std::string *i, VariableType *vt, int lineno) : id(i), variable_type(vt) {line_number = lineno;}
  ~VariableDefinition()
  {
    delete id;
    delete variable_type;
  }
  void printOn(std::ostream &out) const override
  {
    out << "VariableDefinition(" << *id << ", " << *variable_type << ")";
  }

  virtual bool isVariableDefinition() const override { return true; }

  virtual std::string get_variable_name() const override
  {
    return *id;
  }

  virtual llvm::Type *get_llvm_variable_type() const override
  {
    return variable_type->get_llvm_variable_type();
  }

  virtual llvm::Value *get_init_value() const override
  {
    switch (variable_type->getDataType())
    {
    case DataType::TYPE_int:
      return c32(0);
    case DataType::TYPE_char:
      return c8(0);
    case DataType::TYPE_nothing:
      return nullptr;
    }
    return nullptr;
  }

  virtual void sem() override
  {
    st.insert_variable(*id, variable_type->getDataType(), variable_type->getDimensions(), line_number);
  }

  virtual llvm::AllocaInst *codegen() override {
    return nullptr;
  }

private:
  std::string *id;
  VariableType *variable_type;
};

class LocalDefinitionList : public AST
{
public:
  std::vector<std::shared_ptr<LocalDefinition>> local_definition_list;

  unsigned int get_vars_count() const
  {
    unsigned int count = 0;
    for (const auto &ld : local_definition_list)
    {
      if (ld->isVariableDefinition())
        count++;
    }
    return count;
  }

  LocalDefinitionList() : local_definition_list() {}

  void add_variable_definition_list(IdList *il, VariableType *vt, int lineno)
  {
    for (const auto &id : il->id_list)
    {
      add_local_definition(new VariableDefinition(id, vt, lineno));
    }
  }

  void add_local_definition(LocalDefinition *ld)
  {
    local_definition_list.push_back(std::shared_ptr<LocalDefinition>(ld));
  }

  void join(LocalDefinitionList *other)
  {
    local_definition_list.insert(local_definition_list.end(), other->local_definition_list.begin(), other->local_definition_list.end());
  }

  void printOn(std::ostream &out) const override
  {
    out << "LocalDefinitionList(";
    bool first = true;
    for (const auto &ld : local_definition_list)
    {
      if (!first)
        out << ", ";
      first = false;
      out << *ld;
    }
    out << ")";
  }

  virtual void sem() override
  {
    for (const auto &ld : local_definition_list)
    {
      ld->sem();
    }
  }

  virtual llvm::Value *codegen() override {
    return nullptr;
  }
};

class FunctionDeclaration : public LocalDefinition
{
public:
  FunctionDeclaration(Header *h) : header(h) {}

  void printOn(std::ostream &out) const override
  {
    out << "FunctionDeclaration(" << *header << ")";
  }

  virtual bool isVariableDefinition() const override { return false; }

  virtual void sem() override
  {
    header->declare();
  }

  virtual llvm::Value *codegen() override {
    llvm::BasicBlock *OuterBlock = Builder.GetInsertBlock();
    std::string function_parent_name = std::string(OuterBlock->getParent()->getName());

    llvm::Function *TheFunction = header->codegen();
    llvm::BasicBlock *L1 = llvm::BasicBlock::Create(TheContext, "entry", TheFunction);
    Builder.SetInsertPoint(L1);
    std::string function_name = std::string(TheFunction->getName());
    FunctionTranslationTablesRealToLocal[function_name] = new std::map<std::string, std::string>();
    FunctionTranslationTablesLocalToReal[function_name] = new std::map<std::string, std::string>();
    std::map<std::string, std::string> *RealToLocalTranslations = FunctionTranslationTablesRealToLocal[function_name];
    std::map<std::string, std::string> *LocalToRealTranslations = FunctionTranslationTablesLocalToReal[function_name];

    std::map<std::string, std::string> *OldLocalToRealTranslations = FunctionTranslationTablesLocalToReal[function_parent_name];
    std::map<std::string, llvm::Value *>::iterator it = NamedValues[function_parent_name].begin();
    for (auto argIt = TheFunction->arg_begin(); argIt != TheFunction->arg_end(); ++argIt) {
      llvm::Value *arg = &*argIt;
      std::string arg_name = std::string(arg->getName());
      if(argIt < TheFunction->arg_begin() + header->get_params_size()) {
        (*RealToLocalTranslations)[arg_name] = arg_name;
        (*LocalToRealTranslations)[arg_name] = arg_name;
        // std::cout << "Translating " << arg_name << " to " << arg_name << std::endl;
        AST::logToFile("Translating arg " + arg_name + " to " + arg_name);
        llvm::AllocaInst *alloca = Builder.CreateAlloca(argIt->getType(), nullptr, arg_name);
        Builder.CreateStore(argIt, alloca);
        AST::logToFile("Creating alloca for " + arg_name + " in function " + function_name);
        NamedValues[function_name][arg_name] = alloca;
        continue;
      }
      AST::logToFile("old local to real for " + it->first + " is " + (*OldLocalToRealTranslations)[it->first]);
      if((*RealToLocalTranslations).find((*OldLocalToRealTranslations)[it->first]) != (*RealToLocalTranslations).end()) {
        AST::logToFile("FOUND OLD NAME = TO A PARAM NAME while Translating local " + (*OldLocalToRealTranslations)[it->first] + " to " + arg_name);
        (*RealToLocalTranslations)[(*OldLocalToRealTranslations)[it->first]] = (*OldLocalToRealTranslations)[it->first];
      }
      else {
        (*RealToLocalTranslations)[(*OldLocalToRealTranslations)[it->first]] = arg_name;
        AST::logToFile("Translating local " + (*OldLocalToRealTranslations)[it->first] + " to " + arg_name);
      }
      (*LocalToRealTranslations)[arg_name] = (*OldLocalToRealTranslations)[it->first];
      // std::cout << "Translating " << (*OldLocalToRealTranslations)[*it] << " to " << arg_name << std::endl;
      llvm::AllocaInst *alloca = Builder.CreateAlloca(argIt->getType(), nullptr, arg_name);
      Builder.CreateStore(argIt, alloca);
      AST::logToFile("Creating alloca for " + arg_name + " in function " + function_name);
      NamedValues[function_name][arg_name] = alloca;
      ++it;
    }

    Builder.SetInsertPoint(OuterBlock);
    return nullptr;
  }

private:
  Header *header;
};

class FunctionDefinition : public LocalDefinition
{
public:
  FunctionDefinition(Header *h, LocalDefinitionList *d, Block *b, int lineno = 0) : header(h), definition_list(d), block(b) {line_number = lineno;}
  ~FunctionDefinition() {
    AST::logToFile("Deleting function definition");
    delete header;
    delete definition_list;
    delete block;
  }

  void printOn(std::ostream &out) const override
  {
    out << "FunctionDefinition(" << *header << ", " << *definition_list << ", " << *block << ")";
  }

  virtual bool isVariableDefinition() const override { return false; }

  virtual void sem() override
  {
    if (st.not_exists_scope()) {
      //main (outermost) function
      st.openScope();
      st.init_library_functions();
      header->define_main();
      st.openScope(header->get_return_type());
      definition_list->sem();
      block->sem();
      st.check_undefined_functions();
      st.closeScope();
      return;
    }
    if(!header->was_declared()){
      header->sem();
    }
    else {
      header->define();
    }
    st.openScope(header->get_return_type());
    header->register_param_list();
    definition_list->sem();
    block->sem();
    st.check_return_exists(line_number);
    st.check_undefined_functions();
    // std::cout<<"Before end of scope"<<std::endl;
    // st.display();
    st.closeScope();
    // std::cout<<"After end of scope"<<std::endl;
    // st.display();
  }

  virtual llvm::Value *codegen() override {
    //get outer function
    llvm::BasicBlock *OuterBlock = Builder.GetInsertBlock();
    std::string function_name = std::string("user_") + header->get_name();
    std::string function_parent_name = std::string(OuterBlock->getParent()->getName());
    llvm::Function *TheFunction = TheModule->getFunction(function_name);
    llvm::BasicBlock *L1;
    std::map<std::string, std::string> *RealToLocalTranslations;
    std::map<std::string, std::string> *LocalToRealTranslations;
    if(TheFunction == nullptr) {
      // std::cout << "Function " << header->get_name() << " was not declared" << std::endl;
      TheFunction = header->codegen();
      L1 = llvm::BasicBlock::Create(TheContext, "entry", TheFunction);
      Builder.SetInsertPoint(L1);
      FunctionTranslationTablesRealToLocal[function_name] = new std::map<std::string, std::string>();
      FunctionTranslationTablesLocalToReal[function_name] = new std::map<std::string, std::string>();
      RealToLocalTranslations = FunctionTranslationTablesRealToLocal[function_name];
      LocalToRealTranslations = FunctionTranslationTablesLocalToReal[function_name];
      std::map<std::string, std::string> *OldLocalToRealTranslations = FunctionTranslationTablesLocalToReal[function_parent_name];
    
      std::map<std::string, llvm::Value *>::iterator it = NamedValues[function_parent_name].begin();
      for (auto argIt = TheFunction->arg_begin(); argIt != TheFunction->arg_end(); ++argIt) {
        llvm::Value *arg = &*argIt;
        std::string arg_name = std::string(arg->getName());
        if(argIt < TheFunction->arg_begin() + header->get_params_size()) {
          (*RealToLocalTranslations)[arg_name] = arg_name;
          (*LocalToRealTranslations)[arg_name] = arg_name;
          // std::cout << "Translating " << arg_name << " to " << arg_name << std::endl;
          AST::logToFile("Translating arg " + arg_name + " to " + arg_name);
          llvm::AllocaInst *alloca = Builder.CreateAlloca(argIt->getType(), nullptr, arg_name);
          Builder.CreateStore(argIt, alloca);
          AST::logToFile("Creating alloca for " + arg_name + " in function " + function_name);
          NamedValues[function_name][arg_name] = alloca;
          continue;
        }
        AST::logToFile("old local to real for " + it->first + " is " + (*OldLocalToRealTranslations)[it->first]);
        if((*RealToLocalTranslations).find((*OldLocalToRealTranslations)[it->first]) != (*RealToLocalTranslations).end()) {
          AST::logToFile("FOUND OLD NAME = TO A PARAM NAME while Translating local " + (*OldLocalToRealTranslations)[it->first] + " to " + arg_name);
          (*RealToLocalTranslations)[(*OldLocalToRealTranslations)[it->first]] = (*OldLocalToRealTranslations)[it->first];
        }
        else {
          (*RealToLocalTranslations)[(*OldLocalToRealTranslations)[it->first]] = arg_name;
          AST::logToFile("Translating local " + (*OldLocalToRealTranslations)[it->first] + " to " + arg_name);
        }
        (*LocalToRealTranslations)[arg_name] = (*OldLocalToRealTranslations)[it->first];
        // std::cout << "Translating " << (*OldLocalToRealTranslations)[*it] << " to " << arg_name << std::endl;
        llvm::AllocaInst *alloca = Builder.CreateAlloca(argIt->getType(), nullptr, arg_name);
        Builder.CreateStore(argIt, alloca);
        AST::logToFile("Creating alloca for " + arg_name + " in function " + function_name);
        NamedValues[function_name][arg_name] = alloca;
        ++it;
      }
    }
    else {
      llvm::BasicBlock &lastBlock = TheFunction->back();
      Builder.SetInsertPoint(&lastBlock, lastBlock.end());
      RealToLocalTranslations = FunctionTranslationTablesRealToLocal[function_name];
      LocalToRealTranslations = FunctionTranslationTablesLocalToReal[function_name];
    }

    // std::map<std::string, std::string> *OldLocalToRealTranslations = FunctionTranslationTablesLocalToReal[std::string(OuterBlock->getParent()->getName())];
    // std::vector<std::string>::iterator it = OldNames.begin();
    // for (auto argIt = TheFunction->arg_begin(); argIt != TheFunction->arg_end(); ++argIt) {
    //   llvm::Value *arg = &*argIt;
    //   std::string arg_name = std::string(arg->getName());
    //   if(argIt < TheFunction->arg_begin() + header->get_params_size()) {
    //     (*RealToLocalTranslations)[arg_name] = arg_name;
    //     (*LocalToRealTranslations)[arg_name] = arg_name;
    //     // std::cout << "Translating " << arg_name << " to " << arg_name << std::endl;
    //     AST::logToFile("Translating arg " + arg_name + " to " + arg_name);
    //     continue;
    //   }
    //   if((*RealToLocalTranslations).find((*OldLocalToRealTranslations)[*it]) != (*RealToLocalTranslations).end()) {
    //     AST::logToFile("FOUND OLD NAME = TO A PARAM NAME while Translating local " + (*OldLocalToRealTranslations)[*it] + " to " + arg_name);
    //     (*RealToLocalTranslations)[(*OldLocalToRealTranslations)[*it]] = (*OldLocalToRealTranslations)[*it];
    //   }
    //   else {
    //     (*RealToLocalTranslations)[(*OldLocalToRealTranslations)[*it]] = arg_name;
    //     AST::logToFile("Translating local " + (*OldLocalToRealTranslations)[*it] + " to " + arg_name);

    //   }
    //   (*LocalToRealTranslations)[arg_name] = (*OldLocalToRealTranslations)[*it];
    //   // std::cout << "Translating " << (*OldLocalToRealTranslations)[*it] << " to " << arg_name << std::endl;
    //   llvm::AllocaInst *alloca = Builder.CreateAlloca(Arg.getType(), nullptr, arg_name);
    //   Builder.CreateStore(&Arg, alloca);
    //   NamedValues[function_name][param_name] = alloca;
    //   ++it;
    // }
    for (const auto &ld : definition_list->local_definition_list) {
      if(ld == nullptr) yyerror2("Warning: Found a null shared_ptr in local_definition_list.", 0);
      if(ld->isVariableDefinition()) {
        std::string var_name = ld->get_variable_name();
        llvm::Type *var_type = ld->get_llvm_variable_type();
        llvm::Value *alloca = Builder.CreateAlloca(var_type, nullptr, var_name);
        // llvm::Value *init = ld->get_init_value();
        // Builder.CreateStore(init, alloca);
        // OldBindings.push_back(NamedValues[var_name]);
        // DeclaredVariables.push_back(var_name);
        if(alloca->getType()->isPointerTy() && alloca->getType()->getPointerElementType()->isArrayTy()) {
          alloca = Builder.CreateGEP(alloca, std::vector<llvm::Value *>({c32(0), c32(0)}), var_name);
        }
        NamedValues[function_name][var_name] = alloca;
        RealToLocalTranslations->insert(std::pair<std::string, std::string>(var_name, var_name));
        LocalToRealTranslations->insert(std::pair<std::string, std::string>(var_name, var_name));
        AST::logToFile("Translating (declared) " + var_name + " to " + var_name);
        // std::cout << "Translating (declared) " << var_name << " to " << var_name << std::endl;
      }
      else {
        ld->codegen();
      }
    }
    //handle function body
    block->codegen();

    // NamedValues.clear();
    // for(unsigned i = 0, e = OldNames.size(); i != e; ++i) {
    //   NamedValues[OldNames[i]] = OldBindings[i];
    // }
    if(Builder.GetInsertBlock()->getTerminator() == nullptr) {
      if(header->get_return_type() == DataType::TYPE_nothing)
        Builder.CreateRetVoid();
      if(header->get_return_type() == DataType::TYPE_int)
        Builder.CreateRet(c32(0));
      if(header->get_return_type() == DataType::TYPE_char)
        Builder.CreateRet(c8(0));
    }
    Builder.SetInsertPoint(OuterBlock);
    if(OuterBlock->getParent()->getName() == "main") {
      Builder.CreateCall(TheFunction);
    }

    // TheFPM->run(*TheFunction);
    return nullptr;
  }

private:
  Header *header;
  LocalDefinitionList *definition_list;
  Block *block;
};
