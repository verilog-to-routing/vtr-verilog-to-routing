/**CFile****************************************************************

  FileName    [cloudCore.c]

  PackageName [Fast application-specific BDD package.]

  Synopsis    [The package core.]

  Author      [Alan Mishchenko <alanmi@ece.pdx.edu>]
  
  Affiliation [ECE Department. Portland State University, Portland, Oregon.]

  Date        [Ver. 1.0. Started - June 10, 2002.]

  Revision    [$Id: cloudCore.c,v 1.0 2002/06/10 03:00:00 alanmi Exp $]

***********************************************************************/

#include <string.h>
#include "cloud.h"

ABC_NAMESPACE_IMPL_START


// the number of operators using cache
static int CacheOperNum = 4;

// the ratio of cache size to the unique table size for each operator
static int CacheLogRatioDefault[4] = {
	2, // CLOUD_OPER_AND, 
	8, // CLOUD_OPER_XOR, 
	8, // CLOUD_OPER_BDIFF, 
	8  // CLOUD_OPER_LEQ 
};

// the ratio of cache size to the unique table size for each operator
static int CacheSize[4] = {
	2, // CLOUD_OPER_AND, 
	2, // CLOUD_OPER_XOR, 
	2, // CLOUD_OPER_BDIFF, 
	2  // CLOUD_OPER_LEQ 
};

////////////////////////////////////////////////////////////////////////
///                       FUNCTION DECLARATIONS                      ///
////////////////////////////////////////////////////////////////////////

// static functions
static CloudNode * cloudMakeNode( CloudManager * dd, CloudVar v, CloudNode * t, CloudNode * e );
static void        cloudCacheAllocate( CloudManager * dd, CloudOper oper );

////////////////////////////////////////////////////////////////////////
///                       FUNCTION DEFINITIONS                       ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [Starts the cloud manager.]

  Description [The first arguments is the number of elementary variables used.
  The second arguments is the number of bits of the unsigned integer used to 
  represent nodes in the unique table. If the second argument is 0, the package 
  assumes 23 to represent nodes, which is equivalent to 2^23 = 8,388,608 nodes.]
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
CloudManager * Cloud_Init( int nVars, int nBits )
{
	CloudManager * dd;
	int i;
	abctime clk1, clk2;

	assert( nVars <= 100000 );
	assert( nBits < 32 );

	// assign the defaults
	if ( nBits == 0 )
		nBits = CLOUD_NODE_BITS;

	// start the manager
	dd = ABC_CALLOC( CloudManager, 1 );
	dd->nMemUsed          += sizeof(CloudManager);

	// variables
	dd->nVars             = nVars;              // the number of variables allocated
	// bits
	dd->bitsNode          = nBits;              // the number of bits used for the node
	for ( i = 0; i < CacheOperNum; i++ )
		dd->bitsCache[i]  = nBits - CacheLogRatioDefault[i];
	// shifts
	dd->shiftUnique       = 8*sizeof(unsigned) - (nBits + 1); // gets node index in the hash table
	for ( i = 0; i < CacheOperNum; i++ )
		dd->shiftCache[i] = 8*sizeof(unsigned) - dd->bitsCache[i];
	// nodes
	dd->nNodesAlloc       = (1 << (nBits + 1)); // 2 ^ (nBits + 1)
	dd->nNodesLimit       = (1 << nBits);       // 2 ^  nBits

	// unique table
clk1 = Abc_Clock();
	dd->tUnique           = ABC_CALLOC( CloudNode, dd->nNodesAlloc );
	dd->nMemUsed         += sizeof(CloudNode) * dd->nNodesAlloc;
clk2 = Abc_Clock();
//ABC_PRT( "calloc() time", clk2 - clk1 ); 

	// set up the constant node (the only node that is not in the hash table)
	dd->nSignCur          = 1;
	dd->tUnique[0].s      = dd->nSignCur;
	dd->tUnique[0].v      = CLOUD_CONST_INDEX;
	dd->tUnique[0].e      = NULL;
	dd->tUnique[0].t      = NULL;
	dd->one               = dd->tUnique;
	dd->zero              = Cloud_Not(dd->one);
	dd->nNodesCur         = 1;

	// special nodes
	dd->pNodeStart        = dd->tUnique + 1;
	dd->pNodeEnd          = dd->tUnique + dd->nNodesAlloc;

	// set up the elementary variables
	dd->vars              = ABC_ALLOC( CloudNode *, dd->nVars );
	dd->nMemUsed         += sizeof(CloudNode *) * dd->nVars;
	for ( i = 0; i < dd->nVars; i++ )
		dd->vars[i]   = cloudMakeNode( dd, i, dd->one, dd->zero );

	return dd;
};

