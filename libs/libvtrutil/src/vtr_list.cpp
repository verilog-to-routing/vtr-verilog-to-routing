#include <cstdlib>

#include "vtr_list.h"
#include "vtr_memory.h"

namespace vtr {

t_linked_vptr* insert_in_vptr_list(t_linked_vptr* head, void* vptr_to_add) {
    /* Inserts a new element at the head of a linked list of void pointers. *
     * Returns the new head of the list.                                    */

    return new t_linked_vptr{vptr_to_add, head}; /* New head of the list */
}

/* Deletes the element at the head of a linked list of void pointers. *
 * Returns the new head of the list.                                    */
t_linked_vptr* delete_in_vptr_list(t_linked_vptr* head) {
    if (head == nullptr)
        return nullptr;
    t_linked_vptr* const linked_vptr = head->next;
    delete head;
    return linked_vptr; /* New head of the list */
}

} // namespace vtr
