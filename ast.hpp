#pragma once

#include <iostream>
#include <map>
#include <vector>
#include "symbol.hpp"
#include <memory>

extern void yyerror2(const char *msg, int line_number);

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

  int line_number = 0;

  void type_check(DataType t, std::vector<int> dim = {}) {
    if(type != t) {
      yyerror2("Type mismatch", line_number);
      yyerror("Type mismatch");
    }
    if(dimensions != dim) {
      if(dimensions.empty() || dim.empty()) {
        yyerror("Type mismatch, array and non-array");
      }
      if(dimensions[0] == 0 || dim[0] == 0) {
        //the first dimension of either is unknown, so compare the rest
        for(unsigned int i = 1; i < dimensions.size(); i++) {
          if(dimensions[i] != dim[i]) {
            yyerror("Array dimension mismatch");
          }
        }
      }
      else {
        yyerror("Array dimension mismatch");
      }
    }
  }

  DataType get_type() const {
    return type;
  }

  EntryKind get_kind() const {
    return kind;
  }

  std::vector<int> get_dimensions() const {
    return dimensions;
  }

protected:
  DataType type;
  EntryKind kind;
  std::vector<int> dimensions;
};

class Stmt: public AST {
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

class IntConst: public Expr {
public:
    IntConst(int n): num(n) {}
    virtual void printOn(std::ostream &out) const override {
    out << "IntConst(" << num << ")";
  }
  virtual int eval() const override {
    return num;
  }

  virtual void sem() override {
    type = DataType::TYPE_int;
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

  virtual void sem() override {
    type = DataType::TYPE_char;
  }

private:
    char charval;
};

class Id: public Expr {
public:
  Id(std::string *s, int lineno = 0): var(s) { line_number = lineno; }

  void printOn(std::ostream &out) const override {
    out << "Id(" << *var << ")";
  }
    // TODO: implement
  virtual int eval() const override {
    return 0;
  }

