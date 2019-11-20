/**CFile****************************************************************

  FileName    [abcMini.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface to the minimalistic AIG package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcMini.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Hop_Man_t * Abc_NtkToMini( Abc_Ntk_t * pNtk );
static Abc_Ntk_t * Abc_NtkFromMini( Abc_Ntk_t * pNtkOld, Hop_Man_t * pMan );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Gives the current ABC network to AIG manager for processing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkMiniBalance( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkAig;
    Hop_Man_t * pMan, * pTemp;
    assert( Abc_NtkIsStrash(pNtk) );
    // convert to the AIG manager
    pMan = Abc_NtkToMini( pNtk );
    if ( pMan == NULL )
        return NULL;
    if ( !Hop_ManCheck( pMan ) )
    {
        printf( "AIG check has failed.\n" );
        Hop_ManStop( pMan );
        return NULL;
    }
    // perform balance
    Hop_ManPrintStats( pMan );
//    Hop_ManDumpBlif( pMan, "aig_temp.blif" );
    pMan = Hop_ManBalance( pTemp = pMan, 1 );
    Hop_ManStop( pTemp );
    Hop_ManPrintStats( pMan );
    // convert from the AIG manager
    pNtkAig = Abc_NtkFromMini( pNtk, pMan );
    if ( pNtkAig == NULL )
        return NULL;
    Hop_ManStop( pMan );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_NtkStrash: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Man_t * Abc_NtkToMini( Abc_Ntk_t * pNtk )
{
    Hop_Man_t * pMan;
    Abc_Obj_t * pObj;
    int i;
    // create the manager
    pMan = Hop_ManStart();
    // transfer the pointers to the basic nodes
    Abc_AigConst1(pNtk)->pCopy = (Abc_Obj_t *)Hop_ManConst1(pMan);
    Abc_NtkForEachCi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Hop_ObjCreatePi(pMan);
    // perform the conversion of the internal nodes (assumes DFS ordering)
    Abc_NtkForEachNode( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Hop_And( pMan, (Hop_Obj_t *)Abc_ObjChild0Copy(pObj), (Hop_Obj_t *)Abc_ObjChild1Copy(pObj) );
    // create the POs
    Abc_NtkForEachCo( pNtk, pObj, i )
        Hop_ObjCreatePo( pMan, (Hop_Obj_t *)Abc_ObjChild0Copy(pObj) );
    Hop_ManCleanup( pMan );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Converts the network from the AIG manager into ABC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFromMini( Abc_Ntk_t * pNtk, Hop_Man_t * pMan )
{
    Vec_Ptr_t * vNodes;
    Abc_Ntk_t * pNtkNew;
    Hop_Obj_t * pObj;
    int i;
    // perform strashing
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // transfer the pointers to the basic nodes
    Hop_ManConst1(pMan)->pData = Abc_AigConst1(pNtkNew);
    Hop_ManForEachPi( pMan, pObj, i )
        pObj->pData = Abc_NtkCi(pNtkNew, i);
    // rebuild the AIG
    vNodes = Hop_ManDfs( pMan );
    Vec_PtrForEachEntry( vNodes, pObj, i )
        pObj->pData = Abc_AigAnd( pNtkNew->pManFunc, (Abc_Obj_t *)Hop_ObjChild0Copy(pObj), (Abc_Obj_t *)Hop_ObjChild1Copy(pObj) );
    Vec_PtrFree( vNodes );
    // connect the PO nodes
    Hop_ManForEachPo( pMan, pObj, i )
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), (Abc_Obj_t *)Hop_ObjChild0Copy(pObj) );
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkFromMini(): Network check has failed.\n" );
    return pNtkNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


