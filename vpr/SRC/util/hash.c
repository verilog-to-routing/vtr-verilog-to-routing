#include <cstdlib>
#include <cstring>
using namespace std;

#include "vtr_memory.h"
#include "vtr_log.h"

#include "hash.h"

struct s_hash **
alloc_hash_table(void) {

	/* Creates a hash table with HASHSIZE different locations (hash values).   */

	struct s_hash **hash_table;

	hash_table = (struct s_hash **) vtr::calloc(sizeof(struct s_hash *),
			HASHSIZE);
	return (hash_table);
}

void free_hash_table(struct s_hash **hash_table) {

	/* Frees all the storage associated with a hash table. */

	int i;
	struct s_hash *h_ptr, *temp_ptr;

	for (i = 0; i < HASHSIZE; i++) {
		h_ptr = hash_table[i];
		while (h_ptr != NULL) {
			free(h_ptr->name);
			temp_ptr = h_ptr->next;
			free(h_ptr);
			h_ptr = temp_ptr;
		}
	}

	free(hash_table);
}

struct s_hash_iterator start_hash_table_iterator(void) {

	/* Call this routine before you start going through all the elements in    *
	 * a hash table.  It sets the internal indices to the start of the table.  */

	struct s_hash_iterator hash_iterator;

	hash_iterator.i = -1;
	hash_iterator.h_ptr = NULL;
	return (hash_iterator);
}

struct s_hash *
get_next_hash(struct s_hash **hash_table, struct s_hash_iterator *hash_iterator) {

	/* Returns the next occupied hash entry, and moves the iterator structure    *
	 * forward so the next call gets the next entry.                             */

	int i;
	struct s_hash *h_ptr;

	i = hash_iterator->i;
	h_ptr = hash_iterator->h_ptr;

	while (h_ptr == NULL) {
		i++;
		if (i >= HASHSIZE)
			return (NULL); /* End of table */

		h_ptr = hash_table[i];
	}

	hash_iterator->h_ptr = h_ptr->next;
	hash_iterator->i = i;
	return (h_ptr);
}

struct s_hash *
insert_in_hash_table(struct s_hash **hash_table, const char *name,
		int next_free_index) {

	/* Adds the string pointed to by name to the hash table, and returns the    *
	 * hash structure created or updated.  If name is already in the hash table *
	 * the count member of that hash element is incremented.  Otherwise a new   *
	 * hash entry with a count of zero and an index of next_free_index is       *
	 * created.                                                                 */

	int i;
	struct s_hash *h_ptr, *prev_ptr;

	i = hash_value(name);
	prev_ptr = NULL;
	h_ptr = hash_table[i];

	while (h_ptr != NULL) {
		if (strcmp(h_ptr->name, name) == 0) {
			h_ptr->count++;
			return (h_ptr);
		}

		prev_ptr = h_ptr;
		h_ptr = h_ptr->next;
	}

	/* Name string wasn't in the hash table.  Add it. */

	h_ptr = (struct s_hash *) vtr::malloc(sizeof(struct s_hash));
	if (prev_ptr == NULL) {
		hash_table[i] = h_ptr;
	} else {
		prev_ptr->next = h_ptr;
	}
	h_ptr->next = NULL;
	h_ptr->index = next_free_index;
	h_ptr->count = 1;
	h_ptr->name = (char *) vtr::malloc((strlen(name) + 1) * sizeof(char));
	strcpy(h_ptr->name, name);
	return (h_ptr);
}

struct s_hash *
get_hash_entry(struct s_hash **hash_table, const char *name) {

	/* Returns the hash entry with this name, or NULL if there is no            *
	 * corresponding entry.                                                     */

	int i;
	struct s_hash *h_ptr;

	i = hash_value(name);
	h_ptr = hash_table[i];

	while (h_ptr != NULL) {
		if (strcmp(h_ptr->name, name) == 0)
			return (h_ptr);

		h_ptr = h_ptr->next;
	}

	return (NULL);
}

int hash_value(const char *name) {
	/* Creates a hash key from a character string.  The absolute value is taken  *
	 * for the final val to compensate for long strlen that cause val to 	     *
	 * overflow.								     */

	int i;
	int val = 0, mult = 1;

	i = strlen(name);
	for (i = strlen(name) - 1; i >= 0; i--) {
		val += mult * ((int) name[i]);
		mult *= 7;
	}
	val += (int) name[0];
	val %= HASHSIZE;
	
	val = abs(val);
	return (val);
}

void get_hash_stats(struct s_hash **hash_table, char *hash_table_name){

	/* Checks to see how well elements are distributed within the hash table.     *
	 * Will traverse through the hash_table and count the length of the linked    *
	 * list. Will output the hash number, the number of array elements that are   *
	 * NULL, the average number of linked lists and the maximum length of linked  * 
	 * lists.								      */

	int num_NULL = 0, total_elements = 0,  max_num = 0, curr_num;
	double avg_num = 0;
	int i;
	struct s_hash *h_ptr;

	for (i = 0; i<HASHSIZE; i++){
	h_ptr = hash_table[i];
	curr_num = 0;
	
	if (h_ptr == NULL)
		num_NULL++;		
	else{
		while (h_ptr != NULL){
			curr_num ++;		
			h_ptr = h_ptr->next;
		}
	}

	if (curr_num > max_num)
		max_num = curr_num;
	
	total_elements = total_elements + curr_num;
	}

	avg_num = (float) total_elements / ((float)HASHSIZE - (float)num_NULL);
	
	vtr::printf_info("\n");
	vtr::printf_info("The hash table '%s' is of size %d.\n",
			hash_table_name, HASHSIZE);
	vtr::printf_info("It has: %d keys that are never used; total of %d elements; "
			"an average linked-list length of %.1f; and a maximum linked-list length of %d.\n", 
			num_NULL, total_elements, avg_num, max_num); 
	vtr::printf_info("\n");
}
