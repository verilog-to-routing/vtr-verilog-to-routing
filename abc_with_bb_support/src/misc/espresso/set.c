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
 *   set.c -- routines for maniuplating sets and set families
 */

/* LINTLIBRARY */

#include "espresso.h"

ABC_NAMESPACE_IMPL_START

static pset_family set_family_garbage = NULL;

static void intcpy(d, s, n)
register unsigned int *d, *s;
register long n;
{
    register int i;
    for(i = 0; i < n; i++) {
	*d++ = *s++;
    }
}


/* bit_index -- find first bit (from LSB) in a word (MSB=bit n, LSB=bit 0) */
int bit_index(a)
register unsigned int a;
{
    register int i;
    if (a == 0)
	return -1;
    for(i = 0; (a & 1) == 0; a >>= 1, i++)
	;
    return i;
}


/* set_ord -- count number of elements in a set */
int set_ord(a)
register pset a;
{
    register int i, sum = 0;
    register unsigned int val;
    for(i = LOOP(a); i > 0; i--)
	if ((val = a[i]) != 0)
	    sum += count_ones(val);
    return sum;
}

/* set_dist -- distance between two sets (# elements in common) */
int set_dist(a, b)
register pset a, b;
{
    register int i, sum = 0;
    register unsigned int val;
    for(i = LOOP(a); i > 0; i--)
	if ((val = a[i] & b[i]) != 0)
	    sum += count_ones(val);
    return sum;
}

/* set_clear -- make "r" the empty set of "size" elements */
pset set_clear(r, size)
register pset r;
int size;
{
    register int i = LOOPINIT(size);
    *r = i; do r[i] = 0; while (--i > 0);
    return r;
}

/* set_fill -- make "r" the universal set of "size" elements */
pset set_fill(r, size)
register pset r;
register int size;
{
    register int i = LOOPINIT(size);
    *r = i;
    r[i] = ~ (unsigned) 0;
    r[i] >>= i * BPI - size;
    while (--i > 0)
	r[i] = ~ (unsigned) 0;
    return r;
}

/* set_copy -- copy set a into set r */
pset set_copy(r, a)
register pset r, a;
{
    register int i = LOOPCOPY(a);
    do r[i] = a[i]; while (--i >= 0);
    return r;
}

/* set_and -- compute intersection of sets "a" and "b" */
pset set_and(r, a, b)
register pset r, a, b;
{
    register int i = LOOP(a);
    PUTLOOP(r,i); do r[i] = a[i] & b[i]; while (--i > 0);
    return r;
}

/* set_or -- compute union of sets "a" and "b" */
pset set_or(r, a, b)
register pset r, a, b;
{
    register int i = LOOP(a);
    PUTLOOP(r,i); do r[i] = a[i] | b[i]; while (--i > 0);
    return r;
}

/* set_diff -- compute difference of sets "a" and "b" */
pset set_diff(r, a, b)
register pset r, a, b;
{
    register int i = LOOP(a);
    PUTLOOP(r,i); do r[i] = a[i] & ~b[i]; while (--i > 0);
    return r;
}

/* set_xor -- compute exclusive-or of sets "a" and "b" */
pset set_xor(r, a, b)
register pset r, a, b;
{
    register int i = LOOP(a);
#ifdef IBM_WATC
    PUTLOOP(r,i); do r[i] = (a[i]&~b[i]) | (~a[i]&b[i]); while (--i > 0);
#else
    PUTLOOP(r,i); do r[i] = a[i] ^ b[i]; while (--i > 0);
#endif
    return r;
}

/* set_merge -- compute "a" & "mask" | "b" & ~ "mask" */
pset set_merge(r, a, b, mask)
register pset r, a, b, mask;
{
    register int i = LOOP(a);
    PUTLOOP(r,i); do r[i] = (a[i]&mask[i]) | (b[i]&~mask[i]); while (--i > 0);
    return r;
}

/* set_andp -- compute intersection of sets "a" and "b" , TRUE if nonempty */
bool set_andp(r, a, b)
register pset r, a, b;
{
    register int i = LOOP(a);
    register unsigned int x = 0;
    PUTLOOP(r,i); do {r[i] = a[i] & b[i]; x |= r[i];} while (--i > 0);
    return x != 0;
}

