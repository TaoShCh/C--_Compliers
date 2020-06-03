#include "intercode.h"
#include "semantic.h"
#include "syntax_tree.h"
#include "syntax.tab.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

CodeList *translate_Exp(Node *Exp, Operand *place);
CodeList *translate_Cond(Node *Exp, Operand *label_true, Operand *label_false);
CodeList *translate_Stmt(Node *Stmt);
CodeList *translate_CompSt(Node *CompSt);
char *translate_Operand(Operand *op);
char *translate_InterCode(InterCode *code);
int get_type_size(Type *type);

void init(){
    intercodes_head = intercodes_tail = NULL;
    var_num = 0;
    label_num = 0;
}

void translate_CodeList(CodeList *cl){
    while(cl != NULL){
        assert(cl->code != NULL);
        printf("%s\n", translate_InterCode(cl->code));
        cl = cl->next;
    }
}

void insert_code(CodeList *code){
    //translate_CodeList(code);
    if(code == NULL){
        return;
    }
    if(intercodes_head == NULL){
        intercodes_head = code;
        CodeList *tmp = code;
        while(tmp->next != NULL){
            tmp = tmp->next;
        }
        intercodes_tail = tmp;
    }
    else{
        code->prev = intercodes_tail;
        intercodes_tail->next = code;
        intercodes_tail = code;
        while(intercodes_tail->next != NULL){
            intercodes_tail = intercodes_tail->next;
        }
    }
}

void print_node(Node *node){
    for(int i = 0; i < node->child_num; i++){
        printf("%s ", node->child[i]->name);
    }
    printf("\n");
}

static void node_type_check(Node *node, char *correct_name){
    if(node == NULL){
        return;
        //printf("Error: %s NULL node\n", correct_name);
    }
    //printf("%s\n", node->name);
    node->code_visited = 1;
    if(strcmp(node->name, correct_name) != 0){
        printf("It is a '%s' Node, not a '%s' Node\n",node->name, correct_name);
    }
}

static Node* get_id_node(Node *Vardec){
    node_type_check(Vardec, "VarDec");
    Node *tmp = Vardec;
    while(tmp->type != ID){
        tmp = tmp->child[0];
    }
    return tmp;
}

Type *get_id_type(Node *ID){
    node_type_check(ID, "ID");
    Var_hash_node *tmp = get_var_hash_node(ID->val.s);
    assert(tmp != NULL);
    return tmp->type;
}

CodeList* join(CodeList *head, CodeList *body){
    if(head == NULL){
        return body;
    }
    CodeList *tmp = head;
    while(tmp->next != NULL){
        tmp = tmp->next;
    }
    tmp->next = body;
    if(body != NULL){
        body->prev = tmp;
    }
    return head;
}

Operand* new_temp(int kind){
    Operand *tmp = malloc(sizeof(Operand));
    tmp->kind = kind;
    tmp->u.var_no = var_num;
    var_num++;
    return tmp;
}

Operand* new_label(){
    Operand *tmp = malloc(sizeof(Operand));
    tmp->kind = LABEL;
    tmp->u.label_no = label_num;
    label_num++;
    return tmp;
}

Operand* new_constant(int val){
    Operand *tmp = malloc(sizeof(Operand));
    tmp->kind = CONSTANT;
    tmp->u.val = val;
    return tmp;
}

Operand* lookup(Node *ID){
    node_type_check(ID, "ID");
    char *name = ID->val.s;
    Var_hash_node *node = get_var_hash_node(name);
    assert(node != NULL);
    if(node->op == NULL){
        node->op = new_temp(VARIABLE);
    }
    return node->op;
}

InterCode* new_InterCode(int kind){
    InterCode* tmp = malloc(sizeof(InterCode));
    tmp->kind = kind;
    return tmp;
}

CodeList* new_CodeList(InterCode *code){
    CodeList *tmp = malloc(sizeof(CodeList));
    tmp->code = code;
    tmp->prev = tmp->next = NULL;
    return tmp;
}

