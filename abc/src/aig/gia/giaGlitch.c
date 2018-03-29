/**CFile****************************************************************

  FileName    [giaGlitch.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Glitch simulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaGlitch.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Gli_Obj_t_ Gli_Obj_t;
struct Gli_Obj_t_
{
    unsigned       fTerm    :  1;    // terminal node
    unsigned       fPhase   :  1;    // value under 000 pattern
    unsigned       fPhase2  :  1;    // value under 000 pattern
    unsigned       fMark    :  1;    // user-controlled mark
    unsigned       nFanins  :  3;    // the number of fanins
    unsigned       nFanouts : 25;    // total number of fanouts
    unsigned       Handle;           // ID of the node
    word *         pTruth;           // truth table of the node
    unsigned       uSimInfo;         // simulation info of the node
    union 
    {
        int        iFanin;           // the number of fanins added
        int        nSwitches;        // the number of switches
    };
    union 
    {
        int        iFanout;          // the number of fanouts added
        int        nGlitches;        // the number of glitches ( nGlitches >= nSwitches )
    };
    int            Fanios[0];        // the array of fanins/fanouts
};

typedef struct Gli_Man_t_ Gli_Man_t;
struct Gli_Man_t_
{
    Vec_Int_t *    vCis;             // the vector of CIs (PIs + LOs)
    Vec_Int_t *    vCos;             // the vector of COs (POs + LIs)
    Vec_Int_t *    vCisChanged;      // the changed CIs
    Vec_Int_t *    vAffected;        // the affected nodes
    Vec_Int_t *    vFrontier;        // the fanouts of these nodes
    int            nObjs;            // the number of objects
    int            nRegs;            // the number of registers
    int            nTravIds;         // traversal ID of the network
    int            iObjData;         // pointer to the current data
    int            nObjData;         // the size of array to store the logic network
    int *          pObjData;         // the internal nodes
    unsigned *     pSimInfoPrev;     // previous values of the CIs
};


static inline int         Gli_ManCiNum( Gli_Man_t * p )            { return Vec_IntSize(p->vCis);                                         }
static inline int         Gli_ManCoNum( Gli_Man_t * p )            { return Vec_IntSize(p->vCos);                                         }
static inline int         Gli_ManPiNum( Gli_Man_t * p )            { return Vec_IntSize(p->vCis) - p->nRegs;                              }
static inline int         Gli_ManPoNum( Gli_Man_t * p )            { return Vec_IntSize(p->vCos) - p->nRegs;                              }
static inline int         Gli_ManRegNum( Gli_Man_t * p )           { return p->nRegs;                                                     }
static inline int         Gli_ManObjNum( Gli_Man_t * p )           { return p->nObjs;                                                     } 
static inline int         Gli_ManNodeNum( Gli_Man_t * p )          { return p->nObjs - Vec_IntSize(p->vCis) - Vec_IntSize(p->vCos);       } 

static inline Gli_Obj_t * Gli_ManObj( Gli_Man_t * p, int v )       { return (Gli_Obj_t *)(p->pObjData + v);                               } 
static inline Gli_Obj_t * Gli_ManCi( Gli_Man_t * p, int v )        { return Gli_ManObj( p, Vec_IntEntry(p->vCis,v) );                     }
static inline Gli_Obj_t * Gli_ManCo( Gli_Man_t * p, int v )        { return Gli_ManObj( p, Vec_IntEntry(p->vCos,v) );                     }
static inline Gli_Obj_t * Gli_ManPi( Gli_Man_t * p, int v )        { assert( v < Gli_ManPiNum(p) );  return Gli_ManCi( p, v );            }
static inline Gli_Obj_t * Gli_ManPo( Gli_Man_t * p, int v )        { assert( v < Gli_ManPoNum(p) );  return Gli_ManCo( p, v );            }
static inline Gli_Obj_t * Gli_ManRo( Gli_Man_t * p, int v )        { assert( v < Gli_ManRegNum(p) ); return Gli_ManCi( p, Gli_ManRegNum(p)+v );      }
static inline Gli_Obj_t * Gli_ManRi( Gli_Man_t * p, int v )        { assert( v < Gli_ManRegNum(p) ); return Gli_ManCo( p, Gli_ManRegNum(p)+v );      }

static inline int         Gli_ObjIsTerm( Gli_Obj_t * pObj )        { return pObj->fTerm;                                                  } 
static inline int         Gli_ObjIsCi( Gli_Obj_t * pObj )          { return pObj->fTerm && pObj->nFanins == 0;                            } 
static inline int         Gli_ObjIsCo( Gli_Obj_t * pObj )          { return pObj->fTerm && pObj->nFanins == 1;                            } 
static inline int         Gli_ObjIsNode( Gli_Obj_t * pObj )        { return!pObj->fTerm;                                                  } 

static inline int         Gli_ObjFaninNum( Gli_Obj_t * pObj )      { return pObj->nFanins;                                                } 
static inline int         Gli_ObjFanoutNum( Gli_Obj_t * pObj )     { return pObj->nFanouts;                                               } 
static inline int         Gli_ObjSize( Gli_Obj_t * pObj )          { return sizeof(Gli_Obj_t) / 4 + pObj->nFanins + pObj->nFanouts;       } 

static inline Gli_Obj_t * Gli_ObjFanin( Gli_Obj_t * pObj, int i )  { return (Gli_Obj_t *)(((int *)pObj) - pObj->Fanios[i]);               } 
static inline Gli_Obj_t * Gli_ObjFanout( Gli_Obj_t * pObj, int i ) { return (Gli_Obj_t *)(((int *)pObj) + pObj->Fanios[pObj->nFanins+i]); } 

#define Gli_ManForEachObj( p, pObj, i )                 \
    for ( i = 0; (i < p->nObjData) && (pObj = Gli_ManObj(p,i)); i += Gli_ObjSize(pObj) )
#define Gli_ManForEachNode( p, pObj, i )                \
    for ( i = 0; (i < p->nObjData) && (pObj = Gli_ManObj(p,i)); i += Gli_ObjSize(pObj) ) if ( Gli_ObjIsTerm(pObj) ) {} else

#define Gli_ManForEachEntry( vVec, p, pObj, i )         \
    for ( i = 0; (i < Vec_IntSize(vVec)) && (pObj = Gli_ManObj(p,Vec_IntEntry(vVec,i))); i++ )
#define Gli_ManForEachCi( p, pObj, i )                  \
    for ( i = 0; (i < Vec_IntSize(p->vCis)) && (pObj = Gli_ManObj(p,Vec_IntEntry(p->vCis,i))); i++ )
#define Gli_ManForEachCo( p, pObj, i )                  \
    for ( i = 0; (i < Vec_IntSize(p->vCos)) && (pObj = Gli_ManObj(p,Vec_IntEntry(p->vCos,i))); i++ )

#define Gli_ManForEachPi( p, pObj, i )                                  \
    for ( i = 0; (i < Gli_ManPiNum(p)) && ((pObj) = Gli_ManCi(p, i)); i++ )
#define Gli_ManForEachPo( p, pObj, i )                                  \
    for ( i = 0; (i < Gli_ManPoNum(p)) && ((pObj) = Gli_ManCo(p, i)); i++ )
#define Gli_ManForEachRo( p, pObj, i )                                  \
    for ( i = 0; (i < Gli_ManRegNum(p)) && ((pObj) = Gli_ManCi(p, Gli_ManPiNum(p)+i)); i++ )
#define Gli_ManForEachRi( p, pObj, i )                                  \
    for ( i = 0; (i < Gli_ManRegNum(p)) && ((pObj) = Gli_ManCo(p, Gli_ManPoNum(p)+i)); i++ )
#define Gli_ManForEachRiRo( p, pObjRi, pObjRo, i )                      \
    for ( i = 0; (i < Gli_ManRegNum(p)) && ((pObjRi) = Gli_ManCo(p, Gli_ManPoNum(p)+i)) && ((pObjRo) = Gli_ManCi(p, Gli_ManPiNum(p)+i)); i++ )

#define Gli_ObjForEachFanin( pObj, pNext, i )           \
    for ( i = 0; (i < (int)pObj->nFanins) && (pNext = Gli_ObjFanin(pObj,i)); i++ )
#define Gli_ObjForEachFanout( pObj, pNext, i )          \
    for ( i = 0; (i < (int)pObj->nFanouts) && (pNext = Gli_ObjFanout(pObj,i)); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates logic network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gli_Man_t * Gli_ManAlloc( int nObjs, int nRegs, int nFanioPairs )
{
    Gli_Man_t * p;
    p = (Gli_Man_t *)ABC_CALLOC( int, (sizeof(Gli_Man_t) / 4) + (sizeof(Gli_Obj_t) / 4) * nObjs + 2 * nFanioPairs );
    p->nRegs = nRegs;
    p->vCis = Vec_IntAlloc( 1000 );
    p->vCos = Vec_IntAlloc( 1000 );
    p->vCisChanged = Vec_IntAlloc( 1000 );
    p->vAffected = Vec_IntAlloc( 1000 );
    p->vFrontier = Vec_IntAlloc( 1000 );
    p->nObjData = (sizeof(Gli_Obj_t) / 4) * nObjs + 2 * nFanioPairs;
    p->pObjData = (int *)(p + 1);
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes logic network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManStop( Gli_Man_t * p )
{
    Vec_IntFree( p->vCis );
    Vec_IntFree( p->vCos );
    Vec_IntFree( p->vCisChanged );
    Vec_IntFree( p->vAffected );
    Vec_IntFree( p->vFrontier );
    ABC_FREE( p->pSimInfoPrev );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Checks logic network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManPrintObjects( Gli_Man_t * p )
{
    Gli_Obj_t * pObj, * pNext;
    int i, k;
    Gli_ManForEachObj( p, pObj, i )
    {
        printf( "Node %d \n", pObj->Handle );
        printf( "Fanins: " );
        Gli_ObjForEachFanin( pObj, pNext, k )
            printf( "%d ", pNext->Handle );
        printf( "\n" );
        printf( "Fanouts: " );
        Gli_ObjForEachFanout( pObj, pNext, k )
            printf( "%d ", pNext->Handle );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Checks logic network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManFinalize( Gli_Man_t * p )
{
    Gli_Obj_t * pObj;
    int i;
    assert( p->iObjData == p->nObjData );
    Gli_ManForEachObj( p, pObj, i )
    {
        assert( pObj->iFanin == (int)pObj->nFanins );
        assert( pObj->iFanout == (int)pObj->nFanouts );
        pObj->iFanin = 0;
        pObj->iFanout = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Creates fanin/fanout pair.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ObjAddFanin( Gli_Obj_t * pObj, Gli_Obj_t * pFanin )
{ 
    assert( pObj->iFanin < (int)pObj->nFanins );
    assert( pFanin->iFanout < (int)pFanin->nFanouts );
    pFanin->Fanios[pFanin->nFanins + pFanin->iFanout++] = 
    pObj->Fanios[pObj->iFanin++] = pObj->Handle - pFanin->Handle;
}
 
/**Function*************************************************************

  Synopsis    [Allocates object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gli_Obj_t * Gli_ObjAlloc( Gli_Man_t * p, int nFanins, int nFanouts )
{
    Gli_Obj_t * pObj;
    pObj = Gli_ManObj( p, p->iObjData );
    pObj->Handle   = p->iObjData;
    pObj->nFanins  = nFanins;
    pObj->nFanouts = nFanouts;
    p->iObjData += Gli_ObjSize( pObj );
    p->nObjs++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates CI.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gli_ManCreateCi( Gli_Man_t * p, int nFanouts )
{
    Gli_Obj_t * pObj;
    pObj = Gli_ObjAlloc( p, 0, nFanouts );
    pObj->fTerm = 1;
    Vec_IntPush( p->vCis, pObj->Handle );
    return pObj->Handle;
}

/**Function*************************************************************

  Synopsis    [Creates CO.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gli_ManCreateCo( Gli_Man_t * p, int iFanin )
{
    Gli_Obj_t * pObj, * pFanin;
    pObj = Gli_ObjAlloc( p, 1, 0 );
    pObj->fTerm = 1;
    pFanin = Gli_ManObj( p, iFanin );
    Gli_ObjAddFanin( pObj, pFanin );
    pObj->fPhase = pObj->fPhase2 = pFanin->fPhase;
    Vec_IntPush( p->vCos, pObj->Handle );
    return pObj->Handle;
}

/**Function*************************************************************

  Synopsis    [Creates node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gli_NodeComputeValue( Gli_Obj_t * pNode )
{
    int i, Phase = 0;
    for ( i = 0; i < (int)pNode->nFanins; i++ )
        Phase |= (Gli_ObjFanin(pNode, i)->fPhase << i);
    return Abc_InfoHasBit( (unsigned *)pNode->pTruth, Phase );
}

/**Function*************************************************************

  Synopsis    [Creates node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gli_NodeComputeValue2( Gli_Obj_t * pNode )
{
    int i, Phase = 0;
    for ( i = 0; i < (int)pNode->nFanins; i++ )
        Phase |= (Gli_ObjFanin(pNode, i)->fPhase2 << i);
    return Abc_InfoHasBit( (unsigned *)pNode->pTruth, Phase );
}

/**Function*************************************************************

  Synopsis    [Creates node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gli_ManCreateNode( Gli_Man_t * p, Vec_Int_t * vFanins, int nFanouts, word * pGateTruth )
{
    Gli_Obj_t * pObj, * pFanin;
    int i;
    assert( Vec_IntSize(vFanins) <= 16 );
    pObj = Gli_ObjAlloc( p, Vec_IntSize(vFanins), nFanouts );
    Gli_ManForEachEntry( vFanins, p, pFanin, i )
        Gli_ObjAddFanin( pObj, pFanin );
    pObj->pTruth = pGateTruth;
    pObj->fPhase = pObj->fPhase2 = Gli_NodeComputeValue( pObj );
    return pObj->Handle;
}

/**Function*************************************************************

  Synopsis    [Returns the number of switches of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gli_ObjNumSwitches( Gli_Man_t * p, int iNode )
{
    return Gli_ManObj( p, iNode )->nSwitches;
}

/**Function*************************************************************

  Synopsis    [Returns the number of glitches of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gli_ObjNumGlitches( Gli_Man_t * p, int iNode )
{
    return Gli_ManObj( p, iNode )->nGlitches;
}


/**Function*************************************************************

  Synopsis    [Sets random info at the PIs and collects changed PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManSetPiRandom( Gli_Man_t * p, float PiTransProb )
{
    Gli_Obj_t * pObj;
    float Multi = 1.0 / (1 << 16);
    int i;
    assert( 0.0 < PiTransProb && PiTransProb < 1.0 );
    Vec_IntClear( p->vCisChanged );
    Gli_ManForEachCi( p, pObj, i )
        if ( Multi * (Gia_ManRandom(0) & 0xffff) < PiTransProb )
        {
            Vec_IntPush( p->vCisChanged, pObj->Handle );
            pObj->fPhase  ^= 1;
            pObj->fPhase2 ^= 1;
            pObj->nSwitches++;
            pObj->nGlitches++;
        }
}

/**Function*************************************************************

  Synopsis    [Sets random info at the PIs and collects changed PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManSetPiFromSaved( Gli_Man_t * p, int iBit )
{
    Gli_Obj_t * pObj;
    int i;
    Vec_IntClear( p->vCisChanged );
    Gli_ManForEachCi( p, pObj, i )
        if ( (p->pSimInfoPrev[i] ^ pObj->uSimInfo) & (1 << iBit) )
        {
            Vec_IntPush( p->vCisChanged, pObj->Handle );
            pObj->fPhase  ^= 1;
            pObj->fPhase2 ^= 1;
            pObj->nSwitches++;
            pObj->nGlitches++;
        }
}

/**Function*************************************************************

  Synopsis    [Computes switching activity of each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManSwitching( Gli_Man_t * p )
{
    Gli_Obj_t * pThis;
    int i;
    Gli_ManForEachNode( p, pThis, i )
    {
        if ( ((int)pThis->fPhase) == Gli_NodeComputeValue(pThis) )
            continue;
        pThis->fPhase ^= 1;
        pThis->nSwitches++;
    }
}

/**Function*************************************************************

  Synopsis    [Computes glitching activity of each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManGlitching( Gli_Man_t * p )
{
    Gli_Obj_t * pThis, * pFanout;//, * pOther = Gli_ManObj(p, 41);
    int i, k, Handle;
//    Gli_ManForEachObj( p, pThis, i )
//        assert( pThis->fMark == 0 );
    // start the array of affected nodes
    Vec_IntClear( p->vAffected );
    Vec_IntForEachEntry( p->vCisChanged, Handle, i )
        Vec_IntPush( p->vAffected, Handle );
    // iteration propagation
    while ( Vec_IntSize(p->vAffected) > 0 )
    {
        // compute the frontier
        Vec_IntClear( p->vFrontier );
        Gli_ManForEachEntry( p->vAffected, p, pThis, i )
        {
            Gli_ObjForEachFanout( pThis, pFanout, k )
            {
                if ( Gli_ObjIsCo(pFanout) )
                    continue;
                if ( pFanout->fMark )
                    continue;
                pFanout->fMark = 1;
                Vec_IntPush( p->vFrontier, pFanout->Handle );
            }
        }
        // compute the next set of affected nodes
        Vec_IntClear( p->vAffected );
        Gli_ManForEachEntry( p->vFrontier, p, pThis, i )
        {
            pThis->fMark = 0;
            if ( ((int)pThis->fPhase2) == Gli_NodeComputeValue2(pThis) )
                continue;
            pThis->fPhase2 ^= 1;
            pThis->nGlitches++;
            Vec_IntPush( p->vAffected, pThis->Handle );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Checks that the resulting values are the same.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManVerify( Gli_Man_t * p )
{
    Gli_Obj_t * pObj;
    int i;
    Gli_ManForEachObj( p, pObj, i )
    {
        assert( pObj->fPhase == pObj->fPhase2 );
        assert( pObj->nGlitches >= pObj->nSwitches );
    }
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Gli_ManSimulateSeqNode( Gli_Man_t * p, Gli_Obj_t * pNode )
{
    unsigned pSimInfos[6], Result = 0;
    int nFanins = Gli_ObjFaninNum(pNode);
    int i, k, Phase;
    Gli_Obj_t * pFanin;
    assert( nFanins <= 16 );
    Gli_ObjForEachFanin( pNode, pFanin, i )
        pSimInfos[i] = pFanin->uSimInfo;
    for ( i = 0; i < 32; i++ )
    {
        Phase = 0;
        for ( k = 0; k < nFanins; k++ )
            if ( (pSimInfos[k] >> i) & 1 )
                Phase |= (1 << k);
        if ( Abc_InfoHasBit( (unsigned *)pNode->pTruth, Phase ) )
            Result |= (1 << i);
    }
    return Result;
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Gli_ManUpdateRandomInput( unsigned uInfo, float PiTransProb )
{
    float Multi = 1.0 / (1 << 16);
    int i;
    if ( PiTransProb == 0.5 )
        return Gia_ManRandom(0);
    for ( i = 0; i < 32; i++ )
        if ( Multi * (Gia_ManRandom(0) & 0xffff) < PiTransProb )
            uInfo ^= (1 << i);
    return uInfo;
}

/**Function*************************************************************

  Synopsis    [Simulates sequential network randomly for the given number of frames.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManSimulateSeqPref( Gli_Man_t * p, int nPref )
{
    Gli_Obj_t * pObj, * pObjRi, * pObjRo;
    int i, f;
    // initialize simulation data
    Gli_ManForEachPi( p, pObj, i )
        pObj->uSimInfo = Gli_ManUpdateRandomInput( pObj->uSimInfo, 0.5 );
    Gli_ManForEachRo( p, pObj, i )
        pObj->uSimInfo = 0;
    for ( f = 0; f < nPref; f++ )
    {
        // simulate one frame
        Gli_ManForEachNode( p, pObj, i )
            pObj->uSimInfo = Gli_ManSimulateSeqNode( p, pObj );
        Gli_ManForEachRi( p, pObj, i )
            pObj->uSimInfo = Gli_ObjFanin(pObj, 0)->uSimInfo;
        // initialize the next frame
        Gli_ManForEachPi( p, pObj, i )
            pObj->uSimInfo = Gli_ManUpdateRandomInput( pObj->uSimInfo, 0.5 );
        Gli_ManForEachRiRo( p, pObjRi, pObjRo, i )
            pObjRo->uSimInfo = pObjRi->uSimInfo;
    }
    // save simulation data after nPref timeframes
    if ( p->pSimInfoPrev == NULL )
        p->pSimInfoPrev = ABC_ALLOC( unsigned, Gli_ManCiNum(p) );
    Gli_ManForEachCi( p, pObj, i )
        p->pSimInfoPrev[i] = pObj->uSimInfo;
}

/**Function*************************************************************

  Synopsis    [Initialized object values to be one pattern in the saved data.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManSetDataSaved( Gli_Man_t * p, int iBit )
{
    Gli_Obj_t * pObj;
    int i;
    Gli_ManForEachCi( p, pObj, i )
        pObj->fPhase = pObj->fPhase2 = ((p->pSimInfoPrev[i] >> iBit) & 1);
    Gli_ManForEachNode( p, pObj, i )
        pObj->fPhase = pObj->fPhase2 = Gli_NodeComputeValue( pObj );
}

/**Function*************************************************************

  Synopsis    [Sets random info at the PIs and collects changed PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManSetPiRandomSeq( Gli_Man_t * p, float PiTransProb )
{
    Gli_Obj_t * pObj, * pObjRi;
    float Multi = 1.0 / (1 << 16);
    int i;
    assert( 0.0 < PiTransProb && PiTransProb < 1.0 );
    // transfer data to the COs
    Gli_ManForEachCo( p, pObj, i )
        pObj->fPhase = pObj->fPhase2 = Gli_ObjFanin(pObj, 0)->fPhase;
    // set changed PIs
    Vec_IntClear( p->vCisChanged );
    Gli_ManForEachPi( p, pObj, i )
        if ( Multi * (Gia_ManRandom(0) & 0xffff) < PiTransProb )
        {
            Vec_IntPush( p->vCisChanged, pObj->Handle );
            pObj->fPhase  ^= 1;
            pObj->fPhase2 ^= 1;
            pObj->nSwitches++;
            pObj->nGlitches++;
        }
    // set changed ROs
    Gli_ManForEachRiRo( p, pObjRi, pObj, i )
        if ( pObjRi->fPhase != pObj->fPhase )
        {
            Vec_IntPush( p->vCisChanged, pObj->Handle );
            pObj->fPhase  ^= 1;
            pObj->fPhase2 ^= 1;
            pObj->nSwitches++;
            pObj->nGlitches++;
        }

}

/**Function*************************************************************

  Synopsis    [Computes glitching activity of each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gli_ManSwitchesAndGlitches( Gli_Man_t * p, int nPatterns, float PiTransProb, int fVerbose )
{
    int i, k;
    abctime clk = Abc_Clock();
    Gia_ManRandom( 1 );
    Gli_ManFinalize( p );
    if ( p->nRegs == 0 )
    {
        for ( i = 0; i < nPatterns; i++ )
        {
            Gli_ManSetPiRandom( p, PiTransProb );
            Gli_ManSwitching( p );
            Gli_ManGlitching( p );
//            Gli_ManVerify( p );
        }
    }
    else 
    {
        int nIters = Abc_BitWordNum(nPatterns);
        Gli_ManSimulateSeqPref( p, 16 );
        for ( i = 0; i < 32; i++ )
        {
            Gli_ManSetDataSaved( p, i );
            for ( k = 0; k < nIters; k++ )
            {
                Gli_ManSetPiRandomSeq( p, PiTransProb );
                Gli_ManSwitching( p );
                Gli_ManGlitching( p );
//                Gli_ManVerify( p );
            }
        }
    }
    if ( fVerbose )
    {
        printf( "Simulated %d patterns.  Input transition probability %.2f.  ", nPatterns, PiTransProb );
        ABC_PRMn( "Memory", 4*p->nObjData );
        ABC_PRT( "Time", Abc_Clock() - clk );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

