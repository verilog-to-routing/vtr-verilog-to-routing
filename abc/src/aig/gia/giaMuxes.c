/**CFile****************************************************************

  FileName    [giaMuxes.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Multiplexer profiling algorithm.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaMuxes.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/util/utilNam.h"
#include "misc/util/utilTruth.h"
#include "misc/vec/vecWec.h"
#include "misc/vec/vecHsh.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Counts XORs and MUXes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCountMuxXor( Gia_Man_t * p, int * pnMuxes, int * pnXors )
{
    Gia_Obj_t * pObj, * pFan0, * pFan1; int i;
    *pnMuxes = *pnXors = 0;
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !Gia_ObjIsMuxType(pObj) )
            continue;
        if ( Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
            (*pnXors)++;
        else
            (*pnMuxes)++;
    }
}
void Gia_ManPrintMuxStats( Gia_Man_t * p )
{
    int nAnds, nMuxes, nXors, nTotal;
    if ( p->pMuxes )
    {
        nAnds  = Gia_ManAndNotBufNum(p)-Gia_ManXorNum(p)-Gia_ManMuxNum(p);
        nXors  = Gia_ManXorNum(p);
        nMuxes = Gia_ManMuxNum(p);
        nTotal = nAnds + 3*nXors + 3*nMuxes;
    }
    else 
    {
        Gia_ManCountMuxXor( p, &nMuxes, &nXors );
        nAnds  = Gia_ManAndNotBufNum(p) - 3*nMuxes - 3*nXors;
        nTotal = Gia_ManAndNotBufNum(p);
    }
    Abc_Print( 1, "stats:  " );
    Abc_Print( 1, "xor =%8d %6.2f %%   ", nXors,  300.0*nXors/nTotal );
    Abc_Print( 1, "mux =%8d %6.2f %%   ", nMuxes, 300.0*nMuxes/nTotal );
    Abc_Print( 1, "and =%8d %6.2f %%   ", nAnds,  100.0*nAnds/nTotal );
    Abc_Print( 1, "obj =%8d  ", Gia_ManAndNotBufNum(p) );
    fflush( stdout );
}

/**Function*************************************************************

  Synopsis    [Derives GIA with MUXes.]

  Description [Create MUX if the sum of fanin references does not exceed limit.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupMuxes( Gia_Man_t * p, int Limit )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pFan0, * pFan1, * pFanC, * pSiblNew, * pObjNew;
    int i;
    assert( p->pMuxes == NULL );
    assert( Limit >= 0 ); // allows to create AIG with XORs without MUXes
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p ); 
    // start the new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->pMuxes = ABC_CALLOC( unsigned, pNew->nObjsAlloc );
    if ( Gia_ManHasChoices(p) )
        pNew->pSibls = ABC_CALLOC( int, pNew->nObjsAlloc );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashStart( pNew );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsBuf(pObj) )
            pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( !Gia_ObjIsMuxType(pObj) || Gia_ObjSibl(p, Gia_ObjFaninId0(pObj, i)) || Gia_ObjSibl(p, Gia_ObjFaninId1(pObj, i)) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
            pObj->Value = Gia_ManHashXorReal( pNew, Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan0)), Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan1)) );
        else if ( Gia_ObjRefNum(p, Gia_ObjFanin0(pObj)) + Gia_ObjRefNum(p, Gia_ObjFanin1(pObj)) > Limit )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else
        {
            pFanC = Gia_ObjRecognizeMux( pObj, &pFan1, &pFan0 );
            pObj->Value = Gia_ManHashMuxReal( pNew, Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFanC)), Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan1)), Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan0)) );
        }
        if ( !Gia_ObjSibl(p, i) )
            continue;
        pObjNew  = Gia_ManObj( pNew, Abc_Lit2Var(pObj->Value) );
        pSiblNew = Gia_ManObj( pNew, Abc_Lit2Var(Gia_ObjSiblObj(p, i)->Value) );
        if ( Gia_ObjIsAnd(pObjNew) && Gia_ObjIsAnd(pSiblNew) && Gia_ObjId(pNew, pObjNew) > Gia_ObjId(pNew, pSiblNew) )
            pNew->pSibls[Gia_ObjId(pNew, pObjNew)] = Gia_ObjId(pNew, pSiblNew);
    }
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    // perform cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Creates AIG with XORs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManCreateXors( Gia_Man_t * p )
{
    Gia_Man_t * pNew; Gia_Obj_t * pObj, * pFan0, * pFan1; 
    Vec_Int_t * vRefs = Vec_IntStart( Gia_ManObjNum(p) );
    int i, iLit0, iLit1, nXors = 0, nObjs = 0;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = 0;
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
        {
            Vec_IntAddToEntry( vRefs, Gia_ObjId(p, Gia_Regular(pFan0)), 1 );
            Vec_IntAddToEntry( vRefs, Gia_ObjId(p, Gia_Regular(pFan1)), 1 );
            pObj->fMark0 = 1;
            nXors++;
        }
        else
        {
            Vec_IntAddToEntry( vRefs, Gia_ObjFaninId0(pObj, i), 1 );
            Vec_IntAddToEntry( vRefs, Gia_ObjFaninId1(pObj, i), 1 );
        }
    }
    Gia_ManForEachCo( p, pObj, i )
        Vec_IntAddToEntry( vRefs, Gia_ObjFaninId0p(p, pObj), 1 );
    Gia_ManForEachAnd( p, pObj, i )
        nObjs += Vec_IntEntry(vRefs, i) > 0;
    pNew = Gia_ManStart( 1 + Gia_ManCiNum(p) + Gia_ManCoNum(p) + nObjs );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsBuf(pObj) )
            pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( pObj->fMark0 )
        {
            Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1);
            iLit0 = Abc_LitNotCond( Gia_Regular(pFan0)->Value, Gia_IsComplement(pFan0) );
            iLit1 = Abc_LitNotCond( Gia_Regular(pFan1)->Value, Gia_IsComplement(pFan1) );
            pObj->Value = Gia_ManAppendXorReal( pNew, iLit0, iLit1 );
        }
        else if ( Vec_IntEntry(vRefs, i) > 0 )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    }
    assert( pNew->nObjs == pNew->nObjsAlloc );
    pNew->pMuxes = ABC_CALLOC( unsigned, pNew->nObjs );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Vec_IntFree( vRefs );
    //printf( "Created %d XORs.\n", nXors );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Derives GIA without MUXes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupNoMuxes( Gia_Man_t * p, int fSkipBufs )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    assert( p->pMuxes != NULL || Gia_ManXorNum(p) );
    // start the new manager
    pNew = Gia_ManStart( 5000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashStart( pNew );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsBuf(pObj) )
            pObj->Value = fSkipBufs ? Gia_ObjFanin0Copy(pObj) : Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsMuxId(p, i) )
            pObj->Value = Gia_ManHashMux( pNew, Gia_ObjFanin2Copy(p, pObj), Gia_ObjFanin1Copy(pObj), Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsXor(pObj) )
            pObj->Value = Gia_ManHashXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else 
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    }
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    // perform cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Test these procedures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupMuxesTest( Gia_Man_t * p )
{
    Gia_Man_t * pNew, * pNew2;
    pNew = Gia_ManDupMuxes( p, 2 );
    pNew2 = Gia_ManDupNoMuxes( pNew, 0 );
    Gia_ManPrintStats( p, NULL );
    Gia_ManPrintStats( pNew, NULL );
    Gia_ManPrintStats( pNew2, NULL );
    Gia_ManStop( pNew );
//    Gia_ManStop( pNew2 );
    return pNew2;
}


/**Function*************************************************************

  Synopsis    [Test these procedures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManMuxRestructure( Gia_Man_t * p )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, nNodes = 0;
    Vec_Bit_t * vUsed = Vec_BitStart( Gia_ManObjNum(p) );
    assert( !Gia_ManHasChoices(p) );
    assert( !Gia_ManHasMapping(p) );
    assert( p->pMuxes != NULL );
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p ); 
    // start the new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->pMuxes = ABC_CALLOC( unsigned, pNew->nObjsAlloc );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashStart( pNew );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsBuf(pObj) )
            pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsMuxId(p, i) && 
                  Gia_ObjIsMuxId(p, Gia_ObjFaninId0(pObj, i)) && !Vec_BitEntry(vUsed, Gia_ObjFaninId0(pObj, i)) && 
                  Gia_ObjIsMuxId(p, Gia_ObjFaninId1(pObj, i)) && !Vec_BitEntry(vUsed, Gia_ObjFaninId1(pObj, i)) &&
                  Gia_ObjFaninId2(p, Gia_ObjFaninId0(pObj, i)) == Gia_ObjFaninId2(p, Gia_ObjFaninId1(pObj, i))  )
        {
            Gia_Obj_t * pFan1 = Gia_ObjFanin1(pObj);
            int Value0 = Gia_ManHashMux( pNew, Gia_ObjFanin2Copy(p, pObj), Gia_ObjFanin2Copy(p, pFan1), Gia_ObjFanin0Copy(pObj) );
            int Value1 = Gia_ManHashMux( pNew, Value0, Gia_ObjFanin1Copy(pFan1), Gia_ObjFanin0Copy(pFan1) );
            pObj->Value = Gia_ManHashMux( pNew, Gia_ObjFanin2Copy(p, pObj), Value1, Value0 );
            Vec_BitWriteEntry( vUsed, Gia_ObjFaninId0(pObj, i), 1 );
            Vec_BitWriteEntry( vUsed, Gia_ObjFaninId1(pObj, i), 1 );
            Vec_BitWriteEntry( vUsed, i, 1 );
            nNodes++;
        }
        else if ( Gia_ObjIsMuxId(p, i) )
            pObj->Value = Gia_ManHashMux( pNew, Gia_ObjFanin2Copy(p, pObj), Gia_ObjFanin1Copy(pObj), Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsXor(pObj) )
            pObj->Value = Gia_ManHashXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else 
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    }
    Vec_BitFree( vUsed );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    // perform cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Gia_Man_t * Gia_ManDupMuxRestructure( Gia_Man_t * p )
{
    Gia_Man_t * pTemp, * pNew = Gia_ManDupMuxes( p, 2 );
    pNew = Gia_ManMuxRestructure( pTemp = pNew );  Gia_ManStop( pTemp );
    pNew = Gia_ManDupNoMuxes( pTemp = pNew, 0 );   Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Returns the size of MUX structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_MuxRef_rec( Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    if ( !Gia_ObjIsMuxId(p, iObj) )
        return 0;
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjRefInc(p, pObj) )
        return 0;
    return Gia_MuxRef_rec( p, Gia_ObjFaninId0p(p, pObj) ) + 
           Gia_MuxRef_rec( p, Gia_ObjFaninId1p(p, pObj) ) + 
           Gia_MuxRef_rec( p, Gia_ObjFaninId2p(p, pObj) ) + 1;
}
int Gia_MuxRef( Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    assert( Gia_ObjIsMuxId(p, iObj) );
    return Gia_MuxRef_rec( p, Gia_ObjFaninId0p(p, pObj) ) + 
           Gia_MuxRef_rec( p, Gia_ObjFaninId1p(p, pObj) ) + 
           Gia_MuxRef_rec( p, Gia_ObjFaninId2p(p, pObj) ) + 1;
}
int Gia_MuxDeref_rec( Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    if ( !Gia_ObjIsMuxId(p, iObj) )
        return 0;
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjRefDec(p, pObj) )
        return 0;
    return Gia_MuxDeref_rec( p, Gia_ObjFaninId0p(p, pObj) ) + 
           Gia_MuxDeref_rec( p, Gia_ObjFaninId1p(p, pObj) ) + 
           Gia_MuxDeref_rec( p, Gia_ObjFaninId2p(p, pObj) ) + 1;
}
int Gia_MuxDeref( Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    assert( Gia_ObjIsMuxId(p, iObj) );
    return Gia_MuxDeref_rec( p, Gia_ObjFaninId0p(p, pObj) ) + 
           Gia_MuxDeref_rec( p, Gia_ObjFaninId1p(p, pObj) ) + 
           Gia_MuxDeref_rec( p, Gia_ObjFaninId2p(p, pObj) ) + 1;
}
int Gia_MuxMffcSize( Gia_Man_t * p, int iObj )
{
    int Count1, Count2;
    if ( !Gia_ObjIsMuxId(p, iObj) )
        return 0;
    Count1 = Gia_MuxDeref( p, iObj );
    Count2 = Gia_MuxRef( p, iObj );
    assert( Count1 == Count2 );
    return Count1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MuxStructPrint_rec( Gia_Man_t * p, int iObj, int fFirst )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    int iCtrl;
    if ( !fFirst && (!Gia_ObjIsMuxId(p, iObj) || Gia_ObjRefNumId(p, iObj) > 0) )
    {
//        printf( "%d", iObj );
        printf( "<%02d>", Gia_ObjLevelId(p, iObj) );
        return;
    }
    iCtrl = Gia_ObjFaninId2p(p, pObj);
    printf( " [(" );
    if ( Gia_ObjIsMuxId(p, iCtrl) && Gia_ObjRefNumId(p, iCtrl) == 0 )
        Gia_MuxStructPrint_rec( p, iCtrl, 0 );
    else
    {
        printf( "%d", iCtrl );
        printf( "<%d>", Gia_ObjLevelId(p, iCtrl) );
    }
    printf( ")" );
    if ( Gia_ObjFaninC2(p, pObj) )
    {
        Gia_MuxStructPrint_rec( p, Gia_ObjFaninId0p(p, pObj), 0 );
        printf( "|" );
        Gia_MuxStructPrint_rec( p, Gia_ObjFaninId1p(p, pObj), 0 );
        printf( "]" );
    }
    else
    {
        Gia_MuxStructPrint_rec( p, Gia_ObjFaninId1p(p, pObj), 0 );
        printf( "|" );
        Gia_MuxStructPrint_rec( p, Gia_ObjFaninId0p(p, pObj), 0 );
        printf( "]" );
    }
}
void Gia_MuxStructPrint( Gia_Man_t * p, int iObj )
{
    int Count1, Count2;
    assert( Gia_ObjIsMuxId(p, iObj) );
    Count1 = Gia_MuxDeref( p, iObj );
    Gia_MuxStructPrint_rec( p, iObj, 1 );
    Count2 = Gia_MuxRef( p, iObj );
    assert( Count1 == Count2 );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MuxStructDump_rec( Gia_Man_t * p, int iObj, int fFirst, Vec_Str_t * vStr, int nDigitsId )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    int iCtrl;
    if ( !fFirst && (!Gia_ObjIsMuxId(p, iObj) || Gia_ObjRefNumId(p, iObj) > 0) )
        return;
    iCtrl = Gia_ObjFaninId2p(p, pObj);
    Vec_StrPush( vStr, '[' );
    Vec_StrPush( vStr, '(' );
    if ( Gia_ObjIsMuxId(p, iCtrl) && Gia_ObjRefNumId(p, iCtrl) == 0 )
        Gia_MuxStructDump_rec( p, iCtrl, 0, vStr, nDigitsId );
    else
        Vec_StrPrintNumStar( vStr, iCtrl, nDigitsId );
    Vec_StrPush( vStr, ')' );
    if ( Gia_ObjFaninC2(p, pObj) )
    {
        Gia_MuxStructDump_rec( p, Gia_ObjFaninId0p(p, pObj), 0, vStr, nDigitsId );
        Vec_StrPush( vStr, '|' );
        Gia_MuxStructDump_rec( p, Gia_ObjFaninId1p(p, pObj), 0, vStr, nDigitsId );
        Vec_StrPush( vStr, ']' );
    }
    else
    {
        Gia_MuxStructDump_rec( p, Gia_ObjFaninId1p(p, pObj), 0, vStr, nDigitsId );
        Vec_StrPush( vStr, '|' );
        Gia_MuxStructDump_rec( p, Gia_ObjFaninId0p(p, pObj), 0, vStr, nDigitsId );
        Vec_StrPush( vStr, ']' );
    }
}
int Gia_MuxStructDump( Gia_Man_t * p, int iObj, Vec_Str_t * vStr, int nDigitsNum, int nDigitsId )
{
    int Count1, Count2;
    assert( Gia_ObjIsMuxId(p, iObj) );
    Count1 = Gia_MuxDeref( p, iObj );
    Vec_StrClear( vStr );
    Vec_StrPrintNumStar( vStr, Count1, nDigitsNum );
    Gia_MuxStructDump_rec( p, iObj, 1, vStr, nDigitsId );
    Vec_StrPush( vStr, '\0' );
    Count2 = Gia_MuxRef( p, iObj );
    assert( Count1 == Count2 );
    return Count1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManMuxCompare( char ** pp1, char ** pp2 )
{
    int Diff = strcmp( *pp1, *pp2 );
    if ( Diff < 0 )
        return 1;
    if ( Diff > 0) 
        return -1;
    return 0; 
}
int Gia_ManMuxCountOne( char * p )
{
    int Count = 0;
    for ( ; *p; p++ )
        Count += (*p == '[');
    return Count;
}

typedef struct Mux_Man_t_ Mux_Man_t; 
struct Mux_Man_t_
{
    Gia_Man_t *     pGia;      // manager
    Abc_Nam_t *     pNames;    // hashing name into ID
    Vec_Wec_t *     vTops;     // top nodes for each struct
};

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mux_Man_t * Mux_ManAlloc( Gia_Man_t * pGia )
{
    Mux_Man_t * p;
    p = ABC_CALLOC( Mux_Man_t, 1 );
    p->pGia   = pGia;
    p->pNames = Abc_NamStart( 10000, 50 );
    p->vTops  = Vec_WecAlloc( 1000 );
    Vec_WecPushLevel( p->vTops );
    return p;
}
void Mux_ManFree( Mux_Man_t * p )
{
    Abc_NamStop( p->pNames );
    Vec_WecFree( p->vTops );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManMuxProfile( Mux_Man_t * p, int fWidth )
{
    int i, Entry, Counter, Total;
    Vec_Int_t * vVec, * vCounts;
    vCounts = Vec_IntStart( 1000 );
    if ( fWidth )
    {
        Vec_WecForEachLevelStart( p->vTops, vVec, i, 1 )
            Vec_IntAddToEntry( vCounts, Abc_MinInt(Vec_IntSize(vVec), 999), 1 );
    }
    else
    {
        for ( i = 1; i < Vec_WecSize(p->vTops); i++ )
            Vec_IntAddToEntry( vCounts, Abc_MinInt(atoi(Abc_NamStr(p->pNames, i)), 999), 1 );
    }
    Total = Vec_IntCountPositive(vCounts);
    if ( Total == 0 )
        return 0;
    printf( "The distribution of MUX tree %s:\n", fWidth ? "widths" : "sizes"  );
    Counter = 0;
    Vec_IntForEachEntry( vCounts, Entry, i )
    {
        if ( !Entry ) continue;
        if ( ++Counter == 12 )
            printf( "\n" ), Counter = 0;
        printf( "  %d=%d", i, Entry );
    }
    printf( "\nSummary: " );
    printf( "Max = %d  ", Vec_IntFindMax(vCounts) );
    printf( "Ave = %.2f", 1.0*Vec_IntSum(vCounts)/Total );
    printf( "\n" );
    Vec_IntFree( vCounts );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManMuxProfiling( Gia_Man_t * p )
{
    Mux_Man_t * pMan;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    Vec_Str_t * vStr;
    Vec_Int_t * vFans, * vVec;
    int i, Counter, fFound, iStructId, nDigitsId;
    abctime clk = Abc_Clock();

    pNew = Gia_ManDupMuxes( p, 2 );
    nDigitsId = Abc_Base10Log( Gia_ManObjNum(pNew) );

    pMan = Mux_ManAlloc( pNew );
     
    Gia_ManLevelNum( pNew );
    Gia_ManCreateRefs( pNew );
    Gia_ManForEachCo( pNew, pObj, i )
        Gia_ObjRefFanin0Inc( pNew, pObj );

    vStr = Vec_StrAlloc( 1000 );
    vFans = Gia_ManFirstFanouts( pNew );
    Gia_ManForEachMux( pNew, pObj, i )
    {
        // skip MUXes in the middle of the tree (which have only one MUX fanout)
        if ( Gia_ObjRefNumId(pNew, i) == 1 && Gia_ObjIsMuxId(pNew, Vec_IntEntry(vFans, i)) )
            continue;
        // this node is the root of the MUX structure - create hash key
        Counter = Gia_MuxStructDump( pNew, i, vStr, 3, nDigitsId );
        if ( Counter == 1 )
            continue;
        iStructId = Abc_NamStrFindOrAdd( pMan->pNames, Vec_StrArray(vStr), &fFound );
        if ( !fFound )
            Vec_WecPushLevel( pMan->vTops );
        assert( Abc_NamObjNumMax(pMan->pNames) == Vec_WecSize(pMan->vTops) );
        Vec_IntPush( Vec_WecEntry(pMan->vTops, iStructId), i );
    }
    Vec_StrFree( vStr );
    Vec_IntFree( vFans );

    printf( "MUX structure profile for AIG \"%s\":\n", p->pName );
    printf( "Total MUXes = %d.  Total trees = %d.  Unique trees = %d.  Memory = %.2f MB   ", 
        Gia_ManMuxNum(pNew), Vec_WecSizeSize(pMan->vTops), Vec_WecSize(pMan->vTops)-1, 
        1.0*Abc_NamMemUsed(pMan->pNames)/(1<<20) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    if ( Gia_ManMuxProfile(pMan, 0) )
    {
        Gia_ManMuxProfile( pMan, 1 );

        // short the first ones
        printf( "The first %d structures: \n", 10 );
        Vec_WecForEachLevelStartStop( pMan->vTops, vVec, i, 1, Abc_MinInt(Vec_WecSize(pMan->vTops), 10) )
        {
            char * pTemp = Abc_NamStr(pMan->pNames, i);
            printf( "%5d : ", i );
            printf( "Occur = %4d   ", Vec_IntSize(vVec) );
            printf( "Size = %4d   ", atoi(pTemp) );
            printf( "%s\n", pTemp );
        }

        // print trees for the first one
        Counter = 0;
        Vec_WecForEachLevelStart( pMan->vTops, vVec, i, 1 )
        {
            char * pTemp = Abc_NamStr(pMan->pNames, i);
            if ( Vec_IntSize(vVec) > 5 && atoi(pTemp) > 5 )
            {
                int k, Entry;
                printf( "For example, structure %d has %d MUXes and bit-width %d:\n", i, atoi(pTemp), Vec_IntSize(vVec) );
                Vec_IntForEachEntry( vVec, Entry, k )
                    Gia_MuxStructPrint( pNew, Entry );
                if ( ++Counter == 5 )
                    break;
            }
        }
    }

    Mux_ManFree( pMan );
    Gia_ManStop( pNew );
}


/**Function*************************************************************

  Synopsis    [Compute one-level TFI/TFO structural signatures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

// these are object/fanin/fanout attributes
// http://stackoverflow.com/questions/9907160/how-to-convert-enum-names-to-string-in-c
#define GIA_FOREACH_ITEM(ITEM) \
    ITEM(C0)    \
    ITEM(PO)    \
    ITEM(PI)    \
    ITEM(FF)    \
    ITEM(XOR)   \
    ITEM(MUX)   \
    ITEM(AND)   \
    ITEM(iC0)   \
    ITEM(iC1)   \
    ITEM(iPI)   \
    ITEM(iFF)   \
    ITEM(iXOR)  \
    ITEM(iMUX)  \
    ITEM(iAND)  \
    ITEM(iANDn) \
    ITEM(iANDp) \
    ITEM(oPO)   \
    ITEM(oFF)   \
    ITEM(oXOR)  \
    ITEM(oMUXc) \
    ITEM(oMUXd) \
    ITEM(oAND)  \
    ITEM(oANDn) \
    ITEM(oANDp) \
    ITEM(GIA_END)

#define GENERATE_ENUM(ENUM) ENUM,
typedef enum { GIA_FOREACH_ITEM(GENERATE_ENUM) } Gia_ObjType_t;

#define GENERATE_STRING(STRING) #STRING,
static const char * GIA_TYPE_STRINGS[] = { GIA_FOREACH_ITEM(GENERATE_STRING) };

void Gia_ManProfileStructuresTest( Gia_Man_t * p )
{
    int i;
    for ( i = 0; i < GIA_END; i++ )
        printf( "%d = %s\n", i, GIA_TYPE_STRINGS[i] );
}



// find object code
int Gia_ManEncodeObj( Gia_Man_t * p, int i )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, i );
    assert( !Gia_ObjIsRi(p, pObj) );
    if ( Gia_ObjIsConst0(pObj) )
        return C0;
    if ( Gia_ObjIsPo(p, pObj) )
        return PO;
    if ( Gia_ObjIsPi(p, pObj) )
        return PI;
    if ( Gia_ObjIsCi(pObj) )
        return FF;
    if ( Gia_ObjIsXor(pObj) )
        return XOR;
    if ( Gia_ObjIsMux(p, pObj) )
        return MUX;
    assert( Gia_ObjIsAnd(pObj) );
    return AND;
}
// find fanin code
int Gia_ManEncodeFanin( Gia_Man_t * p, int iLit )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, Abc_Lit2Var(iLit) );
    if ( Gia_ObjIsConst0(pObj) )
        return iC0;
    if ( Gia_ObjIsPi(p, pObj) )
        return iPI;
    if ( Gia_ObjIsCi(pObj) )
        return iFF;
    if ( Gia_ObjIsXor(pObj) )
        return iXOR;
    if ( Gia_ObjIsMux(p, pObj) )
        return iMUX;
    assert( Gia_ObjIsAnd(pObj) );
    return iAND;
//    if ( Abc_LitIsCompl(iLit) )
//        return iANDn;
//    else
//        return iANDp;
}
// find fanout code
Gia_ObjType_t Gia_ManEncodeFanout( Gia_Man_t * p, Gia_Obj_t * pObj, int i )
{
//    int iLit;
    if ( Gia_ObjIsPo(p, pObj) )
        return oPO;
    if ( Gia_ObjIsCo(pObj) )
        return oFF;
    if ( Gia_ObjIsXor(pObj) )
        return oXOR;
    if ( Gia_ObjIsMux(p, pObj) )
        return i == 2 ? oMUXc : oMUXd;
    assert( Gia_ObjIsAnd(pObj) );
    return oAND;
//    iLit = i ? Gia_ObjFaninLit1p(p, pObj) : Gia_ObjFaninLit0p(p, pObj);
//    if ( Abc_LitIsCompl(iLit) )
//        return oANDn;
//    else
//        return oANDp;
}

void Gia_ManProfileCollect( Gia_Man_t * p, int i, Vec_Int_t * vCode, Vec_Int_t * vCodeOffsets, Vec_Int_t * vArray )
{
    int k;
    Vec_IntClear( vArray );
    for ( k = Vec_IntEntry(vCodeOffsets, i); k < Vec_IntEntry(vCodeOffsets, i+1); k++ )
        Vec_IntPush( vArray, Vec_IntEntry(vCode, k) );
}

void Gia_ManProfilePrintOne( Gia_Man_t * p, int i, Vec_Int_t * vArray )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, i );
    int k, nFanins, nFanouts;
    if ( Gia_ObjIsRi(p, pObj) )
        return;
    nFanins = Gia_ObjIsRo(p, pObj) ? 1 : Gia_ObjFaninNum(p, pObj);
    nFanouts = Gia_ObjFanoutNum(p, pObj);

    printf( "%6d : ", i );
    for ( k = 0; k < nFanins; k++ )
        printf( "  %5s", GIA_TYPE_STRINGS[Vec_IntEntry(vArray, k + 1)] );
    for (      ; k < 3; k++ )
        printf( "  %5s", "" );
    printf( "  ->" );
    printf( " %5s", GIA_TYPE_STRINGS[Vec_IntEntry(vArray, 0)] );
    printf( "  ->" );
    if ( nFanouts > 0 )
    {
        int Count = 1, Prev = Vec_IntEntry(vArray, 1 + nFanins);
        for ( k = 1; k < nFanouts; k++ )
        {
            if ( Prev != Vec_IntEntry(vArray, k + 1 + nFanins) )
            {
                printf( "  %d x %s", Count, GIA_TYPE_STRINGS[Prev] );
                Prev = Vec_IntEntry(vArray, k + 1 + nFanins);
                Count = 0;
            }
            Count++;
        }
        printf( "  %d x %s", Count, GIA_TYPE_STRINGS[Prev] );
    }
    printf( "\n" );
}

Vec_Int_t * Gia_ManProfileHash( Gia_Man_t * p, Vec_Int_t * vCode, Vec_Int_t * vCodeOffsets )
{
    Hsh_VecMan_t * pHash;
    Vec_Int_t * vRes, * vArray;
    Gia_Obj_t * pObj;
    int i;
    vRes = Vec_IntAlloc( Gia_ManObjNum(p) );
    pHash = Hsh_VecManStart( Gia_ManObjNum(p) );
    // add empty entry
    vArray = Vec_IntAlloc( 100 );
    Hsh_VecManAdd( pHash, vArray );
    // iterate through the entries
    Gia_ManForEachObj( p, pObj, i )
    {
        Gia_ManProfileCollect( p, i, vCode, vCodeOffsets, vArray );
        Vec_IntPush( vRes, Hsh_VecManAdd( pHash, vArray ) );
    }
    Hsh_VecManStop( pHash );
    Vec_IntFree( vArray );
    assert( Vec_IntSize(vRes) == Gia_ManObjNum(p) );
    return vRes;
}


void Gia_ManProfileStructuresInt( Gia_Man_t * p, int nLimit, int fVerbose )
{
    Vec_Int_t * vRes, * vCount, * vFirst;
    Vec_Int_t * vCode, * vCodeOffsets, * vArray;
    Gia_Obj_t * pObj, * pFanout;
    int i, k, nFanins, nFanouts, * pPerm, nClasses;
    assert( p->pMuxes );
    Gia_ManStaticFanoutStart( p );
    // create fanout codes
    vArray = Vec_IntAlloc( 100 );
    vCode = Vec_IntAlloc( 5 * Gia_ManObjNum(p) );
    vCodeOffsets = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        Vec_IntPush( vCodeOffsets, Vec_IntSize(vCode) );
        if ( Gia_ObjIsRi(p, pObj) )
            continue;
        nFanins = Gia_ObjFaninNum(p, pObj);
        nFanouts = Gia_ObjFanoutNum(p, pObj);
        Vec_IntPush( vCode, Gia_ManEncodeObj(p, i) );
        if ( nFanins == 3 )
        {
            int iLit = Gia_ObjFaninLit2p(p, pObj);
            Vec_IntPush( vCode, Gia_ManEncodeFanin(p, Abc_LitRegular(iLit)) );
            if ( Abc_LitIsCompl(iLit) )
            {
                Vec_IntPush( vCode, Gia_ManEncodeFanin(p, Gia_ObjFaninLit0p(p, pObj)) );
                Vec_IntPush( vCode, Gia_ManEncodeFanin(p, Gia_ObjFaninLit1p(p, pObj)) );
            }
            else
            {
                Vec_IntPush( vCode, Gia_ManEncodeFanin(p, Gia_ObjFaninLit1p(p, pObj)) );
                Vec_IntPush( vCode, Gia_ManEncodeFanin(p, Gia_ObjFaninLit0p(p, pObj)) );
            }
        }
        else if ( nFanins == 2 )
        {
            int Code0 = Gia_ManEncodeFanin(p, Gia_ObjFaninLit0p(p, pObj));
            int Code1 = Gia_ManEncodeFanin(p, Gia_ObjFaninLit1p(p, pObj));
            Vec_IntPush( vCode, Code0 < Code1 ? Code0 : Code1 );
            Vec_IntPush( vCode, Code0 < Code1 ? Code1 : Code0 );
        }
        else if ( nFanins == 1 )
            Vec_IntPush( vCode, Gia_ManEncodeFanin(p, Gia_ObjFaninLit0p(p, pObj)) );
        else if ( Gia_ObjIsRo(p, pObj) )
            Vec_IntPush( vCode, Gia_ManEncodeFanin(p, Gia_ObjFaninLit0p(p, Gia_ObjRoToRi(p, pObj))) );

        // add fanouts
        Vec_IntClear( vArray );
        Gia_ObjForEachFanoutStatic( p, pObj, pFanout, k )
        {
            int Index = Gia_ObjWhatFanin( p, pFanout, pObj );
            Gia_ObjType_t Type = Gia_ManEncodeFanout( p, pFanout, Index );
            Vec_IntPush( vArray, Type );
        }
        Vec_IntSort( vArray, 0 );
        Vec_IntAppend( vCode, vArray );
    }
    assert( Vec_IntSize(vCodeOffsets) == Gia_ManObjNum(p) );
    Vec_IntPush( vCodeOffsets, Vec_IntSize(vCode) );
    // print the results
    if ( fVerbose )
    {
        printf( "Showing TFI/node/TFO structures for all nodes:\n" );
        Gia_ManForEachObj( p, pObj, i )
        {
            Gia_ManProfileCollect( p, i, vCode, vCodeOffsets, vArray );
            Gia_ManProfilePrintOne( p, i, vArray );
        }
    }

    // collect statistics
    vRes = Gia_ManProfileHash( p, vCode, vCodeOffsets );
    //Vec_IntPrint( vRes );

    // count how many times each class appears
    nClasses = Vec_IntFindMax(vRes) + 1;
    vCount = Vec_IntStart( nClasses );
    vFirst = Vec_IntStart( nClasses );
    Gia_ManForEachObj( p, pObj, i )
    {
        int Entry = Vec_IntEntry( vRes, i );
        if ( Gia_ObjIsRi(p, pObj) )
            continue;
        if ( Vec_IntEntry(vCount, Entry) == 0 )
            Vec_IntWriteEntry( vFirst, Entry, i );
        Vec_IntAddToEntry( vCount, Entry, -1 );
    }
    // sort the counts
    pPerm = Abc_MergeSortCost( Vec_IntArray(vCount), Vec_IntSize(vCount) );
    printf( "Showing TFI/node/TFO structures that appear more than %d times.\n", nLimit );
    for ( i = 0; i < nClasses-1; i++ )
    {
        if ( nLimit > -Vec_IntEntry(vCount, pPerm[i]) )
            break;
        printf( "%6d : ", i );
        printf( "%6d : ", pPerm[i] );
        printf( "Weight =%6d  ",   -Vec_IntEntry(vCount, pPerm[i]) );
        printf( "First obj =" );
        // print the object
        Gia_ManProfileCollect( p, Vec_IntEntry(vFirst, pPerm[i]), vCode, vCodeOffsets, vArray );
        Gia_ManProfilePrintOne( p, Vec_IntEntry(vFirst, pPerm[i]), vArray );
    }

    // cleanup
    ABC_FREE( pPerm );
    Vec_IntFree( vRes );
    Vec_IntFree( vCount );
    Vec_IntFree( vFirst );

    Vec_IntFree( vArray );
    Vec_IntFree( vCode );
    Vec_IntFree( vCodeOffsets );
    Gia_ManStaticFanoutStop( p );
}
void Gia_ManProfileStructures( Gia_Man_t * p, int nLimit, int fVerbose )
{
    if ( p->pMuxes )
        Gia_ManProfileStructuresInt( p, nLimit, fVerbose );
    else
    {
        Gia_Man_t * pNew = Gia_ManDupMuxes( p, 2 );
        Gia_ManProfileStructuresInt( pNew, nLimit, fVerbose );
        Gia_ManStop( pNew );
    }
}

/**Function*************************************************************

  Synopsis    [Circuit restructuring.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManMarkTfi_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    Gia_ManMarkTfi_rec( p, Gia_ObjFanin0(pObj) );
    Gia_ManMarkTfi_rec( p, Gia_ObjFanin1(pObj) );
}
Vec_Int_t * Gia_ManFindSharedInputs( Gia_Man_t * p )
{
    Gia_Obj_t * pObj, * pObj2; int i, k, Value;
    Vec_Int_t * vRes = Vec_IntStart( Gia_ManCiNum(p) );
    Gia_ManForEachCo( p, pObj, i )
    {
        Gia_ManIncrementTravId( p );
        Gia_ManMarkTfi_rec( p, Gia_ObjFanin0(pObj) );
        Gia_ManForEachCi( p, pObj2, k )
            if ( Gia_ObjIsTravIdCurrent(p, pObj2) )
                Vec_IntAddToEntry( vRes, k, 1 );
    }
    k = 0;
    Vec_IntForEachEntry( vRes, Value, i )
        if ( Value == Gia_ManCoNum(p) )
            Vec_IntWriteEntry( vRes, k++, i );
    Vec_IntShrink( vRes, k );
    //printf( "Found %d candidate inputs.\n", Vec_IntSize(vRes) );
    if ( Vec_IntSize(vRes) == 0 || Vec_IntSize(vRes) > 10 )
        Vec_IntFreeP(&vRes);
    return vRes;
}
Vec_Wec_t * Gia_ManFindCofs( Gia_Man_t * p, Vec_Int_t * vRes, Gia_Man_t ** ppNew )
{
    Gia_Obj_t * pObj;
    Vec_Wec_t * vCofs = Vec_WecStart( 1 << Vec_IntSize(vRes) );
    int Value, i, m, nMints = 1 << Vec_IntSize(vRes);
    Gia_Man_t * pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Gia_ManHashAlloc( pNew );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    assert( Vec_IntSize(vRes) < Gia_ManCiNum(p) );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    for ( m = 0; m < nMints; m++ )
    {
        Vec_Int_t * vLayer = Vec_WecEntry( vCofs, m );
        Vec_IntForEachEntry( vRes, Value, i )
            Gia_ManCi(p, Value)->Value = (unsigned)((m >> i) & 1);
        Gia_ManForEachAnd( p, pObj, i )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachCo( p, pObj, i )
            Vec_IntPush( vLayer, Gia_ObjFanin0Copy(pObj) );
        assert( Vec_IntSize(vLayer) == Gia_ManCoNum(p) );
        //printf( "%3d : ", m ); Vec_IntPrint( vLayer );
    }
    if ( ppNew != NULL )
        *ppNew = pNew;
    return vCofs;
}
Vec_Int_t * Gia_ManFindEquivClasses( Vec_Wec_t * vCofs )
{
    Vec_Int_t * vVec; int i, k, Lev;
    Vec_Int_t * vMap    = Vec_IntAlloc( Vec_WecSize(vCofs) );
    Vec_Int_t * vUnique = Vec_IntAlloc( Vec_WecSize(vCofs) );
    Vec_WecForEachLevel( vCofs, vVec, i )
    {
        Vec_IntForEachEntry( vUnique, Lev, k )
            if ( Vec_IntEqual(vVec, Vec_WecEntry(vCofs, Lev)) )
                break;
        Vec_IntPush( vMap, k );
        if ( k == Vec_IntSize(vUnique) )
            Vec_IntPush( vUnique, i );
    }
    //printf( "Found %d equiv clasess.\n", Vec_IntSize(vUnique) );
    //Vec_IntPrint( vUnique );
    Vec_IntFree( vUnique );
    assert( Vec_IntSize(vMap) == Vec_WecSize(vCofs) );
    //Vec_IntPrint( vMap );
    return vMap;
}
int Gia_ManFindMuxTree_rec( Gia_Man_t * pNew, int * pCtrl, int nCtrl, Vec_Int_t * vData, int Shift )
{
    int iLit0, iLit1;
    if ( nCtrl-- == 0 )
        return Vec_IntEntry( vData, Shift );
    iLit0 = Gia_ManFindMuxTree_rec( pNew, pCtrl, nCtrl, vData, Shift );
    iLit1 = Gia_ManFindMuxTree_rec( pNew, pCtrl, nCtrl, vData, Shift + (1<<nCtrl) );
    return Gia_ManHashMux( pNew, pCtrl[nCtrl], iLit1, iLit0 );
}
void Gia_ManFindDerive( Gia_Man_t * pNew, int nOuts, Vec_Int_t * vIns, Vec_Wec_t * vCofs, Vec_Int_t * vMap )
{
    extern int Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash );
    Vec_Int_t * vMemory = Vec_IntAlloc( 1 << 16 );
    Vec_Int_t * vLeaves = Vec_IntAlloc( 100 );
    Vec_Int_t * vUsed = Vec_IntStart( Vec_WecSize(vCofs) );
    Vec_Int_t * vBits = Vec_IntAlloc( 10 );
    Vec_Int_t * vData = Vec_IntAlloc( Vec_WecSize(vCofs) );
    int nWords = Abc_TtWordNum(Vec_IntSize(vIns));
    word * pTruth = ABC_ALLOC( word, nWords );
    int nValues = Vec_IntFindMax(vMap)+1;
    int i, k, Value, nBits = Abc_Base2Log(nValues);
    assert( nBits < Vec_IntSize(vIns) );
    assert( Vec_IntSize(vMap) == Vec_WecSize(vCofs) );
    assert( Vec_IntSize(vMap) == (1 << Vec_IntSize(vIns)) );
    Vec_IntForEachEntry( vIns, Value, i )
        Vec_IntPush( vLeaves, Gia_ObjToLit(pNew, Gia_ManCi(pNew, Value)) );
    for ( i = 0; i < nBits; i++ )
    {
        Abc_TtClear( pTruth, nWords );
        Vec_IntForEachEntry( vMap, Value, k )
            if ( (Value >> i) & 1 )
                Abc_TtSetBit( pTruth, k );
        if ( nBits < 6 )
            pTruth[0] = Abc_Tt6Stretch( pTruth[0], Vec_IntSize(vIns) );
        Vec_IntPush( vBits, Kit_TruthToGia(pNew, (unsigned*)pTruth, Vec_IntSize(vIns), vMemory, vLeaves, 1) );
        //printf( "Bit %d : ", i ); Dau_DsdPrintFromTruth( pTruth, Vec_IntSize(vIns) );
    }
    for ( i = 0; i < nValues; i++ )
    {
        int Cof = Vec_IntFind(vMap, i);
        assert( Cof >= 0 && Cof < Vec_WecSize(vCofs) );
        Vec_IntWriteEntry( vUsed, Cof, 1 );
    }
    for ( i = 0; i < nOuts; i++ )
    {
        Vec_Int_t * vLevel;
        Vec_IntClear( vData );
        Vec_WecForEachLevel( vCofs, vLevel, k )
            if ( Vec_IntEntry(vUsed, k) )
                Vec_IntPush( vData, Vec_IntEntry(vLevel, i) );
        while ( Vec_IntSize(vData) < (1 << nBits) )
            Vec_IntPush( vData, 0 );
        assert( Vec_IntSize(vData) == (1 << nBits) );
        assert( Vec_IntSize(vBits) == nBits );
        Value = Gia_ManFindMuxTree_rec( pNew, Vec_IntArray(vBits), Vec_IntSize(vBits), vData, 0 );
        Gia_ManAppendCo( pNew, Value );
    }
    ABC_FREE( pTruth );
    Vec_IntFree( vUsed );
    Vec_IntFree( vBits );
    Vec_IntFree( vData );
    Vec_IntFree( vLeaves );
    Vec_IntFree( vMemory );
}
Gia_Man_t * Gia_ManCofStructure( Gia_Man_t * p )
{
    Gia_Man_t * pNew = NULL;
    Vec_Int_t * vIns = Gia_ManFindSharedInputs( p );
    Vec_Wec_t * vCfs = vIns ? Gia_ManFindCofs( p, vIns, &pNew ) : NULL;
    Vec_Int_t * vMap = vCfs ? Gia_ManFindEquivClasses( vCfs ) : NULL;
    if ( vMap && Abc_Base2Log(Vec_IntFindMax(vMap)+1) < Vec_IntSize(vIns) ) 
    {
        Gia_Man_t * pTemp; 
        Gia_ManFindDerive( pNew, Gia_ManCoNum(p), vIns, vCfs, vMap );
        pNew = Gia_ManCleanup( pTemp = pNew );
        Gia_ManStop( pTemp );
    }
    else
        Gia_ManStopP( &pNew );
    Vec_WecFreeP( &vCfs );
    Vec_IntFreeP( &vMap );
    Vec_IntFreeP( &vIns );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

