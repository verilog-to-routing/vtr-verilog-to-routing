/*
 * Revision Control Information
 *
 * $Source: /vol/opua/opua2/sis/sis-1.1/common/src/sis/node/RCS/cubehack.c,v $
 * $Author: sis $
 * $Revision: 1.2 $
 * $Date: 1992/05/06 18:57:41 $
 *
 */
/*
#include "sis.h"
#include "node_int.h"

#ifdef lint
struct cube_struct cube;
bool summary;
bool trace;
bool remove_essential;
bool force_irredundant;
bool unwrap_onset;
bool single_expand;
bool pos;
bool recompute_onset;
bool use_super_gasp;
bool use_random_order;
#endif
*/
#include "espresso.h"

ABC_NAMESPACE_IMPL_START



void 
cautious_define_cube_size(n)
int n;
{
    if (cube.fullset != 0 && cube.num_binary_vars == n)
    return;
    if (cube.fullset != 0) {
    setdown_cube();
    FREE(cube.part_size);
    }
    cube.num_binary_vars = cube.num_vars = n;
    cube.part_size = ALLOC(int, n);
    cube_setup();
}


void
define_cube_size(n)
int n;
{
    register int q, i;
    static int called_before = 0;

    /* check if the cube is already just the right size */
    if (cube.fullset != 0 && cube.num_binary_vars == n && cube.num_vars == n)
    return;

    /* We can't handle more than 100 inputs */
    if (n > 100) {
    cautious_define_cube_size(n);
    called_before = 0;
    return;
    }

    if (cube.fullset == 0 || ! called_before) {
    cautious_define_cube_size(100);
    called_before = 1;
    }

    cube.num_vars = n;
    cube.num_binary_vars = n;
    cube.num_mv_vars = 0;
    cube.output = -1;
    cube.size = n * 2;

    /* first_part, last_part, first_word, last_word, part_size OKAY */
    /* cube.sparse is OKAY */

    /* need to completely re-make cube.fullset and cube.binary_mask */
    (void) set_fill(cube.fullset, n*2);
    (void) set_fill(cube.binary_mask, n*2);

    /* need to resize each set in cube.var_mask and cube.temp */
    q = cube.fullset[0];
    for(i = 0; i < cube.num_vars; i++)
    cube.var_mask[i][0] = q;
    for(i = 0; i < CUBE_TEMP; i++)
    cube.temp[i][0] = q;

    /* need to resize cube.emptyset and cube.mv_mask */
    cube.emptyset[0] = q;
    cube.mv_mask[0] = q;

    /* need to reset the inword and inmask */
    if (cube.num_binary_vars != 0) {
    cube.inword = cube.last_word[cube.num_binary_vars - 1];
    cube.inmask = cube.binary_mask[cube.inword] & DISJOINT;
    } else {
    cube.inword = -1;
    cube.inmask = 0;
    }

    /* cdata (entire structure) is OKAY */
}


void
undefine_cube_size()
{
    if (cube.num_binary_vars > 100) {
    if (cube.fullset != 0) {
        setdown_cube();
        FREE(cube.part_size);
    }
    } else {
    cube.num_vars = cube.num_binary_vars = 100;
    if (cube.fullset != 0) {
        setdown_cube();
        FREE(cube.part_size);
    }
    }
}


void
set_espresso_flags()
{
    summary = FALSE;
    trace = FALSE;
    remove_essential = TRUE;
    force_irredundant = TRUE;
    unwrap_onset = TRUE;
    single_expand = FALSE;
    pos = FALSE;
    recompute_onset = FALSE;
    use_super_gasp = FALSE;
    use_random_order = FALSE;
}
ABC_NAMESPACE_IMPL_END

