#include "semantic.h"
#include "syntax_tree.h"
#include "syntax.tab.h"
#include "intercode.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>


Type* handle_VarDec(Node *node, Type *basic_type);
Type* handle_StructSpecifier(Node *node);
Type* handle_Specifier(Node *node);
Type_node* handle_ParamDec(Node *node);
Type_node* handle_VarList(Node *node);
void handle_FunDec(Node *node, Type *return_type, int defined);
void handle_ExtDef(Node *node);
void handle_Def(Node *node);
Type *get_exp_type(Node *node);
void handle_Stmt(Node *node, Type *correct_type);
void handle_CompSt(Node *node, Type *correct_type, int func_flag);
int cur_depth = 0;

unsigned int hash_func(char *name){
    unsigned int val = 0;
    for(; *name; ++name){
        val = (val << 2) + *name;
        unsigned int i = val & ~HASH_TABLE_SIZE;
        if(i)
            val = (val ^ (i >> 12)) & HASH_TABLE_SIZE;
    }
    //printf("%d\n", val);
    return val;
}

Var_hash_node* get_var_hash_node(char *key){
    int id = hash_func(key);
    Var_hash_node *tmp = var_hash_table[id];
    Var_hash_node *ans = NULL;
    while(tmp != NULL){
        if(strcmp(key, tmp->name) == 0){
            ans = tmp;
        }
        tmp = tmp->next;
    }
    return ans;
}

Func_hash_node* get_func_hash_node(char *key){
    int id = hash_func(key);
    Func_hash_node *tmp = func_hash_table[id];
    while(tmp != NULL){
        if(strcmp(key, tmp->name) == 0){
            return tmp;
        }
        else{
            tmp = tmp->next;
        }
    }
    return NULL;
}

void semantic_error(int error_type, int lineno, char *name){
    char msg[100] = "\0";
    if(error_type == 1)
        sprintf(msg, "Undefined variable \"%s\".", name);
    else if(error_type == 2)
        sprintf(msg, "Undefined function \"%s\".", name);
    else if(error_type == 3)
        sprintf(msg, "Redefined variable \"%s\".", name);
    else if(error_type == 4)
        sprintf(msg, "Redefined function \"%s\".", name);
    else if(error_type == 5)
        sprintf(msg, "Type mismatched for assignment.");
    else if(error_type == 6)
        sprintf(msg, "The left-hand side of an assignment must be a variable.");
    else if(error_type == 7)
        sprintf(msg, "Type mismatched for operands.");
    else if(error_type == 8)
        sprintf(msg, "Type mismatched for return.");
    else if(error_type == 9)
        sprintf(msg, "Function \"%s\" is not applicable for arguments followed.", name);
    else if(error_type == 10)
        sprintf(msg, "\"%s\" is not an array.", name);
    else if(error_type == 11)
        sprintf(msg, "\"%s\" is not a function.", name);
    else if(error_type == 12)
        sprintf(msg, "\"%s\" is not an integer.", name);
    else if(error_type == 13)
        sprintf(msg, "Illegal use of \".\".");
    else if(error_type == 14)
        sprintf(msg, "Non-existent field \"%s\".", name);
    else if(error_type == 15){//two different error report format
        if(name == NULL)
            sprintf(msg, "Field cannot be initialized.");
        else 
            sprintf(msg, "Redefined field \"%s\".", name);
    }      
    else if(error_type == 16)
        sprintf(msg, "Duplicated name \"%s\".", name);
    else if(error_type == 17)
        sprintf(msg, "Undefined structure \"%s\".", name);
    else if(error_type == 18)
        sprintf(msg, "Undefined function \"%s\".", name);
    else if(error_type == 19)
        sprintf(msg, "Inconsistent declaration of function \"%s\".", name);
    else
        printf("Unknown error_type\n");
    printf("Error type %d at Line %d: %s\n", error_type, lineno, msg);
}


void table_init(){
    for(int i = 0; i <= HASH_TABLE_SIZE; i++){
        var_hash_table[i] = NULL;
        func_hash_table[i] = NULL;
    }
    for(int i = 0; i < MAX_DEPTH; i++){
        my_stack[i].head = my_stack[i].tail = NULL;
    }
}

