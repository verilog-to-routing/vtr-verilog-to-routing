/**CFile****************************************************************

  FileName    [satStore.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Proof recording.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: pr.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__sat__bsat__satStore_h
#define ABC__sat__bsat__satStore_h


/*
    The trace of SAT solving contains the original clauses of the problem
    along with the learned clauses derived during SAT solving.
    The first line of the resulting file contains 3 numbers instead of 2:
    c <num_vars> <num_all_clauses> <num_root_clauses>
*/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "satSolver.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#endif

#define STO_MAX(a,b)  ((a) > (b) ? (a) : (b))

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

/*
typedef unsigned lit;
// variable/literal conversions (taken from MiniSat)
static inline lit   toLit    (int v)        { return v + v;  }
static inline lit   toLitCond(int v, int c) { return v + v + (c != 0); }
static inline lit   lit_neg  (lit l)        { return l ^ 1;  }
static inline int   lit_var  (lit l)        { return l >> 1; }
static inline int   lit_sign (lit l)        { return l & 1;  }
static inline int   lit_print(lit l)        { return lit_sign(l)? -lit_var(l)-1 : lit_var(l)+1; }
static inline lit   lit_read (int s)        { return s > 0 ? toLit(s-1) : lit_neg(toLit(-s-1)); }
static inline int   lit_check(lit l, int n) { return l >= 0 && lit_var(l) < n;                  }
*/

typedef struct Sto_Cls_t_ Sto_Cls_t;
struct Sto_Cls_t_
{
    Sto_Cls_t *     pNext;        // the next clause
    Sto_Cls_t *     pNext0;       // the next 0-watch
    Sto_Cls_t *     pNext1;       // the next 1-watch
    int             Id;           // the clause ID
    unsigned        fA     :  1;  // belongs to A
    unsigned        fRoot  :  1;  // original clause
    unsigned        fVisit :  1;  // visited clause
    unsigned        nLits  : 24;  // the number of literals
    lit             pLits[0];     // literals of this clause
};

typedef struct Sto_Man_t_ Sto_Man_t;
struct Sto_Man_t_
{
    // general data
    int             nVars;        // the number of variables
    int             nRoots;       // the number of root clauses
    int             nClauses;     // the number of all clauses
    int             nClausesA;    // the number of clauses of A 
    Sto_Cls_t *     pHead;        // the head clause
    Sto_Cls_t *     pTail;        // the tail clause
    Sto_Cls_t *     pEmpty;       // the empty clause
    // memory management
    int             nChunkSize;   // the number of bytes in a chunk
    int             nChunkUsed;   // the number of bytes used in the last chunk
    char *          pChunkLast;   // the last memory chunk
};

// iterators through the clauses
#define Sto_ManForEachClause( p, pCls )      for( pCls = p->pHead; pCls; pCls = pCls->pNext )
#define Sto_ManForEachClauseRoot( p, pCls )  for( pCls = p->pHead; pCls && pCls->fRoot; pCls = pCls->pNext )

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== satStore.c ==========================================================*/
extern Sto_Man_t *  Sto_ManAlloc();
extern void         Sto_ManFree( Sto_Man_t * p );
extern int          Sto_ManAddClause( Sto_Man_t * p, lit * pBeg, lit * pEnd );
extern int          Sto_ManMemoryReport( Sto_Man_t * p );
extern void         Sto_ManMarkRoots( Sto_Man_t * p );
extern void         Sto_ManMarkClausesA( Sto_Man_t * p );
extern void         Sto_ManDumpClauses( Sto_Man_t * p, char * pFileName );
extern int          Sto_ManChangeLastClause( Sto_Man_t * p );
extern Sto_Man_t *  Sto_ManLoadClauses( char * pFileName );


/*=== satInter.c ==========================================================*/
typedef struct Int_Man_t_ Int_Man_t;
extern Int_Man_t *  Int_ManAlloc();
extern int *        Int_ManSetGlobalVars( Int_Man_t * p, int nGloVars );
extern void         Int_ManFree( Int_Man_t * p );
extern int          Int_ManInterpolate( Int_Man_t * p, Sto_Man_t * pCnf, int fVerbose, unsigned ** ppResult );

/*=== satInterA.c ==========================================================*/
typedef struct Inta_Man_t_ Inta_Man_t;
extern Inta_Man_t * Inta_ManAlloc();
extern void         Inta_ManFree( Inta_Man_t * p );
extern void *       Inta_ManInterpolate( Inta_Man_t * p, Sto_Man_t * pCnf, abctime TimeToStop, void * vVarsAB, int fVerbose );

/*=== satInterB.c ==========================================================*/
typedef struct Intb_Man_t_ Intb_Man_t;
extern Intb_Man_t * Intb_ManAlloc();
extern void         Intb_ManFree( Intb_Man_t * p );
extern void *       Intb_ManInterpolate( Intb_Man_t * p, Sto_Man_t * pCnf, void * vVarsAB, int fVerbose );

/*=== satInterP.c ==========================================================*/
typedef struct Intp_Man_t_ Intp_Man_t;
extern Intp_Man_t * Intp_ManAlloc();
extern void         Intp_ManFree( Intp_Man_t * p );
extern void *       Intp_ManUnsatCore( Intp_Man_t * p, Sto_Man_t * pCnf, int fLearned, int fVerbose );
extern void         Intp_ManUnsatCorePrintForBmc( FILE * pFile, Sto_Man_t * pCnf, void * vCore, void * vVarMap );


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

