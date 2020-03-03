#ifndef OUTPUT_BLIF_H
#define OUTPUT_BLIF_H

#include "odin_types.h"
#include <stdlib.h>

FILE* create_blif(const char* file_name);
void output_blif(FILE* out, netlist_t* netlist);

#endif
