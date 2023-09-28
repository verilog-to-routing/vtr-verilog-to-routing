/**CFile****************************************************************

  FileName    [giaEmbed.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Logic network derived from AIG.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaEmbed.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <math.h>
#include "gia.h"
#include "aig/ioa/ioa.h"

ABC_NAMESPACE_IMPL_START


/* 
    The code is based on the paper by D. Harel and Y. Koren, 
    "Graph drawing by high-dimensional embedding", 
    J. Graph Algs & Apps, Vol 8(2), pp. 195-217 (2004).
    http://www.emis.de/journals/JGAA/accepted/2004/HarelKoren2004.8.2.pdf 

    Iterative refinement is described in the paper: F. A. Aloul, I. L. Markov, and K. A. Sakallah.
    "FORCE: A Fast and Easy-To-Implement Variable-Ordering Heuristic", Proc. GLSVLSI 03.
    http://www.eecs.umich.edu/~imarkov/pubs/conf/glsvlsi03-force.pdf
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define GIA_PLACE_SIZE 0x7fff 
// objects will be placed in box [0, GIA_PLACE_SIZE] x [0, GIA_PLACE_SIZE]

typedef float  Emb_Dat_t;

typedef struct Emb_Obj_t_ Emb_Obj_t;
struct Emb_Obj_t_
{
    unsigned       fCi      :  1;    // terminal node CI
    unsigned       fCo      :  1;    // terminal node CO
    unsigned       fMark0   :  1;    // first user-controlled mark
    unsigned       fMark1   :  1;    // second user-controlled mark
    unsigned       nFanins  : 28;    // the number of fanins
    unsigned       nFanouts;         // the number of fanouts
    int            hHandle;          // the handle of the node
    union {
    unsigned       TravId;           // user-specified value
    unsigned       iFanin; 
    };
    union {
    unsigned       Value;            // user-specified value
    unsigned       iFanout; 
    };
    int            Fanios[0];        // the array of fanins/fanouts
};

typedef struct Emb_Man_t_ Emb_Man_t;
struct Emb_Man_t_
{
    Gia_Man_t *    pGia;             // the original AIG manager
    Vec_Int_t *    vCis;             // the vector of CIs (PIs + LOs)
    Vec_Int_t *    vCos;             // the vector of COs (POs + LIs)
    int            nObjs;            // the number of objects
    int            nRegs;            // the number of registers
    int            nTravIds;         // traversal ID of the network
    int *          pObjData;         // the array containing data for objects
    int            nObjData;         // the size of array to store the logic network
    int            fVerbose;         // verbose output flag
    Emb_Dat_t *    pVecs;            // array of vectors of size nObjs * nDims
    int            nReached;         // the number of nodes reachable from the pivot
    int            nDistMax;         // the maximum distance from the node
    float **       pMatr;            // covariance matrix nDims * nDims
    float **       pEigen;           // the first several eigen values of the matrix
    float *        pSols;            // solutions to the problem nObjs * nSols;
    unsigned short*pPlacement;       // (x,y) coordinates for each cell
};

static inline int         Emb_ManRegNum( Emb_Man_t * p )                              { return p->nRegs;                                    }
static inline int         Emb_ManCiNum( Emb_Man_t * p )                               { return Vec_IntSize(p->vCis);                        }
static inline int         Emb_ManCoNum( Emb_Man_t * p )                               { return Vec_IntSize(p->vCos);                        }
static inline int         Emb_ManPiNum( Emb_Man_t * p )                               { return Vec_IntSize(p->vCis) - p->nRegs;             }
static inline int         Emb_ManPoNum( Emb_Man_t * p )                               { return Vec_IntSize(p->vCos) - p->nRegs;             }
static inline int         Emb_ManObjNum( Emb_Man_t * p )                              { return p->nObjs;                                    } 
static inline int         Emb_ManNodeNum( Emb_Man_t * p )                             { return p->nObjs - Vec_IntSize(p->vCis) - Vec_IntSize(p->vCos); } 

static inline Emb_Obj_t * Emb_ManObj( Emb_Man_t * p, unsigned hHandle )               { return (Emb_Obj_t *)(p->pObjData + hHandle);        } 
static inline Emb_Obj_t * Emb_ManCi( Emb_Man_t * p, int i )                           { return Emb_ManObj( p, Vec_IntEntry(p->vCis,i) );    }
static inline Emb_Obj_t * Emb_ManCo( Emb_Man_t * p, int i )                           { return Emb_ManObj( p, Vec_IntEntry(p->vCos,i) );    }

static inline int         Emb_ObjIsTerm( Emb_Obj_t * pObj )                           { return pObj->fCi || pObj->fCo;                      } 
static inline int         Emb_ObjIsCi( Emb_Obj_t * pObj )                             { return pObj->fCi;                                   } 
static inline int         Emb_ObjIsCo( Emb_Obj_t * pObj )                             { return pObj->fCo;                                   } 
//static inline int         Emb_ObjIsPi( Emb_Obj_t * pObj )                             { return pObj->fCi && pObj->nFanins == 0;             } 
//static inline int         Emb_ObjIsPo( Emb_Obj_t * pObj )                             { return pObj->fCo && pObj->nFanouts == 0;            } 
static inline int         Emb_ObjIsNode( Emb_Obj_t * pObj )                           { return!Emb_ObjIsTerm(pObj) && pObj->nFanins > 0;    } 
//static inline int         Emb_ObjIsConst0( Emb_Obj_t * pObj )                         { return!Emb_ObjIsTerm(pObj) && pObj->nFanins == 0;   } 

static inline int         Emb_ObjSize( Emb_Obj_t * pObj )                             { return sizeof(Emb_Obj_t) / 4 + pObj->nFanins + pObj->nFanouts;  } 
static inline int         Emb_ObjFaninNum( Emb_Obj_t * pObj )                         { return pObj->nFanins;                               } 
static inline int         Emb_ObjFanoutNum( Emb_Obj_t * pObj )                        { return pObj->nFanouts;                              } 
static inline Emb_Obj_t * Emb_ObjFanin( Emb_Obj_t * pObj, int i )                     { return (Emb_Obj_t *)(((int *)pObj) - pObj->Fanios[i]);               } 
static inline Emb_Obj_t * Emb_ObjFanout( Emb_Obj_t * pObj, int i )                    { return (Emb_Obj_t *)(((int *)pObj) + pObj->Fanios[pObj->nFanins+i]); } 

static inline void        Emb_ManResetTravId( Emb_Man_t * p )                         { extern void Emb_ManCleanTravId( Emb_Man_t * p ); Emb_ManCleanTravId( p ); p->nTravIds = 1;  }
static inline void        Emb_ManIncrementTravId( Emb_Man_t * p )                     { p->nTravIds++;                                      }
static inline void        Emb_ObjSetTravId( Emb_Obj_t * pObj, int TravId )            { pObj->TravId = TravId;                              }
static inline void        Emb_ObjSetTravIdCurrent( Emb_Man_t * p, Emb_Obj_t * pObj )  { pObj->TravId = p->nTravIds;                         }
static inline void        Emb_ObjSetTravIdPrevious( Emb_Man_t * p, Emb_Obj_t * pObj ) { pObj->TravId = p->nTravIds - 1;                     }
static inline int         Emb_ObjIsTravIdCurrent( Emb_Man_t * p, Emb_Obj_t * pObj )   { return ((int)pObj->TravId == p->nTravIds);          }
static inline int         Emb_ObjIsTravIdPrevious( Emb_Man_t * p, Emb_Obj_t * pObj )  { return ((int)pObj->TravId == p->nTravIds - 1);      }

static inline Emb_Dat_t * Emb_ManVec( Emb_Man_t * p, int v )                          { return p->pVecs + v * p->nObjs;                     }
static inline float *     Emb_ManSol( Emb_Man_t * p, int v )                          { return p->pSols + v * p->nObjs;                     }

#define Emb_ManForEachObj( p, pObj, i )               \
    for ( i = 0; (i < p->nObjData) && (pObj = Emb_ManObj(p,i)); i += Emb_ObjSize(pObj) )
#define Emb_ManForEachNode( p, pObj, i )              \
    for ( i = 0; (i < p->nObjData) && (pObj = Emb_ManObj(p,i)); i += Emb_ObjSize(pObj) ) if ( Emb_ObjIsTerm(pObj) ) {} else
#define Emb_ManForEachObjVec( vVec, p, pObj, i )                        \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pObj) = Emb_ManObj(p, Vec_IntEntry(vVec,i))); i++ )
#define Emb_ObjForEachFanin( pObj, pNext, i )         \
    for ( i = 0; (i < (int)pObj->nFanins) && (pNext = Emb_ObjFanin(pObj,i)); i++ )
#define Emb_ObjForEachFanout( pObj, pNext, i )        \
    for ( i = 0; (i < (int)pObj->nFanouts) && (pNext = Emb_ObjFanout(pObj,i)); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates fanin/fanout pair.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ObjAddFanin( Emb_Obj_t * pObj, Emb_Obj_t * pFanin )
{ 
    assert( pObj->iFanin < pObj->nFanins );
    assert( pFanin->iFanout < pFanin->nFanouts );
    pFanin->Fanios[pFanin->nFanins + pFanin->iFanout++] = 
    pObj->Fanios[pObj->iFanin++] = pObj->hHandle - pFanin->hHandle;
}

/**Function*************************************************************

  Synopsis    [Creates logic network isomorphic to the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Emb_Man_t * Emb_ManStartSimple( Gia_Man_t * pGia )
{
    Emb_Man_t * p;
    Emb_Obj_t * pObjLog, * pFanLog;
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int i, nNodes, hHandle = 0;
    // prepare the AIG
    Gia_ManCreateRefs( pGia );
    // create logic network
    p = ABC_CALLOC( Emb_Man_t, 1 );
    p->pGia  = pGia;
    p->nRegs = Gia_ManRegNum(pGia);
    p->vCis  = Vec_IntAlloc( Gia_ManCiNum(pGia) );
    p->vCos  = Vec_IntAlloc( Gia_ManCoNum(pGia) );
    p->nObjData = (sizeof(Emb_Obj_t) / 4) * Gia_ManObjNum(pGia) + 2 * (2 * Gia_ManAndNum(pGia) + Gia_ManCoNum(pGia) + Gia_ManRegNum(pGia) + Gia_ManCoNum(pGia));
    p->pObjData = ABC_CALLOC( int, p->nObjData );
    // create constant node
    Gia_ManConst0(pGia)->Value = hHandle;
    pObjLog = Emb_ManObj( p, hHandle );
    pObjLog->hHandle  = hHandle;
    pObjLog->nFanins  = Gia_ManCoNum(pGia);  //0;
    pObjLog->nFanouts = Gia_ObjRefNum( pGia, Gia_ManConst0(pGia) );
    // count objects
    hHandle += Emb_ObjSize( pObjLog );
    nNodes = 1;
    p->nObjs++;
    // create the PIs
    Gia_ManForEachCi( pGia, pObj, i )
    {
        // create PI object
        pObj->Value = hHandle;
        Vec_IntPush( p->vCis, hHandle );
        pObjLog = Emb_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = Gia_ObjIsRo( pGia, pObj );
        pObjLog->nFanouts = Gia_ObjRefNum( pGia, pObj );
        pObjLog->fCi = 1;
        // count objects
        hHandle += Emb_ObjSize( pObjLog );
        p->nObjs++;
    }
    // create internal nodes
    Gia_ManForEachAnd( pGia, pObj, i )
    {
        assert( Gia_ObjRefNum( pGia, pObj ) > 0 );
        // create node object
        pObj->Value = hHandle;
        pObjLog = Emb_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = 2;
        pObjLog->nFanouts = Gia_ObjRefNum( pGia, pObj );
        // add fanins
        pFanLog = Emb_ManObj( p, Gia_ObjValue(Gia_ObjFanin0(pObj)) ); 
        Emb_ObjAddFanin( pObjLog, pFanLog );
        pFanLog = Emb_ManObj( p, Gia_ObjValue(Gia_ObjFanin1(pObj)) ); 
        Emb_ObjAddFanin( pObjLog, pFanLog );
        // count objects
        hHandle += Emb_ObjSize( pObjLog );
        nNodes++;
        p->nObjs++;
    }
    // create the POs
    Gia_ManForEachCo( pGia, pObj, i )
    {
        // create PO object
        pObj->Value = hHandle;
        Vec_IntPush( p->vCos, hHandle );
        pObjLog = Emb_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = 1;
        pObjLog->nFanouts = 1 + Gia_ObjIsRi( pGia, pObj );
        pObjLog->fCo = 1;
        // add fanins
        pFanLog = Emb_ManObj( p, Gia_ObjValue(Gia_ObjFanin0(pObj)) );
        Emb_ObjAddFanin( pObjLog, pFanLog );
        // count objects
        hHandle += Emb_ObjSize( pObjLog );
        p->nObjs++;
    }
    // connect registers
    Gia_ManForEachRiRo( pGia, pObjRi, pObjRo, i )
        Emb_ObjAddFanin( Emb_ManObj(p,Gia_ObjValue(pObjRo)), Emb_ManObj(p,Gia_ObjValue(pObjRi)) );
    assert( nNodes  == Emb_ManNodeNum(p) );
    assert( hHandle == p->nObjData );
    assert( p->nObjs == Gia_ManObjNum(pGia) );
    if ( hHandle != p->nObjData )
        printf( "Emb_ManStartSimple(): Fatal error in internal representation.\n" );
    // make sure the fanin/fanout counters are correct
    Gia_ManForEachObj( pGia, pObj, i )
    {
        if ( !~Gia_ObjValue(pObj) )
            continue;
        pObjLog = Emb_ManObj( p, Gia_ObjValue(pObj) );
        assert( pObjLog->nFanins  == pObjLog->iFanin || Gia_ObjIsConst0(pObj) );
        assert( pObjLog->nFanouts == pObjLog->iFanout || Gia_ObjIsCo(pObj) );
        pObjLog->iFanin = pObjLog->iFanout = 0;
    }
    ABC_FREE( pGia->pRefs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Collect the fanin IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManCollectSuper_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSuper, Vec_Int_t * vVisit )  
{
    if ( pObj->fMark1 )
        return;
    pObj->fMark1 = 1;
    Vec_IntPush( vVisit, Gia_ObjId(p, pObj) );
    if ( pObj->fMark0 )
    {
        Vec_IntPush( vSuper, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Emb_ManCollectSuper_rec( p, Gia_ObjFanin0(pObj), vSuper, vVisit );
    Emb_ManCollectSuper_rec( p, Gia_ObjFanin1(pObj), vSuper, vVisit );
    
}

/**Function*************************************************************

  Synopsis    [Collect the fanin IDs.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
void Emb_ManCollectSuper( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSuper, Vec_Int_t * vVisit )  
{
    int Entry, i;
    Vec_IntClear( vSuper );
    Vec_IntClear( vVisit );
    assert( pObj->fMark0 == 1 );
    pObj->fMark0 = 0;
    Emb_ManCollectSuper_rec( p, pObj, vSuper, vVisit );
    pObj->fMark0 = 1;
    Vec_IntForEachEntry( vVisit, Entry, i )
        Gia_ManObj(p, Entry)->fMark1 = 0;
}

/**Function*************************************************************

  Synopsis    [Assigns references while removing the MUX/XOR ones.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManCreateRefsSpecial( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj, * pFan0, * pFan1;
    Gia_Obj_t * pObjC, * pObjD0, * pObjD1;
    int i;
    assert( p->pRefs == NULL );
    Gia_ManCleanMark0( p );
    Gia_ManCreateRefs( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        assert( pObj->fMark0 == 0 );
        pFan0 = Gia_ObjFanin0(pObj);
        pFan1 = Gia_ObjFanin1(pObj);
        // skip nodes whose fanins are PIs or are already marked
        if ( Gia_ObjIsCi(pFan0) || pFan0->fMark0 || 
             Gia_ObjIsCi(pFan1) || pFan1->fMark0 )
             continue;
        // skip nodes that are not MUX type
        if ( !Gia_ObjIsMuxType(pObj) )
            continue;
        // the node is MUX type, mark it and its fanins
        pObj->fMark0  = 1;
        pFan0->fMark0 = 1;
        pFan1->fMark0 = 1;
        // deref the control 
        pObjC = Gia_ObjRecognizeMux( pObj, &pObjD1, &pObjD0 );
        Gia_ObjRefDec( p, Gia_Regular(pObjC) );
        if ( Gia_Regular(pObjD0) == Gia_Regular(pObjD1) )
            Gia_ObjRefDec( p, Gia_Regular(pObjD0) );
    }
    Gia_ManForEachAnd( p, pObj, i )
        assert( Gia_ObjRefNum(p, pObj) > 0 );
    Gia_ManCleanMark0( p );
}

/**Function*************************************************************

  Synopsis    [Assigns references while removing the MUX/XOR ones.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManTransformRefs( Gia_Man_t * p, int * pnObjs, int * pnFanios )  
{
    Vec_Int_t * vSuper, * vVisit;
    Gia_Obj_t * pObj, * pFanin;
    int i, k, Counter;
    assert( p->pRefs != NULL );

    // mark nodes to be used in the logic network
    Gia_ManCleanMark0( p );
    Gia_ManConst0(p)->fMark0 = 1;
    // mark the inputs
    Gia_ManForEachCi( p, pObj, i )
        pObj->fMark0 = 1;
    // mark those nodes that have ref count more than 1
    Gia_ManForEachAnd( p, pObj, i )
        pObj->fMark0 = (Gia_ObjRefNum(p, pObj) > 1);
    // mark the output drivers
    Gia_ManForEachCoDriver( p, pObj, i )
        pObj->fMark0 = 1;

    // count the number of nodes
    Counter = 0;
    Gia_ManForEachObj( p, pObj, i )
        Counter += pObj->fMark0;
    *pnObjs = Counter + Gia_ManCoNum(p);

    // reset the references
    ABC_FREE( p->pRefs );
    p->pRefs = ABC_CALLOC( int, Gia_ManObjNum(p) );
    // reference from internal nodes
    Counter = 0;
    vSuper = Vec_IntAlloc( 100 );
    vVisit = Vec_IntAlloc( 100 );
    Gia_ManCleanMark1( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( pObj->fMark0 == 0 )
            continue;
        Emb_ManCollectSuper( p, pObj, vSuper, vVisit );
        Gia_ManForEachObjVec( vSuper, p, pFanin, k )
        {
            assert( pFanin->fMark0 );
            Gia_ObjRefInc( p, pFanin );
        }
        Counter += Vec_IntSize( vSuper );
    }
    Gia_ManCheckMark1( p );
    Vec_IntFree( vSuper );
    Vec_IntFree( vVisit );
    // reference from outputs
    Gia_ManForEachCoDriver( p, pObj, i )
    {
        assert( pObj->fMark0 );
        Gia_ObjRefInc( p, pObj );
    }
    *pnFanios = Counter + Gia_ManCoNum(p);
}

/**Function*************************************************************

  Synopsis    [Cleans the value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManCleanTravId( Emb_Man_t * p )  
{
    Emb_Obj_t * pObj;
    int i;
    Emb_ManForEachObj( p, pObj, i )
        pObj->TravId = 0;
}

/**Function*************************************************************

  Synopsis    [Cleans the value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManSetValue( Emb_Man_t * p )  
{
    Emb_Obj_t * pObj;
    int i, Counter = 0;
    Emb_ManForEachObj( p, pObj, i )
    {
        pObj->Value = Counter++;
//        if ( pObj->fCi && pObj->nFanins == 0 ) 
//            printf( "CI:  Handle = %8d.  Value = %6d. Fanins = %d.\n", pObj->hHandle, pObj->Value, pObj->nFanins );
    }
}

/**Function*************************************************************

  Synopsis    [Creates logic network isomorphic to the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Emb_Man_t * Emb_ManStart( Gia_Man_t * pGia )
{
    Emb_Man_t * p;
    Emb_Obj_t * pObjLog, * pFanLog;
    Gia_Obj_t * pObj, * pObjRi, * pObjRo, * pFanin;
    Vec_Int_t * vSuper, * vVisit;
    int nObjs, nFanios, nNodes = 0;
    int i, k, hHandle = 0;
    // prepare the AIG
//    Gia_ManCreateRefs( pGia );
    Emb_ManCreateRefsSpecial( pGia );
    Emb_ManTransformRefs( pGia, &nObjs, &nFanios );
    Gia_ManFillValue( pGia );
    // create logic network
    p = ABC_CALLOC( Emb_Man_t, 1 );
    p->pGia  = pGia;
    p->nRegs = Gia_ManRegNum(pGia);
    p->vCis  = Vec_IntAlloc( Gia_ManCiNum(pGia) );
    p->vCos  = Vec_IntAlloc( Gia_ManCoNum(pGia) );
    p->nObjData = (sizeof(Emb_Obj_t) / 4) * nObjs + 2 * (nFanios + Gia_ManRegNum(pGia) + Gia_ManCoNum(pGia));
    p->pObjData = ABC_CALLOC( int, p->nObjData );
    // create constant node
    Gia_ManConst0(pGia)->Value = hHandle;
    pObjLog = Emb_ManObj( p, hHandle );
    pObjLog->hHandle  = hHandle;
    pObjLog->nFanins  = Gia_ManCoNum(pGia);  //0;
    pObjLog->nFanouts = Gia_ObjRefNum( pGia, Gia_ManConst0(pGia) );
    // count objects
    hHandle += Emb_ObjSize( pObjLog );
    nNodes++;
    p->nObjs++;
    // create the PIs
    Gia_ManForEachCi( pGia, pObj, i )
    {
        // create PI object
        pObj->Value = hHandle;
        Vec_IntPush( p->vCis, hHandle );
        pObjLog = Emb_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = Gia_ObjIsRo( pGia, pObj );
        pObjLog->nFanouts = Gia_ObjRefNum( pGia, pObj );
        pObjLog->fCi = 1;
        // count objects
        hHandle += Emb_ObjSize( pObjLog );
        p->nObjs++;
    }
    // create internal nodes
    vSuper = Vec_IntAlloc( 100 );
    vVisit = Vec_IntAlloc( 100 );
    Gia_ManForEachAnd( pGia, pObj, i )
    {
        if ( pObj->fMark0 == 0 )
        {
            assert( Gia_ObjRefNum( pGia, pObj ) == 0 );
            continue;
        }
        assert( Gia_ObjRefNum( pGia, pObj ) > 0 );
        Emb_ManCollectSuper( pGia, pObj, vSuper, vVisit );
        // create node object
        pObj->Value = hHandle;
        pObjLog = Emb_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = Vec_IntSize( vSuper );
        pObjLog->nFanouts = Gia_ObjRefNum( pGia, pObj );
        // add fanins
        Gia_ManForEachObjVec( vSuper, pGia, pFanin, k )
        {
            pFanLog = Emb_ManObj( p, Gia_ObjValue(pFanin) ); 
            Emb_ObjAddFanin( pObjLog, pFanLog );
        }
        // count objects
        hHandle += Emb_ObjSize( pObjLog );
        nNodes++;
        p->nObjs++;
    }
    Vec_IntFree( vSuper );
    Vec_IntFree( vVisit );
    // create the POs
    Gia_ManForEachCo( pGia, pObj, i )
    {
        // create PO object
        pObj->Value = hHandle;
        Vec_IntPush( p->vCos, hHandle );
        pObjLog = Emb_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = 1;
        pObjLog->nFanouts = 1 + Gia_ObjIsRi( pGia, pObj );
        pObjLog->fCo = 1;
        // add fanins
        pFanLog = Emb_ManObj( p, Gia_ObjValue(Gia_ObjFanin0(pObj)) );
        Emb_ObjAddFanin( pObjLog, pFanLog );
        // count objects
        hHandle += Emb_ObjSize( pObjLog );
        p->nObjs++;
    }
    // connect registers
    Gia_ManForEachRiRo( pGia, pObjRi, pObjRo, i )
        Emb_ObjAddFanin( Emb_ManObj(p,Gia_ObjValue(pObjRo)), Emb_ManObj(p,Gia_ObjValue(pObjRi)) );
    Gia_ManCleanMark0( pGia );
    assert( nNodes  == Emb_ManNodeNum(p) );
    assert( nObjs   == p->nObjs );
    assert( hHandle == p->nObjData );
    if ( hHandle != p->nObjData )
        printf( "Emb_ManStart(): Fatal error in internal representation.\n" );
    // make sure the fanin/fanout counters are correct
    Gia_ManForEachObj( pGia, pObj, i )
    {
        if ( !~Gia_ObjValue(pObj) )
            continue;
        pObjLog = Emb_ManObj( p, Gia_ObjValue(pObj) );
        assert( pObjLog->nFanins  == pObjLog->iFanin || Gia_ObjIsConst0(pObj) );
        assert( pObjLog->nFanouts == pObjLog->iFanout || Gia_ObjIsCo(pObj) );
        pObjLog->iFanin = pObjLog->iFanout = 0;
    }
    ABC_FREE( pGia->pRefs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates logic network isomorphic to the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManPrintStats( Emb_Man_t * p )
{
//    if ( p->pName )
//        printf( "%8s : ", p->pName );
    printf( "i/o =%7d/%7d  ", Emb_ManPiNum(p), Emb_ManPoNum(p) );
    if ( Emb_ManRegNum(p) )
        printf( "ff =%7d  ", Emb_ManRegNum(p) );
    printf( "node =%8d  ", Emb_ManNodeNum(p) );
    printf( "obj =%8d  ", Emb_ManObjNum(p) );
//    printf( "lev =%5d  ", Emb_ManLevelNum(p) );
//    printf( "cut =%5d  ", Emb_ManCrossCut(p) );
    printf( "mem =%5.2f MB", 4.0*p->nObjData/(1<<20) );
//    printf( "obj =%5d  ", Emb_ManObjNum(p) );
    printf( "\n" );

//    Emb_ManSatExperiment( p );
}

/**Function*************************************************************

  Synopsis    [Creates logic network isomorphic to the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManStop( Emb_Man_t * p )
{
    Vec_IntFree( p->vCis );
    Vec_IntFree( p->vCos );
    ABC_FREE( p->pPlacement );
    ABC_FREE( p->pVecs );
    ABC_FREE( p->pSols );
    ABC_FREE( p->pMatr );
    ABC_FREE( p->pEigen );
    ABC_FREE( p->pObjData );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints the distribution of fanins/fanouts in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManPrintFanio( Emb_Man_t * p )
{
    char Buffer[100];
    Emb_Obj_t * pNode;
    Vec_Int_t * vFanins, * vFanouts;
    int nFanins, nFanouts, nFaninsMax, nFanoutsMax, nFaninsAll, nFanoutsAll;
    int i, k, nSizeMax;

    // determine the largest fanin and fanout
    nFaninsMax = nFanoutsMax = 0;
    nFaninsAll = nFanoutsAll = 0;
    Emb_ManForEachNode( p, pNode, i )
    {
        if ( i == 0 ) continue; // skip const 0 obj
        nFanins  = Emb_ObjFaninNum(pNode);
        nFanouts = Emb_ObjFanoutNum(pNode);
        nFaninsAll  += nFanins;
        nFanoutsAll += nFanouts;
        nFaninsMax   = Abc_MaxInt( nFaninsMax, nFanins );
        nFanoutsMax  = Abc_MaxInt( nFanoutsMax, nFanouts );
    }

    // allocate storage for fanin/fanout numbers
    nSizeMax = Abc_MaxInt( 10 * (Abc_Base10Log(nFaninsMax) + 1), 10 * (Abc_Base10Log(nFanoutsMax) + 1) );
    vFanins  = Vec_IntStart( nSizeMax );
    vFanouts = Vec_IntStart( nSizeMax );

    // count the number of fanins and fanouts
    Emb_ManForEachNode( p, pNode, i )
    {
        if ( i == 0 ) continue; // skip const 0 obj
        nFanins  = Emb_ObjFaninNum(pNode);
        nFanouts = Emb_ObjFanoutNum(pNode);

        if ( nFanins < 10 )
            Vec_IntAddToEntry( vFanins, nFanins, 1 );
        else if ( nFanins < 100 )
            Vec_IntAddToEntry( vFanins, 10 + nFanins/10, 1 );
        else if ( nFanins < 1000 )
            Vec_IntAddToEntry( vFanins, 20 + nFanins/100, 1 );
        else if ( nFanins < 10000 )
            Vec_IntAddToEntry( vFanins, 30 + nFanins/1000, 1 );
        else if ( nFanins < 100000 )
            Vec_IntAddToEntry( vFanins, 40 + nFanins/10000, 1 );
        else if ( nFanins < 1000000 )
            Vec_IntAddToEntry( vFanins, 50 + nFanins/100000, 1 );
        else if ( nFanins < 10000000 )
            Vec_IntAddToEntry( vFanins, 60 + nFanins/1000000, 1 );

        if ( nFanouts < 10 )
            Vec_IntAddToEntry( vFanouts, nFanouts, 1 );
        else if ( nFanouts < 100 )
            Vec_IntAddToEntry( vFanouts, 10 + nFanouts/10, 1 );
        else if ( nFanouts < 1000 )
            Vec_IntAddToEntry( vFanouts, 20 + nFanouts/100, 1 );
        else if ( nFanouts < 10000 )
            Vec_IntAddToEntry( vFanouts, 30 + nFanouts/1000, 1 );
        else if ( nFanouts < 100000 )
            Vec_IntAddToEntry( vFanouts, 40 + nFanouts/10000, 1 );
        else if ( nFanouts < 1000000 )
            Vec_IntAddToEntry( vFanouts, 50 + nFanouts/100000, 1 );
        else if ( nFanouts < 10000000 )
            Vec_IntAddToEntry( vFanouts, 60 + nFanouts/1000000, 1 );
    }

    printf( "The distribution of fanins and fanouts in the network:\n" );
    printf( "         Number   Nodes with fanin  Nodes with fanout\n" );
    for ( k = 0; k < nSizeMax; k++ )
    {
        if ( vFanins->pArray[k] == 0 && vFanouts->pArray[k] == 0 )
            continue;
        if ( k < 10 )
            printf( "%15d : ", k );
        else
        {
            sprintf( Buffer, "%d - %d", (int)pow((double)10, k/10) * (k%10), (int)pow((double)10, k/10) * (k%10+1) - 1 ); 
            printf( "%15s : ", Buffer );
        }
        if ( vFanins->pArray[k] == 0 )
            printf( "              " );
        else
            printf( "%12d  ", vFanins->pArray[k] );
        printf( "    " );
        if ( vFanouts->pArray[k] == 0 )
            printf( "              " );
        else
            printf( "%12d  ", vFanouts->pArray[k] );
        printf( "\n" );
    }
    Vec_IntFree( vFanins );
    Vec_IntFree( vFanouts );

    printf( "Fanins: Max = %d. Ave = %.2f.  Fanouts: Max = %d. Ave =  %.2f.\n", 
        nFaninsMax,  1.0*nFaninsAll/Emb_ManNodeNum(p), 
        nFanoutsMax, 1.0*nFanoutsAll/Emb_ManNodeNum(p)  );
}

/**Function*************************************************************

  Synopsis    [Computes the distance from the given object]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Emb_ManComputeDistance_old( Emb_Man_t * p, Emb_Obj_t * pPivot )
{
    Vec_Int_t * vThis, * vNext, * vTemp;
    Emb_Obj_t * pThis, * pNext;
    int i, k, d, nVisited = 0;
//    assert( Emb_ObjIsTerm(pPivot) );
    vThis = Vec_IntAlloc( 1000 );
    vNext = Vec_IntAlloc( 1000 );
    Emb_ManIncrementTravId( p );
    Emb_ObjSetTravIdCurrent( p, pPivot );
    Vec_IntPush( vThis, pPivot->hHandle );
    for ( d = 0; Vec_IntSize(vThis) > 0; d++ )
    {
        nVisited += Vec_IntSize(vThis);
        Vec_IntClear( vNext );
        Emb_ManForEachObjVec( vThis, p, pThis, i )
        {
            Emb_ObjForEachFanin( pThis, pNext, k )
            {
                if ( Emb_ObjIsTravIdCurrent(p, pNext) )
                    continue;
                Emb_ObjSetTravIdCurrent(p, pNext);
                Vec_IntPush( vNext, pNext->hHandle );
                nVisited += !Emb_ObjIsTerm(pNext);
            }
            Emb_ObjForEachFanout( pThis, pNext, k )
            {
                if ( Emb_ObjIsTravIdCurrent(p, pNext) )
                    continue;
                Emb_ObjSetTravIdCurrent(p, pNext);
                Vec_IntPush( vNext, pNext->hHandle );
                nVisited += !Emb_ObjIsTerm(pNext);
            }
        }
        vTemp = vThis; vThis = vNext; vNext = vTemp;
    }
    Vec_IntFree( vThis );
    Vec_IntFree( vNext );
    // check if there are several strongly connected components
//    if ( nVisited < Emb_ManNodeNum(p) )
//        printf( "Visited less nodes (%d) than present (%d).\n", nVisited, Emb_ManNodeNum(p) );
    return d;
}

/**Function*************************************************************

  Synopsis    [Traverses from the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
void Gia_ManTestDistanceInternal( Emb_Man_t * p )
{
    int nAttempts = 20;
    int i, iNode, Dist;
    abctime clk;
    Emb_Obj_t * pPivot, * pNext;
    Gia_ManRandom( 1 );
    Emb_ManResetTravId( p );
    // compute distances from several randomly selected PIs
    clk = Abc_Clock();
    printf( "From inputs: " );
    for ( i = 0; i < nAttempts; i++ )
    {
        iNode = Gia_ManRandom( 0 ) % Emb_ManCiNum(p);
        pPivot = Emb_ManCi( p, iNode );
        if ( Emb_ObjFanoutNum(pPivot) == 0 )
            { i--; continue; }
        pNext = Emb_ObjFanout( pPivot, 0 );
        if ( !Emb_ObjIsNode(pNext) )
            { i--; continue; }
        Dist = Emb_ManComputeDistance_old( p, pPivot );
        printf( "%d ", Dist );
    }
    ABC_PRT( "Time", Abc_Clock() - clk );
    // compute distances from several randomly selected POs
    clk = Abc_Clock();
    printf( "From outputs: " );
    for ( i = 0; i < nAttempts; i++ )
    {
        iNode = Gia_ManRandom( 0 ) % Emb_ManCoNum(p);
        pPivot = Emb_ManCo( p, iNode );
        pNext = Emb_ObjFanin( pPivot, 0 );
        if ( !Emb_ObjIsNode(pNext) )
            { i--; continue; }
        Dist = Emb_ManComputeDistance_old( p, pPivot );
        printf( "%d ", Dist );
    }
    ABC_PRT( "Time", Abc_Clock() - clk );
    // compute distances from several randomly selected nodes
    clk = Abc_Clock();
    printf( "From nodes: " );
    for ( i = 0; i < nAttempts; i++ )
    {
        iNode = Gia_ManRandom( 0 ) % Gia_ManObjNum(p->pGia);
        if ( !~Gia_ManObj(p->pGia, iNode)->Value )
            { i--; continue; }
        pPivot = Emb_ManObj( p, Gia_ManObj(p->pGia, iNode)->Value );
        if ( !Emb_ObjIsNode(pPivot) )
            { i--; continue; }
        Dist = Emb_ManComputeDistance_old( p, pPivot );
        printf( "%d ", Dist );
    }
    ABC_PRT( "Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Returns sorted array of node handles with largest fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTestDistance( Gia_Man_t * pGia )
{
    Emb_Man_t * p;
    abctime clk = Abc_Clock();
    p = Emb_ManStart( pGia );
//    Emb_ManPrintFanio( p );
    Emb_ManPrintStats( p );
ABC_PRT( "Time", Abc_Clock() - clk );
    Gia_ManTestDistanceInternal( p );
    Emb_ManStop( p );
}




/**Function*************************************************************

  Synopsis    [Perform BFS from the set of nodes.]

  Description [Returns one of the most distant objects.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Emb_Obj_t * Emb_ManPerformBfs( Emb_Man_t * p, Vec_Int_t * vThis, Vec_Int_t * vNext, Emb_Dat_t * pDist )
{
    Vec_Int_t * vTemp;
    Emb_Obj_t * pThis, * pNext, * pResult;
    int i, k;
    assert( Vec_IntSize(vThis) > 0 );
    for ( p->nDistMax = 0; Vec_IntSize(vThis) > 0; p->nDistMax++ )
    {
        p->nReached += Vec_IntSize(vThis);
        Vec_IntClear( vNext );
        Emb_ManForEachObjVec( vThis, p, pThis, i )
        {
            if ( pDist ) pDist[pThis->Value] = p->nDistMax;
            Emb_ObjForEachFanin( pThis, pNext, k )
            {
                if ( Emb_ObjIsTravIdCurrent(p, pNext) )
                    continue;
                Emb_ObjSetTravIdCurrent(p, pNext);
                Vec_IntPush( vNext, pNext->hHandle );
            }
            Emb_ObjForEachFanout( pThis, pNext, k )
            {
                if ( Emb_ObjIsTravIdCurrent(p, pNext) )
                    continue;
                Emb_ObjSetTravIdCurrent(p, pNext);
                Vec_IntPush( vNext, pNext->hHandle );
            }
        }
        vTemp = vThis; vThis = vNext; vNext = vTemp;
    }
    assert( Vec_IntSize(vNext) > 0 );
    pResult = Emb_ManObj( p, Vec_IntEntry(vNext, 0) );
    assert( pDist == NULL || pDist[pResult->Value] == p->nDistMax - 1 );
    return pResult;
}

/**Function*************************************************************

  Synopsis    [Computes the distances from the given set of objects.]

  Description [Returns one of the most distant objects.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Emb_ManConnectedComponents( Emb_Man_t * p )
{
    Gia_Obj_t * pObj;
    Vec_Int_t * vThis, * vNext, * vResult;
    Emb_Obj_t * pThis;
    int i;
    vResult = Vec_IntAlloc( 1000 );
    vThis   = Vec_IntAlloc( 1000 );
    vNext   = Vec_IntAlloc( 1000 );
    p->nReached = 0;
    Emb_ManIncrementTravId( p );
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        pThis = Emb_ManObj( p, Gia_ObjValue(pObj) );
        if ( Emb_ObjIsTravIdCurrent(p, pThis) )
            continue;
        Emb_ObjSetTravIdCurrent( p, pThis );
        Vec_IntPush( vResult, pThis->hHandle );
        // perform BFS from this node
        Vec_IntClear( vThis );
        Vec_IntPush( vThis, pThis->hHandle );
        Emb_ManPerformBfs( p, vThis, vNext, NULL ); 
    }
    Vec_IntFree( vThis );
    Vec_IntFree( vNext );
    return vResult;
}

/**Function*************************************************************

  Synopsis    [Computes the distances from the given set of objects.]

  Description [Returns one of the most distant objects.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Emb_Obj_t * Emb_ManFindDistances( Emb_Man_t * p, Vec_Int_t * vStart, Emb_Dat_t * pDist )
{
    Vec_Int_t * vThis, * vNext;
    Emb_Obj_t * pThis, * pResult;
    int i;
    p->nReached = p->nDistMax = 0;
    vThis = Vec_IntAlloc( 1000 );
    vNext = Vec_IntAlloc( 1000 );
    Emb_ManIncrementTravId( p );
    Emb_ManForEachObjVec( vStart, p, pThis, i )
    {
        Emb_ObjSetTravIdCurrent( p, pThis );
        Vec_IntPush( vThis, pThis->hHandle );
    }
    pResult = Emb_ManPerformBfs( p, vThis, vNext, pDist );
    Vec_IntFree( vThis );
    Vec_IntFree( vNext );
    return pResult;
}

/**Function*************************************************************

  Synopsis    [Traverses from the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
Emb_Obj_t * Emb_ManRandomVertex( Emb_Man_t * p )
{
    Emb_Obj_t * pPivot;
    do {
        int iNode = (911 * Gia_ManRandom(0)) % Gia_ManObjNum(p->pGia);
        if ( ~Gia_ManObj(p->pGia, iNode)->Value )
            pPivot = Emb_ManObj( p, Gia_ManObj(p->pGia, iNode)->Value );
        else
            pPivot = NULL;
    }
    while ( pPivot == NULL || !Emb_ObjIsNode(pPivot) );
    return pPivot;
}

/**Function*************************************************************

  Synopsis    [Computes the distances from the given set of objects.]

  Description [Returns one of the most distant objects.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_DumpGraphIntoFile( Emb_Man_t * p )
{
    FILE * pFile;
    Emb_Obj_t * pThis, * pNext;
    int i, k;
    pFile = fopen( "1.g", "w" );
    Emb_ManForEachObj( p, pThis, i )
    {
        if ( !Emb_ObjIsTravIdCurrent(p, pThis) )
            continue;
        Emb_ObjForEachFanout( pThis, pNext, k )
        {
            assert( Emb_ObjIsTravIdCurrent(p, pNext) );
            fprintf( pFile, "%d %d\n", pThis->Value, pNext->Value );
        }
    }
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Computes dimentions of the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManComputeDimensions( Emb_Man_t * p, int nDims )
{
    Emb_Obj_t * pRandom, * pPivot;
    Vec_Int_t * vStart, * vComps;
    int d, nReached;
    int i;//, Counter;
    // connect unconnected components
    vComps = Emb_ManConnectedComponents( p );
//    printf( "Components = %d. Considered %d objects (out of %d).\n", Vec_IntSize(vComps), p->nReached, Emb_ManObjNum(p) );
    if ( Vec_IntSize(vComps) > 1 )
    {
        Emb_Obj_t * pFanin, * pObj = Emb_ManObj( p, 0 );
        Emb_ManForEachObjVec( vComps, p, pFanin, i )
        {
            assert( Emb_ObjIsCo(pFanin) );
            pFanin->Fanios[pFanin->nFanins + pFanin->nFanouts-1] = 
                pObj->Fanios[i] = pObj->hHandle - pFanin->hHandle;
        }
    }
    Vec_IntFree( vComps );
    // allocate memory for vectors
    assert( p->pVecs == NULL );
    p->pVecs = ABC_CALLOC( Emb_Dat_t, p->nObjs * nDims );
//    for ( i = 0; i < p->nObjs * nDims; i++ )
//        p->pVecs[i] = ABC_INFINITY;
    vStart = Vec_IntAlloc( nDims );
    // get the pivot vertex
    pRandom = Emb_ManRandomVertex( p );
    Vec_IntPush( vStart, pRandom->hHandle );
    // get the most distant vertex from the pivot
    pPivot = Emb_ManFindDistances( p, vStart, NULL );
//    Emb_DumpGraphIntoFile( p );
    nReached = p->nReached;
    if ( nReached < Emb_ManObjNum(p) )
    {
//        printf( "Considering a connected component with %d objects (out of %d).\n", p->nReached, Emb_ManObjNum(p) );
    }
    // start dimensions with this vertex
    Vec_IntClear( vStart );
    for ( d = 0; d < nDims; d++ )
    {
//        printf( "%3d : Adding vertex %7d with distance %3d.\n", d+1, pPivot->Value, p->nDistMax ); 
        Vec_IntPush( vStart, pPivot->hHandle );
        if ( d+1 == nReached )
            break;
        pPivot = Emb_ManFindDistances( p, vStart, Emb_ManVec(p, d) );
        assert( nReached == p->nReached );
    }
    Vec_IntFree( vStart );
    // make sure the number of reached objects is correct
//    Counter = 0;
//    for ( i = 0; i < p->nObjs; i++ )
//        if ( p->pVecs[i] < ABC_INFINITY )
//            Counter++;
//    assert( Counter == nReached );
}

/**Function*************************************************************

  Synopsis    [Allocated square matrix of floats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float ** Emb_ManMatrAlloc( int nDims )
{
    int i;
    float ** pMatr = (float **)ABC_ALLOC( char, sizeof(float *) * nDims + sizeof(float) * nDims * nDims );
    pMatr[0] = (float *)(pMatr + nDims);
    for ( i = 1; i < nDims; i++ )
        pMatr[i] = pMatr[i-1] + nDims;
    return pMatr;
}

/**Function*************************************************************

  Synopsis    [Computes covariance matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManComputeCovariance( Emb_Man_t * p, int nDims )
{
    Emb_Dat_t * pOne, * pTwo;
    double Ave;
    float * pRow;
    int d, i, k, v;
    // average vectors
    for ( d = 0; d < nDims; d++ )
    {
        // compute average
        Ave = 0.0;
        pOne = Emb_ManVec( p, d );
        for ( v = 0; v < p->nObjs; v++ )
            if ( pOne[v] < ABC_INFINITY )
                Ave += pOne[v];
        Ave /= p->nReached;
        // update the vector
        for ( v = 0; v < p->nObjs; v++ )
            if ( pOne[v] < ABC_INFINITY )
                pOne[v] -= Ave;
            else
                pOne[v] = 0.0;        
    }
    // compute the matrix
    assert( p->pMatr == NULL );
    assert( p->pEigen == NULL );
    p->pMatr = Emb_ManMatrAlloc( nDims );
    p->pEigen = Emb_ManMatrAlloc( nDims );
    for ( i = 0; i < nDims; i++ )
    {
        pOne = Emb_ManVec( p, i );
        pRow = p->pMatr[i];
        for ( k = 0; k < nDims; k++ )
        {
            pTwo = Emb_ManVec( p, k );
            pRow[k] = 0.0;
            for ( v = 0; v < p->nObjs; v++ )
                pRow[k] += pOne[v]*pTwo[v];
        }
    }
}

/**Function*************************************************************

  Synopsis    [Returns random vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManVecRandom( float * pVec, int nDims )
{
    int i;
    for ( i = 0; i < nDims; i++ )
        pVec[i] = Gia_ManRandom( 0 );
}

/**Function*************************************************************

  Synopsis    [Returns normalized vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManVecNormal( float * pVec, int nDims )
{
    int i;
    double Norm = 0.0;
    for ( i = 0; i < nDims; i++ )
        Norm += pVec[i] * pVec[i];
    Norm = pow( Norm, 0.5 );
    for ( i = 0; i < nDims; i++ )
        pVec[i] /= Norm;
}

/**Function*************************************************************

  Synopsis    [Multiplies vector by vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Emb_ManVecMultiplyOne( float * pVec0, float * pVec1, int nDims )
{
    float Res = 0.0;
    int i;
    for ( i = 0; i < nDims; i++ )
        Res += pVec0[i] * pVec1[i];
    return Res;
}

/**Function*************************************************************

  Synopsis    [Copies the vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManVecCopyOne( float * pVecDest, float * pVecSour, int nDims )
{
    int i;
    for ( i = 0; i < nDims; i++ )
        pVecDest[i] = pVecSour[i];
}

/**Function*************************************************************

  Synopsis    [Multiplies matrix by vector.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManVecMultiply( float ** pMatr, float * pVec, int nDims, float * pRes )
{
    int k;
    for ( k = 0; k < nDims; k++ )
        pRes[k] = Emb_ManVecMultiplyOne( pMatr[k], pVec, nDims );
}

/**Function*************************************************************

  Synopsis    [Multiplies vector by matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManVecOrthogonolizeOne( float * pEigen, float * pVecI, int nDims, float * pVecRes )
{
    int k;
    for ( k = 0; k < nDims; k++ )
        pVecRes[k] = pVecI[k] - pEigen[k] * Emb_ManVecMultiplyOne( pVecI, pEigen, nDims );
}

/**Function*************************************************************

  Synopsis    [Computes the first nSols eigen-vectors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManComputeEigenvectors( Emb_Man_t * p, int nDims, int nSols )
{
    float * pVecUiHat, * pVecUi;
    int i, j, k;
    assert( nSols < nDims );
    pVecUiHat = p->pEigen[nSols];
    for ( i = 0; i < nSols; i++ )
    {
        pVecUi = p->pEigen[i];
        Emb_ManVecRandom( pVecUiHat, nDims );
        Emb_ManVecNormal( pVecUiHat, nDims );
        k = 0;
        do {
            k++;
            Emb_ManVecCopyOne( pVecUi, pVecUiHat, nDims );
            for ( j = 0; j < i; j++ )
            {
                Emb_ManVecOrthogonolizeOne( p->pEigen[j], pVecUi, nDims, pVecUiHat );
                Emb_ManVecCopyOne( pVecUi, pVecUiHat, nDims );
            }
            Emb_ManVecMultiply( p->pMatr, pVecUi, nDims, pVecUiHat );
            Emb_ManVecNormal( pVecUiHat, nDims );
        } while ( Emb_ManVecMultiplyOne( pVecUiHat, pVecUi, nDims ) < 0.999 && k < 100 );
        Emb_ManVecCopyOne( pVecUi, pVecUiHat, nDims );
//        printf( "Converged after %d iterations.\n", k );
    }
}

/**Function*************************************************************

  Synopsis    [Derives solutions from original vectors and eigenvectors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManComputeSolutions( Emb_Man_t * p, int nDims, int nSols )
{
    Emb_Dat_t * pX;
    float * pY;
    int i, j, k;
    assert( p->pSols == NULL );
    p->pSols = ABC_CALLOC( float, p->nObjs * nSols );
    for ( i = 0; i < nDims; i++ )
    {
        pX = Emb_ManVec( p, i );
        for ( j = 0; j < nSols; j++ )
        {
            pY = Emb_ManSol( p, j );
            for ( k = 0; k < p->nObjs; k++ )
                pY[k] += pX[k] * p->pEigen[j][i];
        }
    }
}

/**Function*************************************************************

  Synopsis    [Projects into square of size [0;GIA_PLACE_SIZE] x [0;GIA_PLACE_SIZE].]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManDerivePlacement( Emb_Man_t * p, int nSols )
{
    float * pY0, * pY1, Max0, Max1, Min0, Min1, Str0, Str1;
    int * pPerm0, * pPerm1;
    int k;
    if ( nSols != 2 )
        return;
    // compute intervals
    Min0 =  ABC_INFINITY;
    Max0 = -ABC_INFINITY;
    pY0 = Emb_ManSol( p, 0 );
    for ( k = 0; k < p->nObjs; k++ )
    {
        Min0 = Abc_MinInt( Min0, pY0[k] );
        Max0 = Abc_MaxInt( Max0, pY0[k] );
    }
    Str0 = 1.0*GIA_PLACE_SIZE/(Max0 - Min0);
    // update the coordinates
    for ( k = 0; k < p->nObjs; k++ )
        pY0[k] = (pY0[k] != 0.0) ? ((pY0[k] - Min0) * Str0) : 0.0;

    // compute intervals
    Min1 =  ABC_INFINITY;
    Max1 = -ABC_INFINITY;
    pY1 = Emb_ManSol( p, 1 );
    for ( k = 0; k < p->nObjs; k++ )
    {
        Min1 = Abc_MinInt( Min1, pY1[k] );
        Max1 = Abc_MaxInt( Max1, pY1[k] );
    }
    Str1 = 1.0*GIA_PLACE_SIZE/(Max1 - Min1);
    // update the coordinates
    for ( k = 0; k < p->nObjs; k++ )
        pY1[k] = (pY1[k] != 0.0) ? ((pY1[k] - Min1) * Str1) : 0.0;

    // derive the order of these numbers
    pPerm0 = Gia_SortFloats( pY0, NULL, p->nObjs );
    pPerm1 = Gia_SortFloats( pY1, NULL, p->nObjs );

    // average solutions and project them into square [0;GIA_PLACE_SIZE] x [0;GIA_PLACE_SIZE]
    p->pPlacement = ABC_ALLOC( unsigned short, 2 * p->nObjs );
    for ( k = 0; k < p->nObjs; k++ )
    {
        p->pPlacement[2*pPerm0[k]+0] = (unsigned short)(int)(1.0 * k * GIA_PLACE_SIZE / p->nObjs);
        p->pPlacement[2*pPerm1[k]+1] = (unsigned short)(int)(1.0 * k * GIA_PLACE_SIZE / p->nObjs);
    }
    ABC_FREE( pPerm0 );
    ABC_FREE( pPerm1 );
}


/**Function*************************************************************

  Synopsis    [Computes wire-length.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Emb_ManComputeHPWL( Emb_Man_t * p )
{
    double Result = 0.0;
    Emb_Obj_t * pThis, * pNext;
    int i, k, iMinX, iMaxX, iMinY, iMaxY;
    if ( p->pPlacement == NULL )
        return 0.0;
    Emb_ManForEachObj( p, pThis, i )
    {
        iMinX = iMaxX = p->pPlacement[2*pThis->Value+0];
        iMinY = iMaxY = p->pPlacement[2*pThis->Value+1];
        Emb_ObjForEachFanout( pThis, pNext, k )
        {
            iMinX = Abc_MinInt( iMinX, p->pPlacement[2*pNext->Value+0] );
            iMaxX = Abc_MaxInt( iMaxX, p->pPlacement[2*pNext->Value+0] );
            iMinY = Abc_MinInt( iMinY, p->pPlacement[2*pNext->Value+1] );
            iMaxY = Abc_MaxInt( iMaxY, p->pPlacement[2*pNext->Value+1] );
        }
        Result += (iMaxX - iMinX) + (iMaxY - iMinY);
    }
    return Result;
}


/**Function*************************************************************

  Synopsis    [Performs iterative refinement of the given placement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManPlacementRefine( Emb_Man_t * p, int nIters, int fVerbose )
{
    Emb_Obj_t * pThis, * pNext;
    double CostThis, CostPrev;
    float * pEdgeX, * pEdgeY;
    float * pVertX, * pVertY;
    float VertX, VertY;
    int * pPermX, * pPermY;
    int i, k, Iter, iMinX, iMaxX, iMinY, iMaxY;
    abctime clk = Abc_Clock();
    if ( p->pPlacement == NULL )
        return;
    pEdgeX = ABC_ALLOC( float, p->nObjs );
    pEdgeY = ABC_ALLOC( float, p->nObjs );
    pVertX = ABC_ALLOC( float, p->nObjs );
    pVertY = ABC_ALLOC( float, p->nObjs );
    // refine placement
    CostPrev = 0.0;
    for ( Iter = 0; Iter < nIters; Iter++ )
    {
        // compute centers of hyperedges
        CostThis = 0.0;
        Emb_ManForEachObj( p, pThis, i )
        {
            iMinX = iMaxX = p->pPlacement[2*pThis->Value+0];
            iMinY = iMaxY = p->pPlacement[2*pThis->Value+1];
            Emb_ObjForEachFanout( pThis, pNext, k )
            {
                iMinX = Abc_MinInt( iMinX, p->pPlacement[2*pNext->Value+0] );
                iMaxX = Abc_MaxInt( iMaxX, p->pPlacement[2*pNext->Value+0] );
                iMinY = Abc_MinInt( iMinY, p->pPlacement[2*pNext->Value+1] );
                iMaxY = Abc_MaxInt( iMaxY, p->pPlacement[2*pNext->Value+1] );
            }
            pEdgeX[pThis->Value] = 0.5 * (iMaxX + iMinX);
            pEdgeY[pThis->Value] = 0.5 * (iMaxY + iMinY);
            CostThis += (iMaxX - iMinX) + (iMaxY - iMinY);
        }
        // compute new centers of objects
        Emb_ManForEachObj( p, pThis, i )
        {
            VertX = pEdgeX[pThis->Value];
            VertY = pEdgeY[pThis->Value];
            Emb_ObjForEachFanin( pThis, pNext, k )
            {
                VertX += pEdgeX[pNext->Value];
                VertY += pEdgeY[pNext->Value];
            }
            pVertX[pThis->Value] = VertX / (Emb_ObjFaninNum(pThis) + 1);
            pVertY[pThis->Value] = VertY / (Emb_ObjFaninNum(pThis) + 1);
        }
        // sort these numbers
        pPermX = Gia_SortFloats( pVertX, NULL, p->nObjs );
        pPermY = Gia_SortFloats( pVertY, NULL, p->nObjs );
        for ( k = 0; k < p->nObjs; k++ )
        {
            p->pPlacement[2*pPermX[k]+0] = (unsigned short)(int)(1.0 * k * GIA_PLACE_SIZE / p->nObjs);
            p->pPlacement[2*pPermY[k]+1] = (unsigned short)(int)(1.0 * k * GIA_PLACE_SIZE / p->nObjs);
        }
        ABC_FREE( pPermX );
        ABC_FREE( pPermY );
        // evaluate cost
        if ( fVerbose )
        {
        printf( "%2d : HPWL = %e  ", Iter+1, CostThis );
        ABC_PRT( "Time", Abc_Clock() - clk );
        }
    }
    ABC_FREE( pEdgeX );
    ABC_FREE( pEdgeY );
    ABC_FREE( pVertX );
    ABC_FREE( pVertY );
}


/**Function*************************************************************

  Synopsis    [Derives solutions from original vectors and eigenvectors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManPrintSolutions( Emb_Man_t * p, int nSols )
{
    float * pSol;
    int i, k;
    for ( i = 0; i < nSols; i++ )
    {
        pSol = Emb_ManSol( p, i );
        for ( k = 0; k < p->nObjs; k++ )
            printf( "%4d ", (int)(100 * pSol[k]) );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Prepares image for dumping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Emb_ManDumpGnuplotPrepare( Emb_Man_t * p )
{
//    int nRows = 496;
//    int nCols = 710;
    int nRows = 500;
    int nCols = 700;
    Vec_Int_t * vLines;
    Emb_Obj_t * pThis;
    char * pBuffer, ** ppRows;
    int i, k, placeX, placeY;
    int fStart;
    // alloc memory
    pBuffer = ABC_CALLOC( char, nRows * (nCols+1) );
    ppRows  = ABC_ALLOC( char *, nRows );
    for ( i = 0; i < nRows; i++ )
        ppRows[i] = pBuffer + i*(nCols+1);
    // put data into them
    Emb_ManForEachObj( p, pThis, i )
    {
        placeX = p->pPlacement[2*pThis->Value+0] * nCols / (1<<16);
        placeY = p->pPlacement[2*pThis->Value+1] * nRows / (1<<16);
        assert( placeX < nCols && placeY < nRows );
        ppRows[placeY][placeX] = 1;
    }
    // select lines
    vLines = Vec_IntAlloc( 1000 );
    for ( i = 0; i < nRows; i++ )
    {
        fStart = 0;
        for ( k = 0; k <= nCols; k++ )
        {
            if ( ppRows[i][k] && !fStart )
            {
                Vec_IntPush( vLines, k );
                Vec_IntPush( vLines, i );
                fStart = 1;
            }
            if ( !ppRows[i][k] && fStart )
            {
                Vec_IntPush( vLines, k-1 );
                Vec_IntPush( vLines, i );
                fStart = 0;
            }
        }
        assert( fStart == 0 );
    }
    ABC_FREE( pBuffer );
    ABC_FREE( ppRows );
    return vLines;
}

/**Function*************************************************************

  Synopsis    [Derives solutions from original vectors and eigenvectors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Emb_ManDumpGnuplot( Emb_Man_t * p, char * pName, int fDumpLarge, int fShowImage )
{
    extern void Gia_ManGnuplotShow( char * pPlotFileName );
//    char * pDirectory = "place\\";
    char * pDirectory = "";
//    extern char * Ioa_TimeStamp();
    FILE * pFile;
    char Buffer[1000];
    Emb_Obj_t * pThis, * pNext;
    int i, k;
    if ( p->pPlacement == NULL )
    {
        printf( "Emb_ManDumpGnuplot(): Placement is not available.\n" );
        return;
    }
    sprintf( Buffer, "%s%s", pDirectory, Gia_FileNameGenericAppend(pName, ".plt") ); 
    pFile = fopen( Buffer, "w" );
    fprintf( pFile, "# This Gnuplot file was produced by ABC on %s\n", Ioa_TimeStamp() );
    fprintf( pFile, "\n" );
    fprintf( pFile, "set nokey\n" );
    fprintf( pFile, "\n" );
    if ( !fShowImage )
    {
//    fprintf( pFile, "set terminal postscript\n" );
    fprintf( pFile, "set terminal gif font \'arial\' 10 size 800,600 xffffff x000000 x000000 x000000\n" );
    fprintf( pFile, "set output \'%s\'\n", Gia_FileNameGenericAppend(pName, ".gif") );
    fprintf( pFile, "\n" );
    }
    fprintf( pFile, "set title \"%s :  PI = %d   PO = %d   FF = %d   Node = %d   Obj = %d  HPWL = %.2e\\n", 
        pName, Emb_ManPiNum(p), Emb_ManPoNum(p), Emb_ManRegNum(p), Emb_ManNodeNum(p), Emb_ManObjNum(p), Emb_ManComputeHPWL(p) );
    fprintf( pFile, "(image generated by ABC and Gnuplot on %s)\"", Ioa_TimeStamp() );
    fprintf( pFile, "font \"Times, 12\"\n" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "plot [:] '-' w l\n" );
    fprintf( pFile, "\n" );
    if ( fDumpLarge )
    {
        int begX, begY, endX, endY;
        Vec_Int_t * vLines = Emb_ManDumpGnuplotPrepare( p );
        Vec_IntForEachEntry( vLines, begX, i )
        {
            begY = Vec_IntEntry( vLines, i+1 );
            endX = Vec_IntEntry( vLines, i+2 );
            endY = Vec_IntEntry( vLines, i+3 );
            i += 3;
            fprintf( pFile, "%5d %5d\n", begX, begY );
            fprintf( pFile, "%5d %5d\n", endX, endY );
            fprintf( pFile, "\n" );
        }
        Vec_IntFree( vLines );
    }
    else
    {
        Emb_ManForEachObj( p, pThis, i )
        {
            if ( !Emb_ObjIsTravIdCurrent(p, pThis) )
                continue;
            Emb_ObjForEachFanout( pThis, pNext, k )
            {
                assert( Emb_ObjIsTravIdCurrent(p, pNext) );
                fprintf( pFile, "%5d %5d\n", p->pPlacement[2*pThis->Value+0], p->pPlacement[2*pThis->Value+1] );
                fprintf( pFile, "%5d %5d\n", p->pPlacement[2*pNext->Value+0], p->pPlacement[2*pNext->Value+1] );
                fprintf( pFile, "\n" );
            }
        }
    }
    fprintf( pFile, "EOF\n" );
    fprintf( pFile, "\n" );
    if ( fShowImage )
    {
    fprintf( pFile, "pause -1 \"Close window\"\n" );  // Hit return to continue
    fprintf( pFile, "reset\n" );
    fprintf( pFile, "\n" );
    }
    else
    {
    fprintf( pFile, "# pause -1 \"Close window\"\n" );  // Hit return to continue
    fprintf( pFile, "# reset\n" );
    fprintf( pFile, "\n" );
    }
    fclose( pFile );
    if ( fShowImage )
        Gia_ManGnuplotShow( Buffer );
}

/**Function*************************************************************

  Synopsis    [Computes dimentions of the graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSolveProblem( Gia_Man_t * pGia, Emb_Par_t * pPars )
{
    Emb_Man_t * p;
    int i;
    abctime clkSetup;
    abctime clk;
//   Gia_ManTestDistance( pGia );

    // transform AIG into internal data-structure
clk = Abc_Clock();
    if ( pPars->fCluster )
    {
        p = Emb_ManStart( pGia );
        if ( pPars->fVerbose )
        {
            printf( "Clustered: " );
            Emb_ManPrintStats( p );
        }
    }
    else
        p = Emb_ManStartSimple( pGia );
    p->fVerbose = pPars->fVerbose;
//    Emb_ManPrintFanio( p );

    // prepare data-structure
    Gia_ManRandom( 1 );  // reset random numbers for deterministic behavior
    Emb_ManResetTravId( p );
    Emb_ManSetValue( p );
clkSetup = Abc_Clock() - clk;

clk = Abc_Clock();
    Emb_ManComputeDimensions( p, pPars->nDims );
if ( pPars->fVerbose )
ABC_PRT( "Setup     ", clkSetup );
if ( pPars->fVerbose )
ABC_PRT( "Dimensions", Abc_Clock() - clk );

clk = Abc_Clock();
    Emb_ManComputeCovariance( p, pPars->nDims );
if ( pPars->fVerbose )
ABC_PRT( "Matrix    ", Abc_Clock() - clk );

clk = Abc_Clock();
    Emb_ManComputeEigenvectors( p, pPars->nDims, pPars->nSols );
    Emb_ManComputeSolutions( p, pPars->nDims, pPars->nSols );
    Emb_ManDerivePlacement( p, pPars->nSols );
if ( pPars->fVerbose )
ABC_PRT( "Eigenvecs ", Abc_Clock() - clk );

    if ( pPars->fRefine )
    {
clk = Abc_Clock();
    Emb_ManPlacementRefine( p, pPars->nIters, pPars->fVerbose );
if ( pPars->fVerbose )
ABC_PRT( "Refinement", Abc_Clock() - clk );
    }

    if ( (pPars->fDump || pPars->fDumpLarge) && pPars->nSols == 2 )
    {
clk = Abc_Clock();
        Emb_ManDumpGnuplot( p, pGia->pName, pPars->fDumpLarge, pPars->fShowImage );
if ( pPars->fVerbose )
ABC_PRT( "Image dump", Abc_Clock() - clk );
    }

    // transfer placement
    if ( Gia_ManObjNum(pGia) == p->nObjs )
    {
        // assuming normalized ordering of the AIG
        pGia->pPlacement = ABC_CALLOC( Gia_Plc_t, p->nObjs );
        for ( i = 0; i < p->nObjs; i++ ) 
        {
            pGia->pPlacement[i].xCoord = p->pPlacement[2*i+0];
            pGia->pPlacement[i].yCoord = p->pPlacement[2*i+1];
        }
    }
    Emb_ManStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

