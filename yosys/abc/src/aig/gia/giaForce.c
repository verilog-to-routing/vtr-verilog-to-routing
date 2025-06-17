/**CFile****************************************************************

  FileName    [gia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


/* 
    The code is based on the paper by F. A. Aloul, I. L. Markov, and K. A. Sakallah.
    "FORCE: A Fast and Easy-To-Implement Variable-Ordering Heuristic", Proc. GLSVLSI�03.
    http://www.eecs.umich.edu/~imarkov/pubs/conf/glsvlsi03-force.pdf
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Frc_Obj_t_ Frc_Obj_t;
struct Frc_Obj_t_
{
    unsigned       fCi      :  1;    // terminal node CI
    unsigned       fCo      :  1;    // terminal node CO
    unsigned       fMark0   :  1;    // first user-controlled mark
    unsigned       fMark1   :  1;    // second user-controlled mark
    unsigned       nFanins  : 28;    // the number of fanins
    unsigned       nFanouts;         // the number of fanouts
    unsigned       iFanout;          // the current number of fanouts
    int            hHandle;          // the handle of the node
    int            pPlace;           // the placement of each node
    union {
    float          fEdgeCenter;      // center-of-gravity of the edge
    unsigned       iFanin; 
    };
    int            Fanios[0];        // the array of fanins/fanouts
};

typedef struct Frc_Man_t_ Frc_Man_t;
struct Frc_Man_t_
{
    Gia_Man_t *    pGia;             // the original AIG manager
    Vec_Int_t *    vCis;             // the vector of CIs (PIs + LOs)
    Vec_Int_t *    vCos;             // the vector of COs (POs + LIs)
    int            nObjs;            // the number of objects
    int            nRegs;            // the number of registers
    int *          pObjData;         // the array containing data for objects
    int            nObjData;         // the size of array to store the logic network
    int            fVerbose;         // verbose output flag
    int            nCutCur;          // current cut 
    int            nCutMax;          // max cut seen
};

static inline int         Frc_ManRegNum( Frc_Man_t * p )              { return p->nRegs;                                                     }
static inline int         Frc_ManCiNum( Frc_Man_t * p )               { return Vec_IntSize(p->vCis);                                         }
static inline int         Frc_ManCoNum( Frc_Man_t * p )               { return Vec_IntSize(p->vCos);                                         }
static inline int         Frc_ManPiNum( Frc_Man_t * p )               { return Vec_IntSize(p->vCis) - p->nRegs;                              }
static inline int         Frc_ManPoNum( Frc_Man_t * p )               { return Vec_IntSize(p->vCos) - p->nRegs;                              }
static inline int         Frc_ManObjNum( Frc_Man_t * p )              { return p->nObjs;                                                     } 
static inline int         Frc_ManNodeNum( Frc_Man_t * p )             { return p->nObjs - Vec_IntSize(p->vCis) - Vec_IntSize(p->vCos);       } 

static inline Frc_Obj_t * Frc_ManObj( Frc_Man_t * p, int hHandle )    { return (Frc_Obj_t *)(p->pObjData + hHandle);                         } 
static inline Frc_Obj_t * Frc_ManCi( Frc_Man_t * p, int i )           { return Frc_ManObj( p, Vec_IntEntry(p->vCis,i) );                     }
static inline Frc_Obj_t * Frc_ManCo( Frc_Man_t * p, int i )           { return Frc_ManObj( p, Vec_IntEntry(p->vCos,i) );                     }

static inline int         Frc_ObjIsTerm( Frc_Obj_t * pObj )           { return pObj->fCi || pObj->fCo;                                       } 
static inline int         Frc_ObjIsCi( Frc_Obj_t * pObj )             { return pObj->fCi;                                                    } 
static inline int         Frc_ObjIsCo( Frc_Obj_t * pObj )             { return pObj->fCo;                                                    } 
static inline int         Frc_ObjIsPi( Frc_Obj_t * pObj )             { return pObj->fCi && pObj->nFanins == 0;                              } 
static inline int         Frc_ObjIsPo( Frc_Obj_t * pObj )             { return pObj->fCo && pObj->nFanouts == 0;                             } 
static inline int         Frc_ObjIsNode( Frc_Obj_t * pObj )           { return!Frc_ObjIsTerm(pObj) && pObj->nFanins > 0;                     } 
static inline int         Frc_ObjIsConst0( Frc_Obj_t * pObj )         { return!Frc_ObjIsTerm(pObj) && pObj->nFanins == 0;                    } 

static inline int         Frc_ObjSize( Frc_Obj_t * pObj )             { return sizeof(Frc_Obj_t) / 4 + pObj->nFanins + pObj->nFanouts;       } 
static inline int         Frc_ObjFaninNum( Frc_Obj_t * pObj )         { return pObj->nFanins;                                                } 
static inline int         Frc_ObjFanoutNum( Frc_Obj_t * pObj )        { return pObj->nFanouts;                                               } 
static inline Frc_Obj_t * Frc_ObjFanin( Frc_Obj_t * pObj, int i )     { return (Frc_Obj_t *)(((int *)pObj) - pObj->Fanios[i]);               } 
static inline Frc_Obj_t * Frc_ObjFanout( Frc_Obj_t * pObj, int i )    { return (Frc_Obj_t *)(((int *)pObj) + pObj->Fanios[pObj->nFanins+i]); } 

#define Frc_ManForEachObj( p, pObj, i )               \
    for ( i = 0; (i < p->nObjData) && (pObj = Frc_ManObj(p,i)); i += Frc_ObjSize(pObj) )
#define Frc_ManForEachObjVec( vVec, p, pObj, i )                        \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pObj) = Frc_ManObj(p, Vec_IntEntry(vVec,i))); i++ )

#define Frc_ManForEachNode( p, pObj, i )              \
    for ( i = 0; (i < p->nObjData) && (pObj = Frc_ManObj(p,i)); i += Frc_ObjSize(pObj) ) if ( Frc_ObjIsTerm(pObj) ) {} else
#define Frc_ManForEachCi( p, pObj, i )                  \
    for ( i = 0; (i < Vec_IntSize(p->vCis)) && (pObj = Frc_ManObj(p,Vec_IntEntry(p->vCis,i))); i++ )
#define Frc_ManForEachCo( p, pObj, i )                  \
    for ( i = 0; (i < Vec_IntSize(p->vCos)) && (pObj = Frc_ManObj(p,Vec_IntEntry(p->vCos,i))); i++ )

#define Frc_ObjForEachFanin( pObj, pNext, i )         \
    for ( i = 0; (i < (int)pObj->nFanins) && (pNext = Frc_ObjFanin(pObj,i)); i++ )
#define Frc_ObjForEachFaninReverse( pObj, pNext, i )  \
    for ( i = (int)pObj->nFanins - 1; (i >= 0) && (pNext = Frc_ObjFanin(pObj,i)); i-- )
#define Frc_ObjForEachFanout( pObj, pNext, i )        \
    for ( i = 0; (i < (int)pObj->nFanouts) && (pNext = Frc_ObjFanout(pObj,i)); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Creates fanin/fanout pair.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Frc_ObjAddFanin( Frc_Obj_t * pObj, Frc_Obj_t * pFanin )
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
Frc_Man_t * Frc_ManStartSimple( Gia_Man_t * pGia )
{
    Frc_Man_t * p;
    Frc_Obj_t * pObjLog, * pFanLog;
    Gia_Obj_t * pObj;//, * pObjRi, * pObjRo;
    int i, nNodes, hHandle = 0;
    // prepare the AIG
    Gia_ManCreateRefs( pGia );
    // create logic network
    p = ABC_CALLOC( Frc_Man_t, 1 );
    p->pGia  = pGia;
    p->nRegs = Gia_ManRegNum(pGia);
    p->vCis  = Vec_IntAlloc( Gia_ManCiNum(pGia) );
    p->vCos  = Vec_IntAlloc( Gia_ManCoNum(pGia) );
    p->nObjData = (sizeof(Frc_Obj_t) / 4) * Gia_ManObjNum(pGia) + 2 * (2 * Gia_ManAndNum(pGia) + Gia_ManCoNum(pGia));
    p->pObjData = ABC_CALLOC( int, p->nObjData );
    // create constant node
    Gia_ManConst0(pGia)->Value = hHandle;
    pObjLog = Frc_ManObj( p, hHandle );
    pObjLog->hHandle  = hHandle;
    pObjLog->nFanins  = 0;
    pObjLog->nFanouts = Gia_ObjRefNum( pGia, Gia_ManConst0(pGia) );
    // count objects
    hHandle += Frc_ObjSize( pObjLog );
    nNodes = 1;
    p->nObjs++;
    // create the PIs
    Gia_ManForEachCi( pGia, pObj, i )
    {
        // create PI object
        pObj->Value = hHandle;
        Vec_IntPush( p->vCis, hHandle );
        pObjLog = Frc_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = 0;
        pObjLog->nFanouts = Gia_ObjRefNum( pGia, pObj );
        pObjLog->fCi = 0;
        // count objects
        hHandle += Frc_ObjSize( pObjLog );
        p->nObjs++;
    }
    // create internal nodes
    Gia_ManForEachAnd( pGia, pObj, i )
    {
        assert( Gia_ObjRefNum( pGia, pObj ) > 0 );
        // create node object
        pObj->Value = hHandle;
        pObjLog = Frc_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = 2;
        pObjLog->nFanouts = Gia_ObjRefNum( pGia, pObj );
        // add fanins
        pFanLog = Frc_ManObj( p, Gia_ObjValue(Gia_ObjFanin0(pObj)) ); 
        Frc_ObjAddFanin( pObjLog, pFanLog );
        pFanLog = Frc_ManObj( p, Gia_ObjValue(Gia_ObjFanin1(pObj)) ); 
        Frc_ObjAddFanin( pObjLog, pFanLog );
        // count objects
        hHandle += Frc_ObjSize( pObjLog );
        nNodes++;
        p->nObjs++;
    }
    // create the POs
    Gia_ManForEachCo( pGia, pObj, i )
    {
        // create PO object
        pObj->Value = hHandle;
        Vec_IntPush( p->vCos, hHandle );
        pObjLog = Frc_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = 1;
        pObjLog->nFanouts = 0;
        pObjLog->fCo = 1;
        // add fanins
        pFanLog = Frc_ManObj( p, Gia_ObjValue(Gia_ObjFanin0(pObj)) );
        Frc_ObjAddFanin( pObjLog, pFanLog );
        // count objects
        hHandle += Frc_ObjSize( pObjLog );
        p->nObjs++;
    }
    // connect registers
//    Gia_ManForEachRiRo( pGia, pObjRi, pObjRo, i )
//        Frc_ObjAddFanin( Frc_ManObj(p,Gia_ObjValue(pObjRo)), Frc_ManObj(p,Gia_ObjValue(pObjRi)) );
    assert( nNodes  == Frc_ManNodeNum(p) );
    assert( hHandle == p->nObjData );
    if ( hHandle != p->nObjData )
        printf( "Frc_ManStartSimple(): Fatal error in internal representation.\n" );
    // make sure the fanin/fanout counters are correct
    Gia_ManForEachObj( pGia, pObj, i )
    {
        if ( !~Gia_ObjValue(pObj) )
            continue;
        pObjLog = Frc_ManObj( p, Gia_ObjValue(pObj) );
        assert( pObjLog->nFanins  == pObjLog->iFanin );
        assert( pObjLog->nFanouts == pObjLog->iFanout );
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
void Frc_ManCollectSuper_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSuper, Vec_Int_t * vVisit )  
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
    Frc_ManCollectSuper_rec( p, Gia_ObjFanin0(pObj), vSuper, vVisit );
    Frc_ManCollectSuper_rec( p, Gia_ObjFanin1(pObj), vSuper, vVisit );
    
}

/**Function*************************************************************

  Synopsis    [Collect the fanin IDs.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
void Frc_ManCollectSuper( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSuper, Vec_Int_t * vVisit )  
{
    int Entry, i;
    Vec_IntClear( vSuper );
    Vec_IntClear( vVisit );
    assert( pObj->fMark0 == 1 );
    pObj->fMark0 = 0;
    Frc_ManCollectSuper_rec( p, pObj, vSuper, vVisit );
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
void Frc_ManCreateRefsSpecial( Gia_Man_t * p )  
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
void Frc_ManTransformRefs( Gia_Man_t * p, int * pnObjs, int * pnFanios )  
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
        Frc_ManCollectSuper( p, pObj, vSuper, vVisit );
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

  Synopsis    [Creates logic network isomorphic to the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Frc_Man_t * Frc_ManStart( Gia_Man_t * pGia )
{
    Frc_Man_t * p;
    Frc_Obj_t * pObjLog, * pFanLog;
    Gia_Obj_t * pObj, * pFanin;//, * pObjRi, * pObjRo;
    Vec_Int_t * vSuper, * vVisit;
    int nObjs, nFanios, nNodes = 0;
    int i, k, hHandle = 0;
    // prepare the AIG
//    Gia_ManCreateRefs( pGia );
    Frc_ManCreateRefsSpecial( pGia );
    Frc_ManTransformRefs( pGia, &nObjs, &nFanios );
    Gia_ManFillValue( pGia );
    // create logic network
    p = ABC_CALLOC( Frc_Man_t, 1 );
    p->pGia  = pGia;
    p->nRegs = Gia_ManRegNum(pGia);
    p->vCis  = Vec_IntAlloc( Gia_ManCiNum(pGia) );
    p->vCos  = Vec_IntAlloc( Gia_ManCoNum(pGia) );
    p->nObjData = (sizeof(Frc_Obj_t) / 4) * nObjs + 2 * nFanios;
    p->pObjData = ABC_CALLOC( int, p->nObjData );
    // create constant node
    Gia_ManConst0(pGia)->Value = hHandle;
    pObjLog = Frc_ManObj( p, hHandle );
    pObjLog->hHandle  = hHandle;
    pObjLog->nFanins  = 0;
    pObjLog->nFanouts = Gia_ObjRefNum( pGia, Gia_ManConst0(pGia) );
    // count objects
    hHandle += Frc_ObjSize( pObjLog );
    nNodes++;
    p->nObjs++;
    // create the PIs
    Gia_ManForEachCi( pGia, pObj, i )
    {
        // create PI object
        pObj->Value = hHandle;
        Vec_IntPush( p->vCis, hHandle );
        pObjLog = Frc_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = 0;
        pObjLog->nFanouts = Gia_ObjRefNum( pGia, pObj );
        pObjLog->fCi = 1;
        // count objects
        hHandle += Frc_ObjSize( pObjLog );
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
        Frc_ManCollectSuper( pGia, pObj, vSuper, vVisit );
        // create node object
        pObj->Value = hHandle;
        pObjLog = Frc_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = Vec_IntSize( vSuper );
        pObjLog->nFanouts = Gia_ObjRefNum( pGia, pObj );
        // add fanins
        Gia_ManForEachObjVec( vSuper, pGia, pFanin, k )
        {
            pFanLog = Frc_ManObj( p, Gia_ObjValue(pFanin) ); 
            Frc_ObjAddFanin( pObjLog, pFanLog );
        }
        // count objects
        hHandle += Frc_ObjSize( pObjLog );
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
        pObjLog = Frc_ManObj( p, hHandle );
        pObjLog->hHandle  = hHandle;
        pObjLog->nFanins  = 1;
        pObjLog->nFanouts = 0;
        pObjLog->fCo = 1;
        // add fanins
        pFanLog = Frc_ManObj( p, Gia_ObjValue(Gia_ObjFanin0(pObj)) );
        Frc_ObjAddFanin( pObjLog, pFanLog );
        // count objects
        hHandle += Frc_ObjSize( pObjLog );
        p->nObjs++;
    }
    // connect registers
//    Gia_ManForEachRiRo( pGia, pObjRi, pObjRo, i )
//        Frc_ObjAddFanin( Frc_ManObj(p,Gia_ObjValue(pObjRo)), Frc_ManObj(p,Gia_ObjValue(pObjRi)) );
    Gia_ManCleanMark0( pGia );
    assert( nNodes  == Frc_ManNodeNum(p) );
    assert( nObjs   == p->nObjs );
    assert( hHandle == p->nObjData );
    if ( hHandle != p->nObjData )
        printf( "Frc_ManStart(): Fatal error in internal representation.\n" );
    // make sure the fanin/fanout counters are correct
    Gia_ManForEachObj( pGia, pObj, i )
    {
        if ( !~Gia_ObjValue(pObj) )
            continue;
        pObjLog = Frc_ManObj( p, Gia_ObjValue(pObj) );
        assert( pObjLog->nFanins  == pObjLog->iFanin );
        assert( pObjLog->nFanouts == pObjLog->iFanout );
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
void Frc_ManPrintStats( Frc_Man_t * p )
{
//    if ( p->pName )
//        printf( "%8s : ", p->pName );
    printf( "i/o =%7d/%7d  ", Frc_ManPiNum(p), Frc_ManPoNum(p) );
    if ( Frc_ManRegNum(p) )
        printf( "ff =%7d  ", Frc_ManRegNum(p) );
    printf( "node =%8d  ", Frc_ManNodeNum(p) );
    printf( "obj =%8d  ", Frc_ManObjNum(p) );
//    printf( "lev =%5d  ", Frc_ManLevelNum(p) );
//    printf( "cut =%5d  ", Frc_ManCrossCut(p) );
    printf( "mem =%5.2f MB", 4.0*p->nObjData/(1<<20) );
//    printf( "obj =%5d  ", Frc_ManObjNum(p) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Creates logic network isomorphic to the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Frc_ManStop( Frc_Man_t * p )
{
    Vec_IntFree( p->vCis );
    Vec_IntFree( p->vCos );
    ABC_FREE( p->pObjData );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Computes cross cut size for the given order of POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Frc_ManCrossCut_rec( Frc_Man_t * p, Frc_Obj_t * pObj )
{
    assert( pObj->iFanout > 0 );
    if ( pObj->iFanout-- == pObj->nFanouts )
    {
        Frc_Obj_t * pFanin;
        int i;
        p->nCutCur++;
        p->nCutMax = Abc_MaxInt( p->nCutMax, p->nCutCur );
        if ( !Frc_ObjIsCi(pObj) )
            Frc_ObjForEachFanin( pObj, pFanin, i )
                p->nCutCur -= Frc_ManCrossCut_rec( p, pFanin );
    }
    return pObj->iFanout == 0;
}

/**Function*************************************************************

  Synopsis    [Computes cross cut size for the given order of POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Frc_ManCrossCut2_rec( Frc_Man_t * p, Frc_Obj_t * pObj )
{
    assert( pObj->iFanout > 0 );
    if ( pObj->iFanout-- == pObj->nFanouts )
    {
        Frc_Obj_t * pFanin;
        int i;
        p->nCutCur++;
        p->nCutMax = Abc_MaxInt( p->nCutMax, p->nCutCur );
        if ( !Frc_ObjIsCi(pObj) )
            Frc_ObjForEachFaninReverse( pObj, pFanin, i )
                p->nCutCur -= Frc_ManCrossCut2_rec( p, pFanin );
    }
    return pObj->iFanout == 0;
}

/**Function*************************************************************

  Synopsis    [Computes cross cut size for the given order of POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Frc_ManCrossCut( Frc_Man_t * p, Vec_Int_t * vOrder, int fReverse )
{
    Frc_Obj_t * pObj;
    int i;
    assert( Vec_IntSize(vOrder) == Frc_ManCoNum(p) );
    p->nCutCur = 0;
    p->nCutMax = 0;
    Frc_ManForEachObj( p, pObj, i )
        pObj->iFanout = pObj->nFanouts;
    Frc_ManForEachObjVec( vOrder, p, pObj, i )
    {
        assert( Frc_ObjIsCo(pObj) );
        if ( fReverse )
            p->nCutCur -= Frc_ManCrossCut2_rec( p, Frc_ObjFanin(pObj,0) );
        else
            p->nCutCur -= Frc_ManCrossCut_rec( p, Frc_ObjFanin(pObj,0) );
    }
    assert( p->nCutCur == 0 );
//    Frc_ManForEachObj( p, pObj, i )
//        assert( pObj->iFanout == 0 );
    return p->nCutMax;
}

/**Function*************************************************************

  Synopsis    [Collects CO handles.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Frc_ManCollectCos( Frc_Man_t * p )
{
    Vec_Int_t * vCoOrder;
    Frc_Obj_t * pObj;
    int i;
    vCoOrder = Vec_IntAlloc( Frc_ManCoNum(p) );
    Frc_ManForEachCo( p, pObj, i )
        Vec_IntPush( vCoOrder, pObj->hHandle );
    return vCoOrder;
}

/**Function*************************************************************

  Synopsis    [Computes cross cut size for the given order of POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Frc_ManCrossCutTest( Frc_Man_t * p, Vec_Int_t * vOrderInit )
{
    Vec_Int_t * vOrder;
//    abctime clk = Abc_Clock();
    vOrder = vOrderInit? vOrderInit : Frc_ManCollectCos( p );
    printf( "CrossCut = %6d\n", Frc_ManCrossCut( p, vOrder, 0 ) );
    printf( "CrossCut = %6d\n", Frc_ManCrossCut( p, vOrder, 1 ) );
    Vec_IntReverseOrder( vOrder );
    printf( "CrossCut = %6d\n", Frc_ManCrossCut( p, vOrder, 0 ) );
    printf( "CrossCut = %6d\n", Frc_ManCrossCut( p, vOrder, 1 ) );
    Vec_IntReverseOrder( vOrder );
    if ( vOrder != vOrderInit )
        Vec_IntFree( vOrder );
//    ABC_PRT( "Time", Abc_Clock() - clk );
}



/**Function*************************************************************

  Synopsis    [Generates random placement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Frc_ManPlaceRandom( Frc_Man_t * p )
{
    Frc_Obj_t * pThis;
    int * pPlacement;
    int i, h, Temp, iNext, Counter;
    pPlacement = ABC_ALLOC( int, p->nObjs );
    for ( i = 0; i < p->nObjs; i++ )
        pPlacement[i] = i;
    for ( i = 0; i < p->nObjs; i++ )
    {
        iNext = Gia_ManRandom( 0 ) % p->nObjs;
        Temp = pPlacement[i];
        pPlacement[i] = pPlacement[iNext];
        pPlacement[iNext] = Temp;
    }
    Counter = 0;
    Frc_ManForEachObj( p, pThis, h )
        pThis->pPlace = pPlacement[Counter++];
    ABC_FREE( pPlacement );
}

/**Function*************************************************************

  Synopsis    [Shuffles array of random integers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Frc_ManArrayShuffle( Vec_Int_t * vArray )
{
    int i, iNext, Temp;
    for ( i = 0; i < vArray->nSize; i++ )
    {
        iNext = Gia_ManRandom( 0 ) % vArray->nSize;
        Temp = vArray->pArray[i];
        vArray->pArray[i] = vArray->pArray[iNext];
        vArray->pArray[iNext] = Temp;
    }
}

/**Function*************************************************************

  Synopsis    [Computes cross cut size for the given order of POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Frc_ManPlaceDfs_rec( Frc_Man_t * p, Frc_Obj_t * pObj, int * piPlace )
{
    assert( pObj->iFanout > 0 );
    if ( pObj->iFanout-- == pObj->nFanouts )
    {
        Frc_Obj_t * pFanin;
        int i;
        if ( !Frc_ObjIsCi(pObj) )
            Frc_ObjForEachFanin( pObj, pFanin, i )
                Frc_ManPlaceDfs_rec( p, pFanin, piPlace );
        pObj->pPlace = (*piPlace)++;
    }
}

/**Function*************************************************************

  Synopsis    [Generates DFS placement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Frc_ManPlaceDfs( Frc_Man_t * p, Vec_Int_t * vCoOrder )
{
    Frc_Obj_t * pObj;
    int i, nPlaces = 0;
    Frc_ManForEachObj( p, pObj, i )
    {
        pObj->iFanout = pObj->nFanouts;
        if ( pObj->nFanouts == 0 && !Frc_ObjIsCo(pObj) )
            pObj->pPlace = nPlaces++;
    }
    Frc_ManForEachObjVec( vCoOrder, p, pObj, i )
    {
        assert( Frc_ObjIsCo(pObj) );
        Frc_ManPlaceDfs_rec( p, Frc_ObjFanin(pObj,0), &nPlaces );
        pObj->pPlace = nPlaces++;
    }
    assert( nPlaces == p->nObjs );
}

/**Function*************************************************************

  Synopsis    [Generates DFS placement by trying both orders.]

  Description [Returns the cross cut size of the best order. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Frc_ManPlaceDfsBoth( Frc_Man_t * p, Vec_Int_t * vCoOrder, int * piCutSize2 )
{
    int nCutStart1, nCutStart2;
    nCutStart1 = Frc_ManCrossCut( p, vCoOrder, 0 );
    Vec_IntReverseOrder( vCoOrder );
    nCutStart2 = Frc_ManCrossCut( p, vCoOrder, 0 );
    if ( nCutStart1 <= nCutStart2 )
    {
        Vec_IntReverseOrder( vCoOrder ); // undo
        Frc_ManPlaceDfs( p, vCoOrder );
        *piCutSize2 = nCutStart2;
        return nCutStart1;
    }
    else
    {
        Frc_ManPlaceDfs( p, vCoOrder );
        Vec_IntReverseOrder( vCoOrder ); // undo
        *piCutSize2 = nCutStart1;
        return nCutStart2;
    }
}

/**Function*************************************************************

  Synopsis    [Performs iterative refinement of the given placement.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Frc_ManPlacementRefine( Frc_Man_t * p, int nIters, int fVerbose )
{
    int fRandomize = 0;
    Vec_Int_t * vCoOrder;
    Frc_Obj_t * pThis, * pNext;
    double CostThis, CostPrev;
    float * pVertX, VertX;
    int * pPermX, * pHandles;
    int k, h, Iter, iMinX, iMaxX, Counter, nCutStart, nCutCur, nCutCur2, nCutPrev;
    abctime clk = Abc_Clock(), clk2, clk2Total = 0;
    // create starting one-dimensional placement
    vCoOrder = Frc_ManCollectCos( p );
    if ( fRandomize )
        Frc_ManArrayShuffle( vCoOrder );
    nCutStart = Frc_ManPlaceDfsBoth( p, vCoOrder, &nCutCur2 );
    // refine placement
    CostPrev = 0.0;
    nCutPrev = nCutStart;
    pHandles = ABC_ALLOC( int, p->nObjs );
    pVertX   = ABC_ALLOC( float, p->nObjs );
    for ( Iter = 0; Iter < nIters; Iter++ )
    {
        // compute centers of hyperedges
        CostThis = 0.0;
        Frc_ManForEachObj( p, pThis, h )
        {
            iMinX = iMaxX = pThis->pPlace;
            Frc_ObjForEachFanout( pThis, pNext, k )
            {
                iMinX = Abc_MinInt( iMinX, pNext->pPlace );
                iMaxX = Abc_MaxInt( iMaxX, pNext->pPlace );
            }
            pThis->fEdgeCenter = 0.5 * (iMaxX + iMinX);
            CostThis += (iMaxX - iMinX);
        }
        // compute new centers of objects
        Counter = 0;
        Frc_ManForEachObj( p, pThis, h )
        {
            VertX = pThis->fEdgeCenter;
            Frc_ObjForEachFanin( pThis, pNext, k )
                VertX += pNext->fEdgeCenter;
            pVertX[Counter] = VertX / (Frc_ObjFaninNum(pThis) + 1);
            pHandles[Counter++] = h;
        }
        assert( Counter == Frc_ManObjNum(p) );
        // sort these numbers
        clk2 = Abc_Clock();
        pPermX = Gia_SortFloats( pVertX, pHandles, p->nObjs );
        clk2Total += Abc_Clock() - clk2;
        assert( pPermX == pHandles );
        Vec_IntClear( vCoOrder );
        for ( k = 0; k < p->nObjs; k++ )
        {
            pThis = Frc_ManObj( p, pPermX[k] );
            pThis->pPlace = k;
            if ( Frc_ObjIsCo(pThis) )
                Vec_IntPush( vCoOrder, pThis->hHandle );
        }
/*
        printf( "Ordering of PIs:\n" );
        Frc_ManForEachCi( p, pThis, k )
            printf( "PI number = %7d.  Object handle = %7d,  Coordinate = %7d.\n", 
                k, pThis->hHandle, pThis->pPlace );
*/
        nCutCur = Frc_ManPlaceDfsBoth( p, vCoOrder, &nCutCur2 );
        // evaluate cost
        if ( fVerbose )
        {
            printf( "%2d : Span = %e  ", Iter+1, CostThis );
            printf( "Cut = %6d  (%5.2f %%)  CutR = %6d  ", nCutCur, 100.0*(nCutStart-nCutCur)/nCutStart, nCutCur2 );
            ABC_PRTn( "Total", Abc_Clock() - clk );
            ABC_PRT( "Sort", clk2Total );
//        Frc_ManCrossCutTest( p, vCoOrder );
        }
