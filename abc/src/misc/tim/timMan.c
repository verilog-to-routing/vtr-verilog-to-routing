/**CFile****************************************************************

  FileName    [timMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchy/timing manager.]

  Synopsis    [Manipulation of manager data-structure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: timMan.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "timInt.h"
#include "map/if/if.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Tim_Man_t * Tim_ManStart( int nCis, int nCos )
{
    Tim_Man_t * p;
    Tim_Obj_t * pObj;
    int i;
    p = ABC_ALLOC( Tim_Man_t, 1 );
    memset( p, 0, sizeof(Tim_Man_t) );
    p->pMemObj = Mem_FlexStart();
    p->nCis = nCis;
    p->nCos = nCos;
    p->pCis = ABC_ALLOC( Tim_Obj_t, nCis );
    memset( p->pCis, 0, sizeof(Tim_Obj_t) * nCis );
    p->pCos = ABC_ALLOC( Tim_Obj_t, nCos );
    memset( p->pCos, 0, sizeof(Tim_Obj_t) * nCos );
    Tim_ManForEachCi( p, pObj, i )
    {
        pObj->Id = i;
        pObj->iObj2Box = pObj->iObj2Num = -1;
        pObj->timeReq = TIM_ETERNITY;
    }
    Tim_ManForEachCo( p, pObj, i )
    {
        pObj->Id = i;
        pObj->iObj2Box = pObj->iObj2Num = -1;
        pObj->timeReq = TIM_ETERNITY;
    }
    p->fUseTravId = 1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Tim_Man_t * Tim_ManDup( Tim_Man_t * p, int fUnitDelay )
{
    Tim_Man_t * pNew;
    Tim_Box_t * pBox;
    Tim_Obj_t * pObj;
    float * pDelayTable, * pDelayTableNew;
    int i, k, nInputs, nOutputs;
    // clear traversal IDs
    Tim_ManForEachCi( p, pObj, i ) 
        pObj->TravId = 0;          
    Tim_ManForEachCo( p, pObj, i ) 
        pObj->TravId = 0;          
    // create new manager
    pNew = Tim_ManStart( p->nCis, p->nCos );
    // copy box connectivity information
    memcpy( pNew->pCis, p->pCis, sizeof(Tim_Obj_t) * p->nCis ); 
    memcpy( pNew->pCos, p->pCos, sizeof(Tim_Obj_t) * p->nCos ); 
    if ( fUnitDelay )
    {
        // discretize PI arrival times
//        Tim_ManForEachPi( pNew, pObj, k )
//          pObj->timeArr = (int)pObj->timeArr;
        // discretize PO required times
//        Tim_ManForEachPo( pNew, pObj, k )
//          pObj->timeReq = 1 + (int)pObj->timeReq;
        // clear PI arrival and PO required
        Tim_ManInitPiArrivalAll( p, 0.0 );
        Tim_ManInitPoRequiredAll( p, (float)TIM_ETERNITY );
    }
    // duplicate delay tables
    if ( Tim_ManDelayTableNum(p) > 0 )
    {
        pNew->vDelayTables = Vec_PtrStart( Vec_PtrSize(p->vDelayTables) );
        Tim_ManForEachTable( p, pDelayTable, i )
        {
            if ( pDelayTable == NULL )
                continue;
            assert( i == (int)pDelayTable[0] );
            nInputs   = (int)pDelayTable[1];
            nOutputs  = (int)pDelayTable[2];
            pDelayTableNew = ABC_ALLOC( float, 3 + nInputs * nOutputs );
            pDelayTableNew[0] = (int)pDelayTable[0];
            pDelayTableNew[1] = (int)pDelayTable[1];
            pDelayTableNew[2] = (int)pDelayTable[2];
            for ( k = 0; k < nInputs * nOutputs; k++ )
                if ( pDelayTable[3+k] == -ABC_INFINITY )
                    pDelayTableNew[3+k] = -ABC_INFINITY;
                else
                    pDelayTableNew[3+k] = fUnitDelay ? (float)fUnitDelay : pDelayTable[3+k];
//            assert( (int)pDelayTableNew[0] == Vec_PtrSize(pNew->vDelayTables) );
            assert( Vec_PtrEntry(pNew->vDelayTables, i) == NULL );
            Vec_PtrWriteEntry( pNew->vDelayTables, i, pDelayTableNew );
//printf( "Finished duplicating delay table %d.\n", i );
        }
    }
    // duplicate boxes
    if ( Tim_ManBoxNum(p) > 0 )
    {
        pNew->vBoxes = Vec_PtrAlloc( Tim_ManBoxNum(p) );
        Tim_ManForEachBox( p, pBox, i )
        {
           Tim_ManCreateBox( pNew, pBox->Inouts[0], pBox->nInputs, 
                pBox->Inouts[pBox->nInputs], pBox->nOutputs, pBox->iDelayTable, pBox->fBlack );
           Tim_ManBoxSetCopy( pNew, i, pBox->iCopy );
        }
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Trims the timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Tim_Man_t * Tim_ManTrim( Tim_Man_t * p, Vec_Int_t * vBoxPres )
{
    Tim_Man_t * pNew;
    Tim_Box_t * pBox;
    Tim_Obj_t * pObj;
    float * pDelayTable, * pDelayTableNew;
    int i, k, nNewCis, nNewCos, nInputs, nOutputs;
    assert( Vec_IntSize(vBoxPres) == Tim_ManBoxNum(p) );
    // count the number of CIs and COs in the trimmed manager
    nNewCis = Tim_ManPiNum(p);
    nNewCos = Tim_ManPoNum(p);
    if ( Tim_ManBoxNum(p) )
        Tim_ManForEachBox( p, pBox, i )
            if ( Vec_IntEntry(vBoxPres, i) )
            {
                nNewCis += pBox->nOutputs;
                nNewCos += pBox->nInputs;
            }
    if ( nNewCis == Tim_ManCiNum(p) && nNewCos == Tim_ManCoNum(p) )
        return Tim_ManDup( p, 0 );
    assert( nNewCis < Tim_ManCiNum(p) );
    assert( nNewCos < Tim_ManCoNum(p) );
    // clear traversal IDs
    Tim_ManForEachCi( p, pObj, i ) 
        pObj->TravId = 0;          
    Tim_ManForEachCo( p, pObj, i ) 
        pObj->TravId = 0;          
    // create new manager
    pNew = Tim_ManStart( nNewCis, nNewCos );
    // copy box connectivity information
    memcpy( pNew->pCis, p->pCis, sizeof(Tim_Obj_t) * Tim_ManPiNum(p) );
    memcpy( pNew->pCos + nNewCos - Tim_ManPoNum(p), 
            p->pCos + Tim_ManCoNum(p) - Tim_ManPoNum(p), 
            sizeof(Tim_Obj_t) * Tim_ManPoNum(p) );
    // duplicate delay tables
    if ( Tim_ManDelayTableNum(p) > 0 )
    {
        pNew->vDelayTables = Vec_PtrStart( Vec_PtrSize(p->vDelayTables) );
        Tim_ManForEachTable( p, pDelayTable, i )
        {
            if ( pDelayTable == NULL )
                continue;
            assert( i == (int)pDelayTable[0] );
            nInputs   = (int)pDelayTable[1];
            nOutputs  = (int)pDelayTable[2];
            pDelayTableNew = ABC_ALLOC( float, 3 + nInputs * nOutputs );
            pDelayTableNew[0] = (int)pDelayTable[0];
            pDelayTableNew[1] = (int)pDelayTable[1];
            pDelayTableNew[2] = (int)pDelayTable[2];
            for ( k = 0; k < nInputs * nOutputs; k++ )
                pDelayTableNew[3+k] = pDelayTable[3+k];
//            assert( (int)pDelayTableNew[0] == Vec_PtrSize(pNew->vDelayTables) );
            assert( Vec_PtrEntry(pNew->vDelayTables, i) == NULL );
            Vec_PtrWriteEntry( pNew->vDelayTables, i, pDelayTableNew );
        }
    }
    // duplicate boxes
    if ( Tim_ManBoxNum(p) > 0 )
    {
        int curPi = Tim_ManPiNum(p);
        int curPo = 0;
        pNew->vBoxes = Vec_PtrAlloc( Tim_ManBoxNum(p) );
        Tim_ManForEachBox( p, pBox, i )
            if ( Vec_IntEntry(vBoxPres, i) )
            {
                Tim_ManCreateBox( pNew, curPo, pBox->nInputs, curPi, pBox->nOutputs, pBox->iDelayTable, pBox->fBlack );
                Tim_ManBoxSetCopy( pNew, Tim_ManBoxNum(pNew) - 1, Tim_ManBoxCopy(p, i) == -1 ? i : Tim_ManBoxCopy(p, i) );
                curPi += pBox->nOutputs;
                curPo += pBox->nInputs;
            }
        curPo += Tim_ManPoNum(p);
        assert( curPi == Tim_ManCiNum(pNew) );
        assert( curPo == Tim_ManCoNum(pNew) );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Reduces the timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Tim_Man_t * Tim_ManReduce( Tim_Man_t * p, Vec_Int_t * vBoxesLeft, int nTermsDiff )
{
    Tim_Man_t * pNew;
    Tim_Box_t * pBox;
    Tim_Obj_t * pObj;
    float * pDelayTable, * pDelayTableNew;
    int i, k, iBox, nNewCis, nNewCos, nInputs, nOutputs;
    int nNewPiNum = Tim_ManPiNum(p) - nTermsDiff;
    int nNewPoNum = Tim_ManPoNum(p) - nTermsDiff;
    assert( Vec_IntSize(vBoxesLeft) <= Tim_ManBoxNum(p) );
    // count the number of CIs and COs in the trimmed manager
    nNewCis = nNewPiNum;
    nNewCos = nNewPoNum;
    Vec_IntForEachEntry( vBoxesLeft, iBox, i )
    {
        pBox = Tim_ManBox( p, iBox );
        nNewCis += pBox->nOutputs;
        nNewCos += pBox->nInputs;
    }
    assert( nNewCis <= Tim_ManCiNum(p) - nTermsDiff );
    assert( nNewCos <= Tim_ManCoNum(p) - nTermsDiff );
    // clear traversal IDs
    Tim_ManForEachCi( p, pObj, i ) 
        pObj->TravId = 0;          
    Tim_ManForEachCo( p, pObj, i ) 
        pObj->TravId = 0;          
    // create new manager
    pNew = Tim_ManStart( nNewCis, nNewCos );
    // copy box connectivity information
    memcpy( pNew->pCis, p->pCis, sizeof(Tim_Obj_t) * nNewPiNum );
    memcpy( pNew->pCos + nNewCos - nNewPoNum, 
            p->pCos + Tim_ManCoNum(p) - Tim_ManPoNum(p), 
            sizeof(Tim_Obj_t) * nNewPoNum );
    // duplicate delay tables
    if ( Tim_ManDelayTableNum(p) > 0 )
    {
        int fWarning = 0;
        pNew->vDelayTables = Vec_PtrStart( Vec_PtrSize(p->vDelayTables) );
        Tim_ManForEachTable( p, pDelayTable, i )
        {
            if ( pDelayTable == NULL )
                continue;
            if ( i != (int)pDelayTable[0] && fWarning == 0 )
            {
                printf( "Warning: Mismatch in delay-table number between the manager and the box.\n" );
                fWarning = 1;
            }
            //assert( i == (int)pDelayTable[0] );
            nInputs   = (int)pDelayTable[1];
            nOutputs  = (int)pDelayTable[2];
            pDelayTableNew = ABC_ALLOC( float, 3 + nInputs * nOutputs );
            pDelayTableNew[0] = i;//(int)pDelayTable[0];
            pDelayTableNew[1] = (int)pDelayTable[1];
            pDelayTableNew[2] = (int)pDelayTable[2];
            for ( k = 0; k < nInputs * nOutputs; k++ )
                pDelayTableNew[3+k] = pDelayTable[3+k];
//            assert( (int)pDelayTableNew[0] == Vec_PtrSize(pNew->vDelayTables) );
            assert( Vec_PtrEntry(pNew->vDelayTables, i) == NULL );
            Vec_PtrWriteEntry( pNew->vDelayTables, i, pDelayTableNew );
        }
    }
    // duplicate boxes
    if ( Tim_ManBoxNum(p) > 0 )
    {
        int curPi = nNewPiNum;
        int curPo = 0;
        pNew->vBoxes = Vec_PtrAlloc( Tim_ManBoxNum(p) );
        Vec_IntForEachEntry( vBoxesLeft, iBox, i )
        {
            pBox = Tim_ManBox( p, iBox );
            Tim_ManCreateBox( pNew, curPo, pBox->nInputs, curPi, pBox->nOutputs, pBox->iDelayTable, pBox->fBlack );
            Tim_ManBoxSetCopy( pNew, Tim_ManBoxNum(pNew) - 1, Tim_ManBoxCopy(p, iBox) == -1 ? iBox : Tim_ManBoxCopy(p, iBox) );
            curPi += pBox->nOutputs;
            curPo += pBox->nInputs;
        }
        curPo += nNewPoNum;
        assert( curPi == Tim_ManCiNum(pNew) );
        assert( curPo == Tim_ManCoNum(pNew) );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Aligns two sets of boxes using the copy field.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Tim_ManAlignTwo( Tim_Man_t * pSpec, Tim_Man_t * pImpl )
{
    Vec_Int_t * vBoxPres;
    Tim_Box_t * pBox;
    int i;
    assert( Tim_ManBoxNum(pSpec) > Tim_ManBoxNum(pImpl) );
    // check if boxes of pImpl can be aligned
    Tim_ManForEachBox( pImpl, pBox, i )
        if ( pBox->iCopy < 0 || pBox->iCopy >= Tim_ManBoxNum(pSpec) ) 
            return NULL;
    // map dropped boxes into 1, others into 0
    vBoxPres = Vec_IntStart( Tim_ManBoxNum(pSpec) );
    Tim_ManForEachBox( pImpl, pBox, i )
    {
        assert( !Vec_IntEntry(vBoxPres, pBox->iCopy) );
        Vec_IntWriteEntry( vBoxPres, pBox->iCopy, 1 );
    }
    return vBoxPres;
}

/**Function*************************************************************

  Synopsis    [Stops the timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManStop( Tim_Man_t * p )
{
    Vec_PtrFreeFree( p->vDelayTables );
    Vec_PtrFreeP( &p->vBoxes );
    Mem_FlexStop( p->pMemObj, 0 );
    ABC_FREE( p->pCis );
    ABC_FREE( p->pCos );
    ABC_FREE( p );
}
void Tim_ManStopP( Tim_Man_t ** p )
{
    if ( *p == NULL )
        return;
    Tim_ManStop( *p );
    *p = NULL;
}

/**Function*************************************************************

  Synopsis    [Creates manager using hierarchy / box library / delay info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManCreate( Tim_Man_t * p, void * pLib, Vec_Flt_t * vInArrs, Vec_Flt_t * vOutReqs )
{
    If_LibBox_t * pLibBox = (If_LibBox_t *)pLib;
    If_Box_t * pIfBox;
    Tim_Box_t * pBox;
    Tim_Obj_t * pObj;
    float * pTable;
    int i, k;
    assert( p->vDelayTables == NULL );
    p->vDelayTables = pLibBox ? Vec_PtrStart( Vec_PtrSize(pLibBox->vBoxes) ) : Vec_PtrAlloc( 100 );
    if ( p->vBoxes )
    Tim_ManForEachBox( p, pBox, i )
    {
        if ( pBox->iDelayTable == -1 || pLibBox == NULL )
        {
            // create table with constants
            pTable = ABC_ALLOC( float, 3 + pBox->nInputs * pBox->nOutputs );
            pTable[0] = pBox->iDelayTable;
            pTable[1] = pBox->nInputs;
            pTable[2] = pBox->nOutputs;
            for ( k = 0; k < pBox->nInputs * pBox->nOutputs; k++ )
                pTable[3 + k] = 1.0;
            // save table
            pBox->iDelayTable = Vec_PtrSize(p->vDelayTables);
            Vec_PtrPush( p->vDelayTables, pTable );
            continue;
        }
        assert( pBox->iDelayTable >= 0 && pBox->iDelayTable < Vec_PtrSize(pLibBox->vBoxes) );
        pIfBox = (If_Box_t *)Vec_PtrEntry( pLibBox->vBoxes, pBox->iDelayTable );
        assert( pIfBox != NULL );
        assert( pIfBox->nPis == pBox->nInputs );
        assert( pIfBox->nPos == pBox->nOutputs );
        pBox->fBlack = pIfBox->fBlack;
        if ( Vec_PtrEntry( p->vDelayTables, pBox->iDelayTable ) != NULL )
            continue;
        // create table of boxes
        pTable = ABC_ALLOC( float, 3 + pBox->nInputs * pBox->nOutputs );
        pTable[0] = pBox->iDelayTable;
        pTable[1] = pBox->nInputs;
        pTable[2] = pBox->nOutputs;
        for ( k = 0; k < pBox->nInputs * pBox->nOutputs; k++ )
            pTable[3 + k] = pIfBox->pDelays[k];
        // save table
        Vec_PtrWriteEntry( p->vDelayTables, pBox->iDelayTable, pTable );
    }
    // create arrival times
    if ( vInArrs )
    {
        assert( Vec_FltSize(vInArrs) == Tim_ManPiNum(p) );
        Tim_ManForEachPi( p, pObj, i )
            pObj->timeArr = Vec_FltEntry(vInArrs, i);

    }
    // create required times
    if ( vOutReqs )
    {
        k = 0;
        assert( Vec_FltSize(vOutReqs) == Tim_ManPoNum(p) );
        Tim_ManForEachPo( p, pObj, i )
            pObj->timeReq = Vec_FltEntry(vOutReqs, k++);
        assert( k == Tim_ManPoNum(p) );
    }
}


/**Function*************************************************************

  Synopsis    [Get arrival and required times if they are non-trivial.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float * Tim_ManGetArrTimes( Tim_Man_t * p )
{
    float * pTimes;
    Tim_Obj_t * pObj;
    int i;
    Tim_ManForEachPi( p, pObj, i )
        if ( pObj->timeArr != 0.0 )
            break;
    if ( i == Tim_ManPiNum(p) )
        return NULL;
    pTimes  = ABC_FALLOC( float, Tim_ManCiNum(p) );
    Tim_ManForEachPi( p, pObj, i )
        pTimes[i] = pObj->timeArr;
    return pTimes;
}
float * Tim_ManGetReqTimes( Tim_Man_t * p )
{
    float * pTimes;
    Tim_Obj_t * pObj;
    int i, k = 0;
    Tim_ManForEachPo( p, pObj, i )
        if ( pObj->timeReq != TIM_ETERNITY )
            break;
    if ( i == Tim_ManPoNum(p) )
        return NULL;
    pTimes  = ABC_FALLOC( float, Tim_ManCoNum(p) );
    Tim_ManForEachPo( p, pObj, i )
        pTimes[k++] = pObj->timeArr;
    assert( k == Tim_ManPoNum(p) );
    return pTimes;
}


/**Function*************************************************************

  Synopsis    [Prints the timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManPrint( Tim_Man_t * p )
{
    Tim_Box_t * pBox;
    Tim_Obj_t * pObj, * pPrev;
    float * pTable;
    int i, j, k, TableX, TableY;
    if ( p == NULL )
        return;
    printf( "TIMING MANAGER:\n" );
    printf( "PI = %d. CI = %d. PO = %d. CO = %d. Box = %d.\n", 
        Tim_ManPiNum(p), Tim_ManCiNum(p), Tim_ManPoNum(p), Tim_ManCoNum(p), Tim_ManBoxNum(p) );

    // print CI info
    pPrev = p->pCis;
    Tim_ManForEachPi( p, pObj, i )
        if ( pPrev->timeArr != pObj->timeArr || pPrev->timeReq != pObj->timeReq )
            break;
    if ( i == Tim_ManCiNum(p) )
        printf( "All PIs     :  arrival = %5.3f  required = %5.3f\n", pPrev->timeArr, pPrev->timeReq );
    else
        Tim_ManForEachPi( p, pObj, i )
            printf( "PI%5d     :  arrival = %5.3f  required = %5.3f\n", i, pObj->timeArr, pObj->timeReq );

    // print CO info
    pPrev = p->pCos;
    Tim_ManForEachPo( p, pObj, i )
        if ( pPrev->timeArr != pObj->timeArr || pPrev->timeReq != pObj->timeReq )
            break;
    if ( i == Tim_ManCoNum(p) )
        printf( "All POs     :  arrival = %5.3f  required = %5.3f\n", pPrev->timeArr, pPrev->timeReq );
    else
    {
        int k = 0;
        Tim_ManForEachPo( p, pObj, i )
            printf( "PO%5d     :  arrival = %5.3f  required = %5.3f\n", k++, pObj->timeArr, pObj->timeReq );
    }

    // print box info
    if ( Tim_ManBoxNum(p) > 0 )
    Tim_ManForEachBox( p, pBox, i )
    {
        printf( "*** Box %5d :  I =%4d. O =%4d. I1 =%6d. O1 =%6d. Table =%4d\n", 
            i, pBox->nInputs, pBox->nOutputs, 
            Tim_ManBoxInputFirst(p, i), Tim_ManBoxOutputFirst(p, i), 
            pBox->iDelayTable );

        // print box inputs
        pPrev = Tim_ManBoxInput( p, pBox, 0 );
        Tim_ManBoxForEachInput( p, pBox, pObj, k )
            if ( pPrev->timeArr != pObj->timeArr || pPrev->timeReq != pObj->timeReq )
                break;
        if ( k == Tim_ManBoxInputNum(p, pBox->iBox) )
            printf( "Box inputs  :  arrival = %5.3f  required = %5.3f\n", pPrev->timeArr, pPrev->timeReq );
        else
            Tim_ManBoxForEachInput( p, pBox, pObj, k )
                printf( "box-in%4d :  arrival = %5.3f  required = %5.3f\n", k, pObj->timeArr, pObj->timeReq );

        // print box outputs
        pPrev = Tim_ManBoxOutput( p, pBox, 0 );
        Tim_ManBoxForEachOutput( p, pBox, pObj, k )
            if ( pPrev->timeArr != pObj->timeArr || pPrev->timeReq != pObj->timeReq )
                break;
        if ( k == Tim_ManBoxOutputNum(p, pBox->iBox) )
            printf( "Box outputs :  arrival = %5.3f  required = %5.3f\n", pPrev->timeArr, pPrev->timeReq );
        else
            Tim_ManBoxForEachOutput( p, pBox, pObj, k )
                printf( "box-out%3d :  arrival = %5.3f  required = %5.3f\n", k, pObj->timeArr, pObj->timeReq );

        if ( i > 2 )
            break;
    }

    // print delay tables
    if ( Tim_ManDelayTableNum(p) > 0 )
    Tim_ManForEachTable( p, pTable, i )
    {
        if ( pTable == NULL )
            continue;
        printf( "Delay table %d:\n", i );
        assert( i == (int)pTable[0] );
        TableX = (int)pTable[1];
        TableY = (int)pTable[2];
        for ( j = 0; j < TableY; j++, printf( "\n" ) )
            for ( k = 0; k < TableX; k++ )
                if ( pTable[3+j*TableX+k] == -ABC_INFINITY )
                    printf( "%5s", "-" );
                else
                    printf( "%5.0f", pTable[3+j*TableX+k] );
    }
    printf( "\n" );
}
void Tim_ManPrintBoxCopy( Tim_Man_t * p )
{
    Tim_Box_t * pBox;
    int i;
    if ( p == NULL )
        return;
    printf( "TIMING MANAGER:\n" );
    printf( "PI = %d. CI = %d. PO = %d. CO = %d. Box = %d.\n", 
        Tim_ManPiNum(p), Tim_ManCiNum(p), Tim_ManPoNum(p), Tim_ManCoNum(p), Tim_ManBoxNum(p) );

    if ( Tim_ManBoxNum(p) > 0 )
    Tim_ManForEachBox( p, pBox, i )
        printf( "%d ", pBox->iCopy );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints statistics of the timing manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManPrintStats( Tim_Man_t * p, int nAnd2Delay )
{
    Tim_Box_t * pBox;
    Vec_Int_t * vCounts;
    Vec_Ptr_t * vBoxes;
    int i, Count, IdMax;
    if ( p == NULL )
        return;
    Abc_Print( 1, "Hierarchy      :  " );
    printf( "PI/CI = %d/%d   PO/CO = %d/%d   Box = %d   ", 
        Tim_ManPiNum(p), Tim_ManCiNum(p), 
        Tim_ManPoNum(p), Tim_ManCoNum(p), 
        Tim_ManBoxNum(p) );
    if ( nAnd2Delay )
        printf( "delay(AND2) = %d", nAnd2Delay );
    printf( "\n" );
    if ( Tim_ManBoxNum(p) == 0 )
        return;
    IdMax = 0;
    Tim_ManForEachBox( p, pBox, i )
        IdMax = Abc_MaxInt( IdMax, pBox->iDelayTable );
    vCounts = Vec_IntStart( IdMax+1 );
    vBoxes  = Vec_PtrStart( IdMax+1 );
    Tim_ManForEachBox( p, pBox, i )
    {
        Vec_IntAddToEntry( vCounts, pBox->iDelayTable, 1 );
        Vec_PtrWriteEntry( vBoxes, pBox->iDelayTable, pBox );
    }
    // print statistics about boxes
    Vec_IntForEachEntry( vCounts, Count, i )
    {
        if ( Count == 0 ) continue;
        pBox = (Tim_Box_t *)Vec_PtrEntry( vBoxes, i );
        printf( "    Box %4d      ", i );
        printf( "Num = %4d   ", Count );
        printf( "Ins = %4d   ", pBox->nInputs );
        printf( "Outs = %4d",   pBox->nOutputs );
        printf( "\n" );
    }
    Vec_IntFree( vCounts );
    Vec_PtrFree( vBoxes );
}

/**Function*************************************************************

  Synopsis    [Read parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManCiNum( Tim_Man_t * p )
{
    return p->nCis;
}
int Tim_ManCoNum( Tim_Man_t * p )
{
    return p->nCos;
}
int Tim_ManPiNum( Tim_Man_t * p )
{
    if ( Tim_ManBoxNum(p) == 0 )
        return Tim_ManCiNum(p);
    return Tim_ManBoxOutputFirst(p, 0);
}
int Tim_ManPoNum( Tim_Man_t * p )
{
    int iLastBoxId;
    if ( Tim_ManBoxNum(p) == 0 )
        return Tim_ManCoNum(p);
    iLastBoxId = Tim_ManBoxNum(p) - 1;
    return Tim_ManCoNum(p) - (Tim_ManBoxInputFirst(p, iLastBoxId) + Tim_ManBoxInputNum(p, iLastBoxId));
}
int Tim_ManBoxNum( Tim_Man_t * p )
{
    return p->vBoxes ? Vec_PtrSize(p->vBoxes) : 0;
}
int Tim_ManBlackBoxNum( Tim_Man_t * p )
{
    Tim_Box_t * pBox;
    int i, Counter = 0;
    if ( Tim_ManBoxNum(p) )
        Tim_ManForEachBox( p, pBox, i )
            Counter += pBox->fBlack;
    return Counter;
}
void Tim_ManBlackBoxIoNum( Tim_Man_t * p, int * pnBbIns, int * pnBbOuts )
{
    Tim_Box_t * pBox;
    int i;
    *pnBbIns = *pnBbOuts = 0;
    if ( Tim_ManBoxNum(p) )
        Tim_ManForEachBox( p, pBox, i )
        {
            if ( !pBox->fBlack )//&& pBox->nInputs <= 6 )
                continue;
            *pnBbIns  += Tim_ManBoxInputNum( p, i );
            *pnBbOuts += Tim_ManBoxOutputNum( p, i );
        }
}
int Tim_ManDelayTableNum( Tim_Man_t * p )
{
    return p->vDelayTables ? Vec_PtrSize(p->vDelayTables) : 0;
}

/**Function*************************************************************

  Synopsis    [Sets the vector of timing tables associated with the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManSetDelayTables( Tim_Man_t * p, Vec_Ptr_t * vDelayTables )
{
    assert( p->vDelayTables == NULL );
    p->vDelayTables = vDelayTables;
}

/**Function*************************************************************

  Synopsis    [Disables the use of the traversal ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManTravIdDisable( Tim_Man_t * p )
{
    p->fUseTravId = 0;
}

/**Function*************************************************************

  Synopsis    [Enables the use of the traversal ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManTravIdEnable( Tim_Man_t * p )
{
    p->fUseTravId = 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

