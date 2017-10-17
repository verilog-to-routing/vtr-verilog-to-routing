/*
 * Revision Control Information
 *
 * $Source$
 * $Author$
 * $Revision$
 * $Date$
 *
 */
/*
 *  espresso.h -- header file for Espresso-mv
 */

//#include "port.h"
//#include "utility.h"
#include "sparse.h"
#include "mincov.h"

#include "util_hack.h" // added

#define ABC__misc__espresso__espresso_h
#define print_time(t)	util_print_time(t)

#ifdef IBM_WATC
#define void int
#include "short.h"
#endif

#ifdef IBMPC		/* set default options for IBM/PC */
#define NO_INLINE
#define BPI 16
#endif

/*-----THIS USED TO BE set.h----- */

/*
 *  set.h -- definitions for packed arrays of bits
 *
 *   This header file describes the data structures which comprise a
 *   facility for efficiently implementing packed arrays of bits
 *   (otherwise known as sets, cf. Pascal).
 *
 *   A set is a vector of bits and is implemented here as an array of
 *   unsigned integers.  The low order bits of set[0] give the index of
 *   the last word of set data.  The higher order bits of set[0] are
 *   used to store data associated with the set.  The set data is
 *   contained in elements set[1] ... set[LOOP(set)] as a packed bit
 *   array.
 *
 *   A family of sets is a two-dimensional matrix of bits and is
 *   implemented with the data type "set_family".
 *
 *   BPI == 32 and BPI == 16 have been tested and work.
 */


/* Define host machine characteristics of "unsigned int" */
#ifndef ABC__misc__espresso__espresso_h
#define BPI


ABC_NAMESPACE_HEADER_START
             32              /* # bits per integer */
#endif

#if BPI == 32
#define LOGBPI          5               /* log(BPI)/log(2) */
#else
#define LOGBPI          4               /* log(BPI)/log(2) */
#endif

/* Define the set type */
typedef unsigned int *pset;

/* Define the set family type -- an array of sets */
typedef struct set_family {
    int wsize;                  /* Size of each set in 'ints' */
    int sf_size;                /* User declared set size */
    int capacity;               /* Number of sets allocated */
    int count;                  /* The number of sets in the family */
    int active_count;           /* Number of "active" sets */
    pset data;                  /* Pointer to the set data */
    struct set_family *next;    /* For garbage collection */
} set_family_t, *pset_family;

/* Macros to set and test single elements */
#define WHICH_WORD(element)     (((element) >> LOGBPI) + 1)
#define WHICH_BIT(element)      ((element) & (BPI-1))

/* # of ints needed to allocate a set with "size" elements */
#if BPI == 32
#define SET_SIZE(size)          ((size) <= BPI ? 2 : (WHICH_WORD((size)-1) + 1))
#else
#define SET_SIZE(size)          ((size) <= BPI ? 3 : (WHICH_WORD((size)-1) + 2))
#endif

/*
 *  Three fields are maintained in the first word of the set
 *      LOOP is the index of the last word used for set data
 *      LOOPCOPY is the index of the last word in the set
 *      SIZE is available for general use (e.g., recording # elements in set)
 *      NELEM retrieves the number of elements in the set
 */
#define LOOP(set)               (set[0] & 0x03ff)
#define PUTLOOP(set, i)         (set[0] &= ~0x03ff, set[0] |= (i))
#if BPI == 32
#define LOOPCOPY(set)           LOOP(set)
#define SIZE(set)               (set[0] >> 16)
#define PUTSIZE(set, size)      (set[0] &= 0xffff, set[0] |= ((size) << 16))
#else
#define LOOPCOPY(set)           (LOOP(set) + 1)
#define SIZE(set)               (set[LOOP(set)+1])
#define PUTSIZE(set, size)      ((set[LOOP(set)+1]) = (size))
#endif

#define NELEM(set)		(BPI * LOOP(set))
#define LOOPINIT(size)		((size <= BPI) ? 1 : WHICH_WORD((size)-1))

/*
 *      FLAGS store general information about the set
 */
#define SET(set, flag)          (set[0] |= (flag))
#define RESET(set, flag)        (set[0] &= ~ (flag))
#define TESTP(set, flag)        (set[0] & (flag))

