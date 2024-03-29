#ifndef __SYMBOL_HPP__
#define __SYMBOL_HPP__

#include <vector>
#include <string>
#include <list>
#include <tuple>
#include <functional>

extern void yyerror(const char *msg);

enum PassingType { BY_VALUE = 1, BY_REFERENCE };
enum DataType { TYPE_int = 1, TYPE_char, TYPE_nothing };
enum EntryKind { FUNCTION = 1, VARIABLE, PARAM};

extern void yyerror2(const char *msg, int line_number);

class STEntry {
public:
  int offset;
  std::string name;
  int scope_number;
  int hash_value;
  EntryKind kind;
  bool undefined = false;
  int line_number = 0;

  DataType type;
  std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> paramTypes;
  std::vector<int> dimensions;
  PassingType passingType;
  bool missingFirstDimension;

  std::string TypeName[3] = { "int", "char", "nothing" };
  std::string PassingTypeName[2] = { "by value", "by reference" };

  STEntry() {}
  virtual void printEntry() const {}
  virtual ~STEntry() {}
};

class STEntryFunction : public STEntry {
public:  
  virtual void printEntry() const override {
    std::cout << "name: " << name << std::endl;
    std::cout << "return type: " << TypeName[type] << std::endl;
    if(paramTypes.empty()) { std::cout << "no parameters" << std::endl; return; }
    std::cout << "param types: ";
    for (auto x : paramTypes) {
      std::cout << std::endl;
      std::cout << TypeName[std::get<0>(x)] << " " << PassingTypeName[std::get<1>(x)] << " ";
      if(std::get<3>(x)) std::cout << "[]";
      if(std::get<2>(x).empty()) continue;
      for(auto d : std::get<2>(x)) {
        std::cout << "[" << d << "]";
      }
    }
    std::cout << std::endl;
  }

  STEntryFunction(DataType rt, std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> pt) { 
    type = rt;
    paramTypes = pt;
    kind = EntryKind::FUNCTION;
  }  
};

class STEntryVariable : public STEntry {
public:

  STEntryVariable(DataType t, std::vector<int> d) { 
    type = t;
    dimensions = d;
    kind = EntryKind::VARIABLE;
  }

  virtual void printEntry() const override {
    std::cout << "name: " << name << std::endl;
    std::cout << "type: " << TypeName[type] << " ";
    if(dimensions.empty()) { std::cout << std::endl; return; }
    for (auto d : dimensions) {
      std::cout << "[" << d << "]";
    }
    std::cout << std::endl;
  }

  DataType getType() const { return type; } // not used

  // void setName(std::string str) { name = str; }
};

class STEntryParam : public STEntry {
public:

  STEntryParam(DataType t, PassingType pt, std::vector<int> d, bool m) {
    type = t;
    passingType = pt;
    dimensions = d;
    missingFirstDimension = m;
    kind = EntryKind::PARAM;
  }

  virtual void printEntry() const override {
    std::cout << "name: " << name << std::endl;
    std::cout << "passing type: " << PassingTypeName[passingType] << std::endl;
    std::cout << "type: " << TypeName[type];
    if(missingFirstDimension) std::cout << "[]";
    if(dimensions.empty()) { std::cout<< std::endl; return; }
    for (auto d : dimensions) {
      std::cout << "[" << d << "]";
    }
    std::cout << std::endl;
  }
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

  ~HashTable() {
    // std::cout << "deleting hash table" << std::endl;
    delete[] entries;
    
  }

  void insertItem(STEntry *entry) {
    int index = hashFunction(entry->name);
    entry->hash_value = index;
    // std::cout << "inserting " << entry->name << " at index " << index << std::endl;
    entries[index].push_front(*entry);
  }

