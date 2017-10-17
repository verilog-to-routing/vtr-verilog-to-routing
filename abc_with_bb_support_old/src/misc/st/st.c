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
#include "st.h"

#ifndef ABS
#  define ABS(a)			((a) < 0 ? -(a) : (a))
#endif

#ifndef ALLOC
#define ALLOC(type, num)	 ((type *) malloc(sizeof(type) * (num)))
#endif

#ifndef FREE
#define FREE(obj)		     ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#endif

#ifndef REALLOC
#define REALLOC(type, obj, num)	\
        ((obj) ? ((type *) realloc((char *)(obj), sizeof(type) * (num))) : \
	     ((type *) malloc(sizeof(type) * (num))))
#endif

#define ST_NUMCMP(x,y) ((x) != (y))
#define ST_NUMHASH(x,size) (ABS((long)x)%(size))
#define ST_PTRHASH(x,size) ((int)((unsigned long)(x)>>2)%size)
#define EQUAL(func, x, y) \
    ((((func) == st_numcmp) || ((func) == st_ptrcmp)) ?\
      (ST_NUMCMP((x),(y)) == 0) : ((*func)((x), (y)) == 0))


#define do_hash(key, table)\
    ((table->hash == st_ptrhash) ? ST_PTRHASH((key),(table)->num_bins) :\
     (table->hash == st_numhash) ? ST_NUMHASH((key), (table)->num_bins) :\
     (*table->hash)((key), (table)->num_bins))

static int rehash();
int st_numhash(), st_ptrhash(), st_numcmp(), st_ptrcmp();

st_table *
st_init_table_with_params(compare, hash, size, density, grow_factor,
			  reorder_flag)
int (*compare)();
int (*hash)();
int size;
int density;
double grow_factor;
int reorder_flag;
{
    int i;
    st_table *new;

    new = ALLOC(st_table, 1);
    if (new == NULL) {
	return NULL;
    }
    new->compare = compare;
    new->hash = hash;
    new->num_entries = 0;
    new->max_density = density;
    new->grow_factor = grow_factor;
    new->reorder_flag = reorder_flag;
    if (size <= 0) {
	size = 1;
    }
    new->num_bins = size;
    new->bins = ALLOC(st_table_entry *, size);
    if (new->bins == NULL) {
	FREE(new);
	return NULL;
    }
    for(i = 0; i < size; i++) {
	new->bins[i] = 0;
    }
    return new;
}

st_table *
st_init_table(compare, hash)
int (*compare)();
int (*hash)();
{
    return st_init_table_with_params(compare, hash, ST_DEFAULT_INIT_TABLE_SIZE,
				     ST_DEFAULT_MAX_DENSITY,
				     ST_DEFAULT_GROW_FACTOR,
				     ST_DEFAULT_REORDER_FLAG);
}
			    
void
st_free_table(table)
st_table *table;
{
    register st_table_entry *ptr, *next;
    int i;

    for(i = 0; i < table->num_bins ; i++) {
	ptr = table->bins[i];
	while (ptr != NULL) {
	    next = ptr->next;
	    FREE(ptr);
	    ptr = next;
	}
    }
    FREE(table->bins);
    FREE(table);
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
st_lookup(table, key, value)
st_table *table;
register char *key;
char **value;
{
    int hash_val;
    register st_table_entry *ptr, **last;

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
st_lookup_int(table, key, value)
st_table *table;
register char *key;
int *value;
{
    int hash_val;
    register st_table_entry *ptr, **last;

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
    new = ALLOC(st_table_entry, 1);\
    \
    new->key = key;\
    new->record = value;\
    new->next = table->bins[hash_val];\
    table->bins[hash_val] = new;\
    table->num_entries++;\
}

int
st_insert(table, key, value)
register st_table *table;
register char *key;
char *value;
{
    int hash_val;
    st_table_entry *new;
    register st_table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr, last);

    if (ptr == NULL) {
	if (table->num_entries/table->num_bins >= table->max_density) {
	    if (rehash(table) == ST_OUT_OF_MEM) {
		return ST_OUT_OF_MEM;
	    }
	    hash_val = do_hash(key, table);
	}
	new = ALLOC(st_table_entry, 1);
	if (new == NULL) {
	    return ST_OUT_OF_MEM;
	}
	new->key = key;
	new->record = value;
	new->next = table->bins[hash_val];
	table->bins[hash_val] = new;
	table->num_entries++;
	return 0;
    } else {
	ptr->record = value;
	return 1;
    }
}

int
st_add_direct(table, key, value)
st_table *table;
char *key;
char *value;
{
    int hash_val;
    st_table_entry *new;
    
    hash_val = do_hash(key, table);
    if (table->num_entries / table->num_bins >= table->max_density) {
	if (rehash(table) == ST_OUT_OF_MEM) {
	    return ST_OUT_OF_MEM;
	}
    }
    hash_val = do_hash(key, table);
    new = ALLOC(st_table_entry, 1);
    if (new == NULL) {
	return ST_OUT_OF_MEM;
    }
    new->key = key;
    new->record = value;
    new->next = table->bins[hash_val];
    table->bins[hash_val] = new;
    table->num_entries++;
    return 1;
}

int
st_find_or_add(table, key, slot)
st_table *table;
char *key;
char ***slot;
{
    int hash_val;
    st_table_entry *new, *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr, last);

    if (ptr == NULL) {
	if (table->num_entries / table->num_bins >= table->max_density) {
	    if (rehash(table) == ST_OUT_OF_MEM) {
		return ST_OUT_OF_MEM;
	    }
	    hash_val = do_hash(key, table);
	}
	new = ALLOC(st_table_entry, 1);
	if (new == NULL) {
	    return ST_OUT_OF_MEM;
	}
	new->key = key;
	new->record = (char *) 0;
	new->next = table->bins[hash_val];
	table->bins[hash_val] = new;
	table->num_entries++;
	if (slot != NULL) *slot = &new->record;
	return 0;
    } else {
	if (slot != NULL) *slot = &ptr->record;
	return 1;
    }
}