/**Function********************************************************************

  Synopsis    [Stops the cloud manager.]

  Description [The first arguments tells show many elementary variables are used.
  The second arguments tells how many bits of the unsigned integer are used
  to represent regular nodes in the unique table.]
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
void Cloud_Quit( CloudManager * dd )
{
	int i;
    ABC_FREE( dd->ppNodes );
	ABC_FREE( dd->tUnique ); 
	ABC_FREE( dd->vars ); 
	for ( i = 0; i < 4; i++ )
		ABC_FREE( dd->tCaches[i] );
	ABC_FREE( dd ); 
}

/**Function********************************************************************

  Synopsis    [Prepares the manager for another run.]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
void Cloud_Restart( CloudManager * dd )
{
    int i;
    assert( dd->one->s == dd->nSignCur );
    dd->nSignCur++;
    dd->one->s++;
	for ( i = 0; i < dd->nVars; i++ )
		dd->vars[i]->s++;
    dd->nNodesCur = 1 + dd->nVars;
}

/**Function********************************************************************

  Synopsis    [This optional function allocates operation cache of the given size.]

  Description [Cache for each operation is allocated independently when the first 
  operation of the given type is performed. The user can allocate cache of his/her 
  preferred size by calling Cloud_CacheAllocate before the first operation of the 
  given type is performed, but this call is optional. Argument "logratio" gives
  the binary logarithm of the ratio of the size of the unique table to that of cache.
  For example, if "logratio" is equal to 3, and the unique table will be 2^3=8 times
  larger than cache; so, if unique table is 2^23 = 8,388,608 nodes, the cache size 
  will be 2^3=8 times smaller and equal to 2^20 = 1,048,576 entries.]
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
void Cloud_CacheAllocate( CloudManager * dd, CloudOper oper, int logratio )
{
	assert( logratio > 0 );            // cache cannot be larger than the unique table 
	assert( logratio < dd->bitsNode ); // cache cannot be smaller than 2 entries

	if ( logratio )
	{
		dd->bitsCache[oper]  = dd->bitsNode - logratio;
		dd->shiftCache[oper] = 8*sizeof(unsigned) - dd->bitsCache[oper];
	}
	cloudCacheAllocate( dd, oper );
}

/**Function********************************************************************

  Synopsis    [Internal cache allocation.]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
void cloudCacheAllocate( CloudManager * dd, CloudOper oper )
{
	int nCacheEntries = (1 << dd->bitsCache[oper]);

	if ( CacheSize[oper] == 1 )
	{
		dd->tCaches[oper] = (CloudCacheEntry2 *)ABC_CALLOC( CloudCacheEntry1, nCacheEntries );
		dd->nMemUsed     += sizeof(CloudCacheEntry1) * nCacheEntries;
	}
	else if ( CacheSize[oper] == 2 )
	{
		dd->tCaches[oper] = (CloudCacheEntry2 *)ABC_CALLOC( CloudCacheEntry2, nCacheEntries );
		dd->nMemUsed     += sizeof(CloudCacheEntry2) * nCacheEntries;
	}
	else if ( CacheSize[oper] == 3 )
	{
		dd->tCaches[oper] = (CloudCacheEntry2 *)ABC_CALLOC( CloudCacheEntry3, nCacheEntries );
		dd->nMemUsed     += sizeof(CloudCacheEntry3) * nCacheEntries;
	}
}



/**Function********************************************************************

  Synopsis    [Returns or creates a new node]

  Description [Checks the unique table for the existance of the node. If the node is 
  present, returns the node. If the node is absent, creates a new node.]
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
CloudNode * Cloud_MakeNode( CloudManager * dd, CloudVar v, CloudNode * t, CloudNode * e )
{
	CloudNode * pRes;
	CLOUD_ASSERT(t); 
	CLOUD_ASSERT(e);
    assert( v < Cloud_V(t) && v < Cloud_V(e) );       // variable should be above in the order
    if ( Cloud_IsComplement(t) )
    {
        pRes = cloudMakeNode( dd, v, Cloud_Not(t), Cloud_Not(e) );
        if ( pRes != NULL )
            pRes = Cloud_Not(pRes);            
    }
    else
        pRes = cloudMakeNode( dd, v, t, e );
    return pRes;
}

/**Function********************************************************************

  Synopsis    [Returns or creates a new node]

  Description [Checks the unique table for the existance of the node. If the node is 
  present, returns the node. If the node is absent, creates a new node.]
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
CloudNode * cloudMakeNode( CloudManager * dd, CloudVar v, CloudNode * t, CloudNode * e )
{
	CloudNode * entryUnique;

	CLOUD_ASSERT(t); 
	CLOUD_ASSERT(e);

	assert( ((int)v) >= 0 && ((int)v) < dd->nVars );  // the variable must be in the range
    assert( v < Cloud_V(t) && v < Cloud_V(e) );       // variable should be above in the order
	assert( !Cloud_IsComplement(t) );                 // the THEN edge must not be complemented

	// make sure we are not searching for the constant node
	assert( t && e );

	// get the unique entry
	entryUnique = dd->tUnique + cloudHashCudd3(v, t, e, dd->shiftUnique);
	while ( entryUnique->s == dd->nSignCur )
	{
		// compare the node
		if ( entryUnique->v == v && entryUnique->t == t && entryUnique->e == e )
		{ // the node is found
			dd->nUniqueHits++;
			return entryUnique;  // returns the node
		}
		// increment the hash value modulus the hash table size
		if ( ++entryUnique - dd->tUnique == dd->nNodesAlloc )
			entryUnique = dd->tUnique + 1;
		// increment the number of steps through the table
		dd->nUniqueSteps++;
	}
	dd->nUniqueMisses++;

	// check if the new node can be created
	if ( ++dd->nNodesCur == dd->nNodesLimit ) 
	{ // initiate the restart
		printf( "Cloud needs restart!\n" );
//		fflush( stdout );
//		exit(1);
		return NULL;
	}
	// create the node
	entryUnique->s   = dd->nSignCur;
	entryUnique->v   = v;
	entryUnique->t   = t;
	entryUnique->e   = e;
	return entryUnique;  // returns the node
}


/**Function********************************************************************

  Synopsis    [Performs the AND or two BDDs]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
CloudNode * cloudBddAnd( CloudManager * dd, CloudNode * f, CloudNode * g )
{
	CloudNode * F, * G, * r;
	CloudCacheEntry2 * cacheEntry;
	CloudNode * fv, * fnv, * gv, * gnv, * t, * e;
	CloudVar  var;

	assert( f <= g );

	// terminal cases
	F = Cloud_Regular(f);
	G = Cloud_Regular(g);
	if ( F == G )
	{
		if ( f == g )
			return f;
		else
			return dd->zero;
	}
	if ( F == dd->one )
	{
		if ( f == dd->one )
			return g;
		else
			return f;
	}

	// check cache
	cacheEntry = dd->tCaches[CLOUD_OPER_AND] + cloudHashCudd2(f, g, dd->shiftCache[CLOUD_OPER_AND]);
//	cacheEntry = dd->tCaches[CLOUD_OPER_AND] + cloudHashBuddy2(f, g, dd->shiftCache[CLOUD_OPER_AND]);
	r = cloudCacheLookup2( cacheEntry, dd->nSignCur, f, g );
	if ( r != NULL )
	{
		dd->nCacheHits++;
		return r;
	}
	dd->nCacheMisses++;


	// compute cofactors
	if ( cloudV(F) <= cloudV(G) )
	{
		var = cloudV(F);
		if ( Cloud_IsComplement(f) )
		{
			fnv = Cloud_Not(cloudE(F));
			fv  = Cloud_Not(cloudT(F));
		}
		else
		{
			fnv = cloudE(F);
			fv  = cloudT(F);
		}
	}
	else
	{
		var = cloudV(G);
		fv  = fnv = f;
	}

	if ( cloudV(G) <= cloudV(F) )
	{
		if ( Cloud_IsComplement(g) )
		{
			gnv = Cloud_Not(cloudE(G));
			gv  = Cloud_Not(cloudT(G));
		}
		else
		{
			gnv = cloudE(G);
			gv  = cloudT(G);
		}
	}
	else
	{
		gv = gnv = g;
	}

	if ( fv <= gv )
		t = cloudBddAnd( dd, fv, gv );
	else
		t = cloudBddAnd( dd, gv, fv );

	if ( t == NULL )
		return NULL;

	if ( fnv <= gnv )
		e = cloudBddAnd( dd, fnv, gnv );
	else
		e = cloudBddAnd( dd, gnv, fnv );

	if ( e == NULL )
		return NULL;

	if ( t == e )
		r = t;
	else
	{
		if ( Cloud_IsComplement(t) )
		{
			r = cloudMakeNode( dd, var, Cloud_Not(t), Cloud_Not(e) );
			if ( r == NULL )
				return NULL;
			r = Cloud_Not(r);
		}
		else
		{
			r = cloudMakeNode( dd, var, t, e );
			if ( r == NULL )
				return NULL;
		}
	}
	cloudCacheInsert2( cacheEntry, dd->nSignCur, f, g, r );
	return r;
}

/**Function********************************************************************

  Synopsis    [Performs the AND or two BDDs]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
static inline CloudNode * cloudBddAnd_gate( CloudManager * dd, CloudNode * f, CloudNode * g )
{
	if ( f <= g )
		return cloudBddAnd(dd,f,g);
	else
		return cloudBddAnd(dd,g,f);
}

/**Function********************************************************************

  Synopsis    [Performs the AND or two BDDs]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
CloudNode * Cloud_bddAnd( CloudManager * dd, CloudNode * f, CloudNode * g )
{
    if ( Cloud_Regular(f) == NULL || Cloud_Regular(g) == NULL )
        return NULL;
	CLOUD_ASSERT(f);
	CLOUD_ASSERT(g);
	if ( dd->tCaches[CLOUD_OPER_AND] == NULL )
		cloudCacheAllocate( dd, CLOUD_OPER_AND );
	return cloudBddAnd_gate( dd, f, g );
}

/**Function********************************************************************

  Synopsis    [Performs the OR or two BDDs]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
CloudNode * Cloud_bddOr( CloudManager * dd, CloudNode * f, CloudNode * g )
{
	CloudNode * res;
    if ( Cloud_Regular(f) == NULL || Cloud_Regular(g) == NULL )
        return NULL;
	CLOUD_ASSERT(f);
	CLOUD_ASSERT(g);
	if ( dd->tCaches[CLOUD_OPER_AND] == NULL )
		cloudCacheAllocate( dd, CLOUD_OPER_AND );
	res = cloudBddAnd_gate( dd, Cloud_Not(f), Cloud_Not(g) );
	res = Cloud_NotCond( res, res != NULL );
	return res;
}

/**Function********************************************************************

  Synopsis    [Performs the XOR or two BDDs]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
CloudNode * Cloud_bddXor( CloudManager * dd, CloudNode * f, CloudNode * g )
{
	CloudNode * t0, * t1, * r;
    if ( Cloud_Regular(f) == NULL || Cloud_Regular(g) == NULL )
        return NULL;
	CLOUD_ASSERT(f);
	CLOUD_ASSERT(g);
	if ( dd->tCaches[CLOUD_OPER_AND] == NULL )
		cloudCacheAllocate( dd, CLOUD_OPER_AND );
	t0 = cloudBddAnd_gate( dd, f, Cloud_Not(g) );
	if ( t0 == NULL )
		return NULL;
	t1 = cloudBddAnd_gate( dd, Cloud_Not(f), g );
	if ( t1 == NULL )
		return NULL;
	r  = Cloud_bddOr( dd, t0, t1 );
	return r;
}



/**Function********************************************************************

  Synopsis    [Performs a DFS from f, clearing the LSB of the next
  pointers.]

  Description []

  SideEffects [None]

  SeeAlso     [cloudSupport cloudDagSize]

******************************************************************************/
static void cloudClearMark( CloudManager * dd, CloudNode * n )
{
	if ( !cloudNodeIsMarked(n) )
		return;
	// clear visited flag
	cloudNodeUnmark(n);
	if ( cloudIsConstant(n) )
		return;
	cloudClearMark( dd, cloudT(n) );
	cloudClearMark( dd, Cloud_Regular(cloudE(n)) );
}

