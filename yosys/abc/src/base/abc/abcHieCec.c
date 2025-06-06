/**CFile****************************************************************

  FileName    [abcHieCec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Hierarchical CEC manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcHieCec.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "base/io/ioAbc.h"
#include "aig/gia/gia.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define Abc_ObjForEachFaninReal( pObj, pFanin, i )          \
    for ( i = 0; (i < Abc_ObjFaninNum(pObj)) && (((pFanin) = Abc_ObjFaninReal(pObj, i)), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the real faniin of the object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Abc_Obj_t * Abc_ObjFaninReal( Abc_Obj_t * pObj, int i )    
{
    Abc_Obj_t * pRes;
    if ( Abc_ObjIsBox(pObj) )
        pRes = Abc_ObjFanin0( Abc_ObjFanin0( Abc_ObjFanin(pObj, i) ) );
    else
    {
        assert( Abc_ObjIsPo(pObj) || Abc_ObjIsNode(pObj) );
        pRes = Abc_ObjFanin0( Abc_ObjFanin(pObj, i) );
    }
    if ( Abc_ObjIsBo(pRes) )
        return Abc_ObjFanin0(pRes);
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Performs DFS for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDfsBoxes_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanin;
    int i;
    if ( Abc_ObjIsPi(pNode) )
        return;
    assert( Abc_ObjIsNode(pNode) || Abc_ObjIsBox(pNode) );
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    Abc_NodeSetTravIdCurrent( pNode );
    // visit the transitive fanin of the node
    Abc_ObjForEachFaninReal( pNode, pFanin, i )
        Abc_NtkDfsBoxes_rec( pFanin, vNodes );
    // add the node after the fanins have been added
    Vec_PtrPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Returns the array of node and boxes reachable from POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkDfsBoxes( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkIsNetlist(pNtk) );
    // set the traversal ID
    Abc_NtkIncrementTravId( pNtk );
    // start the array of nodes
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_NtkDfsBoxes_rec( Abc_ObjFaninReal(pObj, 0), vNodes );
    return vNodes;
}


/**Function*************************************************************

  Synopsis    [Strashes one logic node using its SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDeriveFlatGiaSop( Gia_Man_t * pGia, int * gFanins, char * pSop )
{
    char * pCube;
    int gAnd, gSum;
    int i, Value, nFanins;
    // get the number of variables
    nFanins = Abc_SopGetVarNum(pSop);
    if ( Abc_SopIsExorType(pSop) )
    {
        gSum = 0; 
        for ( i = 0; i < nFanins; i++ )
            gSum = Gia_ManHashXor( pGia, gSum, gFanins[i] );
    }
    else
    {
        // go through the cubes of the node's SOP
        gSum = 0; 
        Abc_SopForEachCube( pSop, nFanins, pCube )
        {
            // create the AND of literals
            gAnd = 1;
            Abc_CubeForEachVar( pCube, Value, i )
            {
                if ( Value == '1' )
                    gAnd = Gia_ManHashAnd( pGia, gAnd, gFanins[i] );
                else if ( Value == '0' )
                    gAnd = Gia_ManHashAnd( pGia, gAnd, Abc_LitNot(gFanins[i]) );
            }
            // add to the sum of cubes
            gSum = Gia_ManHashAnd( pGia, Abc_LitNot(gSum), Abc_LitNot(gAnd) );
            gSum = Abc_LitNot( gSum );
        }
    }
    // decide whether to complement the result
    if ( Abc_SopIsComplement(pSop) )
        gSum = Abc_LitNot(gSum);
    return gSum;
}

/**Function*************************************************************

  Synopsis    [Flattens the logic hierarchy of the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDeriveFlatGia_rec( Gia_Man_t * pGia, Abc_Ntk_t * pNtk )
{
    int gFanins[16];
    Vec_Ptr_t * vOrder = (Vec_Ptr_t *)pNtk->pData;
    Abc_Obj_t * pObj, * pTerm;
    Abc_Ntk_t * pNtkModel;
    int i, k;
    Abc_NtkForEachPi( pNtk, pTerm, i )
        assert( Abc_ObjFanout0(pTerm)->iTemp >= 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrder, pObj, i )
    {
        if ( Abc_ObjIsNode(pObj) )
        {
            char * pSop = (char *)pObj->pData;
/*
            int nLength = strlen(pSop);
            if ( nLength == 4 ) // buf/inv
            {
                assert( pSop[2] == '1' );
                assert( pSop[0] == '0' || pSop[0] == '1' );
                assert( Abc_ObjFanin0(pObj)->iTemp >= 0 );
                Abc_ObjFanout0(pObj)->iTemp = Abc_LitNotCond( Abc_ObjFanin0(pObj)->iTemp, pSop[0]=='0' );
                continue;
            }
            if ( nLength == 5 ) // and2
            {
                assert( pSop[3] == '1' );
                assert( pSop[0] == '0' || pSop[0] == '1' );
                assert( pSop[1] == '0' || pSop[1] == '1' );
                assert( Abc_ObjFanin0(pObj)->iTemp >= 0 );
                assert( Abc_ObjFanin1(pObj)->iTemp >= 0 );
                Abc_ObjFanout0(pObj)->iTemp = Gia_ManHashAnd( pGia, 
                    Abc_LitNotCond( Abc_ObjFanin0(pObj)->iTemp, pSop[0]=='0' ),
                    Abc_LitNotCond( Abc_ObjFanin1(pObj)->iTemp, pSop[1]=='0' )
                    );
                continue;
            }
*/
            assert( Abc_ObjFaninNum(pObj) <= 16 );
            assert( Abc_ObjFaninNum(pObj) == Abc_SopGetVarNum(pSop) );
            Abc_ObjForEachFanin( pObj, pTerm, k )
            {
                gFanins[k] = pTerm->iTemp;
                assert( gFanins[k] >= 0 );
            }
            Abc_ObjFanout0(pObj)->iTemp = Abc_NtkDeriveFlatGiaSop( pGia, gFanins, pSop );
            continue;
        }
        assert( Abc_ObjIsBox(pObj) );
        pNtkModel = (Abc_Ntk_t *)pObj->pData;
        Abc_NtkFillTemp( pNtkModel );
        // check the match between the number of actual and formal parameters
        assert( Abc_ObjFaninNum(pObj) == Abc_NtkPiNum(pNtkModel) );
        assert( Abc_ObjFanoutNum(pObj) == Abc_NtkPoNum(pNtkModel) );
        // assign PIs
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Abc_ObjFanout0( Abc_NtkPi(pNtkModel, k) )->iTemp = Abc_ObjFanin0(pTerm)->iTemp;
        // call recursively
        Abc_NtkDeriveFlatGia_rec( pGia, pNtkModel );
        // assign POs
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Abc_ObjFanout0(pTerm)->iTemp = Abc_ObjFanin0( Abc_NtkPo(pNtkModel, k) )->iTemp;
    }
    Abc_NtkForEachPo( pNtk, pTerm, i )
        assert( Abc_ObjFanin0(pTerm)->iTemp >= 0 );
}

