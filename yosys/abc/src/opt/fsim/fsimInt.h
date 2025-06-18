/**CFile****************************************************************

  FileName    [fsimInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast sequential AIG simulator.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: fsimInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__fsim__fsimInt_h
#define ABC__aig__fsim__fsimInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/saig/saig.h"
#include "fsim.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// simulation object
typedef struct Fsim_Obj_t_ Fsim_Obj_t;
struct Fsim_Obj_t_
{
    int             iNode;        // the node ID
    int             iFan0;        // the first fanin
    int             iFan1;        // the second fanin
};

// fast sequential simulation manager
struct Fsim_Man_t_
{
    // parameters
    Aig_Man_t *     pAig;         // the AIG to be used for simulation
    int             nWords;       // the number of simulation words
    // AIG representation
    int             nPis;         // the number of primary inputs
    int             nPos;         // the number of primary outputs
    int             nCis;         // the number of combinational inputs
    int             nCos;         // the number of combinational outputs
    int             nNodes;       // the number of internal nodes
    int             nObjs;        // nCis + nNodes + nCos + 2
    int *           pFans0;       // fanin0 for all objects
    int *           pFans1;       // fanin1 for all objects
    int *           pRefs;        // reference counter for each node
    int *           pRefsCopy;    // reference counter for each node
    Vec_Int_t *     vCis2Ids;     // mapping of CIs into their PI ids
    Vec_Int_t *     vLos;         // register outputs
    Vec_Int_t *     vLis;         // register inputs
    // cross-cut representation
    int             nCrossCut;    // temporary cross-cut variable 
    int             nCrossCutMax; // maximum cross-cut variable
    int             nFront;       // the size of frontier
    // derived AIG representation
    int             nDataAig;     // the length of allocated data
    unsigned char * pDataAig;     // AIG representation
    unsigned char * pDataCur;     // AIG representation (current position)
    int             iNodePrev;    // previous extracted value
    int             iNumber;      // the number of the last object
    Fsim_Obj_t      Obj;          // current object
    // temporary AIG representation
    int *           pDataAig2;    // temporary representation
    int *           pDataCur2;    // AIG representation (current position)
    // simulation information
    unsigned *      pDataSim;     // simulation data
    unsigned *      pDataSimCis;  // simulation data for CIs
    unsigned *      pDataSimCos;  // simulation data for COs
    // other information
    int *           pData1;
    int *           pData2;
};

static inline unsigned * Fsim_SimData( Fsim_Man_t * p, int i )    { return p->pDataSim + i * p->nWords;    }
static inline unsigned * Fsim_SimDataCi( Fsim_Man_t * p, int i )  { return p->pDataSimCis + i * p->nWords; }
static inline unsigned * Fsim_SimDataCo( Fsim_Man_t * p, int i )  { return p->pDataSimCos + i * p->nWords; }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int  Fsim_Var2Lit( int Var, int fCompl )  { return Var + Var + fCompl; }
static inline int  Fsim_Lit2Var( int Lit )              { return Lit >> 1;           }
static inline int  Fsim_LitIsCompl( int Lit )           { return Lit & 1;            }
static inline int  Fsim_LitNot( int Lit )               { return Lit ^ 1;            }
static inline int  Fsim_LitNotCond( int Lit, int c )    { return Lit ^ (int)(c > 0); }
static inline int  Fsim_LitRegular( int Lit )           { return Lit & ~01;          }

#define Fsim_ManForEachObj( p, pObj, i )\
    for ( i = 2, p->pDataCur = p->pDataAig, p->iNodePrev = 0, pObj = &p->Obj;\
        i < p->nObjs && Fsim_ManRestoreObj( p, pObj ); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== fsimFront.c ========================================================*/
extern void                Fsim_ManFront( Fsim_Man_t * p, int fCompressAig );
/*=== fsimMan.c ==========================================================*/
extern Fsim_Man_t *        Fsim_ManCreate( Aig_Man_t * pAig );
extern void                Fsim_ManDelete( Fsim_Man_t * p );
extern void                Fsim_ManTest( Aig_Man_t * pAig );
 


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

