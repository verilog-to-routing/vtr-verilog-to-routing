/*	set.c

	The following is a general-purpose set library originally developed
	by Hank Dietz and enhanced by Terence Parr to allow dynamic sets.
	
	Sets are now structs containing the #words in the set and
	a pointer to the actual set words.
	
	Generally, sets need not be explicitly allocated.  They are
	created/extended/shrunk when appropriate (e.g. in set_of()).
	HOWEVER, sets need to be destroyed (free()ed) when they go out of scope
	or are otherwise no longer needed.  A routine is provided to
	free a set.
	
	Sets can be explicitly created with set_new(s, max_elem).
	
	Sets can be declared to have minimum size to reduce realloc traffic.
	Default minimum size = 1.
	
	Sets can be explicitly initialized to have no elements (set.n == 0)
	by using the 'empty' initializer:
	
	Examples:
		set a = empty;	-- set_deg(a) == 0
		
		return( empty );
	
	Example set creation and destruction:
	
	set
	set_of2(e,g)
	unsigned e,g;
	{
		set a,b,c;
		
		b = set_of(e);		-- Creates space for b and sticks in e
		set_new(c, g);		-- set_new(); set_orel() ==> set_of()
		set_orel(g, &c);
		a = set_or(b, c);
		.
		.
		.
		set_free(b);
		set_free(c);
		return( a );
	}

	1987 by Hank Dietz
	
	Modified by:
		Terence Parr
		Purdue University
		October 1989

	Made it smell less bad to C++ 7/31/93 -- TJP
*/

#include <stdio.h>
#ifdef __cplusplus
#ifndef __STDC__
#define __STDC__
#endif
#endif
#ifdef __STDC__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <string.h>

#include "set.h"

/* elems can be a maximum of 32 bits */
static unsigned bitmask[] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
#if !defined(PC) || defined(PC32)
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000
#endif
};

set empty = set_init;
static unsigned min=1;

#define StrSize		200

#ifdef MEMCHK
#define CHK(a)					\
	if ( a.setword != NULL )	\
	  if ( !valid(a.setword) )	\
		{fprintf(stderr, "%s(%d): invalid set\n",__FILE__,__LINE__); exit(-1);}
#else
#define CHK(a)
#endif

/*
 * Set the minimum size (in words) of a set to reduce realloc calls
 */
void
#ifdef __STDC__
set_size( unsigned n )
#else
set_size( n )
unsigned n;
#endif
{
	min = n;
}

unsigned int
#ifdef __STDC__
set_deg( set a )
#else
set_deg( a )
set a;
#endif
{
	/* Fast compute degree of a set... the number
	   of elements present in the set.  Assumes
	   that all word bits are used in the set
	   and that SETSIZE(a) is a multiple of WORDSIZE.
	*/
	register unsigned *p = &(a.setword[0]);
	register unsigned *endp = &(a.setword[a.n]);
	register unsigned degree = 0;

	CHK(a);
	if ( a.n == 0 ) return(0);
	while ( p < endp )
	{
		register unsigned t = *p;
		register unsigned *b = &(bitmask[0]);
		do {
			if (t & *b) ++degree;
		} while (++b < &(bitmask[WORDSIZE]));
		p++;
	}

	return(degree);
}

set
#ifdef __STDC__
set_or( set b, set c )
#else
set_or( b, c )
set b;
set c;
#endif
{
	/* Fast set union operation */
	/* resultant set size is max(b, c); */
	set *big;
	set t;
	unsigned int m,n;
	register unsigned *r, *p, *q, *endp;

	CHK(b); CHK(c);
	t = empty;
	if (b.n > c.n) {big= &b; m=b.n; n=c.n;} else {big= &c; m=c.n; n=b.n;}
	set_ext(&t, m);
	r = t.setword;

	/* Or b,c until max of smaller set */
	q = c.setword;
	p = b.setword;
	endp = &(b.setword[n]);
	while ( p < endp ) *r++ = *p++ | *q++;	

	/* Copy rest of bigger set into result */
	p = &(big->setword[n]);
	endp = &(big->setword[m]);
	while ( p < endp ) *r++ = *p++;

	return(t);
}

set
#ifdef __STDC__
set_and( set b, set c )
#else
set_and( b, c )
set b;
set c;
#endif
{
	/* Fast set intersection operation */
	/* resultant set size is min(b, c); */
	set t;
	unsigned int n;
	register unsigned *r, *p, *q, *endp;

	CHK(b); CHK(c);
	t = empty;
	n = (b.n > c.n) ? c.n : b.n;
	if ( n == 0 ) return t;		/* TJP 4-27-92 fixed for empty set */
	set_ext(&t, n);
	r = t.setword;

	/* & b,c until max of smaller set */
	q = c.setword;
	p = b.setword;
	endp = &(b.setword[n]);
	while ( p < endp ) *r++ = *p++ & *q++;	

	return(t);
}

