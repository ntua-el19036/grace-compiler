%{
#include <stdio.h>
#include "lexer.h"
%}

%token T_int "int"
%token T_then "then"
%token T_char "char"
%token T_var "var"
%token T_while "while"
%token T_do "do"
%token T_nothing "nothing"
%token T_else "else"
%token T_fun "fun"
%token T_ref "ref"
%token T_if "if"
%token T_return "return"
%token T_assign "<-"
%token T_id
%token T_int_const
%token T_char_const
%token T_string_literal


%left T_or
%left T_and
%nonassoc T_not
%nonassoc '=' '#' '<' '>' T_lessorequal T_greaterorequal
%left '+' '-'
%left '*' T_div T_mod
%nonassoc USIGN

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
"fun" T_id "(" //... to be continued

%%

int main() {
  int result = yyparse();
  if (result == 0) printf("Success.\n");
  return result;
}
