/**CFile****************************************************************

  FileName    [mpmGates.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Configurable technology mapper.]

  Synopsis    [Standard-cell mapping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 1, 2013.]

  Revision    [$Id: mpmGates.c,v 1.00 2013/06/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mpmInt.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "map/scl/sclSize.h"
#include "map/scl/sclTime.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Finds matches fore each DSD class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Mpm_ManFindDsdMatches( Mpm_Man_t * p, void * pScl )
{
    int fVerbose = p->pPars->fVeryVerbose;
    SC_Lib * pLib = (SC_Lib *)pScl;
    Vec_Wec_t * vClasses;
    Vec_Int_t * vClass;
    SC_Cell * pRepr;
    int i, Config, iClass;
    word Truth;
    vClasses = Vec_WecStart( 600 );
    SC_LibForEachCellClass( pLib, pRepr, i )
    {
        if ( pRepr->n_inputs > 6 || pRepr->n_outputs > 1 )
        {
            if ( fVerbose )
            printf( "Skipping cell %s with %d inputs and %d outputs\n", pRepr->pName, pRepr->n_inputs, pRepr->n_outputs );
            continue;
        }
        Truth = *Vec_WrdArray( &SC_CellPin(pRepr, pRepr->n_inputs)->vFunc );
        Config = Mpm_CutCheckDsd6( p, Truth );
        if ( Config == -1 )
        {
            if ( fVerbose )
            printf( "Skipping cell %s with non-DSD function\n", pRepr->pName );
            continue;
        }
        iClass = Config >> 17;
        Config = (pRepr->Id << 17) | (Config & 0x1FFFF);
        // write gate and NPN config for this DSD class
        vClass = Vec_WecEntry( vClasses, iClass );
        Vec_IntPush( vClass, Config );
        if ( !fVerbose )
            continue;

        printf( "Gate %5d  %-30s : ", pRepr->Id, pRepr->pName );
        printf( "Class %3d  ", iClass );
        printf( "Area %10.3f  ", pRepr->area );
        Extra_PrintBinary( stdout, (unsigned *)&Config, 17 );
        printf( "   " );
        Kit_DsdPrintFromTruth( (unsigned *)&Truth, pRepr->n_inputs ); printf( "\n" );
    }
    return vClasses;
}

/**Function*************************************************************

  Synopsis    [Find mapping of DSD classes into Genlib library cells.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Mpm_ManFindCells( Mio_Library_t * pMio, SC_Lib * pScl, Vec_Wec_t * vNpnConfigs )
{
    Vec_Ptr_t * vNpnGatesMio;
    Vec_Int_t * vClass;
    Mio_Gate_t * pMioGate;
    SC_Cell * pCell;
    int Config, iClass;
    vNpnGatesMio = Vec_PtrStart( Vec_WecSize(vNpnConfigs) );
    Vec_WecForEachLevel( vNpnConfigs, vClass, iClass )
    {
        if ( Vec_IntSize(vClass) == 0 )
            continue;
        Config = Vec_IntEntry(vClass, 0);
        pCell = SC_LibCell( pScl, (Config >> 17) );
        pMioGate = Mio_LibraryReadGateByName( pMio, pCell->pName, NULL );
        if ( pMioGate == NULL )
        {
            Vec_PtrFree( vNpnGatesMio );
            return NULL;
        }
        assert( pMioGate != NULL );
        Vec_PtrWriteEntry( vNpnGatesMio, iClass, pMioGate );
    }
    return vNpnGatesMio;
}

/**Function*************************************************************

  Synopsis    [Derive mapped network as an ABC network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Mpm_ManFindMappedNodes( Mpm_Man_t * p )
{
    Vec_Int_t * vNodes;
    Mig_Obj_t * pObj;
    vNodes = Vec_IntAlloc( 1000 );
    Mig_ManForEachObj( p->pMig, pObj )
        if ( Mig_ObjIsNode(pObj) && Mpm_ObjMapRef(p, pObj) )
            Vec_IntPush( vNodes, Mig_ObjId(pObj) );
    return vNodes;
}
Abc_Obj_t * Mpm_ManGetAbcNode( Abc_Ntk_t * pNtk, Vec_Int_t * vCopy, int iMigLit )
{
    Abc_Obj_t * pObj;
    int iObjId = Vec_IntEntry( vCopy, iMigLit );
    if ( iObjId >= 0 )
        return Abc_NtkObj( pNtk, iObjId );
    iObjId = Vec_IntEntry( vCopy, Abc_LitNot(iMigLit) );
    assert( iObjId >= 0 );
    pObj = Abc_NtkCreateNodeInv( pNtk, Abc_NtkObj(pNtk, iObjId) );
    Vec_IntWriteEntry( vCopy, iMigLit, Abc_ObjId(pObj) );
    return pObj;
}
Abc_Ntk_t * Mpm_ManDeriveMappedAbcNtk( Mpm_Man_t * p, Mio_Library_t * pMio )
{
    Abc_Ntk_t * pNtk;
    Vec_Ptr_t * vNpnGatesMio;
    Vec_Int_t * vNodes, * vCopy, * vClass;
    Abc_Obj_t * pObj, * pFanin;
    Mig_Obj_t * pNode;
    Mpm_Cut_t * pCutBest;
    int i, k, iNode, iMigLit, fCompl, Config;

    // find mapping of SCL cells into MIO cells
    vNpnGatesMio = Mpm_ManFindCells( pMio, (SC_Lib *)p->pPars->pScl, p->vNpnConfigs );
    if ( vNpnGatesMio == NULL )
    {
        printf( "Genlib library does not match SCL library.\n" );
        return NULL;
    }

    // create mapping for each phase of each node
    vCopy = Vec_IntStartFull( 2 * Mig_ManObjNum(p->pMig) );

    // get internal nodes
    vNodes = Mpm_ManFindMappedNodes( p );

    // start the network
    pNtk = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_MAP, 1 );
    pNtk->pName = Extra_UtilStrsav( p->pMig->pName );
    pNtk->pManFunc = pMio;

    // create primary inputs
    Mig_ManForEachCi( p->pMig, pNode, i )
    {
        pObj = Abc_NtkCreatePi(pNtk);
        Vec_IntWriteEntry( vCopy, Abc_Var2Lit( Mig_ObjId(pNode), 0 ), Abc_ObjId(pObj) );
    }
    Abc_NtkAddDummyPiNames( pNtk );

    // create constant nodes
    Mig_ManForEachCo( p->pMig, pNode, i )
        if ( Mig_ObjFaninLit(pNode, 0) == 0 )
        {
            pObj = Abc_NtkCreateNodeConst0(pNtk);
            Vec_IntWriteEntry( vCopy, Abc_Var2Lit( 0, 0 ), Abc_ObjId(pObj) );
            break;
        }
    Mig_ManForEachCo( p->pMig, pNode, i )
        if ( Mig_ObjFaninLit(pNode, 0) == 1 )
        {
            pObj = Abc_NtkCreateNodeConst1(pNtk);
            Vec_IntWriteEntry( vCopy, Abc_Var2Lit( 0, 1 ), Abc_ObjId(pObj) );
            break;
        }

    // create internal nodes
    Vec_IntForEachEntry( vNodes, iNode, i )
    {
        pCutBest = Mpm_ObjCutBestP( p, Mig_ManObj(p->pMig, iNode) );
        vClass = Vec_WecEntry( p->vNpnConfigs, Abc_Lit2Var(pCutBest->iFunc) );
        Config = Vec_IntEntry( vClass, 0 );
        pObj = Abc_NtkCreateNode( pNtk );
        pObj->pData = Vec_PtrEntry( vNpnGatesMio, Abc_Lit2Var(pCutBest->iFunc) );
        assert( pObj->pData != NULL );
        fCompl = pCutBest->fCompl ^ Abc_LitIsCompl(pCutBest->iFunc) ^ ((Config >> 16) & 1);
        Config &= 0xFFFF;
        for ( k = 0; k < (int)pCutBest->nLeaves; k++ )
        {
            assert( (Config >> 6) < 720 );
            iMigLit = pCutBest->pLeaves[ (int)(p->Perm6[Config >> 6][k]) ];
            pFanin = Mpm_ManGetAbcNode( pNtk, vCopy, Abc_LitNotCond(iMigLit, (Config >> k) & 1) );
            Abc_ObjAddFanin( pObj, pFanin );
        }
        Vec_IntWriteEntry( vCopy, Abc_Var2Lit(iNode, fCompl), Abc_ObjId(pObj) );
    }

    // create primary outputs
    Mig_ManForEachCo( p->pMig, pNode, i )
    {
        pObj = Abc_NtkCreatePo(pNtk);
        pFanin = Mpm_ManGetAbcNode( pNtk, vCopy, Mig_ObjFaninLit(pNode, 0) );
        Abc_ObjAddFanin( pObj, pFanin );
    }
    Abc_NtkAddDummyPoNames( pNtk );

    // clean up
    Vec_PtrFree( vNpnGatesMio );
    Vec_IntFree( vNodes );
    Vec_IntFree( vCopy );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Mpm_ManPerformCellMapping( Mig_Man_t * pMig, Mpm_Par_t * pPars, Mio_Library_t * pMio )
{
    Abc_Ntk_t * pNew;
    Mpm_Man_t * p;
    assert( pPars->fMap4Gates );
    p = Mpm_ManStart( pMig, pPars );
    if ( p->pPars->fVerbose ) 
        Mpm_ManPrintStatsInit( p );
    p->vNpnConfigs = Mpm_ManFindDsdMatches( p, p->pPars->pScl );
    Mpm_ManPrepare( p );
    Mpm_ManPerform( p );
    if ( p->pPars->fVerbose ) 
        Mpm_ManPrintStats( p );
    pNew = Mpm_ManDeriveMappedAbcNtk( p, pMio );
    Mpm_ManStop( p );
    return pNew;
}
Abc_Ntk_t * Mpm_ManCellMapping( Gia_Man_t * pGia, Mpm_Par_t * pPars, void * pMio )
{
    Mig_Man_t * p;
    Abc_Ntk_t * pNew;
    assert( pMio != NULL );
    assert( pPars->pLib->LutMax <= MPM_VAR_MAX );
    assert( pPars->nNumCuts <= MPM_CUT_MAX );
    if ( pPars->fUseGates )
    {
        pGia = Gia_ManDupMuxes( pGia, 2 );
        p = Mig_ManCreate( pGia );
        Gia_ManStop( pGia );
    }
    else
        p = Mig_ManCreate( pGia );
    pNew = Mpm_ManPerformCellMapping( p, pPars, (Mio_Library_t *)pMio );
    Mig_ManStop( p );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

