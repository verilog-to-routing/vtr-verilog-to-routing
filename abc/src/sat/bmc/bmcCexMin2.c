/**CFile****************************************************************

  FileName    [bmcCexMin2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [CEX minimization.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcCexMin2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig/gia/gia.h"
#include "bmc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int  Abc_InfoGet2Bits( Vec_Int_t * vData, int nWords, int iFrame, int iObj )
{
    unsigned * pInfo = (unsigned *)Vec_IntEntryP( vData, nWords * iFrame );
    return 3 & (pInfo[iObj >> 4] >> ((iObj & 15) << 1));
}
static inline void Abc_InfoSet2Bits( Vec_Int_t * vData, int nWords, int iFrame, int iObj, int Value )
{
    unsigned * pInfo = (unsigned *)Vec_IntEntryP( vData, nWords * iFrame );
    Value ^= (3 & (pInfo[iObj >> 4] >> ((iObj & 15) << 1)));
    pInfo[iObj >> 4] ^= (Value << ((iObj & 15) << 1));
}
static inline void Abc_InfoAdd2Bits( Vec_Int_t * vData, int nWords, int iFrame, int iObj, int Value )
{
    unsigned * pInfo = (unsigned *)Vec_IntEntryP( vData, nWords * iFrame );
    pInfo[iObj >> 4] |= (Value << ((iObj & 15) << 1));
}

static inline int  Gia_ManGetTwo( Gia_Man_t * p, int iFrame, Gia_Obj_t * pObj )              { return Abc_InfoGet2Bits( p->vTruths, p->nTtWords, iFrame, Gia_ObjId(p, pObj) ); }
static inline void Gia_ManSetTwo( Gia_Man_t * p, int iFrame, Gia_Obj_t * pObj, int Value )   { Abc_InfoSet2Bits( p->vTruths, p->nTtWords, iFrame, Gia_ObjId(p, pObj), Value ); }
static inline void Gia_ManAddTwo( Gia_Man_t * p, int iFrame, Gia_Obj_t * pObj, int Value )   { Abc_InfoAdd2Bits( p->vTruths, p->nTtWords, iFrame, Gia_ObjId(p, pObj), Value ); }

/*
    For CEX minimization, all terminals (const0, PI, RO in frame0) have equal priority.
    For abstraction refinement, all terminals, except PPis, have higher priority.
*/

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Annotates the unrolling with CEX value/priority.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManAnnotateUnrolling( Gia_Man_t * p, Abc_Cex_t * pCex, int fJustMax )
{
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int RetValue, f, k, Value, Value0, Value1, iBit;
    // start storage for internal info
    assert( p->vTruths == NULL );
    p->nTtWords = Abc_BitWordNum( 2 * Gia_ManObjNum(p) );
    p->vTruths  = Vec_IntStart( (pCex->iFrame + 1) * p->nTtWords );
    // assign values to all objects (const0 and RO in frame0 are assigned 0)
    Gia_ManCleanMark0(p);
    for ( f = 0, iBit = pCex->nRegs; f <= pCex->iFrame; f++ )
    {
        Gia_ManForEachPi( p, pObj, k )
            if ( (pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++)) )
                Gia_ManAddTwo( p, f, pObj, 1 );
        Gia_ManForEachAnd( p, pObj, k )
            if ( (pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj))) )
                Gia_ManAddTwo( p, f, pObj, 1 );
        Gia_ManForEachCo( p, pObj, k )
            if ( (pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) )
                Gia_ManAddTwo( p, f, pObj, 1 );
        if ( f == pCex->iFrame )
            break;
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            if ( (pObjRo->fMark0 = pObjRi->fMark0) )
                Gia_ManAddTwo( p, f+1, pObjRo, 1 );
    }
    assert( iBit == pCex->nBits );
    // check the output value
    RetValue = Gia_ManPo(p, pCex->iPo)->fMark0;
    if ( RetValue != 1 )
        printf( "Counter-example is invalid.\n" );
    // assign justification to nodes
    Gia_ManCleanMark0(p);
    pObj = Gia_ManPo(p, pCex->iPo);
    pObj->fMark0 = 1;
    Gia_ManAddTwo( p, pCex->iFrame, pObj, 2 );
    for ( f = pCex->iFrame; f >= 0; f-- )
    {
        // transfer to CO drivers
        Gia_ManForEachCo( p, pObj, k )
            if ( (Gia_ObjFanin0(pObj)->fMark0 = pObj->fMark0) )
            {
                pObj->fMark0 = 0;
                Gia_ManAddTwo( p, f, Gia_ObjFanin0(pObj), 2 );
            }
        // compute justification
        Gia_ManForEachAndReverse( p, pObj, k )
        {
            if ( !pObj->fMark0 ) 
                continue;
            pObj->fMark0 = 0;
            Value = (1 & Gia_ManGetTwo(p, f, pObj));
            Value0 = (1 & Gia_ManGetTwo(p, f, Gia_ObjFanin0(pObj))) ^ Gia_ObjFaninC0(pObj);
            Value1 = (1 & Gia_ManGetTwo(p, f, Gia_ObjFanin1(pObj))) ^ Gia_ObjFaninC1(pObj);
            if ( Value0 == Value1 )
            {
                assert( Value == (Value0 & Value1) );
                if ( fJustMax || Value == 1 )
                {
                    Gia_ObjFanin0(pObj)->fMark0 = Gia_ObjFanin1(pObj)->fMark0 = 1;
                    Gia_ManAddTwo( p, f, Gia_ObjFanin0(pObj), 2 );
                    Gia_ManAddTwo( p, f, Gia_ObjFanin1(pObj), 2 );
                }
                else
                {
                    Gia_ObjFanin0(pObj)->fMark0 = 1;
                    Gia_ManAddTwo( p, f, Gia_ObjFanin0(pObj), 2 );
                }
            }
            else if ( Value0 == 0 )
            {
                assert( Value == 0 );
                Gia_ObjFanin0(pObj)->fMark0 = 1;
                Gia_ManAddTwo( p, f, Gia_ObjFanin0(pObj), 2 );
            }
            else if ( Value1 == 0 )
            {
                assert( Value == 0 );
                Gia_ObjFanin1(pObj)->fMark0 = 1;
                Gia_ManAddTwo( p, f, Gia_ObjFanin1(pObj), 2 );
            }
            else assert( 0 );
        }
        if ( f == 0 )
            break;
        // transfer to RIs
        Gia_ManForEachPi( p, pObj, k )
            pObj->fMark0 = 0;
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            if ( (pObjRi->fMark0 = pObjRo->fMark0) )
            {
                pObjRo->fMark0 = 0;
                Gia_ManAddTwo( p, f-1, pObjRi, 2 );
            }
    }
    Gia_ManCleanMark0(p);
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Computing AIG characterizing all justifying assignments.]

  Description [Used in CEX minimization.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManCreateUnate( Gia_Man_t * p, Abc_Cex_t * pCex, int iFrame, int nRealPis, int fUseAllObjects )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int f, k, Value;
    assert( iFrame >= 0 && iFrame <= pCex->iFrame );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "unate" );
    Gia_ManCleanValue( p );
    // set flop outputs
    if ( nRealPis < 0 ) // CEX min
    {
        Gia_ManForEachRo( p, pObj, k )
        {
            if ( fUseAllObjects )
            {
                int Value = Gia_ManAppendCi(pNew);
                if ( (Gia_ManGetTwo(p, iFrame, pObj) >> 1) ) // in the path
                    pObj->Value = Value;
            }
            else if ( (Gia_ManGetTwo(p, iFrame, pObj) >> 1) ) // in the path
                pObj->Value = Gia_ManAppendCi(pNew);
        }
    }
    else
    {
        Gia_ManForEachRo( p, pObj, k )
            pObj->Value = (Gia_ManGetTwo(p, iFrame, pObj) >> 1);
    }
    Gia_ManHashAlloc( pNew );
    for ( f = iFrame; f <= pCex->iFrame; f++ )
    {
/*
        printf( "  F%03d ", f );
        Gia_ManForEachRo( p, pObj, k )
            printf( "%d", pObj->Value > 0 );
        printf( "\n" );
*/
        // set const0 to const1 if present
        pObj = Gia_ManConst0(p);
        pObj->Value = (Gia_ManGetTwo(p, f, pObj) >> 1);
        // set primary inputs 
        if ( nRealPis < 0 ) // CEX min
        {
            Gia_ManForEachPi( p, pObj, k )
                pObj->Value = (Gia_ManGetTwo(p, f, pObj) >> 1);
        }
        else
        {
            Gia_ManForEachPi( p, pObj, k )
            {
                pObj->Value = (Gia_ManGetTwo(p, f, pObj) >> 1);
                if ( k >= nRealPis )
                {
                    if ( fUseAllObjects )
                    {
                        int Value = Gia_ManAppendCi(pNew);
                        if ( (Gia_ManGetTwo(p, f, pObj) >> 1) ) // in the path
                            pObj->Value = Value;
                    }
                    else if ( (Gia_ManGetTwo(p, f, pObj) >> 1) ) // in the path
                        pObj->Value = Gia_ManAppendCi(pNew);
                }
            }
        }
        // traverse internal nodes
        Gia_ManForEachAnd( p, pObj, k )
        { 
            pObj->Value = 0;
            Value = Gia_ManGetTwo(p, f, pObj);
            if ( !(Value >> 1) ) // not in the path
                continue;
            if ( Gia_ObjFanin0(pObj)->Value && Gia_ObjFanin1(pObj)->Value )
            {
                if ( 1 & Gia_ManGetTwo(p, f, pObj) ) // value 1
                {
                    if ( Gia_ObjFanin0(pObj)->Value > 1 && Gia_ObjFanin1(pObj)->Value > 1 )
                        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value );
                    else if ( Gia_ObjFanin0(pObj)->Value > 1 )
                        pObj->Value = Gia_ObjFanin0(pObj)->Value;
                    else if ( Gia_ObjFanin1(pObj)->Value > 1 )
                        pObj->Value = Gia_ObjFanin1(pObj)->Value;
                    else 
                        pObj->Value = 1;
                }
                else // value 0
                {
                    if ( Gia_ObjFanin0(pObj)->Value > 1 && Gia_ObjFanin1(pObj)->Value > 1 )
                        pObj->Value = Gia_ManHashOr( pNew, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value );
                    else if ( Gia_ObjFanin0(pObj)->Value > 1 )
                        pObj->Value = Gia_ObjFanin0(pObj)->Value;
                    else if ( Gia_ObjFanin1(pObj)->Value > 1 )
                        pObj->Value = Gia_ObjFanin1(pObj)->Value;
                    else 
                        pObj->Value = 1;
                }
            }
            else if ( Gia_ObjFanin0(pObj)->Value )
                pObj->Value = Gia_ObjFanin0(pObj)->Value;
            else if ( Gia_ObjFanin1(pObj)->Value )
                pObj->Value = Gia_ObjFanin1(pObj)->Value;
            else assert( 0 );
        }
        Gia_ManForEachCo( p, pObj, k )
            pObj->Value = Gia_ObjFanin0(pObj)->Value;
        if ( f == pCex->iFrame )
            break;
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            pObjRo->Value = pObjRi->Value;
    }
    Gia_ManHashStop( pNew );
    // create primary output
    pObj = Gia_ManPo(p, pCex->iPo);
    assert( (Gia_ManGetTwo(p, pCex->iFrame, pObj) >> 1) );
    assert( pObj->Value );
    Gia_ManAppendCo( pNew, pObj->Value );
    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Gia_ManCexMin( Gia_Man_t * p, Abc_Cex_t * pCex, int iFrameStart, int nRealPis, int fJustMax, int fUseAll, int fVerbose )
{
    Gia_Man_t * pNew;
    int f, RetValue;
    // check CEX
    assert( pCex->nPis == Gia_ManPiNum(p) );
//    assert( pCex->nRegs == Gia_ManRegNum(p) );
    assert( pCex->iPo < Gia_ManPoNum(p) );
    // check frames
    assert( iFrameStart >= 0 && iFrameStart <= pCex->iFrame );
    // check primary inputs
    assert( nRealPis < Gia_ManPiNum(p) );
    // prepare
    RetValue = Gia_ManAnnotateUnrolling( p, pCex, fJustMax );
    if ( nRealPis >= 0 ) // refinement
    {
        pNew = Gia_ManCreateUnate( p, pCex, iFrameStart, nRealPis, fUseAll );
        printf( "%3d : ", iFrameStart );
        Gia_ManPrintStats( pNew, NULL );
        if ( fVerbose )
            Gia_AigerWrite( pNew, "temp.aig", 0, 0 );
        Gia_ManStop( pNew );
    }
    else // CEX min
    {
        for ( f = pCex->iFrame; f >= iFrameStart; f-- )
        {
            pNew = Gia_ManCreateUnate( p, pCex, f, -1, fUseAll );
            printf( "%3d : ", f );
            Gia_ManPrintStats( pNew, NULL );
            if ( fVerbose )
                Gia_AigerWrite( pNew, "temp.aig", 0, 0 );
            Gia_ManStop( pNew );
        }
    }
    Vec_IntFreeP( &p->vTruths );
    p->nTtWords = 0;
    return NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

