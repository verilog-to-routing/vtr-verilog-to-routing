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
#include "misc/extra/extra.h"
#include "stmm.h"

ABC_NAMESPACE_IMPL_START


#define STMM_NUMCMP(x,y) ((x) != (y))
#define STMM_NUMHASH(x,size) (Abc_AbsInt((long)x)%(size))
//#define STMM_PTRHASH(x,size) ((int)((ABC_PTRUINT_T)(x)>>2)%size) //  64-bit bug fix 9/17/2007
#define STMM_PTRHASH(x,size) ((int)(((ABC_PTRUINT_T)(x)>>2)%size))
#define EQUAL(func, x, y) \
    ((((func) == stmm_numcmp) || ((func) == stmm_ptrcmp)) ?\
      (STMM_NUMCMP((x),(y)) == 0) : ((*func)((x), (y)) == 0))


#define do_hash(key, table)\
    ((table->hash == stmm_ptrhash) ? STMM_PTRHASH((key),(table)->num_bins) :\
     (table->hash == stmm_numhash) ? STMM_NUMHASH((key), (table)->num_bins) :\
     (*table->hash)((key), (table)->num_bins))

static int rehash (stmm_table *table);
//int stmm_numhash (), stmm_ptrhash (), stmm_numcmp (), stmm_ptrcmp ();

stmm_table *
stmm_init_table_with_params (stmm_compare_func_type compare, stmm_hash_func_type hash, int size, int density, double grow_factor, int reorder_flag)
{
    int i;
    stmm_table *newTable;

    newTable = ABC_ALLOC(stmm_table, 1);
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
    newTable->bins = ABC_ALLOC(stmm_table_entry *, size);
    if (newTable->bins == NULL) {
    ABC_FREE(newTable);
    return NULL;
    }
    for (i = 0; i < size; i++) {
    newTable->bins[i] = 0;
    }

    // added by alanmi
    newTable->pMemMan = Extra_MmFixedStart(sizeof (stmm_table_entry));
    return newTable;
}

stmm_table *
stmm_init_table (stmm_compare_func_type compare, stmm_hash_func_type hash)
{
    return stmm_init_table_with_params (compare, hash,
                    STMM_DEFAULT_INIT_TABLE_SIZE,
                    STMM_DEFAULT_MAX_DENSITY,
                    STMM_DEFAULT_GROW_FACTOR,
                    STMM_DEFAULT_REORDER_FLAG);
}

void
stmm_free_table (stmm_table *table)
{
/*
    stmm_table_entry *ptr, *next;
    int i;
    for ( i = 0; i < table->num_bins; i++ )
    {
        ptr = table->bins[i];
        while ( ptr != NULL )
        {
            next = ptr->next;
            ABC_FREE( ptr );
            ptr = next;
        }
    }
*/
    // no need to deallocate entries because they are in the memory manager now
    // added by alanmi
    if ( table->pMemMan )
        Extra_MmFixedStop ((Extra_MmFixed_t *)table->pMemMan);
    ABC_FREE(table->bins);
    ABC_FREE(table);
}

