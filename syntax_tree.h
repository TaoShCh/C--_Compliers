#ifndef SYNTAX_TREE
#define SYNTAX_TREE
#define MAX_CHILD 10
typedef struct _Node
{
    int line;
    int visited;
    int code_visited;
    int type;
    char *name;
    int child_num;
    struct _Node *child[MAX_CHILD];
    union Val{
        int i;
        float f;
        char *s;
    }val;
}Node;
Node *root;

Node *new_node(int line, char *nodeName, int type, void *val);
void add_child(Node *fa, Node *ch);
void print_tree();
#endif