/**Function********************************************************************

  Synopsis    [Performs the recursive step of Cloud_Support.]

  Description [Performs the recursive step of Cloud_Support. Performs a
  DFS from f. The support is accumulated in supp as a side effect. Uses
  the LSB of the then pointer as visited flag.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
static void cloudSupport( CloudManager * dd, CloudNode * n, int * support )
{
	if ( cloudIsConstant(n) || cloudNodeIsMarked(n) )
		return;
	// set visited flag
	cloudNodeMark(n);
	support[cloudV(n)] = 1;
	cloudSupport( dd, cloudT(n), support );
	cloudSupport( dd, Cloud_Regular(cloudE(n)), support );
}

/**Function********************************************************************

  Synopsis    [Finds the variables on which a DD depends.]

  Description [Finds the variables on which a DD depends.
  Returns a BDD consisting of the product of the variables if
  successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
CloudNode * Cloud_Support( CloudManager * dd, CloudNode * n )
{
	CloudNode * res;
	int * support, i;

	CLOUD_ASSERT(n);

	// allocate and initialize support array for cloudSupport
	support = ABC_CALLOC( int, dd->nVars );

	// compute support and clean up markers
	cloudSupport( dd, Cloud_Regular(n), support );
	cloudClearMark( dd, Cloud_Regular(n) );

	// transform support from array to cube
	res = dd->one;
	for ( i = dd->nVars - 1; i >= 0; i-- ) // for each level bottom-up 
		if ( support[i] == 1 )
		{
			res = Cloud_bddAnd( dd, res, dd->vars[i] );
			if ( res == NULL )
				break;
		}
	ABC_FREE( support );
	return res;
}

/**Function********************************************************************

  Synopsis    [Counts the variables on which a DD depends.]

  Description [Counts the variables on which a DD depends.
  Returns the number of the variables if successful; Cloud_OUT_OF_MEM
  otherwise.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
int Cloud_SupportSize( CloudManager * dd, CloudNode * n )
{
	int * support, i, count;

	CLOUD_ASSERT(n);

	// allocate and initialize support array for cloudSupport
	support = ABC_CALLOC( int, dd->nVars );

	// compute support and clean up markers
	cloudSupport( dd, Cloud_Regular(n), support );
	cloudClearMark( dd, Cloud_Regular(n) );

	// count support variables
	count = 0;
	for ( i = 0; i < dd->nVars; i++ )
	{
		if ( support[i] == 1 )
			count++;
	}

	ABC_FREE( support );
	return count;
}


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Cloud_DagSize.]

  Description [Performs the recursive step of Cloud_DagSize. Returns the
  number of nodes in the graph rooted at n.]

  SideEffects [None]

******************************************************************************/
static int cloudDagSize( CloudManager * dd, CloudNode * n )
{
	int tval, eval;
	if ( cloudNodeIsMarked(n) )
		return 0;
	// set visited flag
	cloudNodeMark(n);
	if ( cloudIsConstant(n) )
		return 1;
	tval = cloudDagSize( dd, cloudT(n) );
	eval = cloudDagSize( dd, Cloud_Regular(cloudE(n)) );
	return tval + eval + 1;

}

