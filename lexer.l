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

\'{COMMONCHAR}\' { yylval.charval = yytext[1]; return T_char_const; }
\'{ESCAPESEQ}\' { yylval.charval = get_escape_char(yytext[1], yytext[2]); return T_char_const; }
\'{HEX}\' { yylval.charval = get_char_from_hex(yytext[3], yytext[4]); return T_char_const; }

\"({COMMONCHAR}|({ESCAPESEQ})|({HEX}))*\" { yylval.stringval = get_string(yytext+1,yyleng-2);
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

char get_escape_char(char c1, char c2) {
  if(c2 == 'n') return '\n';
  if(c2 == 't') return '\t';
  if(c2 == 'r') return '\r';
  if(c2 == '0') return '\0';
  if(c2 == '\\') return '\\';
  if(c2 == '\'') return '\'';
  if(c2 == '\"') return '\"';
  return c1; //won't be reached
}

char get_char_from_hex(char c1, char c2) {
  return std::stoi(std::string(1, c1), 0, 16)*16 + std::stoi(std::string(1, c2), 0, 16);
}

std::string *get_string(char *str, int len) {
  std::string *stringval = new std::string(str, len);
  size_t pos = stringval->find("\\", 0);
    while(pos != std::string::npos) {
      if(stringval->at(pos + 1) == 'x') {
        stringval->replace(pos, 4, std::string(1, (char)std::stoi(stringval->substr(pos + 2, 2), nullptr, 16)));
      }
      else if(stringval->at(pos + 1) == 'n') {
        stringval->replace(pos, 2, "\n");
      }
      else if(stringval->at(pos + 1) == 't') {
        stringval->replace(pos, 2, "\t");
      }
      else if(stringval->at(pos + 1) == 'r') {
        stringval->replace(pos, 2, "\r");
      }
      else if(stringval->at(pos + 1) == '\\') {
        stringval->replace(pos, 2, "\\");
      }
      else if(stringval->at(pos + 1) == '\'') {
        stringval->replace(pos, 2, "\'");
      }
      else if(stringval->at(pos + 1) == '\"') {
        stringval->replace(pos, 2, "\"");
      }
      pos = stringval->find("\\", pos + 1);
    }
    return stringval;
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
