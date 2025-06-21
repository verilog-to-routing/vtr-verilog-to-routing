/**CFile****************************************************************

  FileName    [sfmArea.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [Area optimization.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sfmArea.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sfmInt.h"
#include "map/mio/mio.h"
#include "misc/util/utilTruth.h"
#include "misc/util/utilNam.h"
#include "map/scl/sclLib.h"
#include "map/scl/sclCon.h"
#include "opt/dau/dau.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Precompute cell parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkPrecomputeCellPairs( Mio_Cell2_t * pCells, int nCells )
{
    Vec_Int_t * vInfo = Vec_IntAlloc( 1000 );
    word iBestArea, tCur, iThis;
    int * pPerm[7], nPerms[7], Perm[7], * Perm1, * Perm2;
    int iBestCell, iBestPerm, iBestDiff;
    int i, k, n, v, p, Count = 0;
    int iGate1 = -1, iGate2 = -1;

    for ( i = 1; i <= 6; i++ )
        pPerm[i] = Extra_PermSchedule( i );
    for ( i = 1; i <= 6; i++ )
        nPerms[i] = Extra_Factorial( i );

    for ( i = 2; i < nCells; i++ )
    {
        int nFanins = pCells[i].nFanins;
        for ( n = 0; n <= nFanins; n++ )
        {
            // get the truth table
            iThis = (n == nFanins) ? ~pCells[i].uTruth : Abc_Tt6Flip(pCells[i].uTruth, n);
            // init the comparison
            iBestArea = ~((word)0);
            iBestCell = iBestPerm = iBestDiff = -1;
            // iterate through cells
            for ( k = 2; k < nCells; k++ )
            {
                if ( nFanins != (int)pCells[k].nFanins )
                    continue;
                if ( i != k && pCells[i].uTruth == pCells[k].uTruth )
                {
                    iGate1 = i;
                    iGate2 = k;
                    Count++;
                    continue;
                }
                // set unit permutation
                for ( v = 0; v < nFanins; v++ )
                    Perm[v] = v;
                // go through all permutation of Cell[k]
                tCur = pCells[k].uTruth;
                for ( p = 0; p < nPerms[nFanins]; p++ )
                {
                    if ( iThis == tCur && iBestArea > pCells[k].AreaW ) 
                    {
                        iBestArea = pCells[k].AreaW;
                        iBestCell = k;
                        iBestPerm = 0;
                        for ( v = 0; v < nFanins; v++ )
                            iBestPerm |= (v << (Perm[v] << 2));
                        iBestDiff = (pCells[i].AreaW >= pCells[k].AreaW) ? (int)(pCells[i].AreaW - pCells[k].AreaW) : -(int)(pCells[k].AreaW - pCells[i].AreaW);
                    }
                    if ( nPerms[nFanins] == 1 )
                        continue;
                    // update
                    tCur  = Abc_Tt6SwapAdjacent( tCur, pPerm[nFanins][p] );
                    Perm1 = Perm + pPerm[nFanins][p];
                    Perm2 = Perm1 + 1;
                    ABC_SWAP( int, *Perm1, *Perm2 );
                }
                assert( tCur == pCells[k].uTruth );
            }
            Vec_IntPushThree( vInfo, iBestCell, iBestPerm, iBestDiff );
        }
    }

    for ( i = 1; i <= 6; i++ )
        ABC_FREE( pPerm[i] );
    if ( Count )
        printf( "In this library, %d cell pairs have equal functions (for example, %s and %s).\n", Count/2, pCells[iGate1].pName, pCells[iGate2].pName );
    return vInfo;
}
Vec_Int_t * Abc_NtkPrecomputeFirsts( Mio_Cell2_t * pCells, int nCells )
{
    int i, Index = 0;
    Vec_Int_t * vFirst = Vec_IntStartFull( 2 ); 
    for ( i = 2; i < nCells; i++ )
    {
        Vec_IntPush( vFirst, Index );
        Index += 3 * (pCells[i].nFanins + 1);
    }
    assert( nCells == Vec_IntSize(vFirst) );
    return vFirst;
}
int Abc_NtkPrecomputePrint( Mio_Cell2_t * pCells, int nCells, Vec_Int_t * vInfo )
{
    int i, n, v, Index = 0, nRecUsed = 0;
    for ( i = 2; i < nCells; i++ )
    {
        int nFanins = pCells[i].nFanins;
        printf( "%3d : %8s   Fanins = %d   ", i, pCells[i].pName, nFanins );
        Dau_DsdPrintFromTruth( &pCells[i].uTruth, nFanins );
        for ( n = 0; n <= nFanins; n++, Index += 3 )
        {
            int iCellA = Vec_IntEntry( vInfo, Index+0 );
            int iPerm  = Vec_IntEntry( vInfo, Index+1 );
            int Diff   = Vec_IntEntry( vInfo, Index+2 );
            if ( iCellA == -1 )
                continue;
            printf( "%d : {", n );
            for ( v = 0; v < nFanins; v++ )
                printf( " %d ", (iPerm >> (v << 2)) & 15 );
            printf( "}  Index = %d  ", Index );

            printf( "Gain = %6.2f  ", Scl_Int2Flt(Diff) );
            Dau_DsdPrintFromTruth( &pCells[iCellA].uTruth, pCells[iCellA].nFanins );
            nRecUsed++;
        }
    }
    return nRecUsed;
}

void Abc_NtkPrecomputeCellPairsTest()
{
    int nCells;
    Mio_Cell2_t * pCells = Mio_CollectRootsNewDefault2( 6, &nCells, 0 );
    Vec_Int_t * vInfo = Abc_NtkPrecomputeCellPairs( pCells, nCells );
    int nRecUsed = Abc_NtkPrecomputePrint( pCells, nCells, vInfo );
    // iterate through the cells
    Vec_Int_t * vFirst = Abc_NtkPrecomputeFirsts( pCells, nCells );
    printf( "Used records = %d.  All records = %d.\n", nRecUsed, Vec_IntSize(vInfo)/3 - nRecUsed );
    assert( nCells == Vec_IntSize(vFirst) );
    Vec_IntFree( vFirst );
    Vec_IntFree( vInfo );
    ABC_FREE( pCells );
}

int Abc_NodeCheckFanoutHasFanin( Abc_Obj_t * pNode, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pThis;
    int i;
    Abc_ObjForEachFanout( pNode, pThis, i )
        if ( Abc_NodeFindFanin(pThis, pFanin) >= 0 )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Evaluate changes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjHasDupFanins( Abc_Obj_t * pObj )
{
    int * pArray = pObj->vFanins.pArray;
    int i, k, Limit = Abc_ObjFaninNum(pObj);
    for ( i = 0; i < Limit; i++ )
        for ( k = i+1; k < Limit; k++ )
            if ( pArray[i] == pArray[k] )
                return 1;
    return 0;
}
int Abc_ObjHasDupFanouts( Abc_Obj_t * pObj )
{
    int * pArray = pObj->vFanouts.pArray;
    int i, k, Limit = Abc_ObjFanoutNum(pObj);
    for ( i = 0; i < Limit; i++ )
        for ( k = i+1; k < Limit; k++ )
            if ( pArray[i] == pArray[k] )
                return 1;
    return 0;
}
int Abc_ObjChangeEval( Abc_Obj_t * pObj, Vec_Int_t * vInfo, Vec_Int_t * vFirst, int InvArea, int * pfUseInv )
{
    Abc_Obj_t * pNext;
    //Mio_Gate_t * pGate = (Mio_Gate_t *)pObj->pData;
    int iFanCell, iNodeCell = Mio_GateReadCell( (Mio_Gate_t *)pObj->pData );
    int * pFanInfo, * pNodeInfo = Vec_IntEntryP( vInfo, Vec_IntEntry(vFirst, iNodeCell) );
    int i, fNeedInv = 0, Gain = 0, iFanin = Abc_ObjFaninNum(pObj), fUseInv = Abc_NodeIsInv(pObj);
    assert( iFanin > 0 );
    *pfUseInv = 0;
    if ( pNodeInfo[3*iFanin] == -1 )
        return 0;
    if ( fUseInv )
        Gain = InvArea;
    else
        Gain = pNodeInfo[3*iFanin+2];
    Abc_ObjForEachFanout( pObj, pNext, i )
    {
        if ( fUseInv && Abc_NodeFindFanin(pNext, Abc_ObjFanin0(pObj)) >= 0 )
            return 0;
        if ( Abc_ObjHasDupFanins(pNext) )
            return 0;
        if ( !Abc_ObjIsNode(pNext) || Abc_NodeIsBuf(pNext) )
        {
            fNeedInv = 1;
            continue;
        }
        if ( Abc_NodeIsInv(pNext) )
        {
            if ( Abc_NodeCheckFanoutHasFanin(pNext, pObj) >= 0 )
                return 0;
            Gain += InvArea;
            continue;
        }
        iFanCell = Mio_GateReadCell( (Mio_Gate_t *)pNext->pData );
        pFanInfo = Vec_IntEntryP( vInfo, Vec_IntEntry(vFirst, iFanCell) );
        iFanin   = Abc_NodeFindFanin( pNext, pObj );
        if ( pFanInfo[3*iFanin] == -1 )
        {
            fNeedInv = 1;
            continue;
        }
        Gain += pFanInfo[3*iFanin+2];
    }
    if ( fNeedInv )
        Gain -= InvArea;
    *pfUseInv = fNeedInv;
    return Gain;
}
void Abc_ObjChangeUpdate( Abc_Obj_t * pObj, int iFanin, Mio_Cell2_t * pCells, int * pNodeInfo, Vec_Int_t * vTemp )
{
    int v, Perm, iNodeCell = pNodeInfo[3*iFanin];
    //Mio_Gate_t * pGate = (Mio_Gate_t *)pObj->pData;
    //Abc_ObjPrint( stdout, pObj );
    //printf( "Replacing fanout %d with %s by %s with fanin %d.\n", Abc_ObjId(pObj), Mio_GateReadName(pGate), Mio_GateReadName((Mio_Gate_t *)pCells[iNodeCell].pMioGate), iFanin );
    pObj->pData = (Mio_Gate_t *)pCells[iNodeCell].pMioGate;
    Perm = pNodeInfo[3*iFanin+1];
    Vec_IntClear( vTemp );
    for ( v = 0; v < Abc_ObjFaninNum(pObj); v++ )
        Vec_IntPush( vTemp, Abc_ObjFaninId(pObj, (Perm >> (v << 2)) & 15) );
    Vec_IntClear( &pObj->vFanins );
    Vec_IntAppend( &pObj->vFanins, vTemp );
}
void Abc_ObjChangePerform( Abc_Obj_t * pObj, Vec_Int_t * vInfo, Vec_Int_t * vFirst, int fUseInv, Vec_Int_t * vTemp, Vec_Ptr_t * vFanout, Vec_Ptr_t * vFanout2, Mio_Cell2_t * pCells )
{
    Abc_Obj_t * pNext, * pNext2, * pNodeInv = NULL;
    int iFanCell, iNodeCell = Mio_GateReadCell( (Mio_Gate_t *)pObj->pData );
    int * pFanInfo, * pNodeInfo = Vec_IntEntryP( vInfo, Vec_IntEntry(vFirst, iNodeCell) );
    int i, k, iFanin = Abc_ObjFaninNum(pObj);
    assert( iFanin > 0 && pNodeInfo[3*iFanin] != -1 );
    // update the node
    Abc_NodeCollectFanouts( pObj, vFanout );
    if ( Abc_NodeIsInv(pObj) )
    {
        Abc_Obj_t * pFanin = Abc_ObjFanin0(pObj);
        Vec_PtrForEachEntry( Abc_Obj_t *, vFanout, pNext, k )
            Abc_ObjPatchFanin( pNext, pObj, pFanin );
        assert( Abc_ObjFanoutNum(pObj) == 0 );
        Abc_NtkDeleteObj(pObj);
        pObj = pFanin;
//        assert( fUseInv == 0 );
    }
    else
        Abc_ObjChangeUpdate( pObj, iFanin, pCells, pNodeInfo, vTemp );
    // add inverter if needed
    if ( fUseInv )
        pNodeInv = Abc_NtkCreateNodeInv(pObj->pNtk, pObj);
    // update the fanouts
    Vec_PtrForEachEntry( Abc_Obj_t *, vFanout, pNext, i )
    {
        if ( !Abc_ObjIsNode(pNext) || Abc_NodeIsBuf(pNext) )
        {
            Abc_ObjPatchFanin( pNext, pObj, pNodeInv );
            continue;
        }
        if ( Abc_NodeIsInv(pNext) )
        {
            Abc_NodeCollectFanouts( pNext, vFanout2 );
            Vec_PtrForEachEntry( Abc_Obj_t *, vFanout2, pNext2, k )
                Abc_ObjPatchFanin( pNext2, pNext, pObj );
            assert( Abc_ObjFanoutNum(pNext) == 0 );
            Abc_NtkDeleteObj(pNext);
            continue;
        }
        iFanin   = Abc_NodeFindFanin( pNext, pObj );
        iFanCell = Mio_GateReadCell( (Mio_Gate_t *)pNext->pData );
        pFanInfo = Vec_IntEntryP( vInfo, Vec_IntEntry(vFirst, iFanCell) );
        if ( pFanInfo[3*iFanin] == -1 )
        {
            Abc_ObjPatchFanin( pNext, pObj, pNodeInv );
            continue;
        }
        Abc_ObjChangeUpdate( pNext, iFanin, pCells, pFanInfo, vTemp );
    }
}
void Abc_NtkChangePerform( Abc_Ntk_t * pNtk, int fVerbose )
{
    abctime clk = Abc_Clock();
    int i, fNeedInv, nCells, Gain, GainAll = 0, Count = 0, CountInv = 0;
    Mio_Cell2_t * pCells = Mio_CollectRootsNewDefault2( 6, &nCells, 0 );
    Vec_Int_t * vInfo = Abc_NtkPrecomputeCellPairs( pCells, nCells );
    Vec_Int_t * vFirst = Abc_NtkPrecomputeFirsts( pCells, nCells );

    Vec_Ptr_t * vFanout = Vec_PtrAlloc( 100 );
    Vec_Ptr_t * vFanout2 = Vec_PtrAlloc( 100 );
    Vec_Int_t * vTemp = Vec_IntAlloc( 100 );
    Abc_Obj_t * pObj;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( Abc_ObjFaninNum(pObj) < 2 && !Abc_NodeIsInv(pObj) )
            continue;
        if ( Abc_ObjHasDupFanouts(pObj) )
            continue;
        Gain = Abc_ObjChangeEval( pObj, vInfo, vFirst, (int)pCells[3].AreaW, &fNeedInv );
        if ( Gain <= 0 )
            continue;
        //printf( "Obj %d\n", Abc_ObjId(pObj) );
        Count++;
        CountInv += Abc_NodeIsInv(pObj);
        GainAll += Gain;
        Abc_ObjChangePerform( pObj, vInfo, vFirst, fNeedInv, vTemp, vFanout, vFanout2, pCells );
    }
    Vec_PtrFree( vFanout2 );
    Vec_PtrFree( vFanout );
    Vec_IntFree( vTemp );

    Vec_IntFree( vFirst );
    Vec_IntFree( vInfo );
    ABC_FREE( pCells );

    if ( fVerbose )
        printf( "Total gain in area = %6.2f after %d changes (including %d inverters). ", Scl_Int2Flt(GainAll), Count, CountInv );
    if ( fVerbose )
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