/**Function********************************************************************

  Synopsis    [Counts the number of nodes in a DD.]

  Description [Counts the number of nodes in a DD. Returns the number
  of nodes in the graph rooted at node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cloud_DagSize( CloudManager * dd, CloudNode * n )
{
	int res;
	res = cloudDagSize( dd, Cloud_Regular( n ) );
	cloudClearMark( dd, Cloud_Regular( n ) );
	return res;

}


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Cloud_DagSize.]

  Description [Performs the recursive step of Cloud_DagSize. Returns the
  number of nodes in the graph rooted at n.]

  SideEffects [None]

******************************************************************************/
static int Cloud_DagCollect_rec( CloudManager * dd, CloudNode * n, int * pCounter )
{
	int tval, eval;
	if ( cloudNodeIsMarked(n) )
		return 0;
	// set visited flag
	cloudNodeMark(n);
	if ( cloudIsConstant(n) )
    {
        dd->ppNodes[(*pCounter)++] = n;
		return 1;
    }
	tval = Cloud_DagCollect_rec( dd, cloudT(n), pCounter );
	eval = Cloud_DagCollect_rec( dd, Cloud_Regular(cloudE(n)), pCounter );
    dd->ppNodes[(*pCounter)++] = n;
	return tval + eval + 1;

}