/* set_orp -- compute union of sets "a" and "b" , TRUE if nonempty */
bool set_orp(r, a, b)
register pset r, a, b;
{
    register int i = LOOP(a);
    register unsigned int x = 0;
    PUTLOOP(r,i); do {r[i] = a[i] | b[i]; x |= r[i];} while (--i > 0);
    return x != 0;
}

/* setp_empty -- check if the set "a" is empty */
bool setp_empty(a)
register pset a;
{
    register int i = LOOP(a);
    do if (a[i]) return FALSE; while (--i > 0);
    return TRUE;
}

/* setp_full -- check if the set "a" is the full set of "size" elements */
bool setp_full(a, size)
register pset a;
register int size;
{
    register int i = LOOP(a);
    register unsigned int test;
    test = ~ (unsigned) 0;
    test >>= i * BPI - size;
    if (a[i] != test)
	return FALSE;
    while (--i > 0)
	if (a[i] != (~(unsigned) 0))
	    return FALSE;
    return TRUE;
}

/* setp_equal -- check if the set "a" equals set "b" */
bool setp_equal(a, b)
register pset a, b;
{
    register int i = LOOP(a);
    do if (a[i] != b[i]) return FALSE; while (--i > 0);
    return TRUE;
}

/* setp_disjoint -- check if intersection of "a" and "b" is empty */
bool setp_disjoint(a, b)
register pset a, b;
{
    register int i = LOOP(a);
    do if (a[i] & b[i]) return FALSE; while (--i > 0);
    return TRUE;
}

/* setp_implies -- check if "a" implies "b" ("b" contains "a") */
bool setp_implies(a, b)
register pset a, b;
{
    register int i = LOOP(a);
    do if (a[i] & ~b[i]) return FALSE; while (--i > 0);
    return TRUE;
}

/* sf_or -- form the "or" of all sets in a set family */
pset sf_or(A)
pset_family A;
{
    register pset or, last, p;

    or = set_new(A->sf_size);
    foreach_set(A, last, p)
	INLINEset_or(or, or, p);
    return or;
}

/* sf_and -- form the "and" of all sets in a set family */
pset sf_and(A)
pset_family A;
{
    register pset and, last, p;

    and = set_fill(set_new(A->sf_size), A->sf_size);
    foreach_set(A, last, p)
	INLINEset_and(and, and, p);
    return and;
}

/* sf_active -- make all members of the set family active */
pset_family sf_active(A)
pset_family A;
{
    register pset p, last;
    foreach_set(A, last, p) {
	SET(p, ACTIVE);
    }
    A->active_count = A->count;
    return A;
}


/* sf_inactive -- remove all inactive cubes in a set family */
pset_family sf_inactive(A)
pset_family A;
{
    register pset p, last, pdest;

    pdest = A->data;
    foreach_set(A, last, p) {
	if (TESTP(p, ACTIVE)) {
	    if (pdest != p) {
		INLINEset_copy(pdest, p);
	    }
	    pdest += A->wsize;
	} else {
	    A->count--;
	}
    }
    return A;
}


/* sf_copy -- copy a set family */
pset_family sf_copy(R, A)
pset_family R, A;
{
    R->sf_size = A->sf_size;
    R->wsize = A->wsize;
/*R->capacity = A->count;*/
/*R->data = REALLOC(unsigned int, R->data, (long) R->capacity * R->wsize);*/
    R->count = A->count;
    R->active_count = A->active_count;
    intcpy(R->data, A->data, (long) A->wsize * A->count);
    return R;
}


/* sf_join -- join A and B into a single set_family */
pset_family sf_join(A, B)
pset_family A, B;
{
    pset_family R;
    long asize = A->count * A->wsize;
    long bsize = B->count * B->wsize;

    if (A->sf_size != B->sf_size) fatal("sf_join: sf_size mismatch");
    R = sf_new(A->count + B->count, A->sf_size);
    R->count = A->count + B->count;
    R->active_count = A->active_count + B->active_count;
    intcpy(R->data, A->data, asize);
    intcpy(R->data + asize, B->data, bsize);
    return R;
}


/* sf_append -- append the sets of B to the end of A, and dispose of B */
pset_family sf_append(A, B)
pset_family A, B;
{
    long asize = A->count * A->wsize;
    long bsize = B->count * B->wsize;

    if (A->sf_size != B->sf_size) fatal("sf_append: sf_size mismatch");
    A->capacity = A->count + B->count;
    A->data = REALLOC(unsigned int, A->data, (long) A->capacity * A->wsize);
    intcpy(A->data + asize, B->data, bsize);
    A->count += B->count;
    A->active_count += B->active_count;
    sf_free(B);
    return A;
}


