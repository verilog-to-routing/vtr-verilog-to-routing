/*
 * Revision Control Information
 *
 * /projects/hsis/CVS/utilities/st/st.c,v
 * serdar
 * 1.1
 * 1993/07/29 01:00:13
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "st.h"

ABC_NAMESPACE_IMPL_START


#define st__NUMCMP(x,y) ((x) != (y))
#define st__NUMHASH(x,size) (Abc_AbsInt((long)x)%(size))
//#define st__PTRHASH(x,size) ((int)((ABC_PTRUINT_T)(x)>>2)%size)  // 64-bit bug fix 9/17/2007
#define st__PTRHASH(x,size) ((int)(((ABC_PTRUINT_T)(x)>>2)%size))
#define EQUAL(func, x, y) \
    ((((func) == st__numcmp) || ((func) == st__ptrcmp)) ?\
      (st__NUMCMP((x),(y)) == 0) : ((*func)((x), (y)) == 0))


#define do_hash(key, table)\
    ((table->hash == st__ptrhash) ? st__PTRHASH((key),(table)->num_bins) :\
     (table->hash == st__numhash) ? st__NUMHASH((key), (table)->num_bins) :\
     (*table->hash)((key), (table)->num_bins))

static int rehash( st__table *table);

int st__numhash(const char*, int);
int st__ptrhash(const char*, int);
int st__numcmp(const char*, const char*);
int st__ptrcmp(const char*, const char*);

 st__table *
 st__init_table_with_params( st__compare_func_type compare, st__hash_func_type hash, int size, int density, double grow_factor, int reorder_flag)
{
    int i;
    st__table *newTable;

    newTable = ABC_ALLOC( st__table, 1);
    if (newTable == NULL) {
    return NULL;
    }
    newTable->compare = compare;
    newTable->hash = hash;
    newTable->num_entries = 0;
    newTable->max_density = density;
    newTable->grow_factor = grow_factor;
    newTable->reorder_flag = reorder_flag;
    if (size <= 0) {
    size = 1;
    }
    newTable->num_bins = size;
    newTable->bins = ABC_ALLOC( st__table_entry *, size);
    if (newTable->bins == NULL) {
    ABC_FREE(newTable);
    return NULL;
    }
    for(i = 0; i < size; i++) {
    newTable->bins[i] = 0;
    }
    return newTable;
}

 st__table *
 st__init_table( st__compare_func_type compare, st__hash_func_type hash)
{
    return st__init_table_with_params(compare, hash, st__DEFAULT_INIT_TABLE_SIZE,
                     st__DEFAULT_MAX_DENSITY,
                     st__DEFAULT_GROW_FACTOR,
                     st__DEFAULT_REORDER_FLAG);
}
                
void
 st__free_table( st__table *table)
{
    st__table_entry *ptr, *next;
    int i;

    for(i = 0; i < table->num_bins ; i++) {
    ptr = table->bins[i];
    while (ptr != NULL) {
        next = ptr->next;
        ABC_FREE(ptr);
        ptr = next;
    }
    }
    ABC_FREE(table->bins);
    ABC_FREE(table);
}

#define PTR_NOT_EQUAL(table, ptr, user_key)\
(ptr != NULL && !EQUAL(table->compare, user_key, (ptr)->key))

#define FIND_ENTRY(table, hash_val, key, ptr, last) \
    (last) = &(table)->bins[hash_val];\
    (ptr) = *(last);\
    while (PTR_NOT_EQUAL((table), (ptr), (key))) {\
    (last) = &(ptr)->next; (ptr) = *(last);\
    }\
    if ((ptr) != NULL && (table)->reorder_flag) {\
    *(last) = (ptr)->next;\
    (ptr)->next = (table)->bins[hash_val];\
    (table)->bins[hash_val] = (ptr);\
    }

int
 st__lookup( st__table *table, const char *key, char **value)
{
    int hash_val;
    st__table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr, last);
    
    if (ptr == NULL) {
    return 0;
    } else {
    if (value != NULL) {
        *value = ptr->record; 
    }
    return 1;
    }
}

int
 st__lookup_int( st__table *table, char *key, int *value)
{
    int hash_val;
    st__table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr, last);
    
    if (ptr == NULL) {
    return 0;
    } else {
    if (value != 0) {
        *value = (long) ptr->record;
    }
    return 1;
    }
}

/* This macro does not check if memory allocation fails. Use at you own risk */
#define ADD_DIRECT(table, key, value, hash_val, new)\
{\
    if (table->num_entries/table->num_bins >= table->max_density) {\
    rehash(table);\
    hash_val = do_hash(key,table);\
    }\
    \
    new = ABC_ALLOC( st__table_entry, 1);\
    \
    new->key = key;\
    new->record = value;\
    new->next = table->bins[hash_val];\
    table->bins[hash_val] = new;\
    table->num_entries++;\
}