CodeList* new_label_code(Operand *op){
    assert(op->kind == LABEL);
    InterCode *code = new_InterCode(LABEL_I);
    code->u.op = op;
    return new_CodeList(code);
}

CodeList* new_goto_code(Operand *op){
    assert(op->kind == LABEL);
    InterCode *code = new_InterCode(GOTO);
    code->u.op = op;
    return new_CodeList(code);
}

CodeList* new_plus_code(Operand* result, Operand *op1, Operand *op2){
    InterCode *code = new_InterCode(PLUS_I);
    code->u.binop.result = result;
    code->u.binop.op1 = op1;
    code->u.binop.op2 = op2;
    return new_CodeList(code);
}

CodeList* new_assign_code(Operand *l, Operand *r){
    InterCode *code = new_InterCode(ASSIGN);
    code->u.assign.left = l;
    code->u.assign.right = r;
    return new_CodeList(code);
}

CodeList *translate_Args(Node *Args, ArgList **arg_list){
    node_type_check(Args, "Args");
    Operand *t1 = new_temp(VARIABLE);
    if(get_exp_type(Args->child[0])->kind != BASIC){
        t1->kind = ADDRESS;
    }
    CodeList *code1 = translate_Exp(Args->child[0], t1);
    ArgList *new_arg = malloc(sizeof(ArgList));
    new_arg->args = t1;
    new_arg->next = *arg_list;
    *arg_list = new_arg;   
    if(Args->child_num == 1){
        return code1;
    }
    else{
        CodeList *code2 = translate_Args(Args->child[2], arg_list);
        return join(code1, code2);
    }
}

int get_array_size(Node *Exp){
    assert(Exp->child[1]->type == LB);
    Type *type = get_exp_type(Exp->child[0]);
    return get_type_size(type->u.array.elem);
}

int get_struct_offset(Type *s, char *id){
    assert(s->kind == STRUCTURE);
    int ans = 0;
    FieldList *fl = s->u.structure;
    while(strcmp(fl->name, id) != 0){
        ans += get_type_size(fl->type);
        fl = fl->next;
    }
    return ans;
}

