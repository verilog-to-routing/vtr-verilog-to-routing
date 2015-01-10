#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <map>

/* Parameter tags for preprocessor to strip */
#define INP
#define OUTP
#define INOUTP

#define BUFSIZE 8192 /* Maximum line length for various parsing proc. */
#define nint(a) ((int) floor (a + 0.5))

#define ERRTAG "ERROR:\t"
#define WARNTAG "WARNING:\t"

#include <time.h>
#ifndef CLOCKS_PER_SEC
    #ifndef CLK_PER_SEC
        #error Neither CLOCKS_PER_SEC nor CLK_PER_SEC is defined.
    #endif
    #define CLOCKS_PER_SEC CLK_PER_SEC
#endif

enum e_vpr_error {
	VPR_ERROR_UNKNOWN = 0, 
	VPR_ERROR_ARCH,
	VPR_ERROR_PACK,
	VPR_ERROR_PLACE,
	VPR_ERROR_ROUTE,
	VPR_ERROR_TIMING,
	VPR_ERROR_SDC,
	VPR_ERROR_NET_F,
	VPR_ERROR_PLACE_F,
	VPR_ERROR_BLIF_F,
	VPR_ERROR_OTHER
};
typedef enum e_vpr_error t_vpr_error_type;

int limit_value(int cur, int max, const char *name);

/* Linked lists of void pointers and integers, respectively. */

typedef struct s_linked_vptr {
	void *data_vptr;
	struct s_linked_vptr *next;
} t_linked_vptr;

typedef struct s_linked_int {
	int data;
	struct s_linked_int *next;
} t_linked_int;

/* Integer vector.  nelem stores length, list[0..nelem-1] stores list of    *
 * integers.                                                                */

typedef struct s_ivec {
	int nelem;
	int *list;
} t_ivec;

/* This structure is to keep track of chunks of memory that is being	*
 * allocated to save overhead when allocating very small memory pieces. *
 * For a complete description, please see the comment in my_chunk_malloc*
 * in libarchfpga/utils.c.						*/

typedef struct s_chunk {
	struct s_linked_vptr *chunk_ptr_head; 
	/* chunk_ptr_head->data_vptr: head of the entire linked 
	 * list of allocated "chunk" memory;
	 * chunk_ptr_head->next: pointer to the next chunk on the linked list*/
	int mem_avail; /* number of bytes left in the current chunk */
	char *next_mem_loc_ptr;/* pointer to the first available (free) *
				* byte in the current chunk		*/
} t_chunk;

/* This structure is thrown back to highest level of VPR flow if an *
 * internal VPR or user input error occurs. */

typedef struct s_vpr_error {
	char* message;
	char* file_name;
	int line_num;
	enum e_vpr_error type;
} t_vpr_error;

