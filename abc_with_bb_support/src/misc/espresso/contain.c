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
    contain.c -- set containment routines

    These are complex routines for performing containment over a
    family of sets, but they have the advantage of being much faster
    than a straightforward n*n routine.

    First the cubes are sorted by size, and as a secondary key they are
    sorted so that if two cubes are equal they end up adjacent.  We can
    than quickly remove equal cubes from further consideration by
    comparing each cube to its neighbor.  Finally, because the cubes
    are sorted by size, we need only check cubes which are larger (or
    smaller) than a given cube for containment.
*/

#include "espresso.h"

ABC_NAMESPACE_IMPL_START



/*
    sf_contain -- perform containment on a set family (delete sets which
    are contained by some larger set in the family).  No assumptions are
    made about A, and the result will be returned in decreasing order of
    set size.
*/
pset_family sf_contain(A)
INOUT pset_family A;            /* disposes of A */
{
    int cnt;
    pset *A1;
    pset_family R;

    A1 = sf_sort(A, descend);           /* sort into descending order */
    cnt = rm_equal(A1, descend);        /* remove duplicates */
    cnt = rm_contain(A1);               /* remove contained sets */
    R = sf_unlist(A1, cnt, A->sf_size); /* recreate the set family */
    sf_free(A);
    return R;
}


/*
    sf_rev_contain -- perform containment on a set family (delete sets which
    contain some smaller set in the family).  No assumptions are made about
    A, and the result will be returned in increasing order of set size
*/
pset_family sf_rev_contain(A)
INOUT pset_family A;            /* disposes of A */
{
    int cnt;
    pset *A1;
    pset_family R;

    A1 = sf_sort(A, ascend);            /* sort into ascending order */
    cnt = rm_equal(A1, ascend);         /* remove duplicates */
    cnt = rm_rev_contain(A1);           /* remove containing sets */
    R = sf_unlist(A1, cnt, A->sf_size); /* recreate the set family */
    sf_free(A);
    return R;
}


/*
    sf_ind_contain -- perform containment on a set family (delete sets which
    are contained by some larger set in the family).  No assumptions are
    made about A, and the result will be returned in decreasing order of
    set size.  Also maintains a set of row_indices to track which rows
    disappear and how the rows end up permuted.
*/
pset_family sf_ind_contain(A, row_indices)
INOUT pset_family A;            /* disposes of A */
INOUT int *row_indices;         /* updated with the new values */
{
    int cnt;
    pset *A1;
    pset_family R;

    A1 = sf_sort(A, descend);           /* sort into descending order */
    cnt = rm_equal(A1, descend);        /* remove duplicates */
    cnt = rm_contain(A1);               /* remove contained sets */
    R = sf_ind_unlist(A1, cnt, A->sf_size, row_indices, A->data);
    sf_free(A);
    return R;
}


/* sf_dupl -- delete duplicate sets in a set family */
pset_family sf_dupl(A)
INOUT pset_family A;            /* disposes of A */
{
    register int cnt;
    register pset *A1;
    pset_family R;

    A1 = sf_sort(A, descend);           /* sort the set family */
    cnt = rm_equal(A1, descend);        /* remove duplicates */
    R = sf_unlist(A1, cnt, A->sf_size); /* recreate the set family */
    sf_free(A);
    return R;
}


/*
    sf_union -- form the contained union of two set families (delete
    sets which are contained by some larger set in the family).  A and
    B are assumed already sorted in decreasing order of set size (and
    the SIZE field is assumed to contain the set size), and the result
    will be returned sorted likewise.
*/
pset_family sf_union(A, B)
INOUT pset_family A, B;         /* disposes of A and B */
{
    int cnt;
    pset_family R;
    pset *A1 = sf_list(A), *B1 = sf_list(B), *E1;

    E1 = ALLOC(pset, MAX(A->count, B->count) + 1);
    cnt = rm2_equal(A1, B1, E1, descend);
    cnt += rm2_contain(A1, B1) + rm2_contain(B1, A1);
    R = sf_merge(A1, B1, E1, cnt, A->sf_size);
    sf_free(A); sf_free(B);
    return R;
}


/*
    dist_merge -- consider all sets to be "or"-ed with "mask" and then
    delete duplicates from the set family.
*/
pset_family dist_merge(A, mask)
INOUT pset_family A;            /* disposes of A */
IN pset mask;                   /* defines variables to mask out */
{
    pset *A1;
    int cnt;
    pset_family R;

    (void) set_copy(cube.temp[0], mask);
    A1 = sf_sort(A, d1_order);
    cnt = d1_rm_equal(A1, d1_order);
    R = sf_unlist(A1, cnt, A->sf_size);
    sf_free(A);
    return R;
}