/**Function*************************************************************

  Synopsis    [Flattens the logic hierarchy of the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Abc_NtkDeriveFlatGia( Abc_Ntk_t * pNtk )
{
    Gia_Man_t * pTemp, * pGia = NULL;
    Abc_Obj_t * pTerm;
    int i;

    assert( Abc_NtkIsNetlist(pNtk) );
    assert( !Abc_NtkLatchNum(pNtk) );
    Abc_NtkFillTemp( pNtk );
    // start the network
    pGia = Gia_ManStart( (1<<16) );
    pGia->pName = Abc_UtilStrsav( Abc_NtkName(pNtk) );
    pGia->pSpec = Abc_UtilStrsav( Abc_NtkSpec(pNtk) );
    Gia_ManHashAlloc( pGia );
    // create PIs
    Abc_NtkForEachPi( pNtk, pTerm, i )
        Abc_ObjFanout0(pTerm)->iTemp = Gia_ManAppendCi( pGia );
    // recursively flatten hierarchy
    Abc_NtkDeriveFlatGia_rec( pGia, pNtk );
    // create POs
    Abc_NtkForEachPo( pNtk, pTerm, i )
        Gia_ManAppendCo( pGia, Abc_ObjFanin0(pTerm)->iTemp );
    // prepare return value
    Gia_ManHashStop( pGia );
    Gia_ManSetRegNum( pGia, 0 );
    pGia = Gia_ManCleanup( pTemp = pGia );
    Gia_ManStop( pTemp );
    return pGia;
}

/**Function*************************************************************

  Synopsis    [Counts the total number of AIG nodes before flattening.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCountAndNodes( Vec_Ptr_t * vOrder )
{
    Gia_Man_t * pGiaBox;
    Abc_Ntk_t * pNtkModel;
    Abc_Obj_t * pObj;
    int i, Counter = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrder, pObj, i )
    {
        if ( Abc_ObjIsNode(pObj) )
        {
            Counter++;
            continue;
        }
        assert( Abc_ObjIsBox(pObj) );
        pNtkModel = (Abc_Ntk_t *)pObj->pData;
        pGiaBox   = (Gia_Man_t *)pNtkModel->pData;
        Counter  += Gia_ManAndNum(pGiaBox);
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Flattens the logic hierarchy of the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Abc_NtkDeriveFlatGia2Derive( Abc_Ntk_t * pNtk, Vec_Ptr_t * vOrder )
{
    int gFanins[16];
    Abc_Ntk_t * pNtkModel;
    Gia_Man_t * pGiaBox, * pGia = NULL;
    Gia_Obj_t * pGiaObj;
    Abc_Obj_t * pTerm, * pObj;
    int i, k;

    assert( Abc_NtkIsNetlist(pNtk) );
    assert( !Abc_NtkLatchNum(pNtk) );
    Abc_NtkFillTemp( pNtk );

    // start the network
    pGia = Gia_ManStart( (1<<15) );
    pGia->pName = Abc_UtilStrsav( Abc_NtkName(pNtk) );
    pGia->pSpec = Abc_UtilStrsav( Abc_NtkSpec(pNtk) );
    Gia_ManHashAlloc( pGia );
    // create PIs
    Abc_NtkForEachPi( pNtk, pTerm, i )
        Abc_ObjFanout0(pTerm)->iTemp = Gia_ManAppendCi( pGia );
    // recursively flatten hierarchy
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrder, pObj, i )
    {
        if ( Abc_ObjIsNode(pObj) )
        {
            char * pSop = (char *)pObj->pData;
            assert( Abc_ObjFaninNum(pObj) <= 16 );
            assert( Abc_ObjFaninNum(pObj) == Abc_SopGetVarNum(pSop) );
            Abc_ObjForEachFanin( pObj, pTerm, k )
            {
                gFanins[k] = pTerm->iTemp;
                assert( gFanins[k] >= 0 );
            }
            Abc_ObjFanout0(pObj)->iTemp = Abc_NtkDeriveFlatGiaSop( pGia, gFanins, pSop );
            continue;
        }
        assert( Abc_ObjIsBox(pObj) );
        pNtkModel = (Abc_Ntk_t *)pObj->pData;
        // check the match between the number of actual and formal parameters
        assert( Abc_ObjFaninNum(pObj) == Abc_NtkPiNum(pNtkModel) );
        assert( Abc_ObjFanoutNum(pObj) == Abc_NtkPoNum(pNtkModel) );
/*
        // assign PIs
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Abc_ObjFanout0( Abc_NtkPi(pNtkModel, k) )->iTemp = Abc_ObjFanin0(pTerm)->iTemp;
        // call recursively
        Abc_NtkDeriveFlatGia_rec( pGia, pNtkModel );
        // assign POs
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Abc_ObjFanout0(pTerm)->iTemp = Abc_ObjFanin0( Abc_NtkPo(pNtkModel, k) )->iTemp;
*/ 
        // duplicate the AIG
        pGiaBox = (Gia_Man_t *)pNtkModel->pData;
        assert( Abc_ObjFaninNum(pObj) == Gia_ManPiNum(pGiaBox) );
        assert( Abc_ObjFanoutNum(pObj) == Gia_ManPoNum(pGiaBox) );
        Gia_ManFillValue( pGiaBox );
        Gia_ManConst0(pGiaBox)->Value = 0;
        Abc_ObjForEachFanin( pObj, pTerm, k )
            Gia_ManPi(pGiaBox, k)->Value = Abc_ObjFanin0(pTerm)->iTemp;
        Gia_ManForEachAnd( pGiaBox, pGiaObj, k )
            pGiaObj->Value = Gia_ManHashAnd( pGia, Gia_ObjFanin0Copy(pGiaObj), Gia_ObjFanin1Copy(pGiaObj) );
        Abc_ObjForEachFanout( pObj, pTerm, k )
            Abc_ObjFanout0(pTerm)->iTemp = Gia_ObjFanin0Copy(Gia_ManPo(pGiaBox, k));
    }
    // create POs
    Abc_NtkForEachPo( pNtk, pTerm, i )
        Gia_ManAppendCo( pGia, Abc_ObjFanin0(pTerm)->iTemp );
    // prepare return value
    Gia_ManHashStop( pGia );
    Gia_ManSetRegNum( pGia, 0 );
    pGia = Gia_ManCleanup( pGiaBox = pGia );
    Gia_ManStop( pGiaBox );

    printf( "%8d -> ", Abc_NtkCountAndNodes(vOrder) );
    Gia_ManPrintStats( pGia, NULL );
    return pGia;
}
/*
void Abc_NtkDeriveFlatGia2_rec( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vOrder;
    Abc_Obj_t * pObj;
    int i;
    if ( pNtk->pData != NULL )
        return;
    vOrder = Abc_NtkDfsBoxes( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrder, pObj, i )
        if ( Abc_ObjIsBox(pObj) )
            Abc_NtkDeriveFlatGia2_rec( (Abc_Ntk_t *)pObj->pData );
    pNtk->pData = Abc_NtkDeriveFlatGia2Derive( pNtk, vOrder );
    Vec_PtrFree( vOrder );
}

Gia_Man_t * Abc_NtkDeriveFlatGia2( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vMods;
    Abc_Ntk_t * pModel;
    Gia_Man_t * pGia = NULL;
    int i;

    assert( Abc_NtkIsNetlist(pNtk) );
    assert( !Abc_NtkLatchNum(pNtk) );

    vMods = pNtk->pDesign->vModules;
    Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
        pModel->pData = NULL;

    Abc_NtkDeriveFlatGia2_rec( pNtk );
    pGia = pNtk->pData;  pNtk->pData = NULL;
    
    Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
        Gia_ManStopP( (Gia_Man_t **)&pModel->pData );

    return pGia;
}
*/
Gia_Man_t * Abc_NtkDeriveFlatGia2( Abc_Ntk_t * pNtk, Vec_Ptr_t * vModels )
{ 
    Vec_Ptr_t * vOrder;
    Abc_Ntk_t * pModel = NULL;
    Gia_Man_t * pGia = NULL;
    int i;

    Vec_PtrForEachEntry( Abc_Ntk_t *, vModels, pModel, i )
    {
        vOrder = Abc_NtkDfsBoxes( pModel );
        pModel->pData = Abc_NtkDeriveFlatGia2Derive( pModel, vOrder );
        Vec_PtrFree( vOrder );
    }

    pGia = (Gia_Man_t *)pModel->pData;  pModel->pData = NULL;

    Vec_PtrForEachEntry( Abc_Ntk_t *, vModels, pModel, i )
        Gia_ManStopP( (Gia_Man_t **)&pModel->pData );

    return pGia;
}


