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
  std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> paramTypes;

  void printEntry() {
    std::cout << "name: " << name << std::endl;
    std::cout << "return type: " << returnType << std::endl;
    std::cout << "param types: " << std::endl;
    for (auto x : paramTypes) {
      std::cout << std::get<0>(x) << " " << std::get<1>(x) << " ";
      for(auto d : std::get<2>(x)) {
        std::cout << d << ", ";
      }
      std::cout << " " << std::get<3>(x) << std::endl;
    }
  }
  STEntryFunction(DataType rt, std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> pt) :
    returnType(rt), paramTypes(pt) {}  
};

class STEntryVariable : public STEntry {
public:
  DataType type;
  std::vector<int> dimensions;

  STEntryVariable(DataType t, std::vector<int> d) : type(t), dimensions(d) {
    std::cout << "creating variable entry of type " << type << std::endl;
    for (auto d : dimensions) {
      std::cout << d << ", ";
    }
    std::cout << std::endl;
  }

  DataType getType() const { return type; }

  // void setName(std::string str) { name = str; }
};

class STEntryParam : public STEntry {
public:
  DataType type;
  PassingType passingType;
  std::vector<int> dimensions;
  bool missingFirstDimension;

  STEntryParam(DataType t, PassingType pt, std::vector<int> d, bool m) : type(t),
    passingType(pt), dimensions(d), missingFirstDimension(m) {}
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

  void deleteScope(int scope_number) {
    for (int i = 0; i < capacity; i++) {
      if(entries[i].empty()) continue;
      for(std::list<STEntry>::iterator it = entries[i].begin(); it != entries[i].end(); it++) {
        std::cout << "deleting " << it->name << " at " << i << std::endl;
        if (it->scope_number == scope_number) {
          it = entries[i].erase(it);
        }
      }
    }
  }

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

  void insert_function(std::string str, DataType rettype, std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> param_types) {
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

  void insert_variable(std::string str, DataType type, std::vector<int> dimensions) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) { 
      yyerror("Duplicate declaration"); 
    }
    else {
      std::cout << "no duplicate" << std::endl;
    }
    STEntryVariable *entry = new STEntryVariable(type, dimensions);
    entry->name = str;
    entry->scope_number = num;
    std::cout << "scope number: " << entry->scope_number << std::endl;
    hash_table->insertItem(entry);
    scopes.back().incrementSize(1);
  }

  void insert_param(std::string str, DataType type, PassingType passing_type, std::vector<int> dimensions, bool missing_first_dimension) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) { 
      yyerror("Duplicate declaration"); 
    }
    STEntryParam *entry = new STEntryParam(type, passing_type, dimensions, missing_first_dimension);
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
    int current = scopes.back().getScopeNumber();
    std::cout << "closing scope " << current << std::endl;
    hash_table->deleteScope(current);
    scopes.pop_back();
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
