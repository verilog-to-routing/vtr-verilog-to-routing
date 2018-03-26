/**CFile****************************************************************

  FileName    [aigRetF.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Retiming frontier.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigRetF.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Mark the nodes reachable from the PIs in the reverse order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManRetimeMark_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    if ( pObj->fMarkB )
        return 1;
    if ( Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
        return 0;
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return pObj->fMarkB;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ManRetimeMark_rec( p, Aig_ObjFanin0(pObj) ) )
        return pObj->fMarkB = 1;
    if ( Aig_ObjIsNode(pObj) && Aig_ManRetimeMark_rec( p, Aig_ObjFanin1(pObj) ) )
        return pObj->fMarkB = 1;
    assert( pObj->fMarkB == 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Mark the nodes reachable from the true PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManRetimeMark( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int fChange, i;
    // mark the PIs
    Aig_ManForEachObj( p, pObj, i )
        assert( pObj->fMarkB == 0 );
    Aig_ManForEachPiSeq( p, pObj, i )
        pObj->fMarkB = 1;
    // map registers into each other
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
    {
        assert( pObjLo->pNext == NULL );
        assert( pObjLi->pNext == NULL );
        pObjLo->pNext = pObjLi;
        pObjLi->pNext = pObjLo;
    }
    // iterativively mark the logic reachable from PIs
    fChange = 1;
    while ( fChange )
    {
        fChange = 0;
        Aig_ManIncrementTravId( p );
        Aig_ManForEachCo( p, pObj, i )
        {
            if ( pObj->fMarkB )
                continue;
            if ( Aig_ManRetimeMark_rec( p, pObj ) )
            {
                if ( pObj->pNext )
                    pObj->pNext->fMarkB = 1;
                fChange = 1;
            }
        }
    }
    // clean register mapping
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        pObjLo->pNext = pObjLi->pNext = NULL;
}


/**Function*************************************************************

  Synopsis    [Performs forward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManRetimeFrontier( Aig_Man_t * p, int nStepsMax )
{
    Aig_Obj_t * pObj, * pObjNew, * pObjLo, * pObjLo0, * pObjLo1, * pObjLi, * pObjLi0, * pObjLi1;//, * pObjLi0_, * pObjLi1_, * pObjLi0__, * pObjLi1__;
    int i, Counter, fCompl, fChange;
    assert( Aig_ManRegNum(p) > 0 );
    // remove structural hashing table
    Aig_TableClear( p );
    // mark the retimable nodes
    Aig_ManRetimeMark( p );
    // mark the register outputs
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
    {
        pObjLo->fMarkA = 1;
        pObjLo->pNext = pObjLi;
        pObjLi->pNext = pObjLo;
    }
    // go through the nodes and find retimable ones
    Counter = 0;
    fChange = 1;
    while ( fChange )
    {
        fChange = 0;
        Aig_ManForEachNode( p, pObj, i )
        {
            if ( !pObj->fMarkB )
                continue;
            if ( Aig_ObjIsBuf(pObj) )
                continue;
            // get the real inputs of the node (skipping the buffers)
            pObjLo0 = Aig_ObjReal_rec( Aig_ObjChild0(pObj) );
            pObjLo1 = Aig_ObjReal_rec( Aig_ObjChild1(pObj) );
            if ( !Aig_Regular(pObjLo0)->fMarkA || !Aig_Regular(pObjLo1)->fMarkA )
                continue;
            // remember complemented attribute
            fCompl = Aig_IsComplement(pObjLo0) & Aig_IsComplement(pObjLo1);
            // get the register inputs
//            pObjLi0_ = Aig_Regular(pObjLo0)->pNext;
//            pObjLi1_ = Aig_Regular(pObjLo1)->pNext;
//            pObjLi0__ = Aig_ObjChild0(Aig_Regular(pObjLo0)->pNext);
//            pObjLi1__ = Aig_ObjChild0(Aig_Regular(pObjLo1)->pNext);
            pObjLi0 = Aig_NotCond( Aig_ObjChild0(Aig_Regular(pObjLo0)->pNext), Aig_IsComplement(pObjLo0) );
            pObjLi1 = Aig_NotCond( Aig_ObjChild0(Aig_Regular(pObjLo1)->pNext), Aig_IsComplement(pObjLo1) );
            // create new node
            pObjNew = Aig_And( p, pObjLi0, pObjLi1 );
            pObjNew->fMarkB = 1;
            // create new register
            pObjLo = Aig_ObjCreateCi(p);
            pObjLo->fMarkA = 1;
            pObjLi = Aig_ObjCreateCo( p, Aig_NotCond(pObjNew, fCompl) );
            p->nRegs++;
            pObjLo->pNext = pObjLi;
            pObjLi->pNext = pObjLo;
            // add the buffer
            Aig_ObjDisconnect( p, pObj );
            pObj->Type = AIG_OBJ_BUF;
            p->nObjs[AIG_OBJ_AND]--;
            p->nObjs[AIG_OBJ_BUF]++;
            Aig_ObjConnect( p, pObj, Aig_NotCond(pObjLo, fCompl), NULL );
            // create HAIG if defined
            // mark the change
            fChange = 1;
            // check the limit
            if ( ++Counter >= nStepsMax )
            {
                fChange = 0;
                break;
            }
        }
    }
    // clean the markings
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
    {
        pObjLo->fMarkA = 0;
        pObjLo->pNext = pObjLi->pNext = NULL;
    }
    Aig_ManForEachObj( p, pObj, i )
        pObj->fMarkB = 0;
    // remove useless registers
    Aig_ManSeqCleanup( p );
    // rehash the nodes
    return Aig_ManDupOrdered( p ); 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