static void node_type_check(Node *node, char *correct_name){
    if(node == NULL)
        printf("Error: %s NULL node\n", correct_name);
    //printf("%s\n", node->name);
    node->visited = 1;
    if(strcmp(node->name, correct_name) != 0){
        printf("It is a '%s' Node, not a '%s' Node\n",node->name, correct_name);
    }
}

Type* my_int_type = NULL;;
Type* my_float_type = NULL;
Type* new_type(int basic){
    if(basic == INT_TYPE){
        if(my_int_type == NULL){
            my_int_type = malloc(sizeof(Type));
            my_int_type->kind = BASIC;
            my_int_type->u.basic = INT_TYPE;
        }
        return my_int_type;
    }
    else if(basic == FLOAT_TYPE){
        if(my_float_type == NULL){
            my_float_type = malloc(sizeof(Type));
            my_float_type->kind = BASIC;
            my_float_type->u.basic = FLOAT_TYPE;
        }
        return my_float_type;
    }
    printf("Unknown basic type\n");
    return NULL;
}

int type_equal(Type *a, Type *b){
    if(a == NULL || b == NULL){
        //error has been detected
        return 1;
    }
    if(a->kind != b->kind){
        return 0;
    }
    else{
        if(a->kind == BASIC){
            return a->u.basic == b->u.basic;
        }
        else if(a->kind == ARRAY){
            return type_equal(a->u.array.elem, b->u.array.elem);
        }
        else if(a->kind == STRUCTURE){
            FieldList *tmpa = a->u.structure;
            FieldList *tmpb = b->u.structure;
            while(tmpa != NULL && tmpb != NULL){
                if(type_equal(tmpa->type, tmpb->type)){
                    tmpa = tmpa->next;
                    tmpb = tmpb->next;
                }
                else return 0;
            }
            if(tmpa != NULL || tmpb != NULL)
                return 0;
            else return 1;
        }
    }
    return 0;
}

void handle_scope(Var_hash_node *node){
    Var_list_stack *cur_slot = &my_stack[cur_depth];
    Var_list_node *list_node = malloc(sizeof(Var_list_node));
    list_node->next = NULL;
    list_node->node = node;
    if(cur_slot->head == NULL){
        cur_slot->head = list_node;
        cur_slot->tail = list_node;
    }
    else{
        cur_slot->tail->next = list_node;
        cur_slot->tail = list_node;
    }
}

void delete_val_from_table(Var_hash_node *node){
    int id = hash_func(node->name);
    if(node->last == NULL){
        var_hash_table[id] = node->next;
    }
    else{
        node->last->next = node->next;
        if(node->next != NULL){
            node->next->last = node->last;
        }
    }
    free(node);
}

void insert_to_val_table(char *name, int line, Type *type){
    //printf("%s\n", name);
    Var_hash_node *existed_node = get_var_hash_node(name);
    if(existed_node != NULL){
        if(type->kind == STRUCTURE){
            semantic_error(16, line, name);
            return;
        }
        else if(existed_node->type->kind == STRUCTURE){
            semantic_error(3, line, name);
        }
        else {
            //in the same scope
            if(existed_node->depth == cur_depth){
                semantic_error(3, line, name);
                return;
            }
        }
    }
    unsigned int i = hash_func(name);
    Var_hash_node *node = malloc(sizeof(Var_hash_node));
    node->name = malloc(strlen(name) + 1);
    strcpy(node->name, name);
    node->next = node->last = NULL;
    node->lineno = line;
    node->type = type;
    node->depth = cur_depth;
    node->op = NULL;
    if(var_hash_table[i] == NULL){
        var_hash_table[i] = node;
    }
    else{
        Var_hash_node *tmp = var_hash_table[i];
        while(tmp->next != NULL){
            tmp = tmp->next;
        }
        node->last = tmp;
        tmp->next = node;
    }
    handle_scope(node);
}

