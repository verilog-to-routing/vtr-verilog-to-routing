/**CFile****************************************************************

  FileName    [wlc.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlc.h,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__wlc__wlc_h
#define ABC__base__wlc__wlc_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/gia/gia.h"
#include "misc/extra/extra.h"
#include "misc/util/utilNam.h"
#include "misc/mem/mem.h"
#include "misc/extra/extra.h"
#include "misc/util/utilTruth.h"
#include "base/main/mainInt.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START 

// object types
typedef enum { 
    WLC_OBJ_NONE = 0,      // 00: unknown
    WLC_OBJ_PI,            // 01: primary input 
    WLC_OBJ_PO,            // 02: primary output (unused)
    WLC_OBJ_FO,            // 03: flop output
    WLC_OBJ_FI,            // 04: flop input (unused)
    WLC_OBJ_FF,            // 05: flop (unused)
    WLC_OBJ_CONST,         // 06: constant
    WLC_OBJ_BUF,           // 07: buffer
    WLC_OBJ_MUX,           // 08: multiplexer
    WLC_OBJ_SHIFT_R,       // 09: shift right
    WLC_OBJ_SHIFT_RA,      // 10: shift right (arithmetic)
    WLC_OBJ_SHIFT_L,       // 11: shift left
    WLC_OBJ_SHIFT_LA,      // 12: shift left (arithmetic)
    WLC_OBJ_ROTATE_R,      // 13: rotate right
    WLC_OBJ_ROTATE_L,      // 14: rotate left
    WLC_OBJ_BIT_NOT,       // 15: bitwise NOT
    WLC_OBJ_BIT_AND,       // 16: bitwise AND
    WLC_OBJ_BIT_OR,        // 17: bitwise OR
    WLC_OBJ_BIT_XOR,       // 18: bitwise XOR
    WLC_OBJ_BIT_NAND,      // 19: bitwise AND
    WLC_OBJ_BIT_NOR,       // 20: bitwise OR
    WLC_OBJ_BIT_NXOR,      // 21: bitwise NXOR
    WLC_OBJ_BIT_SELECT,    // 22: bit selection
    WLC_OBJ_BIT_CONCAT,    // 23: bit concatenation
    WLC_OBJ_BIT_ZEROPAD,   // 24: zero padding
    WLC_OBJ_BIT_SIGNEXT,   // 25: sign extension
    WLC_OBJ_LOGIC_NOT,     // 26: logic NOT
    WLC_OBJ_LOGIC_IMPL,    // 27: logic implication
    WLC_OBJ_LOGIC_AND,     // 28: logic AND
    WLC_OBJ_LOGIC_OR,      // 29: logic OR
    WLC_OBJ_LOGIC_XOR,     // 30: logic XOR
    WLC_OBJ_COMP_EQU,      // 31: compare equal
    WLC_OBJ_COMP_NOTEQU,   // 32: compare not equal
    WLC_OBJ_COMP_LESS,     // 33: compare less
    WLC_OBJ_COMP_MORE,     // 34: compare more
    WLC_OBJ_COMP_LESSEQU,  // 35: compare less or equal
    WLC_OBJ_COMP_MOREEQU,  // 36: compare more or equal
    WLC_OBJ_REDUCT_AND,    // 37: reduction AND
    WLC_OBJ_REDUCT_OR,     // 38: reduction OR
    WLC_OBJ_REDUCT_XOR,    // 39: reduction XOR
    WLC_OBJ_REDUCT_NAND,   // 40: reduction NAND
    WLC_OBJ_REDUCT_NOR,    // 41: reduction NOR
    WLC_OBJ_REDUCT_NXOR,   // 42: reduction NXOR
    WLC_OBJ_ARI_ADD,       // 43: arithmetic addition
    WLC_OBJ_ARI_SUB,       // 44: arithmetic subtraction
    WLC_OBJ_ARI_MULTI,     // 45: arithmetic multiplier
    WLC_OBJ_ARI_DIVIDE,    // 46: arithmetic division
    WLC_OBJ_ARI_REM,       // 47: arithmetic remainder
    WLC_OBJ_ARI_MODULUS,   // 48: arithmetic modulus
    WLC_OBJ_ARI_POWER,     // 49: arithmetic power
    WLC_OBJ_ARI_MINUS,     // 50: arithmetic minus
    WLC_OBJ_ARI_SQRT,      // 51: integer square root
    WLC_OBJ_ARI_SQUARE,    // 52: integer square
    WLC_OBJ_TABLE,         // 53: bit table
    WLC_OBJ_READ,          // 54: read port
    WLC_OBJ_WRITE,         // 55: write port
    WLC_OBJ_NUMBER         // 56: unused
} Wlc_ObjType_t;
// when adding new types, remember to update table Wlc_Names in "wlcNtk.c"


// Unlike AIG managers and logic networks in ABC, this network treats POs and FIs 
// as attributes of internal nodes and *not* as separate types of objects.


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Wlc_Obj_t_  Wlc_Obj_t;
struct Wlc_Obj_t_ // 24 bytes
{
    unsigned               Type    :  6;       // node type
    unsigned               Signed  :  1;       // signed
    unsigned               Mark    :  1;       // user mark
    unsigned               fIsPo   :  1;       // this is PO
    unsigned               fIsFi   :  1;       // this is FI
    unsigned               fXConst :  1;       // this is X-valued constant
    unsigned               nFanins;            // fanin count
    int                    End;                // range end
    int                    Beg;                // range begin
    union { int            Fanins[2];          // fanin IDs
            int *          pFanins[1]; };
};

typedef struct Wlc_Ntk_t_  Wlc_Ntk_t;
struct Wlc_Ntk_t_ 
{
    char *                 pName;              // model name
    char *                 pSpec;              // input file name
    Vec_Int_t              vPis;               // primary inputs 
    Vec_Int_t              vPos;               // primary outputs
    Vec_Int_t              vCis;               // combinational inputs
    Vec_Int_t              vCos;               // combinational outputs
    Vec_Int_t              vFfs;               // flops
    Vec_Int_t *            vInits;             // initial values
    char *                 pInits;             // initial values
    int                    nObjs[WLC_OBJ_NUMBER]; // counter of objects of each type
    int                    nAnds[WLC_OBJ_NUMBER]; // counter of AND gates after blasting
    int                    fSmtLib;            // the network comes from an SMT-LIB file
    int                    nAssert;            // the number of asserts
    // memory for objects
    Wlc_Obj_t *            pObjs;
    int                    iObj;
    int                    nObjsAlloc;
    Mem_Flex_t *           pMemFanin;
    Mem_Flex_t *           pMemTable;
    Vec_Ptr_t *            vTables;
    // object names
    Abc_Nam_t *            pManName;           // object names
    Vec_Int_t              vNameIds;           // object name IDs
    Vec_Int_t              vValues;            // value objects
    // object attributes
    int                    nTravIds;           // counter of traversal IDs
    Vec_Int_t              vTravIds;           // trav IDs of the objects
    Vec_Int_t              vCopies;            // object first bits
    Vec_Int_t              vBits;              // object mapping into AIG nodes
    Vec_Int_t              vLevels;            // object levels
    Vec_Int_t              vRefs;              // object reference counters
};

typedef struct Wlc_Par_t_ Wlc_Par_t;
struct Wlc_Par_t_
{
    int                    nBitsAdd;           // adder bit-width
    int                    nBitsMul;           // multiplier bit-widht 
    int                    nBitsMux;           // MUX bit-width
    int                    nBitsFlop;          // flop bit-width
    int                    nIterMax;           // the max number of iterations
    int                    nLimit;             // the max number of signals
    int                    fXorOutput;         // XOR outputs of word-level miter
    int                    fCheckClauses;      // Check clauses in the reloaded trace
    int                    fPushClauses;       // Push clauses in the reloaded trace
    int                    fMFFC;              // Refine the entire MFFC of a PPI
    int                    fPdra;              // Use pdr -nct
    int                    fLoadTrace;         // Load previous traces if any
    int                    fProofRefine;       // Use proof-based refinement
    int                    fHybrid;            // Use a hybrid of CBR and PBR
    int                    fCheckCombUnsat;    // Check if ABS becomes comb. unsat
    int                    fAbs2;              // Use UFAR style createAbs
    int                    fProofUsePPI;       // Use PPI values in PBR
    int                    fUseBmc3;           // Run BMC3 in parallel 
    int                    fShrinkAbs;         // Shrink Abs with BMC
    int                    fShrinkScratch;     // Restart pdr from scratch after shrinking
    int                    fVerbose;           // verbose output
    int                    fPdrVerbose;        // verbose output
    int                    RunId;              // id in this run 
    int                    (*pFuncStop)(int);  // callback to terminate
};

typedef struct Wlc_BstPar_t_ Wlc_BstPar_t;
struct Wlc_BstPar_t_
{
    int                    iOutput;
    int                    nOutputRange;
    int                    nAdderLimit;
    int                    nMultLimit;
    int                    fGiaSimple;
    int                    fAddOutputs;
    int                    fMulti;
    int                    fBooth;
    int                    fNoCleanup;
    int                    fCreateMiter;
    int                    fDecMuxes;
    int                    fVerbose;
    Vec_Int_t *            vBoxIds;
};

static inline void Wlc_BstParDefault( Wlc_BstPar_t * pPar )
{
    memset( pPar, 0, sizeof(Wlc_BstPar_t) );
    pPar->iOutput      = -1;
    pPar->nOutputRange =  0;
    pPar->nAdderLimit  =  0;
    pPar->nMultLimit   =  0;
    pPar->fGiaSimple   =  0;
    pPar->fAddOutputs  =  0;
    pPar->fMulti       =  0;
    pPar->fBooth       =  0;
    pPar->fCreateMiter =  0;
    pPar->fDecMuxes    =  0;
    pPar->fVerbose     =  0;
}

typedef struct Wla_Man_t_ Wla_Man_t;
struct Wla_Man_t_
{
    Wlc_Ntk_t * p;
    Wlc_Par_t * pPars;
    Vec_Vec_t * vClauses;
    Vec_Int_t * vBlacks;
    Vec_Int_t * vSignals;
    Abc_Cex_t * pCex;
    Gia_Man_t * pGia;
    Vec_Bit_t * vUnmark;
    void      * pPdrPars;
    void      * pThread;

    int iCexFrame;
    int fNewAbs;

    int nIters;
    int nTotalCla;
    int nDisj;
    int nNDisj;

    abctime tPdr;
    abctime tCbr;
    abctime tPbr;
};

static inline int          Wlc_NtkObjNum( Wlc_Ntk_t * p )                           { return p->iObj - 1;                                                      }
static inline int          Wlc_NtkObjNumMax( Wlc_Ntk_t * p )                        { return p->iObj;                                                          }
static inline int          Wlc_NtkPiNum( Wlc_Ntk_t * p )                            { return Vec_IntSize(&p->vPis);                                            }
static inline int          Wlc_NtkPoNum( Wlc_Ntk_t * p )                            { return Vec_IntSize(&p->vPos);                                            }
static inline int          Wlc_NtkCiNum( Wlc_Ntk_t * p )                            { return Vec_IntSize(&p->vCis);                                            }
static inline int          Wlc_NtkCoNum( Wlc_Ntk_t * p )                            { return Vec_IntSize(&p->vCos);                                            }
static inline int          Wlc_NtkFfNum( Wlc_Ntk_t * p )                            { return Vec_IntSize(&p->vCis) - Vec_IntSize(&p->vPis);                    }

static inline Wlc_Obj_t *  Wlc_NtkObj( Wlc_Ntk_t * p, int Id )                      { assert(Id > 0 && Id < p->nObjsAlloc); return p->pObjs + Id;              }
static inline Wlc_Obj_t *  Wlc_NtkPi( Wlc_Ntk_t * p, int i )                        { return Wlc_NtkObj( p, Vec_IntEntry(&p->vPis, i) );                       }
static inline Wlc_Obj_t *  Wlc_NtkPo( Wlc_Ntk_t * p, int i )                        { return Wlc_NtkObj( p, Vec_IntEntry(&p->vPos, i) );                       }
static inline Wlc_Obj_t *  Wlc_NtkCi( Wlc_Ntk_t * p, int i )                        { return Wlc_NtkObj( p, Vec_IntEntry(&p->vCis, i) );                       }
static inline Wlc_Obj_t *  Wlc_NtkCo( Wlc_Ntk_t * p, int i )                        { return Wlc_NtkObj( p, Vec_IntEntry(&p->vCos, i) );                       }
static inline Wlc_Obj_t *  Wlc_NtkFf( Wlc_Ntk_t * p, int i )                        { return Wlc_NtkObj( p, Vec_IntEntry(&p->vFfs, i) );                       }

static inline int          Wlc_ObjIsPi( Wlc_Obj_t * p )                             { return p->Type == WLC_OBJ_PI;                                            }
static inline int          Wlc_ObjIsPo( Wlc_Obj_t * p )                             { return p->fIsPo;                                                         }
static inline int          Wlc_ObjIsCi( Wlc_Obj_t * p )                             { return p->Type == WLC_OBJ_PI || p->Type == WLC_OBJ_FO;                   }
static inline int          Wlc_ObjIsCo( Wlc_Obj_t * p )                             { return p->fIsPo || p->fIsFi;                                             }

static inline int          Wlc_ObjId( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )             { return pObj - p->pObjs;                                                  }
static inline int          Wlc_ObjCiId( Wlc_Obj_t * p )                             { assert( Wlc_ObjIsCi(p) ); return p->Fanins[1];                           }
static inline int          Wlc_ObjType( Wlc_Obj_t * pObj )                          { return pObj->Type;                                                       }
static inline int          Wlc_ObjFaninNum( Wlc_Obj_t * p )                         { return p->nFanins;                                                       }
static inline int          Wlc_ObjHasArray( Wlc_Obj_t * p )                         { return p->nFanins > 2 || p->Type == WLC_OBJ_CONST || p->Type == WLC_OBJ_BIT_SELECT; }
static inline int *        Wlc_ObjFanins( Wlc_Obj_t * p )                           { return Wlc_ObjHasArray(p) ? p->pFanins[0] : p->Fanins;                   }
static inline int          Wlc_ObjFaninId( Wlc_Obj_t * p, int i )                   { return Wlc_ObjFanins(p)[i];                                              }
static inline int          Wlc_ObjFaninId0( Wlc_Obj_t * p )                         { return Wlc_ObjFanins(p)[0];                                              }
static inline int          Wlc_ObjFaninId1( Wlc_Obj_t * p )                         { return Wlc_ObjFanins(p)[1];                                              }
static inline int          Wlc_ObjFaninId2( Wlc_Obj_t * p )                         { return Wlc_ObjFanins(p)[2];                                              }
static inline Wlc_Obj_t *  Wlc_ObjFanin( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, int i )   { return Wlc_NtkObj( p, Wlc_ObjFaninId(pObj, i) );                         }
static inline Wlc_Obj_t *  Wlc_ObjFanin0( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )         { return Wlc_NtkObj( p, Wlc_ObjFaninId(pObj, 0) );                         }
static inline Wlc_Obj_t *  Wlc_ObjFanin1( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )         { return Wlc_NtkObj( p, Wlc_ObjFaninId(pObj, 1) );                         }
static inline Wlc_Obj_t *  Wlc_ObjFanin2( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )         { return Wlc_NtkObj( p, Wlc_ObjFaninId(pObj, 2) );                         }

static inline int          Wlc_ObjRange( Wlc_Obj_t * p )                            { return 1 + (p->End >= p->Beg ? p->End - p->Beg : p->Beg - p->End);       }
static inline int          Wlc_ObjRangeEnd( Wlc_Obj_t * p )                         { assert(p->Type == WLC_OBJ_BIT_SELECT); return p->pFanins[0][1];          }
static inline int          Wlc_ObjRangeBeg( Wlc_Obj_t * p )                         { assert(p->Type == WLC_OBJ_BIT_SELECT); return p->pFanins[0][2];          }
static inline int          Wlc_ObjRangeIsReversed( Wlc_Obj_t * p )                  { return p->End < p->Beg;                                                  }

static inline int          Wlc_ObjIsSigned( Wlc_Obj_t * p )                         { return p->Signed;                                                        }
static inline int          Wlc_ObjIsSignedFanin01( Wlc_Ntk_t * p, Wlc_Obj_t * pObj ){ return p->fSmtLib ? Wlc_ObjIsSigned(pObj) : (Wlc_ObjFanin0(p, pObj)->Signed && Wlc_ObjFanin1(p, pObj)->Signed); }
static inline int          Wlc_ObjIsSignedFanin0( Wlc_Ntk_t * p, Wlc_Obj_t * pObj ) { return p->fSmtLib ? Wlc_ObjIsSigned(pObj) : Wlc_ObjFanin0(p, pObj)->Signed; }
static inline int          Wlc_ObjIsSignedFanin1( Wlc_Ntk_t * p, Wlc_Obj_t * pObj ) { return p->fSmtLib ? Wlc_ObjIsSigned(pObj) : Wlc_ObjFanin1(p, pObj)->Signed; }
static inline int          Wlc_ObjSign( Wlc_Obj_t * p )                             { return Abc_Var2Lit( Wlc_ObjRange(p), Wlc_ObjIsSigned(p) );               }
static inline int *        Wlc_ObjConstValue( Wlc_Obj_t * p )                       { assert(p->Type == WLC_OBJ_CONST);      return Wlc_ObjFanins(p);          }
static inline int          Wlc_ObjTableId( Wlc_Obj_t * p )                          { assert(p->Type == WLC_OBJ_TABLE);      return p->Fanins[1];              }
static inline word *       Wlc_ObjTable( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )          { return (word *)Vec_PtrEntry( p->vTables, Wlc_ObjTableId(pObj) );         }
static inline int          Wlc_ObjLevelId( Wlc_Ntk_t * p, int iObj )                { return Vec_IntEntry( &p->vLevels, iObj );                                }
static inline int          Wlc_ObjLevel( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )          { return Wlc_ObjLevelId( p, Wlc_ObjId(p, pObj) );                          }

static inline void         Wlc_NtkCleanCopy( Wlc_Ntk_t * p )                        { Vec_IntFill( &p->vCopies, p->nObjsAlloc, 0 );                            }
static inline int          Wlc_NtkHasCopy( Wlc_Ntk_t * p )                          { return Vec_IntSize( &p->vCopies ) > 0;                                   }
static inline void         Wlc_ObjSetCopy( Wlc_Ntk_t * p, int iObj, int i )         { Vec_IntWriteEntry( &p->vCopies, iObj, i );                               }
static inline int          Wlc_ObjCopy( Wlc_Ntk_t * p, int iObj )                   { return Vec_IntEntry( &p->vCopies, iObj );                                }
static inline Wlc_Obj_t *  Wlc_ObjCopyObj(Wlc_Ntk_t * pNew, Wlc_Ntk_t * p, Wlc_Obj_t * pObj) {return Wlc_NtkObj(pNew, Wlc_ObjCopy(p, Wlc_ObjId(p, pObj)));   }

static inline void         Wlc_NtkCleanNameId( Wlc_Ntk_t * p )                      { Vec_IntFill( &p->vNameIds, p->nObjsAlloc, 0 );                           }
static inline int          Wlc_NtkHasNameId( Wlc_Ntk_t * p )                        { return Vec_IntSize( &p->vNameIds ) > 0;                                  }
static inline void         Wlc_ObjSetNameId( Wlc_Ntk_t * p, int iObj, int i )       { Vec_IntWriteEntry( &p->vNameIds, iObj, i );                              }
static inline int          Wlc_ObjNameId( Wlc_Ntk_t * p, int iObj )                 { return Vec_IntEntry( &p->vNameIds, iObj );                               }

static inline Wlc_Obj_t *  Wlc_ObjFo2Fi( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )          { assert( pObj->Type == WLC_OBJ_FO ); return Wlc_NtkCo(p, Wlc_NtkPoNum(p) + Wlc_ObjCiId(pObj) - Wlc_NtkPiNum(p)); } 
static inline Wlc_Obj_t *  Wlc_ObjCo2PoFo( Wlc_Ntk_t * p, int iCoId )               { return iCoId < Wlc_NtkPoNum(p) ? Wlc_NtkPo(p, iCoId) : Wlc_NtkCi(p, Wlc_NtkPiNum(p) + iCoId - Wlc_NtkPoNum(p)); } 

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

#define Wlc_NtkForEachObj( p, pObj, i )                                             \
    for ( i = 1; (i < Wlc_NtkObjNumMax(p)) && (((pObj) = Wlc_NtkObj(p, i)), 1); i++ )
#define Wlc_NtkForEachObjReverse( p, pObj, i )                                      \
    for ( i = Wlc_NtkObjNumMax(p) - 1; (i > 0) && (((pObj) = Wlc_NtkObj(p, i)), 1); i-- )
#define Wlc_NtkForEachObjVec( vVec, p, pObj, i )                                    \
    for ( i = 0; (i < Vec_IntSize(vVec)) && (((pObj) = Wlc_NtkObj(p, Vec_IntEntry(vVec, i))), 1); i++ )
#define Wlc_NtkForEachPi( p, pPi, i )                                               \
    for ( i = 0; (i < Wlc_NtkPiNum(p)) && (((pPi) = Wlc_NtkPi(p, i)), 1); i++ )
#define Wlc_NtkForEachPo( p, pPo, i )                                               \
    for ( i = 0; (i < Wlc_NtkPoNum(p)) && (((pPo) = Wlc_NtkPo(p, i)), 1); i++ )
#define Wlc_NtkForEachCi( p, pCi, i )                                               \
    for ( i = 0; (i < Wlc_NtkCiNum(p)) && (((pCi) = Wlc_NtkCi(p, i)), 1); i++ )
#define Wlc_NtkForEachCo( p, pCo, i )                                               \
    for ( i = 0; (i < Wlc_NtkCoNum(p)) && (((pCo) = Wlc_NtkCo(p, i)), 1); i++ )
#define Wlc_NtkForEachFf( p, pFf, i )                                               \
    for ( i = 0; (i < Vec_IntSize(&p->vFfs)) && (((pFf) = Wlc_NtkFf(p, i)), 1); i++ )

#define Wlc_ObjForEachFanin( pObj, iFanin, i )                                      \
    for ( i = 0; (i < Wlc_ObjFaninNum(pObj)) && (((iFanin) = Wlc_ObjFaninId(pObj, i)), 1); i++ )
#define Wlc_ObjForEachFaninObj( p, pObj, pFanin, i )                                \
    for ( i = 0; (i < Wlc_ObjFaninNum(pObj)) && (((pFanin) = Wlc_NtkObj(p, Wlc_ObjFaninId(pObj, i))), 1); i++ )
#define Wlc_ObjForEachFaninReverse( pObj, iFanin, i )                               \
    for ( i = Wlc_ObjFaninNum(pObj) - 1; (i >= 0) && (((iFanin) = Wlc_ObjFaninId(pObj, i)), 1); i-- )


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== wlcAbs.c ========================================================*/
extern int            Wlc_NtkAbsCore( Wlc_Ntk_t * p, Wlc_Par_t * pPars );
extern int            Wlc_NtkPdrAbs( Wlc_Ntk_t * p, Wlc_Par_t * pPars );
/*=== wlcAbs2.c ========================================================*/
extern int            Wlc_NtkAbsCore2( Wlc_Ntk_t * p, Wlc_Par_t * pPars );
/*=== wlcBlast.c ========================================================*/
extern Gia_Man_t *    Wlc_NtkBitBlast( Wlc_Ntk_t * p, Wlc_BstPar_t * pPars );
/*=== wlcCom.c ========================================================*/
extern void           Wlc_SetNtk( Abc_Frame_t * pAbc, Wlc_Ntk_t * pNtk );
/*=== wlcNdr.c ========================================================*/
extern Wlc_Ntk_t *    Wlc_ReadNdr( char * pFileName );
extern void           Wlc_WriteNdr( Wlc_Ntk_t * pNtk, char * pFileName );
extern Wlc_Ntk_t *    Wlc_NtkFromNdr( void * pData );
extern void *         Wlc_NtkToNdr( Wlc_Ntk_t * pNtk );
/*=== wlcNtk.c ========================================================*/
extern void           Wlc_ManSetDefaultParams( Wlc_Par_t * pPars );
extern char *         Wlc_ObjTypeName( Wlc_Obj_t * p );
extern Wlc_Ntk_t *    Wlc_NtkAlloc( char * pName, int nObjsAlloc );
extern int            Wlc_ObjAlloc( Wlc_Ntk_t * p, int Type, int Signed, int End, int Beg );
extern int            Wlc_ObjCreate( Wlc_Ntk_t * p, int Type, int Signed, int End, int Beg, Vec_Int_t * vFanins );
extern void           Wlc_ObjSetCi( Wlc_Ntk_t * p, Wlc_Obj_t * pObj );
extern void           Wlc_ObjSetCo( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, int fFlopInput );
extern char *         Wlc_ObjName( Wlc_Ntk_t * p, int iObj );
extern void           Wlc_ObjUpdateType( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, int Type );
extern void           Wlc_ObjAddFanins( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Int_t * vFanins );
extern void           Wlc_NtkFree( Wlc_Ntk_t * p );
extern int            Wlc_NtkCreateLevels( Wlc_Ntk_t * p );
extern int            Wlc_NtkCreateLevelsRev( Wlc_Ntk_t * p );
extern int            Wlc_NtkCountRealPis( Wlc_Ntk_t * p );
extern void           Wlc_NtkPrintNode( Wlc_Ntk_t * p, Wlc_Obj_t * pObj );
extern void           Wlc_NtkPrintNodeArray( Wlc_Ntk_t * p, Vec_Int_t * vArray );
extern void           Wlc_NtkPrintNodes( Wlc_Ntk_t * p, int Type );
extern void           Wlc_NtkPrintStats( Wlc_Ntk_t * p, int fDistrib, int fTwoSides, int fVerbose );
extern void           Wlc_NtkTransferNames( Wlc_Ntk_t * pNew, Wlc_Ntk_t * p );
extern char *         Wlc_NtkNewName( Wlc_Ntk_t * p, int iCoId, int fSeq );
extern Wlc_Ntk_t *    Wlc_NtkDupDfs( Wlc_Ntk_t * p, int fMarked, int fSeq );
extern Wlc_Ntk_t *    Wlc_NtkDupDfsAbs( Wlc_Ntk_t * p, Vec_Int_t * vPisOld, Vec_Int_t * vPisNew, Vec_Int_t * vFlops );
extern Wlc_Ntk_t *    Wlc_NtkDupDfsSimple( Wlc_Ntk_t * p );
extern void           Wlc_NtkCleanMarks( Wlc_Ntk_t * p );
extern void           Wlc_NtkMarkCone( Wlc_Ntk_t * p, int iCoId, int Range, int fSeq, int fAllPis );
extern void           Wlc_NtkProfileCones( Wlc_Ntk_t * p );
extern Wlc_Ntk_t *    Wlc_NtkDupSingleNodes( Wlc_Ntk_t * p );
extern void           Wlc_NtkShortNames( Wlc_Ntk_t * p );
extern int            Wlc_NtkDcFlopNum( Wlc_Ntk_t * p );
extern void           Wlc_NtkSetRefs( Wlc_Ntk_t * p );
extern int            Wlc_NtkCountObjBits( Wlc_Ntk_t * p, Vec_Int_t * vPisNew );
/*=== wlcReadSmt.c ========================================================*/
extern Wlc_Ntk_t *    Wlc_ReadSmtBuffer( char * pFileName, char * pBuffer, char * pLimit, int fOldParser, int fPrintTree );
extern Wlc_Ntk_t *    Wlc_ReadSmt( char * pFileName, int fOldParser, int fPrintTree );
/*=== wlcSim.c ========================================================*/
extern Vec_Ptr_t *    Wlc_NtkSimulate( Wlc_Ntk_t * p, Vec_Int_t * vNodes, int nWords, int nFrames );
extern void           Wlc_NtkDeleteSim( Vec_Ptr_t * p );
/*=== wlcStdin.c ========================================================*/
extern int            Wlc_StdinProcessSmt( Abc_Frame_t * pAbc, char * pCmd );
/*=== wlcReadVer.c ========================================================*/
extern Wlc_Ntk_t *    Wlc_ReadVer( char * pFileName, char * pStr );
/*=== wlcUif.c ========================================================*/
extern Vec_Int_t *    Wlc_NtkCollectAddMult( Wlc_Ntk_t * p, Wlc_BstPar_t * pPar, int * pCountA, int * CountM );
extern int            Wlc_NtkPairIsUifable( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Wlc_Obj_t * pObj2 );
extern Vec_Int_t *    Wlc_NtkCollectMultipliers( Wlc_Ntk_t * p );
extern Vec_Int_t *    Wlc_NtkFindUifableMultiplierPairs( Wlc_Ntk_t * p );
extern Wlc_Ntk_t *    Wlc_NtkAbstractNodes( Wlc_Ntk_t * pNtk, Vec_Int_t * vNodes );
extern Wlc_Ntk_t *    Wlc_NtkUifNodePairs( Wlc_Ntk_t * pNtk, Vec_Int_t * vPairs );
/*=== wlcWin.c =============================================================*/
extern void           Wlc_WinProfileArith( Wlc_Ntk_t * p );
/*=== wlcWriteVer.c ========================================================*/
extern void           Wlc_WriteVer( Wlc_Ntk_t * p, char * pFileName, int fAddCos, int fNoFlops );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

