#include "vtr_assert.h"
#include "cube.h"
#include "bdd.h"

/* set_clear -- make "r" the empty set of "size" elements */
/*
 pset set_clear(r, size)
 pset r;
 int size;
 {
 int i = LOOPINIT(size);
 *r = i; do r[i] = 0; while (--i > 0);
 return r;
 }*/

ace_cube_t * ace_cube_dup(ace_cube_t * cube) {
	int i;
	ace_cube_t * cube_copy;

	VTR_ASSERT(cube != NULL);
	VTR_ASSERT(cube->num_literals > 0);

	cube_copy = (ace_cube_t*) malloc(sizeof(ace_cube_t));
	cube_copy->static_prob = cube->static_prob;
	cube_copy->num_literals = cube->num_literals;
	cube_copy->cube = set_new (2 * cube->num_literals);
	for (i = 0; i < cube->num_literals; i++) {
		switch (node_get_literal (cube->cube, i)) {
		case ZERO:
			set_insert(cube_copy->cube, 2 * i);
			set_remove(cube_copy->cube, 2 * i + 1);
			break;
		case ONE:
			set_remove(cube_copy->cube, 2 * i);
			set_insert(cube_copy->cube, 2 * i + 1);
			break;
		case TWO:
			set_insert(cube_copy->cube, 2 * i);
			set_insert(cube_copy->cube, 2 * i + 1);
			break;
		default:
			fail("Bad literal.");
		}
	}

	return (cube_copy);
}

ace_cube_t * ace_cube_new_dc(int num_literals) {
	int i;
	ace_cube_t * new_cube;

	new_cube = (ace_cube_t*) malloc(sizeof(ace_cube_t));
	new_cube->num_literals = num_literals;
	new_cube->static_prob = 1.0;
	new_cube->cube = set_new (2 * num_literals);

	for (i = 0; i < num_literals; i++) {
		set_insert(new_cube->cube, 2 * i);
		set_insert(new_cube->cube, 2 * i + 1);
	}

	return (new_cube);
}

void ace_cube_free(ace_cube_t * cube) {
	VTR_ASSERT(cube != NULL);
	VTR_ASSERT(cube->cube != NULL);
	free(cube->cube);
	free(cube);
}
