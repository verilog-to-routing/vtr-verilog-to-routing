#ifndef VTR_LIST_H
#define VTR_LIST_H

namespace vtr {
/* Linked lists of void pointers and integers, respectively. */

struct t_linked_vptr {
    void* data_vptr;
    struct t_linked_vptr* next;
};

t_linked_vptr* insert_in_vptr_list(t_linked_vptr* head,
                                   void* vptr_to_add);
t_linked_vptr* delete_in_vptr_list(t_linked_vptr* head);
} // namespace vtr
#endif
