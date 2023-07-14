#pragma once

#include <iostream>
#include <map>
#include <vector>
#include "symbol.hpp"

class AST {
public:
  virtual ~AST() {}
  virtual void printOn(std::ostream &out) const = 0;
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
    Id(const std::string &s): var(s) {}
    virtual void printOn(std::ostream &out) const override {
    out << "Id(" << var << ")";
  }
  virtual int eval() const override {
    return 0;//var;
  }

private:
    std::string var;
};


class StringLiteral: public Expr {
public:
    StringLiteral(const std::string &s): stringval(s) {}
    virtual void printOn(std::ostream &out) const override {
    out << "StringLiteral(" << stringval << ")";
  }
  virtual int eval() const override {
    return 0; //stringval;
  }

private:
    std::string stringval;
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

class ArrayDimension: public AST {
public:
  bool missingFirstDimension;

  ArrayDimension(): missingFirstDimension(false), dimensions() {}
  // ~ArrayDimension() { for (IntConst *d : dimensions) delete d; }

  int getDimensionCount() const {
    return dimensions.size() + int(missingFirstDimension);
  }

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

class FuncParamType: public AST {
public:
  FuncParamType(DataType t, ArrayDimension *d): datatype(t), dim(d) {}
  ~FuncParamType() { delete dim; }

  void printOn(std::ostream &out) const override {
    out << "FuncParamType(" << datatype << ", " << *dim << ")";
  }

private:
  DataType datatype;
  ArrayDimension *dim;
};


class FuncParam: public AST {
public:
  FuncParam(std::string *il, FuncParamType *t, PassingType pt): id(il), param_type(t), passing_type(pt) {}
  ~FuncParam() { delete id; delete param_type; }

  void printOn(std::ostream &out) const override {
    out << "FuncParam(" << (bool(passing_type) ? "reference, ":"value, ") << *id << ", " << *param_type << ")";
  }

private:
  std::string *id;
  FuncParamType *param_type;
  PassingType passing_type;
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

class FuncParamList: public AST {
public:
  std::vector<FuncParam*> param_list;

  void add_param(FuncParam *p) {
    param_list.push_back(p);
  }

  FuncParamList(IdList* il, FuncParamType* fpt, PassingType pt = PassingType::BY_VALUE) {
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
