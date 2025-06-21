/**CFile****************************************************************

  FileName    [aigRet.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Retiming of AIGs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigRet.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// init values
typedef enum { 
    RTM_VAL_NONE,                    // 0: non-existent value
    RTM_VAL_ZERO,                    // 1: initial value 0
    RTM_VAL_ONE,                     // 2: initial value 1
    RTM_VAL_VOID                     // 3: unused value
} Rtm_Init_t;

typedef struct Rtm_Man_t_    Rtm_Man_t;
struct Rtm_Man_t_
{
    // network representation
    Vec_Ptr_t *      vObjs;          // retiming objects
    Vec_Ptr_t *      vPis;           // PIs only
    Vec_Ptr_t *      vPos;           // POs only
    Aig_MmFlex_t *   pMem;           // the memory manager
    // autonomous components after cutting off
    // storage for overflow latches
    unsigned *       pExtra;   
    int              nExtraCur;
    int              nExtraAlloc;
};

typedef struct Rtm_Edg_t_    Rtm_Edg_t;
struct Rtm_Edg_t_
{
    unsigned long    nLats   :  12;  // the number of latches
    unsigned long    LData   :  20;  // the latches themselves
};

typedef struct Rtm_Obj_t_    Rtm_Obj_t;
struct Rtm_Obj_t_
{
    void *           pCopy;          // the copy of this object
    unsigned long    Type    :  3;   // object type
    unsigned long    fMark   :  1;   // multipurpose mark
    unsigned long    fAuto   :  1;   // this object belongs to an autonomous component
    unsigned long    fCompl0 :  1;   // complemented attribute of the first edge
    unsigned long    fCompl1 :  1;   // complemented attribute of the second edge
    unsigned long    nFanins :  8;   // the number of fanins
    unsigned         Num     : 17;   // the retiming number of this node
    int              Id;             // ID of this object
    int              Temp;           // temporary usage
    int              nFanouts;       // the number of fanouts
    void *           pFanio[0];      // fanins and their edges (followed by fanouts and pointers to their edges)
};

static inline Rtm_Obj_t * Rtm_ObjFanin( Rtm_Obj_t * pObj, int i )        { return (Rtm_Obj_t *)pObj->pFanio[2*i];                     }
static inline Rtm_Obj_t * Rtm_ObjFanout( Rtm_Obj_t * pObj, int i )       { return (Rtm_Obj_t *)pObj->pFanio[2*(pObj->nFanins+i)];     }
static inline Rtm_Edg_t * Rtm_ObjEdge( Rtm_Obj_t * pObj, int i )         { return (Rtm_Edg_t *)(pObj->pFanio + 2*i + 1);              }
static inline Rtm_Edg_t * Rtm_ObjFanoutEdge( Rtm_Obj_t * pObj, int i )   { return (Rtm_Edg_t *)pObj->pFanio[2*(pObj->nFanins+i) + 1]; }

static inline Rtm_Init_t  Rtm_InitNot( Rtm_Init_t Val )                  { if ( Val == RTM_VAL_ZERO ) return (Rtm_Init_t)RTM_VAL_ONE; if ( Val == RTM_VAL_ONE ) return (Rtm_Init_t)RTM_VAL_ZERO; assert( 0 ); return (Rtm_Init_t)-1; }
static inline Rtm_Init_t  Rtm_InitNotCond( Rtm_Init_t Val, int c )       { return c ? Rtm_InitNot(Val) : Val;                         }
static inline Rtm_Init_t  Rtm_InitAnd(Rtm_Init_t ValA, Rtm_Init_t ValB ) { if ( ValA == RTM_VAL_ONE && ValB == RTM_VAL_ONE ) return (Rtm_Init_t)RTM_VAL_ONE;  if ( ValA == RTM_VAL_ZERO || ValB == RTM_VAL_ZERO ) return (Rtm_Init_t)RTM_VAL_ZERO; assert( 0 ); return (Rtm_Init_t)-1;   }

static inline int         Rtm_InitWordsNum( int nLats )                  { return (nLats >> 4) + ((nLats & 15) > 0); }
static inline int         Rtm_InitGetTwo( unsigned * p, int i )          { return (p[i>>4] >> ((i & 15)<<1)) & 3;    }
static inline void        Rtm_InitSetTwo( unsigned * p, int i, int val ) { p[i>>4] |= (val << ((i & 15)<<1));        }
static inline void        Rtm_InitXorTwo( unsigned * p, int i, int val ) { p[i>>4] ^= (val << ((i & 15)<<1));        }

static inline Rtm_Init_t  Rtm_ObjGetFirst1( Rtm_Edg_t * pEdge )          { return (Rtm_Init_t)(pEdge->LData & 3);                                    }
static inline Rtm_Init_t  Rtm_ObjGetLast1( Rtm_Edg_t * pEdge )           { return (Rtm_Init_t)((pEdge->LData >> ((pEdge->nLats-1)<<1)) & 3);         }
static inline Rtm_Init_t  Rtm_ObjGetOne1( Rtm_Edg_t * pEdge, int i )     { assert( i < (int)pEdge->nLats ); return (Rtm_Init_t)((pEdge->LData >> (i << 1)) & 3);  }
static inline Rtm_Init_t  Rtm_ObjRemFirst1( Rtm_Edg_t * pEdge )          { int Val = pEdge->LData & 3; pEdge->LData >>= 2; assert(pEdge->nLats > 0); pEdge->nLats--; return (Rtm_Init_t)Val;  }
static inline Rtm_Init_t  Rtm_ObjRemLast1( Rtm_Edg_t * pEdge )           { int Val = (pEdge->LData >> ((pEdge->nLats-1)<<1)) & 3; pEdge->LData ^= Val << ((pEdge->nLats-1)<<1); assert(pEdge->nLats > 0); pEdge->nLats--; return (Rtm_Init_t)Val;  }
static inline void        Rtm_ObjAddFirst1( Rtm_Edg_t * pEdge, Rtm_Init_t Val ) { assert( Val > 0 ); pEdge->LData = (pEdge->LData << 2) | Val;  pEdge->nLats++;   }
static inline void        Rtm_ObjAddLast1( Rtm_Edg_t * pEdge, Rtm_Init_t Val )  { assert( Val > 0 ); pEdge->LData |= Val << (pEdge->nLats<<1);  pEdge->nLats++;   }

static inline Rtm_Init_t  Rtm_ObjGetFirst2( Rtm_Man_t * p, Rtm_Edg_t * pEdge )                 { return (Rtm_Init_t)Rtm_InitGetTwo( p->pExtra + pEdge->LData, 0 );                }
static inline Rtm_Init_t  Rtm_ObjGetLast2( Rtm_Man_t * p, Rtm_Edg_t * pEdge )                  { return (Rtm_Init_t)Rtm_InitGetTwo( p->pExtra + pEdge->LData, pEdge->nLats - 1 ); }
static inline Rtm_Init_t  Rtm_ObjGetOne2( Rtm_Man_t * p, Rtm_Edg_t * pEdge, int i )            { return (Rtm_Init_t)Rtm_InitGetTwo( p->pExtra + pEdge->LData, i );                }
static        Rtm_Init_t  Rtm_ObjRemFirst2( Rtm_Man_t * p, Rtm_Edg_t * pEdge );
static inline Rtm_Init_t  Rtm_ObjRemLast2( Rtm_Man_t * p, Rtm_Edg_t * pEdge )                  { Rtm_Init_t Val = Rtm_ObjGetLast2( p, pEdge );    Rtm_InitXorTwo( p->pExtra + pEdge->LData, pEdge->nLats - 1, Val ); pEdge->nLats--; return (Rtm_Init_t)Val; }
static        void        Rtm_ObjAddFirst2( Rtm_Man_t * p, Rtm_Edg_t * pEdge, Rtm_Init_t Val );
static inline void        Rtm_ObjAddLast2( Rtm_Man_t * p, Rtm_Edg_t * pEdge, Rtm_Init_t Val )  { Rtm_InitSetTwo( p->pExtra + pEdge->LData, pEdge->nLats, Val );  pEdge->nLats++;  }

static        void        Rtm_ObjTransferToSmall( Rtm_Man_t * p, Rtm_Edg_t * pEdge );
static        void        Rtm_ObjTransferToBig( Rtm_Man_t * p, Rtm_Edg_t * pEdge ); 
static        void        Rtm_ObjTransferToBigger( Rtm_Man_t * p, Rtm_Edg_t * pEdge );

static inline Rtm_Init_t  Rtm_ObjGetFirst( Rtm_Man_t * p, Rtm_Edg_t * pEdge )                  { return pEdge->nLats > 10? Rtm_ObjGetFirst2(p, pEdge)  : Rtm_ObjGetFirst1(pEdge);   }
static inline Rtm_Init_t  Rtm_ObjGetLast( Rtm_Man_t * p, Rtm_Edg_t * pEdge )                   { return pEdge->nLats > 10? Rtm_ObjGetLast2(p, pEdge)   : Rtm_ObjGetLast1(pEdge);    }
static inline Rtm_Init_t  Rtm_ObjGetOne( Rtm_Man_t * p, Rtm_Edg_t * pEdge, int i )             { return pEdge->nLats > 10? Rtm_ObjGetOne2(p, pEdge, i) : Rtm_ObjGetOne1(pEdge, i);  }
static        Rtm_Init_t  Rtm_ObjRemFirst( Rtm_Man_t * p, Rtm_Edg_t * pEdge )                  { Rtm_Init_t Res = pEdge->nLats > 10 ? Rtm_ObjRemFirst2(p, pEdge) : Rtm_ObjRemFirst1(pEdge); if ( pEdge->nLats == 10 ) Rtm_ObjTransferToSmall(p, pEdge); return Res; }
static        Rtm_Init_t  Rtm_ObjRemLast( Rtm_Man_t * p, Rtm_Edg_t * pEdge )                   { Rtm_Init_t Res = pEdge->nLats > 10 ? Rtm_ObjRemLast2(p, pEdge)  : Rtm_ObjRemLast1(pEdge);  if ( pEdge->nLats == 10 ) Rtm_ObjTransferToSmall(p, pEdge); return Res; }
static        void        Rtm_ObjAddFirst( Rtm_Man_t * p, Rtm_Edg_t * pEdge, Rtm_Init_t Val )  { if ( pEdge->nLats == 10 ) Rtm_ObjTransferToBig(p, pEdge); else if ( (pEdge->nLats & 15) == 15 ) Rtm_ObjTransferToBigger(p, pEdge); if ( pEdge->nLats >= 10 ) Rtm_ObjAddFirst2(p, pEdge, Val); else Rtm_ObjAddFirst1(pEdge, Val); }
static        void        Rtm_ObjAddLast( Rtm_Man_t * p, Rtm_Edg_t * pEdge, Rtm_Init_t Val )   { if ( pEdge->nLats == 10 ) Rtm_ObjTransferToBig(p, pEdge); else if ( (pEdge->nLats & 15) == 15 ) Rtm_ObjTransferToBigger(p, pEdge); if ( pEdge->nLats >= 10 ) Rtm_ObjAddLast2(p, pEdge, Val);  else Rtm_ObjAddLast1(pEdge, Val);  }


// iterator over the primary inputs
#define Rtm_ManForEachPi( p, pObj, i )                                          \
    Vec_PtrForEachEntry( Rtm_Obj_t *, p->vPis, pObj, i )
// iterator over the primary outputs
#define Rtm_ManForEachPo( p, pObj, i )                                          \
    Vec_PtrForEachEntry( Rtm_Obj_t *, p->vPos, pObj, i )
// iterator over all objects, including those currently not used
#define Rtm_ManForEachObj( p, pObj, i )                                         \
    Vec_PtrForEachEntry( Rtm_Obj_t *, p->vObjs, pObj, i ) 
// iterate through the fanins
#define Rtm_ObjForEachFanin( pObj, pFanin, i )                                  \
    for ( i = 0; i < (int)(pObj)->nFanins && ((pFanin = Rtm_ObjFanin(pObj, i)), 1); i++ )
// iterate through the fanouts
#define Rtm_ObjForEachFanout( pObj, pFanout, i )                                \
    for ( i = 0; i < (int)(pObj)->nFanouts && ((pFanout = Rtm_ObjFanout(pObj, i)), 1); i++ )
// iterate through the fanin edges
#define Rtm_ObjForEachFaninEdge( pObj, pEdge, i )                               \
    for ( i = 0; i < (int)(pObj)->nFanins && ((pEdge = Rtm_ObjEdge(pObj, i)), 1); i++ )
// iterate through the fanout edges
#define Rtm_ObjForEachFanoutEdge( pObj, pEdge, i )                              \
    for ( i = 0; i < (int)(pObj)->nFanouts && ((pEdge = Rtm_ObjFanoutEdge(pObj, i)), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transfers from big to small storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_ObjTransferToSmall( Rtm_Man_t * p, Rtm_Edg_t * pEdge ) 
{
    assert( pEdge->nLats == 10 );
    pEdge->LData = p->pExtra[pEdge->LData];
}

/**Function*************************************************************

  Synopsis    [Transfers from small to big storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_ObjTransferToBig( Rtm_Man_t * p, Rtm_Edg_t * pEdge ) 
{
    assert( pEdge->nLats == 10 );
    if ( p->nExtraCur + 1 > p->nExtraAlloc )
    {
        int nExtraAllocNew = Abc_MaxInt( 2 * p->nExtraAlloc, 1024 );
        p->pExtra = ABC_REALLOC( unsigned, p->pExtra, nExtraAllocNew );
        p->nExtraAlloc = nExtraAllocNew;
    }
    p->pExtra[p->nExtraCur] = pEdge->LData;
    pEdge->LData = p->nExtraCur++;
}

/**Function*************************************************************

  Synopsis    [Transfers to bigger storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_ObjTransferToBigger( Rtm_Man_t * p, Rtm_Edg_t * pEdge ) 
{
    int nWords;
    assert( (pEdge->nLats & 15) == 15 );
    nWords = (pEdge->nLats + 1) >> 4;
    if ( p->nExtraCur + nWords + 1 > p->nExtraAlloc )
    {
        int nExtraAllocNew = Abc_MaxInt( 2 * p->nExtraAlloc, 1024 );
        p->pExtra = ABC_REALLOC( unsigned, p->pExtra, nExtraAllocNew );
        p->nExtraAlloc = nExtraAllocNew;
    }
    memcpy( p->pExtra + p->nExtraCur, p->pExtra + pEdge->LData, sizeof(unsigned) * nWords );
    p->pExtra[p->nExtraCur + nWords] = 0;
    pEdge->LData = p->nExtraCur;
    p->nExtraCur += nWords + 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rtm_Init_t  Rtm_ObjRemFirst2( Rtm_Man_t * p, Rtm_Edg_t * pEdge )                 
{ 
    Rtm_Init_t Val = (Rtm_Init_t)0, Temp;   
    unsigned * pB = p->pExtra + pEdge->LData, * pE = pB + Rtm_InitWordsNum( pEdge->nLats-- ) - 1;  
    while ( pE >= pB ) 
    {
        Temp = (Rtm_Init_t)(*pE & 3);
        *pE = (*pE >> 2) | (Val << 30);
        Val = Temp;
        pE--;
    }
    assert( Val != 0 );
    return Val;  
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_ObjAddFirst2( Rtm_Man_t * p, Rtm_Edg_t * pEdge, Rtm_Init_t Val ) 
{
    unsigned * pB = p->pExtra + pEdge->LData, * pE = pB + Rtm_InitWordsNum( ++pEdge->nLats ); 
    Rtm_Init_t Temp;
    assert( Val != 0 );
    while ( pB < pE ) 
    {
        Temp = (Rtm_Init_t)(*pB >> 30);
        *pB = (*pB << 2) | Val;
        Val = Temp;
        pB++;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_PrintEdge( Rtm_Man_t * p, Rtm_Edg_t * pEdge ) 
{
//    unsigned LData = pEdge->LData;
    printf( "%d : ", (int)pEdge->nLats );
/*
    if ( pEdge->nLats > 10 )
        Extra_PrintBinary( stdout, p->pExtra + pEdge->LData, 2*(pEdge->nLats+1) );
    else
        Extra_PrintBinary( stdout, &LData, 2*(pEdge->nLats+1) );
*/
    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    [Allocates the retiming manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rtm_Man_t * Rtm_ManAlloc( Aig_Man_t * p )
{
    Rtm_Man_t * pRtm;
    // start the manager
    pRtm = ABC_ALLOC( Rtm_Man_t, 1 );
    memset( pRtm, 0, sizeof(Rtm_Man_t) );
    // perform initializations
    pRtm->vObjs = Vec_PtrAlloc( Aig_ManObjNum(p) );
    pRtm->vPis  = Vec_PtrAlloc( Aig_ManCiNum(p) );
    pRtm->vPos  = Vec_PtrAlloc( Aig_ManCoNum(p) );
    pRtm->pMem  = Aig_MmFlexStart();
    return pRtm;
}

/**Function*************************************************************

  Synopsis    [Allocates the retiming manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_ManFree( Rtm_Man_t * p )
{
    Vec_PtrFree( p->vObjs );
    Vec_PtrFree( p->vPis );
    Vec_PtrFree( p->vPos );
    Aig_MmFlexStop( p->pMem, 0 );
    ABC_FREE( p->pExtra );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Counts the maximum number of latches on an edge.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rtm_ManLatchMax( Rtm_Man_t * p )
{
    Rtm_Obj_t * pObj;
    Rtm_Edg_t * pEdge;
    int nLatchMax = 0, i, k;//, c, Val;
    Rtm_ManForEachObj( p, pObj, i )
    Rtm_ObjForEachFaninEdge( pObj, pEdge, k )
    {
/*
        for ( c = 0; c < (int)pEdge->nLats; c++ )
        {
            Val = Rtm_ObjGetOne( p, pEdge, c );
            assert( Val == 1 || Val == 2 );
        }
*/
        nLatchMax = Abc_MaxInt( nLatchMax, (int)pEdge->nLats );
    }
    return nLatchMax;
}

