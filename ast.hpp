#pragma once

#include <iostream>
#include <map>
#include <vector>
#include "symbol.hpp"
#include <memory>


class AST {
public:
  virtual ~AST() {}
  virtual void printOn(std::ostream &out) const = 0;
  virtual void sem() {}
};

inline std::ostream& operator<< (std::ostream &out, const AST &t) {
  t.printOn(out);
  return out;
}

inline std::ostream &operator<<(std::ostream &out, DataType t) {
  switch (t) {
    case TYPE_int:  out << "int";  break;
    case TYPE_char: out << "char"; break;
    case TYPE_nothing: out << "nothing"; break;
  }
  return out;
}

class Expr: public AST {
public:
  virtual int eval() const = 0;
};

class Stmt: public AST {
public:
  virtual void run() const = 0;
};

class VarDecl: public Stmt {
public:
    VarDecl(const std::string &s, DataType t): var(s), datatype(t) {}

    void printOn(std::ostream &out) const override {
        out << "VarDecl(" << var << ": " << datatype << ")";
  }
private:
  std::string var;
  DataType datatype;
};

class FunDecl: public Stmt {
public:
    FunDecl(const std::string &s, DataType t, std::vector<std::tuple<DataType, std::string>>  p): var(s), rettype(t), params(p) {}

    void printOn(std::ostream &out) const override {
        out << "FunDecl(" << var << ": " << rettype << ")";
  }
private:
  std::string var;
  DataType rettype;
  std::vector<std::tuple<DataType, std::string>> params;
};

class IntConst: public Expr {
public:
    IntConst(int n): num(n) {}
    virtual void printOn(std::ostream &out) const override {
    out << "IntConst(" << num << ")";
  }
  virtual int eval() const override {
    return num;
  }

private:
    int num;
};

class CharConst: public Expr {
public:
    CharConst(char c): charval(c) {}
    virtual void printOn(std::ostream &out) const override {
    out << "CharConst(" << charval << ")";
  }
  virtual int eval() const override {
    return charval;
  }

private:
    char charval;
};

class Id: public Expr {
public:
  Id(std::string *s): var(s) {}

  void printOn(std::ostream &out) const override {
    out << "Id(" << *var << ")";
  }
    // TODO: implement
  virtual int eval() const override {
    return 0;
  }

private:
    std::string* var;
};


class StringLiteral: public Expr {
public:
  StringLiteral(std::string *s): stringval(s) {}
  ~StringLiteral() { delete stringval; }

  void printOn(std::ostream &out) const override {
    out << "StringLiteral(" << *stringval << ")";
  }

  // TODO: implement
  virtual int eval() const override {
    return 0;
  }

private:
    std::string* stringval;
};

class ArrayAccess: public Expr {
public:
  ArrayAccess(Expr* obj, Expr* pos): object(obj), position(pos) {}
  ~ArrayAccess() { delete object; delete position; }

  void printOn(std::ostream &out) const override {
    out << "ArrayAccess(" << *object << ", " << *position << ")";
  }

  // TODO: implement
  virtual int eval() const override {
    return 0;
  }

private:
  Expr* object;
  Expr* position;
};

class ExpressionList: public Expr {
public:
  std::vector<Expr*> expressions;

  ExpressionList(Expr *e): expressions() {expressions.push_back(e); }
  ~ExpressionList() { for (Expr *e: expressions ) delete e; }

  void add_expression(Expr *e) { expressions.insert(expressions.begin(), e); }
  void printOn(std::ostream &out) const override {
      out << "ExpressionList(";
      bool first = true;
      for (const auto &e : expressions) {
      if (!first) out << ", ";
      first = false;
      out << *e;
      }
      out << ")";
  }

  // TODO: implement
  virtual int eval() const override {
    return 0;
  }
};

class FunctionCall: public Expr, public Stmt {
public:
  FunctionCall(std::string *i, ExpressionList* el = nullptr): id(i), args(el) {}
  ~FunctionCall() { delete id; delete args;}

  void printOn(std::ostream &out) const override {
    out << "FunctionCall(" << *id ;
    if(args != nullptr)
        out << ", " << *args ;
    out << ")";
  }

  // TODO: implement
  virtual int eval() const override {
    return 0;
  }

  // TODO: implement
  virtual void run() const override {
    return ;
  }

private:
  std::string *id;
  ExpressionList *args;
};

class Negative: public Expr {
public:
    Negative(Expr *e): expr(e) {}
    ~Negative() { delete expr; }
    virtual void printOn(std::ostream &out) const override {
    out << "Negative(" << *expr << ")";
  }
  virtual int eval() const override {
    return -(expr->eval());
  }

private:
    Expr *expr;

};

class BinOp: public Expr {
public:
    BinOp(Expr *l, char o, Expr *r): left(l), op(o), right(r) {}
    ~BinOp() { delete left; delete right; }