/**Function********************************************************************

  Synopsis    [Counts the number of nodes in a DD.]

  Description [Counts the number of nodes in a DD. Returns the number
  of nodes in the graph rooted at node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cloud_DagCollect( CloudManager * dd, CloudNode * n )
{
	int res, Counter = 0;
    if ( dd->ppNodes == NULL )
        dd->ppNodes = ABC_ALLOC( CloudNode *, dd->nNodesLimit );
	res = Cloud_DagCollect_rec( dd, Cloud_Regular( n ), &Counter );
	cloudClearMark( dd, Cloud_Regular( n ) );
    assert( res == Counter );
	return res;

}

/**Function********************************************************************

  Synopsis    [Counts the number of nodes in an array of DDs.]

  Description [Counts the number of nodes in a DD. Returns the number
  of nodes in the graph rooted at node.]

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cloud_SharingSize( CloudManager * dd, CloudNode ** pn, int nn )
{
	int res, i;
	res = 0;
	for ( i = 0; i < nn; i++ )
		res += cloudDagSize( dd, Cloud_Regular( pn[i] ) );
	for ( i = 0; i < nn; i++ )
		cloudClearMark( dd, Cloud_Regular( pn[i] ) );
	return res;
}


/**Function********************************************************************

  Synopsis    [Returns one cube contained in the given BDD.]

  Description []

  SideEffects []

******************************************************************************/
CloudNode * Cloud_GetOneCube( CloudManager * dd, CloudNode * bFunc )
{
	CloudNode * bFunc0, * bFunc1, * res;

	if ( Cloud_IsConstant(bFunc) )
		return bFunc;

	// cofactor
	if ( Cloud_IsComplement(bFunc) )
	{
		bFunc0 = Cloud_Not( cloudE(bFunc) );
		bFunc1 = Cloud_Not( cloudT(bFunc) );
	}
	else
	{
		bFunc0 = cloudE(bFunc);
		bFunc1 = cloudT(bFunc);
	}

	// try to find the cube with the negative literal
	res = Cloud_GetOneCube( dd, bFunc0 );
	if ( res == NULL )
		return NULL;

	if ( res != dd->zero )
	{
		res = Cloud_bddAnd( dd, res, Cloud_Not(dd->vars[Cloud_V(bFunc)]) );
	}
	else
	{
		// try to find the cube with the positive literal
		res = Cloud_GetOneCube( dd, bFunc1 );
		if ( res == NULL )
			return NULL;
		assert( res != dd->zero );
		res = Cloud_bddAnd( dd, res, dd->vars[Cloud_V(bFunc)] );
	}
	return res;
}