set
#ifdef __STDC__
set_dif( set b, set c )
#else
set_dif( b, c )
set b;
set c;
#endif
{
	/* Fast set difference operation b - c */
	/* resultant set size is size(b) */
	set t;
	unsigned int n;
	register unsigned *r, *p, *q, *endp;

	CHK(b); CHK(c);
	t = empty;
	n = (b.n <= c.n) ? b.n : c.n ;
	if ( b.n == 0 ) return t;		/* TJP 4-27-92 fixed for empty set */
									/* WEC 12-1-92 fixed for c.n = 0 */
	set_ext(&t, b.n);
	r = t.setword;

	/* Dif b,c until smaller set size */
	q = c.setword;
	p = b.setword;
	endp = &(b.setword[n]);
	while ( p < endp ) *r++ = *p++ & (~ *q++);	

	/* Copy rest of b into result if size(b) > c */
	if ( b.n > n )
	{
		p = &(b.setword[n]);
		endp = &(b.setword[b.n]);
		while ( p < endp ) *r++ = *p++;
	}

	return(t);
}

set
#ifdef __STDC__
set_of( unsigned b )
#else
set_of( b )
unsigned b;
#endif
{
	/* Fast singleton set constructor operation */
	static set a;

	if ( b == nil ) return( empty );
	set_new(a, b);
	a.setword[DIVWORD(b)] = bitmask[MODWORD(b)];

	return(a);
}

/*
 * Extend (or shrink) the set passed in to have n words.
 *
 * if n is smaller than the minimum, boost n to have the minimum.
 * if the new set size is the same as the old one, do nothing.
 *
 * TJP 4-27-92 Fixed so won't try to alloc 0 bytes
 */
void
#ifdef __STDC__
set_ext( set *a, unsigned int n )
#else
set_ext( a, n )
set *a;
unsigned int n;
#endif
{
	register unsigned *p;
	register unsigned *endp;
	unsigned int size;
	
	CHK((*a));
    if ( a->n == 0 )
    {
		if ( n == 0 ) return;
        a->setword = (unsigned *) calloc(n, BytesPerWord);
        if ( a->setword == NULL )
        {
            fprintf(stderr, "set_ext(%d words): cannot allocate set\n", n);
            exit(-1);
        }
        a->n = n;
        return;
    }
	if ( n < min ) n = min;
	if ( a->n == n || n == 0 ) return;
	size = a->n;
	a->n = n;
	a->setword = (unsigned *) realloc( (char *)a->setword, (n*BytesPerWord) );
	if ( a->setword == NULL )
	{
		fprintf(stderr, "set_ext(%d words): cannot allocate set\n", n);
		exit(-1);
	}

	p    = &(a->setword[size]);		/* clear from old size to new size */
	endp = &(a->setword[a->n]);
	do {
		*p++ = 0;
	} while ( p < endp );
}

set
#ifdef __STDC__
set_not( set a )
#else
set_not( a )
set a;
#endif
{
	/* Fast not of set a (assumes all bits used) */
	/* size of resultant set is size(a) */
	/* ~empty = empty cause we don't know how bit to make set */
	set t;
	register unsigned *r;
	register unsigned *p = a.setword;
	register unsigned *endp = &(a.setword[a.n]);

	CHK(a);
	t = empty;
	if ( a.n == 0 ) return( empty );
	set_ext(&t, a.n);
	r = t.setword;
	
	do {
		*r++ = (~ *p++);
	} while ( p < endp );

	return(t);
}

int
#ifdef __STDC__
set_equ( set a, set b )
#else
set_equ( a, b )
set a;
set b;
#endif
{
	/* Fast set equality comparison operation */
	register unsigned *p = a.setword;
	register unsigned *q = b.setword;
	register unsigned *endp = &(a.setword[a.n]);

	CHK(a); CHK(b);
/*	if ( a.n != b.n ) return(0);*/
	if ( set_deg(a) != set_deg(b) ) return(0);
	else if ( a.n==0 ) return(1);	/* empty == empty */
	
	do {
		if (*p != *q) return(0);
		++q;
	} while ( ++p < endp );

	return(1);
}