CodeList* translate_Exp(Node *Exp, Operand *place){
    node_type_check(Exp, "Exp");
    if(Exp->child_num >= 2 && (Exp->child[0]->type == NOT || Exp->child[1]->type == RELOP || Exp->child[1]->type == AND || Exp->child[1]->type == OR)){
        Operand *l1 = new_label();
        Operand *l2 = new_label();
        CodeList *code0 = NULL, *code1 = NULL, *code2 = NULL, *code3 = NULL;
        InterCode *code = NULL;
        if(place != NULL){
            code = new_InterCode(ASSIGN);
            code->u.assign.left = place;
            code->u.assign.right = new_constant(0);
            code0 = new_CodeList(code);
        }
        code1 = translate_Cond(Exp, l1, l2);
        code2 = new_label_code(l1);
        if(place != NULL){
            code = new_InterCode(ASSIGN);
            code->u.assign.left = place;
            code->u.assign.right = new_constant(1);
            code2 = join(code2, new_CodeList(code));
        }
        code3 = new_label_code(l2);
        return join(join(code0, code1), join(code2, code3));
    }
    else if(Exp->child_num == 1){
        if(place == NULL){
            return NULL;
        }
        else{
            if(Exp->child[0]->type == INT){
                int val = Exp->child[0]->val.i;
                InterCode *new_code = new_InterCode(ASSIGN);
                new_code->u.assign.left = place;
                new_code->u.assign.right = new_constant(val);
                return new_CodeList(new_code);
            }
            else if(Exp->child[0]->type == ID){
                Operand *op = lookup(Exp->child[0]);
                InterCode *code = new_InterCode(ASSIGN);
                code->u.assign.left = place;
                code->u.assign.right = op;
                if(get_id_type(Exp->child[0])->kind != BASIC && place->kind != ARR_STRU){
                    place->kind = ADDRESS;
                }
                return new_CodeList(code);
            }
        }
    }
    else if(Exp->child_num == 2){
        if(Exp->child[0]->type == MINUS){
            Operand *t1 = new_temp(VARIABLE);
            CodeList *code1 = translate_Exp(Exp->child[1], t1);
            CodeList *code2 = NULL;
            if(place != NULL){
                InterCode *code = new_InterCode(MINUS_I);
                code->u.binop.result = place;
                code->u.binop.op1 = new_constant(0);
                code->u.binop.op2 = t1;
                code2 = new_CodeList(code);
            }
            return join(code1, code2);
        }
    }
    else if(Exp->child_num == 3){
        if(Exp->child[1]->type == PLUS || Exp->child[1]->type == MINUS || Exp->child[1]->type == DIV || Exp->child[1]->type == STAR){
            Operand *t1 = new_temp(VARIABLE);
            Operand *t2 = new_temp(VARIABLE);
            CodeList *code1 = translate_Exp(Exp->child[0], t1);
            CodeList *code2 = translate_Exp(Exp->child[2], t2);
            CodeList *code3 = NULL;
            if(place != NULL){
                InterCode *code = malloc(sizeof(InterCode));
                code->u.binop.result = place;
                code->u.binop.op1 = t1;
                code->u.binop.op2 = t2;
                if(Exp->child[1]->type == PLUS){
                    code->kind = PLUS_I;
                }
                else if(Exp->child[1]->type == MINUS){
                    code->kind = MINUS_I;
                }
                else if(Exp->child[1]->type == DIV){
                    code->kind = DIV_I;
                }
                else if(Exp->child[1]->type == STAR){
                    code->kind = MUL_I;
                }
                code3 = new_CodeList(code);
            }
            return join(join(code1, code2), code3);
        }
        else if(Exp->child[1]->type == ASSIGNOP){
            if(Exp->child[0]->child_num == 1){
                Operand *op = lookup(Exp->child[0]->child[0]);
                Operand *t1 = NULL;
                t1 = new_temp(VARIABLE);
                CodeList *code1 = translate_Exp(Exp->child[2], t1);
                CodeList *code2 = NULL;
                InterCode *code = NULL;
                if(op->kind == ARR_STRU){
                    Operand *addr1 = new_temp(ADDRESS);
                    Operand *addr2 = new_temp(ADDRESS);
                    CodeList *codex = new_assign_code(addr1, op);
                    CodeList *codey = new_assign_code(addr2, t1);
                    code2 = join(code2, join(codex, codey));//join的功能是将两段代码连接起来
                    Node *id = Exp->child[0]->child[0];
                    Type *type = get_id_type(id);
                    int n = get_type_size(type);
                    for(int i = 0; i < n; i += 4){
                        Operand *addrA = new_temp(ADDRESS);
                        Operand *addrB = new_temp(ADDRESS); 
                        CodeList *c1 = new_plus_code(addrA, addr1, new_constant(i));
                        CodeList *c2 = new_plus_code(addrB, addr2, new_constant(i));
                        code2 = join(code2, join(c1, c2));
                        Operand *tmp = new_temp(VARIABLE);
                        CodeList *c3 = new_assign_code(tmp, addrB);
                        CodeList *c4 = new_assign_code(addrA, tmp);
                        c4->code->kind = CHANGE_ADDR;
                        code2 = join(code2, join(c3, c4));
                    }
                }
                else{
                    code = new_InterCode(ASSIGN);
                    code->u.assign.left = op;
                    code->u.assign.right = t1;
                    code2 = new_CodeList(code);
                }
                if(place != NULL){
                    code = new_InterCode(ASSIGN);
                    code->u.assign.left = place;
                    code->u.assign.right = op;
                    code2 = join(code2, new_CodeList(code));
                }
                //translate_CodeList(code1);
                //translate_CodeList(code2);
                //printf("\n");
                return join(code1, code2);
            }
            else{
                Operand *t1 = new_temp(VARIABLE);
                Operand *op = new_temp(ADDRESS);
                CodeList *code0 = translate_Exp(Exp->child[0], op);
                CodeList *code1 = translate_Exp(Exp->child[2], t1);
                InterCode *code = new_InterCode(CHANGE_ADDR);
                code->u.assign.left = op;
                code->u.assign.right = t1;
                CodeList *code2 = new_CodeList(code);
                if(place != NULL){
                    code = new_InterCode(ASSIGN);
                    code->u.assign.left = place;
                    code->u.assign.right = t1;
                    code2 = join(code2, new_CodeList(code));
                }
                return join(code0, join(code1, code2));
            }
        }
        else if(Exp->child[0]->type == LP){
            return translate_Exp(Exp->child[1], place);
        }
        else if(Exp->child[1]->type == LP){
            char *name = Exp->child[0]->val.s;
            if(strcmp(name, "read") == 0){
                if(place == NULL){
                    return NULL;
                }
                InterCode *code = new_InterCode(READ);
                code->u.op = place;
                return new_CodeList(code);
            }
            else{
                InterCode *code = new_InterCode(CALL);
                if(place != NULL){
                    code->u.call.result = place;
                }
                else code->u.call.result = new_temp(VARIABLE);
                code->u.call.func = name;
                return new_CodeList(code);
            }
        }
        else if(Exp->child[1]->type == DOT){
            if(place == NULL)
                return NULL;
            Type *type = get_exp_type(Exp->child[0]);
            Operand *t1 = new_temp(ADDRESS);
            CodeList *code1 = translate_Exp(Exp->child[0], t1);
            int offset = get_struct_offset(type, Exp->child[2]->val.s);
            InterCode *code = new_InterCode(PLUS_I);
            code->u.binop.result = place;
            code->u.binop.op1 = t1;
            code->u.binop.op2 = new_constant(offset);
            CodeList *code2 = new_CodeList(code);
            if(place->kind == VARIABLE){
                Operand *t2 = new_temp(ADDRESS);
                code->u.binop.result = t2;
                code = new_InterCode(ASSIGN);
                code->u.assign.left = place;
                code->u.assign.right = t2;
                code2 = join(code2, new_CodeList(code));
            }
            return join(code1, code2);
        }
    }
    else if(Exp->child_num == 4){
        if(Exp->child[1]->type == LP){
            char *name = Exp->child[0]->val.s;
            ArgList *arg_list = NULL;
            CodeList *code1 = translate_Args(Exp->child[2], &arg_list);
            if(strcmp(name, "write") == 0){
                InterCode *code = new_InterCode(WRITE);
                code->u.op = arg_list->args;
                CodeList *code2 = new_CodeList(code);
                CodeList *code3 = NULL;
                if(place != NULL){
                    code = new_InterCode(ASSIGN);
                    code->u.assign.left = place;
                    code->u.assign.right = new_constant(0);
                    code3 = new_CodeList(code);
                }
                return join(code1, join(code2, code3));
            }
            else{
                CodeList *code2 = NULL;
                while(arg_list != NULL){
                    InterCode *code = new_InterCode(ARG);
                    code->u.op = arg_list->args;
                    code2 = join(code2, new_CodeList(code));
                    arg_list = arg_list->next;
                }
                InterCode *code = new_InterCode(CALL);
                if(place != NULL){
                    code->u.call.result = place;
                }
                else code->u.call.result = new_temp(VARIABLE);
                code->u.call.func = name;
                CodeList *code3 = new_CodeList(code);
                return join(code1, join(code2, code3));
            }
        }
        else if(Exp->child[1]->type == LB){
            Operand *t1 = new_temp(ADDRESS);
            Operand *t2 = new_temp(VARIABLE);
            Operand *t3 = new_temp(VARIABLE);    
            CodeList *code1 = translate_Exp(Exp->child[0], t1);
            CodeList *code2 = translate_Exp(Exp->child[2], t2);
            InterCode *code = new_InterCode(MUL_I);
            code->u.binop.result = t3;
            code->u.binop.op1 = t2;
            code->u.binop.op2 = new_constant(get_array_size(Exp));
            CodeList *code3 = new_CodeList(code);
            CodeList *code4 = NULL;
            if(place != NULL){
                code = new_InterCode(PLUS_I);
                code->u.binop.result = place;
                code->u.binop.op1 = t1;
                code->u.binop.op2 = t3;
                code4 = new_CodeList(code);
                if(place->kind == VARIABLE){
                    Operand *t4 = new_temp(ADDRESS);
                    code->u.binop.result = t4;
                    code = new_InterCode(ASSIGN);
                    code->u.assign.left = place;
                    code->u.assign.right = t4;
                    code4 = join(code4, new_CodeList(code));
                }
            }
            return join(join(code1, code2), join(code3, code4));
        }
    }
    assert(0);
    return NULL;
}

