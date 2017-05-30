#ifndef VTR_MATRIX_H
#define VTR_MATRIX_H

#include <cstdlib>

#include "vtr_list.h"

namespace vtr {

    /* Integer vector.  nelem stores length, list[0..nelem-1] stores list of    *
     * integers.                                                                */

    //TODO: Convert all uses of this data structure to std::vector and elete
    struct t_ivec {
        int nelem;
        int *list;
    };

    void alloc_ivector_and_copy_int_list(t_linked_int ** list_head_ptr,
            int num_items, t_ivec *ivec, t_linked_int ** free_list_head_ptr);
    void free_ivec_vector(t_ivec *ivec_vector, int nrmin, int nrmax);

} //namespace

#endif
