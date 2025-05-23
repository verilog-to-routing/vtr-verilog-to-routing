/**CFile****************************************************************

  FileName    [amapPerm.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [Deriving permutation for the gate.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapPerm.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collects fanins of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibCollectFanins_rec( Amap_Lib_t * pLib, Amap_Nod_t * pNod, Vec_Int_t * vFanins )
{
    Amap_Nod_t * pFan0, * pFan1;
    if ( pNod->Id == 0 )
    {
        Vec_IntPush( vFanins, 0 );
        return;
    }
    pFan0 = Amap_LibNod( pLib, Abc_Lit2Var(pNod->iFan0) );
    if ( Abc_LitIsCompl(pNod->iFan0) || pFan0->Type != pNod->Type )
        Vec_IntPush( vFanins, pNod->iFan0 );
    else
        Amap_LibCollectFanins_rec( pLib, pFan0, vFanins );
    pFan1 = Amap_LibNod( pLib, Abc_Lit2Var(pNod->iFan1) );
    if ( Abc_LitIsCompl(pNod->iFan1) || pFan1->Type != pNod->Type )
        Vec_IntPush( vFanins, pNod->iFan1 );
    else
        Amap_LibCollectFanins_rec( pLib, pFan1, vFanins );
}

/**Function*************************************************************

  Synopsis    [Collects fanins of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Amap_LibCollectFanins( Amap_Lib_t * pLib, Amap_Nod_t * pNod )
{
    Vec_Int_t * vFanins = Vec_IntAlloc( 10 );
    Amap_LibCollectFanins_rec( pLib, pNod, vFanins );
    return vFanins;
}

/**Function*************************************************************

  Synopsis    [Matches the node with the DSD node.]

  Description [Returns perm if the node can be matched.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Amap_LibDeriveGatePerm_rec( Amap_Lib_t * pLib, Kit_DsdNtk_t * pNtk, int iLit, Amap_Nod_t * pNod )
{
    Vec_Int_t * vPerm, * vPermFanin, * vNodFanin, * vDsdLits;
    Kit_DsdObj_t * pDsdObj, * pDsdFanin;
    Amap_Nod_t * pNodFanin;
    int iDsdFanin, iNodFanin, Value, iDsdLit, i, k, j;
//    assert( !Abc_LitIsCompl(iLit) );
    pDsdObj = Kit_DsdNtkObj( pNtk, Abc_Lit2Var(iLit) );
    if ( pDsdObj == NULL )
    {
        vPerm = Vec_IntAlloc( 1 );
        Vec_IntPush( vPerm, iLit );
        return vPerm;
    }
    if ( pDsdObj->Type == KIT_DSD_PRIME && pNod->Type == AMAP_OBJ_MUX )
    {
        vPerm = Vec_IntAlloc( 10 );

        iDsdFanin  = pDsdObj->pFans[0];
        pNodFanin  = Amap_LibNod( pLib, Abc_Lit2Var(pNod->iFan0) );
        vPermFanin = Amap_LibDeriveGatePerm_rec( pLib, pNtk, iDsdFanin, pNodFanin );
        if ( vPermFanin == NULL )
        {
            Vec_IntFree( vPerm );
            return NULL;
        }
        Vec_IntForEachEntry( vPermFanin, Value, k )
            Vec_IntPush( vPerm, Value );
        Vec_IntFree( vPermFanin );

        iDsdFanin  = pDsdObj->pFans[1];
        pNodFanin  = Amap_LibNod( pLib, Abc_Lit2Var(pNod->iFan1) );
        vPermFanin = Amap_LibDeriveGatePerm_rec( pLib, pNtk, iDsdFanin, pNodFanin );
        if ( vPermFanin == NULL )
        {
            Vec_IntFree( vPerm );
            return NULL;
        }
        Vec_IntForEachEntry( vPermFanin, Value, k )
            Vec_IntPush( vPerm, Value );
        Vec_IntFree( vPermFanin );

        iDsdFanin  = pDsdObj->pFans[2];
        pNodFanin  = Amap_LibNod( pLib, Abc_Lit2Var(pNod->iFan2) );
        vPermFanin = Amap_LibDeriveGatePerm_rec( pLib, pNtk, iDsdFanin, pNodFanin );
        if ( vPermFanin == NULL )
        {
            Vec_IntFree( vPerm );
            return NULL;
        }
        Vec_IntForEachEntry( vPermFanin, Value, k )
            Vec_IntPush( vPerm, Value );
        Vec_IntFree( vPermFanin );

        return vPerm;
    }
    // return if wrong types
    if ( pDsdObj->Type == KIT_DSD_PRIME || pNod->Type == AMAP_OBJ_MUX )
        return NULL;
    // return if sizes do not agree
    vNodFanin = Amap_LibCollectFanins( pLib, pNod );
    if ( Vec_IntSize(vNodFanin) != (int)pDsdObj->nFans )
    {
        Vec_IntFree( vNodFanin );
        return NULL;
    }
    // match fanins of DSD with fanins of nodes
    // clean the mark and save variable literals
    vPerm = Vec_IntAlloc( 10 );
    vDsdLits = Vec_IntAlloc( 10 );
    Kit_DsdObjForEachFaninReverse( pNtk, pDsdObj, iDsdFanin, i )
    {
        pDsdFanin = Kit_DsdNtkObj( pNtk, Abc_Lit2Var(iDsdFanin) );
        if ( pDsdFanin )
            pDsdFanin->fMark = 0;
        else
            Vec_IntPush( vDsdLits, iDsdFanin );
    }
    // match each fanins of the node
    iDsdLit = 0;
    Vec_IntForEachEntry( vNodFanin, iNodFanin, k )
    {
        if ( iNodFanin == 0 )
        {
            if ( iDsdLit >= Vec_IntSize(vDsdLits) )
            {
                Vec_IntFree( vPerm );
                Vec_IntFree( vDsdLits );
                Vec_IntFree( vNodFanin );
                return NULL;
            }
            iDsdFanin = Vec_IntEntry( vDsdLits, iDsdLit++ );
            Vec_IntPush( vPerm, iDsdFanin );
            continue;
        }
        // find a matching component
        pNodFanin = Amap_LibNod( pLib, Abc_Lit2Var(iNodFanin) );
        Kit_DsdObjForEachFaninReverse( pNtk, pDsdObj, iDsdFanin, i )
        {
            pDsdFanin = Kit_DsdNtkObj( pNtk, Abc_Lit2Var(iDsdFanin) );
            if ( pDsdFanin == NULL )
                continue;
            if ( pDsdFanin->fMark == 1 )
                continue;
            if ( !((pDsdFanin->Type == KIT_DSD_AND && pNodFanin->Type == AMAP_OBJ_AND) ||
                   (pDsdFanin->Type == KIT_DSD_XOR && pNodFanin->Type == AMAP_OBJ_XOR) ||
                   (pDsdFanin->Type == KIT_DSD_PRIME && pNodFanin->Type == AMAP_OBJ_MUX)) )
                   continue;
            vPermFanin = Amap_LibDeriveGatePerm_rec( pLib, pNtk, Abc_LitRegular(iDsdFanin), pNodFanin );
            if ( vPermFanin == NULL )
            {
                Vec_IntFree( vNodFanin );
                Vec_IntFree( vDsdLits );
                Vec_IntFree( vPerm );
                return NULL;
            }
            pDsdFanin->fMark = 1;
            Vec_IntForEachEntry( vPermFanin, Value, j )
                Vec_IntPush( vPerm, Value );
            Vec_IntFree( vPermFanin );
            break;
        }
    }
//    assert( iDsdLit == Vec_IntSize(vDsdLits) );
    if ( iDsdLit != Vec_IntSize(vDsdLits) )
        Vec_IntFreeP( &vPerm );
    Vec_IntFree( vNodFanin );
    Vec_IntFree( vDsdLits );
    return vPerm;
}

/**Function*************************************************************

  Synopsis    [Performs verification of one gate and one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Amap_LibVerifyPerm_rec( Amap_Lib_t * pLib, Amap_Nod_t * pNod, 
    Vec_Ptr_t * vTtElems, Vec_Int_t * vTruth, int nWords, int * piInput )
{
    Amap_Nod_t * pFan0, * pFan1;
    unsigned * pTruth0, * pTruth1, * pTruth;
    int i;
    assert( pNod->Type != AMAP_OBJ_MUX );
    if ( pNod->Id == 0 )
        return (unsigned *)Vec_PtrEntry( vTtElems, (*piInput)++ );
    pFan0 = Amap_LibNod( pLib, Abc_Lit2Var(pNod->iFan0) );
    pTruth0 = Amap_LibVerifyPerm_rec( pLib, pFan0, vTtElems, vTruth, nWords, piInput );
    pFan1 = Amap_LibNod( pLib, Abc_Lit2Var(pNod->iFan1) );
    pTruth1 = Amap_LibVerifyPerm_rec( pLib, pFan1, vTtElems, vTruth, nWords, piInput );
    pTruth  = Vec_IntFetch( vTruth, nWords );
    if ( pNod->Type == AMAP_OBJ_XOR )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] ^ pTruth1[i];
    else if ( !Abc_LitIsCompl(pNod->iFan0) && !Abc_LitIsCompl(pNod->iFan1) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & pTruth1[i];
    else if ( !Abc_LitIsCompl(pNod->iFan0) && Abc_LitIsCompl(pNod->iFan1) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = pTruth0[i] & ~pTruth1[i];
    else if ( Abc_LitIsCompl(pNod->iFan0) && !Abc_LitIsCompl(pNod->iFan1) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & pTruth1[i];
    else // if ( Abc_LitIsCompl(pNod->iFan0) && Hop_ObjFaninC1(pObj) )
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ~pTruth0[i] & ~pTruth1[i];
    return pTruth;
}

/**Function*************************************************************

  Synopsis    [Performs verification of one gate and one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibVerifyPerm( Amap_Lib_t * pLib, Amap_Gat_t * pGate, Kit_DsdNtk_t * pNtk, Amap_Nod_t * pNod, int * pArray )
{
    Vec_Ptr_t * vTtElems;
    Vec_Ptr_t * vTtElemsPol;
    Vec_Int_t * vTruth;
    unsigned * pTruth;
    int i, nWords;
    int iInput = 0;

    // allocate storage for truth tables
    assert( pGate->nPins > 1 );
    nWords = Kit_TruthWordNum( pGate->nPins );
    vTruth = Vec_IntAlloc( nWords * AMAP_MAXINS );
    vTtElems = Vec_PtrAllocTruthTables( pGate->nPins );
    vTtElemsPol = Vec_PtrAlloc( pGate->nPins );
    for ( i = 0; i < (int)pGate->nPins; i++ )
    {
        pTruth = (unsigned *)Vec_PtrEntry( vTtElems, Abc_Lit2Var(pArray[i]) );
        if ( Abc_LitIsCompl( pArray[i] ) )
            Kit_TruthNot( pTruth, pTruth, pGate->nPins );
        Vec_PtrPush( vTtElemsPol, pTruth );
    }
//Extra_PrintBinary( stdout, Vec_PtrEntry(vTtElemsPol, 0), 4 ); printf("\n" );
//Extra_PrintBinary( stdout, Vec_PtrEntry(vTtElemsPol, 1), 4 ); printf("\n" );
    // compute the truth table recursively
    pTruth = Amap_LibVerifyPerm_rec( pLib, pNod, vTtElemsPol, vTruth, nWords, &iInput );
    assert( iInput == (int)pGate->nPins );
    if ( Abc_LitIsCompl(pNtk->Root) )
        Kit_TruthNot( pTruth, pTruth, pGate->nPins );
//Extra_PrintBinary( stdout, pTruth, 4 ); printf("\n" );
//Extra_PrintBinary( stdout, pGate->pFunc, 4 ); printf("\n" );
    // compare
    if ( !Kit_TruthIsEqual(pGate->pFunc, pTruth, pGate->nPins) )
        printf( "Verification failed for gate %d (%s) and node %d.\n",
            pGate->Id, pGate->pForm, pNod->Id );
    Vec_IntFree( vTruth );
    Vec_PtrFree( vTtElems );
    Vec_PtrFree( vTtElemsPol );
}

/**Function*************************************************************

  Synopsis    [Matches the node with the DSD of a gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibDeriveGatePerm( Amap_Lib_t * pLib, Amap_Gat_t * pGate, Kit_DsdNtk_t * pNtk, Amap_Nod_t * pNod, char * pArray )
{
    int fVerbose = 0;
    Vec_Int_t * vPerm;
    int Entry, Entry2, i, k;
//    Kit_DsdPrint( stdout, pNtk );

    vPerm = Amap_LibDeriveGatePerm_rec( pLib, pNtk, Abc_LitRegular(pNtk->Root), pNod );
    if ( vPerm == NULL )
        return 0;
    // check that the permutation is valid
    assert( Vec_IntSize(vPerm) == (int)pNod->nSuppSize );
    Vec_IntForEachEntry( vPerm, Entry, i )
        Vec_IntForEachEntryStart( vPerm, Entry2, k, i+1 )
            if ( Abc_Lit2Var(Entry) == Abc_Lit2Var(Entry2) )
            {
                Vec_IntFree( vPerm );
                return 0;
            }

    // reverse the permutation
    Vec_IntForEachEntry( vPerm, Entry, i )
    {
        assert( Entry < 2 * (int)pNod->nSuppSize );
        pArray[Abc_Lit2Var(Entry)] = Abc_Var2Lit( i, Abc_LitIsCompl(Entry) );
//        pArray[i] = Entry;
//printf( "%d=%d%c ", Abc_Lit2Var(Entry), i, Abc_LitIsCompl(Entry)?'-':'+' );
    }
//printf( "\n" );
//    if ( Kit_DsdNonDsdSizeMax(pNtk) < 3 )
//        Amap_LibVerifyPerm( pLib, pGate, pNtk, pNod, Vec_IntArray(vPerm) );
    Vec_IntFree( vPerm );
    // print the result
    if ( fVerbose )
    {
    printf( "node %4d : ", pNod->Id );
    for ( i = 0; i < (int)pNod->nSuppSize; i++ )
        printf( "%d=%d%c ", i, Abc_Lit2Var(pArray[i]), Abc_LitIsCompl(pArray[i])?'-':'+' );
    printf( "\n" );
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

