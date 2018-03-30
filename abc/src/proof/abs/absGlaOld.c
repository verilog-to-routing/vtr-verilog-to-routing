/**CFile****************************************************************

  FileName    [absGla.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [Gate-level abstraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absGla.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver2.h"
#include "base/main/main.h"
#include "abs.h"
#include "absRef.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Rfn_Obj_t_ Rfn_Obj_t; // refinement object
struct Rfn_Obj_t_
{
    unsigned       Value     :  1;  // value
    unsigned       fVisit    :  1;  // visited
    unsigned       fPPi      :  1;  // PPI
    unsigned       Prio      : 16;  // priority
    unsigned       Sign      : 12;  // traversal signature
};

typedef struct Gla_Obj_t_ Gla_Obj_t; // abstraction object
struct Gla_Obj_t_
{
    int            iGiaObj;         // corresponding GIA obj
    unsigned       fAbs      :  1;  // belongs to abstraction
    unsigned       fCompl0   :  1;  // compl bit of the first fanin
    unsigned       fConst    :  1;  // object attribute
    unsigned       fPi       :  1;  // object attribute
    unsigned       fPo       :  1;  // object attribute
    unsigned       fRo       :  1;  // object attribute
    unsigned       fRi       :  1;  // object attribute
    unsigned       fAnd      :  1;  // object attribute
    unsigned       fMark     :  1;  // nearby object
    unsigned       nFanins   : 23;  // fanin count
    int            Fanins[4];       // fanins
    Vec_Int_t      vFrames;         // variables in each timeframe
};

typedef struct Gla_Man_t_ Gla_Man_t; // manager
struct Gla_Man_t_
{
    // user data
    Gia_Man_t *    pGia0;           // starting AIG manager
    Gia_Man_t *    pGia;            // working AIG manager
    Abs_Par_t *    pPars;           // parameters
    // internal data
    Vec_Int_t *    vAbs;            // abstracted objects
    Gla_Obj_t *    pObjRoot;        // the primary output
    Gla_Obj_t *    pObjs;           // objects
    unsigned *     pObj2Obj;        // mapping of GIA obj into GLA obj
    int            nObjs;           // the number of objects
    int            nAbsOld;         // previous abstraction
//    int            nAbsNew;         // previous abstraction
//    int            nLrnOld;         // the number of bytes
//    int            nLrnNew;         // the number of bytes
    // other data
    int            nCexes;          // the number of counter-examples
    int            nObjAdded;       // total number of objects added
    int            nSatVars;        // the number of SAT variables
    Cnf_Dat_t *    pCnf;            // CNF derived for the nodes
    sat_solver2 *  pSat;            // incremental SAT solver
    Vec_Int_t *    vTemp;           // temporary array
    Vec_Int_t *    vAddedNew;       // temporary array
    Vec_Int_t *    vObjCounts;      // object counters
    Vec_Int_t *    vCoreCounts;     // counts how many times each object appears in the core
    Vec_Int_t *    vProofIds;       // counts how many times each object appears in the core
    int            nProofIds;       // proof ID counter
    // refinement
    Vec_Int_t *    pvRefis;         // vectors of each object
    // refinement manager
    Gia_Man_t *    pGia2;
    Rnm_Man_t *    pRnm;
    // statistics  
    abctime        timeInit;
    abctime        timeSat;
    abctime        timeUnsat;
    abctime        timeCex;
    abctime        timeOther;
};

// declarations
static Vec_Int_t * Gla_ManCollectPPis( Gla_Man_t * p, Vec_Int_t * vPis );
static int         Gla_ManCheckVar( Gla_Man_t * p, int iObj, int iFrame );
static int         Gla_ManGetVar( Gla_Man_t * p, int iObj, int iFrame );

// object procedures
static inline int         Gla_ObjId( Gla_Man_t * p, Gla_Obj_t * pObj )      { assert( pObj > p->pObjs && pObj < p->pObjs + p->nObjs ); return pObj - p->pObjs;    }
static inline Gla_Obj_t * Gla_ManObj( Gla_Man_t * p, int i )                { assert( i >= 0 && i < p->nObjs ); return i ? p->pObjs + i : NULL;                   }
static inline Gia_Obj_t * Gla_ManGiaObj( Gla_Man_t * p, Gla_Obj_t * pObj )  { return Gia_ManObj( p->pGia, pObj->iGiaObj );                                        }
static inline int         Gla_ObjSatValue( Gla_Man_t * p, int iGia, int f ) { return Gla_ManCheckVar(p, p->pObj2Obj[iGia], f) ? sat_solver2_var_value( p->pSat, Gla_ManGetVar(p, p->pObj2Obj[iGia], f) ) : 0;  }

static inline Rfn_Obj_t * Gla_ObjRef( Gla_Man_t * p, Gia_Obj_t * pObj, int f ) { return (Rfn_Obj_t *)Vec_IntGetEntryP( &(p->pvRefis[Gia_ObjId(p->pGia, pObj)]), f ); }
static inline void        Gla_ObjClearRef( Rfn_Obj_t * p )                     { *((int *)p) = 0;                                                                    }


// iterator through abstracted objects
#define Gla_ManForEachObj( p, pObj )                       \
    for ( pObj = p->pObjs + 1; pObj < p->pObjs + p->nObjs; pObj++ ) 
#define Gla_ManForEachObjAbs( p, pObj, i )                 \
    for ( i = 0; i < Vec_IntSize(p->vAbs) && ((pObj = Gla_ManObj(p, Vec_IntEntry(p->vAbs, i))),1); i++) 
#define Gla_ManForEachObjAbsVec( vVec, p, pObj, i )        \
    for ( i = 0; i < Vec_IntSize(vVec) && ((pObj = Gla_ManObj(p, Vec_IntEntry(vVec, i))),1); i++) 

// iterator through fanins of an abstracted object
#define Gla_ObjForEachFanin( p, pObj, pFanin, i )          \
    for ( i = 0; (i < (int)pObj->nFanins) && ((pFanin = Gla_ManObj(p, pObj->Fanins[i])),1); i++ )

// some lessons learned from debugging mismatches between GIA and mapped CNF
// - inputs/output of AND-node may be PPIs (have SAT vars), but the node is not included in the abstraction
// - some logic node can be a PPI of one LUT and an internal node of another LUT

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prepares CEX and vMap for refinement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_GlaPrepareCexAndMap( Gla_Man_t * p, Abc_Cex_t ** ppCex, Vec_Int_t ** pvMap )
{
    Abc_Cex_t * pCex;
    Vec_Int_t * vMap;
    Gla_Obj_t * pObj, * pFanin;
    Gia_Obj_t * pGiaObj;
    int f, i, k;
    // find PIs and PPIs
    vMap = Vec_IntAlloc( 1000 );
    Gla_ManForEachObjAbs( p, pObj, i )
    {
        assert( pObj->fConst || pObj->fRo || pObj->fAnd );
        Gla_ObjForEachFanin( p, pObj, pFanin, k )
            if ( !pFanin->fAbs )
                Vec_IntPush( vMap, pFanin->iGiaObj );
    }
    Vec_IntUniqify( vMap );
    // derive counter-example
    pCex = Abc_CexAlloc( 0, Vec_IntSize(vMap), p->pPars->iFrame+1 );
    pCex->iFrame = p->pPars->iFrame;
    for ( f = 0; f <= p->pPars->iFrame; f++ )
        Gia_ManForEachObjVec( vMap, p->pGia, pGiaObj, k )
            if ( Gla_ObjSatValue( p, Gia_ObjId(p->pGia, pGiaObj), f ) )
                Abc_InfoSetBit( pCex->pData, f * Vec_IntSize(vMap) + k );
    *pvMap = vMap;
    *ppCex = pCex;
}

/**Function*************************************************************

  Synopsis    [Derives counter-example using current assignments.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Gla_ManDeriveCex( Gla_Man_t * p, Vec_Int_t * vPis )
{
    Abc_Cex_t * pCex;
    Gia_Obj_t * pObj;
    int i, f;
    pCex = Abc_CexAlloc( Gia_ManRegNum(p->pGia), Gia_ManPiNum(p->pGia), p->pPars->iFrame+1 );
    pCex->iPo = 0;
    pCex->iFrame = p->pPars->iFrame;
    Gia_ManForEachObjVec( vPis, p->pGia, pObj, i )
    {
        if ( !Gia_ObjIsPi(p->pGia, pObj) )
            continue;
        assert( Gia_ObjIsPi(p->pGia, pObj) );
        for ( f = 0; f <= pCex->iFrame; f++ )
            if ( Gla_ObjSatValue( p, Gia_ObjId(p->pGia, pObj), f ) )
                Abc_InfoSetBit( pCex->pData, pCex->nRegs + f * pCex->nPis + Gia_ObjCioId(pObj) );
    }
    return pCex;
}

/**Function*************************************************************

  Synopsis    [Collects GIA abstraction objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gla_ManCollectInternal_rec( Gia_Man_t * p, Gia_Obj_t * pGiaObj, Vec_Int_t * vRoAnds )
{
    if ( Gia_ObjIsTravIdCurrent(p, pGiaObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pGiaObj);
    assert( Gia_ObjIsAnd(pGiaObj) );
    Gla_ManCollectInternal_rec( p, Gia_ObjFanin0(pGiaObj), vRoAnds );
    Gla_ManCollectInternal_rec( p, Gia_ObjFanin1(pGiaObj), vRoAnds );
    Vec_IntPush( vRoAnds, Gia_ObjId(p, pGiaObj) );
}
void Gla_ManCollect( Gla_Man_t * p, Vec_Int_t * vPis, Vec_Int_t * vPPis, Vec_Int_t * vCos, Vec_Int_t * vRoAnds )
{
    Gla_Obj_t * pObj, * pFanin; 
    Gia_Obj_t * pGiaObj;
    int i, k;

    // collect COs
    Vec_IntPush( vCos, Gia_ObjId(p->pGia, Gia_ManPo(p->pGia, 0)) );
    // collect fanins of abstracted objects
    Gla_ManForEachObjAbs( p, pObj, i )
    {
        assert( pObj->fConst || pObj->fRo || pObj->fAnd );
        if ( pObj->fRo )
        {
            pGiaObj = Gia_ObjRoToRi( p->pGia, Gia_ManObj(p->pGia, pObj->iGiaObj) );
            Vec_IntPush( vCos, Gia_ObjId(p->pGia, pGiaObj) );
        }
        Gla_ObjForEachFanin( p, pObj, pFanin, k )
            if ( !pFanin->fAbs )
                Vec_IntPush( (pFanin->fPi ? vPis : vPPis), pFanin->iGiaObj );
    }
    Vec_IntUniqify( vPis );
    Vec_IntUniqify( vPPis );
    Vec_IntSort( vCos, 0 );
    // sorting PIs/PPIs/COs leads to refinements that are more "well-aligned"...

    // mark const/PIs/PPIs
    Gia_ManIncrementTravId( p->pGia );
    Gia_ObjSetTravIdCurrent( p->pGia, Gia_ManConst0(p->pGia) );
    Gia_ManForEachObjVec( vPis, p->pGia, pGiaObj, i )
        Gia_ObjSetTravIdCurrent( p->pGia, pGiaObj );
    Gia_ManForEachObjVec( vPPis, p->pGia, pGiaObj, i )
        Gia_ObjSetTravIdCurrent( p->pGia, pGiaObj );
    // mark and add ROs first
    Gia_ManForEachObjVec( vCos, p->pGia, pGiaObj, i )
    {
        if ( i == 0 ) continue;
        pGiaObj = Gia_ObjRiToRo( p->pGia, pGiaObj );
        Gia_ObjSetTravIdCurrent( p->pGia, pGiaObj );
        Vec_IntPush( vRoAnds, Gia_ObjId(p->pGia, pGiaObj) );
    }
    // collect nodes between PIs/PPIs/ROs and COs
    Gia_ManForEachObjVec( vCos, p->pGia, pGiaObj, i )
        Gla_ManCollectInternal_rec( p->pGia, Gia_ObjFanin0(pGiaObj), vRoAnds );
}

/**Function*************************************************************

  Synopsis    [Drive implications of the given node towards primary outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManRefSetAndPropFanout_rec( Gla_Man_t * p, Gia_Obj_t * pObj, int f, Vec_Int_t * vSelect, int Sign )
{
    int i;//, Id = Gia_ObjId(p->pGia, pObj);
    Rfn_Obj_t * pRef0, * pRef1, * pRef = Gla_ObjRef( p, pObj, f );
    Gia_Obj_t * pFanout;
    int k;
    if ( (int)pRef->Sign != Sign )
        return;
    assert( pRef->fVisit == 0 );
    pRef->fVisit = 1;
    if ( pRef->fPPi )
    {
        assert( (int)pRef->Prio > 0 );
        for ( i = p->pPars->iFrame; i >= 0; i-- )
            if ( !Gla_ObjRef(p, pObj, i)->fVisit )
                Gia_ManRefSetAndPropFanout_rec( p, pObj, i, vSelect, Sign );
        Vec_IntPush( vSelect, Gia_ObjId(p->pGia, pObj) );
        return;
    }
    if ( (Gia_ObjIsCo(pObj) && f == p->pPars->iFrame) || Gia_ObjIsPo(p->pGia, pObj) )
        return;
    if ( Gia_ObjIsRi(p->pGia, pObj) )
    {
        pFanout = Gia_ObjRiToRo(p->pGia, pObj);
        if ( !Gla_ObjRef(p, pFanout, f+1)->fVisit )
            Gia_ManRefSetAndPropFanout_rec( p, pFanout, f+1, vSelect, Sign );
        return;
    }
    assert( Gia_ObjIsRo(p->pGia, pObj) || Gia_ObjIsAnd(pObj) );
    Gia_ObjForEachFanoutStatic( p->pGia, pObj, pFanout, k )
    {
//        Rfn_Obj_t * pRefF = Gla_ObjRef(p, pFanout, f);
        if ( Gla_ObjRef(p, pFanout, f)->fVisit )
            continue;
        if ( Gia_ObjIsCo(pFanout) )
        {
            Gia_ManRefSetAndPropFanout_rec( p, pFanout, f, vSelect, Sign );
            continue;
        } 
        assert( Gia_ObjIsAnd(pFanout) );
        pRef0 = Gla_ObjRef( p, Gia_ObjFanin0(pFanout), f );
        pRef1 = Gla_ObjRef( p, Gia_ObjFanin1(pFanout), f );
        if ( ((pRef0->Value ^ Gia_ObjFaninC0(pFanout)) == 0 && pRef0->fVisit) ||
             ((pRef1->Value ^ Gia_ObjFaninC1(pFanout)) == 0 && pRef1->fVisit) || 
           ( ((pRef0->Value ^ Gia_ObjFaninC0(pFanout)) == 1 && pRef0->fVisit) && 
             ((pRef1->Value ^ Gia_ObjFaninC1(pFanout)) == 1 && pRef1->fVisit) ) )
           Gia_ManRefSetAndPropFanout_rec( p, pFanout, f, vSelect, Sign );
    }
}
 
/**Function*************************************************************

  Synopsis    [Selects assignments to be refined.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gla_ManRefSelect_rec( Gla_Man_t * p, Gia_Obj_t * pObj, int f, Vec_Int_t * vSelect, int Sign )
{ 
    int i;//, Id = Gia_ObjId(p->pGia, pObj);
    Rfn_Obj_t * pRef = Gla_ObjRef( p, pObj, f );
//    assert( (int)pRef->Sign == Sign );
    if ( pRef->fVisit )
        return;
    if ( p->pPars->fPropFanout )
        Gia_ManRefSetAndPropFanout_rec( p, pObj, f, vSelect, Sign );
    else
        pRef->fVisit = 1;
    if ( pRef->fPPi )
    {
        assert( (int)pRef->Prio > 0 );
        if ( p->pPars->fPropFanout )
        {
            for ( i = p->pPars->iFrame; i >= 0; i-- )
                if ( !Gla_ObjRef(p, pObj, i)->fVisit )
                    Gia_ManRefSetAndPropFanout_rec( p, pObj, i, vSelect, Sign );
        }
        else
        {
            Vec_IntPush( vSelect, Gia_ObjId(p->pGia, pObj) );
            Vec_IntAddToEntry( p->vObjCounts, f, 1 );
        }
        return;
    }
    if ( Gia_ObjIsPi(p->pGia, pObj) || Gia_ObjIsConst0(pObj) )
        return;
    if ( Gia_ObjIsRo(p->pGia, pObj) )
    {
        if ( f > 0 )
            Gla_ManRefSelect_rec( p, Gia_ObjFanin0(Gia_ObjRoToRi(p->pGia, pObj)), f-1, vSelect, Sign );
        return;
    }
    if ( Gia_ObjIsAnd(pObj) )
    {
        Rfn_Obj_t * pRef0 = Gla_ObjRef( p, Gia_ObjFanin0(pObj), f );
        Rfn_Obj_t * pRef1 = Gla_ObjRef( p, Gia_ObjFanin1(pObj), f );
        if ( pRef->Value == 1 )
        {
            if ( pRef0->Prio > 0 )
                Gla_ManRefSelect_rec( p, Gia_ObjFanin0(pObj), f, vSelect, Sign );
            if ( pRef1->Prio > 0 )
                Gla_ManRefSelect_rec( p, Gia_ObjFanin1(pObj), f, vSelect, Sign );
        }
        else // select one value
        {
            if ( (pRef0->Value ^ Gia_ObjFaninC0(pObj)) == 0 && (pRef1->Value ^ Gia_ObjFaninC1(pObj)) == 0 )
            {
                if ( pRef0->Prio <= pRef1->Prio ) // choice
                {
                    if ( pRef0->Prio > 0 )
                        Gla_ManRefSelect_rec( p, Gia_ObjFanin0(pObj), f, vSelect, Sign );
                }
                else
                {
                    if ( pRef1->Prio > 0 )
                        Gla_ManRefSelect_rec( p, Gia_ObjFanin1(pObj), f, vSelect, Sign );
                }
            }
            else if ( (pRef0->Value ^ Gia_ObjFaninC0(pObj)) == 0 )
            {
                if ( pRef0->Prio > 0 )
                    Gla_ManRefSelect_rec( p, Gia_ObjFanin0(pObj), f, vSelect, Sign );
            }
            else if ( (pRef1->Value ^ Gia_ObjFaninC1(pObj)) == 0 )
            {
                if ( pRef1->Prio > 0 )
                    Gla_ManRefSelect_rec( p, Gia_ObjFanin1(pObj), f, vSelect, Sign );
            }
            else assert( 0 );
        }
    }
    else assert( 0 );
}


/**Function*************************************************************

  Synopsis    [Performs refinement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gla_ManVerifyUsingTerSim( Gla_Man_t * p, Vec_Int_t * vPis, Vec_Int_t * vPPis, Vec_Int_t * vRoAnds, Vec_Int_t * vCos, Vec_Int_t * vRes )
{
    Gia_Obj_t * pObj;
    int i, f;
//    Gia_ManForEachObj( p->pGia, pObj, i )
//        assert( Gia_ObjTerSimGetC(pObj) );
    for ( f = 0; f <= p->pPars->iFrame; f++ )
    {
        Gia_ObjTerSimSet0( Gia_ManConst0(p->pGia) );
        Gia_ManForEachObjVec( vPis, p->pGia, pObj, i )
        {
            if ( Gla_ObjSatValue( p, Gia_ObjId(p->pGia, pObj), f ) )
                Gia_ObjTerSimSet1( pObj );
            else
                Gia_ObjTerSimSet0( pObj );
        }
        Gia_ManForEachObjVec( vPPis, p->pGia, pObj, i )
            Gia_ObjTerSimSetX( pObj );
        Gia_ManForEachObjVec( vRes, p->pGia, pObj, i )
            if ( Gla_ObjSatValue( p, Gia_ObjId(p->pGia, pObj), f ) )
                Gia_ObjTerSimSet1( pObj );
            else
                Gia_ObjTerSimSet0( pObj );

        Gia_ManForEachObjVec( vRoAnds, p->pGia, pObj, i )
        {
            if ( Gia_ObjIsAnd(pObj) )
                Gia_ObjTerSimAnd( pObj );
            else if ( f == 0 )
                Gia_ObjTerSimSet0( pObj );
            else
                Gia_ObjTerSimRo( p->pGia, pObj );
        }
        Gia_ManForEachObjVec( vCos, p->pGia, pObj, i )
            Gia_ObjTerSimCo( pObj );
    }
    pObj = Gia_ManPo( p->pGia, 0 );
    if ( !Gia_ObjTerSimGet1(pObj) )
        Abc_Print( 1, "\nRefinement verification has failed!!!\n" );
    // clear
    Gia_ObjTerSimSetC( Gia_ManConst0(p->pGia) );
    Gia_ManForEachObjVec( vPis, p->pGia, pObj, i )
        Gia_ObjTerSimSetC( pObj );
    Gia_ManForEachObjVec( vPPis, p->pGia, pObj, i )
        Gia_ObjTerSimSetC( pObj );
    Gia_ManForEachObjVec( vRoAnds, p->pGia, pObj, i )
        Gia_ObjTerSimSetC( pObj );
    Gia_ManForEachObjVec( vCos, p->pGia, pObj, i )
        Gia_ObjTerSimSetC( pObj );
}


/**Function*************************************************************

  Synopsis    [Performs refinement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gla_ManRefinement( Gla_Man_t * p )
{
    Abc_Cex_t * pCex;
    Vec_Int_t * vMap, * vVec;
    Gia_Obj_t * pObj;
    int i;
    Gia_GlaPrepareCexAndMap( p, &pCex, &vMap );
    vVec = Rnm_ManRefine( p->pRnm, pCex, vMap, p->pPars->fPropFanout, p->pPars->fNewRefine, 1 );
    Abc_CexFree( pCex );
    if ( Vec_IntSize(vVec) == 0 )
    {
        Vec_IntFree( vVec );
        Abc_CexFreeP( &p->pGia->pCexSeq );
        p->pGia->pCexSeq = Gla_ManDeriveCex( p, vMap );
        Vec_IntFree( vMap );
        return NULL;
    }
    Vec_IntFree( vMap );
    // remap them into GLA objects
    Gia_ManForEachObjVec( vVec, p->pGia, pObj, i )
        Vec_IntWriteEntry( vVec, i, p->pObj2Obj[Gia_ObjId(p->pGia, pObj)] );
    p->nObjAdded += Vec_IntSize(vVec);
    return vVec;
}

/**Function*************************************************************

  Synopsis    [Performs refinement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gla_ManRefinement2( Gla_Man_t * p )
{
    int fVerify = 1;
    static int Sign = 0;
    Vec_Int_t * vPis, * vPPis, * vCos, * vRoAnds, * vSelect = NULL;
    Rfn_Obj_t * pRef, * pRef0, * pRef1;
    Gia_Obj_t * pObj;
    int i, f;
    Sign++;

    // compute PIs and pseudo-PIs
    vCos = Vec_IntAlloc( 1000 );
    vPis = Vec_IntAlloc( 1000 );  
    vPPis = Vec_IntAlloc( 1000 );
    vRoAnds = Vec_IntAlloc( 1000 );  
    Gla_ManCollect( p, vPis, vPPis, vCos, vRoAnds );

/*
    // check how many pseudo PIs have variables
    Gla_ManForEachObjAbsVec( vPis, p, pGla, i )
    {
        Abc_Print( 1, "  %5d : ", Gla_ObjId(p, pGla) );
        for ( f = 0; f <= p->pPars->iFrame; f++ )
            Abc_Print( 1, "%d", Gla_ManCheckVar(p, Gla_ObjId(p, pGla), f) );
        Abc_Print( 1, "\n" );
    }    

    // check how many pseudo PIs have variables
    Gla_ManForEachObjAbsVec( vPPis, p, pGla, i )
    {
        Abc_Print( 1, "%5d : ", Gla_ObjId(p, pGla) );
        for ( f = 0; f <= p->pPars->iFrame; f++ )
            Abc_Print( 1, "%d", Gla_ManCheckVar(p, Gla_ObjId(p, pGla), f) );
        Abc_Print( 1, "\n" );
    }    
*/
    // propagate values
    for ( f = 0; f <= p->pPars->iFrame; f++ )
    {
        // constant
        pRef = Gla_ObjRef( p, Gia_ManConst0(p->pGia), f );  Gla_ObjClearRef( pRef );
        pRef->Value  = 0;
        pRef->Prio   = 0;
        pRef->Sign   = Sign;
        // primary input
        Gia_ManForEachObjVec( vPis, p->pGia, pObj, i )
        {
//            assert( f == p->pPars->iFrame || Gla_ManCheckVar(p, p->pObj2Obj[Gia_ObjId(p->pGia, pObj)], f) );
            pRef = Gla_ObjRef( p, pObj, f );   Gla_ObjClearRef( pRef );
            pRef->Value = Gla_ObjSatValue( p, Gia_ObjId(p->pGia, pObj), f );
            pRef->Prio  = 0;
            pRef->Sign  = Sign;
            assert( pRef->fVisit == 0 );
        }
        // primary input
        Gia_ManForEachObjVec( vPPis, p->pGia, pObj, i )
        {
//            assert( f == p->pPars->iFrame || Gla_ManCheckVar(p, p->pObj2Obj[Gia_ObjId(p->pGia, pObj)], f) );
            assert( Gia_ObjIsAnd(pObj) || Gia_ObjIsRo(p->pGia, pObj) );
            pRef = Gla_ObjRef( p, pObj, f );   Gla_ObjClearRef( pRef );
            pRef->Value = Gla_ObjSatValue( p, Gia_ObjId(p->pGia, pObj), f );
            pRef->Prio  = i+1;
            pRef->fPPi  = 1;
            pRef->Sign  = Sign;
            assert( pRef->fVisit == 0 );
        }
        // internal nodes
        Gia_ManForEachObjVec( vRoAnds, p->pGia, pObj, i )
        {
            assert( Gia_ObjIsAnd(pObj) || Gia_ObjIsRo(p->pGia, pObj) );
            pRef = Gla_ObjRef( p, pObj, f );   Gla_ObjClearRef( pRef );
            if ( Gia_ObjIsRo(p->pGia, pObj) )
            {
                if ( f == 0 )
                {
                    pRef->Value = 0;
                    pRef->Prio  = 0;
                    pRef->Sign  = Sign;
                }
                else
                {
                    pRef0 = Gla_ObjRef( p, Gia_ObjRoToRi(p->pGia, pObj), f-1 );
                    pRef->Value = pRef0->Value;
                    pRef->Prio  = pRef0->Prio;
                    pRef->Sign  = Sign;
                }
                continue;
            }
            assert( Gia_ObjIsAnd(pObj) );
            pRef0 = Gla_ObjRef( p, Gia_ObjFanin0(pObj), f );
            pRef1 = Gla_ObjRef( p, Gia_ObjFanin1(pObj), f );
            pRef->Value = (pRef0->Value ^ Gia_ObjFaninC0(pObj)) & (pRef1->Value ^ Gia_ObjFaninC1(pObj));

            if ( p->pObj2Obj[Gia_ObjId(p->pGia, pObj)] != ~0 && 
                 Gla_ManCheckVar(p, p->pObj2Obj[Gia_ObjId(p->pGia, pObj)], f) &&
                 (int)pRef->Value != Gla_ObjSatValue(p, Gia_ObjId(p->pGia, pObj), f) )
            {
                    Abc_Print( 1, "Object has value mismatch    " );
                    Gia_ObjPrint( p->pGia, pObj );
            }

            if ( pRef->Value == 1 )
                pRef->Prio  = Abc_MaxInt( pRef0->Prio, pRef1->Prio );
            else if ( (pRef0->Value ^ Gia_ObjFaninC0(pObj)) == 0 && (pRef1->Value ^ Gia_ObjFaninC1(pObj)) == 0 )
                pRef->Prio  = Abc_MinInt( pRef0->Prio, pRef1->Prio ); // choice
            else if ( (pRef0->Value ^ Gia_ObjFaninC0(pObj)) == 0 )
                pRef->Prio  = pRef0->Prio;
            else 
                pRef->Prio  = pRef1->Prio;
            assert( pRef->fVisit == 0 );
            pRef->Sign  = Sign;
        }
        // output nodes
        Gia_ManForEachObjVec( vCos, p->pGia, pObj, i )
        {
            pRef = Gla_ObjRef( p, pObj, f );    Gla_ObjClearRef( pRef );
            pRef0 = Gla_ObjRef( p, Gia_ObjFanin0(pObj), f ); 
            pRef->Value = (pRef0->Value ^ Gia_ObjFaninC0(pObj));
            pRef->Prio  = pRef0->Prio;
            assert( pRef->fVisit == 0 );
            pRef->Sign  = Sign;
        }
    } 

    // make sure the output value is 1
    pObj = Gia_ManPo( p->pGia, 0 );
    pRef = Gla_ObjRef( p, pObj, p->pPars->iFrame );
    if ( pRef->Value != 1 )
        Abc_Print( 1, "\nCounter-example verification has failed!!!\n" );

    // check the CEX
    if ( pRef->Prio == 0 )
    {
        p->pGia->pCexSeq = Gla_ManDeriveCex( p, vPis );
        Vec_IntFree( vPis );
        Vec_IntFree( vPPis );
        Vec_IntFree( vRoAnds );
        Vec_IntFree( vCos );
        return NULL;
    }

    // select objects
    vSelect = Vec_IntAlloc( 100 );
    Vec_IntFill( p->vObjCounts, p->pPars->iFrame+1, 0 );
    Gla_ManRefSelect_rec( p, Gia_ObjFanin0(Gia_ManPo(p->pGia, 0)), p->pPars->iFrame, vSelect, Sign );
    Vec_IntUniqify( vSelect );

