/**CFile****************************************************************

  FileName    [gia.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__gia__gia_h
#define ABC__aig__gia__gia_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/vec/vecWec.h"
#include "misc/util/utilCex.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_START

#define GIA_NONE 0x1FFFFFFF
#define GIA_VOID 0x0FFFFFFF

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Gia_MmFixed_t_        Gia_MmFixed_t;    
typedef struct Gia_MmFlex_t_         Gia_MmFlex_t;     
typedef struct Gia_MmStep_t_         Gia_MmStep_t;     

typedef struct Gia_Rpr_t_ Gia_Rpr_t;
struct Gia_Rpr_t_
{
    unsigned       iRepr   : 28;  // representative node
    unsigned       fProved :  1;  // marks the proved equivalence
    unsigned       fFailed :  1;  // marks the failed equivalence
    unsigned       fColorA :  1;  // marks cone of A
    unsigned       fColorB :  1;  // marks cone of B
};

typedef struct Gia_Plc_t_ Gia_Plc_t;
struct Gia_Plc_t_
{
    unsigned       fFixed  :  1;  // the placement of this object is fixed
    unsigned       xCoord  : 15;  // x-ooordinate of the placement
    unsigned       fUndef  :  1;  // the placement of this object is not assigned
    unsigned       yCoord  : 15;  // y-ooordinate of the placement
};

typedef struct Gia_Obj_t_ Gia_Obj_t;
struct Gia_Obj_t_
{
    unsigned       iDiff0 :  29;  // the diff of the first fanin
    unsigned       fCompl0:   1;  // the complemented attribute
    unsigned       fMark0 :   1;  // first user-controlled mark
    unsigned       fTerm  :   1;  // terminal node (CI/CO)

    unsigned       iDiff1 :  29;  // the diff of the second fanin
    unsigned       fCompl1:   1;  // the complemented attribute
    unsigned       fMark1 :   1;  // second user-controlled mark
    unsigned       fPhase :   1;  // value under 000 pattern

    unsigned       Value;         // application-specific value
};
// Value is currently used to store several types of information
// - pointer to the next node in the hash table during structural hashing
// - pointer to the node copy during duplication 

// new AIG manager
typedef struct Gia_Man_t_ Gia_Man_t;
struct Gia_Man_t_
{
    char *         pName;         // name of the AIG
    char *         pSpec;         // name of the input file
    int            nRegs;         // number of registers
    int            nRegsAlloc;    // number of allocated registers
    int            nObjs;         // number of objects
    int            nObjsAlloc;    // number of allocated objects
    Gia_Obj_t *    pObjs;         // the array of objects
    unsigned *     pMuxes;        // control signals of MUXes
    int            nXors;         // the number of XORs
    int            nMuxes;        // the number of MUXes 
    int            nBufs;         // the number of buffers
    Vec_Int_t *    vCis;          // the vector of CIs (PIs + LOs)
    Vec_Int_t *    vCos;          // the vector of COs (POs + LIs)
    Vec_Int_t      vHash;         // hash links
    Vec_Int_t      vHTable;       // hash table
    int            fAddStrash;    // performs additional structural hashing
    int            fSweeper;      // sweeper is running
    int            fGiaSimple;    // simple mode (no const-propagation and strashing)
    Vec_Int_t      vRefs;         // the reference count
    int *          pRefs;         // the reference count
    int *          pLutRefs;      // the reference count
    Vec_Int_t *    vLevels;       // levels of the nodes
    int            nLevels;       // the mamixum level
    int            nConstrs;      // the number of constraints
    int            nTravIds;      // the current traversal ID
    int            nFront;        // frontier size 
    int *          pReprsOld;     // representatives (for CIs and ANDs)
    Gia_Rpr_t *    pReprs;        // representatives (for CIs and ANDs)
    int *          pNexts;        // next nodes in the equivalence classes
    int *          pSibls;        // next nodes in the choice nodes
    int *          pIso;          // pairs of structurally isomorphic nodes
    int            nTerLoop;      // the state where loop begins  
    int            nTerStates;    // the total number of ternary states
    int *          pFanData;      // the database to store fanout information
    int            nFansAlloc;    // the size of fanout representation
    Vec_Int_t *    vFanoutNums;   // static fanout
    Vec_Int_t *    vFanout;       // static fanout
    Vec_Int_t *    vMapping;      // mapping for each node
    Vec_Wec_t *    vMapping2;     // mapping for each node
    Vec_Wec_t *    vFanouts2;     // mapping fanouts 
    Vec_Int_t *    vCellMapping;  // mapping for each node
    void *         pSatlutWinman; // windowing for SAT-based mapping
    Vec_Int_t *    vPacking;      // packing information
    Vec_Int_t *    vConfigs;      // cell configurations
    char *         pCellStr;      // cell description
    Vec_Int_t *    vLutConfigs;   // LUT configurations
    Vec_Int_t *    vEdgeDelay;    // special edge information
    Vec_Int_t *    vEdgeDelayR;   // special edge information
    Vec_Int_t *    vEdge1;        // special edge information
    Vec_Int_t *    vEdge2;        // special edge information
    Abc_Cex_t *    pCexComb;      // combinational counter-example
    Abc_Cex_t *    pCexSeq;       // sequential counter-example
    Vec_Ptr_t *    vSeqModelVec;  // sequential counter-examples
    Vec_Int_t      vCopies;       // intermediate copies
    Vec_Int_t      vCopies2;      // intermediate copies
    Vec_Int_t *    vVar2Obj;      // mapping of variables into objects
    Vec_Int_t *    vTruths;       // used for truth table computation
    Vec_Int_t *    vFlopClasses;  // classes of flops for retiming/merging/etc
    Vec_Int_t *    vGateClasses;  // classes of gates for abstraction
    Vec_Int_t *    vObjClasses;   // classes of objects for abstraction
    Vec_Int_t *    vInitClasses;  // classes of flops for retiming/merging/etc
    Vec_Int_t *    vRegClasses;   // classes of registers for sequential synthesis
    Vec_Int_t *    vRegInits;     // initial state
    Vec_Int_t *    vDoms;         // dominators
    Vec_Int_t *    vBarBufs;      // barrier buffers
    Vec_Int_t *    vXors;         // temporary XORs
    unsigned char* pSwitching;    // switching activity for each object
    Gia_Plc_t *    pPlacement;    // placement of the objects
    Gia_Man_t *    pAigExtra;     // combinational logic of holes
    Vec_Flt_t *    vInArrs;       // PI arrival times
    Vec_Flt_t *    vOutReqs;      // PO required times
    Vec_Int_t *    vCiArrs;       // CI arrival times
    Vec_Int_t *    vCoReqs;       // CO required times
    Vec_Int_t *    vCoArrs;       // CO arrival times
    Vec_Int_t *    vCoAttrs;      // CO attributes
    int            And2Delay;     // delay of the AND gate 
    float          DefInArrs;     // default PI arrival times
    float          DefOutReqs;    // default PO required times
    Vec_Int_t *    vSwitching;    // switching activity
    int *          pTravIds;      // separate traversal ID representation
    int            nTravIdsAlloc; // the number of trav IDs allocated
    Vec_Ptr_t *    vNamesIn;      // the input names 
    Vec_Ptr_t *    vNamesOut;     // the output names
    Vec_Int_t *    vUserPiIds;    // numbers assigned to PIs by the user
    Vec_Int_t *    vUserPoIds;    // numbers assigned to POs by the user
    Vec_Int_t *    vUserFfIds;    // numbers assigned to FFs by the user
    Vec_Int_t *    vCiNumsOrig;   // original CI names
    Vec_Int_t *    vCoNumsOrig;   // original CO names
    Vec_Int_t *    vIdsOrig;      // original object IDs
    Vec_Int_t *    vIdsEquiv;     // original object IDs proved equivalent
    Vec_Int_t *    vCofVars;      // cofactoring variables
    Vec_Vec_t *    vClockDoms;    // clock domains
    Vec_Flt_t *    vTiming;       // arrival/required/slack
    void *         pManTime;      // the timing manager
    void *         pLutLib;       // LUT library
    word           nHashHit;      // hash table hit
    word           nHashMiss;     // hash table miss
    void *         pData;         // various user data
    unsigned *     pData2;        // various user data
    int            iData;         // various user data
    int            iData2;        // various user data
    int            nAnd2Delay;    // AND2 delay scaled to match delay numbers used
    int            fVerbose;      // verbose reports
    int            MappedArea;    // area after mapping
    int            MappedDelay;   // delay after mapping
    // bit-parallel simulation
    int            fBuiltInSim;
    int            iPatsPi;
    int            nSimWords;
    int            iPastPiMax;
    int            nSimWordsMax;
    Vec_Wrd_t *    vSims;
    Vec_Wrd_t *    vSimsPi;
    Vec_Int_t *    vClassOld;
    Vec_Int_t *    vClassNew;
    // incremental simulation
    int            fIncrSim;
    int            iNextPi;
    int            iTimeStamp;
    Vec_Int_t *    vTimeStamps;
    // truth table computation for small functions
    int            nTtVars;       // truth table variables
    int            nTtWords;      // truth table words
    Vec_Int_t *    vTtNums;       // object numbers
    Vec_Int_t *    vTtNodes;      // internal nodes
    Vec_Ptr_t *    vTtInputs;     // truth tables for constant and primary inputs
    Vec_Wrd_t *    vTtMemory;     // truth tables for internal nodes
    // balancing
    Vec_Int_t *    vSuper;        // supergate
    Vec_Int_t *    vStore;        // node storage  
    // existential quantification
    int            iSuppPi;       // the number of support variables
    int            nSuppWords;    // the number of support words
    Vec_Wrd_t *    vSuppWords;    // support information
    Vec_Int_t      vCopiesTwo;    // intermediate copies
    Vec_Int_t      vSuppVars;     // used variables
};


typedef struct Gps_Par_t_ Gps_Par_t;
struct Gps_Par_t_
{
    int            fTents;
    int            fSwitch;
    int            fCut;
    int            fNpn;
    int            fLutProf;
    int            fMuxXor;
    int            fMiter;
    int            fSkipMap;
    int            fSlacks;
    char *         pDumpFile;
};

typedef struct Emb_Par_t_ Emb_Par_t;
struct Emb_Par_t_
{
    int            nDims;         // the number of dimension
    int            nSols;         // the number of solutions (typically, 2)
    int            nIters;        // the number of iterations of FORCE
    int            fRefine;       // use refinement by FORCE
    int            fCluster;      // use clustered representation 
    int            fDump;         // dump Gnuplot file
    int            fDumpLarge;    // dump Gnuplot file for large benchmarks
    int            fShowImage;    // shows image if Gnuplot is installed
    int            fVerbose;      // verbose flag  
};


// frames parameters
typedef struct Gia_ParFra_t_ Gia_ParFra_t;
struct Gia_ParFra_t_
{
    int            nFrames;       // the number of frames to unroll
    int            fInit;         // initialize the timeframes
    int            fSaveLastLit;  // adds POs for outputs of each frame
    int            fDisableSt;    // disables strashing
    int            fOrPos;        // ORs respective POs in each timeframe
    int            fVerbose;      // enables verbose output
};


// simulation parameters
typedef struct Gia_ParSim_t_ Gia_ParSim_t;
struct Gia_ParSim_t_
{
    // user-controlled parameters
    int            nWords;        // the number of machine words
    int            nIters;        // the number of timeframes
    int            RandSeed;      // seed to generate random numbers
    int            TimeLimit;     // time limit in seconds
    int            fCheckMiter;   // check if miter outputs are non-zero
    int            fVerbose;      // enables verbose output
    int            iOutFail;      // index of the failed output
};

typedef struct Gia_ManSim_t_ Gia_ManSim_t;
struct Gia_ManSim_t_
{
    Gia_Man_t *    pAig;
    Gia_ParSim_t * pPars; 
    int            nWords;
    Vec_Int_t *    vCis2Ids;
    Vec_Int_t *    vConsts;
    // simulation information
    unsigned *     pDataSim;     // simulation data
    unsigned *     pDataSimCis;  // simulation data for CIs
    unsigned *     pDataSimCos;  // simulation data for COs
};

typedef struct Jf_Par_t_ Jf_Par_t; 
struct Jf_Par_t_
{
    int            nLutSize;
    int            nCutNum;
    int            nProcNum;
    int            nRounds;
    int            nRoundsEla;
    int            nRelaxRatio;
    int            nCoarseLimit;
    int            nAreaTuner;
    int            nReqTimeFlex;
    int            nVerbLimit;
    int            nDelayLut1;
    int            nDelayLut2;
    int            nFastEdges;
    int            DelayTarget;
    int            fAreaOnly;
    int            fPinPerm;
    int            fPinQuick;
    int            fPinFilter;
    int            fOptEdge;
    int            fUseMux7;
    int            fPower;
    int            fCoarsen;
    int            fCutMin;
    int            fFuncDsd;
    int            fGenCnf;
    int            fCnfObjIds;
    int            fAddOrCla;
    int            fCnfMapping;
    int            fPureAig;
    int            fDoAverage;
    int            fCutHashing;
    int            fCutSimple;
    int            fCutGroup;
    int            fVerbose;
    int            fVeryVerbose;
    int            nLutSizeMax;
    int            nCutNumMax;
    int            nProcNumMax;
    int            nLutSizeMux;
    word           Delay;
    word           Area;
    word           Edge;
    word           Clause;
    word           Mux7;
    word           WordMapDelay;
    word           WordMapArea;
    word           WordMapDelayTarget;
    float          MapDelay;
    float          MapArea;
    float          MapAreaF;
    float          MapDelayTarget;
    float          Epsilon;
    float *        pTimesArr;
    float *        pTimesReq;
};

static inline unsigned     Gia_ObjCutSign( unsigned ObjId )       { return (1 << (ObjId & 31));                                 }
static inline int          Gia_WordHasOneBit( unsigned uWord )    { return (uWord & (uWord-1)) == 0;                            }
static inline int          Gia_WordHasOnePair( unsigned uWord )   { return Gia_WordHasOneBit(uWord & (uWord>>1) & 0x55555555);  }
static inline int          Gia_WordCountOnes( unsigned uWord )
{
    uWord = (uWord & 0x55555555) + ((uWord>>1) & 0x55555555);
    uWord = (uWord & 0x33333333) + ((uWord>>2) & 0x33333333);
    uWord = (uWord & 0x0F0F0F0F) + ((uWord>>4) & 0x0F0F0F0F);
    uWord = (uWord & 0x00FF00FF) + ((uWord>>8) & 0x00FF00FF);
    return  (uWord & 0x0000FFFF) + (uWord>>16);
}
static inline int Gia_WordFindFirstBit( unsigned uWord )
{
    int i;
    for ( i = 0; i < 32; i++ )
        if ( uWord & (1 << i) )
            return i;
    return -1;
}

static inline int Gia_ManTruthIsConst0( unsigned * pIn, int nVars )
{
    int w;
    for ( w = Abc_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn[w] )
            return 0;
    return 1;
}
static inline int Gia_ManTruthIsConst1( unsigned * pIn, int nVars )
{
    int w;
    for ( w = Abc_TruthWordNum(nVars)-1; w >= 0; w-- )
        if ( pIn[w] != ~(unsigned)0 )
            return 0;
    return 1;
}
static inline void Gia_ManTruthCopy( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Abc_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn[w];
}
static inline void Gia_ManTruthClear( unsigned * pOut, int nVars )
{
    int w;
    for ( w = Abc_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = 0;
}
static inline void Gia_ManTruthFill( unsigned * pOut, int nVars )
{
    int w;
    for ( w = Abc_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~(unsigned)0;
}
static inline void Gia_ManTruthNot( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Abc_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~pIn[w];
}

static inline int          Gia_ManConst0Lit()                  { return 0; }
static inline int          Gia_ManConst1Lit()                  { return 1; }
static inline int          Gia_ManIsConst0Lit( int iLit )      { return (iLit == 0); }
static inline int          Gia_ManIsConst1Lit( int iLit )      { return (iLit == 1); }
static inline int          Gia_ManIsConstLit( int iLit )       { return (iLit <= 1); }

static inline Gia_Obj_t *  Gia_Regular( Gia_Obj_t * p )        { return (Gia_Obj_t *)((ABC_PTRUINT_T)(p) & ~01);                           }
static inline Gia_Obj_t *  Gia_Not( Gia_Obj_t * p )            { return (Gia_Obj_t *)((ABC_PTRUINT_T)(p) ^  01);                           }
static inline Gia_Obj_t *  Gia_NotCond( Gia_Obj_t * p, int c ) { return (Gia_Obj_t *)((ABC_PTRUINT_T)(p) ^ (c));                           }
static inline int          Gia_IsComplement( Gia_Obj_t * p )   { return (int)((ABC_PTRUINT_T)(p) & 01);                                    }

static inline char *       Gia_ManName( Gia_Man_t * p )        { return p->pName;                                                          }
static inline int          Gia_ManCiNum( Gia_Man_t * p )       { return Vec_IntSize(p->vCis);                                              }
static inline int          Gia_ManCoNum( Gia_Man_t * p )       { return Vec_IntSize(p->vCos);                                              }
static inline int          Gia_ManPiNum( Gia_Man_t * p )       { return Vec_IntSize(p->vCis) - p->nRegs;                                   }
static inline int          Gia_ManPoNum( Gia_Man_t * p )       { return Vec_IntSize(p->vCos) - p->nRegs;                                   }
static inline int          Gia_ManRegNum( Gia_Man_t * p )      { return p->nRegs;                                                          }
static inline int          Gia_ManObjNum( Gia_Man_t * p )      { return p->nObjs;                                                          }
static inline int          Gia_ManAndNum( Gia_Man_t * p )      { return p->nObjs - Vec_IntSize(p->vCis) - Vec_IntSize(p->vCos) - 1;        }
static inline int          Gia_ManXorNum( Gia_Man_t * p )      { return p->nXors;                                                          }
static inline int          Gia_ManMuxNum( Gia_Man_t * p )      { return p->nMuxes;                                                         }
static inline int          Gia_ManBufNum( Gia_Man_t * p )      { return p->nBufs;                                                          }
static inline int          Gia_ManAndNotBufNum( Gia_Man_t * p ){ return Gia_ManAndNum(p) - Gia_ManBufNum(p);                               }
static inline int          Gia_ManCandNum( Gia_Man_t * p )     { return Gia_ManCiNum(p) + Gia_ManAndNum(p);                                }
static inline int          Gia_ManConstrNum( Gia_Man_t * p )   { return p->nConstrs;                                                       }
static inline void         Gia_ManFlipVerbose( Gia_Man_t * p ) { p->fVerbose ^= 1;                                                         } 
static inline int          Gia_ManHasChoices( Gia_Man_t * p )  { return p->pSibls != NULL;                                                 } 
static inline int          Gia_ManChoiceNum( Gia_Man_t * p )   { int c = 0; if (p->pSibls) { int i; for (i = 0; i < p->nObjs; i++) c += (int)(p->pSibls[i] > 0); } return c; } 

static inline Gia_Obj_t *  Gia_ManConst0( Gia_Man_t * p )      { return p->pObjs;                                                          }
static inline Gia_Obj_t *  Gia_ManConst1( Gia_Man_t * p )      { return Gia_Not(Gia_ManConst0(p));                                         }
static inline Gia_Obj_t *  Gia_ManObj( Gia_Man_t * p, int v )  { assert( v >= 0 && v < p->nObjs ); return p->pObjs + v;                    }
static inline Gia_Obj_t *  Gia_ManCi( Gia_Man_t * p, int v )   { return Gia_ManObj( p, Vec_IntEntry(p->vCis,v) );                          }
static inline Gia_Obj_t *  Gia_ManCo( Gia_Man_t * p, int v )   { return Gia_ManObj( p, Vec_IntEntry(p->vCos,v) );                          }
static inline Gia_Obj_t *  Gia_ManPi( Gia_Man_t * p, int v )   { assert( v < Gia_ManPiNum(p) );  return Gia_ManCi( p, v );                 }
static inline Gia_Obj_t *  Gia_ManPo( Gia_Man_t * p, int v )   { assert( v < Gia_ManPoNum(p) );  return Gia_ManCo( p, v );                 }
static inline Gia_Obj_t *  Gia_ManRo( Gia_Man_t * p, int v )   { assert( v < Gia_ManRegNum(p) ); return Gia_ManCi( p, Gia_ManPiNum(p)+v ); }
static inline Gia_Obj_t *  Gia_ManRi( Gia_Man_t * p, int v )   { assert( v < Gia_ManRegNum(p) ); return Gia_ManCo( p, Gia_ManPoNum(p)+v ); }

static inline int          Gia_ObjId( Gia_Man_t * p, Gia_Obj_t * pObj )        { assert( p->pObjs <= pObj && pObj < p->pObjs + p->nObjs ); return pObj - p->pObjs; }
static inline int          Gia_ObjCioId( Gia_Obj_t * pObj )                    { assert( pObj->fTerm ); return pObj->iDiff1;                 }
static inline void         Gia_ObjSetCioId( Gia_Obj_t * pObj, int v )          { assert( pObj->fTerm ); pObj->iDiff1 = v;                    }
static inline int          Gia_ObjValue( Gia_Obj_t * pObj )                    { return pObj->Value;                                         }
static inline void         Gia_ObjSetValue( Gia_Obj_t * pObj, int i )          { pObj->Value = i;                                            }
static inline int          Gia_ObjPhase( Gia_Obj_t * pObj )                    { return pObj->fPhase;                                        }
static inline int          Gia_ObjPhaseReal( Gia_Obj_t * pObj )                { return Gia_Regular(pObj)->fPhase ^ Gia_IsComplement(pObj);  }

static inline int          Gia_ObjIsTerm( Gia_Obj_t * pObj )                   { return pObj->fTerm;                             } 
static inline int          Gia_ObjIsAndOrConst0( Gia_Obj_t * pObj )            { return!pObj->fTerm;                             } 
static inline int          Gia_ObjIsCi( Gia_Obj_t * pObj )                     { return pObj->fTerm && pObj->iDiff0 == GIA_NONE; } 
static inline int          Gia_ObjIsCo( Gia_Obj_t * pObj )                     { return pObj->fTerm && pObj->iDiff0 != GIA_NONE; } 
static inline int          Gia_ObjIsAnd( Gia_Obj_t * pObj )                    { return!pObj->fTerm && pObj->iDiff0 != GIA_NONE; } 
static inline int          Gia_ObjIsXor( Gia_Obj_t * pObj )                    { return Gia_ObjIsAnd(pObj) && pObj->iDiff0 < pObj->iDiff1; } 
static inline int          Gia_ObjIsMuxId( Gia_Man_t * p, int iObj )           { return p->pMuxes && p->pMuxes[iObj] > 0;        } 
static inline int          Gia_ObjIsMux( Gia_Man_t * p, Gia_Obj_t * pObj )     { return Gia_ObjIsMuxId( p, Gia_ObjId(p, pObj) ); } 
static inline int          Gia_ObjIsAndReal( Gia_Man_t * p, Gia_Obj_t * pObj ) { return Gia_ObjIsAnd(pObj) && pObj->iDiff0 > pObj->iDiff1 && !Gia_ObjIsMux(p, pObj); } 
static inline int          Gia_ObjIsBuf( Gia_Obj_t * pObj )                    { return pObj->iDiff0 == pObj->iDiff1 && pObj->iDiff0 != GIA_NONE && !pObj->fTerm;    } 
static inline int          Gia_ObjIsAndNotBuf( Gia_Obj_t * pObj )              { return Gia_ObjIsAnd(pObj) && pObj->iDiff0 != pObj->iDiff1; } 
static inline int          Gia_ObjIsCand( Gia_Obj_t * pObj )                   { return Gia_ObjIsAnd(pObj) || Gia_ObjIsCi(pObj); } 
static inline int          Gia_ObjIsConst0( Gia_Obj_t * pObj )                 { return pObj->iDiff0 == GIA_NONE && pObj->iDiff1 == GIA_NONE;     } 
static inline int          Gia_ManObjIsConst0( Gia_Man_t * p, Gia_Obj_t * pObj){ return pObj == p->pObjs;                        } 

static inline int          Gia_Obj2Lit( Gia_Man_t * p, Gia_Obj_t * pObj )      { return Abc_Var2Lit(Gia_ObjId(p, Gia_Regular(pObj)), Gia_IsComplement(pObj)); }
static inline Gia_Obj_t *  Gia_Lit2Obj( Gia_Man_t * p, int iLit )              { return Gia_NotCond(Gia_ManObj(p, Abc_Lit2Var(iLit)), Abc_LitIsCompl(iLit));  }
static inline int          Gia_ManCiLit( Gia_Man_t * p, int CiId )             { return Gia_Obj2Lit( p, Gia_ManCi(p, CiId) );                }

static inline int          Gia_ManIdToCioId( Gia_Man_t * p, int Id )           { return Gia_ObjCioId( Gia_ManObj(p, Id) );                   }
static inline int          Gia_ManCiIdToId( Gia_Man_t * p, int CiId )          { return Gia_ObjId( p, Gia_ManCi(p, CiId) );                  }
static inline int          Gia_ManCoIdToId( Gia_Man_t * p, int CoId )          { return Gia_ObjId( p, Gia_ManCo(p, CoId) );                  }

static inline int          Gia_ObjIsPi( Gia_Man_t * p, Gia_Obj_t * pObj )      { return Gia_ObjIsCi(pObj) && Gia_ObjCioId(pObj) < Gia_ManPiNum(p);   } 
static inline int          Gia_ObjIsPo( Gia_Man_t * p, Gia_Obj_t * pObj )      { return Gia_ObjIsCo(pObj) && Gia_ObjCioId(pObj) < Gia_ManPoNum(p);   } 
static inline int          Gia_ObjIsRo( Gia_Man_t * p, Gia_Obj_t * pObj )      { return Gia_ObjIsCi(pObj) && Gia_ObjCioId(pObj) >= Gia_ManPiNum(p);  } 
static inline int          Gia_ObjIsRi( Gia_Man_t * p, Gia_Obj_t * pObj )      { return Gia_ObjIsCo(pObj) && Gia_ObjCioId(pObj) >= Gia_ManPoNum(p);  } 

static inline Gia_Obj_t *  Gia_ObjRoToRi( Gia_Man_t * p, Gia_Obj_t * pObj )    { assert( Gia_ObjIsRo(p, pObj) ); return Gia_ManCo(p, Gia_ManCoNum(p) - Gia_ManCiNum(p) + Gia_ObjCioId(pObj)); } 
static inline Gia_Obj_t *  Gia_ObjRiToRo( Gia_Man_t * p, Gia_Obj_t * pObj )    { assert( Gia_ObjIsRi(p, pObj) ); return Gia_ManCi(p, Gia_ManCiNum(p) - Gia_ManCoNum(p) + Gia_ObjCioId(pObj)); } 
static inline int          Gia_ObjRoToRiId( Gia_Man_t * p, int ObjId )         { return Gia_ObjId( p, Gia_ObjRoToRi( p, Gia_ManObj(p, ObjId) ) );    } 
static inline int          Gia_ObjRiToRoId( Gia_Man_t * p, int ObjId )         { return Gia_ObjId( p, Gia_ObjRiToRo( p, Gia_ManObj(p, ObjId) ) );    }

static inline int          Gia_ObjDiff0( Gia_Obj_t * pObj )                    { return pObj->iDiff0;         }
static inline int          Gia_ObjDiff1( Gia_Obj_t * pObj )                    { return pObj->iDiff1;         }
static inline int          Gia_ObjFaninC0( Gia_Obj_t * pObj )                  { return pObj->fCompl0;        }
static inline int          Gia_ObjFaninC1( Gia_Obj_t * pObj )                  { return pObj->fCompl1;        }
static inline int          Gia_ObjFaninC2( Gia_Man_t * p, Gia_Obj_t * pObj )   { return p->pMuxes && Abc_LitIsCompl(p->pMuxes[Gia_ObjId(p, pObj)]);  }
static inline Gia_Obj_t *  Gia_ObjFanin0( Gia_Obj_t * pObj )                   { return pObj - pObj->iDiff0;  }
static inline Gia_Obj_t *  Gia_ObjFanin1( Gia_Obj_t * pObj )                   { return pObj - pObj->iDiff1;  }
static inline Gia_Obj_t *  Gia_ObjFanin2( Gia_Man_t * p, Gia_Obj_t * pObj )    { return p->pMuxes ? Gia_ManObj(p, Abc_Lit2Var(p->pMuxes[Gia_ObjId(p, pObj)])) : NULL;  }
static inline Gia_Obj_t *  Gia_ObjChild0( Gia_Obj_t * pObj )                   { return Gia_NotCond( Gia_ObjFanin0(pObj), Gia_ObjFaninC0(pObj) ); }
static inline Gia_Obj_t *  Gia_ObjChild1( Gia_Obj_t * pObj )                   { return Gia_NotCond( Gia_ObjFanin1(pObj), Gia_ObjFaninC1(pObj) ); }
static inline Gia_Obj_t *  Gia_ObjChild2( Gia_Man_t * p, Gia_Obj_t * pObj )    { return Gia_NotCond( Gia_ObjFanin2(p, pObj), Gia_ObjFaninC2(p, pObj) ); }
static inline int          Gia_ObjFaninId0( Gia_Obj_t * pObj, int ObjId )      { return ObjId - pObj->iDiff0;    }
static inline int          Gia_ObjFaninId1( Gia_Obj_t * pObj, int ObjId )      { return ObjId - pObj->iDiff1;    }
static inline int          Gia_ObjFaninId2( Gia_Man_t * p, int ObjId )         { return (p->pMuxes && p->pMuxes[ObjId]) ? Abc_Lit2Var(p->pMuxes[ObjId]) : -1; }
static inline int          Gia_ObjFaninId0p( Gia_Man_t * p, Gia_Obj_t * pObj ) { return Gia_ObjFaninId0( pObj, Gia_ObjId(p, pObj) );              }
static inline int          Gia_ObjFaninId1p( Gia_Man_t * p, Gia_Obj_t * pObj ) { return Gia_ObjFaninId1( pObj, Gia_ObjId(p, pObj) );              }
static inline int          Gia_ObjFaninId2p( Gia_Man_t * p, Gia_Obj_t * pObj ) { return (p->pMuxes && p->pMuxes[Gia_ObjId(p, pObj)]) ? Abc_Lit2Var(p->pMuxes[Gia_ObjId(p, pObj)]) : -1; }
static inline int          Gia_ObjFaninLit0( Gia_Obj_t * pObj, int ObjId )     { return Abc_Var2Lit( Gia_ObjFaninId0(pObj, ObjId), Gia_ObjFaninC0(pObj) ); }
static inline int          Gia_ObjFaninLit1( Gia_Obj_t * pObj, int ObjId )     { return Abc_Var2Lit( Gia_ObjFaninId1(pObj, ObjId), Gia_ObjFaninC1(pObj) ); }
static inline int          Gia_ObjFaninLit2( Gia_Man_t * p, int ObjId )        { return (p->pMuxes && p->pMuxes[ObjId]) ? p->pMuxes[ObjId] : -1;           }
static inline int          Gia_ObjFaninLit0p( Gia_Man_t * p, Gia_Obj_t * pObj) { return Abc_Var2Lit( Gia_ObjFaninId0p(p, pObj), Gia_ObjFaninC0(pObj) );    }
static inline int          Gia_ObjFaninLit1p( Gia_Man_t * p, Gia_Obj_t * pObj) { return Abc_Var2Lit( Gia_ObjFaninId1p(p, pObj), Gia_ObjFaninC1(pObj) );    }
static inline int          Gia_ObjFaninLit2p( Gia_Man_t * p, Gia_Obj_t * pObj) { return (p->pMuxes && p->pMuxes[Gia_ObjId(p, pObj)]) ? p->pMuxes[Gia_ObjId(p, pObj)] : -1;    }
static inline void         Gia_ObjFlipFaninC0( Gia_Obj_t * pObj )              { assert( Gia_ObjIsCo(pObj) ); pObj->fCompl0 ^= 1;          }
static inline int          Gia_ObjFaninNum( Gia_Man_t * p, Gia_Obj_t * pObj )  { if ( Gia_ObjIsMux(p, pObj) ) return 3; if ( Gia_ObjIsAnd(pObj) ) return 2; if ( Gia_ObjIsCo(pObj) ) return 1; return 0; }
static inline int          Gia_ObjWhatFanin( Gia_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pFanin )  { if ( Gia_ObjFanin0(pObj) == pFanin ) return 0; if ( Gia_ObjFanin1(pObj) == pFanin ) return 1; if ( Gia_ObjFanin2(p, pObj) == pFanin ) return 2; assert(0); return -1; }

static inline int          Gia_ManPoIsConst( Gia_Man_t * p, int iPoIndex )     { return Gia_ObjFaninId0p(p, Gia_ManPo(p, iPoIndex)) == 0;                   }
static inline int          Gia_ManPoIsConst0( Gia_Man_t * p, int iPoIndex )    { return Gia_ManIsConst0Lit( Gia_ObjFaninLit0p(p, Gia_ManPo(p, iPoIndex)) ); }
static inline int          Gia_ManPoIsConst1( Gia_Man_t * p, int iPoIndex )    { return Gia_ManIsConst1Lit( Gia_ObjFaninLit0p(p, Gia_ManPo(p, iPoIndex)) ); }

static inline Gia_Obj_t *  Gia_ObjCopy( Gia_Man_t * p, Gia_Obj_t * pObj )      { return Gia_ManObj( p, Abc_Lit2Var(pObj->Value) );                              }
static inline int          Gia_ObjLitCopy( Gia_Man_t * p, int iLit )           { return Abc_LitNotCond( Gia_ManObj(p, Abc_Lit2Var(iLit))->Value, Abc_LitIsCompl(iLit));     }

static inline int          Gia_ObjFanin0Copy( Gia_Obj_t * pObj )               { return Abc_LitNotCond( Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj) );     }
static inline int          Gia_ObjFanin1Copy( Gia_Obj_t * pObj )               { return Abc_LitNotCond( Gia_ObjFanin1(pObj)->Value, Gia_ObjFaninC1(pObj) );     }
static inline int          Gia_ObjFanin2Copy( Gia_Man_t * p, Gia_Obj_t * pObj ){ return Abc_LitNotCond(Gia_ObjFanin2(p, pObj)->Value, Gia_ObjFaninC2(p, pObj)); }

static inline int          Gia_ObjCopyF( Gia_Man_t * p, int f, Gia_Obj_t * pObj )               { return Vec_IntEntry(&p->vCopies, Gia_ManObjNum(p) * f + Gia_ObjId(p,pObj));      }
static inline void         Gia_ObjSetCopyF( Gia_Man_t * p, int f, Gia_Obj_t * pObj, int iLit )  { Vec_IntWriteEntry(&p->vCopies, Gia_ManObjNum(p) * f + Gia_ObjId(p,pObj), iLit);  }
static inline int          Gia_ObjCopyArray( Gia_Man_t * p, int iObj )                          { return Vec_IntEntry(&p->vCopies, iObj);                                          }
static inline void         Gia_ObjSetCopyArray( Gia_Man_t * p, int iObj, int iLit )             { Vec_IntWriteEntry(&p->vCopies, iObj, iLit);                                      }
static inline void         Gia_ManCleanCopyArray( Gia_Man_t * p )                               { Vec_IntFill( &p->vCopies, Gia_ManObjNum(p), -1 );                                }

static inline int          Gia_ObjCopy2Array( Gia_Man_t * p, int iObj )                          { return Vec_IntEntry(&p->vCopies2, iObj);                                          }
static inline void         Gia_ObjSetCopy2Array( Gia_Man_t * p, int iObj, int iLit )             { Vec_IntWriteEntry(&p->vCopies2, iObj, iLit);                                      }
static inline void         Gia_ManCleanCopy2Array( Gia_Man_t * p )                               { Vec_IntFill( &p->vCopies2, Gia_ManObjNum(p), -1 );                                }

static inline int          Gia_ObjFanin0CopyF( Gia_Man_t * p, int f, Gia_Obj_t * pObj )         { return Abc_LitNotCond(Gia_ObjCopyF(p, f, Gia_ObjFanin0(pObj)), Gia_ObjFaninC0(pObj));   }
static inline int          Gia_ObjFanin1CopyF( Gia_Man_t * p, int f, Gia_Obj_t * pObj )         { return Abc_LitNotCond(Gia_ObjCopyF(p, f, Gia_ObjFanin1(pObj)), Gia_ObjFaninC1(pObj));   }
static inline int          Gia_ObjFanin0CopyArray( Gia_Man_t * p, Gia_Obj_t * pObj )            { return Abc_LitNotCond(Gia_ObjCopyArray(p, Gia_ObjFaninId0p(p,pObj)), Gia_ObjFaninC0(pObj));  }
static inline int          Gia_ObjFanin1CopyArray( Gia_Man_t * p, Gia_Obj_t * pObj )            { return Abc_LitNotCond(Gia_ObjCopyArray(p, Gia_ObjFaninId1p(p,pObj)), Gia_ObjFaninC1(pObj));  }

static inline Gia_Obj_t *  Gia_ObjFromLit( Gia_Man_t * p, int iLit )           { return Gia_NotCond( Gia_ManObj(p, Abc_Lit2Var(iLit)), Abc_LitIsCompl(iLit) );  }
static inline int          Gia_ObjToLit( Gia_Man_t * p, Gia_Obj_t * pObj )     { return Abc_Var2Lit( Gia_ObjId(p, Gia_Regular(pObj)), Gia_IsComplement(pObj) ); }
static inline int          Gia_ObjPhaseRealLit( Gia_Man_t * p, int iLit )      { return Gia_ObjPhaseReal( Gia_ObjFromLit(p, iLit) );                            }

static inline int          Gia_ObjLevelId( Gia_Man_t * p, int Id )             { return Vec_IntGetEntry(p->vLevels, Id);                    }
static inline int          Gia_ObjLevel( Gia_Man_t * p, Gia_Obj_t * pObj )     { return Gia_ObjLevelId( p, Gia_ObjId(p,pObj) );             }
static inline void         Gia_ObjSetLevelId( Gia_Man_t * p, int Id, int l )   { Vec_IntSetEntry(p->vLevels, Id, l);                        }
static inline void         Gia_ObjSetLevel( Gia_Man_t * p, Gia_Obj_t * pObj, int l )  { Gia_ObjSetLevelId( p, Gia_ObjId(p,pObj), l );       }
static inline void         Gia_ObjSetCoLevel( Gia_Man_t * p, Gia_Obj_t * pObj )  { assert( Gia_ObjIsCo(pObj)  ); Gia_ObjSetLevel( p, pObj, Gia_ObjLevel(p,Gia_ObjFanin0(pObj)) );                                                }
static inline void         Gia_ObjSetBufLevel( Gia_Man_t * p, Gia_Obj_t * pObj ) { assert( Gia_ObjIsAnd(pObj) ); Gia_ObjSetLevel( p, pObj, Gia_ObjLevel(p,Gia_ObjFanin0(pObj)) );                                                }
static inline void         Gia_ObjSetAndLevel( Gia_Man_t * p, Gia_Obj_t * pObj ) { assert( Gia_ObjIsAnd(pObj) ); Gia_ObjSetLevel( p, pObj, 1+Abc_MaxInt(Gia_ObjLevel(p,Gia_ObjFanin0(pObj)),Gia_ObjLevel(p,Gia_ObjFanin1(pObj))) ); }
static inline void         Gia_ObjSetXorLevel( Gia_Man_t * p, Gia_Obj_t * pObj ) { assert( Gia_ObjIsXor(pObj) ); Gia_ObjSetLevel( p, pObj, 2+Abc_MaxInt(Gia_ObjLevel(p,Gia_ObjFanin0(pObj)),Gia_ObjLevel(p,Gia_ObjFanin1(pObj))) ); }
static inline void         Gia_ObjSetMuxLevel( Gia_Man_t * p, Gia_Obj_t * pObj ) { assert( Gia_ObjIsMux(p,pObj) ); Gia_ObjSetLevel( p, pObj, 2+Abc_MaxInt( Abc_MaxInt(Gia_ObjLevel(p,Gia_ObjFanin0(pObj)),Gia_ObjLevel(p,Gia_ObjFanin1(pObj))), Gia_ObjLevel(p,Gia_ObjFanin2(p,pObj))) ); }
static inline void         Gia_ObjSetGateLevel( Gia_Man_t * p, Gia_Obj_t * pObj ){ if ( !p->fGiaSimple && Gia_ObjIsBuf(pObj) ) Gia_ObjSetBufLevel(p, pObj); else if ( Gia_ObjIsMux(p,pObj) ) Gia_ObjSetMuxLevel(p, pObj); else if ( Gia_ObjIsXor(pObj) ) Gia_ObjSetXorLevel(p, pObj); else if ( Gia_ObjIsAnd(pObj) ) Gia_ObjSetAndLevel(p, pObj); }

static inline int          Gia_ObjHasNumId( Gia_Man_t * p, int Id )                { return Vec_IntEntry(p->vTtNums, Id) > -ABC_INFINITY;     }
static inline int          Gia_ObjNumId( Gia_Man_t * p, int Id )                   { return Vec_IntEntry(p->vTtNums, Id);                     }
static inline int          Gia_ObjNum( Gia_Man_t * p, Gia_Obj_t * pObj )           { return Vec_IntEntry(p->vTtNums, Gia_ObjId(p,pObj));      }
static inline void         Gia_ObjSetNumId( Gia_Man_t * p, int Id, int n )         { Vec_IntWriteEntry(p->vTtNums, Id, n);                    }
static inline void         Gia_ObjSetNum( Gia_Man_t * p, Gia_Obj_t * pObj, int n ) { Vec_IntWriteEntry(p->vTtNums, Gia_ObjId(p,pObj), n);     }
static inline void         Gia_ObjResetNumId( Gia_Man_t * p, int Id )              { Vec_IntWriteEntry(p->vTtNums, Id, -ABC_INFINITY);        }

static inline int          Gia_ObjRefNumId( Gia_Man_t * p, int Id )                { return p->pRefs[Id];                              }
static inline int          Gia_ObjRefIncId( Gia_Man_t * p, int Id )                { return p->pRefs[Id]++;                            }
static inline int          Gia_ObjRefDecId( Gia_Man_t * p, int Id )                { return --p->pRefs[Id];                            }
static inline int          Gia_ObjRefNum( Gia_Man_t * p, Gia_Obj_t * pObj )        { return Gia_ObjRefNumId( p, Gia_ObjId(p, pObj) );  }
static inline int          Gia_ObjRefInc( Gia_Man_t * p, Gia_Obj_t * pObj )        { return Gia_ObjRefIncId( p, Gia_ObjId(p, pObj) );  }
static inline int          Gia_ObjRefDec( Gia_Man_t * p, Gia_Obj_t * pObj )        { return Gia_ObjRefDecId( p, Gia_ObjId(p, pObj) );  }
static inline void         Gia_ObjRefFanin0Inc(Gia_Man_t * p, Gia_Obj_t * pObj)    { Gia_ObjRefInc(p, Gia_ObjFanin0(pObj));            }
static inline void         Gia_ObjRefFanin1Inc(Gia_Man_t * p, Gia_Obj_t * pObj)    { Gia_ObjRefInc(p, Gia_ObjFanin1(pObj));            }
static inline void         Gia_ObjRefFanin2Inc(Gia_Man_t * p, Gia_Obj_t * pObj)    { Gia_ObjRefInc(p, Gia_ObjFanin2(p, pObj));         }
static inline void         Gia_ObjRefFanin0Dec(Gia_Man_t * p, Gia_Obj_t * pObj)    { Gia_ObjRefDec(p, Gia_ObjFanin0(pObj));            }
static inline void         Gia_ObjRefFanin1Dec(Gia_Man_t * p, Gia_Obj_t * pObj)    { Gia_ObjRefDec(p, Gia_ObjFanin1(pObj));            }
static inline void         Gia_ObjRefFanin2Dec(Gia_Man_t * p, Gia_Obj_t * pObj)    { Gia_ObjRefDec(p, Gia_ObjFanin2(p, pObj));         }

static inline int          Gia_ObjLutRefNumId( Gia_Man_t * p, int Id )             { assert(p->pLutRefs); return p->pLutRefs[Id];                    }
static inline int          Gia_ObjLutRefIncId( Gia_Man_t * p, int Id )             { assert(p->pLutRefs); return p->pLutRefs[Id]++;                  }
static inline int          Gia_ObjLutRefDecId( Gia_Man_t * p, int Id )             { assert(p->pLutRefs); return --p->pLutRefs[Id];                  }
static inline int          Gia_ObjLutRefNum( Gia_Man_t * p, Gia_Obj_t * pObj )     { assert(p->pLutRefs); return p->pLutRefs[Gia_ObjId(p, pObj)];    }
static inline int          Gia_ObjLutRefInc( Gia_Man_t * p, Gia_Obj_t * pObj )     { assert(p->pLutRefs); return p->pLutRefs[Gia_ObjId(p, pObj)]++;  }
static inline int          Gia_ObjLutRefDec( Gia_Man_t * p, Gia_Obj_t * pObj )     { assert(p->pLutRefs); return --p->pLutRefs[Gia_ObjId(p, pObj)];  }

static inline void         Gia_ObjSetTravIdCurrent( Gia_Man_t * p, Gia_Obj_t * pObj )         { assert( Gia_ObjId(p, pObj) < p->nTravIdsAlloc ); p->pTravIds[Gia_ObjId(p, pObj)] = p->nTravIds;                    }
static inline void         Gia_ObjSetTravIdPrevious( Gia_Man_t * p, Gia_Obj_t * pObj )        { assert( Gia_ObjId(p, pObj) < p->nTravIdsAlloc ); p->pTravIds[Gia_ObjId(p, pObj)] = p->nTravIds - 1;                }
static inline int          Gia_ObjIsTravIdCurrent( Gia_Man_t * p, Gia_Obj_t * pObj )          { assert( Gia_ObjId(p, pObj) < p->nTravIdsAlloc ); return (p->pTravIds[Gia_ObjId(p, pObj)] == p->nTravIds);          }
static inline int          Gia_ObjIsTravIdPrevious( Gia_Man_t * p, Gia_Obj_t * pObj )         { assert( Gia_ObjId(p, pObj) < p->nTravIdsAlloc ); return (p->pTravIds[Gia_ObjId(p, pObj)] == p->nTravIds - 1);      }
static inline void         Gia_ObjSetTravIdCurrentId( Gia_Man_t * p, int Id )                 { assert( Id < p->nTravIdsAlloc ); p->pTravIds[Id] = p->nTravIds;                     }
static inline int          Gia_ObjIsTravIdCurrentId( Gia_Man_t * p, int Id )                  { assert( Id < p->nTravIdsAlloc ); return (p->pTravIds[Id] == p->nTravIds);           }
static inline int          Gia_ObjIsTravIdPreviousId( Gia_Man_t * p, int Id )                 { assert( Id < p->nTravIdsAlloc ); return (p->pTravIds[Id] == p->nTravIds - 1);       }

static inline void         Gia_ManTimeClean( Gia_Man_t * p )                                  { int i; assert( p->vTiming != NULL ); Vec_FltFill(p->vTiming, 3*Gia_ManObjNum(p), 0); for ( i = 0; i < Gia_ManObjNum(p); i++ )  Vec_FltWriteEntry( p->vTiming, 3*i+1, (float)(ABC_INFINITY) ); }
static inline void         Gia_ManTimeStart( Gia_Man_t * p )                                  { assert( p->vTiming == NULL ); p->vTiming = Vec_FltAlloc(0); Gia_ManTimeClean( p );  }
static inline void         Gia_ManTimeStop( Gia_Man_t * p )                                   { assert( p->vTiming != NULL ); Vec_FltFreeP(&p->vTiming);                            }
static inline float        Gia_ObjTimeArrival( Gia_Man_t * p, int Id )                        { return Vec_FltEntry(p->vTiming, 3*Id+0);                                            }
static inline float        Gia_ObjTimeRequired( Gia_Man_t * p, int Id )                       { return Vec_FltEntry(p->vTiming, 3*Id+1);                                            }
static inline float        Gia_ObjTimeSlack( Gia_Man_t * p, int Id )                          { return Vec_FltEntry(p->vTiming, 3*Id+2);                                            }
static inline float        Gia_ObjTimeArrivalObj( Gia_Man_t * p, Gia_Obj_t * pObj )           { return Gia_ObjTimeArrival( p, Gia_ObjId(p, pObj) );                                 }
static inline float        Gia_ObjTimeRequiredObj( Gia_Man_t * p, Gia_Obj_t * pObj )          { return Gia_ObjTimeRequired( p, Gia_ObjId(p, pObj) );                                }
static inline float        Gia_ObjTimeSlackObj( Gia_Man_t * p, Gia_Obj_t * pObj )             { return Gia_ObjTimeSlack( p, Gia_ObjId(p, pObj) );                                   }
static inline void         Gia_ObjSetTimeArrival( Gia_Man_t * p, int Id, float t )            { Vec_FltWriteEntry( p->vTiming, 3*Id+0, t );                                         }
static inline void         Gia_ObjSetTimeRequired( Gia_Man_t * p, int Id, float t )           { Vec_FltWriteEntry( p->vTiming, 3*Id+1, t );                                         }
static inline void         Gia_ObjSetTimeSlack( Gia_Man_t * p, int Id, float t )              { Vec_FltWriteEntry( p->vTiming, 3*Id+2, t );                                         }
static inline void         Gia_ObjSetTimeArrivalObj( Gia_Man_t * p, Gia_Obj_t * pObj, float t )  { Gia_ObjSetTimeArrival( p, Gia_ObjId(p, pObj), t );                               }
static inline void         Gia_ObjSetTimeRequiredObj( Gia_Man_t * p, Gia_Obj_t * pObj, float t ) { Gia_ObjSetTimeRequired( p, Gia_ObjId(p, pObj), t );                              }
static inline void         Gia_ObjSetTimeSlackObj( Gia_Man_t * p, Gia_Obj_t * pObj, float t )    { Gia_ObjSetTimeSlack( p, Gia_ObjId(p, pObj), t );                                 }

static inline int          Gia_ObjSimWords( Gia_Man_t * p )                    { return Vec_WrdSize( p->vSimsPi ) / Gia_ManPiNum( p );          }
static inline word *       Gia_ObjSimPi( Gia_Man_t * p, int PiId )             { return Vec_WrdEntryP( p->vSimsPi, PiId * Gia_ObjSimWords(p) ); }
static inline word *       Gia_ObjSim( Gia_Man_t * p, int Id )                 { return Vec_WrdEntryP( p->vSims, Id * Gia_ObjSimWords(p) );     }
static inline word *       Gia_ObjSimObj( Gia_Man_t * p, Gia_Obj_t * pObj )    { return Gia_ObjSim( p, Gia_ObjId(p, pObj) );                    }

// AIG construction
extern void Gia_ObjAddFanout( Gia_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pFanout );
static inline Gia_Obj_t * Gia_ManAppendObj( Gia_Man_t * p )  
{ 
    if ( p->nObjs == p->nObjsAlloc )
    {
        int nObjNew = Abc_MinInt( 2 * p->nObjsAlloc, (1 << 29) );
        if ( p->nObjs == (1 << 29) )
            printf( "Hard limit on the number of nodes (2^29) is reached. Quitting...\n" ), exit(1);
        assert( p->nObjs < nObjNew );
        if ( p->fVerbose )
            printf("Extending GIA object storage: %d -> %d.\n", p->nObjsAlloc, nObjNew );
        assert( p->nObjsAlloc > 0 );
        p->pObjs = ABC_REALLOC( Gia_Obj_t, p->pObjs, nObjNew );
        memset( p->pObjs + p->nObjsAlloc, 0, sizeof(Gia_Obj_t) * (nObjNew - p->nObjsAlloc) );
        if ( p->pMuxes )
        {
            p->pMuxes = ABC_REALLOC( unsigned, p->pMuxes, nObjNew );
            memset( p->pMuxes + p->nObjsAlloc, 0, sizeof(unsigned) * (nObjNew - p->nObjsAlloc) );
        }
        p->nObjsAlloc = nObjNew;
    }
    if ( Vec_IntSize(&p->vHTable) ) Vec_IntPush( &p->vHash, 0 );
    return Gia_ManObj( p, p->nObjs++ );
}
static inline int Gia_ManAppendCi( Gia_Man_t * p )  
{ 
    Gia_Obj_t * pObj = Gia_ManAppendObj( p );
    pObj->fTerm = 1;
    pObj->iDiff0 = GIA_NONE;
    pObj->iDiff1 = Vec_IntSize( p->vCis );
    Vec_IntPush( p->vCis, Gia_ObjId(p, pObj) );
    return Gia_ObjId( p, pObj ) << 1;
}

extern void Gia_ManQuantSetSuppAnd( Gia_Man_t * p, Gia_Obj_t * pObj );
extern void Gia_ManBuiltInSimPerform( Gia_Man_t * p, int iObj );

static inline int Gia_ManAppendAnd( Gia_Man_t * p, int iLit0, int iLit1 )  
{ 
    Gia_Obj_t * pObj = Gia_ManAppendObj( p );
    assert( iLit0 >= 0 && Abc_Lit2Var(iLit0) < Gia_ManObjNum(p) );
    assert( iLit1 >= 0 && Abc_Lit2Var(iLit1) < Gia_ManObjNum(p) );
    assert( p->fGiaSimple || Abc_Lit2Var(iLit0) != Abc_Lit2Var(iLit1) );
    if ( iLit0 < iLit1 )
    {
        pObj->iDiff0  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit0);
        pObj->fCompl0 = Abc_LitIsCompl(iLit0);
        pObj->iDiff1  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit1);
        pObj->fCompl1 = Abc_LitIsCompl(iLit1);
    }
    else
    {
        pObj->iDiff1  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit0);
        pObj->fCompl1 = Abc_LitIsCompl(iLit0);
        pObj->iDiff0  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit1);
        pObj->fCompl0 = Abc_LitIsCompl(iLit1);
    }
    if ( p->pFanData )
    {
        Gia_ObjAddFanout( p, Gia_ObjFanin0(pObj), pObj );
        Gia_ObjAddFanout( p, Gia_ObjFanin1(pObj), pObj );
    }
    if ( p->fSweeper )
    {
        Gia_Obj_t * pFan0 = Gia_ObjFanin0(pObj);
        Gia_Obj_t * pFan1 = Gia_ObjFanin1(pObj);
        if ( pFan0->fMark0 ) pFan0->fMark1 = 1; else pFan0->fMark0 = 1;
        if ( pFan1->fMark0 ) pFan1->fMark1 = 1; else pFan1->fMark0 = 1;
        pObj->fPhase = (Gia_ObjPhase(pFan0) ^ Gia_ObjFaninC0(pObj)) & (Gia_ObjPhase(pFan1) ^ Gia_ObjFaninC1(pObj));
    }
    if ( p->fBuiltInSim )
    {
        Gia_Obj_t * pFan0 = Gia_ObjFanin0(pObj);
        Gia_Obj_t * pFan1 = Gia_ObjFanin1(pObj);
        pObj->fPhase = (Gia_ObjPhase(pFan0) ^ Gia_ObjFaninC0(pObj)) & (Gia_ObjPhase(pFan1) ^ Gia_ObjFaninC1(pObj));
        Gia_ManBuiltInSimPerform( p, Gia_ObjId( p, pObj ) );
    }
    if ( p->vSuppWords )
        Gia_ManQuantSetSuppAnd( p, pObj );
    return Gia_ObjId( p, pObj ) << 1;
}
static inline int Gia_ManAppendXorReal( Gia_Man_t * p, int iLit0, int iLit1 )  
{ 
    Gia_Obj_t * pObj = Gia_ManAppendObj( p );
    assert( iLit0 >= 0 && Abc_Lit2Var(iLit0) < Gia_ManObjNum(p) );
    assert( iLit1 >= 0 && Abc_Lit2Var(iLit1) < Gia_ManObjNum(p) );
    assert( Abc_Lit2Var(iLit0) != Abc_Lit2Var(iLit1) );
    assert( !Abc_LitIsCompl(iLit0) );
    assert( !Abc_LitIsCompl(iLit1) );
    if ( Abc_Lit2Var(iLit0) > Abc_Lit2Var(iLit1) )
    {
        pObj->iDiff0  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit0);
        pObj->fCompl0 = Abc_LitIsCompl(iLit0);
        pObj->iDiff1  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit1);
        pObj->fCompl1 = Abc_LitIsCompl(iLit1);
    }
    else
    {
        pObj->iDiff1  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit0);
        pObj->fCompl1 = Abc_LitIsCompl(iLit0);
        pObj->iDiff0  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit1);
        pObj->fCompl0 = Abc_LitIsCompl(iLit1);
    }
    p->nXors++;
    return Gia_ObjId( p, pObj ) << 1;
}
static inline int Gia_ManAppendMuxReal( Gia_Man_t * p, int iLitC, int iLit1, int iLit0 )  
{ 
    Gia_Obj_t * pObj = Gia_ManAppendObj( p );
    assert( p->pMuxes != NULL );
    assert( iLit0 >= 0 && Abc_Lit2Var(iLit0) < Gia_ManObjNum(p) );
    assert( iLit1 >= 0 && Abc_Lit2Var(iLit1) < Gia_ManObjNum(p) );
    assert( iLitC >= 0 && Abc_Lit2Var(iLitC) < Gia_ManObjNum(p) );
    assert( Abc_Lit2Var(iLit0) != Abc_Lit2Var(iLit1) );
    assert( Abc_Lit2Var(iLitC) != Abc_Lit2Var(iLit0) );
    assert( Abc_Lit2Var(iLitC) != Abc_Lit2Var(iLit1) );
    assert( !Vec_IntSize(&p->vHTable) || !Abc_LitIsCompl(iLit1) );
    if ( Abc_Lit2Var(iLit0) < Abc_Lit2Var(iLit1) )
    {
        pObj->iDiff0  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit0);
        pObj->fCompl0 = Abc_LitIsCompl(iLit0);
        pObj->iDiff1  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit1);
        pObj->fCompl1 = Abc_LitIsCompl(iLit1);
        p->pMuxes[Gia_ObjId(p, pObj)] = iLitC;
    }
    else
    {
        pObj->iDiff1  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit0);
        pObj->fCompl1 = Abc_LitIsCompl(iLit0);
        pObj->iDiff0  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit1);
        pObj->fCompl0 = Abc_LitIsCompl(iLit1);
        p->pMuxes[Gia_ObjId(p, pObj)] = Abc_LitNot(iLitC);
    }
    p->nMuxes++;
    return Gia_ObjId( p, pObj ) << 1;
}
static inline int Gia_ManAppendBuf( Gia_Man_t * p, int iLit )  
{ 
    Gia_Obj_t * pObj = Gia_ManAppendObj( p );
    assert( iLit >= 0 && Abc_Lit2Var(iLit) < Gia_ManObjNum(p) );
    pObj->iDiff0  = pObj->iDiff1  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit);
    pObj->fCompl0 = pObj->fCompl1 = Abc_LitIsCompl(iLit);
    p->nBufs++;
    return Gia_ObjId( p, pObj ) << 1;
}
static inline int Gia_ManAppendCo( Gia_Man_t * p, int iLit0 )  
{ 
    Gia_Obj_t * pObj;
    assert( iLit0 >= 0 && Abc_Lit2Var(iLit0) < Gia_ManObjNum(p) );
    assert( !Gia_ObjIsCo(Gia_ManObj(p, Abc_Lit2Var(iLit0))) );
    pObj = Gia_ManAppendObj( p );    
    pObj->fTerm = 1;
    pObj->iDiff0  = Gia_ObjId(p, pObj) - Abc_Lit2Var(iLit0);
    pObj->fCompl0 = Abc_LitIsCompl(iLit0);
    pObj->iDiff1  = Vec_IntSize( p->vCos );
    Vec_IntPush( p->vCos, Gia_ObjId(p, pObj) );
    if ( p->pFanData )
        Gia_ObjAddFanout( p, Gia_ObjFanin0(pObj), pObj );
    return Gia_ObjId( p, pObj ) << 1;
}
static inline int Gia_ManAppendOr( Gia_Man_t * p, int iLit0, int iLit1 )
{
    return Abc_LitNot(Gia_ManAppendAnd( p, Abc_LitNot(iLit0), Abc_LitNot(iLit1) ));
}
static inline int Gia_ManAppendMux( Gia_Man_t * p, int iCtrl, int iData1, int iData0 )  
{ 
    int iTemp0 = Gia_ManAppendAnd( p, Abc_LitNot(iCtrl), iData0 );
    int iTemp1 = Gia_ManAppendAnd( p, iCtrl, iData1 );
    return Abc_LitNotCond( Gia_ManAppendAnd( p, Abc_LitNot(iTemp0), Abc_LitNot(iTemp1) ), 1 );
}
static inline int Gia_ManAppendMaj( Gia_Man_t * p, int iData0, int iData1, int iData2 )  
{ 
    int iTemp0 = Gia_ManAppendOr( p, iData1, iData2 );
    int iTemp1 = Gia_ManAppendAnd( p, iData0, iTemp0 );
    int iTemp2 = Gia_ManAppendAnd( p, iData1, iData2 );
    return Gia_ManAppendOr( p, iTemp1, iTemp2 );
}
static inline int Gia_ManAppendXor( Gia_Man_t * p, int iLit0, int iLit1 )  
{ 
    return Gia_ManAppendMux( p, iLit0, Abc_LitNot(iLit1), iLit1 );
}

static inline int Gia_ManAppendAnd2( Gia_Man_t * p, int iLit0, int iLit1 )  
{ 
    if ( !p->fGiaSimple )
    {
        if ( iLit0 < 2 )
            return iLit0 ? iLit1 : 0;
        if ( iLit1 < 2 )
            return iLit1 ? iLit0 : 0;
        if ( iLit0 == iLit1 )
            return iLit1;
        if ( iLit0 == Abc_LitNot(iLit1) )
            return 0;
    }
    return Gia_ManAppendAnd( p, iLit0, iLit1 );
}
static inline int Gia_ManAppendOr2( Gia_Man_t * p, int iLit0, int iLit1 )
{
    return Abc_LitNot(Gia_ManAppendAnd2( p, Abc_LitNot(iLit0), Abc_LitNot(iLit1) ));
}
static inline int Gia_ManAppendMux2( Gia_Man_t * p, int iCtrl, int iData1, int iData0 )  
{ 
    int iTemp0 = Gia_ManAppendAnd2( p, Abc_LitNot(iCtrl), iData0 );
    int iTemp1 = Gia_ManAppendAnd2( p, iCtrl, iData1 );
    return Abc_LitNotCond( Gia_ManAppendAnd2( p, Abc_LitNot(iTemp0), Abc_LitNot(iTemp1) ), 1 );
}
static inline int Gia_ManAppendMaj2( Gia_Man_t * p, int iData0, int iData1, int iData2 )  
{ 
    int iTemp0 = Gia_ManAppendOr2( p, iData1, iData2 );
    int iTemp1 = Gia_ManAppendAnd2( p, iData0, iTemp0 );
    int iTemp2 = Gia_ManAppendAnd2( p, iData1, iData2 );
    return Gia_ManAppendOr2( p, iTemp1, iTemp2 );
}
static inline int Gia_ManAppendXor2( Gia_Man_t * p, int iLit0, int iLit1 )  
{ 
    return Gia_ManAppendMux2( p, iLit0, Abc_LitNot(iLit1), iLit1 );
}

static inline void Gia_ManPatchCoDriver( Gia_Man_t * p, int iCoIndex, int iLit0 )  
{
    Gia_Obj_t * pObjCo  = Gia_ManCo( p, iCoIndex );
    assert( Gia_ObjId(p, pObjCo) > Abc_Lit2Var(iLit0) );
    pObjCo->iDiff0  = Gia_ObjId(p, pObjCo) - Abc_Lit2Var(iLit0);
    pObjCo->fCompl0 = Abc_LitIsCompl(iLit0);
}

#define GIA_ZER 1
#define GIA_ONE 2
#define GIA_UND 3

static inline int Gia_XsimNotCond( int Value, int fCompl )   
{ 
    if ( Value == GIA_UND )
        return GIA_UND;
    if ( Value == GIA_ZER + fCompl )
        return GIA_ZER;
    return GIA_ONE;
}
static inline int Gia_XsimAndCond( int Value0, int fCompl0, int Value1, int fCompl1 )   
{ 
    if ( Value0 == GIA_ZER + fCompl0 || Value1 == GIA_ZER + fCompl1 )
        return GIA_ZER;
    if ( Value0 == GIA_UND || Value1 == GIA_UND )
        return GIA_UND;
    return GIA_ONE;
}


static inline void Gia_ObjTerSimSetC( Gia_Obj_t * pObj ) { pObj->fMark0 = 0; pObj->fMark1 = 0;    }
static inline void Gia_ObjTerSimSet0( Gia_Obj_t * pObj ) { pObj->fMark0 = 1; pObj->fMark1 = 0;    }
static inline void Gia_ObjTerSimSet1( Gia_Obj_t * pObj ) { pObj->fMark0 = 0; pObj->fMark1 = 1;    }
static inline void Gia_ObjTerSimSetX( Gia_Obj_t * pObj ) { pObj->fMark0 = 1; pObj->fMark1 = 1;    }

static inline int  Gia_ObjTerSimGetC( Gia_Obj_t * pObj ) { return !pObj->fMark0 && !pObj->fMark1; }
static inline int  Gia_ObjTerSimGet0( Gia_Obj_t * pObj ) { return  pObj->fMark0 && !pObj->fMark1; }
static inline int  Gia_ObjTerSimGet1( Gia_Obj_t * pObj ) { return !pObj->fMark0 &&  pObj->fMark1; }
static inline int  Gia_ObjTerSimGetX( Gia_Obj_t * pObj ) { return  pObj->fMark0 &&  pObj->fMark1; }

static inline int  Gia_ObjTerSimGet0Fanin0( Gia_Obj_t * pObj ) { return (Gia_ObjTerSimGet1(Gia_ObjFanin0(pObj)) && Gia_ObjFaninC0(pObj)) || (Gia_ObjTerSimGet0(Gia_ObjFanin0(pObj)) && !Gia_ObjFaninC0(pObj)); }
static inline int  Gia_ObjTerSimGet1Fanin0( Gia_Obj_t * pObj ) { return (Gia_ObjTerSimGet0(Gia_ObjFanin0(pObj)) && Gia_ObjFaninC0(pObj)) || (Gia_ObjTerSimGet1(Gia_ObjFanin0(pObj)) && !Gia_ObjFaninC0(pObj)); }

static inline int  Gia_ObjTerSimGet0Fanin1( Gia_Obj_t * pObj ) { return (Gia_ObjTerSimGet1(Gia_ObjFanin1(pObj)) && Gia_ObjFaninC1(pObj)) || (Gia_ObjTerSimGet0(Gia_ObjFanin1(pObj)) && !Gia_ObjFaninC1(pObj)); }
static inline int  Gia_ObjTerSimGet1Fanin1( Gia_Obj_t * pObj ) { return (Gia_ObjTerSimGet0(Gia_ObjFanin1(pObj)) && Gia_ObjFaninC1(pObj)) || (Gia_ObjTerSimGet1(Gia_ObjFanin1(pObj)) && !Gia_ObjFaninC1(pObj)); }

static inline void Gia_ObjTerSimAnd( Gia_Obj_t * pObj )
{
    assert( Gia_ObjIsAnd(pObj) );
    assert( !Gia_ObjTerSimGetC( Gia_ObjFanin0(pObj) ) );
    assert( !Gia_ObjTerSimGetC( Gia_ObjFanin1(pObj) ) );
    if ( Gia_ObjTerSimGet0Fanin0(pObj) || Gia_ObjTerSimGet0Fanin1(pObj) )
        Gia_ObjTerSimSet0( pObj );
    else if ( Gia_ObjTerSimGet1Fanin0(pObj) && Gia_ObjTerSimGet1Fanin1(pObj) )
        Gia_ObjTerSimSet1( pObj );
    else 
        Gia_ObjTerSimSetX( pObj );
}
static inline void Gia_ObjTerSimCo( Gia_Obj_t * pObj )
{
    assert( Gia_ObjIsCo(pObj) );
    assert( !Gia_ObjTerSimGetC( Gia_ObjFanin0(pObj) ) );
    if ( Gia_ObjTerSimGet0Fanin0(pObj) )
        Gia_ObjTerSimSet0( pObj );
    else if ( Gia_ObjTerSimGet1Fanin0(pObj) )
        Gia_ObjTerSimSet1( pObj );
    else 
        Gia_ObjTerSimSetX( pObj );
}
static inline void Gia_ObjTerSimRo( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pTemp = Gia_ObjRoToRi(p, pObj);
    assert( Gia_ObjIsRo(p, pObj) );
    assert( !Gia_ObjTerSimGetC( pTemp ) );
    pObj->fMark0 = pTemp->fMark0;
    pObj->fMark1 = pTemp->fMark1;
}

static inline void Gia_ObjTerSimPrint( Gia_Obj_t * pObj )
{
    if ( Gia_ObjTerSimGet0(pObj) )
        printf( "0" );
    else if ( Gia_ObjTerSimGet1(pObj) )
        printf( "1" );
    else if ( Gia_ObjTerSimGetX(pObj) )
        printf( "X" );
}

static inline int Gia_AigerReadInt( unsigned char * pPos )
{
    int i, Value = 0;
    for ( i = 0; i < 4; i++ )
        Value = (Value << 8) | *pPos++;
    return Value;
}
static inline void Gia_AigerWriteInt( unsigned char * pPos, int Value )
{
    int i;
    for ( i = 3; i >= 0; i-- )
        *pPos++ = (Value >> (8*i)) & 255;
}
static inline unsigned Gia_AigerReadUnsigned( unsigned char ** ppPos )
{
    unsigned x = 0, i = 0;
    unsigned char ch;
    while ((ch = *(*ppPos)++) & 0x80)
        x |= (ch & 0x7f) << (7 * i++);
    return x | (ch << (7 * i));
}
static inline void Gia_AigerWriteUnsigned( Vec_Str_t * vStr, unsigned x )
{
    unsigned char ch;
    while (x & ~0x7f)
    {
        ch = (x & 0x7f) | 0x80;
        Vec_StrPush( vStr, ch );
        x >>= 7;
    }
    ch = x;
    Vec_StrPush( vStr, ch );
}
static inline void Gia_AigerWriteUnsignedFile( FILE * pFile, unsigned x )
{
    unsigned char ch;
    while (x & ~0x7f)
    {
        ch = (x & 0x7f) | 0x80;
        fputc( ch, pFile );
        x >>= 7;
    }
    ch = x;
    fputc( ch, pFile );
}
static inline int Gia_AigerWriteUnsignedBuffer( unsigned char * pBuffer, int Pos, unsigned x )
{
    unsigned char ch;
    while (x & ~0x7f)
    {
        ch = (x & 0x7f) | 0x80;
        pBuffer[Pos++] = ch;
        x >>= 7;
    }
    ch = x;
    pBuffer[Pos++] = ch;
    return Pos;
}

static inline Gia_Obj_t * Gia_ObjReprObj( Gia_Man_t * p, int Id )            { return p->pReprs[Id].iRepr == GIA_VOID ? NULL : Gia_ManObj( p, p->pReprs[Id].iRepr );                  }
static inline int         Gia_ObjRepr( Gia_Man_t * p, int Id )               { return p->pReprs[Id].iRepr;                                                }
static inline void        Gia_ObjSetRepr( Gia_Man_t * p, int Id, int Num )   { assert( Num == GIA_VOID || Num < Id ); p->pReprs[Id].iRepr = Num;          }
static inline void        Gia_ObjSetReprRev( Gia_Man_t * p, int Id, int Num ){ assert( Num == GIA_VOID || Num > Id ); p->pReprs[Id].iRepr = Num;          }
static inline void        Gia_ObjUnsetRepr( Gia_Man_t * p, int Id )          { p->pReprs[Id].iRepr = GIA_VOID;                                            }
static inline int         Gia_ObjHasRepr( Gia_Man_t * p, int Id )            { return p->pReprs[Id].iRepr != GIA_VOID;                                    }
static inline int         Gia_ObjReprSelf( Gia_Man_t * p, int Id )           { return Gia_ObjHasRepr(p, Id) ? Gia_ObjRepr(p, Id) : Id;                    }
static inline int         Gia_ObjSibl( Gia_Man_t * p, int Id )               { return p->pSibls ? p->pSibls[Id] : 0;                                      }
static inline Gia_Obj_t * Gia_ObjSiblObj( Gia_Man_t * p, int Id )            { return (p->pSibls && p->pSibls[Id]) ? Gia_ManObj(p, p->pSibls[Id]) : NULL; }

static inline int         Gia_ObjProved( Gia_Man_t * p, int Id )             { return p->pReprs[Id].fProved;       }
static inline void        Gia_ObjSetProved( Gia_Man_t * p, int Id )          { p->pReprs[Id].fProved = 1;          }
static inline void        Gia_ObjUnsetProved( Gia_Man_t * p, int Id )        { p->pReprs[Id].fProved = 0;          }

static inline int         Gia_ObjFailed( Gia_Man_t * p, int Id )             { return p->pReprs[Id].fFailed;       }
static inline void        Gia_ObjSetFailed( Gia_Man_t * p, int Id )          { p->pReprs[Id].fFailed = 1;          }

static inline int         Gia_ObjColor( Gia_Man_t * p, int Id, int c )       { return c? p->pReprs[Id].fColorB : p->pReprs[Id].fColorA;          }
static inline int         Gia_ObjColors( Gia_Man_t * p, int Id )             { return p->pReprs[Id].fColorB * 2 + p->pReprs[Id].fColorA;         }
static inline void        Gia_ObjSetColor( Gia_Man_t * p, int Id, int c )    { if (c) p->pReprs[Id].fColorB = 1; else p->pReprs[Id].fColorA = 1; }
static inline void        Gia_ObjSetColors( Gia_Man_t * p, int Id )          { p->pReprs[Id].fColorB = p->pReprs[Id].fColorA = 1;                }
static inline int         Gia_ObjVisitColor( Gia_Man_t * p, int Id, int c )  { int x; if (c) { x = p->pReprs[Id].fColorB; p->pReprs[Id].fColorB = 1; } else { x = p->pReprs[Id].fColorA; p->pReprs[Id].fColorA = 1; } return x; }
static inline int         Gia_ObjDiffColors( Gia_Man_t * p, int i, int j )   { return (p->pReprs[i].fColorA ^ p->pReprs[j].fColorA) && (p->pReprs[i].fColorB ^ p->pReprs[j].fColorB); }
static inline int         Gia_ObjDiffColors2( Gia_Man_t * p, int i, int j )  { return (p->pReprs[i].fColorA ^ p->pReprs[j].fColorA) || (p->pReprs[i].fColorB ^ p->pReprs[j].fColorB); }

static inline Gia_Obj_t * Gia_ObjNextObj( Gia_Man_t * p, int Id )            { return p->pNexts[Id] == 0 ? NULL : Gia_ManObj( p, p->pNexts[Id] );}
static inline int         Gia_ObjNext( Gia_Man_t * p, int Id )               { return p->pNexts[Id];                                             }
static inline void        Gia_ObjSetNext( Gia_Man_t * p, int Id, int Num )   { p->pNexts[Id] = Num;                                              }

static inline int         Gia_ObjIsConst( Gia_Man_t * p, int Id )            { return Gia_ObjRepr(p, Id) == 0;                                   }
static inline int         Gia_ObjIsHead( Gia_Man_t * p, int Id )             { return Gia_ObjRepr(p, Id) == GIA_VOID && Gia_ObjNext(p, Id) > 0;  }
static inline int         Gia_ObjIsNone( Gia_Man_t * p, int Id )             { return Gia_ObjRepr(p, Id) == GIA_VOID && Gia_ObjNext(p, Id) <= 0; }
static inline int         Gia_ObjIsTail( Gia_Man_t * p, int Id )             { return (Gia_ObjRepr(p, Id) > 0 && Gia_ObjRepr(p, Id) != GIA_VOID) && Gia_ObjNext(p, Id) <= 0;                  }
static inline int         Gia_ObjIsClass( Gia_Man_t * p, int Id )            { return (Gia_ObjRepr(p, Id) > 0 && Gia_ObjRepr(p, Id) != GIA_VOID) || Gia_ObjNext(p, Id) > 0;                   }
static inline int         Gia_ObjHasSameRepr( Gia_Man_t * p, int i, int k )  { assert( k ); return i? (Gia_ObjRepr(p, i) == Gia_ObjRepr(p, k) && Gia_ObjRepr(p, i) != GIA_VOID) : Gia_ObjRepr(p, k) == 0;  }
static inline int         Gia_ObjIsFailedPair( Gia_Man_t * p, int i, int k ) { assert( k ); return i? (Gia_ObjFailed(p, i) || Gia_ObjFailed(p, k)) : Gia_ObjFailed(p, k);                     }
static inline int         Gia_ClassIsPair( Gia_Man_t * p, int i )            { assert( Gia_ObjIsHead(p, i) ); assert( Gia_ObjNext(p, i) ); return Gia_ObjNext(p, Gia_ObjNext(p, i)) <= 0;     }
static inline void        Gia_ClassUndoPair( Gia_Man_t * p, int i )          { assert( Gia_ClassIsPair(p,i) ); Gia_ObjSetRepr(p, Gia_ObjNext(p, i), GIA_VOID); Gia_ObjSetNext(p, i, 0);       }

#define Gia_ManForEachConst( p, i )                            \
    for ( i = 1; i < Gia_ManObjNum(p); i++ ) if ( !Gia_ObjIsConst(p, i) ) {} else
#define Gia_ManForEachClass( p, i )                            \
    for ( i = 1; i < Gia_ManObjNum(p); i++ ) if ( !Gia_ObjIsHead(p, i) ) {} else
#define Gia_ManForEachClass0( p, i )                            \
    for ( i = 0; i < Gia_ManObjNum(p); i++ ) if ( !Gia_ObjIsHead(p, i) ) {} else
#define Gia_ManForEachClassReverse( p, i )                     \
    for ( i = Gia_ManObjNum(p) - 1; i > 0; i-- ) if ( !Gia_ObjIsHead(p, i) ) {} else
#define Gia_ClassForEachObj( p, i, iObj )                      \
    for ( assert(Gia_ObjIsHead(p, i)), iObj = i; iObj > 0; iObj = Gia_ObjNext(p, iObj) )
#define Gia_ClassForEachObj1( p, i, iObj )                     \
    for ( assert(Gia_ObjIsHead(p, i)), iObj = Gia_ObjNext(p, i); iObj > 0; iObj = Gia_ObjNext(p, iObj) )


static inline int         Gia_ObjFoffsetId( Gia_Man_t * p, int Id )                { return Vec_IntEntry( p->vFanout, Id );                                 }
static inline int         Gia_ObjFoffset( Gia_Man_t * p, Gia_Obj_t * pObj )        { return Gia_ObjFoffsetId( p, Gia_ObjId(p, pObj) );                      }
static inline int         Gia_ObjFanoutNumId( Gia_Man_t * p, int Id )              { return Vec_IntEntry( p->vFanoutNums, Id );                             }
static inline int         Gia_ObjFanoutNum( Gia_Man_t * p, Gia_Obj_t * pObj )      { return Gia_ObjFanoutNumId( p, Gia_ObjId(p, pObj) );                    }
static inline int         Gia_ObjFanoutId( Gia_Man_t * p, int Id, int i )          { return Vec_IntEntry( p->vFanout, Gia_ObjFoffsetId(p, Id) + i );        }
static inline Gia_Obj_t * Gia_ObjFanout0( Gia_Man_t * p, Gia_Obj_t * pObj )        { return Gia_ManObj( p, Gia_ObjFanoutId(p, Gia_ObjId(p, pObj), 0) );     }
static inline Gia_Obj_t * Gia_ObjFanout( Gia_Man_t * p, Gia_Obj_t * pObj, int i )  { return Gia_ManObj( p, Gia_ObjFanoutId(p, Gia_ObjId(p, pObj), i) );     }
static inline void        Gia_ObjSetFanout( Gia_Man_t * p, Gia_Obj_t * pObj, int i, Gia_Obj_t * pFan )   { Vec_IntWriteEntry( p->vFanout, Gia_ObjFoffset(p, pObj) + i, Gia_ObjId(p, pFan) ); }
static inline void        Gia_ObjSetFanoutInt( Gia_Man_t * p, Gia_Obj_t * pObj, int i, int x )           { Vec_IntWriteEntry( p->vFanout, Gia_ObjFoffset(p, pObj) + i, x );                  }

#define Gia_ObjForEachFanoutStatic( p, pObj, pFanout, i )      \
    for ( i = 0; (i < Gia_ObjFanoutNum(p, pObj))   && (((pFanout) = Gia_ObjFanout(p, pObj, i)), 1); i++ )
#define Gia_ObjForEachFanoutStaticId( p, Id, FanId, i )      \
    for ( i = 0; (i < Gia_ObjFanoutNumId(p, Id))   && (((FanId) = Gia_ObjFanoutId(p, Id, i)), 1); i++ )

static inline int         Gia_ManHasMapping( Gia_Man_t * p )                { return p->vMapping != NULL;                                                   }
static inline int         Gia_ObjIsLut( Gia_Man_t * p, int Id )             { return Vec_IntEntry(p->vMapping, Id) != 0;                                    }
static inline int         Gia_ObjLutSize( Gia_Man_t * p, int Id )           { return Vec_IntEntry(p->vMapping, Vec_IntEntry(p->vMapping, Id));              }
static inline int *       Gia_ObjLutFanins( Gia_Man_t * p, int Id )         { return Vec_IntEntryP(p->vMapping, Vec_IntEntry(p->vMapping, Id)) + 1;         }
static inline int         Gia_ObjLutFanin( Gia_Man_t * p, int Id, int i )   { return Gia_ObjLutFanins(p, Id)[i];                                            }
static inline int         Gia_ObjLutMuxId( Gia_Man_t * p, int Id )          { return Gia_ObjLutFanins(p, Id)[Gia_ObjLutSize(p, Id)];                        }
static inline int         Gia_ObjLutIsMux( Gia_Man_t * p, int Id )          { return (int)(Gia_ObjLutMuxId(p, Id) < 0);                                     }

static inline int         Gia_ManHasMapping2( Gia_Man_t * p )               { return p->vMapping2 != NULL;                                                  }
static inline int         Gia_ObjIsLut2( Gia_Man_t * p, int Id )            { return Vec_IntSize(Vec_WecEntry(p->vMapping2, Id)) != 0;                      }
static inline int         Gia_ObjLutSize2( Gia_Man_t * p, int Id )          { return Vec_IntSize(Vec_WecEntry(p->vMapping2, Id));                           }
static inline Vec_Int_t * Gia_ObjLutFanins2( Gia_Man_t * p, int Id )        { return Vec_WecEntry(p->vMapping2, Id);                                        }
static inline int         Gia_ObjLutFanin2( Gia_Man_t * p, int Id, int i )  { return Vec_IntEntry(Vec_WecEntry(p->vMapping2, Id), i);                       }
static inline int         Gia_ObjLutFanoutNum2( Gia_Man_t * p, int Id )     { return Vec_IntSize(Vec_WecEntry(p->vFanouts2, Id));                           }
static inline int         Gia_ObjLutFanout2( Gia_Man_t * p, int Id, int i ) { return Vec_IntEntry(Vec_WecEntry(p->vFanouts2, Id), i);                       }

static inline int         Gia_ManHasCellMapping( Gia_Man_t * p )            { return p->vCellMapping != NULL;                                               }
static inline int         Gia_ObjIsCell( Gia_Man_t * p, int iLit )          { return Vec_IntEntry(p->vCellMapping, iLit) != 0;                              }
static inline int         Gia_ObjIsCellInv( Gia_Man_t * p, int iLit )       { return Vec_IntEntry(p->vCellMapping, iLit) == -1;                             }
static inline int         Gia_ObjIsCellBuf( Gia_Man_t * p, int iLit )       { return Vec_IntEntry(p->vCellMapping, iLit) == -2;                             }
static inline int         Gia_ObjCellSize( Gia_Man_t * p, int iLit )        { return Vec_IntEntry(p->vCellMapping, Vec_IntEntry(p->vCellMapping, iLit));    }
static inline int *       Gia_ObjCellFanins( Gia_Man_t * p, int iLit )      { return Vec_IntEntryP(p->vCellMapping, Vec_IntEntry(p->vCellMapping, iLit))+1; }
static inline int         Gia_ObjCellFanin( Gia_Man_t * p, int iLit, int i ){ return Gia_ObjCellFanins(p, iLit)[i];                                         }
static inline int         Gia_ObjCellId( Gia_Man_t * p, int iLit )          { return Gia_ObjCellFanins(p, iLit)[Gia_ObjCellSize(p, iLit)];                  }

#define Gia_ManForEachLut( p, i )                                       \
    for ( i = 1; i < Gia_ManObjNum(p); i++ ) if ( !Gia_ObjIsLut(p, i) ) {} else
#define Gia_ManForEachLutReverse( p, i )                                \
    for ( i = Gia_ManObjNum(p) - 1; i > 0; i-- ) if ( !Gia_ObjIsLut(p, i) ) {} else
#define Gia_LutForEachFanin( p, i, iFan, k )                            \
    for ( k = 0; k < Gia_ObjLutSize(p,i) && ((iFan = Gia_ObjLutFanins(p,i)[k]),1); k++ )
#define Gia_LutForEachFaninObj( p, i, pFanin, k )                       \
    for ( k = 0; k < Gia_ObjLutSize(p,i) && ((pFanin = Gia_ManObj(p, Gia_ObjLutFanins(p,i)[k])),1); k++ )

#define Gia_ManForEachLut2( p, i )                                      \
    for ( i = 1; i < Gia_ManObjNum(p); i++ ) if ( !Gia_ObjIsLut2(p, i) ) {} else
#define Gia_ManForEachLut2Reverse( p, i )                               \
    for ( i = Gia_ManObjNum(p) - 1; i > 0; i-- ) if ( !Gia_ObjIsLut2(p, i) ) {} else
#define Gia_ManForEachLut2Vec( vIds, p, vVec, iObj, i )                 \
    for ( i = 0; i < Vec_IntSize(vIds) && (vVec = Vec_WecEntry(p->vMapping2, (iObj = Vec_IntEntry(vIds, i)))); i++ )
#define Gia_ManForEachLut2VecReverse( vIds, p, vVec, iObj, i )          \
    for ( i = Vec_IntSize(vIds)-1; i >= 0 && (vVec = Vec_WecEntry(p->vMapping2, (iObj = Vec_IntEntry(vIds, i)))); i-- )
#define Gia_LutForEachFanin2( p, i, iFan, k )                           \
    for ( k = 0; k < Gia_ObjLutSize2(p,i) && ((iFan = Gia_ObjLutFanin2(p,i,k)),1); k++ )
#define Gia_LutForEachFanout2( p, i, iFan, k )                          \
    for ( k = 0; k < Gia_ObjLutFanoutNum2(p,i) && ((iFan = Gia_ObjLutFanout2(p,i,k)),1); k++ )

#define Gia_ManForEachCell( p, i )                                      \
    for ( i = 2; i < 2*Gia_ManObjNum(p); i++ ) if ( !Gia_ObjIsCell(p, i) ) {} else
#define Gia_CellForEachFanin( p, i, iFanLit, k )                        \
    for ( k = 0; k < Gia_ObjCellSize(p,i) && ((iFanLit = Gia_ObjCellFanins(p,i)[k]),1); k++ )

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define Gia_ManForEachObj( p, pObj, i )                                 \
    for ( i = 0; (i < p->nObjs) && ((pObj) = Gia_ManObj(p, i)); i++ )
#define Gia_ManForEachObj1( p, pObj, i )                                \
    for ( i = 1; (i < p->nObjs) && ((pObj) = Gia_ManObj(p, i)); i++ )
#define Gia_ManForEachObjVec( vVec, p, pObj, i )                        \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pObj) = Gia_ManObj(p, Vec_IntEntry(vVec,i))); i++ )
#define Gia_ManForEachObjVecReverse( vVec, p, pObj, i )                        \
    for ( i = Vec_IntSize(vVec) - 1; (i >= 0) && ((pObj) = Gia_ManObj(p, Vec_IntEntry(vVec,i))); i-- )
#define Gia_ManForEachObjVecLit( vVec, p, pObj, fCompl, i )             \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pObj) = Gia_ManObj(p, Abc_Lit2Var(Vec_IntEntry(vVec,i)))) && (((fCompl) = Abc_LitIsCompl(Vec_IntEntry(vVec,i))),1); i++ )
#define Gia_ManForEachObjReverse( p, pObj, i )                          \
    for ( i = p->nObjs - 1; (i >= 0) && ((pObj) = Gia_ManObj(p, i)); i-- )
#define Gia_ManForEachObjReverse1( p, pObj, i )                         \
    for ( i = p->nObjs - 1; (i > 0) && ((pObj) = Gia_ManObj(p, i)); i-- )
#define Gia_ManForEachBuf( p, pObj, i )                                 \
    for ( i = Gia_ManBufNum(p) ? 0 : p->nObjs; (i < p->nObjs) && ((pObj) = Gia_ManObj(p, i)); i++ )      if ( !Gia_ObjIsBuf(pObj) ) {} else
#define Gia_ManForEachBufId( p, i )                                     \
    for ( i = 0; (i < p->nObjs); i++ )                                     if ( !Gia_ObjIsBuf(Gia_ManObj(p, i)) ) {} else
#define Gia_ManForEachAnd( p, pObj, i )                                 \
    for ( i = 0; (i < p->nObjs) && ((pObj) = Gia_ManObj(p, i)); i++ )      if ( !Gia_ObjIsAnd(pObj) ) {} else
#define Gia_ManForEachAndId( p, i )                                     \
    for ( i = 0; (i < p->nObjs); i++ )                                     if ( !Gia_ObjIsAnd(Gia_ManObj(p, i)) ) {} else
#define Gia_ManForEachMuxId( p, i )                                     \
    for ( i = 0; (i < p->nObjs); i++ )                                     if ( !Gia_ObjIsMuxId(p, i) ) {} else
#define Gia_ManForEachCand( p, pObj, i )                                \
    for ( i = 0; (i < p->nObjs) && ((pObj) = Gia_ManObj(p, i)); i++ )      if ( !Gia_ObjIsCand(pObj) ) {} else
#define Gia_ManForEachAndReverse( p, pObj, i )                          \
    for ( i = p->nObjs - 1; (i > 0) && ((pObj) = Gia_ManObj(p, i)); i-- )  if ( !Gia_ObjIsAnd(pObj) ) {} else
#define Gia_ManForEachAndReverseId( p, i )                              \
    for ( i = p->nObjs - 1; (i > 0); i-- )                                 if ( !Gia_ObjIsAnd(Gia_ManObj(p, i)) ) {} else
#define Gia_ManForEachMux( p, pObj, i )                                 \
    for ( i = 0; (i < p->nObjs) && ((pObj) = Gia_ManObj(p, i)); i++ )      if ( !Gia_ObjIsMuxId(p, i) ) {} else
#define Gia_ManForEachCi( p, pObj, i )                                  \
    for ( i = 0; (i < Vec_IntSize(p->vCis)) && ((pObj) = Gia_ManCi(p, i)); i++ )
#define Gia_ManForEachCiId( p, Id, i )                                  \
    for ( i = 0; (i < Vec_IntSize(p->vCis)) && ((Id) = Gia_ObjId(p, Gia_ManCi(p, i))); i++ )
#define Gia_ManForEachCiVec( vVec, p, pObj, i )                         \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pObj) = Gia_ManCi(p, Vec_IntEntry(vVec,i))); i++ )
#define Gia_ManForEachCiReverse( p, pObj, i )                           \
    for ( i = Vec_IntSize(p->vCis) - 1; (i >= 0) && ((pObj) = Gia_ManCi(p, i)); i-- )
#define Gia_ManForEachCo( p, pObj, i )                                  \
    for ( i = 0; (i < Vec_IntSize(p->vCos)) && ((pObj) = Gia_ManCo(p, i)); i++ )
#define Gia_ManForEachCoVec( vVec, p, pObj, i )                         \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((pObj) = Gia_ManCo(p, Vec_IntEntry(vVec,i))); i++ )
#define Gia_ManForEachCoId( p, Id, i )                                  \
    for ( i = 0; (i < Vec_IntSize(p->vCos)) && ((Id) = Gia_ObjId(p, Gia_ManCo(p, i))); i++ )
#define Gia_ManForEachCoReverse( p, pObj, i )                           \
    for ( i = Vec_IntSize(p->vCos) - 1; (i >= 0) && ((pObj) = Gia_ManCo(p, i)); i-- )
#define Gia_ManForEachCoDriver( p, pObj, i )                            \
    for ( i = 0; (i < Vec_IntSize(p->vCos)) && ((pObj) = Gia_ObjFanin0(Gia_ManCo(p, i))); i++ )
#define Gia_ManForEachCoDriverId( p, DriverId, i )                      \
    for ( i = 0; (i < Vec_IntSize(p->vCos)) && (((DriverId) = Gia_ObjFaninId0p(p, Gia_ManCo(p, i))), 1); i++ )
#define Gia_ManForEachPi( p, pObj, i )                                  \
    for ( i = 0; (i < Gia_ManPiNum(p)) && ((pObj) = Gia_ManCi(p, i)); i++ )
#define Gia_ManForEachPo( p, pObj, i )                                  \
    for ( i = 0; (i < Gia_ManPoNum(p)) && ((pObj) = Gia_ManCo(p, i)); i++ )
#define Gia_ManForEachRo( p, pObj, i )                                  \
    for ( i = 0; (i < Gia_ManRegNum(p)) && ((pObj) = Gia_ManCi(p, Gia_ManPiNum(p)+i)); i++ )
#define Gia_ManForEachRi( p, pObj, i )                                  \
    for ( i = 0; (i < Gia_ManRegNum(p)) && ((pObj) = Gia_ManCo(p, Gia_ManPoNum(p)+i)); i++ )
#define Gia_ManForEachRiRo( p, pObjRi, pObjRo, i )                      \
    for ( i = 0; (i < Gia_ManRegNum(p)) && ((pObjRi) = Gia_ManCo(p, Gia_ManPoNum(p)+i)) && ((pObjRo) = Gia_ManCi(p, Gia_ManPiNum(p)+i)); i++ )
 
////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== giaAiger.c ===========================================================*/
extern int                 Gia_FileSize( char * pFileName );
extern Gia_Man_t *         Gia_AigerReadFromMemory( char * pContents, int nFileSize, int fGiaSimple, int fSkipStrash, int fCheck );
extern Gia_Man_t *         Gia_AigerRead( char * pFileName, int fGiaSimple, int fSkipStrash, int fCheck );
extern void                Gia_AigerWrite( Gia_Man_t * p, char * pFileName, int fWriteSymbols, int fCompact );
extern void                Gia_DumpAiger( Gia_Man_t * p, char * pFilePrefix, int iFileNum, int nFileNumDigits );
extern Vec_Str_t *         Gia_AigerWriteIntoMemoryStr( Gia_Man_t * p );
extern Vec_Str_t *         Gia_AigerWriteIntoMemoryStrPart( Gia_Man_t * p, Vec_Int_t * vCis, Vec_Int_t * vAnds, Vec_Int_t * vCos, int nRegs );
extern void                Gia_AigerWriteSimple( Gia_Man_t * pInit, char * pFileName );
/*=== giaBalance.c ===========================================================*/
extern Gia_Man_t *         Gia_ManBalance( Gia_Man_t * p, int fSimpleAnd, int fStrict, int fVerbose );
extern Gia_Man_t *         Gia_ManAreaBalance( Gia_Man_t * p, int fSimpleAnd, int nNewNodesMax, int fVerbose, int fVeryVerbose );
extern Gia_Man_t *         Gia_ManAigSyn2( Gia_Man_t * p, int fOldAlgo, int fCoarsen, int fCutMin, int nRelaxRatio, int fDelayMin, int fVerbose, int fVeryVerbose );
extern Gia_Man_t *         Gia_ManAigSyn3( Gia_Man_t * p, int fVerbose, int fVeryVerbose );
extern Gia_Man_t *         Gia_ManAigSyn4( Gia_Man_t * p, int fVerbose, int fVeryVerbose );
/*=== giaBidec.c ===========================================================*/
extern unsigned *          Gia_ManConvertAigToTruth( Gia_Man_t * p, Gia_Obj_t * pRoot, Vec_Int_t * vLeaves, Vec_Int_t * vTruth, Vec_Int_t * vVisited );
extern Gia_Man_t *         Gia_ManPerformBidec( Gia_Man_t * p, int fVerbose );
/*=== giaCex.c ============================================================*/
extern int                 Gia_ManVerifyCex( Gia_Man_t * pAig, Abc_Cex_t * p, int fDualOut );
extern int                 Gia_ManFindFailedPoCex( Gia_Man_t * pAig, Abc_Cex_t * p, int nOutputs );
extern int                 Gia_ManSetFailedPoCex( Gia_Man_t * pAig, Abc_Cex_t * p );
extern void                Gia_ManCounterExampleValueStart( Gia_Man_t * pGia, Abc_Cex_t * pCex );
extern void                Gia_ManCounterExampleValueStop( Gia_Man_t * pGia );
extern int                 Gia_ManCounterExampleValueLookup( Gia_Man_t * pGia, int Id, int iFrame );
extern Abc_Cex_t *         Gia_ManCexExtendToIncludeCurrentStates( Gia_Man_t * p, Abc_Cex_t * pCex );
extern Abc_Cex_t *         Gia_ManCexExtendToIncludeAllObjects( Gia_Man_t * p, Abc_Cex_t * pCex );
/*=== giaCsatOld.c ============================================================*/
extern Vec_Int_t *         Cbs_ManSolveMiter( Gia_Man_t * pGia, int nConfs, Vec_Str_t ** pvStatus, int fVerbose );
/*=== giaCsat.c ============================================================*/
typedef struct Cbs_Man_t_  Cbs_Man_t;
extern Cbs_Man_t *         Cbs_ManAlloc( Gia_Man_t * pGia );
extern void                Cbs_ManStop( Cbs_Man_t * p );
extern int                 Cbs_ManSolve( Cbs_Man_t * p, Gia_Obj_t * pObj );
extern int                 Cbs_ManSolve2( Cbs_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pObj2 );
extern Vec_Int_t *         Cbs_ManSolveMiterNc( Gia_Man_t * pGia, int nConfs, Vec_Str_t ** pvStatus, int fVerbose );
extern void                Cbs_ManSetConflictNum( Cbs_Man_t * p, int Num );
extern Vec_Int_t *         Cbs_ReadModel( Cbs_Man_t * p );
/*=== giaCTas.c ============================================================*/
extern Vec_Int_t *         Tas_ManSolveMiterNc( Gia_Man_t * pGia, int nConfs, Vec_Str_t ** pvStatus, int fVerbose );
/*=== giaCof.c =============================================================*/
extern void                Gia_ManPrintFanio( Gia_Man_t * pGia, int nNodes );
extern Gia_Man_t *         Gia_ManDupCof( Gia_Man_t * p, int iVar );
extern Gia_Man_t *         Gia_ManDupCofAllInt( Gia_Man_t * p, Vec_Int_t * vSigs, int fVerbose );
extern Gia_Man_t *         Gia_ManDupCofAll( Gia_Man_t * p, int nFanLim, int fVerbose );
/*=== giaDfs.c ============================================================*/
extern void                Gia_ManCollectCis( Gia_Man_t * p, int * pNodes, int nNodes, Vec_Int_t * vSupp );
extern void                Gia_ManCollectAnds_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vNodes );
extern void                Gia_ManCollectAnds( Gia_Man_t * p, int * pNodes, int nNodes, Vec_Int_t * vNodes, Vec_Int_t * vLeaves );
extern Vec_Int_t *         Gia_ManCollectNodesCis( Gia_Man_t * p, int * pNodes, int nNodes );
extern int                 Gia_ManSuppSize( Gia_Man_t * p, int * pNodes, int nNodes );
extern int                 Gia_ManConeSize( Gia_Man_t * p, int * pNodes, int nNodes );
extern Vec_Vec_t *         Gia_ManLevelize( Gia_Man_t * p );
extern Vec_Int_t *         Gia_ManOrderReverse( Gia_Man_t * p );
extern void                Gia_ManCollectTfi( Gia_Man_t * p, Vec_Int_t * vRoots, Vec_Int_t * vNodes );
extern void                Gia_ManCollectTfo( Gia_Man_t * p, Vec_Int_t * vRoots, Vec_Int_t * vNodes );
/*=== giaDup.c ============================================================*/
extern void                Gia_ManDupRemapLiterals( Vec_Int_t * vLits, Gia_Man_t * p );
extern void                Gia_ManDupRemapEquiv( Gia_Man_t * pNew, Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupOrderDfs( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupOrderDfsChoices( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupOrderDfsReverse( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupOutputGroup( Gia_Man_t * p, int iOutStart, int iOutStop );
extern Gia_Man_t *         Gia_ManDupOutputVec( Gia_Man_t * p, Vec_Int_t * vOutPres );
extern Gia_Man_t *         Gia_ManDupSelectedOutputs( Gia_Man_t * p, Vec_Int_t * vOutsLeft );
extern Gia_Man_t *         Gia_ManDupOrderAiger( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupLastPis( Gia_Man_t * p, int nLastPis );
extern Gia_Man_t *         Gia_ManDupFlip( Gia_Man_t * p, int * pInitState );
extern Gia_Man_t *         Gia_ManDupCycled( Gia_Man_t * pAig, Abc_Cex_t * pCex, int nFrames );
extern Gia_Man_t *         Gia_ManDup( Gia_Man_t * p );  
extern Gia_Man_t *         Gia_ManDup2( Gia_Man_t * p1, Gia_Man_t * p2 );
extern Gia_Man_t *         Gia_ManDupWithAttributes( Gia_Man_t * p );  
extern Gia_Man_t *         Gia_ManDupRemovePis( Gia_Man_t * p, int nRemPis );
extern Gia_Man_t *         Gia_ManDupZero( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupPerm( Gia_Man_t * p, Vec_Int_t * vPiPerm );
extern Gia_Man_t *         Gia_ManDupPermFlop( Gia_Man_t * p, Vec_Int_t * vFfPerm );
extern Gia_Man_t *         Gia_ManDupPermFlopGap( Gia_Man_t * p, Vec_Int_t * vFfPerm );
extern void                Gia_ManDupAppend( Gia_Man_t * p, Gia_Man_t * pTwo );
extern void                Gia_ManDupAppendShare( Gia_Man_t * p, Gia_Man_t * pTwo );
extern Gia_Man_t *         Gia_ManDupAppendNew( Gia_Man_t * pOne, Gia_Man_t * pTwo );
extern Gia_Man_t *         Gia_ManDupAppendCones( Gia_Man_t * p, Gia_Man_t ** ppCones, int nCones, int fOnlyRegs );
extern Gia_Man_t *         Gia_ManDupSelf( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupFlopClass( Gia_Man_t * p, int iClass );
extern Gia_Man_t *         Gia_ManDupMarked( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupTimes( Gia_Man_t * p, int nTimes );  
extern Gia_Man_t *         Gia_ManDupDfs( Gia_Man_t * p );  
extern Gia_Man_t *         Gia_ManDupCofactorVar( Gia_Man_t * p, int iVar, int Value );  
extern Gia_Man_t *         Gia_ManDupCofactorObj( Gia_Man_t * p, int iObj, int Value );  
extern Gia_Man_t *         Gia_ManDupMux( int iVar, Gia_Man_t * pCof1, Gia_Man_t * pCof0 );
extern Gia_Man_t *         Gia_ManDupBlock( Gia_Man_t * p, int nBlock );
extern Gia_Man_t *         Gia_ManDupExist( Gia_Man_t * p, int iVar );
extern Gia_Man_t *         Gia_ManDupUniv( Gia_Man_t * p, int iVar );
extern Gia_Man_t *         Gia_ManDupDfsSkip( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupDfsCone( Gia_Man_t * p, Gia_Obj_t * pObj );
extern Gia_Man_t *         Gia_ManDupConeSupp( Gia_Man_t * p, int iLit, Vec_Int_t * vCiIds );
extern int                 Gia_ManDupConeBack( Gia_Man_t * p, Gia_Man_t * pNew, Vec_Int_t * vCiIds );
extern int                 Gia_ManDupConeBackObjs( Gia_Man_t * p, Gia_Man_t * pNew, Vec_Int_t * vObjs );
extern Gia_Man_t *         Gia_ManDupDfsNode( Gia_Man_t * p, Gia_Obj_t * pObj );
extern Gia_Man_t *         Gia_ManDupDfsLitArray( Gia_Man_t * p, Vec_Int_t * vLits );
extern Gia_Man_t *         Gia_ManDupTrimmed( Gia_Man_t * p, int fTrimCis, int fTrimCos, int fDualOut, int OutValue );
extern Gia_Man_t *         Gia_ManDupOntop( Gia_Man_t * p, Gia_Man_t * p2 );
extern Gia_Man_t *         Gia_ManDupWithNewPo( Gia_Man_t * p1, Gia_Man_t * p2 );
extern Gia_Man_t *         Gia_ManDupDfsCiMap( Gia_Man_t * p, int * pCi2Lit, Vec_Int_t * vLits );
extern Gia_Man_t *         Gia_ManPermuteInputs( Gia_Man_t * p, int nPpis, int nExtra );
extern Gia_Man_t *         Gia_ManDupDfsClasses( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupTopAnd( Gia_Man_t * p, int fVerbose );
extern Gia_Man_t *         Gia_ManMiter( Gia_Man_t * pAig0, Gia_Man_t * pAig1, int nInsDup, int fDualOut, int fSeq, int fImplic, int fVerbose );
extern Gia_Man_t *         Gia_ManDupAndOr( Gia_Man_t * p, int nOuts, int fUseOr, int fCompl );
extern Gia_Man_t *         Gia_ManDupZeroUndc( Gia_Man_t * p, char * pInit, int fGiaSimple, int fVerbose );
extern Gia_Man_t *         Gia_ManMiter2( Gia_Man_t * p, char * pInit, int fVerbose );
extern Gia_Man_t *         Gia_ManTransformMiter( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManTransformMiter2( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManTransformToDual( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManTransformTwoWord2DualOutput( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManChoiceMiter( Vec_Ptr_t * vGias );
extern Gia_Man_t *         Gia_ManDupWithConstraints( Gia_Man_t * p, Vec_Int_t * vPoTypes );
extern Gia_Man_t *         Gia_ManDupCones( Gia_Man_t * p, int * pPos, int nPos, int fTrimPis );
extern Gia_Man_t *         Gia_ManDupAndCones( Gia_Man_t * p, int * pAnds, int nAnds, int fTrimPis );
extern Gia_Man_t *         Gia_ManDupAndConesLimit( Gia_Man_t * p, int * pAnds, int nAnds, int Level );
extern Gia_Man_t *         Gia_ManDupAndConesLimit2( Gia_Man_t * p, int * pAnds, int nAnds, int Level );
extern Gia_Man_t *         Gia_ManDupOneHot( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupLevelized( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupFromVecs( Gia_Man_t * p, Vec_Int_t * vCis, Vec_Int_t * vAnds, Vec_Int_t * vCos, int nRegs );
extern Gia_Man_t *         Gia_ManDupSliced( Gia_Man_t * p, int nSuppMax );
extern Gia_Man_t *         Gia_ManDupDemiter( Gia_Man_t * p, int fVerbose );
extern Gia_Man_t *         Gia_ManDemiterToDual( Gia_Man_t * p );
extern int                 Gia_ManDemiterDual( Gia_Man_t * p, Gia_Man_t ** pp0, Gia_Man_t ** pp1 );
extern int                 Gia_ManDemiterTwoWords( Gia_Man_t * p, Gia_Man_t ** pp0, Gia_Man_t ** pp1 );
/*=== giaEdge.c ==========================================================*/
extern void                Gia_ManEdgeFromArray( Gia_Man_t * p, Vec_Int_t * vArray );
extern Vec_Int_t *         Gia_ManEdgeToArray( Gia_Man_t * p );
extern void                Gia_ManConvertPackingToEdges( Gia_Man_t * p );
extern int                 Gia_ObjCheckEdge( Gia_Man_t * p, int iObj, int iNext );
extern int                 Gia_ManEvalEdgeDelay( Gia_Man_t * p );
extern int                 Gia_ManEvalEdgeCount( Gia_Man_t * p );
extern int                 Gia_ManComputeEdgeDelay( Gia_Man_t * p, int fUseTwo );
extern int                 Gia_ManComputeEdgeDelay2( Gia_Man_t * p );
extern void                Gia_ManUpdateMapping( Gia_Man_t * p, Vec_Int_t * vNodes, Vec_Wec_t * vWin );
extern int                 Gia_ManEvalWindow( Gia_Man_t * p, Vec_Int_t * vLeaves, Vec_Int_t * vNodes, Vec_Wec_t * vWin, Vec_Int_t * vTemp, int fUseTwo );
/*=== giaEnable.c ==========================================================*/
extern void                Gia_ManDetectSeqSignals( Gia_Man_t * p, int fSetReset, int fVerbose );
extern Gia_Man_t *         Gia_ManUnrollAndCofactor( Gia_Man_t * p, int nFrames, int nFanMax, int fVerbose );
extern Gia_Man_t *         Gia_ManRemoveEnables( Gia_Man_t * p );
/*=== giaEquiv.c ==========================================================*/
extern void                Gia_ManOrigIdsInit( Gia_Man_t * p );
extern void                Gia_ManOrigIdsStart( Gia_Man_t * p );
extern void                Gia_ManOrigIdsRemap( Gia_Man_t * p, Gia_Man_t * pNew );
extern Gia_Man_t *         Gia_ManOrigIdsReduce( Gia_Man_t * p, Vec_Int_t * vPairs );
extern Gia_Man_t *         Gia_ManComputeGiaEquivs( Gia_Man_t * pGia, int nConfs, int fVerbose );
extern void                Gia_ManEquivFixOutputPairs( Gia_Man_t * p );
extern int                 Gia_ManCheckTopoOrder( Gia_Man_t * p );
extern int *               Gia_ManDeriveNexts( Gia_Man_t * p );
extern void                Gia_ManDeriveReprs( Gia_Man_t * p );
extern int                 Gia_ManEquivCountLits( Gia_Man_t * p );
extern int                 Gia_ManEquivCountLitsAll( Gia_Man_t * p );
extern int                 Gia_ManEquivCountClasses( Gia_Man_t * p );
extern void                Gia_ManEquivPrintOne( Gia_Man_t * p, int i, int Counter );
extern void                Gia_ManEquivPrintClasses( Gia_Man_t * p, int fVerbose, float Mem );
extern Gia_Man_t *         Gia_ManEquivReduce( Gia_Man_t * p, int fUseAll, int fDualOut, int fSkipPhase, int fVerbose );
extern Gia_Man_t *         Gia_ManEquivReduceAndRemap( Gia_Man_t * p, int fSeq, int fMiterPairs );
extern int                 Gia_ManEquivSetColors( Gia_Man_t * p, int fVerbose );
extern Gia_Man_t *         Gia_ManSpecReduce( Gia_Man_t * p, int fDualOut, int fSynthesis, int fReduce, int fSkipSome, int fVerbose );
extern Gia_Man_t *         Gia_ManSpecReduceInit( Gia_Man_t * p, Abc_Cex_t * pInit, int nFrames, int fDualOut );
extern Gia_Man_t *         Gia_ManSpecReduceInitFrames( Gia_Man_t * p, Abc_Cex_t * pInit, int nFramesMax, int * pnFrames, int fDualOut, int nMinOutputs );
extern void                Gia_ManEquivTransform( Gia_Man_t * p, int fVerbose );
extern void                Gia_ManEquivImprove( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManEquivToChoices( Gia_Man_t * p, int nSnapshots );
extern int                 Gia_ManCountChoiceNodes( Gia_Man_t * p );
extern int                 Gia_ManCountChoices( Gia_Man_t * p );
extern int                 Gia_ManFilterEquivsForSpeculation( Gia_Man_t * pGia, char * pName1, char * pName2, int fLatchA, int fLatchB );
extern int                 Gia_ManFilterEquivsUsingParts( Gia_Man_t * pGia, char * pName1, char * pName2 );
extern void                Gia_ManFilterEquivsUsingLatches( Gia_Man_t * pGia, int fFlopsOnly, int fFlopsWith, int fUseRiDrivers );
/*=== giaExist.c =========================================================*/
extern void                Gia_ManQuantSetSuppStart( Gia_Man_t * p );
extern void                Gia_ManQuantSetSuppZero( Gia_Man_t * p );
extern void                Gia_ManQuantSetSuppCi( Gia_Man_t * p, Gia_Obj_t * pObj );
extern void                Gia_ManQuantUpdateCiSupp( Gia_Man_t * p, int iObj );
extern int                 Gia_ManQuantExist( Gia_Man_t * p, int iLit, int(*pFuncCiToKeep)(void *, int), void * pData );
/*=== giaFanout.c =========================================================*/
extern void                Gia_ObjAddFanout( Gia_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pFanout );
extern void                Gia_ObjRemoveFanout( Gia_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pFanout );
extern void                Gia_ManFanoutStart( Gia_Man_t * p );
extern void                Gia_ManFanoutStop( Gia_Man_t * p );
extern void                Gia_ManStaticFanoutStart( Gia_Man_t * p );
extern void                Gia_ManStaticFanoutStop( Gia_Man_t * p );
/*=== giaForce.c =========================================================*/
extern void                For_ManExperiment( Gia_Man_t * pGia, int nIters, int fClustered, int fVerbose );
/*=== giaFrames.c =========================================================*/
extern Gia_Man_t *         Gia_ManUnrollDup( Gia_Man_t * p, Vec_Int_t * vLimit );
extern Vec_Ptr_t *         Gia_ManUnrollAbs( Gia_Man_t * p, int nFrames );
extern void *              Gia_ManUnrollStart( Gia_Man_t * pAig, Gia_ParFra_t * pPars );
extern void *              Gia_ManUnrollAdd( void * pMan, int fMax );
extern void                Gia_ManUnrollStop( void * pMan );
extern int                 Gia_ManUnrollLastLit( void * pMan );
extern void                Gia_ManFraSetDefaultParams( Gia_ParFra_t * p );
extern Gia_Man_t *         Gia_ManFrames( Gia_Man_t * pAig, Gia_ParFra_t * pPars );  
extern Gia_Man_t *         Gia_ManFramesInitSpecial( Gia_Man_t * pAig, int nFrames, int fVerbose );
/*=== giaFront.c ==========================================================*/
extern Gia_Man_t *         Gia_ManFront( Gia_Man_t * p );
extern void                Gia_ManFrontTest( Gia_Man_t * p );
/*=== giaFx.c ==========================================================*/
extern Gia_Man_t *         Gia_ManPerformFx( Gia_Man_t * p, int nNewNodesMax, int LitCountMax, int fReverse, int fVerbose, int fVeryVerbose );
/*=== giaHash.c ===========================================================*/
extern void                Gia_ManHashAlloc( Gia_Man_t * p ); 
extern void                Gia_ManHashStart( Gia_Man_t * p ); 
extern void                Gia_ManHashStop( Gia_Man_t * p );
extern int                 Gia_ManHashXorReal( Gia_Man_t * p, int iLit0, int iLit1 );
extern int                 Gia_ManHashMuxReal( Gia_Man_t * p, int iLitC, int iLit1, int iLit0 );
extern int                 Gia_ManHashAnd( Gia_Man_t * p, int iLit0, int iLit1 ); 
extern int                 Gia_ManHashOr( Gia_Man_t * p, int iLit0, int iLit1 ); 
extern int                 Gia_ManHashXor( Gia_Man_t * p, int iLit0, int iLit1 ); 
extern int                 Gia_ManHashMux( Gia_Man_t * p, int iCtrl, int iData1, int iData0 );
extern int                 Gia_ManHashMaj( Gia_Man_t * p, int iData0, int iData1, int iData2 );
extern int                 Gia_ManHashAndTry( Gia_Man_t * p, int iLit0, int iLit1 );
extern Gia_Man_t *         Gia_ManRehash( Gia_Man_t * p, int fAddStrash );
extern void                Gia_ManHashProfile( Gia_Man_t * p );
extern int                 Gia_ManHashLookupInt( Gia_Man_t * p, int iLit0, int iLit1 );
extern int                 Gia_ManHashLookup( Gia_Man_t * p, Gia_Obj_t * p0, Gia_Obj_t * p1 );
extern int                 Gia_ManHashAndMulti( Gia_Man_t * p, Vec_Int_t * vLits );
extern int                 Gia_ManHashAndMulti2( Gia_Man_t * p, Vec_Int_t * vLits );
/*=== giaIf.c ===========================================================*/
extern void                Gia_ManPrintMappingStats( Gia_Man_t * p, char * pDumpFile );
extern void                Gia_ManPrintPackingStats( Gia_Man_t * p );
extern void                Gia_ManPrintLutStats( Gia_Man_t * p );
extern int                 Gia_ManLutFaninCount( Gia_Man_t * p );
extern int                 Gia_ManLutSizeMax( Gia_Man_t * p );
extern int                 Gia_ManLutNum( Gia_Man_t * p );
extern int                 Gia_ManLutLevel( Gia_Man_t * p, int ** ppLevels );
extern void                Gia_ManLutParams( Gia_Man_t * p, int * pnCurLuts, int * pnCurEdges, int * pnCurLevels );
extern void                Gia_ManSetRefsMapped( Gia_Man_t * p );
extern void                Gia_ManSetLutRefs( Gia_Man_t * p );  
extern void                Gia_ManSetIfParsDefault( void * pIfPars );
extern void                Gia_ManMappingVerify( Gia_Man_t * p );
extern void                Gia_ManTransferMapping( Gia_Man_t * p, Gia_Man_t * pGia );
extern void                Gia_ManTransferPacking( Gia_Man_t * p, Gia_Man_t * pGia );
extern void                Gia_ManTransferTiming( Gia_Man_t * p, Gia_Man_t * pGia );
extern Gia_Man_t *         Gia_ManPerformMapping( Gia_Man_t * p, void * pIfPars );
extern Gia_Man_t *         Gia_ManPerformSopBalance( Gia_Man_t * p, int nCutNum, int nRelaxRatio, int fVerbose );
extern Gia_Man_t *         Gia_ManPerformDsdBalance( Gia_Man_t * p, int nLutSize, int nCutNum, int nRelaxRatio, int fVerbose );
extern Gia_Man_t *         Gia_ManDupHashMapping( Gia_Man_t * p );
/*=== giaJf.c ===========================================================*/
extern void                Jf_ManSetDefaultPars( Jf_Par_t * pPars );
extern Gia_Man_t *         Jf_ManPerformMapping( Gia_Man_t * pGia, Jf_Par_t * pPars );
extern Gia_Man_t *         Jf_ManDeriveCnf( Gia_Man_t * p, int fCnfObjIds );
/*=== giaIso.c ===========================================================*/
extern Gia_Man_t *         Gia_ManIsoCanonicize( Gia_Man_t * p, int fVerbose );
extern Gia_Man_t *         Gia_ManIsoReduce( Gia_Man_t * p, Vec_Ptr_t ** pvPosEquivs, Vec_Ptr_t ** pvPiPerms, int fEstimate, int fDualOut, int fVerbose, int fVeryVerbose );
extern Gia_Man_t *         Gia_ManIsoReduce2( Gia_Man_t * p, Vec_Ptr_t ** pvPosEquivs, Vec_Ptr_t ** pvPiPerms, int fEstimate, int fBetterQual, int fDualOut, int fVerbose, int fVeryVerbose );
/*=== giaLf.c ===========================================================*/
extern void                Lf_ManSetDefaultPars( Jf_Par_t * pPars );
extern Gia_Man_t *         Lf_ManPerformMapping( Gia_Man_t * pGia, Jf_Par_t * pPars );
extern Gia_Man_t *         Gia_ManPerformLfMapping( Gia_Man_t * p, Jf_Par_t * pPars, int fNormalized );
/*=== giaLogic.c ===========================================================*/
extern void                Gia_ManTestDistance( Gia_Man_t * p );
extern void                Gia_ManSolveProblem( Gia_Man_t * pGia, Emb_Par_t * pPars );
 /*=== giaMan.c ===========================================================*/
extern Gia_Man_t *         Gia_ManStart( int nObjsMax ); 
extern void                Gia_ManStop( Gia_Man_t * p );  
extern void                Gia_ManStopP( Gia_Man_t ** p );  
extern double              Gia_ManMemory( Gia_Man_t * p );
extern void                Gia_ManPrintStats( Gia_Man_t * p, Gps_Par_t * pPars ); 
extern void                Gia_ManPrintStatsShort( Gia_Man_t * p ); 
extern void                Gia_ManPrintMiterStatus( Gia_Man_t * p ); 
extern void                Gia_ManPrintStatsMiter( Gia_Man_t * p, int fVerbose );
extern void                Gia_ManSetRegNum( Gia_Man_t * p, int nRegs );
extern void                Gia_ManReportImprovement( Gia_Man_t * p, Gia_Man_t * pNew );
extern void                Gia_ManPrintNpnClasses( Gia_Man_t * p );
/*=== giaMem.c ===========================================================*/
extern Gia_MmFixed_t *     Gia_MmFixedStart( int nEntrySize, int nEntriesMax );
extern void                Gia_MmFixedStop( Gia_MmFixed_t * p, int fVerbose );
extern char *              Gia_MmFixedEntryFetch( Gia_MmFixed_t * p );
extern void                Gia_MmFixedEntryRecycle( Gia_MmFixed_t * p, char * pEntry );
extern void                Gia_MmFixedRestart( Gia_MmFixed_t * p );
extern int                 Gia_MmFixedReadMemUsage( Gia_MmFixed_t * p );
extern int                 Gia_MmFixedReadMaxEntriesUsed( Gia_MmFixed_t * p );
extern Gia_MmFlex_t *      Gia_MmFlexStart();
extern void                Gia_MmFlexStop( Gia_MmFlex_t * p, int fVerbose );
extern char *              Gia_MmFlexEntryFetch( Gia_MmFlex_t * p, int nBytes );
extern void                Gia_MmFlexRestart( Gia_MmFlex_t * p );
extern int                 Gia_MmFlexReadMemUsage( Gia_MmFlex_t * p );
extern Gia_MmStep_t *      Gia_MmStepStart( int nSteps );
extern void                Gia_MmStepStop( Gia_MmStep_t * p, int fVerbose );
extern char *              Gia_MmStepEntryFetch( Gia_MmStep_t * p, int nBytes );
extern void                Gia_MmStepEntryRecycle( Gia_MmStep_t * p, char * pEntry, int nBytes );
extern int                 Gia_MmStepReadMemUsage( Gia_MmStep_t * p );
/*=== giaMf.c ===========================================================*/
extern void                Mf_ManSetDefaultPars( Jf_Par_t * pPars );
extern Gia_Man_t *         Mf_ManPerformMapping( Gia_Man_t * pGia, Jf_Par_t * pPars );
extern void *              Mf_ManGenerateCnf( Gia_Man_t * pGia, int nLutSize, int fCnfObjIds, int fAddOrCla, int fMapping, int fVerbose );
/*=== giaMini.c ===========================================================*/
extern Gia_Man_t *         Gia_ManReadMiniAig( char * pFileName );
extern void                Gia_ManWriteMiniAig( Gia_Man_t * pGia, char * pFileName );
extern Gia_Man_t *         Gia_ManReadMiniLut( char * pFileName );
extern void                Gia_ManWriteMiniLut( Gia_Man_t * pGia, char * pFileName );
/*=== giaMuxes.c ===========================================================*/
extern void                Gia_ManCountMuxXor( Gia_Man_t * p, int * pnMuxes, int * pnXors );
extern void                Gia_ManPrintMuxStats( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupMuxes( Gia_Man_t * p, int Limit );
extern Gia_Man_t *         Gia_ManDupNoMuxes( Gia_Man_t * p );
/*=== giaPat.c ===========================================================*/
extern void                Gia_SatVerifyPattern( Gia_Man_t * p, Gia_Obj_t * pRoot, Vec_Int_t * vCex, Vec_Int_t * vVisit );
/*=== giaRetime.c ===========================================================*/
extern Gia_Man_t *         Gia_ManRetimeForward( Gia_Man_t * p, int nMaxIters, int fVerbose );
/*=== giaSat.c ============================================================*/
extern int                 Sat_ManTest( Gia_Man_t * pGia, Gia_Obj_t * pObj, int nConfsMax );
/*=== giaScl.c ============================================================*/
extern int                 Gia_ManSeqMarkUsed( Gia_Man_t * p );
extern int                 Gia_ManCombMarkUsed( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManCleanup( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManCleanupOutputs( Gia_Man_t * p, int nOutputs );
extern Gia_Man_t *         Gia_ManSeqCleanup( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManSeqStructSweep( Gia_Man_t * p, int fConst, int fEquiv, int fVerbose );
/*=== giaShow.c ===========================================================*/
extern void                Gia_ManShow( Gia_Man_t * pMan, Vec_Int_t * vBold, int fAdders, int fFadds, int fPath );
/*=== giaShrink.c ===========================================================*/
extern Gia_Man_t *         Gia_ManMapShrink4( Gia_Man_t * p, int fKeepLevel, int fVerbose );
extern Gia_Man_t *         Gia_ManMapShrink6( Gia_Man_t * p, int nFanoutMax, int fKeepLevel, int fVerbose );
/*=== giaSopb.c ============================================================*/
extern Gia_Man_t *         Gia_ManExtractWindow( Gia_Man_t * p, int LevelMax, int nTimeWindow, int fVerbose );
extern Gia_Man_t *         Gia_ManPerformSopBalanceWin( Gia_Man_t * p, int LevelMax, int nTimeWindow, int nCutNum, int nRelaxRatio, int fVerbose );
extern Gia_Man_t *         Gia_ManPerformDsdBalanceWin( Gia_Man_t * p, int LevelMax, int nTimeWindow, int nLutSize, int nCutNum, int nRelaxRatio, int fVerbose );
/*=== giaSort.c ============================================================*/
extern int *               Gia_SortFloats( float * pArray, int * pPerm, int nSize );
/*=== giaSim.c ============================================================*/
extern void                Gia_ManSimSetDefaultParams( Gia_ParSim_t * p );
extern int                 Gia_ManSimSimulate( Gia_Man_t * pAig, Gia_ParSim_t * pPars );
extern unsigned *          Gia_SimDataExt( Gia_ManSim_t * p, int i );
extern unsigned *          Gia_SimDataCiExt( Gia_ManSim_t * p, int i );
extern unsigned *          Gia_SimDataCoExt( Gia_ManSim_t * p, int i );
extern void                Gia_ManSimInfoInit( Gia_ManSim_t * p );
extern void                Gia_ManSimInfoTransfer( Gia_ManSim_t * p );
extern void                Gia_ManSimulateRound( Gia_ManSim_t * p );
extern void                Gia_ManBuiltInSimStart( Gia_Man_t * p, int nWords, int nObjs );
extern void                Gia_ManBuiltInSimPerform( Gia_Man_t * p, int iObj );
extern int                 Gia_ManBuiltInSimCheckOver( Gia_Man_t * p, int iLit0, int iLit1 );
extern int                 Gia_ManBuiltInSimCheckEqual( Gia_Man_t * p, int iLit0, int iLit1 );
extern void                Gia_ManBuiltInSimResimulateCone( Gia_Man_t * p, int iLit0, int iLit1 );
extern void                Gia_ManBuiltInSimResimulate( Gia_Man_t * p );
extern int                 Gia_ManBuiltInSimAddPat( Gia_Man_t * p, Vec_Int_t * vPat );
extern void                Gia_ManIncrSimStart( Gia_Man_t * p, int nWords, int nObjs );
extern void                Gia_ManIncrSimSet( Gia_Man_t * p, Vec_Int_t * vObjLits );
extern int                 Gia_ManIncrSimCheckOver( Gia_Man_t * p, int iLit0, int iLit1 );
extern int                 Gia_ManIncrSimCheckEqual( Gia_Man_t * p, int iLit0, int iLit1 );
/*=== giaSpeedup.c ============================================================*/
extern float               Gia_ManDelayTraceLut( Gia_Man_t * p );
extern float               Gia_ManDelayTraceLutPrint( Gia_Man_t * p, int fVerbose );
extern Gia_Man_t *         Gia_ManSpeedup( Gia_Man_t * p, int Percentage, int Degree, int fVerbose, int fVeryVerbose );
/*=== giaSplit.c ============================================================*/
extern void                Gia_ManComputeOneWinStart( Gia_Man_t * p, int nAnds, int fReverse );
extern int                 Gia_ManComputeOneWin( Gia_Man_t * p, int iPivot, Vec_Int_t ** pvRoots, Vec_Int_t ** pvNodes, Vec_Int_t ** pvLeaves, Vec_Int_t ** pvAnds );
/*=== giaStg.c ============================================================*/
extern void                Gia_ManStgPrint( FILE * pFile, Vec_Int_t * vLines, int nIns, int nOuts, int nStates );
extern Gia_Man_t *         Gia_ManStgRead( char * pFileName, int kHot, int fVerbose );
/*=== giaSupp.c ============================================================*/
typedef struct Gia_ManMin_t_ Gia_ManMin_t;
extern Gia_ManMin_t *      Gia_ManSuppStart( Gia_Man_t * pGia );
extern void                Gia_ManSuppStop( Gia_ManMin_t * p );
extern int                 Gia_ManSupportAnd( Gia_ManMin_t * p, int iLit0, int iLit1 );
typedef struct Gia_Man2Min_t_ Gia_Man2Min_t;
extern Gia_Man2Min_t *     Gia_Man2SuppStart( Gia_Man_t * pGia );
extern void                Gia_Man2SuppStop( Gia_Man2Min_t * p );
extern int                 Gia_Man2SupportAnd( Gia_Man2Min_t * p, int iLit0, int iLit1 );
/*=== giaSweep.c ============================================================*/
extern Gia_Man_t *         Gia_ManFraigSweepSimple( Gia_Man_t * p, void * pPars );
extern Gia_Man_t *         Gia_ManSweepWithBoxes( Gia_Man_t * p, void * pParsC, void * pParsS, int fConst, int fEquiv, int fVerbose, int fVerbEquivs );
extern void                Gia_ManCheckIntegrityWithBoxes( Gia_Man_t * p );
/*=== giaSweeper.c ============================================================*/
extern Gia_Man_t *         Gia_SweeperStart( Gia_Man_t * p );
extern void                Gia_SweeperStop( Gia_Man_t * p );
extern int                 Gia_SweeperIsRunning( Gia_Man_t * p );
extern void                Gia_SweeperPrintStats( Gia_Man_t * p );
extern void                Gia_SweeperSetConflictLimit( Gia_Man_t * p, int nConfMax );
extern void                Gia_SweeperSetRuntimeLimit( Gia_Man_t * p, int nSeconds );
extern Vec_Int_t *         Gia_SweeperGetCex( Gia_Man_t * p );
extern int                 Gia_SweeperProbeCreate( Gia_Man_t * p, int iLit );
extern int                 Gia_SweeperProbeDelete( Gia_Man_t * p, int ProbeId );
extern int                 Gia_SweeperProbeUpdate( Gia_Man_t * p, int ProbeId, int iLitNew );
extern int                 Gia_SweeperProbeLit( Gia_Man_t * p, int ProbeId );
extern Vec_Int_t *         Gia_SweeperCollectValidProbeIds( Gia_Man_t * p );
extern int                 Gia_SweeperCondPop( Gia_Man_t * p );
extern void                Gia_SweeperCondPush( Gia_Man_t * p, int ProbeId );
extern Vec_Int_t *         Gia_SweeperCondVector( Gia_Man_t * p );
extern int                 Gia_SweeperCondCheckUnsat( Gia_Man_t * p );
extern int                 Gia_SweeperCheckEquiv( Gia_Man_t * p, int ProbeId1, int ProbeId2 );
extern Gia_Man_t *         Gia_SweeperExtractUserLogic( Gia_Man_t * p, Vec_Int_t * vProbeIds, Vec_Ptr_t * vInNames, Vec_Ptr_t * vOutNames );
extern void                Gia_SweeperLogicDump( Gia_Man_t * p, Vec_Int_t * vProbeIds, int fDumpConds, char * pFileName );
extern Gia_Man_t *         Gia_SweeperCleanup( Gia_Man_t * p, char * pCommLime );
extern Vec_Int_t *         Gia_SweeperGraft( Gia_Man_t * pDst, Vec_Int_t * vProbes, Gia_Man_t * pSrc );
extern int                 Gia_SweeperFraig( Gia_Man_t * p, Vec_Int_t * vProbeIds, char * pCommLime, int nWords, int nConfs, int fVerify, int fVerbose );
extern int                 Gia_SweeperRun( Gia_Man_t * p, Vec_Int_t * vProbeIds, char * pCommLime, int fVerbose );
/*=== giaSwitch.c ============================================================*/
extern float               Gia_ManEvaluateSwitching( Gia_Man_t * p );
extern float               Gia_ManComputeSwitching( Gia_Man_t * p, int nFrames, int nPref, int fProbOne );
extern Vec_Int_t *         Gia_ManComputeSwitchProbs( Gia_Man_t * pGia, int nFrames, int nPref, int fProbOne );
extern Vec_Flt_t *         Gia_ManPrintOutputProb( Gia_Man_t * p );
/*=== giaTim.c ===========================================================*/
extern int                 Gia_ManBoxNum( Gia_Man_t * p );
extern int                 Gia_ManRegBoxNum( Gia_Man_t * p );
extern int                 Gia_ManNonRegBoxNum( Gia_Man_t * p );
extern int                 Gia_ManBlackBoxNum( Gia_Man_t * p );
extern int                 Gia_ManBoxCiNum( Gia_Man_t * p );
extern int                 Gia_ManBoxCoNum( Gia_Man_t * p );
extern int                 Gia_ManClockDomainNum( Gia_Man_t * p );
extern int                 Gia_ManIsSeqWithBoxes( Gia_Man_t * p );
extern int                 Gia_ManIsNormalized( Gia_Man_t * p );
extern Vec_Int_t *         Gia_ManOrderWithBoxes( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupNormalize( Gia_Man_t * p, int fHashMapping );
extern Gia_Man_t *         Gia_ManDupUnnormalize( Gia_Man_t * p );
extern Gia_Man_t *         Gia_ManDupUnshuffleInputs( Gia_Man_t * p );
extern int                 Gia_ManLevelWithBoxes( Gia_Man_t * p );
extern int                 Gia_ManLutLevelWithBoxes( Gia_Man_t * p );
extern void *              Gia_ManUpdateTimMan( Gia_Man_t * p, Vec_Int_t * vBoxPres );
extern void *              Gia_ManUpdateTimMan2( Gia_Man_t * p, Vec_Int_t * vBoxesLeft, int nTermsDiff );
extern Gia_Man_t *         Gia_ManUpdateExtraAig( void * pTime, Gia_Man_t * pAig, Vec_Int_t * vBoxPres );
extern Gia_Man_t *         Gia_ManUpdateExtraAig2( void * pTime, Gia_Man_t * pAig, Vec_Int_t * vBoxesLeft );
extern Gia_Man_t *         Gia_ManDupCollapse( Gia_Man_t * p, Gia_Man_t * pBoxes, Vec_Int_t * vBoxPres, int fSeq );
extern int                 Gia_ManVerifyWithBoxes( Gia_Man_t * pGia, int nBTLimit, int nTimeLim, int fSeq, int fDumpFiles, int fVerbose, char * pFileSpec );
/*=== giaTruth.c ===========================================================*/
extern word                Gia_LutComputeTruth6( Gia_Man_t * p, int iObj, Vec_Wrd_t * vTruths );
extern word                Gia_ObjComputeTruthTable6Lut( Gia_Man_t * p, int iObj, Vec_Wrd_t * vTemp );
extern word                Gia_ObjComputeTruthTable6( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp, Vec_Wrd_t * vTruths );
extern word                Gia_ObjComputeTruth6Cis( Gia_Man_t * p, int iLit, Vec_Int_t * vSupp, Vec_Wrd_t * vTemp );
extern void                Gia_ObjCollectInternal( Gia_Man_t * p, Gia_Obj_t * pObj );
extern word *              Gia_ObjComputeTruthTable( Gia_Man_t * p, Gia_Obj_t * pObj );
extern void                Gia_ObjComputeTruthTableStart( Gia_Man_t * p, int nVarsMax );
extern void                Gia_ObjComputeTruthTableStop( Gia_Man_t * p );
extern word *              Gia_ObjComputeTruthTableCut( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vLeaves );
/*=== giaTsim.c ============================================================*/
extern Gia_Man_t *         Gia_ManReduceConst( Gia_Man_t * pAig, int fVerbose );
/*=== giaUtil.c ===========================================================*/
extern unsigned            Gia_ManRandom( int fReset );
extern word                Gia_ManRandomW( int fReset );
extern void                Gia_ManRandomInfo( Vec_Ptr_t * vInfo, int iInputStart, int iWordStart, int iWordStop );
extern char *              Gia_TimeStamp();
extern char *              Gia_FileNameGenericAppend( char * pBase, char * pSuffix );
extern void                Gia_ManIncrementTravId( Gia_Man_t * p );
extern void                Gia_ManCleanMark01( Gia_Man_t * p );
extern void                Gia_ManSetMark0( Gia_Man_t * p );
extern void                Gia_ManCleanMark0( Gia_Man_t * p );
extern void                Gia_ManCheckMark0( Gia_Man_t * p );
extern void                Gia_ManSetMark1( Gia_Man_t * p );
extern void                Gia_ManCleanMark1( Gia_Man_t * p );
extern void                Gia_ManCheckMark1( Gia_Man_t * p );
extern void                Gia_ManCleanValue( Gia_Man_t * p );
extern void                Gia_ManCleanLevels( Gia_Man_t * p, int Size );
extern void                Gia_ManCleanTruth( Gia_Man_t * p );
extern void                Gia_ManFillValue( Gia_Man_t * p );
extern void                Gia_ObjSetPhase( Gia_Man_t * p, Gia_Obj_t * pObj );
extern void                Gia_ManSetPhase( Gia_Man_t * p );
extern void                Gia_ManSetPhasePattern( Gia_Man_t * p, Vec_Int_t * vCiValues );
extern void                Gia_ManSetPhase1( Gia_Man_t * p );
extern void                Gia_ManCleanPhase( Gia_Man_t * p );
extern int                 Gia_ManCheckCoPhase( Gia_Man_t * p );
extern int                 Gia_ManLevelNum( Gia_Man_t * p );
extern Vec_Int_t *         Gia_ManGetCiLevels( Gia_Man_t * p );
extern int                 Gia_ManSetLevels( Gia_Man_t * p, Vec_Int_t * vCiLevels );
extern Vec_Int_t *         Gia_ManReverseLevel( Gia_Man_t * p );
extern Vec_Int_t *         Gia_ManRequiredLevel( Gia_Man_t * p );
extern void                Gia_ManCreateValueRefs( Gia_Man_t * p );
extern void                Gia_ManCreateRefs( Gia_Man_t * p );
extern int *               Gia_ManCreateMuxRefs( Gia_Man_t * p );
extern int                 Gia_ManCrossCut( Gia_Man_t * p, int fReverse );
extern Vec_Int_t *         Gia_ManCollectPoIds( Gia_Man_t * p );
extern int                 Gia_ObjIsMuxType( Gia_Obj_t * pNode );
extern int                 Gia_ObjRecognizeExor( Gia_Obj_t * pObj, Gia_Obj_t ** ppFan0, Gia_Obj_t ** ppFan1 );
extern Gia_Obj_t *         Gia_ObjRecognizeMux( Gia_Obj_t * pNode, Gia_Obj_t ** ppNodeT, Gia_Obj_t ** ppNodeE );
extern int                 Gia_ObjRecognizeMuxLits( Gia_Man_t * p, Gia_Obj_t * pNode, int * iLitT, int * iLitE );
extern int                 Gia_NodeMffcSize( Gia_Man_t * p, Gia_Obj_t * pNode );
extern int                 Gia_NodeMffcSizeSupp( Gia_Man_t * p, Gia_Obj_t * pNode, Vec_Int_t * vSupp );
extern int                 Gia_ManHasDangling( Gia_Man_t * p );
extern int                 Gia_ManMarkDangling( Gia_Man_t * p );
extern Vec_Int_t *         Gia_ManGetDangling( Gia_Man_t * p );
extern void                Gia_ObjPrint( Gia_Man_t * p, Gia_Obj_t * pObj );
extern void                Gia_ManPrint( Gia_Man_t * p );
extern void                Gia_ManPrintCo( Gia_Man_t * p, Gia_Obj_t * pObj );
extern void                Gia_ManPrintCone( Gia_Man_t * p, Gia_Obj_t * pObj, int * pLeaves, int nLeaves, Vec_Int_t * vNodes );
extern void                Gia_ManPrintConeMulti( Gia_Man_t * p, Vec_Int_t * vObjs, Vec_Int_t * vLeaves, Vec_Int_t * vNodes );
extern void                Gia_ManPrintCone2( Gia_Man_t * p, Gia_Obj_t * pObj );
extern void                Gia_ManInvertConstraints( Gia_Man_t * pAig );
extern void                Gia_ManInvertPos( Gia_Man_t * pAig );
extern int                 Gia_ManCompare( Gia_Man_t * p1, Gia_Man_t * p2 );
extern void                Gia_ManMarkFanoutDrivers( Gia_Man_t * p );
extern void                Gia_ManSwapPos( Gia_Man_t * p, int i );
extern Vec_Int_t *         Gia_ManSaveValue( Gia_Man_t * p );
extern void                Gia_ManLoadValue( Gia_Man_t * p, Vec_Int_t * vValues );
extern Vec_Int_t *         Gia_ManFirstFanouts( Gia_Man_t * p );
extern int                 Gia_ManCheckSuppOverlap( Gia_Man_t * p, int iNode1, int iNode2 );

/*=== giaCTas.c ===========================================================*/
typedef struct Tas_Man_t_  Tas_Man_t;
extern Tas_Man_t *         Tas_ManAlloc( Gia_Man_t * pAig, int nBTLimit );
extern void                Tas_ManStop( Tas_Man_t * p );
extern Vec_Int_t *         Tas_ReadModel( Tas_Man_t * p );
extern void                Tas_ManSatPrintStats( Tas_Man_t * p );
extern int                 Tas_ManSolve( Tas_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pObj2 );
extern int                 Tas_ManSolveArray( Tas_Man_t * p, Vec_Ptr_t * vObjs );


ABC_NAMESPACE_HEADER_END


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

