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



static void dump_irredundant();
static pcover do_minimize();


/*
 *  minimize_exact -- main entry point for exact minimization
 *
 *  Global flags which affect this routine are:
 *
 *      debug
 *      skip_make_sparse
 */

pcover
minimize_exact(F, D, R, exact_cover)
pcover F, D, R;
int exact_cover;
{
    return do_minimize(F, D, R, exact_cover, /*weighted*/ 0);
}


pcover
minimize_exact_literals(F, D, R, exact_cover)
pcover F, D, R;
int exact_cover;
{
    return do_minimize(F, D, R, exact_cover, /*weighted*/ 1);
}



static pcover
do_minimize(F, D, R, exact_cover, weighted)
pcover F, D, R;
int exact_cover;
int weighted;
{
    pcover newF, E, Rt, Rp;
    pset p, last;
    int heur, level, *weights, i;
    sm_matrix *table;
    sm_row *cover;
    sm_element *pe;
    int debug_save = debug;

    if (debug & EXACT) {
	debug |= (IRRED | MINCOV);
    }
#if defined(sun) || defined(bsd4_2)			/* hack ... */
    if (debug & MINCOV) {
	setlinebuf(stdout);
    }
#endif
    level = (debug & MINCOV) ? 4 : 0;
    heur = ! exact_cover;

    /* Generate all prime implicants */
    EXEC(F = primes_consensus(cube2list(F, D)), "PRIMES     ", F);

    /* Setup the prime implicant table */
    EXEC(irred_split_cover(F, D, &E, &Rt, &Rp), "ESSENTIALS ", E);
    EXEC(table = irred_derive_table(D, E, Rp),  "PI-TABLE   ", Rp);

    /* Solve either a weighted or nonweighted covering problem */
    if (weighted) {
	/* correct only for all 2-valued variables */
	weights = ALLOC(int, F->count);
	foreach_set(Rp, last, p) {
	    weights[SIZE(p)] = cube.size - set_ord(p);
	    /* We have added the 0's in the output part instead of the 1's.
	       This loop corrects the literal count. */
	    for (i = cube.first_part[cube.output];
		 i <= cube.last_part[cube.output]; i++) {
		is_in_set(p, i) ? weights[SIZE(p)]++ : weights[SIZE(p)]--;
	    }
	}
    } else {
	weights = NIL(int);
    }
    EXEC(cover=sm_minimum_cover(table,weights,heur,level), "MINCOV     ", F);
    if (weights != 0) {
	FREE(weights);
    }

    if (debug & EXACT) {
	dump_irredundant(E, Rt, Rp, table);
    }

    /* Form the result cover */
    newF = new_cover(100);
    foreach_set(E, last, p) {
	newF = sf_addset(newF, p);
    }
    sm_foreach_row_element(cover, pe) {
	newF = sf_addset(newF, GETSET(F, pe->col_num));
    }

    free_cover(E);
    free_cover(Rt);
    free_cover(Rp);
    sm_free(table);
    sm_row_free(cover);
    free_cover(F);

    /* Attempt to make the results more sparse */
    debug &= ~ (IRRED | SHARP | MINCOV);
    if (! skip_make_sparse && R != 0) {
	newF = make_sparse(newF, D, R);
    }

    debug = debug_save;
    return newF;
}

static void
dump_irredundant(E, Rt, Rp, table)
pcover E, Rt, Rp;
sm_matrix *table;
{
    FILE *fp_pi_table, *fp_primes;
    pPLA PLA;
    pset last, p;
    char *file;

    if (filename == 0 || strcmp(filename, "(stdin)") == 0) {
	fp_pi_table = fp_primes = stdout;
    } else {
	file = ALLOC(char, strlen(filename)+20);
	(void) sprintf(file, "%s.primes", filename);
	if ((fp_primes = fopen(file, "w")) == NULL) {
	    (void) fprintf(stderr, "espresso: Unable to open %s\n", file);
	    fp_primes = stdout;
	}
	(void) sprintf(file, "%s.pi", filename);
	if ((fp_pi_table = fopen(file, "w")) == NULL) {
	    (void) fprintf(stderr, "espresso: Unable to open %s\n", file);
	    fp_pi_table = stdout;
	}
	FREE(file);
    }

    PLA = new_PLA();
    PLA_labels(PLA);

    fpr_header(fp_primes, PLA, F_type);
    free_PLA(PLA);

    (void) fprintf(fp_primes, "# Essential primes are\n");
    foreach_set(E, last, p) {
	(void) fprintf(fp_primes, "%s\n", pc1(p));
    }
    (void) fprintf(fp_primes, "# Totally redundant primes are\n");
    foreach_set(Rt, last, p) {
	(void) fprintf(fp_primes, "%s\n", pc1(p));
    }
    (void) fprintf(fp_primes, "# Partially redundant primes are\n");
    foreach_set(Rp, last, p) {
	(void) fprintf(fp_primes, "%s\n", pc1(p));
    }
    if (fp_primes != stdout) {
	(void) fclose(fp_primes);
    }
	
    sm_write(fp_pi_table, table);
    if (fp_pi_table != stdout) {
	(void) fclose(fp_pi_table);
    }
}
ABC_NAMESPACE_IMPL_END

