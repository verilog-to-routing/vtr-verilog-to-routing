/**CFile****************************************************************

  FileName    [absUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [Interface to pthreads.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absUtil.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "abs.h"

ABC_NAMESPACE_IMPL_START 

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abs_ParSetDefaults( Abs_Par_t * p )
{
    memset( p, 0, sizeof(Abs_Par_t) );
    p->nFramesMax         =      0;   // maximum frames
    p->nFramesStart       =      0;   // starting frame 
    p->nFramesPast        =      4;   // overlap frames
    p->nConfLimit         =      0;   // conflict limit
    p->nLearnedMax        =   1000;   // max number of learned clauses
    p->nLearnedStart      =   1000;   // max number of learned clauses
    p->nLearnedDelta      =    200;   // max number of learned clauses
    p->nLearnedPerce      =     70;   // max number of learned clauses
    p->nTimeOut           =      0;   // timeout in seconds
    p->nRatioMin          =      0;   // stop when less than this % of object is abstracted
    p->nRatioMax          =     30;   // restart when more than this % of object is abstracted
    p->fUseTermVars       =      0;   // use terminal variables
    p->fUseRollback       =      0;   // use rollback to the starting number of frames
    p->fPropFanout        =      1;   // propagate fanouts during refinement
    p->fVerbose           =      0;   // verbose flag
    p->iFrame             =     -1;   // the number of frames covered 
    p->iFrameProved       =     -1;   // the number of frames proved
    p->nFramesNoChangeLim =      2;   // the number of frames without change to dump abstraction
}

/**Function*************************************************************

  Synopsis    [Converting VTA vector to GLA vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_VtaConvertToGla( Gia_Man_t * p, Vec_Int_t * vVta )
{
    Gia_Obj_t * pObj;
    Vec_Int_t * vGla;
    int nObjMask, nObjs = Gia_ManObjNum(p);
    int i, Entry, nFrames = Vec_IntEntry( vVta, 0 );
    assert( Vec_IntEntry(vVta, nFrames+1) == Vec_IntSize(vVta) );
    // get the bitmask
    nObjMask = (1 << Abc_Base2Log(nObjs)) - 1;
    assert( nObjs <= nObjMask );
    // go through objects
    vGla = Vec_IntStart( nObjs );
    Vec_IntForEachEntryStart( vVta, Entry, i, nFrames+2 )
    {
        pObj = Gia_ManObj( p, (Entry &  nObjMask) );
        assert( Gia_ObjIsRo(p, pObj) || Gia_ObjIsAnd(pObj) || Gia_ObjIsConst0(pObj) );
        Vec_IntAddToEntry( vGla, (Entry &  nObjMask), 1 );
    }
    Vec_IntWriteEntry( vGla, 0, nFrames );
    return vGla;
}

/**Function*************************************************************

  Synopsis    [Converting GLA vector to VTA vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_VtaConvertFromGla( Gia_Man_t * p, Vec_Int_t * vGla, int nFrames )
{
    Vec_Int_t * vVta;
    int nObjBits, nObjMask, nObjs = Gia_ManObjNum(p);
    int i, k, j, Entry, Counter, nGlaSize;
    //. get the GLA size
    nGlaSize = Vec_IntSum(vGla);
    // get the bitmask
    nObjBits = Abc_Base2Log(nObjs);
    nObjMask = (1 << Abc_Base2Log(nObjs)) - 1;
    assert( nObjs <= nObjMask );
    // go through objects
    vVta = Vec_IntAlloc( 1000 );
    Vec_IntPush( vVta, nFrames );
    Counter = nFrames + 2;
    for ( i = 0; i <= nFrames; i++, Counter += i * nGlaSize )
        Vec_IntPush( vVta, Counter );
    for ( i = 0; i < nFrames; i++ )
        for ( k = 0; k <= i; k++ )
            Vec_IntForEachEntry( vGla, Entry, j )
                if ( Entry ) 
                    Vec_IntPush( vVta, (k << nObjBits) | j );
    Counter = Vec_IntEntry(vVta, nFrames+1);
    assert( Vec_IntEntry(vVta, nFrames+1) == Vec_IntSize(vVta) );
    return vVta;
}

/**Function*************************************************************

  Synopsis    [Converting GLA vector to FLA vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_FlaConvertToGla_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vGla )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    Vec_IntWriteEntry( vGla, Gia_ObjId(p, pObj), 1 );
    if ( Gia_ObjIsRo(p, pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_FlaConvertToGla_rec( p, Gia_ObjFanin0(pObj), vGla );
    Gia_FlaConvertToGla_rec( p, Gia_ObjFanin1(pObj), vGla );
}

/**Function*************************************************************

  Synopsis    [Converting FLA vector to GLA vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_FlaConvertToGla( Gia_Man_t * p, Vec_Int_t * vFla )
{
    Vec_Int_t * vGla;
    Gia_Obj_t * pObj;
    int i;
    // mark const0 and relevant CI objects
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent(p, Gia_ManConst0(p));
    Gia_ManForEachPi( p, pObj, i )
        Gia_ObjSetTravIdCurrent(p, pObj);
    Gia_ManForEachRo( p, pObj, i )
        if ( !Vec_IntEntry(vFla, i) )
            Gia_ObjSetTravIdCurrent(p, pObj);
    // label all objects reachable from the PO and selected flops
    vGla = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_IntWriteEntry( vGla, 0, 1 );
    Gia_ManForEachPo( p, pObj, i )
        Gia_FlaConvertToGla_rec( p, Gia_ObjFanin0(pObj), vGla );
    Gia_ManForEachRi( p, pObj, i )
        if ( Vec_IntEntry(vFla, i) )
            Gia_FlaConvertToGla_rec( p, Gia_ObjFanin0(pObj), vGla );
    return vGla;
}

/**Function*************************************************************

  Synopsis    [Converting GLA vector to FLA vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_GlaConvertToFla( Gia_Man_t * p, Vec_Int_t * vGla )
{
    Vec_Int_t * vFla;
    Gia_Obj_t * pObj;
    int i;
    vFla = Vec_IntStart( Gia_ManRegNum(p) );
    Gia_ManForEachRo( p, pObj, i )
        if ( Vec_IntEntry(vGla, Gia_ObjId(p, pObj)) )
            Vec_IntWriteEntry( vFla, i, 1 );
    return vFla;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_GlaCountFlops( Gia_Man_t * p, Vec_Int_t * vGla )
{
    Gia_Obj_t * pObj;
    int i, Count = 0;
    Gia_ManForEachRo( p, pObj, i )
        if ( Vec_IntEntry(vGla, Gia_ObjId(p, pObj)) )
            Count++;
    return Count;
}
int Gia_GlaCountNodes( Gia_Man_t * p, Vec_Int_t * vGla )
{
    Gia_Obj_t * pObj;
    int i, Count = 0;
    Gia_ManForEachAnd( p, pObj, i )
        if ( Vec_IntEntry(vGla, Gia_ObjId(p, pObj)) )
            Count++;
    return Count;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

