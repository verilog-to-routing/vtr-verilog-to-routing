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
#ifndef QUEUE_H
#define QUEUE_H
#define queue_t       struct queue_t_t
#define queue_node_t  struct queue_node_t_t

// Constructor 
queue_t* create_queue();

struct queue_t_t
{
	queue_node_t *head;
	queue_node_t *tail;
	int count;

	// Adds an item to the queue.
	void   (*add)        (queue_t *q, void *item);
	// Removes an item from the queue.
	void  *(*remove)     (queue_t *q);
	// Removes an array of all items from the queue.
	void **(*remove_all) (queue_t *q);
	// Checks to see if the queue is empty.
	int    (*is_empty)   (queue_t *q);
	// Destroys the queue, but does not free the items added to it.
	void   (*destroy)    (queue_t *q);
};

struct queue_node_t_t
{
	queue_node_t *next;
	void *item;
};

#endif
