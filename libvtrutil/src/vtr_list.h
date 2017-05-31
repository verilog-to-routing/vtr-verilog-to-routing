#ifndef VTR_LIST_H
#define VTR_LIST_H

namespace vtr {
    /* Linked lists of void pointers and integers, respectively. */

    struct t_linked_vptr {
        void *data_vptr;
        struct t_linked_vptr *next;
    };

    struct t_linked_int {
        int data;
        t_linked_int *next;
    };

    t_linked_vptr *insert_in_vptr_list(t_linked_vptr *head,
            void *vptr_to_add);
    t_linked_vptr *delete_in_vptr_list(t_linked_vptr *head);

    t_linked_int *insert_in_int_list(t_linked_int * head, int data,
            t_linked_int ** free_list_head_ptr);
    void free_int_list(t_linked_int ** int_list_head_ptr);
}
#endif
