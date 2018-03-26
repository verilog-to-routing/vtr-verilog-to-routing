/**CFile****************************************************************

  FileName    [aig.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aig.h,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__aig__aig_h
#define ABC__aig__aig__aig_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/util/utilCex.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START
 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Aig_Man_t_            Aig_Man_t;
typedef struct Aig_Obj_t_            Aig_Obj_t;
typedef struct Aig_MmFixed_t_        Aig_MmFixed_t;    
typedef struct Aig_MmFlex_t_         Aig_MmFlex_t;     
typedef struct Aig_MmStep_t_         Aig_MmStep_t;     

// object types
typedef enum { 
    AIG_OBJ_NONE,                    // 0: non-existent object
    AIG_OBJ_CONST1,                  // 1: constant 1 
    AIG_OBJ_CI,                      // 2: combinational input
    AIG_OBJ_CO,                      // 3: combinational output
    AIG_OBJ_BUF,                     // 4: buffer node
    AIG_OBJ_AND,                     // 5: AND node
    AIG_OBJ_EXOR,                    // 6: EXOR node
    AIG_OBJ_VOID                     // 7: unused object
} Aig_Type_t;

// the AIG node
struct Aig_Obj_t_  // 8 words
{
    union {                         
        Aig_Obj_t *  pNext;          // strashing table
        int          CioId;          // 0-based number of CI/CO
    };
    Aig_Obj_t *      pFanin0;        // fanin
    Aig_Obj_t *      pFanin1;        // fanin
    unsigned int     Type    :  3;   // object type
    unsigned int     fPhase  :  1;   // value under 000...0 pattern
    unsigned int     fMarkA  :  1;   // multipurpose mask
    unsigned int     fMarkB  :  1;   // multipurpose mask
    unsigned int     nRefs   : 26;   // reference count 
    unsigned         Level   : 24;   // the level of this node
    unsigned         nCuts   :  8;   // the number of cuts
    int              TravId;         // unique ID of last traversal involving the node
    int              Id;             // unique ID of the node
    union {                          // temporary store for user's data
        void *       pData;
        int          iData;
        float        dData;
    };
};

// the AIG manager
struct Aig_Man_t_
{
    char *           pName;          // the design name
    char *           pSpec;          // the input file name
    // AIG nodes
    Vec_Ptr_t *      vCis;           // the array of PIs
    Vec_Ptr_t *      vCos;           // the array of POs
    Vec_Ptr_t *      vObjs;          // the array of all nodes (optional)
    Vec_Ptr_t *      vBufs;          // the array of buffers
    Aig_Obj_t *      pConst1;        // the constant 1 node
    Aig_Obj_t        Ghost;          // the ghost node
    int              nRegs;          // the number of registers (registers are last POs)
    int              nTruePis;       // the number of true primary inputs
    int              nTruePos;       // the number of true primary outputs
    int              nAsserts;       // the number of asserts among POs (asserts are first POs)
    int              nConstrs;       // the number of constraints (model checking only)
    int              nBarBufs;       // the number of barrier buffers
    // AIG node counters
    int              nObjs[AIG_OBJ_VOID];// the number of objects by type
    int              nDeleted;       // the number of deleted objects
    // structural hash table
    Aig_Obj_t **     pTable;         // structural hash table
    int              nTableSize;     // structural hash table size
    // representation of fanouts
    int *            pFanData;       // the database to store fanout information
    int              nFansAlloc;     // the size of fanout representation
    Vec_Vec_t *      vLevels;        // used to update timing information
    int              nBufReplaces;   // the number of times replacement led to a buffer
    int              nBufFixes;      // the number of times buffers were propagated
    int              nBufMax;        // the maximum number of buffers during computation
    // topological order
    unsigned *       pOrderData;
    int              nOrderAlloc;
    int              iPrev;
    int              iNext;
    int              nAndTotal;
    int              nAndPrev;
    // representatives
    Aig_Obj_t **     pEquivs;        // linked list of equivalent nodes (when choices are used)
    Aig_Obj_t **     pReprs;         // representatives of each node
    int              nReprsAlloc;    // the number of allocated representatives
    // various data members
    Aig_MmFixed_t *  pMemObjs;       // memory manager for objects
    Vec_Int_t *      vLevelR;        // the reverse level of the nodes
    int              nLevelMax;      // maximum number of levels
    void *           pData;          // the temporary data
    void *           pData2;         // the temporary data
    int              nTravIds;       // the current traversal ID
    int              fCatchExor;     // enables EXOR nodes
    int              fAddStrash;     // performs additional strashing
    Aig_Obj_t **     pObjCopies;     // mapping of AIG nodes into FRAIG nodes
    void (*pImpFunc) (void*, void*); // implication checking precedure
    void *           pImpData;       // implication checking data
    void *           pManTime;       // the timing manager
    void *           pManCuts;
    int *            pFastSim; 
    unsigned *       pTerSimData;    // ternary simulation data
    Vec_Ptr_t *      vMapped;
    Vec_Int_t *      vFlopNums;      
    Vec_Int_t *      vFlopReprs;      
    Abc_Cex_t *      pSeqModel;
    Vec_Ptr_t *      vSeqModelVec;   // vector of counter-examples (for sequential miters)
    Aig_Man_t *      pManExdc;
    Vec_Ptr_t *      vOnehots;
    int              fCreatePios;
    Vec_Int_t *      vEquPairs;   
    Vec_Vec_t *      vClockDoms; 
    Vec_Int_t *      vProbs;         // probability of node being 1 
    Vec_Int_t *      vCiNumsOrig;    // original CI names
    int              nComplEdges;    // complemented edges
    abctime          Time2Quit;
    // timing statistics
    abctime          time1;
    abctime          time2;
  //-- jlong -- begin
  Vec_Ptr_t *      unfold2_type_I;
  Vec_Ptr_t *      unfold2_type_II;
  //-- jlong -- end
};

// cut computation
typedef struct Aig_ManCut_t_         Aig_ManCut_t;
typedef struct Aig_Cut_t_            Aig_Cut_t;

// the cut used to represent node in the AIG
struct Aig_Cut_t_
{
    Aig_Cut_t *     pNext;           // the next cut in the table 
    int             Cost;            // the cost of the cut
    unsigned        uSign;           // cut signature
    int             iNode;           // the node, for which it is the cut
    short           nCutSize;        // the number of bytes in the cut
    char            nLeafMax;        // the maximum number of fanins
    char            nFanins;         // the current number of fanins
    int             pFanins[0];      // the fanins (followed by the truth table)
};

// the CNF computation manager
struct Aig_ManCut_t_
{
    // AIG manager
    Aig_Man_t *     pAig;            // the input AIG manager
    Aig_Cut_t **    pCuts;           // the cuts for each node in the output manager
    // parameters
    int             nCutsMax;        // the max number of cuts at the node
    int             nLeafMax;        // the max number of leaves of a cut
    int             fTruth;          // enables truth table computation
    int             fVerbose;        // enables verbose output
    // internal variables
    int             nCutSize;        // the number of bytes needed to store one cut
    int             nTruthWords;     // the number of truth table words
    Aig_MmFixed_t * pMemCuts;        // memory manager for cuts
    unsigned *      puTemp[4];       // used for the truth table computation
};

static inline Aig_Cut_t *  Aig_ObjCuts( Aig_ManCut_t * p, Aig_Obj_t * pObj )                         { return p->pCuts[pObj->Id];  }
static inline void         Aig_ObjSetCuts( Aig_ManCut_t * p, Aig_Obj_t * pObj, Aig_Cut_t * pCuts )   { p->pCuts[pObj->Id] = pCuts; }

static inline int          Aig_CutLeaveNum( Aig_Cut_t * pCut )          { return pCut->nFanins;                                    }
static inline int *        Aig_CutLeaves( Aig_Cut_t * pCut )            { return pCut->pFanins;                                    }
static inline unsigned *   Aig_CutTruth( Aig_Cut_t * pCut )             { return (unsigned *)(pCut->pFanins + pCut->nLeafMax);     }
static inline Aig_Cut_t *  Aig_CutNext( Aig_Cut_t * pCut )              { return (Aig_Cut_t *)(((char *)pCut) + pCut->nCutSize);   }

// iterator over cuts of the node
#define Aig_ObjForEachCut( p, pObj, pCut, i )                           \
    for ( i = 0, pCut = Aig_ObjCuts(p, pObj); i < p->nCutsMax; i++, pCut = Aig_CutNext(pCut) ) 
// iterator over leaves of the cut
#define Aig_CutForEachLeaf( p, pCut, pLeaf, i )                         \
    for ( i = 0; (i < (int)(pCut)->nFanins) && ((pLeaf) = Aig_ManObj(p, (pCut)->pFanins[i])); i++ )

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline unsigned     Aig_ObjCutSign( unsigned ObjId )       { return (1 << (ObjId & 31));                            }
static inline int          Aig_WordCountOnes( unsigned uWord )
{
    uWord = (uWord & 0x55555555) + ((uWord>>1) & 0x55555555);
    uWord = (uWord & 0x33333333) + ((uWord>>2) & 0x33333333);
    uWord = (uWord & 0x0F0F0F0F) + ((uWord>>4) & 0x0F0F0F0F);
    uWord = (uWord & 0x00FF00FF) + ((uWord>>8) & 0x00FF00FF);
    return  (uWord & 0x0000FFFF) + (uWord>>16);
}
static inline int          Aig_WordFindFirstBit( unsigned uWord )
{
    int i;
    for ( i = 0; i < 32; i++ )
        if ( uWord & (1 << i) )
            return i;
    return -1;
}

static inline Aig_Obj_t *  Aig_Regular( Aig_Obj_t * p )           { return (Aig_Obj_t *)((ABC_PTRUINT_T)(p) & ~01);  }
static inline Aig_Obj_t *  Aig_Not( Aig_Obj_t * p )               { return (Aig_Obj_t *)((ABC_PTRUINT_T)(p) ^  01);  }
static inline Aig_Obj_t *  Aig_NotCond( Aig_Obj_t * p, int c )    { return (Aig_Obj_t *)((ABC_PTRUINT_T)(p) ^ (c));  }
static inline int          Aig_IsComplement( Aig_Obj_t * p )      { return (int)((ABC_PTRUINT_T)(p) & 01);           }

static inline int          Aig_ManCiNum( Aig_Man_t * p )          { return p->nObjs[AIG_OBJ_CI];                     }
static inline int          Aig_ManCoNum( Aig_Man_t * p )          { return p->nObjs[AIG_OBJ_CO];                     }
static inline int          Aig_ManBufNum( Aig_Man_t * p )         { return p->nObjs[AIG_OBJ_BUF];                    }
static inline int          Aig_ManAndNum( Aig_Man_t * p )         { return p->nObjs[AIG_OBJ_AND];                    }
static inline int          Aig_ManExorNum( Aig_Man_t * p )        { return p->nObjs[AIG_OBJ_EXOR];                   }
static inline int          Aig_ManNodeNum( Aig_Man_t * p )        { return p->nObjs[AIG_OBJ_AND]+p->nObjs[AIG_OBJ_EXOR];   }
static inline int          Aig_ManGetCost( Aig_Man_t * p )        { return p->nObjs[AIG_OBJ_AND]+3*p->nObjs[AIG_OBJ_EXOR]; }
static inline int          Aig_ManObjNum( Aig_Man_t * p )         { return Vec_PtrSize(p->vObjs) - p->nDeleted;      }
static inline int          Aig_ManObjNumMax( Aig_Man_t * p )      { return Vec_PtrSize(p->vObjs);                    }
static inline int          Aig_ManRegNum( Aig_Man_t * p )         { return p->nRegs;                                 }
static inline int          Aig_ManConstrNum( Aig_Man_t * p )      { return p->nConstrs;                              }

static inline Aig_Obj_t *  Aig_ManConst0( Aig_Man_t * p )         { return Aig_Not(p->pConst1);                      }
static inline Aig_Obj_t *  Aig_ManConst1( Aig_Man_t * p )         { return p->pConst1;                               }
static inline Aig_Obj_t *  Aig_ManGhost( Aig_Man_t * p )          { return &p->Ghost;                                }
static inline Aig_Obj_t *  Aig_ManCi( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vCis, i);    }
static inline Aig_Obj_t *  Aig_ManCo( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vCos, i);    }
static inline Aig_Obj_t *  Aig_ManLo( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vCis, Aig_ManCiNum(p)-Aig_ManRegNum(p)+i);   }
static inline Aig_Obj_t *  Aig_ManLi( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vCos, Aig_ManCoNum(p)-Aig_ManRegNum(p)+i);   }
static inline Aig_Obj_t *  Aig_ManObj( Aig_Man_t * p, int i )     { return p->vObjs ? (Aig_Obj_t *)Vec_PtrEntry(p->vObjs, i) : NULL;  }

static inline Aig_Type_t   Aig_ObjType( Aig_Obj_t * pObj )        { return (Aig_Type_t)pObj->Type;       }
static inline int          Aig_ObjIsNone( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_NONE;   }
static inline int          Aig_ObjIsConst1( Aig_Obj_t * pObj )    { assert(!Aig_IsComplement(pObj)); return pObj->Type == AIG_OBJ_CONST1; }
static inline int          Aig_ObjIsCi( Aig_Obj_t * pObj )        { return pObj->Type == AIG_OBJ_CI;     }
static inline int          Aig_ObjIsCo( Aig_Obj_t * pObj )        { return pObj->Type == AIG_OBJ_CO;     }
static inline int          Aig_ObjIsBuf( Aig_Obj_t * pObj )       { return pObj->Type == AIG_OBJ_BUF;    }
static inline int          Aig_ObjIsAnd( Aig_Obj_t * pObj )       { return pObj->Type == AIG_OBJ_AND;    }
static inline int          Aig_ObjIsExor( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_EXOR;   }
static inline int          Aig_ObjIsNode( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_AND || pObj->Type == AIG_OBJ_EXOR;   }
static inline int          Aig_ObjIsTerm( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_CI  || pObj->Type == AIG_OBJ_CO || pObj->Type == AIG_OBJ_CONST1;   }
static inline int          Aig_ObjIsHash( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_AND || pObj->Type == AIG_OBJ_EXOR;                                 }
static inline int          Aig_ObjIsChoice( Aig_Man_t * p, Aig_Obj_t * pObj )    { return p->pEquivs && p->pEquivs[pObj->Id] && pObj->nRefs > 0;                    }
static inline int          Aig_ObjIsCand( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_CI || pObj->Type == AIG_OBJ_AND || pObj->Type == AIG_OBJ_EXOR;     }
static inline int          Aig_ObjCioId( Aig_Obj_t * pObj )       { assert( !Aig_ObjIsNode(pObj) ); return pObj->CioId;                                            }
static inline int          Aig_ObjId( Aig_Obj_t * pObj )          { return pObj->Id;                     }

static inline int          Aig_ObjIsMarkA( Aig_Obj_t * pObj )     { return pObj->fMarkA;  }
static inline void         Aig_ObjSetMarkA( Aig_Obj_t * pObj )    { pObj->fMarkA = 1;     }
static inline void         Aig_ObjClearMarkA( Aig_Obj_t * pObj )  { pObj->fMarkA = 0;     }
 
static inline void         Aig_ObjSetTravId( Aig_Obj_t * pObj, int TravId )                { pObj->TravId = TravId;                         }
static inline void         Aig_ObjSetTravIdCurrent( Aig_Man_t * p, Aig_Obj_t * pObj )      { pObj->TravId = p->nTravIds;                    }
static inline void         Aig_ObjSetTravIdPrevious( Aig_Man_t * p, Aig_Obj_t * pObj )     { pObj->TravId = p->nTravIds - 1;                }
static inline int          Aig_ObjIsTravIdCurrent( Aig_Man_t * p, Aig_Obj_t * pObj )       { return (int)(pObj->TravId == p->nTravIds);     }
static inline int          Aig_ObjIsTravIdPrevious( Aig_Man_t * p, Aig_Obj_t * pObj )      { return (int)(pObj->TravId == p->nTravIds - 1); }

static inline int          Aig_ObjPhase( Aig_Obj_t * pObj )       { return pObj->fPhase;                           }
static inline int          Aig_ObjPhaseReal( Aig_Obj_t * pObj )   { return pObj? Aig_Regular(pObj)->fPhase ^ Aig_IsComplement(pObj) : 1;                              }
static inline int          Aig_ObjRefs( Aig_Obj_t * pObj )        { return pObj->nRefs;                            }
static inline void         Aig_ObjRef( Aig_Obj_t * pObj )         { pObj->nRefs++;                                 }
static inline void         Aig_ObjDeref( Aig_Obj_t * pObj )       { assert( pObj->nRefs > 0 ); pObj->nRefs--;      }
static inline void         Aig_ObjClearRef( Aig_Obj_t * pObj )    { pObj->nRefs = 0;                               }
static inline int          Aig_ObjFaninId0( Aig_Obj_t * pObj )    { return pObj->pFanin0? Aig_Regular(pObj->pFanin0)->Id : -1; }
static inline int          Aig_ObjFaninId1( Aig_Obj_t * pObj )    { return pObj->pFanin1? Aig_Regular(pObj->pFanin1)->Id : -1; }
static inline int          Aig_ObjFaninC0( Aig_Obj_t * pObj )     { return Aig_IsComplement(pObj->pFanin0);        }
static inline int          Aig_ObjFaninC1( Aig_Obj_t * pObj )     { return Aig_IsComplement(pObj->pFanin1);        }
static inline Aig_Obj_t *  Aig_ObjFanin0( Aig_Obj_t * pObj )      { return Aig_Regular(pObj->pFanin0);             }
static inline Aig_Obj_t *  Aig_ObjFanin1( Aig_Obj_t * pObj )      { return Aig_Regular(pObj->pFanin1);             }
static inline Aig_Obj_t *  Aig_ObjChild0( Aig_Obj_t * pObj )      { return pObj->pFanin0;                          }
static inline Aig_Obj_t *  Aig_ObjChild1( Aig_Obj_t * pObj )      { return pObj->pFanin1;                          }
static inline Aig_Obj_t *  Aig_ObjChild0Copy( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t *  Aig_ObjChild1Copy( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj)) : NULL;  }
static inline Aig_Obj_t *  Aig_ObjChild0Next( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj)->pNext, Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t *  Aig_ObjChild1Next( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin1(pObj)->pNext, Aig_ObjFaninC1(pObj)) : NULL;  }
static inline void         Aig_ObjChild0Flip( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); pObj->pFanin0 = Aig_Not(pObj->pFanin0);        }
static inline void         Aig_ObjChild1Flip( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); pObj->pFanin1 = Aig_Not(pObj->pFanin1);        }
static inline Aig_Obj_t *  Aig_ObjCopy( Aig_Obj_t * pObj )        { assert( !Aig_IsComplement(pObj) ); return (Aig_Obj_t *)pObj->pData;               } 
static inline void         Aig_ObjSetCopy( Aig_Obj_t * pObj, Aig_Obj_t * pCopy )     {  assert( !Aig_IsComplement(pObj) ); pObj->pData = pCopy;       } 
static inline Aig_Obj_t *  Aig_ObjRealCopy( Aig_Obj_t * pObj )    { return Aig_NotCond((Aig_Obj_t *)Aig_Regular(pObj)->pData, Aig_IsComplement(pObj));}
static inline int          Aig_ObjToLit( Aig_Obj_t * pObj )       { return Abc_Var2Lit( Aig_ObjId(Aig_Regular(pObj)), Aig_IsComplement(pObj) );       }
static inline Aig_Obj_t *  Aig_ObjFromLit( Aig_Man_t * p,int iLit){ return Aig_NotCond( Aig_ManObj(p, Abc_Lit2Var(iLit)), Abc_LitIsCompl(iLit) );     }
static inline int          Aig_ObjLevel( Aig_Obj_t * pObj )       { assert( !Aig_IsComplement(pObj) ); return pObj->Level;                            }
static inline int          Aig_ObjLevelNew( Aig_Obj_t * pObj )    { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? 1 + Aig_ObjIsExor(pObj) + Abc_MaxInt(Aig_ObjFanin0(pObj)->Level, Aig_ObjFanin1(pObj)->Level) : Aig_ObjFanin0(pObj)->Level; }
static inline int          Aig_ObjSetLevel( Aig_Obj_t * pObj, int i ) { assert( !Aig_IsComplement(pObj) ); return pObj->Level = i;                    }
static inline void         Aig_ObjClean( Aig_Obj_t * pObj )       { memset( pObj, 0, sizeof(Aig_Obj_t) );                                                             }
static inline Aig_Obj_t *  Aig_ObjFanout0( Aig_Man_t * p, Aig_Obj_t * pObj )  { assert(p->pFanData && pObj->Id < p->nFansAlloc); return Aig_ManObj(p, p->pFanData[5*pObj->Id] >> 1); } 
static inline Aig_Obj_t *  Aig_ObjEquiv( Aig_Man_t * p, Aig_Obj_t * pObj )    { return p->pEquivs? p->pEquivs[pObj->Id] : NULL;           } 
static inline void         Aig_ObjSetEquiv( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pEqu ) { assert(p->pEquivs); p->pEquivs[pObj->Id] = pEqu;                  }
static inline Aig_Obj_t *  Aig_ObjRepr( Aig_Man_t * p, Aig_Obj_t * pObj )     { return p->pReprs? p->pReprs[pObj->Id] : NULL;             } 
static inline void         Aig_ObjSetRepr( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pRepr )     { assert(p->pReprs); p->pReprs[pObj->Id] = pRepr;                                } 
static inline int          Aig_ObjWhatFanin( Aig_Obj_t * pObj, Aig_Obj_t * pFanin )    
{ 
    if ( Aig_ObjFanin0(pObj) == pFanin ) return 0; 
    if ( Aig_ObjFanin1(pObj) == pFanin ) return 1; 
    assert(0); return -1; 
}
static inline int          Aig_ObjFanoutC( Aig_Obj_t * pObj, Aig_Obj_t * pFanout )    
{ 
    if ( Aig_ObjFanin0(pFanout) == pObj ) return Aig_ObjFaninC0(pObj); 
    if ( Aig_ObjFanin1(pFanout) == pObj ) return Aig_ObjFaninC1(pObj); 
    assert(0); return -1; 
}

// create the ghost of the new node
static inline Aig_Obj_t *  Aig_ObjCreateGhost( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1, Aig_Type_t Type )    
{
    Aig_Obj_t * pGhost;
    assert( Type != AIG_OBJ_AND || !Aig_ObjIsConst1(Aig_Regular(p0)) );
    assert( p1 == NULL || !Aig_ObjIsConst1(Aig_Regular(p1)) );
    assert( Type == AIG_OBJ_CI || Aig_Regular(p0) != Aig_Regular(p1) );
    pGhost = Aig_ManGhost(p);
    pGhost->Type = Type;
    if ( p1 == NULL || Aig_Regular(p0)->Id < Aig_Regular(p1)->Id )
    {
        pGhost->pFanin0 = p0;
        pGhost->pFanin1 = p1;
    }
    else
    {
        pGhost->pFanin0 = p1;
        pGhost->pFanin1 = p0;
    }
    return pGhost;
}

// internal memory manager
static inline Aig_Obj_t * Aig_ManFetchMemory( Aig_Man_t * p )  
{
    extern char * Aig_MmFixedEntryFetch( Aig_MmFixed_t * p );
    Aig_Obj_t * pTemp;
    pTemp = (Aig_Obj_t *)Aig_MmFixedEntryFetch( p->pMemObjs );
    memset( pTemp, 0, sizeof(Aig_Obj_t) ); 
    pTemp->Id = Vec_PtrSize(p->vObjs);
    Vec_PtrPush( p->vObjs, pTemp );
    return pTemp;
}
static inline void Aig_ManRecycleMemory( Aig_Man_t * p, Aig_Obj_t * pEntry )
{
    extern void Aig_MmFixedEntryRecycle( Aig_MmFixed_t * p, char * pEntry );
    assert( pEntry->nRefs == 0 );
    pEntry->Type = AIG_OBJ_NONE; // distinquishes a dead node from a live node
    Aig_MmFixedEntryRecycle( p->pMemObjs, (char *)pEntry );
    p->nDeleted++;
}


////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

// iterator over the combinational inputs
#define Aig_ManForEachCi( p, pObj, i )                                          \
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vCis, pObj, i )
#define Aig_ManForEachCiReverse( p, pObj, i )                                   \
    Vec_PtrForEachEntryReverse( Aig_Obj_t *, p->vCis, pObj, i )
// iterator over the combinational outputs
#define Aig_ManForEachCo( p, pObj, i )                                          \
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vCos, pObj, i )
#define Aig_ManForEachCoReverse( p, pObj, i )                                   \
    Vec_PtrForEachEntryReverse( Aig_Obj_t *, p->vCos, pObj, i )
// iterators over all objects, including those currently not used
#define Aig_ManForEachObj( p, pObj, i )                                         \
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vObjs, pObj, i ) if ( (pObj) == NULL ) {} else
#define Aig_ManForEachObjReverse( p, pObj, i )                                  \
    Vec_PtrForEachEntryReverse( Aig_Obj_t *, p->vObjs, pObj, i ) if ( (pObj) == NULL ) {} else
// iterators over the objects whose IDs are stored in an array
#define Aig_ManForEachObjVec( vIds, p, pObj, i )                                \
    for ( i = 0; i < Vec_IntSize(vIds) && (((pObj) = Aig_ManObj(p, Vec_IntEntry(vIds,i))), 1); i++ )
#define Aig_ManForEachObjVecReverse( vIds, p, pObj, i )                         \
    for ( i = Vec_IntSize(vIds) - 1; i >= 0 && (((pObj) = Aig_ManObj(p, Vec_IntEntry(vIds,i))), 1); i-- )
// iterators over all nodes
#define Aig_ManForEachNode( p, pObj, i )                                        \
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vObjs, pObj, i ) if ( (pObj) == NULL || !Aig_ObjIsNode(pObj) ) {} else
#define Aig_ManForEachNodeReverse( p, pObj, i )                                 \
    Vec_PtrForEachEntryReverse( Aig_Obj_t *, p->vObjs, pObj, i ) if ( (pObj) == NULL || !Aig_ObjIsNode(pObj) ) {} else
// iterator over all nodes
#define Aig_ManForEachExor( p, pObj, i )                                        \
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vObjs, pObj, i ) if ( (pObj) == NULL || !Aig_ObjIsExor(pObj) ) {} else
#define Aig_ManForEachExorReverse( p, pObj, i )                                 \
    Vec_PtrForEachEntryReverse( Aig_Obj_t *, p->vObjs, pObj, i ) if ( (pObj) == NULL || !Aig_ObjIsExor(pObj) ) {} else

// these two procedures are only here for the use inside the iterator
static inline int     Aig_ObjFanout0Int( Aig_Man_t * p, int ObjId )  { assert(ObjId < p->nFansAlloc);  return p->pFanData[5*ObjId];                         }
static inline int     Aig_ObjFanoutNext( Aig_Man_t * p, int iFan )   { assert(iFan/2 < p->nFansAlloc); return p->pFanData[5*(iFan >> 1) + 3 + (iFan & 1)];  }
// iterator over the fanouts
#define Aig_ObjForEachFanout( p, pObj, pFanout, iFan, i )                       \
    for ( assert(p->pFanData), i = 0; (i < (int)(pObj)->nRefs) &&               \
          (((iFan) = i? Aig_ObjFanoutNext(p, iFan) : Aig_ObjFanout0Int(p, pObj->Id)), 1) && \
          (((pFanout) = Aig_ManObj(p, iFan>>1)), 1); i++ )


////////////////////////////////////////////////////////////////////////
///                     SEQUENTIAL ITERATORS                         ///
////////////////////////////////////////////////////////////////////////

// iterator over the primary inputs
#define Aig_ManForEachPiSeq( p, pObj, i )                                       \
    Vec_PtrForEachEntryStop( Aig_Obj_t *, p->vCis, pObj, i, Aig_ManCiNum(p)-Aig_ManRegNum(p) )
// iterator over the latch outputs
#define Aig_ManForEachLoSeq( p, pObj, i )                                       \
    Vec_PtrForEachEntryStart( Aig_Obj_t *, p->vCis, pObj, i, Aig_ManCiNum(p)-Aig_ManRegNum(p) )
// iterator over the primary outputs
#define Aig_ManForEachPoSeq( p, pObj, i )                                       \
    Vec_PtrForEachEntryStop( Aig_Obj_t *, p->vCos, pObj, i, Aig_ManCoNum(p)-Aig_ManRegNum(p) )
// iterator over the latch inputs
#define Aig_ManForEachLiSeq( p, pObj, i )                                       \
    Vec_PtrForEachEntryStart( Aig_Obj_t *, p->vCos, pObj, i, Aig_ManCoNum(p)-Aig_ManRegNum(p) )
// iterator over the latch input and outputs
#define Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, k )                           \
    for ( k = 0; (k < Aig_ManRegNum(p)) && (((pObjLi) = Aig_ManLi(p, k)), 1)    \
        && (((pObjLo)=Aig_ManLo(p, k)), 1); k++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== aigCheck.c ========================================================*/
