/**CFile****************************************************************

  FileName    [bdc.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth-table-based bi-decomposition engine.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 30, 2007.]

  Revision    [$Id: bdc.h,v 1.00 2007/01/30 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__bdc__bdc_h
#define ABC__aig__bdc__bdc_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
 


ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Bdc_Fun_t_ Bdc_Fun_t;
typedef struct Bdc_Man_t_ Bdc_Man_t;
typedef struct Bdc_Par_t_ Bdc_Par_t;
struct Bdc_Par_t_
{
    // general parameters
    int           nVarsMax;      // the maximum support
    int           fVerbose;      // enable basic stats
    int           fVeryVerbose;  // enable detailed stats
};

// working with complemented attributes of objects
static inline int         Bdc_IsComplement( Bdc_Fun_t * p )      { return (int)((ABC_PTRUINT_T)p & (ABC_PTRUINT_T)01);              }
static inline Bdc_Fun_t * Bdc_Regular( Bdc_Fun_t * p )           { return (Bdc_Fun_t *)((ABC_PTRUINT_T)p & ~(ABC_PTRUINT_T)01);     }
static inline Bdc_Fun_t * Bdc_Not( Bdc_Fun_t * p )               { return (Bdc_Fun_t *)((ABC_PTRUINT_T)p ^  (ABC_PTRUINT_T)01);     }
static inline Bdc_Fun_t * Bdc_NotCond( Bdc_Fun_t * p, int c )    { return (Bdc_Fun_t *)((ABC_PTRUINT_T)p ^  (ABC_PTRUINT_T)(c!=0)); }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== bdcCore.c ==========================================================*/
extern Bdc_Man_t * Bdc_ManAlloc( Bdc_Par_t * pPars );
extern void        Bdc_ManFree( Bdc_Man_t * p );
extern void        Bdc_ManDecPrint( Bdc_Man_t * p );
extern int         Bdc_ManDecompose( Bdc_Man_t * p, unsigned * puFunc, unsigned * puCare, int nVars, Vec_Ptr_t * vDivs, int nNodesMax );
extern Bdc_Fun_t * Bdc_ManFunc( Bdc_Man_t * p, int i );
extern Bdc_Fun_t * Bdc_ManRoot( Bdc_Man_t * p );
extern int         Bdc_ManNodeNum( Bdc_Man_t * p );
extern int         Bdc_ManAndNum( Bdc_Man_t * p );
extern Bdc_Fun_t * Bdc_FuncFanin0( Bdc_Fun_t * p );
extern Bdc_Fun_t * Bdc_FuncFanin1( Bdc_Fun_t * p );
extern void *      Bdc_FuncCopy( Bdc_Fun_t * p );
extern int         Bdc_FuncCopyInt( Bdc_Fun_t * p );
extern void        Bdc_FuncSetCopy( Bdc_Fun_t * p, void * pCopy );
extern void        Bdc_FuncSetCopyInt( Bdc_Fun_t * p, int iCopy );
extern int         Bdc_ManBidecNodeNum( word * pFunc, word * pCare, int nVars, int fVerbose );
extern Vec_Int_t * Bdc_ManBidecResub( word * pFunc, word * pCare, int nVars );

/*=== working with saved copies ==========================================*/
static inline int  Bdc_FunObjCopy( Bdc_Fun_t * pObj )     { return Abc_LitNotCond( Bdc_FuncCopyInt(Bdc_Regular(pObj)), Bdc_IsComplement(pObj) );  }
static inline int  Bdc_FunFanin0Copy( Bdc_Fun_t * pObj )  { return Bdc_FunObjCopy( Bdc_FuncFanin0(pObj) );                                        }
static inline int  Bdc_FunFanin1Copy( Bdc_Fun_t * pObj )  { return Bdc_FunObjCopy( Bdc_FuncFanin1(pObj) );                                        }


ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