/**Function*************************************************************

  Synopsis    [Collect models in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCollectHie_rec( Abc_Ntk_t * pNtk, Vec_Ptr_t * vModels )
{
    Vec_Ptr_t * vOrder;
    Abc_Obj_t * pObj;
    int i;
    if ( pNtk->iStep >= 0 )
        return;
    vOrder = Abc_NtkDfsBoxes( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrder, pObj, i )
        if ( Abc_ObjIsBox(pObj) && (Abc_Ntk_t *)pObj->pData != pNtk )
            Abc_NtkCollectHie_rec( (Abc_Ntk_t *)pObj->pData, vModels );
    Vec_PtrFree( vOrder );
    pNtk->iStep = Vec_PtrSize(vModels);
    Vec_PtrPush( vModels, pNtk );
}

Vec_Ptr_t * Abc_NtkCollectHie( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vMods, * vResult;
    Abc_Ntk_t * pModel;
    int i;

    assert( Abc_NtkIsNetlist(pNtk) );
    assert( !Abc_NtkLatchNum(pNtk) );

    vResult = Vec_PtrAlloc( 1000 );
    if ( pNtk->pDesign == NULL )
    {
        Vec_PtrPush( vResult, pNtk );
        return vResult;
    }

    vMods = pNtk->pDesign->vModules;
    Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
        pModel->iStep = -1;

    Abc_NtkCollectHie_rec( pNtk, vResult );
    return vResult;    
}

/**Function*************************************************************

  Synopsis    [Counts the number of intstances.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCountInst_rec( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vOrder;
    Abc_Obj_t * pObj;
    int i, Counter = 0;
    if ( pNtk->iStep >= 0 )
        return pNtk->iStep;
    vOrder = Abc_NtkDfsBoxes( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrder, pObj, i )
        if ( Abc_ObjIsBox(pObj) && (Abc_Ntk_t *)pObj->pData != pNtk )
            Counter += Abc_NtkCountInst_rec( (Abc_Ntk_t *)pObj->pData );
    Vec_PtrFree( vOrder );
    return pNtk->iStep = 1 + Counter;
}

void Abc_NtkCountInst( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vMods;
    Abc_Ntk_t * pModel;
    int i, Counter;

    if ( pNtk->pDesign == NULL )
        Counter = Abc_NtkNodeNum(pNtk);
    else
    {
        vMods = pNtk->pDesign->vModules;
        Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
            pModel->iStep = -1;
        Counter = Abc_NtkCountInst_rec( pNtk );
    }
    printf( "Instances = %10d.\n", Counter );
}

/**Function*************************************************************

  Synopsis    [Counts the number of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Abc_NtkCountNodes_rec( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vOrder;
    Abc_Obj_t * pObj;
    double Counter = 0;
    int i;
    if ( pNtk->dTemp >= 0 )
        return pNtk->dTemp;
    vOrder = Abc_NtkDfsBoxes( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vOrder, pObj, i )
        if ( Abc_ObjIsNode(pObj) )
            Counter++;
        else if ( Abc_ObjIsBox(pObj) && (Abc_Ntk_t *)pObj->pData != pNtk )
            Counter += Abc_NtkCountNodes_rec( (Abc_Ntk_t *)pObj->pData );
    Vec_PtrFree( vOrder );
    return pNtk->dTemp = Counter;
}

void Abc_NtkCountNodes( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vMods;
    Abc_Ntk_t * pModel;
    double Counter;
    int i;

    if ( pNtk->pDesign == NULL )
        Counter = Abc_NtkNodeNum(pNtk);
    else
    {
        vMods = pNtk->pDesign->vModules;
        Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
            pModel->dTemp = -1;
        Counter = Abc_NtkCountNodes_rec( pNtk );
    }
    printf( "Nodes = %.0f\n", Counter );
}

/**Function*************************************************************

  Synopsis    [Checks if there is a recursive definition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckRecursive( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vMods;
    Abc_Ntk_t * pModel;
    Abc_Obj_t * pObj;
    int i, k, RetValue = 0;

    assert( Abc_NtkIsNetlist(pNtk) );
    assert( !Abc_NtkLatchNum(pNtk) );

    if ( pNtk->pDesign == NULL )
        return RetValue;

    vMods = pNtk->pDesign->vModules;
    Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
    {
        Abc_NtkForEachObj( pModel, pObj, k )
            if ( Abc_ObjIsBox(pObj) && pObj->pData == (void *)pModel )
            {
                printf( "WARNING: Model \"%s\" contains a recursive definition.\n", Abc_NtkName(pModel) );
                RetValue = 1;
                break;
            }
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Performs hierarchical equivalence checking.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Abc_NtkHieCecTest( char * pFileName, int fVerbose )
{
    int fUseTest = 1;
    int fUseNew = 0;
    int fCheck = 1;
    Vec_Ptr_t * vMods, * vOrder;
    Abc_Ntk_t * pNtk, * pModel;
    Gia_Man_t * pGia;
    int i;
    abctime clk = Abc_Clock();

    // read hierarchical netlist
    pNtk = Io_ReadBlifMv( pFileName, 0, fCheck );
    if ( pNtk == NULL )
    {
        printf( "Reading BLIF file has failed.\n" );
        return NULL;
    }
    if ( pNtk->pDesign == NULL || pNtk->pDesign->vModules == NULL )
    {
        printf( "There is no hierarchy information.\n" );
//        Abc_NtkDelete( pNtk );
//        return NULL;
    }
    Abc_PrintTime( 1, "Reading file", Abc_Clock() - clk );

    assert( Abc_NtkIsNetlist(pNtk) );
    assert( !Abc_NtkLatchNum(pNtk) );
/*
    if ( pNtk->pDesign != NULL )
    {
        clk = Abc_Clock();
        Abc_NtkCountNodes( pNtk );
        Abc_PrintTime( 1, "Count nodes", Abc_Clock() - clk );
    }
*/
    // print stats
    if ( fVerbose )
        Abc_NtkPrintBoxInfo( pNtk );

    // test the new data-structure
    if ( fUseTest )
    {
        extern Gia_Man_t * Au_ManDeriveTest( Abc_Ntk_t * pRoot );
        Gia_Man_t * pGia;
        pGia = Au_ManDeriveTest( pNtk );
        Abc_NtkDelete( pNtk );
        return pGia;
    }

    if ( Abc_NtkCheckRecursive(pNtk) )
        return NULL;

    if ( fUseNew )
    {
        clk = Abc_Clock();
        vOrder = Abc_NtkCollectHie( pNtk );
        Abc_PrintTime( 1, "Collect DFS ", Abc_Clock() - clk );

        // derive GIA
        clk = Abc_Clock();
        pGia = Abc_NtkDeriveFlatGia2( pNtk, vOrder );
        Abc_PrintTime( 1, "Deriving GIA", Abc_Clock() - clk );
        Gia_ManPrintStats( pGia, NULL );
    //    Gia_ManStop( pGia );
 
        Vec_PtrFree( vOrder );
    }
    else
    {
        // order nodes/boxes of all models
        vMods = pNtk->pDesign->vModules;
        Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
            pModel->pData = Abc_NtkDfsBoxes( pModel );

        // derive GIA
        clk = Abc_Clock();
        pGia = Abc_NtkDeriveFlatGia( pNtk );
        Abc_PrintTime( 1, "Deriving GIA", Abc_Clock() - clk );
        Gia_ManPrintStats( pGia, NULL );

        // clean nodes/boxes of all nodes
        Vec_PtrForEachEntry( Abc_Ntk_t *, vMods, pModel, i )
            Vec_PtrFree( (Vec_Ptr_t *)pModel->pData );
    }

    clk = Abc_Clock();
    Abc_NtkCountInst( pNtk );
    Abc_PrintTime( 1, "Gather stats", Abc_Clock() - clk );

    Abc_NtkDelete( pNtk );
    return pGia;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

