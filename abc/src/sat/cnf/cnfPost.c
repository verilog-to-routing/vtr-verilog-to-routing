/**CFile****************************************************************

  FileName    [cnfPost.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG-to-CNF conversion.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: cnfPost.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cnf.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_ManPostprocess_old( Cnf_Man_t * p )
{
//    extern int Aig_ManLargeCutEval( Aig_Man_t * p, Aig_Obj_t * pRoot, Dar_Cut_t * pCutR, Dar_Cut_t * pCutL, int Leaf );
    int nNew, Gain, nGain = 0, nVars = 0;

    Aig_Obj_t * pObj, * pFan;
    Dar_Cut_t * pCutBest, * pCut;
    int i, k;//, a, b, Counter;
    Aig_ManForEachObj( p->pManAig, pObj, i )
    {
        if ( !Aig_ObjIsNode(pObj) )
            continue;
        if ( pObj->nRefs == 0 )
            continue;
//        pCutBest = Aig_ObjBestCut(pObj);
        pCutBest = NULL;

        Dar_CutForEachLeaf( p->pManAig, pCutBest, pFan, k )
        {
            if ( !Aig_ObjIsNode(pFan) )
                continue;
            assert( pFan->nRefs != 0 );
            if ( pFan->nRefs != 1 )
                continue;
//            pCut = Aig_ObjBestCut(pFan);
            pCut = NULL;
/*
            // find how many common variable they have
            Counter = 0;
            for ( a = 0; a < (int)pCut->nLeaves; a++ )
            {
                for ( b = 0; b < (int)pCutBest->nLeaves; b++ )
                    if ( pCut->pLeaves[a] == pCutBest->pLeaves[b] )
                        break;
                if ( b == (int)pCutBest->nLeaves )
                    continue;
                Counter++;
            }
            printf( "%d ", Counter );
*/
            // find the new truth table after collapsing these two cuts


//            nNew = Aig_ManLargeCutEval( p->pManAig, pObj, pCutBest, pCut, pFan->Id );
            nNew = 0;


//            printf( "%d+%d=%d:%d(%d) ", pCutBest->Cost, pCut->Cost, 
//                pCutBest->Cost+pCut->Cost, nNew, pCutBest->Cost+pCut->Cost-nNew );

            Gain = pCutBest->Value + pCut->Value - nNew;
            if ( Gain > 0 )
            {
                nGain += Gain;
                nVars++;
            }
        }
    }
    printf( "Total gain = %d.  Vars = %d.\n", nGain, nVars );
}

/**Function*************************************************************

  Synopsis    [Transfers cuts of the mapped nodes into internal representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_ManTransferCuts( Cnf_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_MmFlexRestart( p->pMemCuts );
    Aig_ManForEachObj( p->pManAig, pObj, i )
    {
        if ( Aig_ObjIsNode(pObj) && pObj->nRefs > 0 )
            pObj->pData = Cnf_CutCreate( p, pObj );
        else
            pObj->pData = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Transfers cuts of the mapped nodes into internal representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_ManFreeCuts( Cnf_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( p->pManAig, pObj, i )
        if ( pObj->pData )
        {
            Cnf_CutFree( (Cnf_Cut_t *)pObj->pData );
            pObj->pData = NULL;
        }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_ManPostprocess( Cnf_Man_t * p )
{
    Cnf_Cut_t * pCut, * pCutFan, * pCutRes;
    Aig_Obj_t * pObj, * pFan;
    int Order[16], Costs[16];
    int i, k, fChanges;
    Aig_ManForEachNode( p->pManAig, pObj, i )
    {
        if ( pObj->nRefs == 0 )
            continue;
        pCut = Cnf_ObjBestCut(pObj);

        // sort fanins according to their size
        Cnf_CutForEachLeaf( p->pManAig, pCut, pFan, k )
        {
            Order[k] = k;
            Costs[k] = Aig_ObjIsNode(pFan)? Cnf_ObjBestCut(pFan)->Cost : 0;
        }
        // sort the cuts by Weight
        do {
            int Temp;
            fChanges = 0;
            for ( k = 0; k < pCut->nFanins - 1; k++ )
            {
                if ( Costs[Order[k]] <= Costs[Order[k+1]] )
                    continue;
                Temp = Order[k];
                Order[k] = Order[k+1];
                Order[k+1] = Temp;
                fChanges = 1;
            }
        } while ( fChanges );


//        Cnf_CutForEachLeaf( p->pManAig, pCut, pFan, k )
        for ( k = 0; (k < (int)(pCut)->nFanins) && ((pFan) = Aig_ManObj(p->pManAig, (pCut)->pFanins[Order[k]])); k++ )
        {
            if ( !Aig_ObjIsNode(pFan) )
                continue;
            assert( pFan->nRefs != 0 );
            if ( pFan->nRefs != 1 )
                continue;
            pCutFan = Cnf_ObjBestCut(pFan);
            // try composing these two cuts
//            Cnf_CutPrint( pCut );
            pCutRes = Cnf_CutCompose( p, pCut, pCutFan, pFan->Id );
//            Cnf_CutPrint( pCut );
//            printf( "\n" );
            // check if the cost if reduced
            if ( pCutRes == NULL || pCutRes->Cost == 127 || pCutRes->Cost > pCut->Cost + pCutFan->Cost )
            {
                if ( pCutRes )
                    Cnf_CutFree( pCutRes );
                continue;
            }
            // update the cut
            Cnf_ObjSetBestCut( pObj, pCutRes );
            Cnf_ObjSetBestCut( pFan, NULL );
            Cnf_CutUpdateRefs( p, pCut, pCutFan, pCutRes );
            assert( pFan->nRefs == 0 );
            Cnf_CutFree( pCut );
            Cnf_CutFree( pCutFan );
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

