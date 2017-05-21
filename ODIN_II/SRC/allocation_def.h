#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define ALLOCATE_ME(num, type)	((type *) calloc((num),sizeof(type)))