#ifndef VTR_LIST_H
#define VTR_LIST_H

/**
 * @file
 * @brief Linked lists of void pointers and integers, respectively.
 */

namespace vtr {

///@brief Linked list node struct
struct t_linked_vptr {
    void* data_vptr;
    struct t_linked_vptr* next;
};

///@brief Inserts a node to a list
t_linked_vptr* insert_in_vptr_list(t_linked_vptr* head,
                                   void* vptr_to_add);

///@brief Delete a list
t_linked_vptr* delete_in_vptr_list(t_linked_vptr* head);
} // namespace vtr
#endif
