%code requires{
    #include <string>
    }

%{
#include <cstdio>
#include "lexer.hpp"
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
    Block *block;
    Stmt *stmt;
    Expr *expr;
    int num;
    std::string *var;
    std::string *op;
    char charval;
    std::string *stringval;
}

%type<stmt>  stmt
%type<expr>  expr

%%

program: func_def
;

func_def: header local_def_list block
;

local_def_list:
  /* nothing */
| local_def_list local_def
;

header:
  "fun" T_id '(' func_param_def_list ')' ':' ret_type
| "fun" T_id '(' ')' ':' ret_type
;

func_param_def_list:
  func_param_def
| func_param_def ';' func_param_def_list
;

func_param_def:
  "ref" id_list ':' func_param_type
| id_list ':' func_param_type
;

id_list:
  T_id
| T_id ',' id_list
;

data_type:
  "int"
| "char"
;

type:
  data_type array_dimension
;

array_dimension:
  /* nothing */
| '[' T_int_const ']' array_dimension
;

ret_type:
  data_type
| "nothing"
;

func_param_type:
  type
| data_type '[' ']' array_dimension
;

local_def:
  func_def
| func_decl
| var_def
;

func_decl:
  header ';'
;

var_def:
  "var" id_list ':' type ';'
;

stmt:
  ';'
| l_value "<-" expr ';'
| block
| func_call ';'
| "if" cond T_then stmt //ambiguous <3
| "if" cond T_then stmt T_else stmt //ambiguous <3
| "while" cond "do" stmt
| "return" ';'
| "return" expr ';'
;

stmt_list:
  /* nothing */
| stmt stmt_list
;

block:
  '{' stmt_list '}'
;

expr_list:
  expr
| expr ',' expr_list
;

func_call:
  T_id '(' expr_list ')'
| T_id '(' ')' 
;

l_value:
  T_id { $$ = new Id($1); }
| T_string_literal { $$ = new StringLiteral($1); }
| l_value '[' expr ']'
;

expr:
  T_int_const { $$ = new IntConst($1); }
| T_char_const { $$ = new CharConst($1); }
| l_value { $$ = $1; }
| '(' expr ')' { $$ = $2; }
| func_call { $$ = $1; }
| '+' expr %prec USIGN { $$ = $2; }
| '-' expr %prec USIGN { $$ = new Negative($2); }
| expr '+' expr 
| expr '-' expr
| expr '*' expr
| expr T_div expr
| expr T_mod expr
;

cond:
  '(' cond ')'
| T_not cond
| cond T_and cond
| cond T_or cond
| expr '=' expr
| expr '#' expr
| expr '<' expr
| expr '>' expr
| expr T_lessorequal expr
| expr T_greaterorequal expr
;


%%

int main() {
  int result = yyparse();
  if (result == 0) printf("Success.\n");
  return result;
}
