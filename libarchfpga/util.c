#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "util.h"

/* This file contains utility functions widely used in *
 * my programs.  Many are simply versions of file and  *
 * memory grabbing routines that take the same         *
 * arguments as the standard library ones, but exit    *
 * the program if they find an error condition.        */

int file_line_number; /* file in line number being parsed */
char *out_file_prefix = NULL;
messagelogger vpr_printf = PrintHandlerMessage;

static int cont; /* line continued? */

/* Returns the min of cur and max. If cur > max, a warning
 * is emitted. */
int limit_value(int cur, int max, const char *name) {
	if (cur > max) {
		vpr_printf(TIO_MESSAGE_WARNING,
				"%s is being limited from [%d] to [%d]\n", name, cur, max);
		return max;
	}
	return cur;
}

/* An alternate for strncpy since strncpy doesn't work as most
 * people would expect. This ensures null termination */
char *
my_strncpy(char *dest, const char *src, size_t size) {
	/* Find string's length */
	size_t len = strlen(src);

	/* Cap length at (num - 1) to leave room for \0 */
	if (size <= len)
		len = (size - 1);

	/* Copy as much of string as we can fit */
	memcpy(dest, src, len);

	/* explicit null termination */
	dest[len] = '\0';

	return dest;
}

/* Uses global var 'out_file_prefix' */
FILE *
my_fopen(const char *fname, const char *flag, int prompt) {
	FILE *fp;
	int Len;
	char *new_fname = NULL;
	char prompt_filename[256];

	/* Appends a prefix string for output files */
	if (out_file_prefix) {
		if (strchr(flag, 'w')) {
			Len = 1; /* NULL char */
			Len += strlen(out_file_prefix);
			Len += strlen(fname);
			new_fname = (char *) my_malloc(Len * sizeof(char));
			strcpy(new_fname, out_file_prefix);
			strcat(new_fname, fname);
			fname = new_fname;
		}
	}

	if (prompt) {
		int check_num_of_entered_values = scanf("%s", prompt_filename);
		while (getchar() != '\n')
			;

		while (check_num_of_entered_values != 1) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"Was expecting one file name to be entered, with no spaces. You have entered %d parameters. Please try again: \n",
					check_num_of_entered_values);
			check_num_of_entered_values = scanf("%s", prompt_filename);
		}
		fname = prompt_filename;
	}

	if (NULL == (fp = fopen(fname, flag))) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"Error opening file %s for %s access: %s.\n", fname, flag,
				strerror(errno));
		exit(1);
	}

	if (new_fname)
		free(new_fname);

	return (fp);
}

char *
my_strdup(const char *str) {
	int Len;
	char *Dst;

	if (str == NULL ) {
		return NULL ;
	}

	Len = 1 + strlen(str);
	Dst = (char *) my_malloc(Len * sizeof(char));
	memcpy(Dst, str, Len);

	return Dst;
}

int my_atoi(const char *str) {

	/* Returns the integer represented by the first part of the character       *
	 * string.                                              */

	if (str[0] < '0' || str[0] > '9') {
		if (!(str[0] == '-' && str[1] >= '0' && str[1] <= '9')) {
			vpr_printf(TIO_MESSAGE_ERROR, "expected number instead of '%s'.\n",
					str);
			exit(1);
		}
	}
	return (atoi(str));
}

void *
my_calloc(size_t nelem, size_t size) {
	void *ret;
	if (nelem == 0) {
		return NULL ;
	}

	if ((ret = calloc(nelem, size)) == NULL ) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"Error:  Unable to calloc memory.  Aborting.\n");
		exit(1);
	}
	return (ret);
}

void *
my_malloc(size_t size) {
	void *ret;
	if (size == 0) {
		return NULL ;
	}

	if ((ret = malloc(size)) == NULL ) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"Error:  Unable to malloc memory.  Aborting.\n");
		abort();
		exit(1);
	}
	return (ret);
}

void *
my_realloc(void *ptr, size_t size) {
	void *ret;

	if (size <= 0) {
		vpr_printf(TIO_MESSAGE_WARNING, "reallocating of size <= 0.\n");
	}

	ret = realloc(ptr, size);
	if (NULL == ret) {
		vpr_printf(TIO_MESSAGE_ERROR, "Unable to realloc memory. Aborting. "
				"ptr=%p, Size=%d.\n", ptr, (int) size);
		if (ptr == NULL ) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"my_realloc: ptr == NULL. Aborting.\n");
		}
		exit(1);
	}
	return (ret);
}

