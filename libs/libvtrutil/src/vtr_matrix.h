#ifndef VTR_MATRIX_H
#define VTR_MATRIX_H

#include <cstdlib>

#include "vtr_list.h"
#include "vtr_memory.h"

namespace vtr {

    template<typename T>
    T** alloc_matrix(int nrmin, int nrmax, int ncmin, int ncmax) {
        /* allocates an generic matrix with nrmax-nrmin + 1 rows and ncmax - *
         * ncmin + 1 columns, with each element of size elsize. i.e.         *
         * returns a pointer to a storage block [nrmin..nrmax][ncmin..ncmax].*/

        int i;
        T** ptr;

        ptr = (T**) vtr::malloc((nrmax - nrmin + 1) * sizeof(T*));
        ptr -= nrmin;
        for (i = nrmin; i <= nrmax; i++) {
            ptr[i] = (T *) vtr::malloc((ncmax - ncmin + 1) * sizeof(T));
            ptr[i] -= ncmin;
        }
        return ptr;
    }

    template<typename T>
    T*** alloc_matrix3(int nrmin, int nrmax, int ncmin, int ncmax, int ndmin, int ndmax) {
        /* allocates a 3D generic matrix with nrmax-nrmin + 1 rows, ncmax -  *
         * ncmin + 1 columns, and a depth of ndmax-ndmin + 1, with each      *
         * element of size elsize. i.e. returns a pointer to a storage block *
         * [nrmin..nrmax][ncmin..ncmax][ndmin..ndmax].                       */

        int i;
        T*** ptr;

        ptr = (T***) vtr::malloc((nrmax - nrmin + 1) * sizeof(T**));
        ptr -= nrmin;
        for (i = nrmin; i <= nrmax; i++) {
            ptr[i] = alloc_matrix<T>(ncmin, ncmax, ndmin, ndmax);
        }
        return ptr;
    }

    template<typename T>
    T**** alloc_matrix4(int nrmin, int nrmax, int ncmin, int ncmax, int ndmin, int ndmax,
            int nemin, int nemax) {

        /* allocates a 4D generic matrix. Returns a pointer to a storage block *
         * [nrmin..nrmax][ncmin..ncmax][ndmin..ndmax][nemin..nemax].           */
        int i;
        T ****ptr;

        ptr = (T ****) vtr::malloc((nrmax - nrmin + 1) * sizeof(T ***));
        ptr -= nrmin;
        for (i = nrmin; i <= nrmax; i++) {
            ptr[i] = alloc_matrix3<T>(ncmin, ncmax, ndmin, ndmax, nemin, nemax);
        }
        return ptr;
    }

    template<typename T>
    T***** alloc_matrix5(int nrmin, int nrmax, int ncmin, int ncmax, int ndmin, int ndmax,
            int nemin, int nemax, int nfmin, int nfmax) {

        /* allocates a 5D generic matrix. Returns a pointer to a storage block    *
         * [nrmin..nrmax][ncmin..ncmax][ndmin..ndmax][nemin..nemax][nfmin..nfmax] */
        int i;
        T *****ptr;

        ptr = (T *****) vtr::malloc((nrmax - nrmin + 1) * sizeof(T ****));
        ptr -= nrmin;
        for (i = nrmin; i <= nrmax; i++) {
            ptr[i] = alloc_matrix4<T>(ncmin, ncmax, ndmin, ndmax, nemin, nemax, nfmin, nfmax);
        }
        return ptr;
    }

    template<typename T>
    void free_matrix(T* ptr, int nrmin, int nrmax, int ncmin) {
        int i;
        for (i = nrmin; i <= nrmax; i++) {
            //coverity[offset_free]
            std::free(ptr[i] + ncmin);
        }
        //coverity[offset_free]
        std::free(ptr + nrmin);
    }

    template<typename T>
    void free_matrix3(T** ptr, int nrmin, int nrmax, int ncmin, int ncmax, int ndmin) {
        int i;
        for (i = nrmin; i <= nrmax; i++) {
            free_matrix(ptr[i], ncmin, ncmax, ndmin);
        }
        //coverity[offset_free]
        std::free(ptr + nrmin);
    }

    template<typename T>
    void free_matrix4(T**** ptr, int nrmin, int nrmax, int ncmin, int ncmax, int ndmin, int ndmax,
            int nemin) {
        int i;
        for (i = nrmin; i <= nrmax; i++) {
            free_matrix3(ptr[i], ncmin, ncmax, ndmin, ndmax, nemin);
        }
        //coverity[offset_free]
        std::free(ptr + nrmin);
    }

    template<typename T>
    void free_matrix5(T***** ptr, int nrmin, int nrmax, int ncmin, int ncmax, int ndmin, int ndmax,
            int nemin, int nemax, int nfmin) {
        int i;
        for (i = nrmin; i <= nrmax; i++) {
            free_matrix4(ptr[i], ncmin, ncmax, ndmin, ndmax, nemin, nemax, nfmin);
        }
        //coverity[offset_free]
        std::free(ptr + nrmin);
    }

    /* Integer vector.  nelem stores length, list[0..nelem-1] stores list of    *
     * integers.                                                                */

    typedef struct s_ivec {
        int nelem;
        int *list;
    } t_ivec;

    void alloc_ivector_and_copy_int_list(t_linked_int ** list_head_ptr,
            int num_items, t_ivec *ivec, t_linked_int ** free_list_head_ptr);
    void free_ivec_vector(t_ivec *ivec_vector, int nrmin, int nrmax);
    void free_ivec_matrix(t_ivec **ivec_matrix, int nrmin, int nrmax,
            int ncmin, int ncmax);
    void free_ivec_matrix3(t_ivec ***ivec_matrix3, int nrmin,
            int nrmax, int ncmin, int ncmax, int ndmin, int ndmax);

    void print_int_matrix3(int ***vptr, int nrmin, int nrmax, int ncmin,
            int ncmax, int ndmin, int ndmax, char *file);


} //namespace

#endif
