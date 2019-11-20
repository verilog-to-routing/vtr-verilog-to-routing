#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 300     /* Maximum line length for various parsing proc. */
extern int linenum;  /* line in file being parsed */
typedef enum {FALSE, TRUE} boolean;
struct s_linked_vptr {void *data_vptr; struct s_linked_vptr *next;};

#define max(a,b) (((a) > (b))? (a) : (b))
#define min(a,b) ((a) > (b)? (b) : (a))

FILE *my_fopen (char *fname, char *flag, int prompt);
void *my_calloc (size_t nelem, size_t size); 
void *my_malloc (size_t size);
void *my_realloc (void *ptr, size_t size);
void *my_chunk_malloc (size_t size, struct s_linked_vptr **chunk_ptr_head,
       int *mem_avail_ptr, char **next_mem_loc_ptr); 
void free_chunk_memory (struct s_linked_vptr *chunk_ptr_head); 

struct s_linked_vptr *insert_in_vptr_list (struct s_linked_vptr *head,
       void *vptr_to_add); 
char *my_strtok(char *ptr, char *tokens, FILE *fp, char *buf);
char *my_fgets(char *buf, int max_size, FILE *fp); 

void **alloc_matrix (int nrmin, int nrmax, int ncmin, int ncmax, 
      size_t elsize);
void ***alloc_matrix3 (int nrmin, int nrmax, int ncmin, int ncmax, int ndmin,
      int ndmax, size_t elsize); 
void free_matrix (void *vptr, int nrmin, int nrmax, int ncmin, size_t elsize);
void free_matrix3 (void *vptr, int nrmin, int nrmax, int ncmin, int ncmax,
      int ndmin, size_t elsize);

void my_srandom (int seed);
int my_irand (int imax);
float my_frand (void);
