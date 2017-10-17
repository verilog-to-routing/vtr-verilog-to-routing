/**CFile****************************************************************

  FileName    [bacOper.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Operator procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacOper.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"

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
int Bac_BoxCreate( Bac_Ntk_t * p, Bac_ObjType_t Type, Vec_Int_t * vFanins, int nInA, int nInB, int nOuts )
{
    char pName[100]; int i, iObj, iFanin;
    assert( BAC_OBJ_BOX < Type && Type < BAC_BOX_UNKNOWN );
    if ( BAC_BOX_CF <= Type && Type <= BAC_BOX_CZ )
    {
        sprintf( pName, "ABCCTo%d", nOuts );
        assert( 0 == Vec_IntSize(vFanins) );
        iObj = Bac_BoxAlloc( p, Type, 0, nOuts, Abc_NamStrFindOrAdd(p->pDesign->pMods, pName, NULL) );
    }
    else if ( BAC_BOX_BUF <= Type && Type <= BAC_BOX_INV )
    {
        char * pPref[2] = { "ABCBUF", "ABCINV" };
        assert( nInA == nOuts );
        assert( nInA == Vec_IntSize(vFanins) );
        sprintf( pName, "%sa%do%d", pPref[Type - BAC_BOX_BUF], nInA, nOuts );
        iObj = Bac_BoxAlloc( p, Type, Vec_IntSize(vFanins), nOuts, Abc_NamStrFindOrAdd(p->pDesign->pMods, pName, NULL) );
    }
    else if ( BAC_BOX_AND <= Type && Type <= BAC_BOX_XNOR )
    {
        char * pPref[6] = { "ABCAND", "ABCNAND", "ABCOR", "ABCNOR", "ABCXOR", "ABCXNOR" };
        assert( nInA == nOuts && nInB == nOuts );
        assert( nInA + nInB == Vec_IntSize(vFanins) );
        sprintf( pName, "%sa%db%do%d", pPref[Type - BAC_BOX_AND], nInA, nInB, nOuts );
        iObj = Bac_BoxAlloc( p, Type, Vec_IntSize(vFanins), nOuts, Abc_NamStrFindOrAdd(p->pDesign->pMods, pName, NULL) );
    }
    else if ( Type == BAC_BOX_MUX )
    {
        char * pPref[1] = { "ABCMUX" };
        assert( nInA == nOuts && nInB == nOuts );
        assert( 1 + nInA + nInB == Vec_IntSize(vFanins) );
        sprintf( pName, "%sc%da%db%do%d", pPref[Type - BAC_BOX_MUX], 1, nInA, nInB, nOuts );
        iObj = Bac_BoxAlloc( p, Type, Vec_IntSize(vFanins), nOuts, Abc_NamStrFindOrAdd(p->pDesign->pMods, pName, NULL) );
    }
    else if ( Type == BAC_BOX_MAJ )
    {
        char * pPref[1] = { "ABCMAJ" };
        assert( nInA == 1 && nInB == 1 && nOuts == 1 );
        assert( 3 == Vec_IntSize(vFanins) );
        sprintf( pName, "%sa%db%dc%do%d", pPref[Type - BAC_BOX_MAJ], 1, 1, 1, 1 );
        iObj = Bac_BoxAlloc( p, Type, Vec_IntSize(vFanins), nOuts, Abc_NamStrFindOrAdd(p->pDesign->pMods, pName, NULL) );
    }
    else if ( BAC_BOX_RAND <= Type && Type <= BAC_BOX_RXNOR )
    {
        char * pPref[6] = { "ABCRAND", "ABCRNAND", "ABCROR", "ABCRNOR", "ABCRXOR", "ABCRXNOR" };
        assert( nInA == nInB && 1 == nOuts );
        assert( nInA + nInB == Vec_IntSize(vFanins) );
        sprintf( pName, "%sa%db%do%d", pPref[Type - BAC_BOX_RAND], nInA, nInB, nOuts );
        iObj = Bac_BoxAlloc( p, Type, Vec_IntSize(vFanins), nOuts, Abc_NamStrFindOrAdd(p->pDesign->pMods, pName, NULL) );
    }
    else if ( Type == BAC_BOX_SEL )
    {
        char * pPref[1] = { "ABCSEL" };
        assert( nInA * nOuts == nInB );
        assert( nInA + nInB == Vec_IntSize(vFanins) );
        sprintf( pName, "%sa%db%do%d", pPref[Type - BAC_BOX_SEL], nInA, nInB, nOuts );
        iObj = Bac_BoxAlloc( p, Type, Vec_IntSize(vFanins), nOuts, Abc_NamStrFindOrAdd(p->pDesign->pMods, pName, NULL) );
    }
    else if ( Type == BAC_BOX_PSEL )
    {
        char * pPref[1] = { "ABCPSEL" };
        assert( nInA * nOuts == nInB );
        assert( 1 + nInA + nInB == Vec_IntSize(vFanins) );
        sprintf( pName, "%si%da%db%do%d", pPref[Type - BAC_BOX_SEL], 1, nInA, nInB, nOuts );
        iObj = Bac_BoxAlloc( p, Type, Vec_IntSize(vFanins), nOuts, Abc_NamStrFindOrAdd(p->pDesign->pMods, pName, NULL) );
    }
    // add fanins
    Vec_IntForEachEntry( vFanins, iFanin, i )
        Bac_ObjSetFanin( p, Bac_BoxBi(p, iObj, i), iFanin );
    return iObj;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bac_ObjClpWide( Bac_Ntk_t * p, int iBox )
{
    Bac_ObjType_t Type = Bac_ObjType( p, iBox );
    int nBis = Bac_BoxBiNum(p, iBox);
    int nBos = Bac_BoxBoNum(p, iBox);
    int i, k, iObj;
    assert( nBos > 1 );
    Vec_IntClear( &p->vArray );
    if ( BAC_BOX_BUF <= Type && Type <= BAC_BOX_INV )
    {
        for ( i = 0; i < nBos; i++ )
        {
            Vec_IntFill( &p->vArray2, 1, Bac_BoxFanin(p, iBox, i) );
            iObj = Bac_BoxCreate( p, Type, &p->vArray2, 1, -1, 1 );
            Vec_IntPush( &p->vArray, Bac_BoxBo(p, iObj, 0) );
        }
    }
    else if ( BAC_BOX_AND <= Type && Type <= BAC_BOX_XNOR )
    {
        assert( nBis == 2 * nBos );
        for ( i = 0; i < nBos; i++ )
        {
            Vec_IntFillTwo( &p->vArray2, 2, Bac_BoxFanin(p, iBox, i), Bac_BoxFanin(p, iBox, nBos+i) );
            iObj = Bac_BoxCreate( p, Type, &p->vArray2, 1, 1, 1 );
            Vec_IntPush( &p->vArray, Bac_BoxBo(p, iObj, 0) );
        }
    }
    else if ( Type == BAC_BOX_MUX )
    {
        assert( nBis - 1 == 2 * nBos );
        for ( i = 0; i < nBos; i++ )
        {
            Vec_IntFill( &p->vArray2, 1, Bac_BoxFanin(p, iBox, 0) );
            Vec_IntPushTwo( &p->vArray2, Bac_BoxFanin(p, iBox, 1+i), Bac_BoxFanin(p, iBox, 1+nBos+i) );
            iObj = Bac_BoxCreate( p, Type, &p->vArray2, 1, 1, 1 );
            Vec_IntPush( &p->vArray, Bac_BoxBo(p, iObj, 0) );
        }
    }
    else if ( Type == BAC_BOX_NMUX )
    {
        int n, nIns = nBis / nBos;
        assert( nBis % nBos == 0 );
        for ( n = 1; n < 32; n++ )
            if ( n + (1 << n) == nIns )
                break;
        assert( n > 1 && n < 32 );
        for ( i = 0; i < nBos; i++ )
        {
            Vec_IntClear( &p->vArray2 );
            for ( k = 0; k < n; k++ )
                Vec_IntPush( &p->vArray2, Bac_BoxFanin(p, iBox, k) );
            for ( k = 0; k < (1 << n); k++ )
                Vec_IntPush( &p->vArray2, Bac_BoxFanin(p, iBox, n + (1 << n) * i + k) );
            iObj = Bac_BoxCreate( p, Type, &p->vArray2, n, (1 << n), 1 );
            Vec_IntPush( &p->vArray, Bac_BoxBo(p, iObj, 0) );
        }
    }
    else if ( Type == BAC_BOX_SEL )
    {
    }
    else if ( Type == BAC_BOX_PSEL )
    {
    }
    else if ( Type == BAC_BOX_DFF || Type == BAC_BOX_LATCH )
    {
    }
    else if ( Type == BAC_BOX_DFFRS || Type == BAC_BOX_LATCHRS )
    {
    }
    else assert( 0 );
    Bac_BoxReplace( p, iBox, Vec_IntArray(&p->vArray), Vec_IntSize(&p->vArray) );
    return iBox;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bac_ObjClpArith( Bac_Ntk_t * p, int iBox )
{
    Bac_ObjType_t Type = Bac_ObjType( p, iBox );
    int i, iObj = -1;
    int nBis = 0;//Bac_NtkReadRangesPrim( Bac_BoxNtkName(p, iObj), &p->vArray, 0 );
    assert( nBis == Bac_BoxBiNum(p, iBox) );
    if ( Type == BAC_BOX_ADD )
    {
        int Carry = Bac_BoxFanin(p, iBox, 0);
        int nBits = Vec_IntEntry(&p->vArray, 1);
        assert( Vec_IntSize(&p->vArray) == 3 );
        assert( Vec_IntEntry(&p->vArray, 0) == 1 );
        assert( Vec_IntEntry(&p->vArray, 2) == nBits );
        Vec_IntClear( &p->vArray );
        for ( i = 0; i < nBits; i++ )
        {
            Vec_IntFill( &p->vArray2, 1, Carry );
            Vec_IntPushTwo( &p->vArray2, Bac_BoxFanin(p, iBox, 1+i), Bac_BoxFanin(p, iBox, 1+nBits+i) );
            iObj = Bac_BoxCreate( p, BAC_BOX_ADD, &p->vArray2, 1, 1, 1 );
            Carry = Bac_BoxBo(p, iObj, 1);
            Vec_IntPush( &p->vArray, Bac_BoxBo(p, iObj, 0) );
        }
        Vec_IntPush( &p->vArray, Carry );
    }
    else if ( Type == BAC_BOX_SUB )
    {
        int iConst, nBits = Vec_IntEntry(&p->vArray, 0);
        assert( Vec_IntSize(&p->vArray) == 2 );
        assert( Vec_IntEntry(&p->vArray, 1) == nBits );
        // create inverter
        Vec_IntClear( &p->vArray2 );
        for ( i = 0; i < nBits; i++ )
            Vec_IntPush( &p->vArray2, Bac_BoxFanin(p, iBox, nBits+i) );
        iObj = Bac_BoxCreate( p, BAC_BOX_INV, &p->vArray2, nBits, -1, nBits );
        // create constant
        Vec_IntClear( &p->vArray2 );
        iConst = Bac_BoxCreate( p, BAC_BOX_CT, &p->vArray2, -1, -1, 1 );
        // collect fanins
        Vec_IntFill( &p->vArray2, 1, iConst+1 );
        for ( i = 0; i < nBits; i++ )
            Vec_IntPush( &p->vArray2, Bac_BoxFanin(p, iBox, i) );
        for ( i = 0; i < nBits; i++ )
            Vec_IntPush( &p->vArray2, Bac_BoxBo(p, iObj, i) );
        // create adder
        iObj = Bac_BoxCreate( p, BAC_BOX_ADD, &p->vArray2, nBits, nBits, nBits );
        // collect fanins
        Vec_IntClear( &p->vArray );
        for ( i = 0; i < nBits; i++ )
            Vec_IntPush( &p->vArray, Bac_BoxBo(p, iObj, i) );
    }
    else if ( Type == BAC_BOX_MUL )
    {
    }
    else if ( Type == BAC_BOX_DIV )
    {
    }
    else if ( Type == BAC_BOX_MOD )
    {
    }
    else if ( Type == BAC_BOX_REM )
    {
    }
    else if ( Type == BAC_BOX_POW )
    {
    }
    else if ( Type == BAC_BOX_MIN )
    {
    }
    else if ( Type == BAC_BOX_ABS )
    {
    }

    else if ( Type == BAC_BOX_LTHAN )
    {
    }
    else if ( Type == BAC_BOX_LETHAN )
    {
    }
    else if ( Type == BAC_BOX_METHAN )
    {
    }
    else if ( Type == BAC_BOX_MTHAN )
    {
    }
    else if ( Type == BAC_BOX_EQU )
    {
    }
    else if ( Type == BAC_BOX_NEQU )
    {
    }

    else if ( Type == BAC_BOX_SHIL )
    {
    }
    else if ( Type == BAC_BOX_SHIR )
    {
    }
    else if ( Type == BAC_BOX_ROTL )
    {
    }
    else if ( Type == BAC_BOX_ROTR )
    {
    }
    Bac_BoxReplace( p, iBox, Vec_IntArray(&p->vArray), Vec_IntSize(&p->vArray) );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bac_ObjClpMemory( Bac_Ntk_t * p, int iBox )
{
    int i, En, iNext, nItems = Bac_BoxBiNum(p, iBox);
    assert( Bac_ObjType(p, iBox) == BAC_BOX_RAMBOX );
    assert( Bac_BoxBiNum(p, iBox) == Bac_BoxBoNum(p, iBox) );
    // for each fanin of RAMBOX, make sure address width is the same
    Bac_BoxForEachFaninBox( p, iBox, iNext, i )
        assert( Bac_ObjType(p, iNext) == BAC_BOX_RAMWC );

    // create decoders, selectors and flops
    for ( i = 0; i < nItems; i++ )
    {
        int BoxW = Bac_ObjFanin(p, Bac_BoxBi(p, iBox, i));
        int BoxR = Bac_ObjFanout(p, Bac_BoxBo(p, iBox, 0));
        assert( Bac_ObjType(p, BoxW) == BAC_BOX_RAMWC );
        assert( Bac_ObjType(p, BoxR) == BAC_BOX_RAMR );
        // create enable
        Vec_IntFillTwo( &p->vArray2, 2, Bac_BoxFanin(p, BoxW, 1), Bac_BoxFanin(p, BoxR, 0) );
        En = Bac_BoxCreate( p, BAC_BOX_AND, &p->vArray2, 1, 1, 1 );
        En = Bac_BoxBo( p, En, 0 );
        // collect address
    }
    // for each fanout of RAMBOX, makes ure address width is the same
//    Bac_BoxForEachFanoutBox( p, iBox, iNext, i )
//        assert( Bac_ObjType(p, iNext) == BAC_BOX_RAMR );
    // create selectors and connect them
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