/*
    d1merge -- perform an efficient distance-1 merge of cubes of A
*/
pset_family d1merge(A, var)
INOUT pset_family A;            /* disposes of A */
IN int var;
{
    return dist_merge(A, cube.var_mask[var]);
}



/* d1_rm_equal -- distance-1 merge (merge cubes which are equal under a mask) */
int d1_rm_equal(A1, compare)
register pset *A1;				/* array of set pointers */
int (*compare)();				/* comparison function */
{
    register int i, j, dest;

    dest = 0;
    if (A1[0] != (pcube) NULL) {
	for(i = 0, j = 1; A1[j] != (pcube) NULL; j++)
	    if ( (*compare)(&A1[i], &A1[j]) == 0) {
		/* if sets are equal (under the mask) merge them */
		(void) set_or(A1[i], A1[i], A1[j]);
	    } else {
		/* sets are unequal, so save the set i */
		A1[dest++] = A1[i];
		i = j;
	    }
	A1[dest++] = A1[i];
    }
    A1[dest] = (pcube) NULL;
    return dest;
}


/* rm_equal -- scan a sorted array of set pointers for duplicate sets */
int rm_equal(A1, compare)
INOUT pset *A1;                 /* updated in place */
IN int (*compare)();
{
    register pset *p, *pdest = A1;

    if (*A1 != NULL) {                  /* If more than one set */
	for(p = A1+1; *p != NULL; p++)
	    if ((*compare)(p, p-1) != 0)
		*pdest++ = *(p-1);
	*pdest++ = *(p-1);
	*pdest = NULL;
    }
    return pdest - A1;
}


/* rm_contain -- perform containment over a sorted array of set pointers */
int rm_contain(A1)
INOUT pset *A1;                 /* updated in place */
{
    register pset *pa, *pb;
    register pset *pcheck = NULL; // Suppress "might be used uninitialized"
    register pset a, b;
    pset *pdest = A1;
    int last_size = -1;

    /* Loop for all cubes of A1 */
    for(pa = A1; (a = *pa++) != NULL; ) {
	/* Update the check pointer if the size has changed */
	if (SIZE(a) != last_size)
	    last_size = SIZE(a), pcheck = pdest;
	for(pb = A1; pb != pcheck; ) {
	    b = *pb++;
	    INLINEsetp_implies(a, b, /* when_false => */ continue);
	    goto lnext1;
	}
	/* set a was not contained by some larger set, so save it */
	*pdest++ = a;
    lnext1: ;
    }

    *pdest = NULL;
    return pdest - A1;
}


/* rm_rev_contain -- perform rcontainment over a sorted array of set pointers */
int rm_rev_contain(A1)
INOUT pset *A1;                 /* updated in place */
{
    register pset *pa, *pb;
    register pset *pcheck = NULL; // Suppress "might be used uninitialized"
    register pset a, b;
    pset *pdest = A1;
    int last_size = -1;

    /* Loop for all cubes of A1 */
    for(pa = A1; (a = *pa++) != NULL; ) {
	/* Update the check pointer if the size has changed */
	if (SIZE(a) != last_size)
	    last_size = SIZE(a), pcheck = pdest;
	for(pb = A1; pb != pcheck; ) {
	    b = *pb++;
	    INLINEsetp_implies(b, a, /* when_false => */ continue);
	    goto lnext1;
	}
	/* the set a did not contain some smaller set, so save it */
	*pdest++ = a;
    lnext1: ;
    }

    *pdest = NULL;
    return pdest - A1;
}


/* rm2_equal -- check two sorted arrays of set pointers for equal cubes */
int rm2_equal(A1, B1, E1, compare)
INOUT register pset *A1, *B1;           /* updated in place */
OUT pset *E1;
IN int (*compare)();
{
    register pset *pda = A1, *pdb = B1, *pde = E1;

    /* Walk through the arrays advancing pointer to larger cube */
    for(; *A1 != NULL && *B1 != NULL; )
	switch((*compare)(A1, B1)) {
	    case -1:    /* "a" comes before "b" */
		*pda++ = *A1++; break;
	    case 0:     /* equal cubes */
		*pde++ = *A1++; B1++; break;
	    case 1:     /* "a" is to follow "b" */
		*pdb++ = *B1++; break;
	}

    /* Finish moving down the pointers of A and B */
    while (*A1 != NULL)
	*pda++ = *A1++;
    while (*B1 != NULL)
	*pdb++ = *B1++;
    *pda = *pdb = *pde = NULL;

    return pde - E1;
}


