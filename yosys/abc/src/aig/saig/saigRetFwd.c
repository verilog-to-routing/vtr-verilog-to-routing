/**CFile****************************************************************

  FileName    [saigRetFwd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Most-forward retiming.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigRetFwd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline Aig_Obj_t * Aig_ObjFanoutStatic( Aig_Obj_t * pObj, int i )               { return ((Aig_Obj_t **)pObj->pData)[i];             }
static inline void        Aig_ObjSetFanoutStatic( Aig_Obj_t * pObj, Aig_Obj_t * pFan ) { ((Aig_Obj_t **)pObj->pData)[pObj->nRefs++] = pFan; }

#define Aig_ObjForEachFanoutStatic( pObj, pFan, i )                        \
    for ( i = 0; (i < (int)(pObj)->nRefs) && ((pFan) = Aig_ObjFanoutStatic(pObj, i)); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocate static fanout for all nodes in the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t ** Aig_ManStaticFanoutStart( Aig_Man_t * p )
{
    Aig_Obj_t ** ppFanouts, * pObj;
    int i, nFanouts, nFanoutsAlloc;
    // allocate fanouts
    nFanoutsAlloc = 2 * Aig_ManObjNumMax(p) - Aig_ManCiNum(p) - Aig_ManCoNum(p);
    ppFanouts = ABC_ALLOC( Aig_Obj_t *, nFanoutsAlloc );
    // mark up storage
    nFanouts = 0;
    Aig_ManForEachObj( p, pObj, i )
    {
        pObj->pData = ppFanouts + nFanouts;
        nFanouts += pObj->nRefs;  
        pObj->nRefs = 0;
    }
    assert( nFanouts < nFanoutsAlloc ); 
    // add fanouts
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjChild0(pObj) )
            Aig_ObjSetFanoutStatic( Aig_ObjFanin0(pObj), pObj );
        if ( Aig_ObjChild1(pObj) )
            Aig_ObjSetFanoutStatic( Aig_ObjFanin1(pObj), pObj );
    }
    return ppFanouts;
}

/**Function*************************************************************

  Synopsis    [Marks the objects reachable from the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManMarkAutonomous_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pFanout;
    int i;
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    Aig_ObjForEachFanoutStatic( pObj, pFanout, i )
        Aig_ManMarkAutonomous_rec( p, pFanout );
}

/**Function*************************************************************

  Synopsis    [Marks with current trav ID nodes reachable from Const1 and PIs.]

  Description [Returns the number of unreachable registers.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManMarkAutonomous( Aig_Man_t * p )
{
    Aig_Obj_t ** ppFanouts;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i;
    // temporarily connect register outputs to register inputs
    Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
    {
        pObjLo->pFanin0 = pObjLi;
        pObjLi->nRefs = 1;
    }
    // mark nodes reachable from Const1 and PIs
    Aig_ManIncrementTravId( p );
    ppFanouts = Aig_ManStaticFanoutStart( p );
    Aig_ManMarkAutonomous_rec( p, Aig_ManConst1(p) );
    Saig_ManForEachPi( p, pObj, i )
        Aig_ManMarkAutonomous_rec( p, pObj );
    ABC_FREE( ppFanouts );
    // disconnect LIs/LOs and label unreachable registers
    Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
    {
        assert( pObjLo->pFanin0 && pObjLi->nRefs == 1 );
        pObjLo->pFanin0 = NULL;
        pObjLi->nRefs = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Derives the cut for forward retiming.]

  Description [Assumes topological ordering of the nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManRetimeForwardOne( Aig_Man_t * p, int * pnRegFixed, int * pnRegMoves )
{
    Aig_Man_t * pNew;
    Vec_Ptr_t * vCut;
    Aig_Obj_t * pObj, * pFanin;
    int i;
    // mark the retimable nodes
    Saig_ManMarkAutonomous( p );
    // mark the retimable registers with the fresh trav ID
    Aig_ManIncrementTravId( p );
    *pnRegFixed = 0;
    Saig_ManForEachLo( p, pObj, i )
        if ( Aig_ObjIsTravIdPrevious(p, pObj) )
            Aig_ObjSetTravIdCurrent(p, pObj);
        else
            (*pnRegFixed)++;
    // mark all the nodes that can be retimed forward
    *pnRegMoves = 0;
    Aig_ManForEachNode( p, pObj, i )
        if ( Aig_ObjIsTravIdCurrent(p, Aig_ObjFanin0(pObj)) && Aig_ObjIsTravIdCurrent(p, Aig_ObjFanin1(pObj)) )
        {
            Aig_ObjSetTravIdCurrent(p, pObj);
            (*pnRegMoves)++;
        }
    // mark the remaining registers
    Saig_ManForEachLo( p, pObj, i )
        Aig_ObjSetTravIdCurrent(p, pObj);
    // find the cut (all such marked objects that fanout into unmarked nodes)
    vCut = Vec_PtrAlloc( 1000 );
    Aig_ManIncrementTravId( p );
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjIsTravIdPrevious(p, pObj) )
            continue;
        pFanin = Aig_ObjFanin0(pObj);
        if ( pFanin && Aig_ObjIsTravIdPrevious(p, pFanin) )
        {
            Vec_PtrPush( vCut, pFanin );
            Aig_ObjSetTravIdCurrent( p, pFanin );
        }
        pFanin = Aig_ObjFanin1(pObj);
        if ( pFanin && Aig_ObjIsTravIdPrevious(p, pFanin) )
        {
            Vec_PtrPush( vCut, pFanin );
            Aig_ObjSetTravIdCurrent( p, pFanin );
        }
    }
    // finally derive the new manager
    pNew = Saig_ManRetimeDupForward( p, vCut );
    Vec_PtrFree( vCut );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Derives the cut for forward retiming.]

  Description [Assumes topological ordering of the nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManRetimeForward( Aig_Man_t * p, int nMaxIters, int fVerbose )
{
    Aig_Man_t * pNew, * pTemp;
    int i, nRegFixed, nRegMoves = 1;
    abctime clk;
    pNew = p;
    for ( i = 0; i < nMaxIters && nRegMoves > 0; i++ )
    {
        clk = Abc_Clock();
        pNew = Saig_ManRetimeForwardOne( pTemp = pNew, &nRegFixed, &nRegMoves );
        if ( fVerbose )
        {
            printf( "%2d : And = %6d. Reg = %5d. Unret = %5d. Move = %6d. ", 
                i + 1, Aig_ManNodeNum(pTemp), Aig_ManRegNum(pTemp), nRegFixed, nRegMoves );
            ABC_PRT( "Time", Abc_Clock() - clk );
        }
        if ( pTemp != p )
            Aig_ManStop( pTemp );
    }
    clk = Abc_Clock();
    pNew = Aig_ManReduceLaches( pNew, fVerbose );
    if ( fVerbose )
    {
        ABC_PRT( "Register sharing time", Abc_Clock() - clk );
    }
    return pNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

