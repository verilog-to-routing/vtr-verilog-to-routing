/**CFile****************************************************************

  FileName    [simSymStr.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Structural detection of symmetries.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: simSymStr.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "sim.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define SIM_READ_SYMMS(pNode)       ((Vec_Int_t *)pNode->pCopy) 
#define SIM_SET_SYMMS(pNode,vVect)  (pNode->pCopy = (Abc_Obj_t *)(vVect)) 

static void  Sim_SymmsStructComputeOne( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, int * pMap );
static void  Sim_SymmsBalanceCollect_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );
static void  Sim_SymmsPartitionNodes( Vec_Ptr_t * vNodes, Vec_Ptr_t * vNodesPis0, Vec_Ptr_t * vNodesPis1, Vec_Ptr_t * vNodesOther );
static void  Sim_SymmsAppendFromGroup( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodesPi, Vec_Ptr_t * vNodesOther, Vec_Int_t * vSymms, int * pMap );
static void  Sim_SymmsAppendFromNode( Abc_Ntk_t * pNtk, Vec_Int_t * vSymms0, Vec_Ptr_t * vNodesOther, Vec_Ptr_t * vNodesPi0, Vec_Ptr_t * vNodesPi1, Vec_Int_t * vSymms, int * pMap );
static int   Sim_SymmsIsCompatibleWithNodes( Abc_Ntk_t * pNtk, unsigned uSymm, Vec_Ptr_t * vNodesOther, int * pMap );
static int   Sim_SymmsIsCompatibleWithGroup( unsigned uSymm, Vec_Ptr_t * vNodesPi, int * pMap );
static void  Sim_SymmsPrint( Vec_Int_t * vSymms );
static void  Sim_SymmsTrans( Vec_Int_t * vSymms );
static void  Sim_SymmsTransferToMatrix( Extra_BitMat_t * pMatSymm, Vec_Int_t * vSymms, unsigned * pSupport );
static int * Sim_SymmsCreateMap( Abc_Ntk_t * pNtk );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes symmetries for a single output function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsStructCompute( Abc_Ntk_t * pNtk, Vec_Ptr_t * vMatrs, Vec_Ptr_t * vSuppFun )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pTemp;
    int * pMap, i;

    assert( Abc_NtkCiNum(pNtk) + 10 < (1<<16) );

    // get the structural support
    pNtk->vSupps = Sim_ComputeStrSupp( pNtk );
    // set elementary info for the CIs
    Abc_NtkForEachCi( pNtk, pTemp, i )
        SIM_SET_SYMMS( pTemp, Vec_IntAlloc(0) );
    // create the map of CI ids into their numbers
    pMap = Sim_SymmsCreateMap( pNtk );
    // collect the nodes in the TFI cone of this output
    vNodes = Abc_NtkDfs( pNtk, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pTemp, i )
    {
//        if ( Abc_NodeIsConst(pTemp) )
//            continue;
        Sim_SymmsStructComputeOne( pNtk, pTemp, pMap );
    }
    // collect the results for the COs;
    Abc_NtkForEachCo( pNtk, pTemp, i )
    {
//printf( "Output %d:\n", i );
        pTemp = Abc_ObjFanin0(pTemp);
        if ( Abc_ObjIsCi(pTemp) || Abc_AigNodeIsConst(pTemp) )
            continue;
        Sim_SymmsTransferToMatrix( (Extra_BitMat_t *)Vec_PtrEntry(vMatrs, i), SIM_READ_SYMMS(pTemp), (unsigned *)Vec_PtrEntry(vSuppFun, i) );
    }
    // clean the intermediate results
    Sim_UtilInfoFree( pNtk->vSupps );
    pNtk->vSupps = NULL;
    Abc_NtkForEachCi( pNtk, pTemp, i )
        Vec_IntFree( SIM_READ_SYMMS(pTemp) );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pTemp, i )
//        if ( !Abc_NodeIsConst(pTemp) )
            Vec_IntFree( SIM_READ_SYMMS(pTemp) );
    Vec_PtrFree( vNodes );
    ABC_FREE( pMap );
}

/**Function*************************************************************

  Synopsis    [Recursively computes symmetries. ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsStructComputeOne( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, int * pMap )
{
    Vec_Ptr_t * vNodes, * vNodesPi0, * vNodesPi1, * vNodesOther;
    Vec_Int_t * vSymms;
    Abc_Obj_t * pTemp;
    int i;

    // allocate the temporary arrays
    vNodes      = Vec_PtrAlloc( 10 );
    vNodesPi0   = Vec_PtrAlloc( 10 );
    vNodesPi1   = Vec_PtrAlloc( 10 );
    vNodesOther = Vec_PtrAlloc( 10 );  

    // collect the fanins of the implication supergate
    Sim_SymmsBalanceCollect_rec( pNode, vNodes );

    // sort the nodes in the implication supergate
    Sim_SymmsPartitionNodes( vNodes, vNodesPi0, vNodesPi1, vNodesOther );

    // start the resulting set
    vSymms = Vec_IntAlloc( 10 );
    // generate symmetries from the groups
    Sim_SymmsAppendFromGroup( pNtk, vNodesPi0, vNodesOther, vSymms, pMap );
    Sim_SymmsAppendFromGroup( pNtk, vNodesPi1, vNodesOther, vSymms, pMap );
    // add symmetries from other inputs
    for ( i = 0; i < vNodesOther->nSize; i++ )
    {
        pTemp = Abc_ObjRegular((Abc_Obj_t *)vNodesOther->pArray[i]);
        Sim_SymmsAppendFromNode( pNtk, SIM_READ_SYMMS(pTemp), vNodesOther, vNodesPi0, vNodesPi1, vSymms, pMap );
    }
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vNodesPi0 );
    Vec_PtrFree( vNodesPi1 );
    Vec_PtrFree( vNodesOther );

    // set the symmetry at the node
    SIM_SET_SYMMS( pNode, vSymms );
}


/**Function*************************************************************

  Synopsis    [Returns the array of nodes to be combined into one multi-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsBalanceCollect_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    // if the new node is complemented, another gate begins
    if ( Abc_ObjIsComplement(pNode) )
    {
        Vec_PtrPushUnique( vNodes, pNode );
        return;
    }
    // if pNew is the PI node, return
    if ( Abc_ObjIsCi(pNode) )
    {
        Vec_PtrPushUnique( vNodes, pNode );
        return;    
    }
    // go through the branches
    Sim_SymmsBalanceCollect_rec( Abc_ObjChild0(pNode), vNodes );
    Sim_SymmsBalanceCollect_rec( Abc_ObjChild1(pNode), vNodes );
}

/**Function*************************************************************

  Synopsis    [Divides PI variables into groups.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsPartitionNodes( Vec_Ptr_t * vNodes, Vec_Ptr_t * vNodesPis0, 
    Vec_Ptr_t * vNodesPis1, Vec_Ptr_t * vNodesOther )
{
    Abc_Obj_t * pNode;
    int i;
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( !Abc_ObjIsCi(Abc_ObjRegular(pNode)) )
            Vec_PtrPush( vNodesOther, pNode );
        else if ( Abc_ObjIsComplement(pNode) )
            Vec_PtrPush( vNodesPis0, pNode );
        else
            Vec_PtrPush( vNodesPis1, pNode );
    }
}

/**Function*************************************************************

  Synopsis    [Makes the product of two partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsAppendFromGroup( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodesPi, Vec_Ptr_t * vNodesOther, Vec_Int_t * vSymms, int * pMap )
{
    Abc_Obj_t * pNode1, * pNode2;
    unsigned uSymm;
    int i, k;

    if ( vNodesPi->nSize == 0 )
        return;

    // go through the pairs
    for ( i = 0; i < vNodesPi->nSize; i++ )
    for ( k = i+1; k < vNodesPi->nSize; k++ )
    {
        // get the two PI nodes
        pNode1 = Abc_ObjRegular((Abc_Obj_t *)vNodesPi->pArray[i]);
        pNode2 = Abc_ObjRegular((Abc_Obj_t *)vNodesPi->pArray[k]);
        assert( pMap[pNode1->Id] != pMap[pNode2->Id] );
        assert( pMap[pNode1->Id] >= 0 );
        assert( pMap[pNode2->Id] >= 0 );
        // generate symmetry
        if ( pMap[pNode1->Id] < pMap[pNode2->Id] )
            uSymm = ((pMap[pNode1->Id] << 16) | pMap[pNode2->Id]);
        else
            uSymm = ((pMap[pNode2->Id] << 16) | pMap[pNode1->Id]);
        // check if symmetry belongs
        if ( Sim_SymmsIsCompatibleWithNodes( pNtk, uSymm, vNodesOther, pMap ) )
            Vec_IntPushUnique( vSymms, (int)uSymm );
    }
}

/**Function*************************************************************

  Synopsis    [Add the filters symmetries from the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsAppendFromNode( Abc_Ntk_t * pNtk, Vec_Int_t * vSymms0, Vec_Ptr_t * vNodesOther, 
    Vec_Ptr_t * vNodesPi0, Vec_Ptr_t * vNodesPi1, Vec_Int_t * vSymms, int * pMap )
{
    unsigned uSymm;
    int i;

    if ( vSymms0->nSize == 0 )
        return;

    // go through the pairs
    for ( i = 0; i < vSymms0->nSize; i++ )
    {
        uSymm = (unsigned)vSymms0->pArray[i];
        // check if symmetry belongs
        if ( Sim_SymmsIsCompatibleWithNodes( pNtk, uSymm, vNodesOther, pMap ) &&
             Sim_SymmsIsCompatibleWithGroup( uSymm, vNodesPi0, pMap ) && 
             Sim_SymmsIsCompatibleWithGroup( uSymm, vNodesPi1, pMap ) )
            Vec_IntPushUnique( vSymms, (int)uSymm );
    }
}

/**Function*************************************************************

  Synopsis    [Returns 1 if symmetry is compatible with the group of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_SymmsIsCompatibleWithNodes( Abc_Ntk_t * pNtk, unsigned uSymm, Vec_Ptr_t * vNodesOther, int * pMap )
{
    Vec_Int_t * vSymmsNode;
    Abc_Obj_t * pNode;
    int i, s, Ind1, Ind2, fIsVar1, fIsVar2;

    if ( vNodesOther->nSize == 0 )
        return 1;

    // get the indices of the PI variables
    Ind1 = (uSymm & 0xffff);
    Ind2 = (uSymm >> 16);

    // go through the nodes
    // if they do not belong to a support, it is okay
    // if one belongs, the other does not belong, quit
    // if they belong, but are not part of symmetry, quit
    for ( i = 0; i < vNodesOther->nSize; i++ )
    {
        pNode = Abc_ObjRegular((Abc_Obj_t *)vNodesOther->pArray[i]);
        fIsVar1 = Sim_SuppStrHasVar( pNtk->vSupps, pNode, Ind1 );
        fIsVar2 = Sim_SuppStrHasVar( pNtk->vSupps, pNode, Ind2 );

        if ( !fIsVar1 && !fIsVar2 )
            continue;
        if ( fIsVar1 ^ fIsVar2 )
            return 0;
        // both belong
        // check if there is a symmetry
        vSymmsNode = SIM_READ_SYMMS( pNode );
        for ( s = 0; s < vSymmsNode->nSize; s++ )
            if ( uSymm == (unsigned)vSymmsNode->pArray[s] )
                break;
        if ( s == vSymmsNode->nSize )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if symmetry is compatible with the group of PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sim_SymmsIsCompatibleWithGroup( unsigned uSymm, Vec_Ptr_t * vNodesPi, int * pMap )
{
    Abc_Obj_t * pNode;
    int i, Ind1, Ind2, fHasVar1, fHasVar2;

    if ( vNodesPi->nSize == 0 )
        return 1;

    // get the indices of the PI variables
    Ind1 = (uSymm & 0xffff);
    Ind2 = (uSymm >> 16);

    // go through the PI nodes
    fHasVar1 = fHasVar2 = 0;
    for ( i = 0; i < vNodesPi->nSize; i++ )
    {
        pNode = Abc_ObjRegular((Abc_Obj_t *)vNodesPi->pArray[i]);
        if ( pMap[pNode->Id] == Ind1 )
            fHasVar1 = 1;
        else if ( pMap[pNode->Id] == Ind2 )
            fHasVar2 = 1;
    }
    return fHasVar1 == fHasVar2;
}



/**Function*************************************************************

  Synopsis    [Improvements due to transitivity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsTrans( Vec_Int_t * vSymms )
{
    unsigned uSymm, uSymma, uSymmr;
    int i, Ind1, Ind2;
    int k, Ind1a, Ind2a;
    int j;
    int nTrans = 0;

    for ( i = 0; i < vSymms->nSize; i++ )
    {
        uSymm = (unsigned)vSymms->pArray[i];
        Ind1 = (uSymm & 0xffff);
        Ind2 = (uSymm >> 16);
        // find other symmetries that have Ind1
        for ( k = i+1; k < vSymms->nSize; k++ )
        {
            uSymma = (unsigned)vSymms->pArray[k];
            if ( uSymma == uSymm )
                continue;
            Ind1a = (uSymma & 0xffff);
            Ind2a = (uSymma >> 16);
            if ( Ind1a == Ind1 )
            {
                // find the symmetry (Ind2,Ind2a)
                if ( Ind2 < Ind2a )
                    uSymmr = ((Ind2 << 16) | Ind2a);
                else
                    uSymmr = ((Ind2a << 16) | Ind2);
                for ( j = 0; j < vSymms->nSize; j++ )
                    if ( uSymmr == (unsigned)vSymms->pArray[j] )
                        break;
                if ( j == vSymms->nSize )
                    nTrans++;
            }
        }

    }
    printf( "Trans = %d.\n", nTrans );
}


/**Function*************************************************************

  Synopsis    [Transfers from the vector to the matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sim_SymmsTransferToMatrix( Extra_BitMat_t * pMatSymm, Vec_Int_t * vSymms, unsigned * pSupport )
{
    int i, Ind1, Ind2, nInputs;
    unsigned uSymm;
    // add diagonal elements
    nInputs = Extra_BitMatrixReadSize( pMatSymm );
    for ( i = 0; i < nInputs; i++ )
        Extra_BitMatrixInsert1( pMatSymm, i, i );
    // add non-diagonal elements
    for ( i = 0; i < vSymms->nSize; i++ )
    {
        uSymm = (unsigned)vSymms->pArray[i];
        Ind1 = (uSymm & 0xffff);
        Ind2 = (uSymm >> 16);
//printf( "%d,%d ", Ind1, Ind2 );
        // skip variables that are not in the true support
        assert( Sim_HasBit(pSupport, Ind1) == Sim_HasBit(pSupport, Ind2) );
        if ( !Sim_HasBit(pSupport, Ind1) || !Sim_HasBit(pSupport, Ind2) )
            continue;
        Extra_BitMatrixInsert1( pMatSymm, Ind1, Ind2 );
        Extra_BitMatrixInsert2( pMatSymm, Ind1, Ind2 );
    }
}

/**Function*************************************************************

  Synopsis    [Mapping of indices into numbers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Sim_SymmsCreateMap( Abc_Ntk_t * pNtk )
{
    int * pMap;
    Abc_Obj_t * pNode;
    int i;
    pMap = ABC_ALLOC( int, Abc_NtkObjNumMax(pNtk) );
    for ( i = 0; i < Abc_NtkObjNumMax(pNtk); i++ )
        pMap[i] = -1;
    Abc_NtkForEachCi( pNtk, pNode, i )
        pMap[pNode->Id] = i;
    return pMap;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

