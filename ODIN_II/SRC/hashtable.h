/*
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef HASHTABLE_H
#define HASHTABLE_H
#define hashtable_t struct hashtable_t_t
#define hashtable_node_t struct hashtable_node_t_t
#include<sys/types.h>
#include<stdint.h>

// Constructor 
hashtable_t* create_hashtable(int store_size);

struct hashtable_t_t
{
	int count;
	int store_size;
	hashtable_node_t **store;	

	// Adds an item to the hashtable.
	void   (*add)                (hashtable_t *h, void *key, size_t key_length, void *item);
	// Removes an item from the hashtable. If the item is not present, a null pointer is returned.
	void*  (*remove)             (hashtable_t *h, void *key, size_t key_length);
	// Gets an item from the hashtable without removing it. If the item is not present, a null pointer is returned.
	void*  (*get)                (hashtable_t *h, void *key, size_t key_length);
	// Gets an array of all items in the hastable.
	void** (*get_all)            (hashtable_t *h);
	// Check to see if the hashtable is empty.
	int    (*is_empty)           (hashtable_t *h);
	// Destroys the hashtable but does not free the memory used by the items added or the keys.
	void   (*destroy)            (hashtable_t *h);
	// Destroys the hashtable and calls free on each item.
	void   (*destroy_free_items) (hashtable_t *h);
};

struct hashtable_node_t_t
{
	size_t key_length;  
	void *key;	
	void *item;
	hashtable_node_t *next;
};

#endif