// this function recycles all the bins
void
stmm_clean (stmm_table *table)
{
    int i;
    // clean the bins
    for (i = 0; i < table->num_bins; i++)
        table->bins[i] = NULL;
    // reset the parameters
    table->num_entries = 0;
    // restart the memory manager
    Extra_MmFixedRestart ((Extra_MmFixed_t *)table->pMemMan);
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
stmm_lookup (stmm_table *table, char *key, char **value)
{
    int hash_val;
    stmm_table_entry *ptr, **last;

    hash_val = do_hash (key, table);

    FIND_ENTRY (table, hash_val, key, ptr, last);

    if (ptr == NULL) {
    return 0;
    }
    else {
    if (value != NULL)
    {
        *value = ptr->record;
    }
    return 1;
    }
}

int
stmm_lookup_int (stmm_table *table, char *key, int *value)
{
    int hash_val;
    stmm_table_entry *ptr, **last;

    hash_val = do_hash (key, table);

    FIND_ENTRY (table, hash_val, key, ptr, last);

    if (ptr == NULL) {
    return 0;
    }
    else {
    if (value != 0)
    {
        *value = (long) ptr->record;
    }
    return 1;
    }
}

// This macro contained a line
//    new = ABC_ALLOC(stmm_table_entry, 1);
// which was modified by alanmi


/* This macro does not check if memory allocation fails. Use at you own risk */
#define ADD_DIRECT(table, key, value, hash_val, new)\
{\
    if (table->num_entries/table->num_bins >= table->max_density) {\
    rehash(table);\
    hash_val = do_hash(key,table);\
    }\
    \
    new = (stmm_table_entry *)Extra_MmFixedEntryFetch( (Extra_MmFixed_t *)table->pMemMan );\
    \
    new->key = key;\
    new->record = value;\
    new->next = table->bins[hash_val];\
    table->bins[hash_val] = new;\
    table->num_entries++;\
}

int
stmm_insert (stmm_table *table, char *key, char *value)
{
    int hash_val;
    stmm_table_entry *newEntry;
    stmm_table_entry *ptr, **last;

    hash_val = do_hash (key, table);

    FIND_ENTRY (table, hash_val, key, ptr, last);

    if (ptr == NULL) {
    if (table->num_entries / table->num_bins >= table->max_density) {
        if (rehash (table) == STMM_OUT_OF_MEM) {
        return STMM_OUT_OF_MEM;
        }
        hash_val = do_hash (key, table);
    }

//              newEntry = ABC_ALLOC( stmm_table_entry, 1 );
    newEntry = (stmm_table_entry *) Extra_MmFixedEntryFetch ((Extra_MmFixed_t *)table->pMemMan);
    if (newEntry == NULL) {
        return STMM_OUT_OF_MEM;
    }

    newEntry->key = key;
    newEntry->record = value;
    newEntry->next = table->bins[hash_val];
    table->bins[hash_val] = newEntry;
    table->num_entries++;
    return 0;
    }
    else {
    ptr->record = value;
    return 1;
    }
}

int
stmm_add_direct (stmm_table *table, char *key, char *value)
{
    int hash_val;
    stmm_table_entry *newEntry;

    hash_val = do_hash (key, table);
    if (table->num_entries / table->num_bins >= table->max_density) {
    if (rehash (table) == STMM_OUT_OF_MEM) {
        return STMM_OUT_OF_MEM;
    }
    }
    hash_val = do_hash (key, table);

//      newEntry = ABC_ALLOC( stmm_table_entry, 1 );
    newEntry = (stmm_table_entry *) Extra_MmFixedEntryFetch ((Extra_MmFixed_t *)table->pMemMan);
    if (newEntry == NULL) {
    return STMM_OUT_OF_MEM;
    }

    newEntry->key = key;
    newEntry->record = value;
    newEntry->next = table->bins[hash_val];
    table->bins[hash_val] = newEntry;
    table->num_entries++;
    return 1;
}

int
stmm_find_or_add (stmm_table *table, char *key, char ***slot)
{
    int hash_val;
    stmm_table_entry *newEntry, *ptr, **last;

    hash_val = do_hash (key, table);

    FIND_ENTRY (table, hash_val, key, ptr, last);

    if (ptr == NULL) {
    if (table->num_entries / table->num_bins >= table->max_density) {
        if (rehash (table) == STMM_OUT_OF_MEM) {
        return STMM_OUT_OF_MEM;
        }
        hash_val = do_hash (key, table);
    }

    // newEntry = ABC_ALLOC( stmm_table_entry, 1 );
    newEntry = (stmm_table_entry *) Extra_MmFixedEntryFetch ((Extra_MmFixed_t *)table->pMemMan);
    if (newEntry == NULL) {
        return STMM_OUT_OF_MEM;
    }

    newEntry->key = key;
    newEntry->record = (char *) 0;
    newEntry->next = table->bins[hash_val];
    table->bins[hash_val] = newEntry;
    table->num_entries++;
    if (slot != NULL)
         *slot = &newEntry->record;
    return 0;
    }
    else {
    if (slot != NULL)
         *slot = &ptr->record;
    return 1;
    }
}

int
stmm_find (stmm_table *table, char *key, char ***slot)
{
    int hash_val;
    stmm_table_entry *ptr, **last;

    hash_val = do_hash (key, table);

    FIND_ENTRY (table, hash_val, key, ptr, last);

    if (ptr == NULL) {
    return 0;
    }
    else {
    if (slot != NULL)
    {
        *slot = &ptr->record;
    }
    return 1;
    }
}

static int
rehash (stmm_table *table)
{
    stmm_table_entry *ptr, *next, **old_bins;
    int i, old_num_bins, hash_val, old_num_entries;

    /* save old values */
    old_bins = table->bins;
    old_num_bins = table->num_bins;
    old_num_entries = table->num_entries;

    /* rehash */
    table->num_bins = (int) (table->grow_factor * old_num_bins);
    if (table->num_bins % 2 == 0) {
    table->num_bins += 1;
    }
    table->num_entries = 0;
    table->bins = ABC_ALLOC(stmm_table_entry *, table->num_bins);
    if (table->bins == NULL) {
    table->bins = old_bins;
    table->num_bins = old_num_bins;
    table->num_entries = old_num_entries;
    return STMM_OUT_OF_MEM;
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
        hash_val = do_hash (ptr->key, table);
        ptr->next = table->bins[hash_val];
        table->bins[hash_val] = ptr;
        table->num_entries++;
        ptr = next;
    }
    }
    ABC_FREE(old_bins);

    return 1;
}

