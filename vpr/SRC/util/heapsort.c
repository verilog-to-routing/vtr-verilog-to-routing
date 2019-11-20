#include "heapsort.h"

/******************* Subroutines local to heapsort.c ************************/

static void add_to_sort_heap(int *heap, float *sort_values, int index,
		int heap_tail);

static int get_top_of_heap_index(int *heap, float *sort_values, int heap_tail,
		int start_index);

/********************* Subroutine definitions *******************************/

void heapsort(int *sort_index, float *sort_values, int nelem, int start_index) {

	/* This routine loads sort_index [1..nelem] with nelem values:  the indices  *
	 * of the sort_values [1..nelem] array that lead to sort_values[index] being *
	 * decreasing as one goes through sort_index.  Sort_values is not changed.   */

	int i;

	sort_index -= start_index;
	sort_values -= start_index;

	/* Build a heap with the *smallest* element at the top. */

	for (i = 1; i <= nelem; i++)
		add_to_sort_heap(sort_index, sort_values, i, i);

	/* Now pull items off the heap, smallest first.  As the heap (in sort_index) *
	 * shrinks, I store the item pulled off (the smallest) in sort_index,        *
	 * starting from the end. The heap and the stored smallest values never      *
	 * overlap, so I get a nice sort-in-place.                                   */

	for (i = nelem; i >= 1; i--)
		sort_index[i] = get_top_of_heap_index(sort_index, sort_values, i,
				start_index);

	sort_index += start_index;
	sort_values += start_index;
}

static void add_to_sort_heap(int *heap, float *sort_values, int index,
		int heap_tail) {

	/* Adds element index, with value = sort_values[index] to the heap.          *
	 * Heap_tail is the number of elements in the heap *after* the insert.       */

	unsigned int ifrom, ito; /* Making these unsigned helps compiler opts. */
	int temp;

	heap[heap_tail] = index;
	ifrom = heap_tail;
	ito = ifrom / 2;

	while (ito >= 1 && sort_values[heap[ifrom]] < sort_values[heap[ito]]) {
		temp = heap[ito];
		heap[ito] = heap[ifrom];
		heap[ifrom] = temp;
		ifrom = ito;
		ito = ifrom / 2;
	}
}

static int get_top_of_heap_index(int *heap, float *sort_values, int heap_tail,
		int start_index) {

	/* Returns the index of the item at the top of the heap (the smallest one).  *
	 * It then rebuilds the heap.  Heap_tail is the number of items on the heap  *
	 * before the top item is deleted.                                           */

	int index_of_smallest, temp;
	unsigned int ifrom, ito, heap_end;

	index_of_smallest = heap[1];
	heap[1] = heap[heap_tail];
	heap_end = heap_tail - 1; /* One item deleted. */
	ifrom = 1;
	ito = 2 * ifrom;

	while (ito <= heap_end) {
		if (sort_values[heap[ito + 1]] < sort_values[heap[ito]])
			ito++;

		if (sort_values[heap[ito]] > sort_values[heap[ifrom]])
			break;

		temp = heap[ito];
		heap[ito] = heap[ifrom];
		heap[ifrom] = temp;
		ifrom = ito;
		ito = 2 * ifrom;
	}

	/*index must have the start_index subracted to be consistent with the original array*/
	return (index_of_smallest - start_index);
}
