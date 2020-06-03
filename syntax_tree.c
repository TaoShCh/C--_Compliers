#include "syntax_tree.h"
#include "syntax.tab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int space_num = 0;


Node *new_node(int line, char *nodeName, int type, void *val){
    Node *node = malloc(sizeof(Node));
    node->line = line;
    node->child_num = 0;
    node->type = type;
    node->visited = node->code_visited = 0;
    node->name = malloc(strlen(nodeName) + 1);
    strcpy(node->name, nodeName);
    for(int i = 0; i < MAX_CHILD; i++){
        node->child[i] = NULL;
    }
    if(type == INT){
        node->val.i = *(int *)val;
    }
    else if(type == FLOAT){
        node->val.f = *(float *)val;
    }
    else if(type == ID || type == TYPE || type == RELOP){
        node->val.s = malloc(strlen(val) + 1);
        strcpy(node->val.s, val);
    }
    else{
        node->val.i = 0;
    }
    return node;
}

void add_child(Node *fa, Node *ch){
    fa->child[fa->child_num] = ch;
    fa->child_num++;
}

void print_space(int n){
    for(int i = 0; i < n; i++){
        printf(" ");
    }
}

void pre_order(Node *t){
    if(t == NULL) 
        return;
    print_space(space_num);
    if(t->type == 0){
        printf("%s (%d)\n", t->name, t->line);
    }
    else if(t->type == INT){
        printf("%s: %u\n", t->name, t->val.i);
    }
    else if(t->type == FLOAT){
        printf("%s: %f\n", t->name, t->val.f);
    }
    else if(t->type == ID || t->type == TYPE){
        printf("%s: %s\n", t->name, t->val.s);
    }
    else{
        printf("%s\n", t->name);
    }    
    space_num += 2;
    for(int i = 0; i < t->child_num; i++){
        pre_order(t->child[i]);
    }
    space_num -= 2;
}

void print_tree(){
    space_num = 0;
    pre_order(root);
}