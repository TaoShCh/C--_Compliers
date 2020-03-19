#include <stdio.h>
#include "syntax_tree.h"
extern int yyparse();
extern int yyrestart();
int error_num = 0;
int main(int argc, char **argv){
    if (argc > 1){
        FILE *f = fopen(argv[1], "r");
        if (!f){
            perror(argv[1]);
            return 1;
        }
        yyrestart(f);
        yyparse();
        if(1)
            print_tree();
    }
    return 0;
}