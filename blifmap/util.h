#include <stdio.h>
#include <stdlib.h>
FILE *my_open (char *fname, char *flag, int prompt);
void *my_calloc (size_t nelem, size_t size); 
void *my_malloc (size_t size);
void *my_small_malloc (size_t size);
void *my_realloc (void *ptr, size_t size);
char *my_strtok(char *ptr, char *tokens, FILE *fp, char *buf);
char *my_fgets(char *buf, int max_size, FILE *fp); 
void **alloc_matrix (int nrmin, int nrmax, int ncmin, int ncmax, 
   size_t elsize);
void ***alloc_matrix3 (int nrmin, int nrmax, int ncmin, int ncmax,
   int ndmin, int ndmax, size_t elsize); 
void free_matrix (void **vptr, int nrmin, int nrmax, int ncmin,
   size_t elsize);
void free_matrix3 (void ***vptr, int nrmin, int nrmax, int ncmin,
   int ncmax, int ndmin, size_t elsize);
void my_srandom (int seed);
int my_irand (int imax);
float my_frand (void);

#define BUFSIZE 300     /* Maximum line length for various parsing proc. */
extern int linenum;  /* line in file being parsed */
typedef enum {FALSE, TRUE} boolean;
