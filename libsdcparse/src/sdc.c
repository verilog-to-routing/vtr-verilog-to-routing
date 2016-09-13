#include <stdio.h>

#include "sdc.h"
#include "sdc_common.h"

extern int yyparse();
extern FILE	*yyin;

/*
 * Given a filename parses the file as an SDC file
 * and returns a pointer to a struct containing all
 * the sdc commands.  See sdc.h for data structure
 * detials.
 */
t_sdc_commands* sdc_parse_filename(char* filename) {
    yyin = fopen(filename, "r");
    if(yyin != NULL) {
        int error = yyparse();
        if(error) {
            sdc_error(0, "", "File %s failed to parse.\n", filename);
        }
        fclose(yyin);
    } else {
        sdc_error(0, "", "Could not open file %s.\n", filename);
    }


    return g_sdc_commands;
}

t_sdc_commands* sdc_parse_file(FILE* sdc_file) {
    yyin = sdc_file;

    int error = yyparse();
    if(error) {
        sdc_error(0, "", "SDC Error: file failed to parse!\n");
    }

    return g_sdc_commands;
}

void sdc_parse_cleanup() {
    free_sdc_commands(g_sdc_commands);
}
