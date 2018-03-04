/**CFile****************************************************************

  FileName    [ivy.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivy.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__ivy__ivy_h
#define ABC__aig__ivy__ivy_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "misc/extra/extra.h"
#include "misc/vec/vec.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START
 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Ivy_Man_t_            Ivy_Man_t;
typedef struct Ivy_Obj_t_            Ivy_Obj_t;
typedef int                          Ivy_Edge_t;
typedef struct Ivy_FraigParams_t_    Ivy_FraigParams_t;

// object types
typedef enum { 
    IVY_NONE,                        // 0: non-existent object
    IVY_PI,                          // 1: primary input (and constant 1 node)
    IVY_PO,                          // 2: primary output
    IVY_ASSERT,                      // 3: assertion
    IVY_LATCH,                       // 4: sequential element
    IVY_AND,                         // 5: AND node
    IVY_EXOR,                        // 6: EXOR node
    IVY_BUF,                         // 7: buffer (temporary)
    IVY_VOID                         // 8: unused object
} Ivy_Type_t;

// latch initial values
typedef enum { 
    IVY_INIT_NONE,                   // 0: not a latch
    IVY_INIT_0,                      // 1: zero
    IVY_INIT_1,                      // 2: one
    IVY_INIT_DC                      // 3: don't-care
} Ivy_Init_t;

// the AIG node
struct Ivy_Obj_t_  // 24 bytes (32-bit) or 32 bytes (64-bit)   // 10 words - 16 words
{
    int              Id;             // integer ID
    int              TravId;         // traversal ID
    unsigned         Type     :  4;  // object type
    unsigned         fMarkA   :  1;  // multipurpose mask
    unsigned         fMarkB   :  1;  // multipurpose mask
    unsigned         fExFan   :  1;  // set to 1 if last fanout added is EXOR
    unsigned         fPhase   :  1;  // value under 000...0 pattern
    unsigned         fFailTfo :  1;  // the TFO of the failed node
    unsigned         Init     :  2;  // latch initial value
    unsigned         Level    : 21;  // logic level
    int              nRefs;          // reference counter
    Ivy_Obj_t *      pFanin0;        // fanin
    Ivy_Obj_t *      pFanin1;        // fanin
    Ivy_Obj_t *      pFanout;        // fanout
    Ivy_Obj_t *      pNextFan0;      // next fanout of the first fanin
    Ivy_Obj_t *      pNextFan1;      // next fanout of the second fanin
    Ivy_Obj_t *      pPrevFan0;      // prev fanout of the first fanin
    Ivy_Obj_t *      pPrevFan1;      // prev fanout of the second fanin
    Ivy_Obj_t *      pEquiv;         // equivalent node
};

// the AIG manager
struct Ivy_Man_t_
{
    // AIG nodes
    Vec_Ptr_t *      vPis;           // the array of PIs
    Vec_Ptr_t *      vPos;           // the array of POs
    Vec_Ptr_t *      vBufs;          // the array of buffers
    Vec_Ptr_t *      vObjs;          // the array of objects
    Ivy_Obj_t *      pConst1;        // the constant 1 node
    Ivy_Obj_t        Ghost;          // the ghost node
    // AIG node counters
    int              nObjs[IVY_VOID];// the number of objects by type
    int              nCreated;       // the number of created objects
    int              nDeleted;       // the number of deleted objects
    // stuctural hash table
    int *            pTable;         // structural hash table
    int              nTableSize;     // structural hash table size
    // various data members
    int              fCatchExor;     // set to 1 to detect EXORs
    int              nTravIds;       // the traversal ID
    int              nLevelMax;      // the maximum level
    Vec_Int_t *      vRequired;      // required times
    int              fFanout;        // fanout is allocated
    void *           pData;          // the temporary data
    void *           pCopy;          // the temporary data
    Ivy_Man_t *      pHaig;          // history AIG if present
    int              nClassesSkip;   // the number of skipped classes
    // memory management
    Vec_Ptr_t *      vChunks;        // allocated memory pieces
    Vec_Ptr_t *      vPages;         // memory pages used by nodes
    Ivy_Obj_t *      pListFree;      // the list of free nodes 
    // timing statistics
    abctime          time1;
    abctime          time2;
};

struct Ivy_FraigParams_t_
{
    int              nSimWords;         // the number of words in the simulation info
    double           dSimSatur;         // the ratio of refined classes when saturation is reached
    int              fPatScores;        // enables simulation pattern scoring
    int              MaxScore;          // max score after which resimulation is used
    double           dActConeRatio;     // the ratio of cone to be bumped
    double           dActConeBumpMax;   // the largest bump in activity
    int              fProve;            // prove the miter outputs
    int              fVerbose;          // verbose output
    int              fDoSparse;         // skip sparse functions
    int              nBTLimitNode;      // conflict limit at a node
    int              nBTLimitMiter;     // conflict limit at an output
//    int              nBTLimitGlobal;    // conflict limit global
//    int              nInsLimitNode;     // inspection limit at a node
//    int              nInsLimitMiter;    // inspection limit at an output
//    int              nInsLimitGlobal;   // inspection limit global
};


#define IVY_CUT_LIMIT     256
#define IVY_CUT_INPUT       6

typedef struct Ivy_Cut_t_ Ivy_Cut_t;
struct Ivy_Cut_t_
{
    int         nLatches;
    short       nSize;
    short       nSizeMax;
    int         pArray[IVY_CUT_INPUT];
    unsigned    uHash;
};

typedef struct Ivy_Store_t_ Ivy_Store_t;
struct Ivy_Store_t_
{
    int         nCuts;
    int         nCutsM;
    int         nCutsMax;
    int         fSatur;
    Ivy_Cut_t   pCuts[IVY_CUT_LIMIT]; // storage for cuts
};

#define IVY_LEAF_MASK     255
#define IVY_LEAF_BITS       8

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define IVY_MIN(a,b)       (((a) < (b))? (a) : (b))
#define IVY_MAX(a,b)       (((a) > (b))? (a) : (b))

extern void Ivy_ManAddMemory( Ivy_Man_t * p );

static inline int          Ivy_BitWordNum( int nBits )            { return (nBits>>5) + ((nBits&31) > 0);           }
static inline int          Ivy_TruthWordNum( int nVars )          { return nVars <= 5 ? 1 : (1 << (nVars - 5));     }
static inline int          Ivy_InfoHasBit( unsigned * p, int i )  { return (p[(i)>>5] & (1<<((i) & 31))) > 0;       }
static inline void         Ivy_InfoSetBit( unsigned * p, int i )  { p[(i)>>5] |= (1<<((i) & 31));                   }
static inline void         Ivy_InfoXorBit( unsigned * p, int i )  { p[(i)>>5] ^= (1<<((i) & 31));                   }

static inline Ivy_Obj_t *  Ivy_Regular( Ivy_Obj_t * p )           { return (Ivy_Obj_t *)((ABC_PTRUINT_T)(p) & ~01); }
static inline Ivy_Obj_t *  Ivy_Not( Ivy_Obj_t * p )               { return (Ivy_Obj_t *)((ABC_PTRUINT_T)(p) ^  01); }
static inline Ivy_Obj_t *  Ivy_NotCond( Ivy_Obj_t * p, int c )    { return (Ivy_Obj_t *)((ABC_PTRUINT_T)(p) ^ (c)); }
static inline int          Ivy_IsComplement( Ivy_Obj_t * p )      { return (int)((ABC_PTRUINT_T)(p) & 01);          }

static inline Ivy_Obj_t *  Ivy_ManConst0( Ivy_Man_t * p )         { return Ivy_Not(p->pConst1);                     }
static inline Ivy_Obj_t *  Ivy_ManConst1( Ivy_Man_t * p )         { return p->pConst1;                              }
static inline Ivy_Obj_t *  Ivy_ManGhost( Ivy_Man_t * p )          { return &p->Ghost;                               }
static inline Ivy_Obj_t *  Ivy_ManPi( Ivy_Man_t * p, int i )      { return (Ivy_Obj_t *)Vec_PtrEntry(p->vPis, i);   }
static inline Ivy_Obj_t *  Ivy_ManPo( Ivy_Man_t * p, int i )      { return (Ivy_Obj_t *)Vec_PtrEntry(p->vPos, i);   }
static inline Ivy_Obj_t *  Ivy_ManObj( Ivy_Man_t * p, int i )     { return (Ivy_Obj_t *)Vec_PtrEntry(p->vObjs, i);  }

static inline Ivy_Edge_t   Ivy_EdgeCreate( int Id, int fCompl )            { return (Id << 1) | fCompl;             }
static inline int          Ivy_EdgeId( Ivy_Edge_t Edge )                   { return Edge >> 1;                      }
static inline int          Ivy_EdgeIsComplement( Ivy_Edge_t Edge )         { return Edge & 1;                       }
static inline Ivy_Edge_t   Ivy_EdgeRegular( Ivy_Edge_t Edge )              { return (Edge >> 1) << 1;               }
static inline Ivy_Edge_t   Ivy_EdgeNot( Ivy_Edge_t Edge )                  { return Edge ^ 1;                       }
static inline Ivy_Edge_t   Ivy_EdgeNotCond( Ivy_Edge_t Edge, int fCond )   { return Edge ^ fCond;                   }
static inline Ivy_Edge_t   Ivy_EdgeFromNode( Ivy_Obj_t * pNode )           { return Ivy_EdgeCreate( Ivy_Regular(pNode)->Id, Ivy_IsComplement(pNode) );          }
static inline Ivy_Obj_t *  Ivy_EdgeToNode( Ivy_Man_t * p, Ivy_Edge_t Edge ){ return Ivy_NotCond( Ivy_ManObj(p, Ivy_EdgeId(Edge)), Ivy_EdgeIsComplement(Edge) ); }

static inline int          Ivy_LeafCreate( int Id, int Lat )      { return (Id << IVY_LEAF_BITS) | Lat;         }
static inline int          Ivy_LeafId( int Leaf )                 { return Leaf >> IVY_LEAF_BITS;               }
static inline int          Ivy_LeafLat( int Leaf )                { return Leaf & IVY_LEAF_MASK;                }

static inline int          Ivy_ManPiNum( Ivy_Man_t * p )          { return p->nObjs[IVY_PI];                    }
static inline int          Ivy_ManPoNum( Ivy_Man_t * p )          { return p->nObjs[IVY_PO];                    }
static inline int          Ivy_ManAssertNum( Ivy_Man_t * p )      { return p->nObjs[IVY_ASSERT];                }
static inline int          Ivy_ManLatchNum( Ivy_Man_t * p )       { return p->nObjs[IVY_LATCH];                 }
static inline int          Ivy_ManAndNum( Ivy_Man_t * p )         { return p->nObjs[IVY_AND];                   }
static inline int          Ivy_ManExorNum( Ivy_Man_t * p )        { return p->nObjs[IVY_EXOR];                  }
static inline int          Ivy_ManBufNum( Ivy_Man_t * p )         { return p->nObjs[IVY_BUF];                   }
static inline int          Ivy_ManObjNum( Ivy_Man_t * p )         { return p->nCreated - p->nDeleted;           }
static inline int          Ivy_ManObjIdMax( Ivy_Man_t * p )       { return Vec_PtrSize(p->vObjs)-1;             }
static inline int          Ivy_ManNodeNum( Ivy_Man_t * p )        { return p->nObjs[IVY_AND]+p->nObjs[IVY_EXOR];}
static inline int          Ivy_ManHashObjNum( Ivy_Man_t * p )     { return p->nObjs[IVY_AND]+p->nObjs[IVY_EXOR]+p->nObjs[IVY_LATCH];     }
static inline int          Ivy_ManGetCost( Ivy_Man_t * p )        { return p->nObjs[IVY_AND]+3*p->nObjs[IVY_EXOR]+8*p->nObjs[IVY_LATCH]; }

static inline Ivy_Type_t   Ivy_ObjType( Ivy_Obj_t * pObj )        { return (Ivy_Type_t)pObj->Type;               }
static inline Ivy_Init_t   Ivy_ObjInit( Ivy_Obj_t * pObj )        { return (Ivy_Init_t)pObj->Init;               }
static inline int          Ivy_ObjIsConst1( Ivy_Obj_t * pObj )    { return pObj->Id == 0;            }
static inline int          Ivy_ObjIsGhost( Ivy_Obj_t * pObj )     { return pObj->Id < 0;             }
static inline int          Ivy_ObjIsNone( Ivy_Obj_t * pObj )      { return pObj->Type == IVY_NONE;   }
static inline int          Ivy_ObjIsPi( Ivy_Obj_t * pObj )        { return pObj->Type == IVY_PI;     }
static inline int          Ivy_ObjIsPo( Ivy_Obj_t * pObj )        { return pObj->Type == IVY_PO;     }
static inline int          Ivy_ObjIsCi( Ivy_Obj_t * pObj )        { return pObj->Type == IVY_PI || pObj->Type == IVY_LATCH; }
static inline int          Ivy_ObjIsCo( Ivy_Obj_t * pObj )        { return pObj->Type == IVY_PO || pObj->Type == IVY_LATCH; }
static inline int          Ivy_ObjIsAssert( Ivy_Obj_t * pObj )    { return pObj->Type == IVY_ASSERT; }
static inline int          Ivy_ObjIsLatch( Ivy_Obj_t * pObj )     { return pObj->Type == IVY_LATCH;  }
static inline int          Ivy_ObjIsAnd( Ivy_Obj_t * pObj )       { return pObj->Type == IVY_AND;    }
static inline int          Ivy_ObjIsExor( Ivy_Obj_t * pObj )      { return pObj->Type == IVY_EXOR;   }
static inline int          Ivy_ObjIsBuf( Ivy_Obj_t * pObj )       { return pObj->Type == IVY_BUF;    }
static inline int          Ivy_ObjIsNode( Ivy_Obj_t * pObj )      { return pObj->Type == IVY_AND || pObj->Type == IVY_EXOR; }
static inline int          Ivy_ObjIsTerm( Ivy_Obj_t * pObj )      { return pObj->Type == IVY_PI  || pObj->Type == IVY_PO || pObj->Type == IVY_ASSERT; }
static inline int          Ivy_ObjIsHash( Ivy_Obj_t * pObj )      { return pObj->Type == IVY_AND || pObj->Type == IVY_EXOR || pObj->Type == IVY_LATCH; }
static inline int          Ivy_ObjIsOneFanin( Ivy_Obj_t * pObj )  { return pObj->Type == IVY_PO  || pObj->Type == IVY_ASSERT || pObj->Type == IVY_BUF || pObj->Type == IVY_LATCH; }

static inline int          Ivy_ObjIsMarkA( Ivy_Obj_t * pObj )     { return pObj->fMarkA;  }
static inline void         Ivy_ObjSetMarkA( Ivy_Obj_t * pObj )    { pObj->fMarkA = 1;     }
static inline void         Ivy_ObjClearMarkA( Ivy_Obj_t * pObj )  { pObj->fMarkA = 0;     }
 
static inline void         Ivy_ObjSetTravId( Ivy_Obj_t * pObj, int TravId )                { pObj->TravId = TravId;                               }
static inline void         Ivy_ObjSetTravIdCurrent( Ivy_Man_t * p, Ivy_Obj_t * pObj )      { pObj->TravId = p->nTravIds;                          }
static inline void         Ivy_ObjSetTravIdPrevious( Ivy_Man_t * p, Ivy_Obj_t * pObj )     { pObj->TravId = p->nTravIds - 1;                      }
static inline int          Ivy_ObjIsTravIdCurrent( Ivy_Man_t * p, Ivy_Obj_t * pObj )       { return (int )((int)pObj->TravId == p->nTravIds);     }
static inline int          Ivy_ObjIsTravIdPrevious( Ivy_Man_t * p, Ivy_Obj_t * pObj )      { return (int )((int)pObj->TravId == p->nTravIds - 1); }

static inline int          Ivy_ObjId( Ivy_Obj_t * pObj )          { return pObj->Id;                               }
static inline int          Ivy_ObjTravId( Ivy_Obj_t * pObj )      { return pObj->TravId;                           }
static inline int          Ivy_ObjPhase( Ivy_Obj_t * pObj )       { return pObj->fPhase;                           }
static inline int          Ivy_ObjExorFanout( Ivy_Obj_t * pObj )  { return pObj->fExFan;                           }
static inline int          Ivy_ObjRefs( Ivy_Obj_t * pObj )        { return pObj->nRefs;                            }
static inline void         Ivy_ObjRefsInc( Ivy_Obj_t * pObj )     { pObj->nRefs++;                                 }
static inline void         Ivy_ObjRefsDec( Ivy_Obj_t * pObj )     { assert( pObj->nRefs > 0 ); pObj->nRefs--;      }
static inline int          Ivy_ObjFaninId0( Ivy_Obj_t * pObj )    { return pObj->pFanin0? Ivy_ObjId(Ivy_Regular(pObj->pFanin0)) : 0; }
static inline int          Ivy_ObjFaninId1( Ivy_Obj_t * pObj )    { return pObj->pFanin1? Ivy_ObjId(Ivy_Regular(pObj->pFanin1)) : 0; }
static inline int          Ivy_ObjFaninC0( Ivy_Obj_t * pObj )     { return Ivy_IsComplement(pObj->pFanin0);        }
static inline int          Ivy_ObjFaninC1( Ivy_Obj_t * pObj )     { return Ivy_IsComplement(pObj->pFanin1);        }
static inline Ivy_Obj_t *  Ivy_ObjFanin0( Ivy_Obj_t * pObj )      { return Ivy_Regular(pObj->pFanin0);             }
static inline Ivy_Obj_t *  Ivy_ObjFanin1( Ivy_Obj_t * pObj )      { return Ivy_Regular(pObj->pFanin1);             }
static inline Ivy_Obj_t *  Ivy_ObjChild0( Ivy_Obj_t * pObj )      { return pObj->pFanin0;                          }
static inline Ivy_Obj_t *  Ivy_ObjChild1( Ivy_Obj_t * pObj )      { return pObj->pFanin1;                          }
static inline Ivy_Obj_t *  Ivy_ObjChild0Equiv( Ivy_Obj_t * pObj ) { assert( !Ivy_IsComplement(pObj) ); return Ivy_ObjFanin0(pObj)? Ivy_NotCond(Ivy_ObjFanin0(pObj)->pEquiv, Ivy_ObjFaninC0(pObj)) : NULL;  }
static inline Ivy_Obj_t *  Ivy_ObjChild1Equiv( Ivy_Obj_t * pObj ) { assert( !Ivy_IsComplement(pObj) ); return Ivy_ObjFanin1(pObj)? Ivy_NotCond(Ivy_ObjFanin1(pObj)->pEquiv, Ivy_ObjFaninC1(pObj)) : NULL;  }
static inline Ivy_Obj_t *  Ivy_ObjEquiv( Ivy_Obj_t * pObj )       { return Ivy_Regular(pObj)->pEquiv? Ivy_NotCond(Ivy_Regular(pObj)->pEquiv, Ivy_IsComplement(pObj)) : NULL; }
static inline int          Ivy_ObjLevel( Ivy_Obj_t * pObj )       { return pObj->Level;                            }
static inline int          Ivy_ObjLevelNew( Ivy_Obj_t * pObj )    { return 1 + Ivy_ObjIsExor(pObj) + IVY_MAX(Ivy_ObjFanin0(pObj)->Level, Ivy_ObjFanin1(pObj)->Level);       }
static inline int          Ivy_ObjFaninPhase( Ivy_Obj_t * pObj )  { return Ivy_IsComplement(pObj)? !Ivy_Regular(pObj)->fPhase : pObj->fPhase; }

static inline void         Ivy_ObjClean( Ivy_Obj_t * pObj )       
{ 
    int IdSaved = pObj->Id; 
    memset( pObj, 0, sizeof(Ivy_Obj_t) ); 
    pObj->Id = IdSaved; 
}
static inline void         Ivy_ObjOverwrite( Ivy_Obj_t * pBase, Ivy_Obj_t * pData )   
{ 
    int IdSaved = pBase->Id; 
    memcpy( pBase, pData, sizeof(Ivy_Obj_t) ); 
    pBase->Id = IdSaved;         
}
static inline int          Ivy_ObjWhatFanin( Ivy_Obj_t * pObj, Ivy_Obj_t * pFanin )    
{ 
    if ( Ivy_ObjFanin0(pObj) == pFanin ) return 0; 
    if ( Ivy_ObjFanin1(pObj) == pFanin ) return 1; 
    assert(0); return -1; 
}
static inline int          Ivy_ObjFanoutC( Ivy_Obj_t * pObj, Ivy_Obj_t * pFanout )    
{ 
    if ( Ivy_ObjFanin0(pFanout) == pObj ) return Ivy_ObjFaninC0(pObj); 
    if ( Ivy_ObjFanin1(pFanout) == pObj ) return Ivy_ObjFaninC1(pObj); 
    assert(0); return -1; 
}

// create the ghost of the new node
static inline Ivy_Obj_t *  Ivy_ObjCreateGhost( Ivy_Man_t * p, Ivy_Obj_t * p0, Ivy_Obj_t * p1, Ivy_Type_t Type, Ivy_Init_t Init )    
{
    Ivy_Obj_t * pGhost, * pTemp;
    assert( Type != IVY_AND || !Ivy_ObjIsConst1(Ivy_Regular(p0)) );
    assert( p1 == NULL || !Ivy_ObjIsConst1(Ivy_Regular(p1)) );
    assert( Type == IVY_PI || Ivy_Regular(p0) != Ivy_Regular(p1) );
    assert( Type != IVY_LATCH || !Ivy_IsComplement(p0) );
//    assert( p1 == NULL || (!Ivy_ObjIsLatch(Ivy_Regular(p0)) || !Ivy_ObjIsLatch(Ivy_Regular(p1))) );
    pGhost = Ivy_ManGhost(p);
    pGhost->Type = Type;
    pGhost->Init = Init;
    pGhost->pFanin0 = p0;
    pGhost->pFanin1 = p1;
    if ( p1 && Ivy_ObjFaninId0(pGhost) > Ivy_ObjFaninId1(pGhost) )
        pTemp = pGhost->pFanin0, pGhost->pFanin0 = pGhost->pFanin1, pGhost->pFanin1 = pTemp;
    return pGhost;
}

// get the complemented initial state
static Ivy_Init_t Ivy_InitNotCond( Ivy_Init_t Init, int fCompl )
{
    assert( Init != IVY_INIT_NONE );
    if ( fCompl == 0 )
        return Init;
    if ( Init == IVY_INIT_0 )
        return IVY_INIT_1;
    if ( Init == IVY_INIT_1 )
        return IVY_INIT_0;
    return IVY_INIT_DC;
}

// get the initial state after forward retiming over AND gate
static Ivy_Init_t Ivy_InitAnd( Ivy_Init_t InitA, Ivy_Init_t InitB )
{
    assert( InitA != IVY_INIT_NONE && InitB != IVY_INIT_NONE );
    if ( InitA == IVY_INIT_0 || InitB == IVY_INIT_0 )
        return IVY_INIT_0;
    if ( InitA == IVY_INIT_DC || InitB == IVY_INIT_DC )
        return IVY_INIT_DC;
    return IVY_INIT_1;
}

// get the initial state after forward retiming over EXOR gate
static Ivy_Init_t Ivy_InitExor( Ivy_Init_t InitA, Ivy_Init_t InitB )
{
    assert( InitA != IVY_INIT_NONE && InitB != IVY_INIT_NONE );
    if ( InitA == IVY_INIT_DC || InitB == IVY_INIT_DC )
        return IVY_INIT_DC;
    if ( InitA == IVY_INIT_0 && InitB == IVY_INIT_1 )
        return IVY_INIT_1;
    if ( InitA == IVY_INIT_1 && InitB == IVY_INIT_0 )
        return IVY_INIT_1;
    return IVY_INIT_0;
}

// internal memory manager
static inline Ivy_Obj_t * Ivy_ManFetchMemory( Ivy_Man_t * p )  
{ 
    Ivy_Obj_t * pTemp;
    if ( p->pListFree == NULL )
        Ivy_ManAddMemory( p );
    pTemp = p->pListFree;
    p->pListFree = *((Ivy_Obj_t **)pTemp);
    memset( pTemp, 0, sizeof(Ivy_Obj_t) ); 
    return pTemp;
}
static inline void Ivy_ManRecycleMemory( Ivy_Man_t * p, Ivy_Obj_t * pEntry )
{
    pEntry->Type = IVY_NONE; // distinquishes dead node from live node
    *((Ivy_Obj_t **)pEntry) = p->pListFree;
    p->pListFree = pEntry;
}


////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

// iterator over the primary inputs
#define Ivy_ManForEachPi( p, pObj, i )                                          \
    Vec_PtrForEachEntry( Ivy_Obj_t *, p->vPis, pObj, i )
// iterator over the primary outputs
#define Ivy_ManForEachPo( p, pObj, i )                                          \
    Vec_PtrForEachEntry( Ivy_Obj_t *, p->vPos, pObj, i )
// iterator over all objects, including those currently not used
#define Ivy_ManForEachObj( p, pObj, i )                                         \
    Vec_PtrForEachEntry( Ivy_Obj_t *, p->vObjs, pObj, i ) if ( (pObj) == NULL ) {} else
// iterator over the combinational inputs
#define Ivy_ManForEachCi( p, pObj, i )                                          \
    Ivy_ManForEachObj( p, pObj, i ) if ( !Ivy_ObjIsCi(pObj) ) {} else
// iterator over the combinational outputs
#define Ivy_ManForEachCo( p, pObj, i )                                          \
    Ivy_ManForEachObj( p, pObj, i ) if ( !Ivy_ObjIsCo(pObj) ) {} else
// iterator over logic nodes (AND and EXOR gates)
#define Ivy_ManForEachNode( p, pObj, i )                                        \
    Ivy_ManForEachObj( p, pObj, i ) if ( !Ivy_ObjIsNode(pObj) ) {} else
// iterator over logic latches
#define Ivy_ManForEachLatch( p, pObj, i )                                       \
    Ivy_ManForEachObj( p, pObj, i ) if ( !Ivy_ObjIsLatch(pObj) ) {} else
// iterator over the nodes whose IDs are stored in the array
#define Ivy_ManForEachNodeVec( p, vIds, pObj, i )                               \
    for ( i = 0; i < Vec_IntSize(vIds) && ((pObj) = Ivy_ManObj(p, Vec_IntEntry(vIds,i))); i++ )
// iterator over the fanouts of an object
#define Ivy_ObjForEachFanout( p, pObj, vArray, pFanout, i )                     \
    for ( i = 0, Ivy_ObjCollectFanouts(p, pObj, vArray);                        \
          i < Vec_PtrSize(vArray) && ((pFanout) = (Ivy_Obj_t *)Vec_PtrEntry(vArray,i)); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== ivyBalance.c ========================================================*/
