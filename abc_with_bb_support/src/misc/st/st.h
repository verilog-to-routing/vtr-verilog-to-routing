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

#ifndef ST_INCLUDED
#define ST_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct st_table_entry st_table_entry;
struct st_table_entry {
    char *key;
    char *record;
    st_table_entry *next;
};

typedef struct st_table st_table;
struct st_table {
    int (*compare)();
    int (*hash)();
    int num_bins;
    int num_entries;
    int max_density;
    int reorder_flag;
    double grow_factor;
    st_table_entry **bins;
};

typedef struct st_generator st_generator;
struct st_generator {
    st_table *table;
    st_table_entry *entry;
    int index;
};

#define st_is_member(table,key) st_lookup(table,key,(char **) 0)
#define st_count(table) ((table)->num_entries)

enum st_retval {ST_CONTINUE, ST_STOP, ST_DELETE};

typedef enum st_retval (*ST_PFSR)();
typedef int (*ST_PFI)();

extern st_table *st_init_table_with_params (ST_PFI, ST_PFI, int, int, double, int);
extern st_table *st_init_table (ST_PFI, ST_PFI); 
extern void st_free_table (st_table *);
extern int st_lookup (st_table *, char *, char **);
extern int st_lookup_int (st_table *, char *, int *);
extern int st_insert (st_table *, char *, char *);
extern int st_add_direct (st_table *, char *, char *);
extern int st_find_or_add (st_table *, char *, char ***);
extern int st_find (st_table *, char *, char ***);
extern st_table *st_copy (st_table *);
extern int st_delete (st_table *, char **, char **);
extern int st_delete_int (st_table *, long *, char **);
extern int st_foreach (st_table *, ST_PFSR, char *);
extern int st_strhash (char *, int);
extern int st_numhash (char *, int);
extern int st_ptrhash (char *, int);
extern int st_numcmp (char *, char *);
extern int st_ptrcmp (char *, char *);
extern st_generator *st_init_gen (st_table *);
extern int st_gen (st_generator *, char **, char **);
extern int st_gen_int (st_generator *, char **, long *);
extern void st_free_gen (st_generator *);


#define ST_DEFAULT_MAX_DENSITY 5
#define ST_DEFAULT_INIT_TABLE_SIZE 11
#define ST_DEFAULT_GROW_FACTOR 2.0
#define ST_DEFAULT_REORDER_FLAG 0

#define st_foreach_item(table, gen, key, value) \
    for(gen=st_init_gen(table); st_gen(gen,key,value) || (st_free_gen(gen),0);)

#define st_foreach_item_int(table, gen, key, value) \
    for(gen=st_init_gen(table); st_gen_int(gen,key,value) || (st_free_gen(gen),0);)

#define ST_OUT_OF_MEM -10000

#ifdef __cplusplus
}
#endif

#endif /* ST_INCLUDED */
