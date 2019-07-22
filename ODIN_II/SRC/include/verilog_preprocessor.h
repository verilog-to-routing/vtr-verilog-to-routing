#ifndef VERILOG_PREPROCESSOR_H
#define VERILOG_PREPROCESSOR_H

#include <stdio.h>

int init_veri_preproc();
int cleanup_veri_preproc();
FILE* veri_preproc(FILE *source);

#endif