/* sf_new -- allocate "num" sets of "size" elements each */
pset_family sf_new(num, size)
int num, size;
{
    pset_family A;
    if (set_family_garbage == NULL) {
	A = ALLOC(set_family_t, 1);
    } else {
	A = set_family_garbage;
	set_family_garbage = A->next;
    }
    A->sf_size = size;
    A->wsize = SET_SIZE(size);
    A->capacity = num;
    A->data = ALLOC(unsigned int, (long) A->capacity * A->wsize);
    A->count = 0;
    A->active_count = 0;
    return A;
}


/* sf_save -- create a duplicate copy of a set family */
pset_family sf_save(A)
register pset_family A;
{
    return sf_copy(sf_new(A->count, A->sf_size), A);
}


/* sf_free -- free the storage allocated for a set family */
void sf_free(A)
pset_family A;
{
    FREE(A->data);
    A->next = set_family_garbage;
    set_family_garbage = A;
}


/* sf_cleanup -- free all of the set families from the garbage list */
void sf_cleanup()
{
    register pset_family p, pnext;
    for(p = set_family_garbage; p != (pset_family) NULL; p = pnext) {
	pnext = p->next;
	FREE(p);
    }
    set_family_garbage = (pset_family) NULL;
}


/* sf_addset -- add a set to the end of a set family */
pset_family sf_addset(A, s)
pset_family A;
pset s;
{
    register pset p;

    if (A->count >= A->capacity) {
	A->capacity = A->capacity + A->capacity/2 + 1;
	A->data = REALLOC(unsigned int, A->data, (long) A->capacity * A->wsize);
    }
    p = GETSET(A, A->count++);
    INLINEset_copy(p, s);
    return A;
}

/* sf_delset -- delete a set from a set family */
void sf_delset(A, i)
pset_family A;
int i;
{   (void) set_copy(GETSET(A,i), GETSET(A, --A->count));}

/* sf_print -- print a set_family as a set (list the element numbers) */
void sf_print(A)
pset_family A;
{
    char *ps1();
    register pset p;
    register int i;
    foreachi_set(A, i, p)
	printf("A[%d] = %s\n", i, ps1(p));
}

/* sf_bm_print -- print a set_family as a bit-matrix */
void sf_bm_print(A)
pset_family A;
{
    char *pbv1();
    register pset p;
    register int i;
    foreachi_set(A, i, p)
	printf("[%4d] %s\n", i, pbv1(p, A->sf_size));
}


/* sf_write -- output a set family in an unintelligable manner */
void sf_write(fp, A)
FILE *fp;
pset_family A;
{
    register pset p, last;
    (void) fprintf(fp, "%d %d\n", A->count, A->sf_size);
    foreach_set(A, last, p)
	set_write(fp, p);
    (void) fflush(fp);
}


/* sf_read -- read a set family written by sf_write */
pset_family sf_read(fp)
FILE *fp;
{
    int i, j;
    register pset p, last;
    pset_family A;

    (void) fscanf(fp, "%d %d\n", &i, &j);
    A = sf_new(i, j);
    A->count = i;
    foreach_set(A, last, p) {
	(void) fscanf(fp, "%x", p);
	for(j = 1; j <= LOOP(p); j++)
	    (void) fscanf(fp, "%x", p+j);
    }
    return A;
}


/* set_write -- output a set in an unintelligable manner */
void set_write(fp, a)
register FILE *fp;
register pset a;
{
    register int n = LOOP(a), j;

    for(j = 0; j <= n; j++) {
	(void) fprintf(fp, "%x ", a[j]);
	if ((j+1) % 8 == 0 && j != n)
	    (void) fprintf(fp, "\n\t");
    }
    (void) fprintf(fp, "\n");
}


