/**CFile****************************************************************

  FileName    [amapOutput.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [Core mapping procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapOutput.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline char * Amap_OuputStrsav( Aig_MmFlex_t * p, char * pStr ) 
{ return pStr ? strcpy(Aig_MmFlexEntryFetch(p, strlen(pStr)+1), pStr) : NULL; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates structure for storing one gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Out_t * Amap_OutputStructAlloc( Aig_MmFlex_t * pMem, Amap_Gat_t * pGate )
{
    Amap_Out_t * pRes;
    int nFans = pGate? pGate->nPins : 1;
    pRes = (Amap_Out_t *)Aig_MmFlexEntryFetch( pMem, sizeof(Amap_Out_t)+sizeof(int)*nFans );
    memset( pRes, 0, sizeof(Amap_Out_t) );
    memset( pRes->pFans, 0xff, sizeof(int)*nFans );
    pRes->pName = pGate? Amap_OuputStrsav( pMem, pGate->pName ) : NULL;
    pRes->nFans = nFans;
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Returns mapped network as an array of structures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Amap_ManProduceMapped( Amap_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_MmFlex_t * pMem;
    Amap_Obj_t * pObj, * pFanin;
    Amap_Gat_t * pGate;
    Amap_Out_t * pRes;
    int i, k, iFanin, fCompl;
    float TotalArea = 0.0;
    pMem = Aig_MmFlexStart();
    // create mapping object for each node used in the mapping
    vNodes = Vec_PtrAlloc( 10 );
    Amap_ManForEachObj( p, pObj, i )
    {
        if ( Amap_ObjIsPi(pObj) )
        {
            assert( pObj->fPolar == 0 );
            pRes = Amap_OutputStructAlloc( pMem, NULL );
            pRes->Type  = -1;
            pRes->nFans = 0;
            // save this structure
            pObj->iData = Vec_PtrSize( vNodes );
            Vec_PtrPush( vNodes, pRes );
            // create invertor if needed
            if ( pObj->nFouts[1] ) // this PI is used in the neg polarity
            {
                pRes = Amap_OutputStructAlloc( pMem, p->pLib->pGateInv );
                pRes->pFans[0] = pObj->iData;
                // save this structure
                Vec_PtrPush( vNodes, pRes );
                TotalArea += p->pLib->pGateInv->dArea;
            }
            continue;
        }
        if ( Amap_ObjIsNode(pObj) )
        {
            // skip the node that is not used in the mapping
            if ( Amap_ObjRefsTotal(pObj) == 0 )
                continue;
            // get the gate
            pGate = Amap_LibGate( p->pLib, pObj->Best.pSet->iGate );
            assert( pGate->nPins == pObj->Best.pCut->nFans );
            // allocate structure
            pRes = Amap_OutputStructAlloc( pMem, pGate );
            Amap_MatchForEachFaninCompl( p, &pObj->Best, pFanin, fCompl, k )
            {
                assert( Amap_ObjRefsTotal(pFanin) );
                if ( (int)pFanin->fPolar == fCompl )
                    pRes->pFans[k] = pFanin->iData;
                else
                    pRes->pFans[k] = pFanin->iData + 1;
            }
            // save this structure
            pObj->iData = Vec_PtrSize( vNodes );
            Vec_PtrPush( vNodes, pRes );
            TotalArea += pGate->dArea;
            // create invertor if needed
            if ( pObj->nFouts[!pObj->fPolar] ) // needed in the opposite polarity
            {
                pRes = Amap_OutputStructAlloc( pMem, p->pLib->pGateInv );
                pRes->pFans[0] = pObj->iData;
                // save this structure
                Vec_PtrPush( vNodes, pRes );
                TotalArea += p->pLib->pGateInv->dArea;
            }
            continue;
        }
        if ( Amap_ObjIsPo(pObj) )
        {
            assert( pObj->fPolar == 0 );
            pFanin = Amap_ObjFanin0(p, pObj);
            assert( Amap_ObjRefsTotal(pFanin) );
            if ( Amap_ObjIsConst1(pFanin)  )
            { // create constant node
                if ( Amap_ObjFaninC0(pObj) )
                {
                    pRes = Amap_OutputStructAlloc( pMem, p->pLib->pGate0 );
                    TotalArea += p->pLib->pGate0->dArea;
                }
                else
                {
                    pRes = Amap_OutputStructAlloc( pMem, p->pLib->pGate1 );
                    TotalArea += p->pLib->pGate1->dArea;
                }
                // save this structure
                iFanin = Vec_PtrSize( vNodes );
                Vec_PtrPush( vNodes, pRes );
            }
            else 
            {
                if ( (int)pFanin->fPolar == Amap_ObjFaninC0(pObj) )
                    iFanin = pFanin->iData;
                else
                    iFanin = pFanin->iData + 1;
            }
            // create PO node
            pRes = Amap_OutputStructAlloc( pMem, NULL );
            pRes->Type     = 1;
            pRes->pFans[0] = iFanin;
            // save this structure
            Vec_PtrPush( vNodes, pRes );
        }
    }
    // return memory manager in the last entry of the array
    Vec_PtrPush( vNodes, pMem );
    return vNodes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

