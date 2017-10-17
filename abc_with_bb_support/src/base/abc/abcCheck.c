/**CFile****************************************************************

  FileName    [abcCheck.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Consistency checking procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcCheck.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "base/main/main.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Abc_NtkCheckNames( Abc_Ntk_t * pNtk );
static int Abc_NtkCheckPis( Abc_Ntk_t * pNtk );
static int Abc_NtkCheckPos( Abc_Ntk_t * pNtk );
//static int Abc_NtkCheckObj( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj );
static int Abc_NtkCheckNet( Abc_Ntk_t * pNtk, Abc_Obj_t * pNet );
static int Abc_NtkCheckNode( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode );
static int Abc_NtkCheckLatch( Abc_Ntk_t * pNtk, Abc_Obj_t * pLatch );

static int Abc_NtkComparePis( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb );
static int Abc_NtkComparePos( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb );
static int Abc_NtkCompareLatches( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb );

static inline char * Abc_ObjNameNet( Abc_Obj_t * pObj ) { return (Abc_ObjIsNode(pObj) && Abc_NtkIsNetlist(pObj->pNtk)) ? Abc_ObjName(Abc_ObjFanout0(pObj)) : Abc_ObjName(pObj); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks the integrity of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheck( Abc_Ntk_t * pNtk )
{ 
   return !Abc_FrameIsFlagEnabled( "check" ) || Abc_NtkDoCheck( pNtk );
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of the network after reading.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckRead( Abc_Ntk_t * pNtk )
{
   return !Abc_FrameIsFlagEnabled( "checkread" ) || Abc_NtkDoCheck( pNtk );
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDoCheck( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pNet, * pNode;
    int i;

    // check network types
    if ( !Abc_NtkIsNetlist(pNtk) && !Abc_NtkIsLogic(pNtk) && !Abc_NtkIsStrash(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Unknown network type.\n" );
        return 0;
    }
    if ( !Abc_NtkHasSop(pNtk) && !Abc_NtkHasBdd(pNtk) && !Abc_NtkHasAig(pNtk) && !Abc_NtkHasMapping(pNtk) && !Abc_NtkHasBlifMv(pNtk) && !Abc_NtkHasBlackbox(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Unknown functionality type.\n" );
        return 0;
    }
    if ( Abc_NtkHasMapping(pNtk) )
    {
        if ( pNtk->pManFunc != Abc_FrameReadLibGen() )
        {
            fprintf( stdout, "NetworkCheck: The library of the mapped network is not the global library.\n" );
            return 0;
        }
    }

    if ( Abc_NtkHasOnlyLatchBoxes(pNtk) )
    {
        // check CI/CO numbers
        if ( Abc_NtkPiNum(pNtk) + Abc_NtkLatchNum(pNtk) != Abc_NtkCiNum(pNtk) )
        {
            fprintf( stdout, "NetworkCheck: Number of CIs does not match number of PIs and latches.\n" );
            fprintf( stdout, "One possible reason is that latches are added twice:\n" );
            fprintf( stdout, "in procedure Abc_NtkCreateObj() and in the user's code.\n" );
            return 0;
        }
        if ( Abc_NtkPoNum(pNtk) + Abc_NtkLatchNum(pNtk) != Abc_NtkCoNum(pNtk) )
        {
            fprintf( stdout, "NetworkCheck: Number of COs does not match number of POs, asserts, and latches.\n" );
            fprintf( stdout, "One possible reason is that latches are added twice:\n" );
            fprintf( stdout, "in procedure Abc_NtkCreateObj() and in the user's code.\n" );
            return 0;
        }
    }

    // check the names
    if ( !Abc_NtkCheckNames( pNtk ) )
        return 0;

    // check PIs and POs
    Abc_NtkCleanCopy( pNtk );
    if ( !Abc_NtkCheckPis( pNtk ) )
        return 0;
    if ( !Abc_NtkCheckPos( pNtk ) )
        return 0;

    if ( Abc_NtkHasBlackbox(pNtk) )
        return 1;

    // check the connectivity of objects
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_NtkCheckObj( pNtk, pObj ) )
            return 0;

    // if it is a netlist change nets and latches
    if ( Abc_NtkIsNetlist(pNtk) )
    {
        if ( Abc_NtkNetNum(pNtk) == 0 )
            fprintf( stdout, "NetworkCheck: Warning! Netlist has no nets.\n" );
        // check the nets
        Abc_NtkForEachNet( pNtk, pNet, i )
            if ( !Abc_NtkCheckNet( pNtk, pNet ) )
                return 0;
    }
    else
    {
        if ( Abc_NtkNetNum(pNtk) != 0 )
        {
            fprintf( stdout, "NetworkCheck: A network that is not a netlist has nets.\n" );
            return 0;
        }
    }

    // check the nodes
    if ( Abc_NtkIsStrash(pNtk) )
        Abc_AigCheck( (Abc_Aig_t *)pNtk->pManFunc );
    else
    {
        Abc_NtkForEachNode( pNtk, pNode, i )
            if ( !Abc_NtkCheckNode( pNtk, pNode ) )
                return 0;
    }

    // check the latches
    Abc_NtkForEachLatch( pNtk, pNode, i )
        if ( !Abc_NtkCheckLatch( pNtk, pNode ) )
            return 0;

    // finally, check for combinational loops
//  clk = Abc_Clock();
    if ( !Abc_NtkIsAcyclic( pNtk ) )
    {
        fprintf( stdout, "NetworkCheck: Network contains a combinational loop.\n" );
        return 0;
    }
//  ABC_PRT( "Acyclic  ", Abc_Clock() - clk );

    // check the EXDC network if present
    if ( pNtk->pExdc )
        Abc_NtkCheck( pNtk->pExdc );
/*
    // check the hierarchy
    if ( Abc_NtkIsNetlist(pNtk) && pNtk->tName2Model )
    {
        stmm_generator * gen;
        Abc_Ntk_t * pNtkTemp;
        char * pName;
        // check other networks
        stmm_foreach_item( pNtk->tName2Model, gen, &pName, (char **)&pNtkTemp )
        {
            pNtkTemp->fHiePath = pNtkTemp->fHieVisited = 0;
            if ( !Abc_NtkCheck( pNtkTemp ) )
                return 0;
        }
        // check acyclic dependency of the models
        if ( !Abc_NtkIsAcyclicHierarchy( pNtk ) )
        {
            fprintf( stdout, "NetworkCheck: Network hierarchical dependences contains a cycle.\n" );
            return 0;
        }
    }
*/
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks the names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj = NULL; // Ensure pObj isn't used uninitialized.
    Vec_Int_t * vNameIds;
    char * pName;
    int i, NameId;

    if ( Abc_NtkIsNetlist(pNtk) )
        return 1;

    // check that each CI/CO has a name
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        pObj = Abc_ObjFanout0Ntk(pObj);
        if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) == NULL )
        {
            fprintf( stdout, "NetworkCheck: CI with ID %d is in the network but not in the name table.\n", pObj->Id );
            return 0;
        }
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pObj = Abc_ObjFanin0Ntk(pObj);
        if ( Nm_ManFindNameById(pObj->pNtk->pManName, pObj->Id) == NULL )
        {
            fprintf( stdout, "NetworkCheck: CO with ID %d is in the network but not in the name table.\n", pObj->Id );
            return 0;
        }
    }

    assert(pObj); // pObj should point to something here.

    // return the array of all IDs, which have names
    vNameIds = Nm_ManReturnNameIds( pNtk->pManName );
    // make sure that these IDs correspond to live objects
    Vec_IntForEachEntry( vNameIds, NameId, i )
    {
        if ( Vec_PtrEntry( pNtk->vObjs, NameId ) == NULL )
        {
            Vec_IntFree( vNameIds );
            pName = Nm_ManFindNameById(pObj->pNtk->pManName, NameId);
            fprintf( stdout, "NetworkCheck: Object with ID %d is deleted but its name \"%s\" remains in the name table.\n", NameId, pName );
            return 0;
        }
    }
    Vec_IntFree( vNameIds );

    // make sure the CI names are unique
    if ( !Abc_NtkCheckUniqueCiNames(pNtk) )
        return 0;

    // make sure the CO names are unique
    if ( !Abc_NtkCheckUniqueCoNames(pNtk) )
        return 0;

    // make sure that if a CO has the same name as a CI, they point directly
    if ( !Abc_NtkCheckUniqueCioNames(pNtk) )
        return 0;

    return 1;
}


