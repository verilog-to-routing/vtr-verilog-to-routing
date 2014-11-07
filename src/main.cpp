#include <stdio.h>

extern int yyparse();
extern FILE *yyin;

int main(int argc, char** argv) {
    if(argc != 2) {
        printf("Usage: %s tg_echo_file\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if(yyin != NULL) {
        int error = yyparse();
        if(error) {
            printf("Parse Error\n");
            fclose(yyin);
            return 1;
        }
        fclose(yyin);
    } else {
        printf("Could not open file %s\n", argv[1]);
        return 1;
    }
    return 0;
}