int paralist_equal(Type_node *para1, Type_node *para2){
    while(para1 != NULL && para2 != NULL){
        if(!type_equal(para1->type, para2->type))
            return 0;
        else{
            para1 = para1->next;
            para2 = para2->next;
        }
    }
    if(para1 != NULL || para2 != NULL)
        return 0;
    else return 1;
}


int func_equal(Func_hash_node *func1, Func_hash_node *func2){
    if(type_equal(func1->return_type, func2->return_type)){
        if(paralist_equal(func1->para_type_list, func2->para_type_list))
            return 1;
    }
    return 0;
}

void insert_to_func_table(char *name, int line, Type *return_type, Type_node* para_type_list, int defined){
    unsigned int i = hash_func(name);
    Func_hash_node *node = malloc(sizeof(Func_hash_node));
    node->name = malloc(strlen(name) + 1);
    strcpy(node->name, name);
    node->next = node->last = NULL;
    node->whether_dec = 0;
    node->whether_def = defined;
    node->lineno = line;
    node->return_type = return_type;
    node->para_type_list = para_type_list; 
    if(func_hash_table[i] == NULL){
        func_hash_table[i] = node;
    }
    else{
        Func_hash_node *tmp = func_hash_table[i];
        while(tmp->next != NULL){
            tmp = tmp->next;
        }
        node->last = tmp;
        tmp->next = node;
    }
}

Node* get_id_node(Node *Vardec){
    node_type_check(Vardec, "VarDec");
    Node *tmp = Vardec;
    while(tmp->type != ID){
        tmp = tmp->child[0];
    }
    return tmp;
}

static Type* get_id_type(Node *Vardec, Type *basic_type){
    node_type_check(Vardec, "VarDec");
    Type *ans = basic_type;
    while(Vardec->child_num == 4){
        Type *new_type = malloc(sizeof(Type));
        new_type->kind = ARRAY;
        new_type->u.array.size = Vardec->child[2]->val.i;
        new_type->u.array.elem = ans;
        ans = new_type;
        Vardec = Vardec->child[0];
    }
    return ans;
}

Type* get_field_type(FieldList *field, char *id){
    FieldList *tmp = field;
    while(tmp != NULL){
        if(strcmp(tmp->name, id) == 0){
            return tmp->type;
        }
        tmp = tmp->next;
    }
    return NULL;
}

int equal_args_type(Type_node *para_list, Node *args){
    node_type_check(args, "Args");
    while(args != NULL && para_list != NULL){
        if(!type_equal(para_list->type, get_exp_type(args->child[0])))
            return 0;
        else{
            para_list = para_list->next;
            args = args->child[2];
        }
    }
    if(args != NULL || para_list != NULL)
        return 0;
    else return 1;
}

Type* handle_Func_exp(Node *node){
    node_type_check(node, "Exp");
    node_type_check(node->child[1], "LP");
    char *name = node->child[0]->val.s;
    Func_hash_node *func = get_func_hash_node(name);
    if(func == NULL){
        if(get_var_hash_node(name) != NULL)
            semantic_error(11, node->line, name);
        else semantic_error(2, node->line, name);
        return NULL;
    }
    else{
        if(node->child_num == 3){
            if(func->para_type_list != NULL){
                semantic_error(9, node->line, name);
                return NULL;
            }
            else return func->return_type;
        }
        else if(node->child_num == 4){
            if(!equal_args_type(func->para_type_list, node->child[2])){
                semantic_error(9, node->line, name);
                return NULL;
            }
            else return func->return_type;
        } 
    }
}


