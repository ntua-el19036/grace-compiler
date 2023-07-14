#ifndef __SYMBOL_HPP__
#define __SYMBOL_HPP__

#include <map>
#include <vector>
#include <tuple>

extern void yyerror(const char *msg);

enum PassingType { BY_VALUE, BY_REFERENCE };
enum DataType { TYPE_int, TYPE_char, TYPE_nothing };
enum EntryKind { FUNCTION, VARIABLE, PARAM};

struct STEntry {
    EntryKind kind;
    DataType type;
    int offset;
    int param_count;
    std::vector<DataType> paramTypes;
    bool byRef;

    STEntry() {}
    STEntry(EntryKind k, DataType t, int o) : kind(k), type(t), offset(o) {} //for variables
    STEntry(EntryKind k, DataType t, int o, bool r) : kind(k), type(t), offset(o), byRef(r) {} //for params
    STEntry(EntryKind k, DataType t, int o, std::vector<DataType> pt) //for function decls
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

  void insertFunction(std::string str, DataType t, std::vector<std::tuple<DataType, std::string>> pt) {
    if (locals.find(str) != locals.end())
      yyerror("Duplicate declaration");

    std::vector<DataType> paramTypes;
    for (const auto& tuple : pt) {
      paramTypes.push_back(std::get<0>(tuple));  // Add the DataType field to paramTypes
    }
    locals[str] = STEntry(FUNCTION, t, offset++, paramTypes);
  }

  void insertParam(std::string str, DataType t, bool ref) {
    if (locals.find(str) != locals.end())
      yyerror("Duplicate declaration");
    locals[str] = STEntry(PARAM, t, offset++, ref);
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
  void insertToStFunction(std::string str, DataType t, std::vector<std::tuple<DataType, std::string>> pt) {
    scopes.back().insertFunction(str, t, pt);

  }
  void insertToStFunParam(std::string str, DataType t, std::vector<std::tuple<std::string, DataType>> pt) {
    for(const auto& tuple :pt) {
      scopes.back().insertParam(std::get<0>(tuple), std::get<1>(tuple), false);
    }   
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