void *
my_chunk_malloc(size_t size, t_chunk *chunk_info) {

	/* This routine should be used for allocating fairly small data             *
	 * structures where memory-efficiency is crucial.  This routine allocates   *
	 * large "chunks" of data, and parcels them out as requested.  Whenever     *
	 * it mallocs a new chunk it adds it to the linked list pointed to by       *
	 * chunk_info->chunk_ptr_head.  This list can be used to free the	    *
	 * chunked memory.							    *
	 * Information about the currently open "chunk" must be stored by the       *
	 * user program.  chunk_info->mem_avail_ptr points to an int storing	    *
	 * how many bytes are left in the current chunk, while			    *
	 * chunk_info->next_mem_loc_ptr is the address of a pointer to the	    *
	 * next free bytes in the chunk.  To start a new chunk, simply set	    *
	 * chunk_info->mem_avail_ptr = 0.  Each independent set of data		    *
	 * structures should use a new chunk.                                       */

	/* To make sure the memory passed back is properly aligned, I must *
	 * only send back chunks in multiples of the worst-case alignment  *
	 * restriction of the machine.  On most machines this should be    *
	 * a long, but on 64-bit machines it might be a long long or a     *
	 * double.  Change the typedef below if this is the case.          */

	typedef long Align;

#define CHUNK_SIZE 32768
#define FRAGMENT_THRESHOLD 100

	char *tmp_ptr;
	int aligned_size;

	assert(chunk_info->mem_avail >= 0);

	if ((size_t) (chunk_info->mem_avail) < size) { /* Need to malloc more memory. */
		if (size > CHUNK_SIZE) { /* Too big, use standard routine. */
			tmp_ptr = (char *) my_malloc(size);

			/* When debugging, uncomment the code below to see if memory allocation size */
			/* makes sense */
			/*#ifdef DEBUG
			 vpr_printf("NB:  my_chunk_malloc got a request for %d bytes.\n",
			 size);
			 vpr_printf("You should consider using my_malloc for such big requests.\n");
			 #endif */

			assert(chunk_info != NULL);
			chunk_info->chunk_ptr_head = insert_in_vptr_list(
					chunk_info->chunk_ptr_head, tmp_ptr);
			return (tmp_ptr);
		}

		if (chunk_info->mem_avail < FRAGMENT_THRESHOLD) { /* Only a small scrap left. */
			chunk_info->next_mem_loc_ptr = (char *) my_malloc(CHUNK_SIZE);
			chunk_info->mem_avail = CHUNK_SIZE;
			assert(chunk_info != NULL);
			chunk_info->chunk_ptr_head = insert_in_vptr_list(
					chunk_info->chunk_ptr_head, chunk_info->next_mem_loc_ptr);
		}

		/* Execute else clause only when the chunk we want is pretty big,  *
		 * and would leave too big an unused fragment.  Then we use malloc *
		 * to allocate normally.                                           */

		else {
			tmp_ptr = (char *) my_malloc(size);
			assert(chunk_info != NULL);
			chunk_info->chunk_ptr_head = insert_in_vptr_list(
					chunk_info->chunk_ptr_head, tmp_ptr);
			return (tmp_ptr);
		}
	}

	/* Find the smallest distance to advance the memory pointer and keep *
	 * everything aligned.                                               */

	if (size % sizeof(Align) == 0) {
		aligned_size = size;
	} else {
		aligned_size = size + sizeof(Align) - size % sizeof(Align);
	}

	tmp_ptr = chunk_info->next_mem_loc_ptr;
	chunk_info->next_mem_loc_ptr += aligned_size;
	chunk_info->mem_avail -= aligned_size;
	return (tmp_ptr);
}

