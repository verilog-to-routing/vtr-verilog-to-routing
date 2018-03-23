/**CFile****************************************************************

  FileName    [giaShrink6.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Implementation of DAG-aware unmapping for 6-input cuts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaShrink6.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "bool/bdc/bdc.h"
#include "bool/rsb/rsb.h"
//#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static word Truth[8] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000),
    ABC_CONST(0x0000000000000000),
    ABC_CONST(0xFFFFFFFFFFFFFFFF)
};

// fanout structure
typedef struct Shr_Fan_t_ Shr_Fan_t;
struct Shr_Fan_t_
{
    int                iFan;           // fanout ID
    int                Next;           // next structure
};

// operation manager
typedef struct Shr_Man_t_ Shr_Man_t;
struct Shr_Man_t_
{
    Gia_Man_t *        pGia;           // user's AIG
    Gia_Man_t *        pNew;           // constructed AIG
    int                nDivMax;        // max number of divisors
    int                nNewSize;       // max growth size
    // dynamic fanout (can only grow)
    Vec_Wrd_t *        vFanMem;        // fanout memory
    Vec_Int_t *        vObj2Fan;       // fanout
    Shr_Fan_t *        pFanTemp;       // temporary fanout
    // divisors
    Vec_Int_t *        vDivs;          // divisors
    Vec_Int_t *        vPrio;          // priority queue
    Vec_Int_t *        vDivResub;      // resubstitution
    Vec_Int_t *        vLeaves;        // cut leaves
    // truth tables
    Vec_Wrd_t *        vTruths;        // truth tables
    Vec_Wrd_t *        vDivTruths;     // truth tables
    // bidecomposition
    Rsb_Man_t *        pManRsb;
    Bdc_Man_t *        pManDec;       
    Bdc_Par_t          Pars;
    // statistics 
};

#define Shr_ObjForEachFanout( p, iNode, iFan )                      \
    for ( iFan = Shr_ManFanIterStart(p, iNode); iFan; iFan = Shr_ManFanIterNext(p) )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Shr_Man_t * Shr_ManAlloc( Gia_Man_t * pGia )
{
    Shr_Man_t * p;
    p = ABC_CALLOC( Shr_Man_t, 1 );
    p->nDivMax     = 64;
    p->nNewSize    = 2 * Gia_ManObjNum(pGia);
    p->pGia        = pGia;
    p->vFanMem     = Vec_WrdAlloc( 1000 );   Vec_WrdPush(p->vFanMem, -1); 
    p->vObj2Fan    = Vec_IntStart( p->nNewSize );
    p->vDivs       = Vec_IntAlloc( 1000 );
    p->vPrio       = Vec_IntAlloc( 1000 );
    p->vTruths     = Vec_WrdStart( p->nNewSize );
    p->vDivTruths  = Vec_WrdAlloc( 100 );
    p->vDivResub   = Vec_IntAlloc( 6 );
    p->vLeaves     = Vec_IntAlloc( 6 );
    // start new manager
    p->pNew        = Gia_ManStart( p->nNewSize );
    p->pNew->pName = Abc_UtilStrsav( pGia->pName );
    p->pNew->pSpec = Abc_UtilStrsav( pGia->pSpec );
    Gia_ManHashAlloc( p->pNew );
    Gia_ManCleanLevels( p->pNew, p->nNewSize );
    // allocate traversal IDs
    p->pNew->nObjs = p->nNewSize;
    Gia_ManIncrementTravId( p->pNew );
    p->pNew->nObjs = 1;
    // start decompostion
    p->Pars.nVarsMax = 6;
    p->Pars.fVerbose = 0;
    p->pManDec = Bdc_ManAlloc( &p->Pars );
    p->pManRsb = Rsb_ManAlloc( 6, p->nDivMax, 4, 1 );
    return p;
}
Gia_Man_t * Shr_ManFree( Shr_Man_t * p )
{
    // prepare the manager
    Gia_Man_t * pTemp;
    Gia_ManHashStop( p->pNew );
    Vec_IntFreeP( &p->pNew->vLevels );
    if ( Gia_ManHasDangling(p->pNew) )
    {
        p->pNew = Gia_ManCleanup( pTemp = p->pNew );
        if ( Gia_ManAndNum(p->pNew) != Gia_ManAndNum(pTemp) )
            printf( "Node reduction after sweep %6d -> %6d.\n", Gia_ManAndNum(pTemp), Gia_ManAndNum(p->pNew) );
        Gia_ManStop( pTemp );
    }
    Gia_ManSetRegNum( p->pNew, Gia_ManRegNum(p->pGia) );
    pTemp = p->pNew; p->pNew = NULL;
    // free data structures
    Rsb_ManFree( p->pManRsb );
    Bdc_ManFree( p->pManDec );
    Gia_ManStopP( &p->pNew );
    Vec_WrdFree( p->vFanMem );
    Vec_IntFree( p->vObj2Fan );
    Vec_IntFree( p->vDivs );
    Vec_IntFree( p->vPrio );
    Vec_WrdFree( p->vTruths );
    Vec_WrdFree( p->vDivTruths );
    Vec_IntFree( p->vDivResub );
    Vec_IntFree( p->vLeaves );
    ABC_FREE( p );
    return pTemp;
}


/**Function*************************************************************

  Synopsis    [Fanout manipulation.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Shr_ManAddFanout( Shr_Man_t * p, int iFanin, int iFanout )
{
    union {
        Shr_Fan_t sFan;
        word      sWord;
    } FanStr;
    FanStr.sFan.iFan = iFanout;
    FanStr.sFan.Next = Vec_IntEntry(p->vObj2Fan, iFanin);
    Vec_IntWriteEntry( p->vObj2Fan, iFanin, Vec_WrdSize(p->vFanMem) );
    Vec_WrdPush(p->vFanMem, FanStr.sWord );
}
static inline int Shr_ManFanIterStart( Shr_Man_t * p, int iNode )
{
    if ( Vec_IntEntry(p->vObj2Fan, iNode) == 0 )
        return 0;
    p->pFanTemp = (Shr_Fan_t *)Vec_WrdEntryP( p->vFanMem, Vec_IntEntry(p->vObj2Fan, iNode) );
    return p->pFanTemp->iFan;
}
static inline int Shr_ManFanIterNext( Shr_Man_t * p )
{
    if ( p->pFanTemp->Next == 0 )
        return 0;
    p->pFanTemp = (Shr_Fan_t *)Vec_WrdEntryP( p->vFanMem, p->pFanTemp->Next );
    return p->pFanTemp->iFan;
}

/**Function*************************************************************

  Synopsis    [Collect divisors.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Shr_ManDivPushOrderByLevel( Shr_Man_t * p, int iDiv )
{
    int iPlace, * pArray;
    Vec_IntPush( p->vPrio, iDiv );
    if ( Vec_IntSize(p->vPrio) == 1 )
        return 0; 
    pArray = Vec_IntArray(p->vPrio);
    for ( iPlace = Vec_IntSize(p->vPrio) - 1; iPlace > 0; iPlace-- )
        if ( Gia_ObjLevel(p->pNew, Gia_ManObj(p->pNew, pArray[iPlace-1])) > 
             Gia_ObjLevel(p->pNew, Gia_ManObj(p->pNew, pArray[iPlace])) )
             ABC_SWAP( int, pArray[iPlace-1], pArray[iPlace] )
        else
            break;
    return iPlace; // the place of the new divisor
}
static inline int Shr_ManCollectDivisors( Shr_Man_t * p, Vec_Int_t * vLeaves, int Limit, int nFanoutMax )
{
    Gia_Obj_t * pFan;
    int i, c, iDiv, iFan, iPlace;
    assert( Limit > 6 );
    Vec_IntClear( p->vDivs );
    Vec_IntClear( p->vPrio );
    Gia_ManIncrementTravId( p->pNew );
    Vec_IntForEachEntry( vLeaves, iDiv, i )
    {
        Vec_IntPush( p->vDivs, iDiv );
        Shr_ManDivPushOrderByLevel( p, iDiv );
        Gia_ObjSetTravIdCurrentId( p->pNew, iDiv );
    }
    Vec_IntForEachEntry( p->vPrio, iDiv, i )
    {
        c = 0;
        assert( Gia_ObjIsTravIdCurrentId(p->pNew, iDiv) );
        Shr_ObjForEachFanout( p, iDiv, iFan )
        {
            if ( c++ == nFanoutMax )
                break;
            if ( Gia_ObjIsTravIdCurrentId(p->pNew, iFan) )
                continue;
            pFan = Gia_ManObj( p->pNew, iFan );
            assert( Gia_ObjIsAnd(pFan) );
            assert( Gia_ObjLevel(p->pNew, Gia_ManObj(p->pNew, iDiv)) < Gia_ObjLevel(p->pNew, pFan) );
            if ( !Gia_ObjIsTravIdCurrentId(p->pNew, Gia_ObjFaninId0(pFan, iFan)) ||
                 !Gia_ObjIsTravIdCurrentId(p->pNew, Gia_ObjFaninId1(pFan, iFan)) )
                continue;
            Vec_IntPush( p->vDivs, iFan );
            Gia_ObjSetTravIdCurrentId( p->pNew, iFan );
            iPlace = Shr_ManDivPushOrderByLevel( p, iFan );
            assert( i < iPlace );
            if ( Vec_IntSize(p->vDivs) == Limit )
                return Vec_IntSize(p->vDivs);
        }
    }
    return Vec_IntSize(p->vDivs);
}

/**Function*************************************************************

  Synopsis    [Resynthesizes nodes using bi-decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Shr_ObjPerformBidec( Shr_Man_t * p, Bdc_Man_t * pManDec, Gia_Man_t * pNew, Vec_Int_t * vLeafLits, word uTruth1, word uTruthC )
{
    Bdc_Fun_t * pFunc;
    Gia_Obj_t * pObj;
    int i, iVar, iLit, nNodes, iLast;
    int nVars = Vec_IntSize(vLeafLits);
    assert( uTruth1 != 0 && uTruthC != 0 );
    Bdc_ManDecompose( pManDec, (unsigned *)&uTruth1, (unsigned *)&uTruthC, nVars, NULL, 1000 );
    Bdc_FuncSetCopyInt( Bdc_ManFunc(pManDec, 0), 1 );
    Vec_IntForEachEntry( vLeafLits, iVar, i )
        Bdc_FuncSetCopyInt( Bdc_ManFunc(pManDec, i+1), Abc_Var2Lit(iVar, 0) );
    nNodes = Bdc_ManNodeNum( pManDec );
    for ( i = nVars + 1; i < nNodes; i++ )
    {
        pFunc = Bdc_ManFunc( pManDec, i );
        iLast = Gia_ManObjNum(pNew);
        iLit  = Gia_ManHashAnd( pNew, Bdc_FunFanin0Copy(pFunc), Bdc_FunFanin1Copy(pFunc) );
        Bdc_FuncSetCopyInt( pFunc, iLit );
        if ( iLast == Gia_ManObjNum(pNew) )
            continue;
        assert( iLast + 1 == Gia_ManObjNum(pNew) );
        pObj = Gia_ManObj(pNew, Abc_Lit2Var(iLit));
        Gia_ObjSetAndLevel( pNew, pObj );
        Shr_ManAddFanout( p, Gia_ObjFaninId0p(pNew, pObj), Gia_ObjId(pNew, pObj) );
        Shr_ManAddFanout( p, Gia_ObjFaninId1p(pNew, pObj), Gia_ObjId(pNew, pObj) );
        assert( Gia_ManObjNum(pNew) < p->nNewSize );
    }
    return Bdc_FunObjCopy( Bdc_ManRoot(pManDec) );
}


/**Function*************************************************************

  Synopsis    [Compute truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Shr_ManComputeTruth6_rec( Gia_Man_t * p, int iNode, Vec_Wrd_t * vTruths )
{
    Gia_Obj_t * pObj;
    word Truth0, Truth1;
    if ( Gia_ObjIsTravIdCurrentId(p, iNode) )
        return Vec_WrdEntry(vTruths, iNode);
    Gia_ObjSetTravIdCurrentId(p, iNode);
    pObj = Gia_ManObj( p, iNode );
    assert( Gia_ObjIsAnd(pObj) );
    Truth0 = Shr_ManComputeTruth6_rec( p, Gia_ObjFaninId0p(p, pObj), vTruths );
    Truth1 = Shr_ManComputeTruth6_rec( p, Gia_ObjFaninId1p(p, pObj), vTruths );
    if ( Gia_ObjFaninC0(pObj) )
        Truth0 = ~Truth0;
    if ( Gia_ObjFaninC1(pObj) )
        Truth1 = ~Truth1;
    Vec_WrdWriteEntry( vTruths, iNode, Truth0 & Truth1 );
    return Truth0 & Truth1;
}
word Shr_ManComputeTruth6( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vLeaves, Vec_Wrd_t * vTruths )
{
    int i, iLeaf;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vLeaves, iLeaf, i )
    {
        Gia_ObjSetTravIdCurrentId( p, iLeaf );
        Vec_WrdWriteEntry( vTruths, iLeaf, Truth[i] );
    }
    return Shr_ManComputeTruth6_rec( p, Gia_ObjId(p, pObj), vTruths );
}

/**Function*************************************************************

  Synopsis    [Compute truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Shr_ManComputeTruths( Gia_Man_t * p, int nVars, Vec_Int_t * vDivs, Vec_Wrd_t * vDivTruths, Vec_Wrd_t * vTruths )
{
    Gia_Obj_t * pObj;
    word Truth0, Truth1;//, Truthh;
    int i, iDiv;
    Vec_WrdClear( vDivTruths );
    Vec_IntForEachEntryStop( vDivs, iDiv, i, nVars )
    {
        Vec_WrdWriteEntry( vTruths, iDiv, Truth[i] );
        Vec_WrdPush( vDivTruths, Truth[i] );
    }
    Vec_IntForEachEntryStart( vDivs, iDiv, i, nVars )
    {
        pObj = Gia_ManObj( p, iDiv );
        Truth0 = Vec_WrdEntry( vTruths, Gia_ObjFaninId0(pObj, iDiv) );
        Truth1 = Vec_WrdEntry( vTruths, Gia_ObjFaninId1(pObj, iDiv) );
        if ( Gia_ObjFaninC0(pObj) )
            Truth0 = ~Truth0;
        if ( Gia_ObjFaninC1(pObj) )
            Truth1 = ~Truth1;
        Vec_WrdWriteEntry( vTruths, iDiv, Truth0 & Truth1 );
        Vec_WrdPush( vDivTruths, Truth0 & Truth1 );

//        Truthh = Truth0 & Truth1;
//        Abc_TtPrintBinary( &Truthh, nVars );   //printf( "\n" );
//        Kit_DsdPrintFromTruth( &Truthh, nVars );    printf( "\n" );
    }
//    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManMapShrink6( Gia_Man_t * p, int nFanoutMax, int fKeepLevel, int fVerbose )
{
    Shr_Man_t * pMan;
    Gia_Obj_t * pObj, * pFanin;
    word uTruth, uTruth0, uTruth1;
    int i, k, nDivs, iNode;
    int RetValue, Counter1 = 0, Counter2 = 0;
    abctime clk2, clk = Abc_Clock();
    abctime timeFanout = 0;
    assert( Gia_ManHasMapping(p) );
    pMan = Shr_ManAlloc( p );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
        {
            pObj->Value = Gia_ManAppendCi( pMan->pNew );
            if ( p->vLevels )
                Gia_ObjSetLevel( pMan->pNew, Gia_ObjFromLit(pMan->pNew, Gia_ObjValue(pObj)), Gia_ObjLevel(p, pObj) );
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            pObj->Value = Gia_ManAppendCo( pMan->pNew, Gia_ObjFanin0Copy(pObj) );
        }
        else if ( Gia_ObjIsLut(p, i) )
        {
            // collect leaves of this gate  
            Vec_IntClear( pMan->vLeaves );
            Gia_LutForEachFanin( p, i, iNode, k )
                Vec_IntPush( pMan->vLeaves, iNode );
            assert( Vec_IntSize(pMan->vLeaves) <= 6 );
            // compute truth table 
            uTruth = Shr_ManComputeTruth6( pMan->pGia, pObj, pMan->vLeaves, pMan->vTruths );
            assert( pObj->Value == ~0 );
            if ( uTruth == 0 || ~uTruth == 0 )
                pObj->Value = Abc_LitNotCond( 0, ~uTruth == 0 );
            else
                Gia_ManForEachObjVec( pMan->vLeaves, p, pFanin, k )
                    if ( uTruth == Truth[k] || ~uTruth == Truth[k] )
                        pObj->Value = Abc_LitNotCond( pFanin->Value, ~uTruth == Truth[k] );
            if ( pObj->Value != ~0 )
                continue;
            // translate into new nodes
            Gia_ManForEachObjVec( pMan->vLeaves, p, pFanin, k )
            {
                if ( Abc_LitIsCompl(pFanin->Value) )
                    uTruth = ((uTruth & Truth[k]) >> (1 << k)) | ((uTruth & ~Truth[k]) << (1 << k));
                Vec_IntWriteEntry( pMan->vLeaves, k, Abc_Lit2Var(pFanin->Value) );
            }
            // compute divisors
            clk2 = Abc_Clock();
            nDivs = Shr_ManCollectDivisors( pMan, pMan->vLeaves, pMan->nDivMax, nFanoutMax );
            assert( nDivs <= pMan->nDivMax );
            timeFanout += Abc_Clock() - clk2;
            // compute truth tables
            Shr_ManComputeTruths( pMan->pNew, Vec_IntSize(pMan->vLeaves), pMan->vDivs, pMan->vDivTruths, pMan->vTruths );
            // perform resubstitution
            RetValue = Rsb_ManPerformResub6( pMan->pManRsb, Vec_IntSize(pMan->vLeaves), uTruth, pMan->vDivTruths, &uTruth0, &uTruth1, 0 );
            if ( RetValue ) // resub exists
            {
                Vec_Int_t * vResult = Rsb_ManGetFanins(pMan->pManRsb);
                Vec_IntClear( pMan->vDivResub );
                Vec_IntForEachEntry( vResult, iNode, k )
                    Vec_IntPush( pMan->vDivResub, Vec_IntEntry(pMan->vDivs, iNode) );
                pObj->Value = Shr_ObjPerformBidec( pMan, pMan->pManDec, pMan->pNew, pMan->vDivResub, uTruth1, uTruth0 | uTruth1 );
                Counter1++;
            }
            else
            {
                pObj->Value = Shr_ObjPerformBidec( pMan, pMan->pManDec, pMan->pNew, pMan->vLeaves, uTruth, ~(word)0 );
                Counter2++;
            }
        }
    }
    if ( fVerbose )
    {
        printf( "Performed %d resubs and %d decomps.  ", Counter1, Counter2 );
        printf( "Gain in AIG nodes = %d.  ", Gia_ManObjNum(p)-Gia_ManObjNum(pMan->pNew) );
        ABC_PRT( "Runtime", Abc_Clock() - clk );
//        ABC_PRT( "Divisors", timeFanout );        
    }
    return Shr_ManFree( pMan );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