/*
    for ( f = 0; f < p->pPars->iFrame; f++ )
        printf( "%2d", Vec_IntEntry(p->vObjCounts, f) );
    printf( "\n" );
*/
    if ( fVerify )
        Gla_ManVerifyUsingTerSim( p, vPis, vPPis, vRoAnds, vCos, vSelect );

    // remap them into GLA objects
    Gia_ManForEachObjVec( vSelect, p->pGia, pObj, i )
        Vec_IntWriteEntry( vSelect, i, p->pObj2Obj[Gia_ObjId(p->pGia, pObj)] );

    Vec_IntFree( vPis );
    Vec_IntFree( vPPis );
    Vec_IntFree( vRoAnds );
    Vec_IntFree( vCos );

    p->nObjAdded += Vec_IntSize(vSelect);
    return vSelect;
}


/**Function*************************************************************

  Synopsis    [Adds clauses for the given obj in the given frame.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gla_ManCollectFanins( Gla_Man_t * p, Gla_Obj_t * pGla, int iObj, Vec_Int_t * vFanins )
{
    int i, nClauses, iFirstClause, * pLit;
    nClauses = p->pCnf->pObj2Count[pGla->iGiaObj];
    iFirstClause = p->pCnf->pObj2Clause[pGla->iGiaObj];
    Vec_IntClear( vFanins );
    for ( i = iFirstClause; i < iFirstClause + nClauses; i++ )
        for ( pLit = p->pCnf->pClauses[i]; pLit < p->pCnf->pClauses[i+1]; pLit++ )
            if ( lit_var(*pLit) != iObj )
                Vec_IntPushUnique( vFanins, lit_var(*pLit) );
    assert( Vec_IntSize( vFanins ) <= 4 );
    Vec_IntSort( vFanins, 0 );
}


/**Function*************************************************************

  Synopsis    [Duplicates AIG while decoupling nodes duplicated in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupMapped_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Gia_Man_t * pNew )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupMapped_rec( p, Gia_ObjFanin0(pObj), pNew );
    Gia_ManDupMapped_rec( p, Gia_ObjFanin1(pObj), pNew );
    pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Vec_IntPush( pNew->vLutConfigs, Gia_ObjId(p, pObj) );
}
Gia_Man_t * Gia_ManDupMapped( Gia_Man_t * p, Vec_Int_t * vMapping )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj, * pFanin;
    int i, k, * pMapping, * pObj2Obj;
    // start new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // start mapping
    Gia_ManFillValue( p );
    pObj2Obj = ABC_FALLOC( int, Gia_ManObjNum(p) );
    pObj2Obj[0] = 0;
    // create reverse mapping and attach it to the node
    pNew->vLutConfigs = Vec_IntAlloc( Gia_ManObjNum(p) * 4 / 3 );
    Vec_IntPush( pNew->vLutConfigs, 0 );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
        { 
            int Offset = Vec_IntEntry(vMapping, Gia_ObjId(p, pObj));
            if ( Offset == 0 )
                continue;
            pMapping = Vec_IntEntryP( vMapping, Offset );
            Gia_ManIncrementTravId( p );
            for ( k = 1; k <= 4; k++ )
            {
                if ( pMapping[k] == -1 )
                    continue;
                pFanin = Gia_ManObj(p, pMapping[k]);
                Gia_ObjSetTravIdCurrent( p, pFanin );
                pFanin->Value = pObj2Obj[pMapping[k]];
                assert( ~pFanin->Value );
            }
            assert( !Gia_ObjIsTravIdCurrent(p, pObj) );
            assert( !~pObj->Value );
            Gia_ManDupMapped_rec( p, pObj, pNew );
            pObj2Obj[i] = pObj->Value;
            assert( ~pObj->Value );
        }
        else if ( Gia_ObjIsCi(pObj) )
        {
            pObj2Obj[i] = Gia_ManAppendCi( pNew );
            Vec_IntPush( pNew->vLutConfigs, i );
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            Gia_ObjFanin0(pObj)->Value = pObj2Obj[Gia_ObjFaninId0p(p, pObj)];
            assert( ~Gia_ObjFanin0(pObj)->Value );
            pObj2Obj[i] = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
            Vec_IntPush( pNew->vLutConfigs, i );
        }
    }
    assert( Vec_IntSize(pNew->vLutConfigs) == Gia_ManObjNum(pNew) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    // map original AIG into the new AIG
    Gia_ManForEachObj( p, pObj, i )
        pObj->Value = pObj2Obj[i];
    ABC_FREE( pObj2Obj );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Creates a new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gla_Man_t * Gla_ManStart( Gia_Man_t * pGia0, Abs_Par_t * pPars )
{
    Gla_Man_t * p;
    Aig_Man_t * pAig;
    Gia_Obj_t * pObj;
    Gla_Obj_t * pGla;
    Vec_Int_t * vMappingNew;
    int i, k, Offset, * pMapping, * pLits, * pObj2Count, * pObj2Clause;

    // start
    p = ABC_CALLOC( Gla_Man_t, 1 );
    p->pGia0 = pGia0;
    p->pPars = pPars;
    p->vAbs  = Vec_IntAlloc( 100 );
    p->vTemp = Vec_IntAlloc( 100 );
    p->vAddedNew = Vec_IntAlloc( 100 );
    p->vObjCounts = Vec_IntAlloc( 100 );

    // internal data
    pAig = Gia_ManToAigSimple( pGia0 );
    p->pCnf = Cnf_DeriveOther( pAig, 1 );
    Aig_ManStop( pAig );
    // create working GIA
    p->pGia = Gia_ManDupMapped( pGia0, p->pCnf->vMapping );
    if ( pPars->fPropFanout )
        Gia_ManStaticFanoutStart( p->pGia );

    // derive new gate map
    assert( pGia0->vGateClasses != 0 );
    p->pGia->vGateClasses = Vec_IntStart( Gia_ManObjNum(p->pGia) );
    p->vCoreCounts = Vec_IntStart( Gia_ManObjNum(p->pGia) );
    p->vProofIds = Vec_IntAlloc(0);
    // update p->pCnf->vMapping, p->pCnf->pObj2Count, p->pCnf->pObj2Clause 
    // (here are not updating p->pCnf->pVarNums because it is not needed)
    vMappingNew = Vec_IntStart( Gia_ManObjNum(p->pGia) );
    pObj2Count  = ABC_FALLOC( int, Gia_ManObjNum(p->pGia) );
    pObj2Clause = ABC_FALLOC( int, Gia_ManObjNum(p->pGia) );
    Gia_ManForEachObj( pGia0, pObj, i )
    {
        // skip internal nodes not used in the mapping
        if ( !~pObj->Value )
            continue;
        // replace positive literal by variable
        assert( !Abc_LitIsCompl(pObj->Value) );
        pObj->Value = Abc_Lit2Var(pObj->Value);
        assert( (int)pObj->Value < Gia_ManObjNum(p->pGia) );
        // update arrays
        pObj2Count[pObj->Value] = p->pCnf->pObj2Count[i];
        pObj2Clause[pObj->Value] = p->pCnf->pObj2Clause[i];
        if ( Vec_IntEntry(pGia0->vGateClasses, i) )
            Vec_IntWriteEntry( p->pGia->vGateClasses, pObj->Value, 1 );
        // update mappings
        Offset = Vec_IntEntry(p->pCnf->vMapping, i);
        Vec_IntWriteEntry( vMappingNew, pObj->Value, Vec_IntSize(vMappingNew) );
        pMapping = Vec_IntEntryP(p->pCnf->vMapping, Offset);
        Vec_IntPush( vMappingNew, pMapping[0] );
        for ( k = 1; k <= 4; k++ )
        {
            if ( pMapping[k] == -1 )
                Vec_IntPush( vMappingNew, -1 );
            else
            {
                assert( ~Gia_ManObj(pGia0, pMapping[k])->Value );
                Vec_IntPush( vMappingNew, Gia_ManObj(pGia0, pMapping[k])->Value );
            }
        }
    }
    // update mapping after the offset (currently not being done because it is not used)
    Vec_IntFree( p->pCnf->vMapping );  p->pCnf->vMapping    = vMappingNew;
    ABC_FREE( p->pCnf->pObj2Count );   p->pCnf->pObj2Count  = pObj2Count;
    ABC_FREE( p->pCnf->pObj2Clause );  p->pCnf->pObj2Clause = pObj2Clause;


    // count the number of variables
    p->nObjs = 1;
    Gia_ManForEachObj( p->pGia, pObj, i )
        if ( p->pCnf->pObj2Count[i] >= 0 )
            pObj->Value = p->nObjs++;
        else
            pObj->Value = ~0;

    // re-express CNF using new variable IDs
    pLits = p->pCnf->pClauses[0];
    for ( i = 0; i < p->pCnf->nLiterals; i++ )
    {
        // find the original AIG object
        pObj = Gia_ManObj( pGia0, lit_var(pLits[i]) );
        assert( ~pObj->Value );
        // find the working AIG object
        pObj = Gia_ManObj( p->pGia, pObj->Value );
        assert( ~pObj->Value );
        // express literal in terms of LUT variables
        pLits[i] = toLitCond( pObj->Value, lit_sign(pLits[i]) );
    }

    // create objects 
    p->pObjs    = ABC_CALLOC( Gla_Obj_t, p->nObjs );
    p->pObj2Obj = ABC_FALLOC( unsigned, Gia_ManObjNum(p->pGia) );
//    p->pvRefis  = ABC_CALLOC( Vec_Int_t, Gia_ManObjNum(p->pGia) );
    Gia_ManForEachObj( p->pGia, pObj, i )
    {
        p->pObj2Obj[i] = pObj->Value;
        if ( !~pObj->Value )
            continue;
        pGla = Gla_ManObj( p, pObj->Value );
        pGla->iGiaObj = i;
        pGla->fCompl0 = Gia_ObjFaninC0(pObj);
        pGla->fConst  = Gia_ObjIsConst0(pObj);
        pGla->fPi     = Gia_ObjIsPi(p->pGia, pObj);
        pGla->fPo     = Gia_ObjIsPo(p->pGia, pObj);
        pGla->fRi     = Gia_ObjIsRi(p->pGia, pObj);
        pGla->fRo     = Gia_ObjIsRo(p->pGia, pObj);
        pGla->fAnd    = Gia_ObjIsAnd(pObj);
        if ( Gia_ObjIsConst0(pObj) || Gia_ObjIsPi(p->pGia, pObj) )
            continue;
        if ( Gia_ObjIsCo(pObj) )
        {
            pGla->nFanins = 1;
            pGla->Fanins[0] = Gia_ObjFanin0(pObj)->Value;
            continue;
        }
        if ( Gia_ObjIsAnd(pObj) )
        {
//            Gla_ManCollectFanins( p, pGla, pObj->Value, p->vTemp );
//            pGla->nFanins = Vec_IntSize( p->vTemp );
//            memcpy( pGla->Fanins, Vec_IntArray(p->vTemp), sizeof(int) * Vec_IntSize(p->vTemp) );
            Offset = Vec_IntEntry( p->pCnf->vMapping, i );
            pMapping = Vec_IntEntryP( p->pCnf->vMapping, Offset );
            pGla->nFanins = 0;
            for ( k = 1; k <= 4; k++ )
                if ( pMapping[k] != -1 )
                    pGla->Fanins[ pGla->nFanins++ ] = Gia_ManObj(p->pGia, pMapping[k])->Value;
            continue;
        }
        assert( Gia_ObjIsRo(p->pGia, pObj) );
        pGla->nFanins   = 1;
        pGla->Fanins[0] = Gia_ObjFanin0( Gia_ObjRoToRi(p->pGia, pObj) )->Value;
        pGla->fCompl0   = Gia_ObjFaninC0( Gia_ObjRoToRi(p->pGia, pObj) );
    }
    p->pObjRoot = Gla_ManObj( p, Gia_ManPo(p->pGia, 0)->Value );
    // abstraction 
    assert( p->pGia->vGateClasses != NULL );
    Gla_ManForEachObj( p, pGla )
    {
        if ( Vec_IntEntry( p->pGia->vGateClasses, pGla->iGiaObj ) == 0 )
            continue;
        pGla->fAbs = 1;
        Vec_IntPush( p->vAbs, Gla_ObjId(p, pGla) );
    }
    // other 
    p->pSat        = sat_solver2_new();
    if ( pPars->fUseFullProof )
        p->pSat->pPrf1 = Vec_SetAlloc( 20 );
//    p->pSat->fVerbose = p->pPars->fVerbose;
//    sat_solver2_set_learntmax( p->pSat, pPars->nLearnedMax );
    p->pSat->nLearntStart = p->pPars->nLearnedStart;
    p->pSat->nLearntDelta = p->pPars->nLearnedDelta;
    p->pSat->nLearntRatio = p->pPars->nLearnedPerce;
    p->pSat->nLearntMax   = p->pSat->nLearntStart;
    p->nSatVars  = 1;
    // start the refinement manager
//    p->pGia2 = Gia_ManDup( p->pGia );
    p->pRnm = Rnm_ManStart( p->pGia );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates a new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gla_Man_t * Gla_ManStart2( Gia_Man_t * pGia, Abs_Par_t * pPars )
{
    Gla_Man_t * p;
    Aig_Man_t * pAig;
    Gia_Obj_t * pObj;
    Gla_Obj_t * pGla;
    int i, * pLits;
    // start
    p = ABC_CALLOC( Gla_Man_t, 1 );
    p->pGia  = pGia;
    p->pPars = pPars;
    p->vAbs = Vec_IntAlloc( 100 );
    p->vTemp = Vec_IntAlloc( 100 );
    p->vAddedNew = Vec_IntAlloc( 100 );
    // internal data
    pAig = Gia_ManToAigSimple( p->pGia );
    p->pCnf  = Cnf_DeriveOther( pAig, 1 );
    Aig_ManStop( pAig );
    // count the number of variables
    p->nObjs = 1;
    Gia_ManForEachObj( p->pGia, pObj, i )
        if ( p->pCnf->pObj2Count[i] >= 0 )
            pObj->Value = p->nObjs++;
        else
            pObj->Value = ~0;
    // re-express CNF using new variable IDs
    pLits = p->pCnf->pClauses[0];
    for ( i = 0; i < p->pCnf->nLiterals; i++ )
    {
        pObj = Gia_ManObj( p->pGia, lit_var(pLits[i]) );
        assert( ~pObj->Value );
        pLits[i] = toLitCond( pObj->Value, lit_sign(pLits[i]) );
    }
    // create objects 
    p->pObjs    = ABC_CALLOC( Gla_Obj_t, p->nObjs );
    p->pObj2Obj = ABC_FALLOC( unsigned, Gia_ManObjNum(p->pGia) );
//    p->pvRefis  = ABC_CALLOC( Vec_Int_t, Gia_ManObjNum(p->pGia) );
    Gia_ManForEachObj( p->pGia, pObj, i )
    {
        p->pObj2Obj[i] = pObj->Value;
        if ( !~pObj->Value )
            continue;
        pGla = Gla_ManObj( p, pObj->Value );
        pGla->iGiaObj = i;
        pGla->fCompl0 = Gia_ObjFaninC0(pObj);
        pGla->fConst  = Gia_ObjIsConst0(pObj);
        pGla->fPi     = Gia_ObjIsPi(p->pGia, pObj);
        pGla->fPo     = Gia_ObjIsPo(p->pGia, pObj);
        pGla->fRi     = Gia_ObjIsRi(p->pGia, pObj);
        pGla->fRo     = Gia_ObjIsRo(p->pGia, pObj);
        pGla->fAnd    = Gia_ObjIsAnd(pObj);
        if ( Gia_ObjIsConst0(pObj) || Gia_ObjIsPi(p->pGia, pObj) )
            continue;
        if ( Gia_ObjIsAnd(pObj) || Gia_ObjIsCo(pObj) )
        {
            Gla_ManCollectFanins( p, pGla, pObj->Value, p->vTemp );
            pGla->nFanins = Vec_IntSize( p->vTemp );
            memcpy( pGla->Fanins, Vec_IntArray(p->vTemp), sizeof(int) * Vec_IntSize(p->vTemp) );
            continue;
        }
        assert( Gia_ObjIsRo(p->pGia, pObj) );
        pGla->nFanins   = 1;
        pGla->Fanins[0] = Gia_ObjFanin0( Gia_ObjRoToRi(p->pGia, pObj) )->Value;
        pGla->fCompl0   = Gia_ObjFaninC0( Gia_ObjRoToRi(p->pGia, pObj) );
    }
    p->pObjRoot = Gla_ManObj( p, Gia_ManPo(p->pGia, 0)->Value );
    // abstraction 
    assert( pGia->vGateClasses != NULL );
    Gla_ManForEachObj( p, pGla )
    {
        if ( Vec_IntEntry( pGia->vGateClasses, pGla->iGiaObj ) == 0 )
            continue;
        pGla->fAbs = 1;
        Vec_IntPush( p->vAbs, Gla_ObjId(p, pGla) );
    }
    // other 
    p->pSat      = sat_solver2_new();
    p->nSatVars  = 1;
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates a new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gla_ManStop( Gla_Man_t * p )
{
    Gla_Obj_t * pGla;
    int i;

    if ( p->pPars->fVerbose )
        Abc_Print( 1, "SAT solver:  Var = %d  Cla = %d  Conf = %d  Lrn = %d  Reduce = %d  Cex = %d  Objs+ = %d\n", 
            sat_solver2_nvars(p->pSat), sat_solver2_nclauses(p->pSat), sat_solver2_nconflicts(p->pSat), 
            sat_solver2_nlearnts(p->pSat), p->pSat->nDBreduces, p->nCexes, p->nObjAdded );

    // stop the refinement manager
//    Gia_ManStopP( &p->pGia2 );
    Rnm_ManStop( p->pRnm, 0 );

    if ( p->pvRefis )
    for ( i = 0; i < Gia_ManObjNum(p->pGia); i++ )
        ABC_FREE( p->pvRefis[i].pArray );
    Gla_ManForEachObj( p, pGla )
        ABC_FREE( pGla->vFrames.pArray );
    Cnf_DataFree( p->pCnf );
    if ( p->pGia0 != NULL )
        Gia_ManStop( p->pGia );
//    Gia_ManStaticFanoutStart( p->pGia0 );
    sat_solver2_delete( p->pSat );
    Vec_IntFreeP( &p->vObjCounts );
    Vec_IntFreeP( &p->vAddedNew );
    Vec_IntFreeP( &p->vCoreCounts );
    Vec_IntFreeP( &p->vProofIds );
    Vec_IntFreeP( &p->vTemp );
    Vec_IntFreeP( &p->vAbs );
    ABC_FREE( p->pvRefis );
    ABC_FREE( p->pObj2Obj );
    ABC_FREE( p->pObjs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Creates a new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_GlaAbsCount( Gla_Man_t * p, int fRo, int fAnd )
{
    Gla_Obj_t * pObj;
    int i, Counter = 0;
    if ( fRo )
        Gla_ManForEachObjAbs( p, pObj, i )
            Counter += (pObj->fRo && pObj->fAbs);
    else if ( fAnd )
        Gla_ManForEachObjAbs( p, pObj, i )
            Counter += (pObj->fAnd && pObj->fAbs);
    else
        Gla_ManForEachObjAbs( p, pObj, i )
            Counter += (pObj->fAbs);
    return Counter;
}


/**Function*************************************************************

  Synopsis    [Derives new abstraction map.]

  Description [Returns 1 if node contains abstracted leaf on the path.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gla_ManTranslate_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vGla, int nUsageCount )
{
    int Value0, Value1;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return 1;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
        return 0;
    assert( Gia_ObjIsAnd(pObj) );
    Value0 = Gla_ManTranslate_rec( p, Gia_ObjFanin0(pObj), vGla, nUsageCount );
    Value1 = Gla_ManTranslate_rec( p, Gia_ObjFanin1(pObj), vGla, nUsageCount );
    if ( Value0 || Value1 )
        Vec_IntAddToEntry( vGla, Gia_ObjId(p, pObj), nUsageCount );
    return Value0 || Value1;
}
Vec_Int_t * Gla_ManTranslate( Gla_Man_t * p )
{
    Vec_Int_t * vGla, * vGla2;
    Gla_Obj_t * pObj, * pFanin;
    Gia_Obj_t * pGiaObj;
    int i, k, nUsageCount;
    vGla = Vec_IntStart( Gia_ManObjNum(p->pGia) );
    Gla_ManForEachObjAbs( p, pObj, i )
    {
        nUsageCount = Vec_IntEntry(p->vCoreCounts, pObj->iGiaObj);
        assert( nUsageCount >= 0 );
        if ( nUsageCount == 0 )
            nUsageCount++;
        pGiaObj = Gla_ManGiaObj( p, pObj );
        if ( Gia_ObjIsConst0(pGiaObj) || Gia_ObjIsRo(p->pGia, pGiaObj) )
        {
            Vec_IntWriteEntry( vGla, pObj->iGiaObj, nUsageCount );
            continue;
        }
        assert( Gia_ObjIsAnd(pGiaObj) );
        Gia_ManIncrementTravId( p->pGia );
        Gla_ObjForEachFanin( p, pObj, pFanin, k )
            Gia_ObjSetTravIdCurrent( p->pGia, Gla_ManGiaObj(p, pFanin) );
        Gla_ManTranslate_rec( p->pGia, pGiaObj, vGla, nUsageCount );
    }
    Vec_IntWriteEntry( vGla, 0, p->pPars->iFrame+1 );
    if ( p->pGia->vLutConfigs ) // use mapping from new to old
    {
        vGla2 = Vec_IntStart( Gia_ManObjNum(p->pGia0) );
        for ( i = 0; i < Gia_ManObjNum(p->pGia); i++ )
            if ( Vec_IntEntry(vGla, i) )
                Vec_IntWriteEntry( vGla2, Vec_IntEntry(p->pGia->vLutConfigs, i), Vec_IntEntry(vGla, i) );
        Vec_IntFree( vGla );
        return vGla2;
    }
    return vGla;
}


/**Function*************************************************************

  Synopsis    [Collect pseudo-PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gla_ManCollectPPis( Gla_Man_t * p, Vec_Int_t * vPis )
{
    Vec_Int_t * vPPis;
    Gla_Obj_t * pObj, * pFanin;
    int i, k;
    vPPis = Vec_IntAlloc( 1000 );
    if ( vPis )
        Vec_IntClear( vPis );
    Gla_ManForEachObjAbs( p, pObj, i )
    {
        assert( pObj->fConst || pObj->fRo || pObj->fAnd );
        Gla_ObjForEachFanin( p, pObj, pFanin, k )
            if ( !pFanin->fPi && !pFanin->fAbs )
                Vec_IntPush( vPPis, pObj->Fanins[k] );
            else if ( vPis && pFanin->fPi && !pFanin->fAbs )
                Vec_IntPush( vPis, pObj->Fanins[k] );
    }
    Vec_IntUniqify( vPPis );
    Vec_IntReverseOrder( vPPis );
    if ( vPis )
        Vec_IntUniqify( vPis );
    return vPPis;
}
int Gla_ManCountPPis( Gla_Man_t * p )
{
    Vec_Int_t * vPPis = Gla_ManCollectPPis( p, NULL );
    int RetValue = Vec_IntSize( vPPis );
    Vec_IntFree( vPPis );
    return RetValue;
}
void Gla_ManExplorePPis( Gla_Man_t * p, Vec_Int_t * vPPis )
{
    static int Round = 0;
    Gla_Obj_t * pObj, * pFanin;
    int i, j, k, Count;
    if ( (Round++ % 5) == 0 )
        return;
    j = 0;
    Gla_ManForEachObjAbsVec( vPPis, p, pObj, i )
    {
        assert( pObj->fAbs == 0 );
        Count = 0;
        Gla_ObjForEachFanin( p, pObj, pFanin, k )
            Count += pFanin->fAbs;
        if ( Count == 0 || ((Round & 1) && Count == 1) )
            continue;
        Vec_IntWriteEntry( vPPis, j++, Gla_ObjId(p, pObj) );
    }
//    printf( "\n%d -> %d\n", Vec_IntSize(vPPis), j );
    Vec_IntShrink( vPPis, j );
}


/**Function*************************************************************

  Synopsis    [Adds CNF for the given timeframe.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gla_ManCheckVar( Gla_Man_t * p, int iObj, int iFrame )
{
    Gla_Obj_t * pGla = Gla_ManObj( p, iObj );
    int iVar = Vec_IntGetEntry( &pGla->vFrames, iFrame );
    assert( !pGla->fPo && !pGla->fRi );
    return (iVar > 0);
}
int Gla_ManGetVar( Gla_Man_t * p, int iObj, int iFrame )
{
    Gla_Obj_t * pGla = Gla_ManObj( p, iObj );
    int iVar = Vec_IntGetEntry( &pGla->vFrames, iFrame );
    assert( !pGla->fPo && !pGla->fRi );
    if ( iVar == 0 )
    {
        Vec_IntSetEntry( &pGla->vFrames, iFrame, (iVar = p->nSatVars++) );
        // remember the change
        Vec_IntPush( p->vAddedNew, iObj );
        Vec_IntPush( p->vAddedNew, iFrame );
    }
    return iVar;
}
void Gla_ManAddClauses( Gla_Man_t * p, int iObj, int iFrame, Vec_Int_t * vLits )
{
    Gla_Obj_t * pGlaObj = Gla_ManObj( p, iObj );
    int iVar, iVar1, iVar2;
    if ( pGlaObj->fConst )
    {
        iVar = Gla_ManGetVar( p, iObj, iFrame );
        sat_solver2_add_const( p->pSat, iVar, 1, 0, iObj );
    }
    else if ( pGlaObj->fRo )
    {
        assert( pGlaObj->nFanins == 1 );
        if ( iFrame == 0 )
        {
            iVar = Gla_ManGetVar( p, iObj, iFrame );
            sat_solver2_add_const( p->pSat, iVar, 1, 0, iObj );
        }
        else
        {
            iVar1 = Gla_ManGetVar( p, iObj, iFrame );
            iVar2 = Gla_ManGetVar( p, pGlaObj->Fanins[0], iFrame-1 );
            sat_solver2_add_buffer( p->pSat, iVar1, iVar2, pGlaObj->fCompl0, 0, iObj );
        }
    }
    else if ( pGlaObj->fAnd )
    {
        int i, RetValue, nClauses, iFirstClause, * pLit;
        nClauses = p->pCnf->pObj2Count[pGlaObj->iGiaObj];
        iFirstClause = p->pCnf->pObj2Clause[pGlaObj->iGiaObj];
        for ( i = iFirstClause; i < iFirstClause + nClauses; i++ )
        {
            Vec_IntClear( vLits );
            for ( pLit = p->pCnf->pClauses[i]; pLit < p->pCnf->pClauses[i+1]; pLit++ )
            {
                iVar = Gla_ManGetVar( p, lit_var(*pLit), iFrame );
                Vec_IntPush( vLits, toLitCond( iVar, lit_sign(*pLit) ) );
            }
            RetValue = sat_solver2_addclause( p->pSat, Vec_IntArray(vLits), Vec_IntArray(vLits)+Vec_IntSize(vLits), iObj );
        }
    }
    else assert( 0 );
}
void Gia_GlaAddToCounters( Gla_Man_t * p, Vec_Int_t * vCore )
{
    Gla_Obj_t * pGla;
    int i;
    Gla_ManForEachObjAbsVec( vCore, p, pGla, i )
        Vec_IntAddToEntry( p->vCoreCounts, pGla->iGiaObj, 1 );
}
void Gia_GlaAddToAbs( Gla_Man_t * p, Vec_Int_t * vAbsAdd, int fCheck )
{
    Gla_Obj_t * pGla;
    int i, k = 0;
    Gla_ManForEachObjAbsVec( vAbsAdd, p, pGla, i )
    {
        if ( fCheck )
        {
            assert( pGla->fAbs == 0 );
            if ( p->pSat->pPrf2 )
                Vec_IntWriteEntry( p->vProofIds, Gla_ObjId(p, pGla), p->nProofIds++ );
        }
        if ( pGla->fAbs )
            continue;
        pGla->fAbs = 1;
        Vec_IntPush( p->vAbs, Gla_ObjId(p, pGla) );
        // filter clauses to remove those contained in the abstraction
        Vec_IntWriteEntry( vAbsAdd, k++, Gla_ObjId(p, pGla) );
    }
    Vec_IntShrink( vAbsAdd, k );
}
void Gia_GlaAddTimeFrame( Gla_Man_t * p, int f )
{
    Gla_Obj_t * pObj;
    int i;
    Gla_ManForEachObjAbs( p, pObj, i )
        Gla_ManAddClauses( p, Gla_ObjId(p, pObj), f, p->vTemp );
    sat_solver2_simplify( p->pSat );
}
void Gia_GlaAddOneSlice( Gla_Man_t * p, int fCur, Vec_Int_t * vCore )
{
    int f, i, iGlaObj;
    for ( f = fCur; f >= 0; f-- )
        Vec_IntForEachEntry( vCore, iGlaObj, i )
            Gla_ManAddClauses( p, iGlaObj, f, p->vTemp );
    sat_solver2_simplify( p->pSat );
}
void Gla_ManRollBack( Gla_Man_t * p )
{
    int i, iObj, iFrame;
    Vec_IntForEachEntryDouble( p->vAddedNew, iObj, iFrame, i )
    {
        assert( Vec_IntEntry( &Gla_ManObj(p, iObj)->vFrames, iFrame ) > 0 );
        Vec_IntWriteEntry( &Gla_ManObj(p, iObj)->vFrames, iFrame, 0 );
    }
    Vec_IntForEachEntryStart( p->vAbs, iObj, i, p->nAbsOld )
    {
        assert( Gla_ManObj( p, iObj )->fAbs == 1 );
        Gla_ManObj( p, iObj )->fAbs = 0;
    }
    Vec_IntShrink( p->vAbs, p->nAbsOld );
}


            


/**Function*************************************************************

  Synopsis    [Finds the set of clauses involved in the UNSAT core.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gla_ManGetOutLit( Gla_Man_t * p, int f )
{
    Gla_Obj_t * pFanin = Gla_ManObj( p, p->pObjRoot->Fanins[0] );
    int iSat = Vec_IntEntry( &pFanin->vFrames, f );
    assert( iSat > 0 );
    if ( f == 0 && pFanin->fRo && !p->pObjRoot->fCompl0 )
        return -1;
    return Abc_Var2Lit( iSat, p->pObjRoot->fCompl0 );
}
Vec_Int_t * Gla_ManUnsatCore( Gla_Man_t * p, int f, sat_solver2 * pSat, int nConfMax, int fVerbose, int * piRetValue, int * pnConfls )
{
    Vec_Int_t * vCore = NULL;
    int nConfPrev = pSat->stats.conflicts;
    int RetValue, iLit = Gla_ManGetOutLit( p, f );
    abctime clk = Abc_Clock();
    if ( piRetValue )
        *piRetValue = 1;
    // consider special case when PO points to the flop
    // this leads to immediate conflict in the first timeframe
    if ( iLit == -1 )
    {
        vCore = Vec_IntAlloc( 1 );
        Vec_IntPush( vCore, p->pObjRoot->Fanins[0] );
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
    if ( vCore )
        Vec_IntSort( vCore, 1 );
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
void Gla_ManAbsPrintFrame( Gla_Man_t * p, int nCoreSize, int nFrames, int nConfls, int nCexes, abctime Time )
{
    if ( Abc_FrameIsBatchMode() && nCoreSize <= 0 )
        return;
    Abc_Print( 1, "%4d :", nFrames-1 );
    Abc_Print( 1, "%4d", Abc_MinInt(100, 100 * Gia_GlaAbsCount(p, 0, 0) / (p->nObjs - Gia_ManPoNum(p->pGia) + Gia_ManCoNum(p->pGia) + 1)) ); 
    Abc_Print( 1, "%6d", Gia_GlaAbsCount(p, 0, 0) );
    Abc_Print( 1, "%5d", Gla_ManCountPPis(p) );
    Abc_Print( 1, "%5d", Gia_GlaAbsCount(p, 1, 0) );
    Abc_Print( 1, "%6d", Gia_GlaAbsCount(p, 0, 1) );
    Abc_Print( 1, "%8d", nConfls );
    if ( nCexes == 0 )
        Abc_Print( 1, "%5c", '-' ); 
    else
        Abc_Print( 1, "%5d", nCexes ); 
//    Abc_Print( 1, " %9d", sat_solver2_nvars(p->pSat) ); 
    Abc_PrintInt( sat_solver2_nvars(p->pSat) );
    Abc_PrintInt( sat_solver2_nclauses(p->pSat) );
    Abc_PrintInt( sat_solver2_nlearnts(p->pSat) );
//    Abc_Print( 1, " %6d", nCoreSize > 0 ? nCoreSize : 0 ); 
    Abc_Print( 1, "%9.2f sec", 1.0*Time/CLOCKS_PER_SEC );
    Abc_Print( 1, "%5.0f MB", (sat_solver2_memory_proof(p->pSat) + sat_solver2_memory(p->pSat, 0)) / (1<<20) );
//    Abc_PrintInt( p->nAbsNew );
//    Abc_PrintInt( p->nLrnNew );
//    Abc_Print( 1, "%4.1f MB", 4.0 * p->nLrnNew * Abc_BitWordNum(p->nAbsNew) / (1<<20) );
    Abc_Print( 1, "%s", (nCoreSize > 0 && nCexes > 0) ? "\n" : "\r" );
    fflush( stdout );
}
void Gla_ManReportMemory( Gla_Man_t * p )
{
    Gla_Obj_t * pGla;
    double memTot = 0;
    double memAig = Gia_ManObjNum(p->pGia) * sizeof(Gia_Obj_t);
    double memSat = sat_solver2_memory( p->pSat, 1 );
    double memPro = sat_solver2_memory_proof( p->pSat );
    double memMap = p->nObjs * sizeof(Gla_Obj_t) + Gia_ManObjNum(p->pGia) * sizeof(int);
    double memRef = Rnm_ManMemoryUsage( p->pRnm );
    double memOth = sizeof(Gla_Man_t);
    for ( pGla = p->pObjs; pGla < p->pObjs + p->nObjs; pGla++ )
        memMap += Vec_IntCap(&pGla->vFrames) * sizeof(int);
    memOth += Vec_IntCap(p->vAddedNew) * sizeof(int);
    memOth += Vec_IntCap(p->vTemp) * sizeof(int);
    memOth += Vec_IntCap(p->vAbs) * sizeof(int);
    memTot = memAig + memSat + memPro + memMap + memRef + memOth;
    ABC_PRMP( "Memory: AIG      ", memAig, memTot );
    ABC_PRMP( "Memory: SAT      ", memSat, memTot );
    ABC_PRMP( "Memory: Proof    ", memPro, memTot );
    ABC_PRMP( "Memory: Map      ", memMap, memTot );
    ABC_PRMP( "Memory: Refine   ", memRef, memTot );
    ABC_PRMP( "Memory: Other    ", memOth, memTot );
    ABC_PRMP( "Memory: TOTAL    ", memTot, memTot );
}


/**Function*************************************************************

  Synopsis    [Send abstracted model or send cancel.]

  Description [Counter-example will be sent automatically when &vta terminates.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_GlaSendAbsracted( Gla_Man_t * p, int fVerbose )
{
    Gia_Man_t * pAbs;
    Vec_Int_t * vGateClasses;
    assert( Abc_FrameIsBridgeMode() );
//    if ( fVerbose )
//        Abc_Print( 1, "Sending abstracted model...\n" );
    // create abstraction (value of p->pGia is not used here)
    vGateClasses = Gla_ManTranslate( p );
    pAbs = Gia_ManDupAbsGates( p->pGia0, vGateClasses );
    Vec_IntFreeP( &vGateClasses );
    // send it out
    Gia_ManToBridgeAbsNetlist( stdout, pAbs, BRIDGE_ABS_NETLIST );
    Gia_ManStop( pAbs );
}
void Gia_GlaSendCancel( Gla_Man_t * p, int fVerbose )
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
void Gia_GlaDumpAbsracted( Gla_Man_t * p, int fVerbose )
{
    char * pFileNameDef = "glabs.aig";
    char * pFileName = p->pPars->pFileVabs ? p->pPars->pFileVabs : pFileNameDef;
    Gia_Man_t * pAbs;
    Vec_Int_t * vGateClasses;
    if ( fVerbose )
        Abc_Print( 1, "Dumping abstracted model into file \"%s\"...\n", pFileName );
    // create abstraction
    vGateClasses = Gla_ManTranslate( p );
    pAbs = Gia_ManDupAbsGates( p->pGia0, vGateClasses );
    Vec_IntFreeP( &vGateClasses );
    // write into file
    Gia_AigerWrite( pAbs, pFileName, 0, 0 );
    Gia_ManStop( pAbs );
}


/**Function*************************************************************

  Synopsis    [Performs gate-level abstraction]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManPerformGlaOld( Gia_Man_t * pAig, Abs_Par_t * pPars, int fStartVta )
{
    extern int Gia_VtaPerformInt( Gia_Man_t * pAig, Abs_Par_t * pPars );
    extern void Ga2_ManDumpStats( Gia_Man_t * pGia, Abs_Par_t * pPars, sat_solver2 * pSat, int iFrame, int fUseN );
    Gla_Man_t * p;
    Vec_Int_t * vPPis, * vCore;//, * vCore2 = NULL;
    Abc_Cex_t * pCex = NULL;
    int f, i, iPrev, nConfls, Status, nVarsOld = 0, nCoreSize, fOneIsSent = 0, RetValue = -1;
    abctime clk2, clk = Abc_Clock();
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
    if ( pAig->vGateClasses == NULL )
    {
        if ( fStartVta )
        {
            int nFramesMaxOld   = pPars->nFramesMax;
            int nFramesStartOld = pPars->nFramesStart;
            int nTimeOutOld     = pPars->nTimeOut;
            int nDumpOld        = pPars->fDumpVabs;
            pPars->nFramesMax   = pPars->nFramesStart;
            pPars->nFramesStart = Abc_MinInt( pPars->nFramesStart/2 + 1, 3 );
            pPars->nTimeOut     = 20;
            pPars->fDumpVabs    = 0;
            RetValue = Gia_VtaPerformInt( pAig, pPars );
            pPars->nFramesMax   = nFramesMaxOld;
            pPars->nFramesStart = nFramesStartOld;
            pPars->nTimeOut     = nTimeOutOld;
            pPars->fDumpVabs    = nDumpOld;
            // create gate classes
            Vec_IntFreeP( &pAig->vGateClasses );
            if ( pAig->vObjClasses )
                pAig->vGateClasses = Gia_VtaConvertToGla( pAig, pAig->vObjClasses );
            Vec_IntFreeP( &pAig->vObjClasses );
            // return if VTA solve the problem if could not start
            if ( RetValue == 0 || pAig->vGateClasses == NULL )
                return RetValue;
        }
        else
        {
            pAig->vGateClasses = Vec_IntStart( Gia_ManObjNum(pAig) );
            Vec_IntWriteEntry( pAig->vGateClasses, 0, 1 );
            Vec_IntWriteEntry( pAig->vGateClasses, Gia_ObjFaninId0p(pAig, Gia_ManPo(pAig, 0)), 1 );
        }
    }
    // start the manager
    p = Gla_ManStart( pAig, pPars );
    p->timeInit = Abc_Clock() - clk;
    // set runtime limit
    if ( p->pPars->nTimeOut )
        sat_solver2_set_runtime_limit( p->pSat, p->pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock() );
    // perform initial abstraction
    if ( p->pPars->fVerbose )
    {
        Abc_Print( 1, "Running gate-level abstraction (GLA) with the following parameters:\n" );
        Abc_Print( 1, "FrameMax = %d  ConfMax = %d  Timeout = %d  RatioMin = %d %%.\n", 
            pPars->nFramesMax, pPars->nConfLimit, pPars->nTimeOut, pPars->nRatioMin );
        Abc_Print( 1, "LearnStart = %d  LearnDelta = %d  LearnRatio = %d %%.\n", 
            pPars->nLearnedStart, pPars->nLearnedDelta, pPars->nLearnedPerce );
        Abc_Print( 1, " Frame   %%   Abs  PPI   FF   LUT   Confl  Cex   Vars   Clas   Lrns     Time        Mem\n" );
    }
    for ( f = i = iPrev = 0; !p->pPars->nFramesMax || f < p->pPars->nFramesMax; f++, iPrev = i )
    {
        int nConflsBeg = sat_solver2_nconflicts(p->pSat);
        p->pPars->iFrame = f;

        // load timeframe
        Gia_GlaAddTimeFrame( p, f );

        // iterate as long as there are counter-examples
        for ( i = 0; ; i++ )
        { 
            clk2 = Abc_Clock();
            vCore = Gla_ManUnsatCore( p, f, p->pSat, pPars->nConfLimit, pPars->fVerbose, &Status, &nConfls );
//            assert( (vCore != NULL) == (Status == 1) );
            if ( Status == -1 || (p->pSat->nRuntimeLimit && Abc_Clock() > p->pSat->nRuntimeLimit) ) // resource limit is reached
            {
                Prf_ManStopP( &p->pSat->pPrf2 );
//                if ( Gia_ManRegNum(p->pGia) > 1 ) // for comb cases, return the abstraction
//                    Vec_IntShrink( p->vAbs, p->nAbsOld );
                goto finish;
            }
            if ( Status == 1 )
            {
                Prf_ManStopP( &p->pSat->pPrf2 );
                p->timeUnsat += Abc_Clock() - clk2;
                break;
            } 
            p->timeSat += Abc_Clock() - clk2;
            assert( Status == 0 );
            p->nCexes++;

            // cancel old one if it was sent
            if ( Abc_FrameIsBridgeMode() && fOneIsSent )
            {
                Gia_GlaSendCancel( p, pPars->fVerbose );
                fOneIsSent = 0;
            }

            // perform the refinement
            clk2 = Abc_Clock();
            if ( pPars->fAddLayer )
            {
                vPPis = Gla_ManCollectPPis( p, NULL );
//                Gla_ManExplorePPis( p, vPPis );
            }
            else
            {
                vPPis = Gla_ManRefinement( p );
                if ( vPPis == NULL )
                {
                    Prf_ManStopP( &p->pSat->pPrf2 );
                    pCex = p->pGia->pCexSeq; p->pGia->pCexSeq = NULL;
                    break;
                }
            } 
            assert( pCex == NULL );

            // start proof logging
            if ( i == 0 )
            {
                // create bookmark to be used for rollback
                sat_solver2_bookmark( p->pSat );
                Vec_IntClear( p->vAddedNew );
                p->nAbsOld = Vec_IntSize( p->vAbs );
                nVarsOld = p->nSatVars;
//                p->nLrnOld = sat_solver2_nlearnts( p->pSat );
//                p->nAbsNew = 0;
//                p->nLrnNew = 0;

                // start incremental proof manager
                assert( p->pSat->pPrf2 == NULL );
                if ( p->pSat->pPrf1 == NULL )
                    p->pSat->pPrf2 = Prf_ManAlloc();
                if ( p->pSat->pPrf2 )
                {
                    p->nProofIds = 0;
                    Vec_IntFill( p->vProofIds, Gia_ManObjNum(p->pGia), -1 );
                    Prf_ManRestart( p->pSat->pPrf2, p->vProofIds, sat_solver2_nlearnts(p->pSat), Vec_IntSize(vPPis) );
                }
            }
            else
            {
                // resize the proof logger
                if ( p->pSat->pPrf2 )
                    Prf_ManGrow( p->pSat->pPrf2, p->nProofIds + Vec_IntSize(vPPis) );
            }

            Gia_GlaAddToAbs( p, vPPis, 1 );
            Gia_GlaAddOneSlice( p, f, vPPis );
            Vec_IntFree( vPPis );

            // print the result (do not count it towards change)
            if ( p->pPars->fVerbose )
            Gla_ManAbsPrintFrame( p, -1, f+1, sat_solver2_nconflicts(p->pSat)-nConflsBeg, i, Abc_Clock() - clk );
        }
        if ( pCex != NULL )
            break;
        assert( Status == 1 );

        // valid core is obtained
        nCoreSize = 1;
        if ( vCore )
        {
            nCoreSize += Vec_IntSize( vCore );
            Gia_GlaAddToCounters( p, vCore );
        }
        if ( i == 0 )
        {
            p->pPars->nFramesNoChange++;
            Vec_IntFreeP( &vCore );
        }
        else
        {
            p->pPars->nFramesNoChange = 0;
//            p->nAbsNew = Vec_IntSize( p->vAbs ) - p->nAbsOld;
//            p->nLrnNew = Abc_AbsInt( sat_solver2_nlearnts( p->pSat ) - p->nLrnOld );
            // update the SAT solver
            sat_solver2_rollback( p->pSat );
            // update storage
            Gla_ManRollBack( p );
            p->nSatVars = nVarsOld;
            // load this timeframe
            Gia_GlaAddToAbs( p, vCore, 0 );
            Gia_GlaAddOneSlice( p, f, vCore );
            Vec_IntFree( vCore );
            // run SAT solver
            clk2 = Abc_Clock();
            vCore = Gla_ManUnsatCore( p, f, p->pSat, pPars->nConfLimit, p->pPars->fVerbose, &Status, &nConfls );
            p->timeUnsat += Abc_Clock() - clk2;
//            assert( (vCore != NULL) == (Status == 1) );
            Vec_IntFreeP( &vCore );
            if ( Status == -1 ) // resource limit is reached
                break;
            if ( Status == 0 )
            {
                assert( 0 );
    //            Vta_ManSatVerify( p );
                // make sure, there was no initial abstraction (otherwise, it was invalid)
                assert( pAig->vObjClasses == NULL && f < p->pPars->nFramesStart );
    //            pCex = Vga_ManDeriveCex( p );
                break;
            }
        }
        // print the result
        if ( p->pPars->fVerbose )
        Gla_ManAbsPrintFrame( p, nCoreSize, f+1, sat_solver2_nconflicts(p->pSat)-nConflsBeg, i, Abc_Clock() - clk );

        if ( f > 2 && iPrev > 0 && i == 0 ) // change has happened
        {
            if ( Abc_FrameIsBridgeMode() )
            {
                // cancel old one if it was sent
                if ( fOneIsSent )
                    Gia_GlaSendCancel( p, pPars->fVerbose );
                // send new one 
                Gia_GlaSendAbsracted( p, pPars->fVerbose );
                fOneIsSent = 1;
            }

            // dump the model into file
            if ( p->pPars->fDumpVabs )
            {
                char Command[1000];
                Abc_FrameSetStatus( -1 );
                Abc_FrameSetCex( NULL );
                Abc_FrameSetNFrames( f+1 );
                sprintf( Command, "write_status %s", Extra_FileNameGenericAppend((char *)(p->pPars->pFileVabs ? p->pPars->pFileVabs : "glabs.aig"), ".status") );
                Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), Command );
                Gia_GlaDumpAbsracted( p, pPars->fVerbose );
            }
        }

        // check if the number of objects is below limit
        if ( Gia_GlaAbsCount(p,0,0) >= (p->nObjs - 1) * (100 - pPars->nRatioMin) / 100 )
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
//        if ( pAig->vGateClasses != NULL )
//            Abc_Print( 1, "Replacing the old abstraction by a new one.\n" );
        Vec_IntFreeP( &pAig->vGateClasses );
        pAig->vGateClasses = Gla_ManTranslate( p );
        if ( Status == -1 )
        {
            if ( p->pPars->nTimeOut && Abc_Clock() >= p->pSat->nRuntimeLimit ) 
                Abc_Print( 1, "Timeout %d sec in frame %d with a %d-stable abstraction.    ", p->pPars->nTimeOut, f, p->pPars->nFramesNoChange );
            else if ( pPars->nConfLimit && sat_solver2_nconflicts(p->pSat) >= pPars->nConfLimit )
                Abc_Print( 1, "Exceeded %d conflicts in frame %d with a %d-stable abstraction.  ", pPars->nConfLimit, f, p->pPars->nFramesNoChange );
            else if ( Gia_GlaAbsCount(p,0,0) >= (p->nObjs - 1) * (100 - pPars->nRatioMin) / 100 )
                Abc_Print( 1, "The ratio of abstracted objects is less than %d %% in frame %d.  ", pPars->nRatioMin, f );
            else
                Abc_Print( 1, "Abstraction stopped for unknown reason in frame %d.  ", f );
        }
        else
        {
            p->pPars->iFrame++;
            Abc_Print( 1, "GLA completed %d frames with a %d-stable abstraction.  ", f, p->pPars->nFramesNoChange );
        }
    }
    else
    {
        if ( p->pPars->fVerbose )
            printf( "\n" );
        ABC_FREE( pAig->pCexSeq );
        pAig->pCexSeq = pCex;
        if ( !Gia_ManVerifyCex( pAig, pCex, 0 ) )
            Abc_Print( 1, "    Gia_ManPerformGlaOld(): CEX verification has failed!\n" );
        Abc_Print( 1, "Counter-example detected in frame %d.  ", f );
        p->pPars->iFrame = pCex->iFrame - 1;
        Vec_IntFreeP( &pAig->vGateClasses );
        RetValue = 0;
    }
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( p->pPars->fVerbose )
    {
        p->timeOther = (Abc_Clock() - clk) - p->timeUnsat - p->timeSat - p->timeCex - p->timeInit;
        ABC_PRTP( "Runtime: Initializing", p->timeInit,   Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Solver UNSAT", p->timeUnsat,  Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Solver SAT  ", p->timeSat,    Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Refinement  ", p->timeCex,    Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Other       ", p->timeOther,  Abc_Clock() - clk );
        ABC_PRTP( "Runtime: TOTAL       ", Abc_Clock() - clk, Abc_Clock() - clk );
        Gla_ManReportMemory( p );
    }
//    Ga2_ManDumpStats( pAig, p->pPars, p->pSat, p->pPars->iFrame, 1 );
    Gla_ManStop( p );
    fflush( stdout );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