void free_chunk_memory(t_chunk *chunk_info) {

	/* Frees the memory allocated by a sequence of calls to my_chunk_malloc. */

	struct s_linked_vptr *curr_ptr, *prev_ptr;

	curr_ptr = chunk_info->chunk_ptr_head;

	while (curr_ptr != NULL ) {
		free(curr_ptr->data_vptr); /* Free memory "chunk". */
		prev_ptr = curr_ptr;
		curr_ptr = curr_ptr->next;
		free(prev_ptr); /* Free memory used to track "chunk". */
	}
	chunk_info->chunk_ptr_head = NULL;
	chunk_info->mem_avail = 0;
	chunk_info->next_mem_loc_ptr = NULL;
}

struct s_linked_vptr *
insert_in_vptr_list(struct s_linked_vptr *head, void *vptr_to_add) {

	/* Inserts a new element at the head of a linked list of void pointers. *
	 * Returns the new head of the list.                                    */

	struct s_linked_vptr *linked_vptr;

	linked_vptr = (struct s_linked_vptr *) my_malloc(
			sizeof(struct s_linked_vptr));

	linked_vptr->data_vptr = vptr_to_add;
	linked_vptr->next = head;
	return (linked_vptr); /* New head of the list */
}

/* Deletes the element at the head of a linked list of void pointers. *
 * Returns the new head of the list.                                    */
struct s_linked_vptr *
delete_in_vptr_list(struct s_linked_vptr *head) {
	struct s_linked_vptr *linked_vptr;

	if (head == NULL )
		return NULL ;
	linked_vptr = head->next;
	free(head);
	return linked_vptr; /* New head of the list */
}

t_linked_int *
insert_in_int_list(t_linked_int * head, int data,
		t_linked_int ** free_list_head_ptr) {

	/* Inserts a new element at the head of a linked list of integers.  Returns  *
	 * the new head of the list.  One argument is the address of the head of     *
	 * a list of free ilist elements.  If there are any elements on this free    *
	 * list, the new element is taken from it.  Otherwise a new one is malloced. */

	t_linked_int *linked_int;

	if (*free_list_head_ptr != NULL ) {
		linked_int = *free_list_head_ptr;
		*free_list_head_ptr = linked_int->next;
	} else {
		linked_int = (t_linked_int *) my_malloc(sizeof(t_linked_int));
	}

	linked_int->data = data;
	linked_int->next = head;
	return (linked_int);
}

void free_int_list(t_linked_int ** int_list_head_ptr) {

	/* This routine truly frees (calls free) all the integer list elements    * 
	 * on the linked list pointed to by *head, and sets head = NULL.          */

	t_linked_int *linked_int, *next_linked_int;

	linked_int = *int_list_head_ptr;

	while (linked_int != NULL ) {
		next_linked_int = linked_int->next;
		free(linked_int);
		linked_int = next_linked_int;
	}

	*int_list_head_ptr = NULL;
}

void alloc_ivector_and_copy_int_list(t_linked_int ** list_head_ptr,
		int num_items, struct s_ivec *ivec, t_linked_int ** free_list_head_ptr) {

	/* Allocates an integer vector with num_items elements and copies the       *
	 * integers from the list pointed to by list_head (of which there must be   *
	 * num_items) over to it.  The int_list is then put on the free list, and   *
	 * the list_head_ptr is set to NULL.                                        */

	t_linked_int *linked_int, *list_head;
	int i, *list;

	list_head = *list_head_ptr;

	if (num_items == 0) { /* Empty list. */
		ivec->nelem = 0;
		ivec->list = NULL;

		if (list_head != NULL ) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"alloc_ivector_and_copy_int_list: Copied %d elements, "
							"but list at %p contains more.\n", num_items,
					(void *) list_head);
			exit(1);
		}
		return;
	}

	ivec->nelem = num_items;
	list = (int *) my_malloc(num_items * sizeof(int));
	ivec->list = list;
	linked_int = list_head;

	for (i = 0; i < num_items - 1; i++) {
		list[i] = linked_int->data;
		linked_int = linked_int->next;
	}

	list[num_items - 1] = linked_int->data;

	if (linked_int->next != NULL ) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"Error in alloc_ivector_and_copy_int_list:\n Copied %d elements, "
						"but list at %p contains more.\n", num_items,
				(void *) list_head);
		exit(1);
	}

	linked_int->next = *free_list_head_ptr;
	*free_list_head_ptr = list_head;
	*list_head_ptr = NULL;
}

