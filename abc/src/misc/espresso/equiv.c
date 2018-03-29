/*
 * Revision Control Information
 *
 * $Source$
 * $Author$
 * $Revision$
 * $Date$
 *
 */
#include "espresso.h"

ABC_NAMESPACE_IMPL_START



void find_equiv_outputs(PLA)
pPLA PLA;
{
    int i, j, ipart, jpart, some_equiv;
    pcover *R, *F;

    some_equiv = FALSE;

    makeup_labels(PLA);

    F = ALLOC(pcover, cube.part_size[cube.output]);
    R = ALLOC(pcover, cube.part_size[cube.output]);

    for(i = 0; i < cube.part_size[cube.output]; i++) {
    ipart = cube.first_part[cube.output] + i;
    R[i] = cof_output(PLA->R, ipart);
    F[i] = complement(cube1list(R[i]));
    }
    
    for(i = 0; i < cube.part_size[cube.output]-1; i++) {
    for(j = i+1; j < cube.part_size[cube.output]; j++) {
        ipart = cube.first_part[cube.output] + i;
        jpart = cube.first_part[cube.output] + j;

        if (check_equiv(F[i], F[j])) {
        (void) printf("# Outputs %d and %d (%s and %s) are equivalent\n",
            i, j, PLA->label[ipart], PLA->label[jpart]);
        some_equiv = TRUE;
        } else if (check_equiv(F[i], R[j])) {
        (void) printf("# Outputs %d and NOT %d (%s and %s) are equivalent\n",
            i, j, PLA->label[ipart], PLA->label[jpart]);
        some_equiv = TRUE;
        } else if (check_equiv(R[i], F[j])) {
        (void) printf("# Outputs NOT %d and %d (%s and %s) are equivalent\n",
            i, j, PLA->label[ipart], PLA->label[jpart]);
        some_equiv = TRUE;
        } else if (check_equiv(R[i], R[j])) {
        (void) printf("# Outputs NOT %d and NOT %d (%s and %s) are equivalent\n",
            i, j, PLA->label[ipart], PLA->label[jpart]);
        some_equiv = TRUE;
        }
    }
    }

    if (! some_equiv) {
    (void) printf("# No outputs are equivalent\n");
    }

    for(i = 0; i < cube.part_size[cube.output]; i++) {
    free_cover(F[i]);
    free_cover(R[i]);
    }
    FREE(F);
    FREE(R);
}



int check_equiv(f1, f2)
pcover f1, f2;
{
    register pcube *f1list, *f2list;
    register pcube p, last;

    f1list = cube1list(f1);
    foreach_set(f2, last, p) {
    if (! cube_is_covered(f1list, p)) {
        return FALSE;
    }
    }
    free_cubelist(f1list);

    f2list = cube1list(f2);
    foreach_set(f1, last, p) {
    if (! cube_is_covered(f2list, p)) {
        return FALSE;
    }
    }
    free_cubelist(f2list);

    return TRUE;
}
ABC_NAMESPACE_IMPL_END

