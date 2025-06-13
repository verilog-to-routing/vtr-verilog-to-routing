/**CFile****************************************************************

  FileName    [sscClass.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT sweeping under constraints.]

  Synopsis    [Equivalence classes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: sscClass.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sscInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes hash key of the simuation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ssc_GiaSimHashKey( Gia_Man_t * p, int iObj, int nTableSize )
{
    static int s_Primes[16] = { 
        1291, 1699, 1999, 2357, 2953, 3313, 3907, 4177, 
        4831, 5147, 5647, 6343, 6899, 7103, 7873, 8147 };
    word * pSim = Gia_ObjSim( p, iObj );
    unsigned uHash = 0;
    int i, nWords = Gia_ObjSimWords(p);
    if ( pSim[0] & 1 )
        for ( i = 0; i < nWords; i++ )
            uHash ^= ~pSim[i] * s_Primes[i & 0xf];
    else
        for ( i = 0; i < nWords; i++ )
            uHash ^= pSim[i] * s_Primes[i & 0xf];
    return (int)(uHash % nTableSize);
}

/**Function*************************************************************

  Synopsis    [Returns 1 if sim patterns are equal up to the complement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ssc_GiaSimIsConst0( Gia_Man_t * p, int iObj0 )
{
    int w, nWords = Gia_ObjSimWords(p);
    word * pSim = Gia_ObjSim( p, iObj0 );
    if ( pSim[0] & 1 )
    {
        for ( w = 0; w < nWords; w++ )
            if ( ~pSim[w] )
                return 0;
    }
    else
    {
        for ( w = 0; w < nWords; w++ )
            if ( pSim[w] )
                return 0;
    }
    return 1;
}
static inline int Ssc_GiaSimAreEqual( Gia_Man_t * p, int iObj0, int iObj1 )
{
    int w, nWords = Gia_ObjSimWords(p);
    word * pSim0 = Gia_ObjSim( p, iObj0 );
    word * pSim1 = Gia_ObjSim( p, iObj1 );
    if ( (pSim0[0] & 1) != (pSim1[0] & 1) )
    {
        for ( w = 0; w < nWords; w++ )
            if ( pSim0[w] != ~pSim1[w] )
                return 0;
    }
    else
    {
        for ( w = 0; w < nWords; w++ )
            if ( pSim0[w] != pSim1[w] )
                return 0;
    }
    return 1;
}
static inline int Ssc_GiaSimAreEqualBit( Gia_Man_t * p, int iObj0, int iObj1 )
{
    Gia_Obj_t * pObj0 = Gia_ManObj( p, iObj0 );
    Gia_Obj_t * pObj1 = Gia_ManObj( p, iObj1 );
    return (pObj0->fPhase ^ pObj0->fMark0) == (pObj1->fPhase ^ pObj1->fMark0);
}

/**Function*************************************************************

  Synopsis    [Refines one equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssc_GiaSimClassCreate( Gia_Man_t * p, Vec_Int_t * vClass )
{
    int Repr = GIA_VOID, EntPrev = -1, Ent, i;
    assert( Vec_IntSize(vClass) > 0 );
    Vec_IntForEachEntry( vClass, Ent, i )
    {
        if ( i == 0 )
        {
            Repr = Ent;
            Gia_ObjSetRepr( p, Ent, GIA_VOID );
            EntPrev = Ent;
        }
        else
        {
            assert( Repr < Ent );
            Gia_ObjSetRepr( p, Ent, Repr );
            Gia_ObjSetNext( p, EntPrev, Ent );
            EntPrev = Ent;
        }
    }
    Gia_ObjSetNext( p, EntPrev, 0 );
}

/**Function*************************************************************

  Synopsis    [Refines one equivalence class using individual bit-pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssc_GiaSimClassRefineOneBit( Gia_Man_t * p, int i )
{
    Gia_Obj_t * pObj;
    int Ent;
    assert( Gia_ObjIsHead( p, i ) );
    Vec_IntClear( p->vClassOld );
    Vec_IntClear( p->vClassNew );
    Vec_IntPush( p->vClassOld, i );
    pObj = Gia_ManObj(p, i);
    Gia_ClassForEachObj1( p, i, Ent )
    {
        if ( Ssc_GiaSimAreEqualBit( p, i, Ent ) )
            Vec_IntPush( p->vClassOld, Ent );
        else
            Vec_IntPush( p->vClassNew, Ent );
    }
    if ( Vec_IntSize( p->vClassNew ) == 0 )
        return 0;
    Ssc_GiaSimClassCreate( p, p->vClassOld );
    Ssc_GiaSimClassCreate( p, p->vClassNew );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Refines one class using simulation patterns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ssc_GiaSimClassRefineOne( Gia_Man_t * p, int i )
{
    Gia_Obj_t * pObj;
    int Ent;
    assert( Gia_ObjIsHead( p, i ) );
    Vec_IntClear( p->vClassOld );
    Vec_IntClear( p->vClassNew );
    Vec_IntPush( p->vClassOld, i );
    pObj = Gia_ManObj(p, i);
    Gia_ClassForEachObj1( p, i, Ent )
    {
        if ( Ssc_GiaSimAreEqual( p, i, Ent ) )
            Vec_IntPush( p->vClassOld, Ent );
        else
            Vec_IntPush( p->vClassNew, Ent );
    }
    if ( Vec_IntSize( p->vClassNew ) == 0 )
        return 0;
    Ssc_GiaSimClassCreate( p, p->vClassOld );
    Ssc_GiaSimClassCreate( p, p->vClassNew );
    if ( Vec_IntSize(p->vClassNew) > 1 )
        return 1 + Ssc_GiaSimClassRefineOne( p, Vec_IntEntry(p->vClassNew,0) );
    return 1;
}
void Ssc_GiaSimProcessRefined( Gia_Man_t * p, Vec_Int_t * vRefined )
{
    int * pTable, nTableSize, i, k, Key;
    if ( Vec_IntSize(vRefined) == 0 )
        return;
    nTableSize = Abc_PrimeCudd( 100 + Vec_IntSize(vRefined) / 3 );
    pTable = ABC_CALLOC( int, nTableSize );
    Vec_IntForEachEntry( vRefined, i, k )
    {
        assert( !Ssc_GiaSimIsConst0( p, i ) );
        Key = Ssc_GiaSimHashKey( p, i, nTableSize );
        if ( pTable[Key] == 0 )
        {
            assert( Gia_ObjRepr(p, i) == 0 );
            assert( Gia_ObjNext(p, i) == 0 );
            Gia_ObjSetRepr( p, i, GIA_VOID );
        }
        else
        {
            Gia_ObjSetNext( p, pTable[Key], i );
            Gia_ObjSetRepr( p, i, Gia_ObjRepr(p, pTable[Key]) );
            if ( Gia_ObjRepr(p, i) == GIA_VOID )
                Gia_ObjSetRepr( p, i, pTable[Key] );
            assert( Gia_ObjRepr(p, i) > 0 );
        }
        pTable[Key] = i;
    }
    Vec_IntForEachEntry( vRefined, i, k )
        if ( Gia_ObjIsHead( p, i ) )
            Ssc_GiaSimClassRefineOne( p, i );
    ABC_FREE( pTable );
}

/**Function*************************************************************

  Synopsis    [Refines equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssc_GiaClassesInit( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    assert( p->pReprs == NULL );
    p->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
    p->pNexts = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
        Gia_ObjSetRepr( p, i, Gia_ObjIsCand(pObj) ? 0 : GIA_VOID );
    if ( p->vClassOld == NULL )
        p->vClassOld = Vec_IntAlloc( 100 );
    if ( p->vClassNew == NULL )
        p->vClassNew = Vec_IntAlloc( 100 );
}
int Ssc_GiaClassesRefine( Gia_Man_t * p )
{
    Vec_Int_t * vRefinedC;
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    assert( p->pReprs != NULL );
    vRefinedC = Vec_IntAlloc( 100 );
    Gia_ManForEachCand( p, pObj, i )
        if ( Gia_ObjIsTail(p, i) )
            Counter += Ssc_GiaSimClassRefineOne( p, Gia_ObjRepr(p, i) );
        else if ( Gia_ObjIsConst(p, i) && !Ssc_GiaSimIsConst0(p, i) )
            Vec_IntPush( vRefinedC, i );
    Ssc_GiaSimProcessRefined( p, vRefinedC );
    Counter += Vec_IntSize( vRefinedC );
    Vec_IntFree( vRefinedC );
    return Counter;
}


/**Function*************************************************************

  Synopsis    [Check if the pairs have been disproved.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ssc_GiaClassesCheckPairs( Gia_Man_t * p, Vec_Int_t * vDisPairs )
{
    int i, iRepr, iObj, Result = 1;
    Vec_IntForEachEntryDouble( vDisPairs, iRepr, iObj, i )
        if ( iRepr == Gia_ObjRepr(p, iObj) )
            printf( "Pair (%d, %d) are still equivalent.\n", iRepr, iObj ), Result = 0;
//    if ( Result )
//        printf( "Classes are refined correctly.\n" );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