char *
my_fgets(char *buf, int max_size, FILE * fp) {
	/* Get an input line, update the line number and cut off *
	 * any comment part.  A \ at the end of a line with no   *
	 * comment part (#) means continue. my_fgets should give * 
	 * identical results for Windows (\r\n) and Linux (\n)   *
	 * newlines, since it replaces each carriage return \r   *
	 * by a newline character \n.  Returns NULL after EOF.	 */

	char ch;
	int i;

	cont = 0; /* line continued? */
	file_line_number++; /* global variable */

	for (i = 0; i < max_size - 1; i++) { /* Keep going until the line finishes or the buffer is full */

		ch = fgetc(fp);

		if (feof(fp)) { /* end of file */
			if (i == 0) {
				return NULL ; /* required so we can write while (my_fgets(...) != NULL) */
			} else { /* no newline before end of file - last line must be returned */
				buf[i] = '\0';
				return buf;
			}
		}

		if (ch == '#') { /* comment */
			buf[i] = '\0';
			while ((ch = fgetc(fp)) != '\n' && !feof(fp))
				; /* skip the rest of the line */
			return buf;
		}

		if (ch == '\r' || ch == '\n') { /* newline (cross-platform) */
			if (i != 0 && buf[i - 1] == '\\') { /* if \ at end of line, line continued */
				cont = 1;
				buf[i - 1] = '\n'; /* May need this for tokens */
				buf[i] = '\0';
			} else {
				buf[i] = '\n';
				buf[i + 1] = '\0';
			}
			return buf;
		}

		buf[i] = ch; /* copy character into the buffer */

	}

	/* Buffer is full but line has not terminated, so error */
	vpr_printf(TIO_MESSAGE_ERROR,
			"Error on line %d -- line is too long for input buffer.\n",
			file_line_number);
	vpr_printf(TIO_MESSAGE_ERROR,
			"All lines must be at most %d characters long.\n", BUFSIZE - 2);
	exit(1);
}

char *
my_strtok(char *ptr, const char *tokens, FILE * fp, char *buf) {

	/* Get next token, and wrap to next line if \ at end of line.    *
	 * There is a bit of a "gotcha" in strtok.  It does not make a   *
	 * copy of the character array which you pass by pointer on the  *
	 * first call.  Thus, you must make sure this array exists for   *
	 * as long as you are using strtok to parse that line.  Don't    *
	 * use local buffers in a bunch of subroutines calling each      *
	 * other; the local buffer may be overwritten when the stack is  *
	 * restored after return from the subroutine.                    */

	char *val;

	val = strtok(ptr, tokens);
	for (;;) {
		if (val != NULL || cont == 0)
			return (val);

		/* return unless we have a null value and a continuation line */
		if (my_fgets(buf, BUFSIZE, fp) == NULL )
			return (NULL );

		val = strtok(buf, tokens);
	}
}

void free_ivec_vector(struct s_ivec *ivec_vector, int nrmin, int nrmax) {

	/* Frees a 1D array of integer vectors.                              */

	int i;

	for (i = nrmin; i <= nrmax; i++)
		if (ivec_vector[i].nelem != 0)
			free(ivec_vector[i].list);

	free(ivec_vector + nrmin);
}

void free_ivec_matrix(struct s_ivec **ivec_matrix, int nrmin, int nrmax,
		int ncmin, int ncmax) {

	/* Frees a 2D matrix of integer vectors (ivecs).                     */

	int i, j;

	for (i = nrmin; i <= nrmax; i++) {
		for (j = ncmin; j <= ncmax; j++) {
			if (ivec_matrix[i][j].nelem != 0) {
				free(ivec_matrix[i][j].list);
			}
		}
	}

	free_matrix(ivec_matrix, nrmin, nrmax, ncmin, sizeof(struct s_ivec));
}

void free_ivec_matrix3(struct s_ivec ***ivec_matrix3, int nrmin, int nrmax,
		int ncmin, int ncmax, int ndmin, int ndmax) {

	/* Frees a 3D matrix of integer vectors (ivecs).                     */

	int i, j, k;

	for (i = nrmin; i <= nrmax; i++) {
		for (j = ncmin; j <= ncmax; j++) {
			for (k = ndmin; k <= ndmax; k++) {
				if (ivec_matrix3[i][j][k].nelem != 0) {
					free(ivec_matrix3[i][j][k].list);
				}
			}
		}
	}

	free_matrix3(ivec_matrix3, nrmin, nrmax, ncmin, ncmax, ndmin,
			sizeof(struct s_ivec));
}

