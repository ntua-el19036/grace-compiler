%code requires{
    #include <string>
    }

%{
#include <cstdio>
#include "lexer.hpp"
#include "ast.hpp"

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
    AST * ast;
    Block *block;
    Stmt *stmt;
    Expr *expr;
    int num;
    std::string var;
    char op;
    char charval;
    std::string *stringval;
    DataType data_type;
    IdList *idlist;
    FuncParamDef *funparamdef;
}

%type<stmt>  stmt
%type<expr>  expr cond
%type<block> block stmt_list 
%type<data_type> ret_type data_type func_param_type
%type<ast> program func_def local_def local_def_list header func_param_def func_param_def_list func_call
%type<idlist> id_list
%type<funparamdef> func_param_def 

%%

program: 
    func_def { 
        std::cout << "AST: " << *$1 << std::endl;
    //     $1->run(); 
    }
;

func_def: 
    header local_def_list block
;

local_def_list:
  /* nothing */
| local_def_list local_def
;

header:
  "fun" T_id '(' func_param_def_list ')' ':' ret_type {}
| "fun" T_id '(' ')' ':' ret_type
;

func_param_def_list:
  func_param_def  { $$ = new FuncParamDefList($1); }
| func_param_def ';' func_param_def_list { $3->append_func_param($1); $$ = $3; }
;

func_param_def:
  "ref" id_list ':' func_param_type 
| id_list ':' func_param_type { $$ = new FuncParamDef($1, $3); }
;

id_list:
  T_id { $$ = new IdList($1); }
| T_id ',' id_list { $3->appendId($1); $$ = $3; }
;

data_type:
  "int" { $$ = TYPE_int; }
| "char" { $$ = TYPE_char; }
;

type:
  data_type array_dimension
;

array_dimension:
  /* nothing */
| '[' T_int_const ']' array_dimension
;

ret_type:
  data_type { $$ = $1; }
| "nothing" { $$ = TYPE_nothing; }
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
  "var" id_list ':' type ';' { $$ = new VarDecl($1)}
;

stmt:
  ';' { $$ = new EmptyStmt(); }
| l_value "<-" expr ';'
| block { $$ = $1; }
| func_call ';' { $$ = $1; }
| "if" cond T_then stmt { $$ = new If($2, $4); }
| "if" cond T_then stmt T_else stmt { $$ = new If($2, $4, $6); }
| "while" cond "do" stmt { $$ = new While($2, $4); }
| "return" ';' 
| "return" expr ';'
;

stmt_list:
  /* nothing */ { $$ = new Block(); }
| stmt_list stmt { $1->append_stmt($2); $$ = $1; }
;

block:
  '{' stmt_list '}' { $$ = $2; }
;

expr_list:
  expr
| expr ',' expr_list
;

func_call:
  T_id '(' expr_list ')' { $$ = new FunctionCall($1, $3); }
| T_id '(' ')' { $$ = new FunctionCall($1); }
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
| expr '+' expr { $$ = new Binop( $1, $2, $3 ); }
| expr '-' expr { $$ = new Binop( $1, $2, $3 ); }
| expr '*' expr { $$ = new Binop( $1, $2, $3 ); }
| expr T_div expr { $$ = new Binop( $1, $2, $3 ); } 
| expr T_mod expr { $$ = new Binop( $1, $2, $3 ); }
;

cond:
  '(' cond ')' { $$ = $1; }
| T_not cond { $$ = new Not($2); }
| cond T_and cond { $$ = new Binop( $1, $2, $3 ); }
| cond T_or cond { $$ = new Binop( $1, $2, $3 ); }
| expr '=' expr { $$ = new Binop( $1, $2, $3 ); }
| expr '#' expr { $$ = new Binop( $1, $2, $3 ); }
| expr '<' expr { $$ = new Binop( $1, $2, $3 ); }
| expr '>' expr { $$ = new Binop( $1, $2, $3 ); }
| expr T_lessorequal expr { $$ = new Binop( $1, $2, $3 ); }
| expr T_greaterorequal expr { $$ = new Binop( $1, $2, $3 ); }
;


%%

int main() {
  int result = yyparse();
  if (result == 0) printf("Success.\n");
  return result;
}