CodeList *translate_Cond(Node *Exp, Operand *label_true, Operand *label_false){
    if(Exp->child[0]->type == NOT){
        return translate_Cond(Exp->child[1], label_false, label_true);
    }
    else if(Exp->child_num == 3){
        if(Exp->child[1]->type == RELOP){
            Operand *t1 = new_temp(VARIABLE);
            Operand *t2 = new_temp(VARIABLE);
            CodeList *code1 = translate_Exp(Exp->child[0], t1);
            CodeList *code2 = translate_Exp(Exp->child[2], t2);
            char *op = Exp->child[1]->val.s;
            InterCode *code = new_InterCode(IF_GOTO);
            code->u.if_goto.relop = op;
            code->u.if_goto.x = t1;
            code->u.if_goto.y = t2;
            code->u.if_goto.z = label_true;
            CodeList *code3 = new_CodeList(code);
            CodeList *code4 = new_goto_code(label_false);
            return join(code1, join(code2, join(code3, code4)));
        }
        else if(Exp->child[1]->type == AND){
            Operand *l1 = new_label();
            CodeList *code1 = translate_Cond(Exp->child[0], l1, label_false);
            CodeList *code2 = translate_Cond(Exp->child[2], label_true, label_false);
            CodeList *code1x = new_label_code(l1);
            return join(code1, join(code1x, code2));
        }
        else if(Exp->child[1]->type == OR){
            Operand *l1 = new_label();
            CodeList *code1 = translate_Cond(Exp->child[0], label_true, l1);
            CodeList *code2 = translate_Cond(Exp->child[2], label_true, label_false);
            CodeList *code1x = new_label_code(l1);
            return join(code1, join(code1x, code2));
        }
    }
    Operand *t1 = new_temp(VARIABLE);
    CodeList *code1 = translate_Exp(Exp, t1);
    InterCode *code = new_InterCode(IF_GOTO);
    code->u.if_goto.x = t1;
    code->u.if_goto.y = new_constant(0);
    code->u.if_goto.z = label_true;
    code->u.if_goto.relop = malloc(3);
    strcpy(code->u.if_goto.relop, "!=");
    CodeList *code2 = new_CodeList(code);
    CodeList *code3 = new_goto_code(label_false);
    return join(code1, join(code2, code3));
}

