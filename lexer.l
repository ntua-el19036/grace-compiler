${
#define T_eof 0
#define T_and 1001
#define T_int 1002
#define T_then 1003
#define T_char 1004
#define T_mod 1005
#define T_var 1006
#define T_div 1007
#define T_not 1008
#define T_while 1009
#define T_do 1010
#define T_nothing 1011
#define T_else 1012
#define T_or 1013
#define T_fun 1014
#define T_ref 1015
#define T_if 1016
#define T_return 1017

{L}({L}|{D}|\_)* { return T_id; }
{D}+ { return T_constint; }

"<=" { return T_lessorequal; }
">=" { return T_greaterorequal; }
"<-" { return T_assign; }
%}

L [a-zA-Z]
D [0-9]
W [ \t\n\r]

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
{D}+ { return T_constint; }

"<=" { return T_lessorequal; }
">=" { return T_greaterorequal; }
"<-" { return T_assign; }

[\+\-\*\=\#\<\>\(\)\[\]\{\}\,\;\:] { return yytext[0]; }

{W}+ { /* nothing */ }
\$[^\$].*\n { /* single line comment - nothing */ }
\$\$([^\$]+|\$[^\$])*\$\$ { /* multiple line comment - nothing */ }

. { yyerror("lexical error"); }

%%

void yyerror(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(1);
}
