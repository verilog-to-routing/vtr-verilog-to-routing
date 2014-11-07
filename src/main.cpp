#include <stdio.h>
#include "TimingGraph.hpp"

extern int yyparse(TimingGraph& tg);
extern FILE *yyin;

int main(int argc, char** argv) {
    if(argc != 2) {
        printf("Usage: %s tg_echo_file\n", argv[0]);
        return 1;
    }

    TimingGraph timing_graph;

    yyin = fopen(argv[1], "r");
    if(yyin != NULL) {
        int error = yyparse(timing_graph);
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

    timing_graph.print();

    return 0;
}