/**Function********************************************************************

  Synopsis    [Prints the BDD as a set of disjoint cubes to the standard output.]

  Description []

  SideEffects []

******************************************************************************/
void Cloud_bddPrint( CloudManager * dd, CloudNode * Func )
{
	CloudNode * Cube;
	int fFirst = 1;

	if ( Func == dd->zero )
		printf( "Constant 0." );
	else if ( Func == dd->one )
		printf( "Constant 1." );
	else
	{
		while ( 1 )
		{
			Cube = Cloud_GetOneCube( dd, Func );
			if ( Cube == NULL || Cube == dd->zero )
				break;
			if ( fFirst )   	fFirst = 0;
			else				printf( " + " );
			Cloud_bddPrintCube( dd, Cube );
			Func = Cloud_bddAnd( dd, Func, Cloud_Not(Cube) );
		} 
	}
	printf( "\n" );
}

/**Function********************************************************************

  Synopsis    [Prints one cube.]

  Description []

  SideEffects []

******************************************************************************/
void Cloud_bddPrintCube( CloudManager * dd, CloudNode * bCube )
{
	CloudNode * bCube0, * bCube1;

	assert( !Cloud_IsConstant(bCube) );
	while ( 1 )
	{
		// get the node structure
		if ( Cloud_IsConstant(bCube) )
			break;

		// cofactor the cube
		if ( Cloud_IsComplement(bCube) )
		{
			bCube0 = Cloud_Not( cloudE(bCube) );
			bCube1 = Cloud_Not( cloudT(bCube) );
		}
		else
		{
			bCube0 = cloudE(bCube);
			bCube1 = cloudT(bCube);
		}

		if ( bCube0 != dd->zero )
		{
			assert( bCube1 == dd->zero );
			printf( "[%d]'", cloudV(bCube) );
			bCube = bCube0;
		}
		else
		{
			assert( bCube1 != dd->zero );
			printf( "[%d]", cloudV(bCube) );
			bCube = bCube1;
		}
	}
}


