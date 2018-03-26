/**CFile****************************************************************

  FileName    [sbdSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [Simulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sbdSim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sbdInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline word * Sbd_ObjSims( Gia_Man_t * p, int i ) { return Vec_WrdEntryP( p->vSims,   p->iPatsPi * i );  }
static inline word * Sbd_ObjCtrl( Gia_Man_t * p, int i ) { return Vec_WrdEntryP( p->vSimsPi, p->iPatsPi * i );  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This does not work.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sbd_GiaSimRoundBack2( Gia_Man_t * p )
{
    int nWords = p->iPatsPi;
    Gia_Obj_t * pObj;
    int w, i, Id;
    // init primary outputs
    Gia_ManForEachCoId( p, Id, i )
        for ( w = 0; w < nWords; w++ )
            Sbd_ObjSims(p, Id)[w] = Gia_ManRandomW(0);
    // transfer to nodes
    Gia_ManForEachCo( p, pObj, i )
    {
        word * pSims = Sbd_ObjSims(p, Gia_ObjId(p, pObj));
        Abc_TtCopy( Sbd_ObjSims(p, Gia_ObjFaninId0p(p, pObj)), pSims, nWords, Gia_ObjFaninC0(pObj) );
    }
    // simulate nodes
    Gia_ManForEachAndReverse( p, pObj, i )
    {
        word * pSims  = Sbd_ObjSims(p, i);
        word * pSims0 = Sbd_ObjSims(p, Gia_ObjFaninId0(pObj, i));
        word * pSims1 = Sbd_ObjSims(p, Gia_ObjFaninId1(pObj, i));
        word Rand = Gia_ManRandomW(0);
        for ( w = 0; w < nWords; w++ )
        {
            pSims0[w] = pSims[w] | Rand;
            pSims1[w] = pSims[w] | ~Rand;
        }
        if ( Gia_ObjFaninC0(pObj) ) Abc_TtNot( pSims0, nWords );
        if ( Gia_ObjFaninC1(pObj) ) Abc_TtNot( pSims1, nWords );
    }
    // primary inputs are initialized
}


/**Function*************************************************************

  Synopsis    [Tries to falsify a sequence of two-literal SAT problems.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sbd_GiaSatOne_rec( Gia_Man_t * p, Gia_Obj_t * pObj, int Value, int fFirst, int iPat )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return (int)pObj->fMark0 == Value;
    Gia_ObjSetTravIdCurrent(p, pObj);
    pObj->fMark0 = Value;
    if ( Gia_ObjIsCi(pObj) )
    {
        word * pSims = Sbd_ObjSims(p, Gia_ObjId(p, pObj));
        if ( Abc_TtGetBit( pSims, iPat ) != Value )
            Abc_TtXorBit( pSims, iPat );
        return 1;
    }
    assert( Gia_ObjIsAnd(pObj) );
    assert( !Gia_ObjIsXor(pObj) );
    if ( Value )
        return Sbd_GiaSatOne_rec( p, Gia_ObjFanin0(pObj), !Gia_ObjFaninC0(pObj), fFirst, iPat ) &&
               Sbd_GiaSatOne_rec( p, Gia_ObjFanin1(pObj), !Gia_ObjFaninC1(pObj), fFirst, iPat );
    if ( fFirst )
        return Sbd_GiaSatOne_rec( p, Gia_ObjFanin0(pObj), Gia_ObjFaninC0(pObj), fFirst, iPat );
    else
        return Sbd_GiaSatOne_rec( p, Gia_ObjFanin1(pObj), Gia_ObjFaninC1(pObj), fFirst, iPat );
}
int Sbd_GiaSatOne( Gia_Man_t * p, Vec_Int_t * vPairs )
{
    int k, n, Var1, Var2, iPat = 0;
    //Gia_ManSetPhase( p );
    Vec_IntForEachEntryDouble( vPairs, Var1, Var2, k )
    {
        Gia_Obj_t * pObj1 = Gia_ManObj( p, Var1 );
        Gia_Obj_t * pObj2 = Gia_ManObj( p, Var2 );
        assert( Var2 > 0 );
        if ( Var1 == 0 )
        {
            for ( n = 0; n < 2; n++ )
            {
                Gia_ManIncrementTravId( p );
                if ( Sbd_GiaSatOne_rec(p, pObj2, !pObj2->fPhase, n, iPat) )
                {
                    iPat++;
                    break;
                }
            }
            printf( "%c", n == 2 ? '.' : 'c' );
        }
        else
        {
            for ( n = 0; n < 2; n++ )
            {
                Gia_ManIncrementTravId( p );
                if ( Sbd_GiaSatOne_rec(p, pObj1, !pObj1->fPhase, n, iPat) && Sbd_GiaSatOne_rec(p, pObj2, pObj2->fPhase, n, iPat) )
                {
                    iPat++;
                    break;
                }
                Gia_ManIncrementTravId( p );
                if ( Sbd_GiaSatOne_rec(p, pObj1, pObj1->fPhase, n, iPat) && Sbd_GiaSatOne_rec(p, pObj2, !pObj2->fPhase, n, iPat) )
                {
                    iPat++;
                    break;
                }
            }
            printf( "%c", n == 2 ? '.' : 'e' );
        }
        if ( iPat == 64 * p->iPatsPi - 1 )
            break;
    }
    printf( "\n" );
    return iPat;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sbd_GiaSimRoundBack( Gia_Man_t * p )
{
    extern void Sbd_GiaSimRound( Gia_Man_t * p, int fTry, Vec_Int_t ** pvMap );
    Vec_Int_t * vReprs = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vPairs = Vec_IntAlloc( 1000 );
    Vec_Int_t * vMap; // maps each node into its class
    int i, nConsts = 0, nClasses = 0, nPats;
    Sbd_GiaSimRound( p, 0, &vMap );
    Gia_ManForEachAndId( p, i )
    {
        if ( Vec_IntEntry(vMap, i) == 0 )
            Vec_IntPushTwo( vPairs, 0, i ), nConsts++;
        else if ( Vec_IntEntry(vReprs, Vec_IntEntry(vMap, i)) == 0 )
            Vec_IntWriteEntry( vReprs, Vec_IntEntry(vMap, i), i );
        else if ( Vec_IntEntry(vReprs, Vec_IntEntry(vMap, i)) != -1 )
        {
            Vec_IntPushTwo( vPairs, Vec_IntEntry(vReprs, Vec_IntEntry(vMap, i)), i );
            Vec_IntWriteEntry( vReprs, Vec_IntEntry(vMap, i), -1 );
            nClasses++;
        }
    }
    Vec_IntFree( vMap );
    Vec_IntFree( vReprs );
    printf( "Constants = %d.   Classes = %d.\n", nConsts, nClasses );

    nPats = Sbd_GiaSatOne( p, vPairs );
    Vec_IntFree( vPairs );

    printf( "Generated %d patterns.\n", nPats );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sbd_GiaSimRound( Gia_Man_t * p, int fTry, Vec_Int_t ** pvMap )
{
    int nWords = p->iPatsPi;
    Vec_Mem_t * vStore;
    Gia_Obj_t * pObj;
    Vec_Int_t * vMap = Vec_IntStart( Gia_ManObjNum(p) );
    int w, i, Id, fCompl, RetValue;
    // init primary inputs
    if ( fTry )
    {
        Sbd_GiaSimRoundBack( p );
        Gia_ManForEachCiId( p, Id, i )
            Sbd_ObjSims(p, Id)[0] <<= 1;
    }
    else
    {
        Gia_ManForEachCiId( p, Id, i )
            for ( w = 0; w < nWords; w++ )
                Sbd_ObjSims(p, Id)[w] = Gia_ManRandomW(0) << !w;
    }
    // simulate internal nodes
    vStore = Vec_MemAlloc( nWords, 16 ); // 2^12 N-word entries per page
    Vec_MemHashAlloc( vStore, 1 << 16 );
    RetValue = Vec_MemHashInsert( vStore, Sbd_ObjSims(p, 0) ); // const zero
    assert( RetValue == 0 );
    Gia_ManForEachAnd( p, pObj, i )
    {
        word * pSims = Sbd_ObjSims(p, i);
        if ( Gia_ObjIsXor(pObj) )
            Abc_TtXor( pSims, 
                Sbd_ObjSims(p, Gia_ObjFaninId0(pObj, i)), 
                Sbd_ObjSims(p, Gia_ObjFaninId1(pObj, i)), 
                nWords, 
                Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC1(pObj) );
        else
            Abc_TtAndCompl( pSims, 
                Sbd_ObjSims(p, Gia_ObjFaninId0(pObj, i)), Gia_ObjFaninC0(pObj), 
                Sbd_ObjSims(p, Gia_ObjFaninId1(pObj, i)), Gia_ObjFaninC1(pObj), 
                nWords );
        // hash sim info
        fCompl = (int)(pSims[0] & 1);
        if ( fCompl ) Abc_TtNot( pSims, nWords );
        Vec_IntWriteEntry( vMap, i, Vec_MemHashInsert(vStore, pSims) );
        if ( fCompl ) Abc_TtNot( pSims, nWords );
    }
    Gia_ManForEachCo( p, pObj, i )
    {
        word * pSims = Sbd_ObjSims(p, Gia_ObjId(p, pObj));
        Abc_TtCopy( pSims, Sbd_ObjSims(p, Gia_ObjFaninId0p(p, pObj)), nWords, Gia_ObjFaninC0(pObj) );
//        printf( "%d ", Abc_TtCountOnesVec(pSims, nWords) );
        assert( Gia_ObjPhase(pObj) == (int)(pSims[0] & 1) );
    }
//    printf( "\n" );
    Vec_MemHashFree( vStore );
    Vec_MemFree( vStore );
    printf( "Objects = %6d.  Unique = %6d.\n", Gia_ManAndNum(p), Vec_IntCountUnique(vMap) );
    if ( pvMap )
        *pvMap = vMap;
    else
        Vec_IntFree( vMap );
}
void Sbd_GiaSimTest( Gia_Man_t * pGia )
{
    Gia_ManSetPhase( pGia );

    // allocate simulation info
    pGia->iPatsPi = 32;
    pGia->vSims   = Vec_WrdStart( Gia_ManObjNum(pGia) * pGia->iPatsPi );
    pGia->vSimsPi = Vec_WrdStart( Gia_ManObjNum(pGia) * pGia->iPatsPi );

    Gia_ManRandom( 1 );

    Sbd_GiaSimRound( pGia, 0, NULL );
    Sbd_GiaSimRound( pGia, 0, NULL );
    Sbd_GiaSimRound( pGia, 0, NULL );

    printf( "\n" );
    Sbd_GiaSimRound( pGia, 1, NULL );
    printf( "\n" );
    Sbd_GiaSimRound( pGia, 1, NULL );
    printf( "\n" );
    Sbd_GiaSimRound( pGia, 1, NULL );

    Vec_WrdFreeP( &pGia->vSims );
    Vec_WrdFreeP( &pGia->vSimsPi );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

