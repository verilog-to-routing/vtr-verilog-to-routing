/**CFile****************************************************************

  FileName    [timDump.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchy/timing manager.]

  Synopsis    [Saving and loading the hierarchy timing manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: timDump.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "timInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define TIM_DUMP_VER_NUM 1

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transform the timing manager into the char stream.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Tim_ManSave( Tim_Man_t * p, int fHieOnly )
{
    Tim_Box_t * pBox;
    Tim_Obj_t * pObj;
    Vec_Str_t * vStr;
    float * pDelayTable;
    int i, k, TableSize;
    // create output stream
    vStr = Vec_StrAlloc( 10000 );
    // dump version number
    Vec_StrPutI_ne( vStr, TIM_DUMP_VER_NUM );
    // save CI/CO counts
    Vec_StrPutI_ne( vStr, Tim_ManCiNum(p) );
    Vec_StrPutI_ne( vStr, Tim_ManCoNum(p) );
    // save PI/PO counts
    Vec_StrPutI_ne( vStr, Tim_ManPiNum(p) );
    Vec_StrPutI_ne( vStr, Tim_ManPoNum(p) );
    // save number of boxes
    Vec_StrPutI_ne( vStr, Tim_ManBoxNum(p) );
    // for each box, save num_inputs, num_outputs, delay table ID, and copy field
    if ( Tim_ManBoxNum(p) > 0 )
    Tim_ManForEachBox( p, pBox, i )
    {
        Vec_StrPutI_ne( vStr, Tim_ManBoxInputNum(p, pBox->iBox) );
        Vec_StrPutI_ne( vStr, Tim_ManBoxOutputNum(p, pBox->iBox) );
        Vec_StrPutI_ne( vStr, Tim_ManBoxDelayTableId(p, pBox->iBox) ); // can be -1 if delay table is not given
        Vec_StrPutI_ne( vStr, Tim_ManBoxCopy(p, pBox->iBox) );         // can be -1 if the copy is node defined
        //Vec_StrPutI_ne( vStr, Tim_ManBoxIsBlack(p, pBox->iBox) );
    }
    if ( fHieOnly )
        return vStr;
    // save the number of delay tables
    Vec_StrPutI_ne( vStr, Tim_ManDelayTableNum(p) );
    // save the delay tables
    if ( Tim_ManDelayTableNum(p) > 0 )
    Tim_ManForEachTable( p, pDelayTable, i )
    {
        assert( (int)pDelayTable[0] == i );
        // save table ID and dimensions (inputs x outputs)
        Vec_StrPutI_ne( vStr, (int)pDelayTable[0] );
        Vec_StrPutI_ne( vStr, (int)pDelayTable[1] );
        Vec_StrPutI_ne( vStr, (int)pDelayTable[2] );
        // save table contents
        TableSize = (int)pDelayTable[1] * (int)pDelayTable[2];
        for ( k = 0; k < TableSize; k++ )
            Vec_StrPutF( vStr, pDelayTable[k+3] );
    }
    // save PI arrival times
    Tim_ManForEachPi( p, pObj, i )
        Vec_StrPutF( vStr, Tim_ManGetCiArrival(p, pObj->Id) );
    // save PO required times
    Tim_ManForEachPo( p, pObj, i )
        Vec_StrPutF( vStr, Tim_ManGetCoRequired(p, pObj->Id) );
    return vStr;
}

/**Function*************************************************************

  Synopsis    [Restores the timing manager from the char stream.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Tim_Man_t * Tim_ManLoad( Vec_Str_t * p, int fHieOnly )
{
    Tim_Man_t * pMan;
    Tim_Obj_t * pObj;
    int VerNum, nCis, nCos, nPis, nPos;
    int nBoxes, nBoxIns, nBoxOuts, CopyBox, fBlack;
    int TableId, nTables, TableSize, TableX, TableY;
    int i, k, curPi, curPo, iStr = 0;
    float * pDelayTable;
    // get version number 
    VerNum = Vec_StrGetI_ne( p, &iStr );
    assert( VerNum == TIM_DUMP_VER_NUM );
    // get the number of CIs/COs
    nCis = Vec_StrGetI_ne( p, &iStr );
    nCos = Vec_StrGetI_ne( p, &iStr );
    // get the number of PIs/POs
    nPis = Vec_StrGetI_ne( p, &iStr );
    nPos = Vec_StrGetI_ne( p, &iStr );
    // start the timing manager
    pMan = Tim_ManStart( nCis, nCos );
    // start boxes
    nBoxes = Vec_StrGetI_ne( p, &iStr );
    assert( pMan->vBoxes == NULL );
    if ( nBoxes > 0 )
        pMan->vBoxes = Vec_PtrAlloc( nBoxes );
    // create boxes
    curPi = nPis;
    curPo = 0;
    for ( i = 0; i < nBoxes; i++ )
    {
        nBoxIns  = Vec_StrGetI_ne( p, &iStr );
        nBoxOuts = Vec_StrGetI_ne( p, &iStr );
        TableId  = Vec_StrGetI_ne( p, &iStr );
        CopyBox  = Vec_StrGetI_ne( p, &iStr );
        fBlack   = 0;//Vec_StrGetI_ne( p, &iStr );
        Tim_ManCreateBox( pMan, curPo, nBoxIns, curPi, nBoxOuts, TableId, fBlack );
        Tim_ManBoxSetCopy( pMan, i, CopyBox );
        curPi += nBoxOuts;
        curPo += nBoxIns;
    }
    curPo += nPos;
    assert( curPi == nCis );
    assert( curPo == nCos );
    if ( fHieOnly )
        return pMan;
    // create delay tables
    nTables = Vec_StrGetI_ne( p, &iStr );
    assert( pMan->vDelayTables == NULL );
    if ( nTables > 0 )
        pMan->vDelayTables = Vec_PtrAlloc( nTables );
    // read delay tables
    for ( i = 0; i < nTables; i++ )
    {
        // read table ID and dimensions
        TableId = Vec_StrGetI_ne( p, &iStr );
        TableX  = Vec_StrGetI_ne( p, &iStr );
        TableY  = Vec_StrGetI_ne( p, &iStr );
        assert( TableId == i );
        // create new table
        TableSize = TableX * TableY;
        pDelayTable = ABC_ALLOC( float, TableSize + 3 );
        pDelayTable[0] = TableId;
        pDelayTable[1] = TableX;
        pDelayTable[2] = TableY;
        // read table contents
        for ( k = 0; k < TableSize; k++ )
            pDelayTable[k+3] = Vec_StrGetF( p, &iStr );
        assert( Vec_PtrSize(pMan->vDelayTables) == TableId );
        Vec_PtrPush( pMan->vDelayTables, pDelayTable );
    }
    assert( Tim_ManDelayTableNum(pMan) == nTables );
    // read PI arrival times
    Tim_ManForEachPi( pMan, pObj, i )
        Tim_ManInitPiArrival( pMan, i, Vec_StrGetF(p, &iStr) );
    // read PO required times
    Tim_ManForEachPo( pMan, pObj, i )
        Tim_ManInitPoRequired( pMan, i, Vec_StrGetF(p, &iStr) );
    assert( Vec_StrSize(p) == iStr );
//    Tim_ManPrint( pMan );
    return pMan;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

