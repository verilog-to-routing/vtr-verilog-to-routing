/**CFile****************************************************************

  FileName    [amapRule.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [Matching rules.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapRule.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern int Amap_LibDeriveGatePerm( Amap_Lib_t * pLib, Amap_Gat_t * pGate, Kit_DsdNtk_t * pNtk, Amap_Nod_t * pNod, char * pArray );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks if the three-argument rule exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Amap_CreateRulesPrime( Amap_Lib_t * p, Vec_Int_t * vNods0, Vec_Int_t * vNods1, Vec_Int_t * vNods2 )
{
    Vec_Int_t * vRes;
    int i, k, j, iNod, iNod0, iNod1, iNod2;
    if ( p->vRules3 == NULL )
        p->vRules3 = Vec_IntAlloc( 100 );
    vRes = Vec_IntAlloc( 10 );
    Vec_IntForEachEntry( vNods0, iNod0, i )
    Vec_IntForEachEntry( vNods1, iNod1, k )
    Vec_IntForEachEntry( vNods2, iNod2, j )
    {
        iNod = Amap_LibFindMux( p, iNod0, iNod1, iNod2 );
        if ( iNod == -1 )
            iNod = Amap_LibCreateMux( p, iNod0, iNod1, iNod2 );
        Vec_IntPush( vRes, Abc_Var2Lit(iNod, 0) );
    }
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_CreateRulesTwo( Amap_Lib_t * p, Vec_Int_t * vNods, Vec_Int_t * vNods0, Vec_Int_t * vNods1, int fXor )
{
    int i, k, iNod, iNod0, iNod1;
    Vec_IntForEachEntry( vNods0, iNod0, i )
    Vec_IntForEachEntry( vNods1, iNod1, k )
    {
        iNod = Amap_LibFindNode( p, iNod0, iNod1, fXor );
        if ( iNod == -1 )
            iNod = Amap_LibCreateNode( p, iNod0, iNod1, fXor );
        Vec_IntPushUnique( vNods, Abc_Var2Lit(iNod, 0) );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_CreateCheckAllZero( Vec_Ptr_t * vVecNods )
{
    Vec_Int_t * vNods;
    int i;
    Vec_PtrForEachEntryReverse( Vec_Int_t *, vVecNods, vNods, i )
        if ( Vec_IntSize(vNods) != 1 || Vec_IntEntry(vNods,0) != 0 )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Amap_CreateRulesVector_rec( Amap_Lib_t * p, Vec_Ptr_t * vVecNods, int fXor )
{
    Vec_Ptr_t * vVecNods0, * vVecNods1;
    Vec_Int_t * vRes, * vNods, * vNods0, * vNods1;
    int i, k;
    if ( Vec_PtrSize(vVecNods) == 1 )
        return Vec_IntDup( (Vec_Int_t *)Vec_PtrEntry(vVecNods, 0) );
    vRes = Vec_IntAlloc( 10 );
    vVecNods0 = Vec_PtrAlloc( Vec_PtrSize(vVecNods) );
    vVecNods1 = Vec_PtrAlloc( Vec_PtrSize(vVecNods) );
    if ( Amap_CreateCheckAllZero(vVecNods) )
    {
        for ( i = Vec_PtrSize(vVecNods)-1; i > 0; i-- )
        {
            Vec_PtrClear( vVecNods0 );
            Vec_PtrClear( vVecNods1 );
            Vec_PtrForEachEntryStop( Vec_Int_t *, vVecNods, vNods, k, i )
                Vec_PtrPush( vVecNods0, vNods );
            Vec_PtrForEachEntryStart( Vec_Int_t *, vVecNods, vNods, k, i )
                Vec_PtrPush( vVecNods1, vNods );
            vNods0 = Amap_CreateRulesVector_rec( p, vVecNods0, fXor );
            vNods1 = Amap_CreateRulesVector_rec( p, vVecNods1, fXor );
            Amap_CreateRulesTwo( p, vRes, vNods0, vNods1, fXor );
            Vec_IntFree( vNods0 );
            Vec_IntFree( vNods1 );
        }
    }
    else
    {
        int Limit = (1 << Vec_PtrSize(vVecNods))-2;
        for ( i = 1; i < Limit; i++ )
        {
            Vec_PtrClear( vVecNods0 );
            Vec_PtrClear( vVecNods1 );
            Vec_PtrForEachEntryReverse( Vec_Int_t *, vVecNods, vNods, k )
            {
                if ( i & (1 << k) )
                    Vec_PtrPush( vVecNods1, vNods );
                else
                    Vec_PtrPush( vVecNods0, vNods );
            }
            assert( Vec_PtrSize(vVecNods0) > 0 );
            assert( Vec_PtrSize(vVecNods1) > 0 );
            assert( Vec_PtrSize(vVecNods0) < Vec_PtrSize(vVecNods) );
            assert( Vec_PtrSize(vVecNods1) < Vec_PtrSize(vVecNods) );
            vNods0 = Amap_CreateRulesVector_rec( p, vVecNods0, fXor );
            vNods1 = Amap_CreateRulesVector_rec( p, vVecNods1, fXor );
            Amap_CreateRulesTwo( p, vRes, vNods0, vNods1, fXor );
            Vec_IntFree( vNods0 );
            Vec_IntFree( vNods1 );
        }
    }
    Vec_PtrFree( vVecNods0 );
    Vec_PtrFree( vVecNods1 );
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Amap_CreateRulesFromDsd_rec( Amap_Lib_t * pLib, Kit_DsdNtk_t * p, int iLit )
{
    Vec_Int_t * vRes = NULL;
    Vec_Ptr_t * vVecNods;
    Vec_Int_t * vNodsFanin;
    Kit_DsdObj_t * pObj;
    unsigned i;
    int iFanin, iNod, k;
    assert( !Abc_LitIsCompl(iLit) );
    pObj = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit) );
    if ( pObj == NULL )
        return Vec_IntStartNatural( 1 );
    // solve for the inputs
    vVecNods = Vec_PtrAlloc( pObj->nFans );
    Kit_DsdObjForEachFanin( p, pObj, iFanin, i )
    {
        vNodsFanin = Amap_CreateRulesFromDsd_rec( pLib, p, Abc_LitRegular(iFanin) );
        if ( Abc_LitIsCompl(iFanin) )
        {
            Vec_IntForEachEntry( vNodsFanin, iNod, k )
                if ( iNod > 0 )
                    Vec_IntWriteEntry( vNodsFanin, k, Abc_LitNot(iNod) );
        }
        Vec_PtrPush( vVecNods, vNodsFanin );
    }
    if ( pObj->Type == KIT_DSD_AND )
        vRes = Amap_CreateRulesVector_rec( pLib, vVecNods, 0 );
    else if ( pObj->Type == KIT_DSD_XOR )
        vRes = Amap_CreateRulesVector_rec( pLib, vVecNods, 1 );
    else if ( pObj->Type == KIT_DSD_PRIME )
    {
        assert( pObj->nFans == 3 );
        assert( Kit_DsdObjTruth(pObj)[0] == 0xCACACACA );
        vRes = Amap_CreateRulesPrime( pLib, (Vec_Int_t *)Vec_PtrEntry(vVecNods, 0),
            (Vec_Int_t *)Vec_PtrEntry(vVecNods, 1), (Vec_Int_t *)Vec_PtrEntry(vVecNods, 2) );
    }
    else assert( 0 );
    Vec_PtrForEachEntry( Vec_Int_t *, vVecNods, vNodsFanin, k )
        Vec_IntFree( vNodsFanin );
    Vec_PtrFree( vVecNods );
    return vRes;
}
Vec_Int_t * Amap_CreateRulesFromDsd( Amap_Lib_t * pLib, Kit_DsdNtk_t * p )
{
    Vec_Int_t * vNods;
    int iNod, i;
    assert( p->nVars >= 2 );
    vNods = Amap_CreateRulesFromDsd_rec( pLib, p, Abc_LitRegular(p->Root) );
    if ( vNods == NULL )
        return NULL;
    if ( Abc_LitIsCompl(p->Root) )
    {
        Vec_IntForEachEntry( vNods, iNod, i )
            Vec_IntWriteEntry( vNods, i, Abc_LitNot(iNod) );
    }
    return vNods;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if DSD network contains asymentry due to complements.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_CreateCheckEqual_rec( Kit_DsdNtk_t * p, int iLit0, int iLit1 )
{
    Kit_DsdObj_t * pObj0, * pObj1; 
    int i;
    assert( !Abc_LitIsCompl(iLit0) );
    assert( !Abc_LitIsCompl(iLit1) );
    pObj0 = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit0) );
    pObj1 = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit1) );
    if ( pObj0 == NULL && pObj1 == NULL )
        return 1;
    if ( pObj0 == NULL || pObj1 == NULL )
        return 0;
    if ( pObj0->Type != pObj1->Type )
        return 0;
    if ( pObj0->nFans != pObj1->nFans )
        return 0;
    if ( pObj0->Type == KIT_DSD_PRIME )
        return 0;
    assert( pObj0->Type == KIT_DSD_AND || pObj0->Type == KIT_DSD_XOR );
    for ( i = 0; i < (int)pObj0->nFans; i++ )
    {
        if ( Abc_LitIsCompl(pObj0->pFans[i]) != Abc_LitIsCompl(pObj1->pFans[i]) )
            return 0;
        if ( !Amap_CreateCheckEqual_rec( p, Abc_LitRegular(pObj0->pFans[i]), Abc_LitRegular(pObj1->pFans[i]) ) )
            return 0;
    }
    return 1;
}
void Amap_CreateCheckAsym_rec( Kit_DsdNtk_t * p, int iLit, Vec_Int_t ** pvSyms )
{
    Kit_DsdObj_t * pObj; 
    int i, k, iFanin;
    assert( !Abc_LitIsCompl(iLit) );
    pObj = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit) );
    if ( pObj == NULL )
        return;
    Kit_DsdObjForEachFanin( p, pObj, iFanin, i )
        Amap_CreateCheckAsym_rec( p, Abc_LitRegular(iFanin), pvSyms );
    if ( pObj->Type == KIT_DSD_PRIME )
        return;
    assert( pObj->Type == KIT_DSD_AND || pObj->Type == KIT_DSD_XOR );
    for ( i = 0; i < (int)pObj->nFans; i++ )
    for ( k = i+1; k < (int)pObj->nFans; k++ )
    {
        if ( Abc_LitIsCompl(pObj->pFans[i]) != Abc_LitIsCompl(pObj->pFans[k]) && 
             Kit_DsdNtkObj(p, Abc_Lit2Var(pObj->pFans[i])) == NULL && 
             Kit_DsdNtkObj(p, Abc_Lit2Var(pObj->pFans[k])) == NULL )
        {
            if ( *pvSyms == NULL )
                *pvSyms = Vec_IntAlloc( 16 );
            Vec_IntPush( *pvSyms, (Abc_Lit2Var(pObj->pFans[i]) << 8) | Abc_Lit2Var(pObj->pFans[k]) );
        }
    }
}
void Amap_CreateCheckAsym( Kit_DsdNtk_t * p, Vec_Int_t ** pvSyms )
{
    Amap_CreateCheckAsym_rec( p, Abc_LitRegular(p->Root), pvSyms );
}

/**Function*************************************************************

  Synopsis    [Creates rules for the given gate]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_CreateRulesForGate( Amap_Lib_t * pLib, Amap_Gat_t * pGate )
{ 
    Kit_DsdNtk_t * pNtk, * pTemp;
    Vec_Int_t * vSyms = NULL;
    Vec_Int_t * vNods;
    Amap_Nod_t * pNod;
    Amap_Set_t * pSet, * pSet2;
    int iNod, i, k, Entry;
//    if ( pGate->nPins > 4 )
//        return;
    pNtk = Kit_DsdDecomposeMux( pGate->pFunc, pGate->nPins, 2 );
    if ( pGate->nPins == 2 && (pGate->pFunc[0] == 0x66666666 || pGate->pFunc[0] == ~0x66666666) )
         pLib->fHasXor = 1;
    if ( Kit_DsdNonDsdSizeMax(pNtk) == 3 )
        pLib->fHasMux = pGate->fMux = 1;
    pNtk = Kit_DsdExpand( pTemp = pNtk );
    Kit_DsdNtkFree( pTemp );
    Kit_DsdVerify( pNtk, pGate->pFunc, pGate->nPins ); 
    // check symmetries
    Amap_CreateCheckAsym( pNtk, &vSyms );
//    if ( vSyms )
//        Kit_DsdPrint( stdout, pNtk ), printf( "\n" );

if ( pLib->fVerbose )
//if ( pGate->fMux )
{
printf( "\nProcessing library gate %4d: %10s ", pGate->Id, pGate->pName );
Kit_DsdPrint( stdout, pNtk );
}
    vNods = Amap_CreateRulesFromDsd( pLib, pNtk );
    if ( vNods )
    {
        Vec_IntForEachEntry( vNods, iNod, i )
        {
            assert( iNod > 1 );
            pNod = Amap_LibNod( pLib, Abc_Lit2Var(iNod) );
//            assert( pNod->Type == AMAP_OBJ_MUX || pNod->nSuppSize == pGate->nPins );
            pSet = (Amap_Set_t *)Aig_MmFlexEntryFetch( pLib->pMemSet, sizeof(Amap_Set_t) );
            memset( pSet, 0, sizeof(Amap_Set_t) );
            pSet->iGate = pGate->Id;
            pSet->fInv  = Abc_LitIsCompl(iNod);
            pSet->nIns  = pGate->nPins;
            if ( Amap_LibDeriveGatePerm( pLib, pGate, pNtk, pNod, pSet->Ins ) == 0 )
            {
if ( pLib->fVerbose )
{
                printf( "Cound not prepare gate \"%s\": ", pGate->pName );
                Kit_DsdPrint( stdout, pNtk );
}
                continue;
            }
            pSet->pNext = pNod->pSets;
            pNod->pSets = pSet;
            pLib->nSets++;
            if ( vSyms == NULL )
                continue;
//            continue;
            // add sets equivalent due to symmetry
            Vec_IntForEachEntry( vSyms, Entry, k )
            {
                int iThis = Entry & 0xff;
                int iThat = Entry >> 8;
//                printf( "%d %d\n", iThis, iThat );
                // create new set
                pSet2 = (Amap_Set_t *)Aig_MmFlexEntryFetch( pLib->pMemSet, sizeof(Amap_Set_t) );
                memset( pSet2, 0, sizeof(Amap_Set_t) );
                pSet2->iGate = pGate->Id;
                pSet2->fInv  = Abc_LitIsCompl(iNod);
                pSet2->nIns  = pGate->nPins;
                memcpy( pSet2->Ins, pSet->Ins, (size_t)pGate->nPins );
                // update inputs
                pSet2->Ins[iThis] = Abc_Var2Lit( Abc_Lit2Var(pSet->Ins[iThat]), Abc_LitIsCompl(pSet->Ins[iThis]) );
                pSet2->Ins[iThat] = Abc_Var2Lit( Abc_Lit2Var(pSet->Ins[iThis]), Abc_LitIsCompl(pSet->Ins[iThat]) );
                // add set to collection
                pSet2->pNext = pNod->pSets;
                pNod->pSets  = pSet2;
                pLib->nSets++;
            }
        }
        Vec_IntFree( vNods );
    }
    Kit_DsdNtkFree( pNtk );
    Vec_IntFreeP( &vSyms );
}

/**Function*************************************************************

  Synopsis    [Creates rules for the given gate]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_LibCreateRules( Amap_Lib_t * pLib, int fVeryVerbose )
{
    Amap_Gat_t * pGate;
    int i, nGates = 0;
//    abctime clk = Abc_Clock();
    pLib->fVerbose = fVeryVerbose;
    pLib->vRules   = Vec_PtrAlloc( 100 );
    pLib->vRulesX  = Vec_PtrAlloc( 100 );
    pLib->vRules3  = Vec_IntAlloc( 100 );
    Amap_LibCreateVar( pLib );
    Vec_PtrForEachEntry( Amap_Gat_t *, pLib->vSelect, pGate, i )
    {
        if ( pGate->nPins < 2 )
            continue;
        if ( pGate->pFunc == NULL )
        {
            printf( "Amap_LibCreateRules(): Skipping gate %s (%s).\n", pGate->pName, pGate->pForm );
            continue;
        }
        Amap_CreateRulesForGate( pLib, pGate );
        nGates++;
    }
    assert( Vec_PtrSize(pLib->vRules)  == 2*pLib->nNodes );
    assert( Vec_PtrSize(pLib->vRulesX) == 2*pLib->nNodes );
    pLib->pRules  = Amap_LibLookupTableAlloc( pLib->vRules, 0 );
    pLib->pRulesX = Amap_LibLookupTableAlloc( pLib->vRulesX, 0 );
    Vec_VecFree( (Vec_Vec_t *)pLib->vRules );  pLib->vRules  = NULL;
    Vec_VecFree( (Vec_Vec_t *)pLib->vRulesX ); pLib->vRulesX = NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