#ifdef __cplusplus 
extern "C" {
#endif

extern char *out_file_prefix; /* Default prefix string for output files */

/************************ Memory allocation routines *************************/

#define my_calloc(nelem, size) my_calloc_impl(nelem, size, __FILE__, __LINE__)
#define my_malloc(size) my_malloc_impl(size, __FILE__, __LINE__)
#define my_realloc(ptr, size) my_realloc_impl(ptr, size, __FILE__, __LINE__)

void *my_calloc_impl(size_t nelem, size_t size, const char* const file, const int line);
void *my_malloc_impl(size_t size, const char* const file, const int line);
void *my_realloc_impl(void *ptr, size_t size, const char* const file, const int line);

void *my_chunk_malloc(size_t size, t_chunk *chunk_info);
void free_chunk_memory(t_chunk *chunk_info);

/******************* Linked list, matrix and vector utilities ****************/

void free_ivec_vector(struct s_ivec *ivec_vector, int nrmin, int nrmax);
void free_ivec_matrix(struct s_ivec **ivec_matrix, int nrmin, int nrmax,
		int ncmin, int ncmax);
void free_ivec_matrix3(struct s_ivec ***ivec_matrix3, int nrmin,
		int nrmax, int ncmin, int ncmax, int ndmin, int ndmax);

void** alloc_matrix(int nrmin, int nrmax, int ncmin, int ncmax,
		size_t elsize);
void ***alloc_matrix3(int nrmin, int nrmax, int ncmin, int ncmax,
		int ndmin, int ndmax, size_t elsize);
void ****alloc_matrix4(int nrmin, int nrmax, int ncmin, int ncmax,
		int ndmin, int ndmax, int nemin, int nemax, size_t elsize);
void *****alloc_matrix5(int nrmin, int nrmax, int ncmin, int ncmax,
		int ndmin, int ndmax, int nemin, int nemax, int nfmin, int nfmax, size_t elsize);

void free_matrix(void *vptr, int nrmin, int nrmax, int ncmin,
		size_t elsize);
void free_matrix3(void *vptr, int nrmin, int nrmax, int ncmin, int ncmax,
		int ndmin, size_t elsize);
void free_matrix4(void *vptr, int nrmin, int nrmax, int ncmin, int ncmax,
		int ndmin, int ndmax, int nemin, size_t elsize);
void free_matrix5(void *vptr, int nrmin, int nrmax, int ncmin, int ncmax,
		int ndmin, int ndmax, int nemin, int nemax, int nfmin, size_t elsize);

void print_int_matrix3(int ***vptr, int nrmin, int nrmax, int ncmin,
		int ncmax, int ndmin, int ndmax, char *file);

struct s_linked_vptr *insert_in_vptr_list(struct s_linked_vptr *head,
		void *vptr_to_add);
struct s_linked_vptr *delete_in_vptr_list(struct s_linked_vptr *head);
t_linked_int *insert_in_int_list(t_linked_int * head, int data,
		t_linked_int ** free_list_head_ptr);
void free_int_list(t_linked_int ** int_list_head_ptr);
void alloc_ivector_and_copy_int_list(t_linked_int ** list_head_ptr,
		int num_items, struct s_ivec *ivec, t_linked_int ** free_list_head_ptr);

/****************** File and parsing utilities *******************************/

int my_atoi(const char *str);

char* my_strdup(const char *str);
char *my_strncpy(char *dest, const char *src, size_t size);
char *my_strtok(char *ptr, const char *tokens, FILE * fp, char *buf);

FILE* my_fopen(const char *fname, const char *flag, int prompt);
char *my_fgets(char *buf, int max_size, FILE * fp);
bool file_exists(const char * filename);

bool check_file_name_extension(INP const char* file_name, 
								INP const char* file_extension);

int get_file_line_number_of_last_opened_file();

/*********************** Portable random number generators *******************/
void my_srandom(int seed);
unsigned int get_current_random();
int my_irand(int imax);
float my_frand(void);

typedef void (*vpr_PrintHandlerInfo)( 
		const char* pszMessage, ... );
typedef void (*vpr_PrintHandlerWarning)( 
		const char* pszFileName, unsigned int lineNum,
		const char* pszMessage,	... );
typedef void (*vpr_PrintHandlerError)( 
		const char* pszFileName, unsigned int lineNum,
		const char* pszMessage,	... );
typedef void (*vpr_PrintHandlerDirect)( 
		const char* pszMessage,	... );

extern vpr_PrintHandlerInfo vpr_printf_info;
extern vpr_PrintHandlerWarning vpr_printf_warning;
extern vpr_PrintHandlerError vpr_printf_error;
extern vpr_PrintHandlerDirect vpr_printf_direct;

#ifdef __cplusplus 
}
#endif

/*********************** Math operations *************************************/
int ipow(int base, int exp);

template<typename X, typename Y> Y linear_interpolate_or_extrapolate(INP std::map<X,Y> *xy_map, INP X requested_x);

/*********************** Error-related ***************************************/
void Print_VPR_Error(t_vpr_error* vpr_error, char* arch_filename);
t_vpr_error* alloc_and_load_vpr_error(enum e_vpr_error type, unsigned int line, char* file_name);
void vpr_throw(enum e_vpr_error type, const char* psz_file_name, unsigned int line_num, const char* psz_message, ...);
void vvpr_throw(enum e_vpr_error type, const char* psz_file_name, unsigned int line_num, const char* psz_message, va_list args);

#endif