int
st_find(table, key, slot)
st_table *table;
char *key;
char ***slot;
{
    int hash_val;
    st_table_entry *ptr, **last;

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
rehash(table)
register st_table *table;
{
    register st_table_entry *ptr, *next, **old_bins;
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
    table->bins = ALLOC(st_table_entry *, table->num_bins);
    if (table->bins == NULL) {
	table->bins = old_bins;
	table->num_bins = old_num_bins;
	table->num_entries = old_num_entries;
	return ST_OUT_OF_MEM;
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
    FREE(old_bins);

    return 1;
}

st_table *
st_copy(old_table)
st_table *old_table;
{
    st_table *new_table;
    st_table_entry *ptr, *newptr, *next, *new;
    int i, j, num_bins = old_table->num_bins;

    new_table = ALLOC(st_table, 1);
    if (new_table == NULL) {
	return NULL;
    }
    
    *new_table = *old_table;
    new_table->bins = ALLOC(st_table_entry *, num_bins);
    if (new_table->bins == NULL) {
	FREE(new_table);
	return NULL;
    }
    for(i = 0; i < num_bins ; i++) {
	new_table->bins[i] = NULL;
	ptr = old_table->bins[i];
	while (ptr != NULL) {
	    new = ALLOC(st_table_entry, 1);
	    if (new == NULL) {
		for (j = 0; j <= i; j++) {
		    newptr = new_table->bins[j];
		    while (newptr != NULL) {
			next = newptr->next;
			FREE(newptr);
			newptr = next;
		    }
		}
		FREE(new_table->bins);
		FREE(new_table);
		return NULL;
	    }
	    *new = *ptr;
	    new->next = new_table->bins[i];
	    new_table->bins[i] = new;
	    ptr = ptr->next;
	}
    }
    return new_table;
}

int
st_delete(table, keyp, value)
register st_table *table;
register char **keyp;
char **value;
{
    int hash_val;
    char *key = *keyp;
    register st_table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr ,last);
    
    if (ptr == NULL) {
	return 0;
    }

    *last = ptr->next;
    if (value != NULL) *value = ptr->record;
    *keyp = ptr->key;
    FREE(ptr);
    table->num_entries--;
    return 1;
}

int
st_delete_int(table, keyp, value)
register st_table *table;
register long *keyp;
char **value;
{
    int hash_val;
    char *key = (char *) *keyp;
    register st_table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr ,last);

    if (ptr == NULL) {
        return 0;
    }

    *last = ptr->next;
    if (value != NULL) *value = ptr->record;
    *keyp = (long) ptr->key;
    FREE(ptr);
    table->num_entries--;
    return 1;
}

int
st_foreach(table, func, arg)
st_table *table;
enum st_retval (*func)();
char *arg;
{
    st_table_entry *ptr, **last;
    enum st_retval retval;
    int i;

    for(i = 0; i < table->num_bins; i++) {
	last = &table->bins[i]; ptr = *last;
	while (ptr != NULL) {
	    retval = (*func)(ptr->key, ptr->record, arg);
	    switch (retval) {
	    case ST_CONTINUE:
		last = &ptr->next; ptr = *last;
		break;
	    case ST_STOP:
		return 0;
	    case ST_DELETE:
		*last = ptr->next;
		table->num_entries--;	/* cstevens@ic */
		FREE(ptr);
		ptr = *last;
	    }
	}
    }
    return 1;
}

int
st_strhash(string, modulus)
register char *string;
int modulus;
{
    register int val = 0;
    register int c;
    
    while ((c = *string++) != '\0') {
	val = val*997 + c;
    }

    return ((val < 0) ? -val : val)%modulus;
}

int
st_numhash(x, size)
char *x;
int size;
{
    return ST_NUMHASH(x, size);
}

int
st_ptrhash(x, size)
char *x;
int size;
{
    return ST_PTRHASH(x, size);
}

int
st_numcmp(x, y)
char *x;
char *y;
{
    return ST_NUMCMP(x, y);
}

int
st_ptrcmp(x, y)
char *x;
char *y;
{
    return ST_NUMCMP(x, y);
}

st_generator *
st_init_gen(table)
st_table *table;
{
    st_generator *gen;

    gen = ALLOC(st_generator, 1);
    if (gen == NULL) {
	return NULL;
    }
    gen->table = table;
    gen->entry = NULL;
    gen->index = 0;
    return gen;
}


int 
st_gen(gen, key_p, value_p)
st_generator *gen;
char **key_p;
char **value_p;
{
    register int i;

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
	    return 0;		/* that's all folks ! */
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
st_gen_int(gen, key_p, value_p)
st_generator *gen;
char **key_p;
long *value_p;
{
    register int i;

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
	    return 0;		/* that's all folks ! */
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
st_free_gen(gen)
st_generator *gen;
{
    FREE(gen);
}