int
 st__insert( st__table *table, const char *key, char *value)
{
    int hash_val;
    st__table_entry *newEntry;
    st__table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr, last);

    if (ptr == NULL) {
    if (table->num_entries/table->num_bins >= table->max_density) {
        if (rehash(table) == st__OUT_OF_MEM) {
        return st__OUT_OF_MEM;
        }
        hash_val = do_hash(key, table);
    }
    newEntry = ABC_ALLOC( st__table_entry, 1);
    if (newEntry == NULL) {
        return st__OUT_OF_MEM;
    }
    newEntry->key = (char *)key;
    newEntry->record = value;
    newEntry->next = table->bins[hash_val];
    table->bins[hash_val] = newEntry;
    table->num_entries++;
    return 0;
    } else {
    ptr->record = value;
    return 1;
    }
}

int
 st__add_direct( st__table *table, char *key, char *value)
{
    int hash_val;
    st__table_entry *newEntry;
    
    hash_val = do_hash(key, table);
    if (table->num_entries / table->num_bins >= table->max_density) {
    if (rehash(table) == st__OUT_OF_MEM) {
        return st__OUT_OF_MEM;
    }
    }
    hash_val = do_hash(key, table);
    newEntry = ABC_ALLOC( st__table_entry, 1);
    if (newEntry == NULL) {
    return st__OUT_OF_MEM;
    }
    newEntry->key = key;
    newEntry->record = value;
    newEntry->next = table->bins[hash_val];
    table->bins[hash_val] = newEntry;
    table->num_entries++;
    return 1;
}

int
 st__find_or_add( st__table *table, char *key, char ***slot)
{
    int hash_val;
    st__table_entry *newEntry, *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr, last);

    if (ptr == NULL) {
    if (table->num_entries / table->num_bins >= table->max_density) {
        if (rehash(table) == st__OUT_OF_MEM) {
        return st__OUT_OF_MEM;
        }
        hash_val = do_hash(key, table);
    }
    newEntry = ABC_ALLOC( st__table_entry, 1);
    if (newEntry == NULL) {
        return st__OUT_OF_MEM;
    }
    newEntry->key = key;
    newEntry->record = (char *) 0;
    newEntry->next = table->bins[hash_val];
    table->bins[hash_val] = newEntry;
    table->num_entries++;
    if (slot != NULL) *slot = &newEntry->record;
    return 0;
    } else {
    if (slot != NULL) *slot = &ptr->record;
    return 1;
    }
}

int
 st__find( st__table *table, char *key, char ***slot)
{
    int hash_val;
    st__table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr, last);

    if (ptr == NULL) {
    return 0;
    } else {
    if (slot != NULL) {
        *slot = &ptr->record;
    }
    return 1;
    }
}

static int
rehash( st__table *table)
{
    st__table_entry *ptr, *next, **old_bins;
    int             i, old_num_bins, hash_val, old_num_entries;

    /* save old values */
    old_bins = table->bins;
    old_num_bins = table->num_bins;
    old_num_entries = table->num_entries;

    /* rehash */
    table->num_bins = (int)(table->grow_factor * old_num_bins);
    if (table->num_bins % 2 == 0) {
    table->num_bins += 1;
    }
    table->num_entries = 0;
    table->bins = ABC_ALLOC( st__table_entry *, table->num_bins);
    if (table->bins == NULL) {
    table->bins = old_bins;
    table->num_bins = old_num_bins;
    table->num_entries = old_num_entries;
    return st__OUT_OF_MEM;
    }
    /* initialize */
    for (i = 0; i < table->num_bins; i++) {
    table->bins[i] = 0;
    }

    /* copy data over */
    for (i = 0; i < old_num_bins; i++) {
    ptr = old_bins[i];
    while (ptr != NULL) {
        next = ptr->next;
        hash_val = do_hash(ptr->key, table);
        ptr->next = table->bins[hash_val];
        table->bins[hash_val] = ptr;
        table->num_entries++;
        ptr = next;
    }
    }
    ABC_FREE(old_bins);

    return 1;
}

 st__table *
 st__copy( st__table *old_table)
{
    st__table *newEntry_table;
    st__table_entry *ptr, *newEntryptr, *next, *newEntry;
    int i, j, num_bins = old_table->num_bins;

    newEntry_table = ABC_ALLOC( st__table, 1);
    if (newEntry_table == NULL) {
    return NULL;
    }
    
    *newEntry_table = *old_table;
    newEntry_table->bins = ABC_ALLOC( st__table_entry *, num_bins);
    if (newEntry_table->bins == NULL) {
    ABC_FREE(newEntry_table);
    return NULL;
    }
    for(i = 0; i < num_bins ; i++) {
    newEntry_table->bins[i] = NULL;
    ptr = old_table->bins[i];
    while (ptr != NULL) {
        newEntry = ABC_ALLOC( st__table_entry, 1);
        if (newEntry == NULL) {
        for (j = 0; j <= i; j++) {
            newEntryptr = newEntry_table->bins[j];
            while (newEntryptr != NULL) {
            next = newEntryptr->next;
            ABC_FREE(newEntryptr);
            newEntryptr = next;
            }
        }
        ABC_FREE(newEntry_table->bins);
        ABC_FREE(newEntry_table);
        return NULL;
        }
        *newEntry = *ptr;
        newEntry->next = newEntry_table->bins[i];
        newEntry_table->bins[i] = newEntry;
        ptr = ptr->next;
    }
    }
    return newEntry_table;
}

