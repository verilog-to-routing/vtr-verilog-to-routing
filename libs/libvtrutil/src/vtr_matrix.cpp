#include <cstdio>
#include <cstdlib>

#include "vtr_matrix.h"
#include "vtr_util.h"
#include "vtr_error.h"
#include "vtr_memory.h"

namespace vtr {

void free_ivec_vector(std::vector<std::vector<int>> ivec_vector, int nrmin, int nrmax) {

	/* Frees a 1D array of integer vectors.                              */

	int i;

	for (i = nrmin; i <= nrmax; i++)
		if (ivec_vector[i].size() != 0)
			ivec_vector[i].clear();

	ivec_vector.clear();
}

void alloc_ivector_and_copy_int_list(t_linked_int ** list_head_ptr,
		int num_items, std::vector<int> *ivec, t_linked_int ** free_list_head_ptr) {

	/* Allocates an integer vector with num_items elements and copies the       *
	 * integers from the list pointed to by list_head (of which there must be   *
	 * num_items) over to it.  The int_list is then put on the free list, and   *
	 * the list_head_ptr is set to NULL.                                        */

	t_linked_int *linked_int, *list_head;
	int i;

	list_head = *list_head_ptr;

	if (num_items == 0) { /* Empty list. */
            ivec->clear();

		if (list_head != nullptr ) {
            throw VtrError(vtr::string_fmt("alloc_ivector_and_copy_int_list: Copied %d elements, "
                                           "but list at %p contains more.", num_items, (void *) list_head),
                            __FILE__, __LINE__);
		}
		return;
	}

	ivec->resize(num_items);
	linked_int = list_head;

	for (i = 0; i < num_items - 1; i++) {
		(*ivec)[i] = linked_int->data;
		linked_int = linked_int->next;
	}

	(*ivec)[num_items - 1] = linked_int->data;

	if (linked_int->next != nullptr ) {
        throw VtrError(vtr::string_fmt("alloc_ivector_and_copy_int_list: Copied %d elements, "
                                       "but list at %p contains more.", num_items, (void *) list_head),
                        __FILE__, __LINE__);
	}

	linked_int->next = *free_list_head_ptr;
	*free_list_head_ptr = list_head;
	*list_head_ptr = nullptr;
}


} //namespace
