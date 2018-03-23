/**CFile****************************************************************

  FileName    [abcMffc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Computing multi-output maximum fanout-free cones.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcMffc.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

ABC_NAMESPACE_IMPL_START
 
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Dereferences and collects the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MffcDeref_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pFanin;
    int i;
    if ( Abc_ObjIsCi(pNode) )
        return;
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        assert( pFanin->vFanouts.nSize > 0 );
        if ( --pFanin->vFanouts.nSize == 0 )
            Abc_MffcDeref_rec( pFanin, vNodes );
    }
    if ( vNodes )
        Vec_PtrPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [References the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MffcRef_rec( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    int i;
    if ( Abc_ObjIsCi(pNode) )
        return;
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( pFanin->vFanouts.nSize++ == 0 )
            Abc_MffcRef_rec( pFanin );
    }
}

/**Function*************************************************************

  Synopsis    [Collects nodes belonging to the MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MffcCollectNodes( Abc_Obj_t ** pNodes, int nNodes, Vec_Ptr_t * vNodes )
{
    int i;
    Vec_PtrClear( vNodes );
    for ( i = 0; i < nNodes; i++ )
        Abc_MffcDeref_rec( pNodes[i], vNodes );
    for ( i = 0; i < nNodes; i++ )
        Abc_MffcRef_rec( pNodes[i] );
}

/**Function*************************************************************

  Synopsis    [Collects leaves of the MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_MffcCollectLeaves( Vec_Ptr_t * vNodes, Vec_Ptr_t * vLeaves )
{
    Abc_Obj_t * pNode, * pFanin;
    int i, k;
    assert( Vec_PtrSize(vNodes) > 0 );
    pNode = (Abc_Obj_t *)Vec_PtrEntry( vNodes, 0 );
    // label them
    Abc_NtkIncrementTravId( pNode->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
        Abc_NodeSetTravIdCurrent( pNode );
    // collect non-labeled fanins
    Vec_PtrClear( vLeaves );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    Abc_ObjForEachFanin( pNode, pFanin, k )
    {
        if ( Abc_NodeIsTravIdCurrent(pFanin) )
            continue;
        Abc_NodeSetTravIdCurrent( pFanin );
        Vec_PtrPush( vLeaves, pFanin );
    }
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes that are roots of MFFCs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NktMffcMarkRoots( Abc_Ntk_t * pNtk, int fSkipPis )
{
    Vec_Ptr_t * vRoots, * vNodes, * vLeaves;
    Abc_Obj_t * pObj, * pLeaf;
    int i, k;
    Abc_NtkCleanMarkA( pNtk );
    // mark the drivers of combinational outputs
    vRoots  = Vec_PtrAlloc( 1000 );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pObj = Abc_ObjFanin0( pObj );
//        if ( Abc_ObjIsCi(pObj) || Abc_ObjFaninNum(pObj) == 0 || pObj->fMarkA )
        if ( Abc_ObjIsCi(pObj) || pObj->fMarkA )
            continue;
        pObj->fMarkA = 1;
        Vec_PtrPush( vRoots, pObj );
    }
    // explore starting from the drivers
    vNodes  = Vec_PtrAlloc( 100 );
    vLeaves = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        if ( Abc_ObjIsCi(pObj) )
            continue;
        // collect internal nodes 
        Abc_MffcCollectNodes( &pObj, 1, vNodes );
        // collect leaves
        Abc_MffcCollectLeaves( vNodes, vLeaves );
        // add non-PI leaves
        Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pLeaf, k )
        {
            if ( (fSkipPis && Abc_ObjIsCi(pLeaf)) || pLeaf->fMarkA )
                continue;
            pLeaf->fMarkA = 1;
            Vec_PtrPush( vRoots, pLeaf );
        }
    }
    Vec_PtrFree( vLeaves );
    Vec_PtrFree( vNodes );
    Abc_NtkCleanMarkA( pNtk );
    return vRoots;
}

/**Function*************************************************************

  Synopsis    [Collect fanout reachable root nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffcCollectFanout_rec( Abc_Obj_t * pObj, Vec_Ptr_t * vFanouts )
{
    Abc_Obj_t * pFanout;
    int i;
    if ( Abc_ObjIsCo(pObj) )
        return;
    if ( Abc_ObjFanoutNum(pObj) > 64 )
        return;
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return;
    Abc_NodeSetTravIdCurrent(pObj);
    if ( pObj->fMarkA )
    {
        if ( pObj->vFanouts.nSize > 0 )
            Vec_PtrPush( vFanouts, pObj );
        return;
    }
    Abc_ObjForEachFanout( pObj, pFanout, i )
        Abc_NktMffcCollectFanout_rec( pFanout, vFanouts );
}

/**Function*************************************************************

  Synopsis    [Collect fanout reachable root nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffcCollectFanout( Abc_Obj_t ** pNodes, int nNodes, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vFanouts )
{
    Abc_Obj_t * pFanin, * pFanout;
    int i, k;
    // dereference nodes
    for ( i = 0; i < nNodes; i++ )
        Abc_MffcDeref_rec( pNodes[i], NULL );
    // collect fanouts
    Vec_PtrClear( vFanouts );
    pFanin = (Abc_Obj_t *)Vec_PtrEntry( vLeaves, 0 );
    Abc_NtkIncrementTravId( pFanin->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pFanin, i )
        Abc_ObjForEachFanout( pFanin, pFanout, k )
            Abc_NktMffcCollectFanout_rec( pFanout, vFanouts );
    // reference nodes
    for ( i = 0; i < nNodes; i++ )
        Abc_MffcRef_rec( pNodes[i] );
}

/**Function*************************************************************

  Synopsis    [Grow one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NktMffcGrowOne( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppObjs, int nObjs, Vec_Ptr_t * vNodes, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vFanouts )
{
    Abc_Obj_t * pFanout, * pFanoutBest = NULL;
    double CostBest = 0.0;
    int i, k;
    Abc_MffcCollectNodes( ppObjs, nObjs, vNodes );
    Abc_MffcCollectLeaves( vNodes, vLeaves );
    // collect fanouts of all fanins
    Abc_NktMffcCollectFanout( ppObjs, nObjs, vLeaves, vFanouts );
    // try different fanouts
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanouts, pFanout, i )
    {
        for ( k = 0; k < nObjs; k++ )
            if ( pFanout == ppObjs[k] )
                break;
        if ( k < nObjs )
            continue;
        ppObjs[nObjs] = pFanout;
        Abc_MffcCollectNodes( ppObjs, nObjs+1, vNodes );
        Abc_MffcCollectLeaves( vNodes, vLeaves );
        if ( pFanoutBest == NULL || CostBest < 1.0 * Vec_PtrSize(vNodes)/Vec_PtrSize(vLeaves) ) 
        {
            CostBest = 1.0 * Vec_PtrSize(vNodes)/Vec_PtrSize(vLeaves);
            pFanoutBest = pFanout;
        }
    }
    return pFanoutBest;
}

/**Function*************************************************************

  Synopsis    [Procedure to increase MFF size by pairing nodes.]

  Description [For each node in the array vRoots, find a matching node, 
  so that the ratio of nodes inside to the leaf nodes is maximized.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NktMffcGrowRoots( Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots )
{
    Vec_Ptr_t * vRoots1, * vNodes, * vLeaves, * vFanouts;
    Abc_Obj_t * pObj, * pRoot2, * pNodes[2];
    int i;
    Abc_NtkCleanMarkA( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
        pObj->fMarkA = 1;
    vRoots1  = Vec_PtrAlloc( 100 );
    vNodes   = Vec_PtrAlloc( 100 );
    vLeaves  = Vec_PtrAlloc( 100 );
    vFanouts = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        pNodes[0] = pObj;
        pRoot2 = Abc_NktMffcGrowOne( pNtk, pNodes, 1, vNodes, vLeaves, vFanouts );
        Vec_PtrPush( vRoots1, pRoot2 );
    }
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vLeaves );
    Vec_PtrFree( vFanouts );
    Abc_NtkCleanMarkA( pNtk );
    return vRoots1;
}

/**Function*************************************************************

  Synopsis    [Procedure to increase MFF size by pairing nodes.]

  Description [For each node in the array vRoots, find a matching node, 
  so that the ratio of nodes inside to the leaf nodes is maximized.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NktMffcGrowRootsAgain( Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots, Vec_Ptr_t * vRoots1 )
{
    Vec_Ptr_t * vRoots2, * vNodes, * vLeaves, * vFanouts;
    Abc_Obj_t * pObj, * pRoot2, * ppObjs[3];
    int i;
    Abc_NtkCleanMarkA( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
        pObj->fMarkA = 1;
    vRoots2  = Vec_PtrAlloc( 100 );
    vNodes   = Vec_PtrAlloc( 100 );
    vLeaves  = Vec_PtrAlloc( 100 );
    vFanouts = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        ppObjs[0] = pObj;
        ppObjs[1] = (Abc_Obj_t *)Vec_PtrEntry( vRoots1, i );
        if ( ppObjs[1] == NULL )
        {
            Vec_PtrPush( vRoots2, NULL );
            continue;
        }
        pRoot2    = Abc_NktMffcGrowOne( pNtk, ppObjs, 2, vNodes, vLeaves, vFanouts );
        Vec_PtrPush( vRoots2, pRoot2 );
    }
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vLeaves );
    Vec_PtrFree( vFanouts );
    Abc_NtkCleanMarkA( pNtk );
    assert( Vec_PtrSize(vRoots) == Vec_PtrSize(vRoots2) );
    return vRoots2;
}

/**Function*************************************************************

  Synopsis    [Testbench.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffcPrint( char * pFileName, Abc_Obj_t ** pNodes, int nNodes, Vec_Ptr_t * vNodes, Vec_Ptr_t * vLeaves )
{
    FILE * pFile;
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    // convert the network
    Abc_NtkToSop( pNodes[0]->pNtk, -1, ABC_INFINITY );
    // write the file
    pFile = fopen( pFileName, "wb" );
    fprintf( pFile, ".model %s_part\n", pNodes[0]->pNtk->pName );
    fprintf( pFile, ".inputs" );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
        fprintf( pFile, " %s", Abc_ObjName(pObj) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs" );
    for ( i = 0; i < nNodes; i++ )
        fprintf( pFile, " %s", Abc_ObjName(pNodes[i]) );
    fprintf( pFile, "\n" );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        fprintf( pFile, ".names" );
        Abc_ObjForEachFanin( pObj, pFanin, k )
            fprintf( pFile, " %s", Abc_ObjName(pFanin) );
        fprintf( pFile, " %s", Abc_ObjName(pObj) );
        fprintf( pFile, "\n%s", (char *)pObj->pData );
    }
    fprintf( pFile, ".end\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Testbench.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffcPrintInt( char * pFileName, Abc_Ntk_t * pNtk, Vec_Int_t * vRoots, Vec_Int_t * vNodes, Vec_Int_t * vLeaves )
{
    FILE * pFile;
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    // convert the network
    Abc_NtkToSop( pNtk, -1, ABC_INFINITY );
    // write the file
    pFile = fopen( pFileName, "wb" );
    fprintf( pFile, ".model %s_part\n", pNtk->pName );
    fprintf( pFile, ".inputs" );
    Abc_NtkForEachObjVec( vLeaves, pNtk, pObj, i )
        fprintf( pFile, " %s", Abc_ObjName(pObj) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs" );
    Abc_NtkForEachObjVec( vRoots, pNtk, pObj, i )
        fprintf( pFile, " %s", Abc_ObjName(pObj) );
    fprintf( pFile, "\n" );
    Abc_NtkForEachObjVec( vNodes, pNtk, pObj, i )
    {
        fprintf( pFile, ".names" );
        Abc_ObjForEachFanin( pObj, pFanin, k )
            fprintf( pFile, " %s", Abc_ObjName(pFanin) );
        fprintf( pFile, " %s", Abc_ObjName(pObj) );
        fprintf( pFile, "\n%s", (char *)pObj->pData );
    }
    fprintf( pFile, ".end\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Testbench.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffcTest( Abc_Ntk_t * pNtk )
{
    char pFileName[1000];
    Vec_Ptr_t * vRoots, * vRoots1, * vRoots2, * vNodes, * vLeaves;
    Abc_Obj_t * pNodes[3], * pObj;
    int i, nNodes = 0, nNodes2 = 0;
    vRoots  = Abc_NktMffcMarkRoots( pNtk, 1 );
    vRoots1 = Abc_NktMffcGrowRoots( pNtk, vRoots );
    vRoots2 = Abc_NktMffcGrowRootsAgain( pNtk, vRoots, vRoots1 );
    vNodes  = Vec_PtrAlloc( 100 );
    vLeaves = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        printf( "%6d : ",      i );

        Abc_MffcCollectNodes( &pObj, 1, vNodes );
        Abc_MffcCollectLeaves( vNodes, vLeaves );
        nNodes += Vec_PtrSize(vNodes);
        printf( "%6d  ",      Abc_ObjId(pObj) );
        printf( "Vol =%3d  ", Vec_PtrSize(vNodes) );
        printf( "Cut =%3d  ", Vec_PtrSize(vLeaves) );
        if ( Vec_PtrSize(vLeaves) < 2 )
        {
            printf( "\n" );
            continue;
        }

        pNodes[0] = pObj;
        pNodes[1] = (Abc_Obj_t *)Vec_PtrEntry( vRoots1, i );
        pNodes[2] = (Abc_Obj_t *)Vec_PtrEntry( vRoots2, i );
        if ( pNodes[1] == NULL || pNodes[2] == NULL )
        {
            printf( "\n" );
            continue;
        }
        Abc_MffcCollectNodes( pNodes, 3, vNodes );
        Abc_MffcCollectLeaves( vNodes, vLeaves );
        nNodes2 += Vec_PtrSize(vNodes);
        printf( "%6d  ",      Abc_ObjId(pNodes[1]) );
        printf( "%6d  ",      Abc_ObjId(pNodes[2]) );
        printf( "Vol =%3d  ", Vec_PtrSize(vNodes) );
        printf( "Cut =%3d  ", Vec_PtrSize(vLeaves) );
 
        printf( "%4.2f  ",    1.0 * Vec_PtrSize(vNodes)/Vec_PtrSize(vLeaves)  );
        printf( "\n" );

        // generate file
        if ( Vec_PtrSize(vNodes) < 10 )
            continue;
        sprintf( pFileName, "%s_mffc%04d_%02d.blif", Abc_NtkName(pNtk), Abc_ObjId(pObj), Vec_PtrSize(vNodes) );
        Abc_NktMffcPrint( pFileName, pNodes, 3, vNodes, vLeaves );
    }
    printf( "Total nodes = %d.  Root nodes = %d.  Mffc nodes = %d.  Mffc nodes2 = %d.\n", 
        Abc_NtkNodeNum(pNtk), Vec_PtrSize(vRoots), nNodes, nNodes2 );
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vLeaves );
    Vec_PtrFree( vRoots );
    Vec_PtrFree( vRoots1 );
    Vec_PtrFree( vRoots2 );
}



/**Function*************************************************************

  Synopsis    [Create the network of supernodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffcTestSuper( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vRoots, * vFanins, * vFanouts, * vNodes, * vLeaves;
    Abc_Obj_t * pObj, * pFanin; 
    Vec_Int_t * vCounts, * vNumbers, * vSizes, * vMarks;
    Vec_Int_t * vNode1, * vNode2;
    int i, k, Entry, nSizes;
    abctime clk = Abc_Clock();
    vRoots   = Abc_NktMffcMarkRoots( pNtk, 1 );
    vFanins  = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) );
    vFanouts = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) );
    vCounts  = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    vNode1   = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    vNode2   = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    vSizes   = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    vMarks   = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );

    // create fanins/fanouts
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        Vec_PtrWriteEntry( vFanins,  Abc_ObjId(pObj), Vec_IntAlloc(8) );
        Vec_PtrWriteEntry( vFanouts, Abc_ObjId(pObj), Vec_IntAlloc(8) );
    }
    // add fanins/fanouts
    vNodes   = Vec_PtrAlloc( 100 );
    vLeaves  = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        Abc_MffcCollectNodes( &pObj, 1, vNodes );
        Abc_MffcCollectLeaves( vNodes, vLeaves );
        Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pFanin, k )
        {
            if ( !Abc_ObjIsNode(pFanin) )
                continue;
            Vec_IntPush( (Vec_Int_t *)Vec_PtrEntry(vFanins,  Abc_ObjId(pObj)),   Abc_ObjId(pFanin) );
            Vec_IntPush( (Vec_Int_t *)Vec_PtrEntry(vFanouts, Abc_ObjId(pFanin)), Abc_ObjId(pObj) );
            // count how many times each object is a fanin
            Vec_IntAddToEntry( vCounts, Abc_ObjId(pFanin), 1 );
        }
        Vec_IntWriteEntry( vSizes, Abc_ObjId(pObj), Vec_PtrSize(vNodes) );
    }

    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        Abc_MffcCollectNodes( &pObj, 1, vNodes );
        Abc_MffcCollectLeaves( vNodes, vLeaves );
        Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pFanin, k )
        {
            if ( !Abc_ObjIsNode(pFanin) )
                continue;
            if ( Vec_IntEntry(vCounts, Abc_ObjId(pFanin)) != 2 )
                continue;
            if ( Vec_IntEntry(vNode1, Abc_ObjId(pFanin)) == 0 )
                Vec_IntWriteEntry( vNode1, Abc_ObjId(pFanin), Abc_ObjId(pObj) );
            else //if ( Vec_IntEntry(vNode2, Abc_ObjId(pFanin)) == 0 )
                Vec_IntWriteEntry( vNode2, Abc_ObjId(pFanin), Abc_ObjId(pObj) );

            Vec_IntWriteEntry( vMarks, Abc_ObjId(pFanin), 1 );
            Vec_IntWriteEntry( vMarks, Abc_ObjId(pObj),   1 );
        }
    }

    // count sizes
    nSizes = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        if ( Vec_IntEntry( vMarks, Abc_ObjId(pObj) ) )
            nSizes += Vec_IntEntry( vSizes, Abc_ObjId(pObj) );
    }
    printf( "Included = %6d.  Total = %6d. (%6.2f %%)\n", 
        nSizes, Abc_NtkNodeNum(pNtk), 100.0 * nSizes / Abc_NtkNodeNum(pNtk) );


    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        if ( Vec_IntEntry(vCounts, Abc_ObjId(pObj)) != 2 )
            continue;
        printf( "%d ", Vec_IntEntry( vSizes, Abc_ObjId(pObj) ) + 
                       Vec_IntEntry( vSizes, Vec_IntEntry(vNode1, Abc_ObjId(pObj)) ) + 
                       Vec_IntEntry( vSizes, Vec_IntEntry(vNode2, Abc_ObjId(pObj)) ) );
    }
    printf( "\n" );

    // print how many times they appear
    vNumbers = Vec_IntStart( 32 );
    Vec_IntForEachEntry( vCounts, Entry, i )
    {
/*
if ( Entry == 2 )
{
    pObj = Abc_NtkObj( pNtk, i );
    Abc_MffcCollectNodes( &pObj, 1, vNodes );
    Abc_MffcCollectLeaves( vNodes, vLeaves );
    printf( "%d(%d) ", Vec_PtrSize(vNodes), Vec_PtrSize(vLeaves) );
}
*/
        if ( Entry == 0 )
            continue;
        if ( Entry <= 10 )
            Vec_IntAddToEntry( vNumbers, Entry, 1 );
        else if ( Entry <= 100 )
            Vec_IntAddToEntry( vNumbers, 10 + Entry/10, 1 );
        else if ( Entry < 1000 )
            Vec_IntAddToEntry( vNumbers, 20 + Entry/100, 1 );
        else
            Vec_IntAddToEntry( vNumbers, 30, 1 );
    }
    for ( i = 1; i <= 10; i++ )
        if ( Vec_IntEntry(vNumbers,i) )
            printf( "       n =  %4d   %6d\n", i, Vec_IntEntry(vNumbers,i) );
    for ( i = 11; i <= 20; i++ )
        if ( Vec_IntEntry(vNumbers,i) )
            printf( "%4d < n <= %4d   %6d\n", 10*(i-10),  10*(i-9),  Vec_IntEntry(vNumbers,i) );
    for ( i = 21; i < 30; i++ )
        if ( Vec_IntEntry(vNumbers,i) )
            printf( "%4d < n <= %4d   %6d\n", 100*(i-20), 100*(i-19), Vec_IntEntry(vNumbers,i) );
    if ( Vec_IntEntry(vNumbers,31) )
    printf( "       n >  1000   %6d\n", Vec_IntEntry(vNumbers,30) );
    printf( "Total MFFCs = %d. ", Vec_PtrSize(vRoots) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Vec_IntFree( vNumbers );
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vLeaves );

    // delete fanins/fanouts
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        Vec_IntFree( (Vec_Int_t *)Vec_PtrEntry(vFanins,  Abc_ObjId(pObj)) );
        Vec_IntFree( (Vec_Int_t *)Vec_PtrEntry(vFanouts, Abc_ObjId(pObj)) );
    }

    Vec_IntFree( vCounts );
    Vec_PtrFree( vFanouts );
    Vec_PtrFree( vFanins );
    Vec_PtrFree( vRoots );
}


