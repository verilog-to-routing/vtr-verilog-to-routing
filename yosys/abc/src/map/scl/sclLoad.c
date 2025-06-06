/**CFile****************************************************************

  FileName    [sclLoad.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Wire/gate load computations.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: sclLoad.c,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sclSize.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns estimated wire capacitances for each fanout count.]

  Description []
               
  SideEffects []`

  SeeAlso     []

***********************************************************************/
Vec_Flt_t * Abc_SclFindWireCaps( SC_WireLoad * pWL, int nFanoutMax )
{
    Vec_Flt_t * vCaps = NULL;
    float EntryPrev, EntryCur, Slope;
    int i, iPrev, k, Entry, EntryMax;
    assert( pWL != NULL );
    // find the biggest fanout count
    EntryMax = 0;
    Vec_IntForEachEntry( &pWL->vFanout, Entry, i )
        EntryMax = Abc_MaxInt( EntryMax, Entry );
    // create the array
    vCaps = Vec_FltStart( Abc_MaxInt(nFanoutMax, EntryMax) + 1 );
    Vec_IntForEachEntry( &pWL->vFanout, Entry, i )
        Vec_FltWriteEntry( vCaps, Entry, Vec_FltEntry(&pWL->vLen, i) * pWL->cap );
    if ( Vec_FltEntry(vCaps, 1) == 0 )
        return vCaps;
    // interpolate between the values
    assert( Vec_FltEntry(vCaps, 1) != 0 );
    iPrev = 1;
    EntryPrev = Vec_FltEntry(vCaps, 1);
    Vec_FltForEachEntryStart( vCaps, EntryCur, i, 2 )
    {
        if ( EntryCur == 0 )
            continue;
        Slope = (EntryCur - EntryPrev) / (i - iPrev);
        for ( k = iPrev + 1; k < i; k++ )
            Vec_FltWriteEntry( vCaps, k, EntryPrev + Slope * (k - iPrev) );
        EntryPrev = EntryCur;
        iPrev = i;
    }
    // extrapolate after the largest value
    Slope = pWL->cap * pWL->slope;
    for ( k = iPrev + 1; k < i; k++ )
        Vec_FltWriteEntry( vCaps, k, EntryPrev + Slope * (k - iPrev) );
    // show
//    Vec_FltForEachEntry( vCaps, EntryCur, i )
//        printf( "%3d : %f\n", i, EntryCur );
    return vCaps;
}

/**Function*************************************************************

  Synopsis    [Computes load for all nodes in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Abc_SclFindWireLoad( Vec_Flt_t * vWireCaps, int nFans )
{
    if ( vWireCaps == NULL )
        return 0;
    return Vec_FltEntry( vWireCaps, Abc_MinInt(nFans, Vec_FltSize(vWireCaps)-1) );
}
void Abc_SclAddWireLoad( SC_Man * p, Abc_Obj_t * pObj, int fSubtr )
{
    float Load = Abc_SclFindWireLoad( p->vWireCaps, Abc_ObjFanoutNum(pObj) );
    Abc_SclObjLoad(p, pObj)->rise += fSubtr ? -Load : Load;
    Abc_SclObjLoad(p, pObj)->fall += fSubtr ? -Load : Load;
}
void Abc_SclComputeLoad( SC_Man * p )
{
    Abc_Obj_t * pObj, * pFanin;
    int i, k;
    // clear load storage
    Abc_NtkForEachObj( p->pNtk, pObj, i )
    {
        SC_Pair * pLoad = Abc_SclObjLoad( p, pObj );
        if ( !Abc_ObjIsPo(pObj) )
            pLoad->rise = pLoad->fall = 0.0;
    }
    // add cell load
    Abc_NtkForEachNode1( p->pNtk, pObj, i )
    {
        SC_Cell * pCell = Abc_SclObjCell( pObj );
        Abc_ObjForEachFanin( pObj, pFanin, k )
        {
            SC_Pair * pLoad = Abc_SclObjLoad( p, pFanin );
            SC_Pin * pPin = SC_CellPin( pCell, k );
            pLoad->rise += pPin->rise_cap;
            pLoad->fall += pPin->fall_cap;
        }
    }
    // add PO load
    Abc_NtkForEachCo( p->pNtk, pObj, i )
    {
        SC_Pair * pLoadPo = Abc_SclObjLoad( p, pObj );
        SC_Pair * pLoad = Abc_SclObjLoad( p, Abc_ObjFanin0(pObj) );
        pLoad->rise += pLoadPo->rise;
        pLoad->fall += pLoadPo->fall;
    }
    // add wire load
    if ( p->pWLoadUsed != NULL )
    {
        if ( p->vWireCaps == NULL )
            p->vWireCaps = Abc_SclFindWireCaps( p->pWLoadUsed, Abc_NtkGetFanoutMax(p->pNtk) );
        Abc_NtkForEachNode1( p->pNtk, pObj, i )
            Abc_SclAddWireLoad( p, pObj, 0 );
        Abc_NtkForEachPi( p->pNtk, pObj, i )
            Abc_SclAddWireLoad( p, pObj, 0 );
    }
    // check input loads
    if ( p->vInDrive != NULL )
    {
        Abc_NtkForEachPi( p->pNtk, pObj, i )
        {
            SC_Pair * pLoad = Abc_SclObjLoad( p, pObj );
            if ( Abc_SclObjInDrive(p, pObj) != 0 && (pLoad->rise > Abc_SclObjInDrive(p, pObj) || pLoad->fall > Abc_SclObjInDrive(p, pObj)) )
                printf( "Maximum input drive strength is exceeded at primary input %d.\n", i );
        }
    }
/*
    // transfer load from barbufs
    Abc_NtkForEachBarBuf( p->pNtk, pObj, i )
    {
        SC_Pair * pLoad = Abc_SclObjLoad( p, pObj );
        SC_Pair * pLoadF = Abc_SclObjLoad( p, Abc_ObjFanin(pObj, 0) );
        SC_PairAdd( pLoadF, pLoad );
    }
*/
    // calculate average load
