#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

/* This file contains utility functions widely used in *
 * my programs.  Many are simply versions of file and  *
 * memory grabbing routines that take the same         *
 * arguments as the standard library ones, but exit    *
 * the program if they find an error condition.        */

int linenum;  /* Line in file being parsed. */

FILE *my_fopen(char *fname, char *flag, int prompt) {

 FILE *fp;   /* prompt = 1: prompt user.  prompt=0: use fname */

 while (1) {
    if (prompt) scanf("%s",fname);
    if ((fp = fopen(fname,flag)) != NULL) break; 
    printf("Error opening file %s for %s access.\n",fname,flag);
    if (!prompt) exit(1);
    printf("Please enter another filename.\n");
 }
 return (fp);
}

void *my_calloc (size_t nelem, size_t size) {

 void *ret;
 if ((ret = calloc (nelem,size)) == NULL) {
    fprintf(stderr,"Error:  Unable to calloc memory.  Aborting.\n");
    exit (1);
 }
 return (ret);
}

void *my_malloc (size_t size) {
 void *ret;
 if ((ret = malloc (size)) == NULL) {
    fprintf(stderr,"Error:  Unable to malloc memory.  Aborting.\n");
    exit (1);
 }
 return (ret);
}



void *my_realloc (void *ptr, size_t size) {

 void *ret;

 if ((ret = realloc (ptr,size)) == NULL) {
    fprintf(stderr,"Error:  Unable to realloc memory.  Aborting.\n");
    exit (1);
 }
 return (ret);
}


void *my_chunk_malloc (size_t size, struct s_linked_vptr **chunk_ptr_head, 
       int *mem_avail_ptr, char **next_mem_loc_ptr) {
 
/* This routine should be used for allocating fairly small data             *
 * structures where memory-efficiency is crucial.  This routine allocates   *
 * large "chunks" of data, and parcels them out as requested.  Whenever     *
 * it mallocs a new chunk it adds it to the linked list pointed to by       *
 * chunk_ptr_head.  This list can be used to free the chunked memory.       *
 * If chunk_ptr_head is NULL, no list of chunked memory blocks will be kept *
 * -- this is useful for data structures that you never intend to free as   *
 * it means you don't have to keep track of the linked lists.               *
 * Information about the currently open "chunk" is must be stored by the    *
 * user program.  mem_avail_ptr points to an int storing how many bytes are *
 * left in the current chunk, while next_mem_loc_ptr is the address of a    *
 * pointer to the next free bytes in the chunk.  To start a new chunk,      *
 * simply set *mem_avail_ptr = 0.  Each independent set of data structures  *
 * should use a new chunk.                                                  */
 
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

 if (*mem_avail_ptr < size) {       /* Need to malloc more memory. */
    if (size > CHUNK_SIZE) {      /* Too big, use standard routine. */
       tmp_ptr = my_malloc (size);

#ifdef DEBUG
       printf("NB:  my_chunk_malloc got a request for %d bytes.\n",
          size);
       printf("You should consider using my_malloc for such big requests.\n");
#endif

       if (chunk_ptr_head != NULL) 
          *chunk_ptr_head = insert_in_vptr_list (*chunk_ptr_head, tmp_ptr);
       return (tmp_ptr);
    }

    if (*mem_avail_ptr < FRAGMENT_THRESHOLD) {  /* Only a small scrap left. */
       *next_mem_loc_ptr = my_malloc (CHUNK_SIZE);
       *mem_avail_ptr = CHUNK_SIZE;
       if (chunk_ptr_head != NULL) 
          *chunk_ptr_head = insert_in_vptr_list (*chunk_ptr_head, 
                            *next_mem_loc_ptr);
    }

/* Execute else clause only when the chunk we want is pretty big,  *
 * and would leave too big an unused fragment.  Then we use malloc *
 * to allocate normally.                                           */

    else {     
       tmp_ptr = my_malloc (size);
       if (chunk_ptr_head != NULL) 
          *chunk_ptr_head = insert_in_vptr_list (*chunk_ptr_head, tmp_ptr);
       return (tmp_ptr);
    }
 }

/* Find the smallest distance to advance the memory pointer and keep *
 * everything aligned.                                               */

 if (size % sizeof (Align) == 0) {
    aligned_size = size;
 }
 else {
    aligned_size = size + sizeof(Align) - size % sizeof(Align);
 }

 tmp_ptr = *next_mem_loc_ptr;
 *next_mem_loc_ptr += aligned_size; 
 *mem_avail_ptr -= aligned_size;
 return (tmp_ptr);
}


