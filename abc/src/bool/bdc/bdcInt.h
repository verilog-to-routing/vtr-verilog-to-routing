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
 
#ifndef ABC__aig__bdc__bdcInt_h
#define ABC__aig__bdc__bdcInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "bool/kit/kit.h"
#include "bdc.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


#define BDC_SCALE 1000  // value used to compute the cost

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// network types
typedef enum { 
    BDC_TYPE_NONE = 0,  // 0:  unknown
    BDC_TYPE_CONST1,    // 1:  constant 1
    BDC_TYPE_PI,        // 2:  primary input
    BDC_TYPE_AND,       // 3:  AND-gate
    BDC_TYPE_OR,        // 4:  OR-gate (temporary)
    BDC_TYPE_XOR,       // 5:  XOR-gate
    BDC_TYPE_MUX,       // 6:  MUX-gate
    BDC_TYPE_OTHER      // 7:  unused
} Bdc_Type_t;

struct Bdc_Fun_t_
{
    int              Type;         // Const1, PI, AND, XOR, MUX
    Bdc_Fun_t *      pFan0;        // fanin of the given node
    Bdc_Fun_t *      pFan1;        // fanin of the given node
    unsigned         uSupp;        // bit mask of current support
    unsigned *       puFunc;       // the function of the node
    Bdc_Fun_t *      pNext;        // next function with same support
    union { int      iCopy;        // the literal of the node (AIG)
    void *           pCopy; };     // the function of the node (BDD or AIG)

};

typedef struct Bdc_Isf_t_ Bdc_Isf_t;
struct Bdc_Isf_t_
{
    unsigned         uSupp;        // the complete support of this component
    unsigned         uUniq;        // the unique variables of this component
    unsigned *       puOn;         // on-set
    unsigned *       puOff;        // off-set
};

struct Bdc_Man_t_
{
    // external parameters
    Bdc_Par_t *      pPars;        // parameter set
    int              nVars;        // the number of variables
    int              nWords;       // the number of words 
    int              nNodesMax;    // the limit on the number of new nodes
    int              nDivsLimit;   // the limit on the number of divisors
    // internal nodes
    Bdc_Fun_t *      pNodes;       // storage for decomposition nodes
    int              nNodesAlloc;  // the number of nodes allocated  
    int              nNodes;       // the number of all nodes created so far
    int              nNodesNew;    // the number of new AND nodes created so far
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
    // statistics
    int              numCalls;
    int              numNodes;
    int              numMuxes;
    int              numAnds;
    int              numOrs;
    int              numWeaks;
    int              numReuse;
    // runtime
    abctime          timeCache;
    abctime          timeCheck;
    abctime          timeMuxes;
    abctime          timeSupps;
    abctime          timeTotal;
};

static inline Bdc_Fun_t * Bdc_FunNew( Bdc_Man_t * p )                   { Bdc_Fun_t * pRes; if ( p->nNodes >= p->nNodesAlloc || p->nNodesNew >= p->nNodesMax ) return NULL; pRes = p->pNodes + p->nNodes++; p->nNodesNew++; memset( pRes, 0, sizeof(Bdc_Fun_t) ); return pRes; }
static inline Bdc_Fun_t * Bdc_FunWithId( Bdc_Man_t * p, int Id )        { assert( Id < p->nNodes ); return p->pNodes + Id; }
static inline int         Bdc_FunId( Bdc_Man_t * p, Bdc_Fun_t * pFun )  { return pFun - p->pNodes; }
static inline void        Bdc_IsfStart( Bdc_Man_t * p, Bdc_Isf_t * pF ) { pF->uSupp = 0; pF->uUniq = 0; pF->puOn = Vec_IntFetch( p->vMemory, p->nWords ); pF->puOff = Vec_IntFetch( p->vMemory, p->nWords ); assert( pF->puOff && pF->puOn ); }
static inline void        Bdc_IsfClean( Bdc_Isf_t * p )                 { p->uSupp = 0; p->uUniq = 0;                                      }
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
extern void             Bdc_SuppMinimize( Bdc_Man_t * p, Bdc_Isf_t * pIsf );
extern int              Bdc_ManNodeVerify( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Fun_t * pFunc );
/*=== bdcTable.c ==========================================================*/
extern Bdc_Fun_t *      Bdc_TableLookup( Bdc_Man_t * p, Bdc_Isf_t * pIsf );
extern void             Bdc_TableAdd( Bdc_Man_t * p, Bdc_Fun_t * pFunc );
extern void             Bdc_TableClear( Bdc_Man_t * p );
extern int              Bdc_TableCheckContainment( Bdc_Man_t * p, Bdc_Isf_t * pIsf, unsigned * puTruth );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