    virtual void printOn(std::ostream &out) const override {
        out << op << "(" << *left << ", " << *right << ")";
    }

    virtual int eval() const override {
        switch (op) {
            case '+': return left->eval() + right->eval();
            case '-': return left->eval() - right->eval();
            case '*': return left->eval() * right->eval();
            case '/': return left->eval() / right->eval();
            case '%': return left->eval() % right->eval();
            case '=' : return left->eval() == right->eval();
            case '#' : return left->eval() != right->eval();
            case '<' : return left->eval() < right->eval();
            case '>' : return left->eval() > right->eval();
            case 'l' : return left->eval() <= right->eval();
            case 'g' : return left->eval() >= right->eval();

            case '&' : return { !(left->eval()) ? false : right->eval() };
            case '|' : return { (left->eval()) ? true : right->eval() };
        }
        return 0;  // this will never be reached
    }
private:
    Expr *left;
    char op;
    Expr *right;
};

class Not: public Expr {
public:
    Not(Expr *c): cond(c) {}
    ~Not() { delete cond; }
    virtual void printOn(std::ostream &out) const override {
        out << "Not" << "(" << *cond << ")";
    }
    virtual int eval() const override {
        return !(cond->eval());
    }

private:
        Expr *cond;
};

class Block: public Stmt {
public:
    Block(): stmt_list() {}
    ~Block() { for(Stmt *s : stmt_list) delete s; }
    void append_stmt(Stmt *s) { stmt_list.push_back(s); }
    void printOn(std::ostream &out) const override {
        out << "Block(";
        bool first = true;
        for (const auto &s : stmt_list) {
        if (!first) out << ", ";
        first = false;
        out << *s;
        }
        out << ")";
    }
    void run() const override {
        for(Stmt *s : stmt_list) s->run();
    }

private:
    std::vector<Stmt *> stmt_list;
};

class If: public Stmt {
public:
    If(Expr *c, Stmt *s1, Stmt *s2 = nullptr): cond(c), stmt1(s1), stmt2(s2) {}
    ~If() { delete cond; delete stmt1; delete stmt2; }
    void printOn(std::ostream &out) const override {
        out << "If(" << *cond << ", " << *stmt1;
        if (stmt2 != nullptr) out << ", " << *stmt2;
        out << ")";
    }
    void run() const override {
        if(cond->eval())
            stmt1->run();
        else
            stmt2->run();
    }

private:
    Expr *cond;
    Stmt *stmt1;
    Stmt *stmt2;
};

class While: public Stmt {
public:
  While(Expr *c, Stmt *s): cond(c), stmt(s) {}
  ~While() { delete cond; delete stmt; }
  void printOn(std::ostream &out) const override {
    out << "While(" << *cond << " do " << *stmt; out << ")";
  }
  void run() const override {
    while(cond->eval()) stmt->run();
  }

private:
    Expr *cond;
    Stmt *stmt;
};

class EmptyStmt: public Stmt {
public:
  EmptyStmt() {}
  void printOn(std::ostream &out) const override {
    out << "EmptyStmt()";
  }
  void run() const override {}
};

class Assignment: public Stmt {
public:
  Assignment(Expr *l, Expr *e): l_value(l), expr(e) {}
  ~Assignment() { delete l_value; delete expr; }
  void printOn(std::ostream &out) const override {
    out << "Assignment(" << *l_value << ", " << *expr << ")";
  }

    // TODO: implement
  void run() const override {
  }

private:
  Expr *l_value;
  Expr *expr;
};

class Return: public Stmt {
public:
  Return(Expr *e = nullptr): expr(e) {}
  ~Return() { delete expr; }

  // TODO: implement
  void run() const override {
  }

  void printOn(std::ostream &out) const override {
    out << "Return(";
    if(expr != nullptr) out << *expr;      
    out << ")";
  }
private:
Expr *expr;
};

class IdList : public AST {
public:
  std::vector<std::string*> id_list;
  IdList(std::string *i) { append_id(i); }

  void append_id(std::string *id) {
    id_list.insert(id_list.begin(), id);
  }

  void printOn(std::ostream &out) const override {
    out << "IdList(";
    bool first = true;
    for (const auto &id : id_list) {
      if (!first) out << ", ";
      first = false;
      out << *id;
    }
    out << ")";
  }
};

class ArrayDimension: public AST {
public:
  bool missingFirstDimension;

  ArrayDimension(): missingFirstDimension(false), dimensions() {}
  // ~ArrayDimension() { for (IntConst *d : dimensions) delete d; }

  void add_dimension(int d) {
    dimensions.insert(dimensions.begin(), d);
  }

  void printOn(std::ostream &out) const override {
    out << "ArrayDimension(";
    if (missingFirstDimension) out << "[]";
    for (const auto &d : dimensions) {
      out << "[" << d << "]";
    }
    out << ")";
  }

private:
  std::vector<int> dimensions;
};

