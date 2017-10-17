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



/* cost -- compute the cost of a cover */
void cover_cost(F, cost)
IN pcover F;
INOUT pcost cost;
{
    register pcube p, last;
    pcube *T;
    int var;

    /* use the routine used by cofactor to decide splitting variables */
    massive_count(T = cube1list(F));
    free_cubelist(T);

    cost->cubes = F->count;
    cost->total = cost->in = cost->out = cost->mv = cost->primes = 0;

    /* Count transistors (zeros) for each binary variable (inputs) */
    for(var = 0; var < cube.num_binary_vars; var++)
	cost->in += cdata.var_zeros[var];

    /* Count transistors for each mv variable based on sparse/dense */
    for(var = cube.num_binary_vars; var < cube.num_vars - 1; var++)
	if (cube.sparse[var])
	    cost->mv += F->count * cube.part_size[var] - cdata.var_zeros[var];
	else
	    cost->mv += cdata.var_zeros[var];

    /* Count the transistors (ones) for the output variable */
    if (cube.num_binary_vars != cube.num_vars) {
	var = cube.num_vars - 1;
	cost->out = F->count * cube.part_size[var] - cdata.var_zeros[var];
    }

    /* Count the number of nonprime cubes */
    foreach_set(F, last, p)
	cost->primes += TESTP(p, PRIME) != 0;

    /* Count the total number of literals */
    cost->total = cost->in + cost->out + cost->mv;
}


/* fmt_cost -- return a string which reports the "cost" of a cover */
char *fmt_cost(cost)
IN pcost cost;
{
    static char s[200];

    if (cube.num_binary_vars == cube.num_vars - 1)
	(void) sprintf(s, "c=%d(%d) in=%d out=%d tot=%d",
	    cost->cubes, cost->cubes - cost->primes, cost->in,
	    cost->out, cost->total);
    else
	(void) sprintf(s, "c=%d(%d) in=%d mv=%d out=%d",
	   cost->cubes, cost->cubes - cost->primes, cost->in,
	   cost->mv, cost->out);
    return s;
}


char *print_cost(F)
IN pcover F;
{
    cost_t cost;
    cover_cost(F, &cost);
    return fmt_cost(&cost);
}


/* copy_cost -- copy a cost function from s to d */
void copy_cost(s, d)
pcost s, d;
{
    d->cubes = s->cubes;
    d->in = s->in;
    d->out = s->out;
    d->mv = s->mv;
    d->total = s->total;
    d->primes = s->primes;
}


/* size_stamp -- print single line giving the size of a cover */
void size_stamp(T, name)
IN pcover T;
IN char *name;
{
    (void) printf("# %s\tCost is %s\n", name, print_cost(T));
    (void) fflush(stdout);
}


/* print_trace -- print a line reporting size and time after a function */
void print_trace(T, name, time)
pcover T;
char *name;
long time;
{
    (void) printf("# %s\tTime was %s, cost is %s\n",
	name, print_time(time), print_cost(T));
    (void) fflush(stdout);
}


/* totals -- add time spent in the function into the totals */
void totals(time, i, T, cost)
long time;
int i;
pcover T;
pcost cost;
{
    time = ptime() - time;
    total_time[i] += time;
    total_calls[i]++;
    cover_cost(T, cost);
    if (trace) {
	(void) printf("# %s\tTime was %s, cost is %s\n",
	    total_name[i], print_time(time), fmt_cost(cost));
	(void) fflush(stdout);
    }
}


/* fatal -- report fatal error message and take a dive */
void fatal(s)
char *s;
{
    (void) fprintf(stderr, "espresso: %s\n", s);
    exit(1);
}
ABC_NAMESPACE_IMPL_END

