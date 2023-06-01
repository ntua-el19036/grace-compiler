#ifndef __SYMBOL_HPP__
#define __SYMBOL_HPP__

#include <map>

extern void yyerror(const char *msg);

enum DataType { TYPE_int, TYPE_char, TYPE_nothing };

struct STEntry {
    
    DataType type;
    int offset;
    STEntry() {}
    STEntry(DataType t, int o) : type(t), offset(o) {}
};

class Scope {
 public:
  Scope(int o = -1) : offset(o) {}
  STEntry *lookup(std::string str) {
    if (locals.find(str) == locals.end()) return nullptr;
    return &(locals[str]);
  }
  void insert(std::string str, DataType t) {
    if (locals.find(str) != locals.end())
      yyerror("Duplicate variable declaration");
    locals[str] = STEntry(t, offset++);
  }
  int get_offset() {
    return offset;
  }
 private:
  std::map<std::string, STEntry> locals;
  int offset;
};

class SymbolTable {
 public:
  STEntry *lookup(std::string str) {
    for (auto s = scopes.rbegin(); s != scopes.rend(); ++s) {
      STEntry *e = s->lookup(str);
      if (e != nullptr) return e;
    }
    yyerror("Variable not found");
    return nullptr;
  }
  void insert(std::string str, DataType t) {
    scopes.back().insert(str, t);
  }
  void push_scope() {
    int o = scopes.empty() ? 0 : scopes.back().get_offset();
    scopes.push_back(Scope(o));
  }
  void pop_scope() {
    scopes.pop_back();
  }
 private:
  std::vector<Scope> scopes;
};

extern SymbolTable st;

#endif