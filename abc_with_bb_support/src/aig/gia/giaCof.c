/**CFile****************************************************************

  FileName    [giaCof.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Cofactor estimation procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaCof.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <math.h>
#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Cof_Fan_t_ Cof_Fan_t;
struct Cof_Fan_t_
{
    unsigned       iFan     : 31;    // ID of the fanin/fanout
    unsigned       fCompl   :  1;    // complemented attribute
};

typedef struct Cof_Obj_t_ Cof_Obj_t;
struct Cof_Obj_t_
{
    unsigned       fTerm    :  1;    // terminal node (CI/CO)
    unsigned       fPhase   :  1;    // value under 000 pattern
    unsigned       fMark0   :  1;    // first user-controlled mark
    unsigned       fMark1   :  1;    // second user-controlled mark
    unsigned       nFanins  :  4;    // the number of fanins
    unsigned       nFanouts : 24;    // total number of fanouts
    unsigned       nFanoutsM;        // total number of MUX ctrl fanouts
    unsigned       Value;            // application specific data
    int            Id;               // ID of the node
    int            iNext;            // next one in the linked list
    int            iLit;             // literal of the node after rehashing
    Cof_Fan_t      Fanios[0];        // the array of fanins/fanouts
};

typedef struct Cof_Man_t_ Cof_Man_t;
struct Cof_Man_t_
{
    Gia_Man_t *    pGia;             // the original AIG manager
    Vec_Int_t *    vCis;             // the vector of CIs (PIs + LOs)
    Vec_Int_t *    vCos;             // the vector of COs (POs + LIs)
    int            nObjs;            // the number of objects
    int            nNodes;           // the number of nodes
    int            nTravIds;         // traversal ID of the network
    int *          pObjData;         // the logic network defined for the AIG
    int            nObjData;         // the size of array to store the logic network
    int *          pLevels;          // the linked lists of levels
    int            nLevels;          // the max number of logic levels
};

static inline unsigned    Gia_ObjHandle( Gia_Obj_t * pObj )                           { return pObj->Value;                                 } 

static inline int         Cof_ObjLevel( Cof_Man_t * p, Cof_Obj_t * pObj )             { return Gia_ObjLevel(p->pGia, Gia_ManObj(p->pGia,pObj->Id)); } 

static inline unsigned    Cof_ObjHandle( Cof_Man_t * p, Cof_Obj_t * pObj )            { return (unsigned)(((int *)pObj) - p->pObjData);     } 
static inline unsigned    Cof_ObjHandleDiff( Cof_Obj_t * pObj, Cof_Obj_t * pFanin )   { return (unsigned)(((int *)pObj) - ((int *)pFanin)); } 

static inline int         Cof_ObjIsTerm( Cof_Obj_t * pObj )                           { return pObj->fTerm;                                 } 
static inline int         Cof_ObjIsCi( Cof_Obj_t * pObj )                             { return pObj->fTerm && pObj->nFanins == 0;           } 
static inline int         Cof_ObjIsCo( Cof_Obj_t * pObj )                             { return pObj->fTerm && pObj->nFanins == 1;           } 
static inline int         Cof_ObjIsNode( Cof_Obj_t * pObj )                           { return!pObj->fTerm && pObj->nFanins > 0;            } 
static inline int         Cof_ObjIsConst0( Cof_Obj_t * pObj )                         { return!pObj->fTerm && pObj->nFanins == 0;           } 

static inline int         Cof_ObjFaninNum( Cof_Obj_t * pObj )                         { return pObj->nFanins;                               } 
static inline int         Cof_ObjFanoutNum( Cof_Obj_t * pObj )                        { return pObj->nFanouts;                              } 
static inline int         Cof_ObjSize( Cof_Obj_t * pObj )                             { return sizeof(Cof_Obj_t) / 4 + pObj->nFanins + pObj->nFanouts;  } 

static inline Cof_Obj_t * Cof_ManObj( Cof_Man_t * p, unsigned iHandle )               { return (Cof_Obj_t *)(p->pObjData + iHandle);        } 
static inline Cof_Obj_t * Cof_ObjFanin( Cof_Obj_t * pObj, int i )                     { return (Cof_Obj_t *)(((int *)pObj) - pObj->Fanios[i].iFan);               } 
static inline Cof_Obj_t * Cof_ObjFanout( Cof_Obj_t * pObj, int i )                    { return (Cof_Obj_t *)(((int *)pObj) + pObj->Fanios[pObj->nFanins+i].iFan); } 

static inline int         Cof_ManObjNum( Cof_Man_t * p )                              { return p->nObjs;                                    } 
static inline int         Cof_ManNodeNum( Cof_Man_t * p )                             { return p->nNodes;                                   } 

static inline void        Cof_ManResetTravId( Cof_Man_t * p )                         { extern void Cof_ManCleanValue( Cof_Man_t * p ); Cof_ManCleanValue( p ); p->nTravIds = 1;  }
static inline void        Cof_ManIncrementTravId( Cof_Man_t * p )                     { p->nTravIds++;                                      }
static inline void        Cof_ObjSetTravId( Cof_Obj_t * pObj, int TravId )            { pObj->Value = TravId;                               }
static inline void        Cof_ObjSetTravIdCurrent( Cof_Man_t * p, Cof_Obj_t * pObj )  { pObj->Value = p->nTravIds;                          }
static inline void        Cof_ObjSetTravIdPrevious( Cof_Man_t * p, Cof_Obj_t * pObj ) { pObj->Value = p->nTravIds - 1;                      }
static inline int         Cof_ObjIsTravIdCurrent( Cof_Man_t * p, Cof_Obj_t * pObj )   { return ((int)pObj->Value == p->nTravIds);           }
static inline int         Cof_ObjIsTravIdPrevious( Cof_Man_t * p, Cof_Obj_t * pObj )  { return ((int)pObj->Value == p->nTravIds - 1);       }

#define Cof_ManForEachObj( p, pObj, i )               \
    for ( i = 0; (i < p->nObjData) && (pObj = Cof_ManObj(p,i)); i += Cof_ObjSize(pObj) )
#define Cof_ManForEachNode( p, pObj, i )              \
    for ( i = 0; (i < p->nObjData) && (pObj = Cof_ManObj(p,i)); i += Cof_ObjSize(pObj) ) if ( Cof_ObjIsTerm(pObj) ) {} else
#define Cof_ObjForEachFanin( pObj, pNext, i )         \
    for ( i = 0; (i < (int)pObj->nFanins) && (pNext = Cof_ObjFanin(pObj,i)); i++ )
#define Cof_ObjForEachFanout( pObj, pNext, i )        \
    for ( i = 0; (i < (int)pObj->nFanouts) && (pNext = Cof_ObjFanout(pObj,i)); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates logic network isomorphic to the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cof_Man_t * Cof_ManCreateLogicSimple( Gia_Man_t * pGia )
{
    Cof_Man_t * p;
    Cof_Obj_t * pObjLog, * pFanLog;
    Gia_Obj_t * pObj;
    int * pMuxRefs;
    int i, iHandle = 0;
    p = ABC_CALLOC( Cof_Man_t, 1 );
    p->pGia = pGia;
    p->vCis = Vec_IntAlloc( Gia_ManCiNum(pGia) );
    p->vCos = Vec_IntAlloc( Gia_ManCoNum(pGia) );
    p->nObjData = (sizeof(Cof_Obj_t) / 4) * Gia_ManObjNum(pGia) + 4 * Gia_ManAndNum(pGia) + 2 * Gia_ManCoNum(pGia);
    p->pObjData = ABC_CALLOC( int, p->nObjData );
    ABC_FREE( pGia->pRefs );
    Gia_ManCreateRefs( pGia );
    Gia_ManForEachObj( pGia, pObj, i )
    {
        pObj->Value = iHandle;
        pObjLog = Cof_ManObj( p, iHandle );
        pObjLog->nFanins  = 0;
        pObjLog->nFanouts = Gia_ObjRefNum( pGia, pObj );
        pObjLog->Id       = i;
        pObjLog->Value    = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            pFanLog = Cof_ManObj( p, Gia_ObjHandle(Gia_ObjFanin0(pObj)) ); 
            pFanLog->Fanios[pFanLog->nFanins + pFanLog->Value++].iFan = 
                pObjLog->Fanios[pObjLog->nFanins].iFan = Cof_ObjHandleDiff( pObjLog, pFanLog );
            pObjLog->Fanios[pObjLog->nFanins++].fCompl = Gia_ObjFaninC0(pObj);

            pFanLog = Cof_ManObj( p, Gia_ObjHandle(Gia_ObjFanin1(pObj)) ); 
            pFanLog->Fanios[pFanLog->nFanins + pFanLog->Value++].iFan = 
                pObjLog->Fanios[pObjLog->nFanins].iFan = Cof_ObjHandleDiff( pObjLog, pFanLog );
            pObjLog->Fanios[pObjLog->nFanins++].fCompl = Gia_ObjFaninC1(pObj);
            p->nNodes++;
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            pFanLog = Cof_ManObj( p, Gia_ObjHandle(Gia_ObjFanin0(pObj)) ); 
            pFanLog->Fanios[pFanLog->nFanins + pFanLog->Value++].iFan = 
                pObjLog->Fanios[pObjLog->nFanins].iFan = Cof_ObjHandleDiff( pObjLog, pFanLog );
            pObjLog->Fanios[pObjLog->nFanins++].fCompl = Gia_ObjFaninC0(pObj);

            pObjLog->fTerm = 1;
            Vec_IntPush( p->vCos, iHandle );
        }
        else if ( Gia_ObjIsCi(pObj) )
        {
            pObjLog->fTerm = 1;
            Vec_IntPush( p->vCis, iHandle );
        }
        iHandle += Cof_ObjSize( pObjLog );
        p->nObjs++;
    }
    assert( iHandle == p->nObjData );
    pMuxRefs = Gia_ManCreateMuxRefs( pGia );
    Gia_ManForEachObj( pGia, pObj, i )
    {
        pObjLog = Cof_ManObj( p, Gia_ObjHandle(pObj) );
        assert( pObjLog->nFanouts == pObjLog->Value );
        pObjLog->nFanoutsM = pMuxRefs[i];
    }
    ABC_FREE( pMuxRefs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates logic network isomorphic to the given AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cof_ManStop( Cof_Man_t * p )
{
    Vec_IntFree( p->vCis );
    Vec_IntFree( p->vCos );
    ABC_FREE( p->pObjData );
    ABC_FREE( p->pLevels );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cof_ManTfoSize_rec( Cof_Man_t * p, Cof_Obj_t * pObj )
{
    Cof_Obj_t * pNext;
    unsigned i, Counter = 0;
    if ( Cof_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Cof_ObjSetTravIdCurrent(p, pObj);
    if ( Cof_ObjIsCo(pObj) )
        return 0;
    assert( Cof_ObjIsCi(pObj) || Cof_ObjIsNode(pObj) );
    Cof_ObjForEachFanout( pObj, pNext, i )
        Counter += Cof_ManTfoSize_rec( p, pNext );
    return 1 + Counter;
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cof_ManTfoSize( Cof_Man_t * p, Cof_Obj_t ** ppObjs, int nObjs )
{
    int i, Counter = 0; 
    Cof_ManIncrementTravId( p );
    for ( i = 0; i < nObjs; i++ )
        Counter += Cof_ManTfoSize_rec( p, ppObjs[i] ) - 1;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cof_ManTfiSize_rec( Cof_Man_t * p, Cof_Obj_t * pObj )
{
    Cof_Obj_t * pNext;
    unsigned i, Counter = 0;
    if ( Cof_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Cof_ObjSetTravIdCurrent(p, pObj);
    if ( Cof_ObjIsCi(pObj) )
        return 0;
    assert( Cof_ObjIsNode(pObj) );
    Cof_ObjForEachFanin( pObj, pNext, i )
        Counter += Cof_ManTfiSize_rec( p, pNext );
    return 1 + Counter;
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cof_ManTfiSize( Cof_Man_t * p, Cof_Obj_t ** ppObjs, int nObjs )
{
    int i, Counter = 0; 
    Cof_ManIncrementTravId( p );
    for ( i = 0; i < nObjs; i++ )
        if ( Cof_ObjIsCo(ppObjs[i]) )
            Counter += Cof_ManTfiSize_rec( p, Cof_ObjFanin(ppObjs[i],0) );
        else
            Counter += Cof_ManTfiSize_rec( p, ppObjs[i] );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cof_ManSuppSize_rec( Cof_Man_t * p, Cof_Obj_t * pObj )
{
    Cof_Obj_t * pNext;
    unsigned i, Counter = 0;
    if ( Cof_ObjIsTravIdCurrent(p, pObj) )
        return 0;
    Cof_ObjSetTravIdCurrent(p, pObj);
    if ( Cof_ObjIsCi(pObj) )
        return 1;
    assert( Cof_ObjIsNode(pObj) );
    Cof_ObjForEachFanin( pObj, pNext, i )
        Counter += Cof_ManSuppSize_rec( p, pNext );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Collects support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cof_ManSuppSize( Cof_Man_t * p, Cof_Obj_t ** ppObjs, int nObjs )
{
    int i, Counter = 0; 
    Cof_ManIncrementTravId( p );
    for ( i = 0; i < nObjs; i++ )
        if ( Cof_ObjIsCo(ppObjs[i]) )
            Counter += Cof_ManSuppSize_rec( p, Cof_ObjFanin(ppObjs[i],0) );
        else
            Counter += Cof_ManSuppSize_rec( p, ppObjs[i] );
    return Counter;
}


/**Function*************************************************************

  Synopsis    [Cleans the value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cof_ManCleanValue( Cof_Man_t * p )  
{
    Cof_Obj_t * pObj;
    int i;
    Cof_ManForEachObj( p, pObj, i )
        pObj->Value = 0;
}

/**Function*************************************************************

  Synopsis    [Returns sorted array of node handles with largest fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cof_ManInsertEntry_rec( Vec_Ptr_t * vNodes, Cof_Obj_t * pNode, int nNodeMax )
{
    Cof_Obj_t * pLast;
    if ( Vec_PtrSize(vNodes) == 0 )
    {
        Vec_PtrPush(vNodes, pNode);
        return;
    }
    pLast = (Cof_Obj_t *)Vec_PtrPop(vNodes);
    if ( Cof_ObjFanoutNum(pLast) < Cof_ObjFanoutNum(pNode) )
    {
        Cof_ManInsertEntry_rec( vNodes, pNode, nNodeMax );
        if ( Vec_PtrSize(vNodes) < nNodeMax )
            Vec_PtrPush( vNodes, pLast );
    }
    else
    {
        Vec_PtrPush( vNodes, pLast );
        if ( Vec_PtrSize(vNodes) < nNodeMax )
            Vec_PtrPush( vNodes, pNode );
    }
}

/**Function*************************************************************

  Synopsis    [Returns sorted array of node handles with largest fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Cof_ManCollectHighFanout( Cof_Man_t * p, int nNodes )
{
    Vec_Ptr_t * vNodes;
    Cof_Obj_t * pObj;
    int i;
    vNodes = Vec_PtrAlloc( nNodes );
    Cof_ManForEachObj( p, pObj, i )
        if ( Cof_ObjIsCi(pObj) || Cof_ObjIsNode(pObj) )
            Cof_ManInsertEntry_rec( vNodes, pObj, nNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Returns sorted array of node handles with largest fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cof_ManCountRemoved( Cof_Man_t * p, Cof_Obj_t * pRoot, int fConst1 )
{
    Gia_Obj_t * pNextGia;
    Cof_Obj_t * pTemp, * pNext, * pFanin0, * pFanin1;
    int Counter = 0, LevelStart, LevelNext;
    int i, k, iHandle, iLit0, iLit1, iNextNew;
    // restart the trav ids
    Cof_ManIncrementTravId( p );
    Cof_ObjSetTravIdCurrent( p, pRoot );
    // add the node to the queue
    LevelStart = Cof_ObjLevel(p, pRoot);
    assert( p->pLevels[LevelStart] == 0 );
    pRoot->iNext = 0;
    p->pLevels[LevelStart] = Cof_ObjHandle( p, pRoot );
    // set the new literal
    pRoot->iLit = Abc_Var2Lit( 0, fConst1 );
    // process nodes in the levelized order
    for ( i = LevelStart; i < p->nLevels; i++ )
    {
        for ( iHandle = p->pLevels[i]; 
              iHandle && (pTemp = Cof_ManObj(p, iHandle)); 
              iHandle = pTemp->iNext )
        {
            assert( pTemp->Id != Abc_Lit2Var(pTemp->iLit) );
            Cof_ObjForEachFanout( pTemp, pNext, k )
            {
                if ( Cof_ObjIsCo(pNext) )
                    continue;
                if ( Cof_ObjIsTravIdCurrent(p, pNext) )
                    continue;
                pFanin0 = Cof_ObjFanin( pNext, 0 );
                pFanin1 = Cof_ObjFanin( pNext, 1 );
                assert( pFanin0 == pTemp || pFanin1 == pTemp );
                pNextGia = Gia_ManObj( p->pGia, pNext->Id );
                if ( Cof_ObjIsTravIdCurrent(p, pFanin0) )
                    iLit0 = Abc_LitNotCond( pFanin0->iLit, Gia_ObjFaninC0(pNextGia) );
                else
                    iLit0 = Gia_ObjFaninLit0( pNextGia, pNext->Id );
                if ( Cof_ObjIsTravIdCurrent(p, pFanin1) )
                    iLit1 = Abc_LitNotCond( pFanin1->iLit, Gia_ObjFaninC1(pNextGia) );
                else
                    iLit1 = Gia_ObjFaninLit1( pNextGia, pNext->Id );
                iNextNew = Gia_ManHashAndTry( p->pGia, iLit0, iLit1 );
                if ( iNextNew == -1 )
                    continue;
                Cof_ObjSetTravIdCurrent(p, pNext);
                // set the new literal
                pNext->iLit = iNextNew;
                // add it to be processed
                LevelNext = Cof_ObjLevel( p, pNext );
                assert( LevelNext > i && LevelNext < p->nLevels );
                pNext->iNext = p->pLevels[LevelNext];
                p->pLevels[LevelNext] = Cof_ObjHandle( p, pNext );
                Counter++;
            }
        }
        p->pLevels[i] = 0;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns sorted array of node handles with largest fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cof_ManPrintHighFanoutOne( Cof_Man_t * p, Cof_Obj_t * pObj )
{
    printf( "%7d : ",     pObj->Id );
    printf( "i/o/c =%2d %5d %5d  ",  Cof_ObjFaninNum(pObj), Cof_ObjFanoutNum(pObj), 2*pObj->nFanoutsM );
    printf( "l =%4d  ",   Cof_ObjLevel(p, pObj) );
    printf( "s =%5d  ",   Cof_ManSuppSize(p, &pObj, 1) );
    printf( "TFI =%7d  ", Cof_ManTfiSize(p, &pObj, 1) );
    printf( "TFO =%7d  ", Cof_ManTfoSize(p, &pObj, 1) );
    printf( "C0 =%6d  ",  Cof_ManCountRemoved(p, pObj, 0) );
    printf( "C1 =%6d",    Cof_ManCountRemoved(p, pObj, 1) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Returns sorted array of node handles with largest fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cof_ManPrintHighFanout( Cof_Man_t * p, int nNodes )
{
    Vec_Ptr_t * vNodes;
    Cof_Obj_t * pObj;
    int i;
    vNodes = Cof_ManCollectHighFanout( p, nNodes );
    Vec_PtrForEachEntry( Cof_Obj_t *, vNodes, pObj, i )
        Cof_ManPrintHighFanoutOne( p, pObj );
    Vec_PtrFree( vNodes );
}


/**Function*************************************************************

  Synopsis    [Compute MFFC size of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cof_NodeDeref_rec( Cof_Obj_t * pNode )
{
    if ( pNode->nFanins == 0 )
        return 0;
    if ( --pNode->nFanouts > 0 )
        return 0;
    return 1 + Cof_NodeDeref_rec( Cof_ObjFanin(pNode, 0) )
             + Cof_NodeDeref_rec( Cof_ObjFanin(pNode, 1) );
}
int Cof_NodeRef_rec( Cof_Obj_t * pNode )
{
    if ( pNode->nFanins == 0 )
        return 0;
    if ( pNode->nFanouts++ > 0 )
        return 0;
    return 1 + Cof_NodeRef_rec( Cof_ObjFanin(pNode, 0) )
             + Cof_NodeRef_rec( Cof_ObjFanin(pNode, 1) );
}
static inline int Cof_ObjMffcSize( Cof_Obj_t * pNode )
{
    int Count1, Count2, nFanout;
    nFanout = pNode->nFanouts;
    pNode->nFanouts = 1;
    Count1 = Cof_NodeDeref_rec( pNode );
    Count2 = Cof_NodeRef_rec( pNode );
    pNode->nFanouts = nFanout;
    assert( Count1 == Count2 );
    return Count1;
}

/**Function*************************************************************

  Synopsis    [Prints the distribution of fanins/fanouts in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cof_ManPrintFanio( Cof_Man_t * p )
{
    char Buffer[100];
    Cof_Obj_t * pNode;
    Vec_Int_t * vFanins, * vFanouts, * vMffcs;
    int nFanins, nFanouts, nMffcs, nFaninsMax, nFanoutsMax, nMffcsMax, nFaninsAll, nFanoutsAll, nMffcsAll;
    int i, k, nSizeMax, nMffcNodes = 0;

    // determine the largest fanin and fanout
    nFaninsMax = nFanoutsMax = nMffcsMax = 0;
    nFaninsAll = nFanoutsAll = nMffcsAll = 0;
    Cof_ManForEachNode( p, pNode, i )
    {
        if ( i == 0 ) continue;
        nFanins  = Cof_ObjFaninNum(pNode);
        nFanouts = Cof_ObjFanoutNum(pNode);
        nMffcs   = pNode->nFanouts > 1 ? Cof_ObjMffcSize(pNode) : 0;
        nFaninsAll  += nFanins;
        nFanoutsAll += nFanouts;
        nMffcsAll   += nMffcs;
        nFaninsMax   = Abc_MaxInt( nFaninsMax,  nFanins );
        nFanoutsMax  = Abc_MaxInt( nFanoutsMax, nFanouts );
        nMffcsMax    = Abc_MaxInt( nMffcsMax,   nMffcs );
    }

    // allocate storage for fanin/fanout numbers
    nSizeMax = Abc_MaxInt( 10 * (Abc_Base10Log(nFaninsMax) + 1), 10 * (Abc_Base10Log(nFanoutsMax) + 1) );
    nSizeMax = Abc_MaxInt( 10 * (Abc_Base10Log(nMffcsMax) + 1),  nSizeMax );
    vFanins  = Vec_IntStart( nSizeMax );
    vFanouts = Vec_IntStart( nSizeMax );
    vMffcs   = Vec_IntStart( nSizeMax );

    // count the number of fanins and fanouts
    Cof_ManForEachNode( p, pNode, i )
    {
        if ( i == 0 ) continue;
        nFanins  = Cof_ObjFaninNum(pNode);
        nFanouts = Cof_ObjFanoutNum(pNode);
        nMffcs   = pNode->nFanouts > 1 ? Cof_ObjMffcSize(pNode) : 0;

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
       
        if ( nMffcs == 0 )
            continue;
        nMffcNodes++;

        if ( nMffcs < 10 )
            Vec_IntAddToEntry( vMffcs, nMffcs, 1 );
        else if ( nMffcs < 100 )
            Vec_IntAddToEntry( vMffcs, 10 + nMffcs/10, 1 );
        else if ( nMffcs < 1000 )
            Vec_IntAddToEntry( vMffcs, 20 + nMffcs/100, 1 );
        else if ( nMffcs < 10000 )
            Vec_IntAddToEntry( vMffcs, 30 + nMffcs/1000, 1 );
        else if ( nMffcs < 100000 )
            Vec_IntAddToEntry( vMffcs, 40 + nMffcs/10000, 1 );
        else if ( nMffcs < 1000000 )
            Vec_IntAddToEntry( vMffcs, 50 + nMffcs/100000, 1 );
        else if ( nMffcs < 10000000 )
            Vec_IntAddToEntry( vMffcs, 60 + nMffcs/1000000, 1 );
    }

    printf( "The distribution of fanins, fanouts. and MFFCs in the network:\n" );
    printf( "         Number    Nodes with fanin   Nodes with fanout   Nodes with MFFC\n" );

    for ( k = 0; k < nSizeMax; k++ )
    {
        if ( vFanins->pArray[k] == 0 && vFanouts->pArray[k] == 0 && vMffcs->pArray[k] == 0 )
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
            printf( "%11d   ", vFanins->pArray[k] );
        printf( "    " );
        if ( vFanouts->pArray[k] == 0 )
            printf( "              " );
        else
            printf( "%12d  ", vFanouts->pArray[k] );
        printf( "    " );
        if ( vMffcs->pArray[k] == 0 )
            printf( "               " );
        else
            printf( "  %12d  ", vMffcs->pArray[k] );
        printf( "\n" );
    }
    Vec_IntFree( vFanins );
    Vec_IntFree( vFanouts );
    Vec_IntFree( vMffcs );

    printf( "Fanins: Max = %d. Ave = %.2f.  Fanouts: Max = %d. Ave =  %.2f.  MFFCs: Max = %d. Ave =  %.2f.\n", 
        nFaninsMax,  1.0*nFaninsAll /Cof_ManNodeNum(p), 
        nFanoutsMax, 1.0*nFanoutsAll/Cof_ManNodeNum(p), 
        nMffcsMax,   1.0*nMffcsAll  /nMffcNodes  );
}

/**Function*************************************************************

  Synopsis    [Returns sorted array of node handles with largest fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintFanio( Gia_Man_t * pGia, int nNodes )
{
    Cof_Man_t * p;
    abctime clk = Abc_Clock();
    p = Cof_ManCreateLogicSimple( pGia );
    p->nLevels = 1 + Gia_ManLevelNum( pGia );
    p->pLevels = ABC_CALLOC( int, p->nLevels );
    Cof_ManPrintFanio( p );

    if ( nNodes > 0 )
    {
    Cof_ManResetTravId( p );
    Gia_ManHashStart( pGia );
    Cof_ManPrintHighFanout( p, nNodes );
    Gia_ManHashStop( pGia );
ABC_PRMn( "Memory for logic network", 4*p->nObjData );
ABC_PRT( "Time", Abc_Clock() - clk );
    }

    Cof_ManStop( p );
}


/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order while putting CIs first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupCofInt( Gia_Man_t * p, int iVar )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj, * pPivot;
    int i, iCofVar = -1;
    if ( !(iVar > 0 && iVar < Gia_ManObjNum(p)) )
    {
        printf( "Gia_ManDupCof(): Variable %d is out of range (%d; %d).\n", iVar, 0, Gia_ManObjNum(p) );
        return NULL;
    }
    // find the cofactoring variable
    pPivot = Gia_ManObj( p, iVar );
    if ( !Gia_ObjIsCand(pPivot) )
    {
        printf( "Gia_ManDupCof(): Variable %d should be a CI or an AND node.\n", iVar );
        return NULL;
    }
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    // compute negative cofactor
    Gia_ManForEachCi( p, pObj, i )
    {
        pObj->Value = Gia_ManAppendCi(pNew);
        if ( pObj == pPivot )
        {
            iCofVar = pObj->Value;
            pObj->Value = Abc_Var2Lit( 0, 0 );
        }
    }
    Gia_ManForEachAnd( p, pObj, i )
    {
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( pObj == pPivot )
        {
            iCofVar = pObj->Value;
            pObj->Value = Abc_Var2Lit( 0, 0 );
        }
    }
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);
    // compute the positive cofactor
    Gia_ManForEachCi( p, pObj, i )
    {
        pObj->Value = Abc_Var2Lit( Gia_ObjId(pNew, Gia_ManCi(pNew, i)), 0 );
        if ( pObj == pPivot )
            pObj->Value = Abc_Var2Lit( 0, 1 );
    }
    Gia_ManForEachAnd( p, pObj, i )
    {
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( pObj == pPivot )
            pObj->Value = Abc_Var2Lit( 0, 1 );
    }
    // create MUXes
    assert( iCofVar > 0 );
    Gia_ManForEachCo( p, pObj, i )
    {
        if ( pObj->Value == (unsigned)Gia_ObjFanin0Copy(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        else
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ManHashMux(pNew, iCofVar, Gia_ObjFanin0Copy(pObj), pObj->Value) );
    }
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates AIG in the DFS order while putting CIs first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupCof( Gia_Man_t * p, int iVar )
{
    Gia_Man_t * pNew, * pTemp;
    pNew = Gia_ManDupCofInt( p, iVar );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Determines variables whose fanout count is higher than this.]

  Description [Variables are returned in a reverse topological order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManCofVars( Gia_Man_t * p, int nFanLim )
{
    Vec_Int_t * vVars;
    Gia_Obj_t * pObj;
    int i;
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p );
    vVars = Vec_IntAlloc( 100 );
    Gia_ManForEachObj( p, pObj, i )
        if ( Gia_ObjIsCand(pObj) && Gia_ObjRefNum(p, pObj) >= nFanLim )
            Vec_IntPush( vVars, i );
    ABC_FREE( p->pRefs );
    return vVars;
}

/**Function*************************************************************

  Synopsis    [Transfers attributes from the original one to the final one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManTransfer( Gia_Man_t * pAig, Gia_Man_t * pCof, Gia_Man_t * pNew, Vec_Int_t * vSigs )
{
    Vec_Int_t * vSigsNew;
    Gia_Obj_t * pObj, * pObjF;
    int i;
    vSigsNew = Vec_IntAlloc( 100 );
    Gia_ManForEachObjVec( vSigs, pAig, pObj, i )
    {
        assert( Gia_ObjIsCand(pObj) );
        pObjF = Gia_ManObj( pCof, Abc_Lit2Var(pObj->Value) );
        if ( pObjF->Value && ~pObjF->Value )
            Vec_IntPushUnique( vSigsNew, Abc_Lit2Var(pObjF->Value) );
    }
    return vSigsNew;    
}

/**Function*************************************************************

  Synopsis    [Cofactors selected variables (should be in reverse topo order).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupCofAllInt( Gia_Man_t * p, Vec_Int_t * vSigs, int fVerbose )
{
    Vec_Int_t * vSigsNew, * vTemp;
    Gia_Man_t * pAig, * pCof, * pNew;
    int iVar;
    if ( fVerbose )
    { 
        printf( "Cofactoring %d signals.\n", Vec_IntSize(vSigs) );
        Gia_ManPrintStats( p, NULL );
    }
    if ( Vec_IntSize( vSigs ) > 200 )
    {
        printf( "Too many signals to cofactor.\n" );
        return NULL;
    }
    pAig = Gia_ManDup( p );
    vSigsNew = Vec_IntDup( vSigs );
    while ( Vec_IntSize(vSigsNew) > 0 )
    {
        Vec_IntSort( vSigsNew, 0 );
        iVar = Vec_IntPop( vSigsNew );
//        Gia_ManCreateRefs( pAig );
//        printf( "ref count = %d\n", Gia_ObjRefNum( pAig, Gia_ManObj(pAig, iVar) ) );
//        ABC_FREE( pAig->pRefs );
        pCof = Gia_ManDupCofInt( pAig, iVar );
        pNew = Gia_ManCleanup( pCof );
        vSigsNew = Gia_ManTransfer( pAig, pCof, pNew, vTemp = vSigsNew );
        Vec_IntFree( vTemp );
        Gia_ManStop( pAig );
        Gia_ManStop( pCof );
        pAig = pNew;
        if ( fVerbose )
            printf( "Cofactored variable %d.\n", iVar );
        if ( fVerbose )
            Gia_ManPrintStats( pAig, NULL );
    }
    Vec_IntFree( vSigsNew );
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Cofactors all variables whose fanout is higher than this.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupCofAll( Gia_Man_t * p, int nFanLim, int fVerbose )
{
    Gia_Man_t * pNew;
    Vec_Int_t * vSigs = Gia_ManCofVars( p, nFanLim );
    pNew = Gia_ManDupCofAllInt( p, vSigs, fVerbose );
    Vec_IntFree( vSigs );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

