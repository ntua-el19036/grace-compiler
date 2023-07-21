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
  std::vector<DataType> paramTypes;
  std::vector<PassingType> paramPassingTypes;

  STEntryFunction(DataType rt, std::vector<DataType> pt, std::vector<PassingType> ppt) :
   returnType(rt), paramTypes(pt), paramPassingTypes(ppt) {}

  
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

  void insertItem(std::string str, STEntry *entry) {
    int index = hashFunction(str);
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
  Scope(int o = -1) : offset(o), size(0) {}

  int getOffset() const { return offset; }

  int getSize() const { return size; }



private:
  int offset;
  int size;
};

class SymbolTable {

  STEntry *lookup(std::string str) {
    int index = hash_table->hashFunction(str);
    for (auto entry : hash_table->entries[index]) {
      if (entry.name == str) return &entry;
    }
    yyerror("Unknown identifier");
  }

  void insert() {
    
  }

  void openScope() {
    int offset = scopes.empty() ? 0 : scopes.back().getOffset();
    scopes.push_back(Scope(offset));
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
