%option noyywrap



%{
#include <string>
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


"and" { yylval.op = new std::string(yytext); return T_and; }
"int" { return T_int; }
"then" { return T_then; }
"char" { return T_char; }
"mod" { yylval.op = new std::string(yytext); return T_mod; }
"var" { return T_var; }
"div" { yylval.op = new std::string(yytext); return T_div; }
"not" { return T_not; }
"while" { return T_while; }
"do" { return T_do; }
"nothing" { return T_nothing; }
"else" { return T_else; }
"or" { yylval.op = new std::string(yytext); return T_or; }
"fun" { return T_fun; }
"ref" { return T_ref; }
"if" { return T_if; }
"return" { return T_return; }

{L}({L}|{D}|\_)* { yylval.var = new std::string(yytext); return T_id; }
{D}+ { yylval.num = atoi(yytext); return T_int_const; }
\'({COMMONCHAR}|({ESCAPESEQ})|({HEX}))\' { yylval.charval = yytext[1]; return T_char_const; }
\"({COMMONCHAR}|({ESCAPESEQ})|({HEX}))*\" { yylval.stringval = new std::string(yytext+1,yyleng-2);
/* not sure */ return T_string_literal; }

"<=" { yylval.op = new std::string(yytext); return T_lessorequal; }
">=" { yylval.op = new std::string(yytext); return T_greaterorequal; }
"<-" { return T_assign; }

[\+\-\*\=\#\<\>\(\)\[\]\{\}\,\;\:] { yylval.op = new std::string(yytext); return yytext[0]; }

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
