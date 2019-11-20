#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*************** Global variables exported by this module ********************/

extern int linenum;		/* line in file being parsed */


/******************* Types and defines exported by this module ***************/

#ifndef TRUE			/* Some compilers predefine TRUE, FALSE */
typedef enum
{ FALSE, TRUE }
boolean;
#else
typedef int boolean;
#endif

/* Parameter tags for preprocessor to strip */
#define IN
#define OUT
#define INOUT


#define ERRTAG "ERROR:\t"
#define WARNTAG "WARN:\t"

#define BUFSIZE 300		/* Maximum line length for various parsing proc. */
#ifndef max
#define max(a,b) (((a) > (b))? (a) : (b))
#define min(a,b) ((a) > (b)? (b) : (a))
#endif
#define nint(a) ((int) floor (a + 0.5))

int limit_value(int cur,
		int max,
		const char *name);

/* Linked lists of void pointers and integers, respectively.                */

struct s_linked_vptr
{
    void *data_vptr;
    struct s_linked_vptr *next;
};

struct s_linked_int
{
    int data;
    struct s_linked_int *next;
};
typedef struct s_linked_int t_linked_int;


/* Integer vector.  nelem stores length, list[0..nelem-1] stores list of    *
 * integers.                                                                */

struct s_ivec
{
    int nelem;
    int *list;
};
typedef struct s_ivec t_ivec;


/************************ Memory allocation routines *************************/

void *my_calloc(size_t nelem,
		size_t size);
void *my_malloc(size_t size);
void *my_realloc(void *ptr,
		 size_t size);

void *my_chunk_malloc(size_t size,
		      struct s_linked_vptr **chunk_ptr_head,
		      int *mem_avail_ptr,
		      char **next_mem_loc_ptr);

void free_chunk_memory(struct s_linked_vptr *chunk_ptr_head);


/******************* Linked list, matrix and vector utilities ****************/

void free_ivec_vector(struct s_ivec *ivec_vector,
		      int nrmin,
		      int nrmax);

void free_ivec_matrix(struct s_ivec **ivec_matrix,
		      int nrmin,
		      int nrmax,
		      int ncmin,
		      int ncmax);

void free_ivec_matrix3(struct s_ivec ***ivec_matrix3,
		       int nrmin,
		       int nrmax,
		       int ncmin,
		       int ncmax,
		       int ndmin,
		       int ndmax);

void **alloc_matrix(int nrmin,
		    int nrmax,
		    int ncmin,
		    int ncmax,
		    size_t elsize);

void ***alloc_matrix3(int nrmin,
		      int nrmax,
		      int ncmin,
		      int ncmax,
		      int ndmin,
		      int ndmax,
		      size_t elsize);

void ****alloc_matrix4(int nrmin,
		       int nrmax,
		       int ncmin,
		       int ncmax,
		       int ndmin,
		       int ndmax,
		       int nemin,
		       int nemax,
		       size_t elsize);

void free_matrix(void *vptr,
		 int nrmin,
		 int nrmax,
		 int ncmin,
		 size_t elsize);

void free_matrix3(void *vptr,
		  int nrmin,
		  int nrmax,
		  int ncmin,
		  int ncmax,
		  int ndmin,
		  size_t elsize);

void free_matrix4(void *vptr,
		  int nrmin,
		  int nrmax,
		  int ncmin,
		  int ncmax,
		  int ndmin,
		  int ndmax,
		  int nemin,
		  size_t elsize);

void print_int_matrix3(int ***vptr,
		       int nrmin,
		       int nrmax,
		       int ncmin,
		       int ncmax,
		       int ndmin,
		       int ndmax,
		       char *file);

struct s_linked_vptr *insert_in_vptr_list(struct s_linked_vptr *head,
					  void *vptr_to_add);

t_linked_int *insert_in_int_list(t_linked_int * head,
				 int data,
				 t_linked_int ** free_list_head_ptr);

void free_int_list(t_linked_int ** int_list_head_ptr);

void alloc_ivector_and_copy_int_list(t_linked_int ** list_head_ptr,
				     int num_items,
				     struct s_ivec *ivec,
				     t_linked_int ** free_list_head_ptr);


/****************** File and parsing utilities *******************************/

FILE *my_fopen(const char *fname,
	       const char *flag);

char *my_strtok(char *ptr,
		char *tokens,
		FILE * fp,
		char *buf);

char *my_fgets(char *buf,
	       int max_size,
	       FILE * fp);

int my_atoi(const char *str);

char *my_strdup(const char *str);

char *my_strncpy(char *dest,
		 const char *src,
		 size_t size);

/*********************** Portable random number generators *******************/

void my_srandom(int seed);
int my_irand(int imax);
float my_frand(void);
