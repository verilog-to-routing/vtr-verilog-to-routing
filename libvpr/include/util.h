#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef TRUE                    /* Some compilers predefine TRUE, FALSE */
typedef enum
{ FALSE, TRUE }
boolean;
#else
typedef int boolean;
#endif

/* Parameter tags for preprocessor to strip */
#define INP
#define OUTP
#define INOUTP

#define ERRTAG "ERROR:\t"
#define WARNTAG "WARN:\t"

extern int linenum;	/* line in file being parsed */


#define BUFSIZE 65536 /*  Increased for MCML.blif	Maximum line length for various parsing proc. */
#ifndef max
#define max(a,b) (((a) > (b))? (a) : (b))
#define min(a,b) ((a) > (b)? (b) : (a))
#endif
#define nint(a) ((int) floor (a + 0.5))

extern int limit_value(int cur, int max, const char *name);

/* Linked lists of void pointers and integers, respectively. */

struct s_linked_vptr
{
    void *data_vptr;
    struct s_linked_vptr *next;
};

typedef struct s_linked_int
{
    int data;
    struct s_linked_int *next;
} t_linked_int;

/* Integer vector.  nelem stores length, list[0..nelem-1] stores list of    *
 * integers.                                                                */

typedef struct s_ivec
{
    int nelem;
    int *list;
} t_ivec;


/************************ Memory allocation routines *************************/

extern void* my_malloc(size_t size);
extern void* my_calloc(size_t nelem, size_t size);
extern void *my_realloc(void *ptr, size_t size);
extern void *my_chunk_malloc(size_t size, struct s_linked_vptr **chunk_ptr_head,
	int *mem_avail_ptr, char **next_mem_loc_ptr);
extern void free_chunk_memory(struct s_linked_vptr *chunk_ptr_head);

/******************* Linked list, matrix and vector utilities ****************/

extern void free_ivec_vector(struct s_ivec *ivec_vector, int nrmin, int nrmax);
extern void free_ivec_matrix(struct s_ivec **ivec_matrix, int nrmin,
	int nrmax, int ncmin, int ncmax);
extern void free_ivec_matrix3(struct s_ivec ***ivec_matrix3, int nrmin,
	int nrmax, int ncmin, int ncmax, int ndmin, int ndmax);

extern void** alloc_matrix(int nrmin, int nrmax, int ncmin, int ncmax,
	size_t elsize);
extern void ***alloc_matrix3(int nrmin, int nrmax, int ncmin, int ncmax,
	int ndmin, int ndmax, size_t elsize);
extern void ****alloc_matrix4(int nrmin, int nrmax, int ncmin, int ncmax,
	int ndmin, int ndmax, int nemin, int nemax, size_t elsize);

extern void free_matrix(void *vptr, int nrmin, int nrmax, int ncmin,
	size_t elsize);
extern void free_matrix3(void *vptr, int nrmin, int nrmax, int ncmin,
	int ncmax, int ndmin, size_t elsize);
extern void free_matrix4(void *vptr, int nrmin, int nrmax, int ncmin,
	int ncmax, int ndmin, int ndmax, int nemin, size_t elsize);

extern void print_int_matrix3(int ***vptr, int nrmin, int nrmax, int ncmin,
	int ncmax, int ndmin, int ndmax, char *file);

extern struct s_linked_vptr *insert_in_vptr_list(struct s_linked_vptr *head,
	void *vptr_to_add);
extern struct s_linked_vptr *delete_in_vptr_list(struct s_linked_vptr *head);
extern t_linked_int *insert_in_int_list(t_linked_int * head,
	int data, t_linked_int ** free_list_head_ptr);
extern void free_int_list(t_linked_int ** int_list_head_ptr);
extern void alloc_ivector_and_copy_int_list(t_linked_int ** list_head_ptr,
	int num_items, struct s_ivec *ivec, t_linked_int ** free_list_head_ptr);

/****************** File and parsing utilities *******************************/

extern int my_atoi(const char *str);

extern char* my_strdup(const char *str);
extern char *my_strncpy(char *dest, const char *src, size_t size);
extern char *my_strtok(char *ptr, char *tokens, FILE * fp, char *buf);

extern FILE* my_fopen(const char *fname, const char *flag, int prompt);
extern char *my_fgets(char *buf, int max_size, FILE * fp);

/*********************** Portable random number generators *******************/
extern void my_srandom(int seed);
extern int my_irand(int imax);
extern float my_frand(void);

#endif