Type* get_exp_type(Node* node){
    node_type_check(node, "Exp");
    if(node->child_num == 1){
        if(node->child[0]->type == ID){
            Var_hash_node *tmp = get_var_hash_node(node->child[0]->val.s);
            if(tmp == NULL){
                semantic_error(1, node->line, node->child[0]->val.s);
                return NULL;
            } 
            return tmp->type;
        }
        else if(node->child[0]->type == FLOAT){
            return new_type(FLOAT_TYPE);
        }
        else if(node->child[0]->type == INT){
            return new_type(INT_TYPE);
        }
        else printf("Unknown type\n");
    }
    else if(node->child_num == 3){
        if(strcmp(node->child[0]->name, node->child[2]->name) == 0){
            //Exp xx Exp
            int oper = node->child[1]->type;
            Type *type1 = get_exp_type(node->child[0]);
            Type *type2 = get_exp_type(node->child[2]);
            if(oper == PLUS || oper == MINUS || oper == STAR || oper == DIV){
                if(type_equal(type1, type2)){
                    //Exp1.type == Exp2.type
                    if(type1 == NULL || type2 == NULL)
                        return NULL;
                    else return type1;
                }
                else{
                    semantic_error(7, node->line, NULL);
                    return NULL;
                }
            }
            else if(oper == AND || oper == OR){
                if(type1 == NULL || type2 == NULL)//error has occurred before
                    return NULL;
                if(type1->u.basic != INT_TYPE || type2->u.basic != INT_TYPE){
                    semantic_error(7, node->line, NULL);
                    return NULL;
                }
                else{
                    return new_type(INT_TYPE);
                }
            }
            else if(oper == RELOP){ 
                if(type_equal(type1, type2)){
                    return new_type(INT_TYPE);
                }
                else{
                    semantic_error(7, node->line, NULL);
                    return NULL;
                }
            }
            else if(oper == ASSIGNOP){
                Node *exp = node->child[0];
                if(exp->child_num == 1 && exp->child[0]->type == ID
                || exp->child_num == 3 && exp->child[1]->type == DOT
                || exp->child_num == 4 && exp->child[1]->type == LB){
                    if(type_equal(type1, type2)){
                        //Exp1.type == Exp2.type
                        return type1;
                    }
                    else{
                        semantic_error(5, node->line, NULL);
                        return NULL;
                    }
                }
                else{
                    semantic_error(6, node->line, NULL);
                    return NULL;
                }
            }
        }
        else if(node->child[0]->type == LP){
            // LP Exp RP
            return get_exp_type(node->child[1]);
        }
        else if(node->child[1]->type == DOT){
            //Exp DOT ID TODO
            Type *type = get_exp_type(node->child[0]);
            if(type == NULL || type->kind != STRUCTURE){
                semantic_error(13, node->line, NULL);
            }
            else{
                char *id = node->child[2]->val.s;
                Type *field_type = get_field_type(type->u.structure, id);
                if(field_type == NULL){
                    semantic_error(14, node->line, id);
                }
                return field_type;
            }
        }
        else if(node->child[0]->type == ID){
            //Func()
            return handle_Func_exp(node);
        }
    }
    else if(node->child_num == 2){
        if(node->child[0]->type == MINUS){
            return get_exp_type(node->child[1]);
        }
        else if(node->child[0]->type == NOT){
            Type *type = get_exp_type(node->child[1]);
            if(type == NULL)//error has occurred before
                return NULL;
            if(type->u.basic != INT_TYPE){
                semantic_error(7, node->line, NULL);
                return NULL;
            }
            else{
                return new_type(INT_TYPE);
            }
        }
    }
    else if(node->child_num == 4){
        if(node->child[0]->type == ID){
            //Func(args)
            return handle_Func_exp(node);
        }
        else if(node->child[1]->type == LB){
            Type *type = get_exp_type(node->child[0]);
            if(type == NULL || type->kind != ARRAY){
                semantic_error(10, node->line, "Exp");
                return NULL;
            }
            else if(!type_equal(get_exp_type(node->child[2]), my_int_type)){
                semantic_error(12, node->line, "Exp");
                return NULL;
            }
            return type->u.array.elem;
        }
    }
}

Type* handle_VarDec(Node *node, Type *basic_type){
    node_type_check(node, "VarDec");
    Node *ID_node = get_id_node(node);
    Type *ID_type = get_id_type(node, basic_type);
    insert_to_val_table(ID_node->val.s, ID_node->line, ID_type);
    return ID_type;
}

