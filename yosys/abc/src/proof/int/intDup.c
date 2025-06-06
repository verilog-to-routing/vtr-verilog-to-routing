/**CFile****************************************************************

  FileName    [intDup.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Specialized AIG duplication procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 24, 2008.]

  Revision    [$Id: intDup.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "intInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create trivial AIG manager for the init state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Inter_ManStartInitState( int nRegs )
{
    Aig_Man_t * p;
    Aig_Obj_t * pRes;
    Aig_Obj_t ** ppInputs;
    int i;
    assert( nRegs > 0 );
    ppInputs = ABC_ALLOC( Aig_Obj_t *, nRegs );
    p = Aig_ManStart( nRegs );
    for ( i = 0; i < nRegs; i++ )
        ppInputs[i] = Aig_Not( Aig_ObjCreateCi(p) );
    pRes = Aig_Multi( p, ppInputs, nRegs, AIG_OBJ_AND );
    Aig_ObjCreateCo( p, pRes );
    ABC_FREE( ppInputs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicate the AIG w/o POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Inter_ManStartDuplicated( Aig_Man_t * p )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i;
    assert( Aig_ManRegNum(p) > 0 );
    // create the new manager
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // create the PIs
    Aig_ManCleanData( p );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    // set registers
    pNew->nTruePis = p->nTruePis;
    pNew->nTruePos = Saig_ManConstrNum(p);
    pNew->nRegs    = p->nRegs;
    // duplicate internal nodes
    Aig_ManForEachNode( p, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );

    // create constraint outputs
    Saig_ManForEachPo( p, pObj, i )
    {
        if ( i < Saig_ManPoNum(p)-Saig_ManConstrNum(p) )
            continue;
        Aig_ObjCreateCo( pNew, Aig_Not( Aig_ObjChild0Copy(pObj) ) );
    }

    // create register inputs with MUXes
    Saig_ManForEachLi( p, pObj, i )
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    Aig_ManCleanup( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicate the AIG w/o POs and transforms to transit into init state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Inter_ManStartOneOutput( Aig_Man_t * p, int fAddFirstPo )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    Aig_Obj_t * pCtrl = NULL; // Suppress "might be used uninitialized"
    int i;
    assert( Aig_ManRegNum(p) > 0 );
    // create the new manager
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // create the PIs
    Aig_ManCleanData( p );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p, pObj, i )
    {
        if ( i == Saig_ManPiNum(p) )
            pCtrl = Aig_ObjCreateCi( pNew );
        pObj->pData = Aig_ObjCreateCi( pNew );
    }
    // set registers
    pNew->nRegs    = fAddFirstPo? 0 : p->nRegs;
    pNew->nTruePis = fAddFirstPo? Aig_ManCiNum(p) + 1 : p->nTruePis + 1;
    pNew->nTruePos = fAddFirstPo + Saig_ManConstrNum(p);
    // duplicate internal nodes
    Aig_ManForEachNode( p, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );

    // create constraint outputs
    Saig_ManForEachPo( p, pObj, i )
    {
        if ( i < Saig_ManPoNum(p)-Saig_ManConstrNum(p) )
            continue;
        Aig_ObjCreateCo( pNew, Aig_Not( Aig_ObjChild0Copy(pObj) ) );
    }

    // add the PO
    if ( fAddFirstPo )
    {
        pObj = Aig_ManCo( p, 0 );
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    else
    {
        // create register inputs with MUXes
        Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
        {
            pObj = Aig_Mux( pNew, pCtrl, (Aig_Obj_t *)pObjLo->pData, Aig_ObjChild0Copy(pObjLi) );
    //        pObj = Aig_Mux( pNew, pCtrl, Aig_ManConst0(pNew), Aig_ObjChild0Copy(pObjLi) );
            Aig_ObjCreateCo( pNew, pObj );
        }
    }
    Aig_ManCleanup( pNew );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