int get_type_size(Type *type){
    if(type->kind == BASIC){
        return 4;
    }
    else if(type->kind == ARRAY){
        return type->u.array.size * get_type_size(type->u.array.elem);
    }
    else if(type->kind == STRUCTURE){
        int ans = 0;
        FieldList *tmp = type->u.structure;
        while(tmp != NULL){
            ans += get_type_size(tmp->type);
            tmp = tmp->next;
        }
        return ans;
    }
}

CodeList *translate_Dec(Node *Dec){
    node_type_check(Dec, "Dec");
    Node *VarDec = Dec->child[0];
    if(Dec->child_num > 1){
        if(VarDec->child[0]->type == ID){
            Operand *op = lookup(VarDec->child[0]);
            return translate_Exp(Dec->child[2], op);
        }
    }
    else{
        Node *ID = get_id_node(VarDec);
        Var_hash_node *tmp = get_var_hash_node(ID->val.s);
        if(tmp->type->kind == ARRAY || tmp->type->kind == STRUCTURE){
            Operand *op = new_temp(ARR_STRU);
            int size = get_type_size(tmp->type);
            InterCode *code = new_InterCode(DEC);
            code->u.dec.size = size;
            code->u.dec.x = op;
            tmp->op = op;
            return new_CodeList(code);
        }
    }
}


