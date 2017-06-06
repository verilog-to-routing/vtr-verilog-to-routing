#include <cstdio>
#include <cstdlib>

#include "vtr_matrix.h"
#include "vtr_util.h"
#include "vtr_error.h"
#include "vtr_memory.h"

namespace vtr {

void free_ivec_vector(t_ivec *ivec_vector, int nrmin, int nrmax) {

	/* Frees a 1D array of integer vectors.                              */

	int i;

	for (i = nrmin; i <= nrmax; i++)
		if (ivec_vector[i].nelem != 0)
			free(ivec_vector[i].list);

    // coverity[offset_free : Intentional]
	free(ivec_vector + nrmin);
}

void alloc_ivector_and_copy_int_list(t_linked_int ** list_head_ptr,
		int num_items, t_ivec *ivec, t_linked_int ** free_list_head_ptr) {

	/* Allocates an integer vector with num_items elements and copies the       *
	 * integers from the list pointed to by list_head (of which there must be   *
	 * num_items) over to it.  The int_list is then put on the free list, and   *
	 * the list_head_ptr is set to NULL.                                        */

	t_linked_int *linked_int, *list_head;
	int i, *list;

	list_head = *list_head_ptr;

	if (num_items == 0) { /* Empty list. */
		ivec->nelem = 0;
		ivec->list = NULL;

		if (list_head != NULL ) {
            throw VtrError(vtr::string_fmt("alloc_ivector_and_copy_int_list: Copied %d elements, "
                                           "but list at %p contains more.", num_items, (void *) list_head), 
                            __FILE__, __LINE__);
		}
		return;
	}

	ivec->nelem = num_items;
	list = (int *) vtr::malloc(num_items * sizeof(int));
	ivec->list = list;
	linked_int = list_head;

	for (i = 0; i < num_items - 1; i++) {
		list[i] = linked_int->data;
		linked_int = linked_int->next;
	}

	list[num_items - 1] = linked_int->data;

	if (linked_int->next != NULL ) {
        throw VtrError(vtr::string_fmt("alloc_ivector_and_copy_int_list: Copied %d elements, "
                                       "but list at %p contains more.", num_items, (void *) list_head), 
                        __FILE__, __LINE__);
	}

	linked_int->next = *free_list_head_ptr;
	*free_list_head_ptr = list_head;
	*list_head_ptr = NULL;
}


} //namespace