/* sf_bm_read -- read a set family written by sf_bm_print (almost) */
pset_family sf_bm_read(fp)
FILE *fp;
{
    int i, j, rows, cols;
    register pset pdest;
    pset_family A;

    (void) fscanf(fp, "%d %d\n", &rows, &cols);
    A = sf_new(rows, cols);
    for(i = 0; i < rows; i++) {
	pdest = GETSET(A, A->count++);
	(void) set_clear(pdest, A->sf_size);
	for(j = 0; j < cols; j++) {
	    switch(getc(fp)) {
		case '0':
		    break;
		case '1':
		    set_insert(pdest, j);
		    break;
		default:
		    fatal("Error reading set family");
	    }
	}
	if (getc(fp) != '\n') {
	    fatal("Error reading set family (at end of line)");
	}
    }
    return A;
}



/* ps1 -- convert a set into a printable string */
#define largest_string 120
static char s1[largest_string];
char *ps1(a)
register pset a;
{
    register int i, num, l, len = 0, n = NELEM(a);
    char temp[20];
    bool first = TRUE;

    s1[len++] = '[';
    for(i = 0; i < n; i++)
	if (is_in_set(a,i)) {
	    if (! first)
		s1[len++] = ',';
	    first = FALSE; num = i;
	    /* Generate digits (reverse order) */
	    l = 0; do temp[l++] = num % 10 + '0'; while ((num /= 10) > 0);
	    /* Copy them back in correct order */
	    do s1[len++] = temp[--l]; while (l > 0);
	    if (len > largest_string-15) {
		s1[len++] = '.'; s1[len++] = '.'; s1[len++] = '.';
		break;
	    }
	}

    s1[len++] = ']';
    s1[len++] = '\0';
    return s1;
}

/* pbv1 -- print bit-vector */
char *pbv1(s, n)
pset s;
int n;
{
    register int i;
    for(i = 0; i < n; i++)
	s1[i] = is_in_set(s,i) ? '1' : '0';
    s1[n] = '\0';
    return s1;
}


/* set_adjcnt -- adjust the counts for a set by "weight" */
void
set_adjcnt(a, count, weight)
register pset a;
register int *count, weight;
{
    register int i, base;
    register unsigned int val;

    for(i = LOOP(a); i > 0; ) {
	for(val = a[i], base = --i << LOGBPI; val != 0; base++, val >>= 1) {
	    if (val & 1) {
		count[base] += weight;
	    }
	}
    }
}



/* sf_count -- perform a column sum over a set family */
int *sf_count(A)
pset_family A;
{
    register pset p, last;
    register int i, base, *count;
    register unsigned int val;

    count = ALLOC(int, A->sf_size);
    for(i = A->sf_size - 1; i >= 0; i--) {
	count[i] = 0;
    }

    foreach_set(A, last, p) {
	for(i = LOOP(p); i > 0; ) {
	    for(val = p[i], base = --i << LOGBPI; val != 0; base++, val >>= 1) {
		if (val & 1) {
		    count[base]++;
		}
	    }
	}
    }
    return count;
}


/* sf_count_restricted -- perform a column sum over a set family, restricting
 * to only the columns which are in r; also, the columns are weighted by the
 * number of elements which are in each row
 */
int *sf_count_restricted(A, r)
pset_family A;
register pset r;
{
    register pset p;
    register int i, base, *count;
    register unsigned int val;
    int weight;
    pset last;

    count = ALLOC(int, A->sf_size);
    for(i = A->sf_size - 1; i >= 0; i--) {
	count[i] = 0;
    }

    /* Loop for each set */
    foreach_set(A, last, p) {
	weight = 1024 / (set_ord(p) - 1);
	for(i = LOOP(p); i > 0; ) {
	    for(val=p[i]&r[i], base= --i<<LOGBPI; val!=0; base++, val >>= 1) {
		if (val & 1) {
		    count[base] += weight;
		}
	    }
	}
    }
    return count;
}


/*
 *  sf_delc -- delete columns first ... last of A
 */
pset_family sf_delc(A, first, last)
pset_family A;
int first, last;
{
    return sf_delcol(A, first, last-first + 1);
}


/*
 *  sf_addcol -- add columns to a set family; includes a quick check to see
 *  if there is already enough room (and hence, can avoid copying)
 */
pset_family sf_addcol(A, firstcol, n)
pset_family A;
int firstcol, n;
{
    int maxsize;

    /* Check if adding columns at the end ... */
    if (firstcol == A->sf_size) {
	/* If so, check if there is already enough room */
	maxsize = BPI * LOOPINIT(A->sf_size);
	if ((A->sf_size + n) <= maxsize) {
	    A->sf_size += n;
	    return A;
	}
    }
    return sf_delcol(A, firstcol, -n);
}

