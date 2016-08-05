/*
 * Coverity modeling file for VTR
 *
 * The model file is uses special __coverity_*__() commands
 * to suppress false positives.
 *
 * We use this primarily to suppress warnings about some special
 * memory/pointer manipulations used in VPR.
 *
 *
 */

/*
 * From rout_common.c
 *
 * These functions do 'unusual' pointer manimulations while freeing,
 * which cause coverity to report errors.  In reality these are
 * safe if used correctly.
 */
struct t_rt_node;
void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
                                      t_rt_node ** rt_node_of_sink) {
    __coverity_free__(pin_criticality);
    __coverity_free__(sink_order);
    __coverity_free__(rt_node_of_sink);
}

static struct s_heap **heap;
void free_route_structs() {
    __coverity_free__(heap);
}

/*
 * From libarchfpga/util.c
 *
 * The free_matrix*() functions offset the passed pointers before
 * freeing to match the block originally allocated by the alloc_matrix*() function.
 * The alloc_matrix*() functions originally offset the pointers to respected the
 * use specified index ranges.
 */
void free_matrix(void *vptr, int nrmin, int nrmax, int ncmin, size_t elsize) {
    __coverity_free__(vptr);
}

void free_matrix3(void *vptr, int nrmin, int nrmax, int ncmin, int ncmax,
                  int ndmin, size_t elsize) {
    __coverity_free__(vptr);
}

void free_matrix4(void *vptr, int nrmin, int nrmax, int ncmin, int ncmax,
                  int ndmin, int ndmax, int nemin, size_t elsize) {
    __coverity_free__(vptr);
}

void free_matrix5(void *vptr, int nrmin, int nrmax, int ncmin, int ncmax,
                  int ndmin, int ndmax, int nemin, int nemax, int nfmin, size_t elsize) {
    __coverity_free__(vptr);
}