/**Function*************************************************************

  Synopsis    [Checks the PIs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckPis( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;

    // check that PIs are indeed PIs
    Abc_NtkForEachPi( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsPi(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Object \"%s\" (id=%d) is in the PI list but is not a PI.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        if ( pObj->pData )
        {
            fprintf( stdout, "NetworkCheck: A PI \"%s\" has a logic function.\n", Abc_ObjName(pObj) );
            return 0;
        }
        if ( Abc_ObjFaninNum(pObj) > 0 )
        {
            fprintf( stdout, "NetworkCheck: A PI \"%s\" has fanins.\n", Abc_ObjName(pObj) );
            return 0;
        }
        pObj->pCopy = (Abc_Obj_t *)1;
    }
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->pCopy == NULL && Abc_ObjIsPi(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Object \"%s\" (id=%d) is a PI but is not in the PI list.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        pObj->pCopy = NULL;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks the POs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckPos( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;

    // check that POs are indeed POs
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsPo(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Net \"%s\" (id=%d) is in the PO list but is not a PO.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        if ( pObj->pData )
        {
            fprintf( stdout, "NetworkCheck: A PO \"%s\" has a logic function.\n", Abc_ObjName(pObj) );
            return 0;
        }
        if ( Abc_ObjFaninNum(pObj) != 1 )
        {
            fprintf( stdout, "NetworkCheck: A PO \"%s\" does not have one fanin (but %d).\n", Abc_ObjName(pObj), Abc_ObjFaninNum(pObj) );
            return 0;
        }
        if ( Abc_ObjFanoutNum(pObj) > 0 )
        {
            fprintf( stdout, "NetworkCheck: A PO \"%s\" has %d fanout(s).\n", Abc_ObjName(pObj), Abc_ObjFanoutNum(pObj) );
            return 0;
        }
        pObj->pCopy = (Abc_Obj_t *)1;
    }
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->pCopy == NULL && Abc_ObjIsPo(pObj) )
        {
            fprintf( stdout, "NetworkCheck: Net \"%s\" (id=%d) is in a PO but is not in the PO list.\n", Abc_ObjName(pObj), pObj->Id );
            return 0;
        }
        pObj->pCopy = NULL;
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Checks the connectivity of the object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckObj( Abc_Ntk_t * pNtk, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin, * pFanout;
    int Value = 1;
    int i, k;

    // check the network
    if ( pObj->pNtk != pNtk )
    {
        fprintf( stdout, "NetworkCheck: Object \"%s\" does not belong to the network.\n", Abc_ObjName(pObj) );
        return 0;
    }
    // check the object ID
    if ( pObj->Id < 0 || (int)pObj->Id >= Abc_NtkObjNumMax(pNtk) )
    {
        fprintf( stdout, "NetworkCheck: Object \"%s\" has incorrect ID.\n", Abc_ObjName(pObj) );
        return 0;
    }

    if ( !Abc_FrameIsFlagEnabled("checkfio") )
        return Value;

    // go through the fanins of the object and make sure fanins have this object as a fanout
    Abc_ObjForEachFanin( pObj, pFanin, i )
    {
        if ( Vec_IntFind( &pFanin->vFanouts, pObj->Id ) == -1 )
        {
            fprintf( stdout, "NodeCheck: Object \"%s\" has fanin ", Abc_ObjName(pObj) );
            fprintf( stdout, "\"%s\" but the fanin does not have it as a fanout.\n", Abc_ObjName(pFanin) );
            Value = 0;
        }
    }
    // go through the fanouts of the object and make sure fanouts have this object as a fanin
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        if ( Vec_IntFind( &pFanout->vFanins, pObj->Id ) == -1 )
        {
            fprintf( stdout, "NodeCheck: Object \"%s\" has fanout ", Abc_ObjName(pObj) );
            fprintf( stdout, "\"%s\" but the fanout does not have it as a fanin.\n", Abc_ObjName(pFanout) );
            Value = 0;
        }
    }

    // make sure fanins are not duplicated
    for ( i = 0; i < pObj->vFanins.nSize; i++ )
        for ( k = i + 1; k < pObj->vFanins.nSize; k++ )
            if ( pObj->vFanins.pArray[k] == pObj->vFanins.pArray[i] )
            {
                printf( "Warning: Node %s has", Abc_ObjName(pObj) );
                printf( " duplicated fanin %s.\n", Abc_ObjName(Abc_ObjFanin(pObj,k)) );
            }

    // save time: do not check large fanout lists
    if ( pObj->vFanouts.nSize > 100 )
        return Value;

    // make sure fanouts are not duplicated
    for ( i = 0; i < pObj->vFanouts.nSize; i++ )
        for ( k = i + 1; k < pObj->vFanouts.nSize; k++ )
            if ( pObj->vFanouts.pArray[k] == pObj->vFanouts.pArray[i] )
            {
                printf( "Warning: Node %s has", Abc_ObjName(pObj) );
                printf( " duplicated fanout %s.\n", Abc_ObjName(Abc_ObjFanout(pObj,k)) );
            }

    return Value;
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of a net.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckNet( Abc_Ntk_t * pNtk, Abc_Obj_t * pNet )
{
    if ( Abc_ObjFaninNum(pNet) == 0 )
    {
        fprintf( stdout, "NetworkCheck: Net \"%s\" is not driven.\n", Abc_ObjName(pNet) );
        return 0;
    }
    if ( Abc_ObjFaninNum(pNet) > 1 )
    {
        fprintf( stdout, "NetworkCheck: Net \"%s\" has more than one driver.\n", Abc_ObjName(pNet) );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckNode( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode )
{
    // detect internal nodes that do not have nets
    if ( Abc_NtkIsNetlist(pNtk) && Abc_ObjFanoutNum(pNode) == 0 )
    {
        fprintf( stdout, "Node (id = %d) has no net to drive.\n", pNode->Id );
        return 0;
    }
    // the node should have a function assigned unless it is an AIG
    if ( pNode->pData == NULL )
    {
        if ( Abc_ObjIsBarBuf(pNode) )
            return 1;
        fprintf( stdout, "NodeCheck: An internal node \"%s\" does not have a logic function.\n", Abc_ObjNameNet(pNode) );
        return 0;
    }
    // the netlist and SOP logic network should have SOPs
    if ( Abc_NtkHasSop(pNtk) )
    {
        if ( !Abc_SopCheck( (char *)pNode->pData, Abc_ObjFaninNum(pNode) ) )
        {
            fprintf( stdout, "NodeCheck: SOP check for node \"%s\" has failed.\n", Abc_ObjNameNet(pNode) );
            return 0;
        }
    }
    else if ( Abc_NtkHasBdd(pNtk) )
    {
#ifdef ABC_USE_CUDD
        int nSuppSize = Cudd_SupportSize((DdManager *)pNtk->pManFunc, (DdNode *)pNode->pData);
        if ( nSuppSize > Abc_ObjFaninNum(pNode) )
        {
            fprintf( stdout, "NodeCheck: BDD of the node \"%s\" has incorrect support size.\n", Abc_ObjNameNet(pNode) );
            return 0;
        }
#endif
    }
    else if ( !Abc_NtkHasMapping(pNtk) && !Abc_NtkHasBlifMv(pNtk) && !Abc_NtkHasAig(pNtk) )
    {
        assert( 0 );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks the integrity of a latch.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckLatch( Abc_Ntk_t * pNtk, Abc_Obj_t * pLatch )
{
    int Value = 1;
    // check whether the object is a latch
    if ( !Abc_ObjIsLatch(pLatch) )
    {
        fprintf( stdout, "NodeCheck: Latch \"%s\" is in a latch list but is not a latch.\n", Abc_ObjName(pLatch) );
        Value = 0;
    }
    // make sure the latch has a reasonable return value
    if ( (int)(ABC_PTRINT_T)pLatch->pData < ABC_INIT_ZERO || (int)(ABC_PTRINT_T)pLatch->pData > ABC_INIT_DC )
    {
        fprintf( stdout, "NodeCheck: Latch \"%s\" has incorrect reset value (%d).\n", 
            Abc_ObjName(pLatch), (int)(ABC_PTRINT_T)pLatch->pData );
        Value = 0;
    }
    // make sure the latch has only one fanin
    if ( Abc_ObjFaninNum(pLatch) != 1 )
    {
        fprintf( stdout, "NodeCheck: Latch \"%s\" has wrong number (%d) of fanins.\n", Abc_ObjName(pLatch), Abc_ObjFaninNum(pLatch) );
        Value = 0;
    }
    // make sure the latch has only one fanout
    if ( Abc_ObjFanoutNum(pLatch) != 1 )
    {
        fprintf( stdout, "NodeCheck: Latch \"%s\" has wrong number (%d) of fanouts.\n", Abc_ObjName(pLatch), Abc_ObjFanoutNum(pLatch) );
        Value = 0;
    }
    // make sure the latch input has only one fanin
    if ( Abc_ObjFaninNum(Abc_ObjFanin0(pLatch)) != 1 )
    {
        fprintf( stdout, "NodeCheck: Input of latch \"%s\" has wrong number (%d) of fanins.\n", 
            Abc_ObjName(Abc_ObjFanin0(pLatch)), Abc_ObjFaninNum(Abc_ObjFanin0(pLatch)) );
        Value = 0;
    }
    // make sure the latch input has only one fanout
    if ( Abc_ObjFanoutNum(Abc_ObjFanin0(pLatch)) != 1 )
    {
        fprintf( stdout, "NodeCheck: Input of latch \"%s\" has wrong number (%d) of fanouts.\n", 
            Abc_ObjName(Abc_ObjFanin0(pLatch)), Abc_ObjFanoutNum(Abc_ObjFanin0(pLatch)) );
        Value = 0;
    }
    // make sure the latch output has only one fanin
    if ( Abc_ObjFaninNum(Abc_ObjFanout0(pLatch)) != 1 )
    {
        fprintf( stdout, "NodeCheck: Output of latch \"%s\" has wrong number (%d) of fanins.\n", 
            Abc_ObjName(Abc_ObjFanout0(pLatch)), Abc_ObjFaninNum(Abc_ObjFanout0(pLatch)) );
        Value = 0;
    }
    return Value;
}




/**Function*************************************************************

  Synopsis    [Compares the PIs of the two networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkComparePis( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb )
{
    Abc_Obj_t * pObj1;
    int i;
    if ( Abc_NtkPiNum(pNtk1) != Abc_NtkPiNum(pNtk2) )
    {
        printf( "Networks have different number of primary inputs.\n" );
        return 0;
    }
    // for each PI of pNet1 find corresponding PI of pNet2 and reorder them
    Abc_NtkForEachPi( pNtk1, pObj1, i )
    {
        if ( strcmp( Abc_ObjName(pObj1), Abc_ObjName(Abc_NtkPi(pNtk2,i)) ) != 0 )
        {
            printf( "Primary input #%d is different in network 1 ( \"%s\") and in network 2 (\"%s\").\n", 
                i, Abc_ObjName(pObj1), Abc_ObjName(Abc_NtkPi(pNtk2,i)) );
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares the POs of the two networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkComparePos( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb )
{
    Abc_Obj_t * pObj1;
    int i;
    if ( Abc_NtkPoNum(pNtk1) != Abc_NtkPoNum(pNtk2) )
    {
        printf( "Networks have different number of primary outputs.\n" );
        return 0;
    }
    // for each PO of pNet1 find corresponding PO of pNet2 and reorder them
    Abc_NtkForEachPo( pNtk1, pObj1, i )
    {
        if ( strcmp( Abc_ObjName(pObj1), Abc_ObjName(Abc_NtkPo(pNtk2,i)) ) != 0 )
        {
            printf( "Primary output #%d is different in network 1 ( \"%s\") and in network 2 (\"%s\").\n", 
                i, Abc_ObjName(pObj1), Abc_ObjName(Abc_NtkPo(pNtk2,i)) );
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares the latches of the two networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCompareBoxes( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb )
{
    Abc_Obj_t * pObj1;
    int i;
    assert( Abc_NtkHasOnlyLatchBoxes(pNtk1) );
    assert( Abc_NtkHasOnlyLatchBoxes(pNtk2) );
    if ( !fComb )
        return 1;
    if ( Abc_NtkBoxNum(pNtk1) != Abc_NtkBoxNum(pNtk2) )
    {
        printf( "Networks have different number of latches.\n" );
        return 0;
    }
    // for each PI of pNet1 find corresponding PI of pNet2 and reorder them
    Abc_NtkForEachBox( pNtk1, pObj1, i )
    {
        if ( strcmp( Abc_ObjName(Abc_ObjFanout0(pObj1)), Abc_ObjName(Abc_ObjFanout0(Abc_NtkBox(pNtk2,i))) ) != 0 )
        {
            printf( "Box #%d is different in network 1 ( \"%s\") and in network 2 (\"%s\").\n", 
                i, Abc_ObjName(Abc_ObjFanout0(pObj1)), Abc_ObjName(Abc_ObjFanout0(Abc_NtkBox(pNtk2,i))) );
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compares the signals of the networks.]

  Description []
               
  SideEffects [Ordering POs by name is a very bad idea! It destroys
  the natural order of the logic in the circuit.]

  SeeAlso     []

***********************************************************************/
int Abc_NtkCompareSignals( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fOnlyPis, int fComb )
{
    Abc_NtkOrderObjsByName( pNtk1, fComb );
    Abc_NtkOrderObjsByName( pNtk2, fComb );
    if ( !Abc_NtkComparePis( pNtk1, pNtk2, fComb ) )
        return 0;
    if ( !fOnlyPis )
    {
        if ( !Abc_NtkCompareBoxes( pNtk1, pNtk2, fComb ) )
            return 0;
        if ( !Abc_NtkComparePos( pNtk1, pNtk2, fComb ) )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 0 if the network hierachy contains a cycle.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkIsAcyclicHierarchy_rec( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNext;
    Abc_Obj_t * pObj;
    int i;
    // return if visited
    if ( pNtk->fHieVisited )
        return 1;
    pNtk->fHieVisited = 1;
    // return if black box
    if ( Abc_NtkHasBlackbox(pNtk) )
        return 1;
    assert( Abc_NtkIsNetlist(pNtk) );
    // go through all the children networks
    Abc_NtkForEachBox( pNtk, pObj, i )
    {
        if ( Abc_ObjIsLatch(pObj) )
            continue;
        pNtkNext = (Abc_Ntk_t *)pObj->pData;
        assert( pNtkNext != NULL );
        if ( pNtkNext->fHiePath )
            return 0;
        pNtk->fHiePath = 1;
        if ( !Abc_NtkIsAcyclicHierarchy_rec( pNtkNext ) )
            return 0;
        pNtk->fHiePath = 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 0 if the network hierachy contains a cycle.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkIsAcyclicHierarchy( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pTemp;
    int i, RetValue;
    assert( Abc_NtkIsNetlist(pNtk) && pNtk->pDesign );
    // clear the modules
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pTemp, i )
        pTemp->fHieVisited = pTemp->fHiePath = 0;
    // traverse
    pNtk->fHiePath = 1;
    RetValue = Abc_NtkIsAcyclicHierarchy_rec( pNtk );
    pNtk->fHiePath = 0;
    // clear the modules
    Vec_PtrForEachEntry( Abc_Ntk_t *, pNtk->pDesign->vModules, pTemp, i )
        pTemp->fHieVisited = pTemp->fHiePath = 0;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Returns 0 if CI names are repeated.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkNamesCompare( char ** pName1, char ** pName2 )
{
    return strcmp( *pName1, *pName2 );
}

/**Function*************************************************************

  Synopsis    [Returns 0 if CI names are repeated.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckUniqueCiNames( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNames;
    Abc_Obj_t * pObj;
    int i, fRetValue = 1;
    assert( !Abc_NtkIsNetlist(pNtk) );
    vNames = Vec_PtrAlloc( Abc_NtkCiNum(pNtk) );
    Abc_NtkForEachCi( pNtk, pObj, i )
        Vec_PtrPush( vNames, Abc_ObjName(pObj) );
    Vec_PtrSort( vNames, (int (*)())Abc_NtkNamesCompare );
    for ( i = 1; i < Abc_NtkCiNum(pNtk); i++ )
        if ( !strcmp( (const char *)Vec_PtrEntry(vNames,i-1), (const char *)Vec_PtrEntry(vNames,i) ) )
        {
            printf( "Abc_NtkCheck: Repeated CI names: %s and %s.\n", (char*)Vec_PtrEntry(vNames,i-1), (char*)Vec_PtrEntry(vNames,i) );
            fRetValue = 0;
        }
    Vec_PtrFree( vNames );
    return fRetValue;
}

/**Function*************************************************************

  Synopsis    [Returns 0 if CO names are repeated.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckUniqueCoNames( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNames;
    Abc_Obj_t * pObj;
    int i, fRetValue = 1;
    assert( !Abc_NtkIsNetlist(pNtk) );
    vNames = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_PtrPush( vNames, Abc_ObjName(pObj) );
    Vec_PtrSort( vNames, (int (*)())Abc_NtkNamesCompare );
    for ( i = 1; i < Abc_NtkCoNum(pNtk); i++ )
    {
//        printf( "%s\n", Vec_PtrEntry(vNames,i) );
        if ( !strcmp( (const char *)Vec_PtrEntry(vNames,i-1), (const char *)Vec_PtrEntry(vNames,i) ) )
        {
            printf( "Abc_NtkCheck: Repeated CO names: %s and %s.\n", (char*)Vec_PtrEntry(vNames,i-1), (char*)Vec_PtrEntry(vNames,i) );
            fRetValue = 0;
        }
    }
    Vec_PtrFree( vNames );
    return fRetValue;
}

/**Function*************************************************************

  Synopsis    [Returns 0 if there is a pair of CI/CO with the same name and logic in between.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckUniqueCioNames( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pObjCi, * pFanin;
    int i, nCiId, fRetValue = 1;
    assert( !Abc_NtkIsNetlist(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        nCiId = Nm_ManFindIdByNameTwoTypes( pNtk->pManName, Abc_ObjName(pObj), ABC_OBJ_PI, ABC_OBJ_BO );
        if ( nCiId == -1 )
            continue;
        pObjCi = Abc_NtkObj( pNtk, nCiId );
        assert( !strcmp( Abc_ObjName(pObj), Abc_ObjName(pObjCi) ) );
        pFanin = Abc_ObjFanin0(pObj);
        if ( pFanin != pObjCi )
        {
            printf( "Abc_NtkCheck: A CI/CO pair share the name (%s) but do not link directly. The name of the CO fanin is %s.\n", 
                Abc_ObjName(pObj), Abc_ObjName(Abc_ObjFanin0(pObj)) );
            fRetValue = 0;
        }
    }
    return fRetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

