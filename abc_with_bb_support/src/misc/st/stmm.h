/*
 * Revision Control Information
 *
 * /projects/hsis/CVS/utilities/st/st.h,v
 * serdar
 * 1.1
 * 1993/07/29 01:00:21
 *
 */
/* LINTLIBRARY */

/* /projects/hsis/CVS/utilities/st/st.h,v 1.1 1993/07/29 01:00:21 serdar Exp */

#ifndef STMM_INCLUDED
#define STMM_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "extra.h"

typedef struct stmm_table_entry stmm_table_entry;
typedef struct stmm_table stmm_table;
typedef struct stmm_generator stmm_generator;

struct stmm_table_entry
{
    char *key;
    char *record;
    stmm_table_entry *next;
};

struct stmm_table
{
    int (*compare) ();
    int (*hash) ();
    int num_bins;
    int num_entries;
    int max_density;
    int reorder_flag;
    double grow_factor;
    stmm_table_entry **bins;
    // memory manager to improve runtime and prevent memory fragmentation
    // added by alanmi - January 16, 2003
    Extra_MmFixed_t *pMemMan;
};

struct stmm_generator
{
    stmm_table *table;
    stmm_table_entry *entry;
    int index;
};

#define stmm_is_member(table,key) stmm_lookup(table,key,(char **) 0)
#define stmm_count(table) ((table)->num_entries)

enum stmm_retval
{ STMM_CONTINUE, STMM_STOP, STMM_DELETE };

typedef enum stmm_retval (*STMM_PFSR) ();
typedef int (*STMM_PFI) ();

EXTERN stmm_table *stmm_init_table_with_params
ARGS ((STMM_PFI, STMM_PFI, int, int, double, int));
EXTERN stmm_table *stmm_init_table ARGS ((STMM_PFI, STMM_PFI));
EXTERN void stmm_free_table ARGS ((stmm_table *));
EXTERN int stmm_lookup ARGS ((stmm_table *, char *, char **));
EXTERN int stmm_lookup_int ARGS ((stmm_table *, char *, int *));
EXTERN int stmm_insert ARGS ((stmm_table *, char *, char *));
EXTERN int stmm_add_direct ARGS ((stmm_table *, char *, char *));
EXTERN int stmm_find_or_add ARGS ((stmm_table *, char *, char ***));
EXTERN int stmm_find ARGS ((stmm_table *, char *, char ***));
EXTERN stmm_table *stmm_copy ARGS ((stmm_table *));
EXTERN int stmm_delete ARGS ((stmm_table *, char **, char **));
EXTERN int stmm_delete_int ARGS ((stmm_table *, long *, char **));
EXTERN int stmm_foreach ARGS ((stmm_table *, STMM_PFSR, char *));
EXTERN int stmm_strhash ARGS ((char *, int));
EXTERN int stmm_numhash ARGS ((char *, int));
EXTERN int stmm_ptrhash ARGS ((char *, int));
EXTERN int stmm_numcmp ARGS ((char *, char *));
EXTERN int stmm_ptrcmp ARGS ((char *, char *));
EXTERN stmm_generator *stmm_init_gen ARGS ((stmm_table *));
EXTERN int stmm_gen ARGS ((stmm_generator *, char **, char **));
EXTERN int stmm_gen_int ARGS ((stmm_generator *, char **, long *));
EXTERN void stmm_free_gen ARGS ((stmm_generator *));
// additional functions
EXTERN void stmm_clean ARGS ((stmm_table *));



#define STMM_DEFAULT_MAX_DENSITY        5
#define STMM_DEFAULT_INIT_TABLE_SIZE   11
#define STMM_DEFAULT_GROW_FACTOR      2.0
#define STMM_DEFAULT_REORDER_FLAG       0

// added by Zhihong: no need for memory allocation
#define stmm_foreach_item2(tb, /* stmm_generator */gen, key, value) \
    for(gen.table=(tb), gen.entry=NULL, gen.index=0; \
	    stmm_gen(&(gen),key,value);)

#define stmm_foreach_item(table, gen, key, value) \
    for(gen=stmm_init_gen(table); stmm_gen(gen,key,value) || (stmm_free_gen(gen),0);)

#define stmm_foreach_item_int(table, gen, key, value) \
    for(gen=stmm_init_gen(table); stmm_gen_int(gen,key,value) || (stmm_free_gen(gen),0);)

#define STMM_OUT_OF_MEM -10000

/*

// consider adding these other other similar definitions
#define st_table       stmm_table
#define st_insert      stmm_insert
#define st_delete      stmm_delete
#define st_lookup      stmm_lookup
#define st_init_table  stmm_init_table
#define st_free_table  stmm_free_table

*/

#ifdef __cplusplus
}
#endif

#endif /* STMM_INCLUDED */
