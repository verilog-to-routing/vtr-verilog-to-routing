/**CFile****************************************************************

  FileName    [absVta.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [Variable time-frame abstraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absVta.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sat/bsat/satSolver2.h"
#include "base/main/main.h"
#include "abs.h"

ABC_NAMESPACE_IMPL_START

#define VTA_LARGE  0xFFFFFFF

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Vta_Obj_t_ Vta_Obj_t; // object
struct Vta_Obj_t_
{
    int           iObj;
    int           iFrame;
    int           iNext;
    unsigned      Prio   : 28;  // related to VTA_LARGE
    unsigned      Value  :  2;
    unsigned      fAdded :  1;
    unsigned      fVisit :  1;
};

typedef struct Vta_Man_t_ Vta_Man_t; // manager
struct Vta_Man_t_
{
    // user data
    Gia_Man_t *   pGia;         // AIG manager
    Abs_Par_t *   pPars;        // parameters
    // internal data
    int           nObjs;        // the number of objects
    int           nObjsAlloc;   // the number of objects allocated
    int           nBins;        // number of hash table entries
    int *         pBins;        // hash table bins
    Vta_Obj_t *   pObjs;        // storage for objects
    Vec_Int_t *   vOrder;       // objects in DPS order
    // abstraction
    int           nObjBits;     // the number of bits to represent objects
    unsigned      nObjMask;     // object mask
    Vec_Ptr_t *   vFrames;      // start abstraction for each frame
    int           nWords;       // the number of words in the record
    int           nCexes;       // the number of CEXes
    int           nObjAdded;    // objects added to the abstraction
    Vec_Int_t *   vSeens;       // seen objects
    Vec_Bit_t *   vSeenGla;     // seen objects in all frames
    int           nSeenGla;     // seen objects in all frames
    int           nSeenAll;     // seen objects in all frames
    // other data
    Vec_Ptr_t *   vCores;       // unsat core for each frame
    sat_solver2 * pSat;         // incremental SAT solver
    Vec_Int_t *   vAddedNew;    // the IDs of variables added to the solver
    // statistics 
    abctime       timeSat;
    abctime       timeUnsat;
    abctime       timeCex;
    abctime       timeOther;
};


// ternary simulation

#define VTA_VAR0   1
#define VTA_VAR1   2
#define VTA_VARX   3

static inline int Vta_ValIs0( Vta_Obj_t * pThis, int fCompl )
{
    if ( pThis->Value == VTA_VAR1 && fCompl )
        return 1;
    if ( pThis->Value == VTA_VAR0 && !fCompl )
        return 1;
    return 0;
}
static inline int Vta_ValIs1( Vta_Obj_t * pThis, int fCompl )
{
    if ( pThis->Value == VTA_VAR0 && fCompl )
        return 1;
    if ( pThis->Value == VTA_VAR1 && !fCompl )
        return 1;
    return 0;
}

static inline Vta_Obj_t *  Vta_ManObj( Vta_Man_t * p, int i )           { assert( i >= 0 && i < p->nObjs ); return i ? p->pObjs + i : NULL;                     }
static inline int          Vta_ObjId( Vta_Man_t * p, Vta_Obj_t * pObj ) { assert( pObj > p->pObjs && pObj < p->pObjs + p->nObjs ); return pObj - p->pObjs;      }

#define Vta_ManForEachObj( p, pObj, i )                                 \
    for ( i = 1; (i < p->nObjs) && ((pObj) = Vta_ManObj(p, i)); i++ )
#define Vta_ManForEachObjObj( p, pObjVta, pObjGia, i )                  \
    for ( i = 1; (i < p->nObjs) && ((pObjVta) = Vta_ManObj(p, i)) && ((pObjGia) = Gia_ManObj(p->pGia, pObjVta->iObj)); i++ )
#define Vta_ManForEachObjObjReverse( p, pObjVta, pObjGia, i )           \
    for ( i = Vec_IntSize(vVec) - 1; (i >= 1) && ((pObjVta) = Vta_ManObj(p, i)) && ((pObjGia) = Gia_ManObj(p->pGia, pObjVta->iObj)); i++ )

#define Vta_ManForEachObjVec( vVec, p, pObj, i )                        \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pObj) = Vta_ManObj(p, Vec_IntEntry(vVec,i))); i++ )
#define Vta_ManForEachObjVecReverse( vVec, p, pObj, i )                 \
    for ( i = Vec_IntSize(vVec) - 1; (i >= 0) && ((pObj) = Vta_ManObj(p, Vec_IntEntry(vVec,i))); i-- )

#define Vta_ManForEachObjObjVec( vVec, p, pObj, pObjG, i )              \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pObj) = Vta_ManObj(p, Vec_IntEntry(vVec,i))) && ((pObjG) = Gia_ManObj(p->pGia, pObj->iObj)); i++ )
#define Vta_ManForEachObjObjVecReverse( vVec, p, pObj, pObjG, i )       \
    for ( i = Vec_IntSize(vVec) - 1; (i >= 0) && ((pObj) = Vta_ManObj(p, Vec_IntEntry(vVec,i))) && ((pObjG) = Gia_ManObj(p->pGia, pObj->iObj)); i-- )


// abstraction is given as an array of integers:
// - the first entry is the number of timeframes (F)
// - the next (F+1) entries give the beginning position of each timeframe
// - the following entries give the object IDs
// invariant:  assert( vec[vec[0]+1] == size(vec) );

extern void Vga_ManAddClausesOne( Vta_Man_t * p, int iObj, int iFrame );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Converting from one array to per-frame arrays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Gia_VtaAbsToFrames( Vec_Int_t * vAbs )
{
    Vec_Ptr_t * vFrames;
    Vec_Int_t * vFrame;
    int i, k, Entry, iStart, iStop = -1;
    int nFrames = Vec_IntEntry( vAbs, 0 );
    assert( Vec_IntEntry(vAbs, nFrames+1) == Vec_IntSize(vAbs) );
    vFrames = Vec_PtrAlloc( nFrames );
    for ( i = 0; i < nFrames; i++ )
    {
        iStart = Vec_IntEntry( vAbs, i+1 );
        iStop  = Vec_IntEntry( vAbs, i+2 );
        vFrame = Vec_IntAlloc( iStop - iStart );
        Vec_IntForEachEntryStartStop( vAbs, Entry, k, iStart, iStop )
            Vec_IntPush( vFrame, Entry );
        Vec_PtrPush( vFrames, vFrame );
    }
    assert( iStop == Vec_IntSize(vAbs) );
    return vFrames;
}

/**Function*************************************************************

  Synopsis    [Converting from per-frame arrays to one integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_VtaFramesToAbs( Vec_Vec_t * vFrames )
{
    Vec_Int_t * vOne, * vAbs;
    int i, k, Entry, nSize;
    vAbs = Vec_IntAlloc( 2 + Vec_VecSize(vFrames) + Vec_VecSizeSize(vFrames) );
    Vec_IntPush( vAbs, Vec_VecSize(vFrames) );
    nSize = Vec_VecSize(vFrames) + 2;
    Vec_VecForEachLevelInt( vFrames, vOne, i )
    {
        Vec_IntPush( vAbs, nSize );
        nSize += Vec_IntSize( vOne );
    }
    Vec_IntPush( vAbs, nSize );
    assert( Vec_IntSize(vAbs) == Vec_VecSize(vFrames) + 2 );
    Vec_VecForEachLevelInt( vFrames, vOne, i )
        Vec_IntForEachEntry( vOne, Entry, k )
            Vec_IntPush( vAbs, Entry );
    assert( Vec_IntEntry(vAbs, Vec_IntEntry(vAbs,0)+1) == Vec_IntSize(vAbs) );
    return vAbs;
}

/**Function*************************************************************

  Synopsis    [Detects how many frames are completed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Vec_Int_t * Vta_ManDeriveAbsAll( Vec_Int_t * p, int nWords )
{
    Vec_Int_t * vRes;
    unsigned * pThis;
    int i, w, nObjs = Vec_IntSize(p) / nWords;
    assert( Vec_IntSize(p) % nWords == 0 );
    vRes = Vec_IntAlloc( nObjs );
    for ( i = 0; i < nObjs; i++ )
    {
        pThis = (unsigned *)Vec_IntEntryP( p, nWords * i );
        for ( w = 0; w < nWords; w++ )
            if ( pThis[w] )
                break;
        Vec_IntPush( vRes, (int)(w < nWords) );
    }
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Collect nodes/flops involved in different timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vec_IntDoubleWidth( Vec_Int_t * p, int nWords )
{
    int * pArray = ABC_CALLOC( int, Vec_IntSize(p) * 2 );
    int i, w, nObjs = Vec_IntSize(p) / nWords;
    assert( Vec_IntSize(p) % nWords == 0 );
    for ( i = 0; i < nObjs; i++ )
        for ( w = 0; w < nWords; w++ )
            pArray[2 * nWords * i + w] = p->pArray[nWords * i + w];
    ABC_FREE( p->pArray );
    p->pArray = pArray;
    p->nSize *= 2;
    p->nCap = p->nSize;
    return 2 * nWords;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vga_ManHash( int iObj, int iFrame, int nBins )
{
    return ((unsigned)((iObj + iFrame)*(iObj + iFrame + 1))) % nBins;
}
static inline int * Vga_ManLookup( Vta_Man_t * p, int iObj, int iFrame )
{
    Vta_Obj_t * pThis;
    int * pPlace = p->pBins + Vga_ManHash( iObj, iFrame, p->nBins );
    for ( pThis = Vta_ManObj(p, *pPlace); 
          pThis;  pPlace = &pThis->iNext, 
          pThis = Vta_ManObj(p, *pPlace) )
        if ( pThis->iObj == iObj && pThis->iFrame == iFrame )
            break;
    return pPlace;
}
static inline Vta_Obj_t * Vga_ManFind( Vta_Man_t * p, int iObj, int iFrame )
{
    int * pPlace = Vga_ManLookup( p, iObj, iFrame );
    return Vta_ManObj(p, *pPlace);
}
static inline Vta_Obj_t * Vga_ManFindOrAdd( Vta_Man_t * p, int iObj, int iFrame )
{
    Vta_Obj_t * pThis;
    int i, * pPlace;
    assert( iObj >= 0 && iFrame >= -1 );
    if ( p->nObjs == p->nObjsAlloc )
    {
        // resize objects
        p->pObjs = ABC_REALLOC( Vta_Obj_t, p->pObjs, 2 * p->nObjsAlloc );
        memset( p->pObjs + p->nObjsAlloc, 0, p->nObjsAlloc * sizeof(Vta_Obj_t) );
        p->nObjsAlloc *= 2;
        // rehash entries in the table
        ABC_FREE( p->pBins );
        p->nBins = Abc_PrimeCudd( 2 * p->nBins );
        p->pBins = ABC_CALLOC( int, p->nBins );
        Vta_ManForEachObj( p, pThis, i )
        {
            pThis->iNext = 0;
            pPlace = Vga_ManLookup( p, pThis->iObj, pThis->iFrame );
            assert( *pPlace == 0 );
            *pPlace = i;
        }
    }
    pPlace = Vga_ManLookup( p, iObj, iFrame );
    if ( *pPlace )
        return Vta_ManObj(p, *pPlace);
    *pPlace = p->nObjs++;
    pThis = Vta_ManObj(p, *pPlace);
    pThis->iObj   = iObj;
    pThis->iFrame = iFrame;
    return pThis;
}
static inline void Vga_ManDelete( Vta_Man_t * p, int iObj, int iFrame )
{
    int * pPlace = Vga_ManLookup( p, iObj, iFrame );
    Vta_Obj_t * pThis = Vta_ManObj(p, *pPlace);
    assert( pThis != NULL );
    *pPlace = pThis->iNext;
    pThis->iNext = -1;
}


/**Function*************************************************************

  Synopsis    [Derives counter-example using current assignments.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Vga_ManDeriveCex( Vta_Man_t * p )
{
    Abc_Cex_t * pCex;
    Vta_Obj_t * pThis;
    Gia_Obj_t * pObj;
    int i;
    pCex = Abc_CexAlloc( Gia_ManRegNum(p->pGia), Gia_ManPiNum(p->pGia), p->pPars->iFrame+1 );
    pCex->iPo = 0;
    pCex->iFrame = p->pPars->iFrame;
    Vta_ManForEachObjObj( p, pThis, pObj, i )
        if ( Gia_ObjIsPi(p->pGia, pObj) && sat_solver2_var_value(p->pSat, Vta_ObjId(p, pThis)) )
            Abc_InfoSetBit( pCex->pData, pCex->nRegs + pThis->iFrame * pCex->nPis + Gia_ObjCioId(pObj) );
    return pCex;
}

/**Function*************************************************************

  Synopsis    [Remaps core into frame/node pairs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vta_ManUnsatCoreRemap( Vta_Man_t * p, Vec_Int_t * vCore )
{
    Vta_Obj_t * pThis;
    int i, Entry;
    Vec_IntForEachEntry( vCore, Entry, i )
    {
        pThis = Vta_ManObj( p, Entry );
        Entry = (pThis->iFrame << p->nObjBits) | pThis->iObj;
        Vec_IntWriteEntry( vCore, i, Entry );
    }
}

/**Function*************************************************************

  Synopsis    [Compares two objects by their distance.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vta_ManComputeDepthIncrease( Vta_Obj_t ** pp1, Vta_Obj_t ** pp2 )
{
    int Diff = (*pp1)->Prio - (*pp2)->Prio;
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    Diff = (*pp1) - (*pp2);
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the object is already used.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vta_ManObjIsUsed( Vta_Man_t * p, int iObj )
{
    int i;
    unsigned * pInfo = (unsigned *)Vec_IntEntryP( p->vSeens, p->nWords * iObj );
    for ( i = 0; i < p->nWords; i++ )
        if ( pInfo[i] )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Finds predecessors of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vta_ObjPreds( Vta_Man_t * p, Vta_Obj_t * pThis, Gia_Obj_t * pObj, Vta_Obj_t ** ppThis0, Vta_Obj_t ** ppThis1 )
{
    *ppThis0 = NULL;
    *ppThis1 = NULL;
//    if ( !pThis->fAdded )
//        return;
    assert( !Gia_ObjIsPi(p->pGia, pObj) );
    if ( Gia_ObjIsConst0(pObj) || (Gia_ObjIsCi(pObj) && pThis->iFrame == 0) )
        return;
    if ( Gia_ObjIsAnd(pObj) )
    {
        *ppThis0 = Vga_ManFind( p, Gia_ObjFaninId0p(p->pGia, pObj), pThis->iFrame );
        *ppThis1 = Vga_ManFind( p, Gia_ObjFaninId1p(p->pGia, pObj), pThis->iFrame );
//        assert( *ppThis0 && *ppThis1 );
        return;
    }
    assert( Gia_ObjIsRo(p->pGia, pObj) && pThis->iFrame > 0 );
    pObj = Gia_ObjRoToRi( p->pGia, pObj );
    *ppThis0 = Vga_ManFind( p, Gia_ObjFaninId0p(p->pGia, pObj), pThis->iFrame-1 );
//    assert( *ppThis0 );
}

/**Function*************************************************************

  Synopsis    [Collect const/PI/RO/AND in a topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vta_ManCollectNodes_rec( Vta_Man_t * p, Vta_Obj_t * pThis, Vec_Int_t * vOrder )
{
    Gia_Obj_t * pObj;
    Vta_Obj_t * pThis0, * pThis1;
    if ( pThis->fVisit )
        return;
    pThis->fVisit = 1;
    pObj = Gia_ManObj( p->pGia, pThis->iObj );
    if ( pThis->fAdded )
    {
        Vta_ObjPreds( p, pThis, pObj, &pThis0, &pThis1 );
        if ( pThis0 ) Vta_ManCollectNodes_rec( p, pThis0, vOrder );
        if ( pThis1 ) Vta_ManCollectNodes_rec( p, pThis1, vOrder );
    }
    Vec_IntPush( vOrder, Vta_ObjId(p, pThis) );
}
Vec_Int_t * Vta_ManCollectNodes( Vta_Man_t * p, int f )
{
    Vta_Obj_t * pThis;
    Gia_Obj_t * pObj;
    Vec_IntClear( p->vOrder );
    pObj = Gia_ManPo( p->pGia, 0 );
    pThis = Vga_ManFind( p, Gia_ObjFaninId0p(p->pGia, pObj), f );
    assert( pThis != NULL );
    assert( !pThis->fVisit );
    Vta_ManCollectNodes_rec( p, pThis, p->vOrder );
    assert( pThis->fVisit );
    return p->vOrder;
}

/**Function*************************************************************

  Synopsis    [Refines abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vta_ManSatVerify( Vta_Man_t * p )
{
    Vta_Obj_t * pThis, * pThis0, * pThis1;
    Gia_Obj_t * pObj;
    int i;
    Vta_ManForEachObj( p, pThis, i )
        pThis->Value = (sat_solver2_var_value(p->pSat, i) ? VTA_VAR1 : VTA_VAR0);
    Vta_ManForEachObjObj( p, pThis, pObj, i )
    {
        if ( !pThis->fAdded )
            continue;
        Vta_ObjPreds( p, pThis, pObj, &pThis0, &pThis1 );
        if ( Gia_ObjIsAnd(pObj) )
        {
            if ( pThis->Value == VTA_VAR1 )
                assert( Vta_ValIs1(pThis0, Gia_ObjFaninC0(pObj)) && Vta_ValIs1(pThis1, Gia_ObjFaninC1(pObj)) );
            else if ( pThis->Value == VTA_VAR0 )
                assert( Vta_ValIs0(pThis0, Gia_ObjFaninC0(pObj)) || Vta_ValIs0(pThis1, Gia_ObjFaninC1(pObj)) );
            else assert( 0 );
        }
        else if ( Gia_ObjIsRo(p->pGia, pObj) )
        {
            pObj = Gia_ObjRoToRi( p->pGia, pObj );
            if ( pThis->iFrame == 0 )
                assert( pThis->Value == VTA_VAR0 );
            else if ( pThis->Value == VTA_VAR0 )
                assert( Vta_ValIs0(pThis0, Gia_ObjFaninC0(pObj)) );
            else if ( pThis->Value == VTA_VAR1 )
                assert( Vta_ValIs1(pThis0, Gia_ObjFaninC0(pObj)) );
            else assert( 0 );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Refines abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vta_ManProfileAddition( Vta_Man_t * p, Vec_Int_t * vTermsToAdd )
{
    Vta_Obj_t * pThis;
    Gia_Obj_t * pObj;
    // profile the added ones
    int i, * pCounters = ABC_CALLOC( int, p->pPars->iFrame+1 );
    Vta_ManForEachObjObjVec( vTermsToAdd, p, pThis, pObj, i )
        pCounters[pThis->iFrame]++;
    for ( i = 0; i <= p->pPars->iFrame; i++ )
        Abc_Print( 1, "%2d", pCounters[i] );
    Abc_Print( 1, "***\n" );
}

/**Function*************************************************************

  Synopsis    [Refines abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Vta_ManRefineAbstraction( Vta_Man_t * p, int f )
{
    int fVerify = 0;
    Abc_Cex_t * pCex = NULL;
    Vec_Int_t * vOrder, * vTermsToAdd;
    Vec_Ptr_t * vTermsUsed, * vTermsUnused;
    Vta_Obj_t * pThis, * pThis0, * pThis1, * pTop;
    Gia_Obj_t * pObj;
    int i, Counter;

    if ( fVerify )
    Vta_ManSatVerify( p );

    // collect nodes in a topological order
    vOrder = Vta_ManCollectNodes( p, f );
    Vta_ManForEachObjObjVec( vOrder, p, pThis, pObj, i )
    {
        pThis->Prio = VTA_LARGE;
        pThis->Value = sat_solver2_var_value(p->pSat, Vta_ObjId(p, pThis)) ? VTA_VAR1 : VTA_VAR0;
        pThis->fVisit = 0;
    }

    // verify
    if ( fVerify )
    Vta_ManForEachObjObjVec( vOrder, p, pThis, pObj, i )
    {
        if ( !pThis->fAdded )
            continue;
        Vta_ObjPreds( p, pThis, pObj, &pThis0, &pThis1 );
        if ( Gia_ObjIsAnd(pObj) )
        {
            if ( pThis->Value == VTA_VAR1 )
                assert( Vta_ValIs1(pThis0, Gia_ObjFaninC0(pObj)) && Vta_ValIs1(pThis1, Gia_ObjFaninC1(pObj)) );
            else if ( pThis->Value == VTA_VAR0 )
                assert( Vta_ValIs0(pThis0, Gia_ObjFaninC0(pObj)) || Vta_ValIs0(pThis1, Gia_ObjFaninC1(pObj)) );
            else assert( 0 );
        }
        else if ( Gia_ObjIsRo(p->pGia, pObj) )
        {
            pObj = Gia_ObjRoToRi( p->pGia, pObj );
            if ( pThis->iFrame == 0 )
                assert( pThis->Value == VTA_VAR0 );
            else if ( pThis->Value == VTA_VAR0 )
                assert( Vta_ValIs0(pThis0, Gia_ObjFaninC0(pObj)) );
            else if ( pThis->Value == VTA_VAR1 )
                assert( Vta_ValIs1(pThis0, Gia_ObjFaninC0(pObj)) );
            else assert( 0 );
        }
    }

    // compute distance in reverse order
    pThis = Vta_ManObj( p, Vec_IntEntryLast(vOrder) );
    pThis->Prio  = 1;
    // collect used and unused terms
    vTermsUsed   = Vec_PtrAlloc( 1015 );
    vTermsUnused = Vec_PtrAlloc( 1016 );
    Vta_ManForEachObjObjVecReverse( vOrder, p, pThis, pObj, i )
    {
        // there is no unreachable states
        assert( pThis->Prio < VTA_LARGE );
        // skip constants and PIs
        if ( Gia_ObjIsConst0(pObj) || Gia_ObjIsPi(p->pGia, pObj) )
        {
            pThis->Prio = 0; // set highest priority
            continue;
        }
        // collect terminals
        assert( Gia_ObjIsAnd(pObj) || Gia_ObjIsRo(p->pGia, pObj) );
        if ( !pThis->fAdded )
        {
            assert( pThis->Prio > 0 );
            if ( Vta_ManObjIsUsed(p, pThis->iObj) )
                Vec_PtrPush( vTermsUsed, pThis );
            else
                Vec_PtrPush( vTermsUnused, pThis );
            continue;
        }
        // propagate
        Vta_ObjPreds( p, pThis, pObj, &pThis0, &pThis1 );
        if ( pThis0 ) 
            pThis0->Prio = Abc_MinInt( pThis0->Prio, pThis->Prio + 1 );
        if ( pThis1 ) 
            pThis1->Prio = Abc_MinInt( pThis1->Prio, pThis->Prio + 1 );
    }

/*
    Vta_ManForEachObjObjVecReverse( vOrder, p, pThis, pObj, i )
        if ( pThis->Prio > 0 )
            pThis->Prio = 10;
*/
/*
    // update priorities according to reconvergence counters
    Vec_PtrForEachEntry( Vta_Obj_t *, vTermsUsed, pThis, i )
    {
        Vta_Obj_t * pThis0, * pThis1;
        Gia_Obj_t * pObj = Gia_ManObj( p->pGia, pThis->iObj );
        Vta_ObjPreds( p, pThis, pObj, &pThis0, &pThis1 );
        pThis->Prio += 10000000;
        if ( pThis0 )
            pThis->Prio -= 1000000 * pThis0->fAdded;
        if ( pThis1 )
            pThis->Prio -= 1000000 * pThis1->fAdded;
    }
    Vec_PtrForEachEntry( Vta_Obj_t *, vTermsUnused, pThis, i )
    {
        Vta_Obj_t * pThis0, * pThis1;
        Gia_Obj_t * pObj = Gia_ManObj( p->pGia, pThis->iObj );
        Vta_ObjPreds( p, pThis, pObj, &pThis0, &pThis1 );
        pThis->Prio += 10000000;
        if ( pThis0 )
            pThis->Prio -= 1000000 * pThis0->fAdded;
        if ( pThis1 )
            pThis->Prio -= 1000000 * pThis1->fAdded;
    }
*/


    // update priorities according to reconvergence counters
    Vec_PtrForEachEntry( Vta_Obj_t *, vTermsUsed, pThis, i )
        pThis->Prio = pThis->iObj;
    Vec_PtrForEachEntry( Vta_Obj_t *, vTermsUnused, pThis, i )
        pThis->Prio = pThis->iObj;


    // objects with equal distance should receive priority based on number
    // those objects whose prototypes have been added in other timeframes
    // should have higher priority than the current object
    Vec_PtrSort( vTermsUsed,   (int (*)(const void *, const void *))Vta_ManComputeDepthIncrease );
    Vec_PtrSort( vTermsUnused, (int (*)(const void *, const void *))Vta_ManComputeDepthIncrease );
    if ( Vec_PtrSize(vTermsUsed) > 1 )
    {
        pThis0 = (Vta_Obj_t *)Vec_PtrEntry(vTermsUsed, 0);
        pThis1 = (Vta_Obj_t *)Vec_PtrEntryLast(vTermsUsed);
        assert( pThis0->Prio <= pThis1->Prio );
    }
    // assign the priority based on these orders
    Counter = 1;
    Vec_PtrForEachEntry( Vta_Obj_t *, vTermsUsed, pThis, i )
        pThis->Prio = Counter++;
    Vec_PtrForEachEntry( Vta_Obj_t *, vTermsUnused, pThis, i )
        pThis->Prio = Counter++;
