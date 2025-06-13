/**CFile****************************************************************

  FileName    [acecPa.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecPa.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecWec.h"
#include "misc/extra/extra.h"

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
int Pas_ManVerifyPhaseOne_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    int Truth0, Truth1;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return pObj->Value;
    Gia_ObjSetTravIdCurrent(p, pObj);
    assert( Gia_ObjIsAnd(pObj) );
    assert( !Gia_ObjIsXor(pObj) );
    Truth0 = Pas_ManVerifyPhaseOne_rec( p, Gia_ObjFanin0(pObj) );
    Truth1 = Pas_ManVerifyPhaseOne_rec( p, Gia_ObjFanin1(pObj) );
    Truth0 = Gia_ObjFaninC0(pObj) ? 0xFF & ~Truth0 : Truth0;
    Truth1 = Gia_ObjFaninC1(pObj) ? 0xFF & ~Truth1 : Truth1;
    return (pObj->Value = Truth0 & Truth1);
}
void Pas_ManVerifyPhaseOne( Gia_Man_t * p, Vec_Int_t * vAdds, int iBox, Vec_Bit_t * vPhase )
{
    Gia_Obj_t * pObj;
    unsigned TruthXor, TruthMaj, Truths[3] = { 0xAA, 0xCC, 0xF0 };
    int k, iObj, fFadd = Vec_IntEntry(vAdds, 6*iBox+2) > 0;

    if ( !fFadd )
        return;

    Gia_ManIncrementTravId( p );
    for ( k = 0; k < 3; k++ )
    {
        iObj = Vec_IntEntry( vAdds, 6*iBox+k );
        if ( iObj == 0 )
            continue;
        pObj = Gia_ManObj( p, iObj );
        pObj->Value = Vec_BitEntry(vPhase, iObj) ? 0xFF & ~Truths[k] : Truths[k];
        Gia_ObjSetTravIdCurrent( p, pObj );
    }

    iObj = Vec_IntEntry( vAdds, 6*iBox+3 );
    TruthXor = Pas_ManVerifyPhaseOne_rec( p, Gia_ManObj(p, iObj) );
    TruthXor = Vec_BitEntry(vPhase, iObj) ? 0xFF & ~TruthXor : TruthXor;

    iObj = Vec_IntEntry( vAdds, 6*iBox+4 );
    TruthMaj = Pas_ManVerifyPhaseOne_rec( p, Gia_ManObj(p, iObj) );
    TruthMaj = Vec_BitEntry(vPhase, iObj) ? 0xFF & ~TruthMaj : TruthMaj;

    if ( fFadd ) // FADD
    {
        if ( TruthXor != 0x96 )
            printf( "Fadd %d sum is wrong.\n", iBox );
        if ( TruthMaj != 0xE8 )
            printf( "Fadd %d carry is wrong.\n", iBox );
    }
    else
    {
        if ( TruthXor != 0x66 )
            printf( "Hadd %d sum is wrong.\n", iBox );
        if ( TruthMaj != 0x88 )
            printf( "Hadd %d carry is wrong.\n", iBox );
    }
}
void Pas_ManVerifyPhase( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vOrder, Vec_Bit_t * vPhase )
{
    int k, iBox;
    Vec_IntForEachEntry( vOrder, iBox, k )
        Pas_ManVerifyPhaseOne( p, vAdds, iBox, vPhase );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pas_ManPhase_rec( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vMap, Gia_Obj_t * pObj, int fPhase, Vec_Bit_t * vPhase, Vec_Bit_t * vConstPhase )
{
    int k, iBox, iXor, Sign, fXorPhase;
    assert( pObj != Gia_ManConst0(p) );
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( fPhase )
        Vec_BitWriteEntry( vPhase, Gia_ObjId(p, pObj), fPhase );
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    iBox = Vec_IntEntry( vMap, Gia_ObjId(p, pObj) );
    if ( iBox == -1 )
        return;
    iXor = Vec_IntEntry( vAdds, 6*iBox+3 );
    Sign = Vec_IntEntry( vAdds, 6*iBox+5 );
    fXorPhase = ((Sign >> 3) & 1);
    // remember complemented HADD
    if ( Vec_IntEntry(vAdds, 6*iBox+2) == 0 && fPhase )
        Vec_BitWriteEntry( vConstPhase, iBox, 1 );
    for ( k = 0; k < 3; k++ )
    {
        int iObj = Vec_IntEntry( vAdds, 6*iBox+k );
        int fPhaseThis = ((Sign >> k) & 1) ^ fPhase;
        fXorPhase ^= fPhaseThis;
        if ( iObj == 0 )
            continue;
        Pas_ManPhase_rec( p, vAdds, vMap, Gia_ManObj(p, iObj), fPhaseThis, vPhase, vConstPhase );
    }
    Vec_BitWriteEntry( vPhase, iXor, fXorPhase );
}
Vec_Bit_t * Pas_ManPhase( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vMap, Vec_Int_t * vRoots, Vec_Bit_t ** pvConstPhase )
{
    Vec_Bit_t * vPhase = Vec_BitStart( Vec_IntSize(vMap) );
    Vec_Bit_t * vConstPhase = Vec_BitStart( Vec_IntSize(vAdds)/6 );
    int i, iRoot;
    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vRoots, iRoot, i )
        Pas_ManPhase_rec( p, vAdds, vMap, Gia_ManObj(p, iRoot), 1, vPhase, vConstPhase );
    *pvConstPhase = vConstPhase;
    return vPhase;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pas_ManComputeCuts( Gia_Man_t * p, Vec_Int_t * vAdds, Vec_Int_t * vOrder, Vec_Int_t * vIns, Vec_Int_t * vOuts )
{
    Vec_Bit_t * vUnique = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Bit_t * vUsed = Vec_BitStart( Gia_ManObjNum(p) );
    Vec_Int_t * vMap = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_Int_t * vRoots = Vec_IntAlloc( 100 );
    Vec_Bit_t * vPhase, * vConstPhase;
    int i, k, Entry, nTrees;

    // map carries into adder indexes and mark driven nodes
    Vec_IntForEachEntry( vOrder, i, k )
    {
        int Carry = Vec_IntEntry(vAdds, 6*i+4);
        if ( Vec_BitEntry(vUnique, Carry) )
            printf( "Carry %d participates more than once.\n", Carry );
        Vec_BitWriteEntry( vUnique, Carry, 1 );
        Vec_IntWriteEntry( vMap, Carry, i );
        // mark driven 
        Vec_BitWriteEntry( vUsed, Vec_IntEntry(vAdds, 6*i+0), 1 );
        Vec_BitWriteEntry( vUsed, Vec_IntEntry(vAdds, 6*i+1), 1 );
        Vec_BitWriteEntry( vUsed, Vec_IntEntry(vAdds, 6*i+2), 1 );
    }
    // collect carries that do not drive other adders
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        if ( Vec_BitEntry(vUnique, i) && !Vec_BitEntry(vUsed, i) )
            Vec_IntPush( vRoots, i );
    nTrees = Vec_IntSize( vRoots );

    Vec_IntPrint( vRoots );

    // compute phases
    if ( Vec_IntSize(vRoots) > 0 )
    {
        int nCompls = 0;

        vPhase = Pas_ManPhase( p, vAdds, vMap, vRoots, &vConstPhase );
        Pas_ManVerifyPhase( p, vAdds, vOrder, vPhase );

        printf( "Outputs: " );
        Vec_IntForEachEntry( vOuts, Entry, i )
            printf( "%d(%d) ", Entry, Vec_BitEntry(vPhase, Entry) );
        printf( "\n" );

        printf( "Inputs: " );
        Vec_IntForEachEntry( vIns, Entry, i )
        {
            printf( "%d(%d) ", Entry, Vec_BitEntry(vPhase, Entry) );
            nCompls += Vec_BitEntry(vPhase, Entry);
        }
        printf( "   Compl = %d\n", nCompls );

        Vec_BitFreeP( &vPhase );
        Vec_BitFreeP( &vConstPhase );
    }

    Vec_IntFree( vRoots );
    Vec_IntFree( vMap );
    Vec_BitFree( vUnique );
    Vec_BitFree( vUsed );
    return nTrees;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pas_ManComputeCutsTest( Gia_Man_t * p )
{
    abctime clk = Abc_Clock();
    Vec_Int_t * vAdds = Ree_ManComputeCuts( p, NULL, 1 );
    Vec_Int_t * vIns, * vOuts;
    Vec_Int_t * vOrder = Gia_PolynCoreOrder( p, vAdds, NULL, &vIns, &vOuts );
    int nTrees, nFadds = Ree_ManCountFadds( vAdds );
    //Ree_ManPrintAdders( vAdds, 1 );
    printf( "Detected %d FAs and %d HAs.  Collected %d adders.  ", nFadds, Vec_IntSize(vAdds)/6-nFadds, Vec_IntSize(vOrder) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // detect trees
    clk = Abc_Clock();
    nTrees = Pas_ManComputeCuts( p, vAdds, vOrder, vIns, vOuts );
    Vec_IntFree( vAdds );
    Vec_IntFree( vOrder );
    Vec_IntFree( vIns );
    Vec_IntFree( vOuts );

    printf( "Detected %d adder trees. ", nTrees );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