/*
 * sf_delcol -- add/delete columns to/from a set family
 *
 *  if n > 0 then n columns starting from firstcol are deleted if n < 0
 *  then n blank columns are inserted starting at firstcol
 *      (i.e., the first new column number is firstcol)
 *
 *  This is done by copying columns in the array which is a relatively
 *  slow operation.
 */
pset_family sf_delcol(A, firstcol, n)
pset_family A;
register int firstcol, n;
{
    register pset p, last, pdest;
    register int i;
    pset_family B;

    B = sf_new(A->count, A->sf_size - n);
    foreach_set(A, last, p) {
	pdest = GETSET(B, B->count++);
	INLINEset_clear(pdest, B->sf_size);
	for(i = 0; i < firstcol; i++)
	    if (is_in_set(p, i))
		set_insert(pdest, i);
	for(i = n > 0 ? firstcol + n : firstcol; i < A->sf_size; i++)
	    if (is_in_set(p, i))
		set_insert(pdest, i - n);
    }
    sf_free(A);
    return B;
}


/*
 *  sf_copy_col -- copy column "srccol" from "src" to column "dstcol" of "dst"
 */
pset_family sf_copy_col(dst, dstcol, src, srccol)
pset_family dst, src;
int dstcol, srccol;
{
    register pset last, p, pdest;
    register int word_test, word_set;
    unsigned int bit_set, bit_test;

    /* CHEAT! form these constants outside the loop */
    word_test = WHICH_WORD(srccol);
    bit_test = 1 << WHICH_BIT(srccol);
    word_set = WHICH_WORD(dstcol);
    bit_set = 1 << WHICH_BIT(dstcol);

    pdest = dst->data;
    foreach_set(src, last, p) {
	if ((p[word_test] & bit_test) != 0)
	    pdest[word_set] |= bit_set;
/*
 *  equivalent code for this is ...
 *	if (is_in_set(p, srccol)) set_insert(pdest, destcol);
 */
	pdest += dst->wsize;
    }
    return dst;
}



/*
 *  sf_compress -- delete columns from a matrix
 */
pset_family sf_compress(A, c)
pset_family A;			/* will be freed */
register pset c;
{
    register pset p;
    register int i, bcol;
    pset_family B;

    /* create a clean set family for the result */
    B = sf_new(A->count, set_ord(c));
    for(i = 0; i < A->count; i++) {
	p = GETSET(B, B->count++);
	INLINEset_clear(p, B->sf_size);
    }

    /* copy each column of A which has a 1 in c */
    bcol = 0;
    for(i = 0; i < A->sf_size; i++) {
	if (is_in_set(c, i)) {
	    (void) sf_copy_col(B, bcol++, A, i);
	}
    }
    sf_free(A);
    return B;
}



/*
 *  sf_transpose -- transpose a bit matrix
 *
 *  There are trickier ways of doing this, but this works.
 */
pset_family sf_transpose(A)
pset_family A;
{
    pset_family B;
    register pset p;
    register int i, j;

    B = sf_new(A->sf_size, A->count);
    B->count = A->sf_size;
    foreachi_set(B, i, p) {
	INLINEset_clear(p, B->sf_size);
    }
    foreachi_set(A, i, p) {
	for(j = 0; j < A->sf_size; j++) {
	    if (is_in_set(p, j)) {
		set_insert(GETSET(B, j), i);
	    }
	}
    }
    sf_free(A);
    return B;
}


/*
 *   sf_permute -- permute the columns of a set_family
 *
 *   permute is an array of integers containing column numbers of A which
 *   are to be retained.
 */
pset_family sf_permute(A, permute, npermute)
pset_family A;
register int *permute, npermute;
{
    pset_family B;
    register pset p, last, pdest;
    register int j;

    B = sf_new(A->count, npermute);
    B->count = A->count;
    foreach_set(B, last, p)
	INLINEset_clear(p, npermute);

    pdest = B->data;
    foreach_set(A, last, p) {
	for(j = 0; j < npermute; j++)
	    if (is_in_set(p, permute[j]))
		set_insert(pdest, j);
	pdest += B->wsize;
    }
    sf_free(A);
    return B;
}
ABC_NAMESPACE_IMPL_END

