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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"
#include "types.h"

void         ___hashtable_add                (hashtable_t *h, void *key, size_t key_length, void *item);
void*        ___hashtable_remove             (hashtable_t *h, void *key, size_t key_length);
void*        ___hashtable_get                (hashtable_t *h, void *key, size_t key_length);
void**       ___hashtable_get_all            (hashtable_t *h);
int          ___hashtable_is_empty           (hashtable_t *h);
void         ___hashtable_destroy            (hashtable_t *h);
void         ___hashtable_destroy_free_items (hashtable_t *h);
int          ___hashtable_compare_keys       (void *key, size_t key_len, void* key1, size_t key_len1);
unsigned int ___hashtable_hash               (void *key, size_t key_len, int max_key);

hashtable_t* create_hashtable(int store_size)
{
	if (store_size < 1)
	{
		printf("ERROR: Attempted to create a hashtable_t with a store size less than 1.\n");
		exit(1);
	}

	hashtable_t *h = (hashtable_t *)malloc(sizeof(hashtable_t));
	
	h->store_size = store_size; 
	h->store = calloc(store_size, sizeof(hashtable_node_t*)); 
	h->count = 0;

	h->add                = ___hashtable_add;
	h->get                = ___hashtable_get;
	h->get_all            = ___hashtable_get_all;
	h->remove             = ___hashtable_remove;
	h->is_empty           = ___hashtable_is_empty;
	h->destroy            = ___hashtable_destroy;
	h->destroy_free_items = ___hashtable_destroy_free_items;
	
	return h;
}

void ___hashtable_destroy(hashtable_t *h)
{
	int i; 
	for (i = 0; i < h->store_size; i++)
	{
		hashtable_node_t* node; 
		while((node = h->store[i]))
		{
			h->store[i] = node->next; 
			free(node->key);
			free(node); 
			h->count--; 
		}
	} 
	free(h->store);
	free(h);
}

void ___hashtable_destroy_free_items(hashtable_t *h)
{
	int i;
	for (i = 0; i < h->store_size; i++)
	{
		hashtable_node_t* node;
		while((node = h->store[i]))
		{
			free(node->item);
			h->store[i] = node->next;
			free(node->key);
			free(node);
			h->count--;
		}
	}
	free(h->store);
	free(h);
}

void  ___hashtable_add(hashtable_t *h, void *key, size_t key_length, void *item)
{
	hashtable_node_t *node = (hashtable_node_t *)malloc(sizeof(hashtable_node_t));

	node->key_length = key_length; 
	node->key        = malloc(key_length);
	node->item       = item;
	node->next       = NULL;	

	memcpy(node->key, key, key_length);
	
	unsigned int i = ___hashtable_hash(key, key_length, h->store_size);
	hashtable_node_t **location = h->store + i; 

	while(*location)
		location = &((*location)->next);		
	
	*location = node; 	

	h->count++;
}

void* ___hashtable_remove(hashtable_t *h, void *key, size_t key_length)
{
	unsigned int i = ___hashtable_hash(key, key_length, h->store_size); 

	hashtable_node_t **node_location = h->store + i; 
	hashtable_node_t  *node          = *node_location; 
	while(node && !___hashtable_compare_keys(key, key_length, node->key, node->key_length))
	{
		node_location = &(node->next); 
		node          = *node_location; 
	}
	
	void *item = NULL; 
	if (node)
	{
		item = node->item; 
		*node_location = node->next;
		free(node->key);
		free(node); 		 
		h->count--; 
	}
	
	return item;
}

void* ___hashtable_get(hashtable_t *h, void *key, size_t key_length)
{
	unsigned int i = ___hashtable_hash(key, key_length, h->store_size); 

	hashtable_node_t *node = h->store[i]; 
	while(node && !___hashtable_compare_keys(key, key_length, node->key, node->key_length))
		node = node->next; 

	void *item = NULL; 
	if (node)
		item = node->item; 

	return item; 
}	

void** ___hashtable_get_all(hashtable_t *h) {		
	int count = 0;
	void **items = malloc(h->count * sizeof(void*));  

	int i;
	for (i = 0; i < h->store_size; i++)
	{
		hashtable_node_t *node = h->store[i]; 
		while(node)
		{
			items[count++] = node->item;
			node = node->next; 
		}
	} 
	return items; 
}

int ___hashtable_is_empty (hashtable_t *h)
{
	return h->count == 0;
}

int ___hashtable_compare_keys(void *key, size_t key_len, void* key1, size_t key_len1)
{
	if (key_len != key_len1) return FALSE; 
	return memcmp(key, key1, key_len) == 0; 
}

unsigned int ___hashtable_hash(void *key, size_t key_len, int max_key)
{
	unsigned int hash = 1;
	int i;
	for(i = 0; i < key_len; i++)
		hash = (hash << 5) ^ ((unsigned char *)key)[i] ^ hash;

	return (hash + max_key) % max_key;
}
