#include <cstdlib>

#include "vtr_list.h"
#include "vtr_memory.h"

namespace vtr {

t_linked_vptr *insert_in_vptr_list(t_linked_vptr *head, void *vptr_to_add) {

	/* Inserts a new element at the head of a linked list of void pointers. *
	 * Returns the new head of the list.                                    */

	t_linked_vptr *linked_vptr;

	linked_vptr = (t_linked_vptr *) vtr::malloc(sizeof(t_linked_vptr));

	linked_vptr->data_vptr = vptr_to_add;
	linked_vptr->next = head;
	return (linked_vptr); /* New head of the list */
}

/* Deletes the element at the head of a linked list of void pointers. *
 * Returns the new head of the list.                                    */
t_linked_vptr *delete_in_vptr_list(t_linked_vptr *head) {
	t_linked_vptr *linked_vptr;

	if (head == nullptr )
		return nullptr ;
	linked_vptr = head->next;
	free(head);
	return linked_vptr; /* New head of the list */
}

t_linked_int *insert_in_int_list(t_linked_int * head, int data,
		t_linked_int ** free_list_head_ptr) {

	/* Inserts a new element at the head of a linked list of integers.  Returns  *
	 * the new head of the list.  One argument is the address of the head of     *
	 * a list of free ilist elements.  If there are any elements on this free    *
	 * list, the new element is taken from it.  Otherwise a new one is malloced. */

	t_linked_int *linked_int;

	if (*free_list_head_ptr != nullptr ) {
		linked_int = *free_list_head_ptr;
		*free_list_head_ptr = linked_int->next;
	} else {
		linked_int = (t_linked_int *) vtr::malloc(sizeof(t_linked_int));
	}

	linked_int->data = data;
	linked_int->next = head;
	return (linked_int);
}

void free_int_list(t_linked_int ** int_list_head_ptr) {

	/* This routine truly frees (calls free) all the integer list elements    *
	 * on the linked list pointed to by *head, and sets head = NULL.          */

	t_linked_int *linked_int, *next_linked_int;

	linked_int = *int_list_head_ptr;

	while (linked_int != nullptr ) {
		next_linked_int = linked_int->next;
		free(linked_int);
		linked_int = next_linked_int;
	}

	*int_list_head_ptr = nullptr;
}

} //namespace