/**Function********************************************************************

  Synopsis    [Prints info.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Cloud_PrintInfo( CloudManager * dd )
{
    if ( dd == NULL ) return;
	printf( "The number of unique table nodes allocated = %12d.\n", dd->nNodesAlloc );
	printf( "The number of unique table nodes present   = %12d.\n", dd->nNodesCur );
	printf( "The number of unique table hits            = %12d.\n", dd->nUniqueHits );
	printf( "The number of unique table misses          = %12d.\n", dd->nUniqueMisses );
	printf( "The number of unique table steps           = %12d.\n", dd->nUniqueSteps );
	printf( "The number of cache hits                   = %12d.\n", dd->nCacheHits );
	printf( "The number of cache misses                 = %12d.\n", dd->nCacheMisses );
	printf( "The current signature                      = %12d.\n", dd->nSignCur );
	printf( "The total memory in use                    = %12d.\n", dd->nMemUsed );
}

/**Function********************************************************************

  Synopsis    [Prints the state of the hash table.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Cloud_PrintHashTable( CloudManager * dd )
{
	int i;

	for ( i = 0; i < dd->nNodesAlloc; i++ )
		if ( dd->tUnique[i].v == CLOUD_CONST_INDEX )
			printf( "-" );
		else
			printf( "+" );
	printf( "\n" );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

