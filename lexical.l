digit [0-9]
hex_head "0x"|"0X"
hex_char [0-9a-fA-F]
not_hex_char [g-zG-Z]
oct_head 0
oct_char [0-7]
not_oct_char [8-9a-zA-Z]
letter [_a-zA-Z]
%option yylineno
%{
    #include <stdlib.h>
    #include <string.h>
    #include <stdio.h>
    enum{
        INT = 1, FLOAT, ID, SEMI, COMMA, ASSIGNOP, 
        RELOP, PLUS, MINUS, STAR, DIV, AND, 
        OR, DOT, NOT, TYPE, LP, RP, LB, RB,
        LC, RC, STRUCT, RETURN, IF, ELSE, WHILE
    };

    
%}

%%

"struct" {printf("STRUCT "); return STRUCT;}
"return" {printf("RETURN "); return RETURN;}
"if" {printf("IF "); return IF;}
"else" {printf("ELSE "); return ELSE;}
"while" {printf("WHILE "); return WHILE;}
"int"|"float" {printf("TYPE "); return TYPE;}
0|[1-9]{digit}* {printf("INT "); return INT;}
{digit}*\.{digit}*[eE][\+\-]?{digit}+ {printf("FLOAT "); return FLOAT;}
{digit}+\.{digit}+ {printf("FLOAT "); return FLOAT;}
{hex_head}[0-9a-zA-Z]*{not_hex_char}+[0-9a-zA-Z]* {
    printf("Error type A at Line %d: Illegal hexadecimal number '%s'\n", yylineno, yytext);
}
{hex_head}{hex_char}+ {printf("HEX "); return INT;}
{oct_head}{digit}*{not_oct_char}{digit}* {
    printf("Error type A at Line %d: Illegal octal number '%s'\n", yylineno, yytext);
}
{oct_head}{oct_char}+ {printf("OCT "); return INT;}
{letter}({digit}|{letter})* {printf("ID "); return ID;}
"//"[^\n]* {printf("COMMENT");}
"/*" {
    char a = input(), b = input();
    while(a != '*' || b != '/'){
        a = b;
        b = input();
    }
    printf("COMMENT");}
";" {printf("SEMI "); return SEMI;}
"," {printf("COMMA "); return COMMA;}
= {printf("ASSIGNOP "); return ASSIGNOP;}
>|<|>=|<=|==|!= {printf("RELOP "); return RELOP;}
\+ {printf("PLUS "); return PLUS;}
- {printf("MINUS "); return MINUS;}
\* {printf("STAR "); return STAR;}
\/ {printf("DIV "); return DIV;}
"&&" {printf("AND "); return AND;}
"||" {printf("OR "); return OR;}
\. {printf("DOT "); return DOT;}
! {printf("NOT "); return NOT;}
\( {printf("LP "); return LP;}
\) {printf("RP "); return RP;}
\[ {printf("LB "); return LB;}
\] {printf("RB "); return RB;}
\{ {printf("LC "); return LC;}
\} {printf("RC "); return RC;}
\n {printf("\n"); }
[" "|\r]+ {}
. {printf("Error type A at line %d: Mysterious characters '%s'\n", yylineno, yytext);}

%%