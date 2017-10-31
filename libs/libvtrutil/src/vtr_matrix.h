#ifndef VTR_MATRIX_H
#define VTR_MATRIX_H

#include <cstdlib>
#include <vector>

#include "vtr_list.h"

namespace vtr {

    void alloc_ivector_and_copy_int_list(t_linked_int ** list_head_ptr,
            int num_items, std::vector<int> *ivec, t_linked_int ** free_list_head_ptr);

    void free_ivec_vector(std::vector<std::vector<int>> ivec_vector, int nrmin, int nrmax);

} //namespace

#endif
