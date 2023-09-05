%option noyywrap



%{
#include <string>
#include <cstdio>
#include <cstdlib>
#include "lexer.hpp"
#include "parser.hpp"

int mylineno = 1;
%}

%option yylineno

L [a-zA-Z]
D [0-9]
W [ \t\r]
COMMONCHAR [ \!\#-\&\(-\[\]-\~]
ESCAPESEQ \\[ntr0\\\'\"]
HEX \\x[0-9a-fA-f][0-9a-fA-f]

%%


"and" { yylval.op = '&'; return T_and; }
"int" { return T_int; }
"then" { return T_then; }
"char" { return T_char; }
"mod" { yylval.op = '%'; return T_mod; }
"var" { return T_var; }
"div" { yylval.op = '/'; return T_div; }
"not" { return T_not; }
"while" { return T_while; }
"do" { return T_do; }
"nothing" { return T_nothing; }
"else" { return T_else; }
"or" { yylval.op = '|'; return T_or; }
"fun" { return T_fun; }
"ref" { return T_ref; }
"if" { return T_if; }
"return" { return T_return; }

{L}({L}|{D}|\_)* { yylval.var = new std::string(yytext); return T_id; }
{D}+ { yylval.num = atoi(yytext); return T_int_const; }
\'({COMMONCHAR}|({ESCAPESEQ})|({HEX}))\' { yylval.charval = yytext[1]; return T_char_const; }
\"({COMMONCHAR}|({ESCAPESEQ})|({HEX}))*\" { yylval.stringval = new std::string(yytext+1,yyleng-2);
/* not sure */ return T_string_literal; }

"<=" { yylval.op = 'l'; return T_lessorequal; }
">=" { yylval.op = 'g'; return T_greaterorequal; }
"<-" { return T_assign; }

[\+\-\*\=\#\<\>\(\)\[\]\{\}\,\;\:] { yylval.op = yytext[0]; return yytext[0]; }

{W}+ { /* nothing */ }
\n { mylineno++; }
\$[^\$].*\n { mylineno++; /* single line comment - nothing */}
\$\$([^\$]+|\$[^\$])*\$\$ { mylineno = yylineno; /* multiple line comment - nothing */ }

. { yyerror("Illegal character"); }

%%

void yyerror2(const char* msg, int lineno) {
    fprintf(stderr, "Error at line %d: %s\n", lineno, msg);
    exit(1);
}

void yyerror(const char* msg) {
    fprintf(stderr, "Error at line %d:\n%s\n", yylineno, msg);
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