/* Flag definitions are ... */
#define PRIME           0x8000          /* cube is prime */
#define NONESSEN        0x4000          /* cube cannot be essential prime */
#define ACTIVE          0x2000          /* cube is still active */
#define REDUND          0x1000          /* cube is redundant(at this point) */
#define COVERED         0x0800          /* cube has been covered */
#define RELESSEN        0x0400          /* cube is relatively essential */

/* Most efficient way to look at all members of a set family */
#define foreach_set(R, last, p)\
    for(p=R->data,last=p+R->count*R->wsize;p<last;p+=R->wsize)
#define foreach_remaining_set(R, last, pfirst, p)\
    for(p=pfirst+R->wsize,last=R->data+R->count*R->wsize;p<last;p+=R->wsize)
#define foreach_active_set(R, last, p)\
    foreach_set(R,last,p) if (TESTP(p, ACTIVE))

/* Another way that also keeps the index of the current set member in i */
#define foreachi_set(R, i, p)\
    for(p=R->data,i=0;i<R->count;p+=R->wsize,i++)
#define foreachi_active_set(R, i, p)\
    foreachi_set(R,i,p) if (TESTP(p, ACTIVE))

/* Looping over all elements in a set:
 *      foreach_set_element(pset p, int i, unsigned val, int base) {
 *		.
 *		.
 *		.
 *      }
 */
#define foreach_set_element(p, i, val, base) 		\
    for(i = LOOP(p); i > 0; )				\
	for(val = p[i], base = --i << LOGBPI; val != 0; base++, val >>= 1)  \
	    if (val & 1)

/* Return a pointer to a given member of a set family */
#define GETSET(family, index)   ((family)->data + (family)->wsize * (index))

/* Allocate and deallocate sets */
#define set_new(size)	set_clear(ALLOC(unsigned int, SET_SIZE(size)), size)
#define set_full(size)	set_fill(ALLOC(unsigned int, SET_SIZE(size)), size)
#define set_save(r)	set_copy(ALLOC(unsigned int, SET_SIZE(NELEM(r))), r)
#define set_free(r)	FREE(r)

/* Check for set membership, remove set element and insert set element */
#define is_in_set(set, e)       (set[WHICH_WORD(e)] & (1 << WHICH_BIT(e)))
#define set_remove(set, e)      (set[WHICH_WORD(e)] &= ~ (1 << WHICH_BIT(e)))
#define set_insert(set, e)      (set[WHICH_WORD(e)] |= 1 << WHICH_BIT(e))

/* Inline code substitution for those places that REALLY need it on a VAX */
#ifdef NO_INLINE
#define INLINEset_copy(r, a)		(void) set_copy(r,a)
#define INLINEset_clear(r, size)	(void) set_clear(r, size)
#define INLINEset_fill(r, size)		(void) set_fill(r, size)
#define INLINEset_and(r, a, b)		(void) set_and(r, a, b)
#define INLINEset_or(r, a, b)		(void) set_or(r, a, b)
#define INLINEset_diff(r, a, b)		(void) set_diff(r, a, b)
#define INLINEset_ndiff(r, a, b, f)	(void) set_ndiff(r, a, b, f)
#define INLINEset_xor(r, a, b)		(void) set_xor(r, a, b)
#define INLINEset_xnor(r, a, b, f)	(void) set_xnor(r, a, b, f)
#define INLINEset_merge(r, a, b, mask)	(void) set_merge(r, a, b, mask)
#define INLINEsetp_implies(a, b, when_false)	\
    if (! setp_implies(a,b)) when_false
#define INLINEsetp_disjoint(a, b, when_false)	\
    if (! setp_disjoint(a,b)) when_false
#define INLINEsetp_equal(a, b, when_false)	\
    if (! setp_equal(a,b)) when_false

#else

#define INLINEset_copy(r, a)\
    {register int i_=LOOPCOPY(a); do r[i_]=a[i_]; while (--i_>=0);}
#define INLINEset_clear(r, size)\
    {register int i_=LOOPINIT(size); *r=i_; do r[i_] = 0; while (--i_ > 0);}
#define INLINEset_fill(r, size)\
    {register int i_=LOOPINIT(size); *r=i_; \
    r[i_]=((unsigned int)(~0))>>(i_*BPI-size); while(--i_>0) r[i_]=~0;}
