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

  STEntry() {}
  virtual ~STEntry() {}
};

class STEntryFunction : public STEntry {
public:  
  DataType returnType;
  std::vector<std::tuple<DataType, PassingType>> paramTypes;
  //maybe we don't need the passing type for semantic checking

  void printEntry() {
    std::cout << "name: " << name << std::endl;
    std::cout << "return type: " << returnType << std::endl;
    std::cout << "param types: " << std::endl;
    for (auto x : paramTypes) {
      std::cout << std::get<0>(x) << " " << std::get<1>(x) << std::endl;
    }
  }
  STEntryFunction(DataType rt, std::vector<std::tuple<DataType, PassingType>> pt) :
    returnType(rt), paramTypes(pt) {}  
};

class STEntryVariable : public STEntry {
public:
  DataType type;

  STEntryVariable(DataType t) : type(t) {}

  DataType getType() const { return type; }

  void setName(std::string str) { name = str; }
};

class STEntryParam : public STEntry {
public:
  DataType type;
  PassingType passingType;

  STEntryParam(DataType t, PassingType pt) : type(t), passingType(pt) {}
};

class HashTable {
public:
  int hashFunction(std::string str) {
    std::hash<std::string> hash_fn;
    return hash_fn(str) % capacity;
  }

  HashTable(int c) : capacity(c) {
    entries = new std::list<STEntry>[capacity];
  }

  void insertItem(STEntry *entry) {
    int index = hashFunction(entry->name);
    entry->hash_value = index;
    std::cout << "inserting " << entry->name << " at " << index << std::endl;
    entries[index].push_front(*entry);
  }

  // TODO: implement this
  void deleteItem(STEntry *entry) {}

  void displayHash() {
    for (int i = 0; i < capacity; i++) {
      if(entries[i].empty()) continue;
       std::cout << i << " --> " ;
      for (auto x : entries[i]) {
        std::cout << x.name << " " << x.hash_value << " ";
      }
      std::cout << std::endl;
    }
  }

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

  void incrementSize(int s) { size += s; }

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

  // ~SymbolTable() {
  //   delete hash_table;
  // }

  void display() {
    hash_table->displayHash();
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

  void insert_function(std::string str, DataType rettype, std::vector<std::tuple<DataType, PassingType>> param_types) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) {
      yyerror("Duplicate declaration"); 
    }
    STEntryFunction *entry = new STEntryFunction(rettype, param_types);
    entry->name = str;
    entry->scope_number = num;
    std::cout << "scope number: " << entry->scope_number << std::endl;
    hash_table->insertItem(entry);
    entry->printEntry();
    scopes.back().incrementSize(1);
  }

  void insert_variable(std::string str, DataType type) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) { 
      yyerror("Duplicate declaration"); 
    }
    else {
      std::cout << "no duplicate" << std::endl;
    }
    STEntryVariable *entry = new STEntryVariable(type);
    entry->name = str;
    entry->scope_number = num;
    std::cout << "scope number: " << entry->scope_number << std::endl;
    hash_table->insertItem(entry);
    scopes.back().incrementSize(1);
  }

  void insert_param(std::string str, DataType type, PassingType passing_type) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) { 
      yyerror("Duplicate declaration"); 
    }
    STEntryParam *entry = new STEntryParam(type, passing_type);
    entry->name = str;
    entry->scope_number = num;
    std::cout << "scope number: " << entry->scope_number << std::endl;
    hash_table->insertItem(entry);
    scopes.back().incrementSize(1);
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

  int not_exists_scope() {
    return scopes.empty();
  }

private:
  std::vector<Scope> scopes;
  HashTable *hash_table;
};

extern SymbolTable st;

#endif
