#ifndef __SYMBOL_HPP__
#define __SYMBOL_HPP__

#include <vector>
#include <string>
#include <list>
#include <tuple>
#include <functional>

extern void yyerror(const char *msg);

enum PassingType { BY_VALUE, BY_REFERENCE };
enum DataType { TYPE_int, TYPE_char, TYPE_nothing };
enum EntryKind { FUNCTION, VARIABLE, PARAM};

class STEntry {
public:
  int offset;
  std::string name;
  int scope_number;
  int hash_value;

  virtual ~STEntry() {}
};

class STEntryFunction : public STEntry {
public:
  DataType returnType;
  std::vector<std::tuple<DataType, PassingType>> paramTypes;

  STEntryFunction(std::string str, DataType rt, std::vector<std::tuple<DataType, PassingType>> pt) :
    returnType(rt), paramTypes(pt) { name = str; }  
};

class STEntryVariable : public STEntry {
public:
  DataType type;

  STEntryVariable(std::string str, DataType t) : type(t) { name = str; }
};

class STEntryParam : public STEntry {
public:
  DataType type;
  PassingType passingType;

  STEntryParam(std::string str, DataType t, PassingType pt) : type(t), passingType(pt) { name = str; }
};

class HashTable {
public:
  int hashFunction(std::string str) {
    std::hash<std::string> hash_fn;
    return hash_fn(str) % capacity;
  }

  HashTable(int c) : capacity(c) {
    entries = new std::list<STEntry>(capacity);
  }

  void insertItem(STEntry *entry) {
    int index = hashFunction(entry->name);
    entry->hash_value = index;
    entries[index].push_front(*entry);
  }

  // TODO: implement this
  void deleteItem(STEntry *entry) {}

  // STEntry **table; //pointers to most recent hashed entries
  std::list<STEntry> *entries;
  int capacity;
};

class Scope {
public:
  Scope(int o = -1, int n = 1) : offset(o), scope_number(n), size(0) {}

  int getOffset() const { return offset; }

  int getSize() const { return size; }

  int getScopeNumber() const { return scope_number; }

private:
  int offset;
  int scope_number;
  int size;
};

class SymbolTable {
public:
  SymbolTable(int c = 1001) {
    hash_table = new HashTable(c);
  }

  /*
  * Will return nullptr if there is no entry
  * with the given name in the symbol table
  */
  STEntry *lookup(std::string str) {
    int index = hash_table->hashFunction(str);
    for(auto entry = hash_table->entries[index].begin(); entry != hash_table->entries[index].end(); entry++) {
      if (entry->name == str) return &(*entry);
    }
    return nullptr;
  }

  void insert_function(std::string str, DataType type, std::vector<std::tuple<DataType, PassingType>> param_types) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) { yyerror("Duplicate function declaration"); }
    STEntryFunction *entry = new STEntryFunction(str, type, param_types);
    hash_table->insertItem(entry);
  }

  void insert_variable(std::string str, DataType type) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) { yyerror("Duplicate variable declaration"); }
    STEntryVariable *entry = new STEntryVariable(str, type);
    hash_table->insertItem(entry);
  }

  void insert_param(std::string str, DataType type, PassingType passing_type) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) { yyerror("Duplicate parameter declaration"); }
    STEntryParam *entry = new STEntryParam(str, type, passing_type);
    hash_table->insertItem(entry);
  }

  void openScope() {
    int ofs = 0, number = 1;
    if(!scopes.empty()) {
      ofs = scopes.back().getOffset();
      number = scopes.back().getScopeNumber() + 1;
    }
    scopes.push_back(Scope(ofs, number));
  }

  // TODO: implement this
  void closeScope() {
    
  }

private:
  std::vector<Scope> scopes;
  std::vector<STEntry> table;
  HashTable *hash_table;
};

extern SymbolTable st;

#endif