//    Abc_Print( 1, "Used %d  Unused %d\n", Vec_PtrSize(vTermsUsed), Vec_PtrSize(vTermsUnused) );


    // propagate in the direct order
    Vta_ManForEachObjObjVec( vOrder, p, pThis, pObj, i )
    {
        assert( pThis->fVisit == 0 );
        assert( pThis->Prio < VTA_LARGE );
        // skip terminal objects
        if ( !pThis->fAdded )
            continue;
        // assumes that values are assigned!!!
        assert( pThis->Value != 0 );
        // propagate
        if ( Gia_ObjIsAnd(pObj) )
        {
            pThis0 = Vga_ManFind( p, Gia_ObjFaninId0p(p->pGia, pObj), pThis->iFrame );
            pThis1 = Vga_ManFind( p, Gia_ObjFaninId1p(p->pGia, pObj), pThis->iFrame );
            assert( pThis0 && pThis1 );
            if ( pThis->Value == VTA_VAR1 )
            {
                assert( Vta_ValIs1(pThis0, Gia_ObjFaninC0(pObj)) && Vta_ValIs1(pThis1, Gia_ObjFaninC1(pObj)) );
                pThis->Prio = Abc_MaxInt( pThis0->Prio, pThis1->Prio );
            }
            else if ( pThis->Value == VTA_VAR0 )
            {
                if ( Vta_ValIs0(pThis0, Gia_ObjFaninC0(pObj)) && Vta_ValIs0(pThis1, Gia_ObjFaninC1(pObj)) )
                    pThis->Prio = Abc_MinInt( pThis0->Prio, pThis1->Prio ); // choice!!!
                else if ( Vta_ValIs0(pThis0, Gia_ObjFaninC0(pObj)) )
                    pThis->Prio = pThis0->Prio;
                else if ( Vta_ValIs0(pThis1, Gia_ObjFaninC1(pObj)) )
                    pThis->Prio = pThis1->Prio;
                else assert( 0 );
            }
            else assert( 0 );
        }
        else if ( Gia_ObjIsRo(p->pGia, pObj) )
        {
            if ( pThis->iFrame > 0 )
            {
                pObj = Gia_ObjRoToRi( p->pGia, pObj );
                pThis0 = Vga_ManFind( p, Gia_ObjFaninId0p(p->pGia, pObj), pThis->iFrame-1 );
                assert( pThis0 );
                pThis->Prio = pThis0->Prio;
            }
            else
                pThis->Prio = 0;
        }
        else if ( Gia_ObjIsConst0(pObj) )
            pThis->Prio = 0;
        else
            assert( 0 );
    }

    // select important values
    pTop = Vta_ManObj( p, Vec_IntEntryLast(vOrder) );
    pTop->fVisit = 1;
    vTermsToAdd = Vec_IntAlloc( 100 );
    Vta_ManForEachObjObjVecReverse( vOrder, p, pThis, pObj, i )
    {
        if ( !pThis->fVisit )
            continue;
        pThis->fVisit = 0;
        assert( pThis->Prio >= 0 && pThis->Prio <= pTop->Prio );
        // skip terminal objects
        if ( !pThis->fAdded )
        {
            assert( Gia_ObjIsAnd(pObj) || Gia_ObjIsRo(p->pGia, pObj) || Gia_ObjIsConst0(pObj) || Gia_ObjIsPi(p->pGia, pObj) );
            Vec_IntPush( vTermsToAdd, Vta_ObjId(p, pThis) );
            continue;
        }
        // assumes that values are assigned!!!
        assert( pThis->Value != 0 );
        // propagate
        if ( Gia_ObjIsAnd(pObj) )
        {
            pThis0 = Vga_ManFind( p, Gia_ObjFaninId0p(p->pGia, pObj), pThis->iFrame );
            pThis1 = Vga_ManFind( p, Gia_ObjFaninId1p(p->pGia, pObj), pThis->iFrame );
            assert( pThis0 && pThis1 );
            if ( pThis->Value == VTA_VAR1 )
            {
                assert( Vta_ValIs1(pThis0, Gia_ObjFaninC0(pObj)) && Vta_ValIs1(pThis1, Gia_ObjFaninC1(pObj)) );
                assert( pThis0->Prio <= pThis->Prio );
                assert( pThis1->Prio <= pThis->Prio );
                pThis0->fVisit = 1;
                pThis1->fVisit = 1;
            }
            else if ( pThis->Value == VTA_VAR0 )
            {
                if ( Vta_ValIs0(pThis0, Gia_ObjFaninC0(pObj)) && Vta_ValIs0(pThis1, Gia_ObjFaninC1(pObj)) )
                {
                    if ( pThis0->fVisit )
                    {
                    }
                    else if ( pThis1->fVisit )
                    {
                    }
                    else if ( pThis0->Prio <= pThis1->Prio ) // choice!!!
                    {
                        pThis0->fVisit = 1;
                        assert( pThis0->Prio == pThis->Prio );
                    }
                    else
                    {
                        pThis1->fVisit = 1;
                        assert( pThis1->Prio == pThis->Prio );
                    }
                }
                else if ( Vta_ValIs0(pThis0, Gia_ObjFaninC0(pObj)) )
                {
                    pThis0->fVisit = 1;
                    assert( pThis0->Prio == pThis->Prio );
                }
                else if ( Vta_ValIs0(pThis1, Gia_ObjFaninC1(pObj)) )
                {
                    pThis1->fVisit = 1;
                    assert( pThis1->Prio == pThis->Prio );
                }
                else assert( 0 );
            }
            else assert( 0 );
        }
        else if ( Gia_ObjIsRo(p->pGia, pObj) )
        {
            if ( pThis->iFrame > 0 )
            {
                pObj = Gia_ObjRoToRi( p->pGia, pObj );
                pThis0 = Vga_ManFind( p, Gia_ObjFaninId0p(p->pGia, pObj), pThis->iFrame-1 );
                assert( pThis0 );
                pThis0->fVisit = 1;
                assert( pThis0->Prio == pThis->Prio );
            }
        }
        else if ( !Gia_ObjIsConst0(pObj) )
            assert( 0 );
    }

    if ( p->pPars->fAddLayer )
    {
        // mark those currently included
        Vta_ManForEachObjVec( vTermsToAdd, p, pThis, i )
        {
            assert( pThis->fVisit == 0 );
            pThis->fVisit = 1;
        }
        // add used terms, which have close relationship
        Counter = Vec_IntSize(vTermsToAdd);
        Vec_PtrForEachEntry( Vta_Obj_t *, vTermsUsed, pThis, i )
        {
            if ( pThis->fVisit )
                continue;
    //        Vta_ObjPreds( p, pThis, Gia_ManObj(p->pGia, pThis->iObj), &pThis0, &pThis1 );
    //        if ( (pThis0 && (pThis0->fAdded || pThis0->fVisit)) || (pThis1 && (pThis1->fAdded || pThis1->fVisit)) )
                Vec_IntPush( vTermsToAdd, Vta_ObjId(p, pThis) );
        }
        // remove those currenty included
        Vta_ManForEachObjVec( vTermsToAdd, p, pThis, i )
            pThis->fVisit = 0;
    }