void **
alloc_matrix(int nrmin, int nrmax, int ncmin, int ncmax, size_t elsize) {

	/* allocates an generic matrix with nrmax-nrmin + 1 rows and ncmax - *
	 * ncmin + 1 columns, with each element of size elsize. i.e.         *
	 * returns a pointer to a storage block [nrmin..nrmax][ncmin..ncmax].*
	 * Simply cast the returned array pointer to the proper type.        */

	int i;
	char **cptr;

	cptr = (char **) my_malloc((nrmax - nrmin + 1) * sizeof(char *));
	cptr -= nrmin;
	for (i = nrmin; i <= nrmax; i++) {
		cptr[i] = (char *) my_malloc((ncmax - ncmin + 1) * elsize);
		cptr[i] -= ncmin * elsize / sizeof(char); /* sizeof(char) = 1 */
	}
	return ((void **) cptr);
}

/* NB:  need to make the pointer type void * instead of void ** to allow   *
 * any pointer to be passed in without a cast.                             */

void free_matrix(void *vptr, int nrmin, int nrmax, int ncmin, size_t elsize) {
	int i;
	char **cptr;

	cptr = (char **) vptr;

	for (i = nrmin; i <= nrmax; i++)
		free(cptr[i] + ncmin * elsize / sizeof(char));
	free(cptr + nrmin);
}

void ***
alloc_matrix3(int nrmin, int nrmax, int ncmin, int ncmax, int ndmin, int ndmax,
		size_t elsize) {

	/* allocates a 3D generic matrix with nrmax-nrmin + 1 rows, ncmax -  *
	 * ncmin + 1 columns, and a depth of ndmax-ndmin + 1, with each      *
	 * element of size elsize. i.e. returns a pointer to a storage block *
	 * [nrmin..nrmax][ncmin..ncmax][ndmin..ndmax].  Simply cast the      *
	 *  returned array pointer to the proper type.                       */

	int i, j;
	char ***cptr;

	cptr = (char ***) my_malloc((nrmax - nrmin + 1) * sizeof(char **));
	cptr -= nrmin;
	for (i = nrmin; i <= nrmax; i++) {
		cptr[i] = (char **) my_malloc((ncmax - ncmin + 1) * sizeof(char *));
		cptr[i] -= ncmin;
		for (j = ncmin; j <= ncmax; j++) {
			cptr[i][j] = (char *) my_malloc((ndmax - ndmin + 1) * elsize);
			cptr[i][j] -= ndmin * elsize / sizeof(char); /* sizeof(char) = 1) */
		}
	}
	return ((void ***) cptr);
}

void ****
alloc_matrix4(int nrmin, int nrmax, int ncmin, int ncmax, int ndmin, int ndmax,
		int nemin, int nemax, size_t elsize) {

	/* allocates a 3D generic matrix with nrmax-nrmin + 1 rows, ncmax -  *
	 * ncmin + 1 columns, and a depth of ndmax-ndmin + 1, with each      *
	 * element of size elsize. i.e. returns a pointer to a storage block *
	 * [nrmin..nrmax][ncmin..ncmax][ndmin..ndmax].  Simply cast the      *
	 *  returned array pointer to the proper type.                       */

	int i, j, k;
	char ****cptr;

	cptr = (char ****) my_malloc((nrmax - nrmin + 1) * sizeof(char ***));
	cptr -= nrmin;
	for (i = nrmin; i <= nrmax; i++) {
		cptr[i] = (char ***) my_malloc((ncmax - ncmin + 1) * sizeof(char **));
		cptr[i] -= ncmin;
		for (j = ncmin; j <= ncmax; j++) {
			cptr[i][j] = (char **) my_malloc(
					(ndmax - ndmin + 1) * sizeof(char *));
			cptr[i][j] -= ndmin;
			for (k = ndmin; k <= ndmax; k++) {
				cptr[i][j][k] = (char *) my_malloc(
						(nemax - nemin + 1) * elsize);
				cptr[i][j][k] -= nemin * elsize / sizeof(char); /* sizeof(char) = 1) */
			}
		}
	}
	return ((void ****) cptr);
}