FieldList* handle_struct_VarDec(Node *node, Type *basic_type){
    node_type_check(node, "VarDec");
    Node *ID_node = get_id_node(node);
    Type *ID_type = get_id_type(node, basic_type);
    FieldList *field = malloc(sizeof(FieldList));
    field->name = ID_node->val.s;
    field->type = ID_type;
    field->next = NULL;
    return field;
}

int redefined_in_FL(FieldList *fl, char *name){
    FieldList *tmp = fl;
    while(fl != NULL){
        if(strcmp(fl->name, name) == 0)
            return 1;
        fl = fl->next;
    }
    return 0;
}

Type* handle_StructSpecifier(Node *node){
    node_type_check(node, "StructSpecifier");
    Type *type = malloc(sizeof(Type));
    type->kind = STRUCTURE;
    type->u.structure = NULL;
    FieldList *last_node = type->u.structure;
    if(node->child_num == 5){
        Node *DefList = node->child[3];       
        while(DefList != NULL && DefList->child_num == 2){
            DefList->visited = 1;
            Node *Def = DefList->child[0];
            //handle type
            Type *basic_type = handle_Specifier(Def->child[0]);
            //handle variable
            Node *Declist_node = Def->child[1];
            while(1){
                Node *Dec_node = Declist_node->child[0];
                Node *Vardec_node = Dec_node->child[0];
                if(Dec_node->child_num == 1){
                    FieldList* field = handle_struct_VarDec(Vardec_node, basic_type); 
                    if(redefined_in_FL(type->u.structure, field->name)){
                        semantic_error(15, Dec_node->line, field->name);
                        free(field);
                    }
                    else{
                        if(last_node == NULL){
                            type->u.structure = last_node = field;
                        }
                        else{
                            last_node->next = field;
                            last_node = field;
                        }
                    }
                         
                }
                //Dec -> VarDec ASSIGNOP Exp
                else if(Dec_node->child_num == 3){
                    semantic_error(15, Dec_node->line, NULL);
                }
                if(Declist_node->child_num > 1){
                    Declist_node = Declist_node->child[2];
                }
                else break;
            }
            DefList = DefList->child[1];   
        }
        if(node->child[1] != NULL){
            Node *id_node = node->child[1]->child[0];
            insert_to_val_table(id_node->val.s, id_node->line, type);
        }
    }
    else if(node->child_num == 2){
        char *name = node->child[1]->child[0]->val.s;
        Var_hash_node *hash_node = get_var_hash_node(name);
        if(hash_node == NULL){
            semantic_error(17, node->line, name);
            return NULL;
        }
        type = hash_node->type;
    }
    return type;
}

Type* handle_Specifier(Node *node){
    node_type_check(node, "Specifier");
    Type *type;
    Node *type_node = node->child[0];
    if(type_node->type == TYPE){
        if(strcmp(type_node->val.s, "int") == 0){            
            type = new_type(INT_TYPE);
        }
        else if(strcmp(type_node->val.s, "float") == 0){
            type = new_type(FLOAT_TYPE);
        }
    }
    else if(strcmp(type_node->name, "StructSpecifier") == 0){
        type = handle_StructSpecifier(type_node);
        
    }
    return type;
}

Type_node* handle_ParamDec(Node *node){
    node_type_check(node, "ParamDec");
    Type *type = handle_Specifier(node->child[0]);
    type = handle_VarDec(node->child[1], type);
    Type_node* type_node = malloc(sizeof(Type_node));
    Node *ID = get_id_node(node->child[1]);
    type_node->type = type;
    type_node->next = NULL;
    type_node->name = ID->val.s;
    return type_node;
}

Type_node* handle_VarList(Node *node){
    node_type_check(node, "VarList");
    Node *cur_node = node;
    Type_node *type_node = handle_ParamDec(node->child[0]);
    if(node->child_num > 1){
        type_node->next = handle_VarList(node->child[2]);
    }
    return type_node;
}