class VariableType: public AST {
public:
  VariableType(DataType t, ArrayDimension *d): datatype(t), dim(d) {}
  ~VariableType() { delete dim; }

  void printOn(std::ostream &out) const override {
    out << "VariableType(" << datatype << ", " << *dim << ")";
  }

  DataType getDataType() const { return datatype; }

private:
  DataType datatype;
  ArrayDimension *dim;
};


class FuncParam: public AST {
public:
  FuncParam(std::string *il, VariableType *t, PassingType pt): id(il), param_type(t), passing_type(pt) {}
  ~FuncParam() { delete id; delete param_type; }

  void printOn(std::ostream &out) const override {
    out << "FuncParam(" << (bool(passing_type) ? "reference, ":"value, ") << *id << ", " << *param_type << ")";
  }

private:
  std::string *id;
  VariableType *param_type;
  PassingType passing_type;
};

class FuncParamList: public AST {
public:
  std::vector<FuncParam*> param_list;

  void add_param(FuncParam *p) {
    param_list.push_back(p);
  }

  FuncParamList(IdList* il, VariableType* fpt, PassingType pt = PassingType::BY_VALUE) {
    for (const auto &id : il->id_list) {
      add_param(new FuncParam(id, fpt, pt));
    }
  }
  // ~FuncParamList() {}

  void join(FuncParamList *other) {
    param_list.insert(param_list.end(), other->param_list.begin(), other->param_list.end());
  }

  void printOn(std::ostream &out) const override {
    out << "FuncParamList(";
    bool first = true;
    for (const auto &p : param_list) {
      if (!first) out << ", ";
      first = false;
      out << *p;
    }
    out << ")";
  }
};

class Header: public AST {
public:
  Header(std::string *i, DataType t, FuncParamList *p = nullptr): id(i), returntype(t), paramlist(p) {}
  ~Header() { delete id; delete paramlist; }
  void printOn(std::ostream &out) const override {
    out << "Header(" << *id << ": " << returntype;
    if (paramlist != nullptr) out << ", " << *paramlist;
    out << ")";
  }

private:
  std::string *id;
  DataType returntype;
  FuncParamList *paramlist;
};

class LocalDefinition: public AST {
public:
  virtual void printOn(std::ostream &out) const = 0;
};

class VariableDefinition: public LocalDefinition {
public:
  VariableDefinition(std::string *i, VariableType *vt): id(i), variable_type(vt) {}
  ~VariableDefinition() {delete id; delete variable_type; }
  void printOn(std::ostream &out) const override {
    out << "VariableDefinition(" << *id << ", " << *variable_type << ")";
  }

  virtual void sem() override {
    std::cout << "VariableDefinition(" << *id << ", " << variable_type->getDataType() << ")" << std::endl;
    st.insert_variable(*id, variable_type->getDataType());
  }
private:
  std::string *id;
  VariableType *variable_type;
};

class LocalDefinitionList: public AST {
public:
  std::vector<std::shared_ptr<LocalDefinition>> local_definition_list;

  LocalDefinitionList(): local_definition_list() {}

  void add_variable_definition_list(IdList *il, VariableType *vt) {
    for (const auto &id : il->id_list) {
      add_local_definition(new VariableDefinition(id, vt));
    }
  }

  void add_local_definition(LocalDefinition *ld) {
    local_definition_list.push_back(std::shared_ptr<LocalDefinition>(ld));
  }

  void join(LocalDefinitionList *other) {
    local_definition_list.insert(local_definition_list.end(), other->local_definition_list.begin(), other->local_definition_list.end());
  }

  void printOn(std::ostream &out) const override {
    out << "LocalDefinitionList(";
    bool first = true;
    for (const auto &ld : local_definition_list) {
      if (!first) out << ", ";
      first = false;
      out << *ld;
    }
    out << ")";
  }

  virtual void sem() override {
    st.openScope();
    for (const auto &ld : local_definition_list) {
      ld->sem();
    }
    st.display();
    // st.closeScope();
  }
};

class FunctionDeclaration: public LocalDefinition {
public:
  FunctionDeclaration(Header *h): header(h) {}

  void printOn(std::ostream &out) const override {
    out << "FunctionDeclaration(" << *header << ")";
  }

private:
  Header *header;
};

// TODO: Complete this class
class FunctionDefinition: public LocalDefinition {
public:
  FunctionDefinition(Header *h, LocalDefinitionList *d, Block *b): header(h), definition_list(d), block(b) {}
  ~FunctionDefinition() { delete header; delete definition_list; delete block; }

  void printOn(std::ostream &out) const override {
    out << "FunctionDefinition(" << *header << 
      ", " << *definition_list << 
      ", " << *block << ")";
  }

private:
  Header *header;
  LocalDefinitionList *definition_list;
  Block *block;
};