CodeList *translate_DefList(Node *DefList){
    //TODO
    node_type_check(DefList, "DefList");
    CodeList *ans = NULL;
    while(DefList != NULL){
        Node *Def = DefList->child[0];
        Node *DecList = Def->child[1];
        while(1){
            Node *Dec = DecList->child[0];
            ans = join(ans, translate_Dec(Dec));
            if(DecList->child_num > 1){
                DecList = DecList->child[2];
            }
            else break;
        }
        DefList = DefList->child[1];
    }
    return ans;
}

CodeList *translate_CompSt(Node *CompSt){
    node_type_check(CompSt, "CompSt");
    CodeList *code1 = translate_DefList(CompSt->child[1]);
    CodeList *code2 = NULL;
    Node *StmtList = CompSt->child[2];
    while(StmtList != NULL){
        code2 = join(code2, translate_Stmt(StmtList->child[0]));
        StmtList = StmtList->child[1];
    }
    return join(code1, code2);
}

CodeList *translate_Stmt(Node *Stmt){
    node_type_check(Stmt, "Stmt");
    if(Stmt->child_num == 1){
        //CompSt
        return translate_CompSt(Stmt->child[0]);
    }
    else if(Stmt->child_num == 2){
        //Exp SEMI
        return translate_Exp(Stmt->child[0], NULL);
    }
    else if(Stmt->child_num == 3){
        //RETURN EXP SEMI
        Operand *t1 = new_temp(VARIABLE);
        CodeList *code1 = translate_Exp(Stmt->child[1], t1);
        InterCode *code = new_InterCode(RETURN_I);
        code->u.op = t1;
        CodeList *code2 = new_CodeList(code);
        return join(code1, code2);
    }
    else if(Stmt->child_num == 7){
        //IF ELSE
        Operand *l1 = new_label();
        Operand *l2 = new_label();
        Operand *l3 = new_label();
        CodeList *code1 = translate_Cond(Stmt->child[2], l1, l2);
        CodeList *code2 = translate_Stmt(Stmt->child[4]);
        CodeList *code3 = translate_Stmt(Stmt->child[6]);
        CodeList *code1x = new_label_code(l1);
        CodeList *code2x1 = new_goto_code(l3);
        CodeList *code2x2 = new_label_code(l2);
        CodeList *code4 = new_label_code(l3);
        CodeList *tmp1 = join(code1, join(code1x, code2));
        CodeList *tmp2 = join(join(code2x1, code2x2), join(code3, code4));
        return join(tmp1, tmp2);
    }
    else if(Stmt->child[0]->type == IF && Stmt->child_num == 5){
        //IF
        Operand *l1 = new_label();
        Operand *l2 = new_label();
        CodeList *code1 = translate_Cond(Stmt->child[2], l1, l2);
        CodeList *code2 = translate_Stmt(Stmt->child[4]);
        CodeList *code1x = new_label_code(l1);
        CodeList *code2x = new_label_code(l2);
        return join(join(code1, code1x), join(code2, code2x));
    }
    else if(Stmt->child[0]->type == WHILE){
        //WHILE
        Operand *l1 = new_label();
        Operand *l2 = new_label();
        Operand *l3 = new_label();
        CodeList *code1 = translate_Cond(Stmt->child[2], l2, l3);
        CodeList *code2 = translate_Stmt(Stmt->child[4]);
        CodeList *code0 = new_label_code(l1);
        CodeList *code1x = new_label_code(l2);
        CodeList *code3 = new_goto_code(l1);
        CodeList *code4 = new_label_code(l3);
        CodeList *tmp1 = join(code0, join(code1, code1x));
        CodeList *tmp2 = join(code2, join(code3, code4));
        return join(tmp1, tmp2);
    }
    assert(0);
}

