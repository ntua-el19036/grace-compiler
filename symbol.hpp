#ifndef __SYMBOL_HPP__
#define __SYMBOL_HPP__

#include <map>

extern void yyerror(const char *msg);

enum DataType { TYPE_int, TYPE_char, TYPE_nothing };
enum EntryKind { FUNCTION, VARIABLE };

struct STEntry {
    DataType type;
    EntryKind kind;
    int param_count;
    std::vector<DataType> paramTypes;
    int offset;

    STEntry() {}
    STEntry(EntryKind k, DataType t, int o) : kind(k), type(t), offset(o) {}
    STEntry(EntryKind k, DataType t, int o, std::vector<DataType> pt)
      : kind(k), type(t), offset(o), param_count(pt.size()), paramTypes(pt) {}
};

class Scope {
 public:
  Scope(int o = -1) : offset(o) {}

  STEntry *lookup(std::string str) {
    if (locals.find(str) == locals.end()) return nullptr;
    return &(locals[str]);
  }

  void insertVariable(std::string str, DataType t) {
    if (locals.find(str) != locals.end())
      yyerror("Duplicate declaration");
    locals[str] = STEntry(VARIABLE, t, offset++);
  }

  void insertFunction(std::string str, DataType t, std::vector<DataType> pt) {
    if (locals.find(str) != locals.end())
      yyerror("Duplicate declaration");
    locals[str] = STEntry(FUNCTION, t, offset++, pt);
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
    yyerror("Identifier not found");
    return nullptr;
  }
  void insertToStVariable(std::string str, DataType t) {
    scopes.back().insertVariable(str, t);
  }
  void insertToStFunction(std::string str, DataType t, std::vector<DataType> pt) {
    scopes.back().insertFunction(str, t, pt);
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
