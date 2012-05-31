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
#include "queue.h"
#include "types.h"

void   ___queue_add(queue_t *q, void *item);
void  *___queue_remove(queue_t *q);
void **___queue_remove_all(queue_t *q);
int    ___queue_is_empty (queue_t *q);
void   ___queue_destroy(queue_t *q);

queue_t* create_queue()
{
	queue_t *q = (queue_t *)malloc(sizeof(queue_t));

	q->head = NULL;
	q->tail = NULL;
	q->count = 0;

	q->add        = ___queue_add; 
	q->remove     = ___queue_remove; 
	q->remove_all = ___queue_remove_all; 
	q->is_empty   = ___queue_is_empty; 
	q->destroy    = ___queue_destroy; 

	return q;
}

void ___queue_destroy(queue_t *q)
{
	while (q->remove(q)); 
	free(q);
}

void ___queue_add(queue_t *q, void *item)
{
	queue_node_t *node = (queue_node_t *)malloc(sizeof(queue_node_t));

	node->next = NULL;
	node->item = item;

	if (q->tail)
		q->tail->next = node;

	q->tail = node;

	if (!q->head)
		q->head = node;

	q->count++;
}

void* ___queue_remove(queue_t *q)
{
	void *item = NULL; 
	if (!q->is_empty(q))
	{
		queue_node_t *node = q->head;
		item = node->item;
		q->head = node->next;
		free(node);
		q->count--;	

		if (!q->count)
		{
			q->head = NULL; 
			q->tail = NULL; 
		}
	}	
	return item;
}

void **___queue_remove_all(queue_t *q)
{
	void **items = NULL; 
	if (!q->is_empty(q))
	{
		items = malloc(q->count * sizeof(void *)); 
		int count = 0; 
		void *item;
		while ((item = q->remove(q)))
			items[count++] = item; 
	}
	return items; 
}

int ___queue_is_empty (queue_t *q)
{
	return !q->head;
}