  virtual void sem() override {
    std::cout<<"looking up "<<*var<<std::endl;
    STEntry *entry = st.lookup(*var);
    if (entry == nullptr) {
      yyerror("Variable not declared");
    }
    std::cout<<"entry kind: " << entry->kind << std::endl;
    if(!entry->dimensions.empty()) {
      dimensions = entry->dimensions;
    }
    if(entry->missingFirstDimension) {
      dimensions.insert(dimensions.begin(), 0);
    }
    type = entry->type;
    kind = entry->kind;
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

  virtual void sem() override {
    type = DataType::TYPE_char;
    dimensions.push_back(stringval->length()+1);
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

  virtual void sem() override {
    object->sem();
    position->sem();
    if(object->get_kind() == EntryKind::FUNCTION) {
      yyerror("Cannot index a function");
    }
    if(position->get_kind() == EntryKind::FUNCTION) {
      yyerror("Cannot index with a function");
    }
    position->type_check(DataType::TYPE_int); // maybe not needed
    if(object->get_dimensions().empty()) {
      yyerror("Cannot index a non-array");
    }
    type = object->get_type();
    dimensions = object->get_dimensions();
    dimensions.erase(dimensions.begin());
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

  virtual void sem() override {
    for (const auto &e : expressions) {
      e->sem();
    }
  }
};

class FunctionCall: public Expr, public Stmt {
public:
  FunctionCall(std::string *i, ExpressionList* el = nullptr, int lineno = 0): id(i), args(el) { 
    line_number = lineno;
  }
  FunctionCall(std::string *i, int lineno = 0): id(i) { line_number = lineno; }
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

  virtual void sem() override {
    STEntry *entry = st.lookup(*id);
    if (entry == nullptr) {
      yyerror2("Function not declared", line_number); 
    }
    if (entry->kind != EntryKind::FUNCTION) {
      yyerror2("Not a function", line_number);
    }
    type = entry->type;
    if(args == nullptr) {
      if(!entry->paramTypes.empty()) {
        yyerror2("No arguments provided", line_number);
      }
    }
    else {
      if(entry->paramTypes.size() != args->expressions.size()) {
        yyerror2("Wrong number of arguments", line_number);
      }
      std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>>::iterator param_it = entry->paramTypes.begin();
      for (const auto &e : args->expressions) {
        e->sem();
        std::vector<int> param_dimensions = std::get<2>(*param_it);
        if(std::get<3>(*param_it)) param_dimensions.insert(param_dimensions.begin(), 0);
        e->type_check(std::get<0>(*param_it), param_dimensions);
        ++param_it;
      }
    }
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

  virtual void sem() override {
    expr->sem();
    type = expr->get_type();
    dimensions = expr->get_dimensions();
    if(expr->get_kind() == EntryKind::FUNCTION) {
      yyerror("Cannot negate a function");
    }
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

    // TODO: maybe check to see if operands are ints only
    // otherwise we allow char operations
    virtual void sem() override {
      left->sem();
      right->sem();
      if(left->get_kind() == EntryKind::FUNCTION) {
        yyerror("left operand cannot be a function");
      }
      if(right->get_kind() == EntryKind::FUNCTION) {
        yyerror("right operand cannot be a function");
      }
      left->type_check(right->get_type(), right->get_dimensions());
      type = left->get_type();
      if(left->get_dimensions().empty() || left->get_dimensions().front() == 0) dimensions = right->get_dimensions();
      else dimensions = left->get_dimensions(); 
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

    virtual void sem() override {
      cond->sem();
      // if(cond->get_kind() == EntryKind::FUNCTION) {
      //   yyerror("Cannot negate a function");
      // }
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

    virtual void sem() override {
        for(Stmt *s : stmt_list) s->sem();
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

    virtual void sem() override {
        cond->sem();
        stmt1->sem();
        if(stmt2 != nullptr) stmt2->sem();
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

  virtual void sem() override {
    cond->sem();
    stmt->sem();
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

  virtual void sem() override {} // idk if this is needed
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

  virtual void sem() override {
    l_value->sem();
    expr->sem();
    if(l_value->get_kind() == EntryKind::FUNCTION) {
      yyerror("Cannot assign to a function");
    }
    if(expr->get_kind() == EntryKind::FUNCTION) {
      yyerror("Cannot assign a function");
    }
    std::cout<<"l_value type: "<<l_value->get_type()<<std::endl;
    std::cout<<"expr type: "<<expr->get_type()<<std::endl;
    l_value->type_check(expr->get_type(), expr->get_dimensions());
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

  virtual void sem() override {
    if(expr == nullptr) {
      if(st.get_return_type() != DataType::TYPE_nothing) {
        yyerror("Type mismatch");
      }
    }
    else {
      expr->sem();
      expr->type_check(st.get_return_type());
    }

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

  std::vector<int> getDimensions() const {
    return dimensions;
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

  // TODO: implement array types
  DataType getDataType() const { return datatype; }

  std::vector<int> getDimensions() const {
    return dim->getDimensions();
  }

  bool getMissingFirstDimension() const {
    return dim->missingFirstDimension;
  }
    

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

  std::tuple<DataType, PassingType, std::vector<int>, bool> getParam() const {
    return std::make_tuple(param_type->getDataType(), passing_type, param_type->getDimensions(), param_type->getMissingFirstDimension());
  }

  virtual void sem() override {
    st.insert_param(*id, param_type->getDataType(), passing_type, param_type->getDimensions(), param_type->getMissingFirstDimension());
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
  ~FuncParamList() {
    for (FuncParam *p : param_list) delete p;
  }

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

  virtual void sem() override {
    for (const auto &p : param_list) {
      p->sem();
    }
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

  virtual void sem() override {
    std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> param_types;
    if (paramlist != nullptr) {
      for(const auto &p : paramlist->param_list) {
        param_types.push_back(p->getParam());
      }
    }
    st.insert_function(*id, returntype, param_types);
  }

  void register_param_list() {
    if (paramlist != nullptr) {
      paramlist->sem();
    }
  }

  DataType get_return_type() const {
    return returntype;
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
    st.insert_variable(*id, variable_type->getDataType(), variable_type->getDimensions());
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
    for (const auto &ld : local_definition_list) {
      ld->sem();
    }
  }
};

class FunctionDeclaration: public LocalDefinition {
public:
  FunctionDeclaration(Header *h): header(h) {}

  void printOn(std::ostream &out) const override {
    out << "FunctionDeclaration(" << *header << ")";
  }

  virtual void sem() override {
    header->sem();
  }

private:
  Header *header;
};

class FunctionDefinition: public LocalDefinition {
public:
  FunctionDefinition(Header *h, LocalDefinitionList *d, Block *b): header(h), definition_list(d), block(b) {}
  ~FunctionDefinition() { delete header; delete definition_list; delete block; }

  void printOn(std::ostream &out) const override {
    out << "FunctionDefinition(" << *header << 
      ", " << *definition_list << 
      ", " << *block << ")";
  }

  virtual void sem() override {
      if(st.not_exists_scope()) st.openScope(); 
      header->sem();
      st.openScope(header->get_return_type());
      header->register_param_list();
      definition_list->sem();
      block->sem();
      // std::cout<<"Before end of scope"<<std::endl;
      // st.display();
      st.closeScope();
      // std::cout<<"After end of scope"<<std::endl;
      // st.display();
  }

private:
  Header *header;
  LocalDefinitionList *definition_list;
  Block *block;
};