#define INLINEset_and(r, a, b)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = a[i_] & b[i_]; while (--i_>0);}
#define INLINEset_or(r, a, b)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = a[i_] | b[i_]; while (--i_>0);}
#define INLINEset_diff(r, a, b)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = a[i_] & ~ b[i_]; while (--i_>0);}
#define INLINEset_ndiff(r, a, b, fullset)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = fullset[i_] & (a[i_] | ~ b[i_]); while (--i_>0);}
#ifdef IBM_WATC
#define INLINEset_xor(r, a, b)		(void) set_xor(r, a, b)
#define INLINEset_xnor(r, a, b, f)	(void) set_xnor(r, a, b, f)
#else
#define INLINEset_xor(r, a, b)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = a[i_] ^ b[i_]; while (--i_>0);}
#define INLINEset_xnor(r, a, b, fullset)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = fullset[i_] & ~ (a[i_] ^ b[i_]); while (--i_>0);}
#endif
#define INLINEset_merge(r, a, b, mask)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = (a[i_]&mask[i_]) | (b[i_]&~mask[i_]); while (--i_>0);}
#define INLINEsetp_implies(a, b, when_false)\
    {register int i_=LOOP(a); do if (a[i_]&~b[i_]) break; while (--i_>0);\
    if (i_ != 0) when_false;}
#define INLINEsetp_disjoint(a, b, when_false)\
    {register int i_=LOOP(a); do if (a[i_]&b[i_]) break; while (--i_>0);\
    if (i_ != 0) when_false;}
#define INLINEsetp_equal(a, b, when_false)\
    {register int i_=LOOP(a); do if (a[i_]!=b[i_]) break; while (--i_>0);\
    if (i_ != 0) when_false;}

#endif

#if BPI == 32
#define count_ones(v)\
    (bit_count[v & 255] + bit_count[(v >> 8) & 255]\
    + bit_count[(v >> 16) & 255] + bit_count[(v >> 24) & 255])
#else
#define count_ones(v)   (bit_count[v & 255] + bit_count[(v >> 8) & 255])
#endif

/* Table for efficient bit counting */
extern int bit_count[256];
/*----- END OF set.h ----- */


/* Define a boolean type */
#define bool	int
#define FALSE	0
#define TRUE 	1
#define MAYBE	2
#define print_bool(x) ((x) == 0 ? "FALSE" : ((x) == 1 ? "TRUE" : "MAYBE"))

/* Map many cube/cover types/routines into equivalent set types/routines */
#define pcube                   pset
#define new_cube()              set_new(cube.size)
#define free_cube(r)            set_free(r)
#define pcover                  pset_family
#define new_cover(i)            sf_new(i, cube.size)
#define free_cover(r)           sf_free(r)
#define free_cubelist(T)        FREE(T[0]); FREE(T);


/* cost_t describes the cost of a cover */
typedef struct cost_struct {
    int cubes;			/* number of cubes in the cover */
    int in;			/* transistor count, binary-valued variables */
    int out;			/* transistor count, output part */
    int mv;			/* transistor count, multiple-valued vars */
    int total;			/* total number of transistors */
    int primes;			/* number of prime cubes */
} cost_t, *pcost;


/* pair_t describes bit-paired variables */
typedef struct pair_struct {
    int cnt;
    int *var1;
    int *var2;
} pair_t, *ppair;


/* symbolic_list_t describes a single ".symbolic" line */
typedef struct symbolic_list_struct {
    int variable;
    int pos;
    struct symbolic_list_struct *next;
} symbolic_list_t;


/* symbolic_list_t describes a single ".symbolic" line */
typedef struct symbolic_label_struct {
    char *label;
    struct symbolic_label_struct *next;
} symbolic_label_t;


/* symbolic_t describes a linked list of ".symbolic" lines */
typedef struct symbolic_struct {
    symbolic_list_t *symbolic_list;	/* linked list of items */
    int symbolic_list_length;		/* length of symbolic_list list */
    symbolic_label_t *symbolic_label;	/* linked list of new names */
    int symbolic_label_length;		/* length of symbolic_label list */
    struct symbolic_struct *next;
} symbolic_t;