//        if ( 1.0 * nCutPrev / nCutCur < 1.001 )
//            break;
        nCutPrev = nCutCur;
    }
    ABC_FREE( pHandles );
    ABC_FREE( pVertX );
    Vec_IntFree( vCoOrder );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if all fanouts are COsw.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Frc_ObjFanoutsAreCos( Frc_Obj_t * pThis )
{
    Frc_Obj_t * pNext;
    int i;
    Frc_ObjForEachFanout( pThis, pNext, i )
        if ( !Frc_ObjIsCo(pNext) )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes the distances from the given set of objects.]

  Description [Returns one of the most distant objects.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Frc_DumpGraphIntoFile( Frc_Man_t * p )
{
    FILE * pFile;
    Frc_Obj_t * pThis, * pNext;
    int i, k, Counter = 0;
    // assign numbers to CIs and internal nodes
    Frc_ManForEachObj( p, pThis, i )
    {
        if ( i && ((Frc_ObjIsCi(pThis) && !Frc_ObjFanoutsAreCos(pThis)) || Frc_ObjIsNode(pThis)) )
            pThis->iFanin = Counter++;
        else
            pThis->iFanin = ~0;
    }
    // assign numbers to all other nodes
    pFile = fopen( "x\\large\\aig\\dg1.g", "w" );
    Frc_ManForEachObj( p, pThis, i )
    {
        Frc_ObjForEachFanout( pThis, pNext, k )
        {
            if ( ~pThis->iFanin && ~pNext->iFanin )
                fprintf( pFile, "%d %d\n", pThis->iFanin, pNext->iFanin );
        }
    }
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Experiment with the FORCE algorithm.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void For_ManExperiment( Gia_Man_t * pGia, int nIters, int fClustered, int fVerbose )
{
    Frc_Man_t * p;
    Gia_ManRandom( 1 );
    if ( fClustered )
        p = Frc_ManStart( pGia );
    else
        p = Frc_ManStartSimple( pGia );
//    Frc_DumpGraphIntoFile( p );
    if ( fVerbose )
        Frc_ManPrintStats( p );
//    Frc_ManCrossCutTest( p, NULL );
    Frc_ManPlacementRefine( p, nIters, fVerbose );
    Frc_ManStop( p );
}

/**Function*************************************************************

  Synopsis    [Experiment with the FORCE algorithm.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void For_ManFileExperiment()
{
    FILE * pFile;
    int * pBuffer;
    int i, Size, Exp = 25;
    abctime clk = Abc_Clock();
    int RetValue;

    Size = (1 << Exp);
    printf( "2^%d machine words (%d bytes).\n", Exp, (int)sizeof(int) * Size );

    pBuffer = ABC_ALLOC( int, Size );
    for ( i = 0; i < Size; i++ )
        pBuffer[i] = i;
ABC_PRT( "Fillup", Abc_Clock() - clk );

clk = Abc_Clock();
    pFile = fopen( "test.txt", "rb" );
    RetValue = fread( pBuffer, 1, sizeof(int) * Size, pFile );
    fclose( pFile );
ABC_PRT( "Read  ", Abc_Clock() - clk );

clk = Abc_Clock();
    pFile = fopen( "test.txt", "wb" );
    fwrite( pBuffer, 1, sizeof(int) * Size, pFile );
    fclose( pFile );
ABC_PRT( "Write ", Abc_Clock() - clk );
/*
2^25 machine words (134217728 bytes).
Fillup =    0.06 sec
Read   =    0.08 sec
Write  =    1.81 sec
*/
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