//    if ( p->EstLoadMax )
    {
        double TotalLoad = 0;
        int nObjs = 0;
        Abc_NtkForEachNode1( p->pNtk, pObj, i )
        {
            SC_Pair * pLoad = Abc_SclObjLoad( p, pObj );
            TotalLoad += 0.5 * pLoad->fall + 0.5 * pLoad->rise;
            nObjs++;
        }
        Abc_NtkForEachPi( p->pNtk, pObj, i )
        {
            SC_Pair * pLoad = Abc_SclObjLoad( p, pObj );
            TotalLoad += 0.5 * pLoad->fall + 0.5 * pLoad->rise;
            nObjs++;
        }
        p->EstLoadAve = (float)(TotalLoad / nObjs);
//        printf( "Average load = %.2f\n", p->EstLoadAve );
    }
}

/**Function*************************************************************

  Synopsis    [Updates load of the node's fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclUpdateLoad( SC_Man * p, Abc_Obj_t * pObj, SC_Cell * pOld, SC_Cell * pNew )
{
    Abc_Obj_t * pFanin;
    int k;
    Abc_ObjForEachFanin( pObj, pFanin, k )
    {
        SC_Pair * pLoad = Abc_SclObjLoad( p, pFanin );
        SC_Pin * pPinOld = SC_CellPin( pOld, k );
        SC_Pin * pPinNew = SC_CellPin( pNew, k );
        pLoad->rise += pPinNew->rise_cap - pPinOld->rise_cap;
        pLoad->fall += pPinNew->fall_cap - pPinOld->fall_cap;
    }
}
void Abc_SclUpdateLoadSplit( SC_Man * p, Abc_Obj_t * pBuffer, Abc_Obj_t * pFanout )
{
    SC_Pin * pPin;
    SC_Pair * pLoad;
    int iFanin = Abc_NodeFindFanin( pFanout, pBuffer );
    assert( iFanin >= 0 );
    assert( Abc_ObjFaninNum(pBuffer) == 1 );
    pPin = SC_CellPin( Abc_SclObjCell(pFanout), iFanin );
    // update load of the buffer
    pLoad = Abc_SclObjLoad( p, pBuffer );
    pLoad->rise -= pPin->rise_cap;
    pLoad->fall -= pPin->fall_cap;
    // update load of the fanin
    pLoad = Abc_SclObjLoad( p, Abc_ObjFanin0(pBuffer) );
    pLoad->rise += pPin->rise_cap;
    pLoad->fall += pPin->fall_cap;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

