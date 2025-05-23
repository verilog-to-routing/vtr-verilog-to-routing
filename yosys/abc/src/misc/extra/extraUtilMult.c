/**CFile****************************************************************

  FileName    [extraUtilMult.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Simple BDD package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 23, 2018.]

  Revision    [$Id: extraUtilMult.c,v 1.0 2018/05/23 00:00:00 alanmi Exp $]

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

typedef struct Abc_BddMan_ Abc_BddMan;
struct Abc_BddMan_ 
{
    int                nVars;         // the number of variables
    int                nObjs;         // the number of nodes used
    int                nObjsAlloc;    // the number of nodes allocated
    int *              pUnique;       // unique table for nodes
    int *              pNexts;        // next pointer for nodes
    int *              pCache;        // array of triples <arg0, arg1, AND(arg0, arg1)>
    int *              pObjs;         // array of pairs <cof0, cof1> for each node
    unsigned char *    pVars;         // array of variables for each node
    unsigned char *    pMark;         // array of marks for each BDD node
    unsigned           nUniqueMask;   // selection mask for unique table
    unsigned           nCacheMask;    // selection mask for computed table
    int                nCacheLookups; // the number of computed table lookups
    int                nCacheMisses;  // the number of computed table misses
    word               nMemory;       // total amount of memory used (in bytes)
};

static inline int      Abc_BddIthVar( int i )                        { return Abc_Var2Lit(i + 1, 0);                            }
static inline unsigned Abc_BddHash( int Arg0, int Arg1, int Arg2 )   { return 12582917 * Arg0 + 4256249 * Arg1 + 741457 * Arg2; }

static inline int      Abc_BddVar( Abc_BddMan * p, int i )           { return (int)p->pVars[Abc_Lit2Var(i)];                    }
static inline int      Abc_BddThen( Abc_BddMan * p, int i )          { return Abc_LitNotCond(p->pObjs[Abc_LitRegular(i)], Abc_LitIsCompl(i));    }
static inline int      Abc_BddElse( Abc_BddMan * p, int i )          { return Abc_LitNotCond(p->pObjs[Abc_LitRegular(i)+1], Abc_LitIsCompl(i));  }

static inline int      Abc_BddMark( Abc_BddMan * p, int i )          { return (int)p->pMark[Abc_Lit2Var(i)];                    }
static inline void     Abc_BddSetMark( Abc_BddMan * p, int i, int m ){ p->pMark[Abc_Lit2Var(i)] = m;                            }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creating new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_BddUniqueCreateInt( Abc_BddMan * p, int Var, int Then, int Else )
{
    int *q = p->pUnique + (Abc_BddHash(Var, Then, Else) & p->nUniqueMask);
    for ( ; *q; q = p->pNexts + *q )
        if ( (int)p->pVars[*q] == Var && p->pObjs[*q+*q] == Then && p->pObjs[*q+*q+1] == Else )
            return Abc_Var2Lit(*q, 0);
    if ( p->nObjs == p->nObjsAlloc )
        printf( "Aborting because the number of nodes exceeded %d.\n", p->nObjsAlloc ), fflush(stdout);
    assert( p->nObjs < p->nObjsAlloc );     
    *q = p->nObjs++;
    p->pVars[*q] = Var;
    p->pObjs[*q+*q] = Then;
    p->pObjs[*q+*q+1] = Else;
//    printf( "Added node %3d: Var = %3d.  Then = %3d.  Else = %3d\n", *q, Var, Then, Else );
    assert( !Abc_LitIsCompl(Else) );
    return Abc_Var2Lit(*q, 0);
}
static inline int Abc_BddUniqueCreate( Abc_BddMan * p, int Var, int Then, int Else )
{
    assert( Var >= 0 && Var < p->nVars );
    assert( Var < Abc_BddVar(p, Then)  );
    assert( Var < Abc_BddVar(p, Else) );
    if ( Then == Else )
        return Else;
    if ( !Abc_LitIsCompl(Else) )
        return Abc_BddUniqueCreateInt( p, Var, Then, Else );
    return Abc_LitNot( Abc_BddUniqueCreateInt(p, Var, Abc_LitNot(Then), Abc_LitNot(Else)) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_BddCacheLookup( Abc_BddMan * p, int Arg1, int Arg2 )
{
    int * pEnt = p->pCache + 3*(Abc_BddHash(0, Arg1, Arg2) & p->nCacheMask);
    p->nCacheLookups++;
    return (pEnt[0] == Arg1 && pEnt[1] == Arg2) ? pEnt[2] : -1;
}
static inline int Abc_BddCacheInsert( Abc_BddMan * p, int Arg1, int Arg2, int Res )
{
    int * pEnt = p->pCache + 3*(Abc_BddHash(0, Arg1, Arg2) & p->nCacheMask);
    pEnt[0] = Arg1;  pEnt[1] = Arg2;  pEnt[2] = Res;
    p->nCacheMisses++;
    assert( Res >= 0 );
    return Res;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_BddMan * Abc_BddManAlloc( int nVars, int nObjs )
{
    Abc_BddMan * p; int i;
    p = ABC_CALLOC( Abc_BddMan, 1 );
    p->nVars       = nVars;
    p->nObjsAlloc  = nObjs;
    p->nUniqueMask = (1 << Abc_Base2Log(nObjs)) - 1;
    p->nCacheMask  = (1 << Abc_Base2Log(nObjs)) - 1;
    p->pUnique     = ABC_CALLOC( int, p->nUniqueMask + 1 );
    p->pNexts      = ABC_CALLOC( int, p->nObjsAlloc );
    p->pCache      = ABC_CALLOC( int, 3*(p->nCacheMask + 1) );
    p->pObjs       = ABC_CALLOC( int, 2*p->nObjsAlloc );
    p->pMark       = ABC_CALLOC( unsigned char, p->nObjsAlloc );
    p->pVars       = ABC_CALLOC( unsigned char, p->nObjsAlloc );
    p->pVars[0]    = 0xff;
    p->nObjs       = 1;
    for ( i = 0; i < nVars; i++ )
        Abc_BddUniqueCreate( p, i, 1, 0 );
    assert( p->nObjs == nVars + 1 );
    p->nMemory = sizeof(Abc_BddMan)/4 + 
        p->nUniqueMask + 1 + p->nObjsAlloc + 
        (p->nCacheMask + 1) * 3 * sizeof(int)/4 + 
        p->nObjsAlloc * 2 * sizeof(int)/4;
    return p;
}
void Abc_BddManFree( Abc_BddMan * p )
{
    printf( "BDD stats: Var = %d  Obj = %d  Alloc = %d  Hit = %d  Miss = %d  ", 
        p->nVars, p->nObjs, p->nObjsAlloc, p->nCacheLookups-p->nCacheMisses, p->nCacheMisses );
    printf( "Mem = %.2f MB\n", 4.0*(int)(p->nMemory/(1<<20)) );
    ABC_FREE( p->pUnique );
    ABC_FREE( p->pNexts );
    ABC_FREE( p->pCache );
    ABC_FREE( p->pObjs );
    ABC_FREE( p->pVars );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Boolean AND.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_BddAnd( Abc_BddMan * p, int a, int b )
{
    int r0, r1, r;
    if ( a == 0 ) return 0;
    if ( b == 0 ) return 0;
    if ( a == 1 ) return b;
    if ( b == 1 ) return a;
    if ( a == b ) return a;
    if ( a > b )  return Abc_BddAnd( p, b, a );
    if ( (r = Abc_BddCacheLookup(p, a, b)) >= 0 )
        return r;
    if ( Abc_BddVar(p, a) < Abc_BddVar(p, b) )
        r0 = Abc_BddAnd( p, Abc_BddElse(p, a), b ), 
        r1 = Abc_BddAnd( p, Abc_BddThen(p, a), b );
    else if ( Abc_BddVar(p, a) > Abc_BddVar(p, b) )
        r0 = Abc_BddAnd( p, a, Abc_BddElse(p, b) ), 
        r1 = Abc_BddAnd( p, a, Abc_BddThen(p, b) );
    else // if ( Abc_BddVar(p, a) == Abc_BddVar(p, b) )
        r0 = Abc_BddAnd( p, Abc_BddElse(p, a), Abc_BddElse(p, b) ), 
        r1 = Abc_BddAnd( p, Abc_BddThen(p, a), Abc_BddThen(p, b) );
    r = Abc_BddUniqueCreate( p, Abc_MinInt(Abc_BddVar(p, a), Abc_BddVar(p, b)), r1, r0 );
    return Abc_BddCacheInsert( p, a, b, r );
}
int Abc_BddOr( Abc_BddMan * p, int a, int b )
{
    return Abc_LitNot( Abc_BddAnd(p, Abc_LitNot(a), Abc_LitNot(b)) );
}

/**Function*************************************************************

  Synopsis    [Counting nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_BddCount_rec( Abc_BddMan * p, int i )
{
    if ( i < 2 )
        return 0;
    if ( Abc_BddMark(p, i) )
        return 0;
    Abc_BddSetMark( p, i, 1 );
    return 1 + Abc_BddCount_rec(p, Abc_BddElse(p, i)) + Abc_BddCount_rec(p, Abc_BddThen(p, i));
}
void Abc_BddUnmark_rec( Abc_BddMan * p, int i )
{
    if ( i < 2 )
        return;
    if ( !Abc_BddMark(p, i) )
        return;
    Abc_BddSetMark( p, i, 0 );
    Abc_BddUnmark_rec( p, Abc_BddElse(p, i) );
    Abc_BddUnmark_rec( p, Abc_BddThen(p, i) );
}
int Abc_BddCountNodes( Abc_BddMan * p, int i )
{
    int Count = Abc_BddCount_rec( p, i );
    Abc_BddUnmark_rec( p, i );
    return Count;
}
int Abc_BddCountNodesArray( Abc_BddMan * p, Vec_Int_t * vNodes )
{
    int i, a, Count = 0;
    Vec_IntForEachEntry( vNodes, a, i )
        Count += Abc_BddCount_rec( p, a );
    Vec_IntForEachEntry( vNodes, a, i )
        Abc_BddUnmark_rec( p, a );
    return Count;
}
int Abc_BddCountNodesArray2( Abc_BddMan * p, Vec_Int_t * vNodes )
{
    int i, a, Count = 0;
    Vec_IntForEachEntry( vNodes, a, i )
    {
        Count += Abc_BddCount_rec( p, a );
        Abc_BddUnmark_rec( p, a );
    }
    return Count;
}



/**Function*************************************************************

  Synopsis    [Printing BDD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_BddPrint_rec( Abc_BddMan * p, int a, int * pPath )
{
    if ( a == 0 ) 
        return;
    if ( a == 1 )
    { 
        int i;
        for ( i = 0; i < p->nVars; i++ )
            if ( pPath[i] == 0 || pPath[i] == 1 )
                printf( "%c%d", pPath[i] ? '+' : '-', i );
        printf( " " );
        return; 
    }
    pPath[Abc_BddVar(p, a)] =  0;
    Abc_BddPrint_rec( p, Abc_BddElse(p, a), pPath );
    pPath[Abc_BddVar(p, a)] =  1;
    Abc_BddPrint_rec( p, Abc_BddThen(p, a), pPath );
    pPath[Abc_BddVar(p, a)] = -1;
}
void Abc_BddPrint( Abc_BddMan * p, int a )
{
    int * pPath = ABC_FALLOC( int, p->nVars );
    printf( "BDD %d = ", a );
    Abc_BddPrint_rec( p, a, pPath );
    ABC_FREE( pPath );
    printf( "\n" );
}
void Abc_BddPrintTest( Abc_BddMan * p )
{
    int bVarA = Abc_BddIthVar(0);
    int bVarB = Abc_BddIthVar(1);
    int bVarC = Abc_BddIthVar(2);
    int bVarD = Abc_BddIthVar(3);
    int bAndAB = Abc_BddAnd( p, bVarA, bVarB );
    int bAndCD = Abc_BddAnd( p, bVarC, bVarD );
    int bFunc  = Abc_BddOr( p, bAndAB, bAndCD );
    Abc_BddPrint( p, bFunc );
    printf( "Nodes = %d\n", Abc_BddCountNodes(p, bFunc) );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_BddGiaTest2( Gia_Man_t * pGia, int fVerbose )
{
    Abc_BddMan * p = Abc_BddManAlloc( 10, 100 );
    Abc_BddPrintTest( p );
    Abc_BddManFree( p );
}

void Abc_BddGiaTest( Gia_Man_t * pGia, int fVerbose )
{
    Abc_BddMan * p;
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj; int i;
    p = Abc_BddManAlloc( Gia_ManCiNum(pGia), 1 << 20 ); // 30 B/node
    Gia_ManFillValue( pGia );
    Gia_ManConst0(pGia)->Value = 0;
    Gia_ManForEachCi( pGia, pObj, i )
        pObj->Value = Abc_BddIthVar( i );
    vNodes = Vec_IntAlloc( Gia_ManAndNum(pGia) );
    Gia_ManForEachAnd( pGia, pObj, i )
    {
        int Cof0 = Abc_LitNotCond(Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj));
        int Cof1 = Abc_LitNotCond(Gia_ObjFanin1(pObj)->Value, Gia_ObjFaninC1(pObj));
        pObj->Value = Abc_BddAnd( p, Cof0, Cof1 );
    }
    Gia_ManForEachCo( pGia, pObj, i )
        pObj->Value = Abc_LitNotCond(Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj));
    Gia_ManForEachCo( pGia, pObj, i )
    {
        if ( fVerbose )
            Abc_BddPrint( p, pObj->Value );
        Vec_IntPush( vNodes, pObj->Value );
    }
    printf( "Shared nodes = %d.\n", Abc_BddCountNodesArray2(p, vNodes) );
    Vec_IntFree( vNodes );
    Abc_BddManFree( p );
}

/*
    abc 04> c.aig; &get; &test
    Shared nodes = 80.
    BDD stats: Var = 23  Obj = 206  Alloc = 1048576  Hit = 59  Miss = 216  Mem = 28.00 MB

    abc 05> c.aig; clp -r; ps
    c                             : i/o =   23/    2  lat =    0  nd =     2  edge =     46  bdd  =    80  lev = 1
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