/**Function*************************************************************

  Synopsis    [Collects the leaves and the roots of the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffCollectLeafRoot( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vRoots )
{
    Abc_Obj_t * pObj, * pNext;
    int i, k;
    // mark
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->fMarkA = 1;
    // collect leaves
    Vec_PtrClear( vLeaves );
    Abc_NtkIncrementTravId( pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        Abc_ObjForEachFanin( pObj, pNext, k )
        {
            if ( pNext->fMarkA || Abc_NodeIsTravIdCurrent(pNext) )
                continue;
            Abc_NodeSetTravIdCurrent(pNext);
            Vec_PtrPush( vLeaves, pNext );
        }
    // collect roots
    Vec_PtrClear( vRoots );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        Abc_ObjForEachFanout( pObj, pNext, k )
            if ( !pNext->fMarkA )
            {
                Vec_PtrPush( vRoots, pObj );
                break;
            }
    }
    // unmark
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->fMarkA = 0;
}

/**Function*************************************************************

  Synopsis    [Collects the leaves and the roots of the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffCollectLeafRootInt( Abc_Ntk_t * pNtk, Vec_Int_t * vNodes, Vec_Int_t * vLeaves, Vec_Int_t * vRoots )
{
    Abc_Obj_t * pObj, * pNext;
    int i, k;
    // mark
    Abc_NtkForEachObjVec( vNodes, pNtk, pObj, i )
        pObj->fMarkA = 1;
    // collect leaves
    Vec_IntClear( vLeaves );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkForEachObjVec( vNodes, pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pNext, k )
        {
            if ( pNext->fMarkA || Abc_NodeIsTravIdCurrent(pNext) )
                continue;
            Abc_NodeSetTravIdCurrent(pNext);
            Vec_IntPush( vLeaves, Abc_ObjId(pNext) );
        }
    // collect roots
    if ( vRoots )
    {
        Vec_IntClear( vRoots );
        Abc_NtkForEachObjVec( vNodes, pNtk, pObj, i )
        {
            Abc_ObjForEachFanout( pObj, pNext, k )
                if ( !pNext->fMarkA )
                {
                    Vec_IntPush( vRoots, Abc_ObjId(pObj) );
                    break;
                }
        }
    }
    // unmark
    Abc_NtkForEachObjVec( vNodes, pNtk, pObj, i )
        pObj->fMarkA = 0;
}


/**Function*************************************************************

  Synopsis    [Create the network of supernodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffcTestIdeaOne( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj )
{
    Vec_Ptr_t * vNodes, * vLeaves, * vRoots, * vVolume;
    Vec_Ptr_t * vLeaves2, * vRoots2, * vVolume2;
    Abc_Obj_t * pNode, * pNodeBest = pObj;
    double Cost, CostBest = 0.0;
    int i, k;
    vNodes   = Vec_PtrAlloc( 100 );
    vLeaves  = Vec_PtrAlloc( 100 );
    vRoots   = Vec_PtrAlloc( 100 );
    vVolume  = Vec_PtrAlloc( 100 );
    vLeaves2 = Vec_PtrAlloc( 100 );
    vRoots2  = Vec_PtrAlloc( 100 );
    vVolume2 = Vec_PtrAlloc( 100 );
printf( "\n" );
    for ( i = 1; i <= 16; i++ )
    {
        Vec_PtrPush( vNodes, pNodeBest );
        Abc_NktMffCollectLeafRoot( pNtk, vNodes, vLeaves, vRoots );
        Abc_MffcCollectNodes( (Abc_Obj_t **)Vec_PtrArray(vRoots), Vec_PtrSize(vRoots), vVolume );

        printf( "%2d : Node =%6d (%2d%3d)  Cost =%6.2f   ", i, Abc_ObjId(pNodeBest), 
            Abc_ObjFaninNum(pNodeBest), Abc_ObjFanoutNum(pNodeBest), CostBest );
        printf( "Leaf =%2d  Root =%2d  Vol =%2d\n", Vec_PtrSize(vLeaves), Vec_PtrSize(vRoots), Vec_PtrSize(vVolume) );

        // try including different nodes
        pNodeBest = NULL;
        Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pNode, k )
        {
            if ( !Abc_ObjIsNode(pNode) )
                continue;
            Vec_PtrPush( vNodes, pNode );
            Abc_NktMffCollectLeafRoot( pNtk, vNodes, vLeaves2, vRoots2 );
            Abc_MffcCollectNodes( (Abc_Obj_t **)Vec_PtrArray(vRoots2), Vec_PtrSize(vRoots2), vVolume2 );
            Cost = 1.0 * Vec_PtrSize(vVolume2) / (Vec_PtrSize(vLeaves2) + 3 * Vec_PtrSize(vRoots2));
            if ( pNodeBest == NULL || CostBest < Cost )
            {
                pNodeBest = pNode;
                CostBest  = Cost;
            }
            Vec_PtrPop( vNodes );
        }
        Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pNode, k )
        {
            if ( Vec_PtrFind(vNodes, pNode) >= 0 )
                continue;
            if ( !Abc_ObjIsNode(pNode) )
                continue;
            Vec_PtrPush( vNodes, pNode );
            Abc_NktMffCollectLeafRoot( pNtk, vNodes, vLeaves2, vRoots2 );
            Abc_MffcCollectNodes( (Abc_Obj_t **)Vec_PtrArray(vRoots2), Vec_PtrSize(vRoots2), vVolume2 );
            Cost = 1.0 * Vec_PtrSize(vVolume2) / (Vec_PtrSize(vLeaves2) + 3 * Vec_PtrSize(vRoots2));
            if ( pNodeBest == NULL || CostBest < Cost )
            {
                pNodeBest = pNode;
                CostBest  = Cost;
            }
            Vec_PtrPop( vNodes );
        }
        if ( pNodeBest == NULL )
            break;
    }

    Vec_PtrFree( vNodes );
    Vec_PtrFree( vLeaves );
    Vec_PtrFree( vRoots );
    Vec_PtrFree( vVolume );
    Vec_PtrFree( vLeaves2 );
    Vec_PtrFree( vRoots2 );
    Vec_PtrFree( vVolume2 );
}

/**Function*************************************************************

  Synopsis    [Create the network of supernodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffcTestIdea( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj; 
    int i;
    Abc_NtkForEachNode( pNtk, pObj, i )
        if ( Abc_ObjIsNode(pObj) && Abc_ObjId(pObj) % 100 == 0 )
            Abc_NktMffcTestIdeaOne( pNtk, pObj );
}





/**Function*************************************************************

  Synopsis    [Creates MFFCs and their fanins/fanouts/volumes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NktMffcDerive( Abc_Ntk_t * pNtk, Vec_Ptr_t ** pvFanins, Vec_Ptr_t ** pvFanouts, Vec_Ptr_t ** pvVolumes )
{
    Vec_Ptr_t * vRoots, * vFanins, * vFanouts, * vVolumes, * vNodes, * vLeaves;
    Abc_Obj_t * pObj, * pFanin; 
    int i, k;
    abctime clk = Abc_Clock();
    // create roots
    vRoots   = Abc_NktMffcMarkRoots( pNtk, 0 );
    // create fanins/fanouts/volumes
    vFanins  = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) );
    vFanouts = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) );
    vVolumes = Vec_PtrStart( Abc_NtkObjNumMax(pNtk) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        Vec_PtrWriteEntry( vFanins,  Abc_ObjId(pObj), Vec_IntAlloc(8) );
        Vec_PtrWriteEntry( vFanouts, Abc_ObjId(pObj), Vec_IntAlloc(8) );
        Vec_PtrWriteEntry( vVolumes, Abc_ObjId(pObj), Vec_IntAlloc(8) );
    }
    // add fanins/fanouts
    vNodes   = Vec_PtrAlloc( 100 );
    vLeaves  = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        if ( Abc_ObjIsCi(pObj) )
            continue;
        Abc_MffcCollectNodes( &pObj, 1, vNodes );
        Abc_MffcCollectLeaves( vNodes, vLeaves );
        Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pFanin, k )
        {
            Vec_IntPush( (Vec_Int_t *)Vec_PtrEntry(vFanins,  Abc_ObjId(pObj)),   Abc_ObjId(pFanin) );
            Vec_IntPush( (Vec_Int_t *)Vec_PtrEntry(vFanouts, Abc_ObjId(pFanin)), Abc_ObjId(pObj) );
        }
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pFanin, k )
            Vec_IntPush( (Vec_Int_t *)Vec_PtrEntry(vVolumes, Abc_ObjId(pObj)),   Abc_ObjId(pFanin) );
    }

    Vec_PtrFree( vNodes );
    Vec_PtrFree( vLeaves );
    // sort
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        Vec_IntSort( (Vec_Int_t *)Vec_PtrEntry(vFanins,  Abc_ObjId(pObj)), 0 );
        Vec_IntSort( (Vec_Int_t *)Vec_PtrEntry(vFanouts, Abc_ObjId(pObj)), 0 );
    }
    // return
    *pvFanins  = vFanins;
    *pvFanouts = vFanouts;
    *pvVolumes = vVolumes;
    return vRoots;
}

/**Function*************************************************************

  Synopsis    [Frees MFFCs and their fanins/fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffcFree( Vec_Ptr_t * vRoots, Vec_Ptr_t * vFanins, Vec_Ptr_t * vFanouts, Vec_Ptr_t * vVolumes )
{
    Abc_Obj_t * pObj; 
    int i;
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
    {
        Vec_IntFree( (Vec_Int_t *)Vec_PtrEntry(vFanins,  Abc_ObjId(pObj)) );
        Vec_IntFree( (Vec_Int_t *)Vec_PtrEntry(vFanouts, Abc_ObjId(pObj)) );
        Vec_IntFree( (Vec_Int_t *)Vec_PtrEntry(vVolumes, Abc_ObjId(pObj)) );
    }
    Vec_PtrFree( vVolumes );
    Vec_PtrFree( vFanouts );
    Vec_PtrFree( vFanins );
    Vec_PtrFree( vRoots );
}


/**Function*************************************************************

  Synopsis    [Returns the cost of two supports.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Abc_NktMffcCostTwo( Vec_Int_t * vSupp1, Vec_Int_t * vSupp2, int Volume, int Limit )
{
    int nCommon = Vec_IntTwoCountCommon( vSupp1, vSupp2 );
//printf( "s1=%2d s2=%2d c=%2d v=%2d   ", Vec_IntSize(vSupp1), Vec_IntSize(vSupp2), nCommon, Volume );
    if ( Vec_IntSize(vSupp1) + Vec_IntSize(vSupp2) - nCommon > Limit )
        return (double)-ABC_INFINITY;
    return 0.6 * nCommon - 1.2 * Vec_IntSize(vSupp2) + 0.8 * Volume;
}

/**Function*************************************************************

  Synopsis    [Returns support of the group.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NktMffcSupport( Vec_Ptr_t * vThis, Vec_Ptr_t * vFanins )
{
    Vec_Int_t * vIns, * vIns2, * vTemp;
    Abc_Obj_t * pObj;
    int i;
    vIns = Vec_IntAlloc( 100 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vThis, pObj, i )
    {
        vIns2 = (Vec_Int_t *)Vec_PtrEntry( vFanins, Abc_ObjId(pObj) );
        vIns  = Vec_IntTwoMerge( vTemp = vIns, vIns2 );
        Vec_IntFree( vTemp );
    }
    return vIns;
}

/**Function*************************************************************

  Synopsis    [Returns the best merger for the cluster of given node (pPivot).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NktMffcFindBest( Abc_Ntk_t * pNtk, Vec_Int_t * vMarks, Vec_Int_t * vIns, Vec_Ptr_t * vFanins, Vec_Ptr_t * vFanouts, Vec_Ptr_t * vVolumes, int Limit )
{
    Vec_Int_t * vIns2, * vOuts, * vOuts2, * vTemp;
    Abc_Obj_t * pPivot2, * pObj, * pObjBest = NULL;
    double Cost, CostBest = (double)-ABC_INFINITY;
    int i, Volume;
    // collect the fanouts of the fanins
    vOuts = Vec_IntAlloc( 100 );
    Abc_NtkForEachObjVec( vIns, pNtk, pObj, i )
    {
        vOuts2 = (Vec_Int_t *)Vec_PtrEntry( vFanouts, Abc_ObjId(pObj) );
        if ( Vec_IntSize(vOuts2) > 16 )
            continue;
        vOuts  = Vec_IntTwoMerge( vTemp = vOuts, vOuts2 );
        Vec_IntFree( vTemp );
    }
    // check the pairs
    Abc_NtkForEachObjVec( vOuts, pNtk, pPivot2, i )
    {       
        if ( Vec_IntEntry(vMarks, Abc_ObjId(pPivot2)) == 0 )
            continue;
        vIns2  = (Vec_Int_t *)Vec_PtrEntry( vFanins, Abc_ObjId(pPivot2) );
        Volume = Vec_IntSize((Vec_Int_t *)Vec_PtrEntry(vVolumes, Abc_ObjId(pPivot2)));
        Cost   = Abc_NktMffcCostTwo( vIns, vIns2, Volume, Limit );
//printf( "%5d  %2d\n", Abc_ObjId(pPivot2), Cost  );
        if ( Cost == (double)-ABC_INFINITY )
            continue;
        if ( pObjBest == NULL || CostBest < Cost )
        {
            pObjBest = pPivot2;
            CostBest = Cost;
        }
    }    
//printf( "Choosing %d\n", pObjBest->Id );
    Vec_IntFree( vOuts );
    return pObjBest;
}

/**Function*************************************************************

  Synopsis    [Processes one cluster.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NktMffcSaveOne( Vec_Ptr_t * vThis, Vec_Ptr_t * vVolumes )
{
    Vec_Int_t * vVolume, * vResult;
    Abc_Obj_t * pObj;
    int i, k, Entry;
    vResult = Vec_IntAlloc( 100 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vThis, pObj, i )
    {
        vVolume = (Vec_Int_t *)Vec_PtrEntry( vVolumes, Abc_ObjId(pObj) );
        Vec_IntForEachEntry( vVolume, Entry, k )
            Vec_IntPush( vResult, Entry );
    }
    return vResult;
}

/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeCompareVolumeDecrease( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 )
{
    int Diff = Abc_ObjRegular(*pp1)->iTemp - Abc_ObjRegular(*pp2)->iTemp;
    if ( Diff > 0 )
        return -1;
    if ( Diff < 0 ) 
        return 1;
    Diff = Abc_ObjRegular(*pp1)->Id - Abc_ObjRegular(*pp2)->Id;
    if ( Diff > 0 )
        return -1;
    if ( Diff < 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Create the network of supernodes.]

  Description [Returns array of interger arrays of IDs of nodes
  included in a disjoint structural decomposition of the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NktMffcServer( Abc_Ntk_t * pNtk, int nInMax, int nOutMax )
{
    Vec_Ptr_t * vResult, * vThis;
    Vec_Ptr_t * vPivots, * vFanins, * vFanouts, * vVolumes;
    Vec_Int_t * vLeaves, * vMarks;
    Abc_Obj_t * pObj, * pObj2;
    int i, k;
    assert( nOutMax >= 1 && nOutMax <= 32 );
    vResult = Vec_PtrAlloc( 100 );
    // create fanins/fanouts
    vPivots = Abc_NktMffcDerive( pNtk, &vFanins, &vFanouts, &vVolumes );
    // sort by their MFFC size
    Vec_PtrForEachEntry( Abc_Obj_t *, vPivots, pObj, i )
        pObj->iTemp = Vec_IntSize((Vec_Int_t *)Vec_PtrEntry(vVolumes, Abc_ObjId(pObj)));
    Vec_PtrSort( vPivots, (int (*)(void))Abc_NodeCompareVolumeDecrease ); 
    // create marks
    vMarks = Vec_IntStart( Abc_NtkObjNumMax(pNtk) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vPivots, pObj, i )
        if ( Abc_ObjIsNode(pObj) && Vec_IntSize((Vec_Int_t *)Vec_PtrEntry(vVolumes, Abc_ObjId(pObj))) > 1 )
            Vec_IntWriteEntry( vMarks, Abc_ObjId(pObj), 1 );
    // consider nodes in the order of the marks
    vThis = Vec_PtrAlloc( 10 );
//    while ( 1 )
    Vec_PtrForEachEntry( Abc_Obj_t *, vPivots, pObj, i )
    {
//        pObj = Abc_NtkObj( pNtk, 589 );
        if ( Vec_IntEntry(vMarks, Abc_ObjId(pObj)) == 0 )
            continue;
        // start the set
        Vec_PtrClear( vThis );        
        Vec_PtrPush( vThis, pObj );
        Vec_IntWriteEntry( vMarks, Abc_ObjId(pObj), 0 );
        // quit if exceeded the limit
        vLeaves = (Vec_Int_t *)Vec_PtrEntry( vFanins, Abc_ObjId(pObj) );
        if ( Vec_IntSize(vLeaves) > nInMax )
        {
            Vec_PtrPush( vResult, Abc_NktMffcSaveOne(vThis, vVolumes) );
            continue;
        }
        // try adding one node at a time
        for ( k = 1; k < nOutMax; k++ )
        {
            // quit if exceeded the limit
            vLeaves = Abc_NktMffcSupport( vThis, vFanins );
            assert( Vec_IntSize(vLeaves) <= nInMax );
            pObj2 = Abc_NktMffcFindBest( pNtk, vMarks, vLeaves, vFanins, vFanouts, vVolumes, nInMax );
            Vec_IntFree( vLeaves );
            // quit if there is no extension
            if ( pObj2 == NULL )
                break;
            Vec_PtrPush( vThis, pObj2 );
            Vec_IntWriteEntry( vMarks, Abc_ObjId(pObj2), 0 );
        }
        Vec_PtrPush( vResult, Abc_NktMffcSaveOne(vThis, vVolumes) );
//        break;
    }
    Vec_PtrFree( vThis );
    Vec_IntFree( vMarks );
    // delele fanins/outputs
    Abc_NktMffcFree( vPivots, vFanins, vFanouts, vVolumes );
    return vResult;
}

/**Function*************************************************************

  Synopsis    [Testbench.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NktMffcServerTest( Abc_Ntk_t * pNtk )
{
    char pFileName[1000];
    Vec_Ptr_t * vGlobs;
    Vec_Int_t * vGlob, * vLeaves, * vRoots;
    double Cost, CostAll = 0.0;
    int i, k, Entry, nNodes = 0;
    abctime clk = Abc_Clock();
    vGlobs  = Abc_NktMffcServer( pNtk, 18, 3 );
    vLeaves = Vec_IntAlloc( 100 );
    vRoots  = Vec_IntAlloc( 100 );
    Vec_PtrForEachEntry( Vec_Int_t *, vGlobs, vGlob, i )
    {
        nNodes += Vec_IntSize(vGlob);
        Abc_NktMffCollectLeafRootInt( pNtk, vGlob, vLeaves, vRoots );
        if ( Vec_IntSize(vGlob) <= Vec_IntSize(vRoots) )
            continue;
        Cost = 1.0 * Vec_IntSize(vGlob)/(Vec_IntSize(vLeaves) + Vec_IntSize(vRoots));
        CostAll += Cost;
        if ( Cost < 0.5 )
            continue;

        printf( "%6d : Root =%3d. Leaf =%3d. Node =%4d. ", 
            i, Vec_IntSize(vRoots), Vec_IntSize(vLeaves), Vec_IntSize(vGlob) );
        printf( "Cost =%6.2f     ", Cost );
        Vec_IntForEachEntry( vRoots, Entry, k )
            printf( "%d ", Entry );
        printf( "\n" );

        sprintf( pFileName, "%sc%04di%02dn%02d.blif", Abc_NtkName(pNtk), i, Vec_IntSize(vLeaves), Vec_IntSize(vGlob) );
        Abc_NktMffcPrintInt( pFileName, pNtk, vRoots, vGlob, vLeaves );
    }
    Vec_IntFree( vLeaves );
    Vec_IntFree( vRoots );
    Vec_PtrForEachEntry( Vec_Int_t *, vGlobs, vGlob, i )
        Vec_IntFree( vGlob );
    Vec_PtrFree( vGlobs );
    printf( "Total = %6d.  Nodes = %6d.  ", Abc_NtkNodeNum(pNtk), nNodes );
    printf( "Cost = %6.2f  ", CostAll );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

ABC_NAMESPACE_IMPL_END

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


