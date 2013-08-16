#define HASHSIZE 5000001

struct s_hash {
	char *name;
	int index;
	int count;
	struct s_hash *next;
};

/* name:  The string referred to by this hash entry.							*
 * index: The integer identifier for this entry.								*
 * count: Number of times an element with this name has been inserted into		*
 *        the table. EXCEPTION: For the structure for blif parsing/reading,		*
 * 	  blif_hash, value of count is the number of pins on this g_atoms_nlist.net *
 * 	  so far.																	*
 * next:  A pointer to the next (string,index) entry that mapped to the			*
 *        same hash value, or NULL if there are no more entries.				*/

struct s_hash_iterator {
	int i;
	struct s_hash *h_ptr;
};

/* i:  current "line" of the hash table.  That is, hash_table[i] is the     *
 *     start of the hash linked list for this hash value.                   *
 * h_ptr:  Pointer to the next hash structure to be examined in the         *
 *         iteration.                                                       */

struct s_hash **alloc_hash_table(void);
void free_hash_table(struct s_hash **hash_table);
struct s_hash_iterator start_hash_table_iterator(void);
struct s_hash *get_next_hash(struct s_hash **hash_table,
		struct s_hash_iterator *hash_iterator);
struct s_hash *insert_in_hash_table(struct s_hash **hash_table, char *name,
		int next_free_index);
struct s_hash *get_hash_entry(struct s_hash **hash_table, char *name);
int hash_value(char *name);
void get_hash_stats(struct s_hash **hash_table, char *hash_table_name);