void handle_FunDec(Node *node, Type *return_type, int defined){
    node_type_check(node, "FunDec");
    char *name = node->child[0]->val.s;
    int lineno = node->child[0]->line;
    Type_node *para_list = NULL;
    if(node->child_num == 4){
        para_list = handle_VarList(node->child[2]);
    }
    Func_hash_node *old_func = get_func_hash_node(name);
    if(old_func == NULL)      
        insert_to_func_table(name, lineno, return_type, para_list, defined);
    else{
        if(type_equal(return_type, old_func->return_type) && paralist_equal(para_list, old_func->para_type_list)){
            if(old_func->whether_def == 0){
                old_func->whether_def = defined;
            }
            else{
                if(defined == 1){//redefine function
                    semantic_error(4, node->line, name);
                }
            }
        }
        else{
            if(defined == 1 && old_func->whether_def == 1)
                semantic_error(4, node->line, name);
            else
                semantic_error(19, node->line, name);
        }
    }
}

void handle_ExtDef(Node *node){//node shoule be a ExtDef
    node_type_check(node, "ExtDef");    
    //handle type
    Type *basic_type = handle_Specifier(node->child[0]);     
    //handle variable
    if(strcmp(node->child[1]->name, "ExtDecList") == 0){
        Node *ExtDecList_node = node->child[1];
        while(1){
            Node *Vardec_node = ExtDecList_node->child[0];
            handle_VarDec(Vardec_node, basic_type);
            if(ExtDecList_node->child_num > 1){
                ExtDecList_node = ExtDecList_node->child[2];
            }
            else break;
        }
    }
    else if(strcmp(node->child[1]->name, "FunDec") == 0){
        Node *FunDec_node = node->child[1];
        if(node->child[2]->type == SEMI){
            handle_FunDec(FunDec_node, basic_type, 0);
        }
        else if(strcmp(node->child[2]->name, "CompSt") == 0){
            cur_depth++;
            handle_FunDec(FunDec_node, basic_type, 1);
            handle_CompSt(node->child[2], basic_type, 1);
        }
        else
            printf("Uknown FunDec\n");
        
    }
}

void handle_Def(Node *node){
    node_type_check(node, "Def");
    //handle type
    Type *basic_type = handle_Specifier(node->child[0]);
    //handle variable
    Node *Declist_node = node->child[1];
    while(1){
        Node *Dec_node = Declist_node->child[0];
        Node *Vardec_node = Dec_node->child[0];
        if(Dec_node->child_num == 1){
            handle_VarDec(Vardec_node, basic_type);
        }
        //Dec -> VarDec ASSIGNOP Exp
        else if(Dec_node->child_num == 3){
            Type *type = handle_VarDec(Vardec_node, basic_type);
            if(!type_equal(type, get_exp_type(Dec_node->child[2]))){
                semantic_error(5, Dec_node->line, NULL);
            }
            
        }
        if(Declist_node->child_num > 1){
            Declist_node = Declist_node->child[2];
        }
        else break;
    }
}

void handle_DefList(Node *node){
    if(node == NULL)
        return;
    node_type_check(node, "DefList");
    Node *DefList = node;
    while(DefList != NULL && DefList->child_num == 2){
        DefList->visited = 1;
        handle_Def(DefList->child[0]);
        DefList = DefList->child[1];
    }
}

void delete_stack(){
    Var_list_node *head = my_stack[cur_depth].head;
    while(head != NULL){
        delete_val_from_table(head->node);
        head = head->next;
    }
    my_stack[cur_depth].head = my_stack[cur_depth].tail = NULL;
    cur_depth--;
}

void handle_CompSt(Node *node, Type *correct_type, int func_flag){    
    node_type_check(node, "CompSt");
    if(!func_flag){//there is not a funcDec before it
        cur_depth++;
    }
    handle_DefList(node->child[1]); 
    Node *StmtList = node->child[2];  
    while(StmtList != NULL && StmtList->child_num > 1){   
        handle_Stmt(StmtList->child[0], correct_type);
        StmtList = StmtList->child[1];
    }
    //delete_stack();
}