CodeList *translate_FunDec(Node *FunDec){
    node_type_check(FunDec, "FunDec");
    char *name = FunDec->child[0]->val.s;
    Func_hash_node *func = get_func_hash_node(name);
    assert(func != NULL);
    Type_node *para = func->para_type_list;
    InterCode *fundec = new_InterCode(FUNC);
    fundec->u.func = FunDec->child[0]->val.s;
    CodeList *code1 = new_CodeList(fundec);
    while(para != NULL){
        Operand *op = NULL;
        InterCode *code = new_InterCode(PARAM);
        if(para->type->kind == BASIC){
            op = new_temp(VARIABLE);
        }
        else if(para->type->kind == ARRAY || para->type->kind == STRUCTURE){
            op = new_temp(ADDRESS);
        }
        code->u.op = op;
        Var_hash_node *var = get_var_hash_node(para->name);
        var->op = op;
        code1 = join(code1, new_CodeList(code));
        para = para->next;
    }
    return code1;
}

CodeList *translate_ExtDef(Node *ExtDef){
    node_type_check(ExtDef, "ExtDef");
    if(ExtDef->child_num == 3){
        CodeList *code1 = translate_FunDec(ExtDef->child[1]);
        CodeList *code2 = translate_CompSt(ExtDef->child[2]);
        return join(code1, code2);
    }
    return NULL;
}

char *translate_Operand(Operand *op){
    char msg[100] = "";
    if(op->kind == CONSTANT){
        sprintf(msg, "#%d", op->u.val);
    }
    else if(op->kind == LABEL){
        sprintf(msg, "l%d", op->u.label_no);
    }
    else{
        sprintf(msg, "t%d", op->u.var_no);
    }

    char *ans = malloc(strlen(msg) + 1);
    strcpy(ans, msg);
    return ans;
}

