#include <cstdlib>

#include "vtr_list.h"
#include "vtr_memory.h"

namespace vtr {

t_linked_vptr* insert_in_vptr_list(t_linked_vptr* head, void* vptr_to_add) {
    /* Inserts a new element at the head of a linked list of void pointers. *
     * Returns the new head of the list.                                    */

    t_linked_vptr* linked_vptr;

    linked_vptr = (t_linked_vptr*)vtr::malloc(sizeof(t_linked_vptr));

    linked_vptr->data_vptr = vptr_to_add;
    linked_vptr->next = head;
    return (linked_vptr); /* New head of the list */
}

/* Deletes the element at the head of a linked list of void pointers. *
 * Returns the new head of the list.                                    */
t_linked_vptr* delete_in_vptr_list(t_linked_vptr* head) {
    t_linked_vptr* linked_vptr;

    if (head == nullptr)
        return nullptr;
    linked_vptr = head->next;
    free(head);
    return linked_vptr; /* New head of the list */
}

} // namespace vtr