void handle_Stmt(Node *node, Type *correct_type){
    node_type_check(node, "Stmt");
    if(node->child[1] != NULL && node->child[1]->type == LP){
        //IF/WHILE LP EXP RP       
        if(!type_equal(my_int_type, get_exp_type(node->child[2])))
            semantic_error(7, node->line, NULL);
        handle_Stmt(node->child[4], correct_type);
        if(node->child_num == 7){
            handle_Stmt(node->child[6], correct_type);
        }
    }
    else if(node->child[0]->type == RETURN){
        if(!type_equal(correct_type, get_exp_type(node->child[1])))
            semantic_error(8, node->line, NULL);
    }
    else if(strcmp(node->child[0]->name, "CompSt") == 0){
        handle_CompSt(node->child[0], correct_type, 0);
    }
    else if(node->child[1]->type == SEMI){//Exp SEMI
        get_exp_type(node->child[0]);
    }
    else{
        printf("Error, unknown Stmt");
    }
}

void add_var(Node *node){
    if(strcmp(node->name, "ExtDef") == 0){
        handle_ExtDef(node);
    }
    else if(strcmp(node->name, "DefList") == 0 && node->visited == 0){
        handle_DefList(node);
    }
    else if(strcmp(node->name, "Exp") == 0){
        get_exp_type(node);
    }
}

static void tree_search(Node *node){
    if(node == NULL || node->visited == 1) return;
    add_var(node);
    for(int i = 0; i < node->child_num; i++){
        Node *tmp = node->child[i];
        if(node->child[i] != NULL){
            tree_search(node->child[i]);
        }
    }
}
void print_basic_type(int basic){
    if(basic == INT_TYPE){
        printf("int ");
    }
    else if(basic == FLOAT_TYPE){
        printf("float ");
    }
    else{
        printf("error ");
    }
}

void print_type(Type *type){
    Type *x = type;
    if(x->kind == ARRAY){
        while(x->kind == ARRAY){
            printf("%d ", x->u.array.size);
            x = x->u.array.elem;
        }
        print_type(x);
    }
    else if(x->kind == STRUCTURE){
        FieldList *tmp = x->u.structure;
        printf("{ ");
        while(tmp != NULL){
            print_type(tmp->type);
            printf("%s; ", tmp->name);
            tmp = tmp->next;
        }
        printf("} ");
    }
    else if(x->kind == BASIC){
        print_basic_type(x->u.basic);
    }
    else{
        printf("ERROR!\n");
    }
}

void print_var_table(){
    printf("Var table\n ----- -----\n");
    for(int i = 0; i <= HASH_TABLE_SIZE; i++){
        Var_hash_node *tmp = var_hash_table[i];
        while(tmp != NULL){
            printf("%s %d ", tmp->name, tmp->depth);
            Type *x = tmp->type;
            print_type(x);
            printf("\n");
            tmp = tmp->next;
        }
    }
    printf("----- -----\n");
}

void print_func_table(){
    printf("Func table\n ----- -----\n");
    for(int i = 0; i <= HASH_TABLE_SIZE; i++){
        Func_hash_node *tmp = func_hash_table[i];
        while(tmp != NULL){
            print_type(tmp->return_type);
            printf("%s ", tmp->name);
            Type_node *arg = tmp->para_type_list;
            while(arg != NULL){
                print_type(arg->type);
                arg = arg->next;
            }
            printf("\n");
            tmp = tmp->next;
        }
    }
    printf("----- -----\n");
}

void func_table_check(){
     for(int i = 0; i <= HASH_TABLE_SIZE; i++){
        Func_hash_node *tmp = func_hash_table[i];
        while(tmp != NULL){
            if(tmp->whether_def == 0)
                semantic_error(18, tmp->lineno, tmp->name);
            tmp = tmp->next;
        }
    }
}

void func_tabel_init(){
    Type_node *write_para = malloc(sizeof(Type_node));
    write_para->next = NULL;
    write_para->type = new_type(INT_TYPE);
    insert_to_func_table("read", 0, new_type(INT_TYPE), NULL, 1);
    insert_to_func_table("write", 0, new_type(INT_TYPE), write_para, 1);
}

void semantic_func(){
    table_init();
    func_tabel_init();
    tree_search(root);
    //print_var_table();
    //print_func_table();
    func_table_check();
}




