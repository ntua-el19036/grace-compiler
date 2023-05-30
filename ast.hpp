#pragma once

#include <iostream>
#include <map>
#include <vector>

class AST {
public:
  virtual ~AST() {}
  virtual void printOn(std::ostream &out) const = 0;
};

inline std::ostream& operator<< (std::ostream &out, const AST &t) {
  t.printOn(out);
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
    return var;
  }
    
private:
    string var;
};


class StringLiteral: public Expr {
public:
    StringLiteral(const std::string &s): stringval(s) {}
    virtual void printOn(std::ostream &out) const override {
    out << "StringLiteral(" << stringval << ")";
  }
  virtual int eval() const override {
    return stringval;
  }
    
private:
    string stringval;
};    

class Negative: public Expr {
public:
    Negative(Expr *e): expr(e) {}
    virtual void printOn(std::ostream &out) const override {
    out << "Negative(" << *expr << ")";
  }
  virtual int eval() const override {
    return -expr->eval();
  }
        
private:
    Expr *expr;
    
};
