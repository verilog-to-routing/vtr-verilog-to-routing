/**CFile****************************************************************

  FileName    [aigFrames.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Performs timeframe expansion of the AIG.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigFrames.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline Aig_Obj_t * Aig_ObjFrames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i )                       { return pObjMap[nFs*pObj->Id + i];  }
static inline void        Aig_ObjSetFrames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i, Aig_Obj_t * pNode ) { pObjMap[nFs*pObj->Id + i] = pNode; }

static inline Aig_Obj_t * Aig_ObjChild0Frames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i ) { return Aig_ObjFanin0(pObj)? Aig_NotCond(Aig_ObjFrames(pObjMap,nFs,Aig_ObjFanin0(pObj),i), Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t * Aig_ObjChild1Frames( Aig_Obj_t ** pObjMap, int nFs, Aig_Obj_t * pObj, int i ) { return Aig_ObjFanin1(pObj)? Aig_NotCond(Aig_ObjFrames(pObjMap,nFs,Aig_ObjFanin1(pObj),i), Aig_ObjFaninC1(pObj)) : NULL;  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs timeframe expansion of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManFrames( Aig_Man_t * pAig, int nFs, int fInit, int fOuts, int fRegs, int fEnlarge, Aig_Obj_t *** ppObjMap )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pObjNew;
    Aig_Obj_t ** pObjMap;
    int i, f;

    // create mapping for the frames nodes
    pObjMap  = ABC_ALLOC( Aig_Obj_t *, nFs * Aig_ManObjNumMax(pAig) );
    memset( pObjMap, 0, sizeof(Aig_Obj_t *) * nFs * Aig_ManObjNumMax(pAig) );

    // start the fraig package
    pFrames = Aig_ManStart( Aig_ManObjNumMax(pAig) * nFs );
    pFrames->pName = Abc_UtilStrsav( pAig->pName );
    pFrames->pSpec = Abc_UtilStrsav( pAig->pSpec );
    // map constant nodes
    for ( f = 0; f < nFs; f++ )
        Aig_ObjSetFrames( pObjMap, nFs, Aig_ManConst1(pAig), f, Aig_ManConst1(pFrames) );
    // create PI nodes for the frames
    for ( f = 0; f < nFs; f++ )
        Aig_ManForEachPiSeq( pAig, pObj, i )
            Aig_ObjSetFrames( pObjMap, nFs, pObj, f, Aig_ObjCreateCi(pFrames) );
    // set initial state for the latches
    if ( fInit )
    {
        Aig_ManForEachLoSeq( pAig, pObj, i )
            Aig_ObjSetFrames( pObjMap, nFs, pObj, 0, Aig_ManConst0(pFrames) );
    }
    else 
    {
        Aig_ManForEachLoSeq( pAig, pObj, i )
            Aig_ObjSetFrames( pObjMap, nFs, pObj, 0, Aig_ObjCreateCi(pFrames) );
    }

    // add timeframes
    for ( f = 0; f < nFs; f++ )
    {
//        printf( "Frame = %d.\n", f );
        // add internal nodes of this frame
        Aig_ManForEachNode( pAig, pObj, i )
        {
//            Aig_Obj_t * pFanin0 = Aig_ObjChild0Frames(pObjMap,nFs,pObj,f);
//            Aig_Obj_t * pFanin1 = Aig_ObjChild1Frames(pObjMap,nFs,pObj,f);
//            printf( "Node = %3d.  Fanin0 = %3d. Fanin1 = %3d.\n", pObj->Id, Aig_Regular(pFanin0)->Id, Aig_Regular(pFanin1)->Id );
            pObjNew = Aig_And( pFrames, Aig_ObjChild0Frames(pObjMap,nFs,pObj,f), Aig_ObjChild1Frames(pObjMap,nFs,pObj,f) );
            Aig_ObjSetFrames( pObjMap, nFs, pObj, f, pObjNew );
        }
        // set the latch inputs and copy them into the latch outputs of the next frame
        Aig_ManForEachLiLoSeq( pAig, pObjLi, pObjLo, i )
        {
            pObjNew = Aig_ObjChild0Frames(pObjMap,nFs,pObjLi,f);
            if ( f < nFs - 1 )
                Aig_ObjSetFrames( pObjMap, nFs, pObjLo, f+1, pObjNew );
        }
    }
    if ( fOuts )
    {
        for ( f = fEnlarge?nFs-1:0; f < nFs; f++ )
            Aig_ManForEachPoSeq( pAig, pObj, i )
            {
                pObjNew = Aig_ObjCreateCo( pFrames, Aig_ObjChild0Frames(pObjMap,nFs,pObj,f) );
                Aig_ObjSetFrames( pObjMap, nFs, pObj, f, pObjNew );
            }
    }
    if ( fRegs )
    {
        pFrames->nRegs = pAig->nRegs;
        Aig_ManForEachLiSeq( pAig, pObj, i )
        {
            pObjNew = Aig_ObjCreateCo( pFrames, Aig_ObjChild0Frames(pObjMap,nFs,pObj,fEnlarge?0:nFs-1) );
            Aig_ObjSetFrames( pObjMap, nFs, pObj, nFs-1, pObjNew );
        }
        Aig_ManSetRegNum( pFrames, Aig_ManRegNum(pAig) );
    }
    Aig_ManCleanup( pFrames );
    // return the new manager
    if ( ppObjMap )
        *ppObjMap = pObjMap;
    else
        ABC_FREE( pObjMap );
    return pFrames;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

