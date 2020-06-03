#ifndef INTERCODE
#define INTERCODE

typedef struct _Operand Operand;
typedef struct _InterCode InterCode;
typedef struct _CodeList CodeList;
typedef struct _ArgList ArgList;

#define MAX_STR 100

struct _Operand{
    enum{VARIABLE, CONSTANT, ADDRESS, LABEL, ARR_STRU} kind;
    union{
        int var_no;
        int label_no;
        int val;
    }u;
};

struct _InterCode{
    enum{ASSIGN, LABEL_I, PLUS_I, MINUS_I, MUL_I, DIV_I, FUNC, GOTO, IF_GOTO, RET, DEC, ARG, CALL, PARAM, READ, WRITE, RETURN_I, CHANGE_ADDR} kind;
    union{
        Operand *op;
        char *func;
        struct{Operand *right, *left; } assign;
        struct{Operand *result, *op1, *op2; } binop;
        struct{Operand *x, *y, *z; char *relop;} if_goto;
        struct{Operand *x; int size;} dec;
        struct{Operand *result; char *func;} call;
    }u;
};

struct _CodeList{
    InterCode *code;
    CodeList *prev, *next;
};

struct _ArgList{
    Operand *args;
    ArgList *next;
};

CodeList *intercodes_head, *intercodes_tail;

int var_num, label_num;

void InterCode_function();
#endif