/* PLA_t stores the logical representation of a PLA */
typedef struct {
    pcover F, D, R;		/* on-set, off-set and dc-set */
    char *filename;             /* filename */
    int pla_type;               /* logical PLA format */
    pcube phase;                /* phase to split into on-set and off-set */
    ppair pair;                 /* how to pair variables */
    char **label;		/* labels for the columns */
    symbolic_t *symbolic;	/* allow binary->symbolic mapping */
    symbolic_t *symbolic_output;/* allow symbolic output mapping */
} PLA_t, *pPLA;

#define equal(a,b)      (strcmp(a,b) == 0)

/* This is a hack which I wish I hadn't done, but too painful to change */
#define CUBELISTSIZE(T)         (((pcube *) T[1] - T) - 3)

/* For documentation purposes */
#define IN
#define OUT
#define INOUT

/* The pla_type field describes the input and output format of the PLA */
#define F_type          1
#define D_type          2
#define R_type          4
#define PLEASURE_type   8               /* output format */
#define EQNTOTT_type    16              /* output format algebraic eqns */
#define KISS_type	128		/* output format kiss */
#define CONSTRAINTS_type	256	/* output the constraints (numeric) */
#define SYMBOLIC_CONSTRAINTS_type 512	/* output the constraints (symbolic) */
#define FD_type (F_type | D_type)
#define FR_type (F_type | R_type)
#define DR_type (D_type | R_type)
#define FDR_type (F_type | D_type | R_type)

/* Definitions for the debug variable */
#define COMPL           0x0001
#define ESSEN           0x0002
#define EXPAND          0x0004
#define EXPAND1         0x0008
#define GASP            0x0010
#define IRRED           0x0020
#define REDUCE          0x0040
#define REDUCE1         0x0080
#define SPARSE          0x0100
#define TAUT            0x0200
#define EXACT           0x0400
#define MINCOV          0x0800
#define MINCOV1         0x1000
#define SHARP           0x2000
#define IRRED1		0x4000

#define VERSION\
    "UC Berkeley, Espresso Version #2.3, Release date 01/31/88"

/* Define constants used for recording program statistics */
#define TIME_COUNT      16
#define READ_TIME       0
#define COMPL_TIME      1
#define ONSET_TIME	2
#define ESSEN_TIME      3
#define EXPAND_TIME     4
#define IRRED_TIME      5
#define REDUCE_TIME     6
#define GEXPAND_TIME    7
#define GIRRED_TIME     8
#define GREDUCE_TIME    9
#define PRIMES_TIME     10
#define MINCOV_TIME	11
#define MV_REDUCE_TIME  12
#define RAISE_IN_TIME   13
#define VERIFY_TIME     14
#define WRITE_TIME	15


/* For those who like to think about PLAs, macros to get at inputs/outputs */
#define NUMINPUTS       cube.num_binary_vars
#define NUMOUTPUTS      cube.part_size[cube.num_vars - 1]

#define POSITIVE_PHASE(pos)\
    (is_in_set(PLA->phase, cube.first_part[cube.output]+pos) != 0)

#define INLABEL(var)    PLA->label[cube.first_part[var] + 1]
#define OUTLABEL(pos)   PLA->label[cube.first_part[cube.output] + pos]

#define GETINPUT(c, pos)\
    ((c[WHICH_WORD(2*pos)] >> WHICH_BIT(2*pos)) & 3)
#define GETOUTPUT(c, pos)\
    (is_in_set(c, cube.first_part[cube.output] + pos) != 0)

#define PUTINPUT(c, pos, value)\
    c[WHICH_WORD(2*pos)] = (c[WHICH_WORD(2*pos)] & ~(3 << WHICH_BIT(2*pos)))\
		| (value << WHICH_BIT(2*pos))
#define PUTOUTPUT(c, pos, value)\
    c[WHICH_WORD(pos)] = (c[WHICH_WORD(pos)] & ~(1 << WHICH_BIT(pos)))\
		| (value << WHICH_BIT(pos))

#define TWO     3
#define DASH    3
#define ONE     2
#define ZERO    1


#define EXEC(fct, name, S)\
    {long t=ptime();fct;if(trace)print_trace(S,name,ptime()-t);}
