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

#ifndef ABC__misc__st__st_h
#define ABC__misc__st__st_h
#define st__INCLUDED

#include "misc/util/abc_global.h"

ABC_NAMESPACE_HEADER_START


/* These are potential duplicates. */
#ifndef EXTERN
#   ifdef __cplusplus
#       ifdef ABC_NAMESPACE
#           define EXTERN extern
#       else
#           define EXTERN extern "C"
#       endif
#   else
#       define EXTERN extern
#   endif
#endif

#ifndef ARGS
#define ARGS(protos) protos
#endif


typedef int (* st__compare_func_type)(const char*, const char*);
typedef int (* st__hash_func_type)(const char*, int);

typedef struct st__table_entry st__table_entry;
struct st__table_entry {
    char *key;
    char *record;
    st__table_entry *next;
};

typedef struct st__table st__table;
struct st__table {
    st__compare_func_type compare;
    st__hash_func_type hash;
    int num_bins;
    int num_entries;
    int max_density;
    int reorder_flag;
    double grow_factor;
    st__table_entry **bins;
};

typedef struct st__generator st__generator;
struct st__generator {
    st__table *table;
    st__table_entry *entry;
    int index;
};

#define st__is_member(table,key) st__lookup(table,key,(char **) 0)
#define st__count(table) ((table)->num_entries)

enum st__retval { st__CONTINUE, st__STOP, st__DELETE};

typedef enum st__retval (* st__PFSR)(char *, char *, char *);
typedef int (* st__PFI)();

extern st__table * st__init_table_with_params ( st__compare_func_type compare, st__hash_func_type hash, int size, int density, double grow_factor, int reorder_flag);
extern st__table * st__init_table ( st__compare_func_type, st__hash_func_type);
extern void st__free_table ( st__table *);
extern int st__lookup ( st__table *, const char *, char **);
extern int st__lookup_int ( st__table *, char *, int *);
extern int st__insert ( st__table *, const char *, char *);
extern int st__add_direct ( st__table *, char *, char *);
extern int st__find_or_add ( st__table *, char *, char ***);
extern int st__find ( st__table *, char *, char ***);
extern st__table * st__copy ( st__table *);
extern int st__delete ( st__table *, const char **, char **);
extern int st__delete_int ( st__table *, long *, char **);
extern int st__foreach ( st__table *, st__PFSR, char *);
extern int st__strhash (const char *, int);
extern int st__numhash (const char *, int);
extern int st__ptrhash (const char *, int);
extern int st__numcmp (const char *, const char *);
extern int st__ptrcmp (const char *, const char *);
extern st__generator * st__init_gen ( st__table *);
extern int st__gen ( st__generator *, const char **, char **);
extern int st__gen_int ( st__generator *, const char **, long *);
extern void st__free_gen ( st__generator *);


#define st__DEFAULT_MAX_DENSITY 5
#define st__DEFAULT_INIT_TABLE_SIZE 11
#define st__DEFAULT_GROW_FACTOR 2.0
#define st__DEFAULT_REORDER_FLAG 0

#define st__foreach_item(table, gen, key, value) \
    for(gen= st__init_gen(table); st__gen(gen,key,value) || ( st__free_gen(gen),0);)

#define st__foreach_item_int(table, gen, key, value) \
    for(gen= st__init_gen(table); st__gen_int(gen,key,value) || ( st__free_gen(gen),0);)

#define st__OUT_OF_MEM -10000



ABC_NAMESPACE_HEADER_END



#endif /* st__INCLUDED */