void free_chunk_memory (struct s_linked_vptr *chunk_ptr_head) {

/* Frees the memory allocated by a sequence of calls to my_chunk_malloc. *
 * All the memory allocated between calls with new_chunk == TRUE by      *
 * my_chunk_malloc will be freed.                                        */

 struct s_linked_vptr *curr_ptr, *prev_ptr;

 curr_ptr = chunk_ptr_head;
 
 while (curr_ptr != NULL) {
    free (curr_ptr->data_vptr);   /* Free memory "chunk". */
    prev_ptr = curr_ptr;
    curr_ptr = curr_ptr->next;
    free (prev_ptr);              /* Free memory used to track "chunk". */
 }
}


struct s_linked_vptr *insert_in_vptr_list (struct s_linked_vptr *head,
              void *vptr_to_add) {

/* Inserts a new element at the head of a linked list of void pointers. *
 * Returns the new head of the list.                                    */

 struct s_linked_vptr *linked_vptr;

 linked_vptr = (struct s_linked_vptr *) my_malloc (sizeof(struct 
                 s_linked_vptr));

 linked_vptr->data_vptr = vptr_to_add;
 linked_vptr->next = head;
 return (linked_vptr);     /* New head of the list */
}


static int cont;  /* line continued? */

char *my_fgets(char *buf, int max_size, FILE *fp) {
 /* Get an input line, update the line number and cut off *
  * any comment part.  A \ at the end of a line with no   *
  * comment part (#) means continue.                      */

 char *val;
 int i;
 
 cont = 0;
 val = fgets(buf,max_size,fp);
 linenum++;
 if (val == NULL) return(val);

/* Check that line completely fit into buffer.  (Flags long line   *
 * truncation).                                                    */

 for (i=0;i<max_size;i++) {
    if (buf[i] == '\n') 
       break;
    if (buf[i] == '\0') {
       printf("Error on line %d -- line is too long for input buffer.\n",
          linenum);
       printf("All lines must be at most %d characters long.\n",BUFSIZE-2);
       printf("The problem could also be caused by a missing newline.\n");
       exit (1);
    }
 }


 for (i=0;i<max_size && buf[i] != '\0';i++) {
    if (buf[i] == '#') {
        buf[i] = '\0';
        break;
    }
 }

 if (i<2) return (val);
 if (buf[i-1] == '\n' && buf[i-2] == '\\') { 
    cont = 1;   /* line continued */
    buf[i-2] = '\n';  /* May need this for tokens */
    buf[i-1] = '\0';
 }
 return(val);
}


char *my_strtok(char *ptr, char *tokens, FILE *fp, char *buf) {

/* Get next token, and wrap to next line if \ at end of line.    *
 * There is a bit of a "gotcha" in strtok.  It does not make a   *
 * copy of the character array which you pass by pointer on the  *
 * first call.  Thus, you must make sure this array exists for   *
 * as long as you are using strtok to parse that line.  Don't    *
 * use local buffers in a bunch of subroutines calling each      *
 * other; the local buffer may be overwritten when the stack is  *
 * restored after return from the subroutine.                    */

 char *val;

 val = strtok(ptr,tokens);
 while (1) {
    if (val != NULL || cont == 0) return(val);
   /* return unless we have a null value and a continuation line */
    if (my_fgets(buf,BUFSIZE,fp) == NULL) 
       return(NULL);
    val = strtok(buf,tokens);
 }
}

void **alloc_matrix (int nrmin, int nrmax, int ncmin, int ncmax, 
   size_t elsize) {

/* allocates an generic matrix with nrmax-nrmin + 1 rows and ncmax - *
 * ncmin + 1 columns, with each element of size elsize. i.e.         *
 * returns a pointer to a storage block [nrmin..nrmax][ncmin..ncmax].*
 * Simply cast the returned array pointer to the proper type.        */

 int i;
 char **cptr;

 cptr = (char **) my_malloc ((nrmax - nrmin + 1) * sizeof (char *));
 cptr -= nrmin;
 for (i=nrmin;i<=nrmax;i++) {
    cptr[i] = (char *) my_malloc ((ncmax - ncmin + 1) * elsize);
    cptr[i] -= ncmin * elsize / sizeof(char);  /* sizeof(char) = 1 */
 }   
 return ((void **) cptr);
}


