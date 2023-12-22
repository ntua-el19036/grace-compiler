#ifndef __LEXER_HPP__
#define __LEXER_HPP__
#include <string>

int yylex();
void yyerror(const char *msg);
char get_escape_char(char c1, char c2);
char get_char_from_hex(char c1, char c2);
std::string *get_string(char *str, int len);

#endif
