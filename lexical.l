digit [0-9]
hex_head "0x"|"0X"
hex_char [0-9a-fA-F]
not_hex_char [g-zG-Z]
oct_head 0
oct_char [0-7]
not_oct_char [8-9]
letter [_a-zA-Z]
%option yylineno

%{
    #include <stdlib.h>
    #include <string.h>
    #include <stdio.h>
    #include <math.h>
    #include "syntax_tree.h"
    #include "syntax.tab.h"
    int yycolumn = 1;
    extern int error_num;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;
    
%}

%%

"struct" {
    yylval.node = new_node(yylineno, "STRUCT", STRUCT, NULL);
    //printf("STRUCT ");
    return STRUCT;
}
"return" {
    yylval.node = new_node(yylineno, "RETURN", RETURN, NULL);
    //printf("RETURN ");
    return RETURN;
}
"if" {
    yylval.node = new_node(yylineno, "IF", IF, NULL);
    //printf("IF");
    return IF;
}
"else" {
    yylval.node = new_node(yylineno, "ELSE", ELSE, NULL);
    //printf("ELSE ");
    return ELSE;
}
"while" {
    yylval.node = new_node(yylineno, "WHILE", WHILE, NULL);
    //printf("WHILE ");
    return WHILE;
}
"int"|"float" {
    char *s = malloc(strlen(yytext) + 1);
    strcpy(s, yytext);
    yylval.node = new_node(yylineno, "TYPE", TYPE, s);
    //printf("TYPE ");
    return TYPE;
}
0|[1-9]{digit}* {
    unsigned int val = atol(yytext);
    yylval.node = new_node(yylineno, "INT", INT, &val);
    //printf("INT ");
    return INT;
}
{digit}*\.{digit}*[eE][\+\-]?{digit}+ {
    float val = strtof(yytext, NULL);
    yylval.node = new_node(yylineno, "FLOAT", FLOAT, &val);
    //printf("FLOAT ");
    return FLOAT;
}
{digit}+\.{digit}+ {
    float val = strtof(yytext, NULL);
    yylval.node = new_node(yylineno, "FLOAT", FLOAT, &val);
    //printf("FLOAT ");
    return FLOAT;
}
{hex_head}[0-9a-zA-Z]*{not_hex_char}+[0-9a-zA-Z]* {
    printf("Error type A at Line %d: Illegal hexadecimal number '%s'.\n", yylineno, yytext);
    error_num++;
    int val = 0;
    yylval.node = new_node(yylineno, "INT", INT, &val);
    return INT;
}
{hex_head}{hex_char}+ {
    unsigned int val = 0;
    sscanf(yytext, "%x", &val);
    yylval.node = new_node(yylineno, "INT", INT, &val);
    //printf("HEX ");
    return INT;
}
{oct_head}{digit}*{not_oct_char}{digit}* {
    printf("Error type A at Line %d: Illegal octal number '%s'.\n", yylineno, yytext);
    error_num++;
    int val = 0;
    yylval.node = new_node(yylineno, "INT", INT, &val);
    return INT;
}
{oct_head}{oct_char}+ {
    unsigned int val = 0;
    sscanf(yytext, "%o", &val);
    yylval.node = new_node(yylineno, "INT", INT, &val);
    //printf("OCT ");
    return INT;
}
{letter}({digit}|{letter})* {
    char *s = malloc(strlen(yytext) + 1);
    strcpy(s, yytext);
    yylval.node = new_node(yylineno, "ID", ID, s);
    //printf("ID ");
    return ID;
}
"//"[^\n]* {}
"/*" {
    char a = input(), b = input();
    while(a != '*' || b != '/'){
        a = b;
        b = input();
    }
    //printf("COMMENT ");
 }
";" {
    yylval.node = new_node(yylineno, "SEMI", SEMI, NULL);
    //printf("SEMI ");
    return SEMI;
}
"," {
    yylval.node = new_node(yylineno, "COMMA", COMMA, NULL);
    //printf("COMMA ");
    return COMMA;
}
"=" {
    yylval.node = new_node(yylineno, "ASSIGNOP", ASSIGNOP, NULL);
    //printf("ASSIGNOP ");
    return ASSIGNOP;
}
">"|"<"|">="|"<="|"=="|"!=" {
    yylval.node = new_node(yylineno, "RELOP", RELOP, NULL);
    //printf("RELOP ");    
    return RELOP;
}
\+ {
    yylval.node = new_node(yylineno, "PLUS", PLUS, NULL);
    //printf("PLUS ");
    return PLUS;
}
"-" {
    yylval.node = new_node(yylineno, "MINUS", MINUS, NULL);
    //printf("MINUS ");
    return MINUS;
}
\* {
    yylval.node = new_node(yylineno, "STAR", STAR, NULL);
    //printf("STAR ");
    return STAR;
}
\/ {
    yylval.node = new_node(yylineno, "DIV", DIV, NULL);
    //printf("DIV ");
    return DIV;
}
"&&" {
    yylval.node = new_node(yylineno, "AND", AND, NULL);
    //printf("AND ");
    return AND;
}
"||" {
    yylval.node = new_node(yylineno, "OR", OR, NULL);
    //printf("OR");
    return OR;
}
\. {
    yylval.node = new_node(yylineno, "DOT", DOT, NULL);
    //printf("DOT ");
    return DOT;
}
"!" {
    yylval.node = new_node(yylineno, "NOT", NOT, NULL);
    //printf("NOT ");
    return NOT;
}
\( {
    yylval.node = new_node(yylineno, "LP", LP, NULL);
    //printf("LP ");
    return LP;
}
\) {
    yylval.node = new_node(yylineno, "RP", RP, NULL);
    //printf("RP ");
    return RP;
}
\[ {
    yylval.node = new_node(yylineno, "LB", LB, NULL);
    //printf("LB ");
    return LB;
}
\] {
    yylval.node = new_node(yylineno, "RB", RB, NULL);
    //printf("RB ");
    return RB;
}
\{ {
    yylval.node = new_node(yylineno, "LC", LC, NULL);
    //printf("LC ");
    return LC;
}
\} {
    yylval.node = new_node(yylineno, "RC", RC, NULL);
    //printf("RC ");
    return RC;
}
\n {yycolumn = 1;}
[" "|\r|\t] {}
. {
    error_num++;
    printf("Error type A at line %d: Mysterious characters '%s'.\n", yylineno, yytext);
}

%%