#define EXEC_S(fct, name, S)\
    {long t=ptime();fct;if(summary)print_trace(S,name,ptime()-t);}
#define EXECUTE(fct,i,S,cost)\
    {long t=ptime();fct;totals(t,i,S,&(cost));}

/*
 *    Global Variable Declarations
 */

extern unsigned int debug;              /* debug parameter */
extern bool verbose_debug;              /* -v:  whether to print a lot */
extern char *total_name[TIME_COUNT];    /* basic function names */
extern long total_time[TIME_COUNT];     /* time spent in basic fcts */
extern int total_calls[TIME_COUNT];     /* # calls to each fct */

extern bool echo_comments;		/* turned off by -eat option */
extern bool echo_unknown_commands;	/* always true ?? */
extern bool force_irredundant;          /* -nirr command line option */
extern bool skip_make_sparse;
extern bool kiss;                       /* -kiss command line option */
extern bool pos;                        /* -pos command line option */
extern bool print_solution;             /* -x command line option */
extern bool recompute_onset;            /* -onset command line option */
extern bool remove_essential;           /* -ness command line option */
extern bool single_expand;              /* -fast command line option */
extern bool summary;                    /* -s command line option */
extern bool trace;                      /* -t command line option */
extern bool unwrap_onset;               /* -nunwrap command line option */
extern bool use_random_order;		/* -random command line option */
extern bool use_super_gasp;		/* -strong command line option */
extern char *filename;			/* filename PLA was read from */
extern bool debug_exact_minimization;   /* dumps info for -do exact */


/*
 *  pla_types are the input and output types for reading/writing a PLA
 */
struct pla_types_struct {
    char *key;
    int value;
};


/*
 *  The cube structure is a global structure which contains information
 *  on how a set maps into a cube -- i.e., number of parts per variable,
 *  number of variables, etc.  Also, many fields are pre-computed to
 *  speed up various primitive operations.
 */
#define CUBE_TEMP       10

struct cube_struct {
    int size;                   /* set size of a cube */
    int num_vars;               /* number of variables in a cube */
    int num_binary_vars;        /* number of binary variables */
    int *first_part;            /* first element of each variable */
    int *last_part;             /* first element of each variable */
    int *part_size;             /* number of elements in each variable */
    int *first_word;            /* first word for each variable */
    int *last_word;             /* last word for each variable */
    pset binary_mask;           /* Mask to extract binary variables */
    pset mv_mask;               /* mask to get mv parts */
    pset *var_mask;             /* mask to extract a variable */
    pset *temp;                 /* an array of temporary sets */
    pset fullset;               /* a full cube */
    pset emptyset;              /* an empty cube */
    unsigned int inmask;        /* mask to get odd word of binary part */
    int inword;                 /* which word number for above */
    int *sparse;                /* should this variable be sparse? */
    int num_mv_vars;            /* number of multiple-valued variables */
    int output;                 /* which variable is "output" (-1 if none) */
};

struct cdata_struct {
    int *part_zeros;            /* count of zeros for each element */
    int *var_zeros;             /* count of zeros for each variable */
    int *parts_active;          /* number of "active" parts for each var */
    bool *is_unate;             /* indicates given var is unate */
    int vars_active;            /* number of "active" variables */
    int vars_unate;             /* number of unate variables */
    int best;                   /* best "binate" variable */
};


extern struct pla_types_struct pla_types[];
extern struct cube_struct cube, temp_cube_save;
extern struct cdata_struct cdata, temp_cdata_save;

#ifdef lint
#define DISJOINT 0x5555
#else
#if BPI == 32
#define DISJOINT 0x55555555
#else
#define DISJOINT 0x5555
#endif


ABC_NAMESPACE_HEADER_END

#endif

/* function declarations */

