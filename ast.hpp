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
    std::string var;
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

<<<<<<< HEAD
class BinOp: public Expr {
=======
class Binop: public Expr {
>>>>>>> c1b1712d88b6924838c59398ba5c86b50b2aa51a
public:
    BinOp(Expr *l, const std::string &o, Expr *r): left(l), op(o), right(r) {}
    ~BinOp() { delete left; delete right; }
    virtual void printOn(std::ostream &out) const override {
        out << *op << "(" << *left << ", " << *right << ")";
    }
    virtual int eval() const override {
        switch (*op) {
            case "+": return left->eval() + right->eval();
            case "-": return left->eval() - right->eval();
            case "*": return left->eval() * right->eval();
            case "div": return left->eval() / right->eval();
            case "mod": return left->eval() % right->eval();
            case "=" : return left->eval() == right->eval();
            case "#" : return left->eval() != right->eval();
            case "<" : return left->eval() < right->eval();
            case ">" : return left->eval() > right->eval();
            case "<=" : return left->eval() <= right->eval();
            case ">=" : return left->eval() >= right->eval();
            
            case "and" : return { !(left->eval()) ? false : right->eval() };
            case "or" : return { (left->eval()) ? true : right->eval() };
        }
        return 0;  // this will never be reached
    }
private:
    Expr *left;
    std::string *op;
    Expr *right;
};

class Not: public Expr {
public:
<<<<<<< HEAD
    Not(Expr *c): cond(c) {}
=======
    Not(Expr *c): cond(e) {}
>>>>>>> c1b1712d88b6924838c59398ba5c86b50b2aa51a
    ~Not() { delete cond; }
    virtual void printOn(std::ostream &out) const override {
        out << "Not" << "(" << *cond << ")";
    }
    virtual int eval() const override {
        return !(cond->eval());
    }
    
private:
        Expr *cond;
<<<<<<< HEAD
};
=======
}
>>>>>>> c1b1712d88b6924838c59398ba5c86b50b2aa51a

class Block: public Stmt {
public:
    Block(): stmt_list() {}
<<<<<<< HEAD
    ~Block() { for(Stmt *s : stmt_list) delete s; }
=======
    ~Block(): { for(Stmt *s : stmt_list) delete s; }
>>>>>>> c1b1712d88b6924838c59398ba5c86b50b2aa51a
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
<<<<<<< HEAD
};

class If: public Stmt {
public:
    If(Expr *c, Stmt *s1, Stmt *s2 = nullptr): cond(c), stmt1(s1), stmt2(s2) {}
=======
}

class If: public Stmt {
public:
    If(Expr *c, Stmt *s1, Stmt *2 = nullptr): cond(c), stmt1(s1), stmt2(s2) {}
>>>>>>> c1b1712d88b6924838c59398ba5c86b50b2aa51a
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
<<<<<<< HEAD
};
=======
}
>>>>>>> c1b1712d88b6924838c59398ba5c86b50b2aa51a

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
<<<<<<< HEAD
};
=======
}
>>>>>>> c1b1712d88b6924838c59398ba5c86b50b2aa51a