void print_int_matrix3(int ***vptr, int nrmin, int nrmax, int ncmin, int ncmax,
		int ndmin, int ndmax, char *file) {
	FILE *outfile;
	int i, j, k;

	outfile = my_fopen(file, "w", 0);

	for (k = nrmin; k <= nrmax; ++k) {
		fprintf(outfile, "Plane %d\n", k);
		for (j = ncmin; j <= ncmax; ++j) {
			for (i = ndmin; i <= ndmax; ++i) {
				fprintf(outfile, "%d ", vptr[k][j][i]);
			}
			fprintf(outfile, "\n");
		}
		fprintf(outfile, "\n");
	}

	fclose(outfile);
}

void free_matrix3(void *vptr, int nrmin, int nrmax, int ncmin, int ncmax,
		int ndmin, size_t elsize) {
	int i, j;
	char ***cptr;

	cptr = (char ***) vptr;

	for (i = nrmin; i <= nrmax; i++) {
		for (j = ncmin; j <= ncmax; j++)
			free(cptr[i][j] + ndmin * elsize / sizeof(char));
		free(cptr[i] + ncmin);
	}
	free(cptr + nrmin);
}

void free_matrix4(void *vptr, int nrmin, int nrmax, int ncmin, int ncmax,
		int ndmin, int ndmax, int nemin, size_t elsize) {
	int i, j, k;
	char ****cptr;

	cptr = (char ****) vptr;

	for (i = nrmin; i <= nrmax; i++) {
		for (j = ncmin; j <= ncmax; j++) {
			for (k = ndmin; k <= ndmax; k++)
				free(cptr[i][j][k] + nemin * elsize / sizeof(char));
			free(cptr[i][j] + ndmin * elsize / sizeof(char));
		}
		free(cptr[i] + ncmin);
	}
	free(cptr + nrmin);
}

/* Portable random number generator defined below.  Taken from ANSI C by  *
 * K & R.  Not a great generator, but fast, and good enough for my needs. */

#define IA 1103515245u
#define IC 12345u
#define IM 2147483648u
#define CHECK_RAND

static unsigned int current_random = 0;

void my_srandom(int seed) {
	current_random = (unsigned int) seed;
}

int my_irand(int imax) {

	/* Creates a random integer between 0 and imax, inclusive.  i.e. [0..imax] */

	int ival;

	/* current_random = (current_random * IA + IC) % IM; */
	current_random = current_random * IA + IC; /* Use overflow to wrap */
	ival = current_random & (IM - 1); /* Modulus */
	ival = (int) ((float) ival * (float) (imax + 0.999) / (float) IM);

#ifdef CHECK_RAND
	if ((ival < 0) || (ival > imax)) {
		if (ival == imax + 1) {
			/* Due to random floating point rounding, sometimes above calculation gives number greater than ival by 1 */
			ival = imax;
		} else {
			vpr_printf(TIO_MESSAGE_ERROR,
					"Bad value in my_irand, imax = %d  ival = %d\n", imax,
					ival);
			exit(1);
		}
	}
#endif

	return (ival);
}

float my_frand(void) {

	/* Creates a random float between 0 and 1.  i.e. [0..1).        */

	float fval;
	int ival;

	current_random = current_random * IA + IC; /* Use overflow to wrap */
	ival = current_random & (IM - 1); /* Modulus */
	fval = (float) ival / (float) IM;

#ifdef CHECK_RAND
	if ((fval < 0) || (fval > 1.)) {
		vpr_printf(TIO_MESSAGE_ERROR, "Bad value in my_frand, fval = %g\n",
				fval);
		exit(1);
	}
#endif

	return (fval);
}

boolean file_exists(const char * filename) {
	FILE * file;

	if (filename == NULL ) {
		return FALSE;
	}

	file = fopen(filename, "r");
	if (file) {
		fclose(file);
		return TRUE;
	}
	return FALSE;
}

int ipow(int base, int exp) {
	int result = 1;

	assert(exp >= 0);

	while (exp) {
		if (exp & 1)
			result *= base;
		exp >>= 1;
		base *= base;
	}

	return result;
}