/**Function*************************************************************

  Synopsis    [Allocates the retiming object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rtm_Obj_t * Rtm_ObjAlloc( Rtm_Man_t * pRtm, int nFanins, int nFanouts )
{
    Rtm_Obj_t * pObj;
    int Size = sizeof(Rtm_Obj_t) + sizeof(Rtm_Obj_t *) * (nFanins + nFanouts) * 2;
    pObj = (Rtm_Obj_t *)Aig_MmFlexEntryFetch( pRtm->pMem, Size );
    memset( pObj, 0, sizeof(Rtm_Obj_t) );
    pObj->Type = (int)(nFanins == 1 && nFanouts == 0); // mark PO
    pObj->Num  = nFanins;  // temporary
    pObj->Temp = nFanouts;
    pObj->Id = Vec_PtrSize(pRtm->vObjs);
    Vec_PtrPush( pRtm->vObjs, pObj );
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Allocates the retiming object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_ObjAddFanin( Rtm_Obj_t * pObj, Rtm_Obj_t * pFanin, int fCompl )
{
    pObj->pFanio[ 2*pObj->nFanins ] = pFanin;
    pObj->pFanio[ 2*pObj->nFanins + 1 ] = NULL;
    pFanin->pFanio[ 2*(pFanin->Num + pFanin->nFanouts) ] = pObj;
    pFanin->pFanio[ 2*(pFanin->Num + pFanin->nFanouts) + 1 ] = pObj->pFanio + 2*pObj->nFanins + 1;
    if ( pObj->nFanins == 0 )
        pObj->fCompl0 = fCompl;
    else if ( pObj->nFanins == 1 )
        pObj->fCompl1 = fCompl;
    else
        assert( 0 );
    pObj->nFanins++;
    pFanin->nFanouts++;
    assert( pObj->nFanins <= pObj->Num );
    assert( pFanin->nFanouts <= pFanin->Temp );
}

/**Function*************************************************************

  Synopsis    [Check the possibility of forward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rtm_ObjCheckRetimeFwd( Rtm_Obj_t * pObj )
{
    Rtm_Edg_t * pEdge;
    int i;
    Rtm_ObjForEachFaninEdge( pObj, pEdge, i )
        if ( pEdge->nLats == 0 )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Check the possibility of forward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rtm_ObjCheckRetimeBwd( Rtm_Obj_t * pObj )
{
    Rtm_Edg_t * pEdge;
    int i;
    Rtm_ObjForEachFanoutEdge( pObj, pEdge, i )
        if ( pEdge->nLats == 0 )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Check the possibility of forward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rtm_ObjGetDegreeFwd( Rtm_Obj_t * pObj )
{
    Rtm_Obj_t * pFanin;
    int i, Degree = 0;
    Rtm_ObjForEachFanin( pObj, pFanin, i )
        Degree = Abc_MaxInt( Degree, (int)pFanin->Num );
    return Degree + 1;
}

/**Function*************************************************************

  Synopsis    [Check the possibility of forward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rtm_ObjGetDegreeBwd( Rtm_Obj_t * pObj )
{
    Rtm_Obj_t * pFanout;
    int i, Degree = 0;
    Rtm_ObjForEachFanout( pObj, pFanout, i )
        Degree = Abc_MaxInt( Degree, (int)pFanout->Num );
    return Degree + 1;
}

/**Function*************************************************************

  Synopsis    [Performs forward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_ObjRetimeFwd( Rtm_Man_t * pRtm, Rtm_Obj_t * pObj )
{
    Rtm_Init_t ValTotal, ValCur;
    Rtm_Edg_t * pEdge;
    int i;
    assert( Rtm_ObjCheckRetimeFwd(pObj) );
    // extract values and compute the result
    ValTotal = RTM_VAL_ONE;
    Rtm_ObjForEachFaninEdge( pObj, pEdge, i )
    {
        ValCur = Rtm_ObjRemFirst( pRtm, pEdge );
        ValCur = Rtm_InitNotCond( ValCur, i? pObj->fCompl1 : pObj->fCompl0 );
        ValTotal = Rtm_InitAnd( ValTotal, ValCur );
    }
    // insert the result in the fanout values
    Rtm_ObjForEachFanoutEdge( pObj, pEdge, i )
        Rtm_ObjAddLast( pRtm, pEdge, ValTotal );
}

/**Function*************************************************************

  Synopsis    [Performs forward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_ObjRetimeBwd( Rtm_Man_t * pRtm, Rtm_Obj_t * pObj )
{
    Rtm_Edg_t * pEdge;
    int i;
    assert( Rtm_ObjCheckRetimeBwd(pObj) );
    // extract values and compute the result
    Rtm_ObjForEachFanoutEdge( pObj, pEdge, i )
        Rtm_ObjRemLast( pRtm, pEdge );
    // insert the result in the fanout values
    Rtm_ObjForEachFaninEdge( pObj, pEdge, i )
        Rtm_ObjAddFirst( pRtm, pEdge, RTM_VAL_VOID );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_ObjMarkAutoFwd_rec( Rtm_Obj_t * pObj )
{
    Rtm_Obj_t * pFanout;
    int i;
    if ( pObj->fAuto )
        return;
    pObj->fAuto = 1;
    Rtm_ObjForEachFanout( pObj, pFanout, i )
        Rtm_ObjMarkAutoFwd_rec( pFanout );
}

/**Function*************************************************************

  Synopsis    [Marks the nodes unreachable from the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rtm_ManMarkAutoFwd( Rtm_Man_t * pRtm )
{
    Rtm_Obj_t * pObjRtm;
    int i, Counter = 0;
    // mark nodes reachable from the PIs
    pObjRtm = (Rtm_Obj_t *)Vec_PtrEntry( pRtm->vObjs, 0 );
    Rtm_ObjMarkAutoFwd_rec( pObjRtm );
    Rtm_ManForEachPi( pRtm, pObjRtm, i )
        Rtm_ObjMarkAutoFwd_rec( pObjRtm );
    // count the number of autonomous nodes
    Rtm_ManForEachObj( pRtm, pObjRtm, i )
    {
        pObjRtm->fAuto = !pObjRtm->fAuto;
        Counter += pObjRtm->fAuto;
    }
    // mark the fanins of the autonomous nodes
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rtm_ObjMarkAutoBwd_rec( Rtm_Obj_t * pObj )
{
    Rtm_Obj_t * pFanin;
    int i;
    if ( pObj->fAuto )
        return;
    pObj->fAuto = 1;
    Rtm_ObjForEachFanin( pObj, pFanin, i )
        Rtm_ObjMarkAutoBwd_rec( pFanin );
}

/**Function*************************************************************

  Synopsis    [Marks the nodes unreachable from the POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rtm_ManMarkAutoBwd( Rtm_Man_t * pRtm )
{
    Rtm_Obj_t * pObjRtm;
    int i, Counter = 0;
    // mark nodes reachable from the PIs
    pObjRtm = (Rtm_Obj_t *)Vec_PtrEntry( pRtm->vObjs, 0 );
    pObjRtm->fAuto = 1;
    Rtm_ManForEachPi( pRtm, pObjRtm, i )
        pObjRtm->fAuto = 1;
    Rtm_ManForEachPo( pRtm, pObjRtm, i )
        Rtm_ObjMarkAutoBwd_rec( pObjRtm );
    // count the number of autonomous nodes
    Rtm_ManForEachObj( pRtm, pObjRtm, i )
    {
        pObjRtm->fAuto = !pObjRtm->fAuto;
        Counter += pObjRtm->fAuto;
    }
    // mark the fanins of the autonomous nodes
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Derive retiming manager from the given AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rtm_Man_t * Rtm_ManFromAig( Aig_Man_t * p )
{
    Rtm_Man_t * pRtm;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i;
    assert( Aig_ManRegNum(p) > 0 );
    assert( Aig_ManBufNum(p) == 0 );
    // allocate the manager
    pRtm = Rtm_ManAlloc( p );
    // allocate objects
    pObj = Aig_ManConst1(p);
    pObj->pData = Rtm_ObjAlloc( pRtm, 0, pObj->nRefs );
    Aig_ManForEachPiSeq( p, pObj, i )
    {
        pObj->pData = Rtm_ObjAlloc( pRtm, 0, pObj->nRefs );
        Vec_PtrPush( pRtm->vPis, pObj->pData );
    }
    Aig_ManForEachPoSeq( p, pObj, i )
    {
        pObj->pData = Rtm_ObjAlloc( pRtm, 1, 0 );
        Vec_PtrPush( pRtm->vPos, pObj->pData );
    }
    Aig_ManForEachLoSeq( p, pObj, i )
        pObj->pData = Rtm_ObjAlloc( pRtm, 1, pObj->nRefs );
    Aig_ManForEachLiSeq( p, pObj, i )
        pObj->pData = Rtm_ObjAlloc( pRtm, 1, 1 );
    Aig_ManForEachNode( p, pObj, i )
        pObj->pData = Rtm_ObjAlloc( pRtm, 2, pObj->nRefs );
    // connect objects
    Aig_ManForEachPoSeq( p, pObj, i )
        Rtm_ObjAddFanin( (Rtm_Obj_t *)pObj->pData, (Rtm_Obj_t *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
    Aig_ManForEachLiSeq( p, pObj, i )
        Rtm_ObjAddFanin( (Rtm_Obj_t *)pObj->pData, (Rtm_Obj_t *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
    Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )
        Rtm_ObjAddFanin( (Rtm_Obj_t *)pObjLo->pData, (Rtm_Obj_t *)pObjLi->pData, 0 );
    Aig_ManForEachNode( p, pObj, i )
    {
        Rtm_ObjAddFanin( (Rtm_Obj_t *)pObj->pData, (Rtm_Obj_t *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
        Rtm_ObjAddFanin( (Rtm_Obj_t *)pObj->pData, (Rtm_Obj_t *)Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj) );
    }
    return pRtm;
}

/**Function*************************************************************

  Synopsis    [Derive AIG manager after retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Rtm_ManToAig_rec( Aig_Man_t * pNew, Rtm_Man_t * pRtm, Rtm_Obj_t * pObjRtm, int * pLatches )
{
    Rtm_Edg_t * pEdge;
    Aig_Obj_t * pRes, * pFanin;
    int k, Val;
    if ( pObjRtm->pCopy )
        return (Aig_Obj_t *)pObjRtm->pCopy;
    // get the inputs
    pRes = Aig_ManConst1( pNew );
    Rtm_ObjForEachFaninEdge( pObjRtm, pEdge, k )
    {
        if ( pEdge->nLats == 0 )
            pFanin = Rtm_ManToAig_rec( pNew, pRtm, Rtm_ObjFanin(pObjRtm, k), pLatches );
        else
        {
            Val = Rtm_ObjGetFirst( pRtm, pEdge );
            pFanin = Aig_ManCi( pNew, pLatches[2*pObjRtm->Id + k] + pEdge->nLats - 1 );
            pFanin = Aig_NotCond( pFanin, Val == RTM_VAL_ONE );
        }
        pFanin = Aig_NotCond( pFanin, k ? pObjRtm->fCompl1 : pObjRtm->fCompl0 );
        pRes = Aig_And( pNew, pRes, pFanin );
    }
    return (Aig_Obj_t *)(pObjRtm->pCopy = pRes);
}

/**Function*************************************************************

  Synopsis    [Derive AIG manager after retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Rtm_ManToAig( Rtm_Man_t * pRtm )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObjNew;
    Rtm_Obj_t * pObjRtm;
    Rtm_Edg_t * pEdge;
    int i, k, m, Val, nLatches, * pLatches;
    // count latches and mark the first latch on each edge
    pLatches = ABC_ALLOC( int, 2 * Vec_PtrSize(pRtm->vObjs) );
    nLatches = 0;
    Rtm_ManForEachObj( pRtm, pObjRtm, i )
    Rtm_ObjForEachFaninEdge( pObjRtm, pEdge, k )
    {
        pLatches[2*pObjRtm->Id + k] = Vec_PtrSize(pRtm->vPis) + nLatches;
        nLatches += pEdge->nLats;
    }
    // create the new manager
    pNew = Aig_ManStart( Vec_PtrSize(pRtm->vObjs) + nLatches );
    // create PIs/POs and latches
    pObjRtm = (Rtm_Obj_t *)Vec_PtrEntry( pRtm->vObjs, 0 );
    pObjRtm->pCopy = Aig_ManConst1(pNew);
    Rtm_ManForEachPi( pRtm, pObjRtm, i )
        pObjRtm->pCopy = Aig_ObjCreateCi(pNew);
    for ( i = 0; i < nLatches; i++ )
        Aig_ObjCreateCi(pNew);
    // create internal nodes
    Rtm_ManForEachObj( pRtm, pObjRtm, i )
        Rtm_ManToAig_rec( pNew, pRtm, pObjRtm, pLatches );
    // create POs
    Rtm_ManForEachPo( pRtm, pObjRtm, i )
        Aig_ObjCreateCo( pNew, (Aig_Obj_t *)pObjRtm->pCopy );
    // connect latches 
    Rtm_ManForEachObj( pRtm, pObjRtm, i )
    Rtm_ObjForEachFaninEdge( pObjRtm, pEdge, k )
    {
        if ( pEdge->nLats == 0 )
            continue;
        pObjNew = (Aig_Obj_t *)Rtm_ObjFanin( pObjRtm, k )->pCopy;
        for ( m = 0; m < (int)pEdge->nLats; m++ )
        {
            Val = Rtm_ObjGetOne( pRtm, pEdge, pEdge->nLats - 1 - m );
            assert( Val == RTM_VAL_ZERO || Val == RTM_VAL_ONE || Val == RTM_VAL_VOID );
            pObjNew = Aig_NotCond( pObjNew, Val == RTM_VAL_ONE );
            Aig_ObjCreateCo( pNew, pObjNew );
            pObjNew = Aig_ManCi( pNew, pLatches[2*pObjRtm->Id + k] + m );
            pObjNew = Aig_NotCond( pObjNew, Val == RTM_VAL_ONE );
        }
//        assert( Aig_Regular(pObjNew)->nRefs > 0 );
    }
    ABC_FREE( pLatches );
    Aig_ManSetRegNum( pNew, nLatches );
    // remove useless nodes
    Aig_ManCleanup( pNew );
    if ( !Aig_ManCheck( pNew ) )
        printf( "Rtm_ManToAig: The network check has failed.\n" );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Performs forward retiming with the given limit on depth.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Rtm_ManRetime( Aig_Man_t * p, int fForward, int nStepsMax, int fVerbose )
{
    Vec_Ptr_t * vQueue;
    Aig_Man_t * pNew;
    Rtm_Man_t * pRtm;
    Rtm_Obj_t * pObj, * pNext;
    Aig_Obj_t * pObjAig;
    int i, k, nAutos, Degree, DegreeMax = 0; 
    abctime clk;

    // create the retiming manager
clk = Abc_Clock();
    pRtm = Rtm_ManFromAig( p );
    // set registers
    Aig_ManForEachLoSeq( p, pObjAig, i )
        Rtm_ObjAddFirst( pRtm, Rtm_ObjEdge((Rtm_Obj_t *)pObjAig->pData, 0), fForward? RTM_VAL_ZERO : RTM_VAL_VOID );
    // detect and mark the autonomous components
    if ( fForward )
        nAutos = Rtm_ManMarkAutoFwd( pRtm );
    else
        nAutos = Rtm_ManMarkAutoBwd( pRtm );
    if ( fVerbose )
    {
        printf( "Detected %d autonomous objects. ", nAutos );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }

    // set the current retiming number
    Rtm_ManForEachObj( pRtm, pObj, i )
    {
        assert( pObj->nFanins == pObj->Num );
        assert( pObj->nFanouts == pObj->Temp );
        pObj->Num = 0;
    }

clk = Abc_Clock();
    // put the LOs on the queue
    vQueue = Vec_PtrAlloc( 1000 );
    if ( fForward )
    {
        Aig_ManForEachLoSeq( p, pObjAig, i )
        {
            pObj = (Rtm_Obj_t *)pObjAig->pData;
            if ( pObj->fAuto )
                continue;
            pObj->fMark = 1;
            Vec_PtrPush( vQueue, pObj );
        }
    }
    else
    {
        Aig_ManForEachLiSeq( p, pObjAig, i )
        {
            pObj = (Rtm_Obj_t *)pObjAig->pData;
            if ( pObj->fAuto )
                continue;
            pObj->fMark = 1;
            Vec_PtrPush( vQueue, pObj );
        }
    }
    // perform retiming 
    DegreeMax = 0;
    Vec_PtrForEachEntry( Rtm_Obj_t *, vQueue, pObj, i )
    {
        pObj->fMark = 0;
        // retime the node 
        if ( fForward )
        {
            Rtm_ObjRetimeFwd( pRtm, pObj );
            // check if its fanouts should be retimed
            Rtm_ObjForEachFanout( pObj, pNext, k )
            {
                if ( pNext->fMark ) // skip aleady scheduled
                    continue;
                if ( pNext->Type ) // skip POs
                    continue;
                if ( !Rtm_ObjCheckRetimeFwd( pNext ) ) // skip non-retimable
                    continue;
                Degree = Rtm_ObjGetDegreeFwd( pNext );
                DegreeMax = Abc_MaxInt( DegreeMax, Degree );
                if ( Degree > nStepsMax ) // skip nodes with high degree
                    continue;
                pNext->fMark = 1;
                pNext->Num = Degree;
                Vec_PtrPush( vQueue, pNext );
            }
        }
        else
        {
            Rtm_ObjRetimeBwd( pRtm, pObj );
            // check if its fanouts should be retimed
            Rtm_ObjForEachFanin( pObj, pNext, k )
            {
                if ( pNext->fMark ) // skip aleady scheduled
                    continue;
                if ( pNext->nFanins == 0 ) // skip PIs
                    continue;
                if ( !Rtm_ObjCheckRetimeBwd( pNext ) ) // skip non-retimable
                    continue;
                Degree = Rtm_ObjGetDegreeBwd( pNext );
                DegreeMax = Abc_MaxInt( DegreeMax, Degree );
                if ( Degree > nStepsMax ) // skip nodes with high degree
                    continue;
                pNext->fMark = 1;
                pNext->Num = Degree;
                Vec_PtrPush( vQueue, pNext );
            }
        }
    }

    if ( fVerbose )
    {
        printf( "Performed %d %s latch moves of max depth %d and max latch count %d.\n", 
            Vec_PtrSize(vQueue), fForward? "fwd":"bwd", DegreeMax, Rtm_ManLatchMax(pRtm) );
        printf( "Memory usage = %d.  ", pRtm->nExtraCur );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    Vec_PtrFree( vQueue );

    // get the new manager
    pNew = Rtm_ManToAig( pRtm );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Rtm_ManFree( pRtm );
    // group the registers
clk = Abc_Clock();
    pNew = Aig_ManReduceLaches( pNew, fVerbose );
    if ( fVerbose )
    {
        ABC_PRT( "Register sharing time", Abc_Clock() - clk );
    }
    return pNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