//    printf( "\n%d -> %d\n", Counter, Vec_IntSize(vTermsToAdd) );
//Vec_IntReverseOrder( vTermsToAdd );
//Vec_IntSort( vTermsToAdd, 1 );


    // cleanup
    Vec_PtrFree( vTermsUsed );
    Vec_PtrFree( vTermsUnused );


    if ( fVerify )
    {
    // verify
    Vta_ManForEachObjVec( vOrder, p, pThis, i )
        pThis->Value = VTA_VARX;
    Vta_ManForEachObjVec( vTermsToAdd, p, pThis, i )
    {
        assert( !pThis->fAdded );
        pThis->Value = sat_solver2_var_value(p->pSat, Vta_ObjId(p, pThis)) ? VTA_VAR1 : VTA_VAR0;
    }
    // simulate 
    Vta_ManForEachObjObjVec( vOrder, p, pThis, pObj, i )
    {
        assert( pThis->fVisit == 0 );
        if ( !pThis->fAdded )
            continue;
        if ( Gia_ObjIsAnd(pObj) )
        {
            pThis0 = Vga_ManFind( p, Gia_ObjFaninId0p(p->pGia, pObj), pThis->iFrame );
            pThis1 = Vga_ManFind( p, Gia_ObjFaninId1p(p->pGia, pObj), pThis->iFrame );
            assert( pThis0 && pThis1 );
            if ( Vta_ValIs1(pThis0, Gia_ObjFaninC0(pObj)) && Vta_ValIs1(pThis1, Gia_ObjFaninC1(pObj)) )
                pThis->Value = VTA_VAR1;
            else if ( Vta_ValIs0(pThis0, Gia_ObjFaninC0(pObj)) || Vta_ValIs0(pThis1, Gia_ObjFaninC1(pObj)) )
                pThis->Value = VTA_VAR0;
            else
                pThis->Value = VTA_VARX;
        }
        else if ( Gia_ObjIsRo(p->pGia, pObj) )
        {
            if ( pThis->iFrame > 0 )
            {
                pObj = Gia_ObjRoToRi( p->pGia, pObj );
                pThis0 = Vga_ManFind( p, Gia_ObjFaninId0p(p->pGia, pObj), pThis->iFrame-1 );
                assert( pThis0 );
                if ( Vta_ValIs0(pThis0, Gia_ObjFaninC0(pObj)) )
                    pThis->Value = VTA_VAR0;
                else if ( Vta_ValIs1(pThis0, Gia_ObjFaninC0(pObj)) )
                    pThis->Value = VTA_VAR1;
                else
                    pThis->Value = VTA_VARX;
            }
            else
            {
                pThis->Value = VTA_VAR0;
            }
        }
        else if ( Gia_ObjIsConst0(pObj) )
        {
            pThis->Value = VTA_VAR0;
        }
        else assert( 0 );
        // double check the solver
        assert( pThis->Value == VTA_VARX || (int)pThis->Value == (sat_solver2_var_value(p->pSat, Vta_ObjId(p, pThis)) ? VTA_VAR1 : VTA_VAR0) );
    }

    // check the output
    if ( !Vta_ValIs1(pTop, Gia_ObjFaninC0(Gia_ManPo(p->pGia, 0))) )
        Abc_Print( 1, "Vta_ManRefineAbstraction(): Terminary simulation verification failed!\n" );
//    else
//        Abc_Print( 1, "Verification OK.\n" );
    }


    // produce true counter-example
    if ( pTop->Prio == 0 )
        pCex = Vga_ManDeriveCex( p );
    else
    {
//       Vta_ManProfileAddition( p, vTermsToAdd );

        Vta_ManForEachObjObjVec( vTermsToAdd, p, pThis, pObj, i )
            if ( !Gia_ObjIsPi(p->pGia, pObj) )
                Vga_ManAddClausesOne( p, pThis->iObj, pThis->iFrame );
        sat_solver2_simplify( p->pSat );
    }
    p->nObjAdded += Vec_IntSize(vTermsToAdd);
    Vec_IntFree( vTermsToAdd );
    return pCex;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vta_Man_t * Vga_ManStart( Gia_Man_t * pGia, Abs_Par_t * pPars )
{
    Vta_Man_t * p;
    p = ABC_CALLOC( Vta_Man_t, 1 );
    p->pGia        = pGia;
    p->pPars       = pPars;
    // internal data
    p->nObjsAlloc  = (1 << 18);
    p->pObjs       = ABC_CALLOC( Vta_Obj_t, p->nObjsAlloc );
    p->nObjs       = 1;
    p->nBins       = Abc_PrimeCudd( 2*p->nObjsAlloc );
    p->pBins       = ABC_CALLOC( int, p->nBins );
    p->vOrder      = Vec_IntAlloc( 1013 );
    // abstraction
    p->nObjBits    = Abc_Base2Log( Gia_ManObjNum(pGia) );
    p->nObjMask    = (1 << p->nObjBits) - 1;
    assert( Gia_ManObjNum(pGia) <= (int)p->nObjMask );
    p->nWords      = 1;
    p->vSeens      = Vec_IntStart( Gia_ManObjNum(pGia) * p->nWords );
    p->vSeenGla    = Vec_BitStart( Gia_ManObjNum(pGia) );
    p->nSeenGla    = 1;
    p->nSeenAll    = 1;
    // other data
    p->vCores      = Vec_PtrAlloc( 100 );
    p->pSat        = sat_solver2_new();
    p->pSat->pPrf1 = Vec_SetAlloc( 20 );
//    p->pSat->fVerbose = p->pPars->fVerbose;
//    sat_solver2_set_learntmax( p->pSat, pPars->nLearnedMax );
    p->pSat->nLearntStart = p->pPars->nLearnedStart;
    p->pSat->nLearntDelta = p->pPars->nLearnedDelta;
    p->pSat->nLearntRatio = p->pPars->nLearnedPerce;
    p->pSat->nLearntMax   = p->pSat->nLearntStart;
    // start the abstraction
    assert( pGia->vObjClasses != NULL );
    p->vFrames = Gia_VtaAbsToFrames( pGia->vObjClasses );
    p->vAddedNew   = Vec_IntAlloc( 1000 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Delete manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vga_ManStop( Vta_Man_t * p )
{
    if ( p->pPars->fVerbose )
        Abc_Print( 1, "SAT solver:  Var = %d  Cla = %d  Conf = %d  Lrn = %d  Reduce = %d  Cex = %d  Objs+ = %d\n", 
            sat_solver2_nvars(p->pSat), sat_solver2_nclauses(p->pSat), sat_solver2_nconflicts(p->pSat), 
            sat_solver2_nlearnts(p->pSat), p->pSat->nDBreduces, p->nCexes, p->nObjAdded );
    Vec_VecFreeP( (Vec_Vec_t **)&p->vCores );
    Vec_VecFreeP( (Vec_Vec_t **)&p->vFrames );
    Vec_BitFreeP( &p->vSeenGla );
    Vec_IntFreeP( &p->vSeens );
    Vec_IntFreeP( &p->vOrder );
    Vec_IntFreeP( &p->vAddedNew );
    sat_solver2_delete( p->pSat );
    ABC_FREE( p->pBins );
    ABC_FREE( p->pObjs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns the output literal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vga_ManGetOutLit( Vta_Man_t * p, int f )
{
    Gia_Obj_t * pObj = Gia_ManPo(p->pGia, 0);
    Vta_Obj_t * pThis = Vga_ManFind( p, Gia_ObjFaninId0p(p->pGia, pObj), f );
    assert( pThis != NULL && pThis->fAdded );
    if ( f == 0 && Gia_ObjIsRo(p->pGia, Gia_ObjFanin0(pObj)) && !Gia_ObjFaninC0(pObj) )
        return -Vta_ObjId(p, pThis);
    return Abc_Var2Lit( Vta_ObjId(p, pThis), Gia_ObjFaninC0(pObj) );
} 

/**Function*************************************************************

  Synopsis    [Finds the set of clauses involved in the UNSAT core.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Vta_ManUnsatCore( int iLit, sat_solver2 * pSat, int nConfMax, int fVerbose, int * piRetValue, int * pnConfls )
{
    abctime clk = Abc_Clock();
    Vec_Int_t * vCore;
    int RetValue, nConfPrev = pSat->stats.conflicts;
    if ( piRetValue )
        *piRetValue = 1;
    // consider special case when PO points to the flop
    // this leads to immediate conflict in the first timeframe
    if ( iLit < 0 )
    {
        vCore = Vec_IntAlloc( 1 );
        Vec_IntPush( vCore, -iLit );
        return vCore;
    }
    // solve the problem
    RetValue = sat_solver2_solve( pSat, &iLit, &iLit+1, (ABC_INT64_T)nConfMax, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( pnConfls )
        *pnConfls = (int)pSat->stats.conflicts - nConfPrev;
    if ( RetValue == l_Undef )
    {
        if ( piRetValue )
            *piRetValue = -1;
        return NULL;
    }
    if ( RetValue == l_True )
    {
        if ( piRetValue )
            *piRetValue = 0;
        return NULL;
    }
    if ( fVerbose )
    {
//        Abc_Print( 1, "%6d", (int)pSat->stats.conflicts - nConfPrev );
//        Abc_Print( 1, "UNSAT after %7d conflicts.      ", pSat->stats.conflicts );
//        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    assert( RetValue == l_False );
    // derive the UNSAT core
    clk = Abc_Clock();
    vCore = (Vec_Int_t *)Sat_ProofCore( pSat );
    if ( fVerbose )
    {
//        Abc_Print( 1, "Core is %8d vars    (out of %8d).   ", Vec_IntSize(vCore), sat_solver2_nvars(pSat) );
//        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    return vCore;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Vta_ManAbsPrintFrame( Vta_Man_t * p, Vec_Int_t * vCore, int nFrames, int nConfls, int nCexes, abctime Time, int fVerbose )
{
    unsigned * pInfo;
    int * pCountAll = NULL, * pCountUni = NULL;
    int i, iFrame, iObj, Entry, fChanges = 0;
    // print info about frames
    if ( vCore )
    {
        pCountAll = ABC_CALLOC( int, nFrames + 1 );
        pCountUni = ABC_CALLOC( int, nFrames + 1 );
        Vec_IntForEachEntry( vCore, Entry, i )
        {
            iObj   = (Entry &  p->nObjMask);
            iFrame = (Entry >> p->nObjBits);
            assert( iFrame < nFrames );
            pInfo  = (unsigned *)Vec_IntEntryP( p->vSeens, p->nWords * iObj );
            if ( !Abc_InfoHasBit(pInfo, iFrame) )
            {
                Abc_InfoSetBit( pInfo, iFrame );
                pCountUni[iFrame+1]++;
                pCountUni[0]++;
                p->nSeenAll++;
            }
            pCountAll[iFrame+1]++;
            pCountAll[0]++;
            if ( !Vec_BitEntry(p->vSeenGla, iObj) )
            {
                Vec_BitWriteEntry(p->vSeenGla, iObj, 1);
                p->nSeenGla++;
                fChanges = 1;
            }
        }
    }
    if ( !fVerbose )
    {
        ABC_FREE( pCountAll );
        ABC_FREE( pCountUni );
        return fChanges;
    }

    if ( Abc_FrameIsBatchMode() && !vCore )
        return fChanges;

//    Abc_Print( 1, "%5d%5d", pCountAll[0], pCountUni[0] ); 
    Abc_Print( 1, "%4d :", nFrames-1 );
    Abc_Print( 1, "%4d", Abc_MinInt(100, 100 * p->nSeenGla / (Gia_ManRegNum(p->pGia) + Gia_ManAndNum(p->pGia) + 1)) ); 
    Abc_Print( 1, "%6d", p->nSeenGla ); 
    Abc_Print( 1, "%4d", Abc_MinInt(100, 100 * p->nSeenAll / (p->nSeenGla * nFrames)) ); 
    Abc_Print( 1, "%8d", nConfls );
    if ( nCexes == 0 )
        Abc_Print( 1, "%5c", '-' ); 
    else
        Abc_Print( 1, "%5d", nCexes ); 
//    Abc_Print( 1, " %9d", sat_solver2_nvars(p->pSat) ); 
    Abc_PrintInt( sat_solver2_nvars(p->pSat) );
    Abc_PrintInt( sat_solver2_nclauses(p->pSat) );
    Abc_PrintInt( sat_solver2_nlearnts(p->pSat) );
    if ( vCore == NULL )
    {
        Abc_Print( 1, "    ..." ); 
//        for ( k = 0; k < 7; k++ )
//            Abc_Print( 1, "     " );
        Abc_Print( 1, "%9.2f sec", 1.0*Time/CLOCKS_PER_SEC );
        Abc_Print( 1, "%5.1f GB", (sat_solver2_memory_proof(p->pSat) + sat_solver2_memory(p->pSat, 0)) / (1<<30) );
        Abc_Print( 1, "\r" );
    }
    else
    {
        Abc_PrintInt( pCountAll[0] );
/*
        if ( nFrames > 7 )
        {
            for ( k = 0; k < 3; k++ )
                Abc_Print( 1, "%5d", pCountAll[k+1] ); 
            Abc_Print( 1, "  ..." );
            for ( k = nFrames-3; k < nFrames; k++ )
                Abc_Print( 1, "%5d", pCountAll[k+1] ); 
        }
        else
        {
            for ( k = 0; k < nFrames; k++ )
                Abc_Print( 1, "%5d", pCountAll[k+1] ); 
            for ( k = nFrames; k < 7; k++ )
                Abc_Print( 1, "     " );
        }
*/
        Abc_Print( 1, "%9.2f sec", 1.0*Time/CLOCKS_PER_SEC );
        Abc_Print( 1, "%5.1f GB", (sat_solver2_memory_proof(p->pSat) + sat_solver2_memory(p->pSat, 0)) / (1<<30) );
        Abc_Print( 1, "\n" );
    }
    fflush( stdout );

    if ( vCore )
    {
        ABC_FREE( pCountAll );
        ABC_FREE( pCountUni );
    }
    return fChanges;
}

/**Function*************************************************************

  Synopsis    [Adds clauses to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vga_ManAddClausesOne( Vta_Man_t * p, int iObj, int iFrame )
{ 
    Vta_Obj_t * pThis0, * pThis1;
    Gia_Obj_t * pObj = Gia_ManObj( p->pGia, iObj );
    Vta_Obj_t * pThis = Vga_ManFindOrAdd( p, iObj, iFrame );
    int iThis0, iMainVar = Vta_ObjId(p, pThis);
    assert( pThis->iObj == iObj && pThis->iFrame == iFrame );
    if ( pThis->fAdded )
        return;
    pThis->fAdded = 1;
    Vec_IntPush( p->vAddedNew, iMainVar );
    if ( Gia_ObjIsAnd(pObj) )
    {
        pThis0 = Vga_ManFindOrAdd( p, Gia_ObjFaninId0p(p->pGia, pObj), iFrame );
        iThis0 = Vta_ObjId(p, pThis0);
        pThis1 = Vga_ManFindOrAdd( p, Gia_ObjFaninId1p(p->pGia, pObj), iFrame );
        sat_solver2_add_and( p->pSat, iMainVar, iThis0, Vta_ObjId(p, pThis1), 
            Gia_ObjFaninC0(pObj), Gia_ObjFaninC1(pObj), 0, iMainVar );
    }
    else if ( Gia_ObjIsRo(p->pGia, pObj) )
    {
        if ( iFrame == 0 )
        {
            if ( p->pPars->fUseTermVars )
            {
                pThis0 = Vga_ManFindOrAdd( p, iObj, -1 );
                sat_solver2_add_constraint( p->pSat, iMainVar, Vta_ObjId(p, pThis0), 1, 0, iMainVar );
            }
            else
            {
                sat_solver2_add_const( p->pSat, iMainVar, 1, 0, iMainVar );
            }
        }
        else
        {
            pObj = Gia_ObjRoToRi( p->pGia, pObj );
            pThis0 = Vga_ManFindOrAdd( p, Gia_ObjFaninId0p(p->pGia, pObj), iFrame-1 );
            sat_solver2_add_buffer( p->pSat, iMainVar, Vta_ObjId(p, pThis0), Gia_ObjFaninC0(pObj), 0, iMainVar );  
        }
    }
    else if ( Gia_ObjIsConst0(pObj) )
    {
        sat_solver2_add_const( p->pSat, iMainVar, 1, 0, iMainVar );
    }
    else //if ( !Gia_ObjIsPi(p->pGia, pObj) )
        assert( 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vga_ManLoadSlice( Vta_Man_t * p, Vec_Int_t * vOne, int Lift )
{
    int i, Entry;
    Vec_IntForEachEntry( vOne, Entry, i )
        Vga_ManAddClausesOne( p, Entry & p->nObjMask, (Entry >> p->nObjBits) + Lift );
    sat_solver2_simplify( p->pSat );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vga_ManPrintCore( Vta_Man_t * p, Vec_Int_t * vCore, int Lift )
{
    int i, Entry, iObj, iFrame;
    Vec_IntForEachEntry( vCore, Entry, i )
    {
        iObj   = (Entry &  p->nObjMask);
        iFrame = (Entry >> p->nObjBits);
        Abc_Print( 1, "%d*%d ", iObj, iFrame+Lift );
    }
    Abc_Print( 1, "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Vga_ManRollBack( Vta_Man_t * p, int nObjOld )
{
    Vta_Obj_t * pThis  = p->pObjs + nObjOld;
    Vta_Obj_t * pLimit = p->pObjs + p->nObjs;
    int i, Entry;
    for ( ; pThis < pLimit; pThis++ )
        Vga_ManDelete( p, pThis->iObj, pThis->iFrame );
    memset( p->pObjs + nObjOld, 0, sizeof(Vta_Obj_t) * (p->nObjs - nObjOld) );
    p->nObjs = nObjOld;
    Vec_IntForEachEntry( p->vAddedNew, Entry, i )
        if ( Entry < p->nObjs )
        {
            pThis = Vta_ManObj(p, Entry);
            assert( pThis->fAdded == 1 );
            pThis->fAdded = 0;
        }
}

/**Function*************************************************************

  Synopsis    [Send abstracted model or send cancel.]

  Description [Counter-example will be sent automatically when &vta terminates.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_VtaSendAbsracted( Vta_Man_t * p, int fVerbose )
{
    Gia_Man_t * pAbs;
    assert( Abc_FrameIsBridgeMode() );
//    if ( fVerbose )
//        Abc_Print( 1, "Sending abstracted model...\n" );
    // create obj classes
    Vec_IntFreeP( &p->pGia->vObjClasses );
    p->pGia->vObjClasses = Gia_VtaFramesToAbs( (Vec_Vec_t *)p->vCores );
    // create gate classes
    Vec_IntFreeP( &p->pGia->vGateClasses );
    p->pGia->vGateClasses = Gia_VtaConvertToGla( p->pGia, p->pGia->vObjClasses );
    Vec_IntFreeP( &p->pGia->vObjClasses );
    // create abstrated model
    pAbs = Gia_ManDupAbsGates( p->pGia, p->pGia->vGateClasses );
    Vec_IntFreeP( &p->pGia->vGateClasses );
    // send it out
    Gia_ManToBridgeAbsNetlist( stdout, pAbs, BRIDGE_ABS_NETLIST );
    Gia_ManStop( pAbs );
}
void Gia_VtaSendCancel( Vta_Man_t * p, int fVerbose )
{
    extern int Gia_ManToBridgeBadAbs( FILE * pFile );
    assert( Abc_FrameIsBridgeMode() );
//    if ( fVerbose )
//        Abc_Print( 1, "Cancelling previously sent model...\n" );
    Gia_ManToBridgeBadAbs( stdout );
}

/**Function*************************************************************

  Synopsis    [Send abstracted model or send cancel.]

  Description [Counter-example will be sent automatically when &vta terminates.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_VtaDumpAbsracted( Vta_Man_t * p, int fVerbose )
{
    char * pFileNameDef = "vabs.aig";
    char * pFileName = p->pPars->pFileVabs ? p->pPars->pFileVabs : pFileNameDef;
    Gia_Man_t * pAbs;
    if ( fVerbose )
        Abc_Print( 1, "Dumping abstracted model into file \"%s\"...\n", pFileName );
    // create obj classes
    Vec_IntFreeP( &p->pGia->vObjClasses );
    p->pGia->vObjClasses = Gia_VtaFramesToAbs( (Vec_Vec_t *)p->vCores );
    // create gate classes
    Vec_IntFreeP( &p->pGia->vGateClasses );
    p->pGia->vGateClasses = Gia_VtaConvertToGla( p->pGia, p->pGia->vObjClasses );
    Vec_IntFreeP( &p->pGia->vObjClasses );
    // create abstrated model
    pAbs = Gia_ManDupAbsGates( p->pGia, p->pGia->vGateClasses );
    Vec_IntFreeP( &p->pGia->vGateClasses );
    // send it out
    Gia_AigerWrite( pAbs, pFileName, 0, 0, 0 );
    Gia_ManStop( pAbs );
}

    
/**Function*************************************************************

  Synopsis    [Print memory report.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_VtaPrintMemory( Vta_Man_t * p )
{
    double memTot = 0;
    double memAig = Gia_ManObjNum(p->pGia) * sizeof(Gia_Obj_t);
    double memSat = sat_solver2_memory( p->pSat, 1 );
    double memPro = sat_solver2_memory_proof( p->pSat );
    double memMap = p->nObjsAlloc * sizeof(Vta_Obj_t) + p->nBins * sizeof(int);
    double memOth = sizeof(Vta_Man_t);
    memOth += Vec_IntCap(p->vOrder) * sizeof(int);
    memOth += Vec_VecMemoryInt( (Vec_Vec_t *)p->vFrames );
    memOth += Vec_BitCap(p->vSeenGla) * sizeof(int);
    memOth += Vec_VecMemoryInt( (Vec_Vec_t *)p->vCores );
    memOth += Vec_IntCap(p->vAddedNew) * sizeof(int);
    memTot = memAig + memSat + memPro + memMap + memOth;
    ABC_PRMP( "Memory: AIG     ", memAig, memTot );
    ABC_PRMP( "Memory: SAT     ", memSat, memTot );
    ABC_PRMP( "Memory: Proof   ", memPro, memTot );
    ABC_PRMP( "Memory: Map     ", memMap, memTot );
    ABC_PRMP( "Memory: Other   ", memOth, memTot );
    ABC_PRMP( "Memory: TOTAL   ", memTot, memTot );
}


/**Function*************************************************************

  Synopsis    [Collect nodes/flops involved in different timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_VtaPerformInt( Gia_Man_t * pAig, Abs_Par_t * pPars )
{
    Vta_Man_t * p;
    Vec_Int_t * vCore;
    Abc_Cex_t * pCex = NULL;
    int i, f, nConfls, Status, nObjOld, RetValue = -1, nCountNoChange = 0, fOneIsSent = 0;
    abctime clk = Abc_Clock(), clk2;
    // preconditions
    assert( Gia_ManPoNum(pAig) == 1 );
    assert( pPars->nFramesMax == 0 || pPars->nFramesStart <= pPars->nFramesMax );
    if ( Gia_ObjIsConst0(Gia_ObjFanin0(Gia_ManPo(pAig,0))) )
    {
        if ( !Gia_ObjFaninC0(Gia_ManPo(pAig,0)) )
        {
            printf( "Sequential miter is trivially UNSAT.\n" );
            return 1;
        } 
        ABC_FREE( pAig->pCexSeq );
        pAig->pCexSeq = Abc_CexMakeTriv( Gia_ManRegNum(pAig), Gia_ManPiNum(pAig), 1, 0 );
        printf( "Sequential miter is trivially SAT.\n" );
        return 0;
    }

    // compute intial abstraction
    if ( pAig->vObjClasses == NULL )
    {
        pAig->vObjClasses = Vec_IntAlloc( 5 );
        Vec_IntPush( pAig->vObjClasses, 1 );
        Vec_IntPush( pAig->vObjClasses, 3 );
        Vec_IntPush( pAig->vObjClasses, 4 );
        Vec_IntPush( pAig->vObjClasses, Gia_ObjFaninId0p(pAig, Gia_ManPo(pAig, 0)) );
    }
    // start the manager
    p = Vga_ManStart( pAig, pPars );
    // set runtime limit
    if ( p->pPars->nTimeOut )
        sat_solver2_set_runtime_limit( p->pSat, p->pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock() );
    // perform initial abstraction
    if ( p->pPars->fVerbose )
    {
        Abc_Print( 1, "Running variable-timeframe abstraction (VTA) with the following parameters:\n" );
        Abc_Print( 1, "FramePast = %d  FrameMax = %d  ConfMax = %d  Timeout = %d  RatioMin = %d %%\n", 
            pPars->nFramesPast, pPars->nFramesMax, pPars->nConfLimit, pPars->nTimeOut, pPars->nRatioMin );
        Abc_Print( 1, "LearnStart = %d  LearnDelta = %d  LearnRatio = %d %%.\n", 
            pPars->nLearnedStart, pPars->nLearnedDelta, pPars->nLearnedPerce );
//        Abc_Print( 1, "Frame   %%   Abs   %%   Confl  Cex    SatVar   Core   F0   F1   F2  ...\n" );
        Abc_Print( 1, " Frame   %%   Abs   %%   Confl  Cex   Vars   Clas   Lrns   Core     Time      Mem\n" );
    }
    assert( Vec_PtrSize(p->vFrames) > 0 );
    for ( f = i = 0; !p->pPars->nFramesMax || f < p->pPars->nFramesMax; f++ )
    {
        int nConflsBeg = sat_solver2_nconflicts(p->pSat);
        p->pPars->iFrame = f;
        // realloc storage for abstraction marks
        if ( f == p->nWords * 32 )
            p->nWords = Vec_IntDoubleWidth( p->vSeens, p->nWords );

        // create bookmark to be used for rollback
        nObjOld = p->nObjs;
        sat_solver2_bookmark( p->pSat );
        Vec_IntClear( p->vAddedNew );

        // load new timeframe
        Vga_ManAddClausesOne( p, 0, f );
        if ( f < Vec_PtrSize(p->vFrames) )
            Vga_ManLoadSlice( p, (Vec_Int_t *)Vec_PtrEntry(p->vFrames, f), 0 );
        else
        {
            for ( i = 1; i <= Abc_MinInt(p->pPars->nFramesPast, f); i++ )
                Vga_ManLoadSlice( p, (Vec_Int_t *)Vec_PtrEntry(p->vCores, f-i), i );
        }

        // iterate as long as there are counter-examples
        for ( i = 0; ; i++ )
        { 
            clk2 = Abc_Clock();
            vCore = Vta_ManUnsatCore( Vga_ManGetOutLit(p, f), p->pSat, pPars->nConfLimit, pPars->fVerbose, &Status, &nConfls );
            assert( (vCore != NULL) == (Status == 1) );
            if ( Status == -1 ) // resource limit is reached
            {
                Vga_ManRollBack( p, nObjOld );
                goto finish;
            }
            // check timeout
            if ( p->pSat->nRuntimeLimit && Abc_Clock() > p->pSat->nRuntimeLimit )
            {
                Vga_ManRollBack( p, nObjOld );
                goto finish;
            }
            if ( vCore != NULL )
            {
                p->timeUnsat += Abc_Clock() - clk2;
                break;
            }
            p->timeSat += Abc_Clock() - clk2;
            assert( Status == 0 );
            p->nCexes++;
            // perform the refinement
            clk2 = Abc_Clock();
            pCex = Vta_ManRefineAbstraction( p, f );
            p->timeCex += Abc_Clock() - clk2;
            if ( pCex != NULL )
                goto finish;
            // print the result (do not count it towards change)
            Vta_ManAbsPrintFrame( p, NULL, f+1, sat_solver2_nconflicts(p->pSat)-nConflsBeg, i, Abc_Clock() - clk, p->pPars->fVerbose );
        }
        assert( Status == 1 );
        // valid core is obtained
        Vta_ManUnsatCoreRemap( p, vCore );
        Vec_IntSort( vCore, 1 );
        // update the SAT solver
        sat_solver2_rollback( p->pSat );
        // update storage
        Vga_ManRollBack( p, nObjOld );
        // load this timeframe
        Vga_ManLoadSlice( p, vCore, 0 );
        Vec_IntFree( vCore );

        // run SAT solver
        clk2 = Abc_Clock();
        vCore = Vta_ManUnsatCore( Vga_ManGetOutLit(p, f), p->pSat, pPars->nConfLimit, p->pPars->fVerbose, &Status, &nConfls );
        p->timeUnsat += Abc_Clock() - clk2;
        assert( (vCore != NULL) == (Status == 1) );
        if ( Status == -1 ) // resource limit is reached
            break;
        if ( Status == 0 )
        {
            Vta_ManSatVerify( p );
            // make sure, there was no initial abstraction (otherwise, it was invalid)
            assert( pAig->vObjClasses == NULL && f < p->pPars->nFramesStart );
            pCex = Vga_ManDeriveCex( p );
            break;
        }
        // add the core
        Vta_ManUnsatCoreRemap( p, vCore );
        // add in direct topological order
        Vec_IntSort( vCore, 1 );
        Vec_PtrPush( p->vCores, vCore );
        // print the result
        if ( Vta_ManAbsPrintFrame( p, vCore, f+1, sat_solver2_nconflicts(p->pSat)-nConflsBeg, i, Abc_Clock() - clk, p->pPars->fVerbose ) )
        {
            // reset the counter of frames without change
            nCountNoChange = 1;
            p->pPars->nFramesNoChange = 0;
        }
        else if ( ++nCountNoChange == 2 ) // time to send
        {
            p->pPars->nFramesNoChange++;
            if ( Abc_FrameIsBridgeMode() )
            {
                // cancel old one if it was sent
                if ( fOneIsSent )
                    Gia_VtaSendCancel( p, pPars->fVerbose );
                // send new one 
                Gia_VtaSendAbsracted( p, pPars->fVerbose );
                fOneIsSent = 1;
            }
        }
        // dump the model
        if ( p->pPars->fDumpVabs && (f & 1) )
        {
            char Command[1000];
            Abc_FrameSetStatus( -1 );
            Abc_FrameSetCex( NULL );
            Abc_FrameSetNFrames( f+1 );
            sprintf( Command, "write_status %s", Extra_FileNameGenericAppend((char *)(p->pPars->pFileVabs ? p->pPars->pFileVabs : "vtabs.aig"), ".status") );
            Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), Command );
            Gia_VtaDumpAbsracted( p, pPars->fVerbose );
        }
        // check if the number of objects is below limit
        if ( p->nSeenGla >= Gia_ManCandNum(pAig) * (100-pPars->nRatioMin) / 100 )
        {
            Status = -1;
            break;
        }
    }
finish:
    // analize the results
    if ( pCex == NULL )
    {
        if ( p->pPars->fVerbose && Status == -1 )
            printf( "\n" );
        if ( Vec_PtrSize(p->vCores) == 0 )
            Abc_Print( 1, "Abstraction is not produced because first frame is not solved.  " );
        else
        {
            assert( Vec_PtrSize(p->vCores) > 0 );
//            if ( pAig->vObjClasses != NULL )
//                Abc_Print( 1, "Replacing the old abstraction by a new one.\n" );
            Vec_IntFreeP( &pAig->vObjClasses );
            pAig->vObjClasses = Gia_VtaFramesToAbs( (Vec_Vec_t *)p->vCores );
            if ( Status == -1 )
            {
                if ( p->pPars->nTimeOut && Abc_Clock() >= p->pSat->nRuntimeLimit ) 
                    Abc_Print( 1, "Timeout %d sec in frame %d with a %d-stable abstraction.    ", p->pPars->nTimeOut, f, p->pPars->nFramesNoChange );
                else if ( pPars->nConfLimit && sat_solver2_nconflicts(p->pSat) >= pPars->nConfLimit )
                    Abc_Print( 1, "Exceeded %d conflicts in frame %d with a %d-stable abstraction.  ", pPars->nConfLimit, f, p->pPars->nFramesNoChange );
                else if ( p->nSeenGla >= Gia_ManCandNum(pAig) * (100-pPars->nRatioMin) / 100 )
                    Abc_Print( 1, "The ratio of abstracted objects is less than %d %% in frame %d.  ", pPars->nRatioMin, f );
                else
                    Abc_Print( 1, "Abstraction stopped for unknown reason in frame %d.  ", f );
            }
            else
            {
                p->pPars->iFrame++;
                Abc_Print( 1, "VTA completed %d frames with a %d-stable abstraction.  ", f, p->pPars->nFramesNoChange );
            }
        }
    }
    else
    {
        if ( p->pPars->fVerbose )
            printf( "\n" );
        ABC_FREE( p->pGia->pCexSeq );
        p->pGia->pCexSeq = pCex;
        if ( !Gia_ManVerifyCex( p->pGia, pCex, 0 ) )
            Abc_Print( 1, "    Gia_VtaPerform(): CEX verification has failed!\n" );
        Abc_Print( 1, "Counter-example detected in frame %d.  ", f );
        p->pPars->iFrame = pCex->iFrame - 1;
        Vec_IntFreeP( &pAig->vObjClasses );
        RetValue = 0;
    }
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    if ( p->pPars->fVerbose )
    {
        p->timeOther = (Abc_Clock() - clk) - p->timeUnsat - p->timeSat - p->timeCex;
        ABC_PRTP( "Runtime: Solver UNSAT", p->timeUnsat,  Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Solver SAT  ", p->timeSat,    Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Refinement  ", p->timeCex,    Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Other       ", p->timeOther,  Abc_Clock() - clk );
        ABC_PRTP( "Runtime: TOTAL       ", Abc_Clock() - clk, Abc_Clock() - clk );
        Gia_VtaPrintMemory( p );
    }

    Vga_ManStop( p );
    fflush( stdout );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Collect nodes/flops involved in different timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_VtaPerform( Gia_Man_t * pAig, Abs_Par_t * pPars )
{
    int RetValue = -1;
    if ( pAig->vObjClasses == NULL && pPars->fUseRollback )
    {
        int nFramesMaxOld = pPars->nFramesMax;
        pPars->nFramesMax = pPars->nFramesStart;
        RetValue = Gia_VtaPerformInt( pAig, pPars );
        pPars->nFramesMax = nFramesMaxOld;
    }
    if ( RetValue == 0 )
        return RetValue;
    return Gia_VtaPerformInt( pAig, pPars );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