/* cofactor.c */	extern int binate_split_select();
/* cofactor.c */	extern pcover cubeunlist();
/* cofactor.c */	extern pcube *cofactor();
/* cofactor.c */	extern pcube *cube1list();
/* cofactor.c */	extern pcube *cube2list();
/* cofactor.c */	extern pcube *cube3list();
/* cofactor.c */	extern pcube *scofactor();
/* cofactor.c */	extern void massive_count();
/* compl.c */	extern pcover complement();
/* compl.c */	extern pcover simplify();
/* compl.c */	extern void simp_comp();
/* contain.c */	extern int d1_rm_equal();
/* contain.c */	extern int rm2_contain();
/* contain.c */	extern int rm2_equal();
/* contain.c */	extern int rm_contain();
/* contain.c */	extern int rm_equal();
/* contain.c */	extern int rm_rev_contain();
/* contain.c */	extern pset *sf_list();
/* contain.c */	extern pset *sf_sort();
/* contain.c */	extern pset_family d1merge();
/* contain.c */	extern pset_family dist_merge();
/* contain.c */	extern pset_family sf_contain();
/* contain.c */	extern pset_family sf_dupl();
/* contain.c */	extern pset_family sf_ind_contain();
/* contain.c */	extern pset_family sf_ind_unlist();
/* contain.c */	extern pset_family sf_merge();
/* contain.c */	extern pset_family sf_rev_contain();
/* contain.c */	extern pset_family sf_union();
/* contain.c */	extern pset_family sf_unlist();
/* cubestr.c */	extern void cube_setup();
/* cubestr.c */	extern void restore_cube_struct();
/* cubestr.c */	extern void save_cube_struct();
/* cubestr.c */	extern void setdown_cube();
/* cvrin.c */	extern void PLA_labels();
/* cvrin.c */	extern char *get_word();
/* cvrin.c */	extern int label_index();
/* cvrin.c */	extern int read_pla();
/* cvrin.c */	extern int read_symbolic();
/* cvrin.c */	extern pPLA new_PLA();
/* cvrin.c */	extern void PLA_summary();
/* cvrin.c */	extern void free_PLA();
/* cvrin.c */	extern void parse_pla();
/* cvrin.c */	extern void read_cube();
/* cvrin.c */	extern void skip_line();
/* cvrm.c */	extern void foreach_output_function();
/* cvrm.c */	extern int cubelist_partition();
/* cvrm.c */	extern int so_both_do_espresso();
/* cvrm.c */	extern int so_both_do_exact();
/* cvrm.c */	extern int so_both_save();
/* cvrm.c */	extern int so_do_espresso();
/* cvrm.c */	extern int so_do_exact();
/* cvrm.c */	extern int so_save();
/* cvrm.c */	extern pcover cof_output();
/* cvrm.c */	extern pcover lex_sort();
/* cvrm.c */	extern pcover mini_sort();
/* cvrm.c */	extern pcover random_order();
/* cvrm.c */	extern pcover size_sort();
/* cvrm.c */	extern pcover sort_reduce();
/* cvrm.c */	extern pcover uncof_output();
/* cvrm.c */	extern pcover unravel();
/* cvrm.c */	extern pcover unravel_range();
/* cvrm.c */	extern void so_both_espresso();
/* cvrm.c */	extern void so_espresso();
/* cvrmisc.c */	extern char *fmt_cost();
/* cvrmisc.c */	extern char *print_cost();
/* cvrmisc.c */	extern char *strsav();
/* cvrmisc.c */	extern void copy_cost();
/* cvrmisc.c */	extern void cover_cost();
/* cvrmisc.c */	extern void fatal();
/* cvrmisc.c */	extern void print_trace();
/* cvrmisc.c */	extern void size_stamp();
/* cvrmisc.c */	extern void totals();
/* cvrout.c */	extern char *fmt_cube();
/* cvrout.c */	extern char *fmt_expanded_cube();
/* cvrout.c */	extern char *pc1();
/* cvrout.c */	extern char *pc2();
/* cvrout.c */	extern char *pc3();
/* cvrout.c */	extern void makeup_labels();
/* cvrout.c */	extern void kiss_output();
/* cvrout.c */	extern void kiss_print_cube();
/* cvrout.c */	extern void output_symbolic_constraints();
/* cvrout.c */	extern void cprint();
/* cvrout.c */	extern void debug1_print();
/* cvrout.c */	extern void debug_print();
/* cvrout.c */	extern void eqn_output();
/* cvrout.c */	extern void fpr_header();
/* cvrout.c */	extern void fprint_pla();
/* cvrout.c */	extern void pls_group();
/* cvrout.c */	extern void pls_label();
/* cvrout.c */	extern void pls_output();
/* cvrout.c */	extern void print_cube();
/* cvrout.c */	extern void print_expanded_cube();
/* cvrout.c */	extern void sf_debug_print();
/* equiv.c */	extern void find_equiv_outputs();
/* equiv.c */	extern int check_equiv();
/* espresso.c */	extern pcover espresso();
/* essen.c */	extern bool essen_cube();
/* essen.c */	extern pcover cb_consensus();
/* essen.c */	extern pcover cb_consensus_dist0();
/* essen.c */	extern pcover essential();
/* exact.c */	extern pcover minimize_exact();
/* exact.c */	extern pcover minimize_exact_literals();
/* expand.c */	extern bool feasibly_covered();
/* expand.c */	extern int most_frequent();
/* expand.c */	extern pcover all_primes();
/* expand.c */	extern pcover expand();
/* expand.c */	extern pcover find_all_primes();
/* expand.c */	extern void elim_lowering();
/* expand.c */	extern void essen_parts();
/* expand.c */	extern void essen_raising();
/* expand.c */	extern void expand1();
/* expand.c */	extern void mincov();
/* expand.c */	extern void select_feasible();
/* expand.c */	extern void setup_BB_CC();
/* gasp.c */	extern pcover expand_gasp();
/* gasp.c */	extern pcover irred_gasp();
/* gasp.c */	extern pcover last_gasp();
/* gasp.c */	extern pcover super_gasp();
/* gasp.c */	extern void expand1_gasp();
/* getopt.c */	extern int util_getopt();
/* hack.c */	extern void find_dc_inputs();
/* hack.c */	extern void find_inputs();
/* hack.c */	extern void form_bitvector();
/* hack.c */	extern void map_dcset();
/* hack.c */	extern void map_output_symbolic();
/* hack.c */	extern void map_symbolic();
/* hack.c */	extern pcover map_symbolic_cover();
/* hack.c */	extern void symbolic_hack_labels();
/* irred.c */	extern bool cube_is_covered();
/* irred.c */	extern bool taut_special_cases();
/* irred.c */	extern bool tautology();
/* irred.c */	extern pcover irredundant();
/* irred.c */	extern void mark_irredundant();
/* irred.c */	extern void irred_split_cover();
/* irred.c */	extern sm_matrix *irred_derive_table();
/* map.c */	extern pset minterms();
/* map.c */	extern void explode();
/* map.c */	extern void map();
/* opo.c */	extern void output_phase_setup();
/* opo.c */	extern pPLA set_phase();
/* opo.c */	extern pcover opo();
/* opo.c */	extern pcube find_phase();
/* opo.c */	extern pset_family find_covers();
/* opo.c */	extern pset_family form_cover_table();
/* opo.c */	extern pset_family opo_leaf();
/* opo.c */	extern pset_family opo_recur();
/* opo.c */	extern void opoall();
/* opo.c */	extern void phase_assignment();
/* opo.c */	extern void repeated_phase_assignment();
/* pair.c */	extern void generate_all_pairs();
/* pair.c */	extern int **find_pairing_cost();
/* pair.c */	extern void find_best_cost();
/* pair.c */	extern int greedy_best_cost();
/* pair.c */	extern void minimize_pair();
/* pair.c */	extern void pair_free();
/* pair.c */	extern void pair_all();
/* pair.c */	extern pcover delvar();
/* pair.c */	extern pcover pairvar();
/* pair.c */	extern ppair pair_best_cost();
/* pair.c */	extern ppair pair_new();
/* pair.c */	extern ppair pair_save();
/* pair.c */	extern void print_pair();
/* pair.c */	extern void find_optimal_pairing();
/* pair.c */	extern void set_pair();
/* pair.c */	extern void set_pair1();
/* primes.c */	extern pcover primes_consensus();
/* reduce.c */	extern bool sccc_special_cases();
/* reduce.c */	extern pcover reduce();
/* reduce.c */	extern pcube reduce_cube();
/* reduce.c */	extern pcube sccc();
/* reduce.c */	extern pcube sccc_cube();
/* reduce.c */	extern pcube sccc_merge();
/* set.c */	extern bool set_andp();
/* set.c */	extern bool set_orp();
/* set.c */	extern bool setp_disjoint();
/* set.c */	extern bool setp_empty();
/* set.c */	extern bool setp_equal();
/* set.c */	extern bool setp_full();
/* set.c */	extern bool setp_implies();
/* set.c */	extern char *pbv1();
/* set.c */	extern char *ps1();
/* set.c */	extern int *sf_count();
/* set.c */	extern int *sf_count_restricted();
/* set.c */	extern int bit_index();
/* set.c */	extern int set_dist();
/* set.c */	extern int set_ord();
/* set.c */	extern void set_adjcnt();
/* set.c */	extern pset set_and();
/* set.c */	extern pset set_clear();
/* set.c */	extern pset set_copy();
/* set.c */	extern pset set_diff();
/* set.c */	extern pset set_fill();
/* set.c */	extern pset set_merge();
/* set.c */	extern pset set_or();
/* set.c */	extern pset set_xor();
/* set.c */	extern pset sf_and();
/* set.c */	extern pset sf_or();
/* set.c */	extern pset_family sf_active();
/* set.c */	extern pset_family sf_addcol();
/* set.c */	extern pset_family sf_addset();
/* set.c */	extern pset_family sf_append();
/* set.c */	extern pset_family sf_bm_read();
/* set.c */	extern pset_family sf_compress();
/* set.c */	extern pset_family sf_copy();
/* set.c */	extern pset_family sf_copy_col();
/* set.c */	extern pset_family sf_delc();
/* set.c */	extern pset_family sf_delcol();
/* set.c */	extern pset_family sf_inactive();
/* set.c */	extern pset_family sf_join();
/* set.c */	extern pset_family sf_new();
/* set.c */	extern pset_family sf_permute();
/* set.c */	extern pset_family sf_read();
/* set.c */	extern pset_family sf_save();
/* set.c */	extern pset_family sf_transpose();
/* set.c */	extern void set_write();
/* set.c */	extern void sf_bm_print();
/* set.c */	extern void sf_cleanup();
/* set.c */	extern void sf_delset();
/* set.c */	extern void sf_free();
/* set.c */	extern void sf_print();
/* set.c */	extern void sf_write();
/* setc.c */	extern bool ccommon();
/* setc.c */	extern bool cdist0();
/* setc.c */	extern bool full_row();
/* setc.c */	extern int ascend();
/* setc.c */	extern int cactive();
/* setc.c */	extern int cdist();
/* setc.c */	extern int cdist01();
/* setc.c */	extern int cvolume();
/* setc.c */	extern int d1_order();
/* setc.c */	extern int d1_order_size();
/* setc.c */	extern int desc1();
/* setc.c */	extern int descend();
/* setc.c */	extern int lex_order();
/* setc.c */	extern int lex_order1();
/* setc.c */	extern pset force_lower();
/* setc.c */	extern void consensus();
/* sharp.c */	extern pcover cb1_dsharp();
/* sharp.c */	extern pcover cb_dsharp();
/* sharp.c */	extern pcover cb_recur_dsharp();
/* sharp.c */	extern pcover cb_recur_sharp();
/* sharp.c */	extern pcover cb_sharp();
/* sharp.c */	extern pcover cv_dsharp();
/* sharp.c */	extern pcover cv_intersect();
/* sharp.c */	extern pcover cv_sharp();
/* sharp.c */	extern pcover dsharp();
/* sharp.c */	extern pcover make_disjoint();
/* sharp.c */	extern pcover sharp();
/* sminterf.c */pset do_sm_minimum_cover();
/* sparse.c */	extern pcover make_sparse();
/* sparse.c */	extern pcover mv_reduce();
/* unate.c */	extern pcover find_all_minimal_covers_petrick();
/* unate.c */	extern pcover map_cover_to_unate();
/* unate.c */	extern pcover map_unate_to_cover();
/* unate.c */	extern pset_family exact_minimum_cover();
/* unate.c */	extern pset_family gen_primes();
/* unate.c */	extern pset_family unate_compl();
/* unate.c */	extern pset_family unate_complement();
/* unate.c */	extern pset_family unate_intersect();
/* verify.c */	extern void PLA_permute();
/* verify.c */	extern bool PLA_verify();
/* verify.c */	extern bool check_consistency();
/* verify.c */	extern bool verify();