extern ABC_DLL int     Aig_ManCheck( Aig_Man_t * p );
extern void            Aig_ManCheckMarkA( Aig_Man_t * p );
extern void            Aig_ManCheckPhase( Aig_Man_t * p );
/*=== aigCuts.c ========================================================*/
extern Aig_ManCut_t *  Aig_ComputeCuts( Aig_Man_t * pAig, int nCutsMax, int nLeafMax, int fTruth, int fVerbose );
extern void            Aig_ManCutStop( Aig_ManCut_t * p );
/*=== aigDfs.c ==========================================================*/
extern int             Aig_ManVerifyTopoOrder( Aig_Man_t * p );
extern Vec_Ptr_t *     Aig_ManDfs( Aig_Man_t * p, int fNodesOnly );
extern Vec_Ptr_t *     Aig_ManDfsAll( Aig_Man_t * p );
extern Vec_Ptr_t *     Aig_ManDfsPreorder( Aig_Man_t * p, int fNodesOnly );
extern Vec_Vec_t *     Aig_ManLevelize( Aig_Man_t * p );
extern Vec_Ptr_t *     Aig_ManDfsNodes( Aig_Man_t * p, Aig_Obj_t ** ppNodes, int nNodes );
extern Vec_Ptr_t *     Aig_ManDfsChoices( Aig_Man_t * p );
extern Vec_Ptr_t *     Aig_ManDfsReverse( Aig_Man_t * p );
extern int             Aig_ManLevelNum( Aig_Man_t * p );
extern int             Aig_ManChoiceLevel( Aig_Man_t * p );
extern int             Aig_DagSize( Aig_Obj_t * pObj );
extern int             Aig_SupportSize( Aig_Man_t * p, Aig_Obj_t * pObj );
extern Vec_Ptr_t *     Aig_Support( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_SupportNodes( Aig_Man_t * p, Aig_Obj_t ** ppObjs, int nObjs, Vec_Ptr_t * vSupp );
extern void            Aig_ConeUnmark_rec( Aig_Obj_t * pObj );
extern Aig_Obj_t *     Aig_Transfer( Aig_Man_t * pSour, Aig_Man_t * pDest, Aig_Obj_t * pObj, int nVars );
extern Aig_Obj_t *     Aig_Compose( Aig_Man_t * p, Aig_Obj_t * pRoot, Aig_Obj_t * pFunc, int iVar );
extern void            Aig_ObjCollectCut( Aig_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vNodes );
extern int             Aig_ObjCollectSuper( Aig_Obj_t * pObj, Vec_Ptr_t * vSuper );
/*=== aigDup.c ==========================================================*/
extern Aig_Obj_t *     Aig_ManDupSimpleDfs_rec( Aig_Man_t * pNew, Aig_Man_t * p, Aig_Obj_t * pObj );
extern Aig_Man_t *     Aig_ManDupSimple( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupSimpleWithHints( Aig_Man_t * p, Vec_Int_t * vHints );
extern Aig_Man_t *     Aig_ManDupSimpleDfs( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupSimpleDfsPart( Aig_Man_t * p, Vec_Ptr_t * vPis, Vec_Ptr_t * vCos );
extern Aig_Man_t *     Aig_ManDupOrdered( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupCof( Aig_Man_t * p, int iInput, int Value );
extern Aig_Man_t *     Aig_ManDupTrim( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupExor( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupDfs( Aig_Man_t * p );
extern Vec_Ptr_t *     Aig_ManOrderPios( Aig_Man_t * p, Aig_Man_t * pOrder );
extern Aig_Man_t *     Aig_ManDupDfsGuided( Aig_Man_t * p, Vec_Ptr_t * vPios );
extern Aig_Man_t *     Aig_ManDupLevelized( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupWithoutPos( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupFlopsOnly( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupRepres( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupRepresDfs( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManCreateMiter( Aig_Man_t * p1, Aig_Man_t * p2, int fImpl );
extern Aig_Man_t *     Aig_ManDupOrpos( Aig_Man_t * p, int fAddRegs );
extern Aig_Man_t *     Aig_ManDupOneOutput( Aig_Man_t * p, int iPoNum, int fAddRegs );
extern Aig_Man_t *     Aig_ManDupUnsolvedOutputs( Aig_Man_t * p, int fAddRegs );
extern Aig_Man_t *     Aig_ManDupArray( Vec_Ptr_t * vArray );
extern Aig_Man_t *     Aig_ManDupNodes( Aig_Man_t * pMan, Vec_Ptr_t * vArray );
/*=== aigFanout.c ==========================================================*/
extern void            Aig_ObjAddFanout( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFanout );
extern void            Aig_ObjRemoveFanout( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFanout );
extern void            Aig_ManFanoutStart( Aig_Man_t * p );
extern void            Aig_ManFanoutStop( Aig_Man_t * p );
/*=== aigFrames.c ==========================================================*/
extern Aig_Man_t *     Aig_ManFrames( Aig_Man_t * pAig, int nFs, int fInit, int fOuts, int fRegs, int fEnlarge, Aig_Obj_t *** ppObjMap );
/*=== aigMan.c ==========================================================*/
extern Aig_Man_t *     Aig_ManStart( int nNodesMax );
extern Aig_Man_t *     Aig_ManStartFrom( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManExtractMiter( Aig_Man_t * p, Aig_Obj_t * pNode1, Aig_Obj_t * pNode2 );
extern void            Aig_ManStop( Aig_Man_t * p );
extern void            Aig_ManStopP( Aig_Man_t ** p );
extern int             Aig_ManCleanup( Aig_Man_t * p );
extern int             Aig_ManAntiCleanup( Aig_Man_t * p );
extern int             Aig_ManCiCleanup( Aig_Man_t * p );
extern int             Aig_ManCoCleanup( Aig_Man_t * p );
extern void            Aig_ManPrintStats( Aig_Man_t * p );
extern void            Aig_ManReportImprovement( Aig_Man_t * p, Aig_Man_t * pNew );
extern void            Aig_ManSetRegNum( Aig_Man_t * p, int nRegs );
extern void            Aig_ManFlipFirstPo( Aig_Man_t * p );
extern void *          Aig_ManReleaseData( Aig_Man_t * p );
/*=== aigMem.c ==========================================================*/
extern void            Aig_ManStartMemory( Aig_Man_t * p );
extern void            Aig_ManStopMemory( Aig_Man_t * p );
/*=== aigMffc.c ==========================================================*/
extern int             Aig_NodeRef_rec( Aig_Obj_t * pNode, unsigned LevelMin );
extern int             Aig_NodeDeref_rec( Aig_Obj_t * pNode, unsigned LevelMin, float * pPower, float * pProbs );
extern int             Aig_NodeMffcSupp( Aig_Man_t * p, Aig_Obj_t * pNode, int LevelMin, Vec_Ptr_t * vSupp );
extern int             Aig_NodeMffcLabel( Aig_Man_t * p, Aig_Obj_t * pNode, float * pPower );
extern int             Aig_NodeMffcLabelCut( Aig_Man_t * p, Aig_Obj_t * pNode, Vec_Ptr_t * vLeaves );
extern int             Aig_NodeMffcExtendCut( Aig_Man_t * p, Aig_Obj_t * pNode, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vResult );
/*=== aigObj.c ==========================================================*/
extern Aig_Obj_t *     Aig_ObjCreateCi( Aig_Man_t * p );
extern Aig_Obj_t *     Aig_ObjCreateCo( Aig_Man_t * p, Aig_Obj_t * pDriver );
extern Aig_Obj_t *     Aig_ObjCreate( Aig_Man_t * p, Aig_Obj_t * pGhost );
extern void            Aig_ObjConnect( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFan0, Aig_Obj_t * pFan1 );
extern void            Aig_ObjDisconnect( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_ObjDelete( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_ObjDelete_rec( Aig_Man_t * p, Aig_Obj_t * pObj, int fFreeTop );
extern void            Aig_ObjDeletePo( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_ObjPrint( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_ObjPatchFanin0( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFaninNew );
extern void            Aig_ObjReplace( Aig_Man_t * p, Aig_Obj_t * pObjOld, Aig_Obj_t * pObjNew, int fUpdateLevel );
/*=== aigOper.c =========================================================*/
extern Aig_Obj_t *     Aig_IthVar( Aig_Man_t * p, int i );
extern Aig_Obj_t *     Aig_Oper( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1, Aig_Type_t Type );
extern Aig_Obj_t *     Aig_And( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 );
extern Aig_Obj_t *     Aig_Or( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 );
extern Aig_Obj_t *     Aig_Exor( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 );
extern Aig_Obj_t *     Aig_Mux( Aig_Man_t * p, Aig_Obj_t * pC, Aig_Obj_t * p1, Aig_Obj_t * p0 );
extern Aig_Obj_t *     Aig_Maj( Aig_Man_t * p, Aig_Obj_t * pA, Aig_Obj_t * pB, Aig_Obj_t * pC );
extern Aig_Obj_t *     Aig_Multi( Aig_Man_t * p, Aig_Obj_t ** pArgs, int nArgs, Aig_Type_t Type );
extern Aig_Obj_t *     Aig_Miter( Aig_Man_t * p, Vec_Ptr_t * vPairs );
extern Aig_Obj_t *     Aig_MiterTwo( Aig_Man_t * p, Vec_Ptr_t * vNodes1, Vec_Ptr_t * vNodes2 );
extern Aig_Obj_t *     Aig_CreateAnd( Aig_Man_t * p, int nVars );
extern Aig_Obj_t *     Aig_CreateOr( Aig_Man_t * p, int nVars );
extern Aig_Obj_t *     Aig_CreateExor( Aig_Man_t * p, int nVars );
/*=== aigOrder.c =========================================================*/
extern void            Aig_ManOrderStart( Aig_Man_t * p );
extern void            Aig_ManOrderStop( Aig_Man_t * p );
extern void            Aig_ObjOrderInsert( Aig_Man_t * p, int ObjId );
extern void            Aig_ObjOrderRemove( Aig_Man_t * p, int ObjId );
extern void            Aig_ObjOrderAdvance( Aig_Man_t * p );
/*=== aigPart.c =========================================================*/
extern Vec_Ptr_t *     Aig_ManSupports( Aig_Man_t * p );
extern Vec_Ptr_t *     Aig_ManSupportsInverse( Aig_Man_t * p );
extern Vec_Ptr_t *     Aig_ManSupportsRegisters( Aig_Man_t * p );
extern Vec_Ptr_t *     Aig_ManPartitionSmart( Aig_Man_t * p, int nPartSizeLimit, int fVerbose, Vec_Ptr_t ** pvPartSupps );
extern Vec_Ptr_t *     Aig_ManPartitionSmartRegisters( Aig_Man_t * pAig, int nSuppSizeLimit, int fVerbose );
extern Vec_Ptr_t *     Aig_ManPartitionNaive( Aig_Man_t * p, int nPartSize );
extern Vec_Ptr_t *     Aig_ManMiterPartitioned( Aig_Man_t * p1, Aig_Man_t * p2, int nPartSize, int fSmart );
extern Aig_Man_t *     Aig_ManChoicePartitioned( Vec_Ptr_t * vAigs, int nPartSize, int nConfMax, int nLevelMax, int fVerbose );
extern Aig_Man_t *     Aig_ManFraigPartitioned( Aig_Man_t * pAig, int nPartSize, int nConfMax, int nLevelMax, int fVerbose );
extern Aig_Man_t *     Aig_ManChoiceConstructive( Vec_Ptr_t * vAigs, int fVerbose );
/*=== aigPartReg.c =========================================================*/
extern Vec_Ptr_t *     Aig_ManRegPartitionSimple( Aig_Man_t * pAig, int nPartSize, int nOverSize );
extern void            Aig_ManPartDivide( Vec_Ptr_t * vResult, Vec_Int_t * vDomain, int nPartSize, int nOverSize );
extern Vec_Ptr_t *     Aig_ManRegPartitionSmart( Aig_Man_t * pAig, int nPartSize );
extern Aig_Man_t *     Aig_ManRegCreatePart( Aig_Man_t * pAig, Vec_Int_t * vPart, int * pnCountPis, int * pnCountRegs, int ** ppMapBack );
extern Vec_Ptr_t *     Aig_ManRegProjectOnehots( Aig_Man_t * pAig, Aig_Man_t * pPart, Vec_Ptr_t * vOnehots, int fVerbose );
/*=== aigRepr.c =========================================================*/
extern void            Aig_ManReprStart( Aig_Man_t * p, int nIdMax );
extern void            Aig_ManReprStop( Aig_Man_t * p );
extern void            Aig_ObjCreateRepr( Aig_Man_t * p, Aig_Obj_t * pNode1, Aig_Obj_t * pNode2 );
extern void            Aig_ManTransferRepr( Aig_Man_t * pNew, Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupRepr( Aig_Man_t * p, int fOrdered );
extern Aig_Man_t *     Aig_ManDupReprBasic( Aig_Man_t * p );
extern int             Aig_ManCountReprs( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManRehash( Aig_Man_t * p );
extern int             Aig_ObjCheckTfi( Aig_Man_t * p, Aig_Obj_t * pNew, Aig_Obj_t * pOld );
extern void            Aig_ManMarkValidChoices( Aig_Man_t * p );
extern int             Aig_TransferMappedClasses( Aig_Man_t * pAig, Aig_Man_t * pPart, int * pMapBack );
/*=== aigRet.c ========================================================*/
extern Aig_Man_t *     Rtm_ManRetime( Aig_Man_t * p, int fForward, int nStepsMax, int fVerbose );
/*=== aigRetF.c ========================================================*/
extern Aig_Man_t *     Aig_ManRetimeFrontier( Aig_Man_t * p, int nStepsMax );
/*=== aigScl.c ==========================================================*/
extern Aig_Man_t *     Aig_ManRemap( Aig_Man_t * p, Vec_Ptr_t * vMap );
extern int             Aig_ManSeqCleanup( Aig_Man_t * p );
extern int             Aig_ManSeqCleanupBasic( Aig_Man_t * p );
extern int             Aig_ManCountMergeRegs( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManReduceLaches( Aig_Man_t * p, int fVerbose );
extern void            Aig_ManComputeSccs( Aig_Man_t * p ); 
extern Aig_Man_t *     Aig_ManScl( Aig_Man_t * pAig, int fLatchConst, int fLatchEqual, int fUseMvSweep, int nFramesSymb, int nFramesSatur, int fVerbose, int fVeryVerbose );
/*=== aigShow.c ========================================================*/
extern void            Aig_ManShow( Aig_Man_t * pMan, int fHaig, Vec_Ptr_t * vBold );
/*=== aigTable.c ========================================================*/
extern Aig_Obj_t *     Aig_TableLookup( Aig_Man_t * p, Aig_Obj_t * pGhost );
extern Aig_Obj_t *     Aig_TableLookupTwo( Aig_Man_t * p, Aig_Obj_t * pFanin0, Aig_Obj_t * pFanin1 );
extern void            Aig_TableInsert( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_TableDelete( Aig_Man_t * p, Aig_Obj_t * pObj );
extern int             Aig_TableCountEntries( Aig_Man_t * p );
extern void            Aig_TableProfile( Aig_Man_t * p );
extern void            Aig_TableClear( Aig_Man_t * p );
/*=== aigTiming.c ========================================================*/
extern void            Aig_ObjClearReverseLevel( Aig_Man_t * p, Aig_Obj_t * pObj );
extern int             Aig_ObjRequiredLevel( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_ManStartReverseLevels( Aig_Man_t * p, int nMaxLevelIncrease );
extern void            Aig_ManStopReverseLevels( Aig_Man_t * p );
extern void            Aig_ManUpdateLevel( Aig_Man_t * p, Aig_Obj_t * pObjNew );
extern void            Aig_ManUpdateReverseLevel( Aig_Man_t * p, Aig_Obj_t * pObjNew );
extern void            Aig_ManVerifyLevel( Aig_Man_t * p );
extern void            Aig_ManVerifyReverseLevel( Aig_Man_t * p );
/*=== aigTruth.c ========================================================*/
extern unsigned *      Aig_ManCutTruth( Aig_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vNodes, Vec_Ptr_t * vTruthElem, Vec_Ptr_t * vTruthStore );
/*=== aigTsim.c ========================================================*/
extern Aig_Man_t *     Aig_ManConstReduce( Aig_Man_t * p, int fUseMvSweep, int nFramesSymb, int nFramesSatur, int fVerbose, int fVeryVerbose );
/*=== aigUtil.c =========================================================*/
extern void            Aig_ManIncrementTravId( Aig_Man_t * p );
extern char *          Aig_TimeStamp();
extern int             Aig_ManHasNoGaps( Aig_Man_t * p );
extern int             Aig_ManLevels( Aig_Man_t * p );
extern void            Aig_ManResetRefs( Aig_Man_t * p );
extern void            Aig_ManCleanMarkA( Aig_Man_t * p );
extern void            Aig_ManCleanMarkB( Aig_Man_t * p );
extern void            Aig_ManCleanMarkAB( Aig_Man_t * p );
extern void            Aig_ManCleanData( Aig_Man_t * p );
extern void            Aig_ObjCleanData_rec( Aig_Obj_t * pObj );
extern void            Aig_ManCleanNext( Aig_Man_t * p );
extern void            Aig_ObjCollectMulti( Aig_Obj_t * pFunc, Vec_Ptr_t * vSuper );
extern int             Aig_ObjIsMuxType( Aig_Obj_t * pObj );
extern int             Aig_ObjRecognizeExor( Aig_Obj_t * pObj, Aig_Obj_t ** ppFan0, Aig_Obj_t ** ppFan1 );
extern Aig_Obj_t *     Aig_ObjRecognizeMux( Aig_Obj_t * pObj, Aig_Obj_t ** ppObjT, Aig_Obj_t ** ppObjE );
extern Aig_Obj_t *     Aig_ObjReal_rec( Aig_Obj_t * pObj );
extern int             Aig_ObjCompareIdIncrease( Aig_Obj_t ** pp1, Aig_Obj_t ** pp2 );
extern void            Aig_ObjPrintEqn( FILE * pFile, Aig_Obj_t * pObj, Vec_Vec_t * vLevels, int Level );
extern void            Aig_ObjPrintVerilog( FILE * pFile, Aig_Obj_t * pObj, Vec_Vec_t * vLevels, int Level );
extern void            Aig_ObjPrintVerbose( Aig_Obj_t * pObj, int fHaig );
extern void            Aig_ManPrintVerbose( Aig_Man_t * p, int fHaig );
extern void            Aig_ManDump( Aig_Man_t * p );
extern void            Aig_ManDumpBlif( Aig_Man_t * p, char * pFileName, Vec_Ptr_t * vPiNames, Vec_Ptr_t * vPoNames );
extern void            Aig_ManDumpVerilog( Aig_Man_t * p, char * pFileName );
extern void            Aig_ManSetCioIds( Aig_Man_t * p );
extern void            Aig_ManCleanCioIds( Aig_Man_t * p );
extern int             Aig_ManChoiceNum( Aig_Man_t * p );
extern char *          Aig_FileNameGenericAppend( char * pBase, char * pSuffix );
extern unsigned        Aig_ManRandom( int fReset );
extern word            Aig_ManRandom64( int fReset );
extern void            Aig_ManRandomInfo( Vec_Ptr_t * vInfo, int iInputStart, int iWordStart, int iWordStop );
extern void            Aig_NodeUnionLists( Vec_Ptr_t * vArr1, Vec_Ptr_t * vArr2, Vec_Ptr_t * vArr );
extern void            Aig_NodeIntersectLists( Vec_Ptr_t * vArr1, Vec_Ptr_t * vArr2, Vec_Ptr_t * vArr );
extern void            Aig_ManSetPhase( Aig_Man_t * pAig );
extern Vec_Ptr_t *     Aig_ManMuxesCollect( Aig_Man_t * pAig );
extern void            Aig_ManMuxesDeref( Aig_Man_t * pAig, Vec_Ptr_t * vMuxes );
extern void            Aig_ManMuxesRef( Aig_Man_t * pAig, Vec_Ptr_t * vMuxes );
extern void            Aig_ManInvertConstraints( Aig_Man_t * pAig );

/*=== aigWin.c =========================================================*/
extern void            Aig_ManFindCut( Aig_Obj_t * pRoot, Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited, int nSizeLimit, int nFanoutLimit );
 
/*=== aigMem.c ===========================================================*/
// fixed-size-block memory manager
extern Aig_MmFixed_t * Aig_MmFixedStart( int nEntrySize, int nEntriesMax );
extern void            Aig_MmFixedStop( Aig_MmFixed_t * p, int fVerbose );
extern char *          Aig_MmFixedEntryFetch( Aig_MmFixed_t * p );
extern void            Aig_MmFixedEntryRecycle( Aig_MmFixed_t * p, char * pEntry );
extern void            Aig_MmFixedRestart( Aig_MmFixed_t * p );
extern int             Aig_MmFixedReadMemUsage( Aig_MmFixed_t * p );
extern int             Aig_MmFixedReadMaxEntriesUsed( Aig_MmFixed_t * p );
// flexible-size-block memory manager
extern Aig_MmFlex_t *  Aig_MmFlexStart();
extern void            Aig_MmFlexStop( Aig_MmFlex_t * p, int fVerbose );
extern char *          Aig_MmFlexEntryFetch( Aig_MmFlex_t * p, int nBytes );
extern void            Aig_MmFlexRestart( Aig_MmFlex_t * p );
extern int             Aig_MmFlexReadMemUsage( Aig_MmFlex_t * p );
// hierarchical memory manager
extern Aig_MmStep_t *  Aig_MmStepStart( int nSteps );
extern void            Aig_MmStepStop( Aig_MmStep_t * p, int fVerbose );
extern char *          Aig_MmStepEntryFetch( Aig_MmStep_t * p, int nBytes );
extern void            Aig_MmStepEntryRecycle( Aig_MmStep_t * p, char * pEntry, int nBytes );
extern int             Aig_MmStepReadMemUsage( Aig_MmStep_t * p );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