int
#ifdef __STDC__
set_sub( set a, set b )
#else
set_sub( a, b )
set a;
set b;
#endif
{
	/* Fast check for a is a proper subset of b (alias a < b) */
	register unsigned *p = a.setword;
	register unsigned *q = b.setword;
	register unsigned *endp = &(a.setword[a.n]);
	register int asubset = 0;

	CHK(a); CHK(b);
	if ( set_deg(a) > set_deg(b) ) return(0);
	if ( set_deg(a)==0 && set_deg(b)==0) return(1);/* empty prop sub of empty */
	if ( set_deg(a) == 0 ) return(1);		/* empty is sub of everything */
#ifdef DUH
/* Was: */
	if ( a.n > b.n ) return(0);
	if (a.n==0 && b.n==0) return(1);/* empty prop sub of empty */
	if ( a.n == 0 ) return(1);		/* empty is sub of everything */
#endif

	do {
		/* Prune tests based on guess that most set words
		   will match, particularly if a is a subset of b.
		*/
		if (*p != *q) {
			if (*p & ~(*q)) {
				/* Fail -- a contains something b does not */
				return(0);
			}
			/* At least this word was a proper subset, hence
			   even if all other words are equal, a is a
			   proper subset of b.
			*/
			asubset = 1;
		}
		++q;
	} while (++p < endp);

	/* at this point, a,b are equ or a subset */
	if ( asubset || b.n == a.n ) return(asubset);
	
	/* if !asubset, size(b) > size(a), then a=b and must check rest of b */
	p = q;
	endp = &(b.setword[b.n]);
	do
	{
		if ( *p++ ) return(1);
	} while ( p < endp );

	return(0);
}

unsigned
#ifdef __STDC__
set_int( set b )
#else
set_int( b )
set b;
#endif
{
	/* Fast pick any element of the set b */
	register unsigned *p = b.setword;
	register unsigned *endp = &(b.setword[b.n]);

	CHK(b);
	if ( b.n == 0 ) return( nil );

	do {
		if (*p) {
			/* Found a non-empty word of the set */
			register unsigned i = ((p - b.setword) << LogWordSize);
			register unsigned t = *p;
			p = &(bitmask[0]);
			while (!(*p & t)) {
				++i; ++p;
			}
			return(i);
		}
	} while (++p < endp);

	/* Empty -- only element it contains is nil */
	return(nil);
}

int
#ifdef __STDC__
set_el( unsigned b, set a )
#else
set_el( b, a )
unsigned b;
set a;
#endif
{
	CHK(a);
	/* nil is an element of every set */
	if (b == nil) return(1);
	if ( a.n == 0 || NumWords(b) > a.n ) return(0);
	
	/* Otherwise, we have to check */
	return( a.setword[DIVWORD(b)] & bitmask[MODWORD(b)] );
}

int
#ifdef __STDC__
set_nil( set a )
#else
set_nil( a )
set a;
#endif
{
	/* Fast check for nil set */
	register unsigned *p = a.setword;
	register unsigned *endp = &(a.setword[a.n]);

	CHK(a);
	if ( a.n == 0 ) return(1);
	/* The set is not empty if any word used to store
	   the set is non-zero.  This means one must be a
	   bit careful about doing things like negation.
	*/
	do {
		if (*p) return(0);
	} while (++p < endp);
	
	return(1);
}

char *
#ifdef __STDC__
set_str( set a )
#else
set_str( a )
set a;
#endif
{
	/* Fast convert set a into ASCII char string...
	   assumes that all word bits are used in the set
	   and that SETSIZE is a multiple of WORDSIZE.
	   Trailing 0 bits are removed from the string.
	   if no bits are on or set is empty, "" is returned.
	*/
	register unsigned *p = a.setword;
	register unsigned *endp = &(a.setword[a.n]);
	static char str_tmp[StrSize+1];
	register char *q = &(str_tmp[0]);

	CHK(a);
	if ( a.n==0 ) {*q=0; return( &(str_tmp[0]) );}
	do {
		register unsigned t = *p;
		register unsigned *b = &(bitmask[0]);
		do {
			*(q++) = (char) ((t & *b) ? '1' : '0');
		} while (++b < &(bitmask[WORDSIZE]));
	} while (++p < endp);

	/* Trim trailing 0s & NULL terminate the string */
	while ((q > &(str_tmp[0])) && (*(q-1) != '1')) --q;
	*q = 0;

	return(&(str_tmp[0]));
}

set
#ifdef __STDC__
set_val( register char *s )
#else
set_val( s )
register char *s;
#endif
{
	/* Fast convert set ASCII char string into a set.
	   If the string ends early, the remaining set bits
	   are all made zero.
	   The resulting set size is just big enough to hold all elements.
	*/
	static set a;
	register unsigned *p, *endp;

	set_new(a, strlen(s));
	p = a.setword;
	endp = &(a.setword[a.n]);
	do {
		register unsigned *b = &(bitmask[0]);
		/* Start with a word with no bits on */
		*p = 0;
		do {
			if (*s) {
				if (*s == '1') {
					/* Turn-on this bit */
					*p |= *b;
				}
				++s;
			}
		} while (++b < &(bitmask[WORDSIZE]));
	} while (++p < endp);

	return(a);
}

/*
 * Or element e into set a.  a can be empty.
 */
