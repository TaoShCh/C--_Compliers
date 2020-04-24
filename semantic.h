#ifndef SEMANTIC
#define SEMANTIC

#define HASH_TABLE_SIZE 0x3fff

typedef struct _Type Type;
typedef struct _FieldList FieldList;
typedef struct _Var_hash_node Var_hash_node;
typedef struct _Func_hash_node Func_hash_node;
typedef struct _Type_node Type_node;
typedef struct _Var_list_node Var_list_node;
typedef struct _Var_list_stack Var_list_stack;

Var_hash_node *var_hash_table[HASH_TABLE_SIZE + 1];
Func_hash_node *func_hash_table[HASH_TABLE_SIZE + 1];

struct _FieldList{
    char* name;
    Type* type;
    FieldList *next;
};


enum{
    INT_TYPE, FLOAT_TYPE
};

struct _Type{
    enum{BASIC, ARRAY, STRUCTURE} kind;
    union{
        int basic;
        struct{
            Type* elem;
            int size;
        }array;
        FieldList* structure;
    }u;
};

struct _Var_hash_node
{  
    char *name;
    int lineno;
    int depth;
    Type *type;
    Var_hash_node *next;
    Var_hash_node *last;
};

struct _Type_node{
    Type *type;
    Type_node *next;
};

struct _Func_hash_node{
    char *name;
    int lineno;
    int whether_dec;
    int whether_def;
    Func_hash_node *next;
    Func_hash_node *last;
    Type *return_type;
    Type_node* para_type_list;
};

struct _Var_list_node{
    Var_hash_node *node;
    Var_list_node *next;
};

struct _Var_list_stack{
    Var_list_node *head;
    Var_list_node *tail;
};

#define MAX_DEPTH 30

Var_list_stack my_stack[MAX_DEPTH];

void semantic_func();

#endif