/* NB:  need to make the pointer type void * instead of void ** to allow   *
 * any pointer to be passed in without a cast.                             */

void free_matrix (void *vptr, int nrmin, int nrmax, int ncmin,
                  size_t elsize) {

 int i;
 char **cptr;

 cptr = (char **) vptr;

 for (i=nrmin;i<=nrmax;i++) 
    free (cptr[i] + ncmin * elsize / sizeof (char));
 free (cptr + nrmin);   
} 


void ***alloc_matrix3 (int nrmin, int nrmax, int ncmin, int ncmax, 
      int ndmin, int ndmax, size_t elsize) {

/* allocates a 3D generic matrix with nrmax-nrmin + 1 rows, ncmax -  *
 * ncmin + 1 columns, and a depth of ndmax-ndmin + 1, with each      *
 * element of size elsize. i.e. returns a pointer to a storage block *
 * [nrmin..nrmax][ncmin..ncmax][ndmin..ndmax].  Simply cast the      *
 *  returned array pointer to the proper type.                       */

 int i, j;
 char ***cptr;

 cptr = (char ***) my_malloc ((nrmax - nrmin + 1) * sizeof (char **));
 cptr -= nrmin;
 for (i=nrmin;i<=nrmax;i++) {
    cptr[i] = (char **) my_malloc ((ncmax - ncmin + 1) * sizeof (char *));
    cptr[i] -= ncmin;
    for (j=ncmin;j<=ncmax;j++) {
       cptr[i][j] = (char *) my_malloc ((ndmax - ndmin + 1) * elsize);
       cptr[i][j] -= ndmin * elsize / sizeof(char); /* sizeof(char) = 1) */
    }
 }   
 return ((void ***) cptr);
}


void free_matrix3 (void *vptr, int nrmin, int nrmax, int ncmin, int ncmax, 
        int ndmin, size_t elsize) {

 int i, j;
 char ***cptr;

 cptr = (char ***) vptr;

 for (i=nrmin;i<=nrmax;i++) {
    for (j=ncmin;j<=ncmax;j++) 
       free (cptr[i][j] + ndmin * elsize / sizeof (char));
    free (cptr[i] + ncmin);
 }
 free (cptr + nrmin);   
} 


void my_srandom (int seed) {

/* Calls the random number generator seed function available on this *
 * architecture.                                                     */
 
#if defined (HP)
 void srand48 (long seed_val);
#else
 void srandom (int seed_val);
#endif

#if defined (HP)
 srand48 (seed);
#else
 srandom (seed);
#endif
}


int my_irand (int imax) {

/* Uses random (better spectral properties) to obtain a random number *
 * which is then scaled to be an integer between 0 and imax.          */

#define MYRANDOM_MAX 2147483647

 float fval;
 int ival;

#if defined (SPARC)
 long random (void);
#elif defined (SGI)
 long random (void);
#else
 long lrand48 (void);
#endif


#if defined (SPARC)
 fval = ((float) random ()) / ((float) MYRANDOM_MAX);
#elif defined (SGI)
 fval = ((float) random ()) / ((float) MYRANDOM_MAX);
#else
 fval = ((float) lrand48()) / ((float) MYRANDOM_MAX);
#endif

 ival = (int) (fval*(imax+1)-0.001);

#ifdef DEBUG
 if ((ival < 0) || (ival > imax)) {
    printf("Bad value in my_irand, imax = %d  ival = %d\n",imax,ival);
    exit(1);
 }
#endif

 return(ival);
} 

float my_frand (void) {

/* Uses random (better spectral properties) to obtain a random number *
 * which is scaled to be a float between 0 and 1.                     */

 float fval;

#if defined (SPARC)
 long random (void);
#elif defined (SGI)
 long random (void);
#else
 long lrand48 (void);
#endif


#if defined (SPARC)
 fval = ((float) random ()) / ((float) MYRANDOM_MAX);
#elif defined (SGI)
 fval = ((float) random ()) / ((float) MYRANDOM_MAX);
#else
 fval = ((float) lrand48()) / ((float) MYRANDOM_MAX);
#endif

#ifdef DEBUG
 if ((fval < 0) || (fval > 1.)) {
    printf("Bad value in my_frand, fval = %g\n",fval);
    exit(1);
 }
#endif

 return(fval);
}
