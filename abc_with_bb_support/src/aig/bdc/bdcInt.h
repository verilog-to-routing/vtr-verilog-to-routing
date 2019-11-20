/**CFile****************************************************************

  FileName    [bdcInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth-table-based bi-decomposition engine.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 15, 2007.]

  Revision    [$Id: resInt.h,v 1.00 2007/01/15 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef __BDC_INT_H__
#define __BDC_INT_H__

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "kit.h"
#include "bdc.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define BDC_SCALE 100  // value used to compute the cost

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// network types
typedef enum { 
    BDC_TYPE_NONE = 0,  // 0:  unknown
    BDC_TYPE_CONST1,    // 1:  constant 1
    BDC_TYPE_PI,        // 2:  primary input
    BDC_TYPE_AND,       // 4:  AND-gate
    BDC_TYPE_OR,        // 5:  OR-gate (temporary)
    BDC_TYPE_XOR,       // 6:  XOR-gate
    BDC_TYPE_MUX,       // 7:  MUX-gate
    BDC_TYPE_OTHER      // 8:  unused
} Bdc_Type_t;

typedef struct Bdc_Fun_t_ Bdc_Fun_t;
struct Bdc_Fun_t_
{
    int              Type;         // Const1, PI, AND, XOR, MUX
    Bdc_Fun_t *      pFan0;        // fanin of the given node
    Bdc_Fun_t *      pFan1;        // fanin of the given node
    Bdc_Fun_t *      pFan2;        // fanin of the given node
    unsigned         uSupp;        // bit mask of current support
    unsigned *       puFunc;       // the function of the node
    Bdc_Fun_t *      pNext;        // next function with same support
    void *           pCopy;        // the copy field
};

typedef struct Bdc_Isf_t_ Bdc_Isf_t;
struct Bdc_Isf_t_
{
    int              Var;          // the first variable assigned
    unsigned         uSupp;        // the current support
    unsigned *       puOn;         // on-set
    unsigned *       puOff;        // off-set
};

struct Bdc_Man_t_
{
    // external parameters
    Bdc_Par_t *      pPars;        // parameter set
    int              nVars;        // the number of variables
    int              nWords;       // the number of words 
    int              nNodesLimit;  // the limit on the number of new nodes
    int              nDivsLimit;   // the limit on the number of divisors
    // internal nodes
    Bdc_Fun_t *      pNodes;       // storage for decomposition nodes
    int              nNodes;       // the number of nodes used
    int              nNodesNew;    // the number of nodes used
    int              nNodesAlloc;  // the number of nodes allocated  
    Bdc_Fun_t *      pRoot;        // the root node
    // resub candidates
    Bdc_Fun_t **     pTable;       // hash table of candidates
    int              nTableSize;   // hash table size (1 << nVarsMax)
    Vec_Int_t *      vSpots;       // the occupied spots in the table
    // elementary truth tables 
    Vec_Ptr_t *      vTruths;      // for const 1 and elementary variables
    unsigned *       puTemp1;      // temporary truth table
    unsigned *       puTemp2;      // temporary truth table
    unsigned *       puTemp3;      // temporary truth table
    unsigned *       puTemp4;      // temporary truth table
    // temporary ISFs
    Bdc_Isf_t * pIsfOL, IsfOL;
    Bdc_Isf_t * pIsfOR, IsfOR;
    Bdc_Isf_t * pIsfAL, IsfAL;
    Bdc_Isf_t * pIsfAR, IsfAR;
    // internal memory manager
    Vec_Int_t *      vMemory;      // memory for internal truth tables
};

// working with complemented attributes of objects
static inline int         Bdc_IsComplement( Bdc_Fun_t * p )             { return (int)((unsigned long)p & (unsigned long)01);              }
static inline Bdc_Fun_t * Bdc_Regular( Bdc_Fun_t * p )                  { return (Bdc_Fun_t *)((unsigned long)p & ~(unsigned long)01);     }
static inline Bdc_Fun_t * Bdc_Not( Bdc_Fun_t * p )                      { return (Bdc_Fun_t *)((unsigned long)p ^  (unsigned long)01);     }
static inline Bdc_Fun_t * Bdc_NotCond( Bdc_Fun_t * p, int c )           { return (Bdc_Fun_t *)((unsigned long)p ^  (unsigned long)(c!=0)); }

static inline Bdc_Fun_t * Bdc_FunNew( Bdc_Man_t * p )                   { Bdc_Fun_t * pRes; if ( p->nNodes == p->nNodesLimit ) return NULL; pRes = p->pNodes + p->nNodes++; memset( pRes, 0, sizeof(Bdc_Fun_t) ); p->nNodesNew++; return pRes; }
static inline void        Bdc_IsfStart( Bdc_Man_t * p, Bdc_Isf_t * pF ) { pF->puOn = Vec_IntFetch( p->vMemory, p->nWords ); pF->puOff = Vec_IntFetch( p->vMemory, p->nWords ); }
static inline void        Bdc_IsfClean( Bdc_Isf_t * p )                 { p->uSupp = 0; p->Var = 0;                                        }
static inline void        Bdc_IsfCopy( Bdc_Isf_t * p, Bdc_Isf_t * q )   { Bdc_Isf_t T = *p; *p = *q; *q = T;                               }
static inline void        Bdc_IsfNot( Bdc_Isf_t * p )                   { unsigned * puT = p->puOn; p->puOn = p->puOff; p->puOff = puT;    }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== bdcDec.c ==========================================================*/
extern Bdc_Fun_t *      Bdc_ManDecompose_rec( Bdc_Man_t * p, Bdc_Isf_t * pIsf );
/*=== bdcTable.c ==========================================================*/
extern Bdc_Fun_t *      Bdc_TableLookup( Bdc_Man_t * p, Bdc_Isf_t * pIsf );
extern void             Bdc_TableAdd( Bdc_Man_t * p, Bdc_Fun_t * pFunc );
extern void             Bdc_TableClear( Bdc_Man_t * p );
extern void             Bdc_SuppMinimize( Bdc_Man_t * p, Bdc_Isf_t * pIsf );
extern int              Bdc_TableCheckContainment( Bdc_Man_t * p, Bdc_Isf_t * pIsf, unsigned * puTruth );

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

