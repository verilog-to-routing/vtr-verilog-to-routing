/**CFile****************************************************************

  FileName    [acecCover.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecCover.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_AcecMark_rec( Gia_Man_t * p, int iObj, int fFirst )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( pObj->fMark0 && !fFirst )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    pObj->fMark1 = 1;
    Gia_AcecMark_rec( p, Gia_ObjFaninId0(pObj, iObj), 0 );
    Gia_AcecMark_rec( p, Gia_ObjFaninId1(pObj, iObj), 0 );
}
void Gia_AcecMarkFadd( Gia_Man_t * p, int * pSigs )
{
//    if ( Gia_ManObj(p, pSigs[3])->fMark1 || Gia_ManObj(p, pSigs[4])->fMark1 )
//        return;
    Gia_ManObj( p, pSigs[0] )->fMark0 = 1;
    Gia_ManObj( p, pSigs[1] )->fMark0 = 1;
    Gia_ManObj( p, pSigs[2] )->fMark0 = 1;
//    assert( !Gia_ManObj(p, pSigs[3])->fMark1 );
//    assert( !Gia_ManObj(p, pSigs[4])->fMark1 );
    Gia_AcecMark_rec( p, pSigs[3], 1 );
    Gia_AcecMark_rec( p, pSigs[4], 1 );
}
void Gia_AcecMarkHadd( Gia_Man_t * p, int * pSigs )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, pSigs[0] );
    int iFan0 = Gia_ObjFaninId0( pObj, pSigs[0] );
    int iFan1 = Gia_ObjFaninId1( pObj, pSigs[0] );
    Gia_ManObj( p, iFan0 )->fMark0 = 1;
    Gia_ManObj( p, iFan1 )->fMark0 = 1;
    Gia_AcecMark_rec( p, pSigs[0], 1 );
    Gia_AcecMark_rec( p, pSigs[1], 1 );
}

/**Function*************************************************************

  Synopsis    [Collect XORs reachable from the last output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_AcecCollectXors_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Bit_t * vMap, Vec_Int_t * vXors )
{
    if ( !Gia_ObjIsXor(pObj) )//|| Vec_BitEntry(vMap, Gia_ObjId(p, pObj)) )
        return;
    Vec_IntPush( vXors, Gia_ObjId(p, pObj) );
    Gia_AcecCollectXors_rec( p, Gia_ObjFanin0(pObj), vMap, vXors );
    Gia_AcecCollectXors_rec( p, Gia_ObjFanin1(pObj), vMap, vXors );
}
Vec_Int_t * Gia_AcecCollectXors( Gia_Man_t * p, Vec_Bit_t * vMap )
{
    Vec_Int_t * vXors = Vec_IntAlloc( 100 );
    Gia_Obj_t * pObj = Gia_ObjFanin0( Gia_ManCo(p, Gia_ManCoNum(p)-1) );
    Gia_AcecCollectXors_rec( p, pObj, vMap, vXors );
    return vXors;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_AcecExplore( Gia_Man_t * p, int fVerbose )
{
    Vec_Int_t * vNodes = Vec_IntAlloc( 100 );
    Vec_Int_t * vFadds, * vHadds, * vXors;
    Vec_Bit_t * vMap = Vec_BitStart( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj; 
    int i, nSupp, nCone, nHadds = 0;
    assert( p->pMuxes != NULL );
    vFadds = Gia_ManDetectFullAdders( p, fVerbose, NULL );
    vHadds = Gia_ManDetectHalfAdders( p, fVerbose );

    pObj = Gia_ManObj( p, 352 );
    printf( "Xor = %d.\n", Gia_ObjIsXor(pObj) );
    printf( "Fanin0 = %d.  Fanin1 = %d.\n", Gia_ObjFaninId0(pObj, 352), Gia_ObjFaninId1(pObj, 352) );
    printf( "Fan00 = %d.  Fan01 = %d.   Fan10 = %d.  Fan11 = %d.\n", 
        Gia_ObjFaninId0(Gia_ObjFanin0(pObj), Gia_ObjFaninId0(pObj, 352)), 
        Gia_ObjFaninId1(Gia_ObjFanin0(pObj), Gia_ObjFaninId0(pObj, 352)), 
        Gia_ObjFaninId0(Gia_ObjFanin1(pObj), Gia_ObjFaninId1(pObj, 352)), 
        Gia_ObjFaninId1(Gia_ObjFanin1(pObj), Gia_ObjFaninId1(pObj, 352)) );

    // create a map of all HADD/FADD outputs
    for ( i = 0; i < Vec_IntSize(vHadds)/2; i++ )
    {
        int * pSigs = Vec_IntEntryP(vHadds, 2*i);
        Vec_BitWriteEntry( vMap, pSigs[0], 1 );
        Vec_BitWriteEntry( vMap, pSigs[1], 1 );
    }
    for ( i = 0; i < Vec_IntSize(vFadds)/5; i++ )
    {
        int * pSigs = Vec_IntEntryP(vFadds, 5*i);
        Vec_BitWriteEntry( vMap, pSigs[3], 1 );
        Vec_BitWriteEntry( vMap, pSigs[4], 1 );
    }

    Gia_ManCleanMark01( p );

    // mark outputs
    Gia_ManForEachCo( p, pObj, i )
        Gia_ObjFanin0(pObj)->fMark0 = 1;

    // collect XORs
    vXors = Gia_AcecCollectXors( p, vMap );
    Vec_BitFree( vMap );

    printf( "Collected XORs: " );
    Vec_IntPrint( vXors );

    // mark their fanins
    Gia_ManForEachObjVec( vXors, p, pObj, i )
    {
        pObj->fMark1 = 1;
        Gia_ObjFanin0(pObj)->fMark0 = 1;
        Gia_ObjFanin1(pObj)->fMark0 = 1;
    }

    // mark FADDs
    for ( i = 0; i < Vec_IntSize(vFadds)/5; i++ )
        Gia_AcecMarkFadd( p, Vec_IntEntryP(vFadds, 5*i) );

    // iterate through HADDs and find those that fit in
    while ( 1 )
    {
        int fChange = 0;
        for ( i = 0; i < Vec_IntSize(vHadds)/2; i++ )
        {
            int * pSigs = Vec_IntEntryP(vHadds, 2*i);
            if ( !Gia_ManObj(p, pSigs[0])->fMark0 || !Gia_ManObj(p, pSigs[1])->fMark0 )
                continue;
            if ( Gia_ManObj(p, pSigs[0])->fMark1 || Gia_ManObj(p, pSigs[1])->fMark1 )
                continue;
            Gia_AcecMarkHadd( p, pSigs );
            fChange = 1;
            nHadds++;
        }
        if ( !fChange )
            break;
    }
    // print inputs to the adder network
    Gia_ManForEachAnd( p, pObj, i )
        if ( pObj->fMark0 && !pObj->fMark1 )
        {
            nSupp = Gia_ManSuppSize( p, &i, 1 );
            nCone = Gia_ManConeSize( p, &i, 1 );
            printf( "Node %5d : Supp = %5d.  Cone = %5d.\n", i, nSupp, nCone );
            Vec_IntPush( vNodes, i );
        }
    printf( "Fadds = %d. Hadds = %d.  Root nodes found = %d.\n", Vec_IntSize(vFadds)/5, nHadds, Vec_IntSize(vNodes) );

    Gia_ManCleanMark01( p );

    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        pObj->fMark0 = 1;

    Vec_IntFree( vFadds );
    Vec_IntFree( vHadds );
    Vec_IntFree( vNodes );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_AcecCover( Gia_Man_t * p )
{
    int fVerbose = 1;
    int i, k, Entry;
    Gia_Obj_t * pObj;
    Vec_Int_t * vCutsXor2 = NULL;
    Vec_Int_t * vFadds = Gia_ManDetectFullAdders( p, fVerbose, &vCutsXor2 );

    // mark FADDs
    Gia_ManCleanMark01( p );
    for ( i = 0; i < Vec_IntSize(vFadds)/5; i++ )
        Gia_AcecMarkFadd( p, Vec_IntEntryP(vFadds, 5*i) );

    k = 0;
    Vec_IntForEachEntry( vCutsXor2, Entry, i )
    {
        if ( i % 3 != 2 )
            continue;
        pObj = Gia_ManObj( p, Entry );
        if ( pObj->fMark1 )
            continue;
        printf( "%d ", Entry );
    }
    printf( "\n" );

    Gia_ManCleanMark01( p );

    Vec_IntFree( vFadds );
    Vec_IntFree( vCutsXor2 );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

