/**CFile****************************************************************

  FileName    [resFilter.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resynthesis package.]

  Synopsis    [Filtering resubstitution candidates.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 15, 2007.]

  Revision    [$Id: resFilter.c,v 1.00 2007/01/15 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "resInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static unsigned * Res_FilterCollectFaninInfo( Res_Win_t * pWin, Res_Sim_t * pSim, unsigned uMask );
static int        Res_FilterCriticalFanin( Abc_Obj_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Finds sets of feasible candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
int Res_FilterCandidates( Res_Win_t * pWin, Abc_Ntk_t * pAig, Res_Sim_t * pSim, Vec_Vec_t * vResubs, Vec_Vec_t * vResubsW, int nFaninsMax, int fArea )
{
    Abc_Obj_t * pFanin, * pFanin2, * pFaninTemp;
    unsigned * pInfo, * pInfoDiv, * pInfoDiv2;
    int Counter, RetValue, i, i2, d, d2, iDiv, iDiv2, k;

    // check that the info the node is one
    pInfo = (unsigned *)Vec_PtrEntry( pSim->vOuts, 1 );
    RetValue = Abc_InfoIsOne( pInfo, pSim->nWordsOut );
    if ( RetValue == 0 )
    {
//        printf( "Failed 1!\n" );
        return 0;
    }

    // collect the fanin info
    pInfo = Res_FilterCollectFaninInfo( pWin, pSim, ~0 );
    RetValue = Abc_InfoIsOne( pInfo, pSim->nWordsOut );
    if ( RetValue == 0 )
    {
//        printf( "Failed 2!\n" );
        return 0;
    }

    // try removing each fanin
//    printf( "Fanins: " );
    Counter = 0;
    Vec_VecClear( vResubs );
    Vec_VecClear( vResubsW );
    Abc_ObjForEachFanin( pWin->pNode, pFanin, i )
    {
        if ( fArea && Abc_ObjFanoutNum(pFanin) > 1 )
            continue;
        // get simulation info without this fanin
        pInfo = Res_FilterCollectFaninInfo( pWin, pSim, ~(1 << i) );
        RetValue = Abc_InfoIsOne( pInfo, pSim->nWordsOut );
        if ( RetValue )
        {
//            printf( "Node %4d. Candidate fanin %4d.\n", pWin->pNode->Id, pFanin->Id );
            // collect the nodes
            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,0) );
            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,1) );
            Abc_ObjForEachFanin( pWin->pNode, pFaninTemp, k )
            {
                if ( k != i )
                {
                    Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,2+k) );
                    Vec_VecPush( vResubsW, Counter, pFaninTemp );
                }
            }
            Counter++;
            if ( Counter == Vec_VecSize(vResubs) )
                return Counter;    
        }
    }

    // try replacing each critical fanin by a non-critical fanin
    Abc_ObjForEachFanin( pWin->pNode, pFanin, i )
    {
        if ( Abc_ObjFanoutNum(pFanin) > 1 )
            continue;
        // get simulation info without this fanin
        pInfo = Res_FilterCollectFaninInfo( pWin, pSim, ~(1 << i) );
        // go over the set of divisors
        for ( d = Abc_ObjFaninNum(pWin->pNode) + 2; d < Abc_NtkPoNum(pAig); d++ )
        {
            pInfoDiv = (unsigned *)Vec_PtrEntry( pSim->vOuts, d );
            iDiv = d - (Abc_ObjFaninNum(pWin->pNode) + 2);
            if ( !Abc_InfoIsOrOne( pInfo, pInfoDiv, pSim->nWordsOut ) )
                continue;
            // collect the nodes
            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,0) );
            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,1) );
            // collect the remaning fanins and the divisor
            Abc_ObjForEachFanin( pWin->pNode, pFaninTemp, k )
            {
                if ( k != i )
                {
                    Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,2+k) );
                    Vec_VecPush( vResubsW, Counter, pFaninTemp );
                }
            }
            // collect the divisor
            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,d) );
            Vec_VecPush( vResubsW, Counter, Vec_PtrEntry(pWin->vDivs, iDiv) );
            Counter++;
            if ( Counter == Vec_VecSize(vResubs) )
                return Counter;           
        }
    }

    // consider the case when two fanins can be added instead of one
    if ( Abc_ObjFaninNum(pWin->pNode) < nFaninsMax )
    {
        // try to replace each critical fanin by two non-critical fanins
        Abc_ObjForEachFanin( pWin->pNode, pFanin, i )
        {
            if ( Abc_ObjFanoutNum(pFanin) > 1 )
                continue;
            // get simulation info without this fanin
            pInfo = Res_FilterCollectFaninInfo( pWin, pSim, ~(1 << i) );
            // go over the set of divisors
            for ( d = Abc_ObjFaninNum(pWin->pNode) + 2; d < Abc_NtkPoNum(pAig); d++ )
            {
                pInfoDiv = (unsigned *)Vec_PtrEntry( pSim->vOuts, d );
                iDiv = d - (Abc_ObjFaninNum(pWin->pNode) + 2);
                // go through the second divisor
                for ( d2 = d + 1; d2 < Abc_NtkPoNum(pAig); d2++ )
                {
                    pInfoDiv2 = (unsigned *)Vec_PtrEntry( pSim->vOuts, d2 );
                    iDiv2 = d2 - (Abc_ObjFaninNum(pWin->pNode) + 2);
                    if ( !Abc_InfoIsOrOne3( pInfo, pInfoDiv, pInfoDiv2, pSim->nWordsOut ) )
                        continue;
                    // collect the nodes
                    Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,0) );
                    Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,1) );
                    // collect the remaning fanins and the divisor
                    Abc_ObjForEachFanin( pWin->pNode, pFaninTemp, k )
                    {
                        if ( k != i )
                        {
                            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,2+k) );
                            Vec_VecPush( vResubsW, Counter, pFaninTemp );
                        }
                    }
                    // collect the divisor
                    Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,d) );
                    Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,d2) );
                    Vec_VecPush( vResubsW, Counter, Vec_PtrEntry(pWin->vDivs, iDiv) );
                    Vec_VecPush( vResubsW, Counter, Vec_PtrEntry(pWin->vDivs, iDiv2) );
                    Counter++;
                    if ( Counter == Vec_VecSize(vResubs) )
                        return Counter;           
                }
            }
        }
    }

    // try to replace two nets by one
    if ( !fArea )
    {
        Abc_ObjForEachFanin( pWin->pNode, pFanin, i )
        {
            for ( i2 = i + 1; i2 < Abc_ObjFaninNum(pWin->pNode); i2++ )
            {
                pFanin2 = Abc_ObjFanin(pWin->pNode, i2);
                // get simulation info without these fanins
                pInfo = Res_FilterCollectFaninInfo( pWin, pSim, (~(1 << i)) & (~(1 << i2)) );
                // go over the set of divisors
                for ( d = Abc_ObjFaninNum(pWin->pNode) + 2; d < Abc_NtkPoNum(pAig); d++ )
                {
                    pInfoDiv = (unsigned *)Vec_PtrEntry( pSim->vOuts, d );
                    iDiv = d - (Abc_ObjFaninNum(pWin->pNode) + 2);
                    if ( !Abc_InfoIsOrOne( pInfo, pInfoDiv, pSim->nWordsOut ) )
                        continue;
                    // collect the nodes
                    Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,0) );
                    Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,1) );
                    // collect the remaning fanins and the divisor
                    Abc_ObjForEachFanin( pWin->pNode, pFaninTemp, k )
                    {
                        if ( k != i && k != i2 )
                        {
                            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,2+k) );
                            Vec_VecPush( vResubsW, Counter, pFaninTemp );
                        }
                    }
                    // collect the divisor
                    Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,d) );
                    Vec_VecPush( vResubsW, Counter, Vec_PtrEntry(pWin->vDivs, iDiv) );
                    Counter++;
                    if ( Counter == Vec_VecSize(vResubs) )
                        return Counter;           
                }
            }
        }
    }
    return Counter;
}


/**Function*************************************************************

  Synopsis    [Finds sets of feasible candidates.]

  Description [This procedure is a special case of the above.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_FilterCandidatesArea( Res_Win_t * pWin, Abc_Ntk_t * pAig, Res_Sim_t * pSim, Vec_Vec_t * vResubs, Vec_Vec_t * vResubsW, int nFaninsMax )
{
    Abc_Obj_t * pFanin;
    unsigned * pInfo, * pInfoDiv, * pInfoDiv2;
    int Counter, RetValue, d, d2, k, iDiv, iDiv2, iBest;

    // check that the info the node is one
    pInfo = (unsigned *)Vec_PtrEntry( pSim->vOuts, 1 );
    RetValue = Abc_InfoIsOne( pInfo, pSim->nWordsOut );
    if ( RetValue == 0 )
    {
//        printf( "Failed 1!\n" );
        return 0;
    }

    // collect the fanin info
    pInfo = Res_FilterCollectFaninInfo( pWin, pSim, ~0 );
    RetValue = Abc_InfoIsOne( pInfo, pSim->nWordsOut );
    if ( RetValue == 0 )
    {
//        printf( "Failed 2!\n" );
        return 0;
    }

    // try removing fanins
//    printf( "Fanins: " );
    Counter = 0;
    Vec_VecClear( vResubs );
    Vec_VecClear( vResubsW );
    // get the best fanins
    iBest = Res_FilterCriticalFanin( pWin->pNode );
    if ( iBest == -1 )
        return 0;

    // get the info without the critical fanin
    pInfo = Res_FilterCollectFaninInfo( pWin, pSim, ~(1 << iBest) );
    RetValue = Abc_InfoIsOne( pInfo, pSim->nWordsOut );
    if ( RetValue )
    {
//        printf( "Can be done without one!\n" );
        // collect the nodes
        Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,0) );
        Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,1) );
        Abc_ObjForEachFanin( pWin->pNode, pFanin, k )
        {
            if ( k != iBest )
            {
                Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,2+k) );
                Vec_VecPush( vResubsW, Counter, pFanin );
            }
        }
        Counter++;
//        printf( "*" );
        return Counter;
    }

    // go through the divisors
    for ( d = Abc_ObjFaninNum(pWin->pNode) + 2; d < Abc_NtkPoNum(pAig); d++ )
    {
        pInfoDiv = (unsigned *)Vec_PtrEntry( pSim->vOuts, d );
        iDiv = d - (Abc_ObjFaninNum(pWin->pNode) + 2);
        if ( !Abc_InfoIsOrOne( pInfo, pInfoDiv, pSim->nWordsOut ) )
            continue;
//if ( Abc_ObjLevel(pWin->pNode) <= Abc_ObjLevel( Vec_PtrEntry(pWin->vDivs, iDiv) ) )
//    printf( "Node level = %d. Divisor level = %d.\n", Abc_ObjLevel(pWin->pNode), Abc_ObjLevel( Vec_PtrEntry(pWin->vDivs, iDiv) ) );
        // collect the nodes
        Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,0) );
        Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,1) );
        // collect the remaning fanins and the divisor
        Abc_ObjForEachFanin( pWin->pNode, pFanin, k )
        {
            if ( k != iBest )
            {
                Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,2+k) );
                Vec_VecPush( vResubsW, Counter, pFanin );
            }
        }
        // collect the divisor
        Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,d) );
        Vec_VecPush( vResubsW, Counter, Vec_PtrEntry(pWin->vDivs, iDiv) );
        Counter++;

        if ( Counter == Vec_VecSize(vResubs) )
            break;            
    }

    if ( Counter > 0 || Abc_ObjFaninNum(pWin->pNode) >= nFaninsMax )
        return Counter;

    // try to find the node pairs
    for ( d = Abc_ObjFaninNum(pWin->pNode) + 2; d < Abc_NtkPoNum(pAig); d++ )
    {
        pInfoDiv = (unsigned *)Vec_PtrEntry( pSim->vOuts, d );
        iDiv = d - (Abc_ObjFaninNum(pWin->pNode) + 2);
        // go through the second divisor
        for ( d2 = d + 1; d2 < Abc_NtkPoNum(pAig); d2++ )
        {
            pInfoDiv2 = (unsigned *)Vec_PtrEntry( pSim->vOuts, d2 );
            iDiv2 = d2 - (Abc_ObjFaninNum(pWin->pNode) + 2);

            if ( !Abc_InfoIsOrOne3( pInfo, pInfoDiv, pInfoDiv2, pSim->nWordsOut ) )
                continue;
            // collect the nodes
            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,0) );
            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,1) );
            // collect the remaning fanins and the divisor
            Abc_ObjForEachFanin( pWin->pNode, pFanin, k )
            {
                if ( k != iBest )
                {
                    Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,2+k) );
                    Vec_VecPush( vResubsW, Counter, pFanin );
                }
            }
            // collect the divisor
            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,d) );
            Vec_VecPush( vResubs, Counter, Abc_NtkPo(pAig,d2) );
            Vec_VecPush( vResubsW, Counter, Vec_PtrEntry(pWin->vDivs, iDiv) );
            Vec_VecPush( vResubsW, Counter, Vec_PtrEntry(pWin->vDivs, iDiv2) );
            Counter++;

            if ( Counter == Vec_VecSize(vResubs) )
                break;            
        }
        if ( Counter == Vec_VecSize(vResubs) )
            break;            
    }
    return Counter;
}


/**Function*************************************************************

  Synopsis    [Finds sets of feasible candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Res_FilterCollectFaninInfo( Res_Win_t * pWin, Res_Sim_t * pSim, unsigned uMask )
{
    Abc_Obj_t * pFanin;
    unsigned * pInfo;
    int i;
    pInfo = (unsigned *)Vec_PtrEntry( pSim->vOuts, 0 );
    Abc_InfoClear( pInfo, pSim->nWordsOut );
    Abc_ObjForEachFanin( pWin->pNode, pFanin, i )
    {
        if ( uMask & (1 << i) )
            Abc_InfoOr( pInfo, (unsigned *)Vec_PtrEntry(pSim->vOuts, 2+i), pSim->nWordsOut );
    }
    return pInfo;
}


/**Function*************************************************************

  Synopsis    [Returns the index of the most critical fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_FilterCriticalFanin( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    int i, iBest = -1, CostMax = 0, CostCur;
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( !Abc_ObjIsNode(pFanin) )
            continue;
        if ( Abc_ObjFanoutNum(pFanin) > 1 )
            continue;
        CostCur = Res_WinVisitMffc( pFanin );
        if ( CostMax < CostCur )
        {
            CostMax = CostCur;
            iBest = i;
        }
    }
//    if ( CostMax > 0 )
//        printf( "<%d>", CostMax );
    return iBest;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

