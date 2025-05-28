/**CFile****************************************************************

  FileName    [giaCSat2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Circuit-based SAT solver.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaCSat2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Tas_Par_t_ Tas_Par_t;
struct Tas_Par_t_
{
    // conflict limits
    int           nBTLimit;     // limit on the number of conflicts
    // current parameters
    int           nBTThis;      // number of conflicts
    int           nBTTotal;     // total number of conflicts
    // decision heuristics
    int           fUseHighest;  // use node with the highest ID
    // other parameters
    int           fVerbose;
};

typedef struct Tas_Sto_t_ Tas_Sto_t;
struct Tas_Sto_t_
{
    int           iCur;         // currently used
    int           nSize;        // allocated size
    char *        pBuffer;      // handles of objects stored in the queue
};

typedef struct Tas_Que_t_ Tas_Que_t;
struct Tas_Que_t_
{
    int           iHead;        // beginning of the queue
    int           iTail;        // end of the queue
    int           nSize;        // allocated size
    int *         pData;        // handles of objects stored in the queue
};

typedef struct Tas_Var_t_ Tas_Var_t;
struct Tas_Var_t_
{
    unsigned      fTerm   :  1; // terminal node
    unsigned      fVal    :  1; // current value
    unsigned      fValOld :  1; // previous value
    unsigned      fAssign :  1; // assigned status
    unsigned      fJQueue :  1; // part of J-frontier
    unsigned      fCompl0 :  1; // complemented attribute
    unsigned      fCompl1 :  1; // complemented attribute
    unsigned      fMark0  :  1; // multi-purpose mark
    unsigned      fMark1  :  1; // multi-purpose mark
    unsigned      fPhase  :  1; // polarity
    unsigned      Level   : 22; // decision level 
    int           Id;           // unique ID of this variable
    int           IdAig;        // original ID of this variable
    int           Reason0;      // reason of this variable
    int           Reason1;      // reason of this variable
    int           Diff0;        // difference for the first fanin 
    int           Diff1;        // difference for the second fanin 
    int           Watch0;       // handle of first watch
    int           Watch1;       // handle of second watch   
};

typedef struct Tas_Cls_t_ Tas_Cls_t;
struct Tas_Cls_t_
{
    int           Watch0;       // next clause to watch
    int           Watch1;       // next clause to watch
    int           pVars[0];     // variable handles
};
 
typedef struct Tas_Man_t_ Tas_Man_t;
struct Tas_Man_t_
{
    // user data
    Gia_Man_t *   pAig;         // AIG manager
    Tas_Par_t     Pars;         // parameters
    // solver data
    Tas_Sto_t *   pVars;        // variables
    Tas_Sto_t *   pClauses;     // clauses
    // state representation
    Tas_Que_t     pProp;        // propagation queue
    Tas_Que_t     pJust;        // justification queue
    Vec_Int_t *   vModel;       // satisfying assignment
    Vec_Ptr_t *   vTemp;        // temporary storage
    // SAT calls statistics
    int           nSatUnsat;    // the number of proofs
    int           nSatSat;      // the number of failure
    int           nSatUndec;    // the number of timeouts
    int           nSatTotal;    // the number of calls
    // conflicts
    int           nConfUnsat;   // conflicts in unsat problems
    int           nConfSat;     // conflicts in sat problems
    int           nConfUndec;   // conflicts in undec problems
    int           nConfTotal;   // total conflicts
    // runtime stats
    clock_t       timeSatUnsat; // unsat
    clock_t       timeSatSat;   // sat
    clock_t       timeSatUndec; // undecided
    clock_t       timeTotal;    // total runtime
};

static inline int         Tas_VarIsAssigned( Tas_Var_t * pVar )        { return pVar->fAssign;                                      }
static inline void        Tas_VarAssign( Tas_Var_t * pVar )            { assert(!pVar->fAssign); pVar->fAssign = 1;                 }
static inline void        Tas_VarUnassign( Tas_Var_t * pVar )          { assert(pVar->fAssign);  pVar->fAssign = 0; pVar->fVal = 0; }
static inline int         Tas_VarValue( Tas_Var_t * pVar )             { assert(pVar->fAssign);  return pVar->fVal;                 }
static inline void        Tas_VarSetValue( Tas_Var_t * pVar, int v )   { assert(pVar->fAssign);  pVar->fVal = v;                    }
static inline int         Tas_VarIsJust( Tas_Var_t * pVar )            { return Gia_ObjIsAnd(pVar) && !Tas_VarIsAssigned(Gia_ObjFanin0(pVar)) && !Tas_VarIsAssigned(Gia_ObjFanin1(pVar)); } 
static inline int         Tas_VarFanin0Value( Tas_Var_t * pVar )       { return !Tas_VarIsAssigned(Gia_ObjFanin0(pVar)) ? 2 : (Tas_VarValue(Gia_ObjFanin0(pVar)) ^ Gia_ObjFaninC0(pVar)); }
static inline int         Tas_VarFanin1Value( Tas_Var_t * pVar )       { return !Tas_VarIsAssigned(Gia_ObjFanin1(pVar)) ? 2 : (Tas_VarValue(Gia_ObjFanin1(pVar)) ^ Gia_ObjFaninC1(pVar)); }

static inline int         Tas_VarDecLevel( Tas_Man_t * p, Tas_Var_t * pVar )  { assert( pVar->Value != ~0 ); return Vec_IntEntry(p->vLevReas, 3*pVar->Value);          }
static inline Tas_Var_t * Tas_VarReason0( Tas_Man_t * p, Tas_Var_t * pVar )   { assert( pVar->Value != ~0 ); return pVar + Vec_IntEntry(p->vLevReas, 3*pVar->Value+1); }
static inline Tas_Var_t * Tas_VarReason1( Tas_Man_t * p, Tas_Var_t * pVar )   { assert( pVar->Value != ~0 ); return pVar + Vec_IntEntry(p->vLevReas, 3*pVar->Value+2); }
static inline int         Tas_ClauseDecLevel( Tas_Man_t * p, int hClause )    { return Tas_VarDecLevel( p, p->pClauses.pData[hClause] );                               }

static inline Tas_Var_t * Tas_ManVar( Tas_Man_t * p, int h )           { return (Tas_Var_t *)(p->pVars->pBuffer + h);               }
static inline Tas_Cls_t * Tas_ManClause( Tas_Man_t * p, int h )        { return (Tas_Cls_t *)(p->pClauses->pBuffer + h);            }

#define Tas_ClaForEachVar( p, pClause, pVar, i )          \
    for ( pVar = Tas_ManVar(p, pClause->pVars[(i=0)]); pClause->pVars[i]; pVar = (Tas_Var_t *)(((char *)pVar + pClause->pVars[++i])) )

#define Tas_QueForEachVar( p, pQue, pVar, i )             \
    for ( pVar = Tas_ManVar(p, pQue->pVars[(i=pQue->iHead)]); i < pQue->iTail; pVar = Tas_ManVar(p, pQue->pVars[i++]) )


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Tas_Var_t * Tas_ManCreateVar( Tas_Man_t * p )
{
    Tas_Var_t * pVar;
    if ( p->pVars->iCur + sizeof(Tas_Var_t) > p->pVars->nSize )
    {
        p->pVars->nSize *= 2;
        p->pVars->pData = ABC_REALLOC( char, p->pVars->pData, p->pVars->nSize ); 
    }
    pVar = p->pVars->pData + p->pVars->iCur;
    p->pVars->iCur += sizeof(Tas_Var_t);
    memset( pVar, 0, sizeof(Tas_Var_t) );
    pVar->Id = pVar - ((Tas_Var_t *)p->pVars->pData);
    return pVar;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Tas_Var_t * Tas_ManObj2Var( Tas_Man_t * p, Gia_Obj_t * pObj )
{
    Tas_Var_t * pVar;
    assert( !Gia_ObjIsComplement(pObj) );
    if ( pObj->Value == 0 )
    {
        pVar = Tas_ManCreateVar( p );
        pVar->

    }
    return Tas_ManVar( p, pObj->Value );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

