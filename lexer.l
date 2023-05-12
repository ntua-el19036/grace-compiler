%option noyywrap

%{
#include <cstdio>
#include <cstdlib>
#include "lexer.hpp"
#include "parser.hpp"
%}

L [a-zA-Z]
D [0-9]
W [ \t\n\r]
COMMONCHAR [ \!\#-\&\(-\[\]-\~]
ESCAPESEQ \\[ntr0\\\'\"]
HEX \\x[0-9a-fA-f][0-9a-fA-f]

%%


"and" { return T_and; }
"int" { return T_int; }
"then" { return T_then; }
"char" { return T_char; }
"mod" { return T_mod; }
"var" { return T_var; }
"div" { return T_div; }
"not" { return T_not; }
"while" { return T_while; }
"do" { return T_do; }
"nothing" { return T_nothing; }
"else" { return T_else; }
"or" { return T_or; }
"fun" { return T_fun; }
"ref" { return T_ref; }
"if" { return T_if; }
"return" { return T_return; }

{L}({L}|{D}|\_)* { return T_id; }
{D}+ { return T_int_const; }
\'({COMMONCHAR}|({ESCAPESEQ})|({HEX}))\' {  return T_char_const; }
\"({COMMONCHAR}|({ESCAPESEQ})|({HEX}))*\" { /* not sure */ return T_string_literal; }

"<=" { return T_lessorequal; }
">=" { return T_greaterorequal; }
"<-" { return T_assign; }

[\+\-\*\=\#\<\>\(\)\[\]\{\}\,\;\:] { return yytext[0]; }

{W}+ { /* nothing */ }
\$[^\$].*\n { /* single line comment - nothing */ }
\$\$([^\$]+|\$[^\$])*\$\$ { /* multiple line comment - nothing */ }

. { fprintf(stderr, "Illegal character %s", yytext);
  exit(1); }

%%

void yyerror(const char* msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

/*
int main () {
  int token;
  do {
    token = yylex();
    printf("token=%d, lexeme=\"%s\"\n", token, yytext);
  } while (token != 0);
}
*/
