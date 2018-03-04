/**CFile****************************************************************

  FileName    [extraUtilPerm.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Permutation computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilPerm.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "misc/vec/vec.h"
#include "aig/gia/gia.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef enum  
{
    ABC_ZDD_OPER_NONE,
    ABC_ZDD_OPER_DIFF,
    ABC_ZDD_OPER_UNION,
    ABC_ZDD_OPER_MIN_UNION,
    ABC_ZDD_OPER_INTER,
    ABC_ZDD_OPER_PERM,
    ABC_ZDD_OPER_PERM_PROD,
    ABC_ZDD_OPER_COF0,
    ABC_ZDD_OPER_COF1,
    ABC_ZDD_OPER_THRESH,
    ABC_ZDD_OPER_DOT_PROD,
    ABC_ZDD_OPER_DOT_PROD_6,
    ABC_ZDD_OPER_INSERT,
    ABC_ZDD_OPER_PATHS,
    ABC_ZDD_OPER_NODES,
    ABC_ZDD_OPER_ITE
} Abc_ZddOper;

typedef struct Abc_ZddObj_ Abc_ZddObj;
struct Abc_ZddObj_ 
{
    unsigned     Var  : 31;
    unsigned     Mark :  1;
    unsigned     True;
    unsigned     False;
};  

typedef struct Abc_ZddEnt_ Abc_ZddEnt;
struct Abc_ZddEnt_ 
{
    int          Arg0;
    int          Arg1;
    int          Arg2;
    int          Res;
};

typedef struct Abc_ZddMan_ Abc_ZddMan;
struct Abc_ZddMan_ 
{
    int          nVars;
    int          nObjs;
    int          nObjsAlloc;
    int          nPermSize;
    unsigned     nUniqueMask;
    unsigned     nCacheMask;
    int *        pUnique;
    int *        pNexts;
    Abc_ZddEnt * pCache;
    Abc_ZddObj * pObjs;
    int          nCacheLookups;
    int          nCacheMisses;
    word         nMemory;
    int *        pV2TI;
    int *        pV2TJ;
    int *        pT2V;
};

static inline int          Abc_ZddIthVar( int i )                                  { return i + 2;                                            }
static inline unsigned     Abc_ZddHash( int Arg0, int Arg1, int Arg2 )             { return 12582917 * Arg0 + 4256249 * Arg1 + 741457 * Arg2; }

static inline Abc_ZddObj * Abc_ZddNode( Abc_ZddMan * p, int i )                    { return p->pObjs + i;                                     }
static inline int          Abc_ZddObjId( Abc_ZddMan * p, Abc_ZddObj * pObj )       { return pObj - p->pObjs;                                  }
static inline int          Abc_ZddObjVar( Abc_ZddMan * p, int i )                  { return Abc_ZddNode(p, i)->Var;                           }

static inline void         Abc_ZddSetVarIJ( Abc_ZddMan * p, int i, int j, int v )  { assert( i < j ); p->pT2V[i * p->nPermSize + j] = v;      }
static inline int          Abc_ZddVarIJ( Abc_ZddMan * p, int i, int j )            { assert( i < j ); return p->pT2V[i * p->nPermSize + j];   }
static inline int          Abc_ZddVarsClash( Abc_ZddMan * p, int v0, int v1 )      { return p->pV2TI[v0] == p->pV2TI[v1] || p->pV2TJ[v0] == p->pV2TJ[v1] || p->pV2TI[v0] == p->pV2TJ[v1] || p->pV2TJ[v0] == p->pV2TI[v1];  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_ZddCacheLookup( Abc_ZddMan * p, int Arg0, int Arg1, int Arg2 )
{
    Abc_ZddEnt * pEnt = p->pCache + (Abc_ZddHash(Arg0, Arg1, Arg2) & p->nCacheMask);
    p->nCacheLookups++;
    return (pEnt->Arg0 == Arg0 && pEnt->Arg1 == Arg1 && pEnt->Arg2 == Arg2) ? pEnt->Res : -1;
}
static inline int Abc_ZddCacheInsert( Abc_ZddMan * p, int Arg0, int Arg1, int Arg2, int Res )
{
    Abc_ZddEnt * pEnt = p->pCache + (Abc_ZddHash(Arg0, Arg1, Arg2) & p->nCacheMask);
    pEnt->Arg0 = Arg0;  pEnt->Arg1 = Arg1;  pEnt->Arg2 = Arg2;  pEnt->Res = Res;
    p->nCacheMisses++;
    assert( Res >= 0 );
    return Res;
}
static inline int Abc_ZddUniqueLookup( Abc_ZddMan * p, int Var, int True, int False )
{
    int *q = p->pUnique + (Abc_ZddHash(Var, True, False) & p->nUniqueMask);
    for ( ; *q; q = p->pNexts + *q )
        if ( p->pObjs[*q].Var == (unsigned)Var && p->pObjs[*q].True == (unsigned)True && p->pObjs[*q].False == (unsigned)False )
            return *q;
    return 0;
}
static inline int Abc_ZddUniqueCreate( Abc_ZddMan * p, int Var, int True, int False )
{
    assert( Var >= 0 && Var < p->nVars );
    assert( Var < Abc_ZddObjVar(p, True) );
    assert( Var < Abc_ZddObjVar(p, False) );
    if ( True == 0 )
        return False;
    {
        int *q = p->pUnique + (Abc_ZddHash(Var, True, False) & p->nUniqueMask);
        for ( ; *q; q = p->pNexts + *q )
            if ( p->pObjs[*q].Var == (unsigned)Var && p->pObjs[*q].True == (unsigned)True && p->pObjs[*q].False == (unsigned)False )
                return *q;
        if ( p->nObjs == p->nObjsAlloc )
            printf( "Aborting because the number of nodes exceeded %d.\n", p->nObjsAlloc ), fflush(stdout);
        assert( p->nObjs < p->nObjsAlloc );     
        *q = p->nObjs++;
        p->pObjs[*q].Var = Var;
        p->pObjs[*q].True = True;
        p->pObjs[*q].False = False;
//        printf( "Added node %3d: Var = %3d.  True = %3d.  False = %3d\n", *q, Var, True, False );
        return *q;
    }
}
int Abc_ZddBuildSet( Abc_ZddMan * p, int * pSet, int Size )
{
    int i, Res = 1;
    Vec_IntSelectSort( pSet, Size );
    for ( i = Size - 1; i >= 0; i-- )
        Res = Abc_ZddUniqueCreate( p, pSet[i], Res, 0 );
    return Res;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_ZddMan * Abc_ZddManAlloc( int nVars, int nObjs )
{
    Abc_ZddMan * p; int i;
    p = ABC_CALLOC( Abc_ZddMan, 1 );
    p->nVars       = nVars;
    p->nObjsAlloc  = nObjs;
    p->nUniqueMask = (1 << Abc_Base2Log(nObjs)) - 1;
    p->nCacheMask  = (1 << Abc_Base2Log(nObjs)) - 1;
    p->pUnique     = ABC_CALLOC( int, p->nUniqueMask + 1 );
    p->pNexts      = ABC_CALLOC( int, p->nObjsAlloc );
    p->pCache      = ABC_CALLOC( Abc_ZddEnt, p->nCacheMask + 1 );
    p->pObjs       = ABC_CALLOC( Abc_ZddObj, p->nObjsAlloc );
    p->nObjs       = 2;
    memset( p->pObjs, 0xff, sizeof(Abc_ZddObj) * 2 );
    p->pObjs[0].Var = nVars;
    p->pObjs[1].Var = nVars;
    for ( i = 0; i < nVars; i++ )
        Abc_ZddUniqueCreate( p, i, 1, 0 );
    assert( p->nObjs == nVars + 2 );
    p->nMemory = sizeof(Abc_ZddMan)/4 + 
        p->nUniqueMask + 1 + p->nObjsAlloc + 
        (p->nCacheMask + 1) * sizeof(Abc_ZddEnt)/4 + 
        p->nObjsAlloc * sizeof(Abc_ZddObj)/4;
    return p;
}
void Abc_ZddManCreatePerms( Abc_ZddMan * p, int nPermSize )
{
    int i, j, v = 0;
    assert( 2 * p->nVars == nPermSize * (nPermSize - 1) );
    assert( p->nPermSize == 0 );
    p->nPermSize = nPermSize;
    p->pV2TI = ABC_FALLOC( int, p->nVars );
    p->pV2TJ = ABC_FALLOC( int, p->nVars );
    p->pT2V  = ABC_FALLOC( int, p->nPermSize * p->nPermSize );
    for ( i = 0; i < nPermSize; i++ )
        for ( j = i + 1; j < nPermSize; j++ )
        {
            p->pV2TI[v] = i;
            p->pV2TJ[v] = j;
            Abc_ZddSetVarIJ( p, i, j, v++ );
        }
    assert( v == p->nVars );
}
void Abc_ZddManFree( Abc_ZddMan * p )
{
    printf( "ZDD stats: Var = %d  Obj = %d  Alloc = %d  Hit = %d  Miss = %d  ", 
        p->nVars, p->nObjs, p->nObjsAlloc, p->nCacheLookups-p->nCacheMisses, p->nCacheMisses );
    printf( "Mem = %.2f MB\n", 4.0*(int)(p->nMemory/(1<<20)) );
    ABC_FREE( p->pT2V );
    ABC_FREE( p->pV2TI );
    ABC_FREE( p->pV2TJ );
    ABC_FREE( p->pUnique );
    ABC_FREE( p->pNexts );
    ABC_FREE( p->pCache );
    ABC_FREE( p->pObjs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ZddDiff( Abc_ZddMan * p, int a, int b )
{
    Abc_ZddObj * A, * B; 
    int r0, r1, r;
    if ( a == 0 ) return 0;
    if ( b == 0 ) return a;
    if ( a == b ) return 0;
    if ( (r = Abc_ZddCacheLookup(p, a, b, ABC_ZDD_OPER_DIFF)) >= 0 )
        return r;
    A = Abc_ZddNode( p, a );
    B = Abc_ZddNode( p, b );
    if ( A->Var < B->Var )
        r0 = Abc_ZddDiff( p, A->False, b ), 
        r = Abc_ZddUniqueCreate( p, A->Var, A->True, r0 );
    else if ( A->Var > B->Var )
        r = Abc_ZddDiff( p, a, B->False );
    else
        r0 = Abc_ZddDiff( p, A->False, B->False ), 
        r1 = Abc_ZddDiff( p, A->True, B->True ),
        r = Abc_ZddUniqueCreate( p, A->Var, A->True, r0 );
    return Abc_ZddCacheInsert( p, a, b, ABC_ZDD_OPER_DIFF, r );
}
int Abc_ZddUnion( Abc_ZddMan * p, int a, int b )
{
    Abc_ZddObj * A, * B; 
    int r0, r1, r;
    if ( a == 0 ) return b;
    if ( b == 0 ) return a;
    if ( a == b ) return a;
    if ( a > b )  return Abc_ZddUnion( p, b, a );
    if ( (r = Abc_ZddCacheLookup(p, a, b, ABC_ZDD_OPER_UNION)) >= 0 )
        return r;
    A = Abc_ZddNode( p, a );
    B = Abc_ZddNode( p, b );
    if ( A->Var < B->Var )
        r0 = Abc_ZddUnion( p, A->False, b ), 
        r1 = A->True;
    else if ( A->Var > B->Var )
        r0 = Abc_ZddUnion( p, a, B->False ), 
        r1 = B->True;
    else
        r0 = Abc_ZddUnion( p, A->False, B->False ), 
        r1 = Abc_ZddUnion( p, A->True, B->True );
    r = Abc_ZddUniqueCreate( p, Abc_MinInt(A->Var, B->Var), r1, r0 );
    return Abc_ZddCacheInsert( p, a, b, ABC_ZDD_OPER_UNION, r );
}
int Abc_ZddMinUnion( Abc_ZddMan * p, int a, int b )
{
    Abc_ZddObj * A, * B; 
    int r0, r1, r;
    if ( a == 0 ) return b;
    if ( b == 0 ) return a;
    if ( a == b ) return a;
    if ( a > b )  return Abc_ZddMinUnion( p, b, a );
    if ( (r = Abc_ZddCacheLookup(p, a, b, ABC_ZDD_OPER_MIN_UNION)) >= 0 )
        return r;
    A = Abc_ZddNode( p, a );
    B = Abc_ZddNode( p, b );
    if ( A->Var < B->Var )
        r0 = Abc_ZddMinUnion( p, A->False, b ), 
        r1 = A->True;
    else if ( A->Var > B->Var )
        r0 = Abc_ZddMinUnion( p, a, B->False ), 
        r1 = B->True;
    else
        r0 = Abc_ZddMinUnion( p, A->False, B->False ), 
        r1 = Abc_ZddMinUnion( p, A->True, B->True );
    r1 = Abc_ZddDiff( p, r1, r0 ); // assume args are minimal
    r = Abc_ZddUniqueCreate( p, Abc_MinInt(A->Var, B->Var), r1, r0 );
    return Abc_ZddCacheInsert( p, a, b, ABC_ZDD_OPER_MIN_UNION, r );
}
int Abc_ZddIntersect( Abc_ZddMan * p, int a, int b )
{
    Abc_ZddObj * A, * B; 
    int r0, r1, r;
    if ( a == 0 ) return 0;
    if ( b == 0 ) return 0;
    if ( a == b ) return a;
    if ( a > b )  return Abc_ZddIntersect( p, b, a );
    if ( (r = Abc_ZddCacheLookup(p, a, b, ABC_ZDD_OPER_INTER)) >= 0 )
        return r;
    A = Abc_ZddNode( p, a );
    B = Abc_ZddNode( p, b );
    if ( A->Var < B->Var )
        r0 = Abc_ZddIntersect( p, A->False, b ), 
        r1 = A->True;
    else if ( A->Var > B->Var )
        r0 = Abc_ZddIntersect( p, a, B->False ), 
        r1 = B->True;
    else
        r0 = Abc_ZddIntersect( p, A->False, B->False ), 
        r1 = Abc_ZddIntersect( p, A->True, B->True );
    r = Abc_ZddUniqueCreate( p, Abc_MinInt(A->Var, B->Var), r1, r0 );
    return Abc_ZddCacheInsert( p, a, b, ABC_ZDD_OPER_INTER, r );
}
int Abc_ZddCof0( Abc_ZddMan * p, int a, int Var )
{
    Abc_ZddObj * A; 
    int r0, r1, r;
    if ( a < 2 ) return a;
    A = Abc_ZddNode( p, a );
    if ( (int)A->Var > Var )
        return a;
    if ( (r = Abc_ZddCacheLookup(p, a, Var, ABC_ZDD_OPER_COF0)) >= 0 )
        return r;
    if ( (int)A->Var < Var )
        r0 = Abc_ZddCof0( p, A->False, Var ),
        r1 = Abc_ZddCof0( p, A->True, Var ),
        r  = Abc_ZddUniqueCreate( p, A->Var, r1, r0 );
    else
        r = Abc_ZddCof0( p, A->False, Var );
    return Abc_ZddCacheInsert( p, a, Var, ABC_ZDD_OPER_COF0, r );
}
int Abc_ZddCof1( Abc_ZddMan * p, int a, int Var )
{
    Abc_ZddObj * A; 
    int r0, r1, r;
    if ( a < 2 ) return a;
    A = Abc_ZddNode( p, a );
    if ( (int)A->Var > Var )
        return a;
    if ( (r = Abc_ZddCacheLookup(p, a, Var, ABC_ZDD_OPER_COF1)) >= 0 )
        return r;
    if ( (int)A->Var < Var )
        r0 = Abc_ZddCof1( p, A->False, Var ),
        r1 = Abc_ZddCof1( p, A->True, Var );
    else
        r0 = 0,
        r1 = Abc_ZddCof1( p, A->True, Var );
    r = Abc_ZddUniqueCreate( p, A->Var, r1, r0 );
    return Abc_ZddCacheInsert( p, a, Var, ABC_ZDD_OPER_COF1, r );
}
int Abc_ZddCountPaths( Abc_ZddMan * p, int a )
{
    Abc_ZddObj * A;  
    int r;
    if ( a < 2 ) return a;
    if ( (r = Abc_ZddCacheLookup(p, a, 0, ABC_ZDD_OPER_PATHS)) >= 0 )
        return r;
    A = Abc_ZddNode( p, a );
    r = Abc_ZddCountPaths( p, A->False ) + Abc_ZddCountPaths( p, A->True );
    return Abc_ZddCacheInsert( p, a, 0, ABC_ZDD_OPER_PATHS, r );
}
/*
int Abc_ZddCountNodes( Abc_ZddMan * p, int a )
{
    Abc_ZddObj * A;  
    int r;
    if ( a < 2 ) return 0;
    if ( (r = Abc_ZddCacheLookup(p, a, 0, ABC_ZDD_OPER_NODES)) >= 0 )
        return r;
    A = Abc_ZddNode( p, a );
    r = 1 + Abc_ZddCountNodes( p, A->False ) + Abc_ZddCountNodes( p, A->True );
    return Abc_ZddCacheInsert( p, a, 0, ABC_ZDD_OPER_NODES, r );
}
*/
int Abc_ZddCount_rec( Abc_ZddMan * p, int i )
{
    Abc_ZddObj * A;
    if ( i < 2 )
        return 0;
    A = Abc_ZddNode( p, i );
    if ( A->Mark )
        return 0;
    A->Mark = 1;
    return 1 + Abc_ZddCount_rec(p, A->False) + Abc_ZddCount_rec(p, A->True);
}
void Abc_ZddUnmark_rec( Abc_ZddMan * p, int i )
{
    Abc_ZddObj * A;
    if ( i < 2 )
        return;
    A = Abc_ZddNode( p, i );
    if ( !A->Mark )
        return;
    A->Mark = 0;
    Abc_ZddUnmark_rec( p, A->False );
    Abc_ZddUnmark_rec( p, A->True );
}
int Abc_ZddCountNodes( Abc_ZddMan * p, int i )
{
    int Count = Abc_ZddCount_rec( p, i );
    Abc_ZddUnmark_rec( p, i );
    return Count;
}
int Abc_ZddCountNodesArray( Abc_ZddMan * p, Vec_Int_t * vNodes )
{
    int i, Id, Count = 0;
    Vec_IntForEachEntry( vNodes, Id, i )
        Count += Abc_ZddCount_rec( p, Id );
    Vec_IntForEachEntry( vNodes, Id, i )
        Abc_ZddUnmark_rec( p, Id );
    return Count;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ZddThresh( Abc_ZddMan * p, int a, int b )
{
    Abc_ZddObj * A; 
    int r0, r1, r;
    if ( a < 2 )  return a;
    if ( b == 0 ) return 0;
    if ( (r = Abc_ZddCacheLookup(p, a, b, ABC_ZDD_OPER_THRESH)) >= 0 )
        return r;
    A  = Abc_ZddNode( p, a );
    r0 = Abc_ZddThresh( p, A->False, b ),
    r1 = Abc_ZddThresh( p, A->True, b-1 );
    r  = Abc_ZddUniqueCreate( p, A->Var, r1, r0 );
    return Abc_ZddCacheInsert( p, a, b, ABC_ZDD_OPER_THRESH, r );
}
int Abc_ZddDotProduct( Abc_ZddMan * p, int a, int b )
{
    Abc_ZddObj * A, * B; 
    int r0, r1, b2, t1, t2, r;
    if ( a == 0 ) return 0;
    if ( b == 0 ) return 0;
    if ( a == 1 ) return b;
    if ( b == 1 ) return a;
    if ( a > b )  return Abc_ZddDotProduct( p, b, a );
    if ( (r = Abc_ZddCacheLookup(p, a, b, ABC_ZDD_OPER_DOT_PROD)) >= 0 )
        return r;
    A = Abc_ZddNode( p, a );
    B = Abc_ZddNode( p, b );
    if ( A->Var < B->Var )
        r0 = Abc_ZddDotProduct( p, A->False, b ), 
        r1 = Abc_ZddDotProduct( p, A->True, b );
    else if ( A->Var > B->Var )
        r0 = Abc_ZddDotProduct( p, a, B->False ), 
        r1 = Abc_ZddDotProduct( p, a, B->True ); 
    else
        r0 = Abc_ZddDotProduct( p, A->False, B->False ),
        b2 = Abc_ZddUnion( p, B->False, B->True ), 
        t1 = Abc_ZddDotProduct( p, A->True, b2 ), 
        t2 = Abc_ZddDotProduct( p, A->False, B->True ), 
        r1 = Abc_ZddUnion( p, t1, t2 );
    r = Abc_ZddUniqueCreate( p, Abc_MinInt(A->Var, B->Var), r1, r0 );
    return Abc_ZddCacheInsert( p, a, b, ABC_ZDD_OPER_DOT_PROD, r );
}
int Abc_ZddDotMinProduct6( Abc_ZddMan * p, int a, int b )
{
    Abc_ZddObj * A, * B; 
    int r0, r1, b2, t1, t2, r;
    if ( a == 0 ) return 0;
    if ( b == 0 ) return 0;
    if ( a == 1 ) return b;
    if ( b == 1 ) return a;
    if ( a > b )  return Abc_ZddDotMinProduct6( p, b, a );
    if ( (r = Abc_ZddCacheLookup(p, a, b, ABC_ZDD_OPER_DOT_PROD_6)) >= 0 )
        return r;
    A = Abc_ZddNode( p, a );
    B = Abc_ZddNode( p, b );
    if ( A->Var < B->Var )
        r0 = Abc_ZddDotMinProduct6( p, A->False, b ), 
        r1 = Abc_ZddDotMinProduct6( p, A->True, b );
    else if ( A->Var > B->Var )
        r0 = Abc_ZddDotMinProduct6( p, a, B->False ), 
        r1 = Abc_ZddDotMinProduct6( p, a, B->True ); 
    else
        r0 = Abc_ZddDotMinProduct6( p, A->False, B->False ),
        b2 = Abc_ZddMinUnion( p, B->False, B->True ), 
        t1 = Abc_ZddDotMinProduct6( p, A->True, b2 ), 
        t2 = Abc_ZddDotMinProduct6( p, A->False, B->True ), 
        r1 = Abc_ZddMinUnion( p, t1, t2 );
    r1 = Abc_ZddThresh( p, r1, 5 ),
    r1 = Abc_ZddDiff( p, r1, r0 ); 
    r = Abc_ZddUniqueCreate( p, Abc_MinInt(A->Var, B->Var), r1, r0 );
    return Abc_ZddCacheInsert( p, a, b, ABC_ZDD_OPER_DOT_PROD_6, r );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ZddPerm( Abc_ZddMan * p, int a, int Var )
{
    Abc_ZddObj * A; 
    int r0, r1, r;
    assert( Var < p->nVars );
    if ( a == 0 )  return 0;
    if ( a == 1 )  return Abc_ZddIthVar(Var);
    if ( (r = Abc_ZddCacheLookup(p, a, Var, ABC_ZDD_OPER_PERM)) >= 0 )
        return r;
    A = Abc_ZddNode( p, a );
    if ( p->pV2TI[A->Var] > p->pV2TI[Var] ) // Ai > Bi
        r = Abc_ZddUniqueCreate( p, Var, a, 0 );
    else if ( (int)A->Var == Var ) // Ai == Bi && Aj == Bj
        r0 = Abc_ZddPerm( p, A->False, Var ), 
        r  = Abc_ZddUnion( p, r0, A->True );
    else 
    {
        int VarPerm, VarTop;
        int Ai = p->pV2TI[A->Var];
        int Aj = p->pV2TJ[A->Var];
        int Bi = p->pV2TI[Var];
        int Bj = p->pV2TJ[Var];
        assert( Ai < Aj && Bi < Bj && Ai <= Bi );
        if ( Aj == Bi )
            VarPerm = Var,
            VarTop  = Abc_ZddVarIJ(p, Ai, Bj);
        else if ( Aj == Bj )
            VarPerm = Var,
            VarTop  = Abc_ZddVarIJ(p, Ai, Bi);
        else if ( Ai == Bi )
            VarPerm = Abc_ZddVarIJ(p, Abc_MinInt(Aj, Bj), Abc_MaxInt(Aj, Bj)),
            VarTop  = A->Var;
        else // no clash
            VarPerm = Var, 
            VarTop  = A->Var;
        assert( p->pV2TI[VarPerm] > p->pV2TI[VarTop] );
        r0 = Abc_ZddPerm( p, A->False, Var ); 
        r1 = Abc_ZddPerm( p, A->True, VarPerm );
        assert( Abc_ZddObjVar(p, r1) > VarTop );
        if ( Abc_ZddObjVar(p, r0) > VarTop )
            r = Abc_ZddUniqueCreate( p, VarTop, r1, r0 );
        else
            r1 = Abc_ZddUniqueCreate( p, VarTop, r1, 0 ),
            r = Abc_ZddUnion( p, r0, r1 );
    }
    return Abc_ZddCacheInsert( p, a, Var, ABC_ZDD_OPER_PERM, r );
}
int Abc_ZddPermProduct( Abc_ZddMan * p, int a, int b )
{
    Abc_ZddObj * B; 
    int r0, r1, r;
    if ( a == 0 ) return 0;
    if ( a == 1 ) return b;
    if ( b == 0 ) return 0;
    if ( b == 1 ) return a;
    if ( (r = Abc_ZddCacheLookup(p, a, b, ABC_ZDD_OPER_PERM_PROD)) >= 0 )
        return r;
    B  = Abc_ZddNode( p, b );
    r0 = Abc_ZddPermProduct( p, a, B->False ); 
    r1 = Abc_ZddPermProduct( p, a, B->True ); 
    r1 = Abc_ZddPerm( p, r1, B->Var );
    r  = Abc_ZddUnion( p, r0, r1 );
    return Abc_ZddCacheInsert( p, a, b, ABC_ZDD_OPER_PERM_PROD, r );
}

/**Function*************************************************************

  Synopsis    [Printing permutations and transpositions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ZddPermPrint( int * pPerm, int Size )
{
    int i;
    printf( "{" );
    for ( i = 0; i < Size; i++ )
        printf( " %2d", pPerm[i] );
    printf( " }\n" );
}
void Abc_ZddCombPrint( int * pComb, int nTrans )
{
    int i;
    if ( nTrans == 0 )
        printf( "Empty set" );
    for ( i = 0; i < nTrans; i++ )
        printf( "(%d %d)", pComb[i] >> 16, pComb[i] & 0xffff );
    printf( "\n" );
}
int Abc_ZddPerm2Comb( int * pPerm, int Size, int * pComb )
{
    int i, j, nTrans = 0;
    for ( i = 0; i < Size; i++ )
        if ( i != pPerm[i] )
        {
            for ( j = i+1; j < Size; j++ )
                if ( i == pPerm[j] )
                    break;
            pComb[nTrans++] = (i << 16) | j;
            ABC_SWAP( int, pPerm[i], pPerm[j] );
            assert( i == pPerm[i] );
        }
    return nTrans;
}
void Abc_ZddComb2Perm( int * pComb, int nTrans, int * pPerm, int Size )
{
    int v;
    for ( v = 0; v < Size; v++ )
        pPerm[v] = v;
    for ( v = nTrans-1; v >= 0; v-- )
        ABC_SWAP( int, pPerm[pComb[v] >> 16], pPerm[pComb[v] & 0xffff] );
}
void Abc_ZddPermCombTest()
{
    int Size = 10;
    int pPerm[10] = { 6, 5, 7, 0, 3, 2, 1, 8, 9, 4 };
    int pComb[10], nTrans;
    Abc_ZddPermPrint( pPerm, Size );
    nTrans = Abc_ZddPerm2Comb( pPerm, Size, pComb );
    Abc_ZddCombPrint( pComb, nTrans );
    Abc_ZddComb2Perm( pComb, nTrans, pPerm, Size );
    Abc_ZddPermPrint( pPerm, Size );
    Size = 0;
}

/**Function*************************************************************

  Synopsis    [Printing ZDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ZddPrint_rec( Abc_ZddMan * p, int a, int * pPath, int Size )
{
    Abc_ZddObj * A;  
    if ( a == 0 ) return;
//    if ( a == 1 ) { Abc_ZddPermPrint( pPath, Size ); return; }
    if ( a == 1 )
    { 
        int pPerm[24], pComb[24], i;
        assert( p->nPermSize <= 24 );
        for ( i = 0; i < Size; i++ )
            pComb[i] = (p->pV2TI[pPath[i]] << 16) | p->pV2TJ[pPath[i]];
        Abc_ZddCombPrint( pComb, Size ); 
        Abc_ZddComb2Perm( pComb, Size, pPerm, p->nPermSize );
        Abc_ZddPermPrint( pPerm, p->nPermSize );
        return; 
    }
    A = Abc_ZddNode( p, a );
    Abc_ZddPrint_rec( p, A->False, pPath, Size );
    pPath[Size] = A->Var;
    Abc_ZddPrint_rec( p, A->True, pPath, Size + 1 );
}
void Abc_ZddPrint( Abc_ZddMan * p, int a )
{
    int * pPath = ABC_CALLOC( int, p->nVars );
    Abc_ZddPrint_rec( p, a, pPath, 0 );
    ABC_FREE( pPath );
}
void Abc_ZddPrintTest( Abc_ZddMan * p )
{
//    int nSets = 2;
//    int Size = 2;
//    int pSets[2][2] = { {5, 0}, {3, 11} };
    int nSets = 3;
    int Size = 5;
    int pSets[3][5] = { {5, 0, 2, 10, 7}, {3, 11, 10, 7, 2}, {0, 2, 5, 10, 7} };
    int i, Set, Union = 0;
    for ( i = 0; i < nSets; i++ )
    {
        Abc_ZddPermPrint( pSets[i], Size );
        Set = Abc_ZddBuildSet( p, pSets[i], Size );
        Union = Abc_ZddUnion( p, Union, Set );
    }
    printf( "Resulting set:\n" );
    Abc_ZddPrint( p, Union );
    printf( "\n" );
    printf( "Nodes = %d.   Path = %d.\n", Abc_ZddCountNodes(p, Union), Abc_ZddCountPaths(p, Union) );
    Size = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ZddGiaTest( Gia_Man_t * pGia )
{
    Abc_ZddMan * p;
    Gia_Obj_t * pObj;
    Vec_Int_t * vNodes;
    int i, r, Paths = 0;
    p = Abc_ZddManAlloc( Gia_ManObjNum(pGia), 1 << 24 ); // 576 MB  (36 B/node)
    Gia_ManFillValue( pGia );
    Gia_ManForEachCi( pGia, pObj, i )
        pObj->Value = Abc_ZddIthVar( Gia_ObjId(pGia, pObj) );
    vNodes = Vec_IntAlloc( Gia_ManAndNum(pGia) );
    Gia_ManForEachAnd( pGia, pObj, i )
    {
        r = Abc_ZddDotMinProduct6( p, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value );
        r = Abc_ZddUnion( p, r, Abc_ZddIthVar(i) );
        pObj->Value = r;
        Vec_IntPush( vNodes, r );
        // print
//        printf( "Node %d:\n", i );
//        Abc_ZddPrint( p, r );
//        printf( "Node %d   ZddNodes = %d\n", i, Abc_ZddCountNodes(p, r) );
    }
    Gia_ManForEachAnd( pGia, pObj, i )
        Paths += Abc_ZddCountPaths(p, pObj->Value);
    printf( "Paths = %d.  Shared nodes = %d.\n", Paths, Abc_ZddCountNodesArray(p, vNodes) );
    Vec_IntFree( vNodes );
    Abc_ZddManFree( p );
}
/*
    abc 01> &r pj1.aig; &ps; &test
    pj1      : i/o =   1769/   1063  and =   16285  lev =  156 (12.91)  mem = 0.23 MB
    Paths = 839934.  Shared nodes = 770999.
    ZDD stats: Var = 19118  Obj = 11578174  All = 16777216  Hits = 25617277  Miss = 40231476  Mem = 576.00 MB
*/

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ZddPermTestInt( Abc_ZddMan * p )
{
    int nPerms = 3;
    int Size = 5;
    int pPerms[3][5] = { {1, 0, 2, 4, 3}, {1, 2, 4, 0, 3}, {0, 3, 2, 1, 4} };
    int pComb[5], nTrans;
    int i, k, Set, Union = 0, iPivot;
    for ( i = 0; i < nPerms; i++ )
        Abc_ZddPermPrint( pPerms[i], Size );
    for ( i = 0; i < nPerms; i++ )
    {
        printf( "Perm %d:\n", i );
        Abc_ZddPermPrint( pPerms[i], Size );
        nTrans = Abc_ZddPerm2Comb( pPerms[i], Size, pComb );
        Abc_ZddCombPrint( pComb, nTrans );
        for ( k = 0; k < nTrans; k++ )
            pComb[k] = Abc_ZddVarIJ( p, pComb[k] >> 16, pComb[k] & 0xFFFF );
        Abc_ZddPermPrint( pComb, nTrans );
        // add to ZDD
        Set = Abc_ZddBuildSet( p, pComb, nTrans );
        Union = Abc_ZddUnion( p, Union, Set );
    }
    printf( "\nResulting set of permutations:\n" );
    Abc_ZddPrint( p, Union );
    printf( "Nodes = %d.   Path = %d.\n", Abc_ZddCountNodes(p, Union), Abc_ZddCountPaths(p, Union) );

    iPivot = Abc_ZddVarIJ( p, 3, 4 );
    Union = Abc_ZddPerm( p, Union, iPivot );

    printf( "\nResulting set of permutations:\n" );
    Abc_ZddPrint( p, Union );
    printf( "Nodes = %d.   Path = %d.\n", Abc_ZddCountNodes(p, Union), Abc_ZddCountPaths(p, Union) );
    printf( "\n" );
}

void Abc_ZddPermTest()
{
    Abc_ZddMan * p;
    p = Abc_ZddManAlloc( 10, 1 << 20 );
    Abc_ZddManCreatePerms( p, 5 );
    Abc_ZddPermTestInt( p );
    Abc_ZddManFree( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_EnumerateCubeStatesZdd()
{
    int pXYZ[3][9][2] = {
        { {3, 5}, {3,17}, {3,15}, {1, 6}, {1,16}, {1,14}, {2, 4}, {2,18}, {2,13} },
        { {2,14}, {2,24}, {2,12}, {3,13}, {3,23}, {3,10}, {1,15}, {1,22}, {1,11} },
        { {1,10}, {1, 7}, {1, 4}, {3,12}, {3, 9}, {3, 6}, {2,11}, {2, 8}, {2, 5} }  };
#ifdef WIN32
    int LogObj = 24;
#else
    int LogObj = 27;
#endif
    Abc_ZddMan * p;
    int i, k, pComb[9], pPerm[24], nSize;
    int ZddTurn1, ZddTurn2, ZddTurn3, ZddTurns, ZddAll;
    abctime clk = Abc_Clock();
    printf( "Enumerating states of 2x2x2 cube.\n" );
    p = Abc_ZddManAlloc( 24 * 23 / 2, 1 << LogObj ); // finished with 2^27 (4 GB)
    Abc_ZddManCreatePerms( p, 24 );
    // init state
    printf( "Iter %2d -> %8d  Nodes = %7d  Used = %10d  ", 0, 1, 0, 2 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // first 9 states
    ZddTurns = 1;
    for ( i = 0; i < 3; i++ )
    {
        for ( k = 0; k < 24; k++ )
            pPerm[k] = k;
        for ( k = 0; k < 9; k++ )
            ABC_SWAP( int, pPerm[pXYZ[i][k][0]-1], pPerm[pXYZ[i][k][1]-1] );
        nSize = Abc_ZddPerm2Comb( pPerm, 24, pComb );
        assert( nSize == 9 );
        for ( k = 0; k < 9; k++ )
            pComb[k] = Abc_ZddVarIJ( p, pComb[k] >> 16, pComb[k] & 0xffff );
        // add first turn
        ZddTurn1  = Abc_ZddBuildSet( p, pComb, 9 );
        ZddTurns = Abc_ZddUnion( p, ZddTurns, ZddTurn1 );
        //Abc_ZddPrint( p, ZddTurn1 );
        // add second turn
        ZddTurn2 = Abc_ZddPermProduct( p, ZddTurn1, ZddTurn1 );
        ZddTurns = Abc_ZddUnion( p, ZddTurns, ZddTurn2 );
        //Abc_ZddPrint( p, ZddTurn2 );
        // add third turn
        ZddTurn3 = Abc_ZddPermProduct( p, ZddTurn2, ZddTurn1 );
        ZddTurns = Abc_ZddUnion( p, ZddTurns, ZddTurn3 );
        //Abc_ZddPrint( p, ZddTurn3 );
        //printf( "\n" );
    }
    //Abc_ZddPrint( p, ZddTurns );
    printf( "Iter %2d -> %8d  Nodes = %7d  Used = %10d  ", 1, Abc_ZddCountPaths(p, ZddTurns), Abc_ZddCountNodes(p, ZddTurns), p->nObjs );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // other states
    ZddAll = ZddTurns;
    for ( i = 2; i <= 100; i++ )
    {
        int ZddAllPrev = ZddAll;
        ZddAll = Abc_ZddPermProduct( p, ZddAll, ZddTurns );
        printf( "Iter %2d -> %8d  Nodes = %7d  Used = %10d  ", i, Abc_ZddCountPaths(p, ZddAll), Abc_ZddCountNodes(p, ZddAll), p->nObjs );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        if ( ZddAllPrev == ZddAll )
            break;
    }
    Abc_ZddManFree( p );
}

/*
Enumerating states of 2x2x2 cube.
Iter  0 ->        1  Nodes =       0  Used =          2  Time =     0.00 sec
Iter  1 ->       10  Nodes =      63  Used =        577  Time =     0.00 sec
Iter  2 ->       64  Nodes =     443  Used =       4349  Time =     0.03 sec
Iter  3 ->      385  Nodes =    2018  Used =      26654  Time =     0.14 sec
Iter  4 ->     2232  Nodes =    7451  Used =     119442  Time =     0.45 sec
Iter  5 ->    12224  Nodes =   25178  Used =     490038  Time =     1.10 sec
Iter  6 ->    62360  Nodes =   83955  Used =    1919750  Time =     1.79 sec
Iter  7 ->   289896  Nodes =  290863  Used =    7182932  Time =     3.15 sec
Iter  8 ->  1159968  Nodes =  614845  Used =   25301123  Time =     8.03 sec
Iter  9 ->  3047716  Nodes =  585664  Used =   66228369  Time =    20.22 sec
Iter 10 ->  3671516  Nodes =   19430  Used =  102292452  Time =    33.41 sec
Iter 11 ->  3674160  Nodes =     511  Used =  103545878  Time =    33.92 sec
Iter 12 ->  3674160  Nodes =     511  Used =  103566266  Time =    33.93 sec
ZDD stats: Var = 276  Obj = 103566266  Alloc = 134217728  Hit = 63996630  Miss = 141768893  Mem = 4608.00 MB
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