  void deleteScope(int scope_number) {
    for (int i = 0; i < capacity; i++) {
      if(entries[i].empty()) continue;
      for(std::list<STEntry>::iterator it = entries[i].begin(); it != entries[i].end(); it++) {
        // std::cout << "deleting " << it->name << " at " << i << std::endl;
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
        std::cout << x.name << " ";
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
  Scope(int o = -1, int n = 1, DataType rt = DataType::TYPE_nothing) : offset(o), scope_number(n), size(0), return_type(rt) {}

  int getOffset() const { return offset; }

  int getSize() const { return size; }

  int getScopeNumber() const { return scope_number; }

  void incrementSize(int s) { size += s; }

  DataType get_return_type() {
    return return_type;
  }

  bool return_exists = false;

  void set_return_exists() {
    return_exists = true;
  }

  bool get_return_exists() {
    return return_exists;
  }

private:
  int offset;
  int scope_number;
  int size;
  DataType return_type;
};

class SymbolTable {
public:
  SymbolTable(int c = 1001) {
    hash_table = new HashTable(c);
  }

  ~SymbolTable() {
    delete hash_table;
  }
  void init_library_functions() {
    std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> temp_param_vector;
    insert_function(std::string("readInteger"), DataType::TYPE_int, temp_param_vector);
    insert_function(std::string("readChar"), DataType::TYPE_char, temp_param_vector);
    temp_param_vector.push_back(std::make_tuple(DataType::TYPE_char, PassingType::BY_VALUE, std::vector<int>(), false));
    insert_function(std::string("writeChar"), DataType::TYPE_nothing, temp_param_vector);
    insert_function(std::string("ascii"), DataType::TYPE_int, temp_param_vector);
    temp_param_vector.clear();
    temp_param_vector.push_back(std::make_tuple(DataType::TYPE_int, PassingType::BY_VALUE, std::vector<int>(), false));
    insert_function(std::string("writeInteger"), DataType::TYPE_nothing, temp_param_vector);
    insert_function(std::string("chr"), DataType::TYPE_char, temp_param_vector);
    temp_param_vector.push_back(std::make_tuple(DataType::TYPE_char, PassingType::BY_REFERENCE, std::vector<int>(), true));
    insert_function(std::string("readString"), DataType::TYPE_nothing, temp_param_vector);
    temp_param_vector.clear();
    temp_param_vector.push_back(std::make_tuple(DataType::TYPE_char, PassingType::BY_REFERENCE, std::vector<int>(), true));
    insert_function(std::string("writeString"), DataType::TYPE_nothing, temp_param_vector);
    insert_function(std::string("strlen"), DataType::TYPE_int, temp_param_vector);
    temp_param_vector.push_back(std::make_tuple(DataType::TYPE_char, PassingType::BY_REFERENCE, std::vector<int>(), true));
    insert_function(std::string("strcmp"), DataType::TYPE_int, temp_param_vector);
    insert_function(std::string("strcpy"), DataType::TYPE_nothing, temp_param_vector);
    insert_function(std::string("strcat"), DataType::TYPE_nothing, temp_param_vector);
  }

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
      if (entry->name == str) {
        // std::cout << "found " << str << " of kind " << entry->kind << std::endl;
        return &(*entry);
      }
    }
    return nullptr;
  }

  bool was_declared(std::string function_name) {
    int index = hash_table->hashFunction(function_name);
    for(auto entry = hash_table->entries[index].begin(); entry != hash_table->entries[index].end(); entry++) {
      if (entry->name == function_name && entry->kind == EntryKind::FUNCTION) {
        return true;
      }
    }
    return false;
  }

  void insert_function(std::string str, DataType rettype, std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> temp_param_vector, int lineno = 0) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) {
      //this might be impossible to reach
      yyerror2("Function name was already declared in this scope", lineno);
    }
    STEntryFunction *entry = new STEntryFunction(rettype, temp_param_vector);
    entry->name = str;
    entry->scope_number = num;
    entry->line_number = lineno;
    // std::cout << "scope number: " << entry->scope_number << std::endl;
    hash_table->insertItem(entry);
    // entry->printEntry();
    scopes.back().incrementSize(1);
  }

  void insert_function_declaration(std::string str, DataType rettype, std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> temp_param_vector, int lineno = 0){
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) {
      yyerror2("Duplicate declaration in the same scope", lineno); 
    }
    STEntryFunction *entry = new STEntryFunction(rettype, temp_param_vector);
    entry->name = str;
    entry->undefined = true;
    entry->scope_number = num;
    entry->line_number = lineno;
    // std::cout << "scope number: " << entry->scope_number << std::endl;
    hash_table->insertItem(entry);
    // entry->printEntry();
  }

  void insert_function_definition(std::string str, DataType rettype, std::vector<std::tuple<DataType, PassingType, std::vector<int>, bool>> temp_param_vector, int lineno = 0){
    //check if declaration is matching
    STEntry *previous_entry = lookup(str);
    int current_scope = scopes.back().getScopeNumber();
    if (previous_entry == nullptr) {
      yyerror2("Function not declared", lineno); 
    }
    if(previous_entry->kind != EntryKind::FUNCTION) {
      yyerror2("Duplicate function declaration", lineno);
    }
    if(previous_entry->undefined == false) {
      yyerror2("Function already defined", lineno);
    }
    if(previous_entry->undefined == true && previous_entry->scope_number != current_scope) {
      yyerror2("Function declaration and definition are not in the same scope", lineno);
    }
    if(previous_entry->type != rettype) {
      yyerror2("Function return type does not match declaration", lineno);
    }
    if(previous_entry->paramTypes.size() != temp_param_vector.size()) {
      yyerror2("Function parameter number does not match declaration", lineno);
    }
    unsigned int e = previous_entry->paramTypes.size();
    for(unsigned int i = 0; i < e; i++) {
      if(std::get<0>(previous_entry->paramTypes[i]) != std::get<0>(temp_param_vector[i])) {
        yyerror2("Function parameter type does not match declaration", lineno);
      }
      if(std::get<1>(previous_entry->paramTypes[i]) != std::get<1>(temp_param_vector[i])) {
        yyerror2("Function parameter passing type does not match declaration", lineno);
      }
      if(std::get<2>(previous_entry->paramTypes[i]) != std::get<2>(temp_param_vector[i])) {
        yyerror2("Function parameter array dimensions do not match declaration", lineno);
      }
      if(std::get<3>(previous_entry->paramTypes[i]) != std::get<3>(temp_param_vector[i])) {
        yyerror2("Function parameter array missing first dimension does not match declaration", lineno);
      }
    }
    //set undefined to false
    previous_entry->undefined = false;
  }

  void insert_variable(std::string str, DataType type, std::vector<int> dimensions, int lineno) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) { 
      yyerror2("Duplicate declaration", lineno); 
    }
    for(auto d : dimensions) {
      if(d <= 0) {
        yyerror2("Invalid array size", lineno);
      }
    }
    STEntryVariable *entry = new STEntryVariable(type, dimensions);
    entry->name = str;
    entry->scope_number = num;
    // std::cout << "scope number: " << entry->scope_number << std::endl;
    hash_table->insertItem(entry);
    // entry->printEntry();
    scopes.back().incrementSize(1);
  }

  void insert_param(std::string str, DataType type, PassingType passing_type, std::vector<int> dimensions, bool missing_first_dimension, int lineno = 0) {
    STEntry *previous_entry = lookup(str);
    int num = scopes.back().getScopeNumber();
    if (previous_entry != nullptr && previous_entry->scope_number == num) { 
      yyerror2("Duplicate parameter declaration", lineno); 
    }
    STEntryParam *entry = new STEntryParam(type, passing_type, dimensions, missing_first_dimension);
    entry->name = str;
    entry->scope_number = num;
    // std::cout << "scope number: " << entry->scope_number << std::endl;
    hash_table->insertItem(entry);
    // entry->printEntry();
    scopes.back().incrementSize(1);
  }

  void openScope(DataType return_type = DataType::TYPE_nothing) {
    int ofs = 0, number = 1;
    if(!scopes.empty()) {
      ofs = scopes.back().getOffset();
      number = scopes.back().getScopeNumber() + 1;
    }
    scopes.push_back(Scope(ofs, number, return_type));
    // std::cout << "Opening scope: " << number << std::endl;
  }

  void set_return_exists() {
    scopes.back().set_return_exists();
  }

  void check_return_exists(int lineno = 0){
    if(scopes.back().get_return_exists() == false && scopes.back().get_return_type() != DataType::TYPE_nothing) {
      yyerror2("Function does not return a value", lineno);
    }
  }

  // TODO: implement this
  void closeScope() {
    int current = scopes.back().getScopeNumber();
    // std::cout << "closing scope " << current << std::endl;
    hash_table->deleteScope(current);
    scopes.pop_back();
  }

  void check_undefined_functions(){
    for (int i = 0; i < hash_table->capacity; i++) {
      if(hash_table->entries[i].empty()) continue;
      for(std::list<STEntry>::iterator it = hash_table->entries[i].begin(); it != hash_table->entries[i].end(); it++) {
        if(it->kind == EntryKind::FUNCTION && (it->scope_number == scopes.back().getScopeNumber()) && it->undefined == true) {
          yyerror2("A function is undefined", it->line_number);
        }
      }
    }
  }

  int not_exists_scope() {
    return scopes.empty();
  }

  DataType get_return_type() {
    return scopes.back().get_return_type();
  }

private:
  std::vector<Scope> scopes;
  HashTable *hash_table;
};

extern SymbolTable st;

#endif