/* rm2_contain -- perform containment between two arrays of set pointers */
int rm2_contain(A1, B1)
INOUT pset *A1;                 /* updated in place */
IN pset *B1;                    /* unchanged */
{
    register pset *pa, *pb, a, b, *pdest = A1;

    /* for each set in the first array ... */
    for(pa = A1; (a = *pa++) != NULL; ) {
	/* for each set in the second array which is larger ... */
	for(pb = B1; (b = *pb++) != NULL && SIZE(b) > SIZE(a); ) {
	    INLINEsetp_implies(a, b, /* when_false => */ continue);
	    /* set was contained in some set of B, so don't save pointer */
	    goto lnext1;
	}
	/* set wasn't contained in any set of B, so save the pointer */
	*pdest++ = a;
    lnext1: ;
    }

    *pdest = NULL;                      /* sentinel */
    return pdest - A1;                  /* # elements in A1 */
}



/* sf_sort -- sort the sets of A */
pset *sf_sort(A, compare)
IN pset_family A;
IN int (*compare)();
{
    register pset p, last, *pdest, *A1;

    /* Create a single array pointing to each cube of A */
    pdest = A1 = ALLOC(pset, A->count + 1);
    foreach_set(A, last, p) {
	PUTSIZE(p, set_ord(p));         /* compute the set size */
	*pdest++ = p;                   /* save the pointer */
    }
    *pdest = NULL;                      /* Sentinel -- never seen by sort */

    /* Sort cubes by size */
    qsort((char *) A1, A->count, sizeof(pset), compare);
    return A1;
}


/* sf_list -- make a list of pointers to the sets in a set family */
pset *sf_list(A)
IN register pset_family A;
{
    register pset p, last, *pdest, *A1;

    /* Create a single array pointing to each cube of A */
    pdest = A1 = ALLOC(pset, A->count + 1);
    foreach_set(A, last, p)
	*pdest++ = p;                   /* save the pointer */
    *pdest = NULL;                      /* Sentinel */
    return A1;
}


/* sf_unlist -- make a set family out of a list of pointers to sets */
pset_family sf_unlist(A1, totcnt, size)
IN pset *A1;
IN int totcnt, size;
{
    register pset pr, p, *pa;
    pset_family R = sf_new(totcnt, size);

    R->count = totcnt;
    for(pr = R->data, pa = A1; (p = *pa++) != NULL; pr += R->wsize)
	INLINEset_copy(pr, p);
    FREE(A1);
    return R;
}


/* sf_ind_unlist -- make a set family out of a list of pointers to sets */
pset_family sf_ind_unlist(A1, totcnt, size, row_indices, pfirst)
IN pset *A1;
IN int totcnt, size;
INOUT int *row_indices;
IN register pset pfirst;
{
    register pset pr, p, *pa;
    register int i, *new_row_indices;
    pset_family R = sf_new(totcnt, size);

    R->count = totcnt;
    new_row_indices = ALLOC(int, totcnt);
    for(pr = R->data, pa = A1, i=0; (p = *pa++) != NULL; pr += R->wsize, i++) {
	INLINEset_copy(pr, p);
	new_row_indices[i] = row_indices[(p - pfirst)/R->wsize];
    }
    for(i = 0; i < totcnt; i++)
	row_indices[i] = new_row_indices[i];
    FREE(new_row_indices);
    FREE(A1);
    return R;
}


/* sf_merge -- merge three sorted lists of set pointers */
pset_family sf_merge(A1, B1, E1, totcnt, size)
INOUT pset *A1, *B1, *E1;               /* will be disposed of */
IN int totcnt, size;
{
    register pset pr, ps, *pmin, *pmid, *pmax;
    pset_family R;
    pset *temp[3], *swap;
    int i, j, n;

    /* Allocate the result set_family */
    R = sf_new(totcnt, size);
    R->count = totcnt;
    pr = R->data;

    /* Quick bubble sort to order the top member of the three arrays */
    n = 3;  temp[0] = A1;  temp[1] = B1;  temp[2] = E1;
    for(i = 0; i < n-1; i++)
	for(j = i+1; j < n; j++)
	    if (desc1(*temp[i], *temp[j]) > 0) {
		swap = temp[j];
		temp[j] = temp[i];
		temp[i] = swap;
	    }
    pmin = temp[0];  pmid = temp[1];  pmax = temp[2];

    /* Save the minimum element, then update pmin, pmid, pmax */
    while (*pmin != (pset) NULL) {
	ps = *pmin++;
	INLINEset_copy(pr, ps);
	pr += R->wsize;
	if (desc1(*pmin, *pmax) > 0) {
	    swap = pmax; pmax = pmin; pmin = pmid; pmid = swap;
	} else if (desc1(*pmin, *pmid) > 0) {
	    swap = pmin; pmin = pmid; pmid = swap;
	}
    }

    FREE(A1);
    FREE(B1);
    FREE(E1);
    return R;
}
ABC_NAMESPACE_IMPL_END

