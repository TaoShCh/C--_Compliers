digits [0-9]
letter [_a-zA-Z]

%{
    #include <stdlib>
    enum{
        INT, FLOAT, SEMI, COMMA, ASSIGNOP, 
        RELOP, PLUS, MINUS, STAR, DIV, AND, 
        OR, DOT, NOT, TYPE, LP, RP, LB, RB,
        LC, RC, STRUCT, RETURN, IF, ELSE, WHILE
    }
    typedef struct tks{
        int type;
        void *value
    }Tokens;
%}

%%
{digits}+ {
    Tokens *p = malloc(sizeof(Tokens));
    p -> type = INT;
    p -> value = malloc(sizeof(int));
    *(p -> value) = atoi(yytext);
}

%%