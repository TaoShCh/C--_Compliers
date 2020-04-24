
%{
    #include "lex.yy.c"
    #include <stdio.h>
    #include "syntax_tree.h"
    extern int error_num;
%}

%error-verbose
%locations
/* declared types */
%union {
    Node* node;
}

/* declared tokens */
%token <node> INT FLOAT ID
%token <node> SEMI COMMA
%token <node> ASSIGNOP RELOP PLUS MINUS STAR DIV AND OR DOT NOT
%token <node> TYPE
%token <node> LP RP LB RB LC RC
%token <node> STRUCT RETURN IF ELSE WHILE

%type <node> Program ExtDefList ExtDef ExtDecList
%type <node> Specifier StructSpecifier OptTag Tag
%type <node> VarDec FunDec VarList ParamDec
%type <node> CompSt StmtList Stmt
%type <node> DefList Def DecList Dec
%type <node> Exp Args


%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT UMINUS
%left LP RP LB RB DOT


%%
/* High-level Definition */
Program : ExtDefList{
    if($1 == NULL){
        $$ = new_node(yylineno, "Program", 0, NULL);
    }
    else {
        $$ = new_node(@1.first_line, "Program", 0, NULL);
    }
    add_child($$, $1);
    root = $$; 
}
    ;

ExtDefList : ExtDef ExtDefList{
    $$ = new_node(@1.first_line, "ExtDefList", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
}
    |{
    $$ = NULL;
}
    ;

ExtDef : Specifier ExtDecList SEMI{
    $$ = new_node(@1.first_line, "ExtDef", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | Specifier SEMI{
    $$ = new_node(@1.first_line, "ExtDef", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
}
    | Specifier FunDec CompSt{
    $$ = new_node(@1.first_line, "ExtDef", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}   
    | Specifier FunDec SEMI{
    $$ = new_node(@1.first_line, "ExtDef", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
    }
    ;

ExtDecList : VarDec{
    $$ = new_node(@1.first_line, "ExtDecList", 0, NULL);
    add_child($$, $1);
}
    | VarDec COMMA ExtDecList{
    $$ = new_node(@1.first_line, "ExtDecList", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    ;

/* Specifiers */
Specifier : TYPE{
    $$ = new_node(@1.first_line, "Specifier", 0, NULL);
    add_child($$, $1);
}
    | StructSpecifier{
    $$ = new_node(@1.first_line, "Specifier", 0, NULL);
    add_child($$, $1);
}
    ;

StructSpecifier : STRUCT OptTag LC DefList RC{
    $$ = new_node(@1.first_line, "StructSpecifier", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
    add_child($$, $4);
    add_child($$, $5);
}
    | STRUCT Tag{
    $$ = new_node(@1.first_line, "StructSpecifier", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
}
    ;

OptTag : ID{
    $$ = new_node(@1.first_line, "OptTag", 0, NULL);
    add_child($$, $1);
}
    |{
    $$ = NULL;
}
    ;

Tag : ID{
    $$ = new_node(@1.first_line, "Tag", 0, NULL);
    add_child($$, $1);
}
    ;

/* Declarators */
VarDec : ID{
    $$ = new_node(@1.first_line, "VarDec", 0, NULL);
    add_child($$, $1);
}
    | VarDec LB INT RB{
    $$ = new_node(@1.first_line, "VarDec", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
    add_child($$, $4);
}
    | VarDec LB INT error{}
    ;

FunDec : ID LP VarList RP{
    $$ = new_node(@1.first_line, "FunDec", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
    add_child($$, $4);
}
    | ID LP RP{
    $$ = new_node(@1.first_line, "FunDec", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    ;

VarList : ParamDec COMMA VarList{
    $$ = new_node(@1.first_line, "VarList", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | ParamDec{
    $$ = new_node(@1.first_line, "VarList", 0, NULL);
    add_child($$, $1);
}
    ;

ParamDec : Specifier VarDec{
    $$ = new_node(@1.first_line, "ParamDec", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
}

/* Statements */
CompSt : LC DefList StmtList RC{
    $$ = new_node(@1.first_line, "CompSt", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
    add_child($$, $4);
}
    | error RC{}
    | LC DefList StmtList error{}
    ;

StmtList : Stmt StmtList{
    $$ = new_node(@1.first_line, "StmtList", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
}
    |{
    $$ = NULL;
}
    ;

Stmt : Exp SEMI{
    $$ = new_node(@1.first_line, "Stmt", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
}

    | CompSt{
    $$ = new_node(@1.first_line, "Stmt", 0, NULL);
    add_child($$, $1);
}
    | RETURN Exp SEMI{
    $$ = new_node(@1.first_line, "Stmt", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}

    | IF LP Exp RP Stmt{
    $$ = new_node(@1.first_line, "Stmt", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
    add_child($$, $4);
    add_child($$, $5);
}
    | IF LP Exp RP Stmt ELSE Stmt{
    $$ = new_node(@1.first_line, "Stmt", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
    add_child($$, $4);
    add_child($$, $5);
    add_child($$, $6);
    add_child($$, $7);
}
    | WHILE LP Exp RP Stmt{
    $$ = new_node(@1.first_line, "Stmt", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
    add_child($$, $4);
    add_child($$, $5);
}   
    | error SEMI{}
    ;

/* Local Definitions */
DefList : Def DefList{
    $$ = new_node(@1.first_line, "DefList", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
}
    |{
    $$ = NULL;
}
    ;

Def : Specifier DecList SEMI{
    $$ = new_node(@1.first_line, "Def", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    ;

DecList : Dec{
    $$ = new_node(@1.first_line, "DecList", 0, NULL);
    add_child($$, $1);
}
    | Dec COMMA DecList{
    $$ = new_node(@1.first_line, "DecList", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    ;

Dec : VarDec{
    $$ = new_node(@1.first_line, "Dec", 0, NULL);
    add_child($$, $1);
}
    | VarDec ASSIGNOP Exp{
    $$ = new_node(@1.first_line, "Dec", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | Exp AND Exp{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | Exp OR Exp{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | Exp RELOP Exp{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | Exp PLUS Exp{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | Exp MINUS Exp{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | Exp STAR Exp{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | Exp DIV Exp{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | LP Exp RP{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | MINUS Exp %prec UMINUS{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
}
    | NOT Exp{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
}
    | ID LP Args RP{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
    add_child($$, $4);
}
    | ID LP RP{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | Exp LB Exp RB{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
    add_child($$, $4);
}
    | Exp DOT ID{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
} 
    | ID{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
}
    | INT{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
}
    | FLOAT{
    $$ = new_node(@1.first_line, "Exp", 0, NULL);
    add_child($$, $1);
}
    | error RP{}
    | Exp LB Exp error{}
    ;

Args : Exp COMMA Args{
    $$ = new_node(@1.first_line, "Args", 0, NULL);
    add_child($$, $1);
    add_child($$, $2);
    add_child($$, $3);
}
    | Exp{
    $$ = new_node(@1.first_line, "Args", 0, NULL);
    add_child($$, $1);
}
    ;

%%

void yyerror(char const *msg){    
    error_num++;
    printf("Error type B at Line %d: %s.\n", yylineno, msg);
}