char* translate_InterCode(InterCode *code){
    char *msg = malloc(MAX_STR);
    memset(msg, 0, MAX_STR);
    if(code->kind == LABEL_I){
        char *x = translate_Operand(code->u.op);
        sprintf(msg, "LABEL %s :", x);
    }
    else if(code->kind == FUNC){
        char *f = code->u.func;
        sprintf(msg, "FUNCTION %s :", f);
    }
    else if(code->kind == ASSIGN){
        Operand *left = code->u.assign.left;
        Operand *right = code->u.assign.right;
        if(code->u.assign.left != NULL){
            char *x = translate_Operand(left);
            char *y = translate_Operand(right);
            if(left->kind == ADDRESS && right->kind != ADDRESS){
                sprintf(msg, "%s := &%s", x, y);
            }
            else if(right->kind == ADDRESS && left->kind != ADDRESS){
                sprintf(msg, "%s := *%s", x, y);
            }
            else{
                sprintf(msg, "%s := %s", x, y);
            }
        }
    }
    else if(code->kind == CHANGE_ADDR){
        if(code->u.assign.left != NULL){
            char *x = translate_Operand(code->u.assign.left);
            char *y = translate_Operand(code->u.assign.right);
            sprintf(msg, "*%s := %s", x, y);
        }
    }
    else if(code->kind == PLUS_I){
        if(code->u.binop.result != NULL){
            char *x = translate_Operand(code->u.binop.result);
            char *y = translate_Operand(code->u.binop.op1);
            char *z = translate_Operand(code->u.binop.op2);
            sprintf(msg, "%s := %s + %s",x, y, z);
        }
    }
    else if(code->kind == MINUS_I){
        if(code->u.binop.result != NULL){
            char *x = translate_Operand(code->u.binop.result);
            char *y = translate_Operand(code->u.binop.op1);
            char *z = translate_Operand(code->u.binop.op2);
            sprintf(msg, "%s := %s - %s",x, y, z);
        }
    }
    else if(code->kind == MUL_I){
        if(code->u.binop.result != NULL){
            char *x = translate_Operand(code->u.binop.result);
            char *y = translate_Operand(code->u.binop.op1);
            char *z = translate_Operand(code->u.binop.op2);
            sprintf(msg, "%s := %s * %s",x, y, z);
        }
    }
    else if(code->kind == DIV_I){
        if(code->u.binop.result != NULL){
            char *x = translate_Operand(code->u.binop.result);
            char *y = translate_Operand(code->u.binop.op1);
            char *z = translate_Operand(code->u.binop.op2);
            sprintf(msg, "%s := %s / %s",x, y, z);
        }
    }
    else if(code->kind == GOTO){
        char *x = translate_Operand(code->u.op);
        sprintf(msg, "GOTO %s", x);
    }
    else if(code->kind == IF_GOTO){
        char *x = translate_Operand(code->u.if_goto.x);
        char *y = translate_Operand(code->u.if_goto.y);
        char *z = translate_Operand(code->u.if_goto.z);
        sprintf(msg, "IF %s %s %s GOTO %s", x, code->u.if_goto.relop, y, z);
    }
    else if(code->kind == RETURN_I){
        char *x = translate_Operand(code->u.op);
        sprintf(msg, "RETURN %s", x);
    }
    else if(code->kind == DEC){
        char *x = translate_Operand(code->u.dec.x);
        sprintf(msg, "DEC %s %d", x, code->u.dec.size);
    }
    else if(code->kind == ARG){
        char *x = translate_Operand(code->u.op);
        if(code->u.op->kind == ADDRESS)
            sprintf(msg, "ARG %s", x);
        sprintf(msg, "ARG %s", x);
    }
    else if(code->kind == CALL){
        char *x = translate_Operand(code->u.call.result);
        char *f = code->u.call.func;
        sprintf(msg, "%s := CALL %s", x, f);
    }
    else if(code->kind == PARAM){
        char *x = translate_Operand(code->u.op);
        sprintf(msg, "PARAM %s", x);
    }
    else if(code->kind == READ){
        char *x = translate_Operand(code->u.op);
        sprintf(msg, "READ %s", x);
    }
    else if(code->kind == WRITE){
        char *x = translate_Operand(code->u.op);
        sprintf(msg, "WRITE %s", x);
    }
    char *ans = malloc(strlen(msg) + 1);
    strcpy(ans, msg);
    return ans;
}

void output(char *file){
    FILE *f;
    if(file != NULL){
        f = fopen(file, "w+");
    }
    else f = fopen("output.ir", "w+");
    if(!f){
        perror(file);
        return;
    }
    CodeList *tmp = intercodes_head;
    while(tmp != NULL){
        char *msg = translate_InterCode(tmp->code);
        if(strlen(msg) > 0)
            fprintf(f, "%s\n", translate_InterCode(tmp->code));
        tmp = tmp->next;
    }
    fclose(f);
}

void handle_node(Node *node){
    if(strcmp(node->name, "ExtDef") == 0){
        insert_code(translate_ExtDef(node));
    }

}

void tree_search(Node *node){
    if(node == NULL || node->code_visited == 1) return;
    handle_node(node);
    for(int i = 0; i < node->child_num; i++){
        Node *tmp = node->child[i];
        if(node->child[i] != NULL){
            tree_search(node->child[i]);
        }
    }
}

void InterCode_function(char *file){
    tree_search(root);
    output(file);
}