stmm_table *
stmm_copy (stmm_table *old_table)
{
    stmm_table *newEntry_table;
    stmm_table_entry *ptr, /* *newEntryptr, *next, */ *newEntry;
    int i, /*j, */ num_bins = old_table->num_bins;

    newEntry_table = ABC_ALLOC(stmm_table, 1);
    if (newEntry_table == NULL) {
    return NULL;
    }

    *newEntry_table = *old_table;
    newEntry_table->bins = ABC_ALLOC(stmm_table_entry *, num_bins);
    if (newEntry_table->bins == NULL) {
    ABC_FREE(newEntry_table);
    return NULL;
    }

    // allocate the memory manager for the newEntry table
    newEntry_table->pMemMan = Extra_MmFixedStart (sizeof (stmm_table_entry));

    for (i = 0; i < num_bins; i++) {
    newEntry_table->bins[i] = NULL;
    ptr = old_table->bins[i];
    while (ptr != NULL) {
//                      newEntry = ABC_ALLOC( stmm_table_entry, 1 );
        newEntry = (stmm_table_entry *)Extra_MmFixedEntryFetch ((Extra_MmFixed_t *)newEntry_table->pMemMan);
        if (newEntry == NULL) {
/*
                for ( j = 0; j <= i; j++ )
                {
                    newEntryptr = newEntry_table->bins[j];
                    while ( newEntryptr != NULL )
                    {
                        next = newEntryptr->next;
                        ABC_FREE( newEntryptr );
                        newEntryptr = next;
                    }
                }
*/
        Extra_MmFixedStop ((Extra_MmFixed_t *)newEntry_table->pMemMan);

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
stmm_delete (stmm_table *table, char **keyp, char **value)
{
    int hash_val;
    char *key = *keyp;
    stmm_table_entry *ptr, **last;

    hash_val = do_hash (key, table);

    FIND_ENTRY (table, hash_val, key, ptr, last);

    if (ptr == NULL) {
    return 0;
    }

    *last = ptr->next;
    if (value != NULL)
     *value = ptr->record;
    *keyp = ptr->key;
//      ABC_FREE( ptr );
    Extra_MmFixedEntryRecycle ((Extra_MmFixed_t *)table->pMemMan, (char *) ptr);

    table->num_entries--;
    return 1;
}

int
stmm_delete_int (stmm_table *table, long *keyp, char **value)
{
    int hash_val;
    char *key = (char *) *keyp;
    stmm_table_entry *ptr, **last;

    hash_val = do_hash (key, table);

    FIND_ENTRY (table, hash_val, key, ptr, last);

    if (ptr == NULL) {
    return 0;
    }

    *last = ptr->next;
    if (value != NULL)
     *value = ptr->record;
    *keyp = (long) ptr->key;
//      ABC_FREE( ptr );
    Extra_MmFixedEntryRecycle ((Extra_MmFixed_t *)table->pMemMan, (char *) ptr);

    table->num_entries--;
    return 1;
}

int
stmm_foreach (stmm_table *table, enum stmm_retval (*func) (char *, char *, char *), char *arg)
{
    stmm_table_entry *ptr, **last;
    enum stmm_retval retval;
    int i;

    for (i = 0; i < table->num_bins; i++) {
    last = &table->bins[i];
    ptr = *last;
    while (ptr != NULL) {
        retval = (*func) (ptr->key, ptr->record, arg);
        switch (retval) {
        case STMM_CONTINUE:
        last = &ptr->next;
        ptr = *last;
        break;
        case STMM_STOP:
        return 0;
        case STMM_DELETE:
        *last = ptr->next;
        table->num_entries--;   /* cstevens@ic */
//                              ABC_FREE( ptr );
        Extra_MmFixedEntryRecycle ((Extra_MmFixed_t *)table->pMemMan, (char *) ptr);

        ptr = *last;
        }
    }
    }
    return 1;
}

int
stmm_strhash (const char *string, int modulus)
{
    int val = 0;
    int c;

    while ((c = *string++) != '\0') {
    val = val * 997 + c;
    }

    return ((val < 0) ? -val : val) % modulus;
}

int
stmm_numhash (const char *x, int size)
{
    return STMM_NUMHASH (x, size);
}

int
stmm_ptrhash (const char *x, int size)
{
    return STMM_PTRHASH (x, size);
}

int
stmm_numcmp (const char *x, const char *y)
{
    return STMM_NUMCMP (x, y);
}

int
stmm_ptrcmp (const char *x, const char *y)
{
    return STMM_NUMCMP (x, y);
}

stmm_generator *
stmm_init_gen (stmm_table *table)
{
    stmm_generator *gen;

    gen = ABC_ALLOC(stmm_generator, 1);
    if (gen == NULL) {
    return NULL;
    }
    gen->table = table;
    gen->entry = NULL;
    gen->index = 0;
    return gen;
}


int
stmm_gen (stmm_generator *gen, char **key_p, char **value_p)
{
    int i;

    if (gen->entry == NULL) {
    /* try to find next entry */
    for (i = gen->index; i < gen->table->num_bins; i++) {
        if (gen->table->bins[i] != NULL) {
        gen->index = i + 1;
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
stmm_gen_int (stmm_generator *gen, char **key_p, long *value_p)
{
    int i;

    if (gen->entry == NULL) {
    /* try to find next entry */
    for (i = gen->index; i < gen->table->num_bins; i++) {
        if (gen->table->bins[i] != NULL) {
        gen->index = i + 1;
        gen->entry = gen->table->bins[i];
        break;
        }
    }
    if (gen->entry == NULL) {
        return 0;       /* that's all folks ! */
    }
    }
    *key_p = gen->entry->key;
    if (value_p != 0)
    {
    *value_p = (long) gen->entry->record;
    }
    gen->entry = gen->entry->next;
    return 1;
}


void
stmm_free_gen (stmm_generator *gen)
{
    ABC_FREE(gen);
}
ABC_NAMESPACE_IMPL_END

