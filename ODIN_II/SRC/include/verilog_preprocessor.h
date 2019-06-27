#ifndef verilog_preprocessor_h
#define verilog_preprocessor_h

#include <stdio.h>

int init_veri_preproc();
int cleanup_veri_preproc();
FILE* veri_preproc(FILE *source);

#endif
