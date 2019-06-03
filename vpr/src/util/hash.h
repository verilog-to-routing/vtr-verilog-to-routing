#define HASHSIZE 5000001

struct t_hash {
    char* name;
    int index;
    int count;
    t_hash* next;
};

/* name:  The string referred to by this hash entry.							*
 * index: The integer identifier for this entry.								*
 * count: Number of times an element with this name has been inserted into		*
 *        the table.    														*
 * next:  A pointer to the next (string,index) entry that mapped to the			*
 *        same hash value, or NULL if there are no more entries.				*/

struct t_hash_iterator {
    int i;
    t_hash* h_ptr;
};

/* i:  current "line" of the hash table.  That is, hash_table[i] is the     *
 *     start of the hash linked list for this hash value.                   *
 * h_ptr:  Pointer to the next hash structure to be examined in the         *
 *         iteration.                                                       */

t_hash** alloc_hash_table();
void free_hash_table(t_hash** hash_table);
t_hash_iterator start_hash_table_iterator();
t_hash* get_next_hash(t_hash** hash_table,
                      t_hash_iterator* hash_iterator);
t_hash* insert_in_hash_table(t_hash** hash_table, const char* name, int next_free_index);
t_hash* get_hash_entry(t_hash** hash_table, const char* name);
int hash_value(const char* name);
void get_hash_stats(t_hash** hash_table, char* hash_table_name);