int
 st__delete( st__table *table, const char **keyp, char **value)
{
    int hash_val;
    const char *key = *keyp;
    st__table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr ,last);
    
    if (ptr == NULL) {
    return 0;
    }

    *last = ptr->next;
    if (value != NULL) *value = ptr->record;
    *keyp = ptr->key;
    ABC_FREE(ptr);
    table->num_entries--;
    return 1;
}

int
 st__delete_int( st__table *table, long *keyp, char **value)
{
    int hash_val;
    char *key = (char *) *keyp;
    st__table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr ,last);

    if (ptr == NULL) {
        return 0;
    }

    *last = ptr->next;
    if (value != NULL) *value = ptr->record;
    *keyp = (long) ptr->key;
    ABC_FREE(ptr);
    table->num_entries--;
    return 1;
}

int
 st__foreach( st__table *table, enum st__retval (*func)(char *, char *, char *), char *arg)
{
    st__table_entry *ptr, **last;
    enum st__retval retval;
    int i;

    for(i = 0; i < table->num_bins; i++) {
    last = &table->bins[i]; ptr = *last;
    while (ptr != NULL) {
        retval = (*func)(ptr->key, ptr->record, arg);
        switch (retval) {
        case st__CONTINUE:
        last = &ptr->next; ptr = *last;
        break;
        case st__STOP:
        return 0;
        case st__DELETE:
        *last = ptr->next;
        table->num_entries--;   /* cstevens@ic */
        ABC_FREE(ptr);
        ptr = *last;
        }
    }
    }
    return 1;
}

int
 st__strhash(const char *string, int modulus)
{
    unsigned char * ustring = (unsigned char *)string;
    unsigned c, val = 0;
    assert( modulus > 0 );    
    while ((c = *ustring++) != '\0') {
        val = val*997 + c;
    }
    return (int)(val%modulus);
}

int
 st__numhash(const char *x, int size)
{
    return st__NUMHASH(x, size);
}

int
 st__ptrhash(const char *x, int size)
{
    return st__PTRHASH(x, size);
}

int
 st__numcmp(const char *x, const char *y)
{
    return st__NUMCMP(x, y);
}

int
 st__ptrcmp(const char *x, const char *y)
{
    return st__NUMCMP(x, y);
}

 st__generator *
 st__init_gen( st__table *table)
{
    st__generator *gen;

    gen = ABC_ALLOC( st__generator, 1);
    if (gen == NULL) {
    return NULL;
    }
    gen->table = table;
    gen->entry = NULL;
    gen->index = 0;
    return gen;
}


int 
 st__gen( st__generator *gen, const char **key_p, char **value_p)
{
    int i;

    if (gen->entry == NULL) {
    /* try to find next entry */
    for(i = gen->index; i < gen->table->num_bins; i++) {
        if (gen->table->bins[i] != NULL) {
        gen->index = i+1;
        gen->entry = gen->table->bins[i];
        break;
        }
    }
    if (gen->entry == NULL) {
        return 0;       /* that's all folks ! */
    }
    }
    *key_p = gen->entry->key;
    if (value_p != 0) {
    *value_p = gen->entry->record;
    }
    gen->entry = gen->entry->next;
    return 1;
}


int 
 st__gen_int( st__generator *gen, const char **key_p, long *value_p)
{
    int i;

    if (gen->entry == NULL) {
    /* try to find next entry */
    for(i = gen->index; i < gen->table->num_bins; i++) {
        if (gen->table->bins[i] != NULL) {
        gen->index = i+1;
        gen->entry = gen->table->bins[i];
        break;
        }
    }
    if (gen->entry == NULL) {
        return 0;       /* that's all folks ! */
    }
    }
    *key_p = gen->entry->key;
    if (value_p != 0) {
    *value_p = (long) gen->entry->record;
    }
    gen->entry = gen->entry->next;
    return 1;
}


void
 st__free_gen( st__generator *gen)
{
    ABC_FREE(gen);
}

ABC_NAMESPACE_IMPL_END