void
#ifdef __STDC__
set_orel( unsigned e, set *a )
#else
set_orel( e, a )
unsigned e;
set *a;
#endif
{
	CHK((*a));
	if ( e == nil ) return;
	if ( NumWords(e) > a->n ) set_ext(a, NumWords(e));
	a->setword[DIVWORD(e)] |= bitmask[MODWORD(e)];
}

/*
 * Or set b into set a.  a can be empty. does nothing if b empty.
 */
void
#ifdef __STDC__
set_orin( set *a, set b )
#else
set_orin( a, b )
set *a;
set b;
#endif
{
	/* Fast set union operation */
	/* size(a) is max(a, b); */
	unsigned int m;
	register unsigned *p,
					  *q    = b.setword,
					  *endq = &(b.setword[b.n]);

	CHK((*a)); CHK(b);
	if ( b.n == 0 ) return;
	m = (a->n > b.n) ? a->n : b.n;
	set_ext(a, m);
	p = a->setword;
	do {
		*p++ |= *q++;
	} while ( q < endq );
}

void
#ifdef __STDC__
set_rm( unsigned e, set a )
#else
set_rm( e, a )
unsigned e;
set a;
#endif
{
	/* Does not effect size of set */
	CHK(a);
	if ( (e == nil) || (NumWords(e) > a.n) ) return;
	a.setword[DIVWORD(e)] ^= (a.setword[DIVWORD(e)]&bitmask[MODWORD(e)]);
}

void
#ifdef __STDC__
set_clr( set a )
#else
set_clr( a )
set a;
#endif
{
	/* Does not effect size of set */
	register unsigned *p = a.setword;
	register unsigned *endp = &(a.setword[a.n]);
	
	CHK(a);
	if ( a.n == 0 ) return;
	do {
		*p++ = 0;
	} while ( p < endp );
}

set
#ifdef __STDC__
set_dup( set a )
#else
set_dup( a )
set a;
#endif
{
	set b;
	register unsigned *p,
					  *q    = a.setword,
					  *endq = &(a.setword[a.n]);
	
	CHK(a);
	b = empty;
	if ( a.n == 0 ) return( empty );
	set_ext(&b, a.n);
	p = b.setword;
	do {
		*p++ = *q++;
	} while ( q < endq );
	
	return(b);
}

/*
 * Return a nil terminated list of unsigned ints that represents all
 * "on" bits in the bit set.
 *
 * e.g. {011011} --> {1, 2, 4, 5, nil}
 *
 * _set_pdq and set_pdq are useful when an operation is required on each element
 * of a set.  Normally, the sequence is:
 *
 *		while ( set_deg(a) > 0 ) {
 *			e = set_int(a);
 *			set_rm(e, a);
 *			...process e...
 *		}
 * Now,
 *
 *		t = e = set_pdq(a);
 *		while ( *e != nil ) {
 *			...process *e...
 *			e++;
 *		}
 *		free( t );
 *
 * We have saved many set calls and have not destroyed set a.
 */
void
#ifdef __STDC__
_set_pdq( set a, register unsigned *q )
#else
_set_pdq( a, q )
set a;
register unsigned *q;
#endif
{
	register unsigned *p = a.setword,
					  *endp = &(a.setword[a.n]);
	register unsigned e=0;

	CHK(a);
	/* are there any space (possibility of elements)? */
	if ( a.n == 0 ) return;
	do {
		register unsigned t = *p;
		register unsigned *b = &(bitmask[0]);
		do {
			if ( t & *b ) *q++ = e;
			++e;
		} while (++b < &(bitmask[WORDSIZE]));
	} while (++p < endp);
	*q = nil;
}

/*
 * Same as _set_pdq except allocate memory.  set_pdq is the natural function
 * to use.
 */
unsigned *
#ifdef __STDC__
set_pdq( set a )
#else
set_pdq( a )
set a;
#endif
{
	unsigned *q;
	int max_deg;
	
	CHK(a);
	max_deg = WORDSIZE*a.n;
	/* assume a.n!=0 & no elements is rare, but still ok */
	if ( a.n == 0 ) return(NULL);
	q = (unsigned *) malloc((max_deg+1)*BytesPerWord);
	if ( q == NULL ) return( NULL );
	_set_pdq(a, q);
	return( q );
}

/* a function that produces a hash number for the set
 */
unsigned int
#ifdef __STDC__
set_hash( set a, register unsigned int mod )
#else
set_hash( a, mod )
set a;
register unsigned int mod;
#endif
{
	/* Fast hash of set a (assumes all bits used) */
	register unsigned *p = &(a.setword[0]);
	register unsigned *endp = &(a.setword[a.n]);
	register unsigned i = 0;

	CHK(a);
	while (p<endp){
		i += (*p);
		++p;
	}

	return(i % mod);
}
