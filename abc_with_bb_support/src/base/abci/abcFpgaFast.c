/**CFile****************************************************************

  FileName    [abcFpgaFast.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Fast FPGA mapper.]

  Author      [Sungmin Cho]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcFpgaFast.c,v 1.00 2006/09/02 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/ivy/ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Ivy_Man_t * Abc_NtkIvyBefore( Abc_Ntk_t * pNtk, int fSeq, int fUseDc );

static Abc_Ntk_t * Ivy_ManFpgaToAbc( Abc_Ntk_t * pNtk, Ivy_Man_t * pMan );
static Abc_Obj_t * Ivy_ManToAbcFast_rec( Abc_Ntk_t * pNtkNew, Ivy_Man_t * pMan, Ivy_Obj_t * pObjIvy, Vec_Int_t * vNodes );

static inline void        Abc_ObjSetIvy2Abc( Ivy_Man_t * p, int IvyId, Abc_Obj_t * pObjAbc ) {  assert(Vec_PtrEntry((Vec_Ptr_t *)p->pCopy, IvyId) == NULL); assert(!Abc_ObjIsComplement(pObjAbc)); Vec_PtrWriteEntry( (Vec_Ptr_t *)p->pCopy, IvyId, pObjAbc );  }
static inline Abc_Obj_t * Abc_ObjGetIvy2Abc( Ivy_Man_t * p, int IvyId )                      {  return (Abc_Obj_t *)Vec_PtrEntry( (Vec_Ptr_t *)p->pCopy, IvyId );         }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs fast FPGA mapping of the network.]

  Description [Takes the AIG to be mapped, the LUT size, and verbosity
  flag. Produces the new network by fast FPGA mapping of the current 
  network. If the current network in ABC in not an AIG, the user should 
  run command "strash" to make sure that the current network into an AIG 
  before calling this procedure.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkFpgaFast( Abc_Ntk_t * pNtk, int nLutSize, int fRecovery, int fVerbose )
{
    Ivy_Man_t * pMan;
    Abc_Ntk_t * pNtkNew;
    // make sure the network is an AIG
    assert( Abc_NtkIsStrash(pNtk) );
    // convert the network into the AIG
    pMan = Abc_NtkIvyBefore( pNtk, 0, 0 );
    // perform fast FPGA mapping
    Ivy_FastMapPerform( pMan, nLutSize, fRecovery, fVerbose );
    // convert back into the ABC network
    pNtkNew = Ivy_ManFpgaToAbc( pNtk, pMan );
    Ivy_FastMapStop( pMan );
    Ivy_ManStop( pMan );  
    // make sure that the final network passes the test
    if ( pNtkNew != NULL && !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkFastMap: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Constructs the ABC network after mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Ivy_ManFpgaToAbc( Abc_Ntk_t * pNtk, Ivy_Man_t * pMan )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObjAbc, * pObj;
    Ivy_Obj_t * pObjIvy;
    Vec_Int_t * vNodes;
    int i;
    // start mapping from Ivy into Abc
    pMan->pCopy = Vec_PtrStart( Ivy_ManObjIdMax(pMan) + 1 );
    // start the new ABC network
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_AIG );
    // transfer the pointers to the basic nodes
    Abc_ObjSetIvy2Abc( pMan, Ivy_ManConst1(pMan)->Id, Abc_NtkCreateNodeConst1(pNtkNew) );
    Abc_NtkForEachCi( pNtkNew, pObjAbc, i )
        Abc_ObjSetIvy2Abc( pMan, Ivy_ManPi(pMan, i)->Id, pObjAbc ); 
    // recursively construct the network
    vNodes = Vec_IntAlloc( 100 );
    Ivy_ManForEachPo( pMan, pObjIvy, i )
    {
        // get the new ABC node corresponding to the old fanin of the PO in IVY
        pObjAbc = Ivy_ManToAbcFast_rec( pNtkNew, pMan, Ivy_ObjFanin0(pObjIvy), vNodes );
        // consider the case of complemented fanin of the PO
        if ( Ivy_ObjFaninC0(pObjIvy) ) // complement
        {
            if ( Abc_ObjIsCi(pObjAbc) )
                pObjAbc = Abc_NtkCreateNodeInv( pNtkNew, pObjAbc );
            else
            {
                // clone the node
                pObj = Abc_NtkCloneObj( pObjAbc );
                // set complemented functions
                pObj->pData = Hop_Not( (Hop_Obj_t *)pObjAbc->pData );
                // return the new node
                pObjAbc = pObj;
            }
        }
        Abc_ObjAddFanin( Abc_NtkCo(pNtkNew, i), pObjAbc );
    }
    Vec_IntFree( vNodes );
    Vec_PtrFree( (Vec_Ptr_t *)pMan->pCopy ); 
    pMan->pCopy = NULL;
    // remove dangling nodes
    Abc_NtkCleanup( pNtkNew, 0 );
    // fix CIs feeding directly into COs
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Recursively construct the new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Ivy_ManToAbcFast_rec( Abc_Ntk_t * pNtkNew, Ivy_Man_t * pMan, Ivy_Obj_t * pObjIvy, Vec_Int_t * vNodes )
{
    Vec_Int_t Supp, * vSupp = &Supp;
    Abc_Obj_t * pObjAbc, * pFaninAbc;
    Ivy_Obj_t * pNodeIvy;
    int i, Entry;
    // skip the node if it is a constant or already processed
    pObjAbc = Abc_ObjGetIvy2Abc( pMan, pObjIvy->Id );
    if ( pObjAbc )
        return pObjAbc;
    assert( Ivy_ObjIsAnd(pObjIvy) || Ivy_ObjIsExor(pObjIvy) );
    // get the support of K-LUT
    Ivy_FastMapReadSupp( pMan, pObjIvy, vSupp );
    // create new ABC node and its fanins
    pObjAbc = Abc_NtkCreateNode( pNtkNew );
    Vec_IntForEachEntry( vSupp, Entry, i )
    {
        pFaninAbc = Ivy_ManToAbcFast_rec( pNtkNew, pMan, Ivy_ManObj(pMan, Entry), vNodes );
        Abc_ObjAddFanin( pObjAbc, pFaninAbc );
    }
    // collect the nodes used in the cut
    Ivy_ManCollectCut( pMan, pObjIvy, vSupp, vNodes );
    // create the local function
    Ivy_ManForEachNodeVec( pMan, vNodes, pNodeIvy, i )
    {
        if ( i < Vec_IntSize(vSupp) )
            pNodeIvy->pEquiv = (Ivy_Obj_t *)Hop_IthVar( (Hop_Man_t *)pNtkNew->pManFunc, i );
        else
            pNodeIvy->pEquiv = (Ivy_Obj_t *)Hop_And( (Hop_Man_t *)pNtkNew->pManFunc, (Hop_Obj_t *)Ivy_ObjChild0Equiv(pNodeIvy), (Hop_Obj_t *)Ivy_ObjChild1Equiv(pNodeIvy) );
    }
    // set the local function
    pObjAbc->pData = (Abc_Obj_t *)pObjIvy->pEquiv;
    // set the node
    Abc_ObjSetIvy2Abc( pMan, pObjIvy->Id, pObjAbc ); 
    return pObjAbc;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

