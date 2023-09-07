%code requires{
    #include <string>
    #include "ast.hpp"
    }

%{
#include <cstdio>
#include "lexer.hpp"
#include "ast.hpp"

extern int mylineno;
SymbolTable st;
%}


%token T_int "int"
%token T_char "char"
%token T_var "var"
%token T_while "while"
%token T_do "do"
%token T_nothing "nothing"
%token T_fun "fun"
%token T_ref "ref"
%token T_if "if"
%token T_return "return"
%token T_assign "<-"
%token<var> T_id
%token<num> T_int_const
%token<charval> T_char_const
%token<stringval> T_string_literal

%precedence T_then
%precedence T_else

%left<op> T_or
%left<op> T_and
%nonassoc<op> T_not
%nonassoc<op> '=' '#' '<' '>' T_lessorequal T_greaterorequal
%left<op> '+' '-'
%left<op> '*' T_div T_mod
%nonassoc USIGN

%union {
  AST *ast;
  Block *block;
  Stmt *stmt;
  Expr *expr;
  int num;
  std::string *var;
  char op;
  char charval;
  std::string *stringval;
  DataType data_type;
  ArrayDimension *dimension;
  VariableType *t_variable_type;
  IdList* t_id_list;
  FuncParamList* t_func_param_list;
  Header *t_header;
  LocalDefinitionList* t_local_definition_list;
  FunctionDeclaration* t_function_declaration;
  FunctionDefinition* t_function_definition;
  ExpressionList *t_expression_list;
}

%type<ast> program
%type<stmt> stmt func_call_stmt
%type<expr> expr cond l_value func_call
%type<t_expression_list> expr_list
%type<block> block stmt_list
%type<data_type> ret_type data_type
%type<t_variable_type> func_param_type

%type<dimension> array_dimension
%type<t_header> header
%type<t_func_param_list> func_param_def func_param_def_list
%type<t_id_list> id_list
%type<t_local_definition_list> local_def var_def local_def_list
%type<t_function_declaration> func_decl
%type<t_function_definition> func_def

%%

program:
  func_def { 
    //std::cout << "AST: " << *$1 << std::endl;
    //delete $1;
    $1->sem();
    }
;

func_def:
  header local_def_list block { $$ = new FunctionDefinition($1,$2,$3);}
;

local_def_list:
  /* nothing */ { $$ = new LocalDefinitionList(); }
| local_def_list local_def { $1->join($2); $$ = $1; }
;

header:
  "fun" T_id '(' func_param_def_list ')' ':' ret_type { $$ = new Header($2, $7, $4); }
| "fun" T_id '(' ')' ':' ret_type { $$ = new Header($2, $6); }
;

func_param_def_list:
  func_param_def  { $$ = $1; }
| func_param_def ';' func_param_def_list { $1->join($3); $$ = $1; }
;

func_param_def:
  "ref" id_list ':' func_param_type { $$ = new FuncParamList($2, $4, PassingType::BY_REFERENCE); }
| id_list ':' func_param_type { $$ = new FuncParamList($1, $3); }
;

id_list:
  T_id { $$ = new IdList($1); }
| T_id ',' id_list { $3->append_id($1); $$ = $3; }
;

data_type:
  "int" { $$ = TYPE_int; }
| "char" { $$ = TYPE_char; }
;

array_dimension:
  /* nothing */ { $$ = new ArrayDimension(); }
| '[' T_int_const ']' array_dimension { $4->add_dimension($2); $$ = $4; }
;

ret_type:
  data_type { $$ = $1; }
| "nothing" { $$ = TYPE_nothing; }
;

func_param_type:
  data_type array_dimension { $$ = new VariableType($1, $2); }
| data_type '[' ']' array_dimension { $4->missingFirstDimension = true; $$ = new VariableType($1, $4); }
;

local_def:
  func_def { $$ = new LocalDefinitionList(); $$->add_local_definition($1);}
| func_decl { $$ = new LocalDefinitionList(); $$->add_local_definition($1);}
| var_def { $$ = $1; }
;

func_decl:
  header ';' { $$ = new FunctionDeclaration($1);}
;

var_def:
  "var" id_list ':' data_type array_dimension ';' { $$ = new LocalDefinitionList(); $$->add_variable_definition_list($2, new VariableType($4,$5));}
;

stmt:
  ';' { $$ = new EmptyStmt(); }
| l_value "<-" expr ';' { $$ = new Assignment($1, $3); }
| block { $$ = $1; }
| func_call_stmt ';' { $$ = $1; }
| "if" cond T_then stmt { $$ = new If($2, $4); }
| "if" cond T_then stmt T_else stmt { $$ = new If($2, $4, $6); }
| "while" cond "do" stmt { $$ = new While($2, $4); }
| "return" ';' { $$ = new Return(); }
| "return" expr ';' { $$ = new Return($2); }
;

stmt_list:
  /* nothing */ { $$ = new Block(); }
| stmt_list stmt { $1->append_stmt($2); $$ = $1; }
;

block:
  '{' stmt_list '}' { $$ = $2; }
;

expr_list:
  expr { $$ = new ExpressionList($1); }
| expr ',' expr_list { $3->add_expression($1); $$ = $3; }
;

func_call:
  T_id '(' expr_list ')' { $$ = new FunctionCall($1, $3, mylineno); }
| T_id '(' ')' { $$ = new FunctionCall($1, mylineno); }
;

func_call_stmt:
  T_id '(' expr_list ')' { $$ = new FunctionCall($1, $3, mylineno); }
| T_id '(' ')' { $$ = new FunctionCall($1, mylineno); }
;

l_value:
  T_id { $$ = new Id($1, mylineno); }
| T_string_literal { $$ = new StringLiteral($1); }
| l_value '[' expr ']' { $$ = new ArrayAccess($1, $3); }
;

expr:
  T_int_const { $$ = new IntConst($1); }
| T_char_const { $$ = new CharConst($1); }
| l_value { $$ = $1; }
| '(' expr ')' { $$ = $2; }
| func_call { $$ = $1; }
| '+' expr %prec USIGN { $$ = $2; }
| '-' expr %prec USIGN { $$ = new Negative($2); } 
| expr '+' expr { $$ = new BinOp( $1, $2, $3 ); }
| expr '-' expr { $$ = new BinOp( $1, $2, $3 ); }
| expr '*' expr { $$ = new BinOp( $1, $2, $3 ); }
| expr T_div expr { $$ = new BinOp( $1, $2, $3 ); }
| expr T_mod expr { $$ = new BinOp( $1, $2, $3 ); }
;

cond:
  '(' cond ')' { $$ = $2; }
| T_not cond { $$ = new Not($2); }
| cond T_and cond { $$ = new BinOp( $1, $2, $3 ); }
| cond T_or cond { $$ = new BinOp( $1, $2, $3 ); }
| expr '=' expr { $$ = new BinOp( $1, $2, $3 ); }
| expr '#' expr { $$ = new BinOp( $1, $2, $3 ); }
| expr '<' expr { $$ = new BinOp( $1, $2, $3 ); }
| expr '>' expr { $$ = new BinOp( $1, $2, $3 ); }
| expr T_lessorequal expr { $$ = new BinOp( $1, $2, $3 ); }
| expr T_greaterorequal expr { $$ = new BinOp( $1, $2, $3 ); }
;


%%

int main() {
  int result = yyparse();
  if (result == 0) printf("Success.\n");
  return result;
}