extern Ivy_Man_t *     Ivy_ManBalance( Ivy_Man_t * p, int fUpdateLevel );
extern Ivy_Obj_t *     Ivy_NodeBalanceBuildSuper( Ivy_Man_t * p, Vec_Ptr_t * vSuper, Ivy_Type_t Type, int fUpdateLevel );
/*=== ivyCanon.c ========================================================*/
extern Ivy_Obj_t *     Ivy_CanonAnd( Ivy_Man_t * p, Ivy_Obj_t * p0, Ivy_Obj_t * p1 );
extern Ivy_Obj_t *     Ivy_CanonExor( Ivy_Man_t * p, Ivy_Obj_t * p0, Ivy_Obj_t * p1 );
extern Ivy_Obj_t *     Ivy_CanonLatch( Ivy_Man_t * p, Ivy_Obj_t * pObj, Ivy_Init_t Init );
/*=== ivyCheck.c ========================================================*/
extern int             Ivy_ManCheck( Ivy_Man_t * p );
extern int             Ivy_ManCheckFanoutNums( Ivy_Man_t * p );
extern int             Ivy_ManCheckFanouts( Ivy_Man_t * p );
extern int             Ivy_ManCheckChoices( Ivy_Man_t * p );
/*=== ivyCut.c ==========================================================*/
extern void            Ivy_ManSeqFindCut( Ivy_Man_t * p, Ivy_Obj_t * pNode, Vec_Int_t * vFront, Vec_Int_t * vInside, int nSize );
extern Ivy_Store_t *   Ivy_NodeFindCutsAll( Ivy_Man_t * p, Ivy_Obj_t * pObj, int nLeaves );
/*=== ivyDfs.c ==========================================================*/
extern Vec_Int_t *     Ivy_ManDfs( Ivy_Man_t * p );
extern Vec_Int_t *     Ivy_ManDfsSeq( Ivy_Man_t * p, Vec_Int_t ** pvLatches );
extern void            Ivy_ManCollectCone( Ivy_Obj_t * pObj, Vec_Ptr_t * vFront, Vec_Ptr_t * vCone );
extern Vec_Vec_t *     Ivy_ManLevelize( Ivy_Man_t * p );
extern Vec_Int_t *     Ivy_ManRequiredLevels( Ivy_Man_t * p );
extern int             Ivy_ManIsAcyclic( Ivy_Man_t * p );
extern int             Ivy_ManSetLevels( Ivy_Man_t * p, int fHaig );
/*=== ivyDsd.c ==========================================================*/
extern int             Ivy_TruthDsd( unsigned uTruth, Vec_Int_t * vTree );
extern void            Ivy_TruthDsdPrint( FILE * pFile, Vec_Int_t * vTree );
extern unsigned        Ivy_TruthDsdCompute( Vec_Int_t * vTree );
extern void            Ivy_TruthDsdComputePrint( unsigned uTruth );
extern Ivy_Obj_t *     Ivy_ManDsdConstruct( Ivy_Man_t * p, Vec_Int_t * vFront, Vec_Int_t * vTree );
/*=== ivyFanout.c ==========================================================*/
extern void            Ivy_ManStartFanout( Ivy_Man_t * p );
extern void            Ivy_ManStopFanout( Ivy_Man_t * p );
extern void            Ivy_ObjAddFanout( Ivy_Man_t * p, Ivy_Obj_t * pObj, Ivy_Obj_t * pFanout );
extern void            Ivy_ObjDeleteFanout( Ivy_Man_t * p, Ivy_Obj_t * pObj, Ivy_Obj_t * pFanout );
extern void            Ivy_ObjPatchFanout( Ivy_Man_t * p, Ivy_Obj_t * pObj, Ivy_Obj_t * pFanoutOld, Ivy_Obj_t * pFanoutNew );
extern void            Ivy_ObjCollectFanouts( Ivy_Man_t * p, Ivy_Obj_t * pObj, Vec_Ptr_t * vArray );
extern Ivy_Obj_t *     Ivy_ObjReadFirstFanout( Ivy_Man_t * p, Ivy_Obj_t * pObj );
extern int             Ivy_ObjFanoutNum( Ivy_Man_t * p, Ivy_Obj_t * pObj );
/*=== ivyFastMap.c =============================================================*/
extern void            Ivy_FastMapPerform( Ivy_Man_t * pAig, int nLimit, int fRecovery, int fVerbose );
extern void            Ivy_FastMapStop( Ivy_Man_t * pAig );
extern void            Ivy_FastMapReadSupp( Ivy_Man_t * pAig, Ivy_Obj_t * pObj, Vec_Int_t * vLeaves );
extern void            Ivy_FastMapReverseLevel( Ivy_Man_t * pAig );
/*=== ivyFraig.c ==========================================================*/
extern int             Ivy_FraigProve( Ivy_Man_t ** ppManAig, void * pPars );
extern Ivy_Man_t *     Ivy_FraigPerform( Ivy_Man_t * pManAig, Ivy_FraigParams_t * pParams );
extern Ivy_Man_t *     Ivy_FraigMiter( Ivy_Man_t * pManAig, Ivy_FraigParams_t * pParams );
extern void            Ivy_FraigParamsDefault( Ivy_FraigParams_t * pParams );
/*=== ivyHaig.c ==========================================================*/
extern void            Ivy_ManHaigStart( Ivy_Man_t * p, int fVerbose );
extern void            Ivy_ManHaigTrasfer( Ivy_Man_t * p, Ivy_Man_t * pNew );
extern void            Ivy_ManHaigStop( Ivy_Man_t * p );
extern void            Ivy_ManHaigPostprocess( Ivy_Man_t * p, int fVerbose );
extern void            Ivy_ManHaigCreateObj( Ivy_Man_t * p, Ivy_Obj_t * pObj );
extern void            Ivy_ManHaigCreateChoice( Ivy_Man_t * p, Ivy_Obj_t * pObjOld, Ivy_Obj_t * pObjNew );
extern void            Ivy_ManHaigSimulate( Ivy_Man_t * p );
/*=== ivyMan.c ==========================================================*/
extern Ivy_Man_t *     Ivy_ManStart();
extern Ivy_Man_t *     Ivy_ManStartFrom( Ivy_Man_t * p );
extern Ivy_Man_t *     Ivy_ManDup( Ivy_Man_t * p );
extern Ivy_Man_t *     Ivy_ManFrames( Ivy_Man_t * pMan, int nLatches, int nFrames, int fInit, Vec_Ptr_t ** pvMapping );
extern void            Ivy_ManStop( Ivy_Man_t * p );
extern int             Ivy_ManCleanup( Ivy_Man_t * p );
extern int             Ivy_ManPropagateBuffers( Ivy_Man_t * p, int fUpdateLevel );
extern void            Ivy_ManPrintStats( Ivy_Man_t * p );
extern void            Ivy_ManMakeSeq( Ivy_Man_t * p, int nLatches, int * pInits );
/*=== ivyMem.c ==========================================================*/
extern void            Ivy_ManStartMemory( Ivy_Man_t * p );
extern void            Ivy_ManStopMemory( Ivy_Man_t * p );
/*=== ivyMulti.c ==========================================================*/
extern Ivy_Obj_t *     Ivy_Multi( Ivy_Man_t * p, Ivy_Obj_t ** pArgs, int nArgs, Ivy_Type_t Type );
extern Ivy_Obj_t *     Ivy_Multi1( Ivy_Man_t * p, Ivy_Obj_t ** pArgs, int nArgs, Ivy_Type_t Type );
extern Ivy_Obj_t *     Ivy_Multi_rec( Ivy_Man_t * p, Ivy_Obj_t ** ppObjs, int nObjs, Ivy_Type_t Type );
extern Ivy_Obj_t *     Ivy_MultiBalance_rec( Ivy_Man_t * p, Ivy_Obj_t ** pArgs, int nArgs, Ivy_Type_t Type );
extern int             Ivy_MultiPlus( Ivy_Man_t * p, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vCone, Ivy_Type_t Type, int nLimit, Vec_Ptr_t * vSol );
/*=== ivyObj.c ==========================================================*/
extern Ivy_Obj_t *     Ivy_ObjCreatePi( Ivy_Man_t * p );
extern Ivy_Obj_t *     Ivy_ObjCreatePo( Ivy_Man_t * p, Ivy_Obj_t * pDriver );
extern Ivy_Obj_t *     Ivy_ObjCreate( Ivy_Man_t * p, Ivy_Obj_t * pGhost );
extern void            Ivy_ObjConnect( Ivy_Man_t * p, Ivy_Obj_t * pObj, Ivy_Obj_t * pFan0, Ivy_Obj_t * pFan1 );
extern void            Ivy_ObjDisconnect( Ivy_Man_t * p, Ivy_Obj_t * pObj );
extern void            Ivy_ObjPatchFanin0( Ivy_Man_t * p, Ivy_Obj_t * pObj, Ivy_Obj_t * pFaninNew );
extern void            Ivy_ObjDelete( Ivy_Man_t * p, Ivy_Obj_t * pObj, int fFreeTop );
extern void            Ivy_ObjDelete_rec( Ivy_Man_t * p, Ivy_Obj_t * pObj, int fFreeTop );
extern void            Ivy_ObjReplace( Ivy_Man_t * p, Ivy_Obj_t * pObjOld, Ivy_Obj_t * pObjNew, int fDeleteOld, int fFreeTop, int fUpdateLevel );
extern void            Ivy_NodeFixBufferFanins( Ivy_Man_t * p, Ivy_Obj_t * pNode, int fUpdateLevel );
/*=== ivyOper.c =========================================================*/
extern Ivy_Obj_t *     Ivy_Oper( Ivy_Man_t * p, Ivy_Obj_t * p0, Ivy_Obj_t * p1, Ivy_Type_t Type );
extern Ivy_Obj_t *     Ivy_And( Ivy_Man_t * p, Ivy_Obj_t * p0, Ivy_Obj_t * p1 );
extern Ivy_Obj_t *     Ivy_Or( Ivy_Man_t * p, Ivy_Obj_t * p0, Ivy_Obj_t * p1 );
extern Ivy_Obj_t *     Ivy_Exor( Ivy_Man_t * p, Ivy_Obj_t * p0, Ivy_Obj_t * p1 );
extern Ivy_Obj_t *     Ivy_Mux( Ivy_Man_t * p, Ivy_Obj_t * pC, Ivy_Obj_t * p1, Ivy_Obj_t * p0 );
extern Ivy_Obj_t *     Ivy_Maj( Ivy_Man_t * p, Ivy_Obj_t * pA, Ivy_Obj_t * pB, Ivy_Obj_t * pC );
extern Ivy_Obj_t *     Ivy_Miter( Ivy_Man_t * p, Vec_Ptr_t * vPairs );
extern Ivy_Obj_t *     Ivy_Latch( Ivy_Man_t * p, Ivy_Obj_t * pObj, Ivy_Init_t Init );
/*=== ivyResyn.c =========================================================*/
extern Ivy_Man_t *     Ivy_ManResyn0( Ivy_Man_t * p, int fUpdateLevel, int fVerbose );
extern Ivy_Man_t *     Ivy_ManResyn( Ivy_Man_t * p, int fUpdateLevel, int fVerbose );
extern Ivy_Man_t *     Ivy_ManRwsat( Ivy_Man_t * pMan, int fVerbose );
/*=== ivyRewrite.c =========================================================*/
extern int             Ivy_ManSeqRewrite( Ivy_Man_t * p, int fUpdateLevel, int fUseZeroCost );
extern int             Ivy_ManRewriteAlg( Ivy_Man_t * p, int fUpdateLevel, int fUseZeroCost );
extern int             Ivy_ManRewritePre( Ivy_Man_t * p, int fUpdateLevel, int fUseZeroCost, int fVerbose );
/*=== ivySeq.c =========================================================*/
extern int             Ivy_ManRewriteSeq( Ivy_Man_t * p, int fUseZeroCost, int fVerbose );
/*=== ivyShow.c =========================================================*/
extern void            Ivy_ManShow( Ivy_Man_t * pMan, int fHaig, Vec_Ptr_t * vBold );
/*=== ivyTable.c ========================================================*/
extern Ivy_Obj_t *     Ivy_TableLookup( Ivy_Man_t * p, Ivy_Obj_t * pObj );
extern void            Ivy_TableInsert( Ivy_Man_t * p, Ivy_Obj_t * pObj );
extern void            Ivy_TableDelete( Ivy_Man_t * p, Ivy_Obj_t * pObj );
extern void            Ivy_TableUpdate( Ivy_Man_t * p, Ivy_Obj_t * pObj, int ObjIdNew );
extern int             Ivy_TableCountEntries( Ivy_Man_t * p );
extern void            Ivy_TableProfile( Ivy_Man_t * p );
/*=== ivyUtil.c =========================================================*/
extern void            Ivy_ManIncrementTravId( Ivy_Man_t * p );
extern void            Ivy_ManCleanTravId( Ivy_Man_t * p );
extern unsigned *      Ivy_ManCutTruth( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Vec_Int_t * vLeaves, Vec_Int_t * vNodes, Vec_Int_t * vTruth );
extern void            Ivy_ManCollectCut( Ivy_Man_t * p, Ivy_Obj_t * pRoot, Vec_Int_t * vLeaves, Vec_Int_t * vNodes );
extern Vec_Int_t *     Ivy_ManLatches( Ivy_Man_t * p );
extern int             Ivy_ManLevels( Ivy_Man_t * p );
extern void            Ivy_ManResetLevels( Ivy_Man_t * p );
extern int             Ivy_ObjMffcLabel( Ivy_Man_t * p, Ivy_Obj_t * pObj );
extern void            Ivy_ObjUpdateLevel_rec( Ivy_Man_t * p, Ivy_Obj_t * pObj );
extern void            Ivy_ObjUpdateLevelR_rec( Ivy_Man_t * p, Ivy_Obj_t * pObj, int ReqNew );
extern int             Ivy_ObjIsMuxType( Ivy_Obj_t * pObj );
extern Ivy_Obj_t *     Ivy_ObjRecognizeMux( Ivy_Obj_t * pObj, Ivy_Obj_t ** ppObjT, Ivy_Obj_t ** ppObjE );
extern Ivy_Obj_t *     Ivy_ObjReal( Ivy_Obj_t * pObj );
extern void            Ivy_ObjPrintVerbose( Ivy_Man_t * p, Ivy_Obj_t * pObj, int fHaig );
extern void            Ivy_ManPrintVerbose( Ivy_Man_t * p, int fHaig );
extern int             Ivy_CutTruthPrint( Ivy_Man_t * p, Ivy_Cut_t * pCut, unsigned uTruth );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

