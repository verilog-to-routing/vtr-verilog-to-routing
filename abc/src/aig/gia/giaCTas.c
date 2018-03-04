/**CFile****************************************************************

  FileName    [giaCSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [A simple circuit-based solver.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaCSat.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


//#define gia_assert(exp)     ((void)0)
//#define gia_assert(exp)     (assert(exp))

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Tas_Par_t_ Tas_Par_t;
struct Tas_Par_t_
{
    // conflict limits
    int           nBTLimit;     // limit on the number of conflicts
    int           nJustLimit;   // limit on the size of justification queue
    // current parameters
    int           nBTThis;      // number of conflicts
    int           nBTThisNc;    // number of conflicts
    int           nJustThis;    // max size of the frontier
    int           nBTTotal;     // total number of conflicts
    int           nJustTotal;   // total size of the frontier
    // activity
    float         VarDecay;     // variable activity decay
    int           VarInc;       // variable increment
    // decision heuristics
    int           fUseActive;   // use most active
    int           fUseHighest;  // use node with the highest ID
    int           fUseLowest;   // use node with the highest ID
    int           fUseMaxFF;    // use node with the largest fanin fanout
    // other
    int           fVerbose;
};

typedef struct Tas_Cls_t_ Tas_Cls_t;
struct Tas_Cls_t_
{
    int           iNext[2];     // beginning of the queue
    int           nLits;        // the number of literals
    int           pLits[0];     // clause literals
};

typedef struct Tas_Sto_t_ Tas_Sto_t;
struct Tas_Sto_t_
{
    int           iCur;         // current position
    int           nSize;        // allocated size
    int *         pData;        // clause information
};

typedef struct Tas_Que_t_ Tas_Que_t;
struct Tas_Que_t_
{
    int           iHead;        // beginning of the queue
    int           iTail;        // end of the queue
    int           nSize;        // allocated size
    Gia_Obj_t **  pData;        // nodes stored in the queue
};
 
struct Tas_Man_t_
{
    Tas_Par_t     Pars;         // parameters
    Gia_Man_t *   pAig;         // AIG manager
    Tas_Que_t     pProp;        // propagation queue
    Tas_Que_t     pJust;        // justification queue
    Tas_Que_t     pClauses;     // clause queue
    Gia_Obj_t **  pIter;        // iterator through clause vars
    Vec_Int_t *   vLevReas;     // levels and decisions
    Vec_Int_t *   vModel;       // satisfying assignment
    Vec_Ptr_t *   vTemp;        // temporary storage
    // watched clauses
    Tas_Sto_t     pStore;       // storage for watched clauses
    int *         pWatches;     // watched lists for each literal
    Vec_Int_t *   vWatchLits;   // lits whose watched are assigned
    int           nClauses;     // the counter of clauses
    // activity
    float *       pActivity;    // variable activity
    Vec_Int_t *   vActiveVars;  // variables with activity
    // SAT calls statistics
    int           nSatUnsat;    // the number of proofs
    int           nSatSat;      // the number of failure
    int           nSatUndec;    // the number of timeouts
    int           nSatTotal;    // the number of calls
    // conflicts
    int           nConfUnsat;   // conflicts in unsat problems
    int           nConfSat;     // conflicts in sat problems
    int           nConfUndec;   // conflicts in undec problems
    // runtime stats
    abctime       timeSatUnsat; // unsat
    abctime       timeSatSat;   // sat
    abctime       timeSatUndec; // undecided
    abctime       timeTotal;    // total runtime
};

static inline int   Tas_VarIsAssigned( Gia_Obj_t * pVar )      { return pVar->fMark0;                        }
static inline void  Tas_VarAssign( Gia_Obj_t * pVar )          { assert(!pVar->fMark0); pVar->fMark0 = 1;    }
static inline void  Tas_VarUnassign( Gia_Obj_t * pVar )        { assert(pVar->fMark0);  pVar->fMark0 = 0; pVar->fMark1 = 0; pVar->Value = ~0; }
static inline int   Tas_VarValue( Gia_Obj_t * pVar )           { assert(pVar->fMark0);  return pVar->fMark1; }
static inline void  Tas_VarSetValue( Gia_Obj_t * pVar, int v ) { assert(pVar->fMark0);  pVar->fMark1 = v;    }
static inline int   Tas_VarIsJust( Gia_Obj_t * pVar )          { return Gia_ObjIsAnd(pVar) && !Tas_VarIsAssigned(Gia_ObjFanin0(pVar)) && !Tas_VarIsAssigned(Gia_ObjFanin1(pVar)); } 
static inline int   Tas_VarFanin0Value( Gia_Obj_t * pVar )     { return !Tas_VarIsAssigned(Gia_ObjFanin0(pVar)) ? 2 : (Tas_VarValue(Gia_ObjFanin0(pVar)) ^ Gia_ObjFaninC0(pVar)); }
static inline int   Tas_VarFanin1Value( Gia_Obj_t * pVar )     { return !Tas_VarIsAssigned(Gia_ObjFanin1(pVar)) ? 2 : (Tas_VarValue(Gia_ObjFanin1(pVar)) ^ Gia_ObjFaninC1(pVar)); }
static inline int   Tas_VarToLit( Tas_Man_t * p, Gia_Obj_t * pObj ) { assert( Tas_VarIsAssigned(pObj) ); return Abc_Var2Lit( Gia_ObjId(p->pAig, pObj), !Tas_VarValue(pObj) );     }
static inline int   Tas_LitIsTrue( Gia_Obj_t * pObj, int Lit ) { assert( Tas_VarIsAssigned(pObj) ); return Tas_VarValue(pObj) != Abc_LitIsCompl(Lit);                             }

static inline int         Tas_ClsHandle( Tas_Man_t * p, Tas_Cls_t * pClause ) { return ((int *)pClause) - p->pStore.pData;   }
static inline Tas_Cls_t * Tas_ClsFromHandle( Tas_Man_t * p, int h )           { return (Tas_Cls_t *)(p->pStore.pData + h);   }

static inline int         Tas_VarDecLevel( Tas_Man_t * p, Gia_Obj_t * pVar )  { assert( pVar->Value != ~0 ); return Vec_IntEntry(p->vLevReas, 3*pVar->Value);          }
static inline Gia_Obj_t * Tas_VarReason0( Tas_Man_t * p, Gia_Obj_t * pVar )   { assert( pVar->Value != ~0 ); return pVar + Vec_IntEntry(p->vLevReas, 3*pVar->Value+1); }
static inline Gia_Obj_t * Tas_VarReason1( Tas_Man_t * p, Gia_Obj_t * pVar )   { assert( pVar->Value != ~0 ); return pVar + Vec_IntEntry(p->vLevReas, 3*pVar->Value+2); }
static inline int         Tas_ClauseDecLevel( Tas_Man_t * p, int hClause )    { return Tas_VarDecLevel( p, p->pClauses.pData[hClause] );                               }

static inline int         Tas_VarHasReasonCls( Tas_Man_t * p, Gia_Obj_t * pVar ) { assert( pVar->Value != ~0 ); return Vec_IntEntry(p->vLevReas, 3*pVar->Value+1) == 0 && Vec_IntEntry(p->vLevReas, 3*pVar->Value+2) != 0; }
static inline Tas_Cls_t * Tas_VarReasonCls( Tas_Man_t * p, Gia_Obj_t * pVar )    { assert( pVar->Value != ~0 ); return Tas_ClsFromHandle( p, Vec_IntEntry(p->vLevReas, 3*pVar->Value+2) );                                 }

#define Tas_QueForEachEntry( Que, pObj, i )                         \
    for ( i = (Que).iHead; (i < (Que).iTail) && ((pObj) = (Que).pData[i]); i++ )

#define Tas_ClauseForEachVar( p, hClause, pObj )                    \
    for ( (p)->pIter = (p)->pClauses.pData + hClause; (pObj = *pIter); (p)->pIter++ )
#define Tas_ClauseForEachVar1( p, hClause, pObj )                   \
    for ( (p)->pIter = (p)->pClauses.pData+hClause+1; (pObj = *pIter); (p)->pIter++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sets default values of the parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tas_SetDefaultParams( Tas_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Tas_Par_t) );
    pPars->nBTLimit    =        2000;   // limit on the number of conflicts
    pPars->nJustLimit  =        2000;   // limit on the size of justification queue
    pPars->fUseActive  =           0;   // use node with the highest activity
    pPars->fUseHighest =           1;   // use node with the highest ID
    pPars->fUseLowest  =           0;   // use node with the lowest ID
    pPars->fUseMaxFF   =           0;   // use node with the largest fanin fanout
    pPars->fVerbose    =           1;   // print detailed statistics
    pPars->VarDecay    = (float)0.95;   // variable decay
    pPars->VarInc      =         1.0;   // variable increment
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Tas_Man_t * Tas_ManAlloc( Gia_Man_t * pAig, int nBTLimit )
{
    Tas_Man_t * p;
    p = ABC_CALLOC( Tas_Man_t, 1 );
    Tas_SetDefaultParams( &p->Pars );
    p->pAig           = pAig;
    p->Pars.nBTLimit  = nBTLimit;
    p->pProp.nSize    = p->pJust.nSize = p->pClauses.nSize = 10000;
    p->pProp.pData    = ABC_ALLOC( Gia_Obj_t *, p->pProp.nSize );
    p->pJust.pData    = ABC_ALLOC( Gia_Obj_t *, p->pJust.nSize );
    p->pClauses.pData = ABC_ALLOC( Gia_Obj_t *, p->pClauses.nSize );
    p->pClauses.iHead = p->pClauses.iTail = 1;
    p->vModel         = Vec_IntAlloc( 1000 );
    p->vLevReas       = Vec_IntAlloc( 1000 );
    p->vTemp          = Vec_PtrAlloc( 1000 );
    p->pStore.iCur    = 16;
    p->pStore.nSize   = 10000;
    p->pStore.pData   = ABC_ALLOC( int, p->pStore.nSize );
    p->pWatches       = ABC_CALLOC( int, 2 * Gia_ManObjNum(pAig) );
    p->vWatchLits     = Vec_IntAlloc( 100 );
    p->pActivity      = ABC_CALLOC( float, Gia_ManObjNum(pAig) );
    p->vActiveVars    = Vec_IntAlloc( 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tas_ManStop( Tas_Man_t * p )
{
    Vec_IntFree( p->vActiveVars );
    Vec_IntFree( p->vWatchLits );
    Vec_IntFree( p->vLevReas );
    Vec_IntFree( p->vModel );
    Vec_PtrFree( p->vTemp );
    ABC_FREE( p->pActivity );
    ABC_FREE( p->pWatches );
    ABC_FREE( p->pStore.pData );
    ABC_FREE( p->pClauses.pData );
    ABC_FREE( p->pProp.pData );
    ABC_FREE( p->pJust.pData );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns satisfying assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Tas_ReadModel( Tas_Man_t * p )
{
    return p->vModel;
}




/**Function*************************************************************

  Synopsis    [Returns 1 if the solver is out of limits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_ManCheckLimits( Tas_Man_t * p )
{
    return p->Pars.nJustThis > p->Pars.nJustLimit || p->Pars.nBTThis > p->Pars.nBTLimit;
}

/**Function*************************************************************

  Synopsis    [Saves the satisfying assignment as an array of literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Tas_ManSaveModel( Tas_Man_t * p, Vec_Int_t * vCex )
{
    Gia_Obj_t * pVar;
    int i;
    Vec_IntClear( vCex );
    p->pProp.iHead = 0;
//    printf( "\n" );
    Tas_QueForEachEntry( p->pProp, pVar, i )
    {
        if ( Gia_ObjIsCi(pVar) )
//            Vec_IntPush( vCex, Abc_Var2Lit(Gia_ObjId(p->pAig,pVar), !Tas_VarValue(pVar)) );
            Vec_IntPush( vCex, Abc_Var2Lit(Gia_ObjCioId(pVar), !Tas_VarValue(pVar)) );
/*
        printf( "%5d(%d) = ", Gia_ObjId(p->pAig, pVar), Tas_VarValue(pVar) );
        if ( Gia_ObjIsCi(pVar) )
            printf( "pi %d\n", Gia_ObjCioId(pVar) );
        else
        {
            printf( "%5d %d  &  ", Gia_ObjFaninId0p(p->pAig, pVar), Gia_ObjFaninC0(pVar) );
            printf( "%5d %d     ", Gia_ObjFaninId1p(p->pAig, pVar), Gia_ObjFaninC1(pVar) );
            printf( "\n" );
        }
*/
    }
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_QueIsEmpty( Tas_Que_t * p )
{
    return p->iHead == p->iTail;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Tas_QuePush( Tas_Que_t * p, Gia_Obj_t * pObj )
{
    if ( p->iTail == p->nSize )
    {
        p->nSize *= 2;
        p->pData = ABC_REALLOC( Gia_Obj_t *, p->pData, p->nSize ); 
    }
    p->pData[p->iTail++] = pObj;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the object in the queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_QueHasNode( Tas_Que_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pTemp;
    int i;
    Tas_QueForEachEntry( *p, pTemp, i )
        if ( pTemp == pObj )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Tas_QueStore( Tas_Que_t * p, int * piHeadOld, int * piTailOld )
{
    int i;
    *piHeadOld = p->iHead;
    *piTailOld = p->iTail;
    for ( i = *piHeadOld; i < *piTailOld; i++ )
        Tas_QuePush( p, p->pData[i] );
    p->iHead = *piTailOld;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Tas_QueRestore( Tas_Que_t * p, int iHeadOld, int iTailOld )
{
    p->iHead = iHeadOld;
    p->iTail = iTailOld;
}

/**Function*************************************************************

  Synopsis    [Finalized the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_QueFinish( Tas_Que_t * p )
{
    int iHeadOld = p->iHead;
    assert( p->iHead < p->iTail );
    Tas_QuePush( p, NULL );
    p->iHead = p->iTail;
    return iHeadOld;
}


/**Function*************************************************************

  Synopsis    [Max number of fanins fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_VarFaninFanoutMax( Tas_Man_t * p, Gia_Obj_t * pObj )
{
    int Count0, Count1;
    assert( !Gia_IsComplement(pObj) );
    assert( Gia_ObjIsAnd(pObj) );
    Count0 = Gia_ObjRefNum( p->pAig, Gia_ObjFanin0(pObj) );
    Count1 = Gia_ObjRefNum( p->pAig, Gia_ObjFanin1(pObj) );
    return Abc_MaxInt( Count0, Count1 );
}



/**Function*************************************************************

  Synopsis    [Find variable with the highest activity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Tas_ManFindActive( Tas_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMax = NULL;
    float BestCost = 0.0;
    int i, ObjId;
    Tas_QueForEachEntry( p->pJust, pObj, i )
    {
        assert( Gia_ObjIsAnd(pObj) );
        ObjId = Gia_ObjId( p->pAig, pObj );
        if ( pObjMax == NULL || 
             p->pActivity[Gia_ObjFaninId0(pObj,ObjId)] > BestCost || 
            (p->pActivity[Gia_ObjFaninId0(pObj,ObjId)] == BestCost && pObjMax < Gia_ObjFanin0(pObj)) )
        {
            pObjMax  = Gia_ObjFanin0(pObj);
            BestCost = p->pActivity[Gia_ObjFaninId0(pObj,ObjId)];
        }
        if ( p->pActivity[Gia_ObjFaninId1(pObj,ObjId)] > BestCost || 
            (p->pActivity[Gia_ObjFaninId1(pObj,ObjId)] == BestCost && pObjMax < Gia_ObjFanin1(pObj)) )
        {
            pObjMax  = Gia_ObjFanin1(pObj);
            BestCost = p->pActivity[Gia_ObjFaninId1(pObj,ObjId)];
        }
    }
    return pObjMax;
}

/**Function*************************************************************

  Synopsis    [Find variable with the highest activity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Tas_ManDecideHighestFanin( Tas_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMax = NULL;
    int i, ObjId;
    Tas_QueForEachEntry( p->pJust, pObj, i )
    {
        assert( Gia_ObjIsAnd(pObj) );
        ObjId = Gia_ObjId( p->pAig, pObj );
        if ( pObjMax == NULL || pObjMax < Gia_ObjFanin0(pObj) )
            pObjMax  = Gia_ObjFanin0(pObj);
        if ( pObjMax < Gia_ObjFanin1(pObj) )
            pObjMax  = Gia_ObjFanin1(pObj);
    }
    return pObjMax;
}

/**Function*************************************************************

  Synopsis    [Find variable with the highest ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Tas_ManDecideHighest( Tas_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMax = NULL;
    int i;
    Tas_QueForEachEntry( p->pJust, pObj, i )
    {
//printf( "%d %6.2f   ", Gia_ObjId(p->pAig, pObj), p->pActivity[Gia_ObjId(p->pAig, pObj)] );
        if ( pObjMax == NULL || pObjMax < pObj )
            pObjMax = pObj;
    }
//printf( "\n" );
    return pObjMax;
}

/**Function*************************************************************

  Synopsis    [Find variable with the highest ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Tas_ManDecideHighestA( Tas_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMax = NULL;
    int i;
    Tas_QueForEachEntry( p->pJust, pObj, i )
    {
        if ( pObjMax == NULL || 
             p->pActivity[Gia_ObjId(p->pAig, pObjMax)] < p->pActivity[Gia_ObjId(p->pAig, pObj)] )
            pObjMax = pObj;
    }
    return pObjMax;
}

/**Function*************************************************************

  Synopsis    [Find variable with the lowest ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Tas_ManDecideLowest( Tas_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMin = NULL;
    int i;
    Tas_QueForEachEntry( p->pJust, pObj, i )
        if ( pObjMin == NULL || pObjMin > pObj )
            pObjMin = pObj;
    return pObjMin;
}

/**Function*************************************************************

  Synopsis    [Find variable with the maximum number of fanin fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Tas_ManDecideMaxFF( Tas_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMax = NULL;
    int i, iMaxFF = 0, iCurFF;
    assert( p->pAig->pRefs != NULL );
    Tas_QueForEachEntry( p->pJust, pObj, i )
    {
        iCurFF = Tas_VarFaninFanoutMax( p, pObj );
        assert( iCurFF > 0 );
        if ( iMaxFF < iCurFF )
        {
            iMaxFF = iCurFF;
            pObjMax = pObj;
        }
    }
    return pObjMax; 
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Tas_ManCancelUntil( Tas_Man_t * p, int iBound )
{
    Gia_Obj_t * pVar;
    int i;
    assert( iBound <= p->pProp.iTail );
    p->pProp.iHead = iBound;
    Tas_QueForEachEntry( p->pProp, pVar, i )
        Tas_VarUnassign( pVar );
    p->pProp.iTail = iBound;
    Vec_IntShrink( p->vLevReas, 3*iBound );
}

int s_Counter2 = 0;
int s_Counter3 = 0;
int s_Counter4 = 0;

/**Function*************************************************************

  Synopsis    [Assigns the variables a value.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Tas_ManAssign( Tas_Man_t * p, Gia_Obj_t * pObj, int Level, Gia_Obj_t * pRes0, Gia_Obj_t * pRes1 )
{
    Gia_Obj_t * pObjR = Gia_Regular(pObj);
    assert( Gia_ObjIsCand(pObjR) );
    assert( !Tas_VarIsAssigned(pObjR) );
    Tas_VarAssign( pObjR );
    Tas_VarSetValue( pObjR, !Gia_IsComplement(pObj) );
    assert( pObjR->Value == ~0 );
    pObjR->Value = p->pProp.iTail;
    Tas_QuePush( &p->pProp, pObjR );
    Vec_IntPush( p->vLevReas, Level );
    if ( pRes0 == NULL && pRes1 != 0 ) // clause
    {
        Vec_IntPush( p->vLevReas, 0 );
        Vec_IntPush( p->vLevReas, Tas_ClsHandle( p, (Tas_Cls_t *)pRes1 ) );
    }
    else
    {
        Vec_IntPush( p->vLevReas, pRes0 ? pRes0-pObjR : 0 );
        Vec_IntPush( p->vLevReas, pRes1 ? pRes1-pObjR : 0 );
    }
    assert( Vec_IntSize(p->vLevReas) == 3 * p->pProp.iTail );
    s_Counter2++;
}


/**Function*************************************************************

  Synopsis    [Returns clause size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_ManClauseSize( Tas_Man_t * p, int hClause )
{
    Tas_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t ** pIter;
    for ( pIter = pQue->pData + hClause; *pIter; pIter++ );
    return pIter - pQue->pData - hClause ;
}

/**Function*************************************************************

  Synopsis    [Prints conflict clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Tas_ManPrintClause( Tas_Man_t * p, int Level, int hClause )
{
    Tas_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pObj;
    int i;
    assert( Tas_QueIsEmpty( pQue ) );
    printf( "Level %2d : ", Level );
    for ( i = hClause; (pObj = pQue->pData[i]); i++ )
        printf( "%d=%d(%d) ", Gia_ObjId(p->pAig, pObj), Tas_VarValue(pObj), Tas_VarDecLevel(p, pObj) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints conflict clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Tas_ManPrintClauseNew( Tas_Man_t * p, int Level, int hClause )
{
    Tas_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pObj;
    int i;
    assert( Tas_QueIsEmpty( pQue ) );
    printf( "Level %2d : ", Level );
    for ( i = hClause; (pObj = pQue->pData[i]); i++ )
        printf( "%c%d ", Tas_VarValue(pObj)? '+':'-', Gia_ObjId(p->pAig, pObj) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Returns conflict clause.]

  Description [Performs conflict analysis.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Tas_ManDeriveReason( Tas_Man_t * p, int Level )
{
    Tas_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pObj, * pReason;
    int i, k, j, iLitLevel, iLitLevel2;//, Id;
    assert( pQue->pData[pQue->iHead] == NULL );
    assert( pQue->iHead + 1 < pQue->iTail );
/*
    for ( i = pQue->iHead + 1; i < pQue->iTail; i++ )
    {
        pObj = pQue->pData[i];
        assert( pObj->fPhase == 0 );
    }
*/
    // compact literals
    Vec_PtrClear( p->vTemp );
    for ( i = k = pQue->iHead + 1; i < pQue->iTail; i++ )
    {
        pObj = pQue->pData[i];
        if ( pObj->fPhase ) // unassigned - seen again
            continue;
        // assigned - seen first time
        pObj->fPhase = 1;
        Vec_PtrPush( p->vTemp, pObj );
        // bump activity
//        Id = Gia_ObjId( p->pAig, pObj );
//        if ( p->pActivity[Id] == 0.0 )
//            Vec_IntPush( p->vActiveVars, Id );
//        p->pActivity[Id] += p->Pars.VarInc;
        // check decision level
        iLitLevel = Tas_VarDecLevel( p, pObj );
        if ( iLitLevel < Level )
        {
            pQue->pData[k++] = pObj;
            continue;
        }
        assert( iLitLevel == Level );
        if ( Tas_VarHasReasonCls( p, pObj ) )
        {
            Tas_Cls_t * pCls = Tas_VarReasonCls( p, pObj );
            pReason = Gia_ManObj( p->pAig, Abc_Lit2Var(pCls->pLits[0]) );
            assert( pReason == pObj );
            for ( j = 1; j < pCls->nLits; j++ )
            {
                pReason = Gia_ManObj( p->pAig, Abc_Lit2Var(pCls->pLits[j]) );
                iLitLevel2 = Tas_VarDecLevel( p, pReason );
                assert( Tas_VarIsAssigned( pReason ) );
                assert( !Tas_LitIsTrue( pReason, pCls->pLits[j] ) );
                Tas_QuePush( pQue, pReason );
            }
        }
        else
        {
            pReason = Tas_VarReason0( p, pObj );
            if ( pReason == pObj ) // no reason
            {
                assert( pQue->pData[pQue->iHead] == NULL || Level == 0 );
                if ( pQue->pData[pQue->iHead] == NULL )
                    pQue->pData[pQue->iHead] = pObj;
                else
                    Tas_QuePush( pQue, pObj );
                continue;
            }
            Tas_QuePush( pQue, pReason );
            pReason = Tas_VarReason1( p, pObj );
            if ( pReason != pObj ) // second reason
                Tas_QuePush( pQue, pReason );
        }
    }
    assert( pQue->pData[pQue->iHead] != NULL );
    if ( pQue->pData[pQue->iHead] == NULL )
        printf( "Tas_ManDeriveReason(): Failed to derive the clause!!!\n" );
    pQue->iTail = k;
    // clear the marks
    Vec_PtrForEachEntry( Gia_Obj_t *, p->vTemp, pObj, i )
        pObj->fPhase = 0;
}

/**Function*************************************************************

  Synopsis    [Returns conflict clause.]

  Description [Performs conflict analysis.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_ManAnalyze( Tas_Man_t * p, int Level, Gia_Obj_t * pVar, Gia_Obj_t * pFan0, Gia_Obj_t * pFan1 )
{
    Tas_Que_t * pQue = &(p->pClauses);
    assert( Tas_VarIsAssigned(pVar) );
    assert( Tas_VarIsAssigned(pFan0) );
    assert( pFan1 == NULL || Tas_VarIsAssigned(pFan1) );
    assert( Tas_QueIsEmpty( pQue ) );
    Tas_QuePush( pQue, NULL );
    Tas_QuePush( pQue, pVar );
    Tas_QuePush( pQue, pFan0 );
    if ( pFan1 )
        Tas_QuePush( pQue, pFan1 );
    Tas_ManDeriveReason( p, Level );
    return Tas_QueFinish( pQue );
}


/**Function*************************************************************

  Synopsis    [Performs resolution of two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_ManResolve( Tas_Man_t * p, int Level, int hClause0, int hClause1 )
{
    Tas_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pObj;
    int i, LevelMax = -1, LevelCur;
    assert( pQue->pData[hClause0] != NULL );
    assert( pQue->pData[hClause0] == pQue->pData[hClause1] );
/*
    for ( i = hClause0 + 1; (pObj = pQue->pData[i]); i++ )
        assert( pObj->fPhase == 0 );
    for ( i = hClause1 + 1; (pObj = pQue->pData[i]); i++ )
        assert( pObj->fPhase == 0 );
*/
    assert( Tas_QueIsEmpty( pQue ) );
    Tas_QuePush( pQue, NULL );
    for ( i = hClause0 + 1; (pObj = pQue->pData[i]); i++ )
    {
        if ( pObj->fPhase ) // unassigned - seen again
            continue;
        // assigned - seen first time
        pObj->fPhase = 1;
        Tas_QuePush( pQue, pObj );
        LevelCur = Tas_VarDecLevel( p, pObj );
        if ( LevelMax < LevelCur )
            LevelMax = LevelCur;
    }
    for ( i = hClause1 + 1; (pObj = pQue->pData[i]); i++ )
    {
        if ( pObj->fPhase ) // unassigned - seen again
            continue;
        // assigned - seen first time
        pObj->fPhase = 1;
        Tas_QuePush( pQue, pObj );
        LevelCur = Tas_VarDecLevel( p, pObj );
        if ( LevelMax < LevelCur )
            LevelMax = LevelCur;
    }
    for ( i = pQue->iHead + 1; i < pQue->iTail; i++ )
        pQue->pData[i]->fPhase = 0;
    Tas_ManDeriveReason( p, LevelMax );
    return Tas_QueFinish( pQue );
}



/**Function*************************************************************

  Synopsis    [Allocates clause of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Tas_Cls_t * Tas_ManAllocCls( Tas_Man_t * p, int nSize )
{
    Tas_Cls_t * pCls;
    if ( p->pStore.iCur + nSize > p->pStore.nSize )
    {
        p->pStore.nSize *= 2;
        p->pStore.pData  = ABC_REALLOC( int, p->pStore.pData, p->pStore.nSize ); 
    }
    pCls = Tas_ClsFromHandle( p, p->pStore.iCur ); p->pStore.iCur += nSize;
    memset( pCls, 0, sizeof(int) * nSize );
    p->nClauses++;
    return pCls;
}

/**Function*************************************************************

  Synopsis    [Adds one clause to the watcher list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Tas_ManWatchClause( Tas_Man_t * p, Tas_Cls_t * pClause, int Lit )
{
    assert( Abc_Lit2Var(Lit) < Gia_ManObjNum(p->pAig) );
    assert( pClause->nLits >= 2 );
    assert( pClause->pLits[0] == Lit || pClause->pLits[1] == Lit );
    if ( pClause->pLits[0] == Lit )
        pClause->iNext[0] = p->pWatches[Abc_LitNot(Lit)];  
    else
        pClause->iNext[1] = p->pWatches[Abc_LitNot(Lit)];  
    if ( p->pWatches[Abc_LitNot(Lit)] == 0 )
        Vec_IntPush( p->vWatchLits, Abc_LitNot(Lit) );
    p->pWatches[Abc_LitNot(Lit)] = Tas_ClsHandle( p, pClause );
}

/**Function*************************************************************

  Synopsis    [Creates clause of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Tas_Cls_t * Tas_ManCreateCls( Tas_Man_t * p, int hClause )
{
    Tas_Cls_t * pClause;
    Tas_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pObj;
    int i, nLits = 0;
    assert( Tas_QueIsEmpty( pQue ) );
    assert( pQue->pData[hClause] != NULL );
    for ( i = hClause; (pObj = pQue->pData[i]); i++ )
        nLits++;
    if ( nLits == 1 )
        return NULL;
    // create this clause
    pClause = Tas_ManAllocCls( p, nLits + 3 );
    pClause->nLits = nLits;
    for ( i = hClause; (pObj = pQue->pData[i]); i++ )
    {
        assert( Tas_VarIsAssigned( pObj ) );
        pClause->pLits[i-hClause] = Abc_LitNot( Tas_VarToLit(p, pObj) );
    }
    // add the clause as watched one
    if ( nLits >= 2 )
    {
        Tas_ManWatchClause( p, pClause, pClause->pLits[0] );
        Tas_ManWatchClause( p, pClause, pClause->pLits[1] );
    }
    // increment activity
//    p->Pars.VarInc /= p->Pars.VarDecay;
    return pClause;
}

/**Function*************************************************************

  Synopsis    [Creates clause of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_ManCreateFromCls( Tas_Man_t * p, Tas_Cls_t * pCls, int Level )
{
    Tas_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pObj;
    int i;
    assert( Tas_QueIsEmpty( pQue ) );
    Tas_QuePush( pQue, NULL );
    for ( i = 0; i < pCls->nLits; i++ )
    {
        pObj = Gia_ManObj( p->pAig, Abc_Lit2Var(pCls->pLits[i]) );
        assert( Tas_VarIsAssigned(pObj) );
        assert( !Tas_LitIsTrue( pObj, pCls->pLits[i] ) );
        Tas_QuePush( pQue, pObj );
    }
    Tas_ManDeriveReason( p, Level );
    return Tas_QueFinish( pQue );
}

/**Function*************************************************************

  Synopsis    [Propagate one assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_ManPropagateWatch( Tas_Man_t * p, int Level, int Lit )
{
    Gia_Obj_t * pObj;
    Tas_Cls_t * pCur;
    int * piPrev, iCur, iTemp;
    int i, LitF = Abc_LitNot(Lit);
    // iterate through the clauses
    piPrev = p->pWatches + Lit;
    for ( iCur = p->pWatches[Lit]; iCur; iCur = *piPrev )
    {
        pCur = Tas_ClsFromHandle( p, iCur );
        // make sure the false literal is in the second literal of the clause
        if ( pCur->pLits[0] == LitF )
        {
            pCur->pLits[0] = pCur->pLits[1];
            pCur->pLits[1] = LitF;
            iTemp = pCur->iNext[0];
            pCur->iNext[0] = pCur->iNext[1];
            pCur->iNext[1] = iTemp;
        }
        assert( pCur->pLits[1] == LitF );

        // if the first literal is true, the clause is satisfied
//        if ( pCur->pLits[0] == p->pAssigns[Abc_Lit2Var(pCur->pLits[0])] )
        pObj = Gia_ManObj( p->pAig, Abc_Lit2Var(pCur->pLits[0]) );
        if ( Tas_VarIsAssigned(pObj) && Tas_LitIsTrue( pObj, pCur->pLits[0] ) )
        {
            piPrev = &pCur->iNext[1];
            continue;
        }

        // look for a new literal to watch
        for ( i = 2; i < (int)pCur->nLits; i++ )
        {
            // skip the case when the literal is false
//            if ( Abc_LitNot(pCur->pLits[i]) == p->pAssigns[Abc_Lit2Var(pCur->pLits[i])] )
            pObj = Gia_ManObj( p->pAig, Abc_Lit2Var(pCur->pLits[i]) );
            if ( Tas_VarIsAssigned(pObj) && !Tas_LitIsTrue( pObj, pCur->pLits[i] ) )
                continue;
            // the literal is either true or unassigned - watch it
            pCur->pLits[1] = pCur->pLits[i];
            pCur->pLits[i] = LitF;
            // remove this clause from the watch list of Lit
            *piPrev = pCur->iNext[1];
            // add this clause to the watch list of pCur->pLits[i] (now it is pCur->pLits[1])
            Tas_ManWatchClause( p, pCur, pCur->pLits[1] );
            break;
        }
        if ( i < (int)pCur->nLits ) // found new watch
            continue;

        // clause is unit - enqueue new implication
        pObj = Gia_ManObj( p->pAig, Abc_Lit2Var(pCur->pLits[0]) );
        if ( !Tas_VarIsAssigned(pObj) )
        {
/*
            {
                int iLitLevel, iPlace;
                for ( i = 1; i < (int)pCur->nLits; i++ )
                {
                    pObj = Gia_ManObj( p->pAig, Abc_Lit2Var(pCur->pLits[i]) );
                    iLitLevel = Tas_VarDecLevel( p, pObj );
                    iPlace = pObj->Value;
                    printf( "Lit = %d. Level = %d. Place = %d.\n", pCur->pLits[i], iLitLevel, iPlace );
                    i = i;
                }
            }
*/
            Tas_ManAssign( p, Gia_ObjFromLit(p->pAig, pCur->pLits[0]), Level, NULL, (Gia_Obj_t *)pCur );
            piPrev = &pCur->iNext[1];
            continue;
        }
        // conflict detected - return the conflict clause
        assert( !Tas_LitIsTrue( pObj, pCur->pLits[0] ) );
        return Tas_ManCreateFromCls( p, pCur, Level );
    }
    return 0;
}



/**Function*************************************************************

  Synopsis    [Propagates a variable.]

  Description [Returns clause handle if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_ManPropagateOne( Tas_Man_t * p, Gia_Obj_t * pVar, int Level )
{
    int Value0, Value1, hClause;
    assert( !Gia_IsComplement(pVar) );
    assert( Tas_VarIsAssigned(pVar) );
    s_Counter3++;
    if ( (hClause = Tas_ManPropagateWatch( p, Level, Tas_VarToLit(p, pVar) )) )
        return hClause;
    if ( Gia_ObjIsCi(pVar) )
        return 0;
/*
    if ( pVar->iDiff0 == 570869 && pVar->iDiff1 == 546821 && Level == 3 )
    {
        Gia_Obj_t * pFan0 = Gia_ObjFanin0(pVar);
        Gia_Obj_t * pFan1 = Gia_ObjFanin1(pVar);
        int s = 0;
    }
*/
    assert( Gia_ObjIsAnd(pVar) );
    Value0 = Tas_VarFanin0Value(pVar);
    Value1 = Tas_VarFanin1Value(pVar);
    if ( Tas_VarValue(pVar) )
    { // value is 1
        if ( Value0 == 0 || Value1 == 0 ) // one is 0
        {
            if ( Value0 == 0 && Value1 != 0 )
                return Tas_ManAnalyze( p, Level, pVar, Gia_ObjFanin0(pVar), NULL );
            if ( Value0 != 0 && Value1 == 0 )
                return Tas_ManAnalyze( p, Level, pVar, Gia_ObjFanin1(pVar), NULL );
            assert( Value0 == 0 && Value1 == 0 );
            return Tas_ManAnalyze( p, Level, pVar, Gia_ObjFanin0(pVar), Gia_ObjFanin1(pVar) );
        }
        if ( Value0 == 2 ) // first is unassigned
            Tas_ManAssign( p, Gia_ObjChild0(pVar), Level, pVar, NULL );
        if ( Value1 == 2 ) // first is unassigned
            Tas_ManAssign( p, Gia_ObjChild1(pVar), Level, pVar, NULL );
        return 0;
    }
    // value is 0
    if ( Value0 == 0 || Value1 == 0 ) // one is 0
        return 0;
    if ( Value0 == 1 && Value1 == 1 ) // both are 1
        return Tas_ManAnalyze( p, Level, pVar, Gia_ObjFanin0(pVar), Gia_ObjFanin1(pVar) );
    if ( Value0 == 1 || Value1 == 1 ) // one is 1 
    {
        if ( Value0 == 2 ) // first is unassigned
            Tas_ManAssign( p, Gia_Not(Gia_ObjChild0(pVar)), Level, pVar, Gia_ObjFanin1(pVar) );
        if ( Value1 == 2 ) // second is unassigned
            Tas_ManAssign( p, Gia_Not(Gia_ObjChild1(pVar)), Level, pVar, Gia_ObjFanin0(pVar) );
        return 0;
    }
    assert( Tas_VarIsJust(pVar) );
    assert( !Tas_QueHasNode( &p->pJust, pVar ) );
    Tas_QuePush( &p->pJust, pVar );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Propagates a variable.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Tas_ManPropagateTwo( Tas_Man_t * p, Gia_Obj_t * pVar, int Level )
{
    int Value0, Value1;
    s_Counter4++;
    assert( !Gia_IsComplement(pVar) );
    assert( Gia_ObjIsAnd(pVar) );
    assert( Tas_VarIsAssigned(pVar) );
    assert( !Tas_VarValue(pVar) );
    Value0 = Tas_VarFanin0Value(pVar);
    Value1 = Tas_VarFanin1Value(pVar);
    // value is 0
    if ( Value0 == 0 || Value1 == 0 ) // one is 0
        return 0;
    if ( Value0 == 1 && Value1 == 1 ) // both are 1
        return Tas_ManAnalyze( p, Level, pVar, Gia_ObjFanin0(pVar), Gia_ObjFanin1(pVar) );
    assert( Value0 == 1 || Value1 == 1 );
    if ( Value0 == 2 ) // first is unassigned
        Tas_ManAssign( p, Gia_Not(Gia_ObjChild0(pVar)), Level, pVar, Gia_ObjFanin1(pVar) );
    if ( Value1 == 2 ) // first is unassigned
        Tas_ManAssign( p, Gia_Not(Gia_ObjChild1(pVar)), Level, pVar, Gia_ObjFanin0(pVar) );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Propagates all variables.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tas_ManPropagate( Tas_Man_t * p, int Level )
{
    int hClause;
    Gia_Obj_t * pVar;
    int i, k;//, nIter = 0;
    while ( 1 )
    {
//        nIter++;
        Tas_QueForEachEntry( p->pProp, pVar, i )
        {
            if ( (hClause = Tas_ManPropagateOne( p, pVar, Level )) )
                return hClause;
        }
        p->pProp.iHead = p->pProp.iTail;
        k = p->pJust.iHead;
        Tas_QueForEachEntry( p->pJust, pVar, i )
        {
            if ( Tas_VarIsJust( pVar ) )
                p->pJust.pData[k++] = pVar;
            else if ( (hClause = Tas_ManPropagateTwo( p, pVar, Level )) )
                return hClause;
        }
        if ( k == p->pJust.iTail )
            break;
        p->pJust.iTail = k;
    }
//    printf( "%d ", nIter );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Solve the problem recursively.]

  Description [Returns learnt clause if unsat, NULL if sat or undecided.]
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
int Tas_ManSolve_rec( Tas_Man_t * p, int Level )
{ 
    Tas_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pVar = NULL, * pDecVar = NULL;
    int hClause, hLearn0, hLearn1;
    int iPropHead, iJustHead, iJustTail;
    // propagate assignments
    assert( !Tas_QueIsEmpty(&p->pProp) );
    if ( (hClause = Tas_ManPropagate( p, Level )) )
    {
        Tas_ManCreateCls( p, hClause );
        return hClause;
    }
    // check for satisfying assignment
    assert( Tas_QueIsEmpty(&p->pProp) );
    if ( Tas_QueIsEmpty(&p->pJust) )
        return 0;
    // quit using resource limits
    p->Pars.nJustThis = Abc_MaxInt( p->Pars.nJustThis, p->pJust.iTail - p->pJust.iHead );
    if ( Tas_ManCheckLimits( p ) )
        return 0;
    // remember the state before branching
    iPropHead = p->pProp.iHead;
    Tas_QueStore( &p->pJust, &iJustHead, &iJustTail );
    // find the decision variable
    if ( p->Pars.fUseActive )
        pVar = NULL, pDecVar = Tas_ManFindActive( p );
    else if ( p->Pars.fUseHighest )
//        pVar = NULL, pDecVar = Tas_ManDecideHighestFanin( p );
        pVar = Tas_ManDecideHighest( p );
    else if ( p->Pars.fUseLowest )
        pVar = Tas_ManDecideLowest( p );
    else if ( p->Pars.fUseMaxFF )
        pVar = Tas_ManDecideMaxFF( p );
    else assert( 0 );
    // chose decision variable using fanout count
    if ( pVar != NULL )
    {
        assert( Tas_VarIsJust( pVar ) );
        if ( Gia_ObjRefNum(p->pAig, Gia_ObjFanin0(pVar)) > Gia_ObjRefNum(p->pAig, Gia_ObjFanin1(pVar)) )
            pDecVar = Gia_Not(Gia_ObjChild0(pVar));
        else
            pDecVar = Gia_Not(Gia_ObjChild1(pVar));
//        pDecVar = Gia_NotCond( pDecVar, Gia_Regular(pDecVar)->fMark1 ^ !Gia_IsComplement(pDecVar) );
    }
    // decide on first fanin
    Tas_ManAssign( p, pDecVar, Level+1, NULL, NULL );
    if ( !(hLearn0 = Tas_ManSolve_rec( p, Level+1 )) )
        return 0;
    if ( pQue->pData[hLearn0] != Gia_Regular(pDecVar) )
        return hLearn0;
    Tas_ManCancelUntil( p, iPropHead );
    Tas_QueRestore( &p->pJust, iJustHead, iJustTail );
    // decide on second fanin
    Tas_ManAssign( p, Gia_Not(pDecVar), Level+1, NULL, NULL );
    if ( !(hLearn1 = Tas_ManSolve_rec( p, Level+1 )) )
        return 0;
    if ( pQue->pData[hLearn1] != Gia_Regular(pDecVar) )
        return hLearn1;
    hClause = Tas_ManResolve( p, Level, hLearn0, hLearn1 );
    Tas_ManCreateCls( p, hClause );
//    Tas_ManPrintClauseNew( p, Level, hClause );
//    if ( Level > Tas_ClauseDecLevel(p, hClause) )
//        p->Pars.nBTThisNc++;
    p->Pars.nBTThis++;
    return hClause;
}

/**Function*************************************************************

  Synopsis    [Looking for a satisfying assignment of the node.]

  Description [Assumes that each node has flag pObj->fMark0 set to 0.
  Returns 1 if unsatisfiable, 0 if satisfiable, and -1 if undecided.
  The node may be complemented. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tas_ManSolve( Tas_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pObj2 )
{
    int i, Entry, RetValue = 0;
    s_Counter2 = 0;
    Vec_IntClear( p->vModel );
    if ( pObj == Gia_ManConst0(p->pAig) || pObj2 == Gia_ManConst0(p->pAig) || pObj == Gia_Not(pObj2) )
        return 1;
    if ( pObj == Gia_ManConst1(p->pAig) && (pObj2 == NULL || pObj2 == Gia_ManConst1(p->pAig)) )
        return 0;
    assert( !p->pProp.iHead && !p->pProp.iTail );
    assert( !p->pJust.iHead && !p->pJust.iTail );
    assert( p->pClauses.iHead == 1 && p->pClauses.iTail == 1 );
    p->Pars.nBTThis = p->Pars.nJustThis = p->Pars.nBTThisNc = 0;
    Tas_ManAssign( p, pObj, 0, NULL, NULL );
    if ( pObj2 && !Tas_VarIsAssigned(Gia_Regular(pObj2)) )
        Tas_ManAssign( p, pObj2, 0, NULL, NULL );
    if ( !Tas_ManSolve_rec(p, 0) && !Tas_ManCheckLimits(p) )
        Tas_ManSaveModel( p, p->vModel );
    else
        RetValue = 1;
    Tas_ManCancelUntil( p, 0 );
    p->pJust.iHead = p->pJust.iTail = 0;
    p->pClauses.iHead = p->pClauses.iTail = 1;
    // clauses
    if ( p->nClauses > 0 )
    {
        p->pStore.iCur = 16;
        Vec_IntForEachEntry( p->vWatchLits, Entry, i )
            p->pWatches[Entry] = 0;
        Vec_IntClear( p->vWatchLits );
        p->nClauses = 0;
    }
    // activity
    Vec_IntForEachEntry( p->vActiveVars, Entry, i )
        p->pActivity[Entry] = 0.0;
    Vec_IntClear( p->vActiveVars );
    // statistics
    p->Pars.nBTTotal += p->Pars.nBTThis;
    p->Pars.nJustTotal = Abc_MaxInt( p->Pars.nJustTotal, p->Pars.nJustThis );
    if ( Tas_ManCheckLimits( p ) )
        RetValue = -1;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Looking for a satisfying assignment of the node.]

  Description [Assumes that each node has flag pObj->fMark0 set to 0.
  Returns 1 if unsatisfiable, 0 if satisfiable, and -1 if undecided.
  The node may be complemented. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tas_ManSolveArray( Tas_Man_t * p, Vec_Ptr_t * vObjs )
{
    Gia_Obj_t * pObj;
    int i, Entry, RetValue = 0;
    s_Counter2 = 0;
    s_Counter3 = 0;
    s_Counter4 = 0;
    Vec_IntClear( p->vModel );
    Vec_PtrForEachEntry( Gia_Obj_t *, vObjs, pObj, i )
        if ( pObj == Gia_ManConst0(p->pAig) )
            return 1;
    assert( !p->pProp.iHead && !p->pProp.iTail );
    assert( !p->pJust.iHead && !p->pJust.iTail );
    assert( p->pClauses.iHead == 1 && p->pClauses.iTail == 1 );
    p->Pars.nBTThis = p->Pars.nJustThis = p->Pars.nBTThisNc = 0;
    Vec_PtrForEachEntry( Gia_Obj_t *, vObjs, pObj, i )
        if ( pObj != Gia_ManConst1(p->pAig) && !Tas_VarIsAssigned(Gia_Regular(pObj)) )
            Tas_ManAssign( p, pObj, 0, NULL, NULL );
    if ( !Tas_ManSolve_rec(p, 0) && !Tas_ManCheckLimits(p) )
        Tas_ManSaveModel( p, p->vModel );
    else
        RetValue = 1;
    Tas_ManCancelUntil( p, 0 );
    p->pJust.iHead = p->pJust.iTail = 0;
    p->pClauses.iHead = p->pClauses.iTail = 1;
    // clauses
    if ( p->nClauses > 0 )
    {
        p->pStore.iCur = 16;
        Vec_IntForEachEntry( p->vWatchLits, Entry, i )
            p->pWatches[Entry] = 0;
        Vec_IntClear( p->vWatchLits );
        p->nClauses = 0;
    }
    // activity
    Vec_IntForEachEntry( p->vActiveVars, Entry, i )
        p->pActivity[Entry] = 0.0;
    Vec_IntClear( p->vActiveVars );
    // statistics
    p->Pars.nBTTotal += p->Pars.nBTThis;
    p->Pars.nJustTotal = Abc_MaxInt( p->Pars.nJustTotal, p->Pars.nJustThis );
    if ( Tas_ManCheckLimits( p ) )
        RetValue = -1;

//    printf( "%d ", Gia_ManObjNum(p->pAig) );
//    printf( "%d ", p->Pars.nBTThis );
//    printf( "%d ", p->Pars.nJustThis );
//    printf( "%d ", s_Counter2 );
//    printf( "%d ", s_Counter3 );
//    printf( "%d  ", s_Counter4 );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Prints statistics of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tas_ManSatPrintStats( Tas_Man_t * p )
{
    printf( "CO = %8d  ", Gia_ManCoNum(p->pAig) );
    printf( "AND = %8d  ", Gia_ManAndNum(p->pAig) );
    printf( "Conf = %6d  ", p->Pars.nBTLimit );
    printf( "JustMax = %5d  ", p->Pars.nJustLimit );
    printf( "\n" );
    printf( "Unsat calls %6d  (%6.2f %%)   Ave conf = %8.1f   ", 
        p->nSatUnsat, p->nSatTotal? 100.0*p->nSatUnsat/p->nSatTotal :0.0, p->nSatUnsat? 1.0*p->nConfUnsat/p->nSatUnsat :0.0 );
    ABC_PRTP( "Time", p->timeSatUnsat, p->timeTotal );
    printf( "Sat   calls %6d  (%6.2f %%)   Ave conf = %8.1f   ", 
        p->nSatSat,   p->nSatTotal? 100.0*p->nSatSat/p->nSatTotal :0.0, p->nSatSat? 1.0*p->nConfSat/p->nSatSat : 0.0 );
    ABC_PRTP( "Time", p->timeSatSat,   p->timeTotal );
    printf( "Undef calls %6d  (%6.2f %%)   Ave conf = %8.1f   ", 
        p->nSatUndec, p->nSatTotal? 100.0*p->nSatUndec/p->nSatTotal :0.0, p->nSatUndec? 1.0*p->nConfUndec/p->nSatUndec : 0.0 );
    ABC_PRTP( "Time", p->timeSatUndec, p->timeTotal );
    ABC_PRT( "Total time", p->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Procedure to test the new SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Tas_ManSolveMiterNc( Gia_Man_t * pAig, int nConfs, Vec_Str_t ** pvStatus, int fVerbose )
{
    extern void Gia_ManCollectTest( Gia_Man_t * pAig );
    extern void Cec_ManSatAddToStore( Vec_Int_t * vCexStore, Vec_Int_t * vCex, int Out );
    Tas_Man_t * p; 
    Vec_Int_t * vCex, * vVisit, * vCexStore;
    Vec_Str_t * vStatus;
    Gia_Obj_t * pRoot;//, * pRootCopy; 
//    Gia_Man_t * pAigCopy = Gia_ManDup( pAig ), * pAigTemp;

    int i, status;
    abctime clk, clkTotal = Abc_Clock();
    assert( Gia_ManRegNum(pAig) == 0 );
//    Gia_ManCollectTest( pAig );
    // prepare AIG
    Gia_ManCreateRefs( pAig );
    Gia_ManCleanMark0( pAig );
    Gia_ManCleanMark1( pAig );
    Gia_ManFillValue( pAig ); // maps nodes into trail ids
    Gia_ManCleanPhase( pAig ); // maps nodes into trail ids
    // create logic network
    p = Tas_ManAlloc( pAig, nConfs );
    p->pAig   = pAig;
    // create resulting data-structures
    vStatus   = Vec_StrAlloc( Gia_ManPoNum(pAig) );
    vCexStore = Vec_IntAlloc( 10000 );
    vVisit    = Vec_IntAlloc( 100 );
    vCex      = Tas_ReadModel( p );
    // solve for each output
    Gia_ManForEachCo( pAig, pRoot, i )
    {
//        printf( "%d=", i );

        Vec_IntClear( vCex );
        if ( Gia_ObjIsConst0(Gia_ObjFanin0(pRoot)) )
        {
            if ( Gia_ObjFaninC0(pRoot) )
            {
//                printf( "Constant 1 output of SRM!!!\n" );
                Cec_ManSatAddToStore( vCexStore, vCex, i ); // trivial counter-example
                Vec_StrPush( vStatus, 0 );
            }
            else
            {
//                printf( "Constant 0 output of SRM!!!\n" );
                Vec_StrPush( vStatus, 1 );
            }
            continue;
        } 
        clk = Abc_Clock();
//        p->Pars.fUseActive  = 1;
        p->Pars.fUseHighest = 1;
        p->Pars.fUseLowest  = 0;
        status = Tas_ManSolve( p, Gia_ObjChild0(pRoot), NULL );
//        printf( "\n" );
/*
        if ( status == -1 )
        {
            p->Pars.fUseHighest = 0;
            p->Pars.fUseLowest  = 1;
            status = Tas_ManSolve( p, Gia_ObjChild0(pRoot) );
        }
*/
        Vec_StrPush( vStatus, (char)status );
        if ( status == -1 )
        {
//            printf( "Unsolved %d.\n", i );

            p->nSatUndec++;
            p->nConfUndec += p->Pars.nBTThis;
            Cec_ManSatAddToStore( vCexStore, NULL, i ); // timeout
            p->timeSatUndec += Abc_Clock() - clk;
            continue;
        }

//        pRootCopy = Gia_ManCo( pAigCopy, i );
//        pRootCopy->iDiff0 = Gia_ObjId( pAigCopy, pRootCopy );
//        pRootCopy->fCompl0 = 0;

        if ( status == 1 )
        {
            p->nSatUnsat++;
            p->nConfUnsat += p->Pars.nBTThis;
            p->timeSatUnsat += Abc_Clock() - clk;
            continue;
        }
        p->nSatSat++;
        p->nConfSat += p->Pars.nBTThis;
//        Gia_SatVerifyPattern( pAig, pRoot, vCex, vVisit );
        Cec_ManSatAddToStore( vCexStore, vCex, i );
        p->timeSatSat += Abc_Clock() - clk;

//        printf( "%d ", Vec_IntSize(vCex) );
    }
//    pAigCopy = Gia_ManCleanup( pAigTemp = pAigCopy );
//    Gia_ManStop( pAigTemp );
//    Gia_DumpAiger( pAigCopy, "test", 0, 2 );
//    Gia_ManStop( pAigCopy );

    Vec_IntFree( vVisit );
    p->nSatTotal = Gia_ManPoNum(pAig);
    p->timeTotal = Abc_Clock() - clkTotal;
    if ( fVerbose )
        Tas_ManSatPrintStats( p );
//    printf( "RecCalls = %8d.  RecClause = %8d.  RecNonChro = %8d.\n", p->nRecCall, p->nRecClause, p->nRecNonChro );
    Tas_ManStop( p );
    *pvStatus = vStatus;

//    printf( "Total number of cex literals = %d. (Ave = %d)\n", 
//         Vec_IntSize(vCexStore)-2*p->nSatUndec-2*p->nSatSat, 
//        (Vec_IntSize(vCexStore)-2*p->nSatUndec-2*p->nSatSat)/p->nSatSat );
    return vCexStore;
}

/**Function*************************************************************

  Synopsis    [Packs patterns into array of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

*************************************`**********************************/
int Tas_StorePatternTry( Vec_Ptr_t * vInfo, Vec_Ptr_t * vPres, int iBit, int * pLits, int nLits )
{
    unsigned * pInfo, * pPres;
    int i;
    for ( i = 0; i < nLits; i++ )
    {
        pInfo = (unsigned *)Vec_PtrEntry(vInfo, Abc_Lit2Var(pLits[i]));
        pPres = (unsigned *)Vec_PtrEntry(vPres, Abc_Lit2Var(pLits[i]));
        if ( Abc_InfoHasBit( pPres, iBit ) && 
             Abc_InfoHasBit( pInfo, iBit ) == Abc_LitIsCompl(pLits[i]) )
             return 0;
    }
    for ( i = 0; i < nLits; i++ )
    {
        pInfo = (unsigned *)Vec_PtrEntry(vInfo, Abc_Lit2Var(pLits[i]));
        pPres = (unsigned *)Vec_PtrEntry(vPres, Abc_Lit2Var(pLits[i]));
        Abc_InfoSetBit( pPres, iBit );
        if ( Abc_InfoHasBit( pInfo, iBit ) == Abc_LitIsCompl(pLits[i]) )
            Abc_InfoXorBit( pInfo, iBit );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Procedure to test the new SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tas_StorePattern( Vec_Ptr_t * vSimInfo, Vec_Ptr_t * vPres, Vec_Int_t * vCex )
{
    int k;
    for ( k = 1; k < 32; k++ )
        if ( Tas_StorePatternTry( vSimInfo, vPres, k, (int *)Vec_IntArray(vCex), Vec_IntSize(vCex) ) )
            break;
    return (int)(k < 32);
}

/**Function*************************************************************

  Synopsis    [Procedure to test the new SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tas_ManSolveMiterNc2( Gia_Man_t * pAig, int nConfs, Gia_Man_t * pAigOld, Vec_Ptr_t * vOldRoots, Vec_Ptr_t * vSimInfo )
{
    int nPatMax = 1000;
    int fVerbose = 1;
    extern void Gia_ManCollectTest( Gia_Man_t * pAig );
    extern void Cec_ManSatAddToStore( Vec_Int_t * vCexStore, Vec_Int_t * vCex, int Out );
    Tas_Man_t * p; 
    Vec_Ptr_t * vPres;
    Vec_Int_t * vCex, * vVisit, * vCexStore;
    Vec_Str_t * vStatus;
    Gia_Obj_t * pRoot, * pOldRoot; 
    int i, status;
    abctime clk, clkTotal = Abc_Clock();
    int Tried = 0, Stored = 0, Step = Gia_ManCoNum(pAig) / nPatMax;
    assert( Gia_ManRegNum(pAig) == 0 );
//    Gia_ManCollectTest( pAig );
    // prepare AIG
    Gia_ManCreateRefs( pAig );
    Gia_ManCleanMark0( pAig );
    Gia_ManCleanMark1( pAig );
    Gia_ManFillValue( pAig ); // maps nodes into trail ids
    Gia_ManCleanPhase( pAig ); // maps nodes into trail ids
    // create logic network
    p = Tas_ManAlloc( pAig, nConfs );
    p->pAig   = pAig;
    // create resulting data-structures
    vStatus   = Vec_StrAlloc( Gia_ManPoNum(pAig) );
    vCexStore = Vec_IntAlloc( 10000 );
    vVisit    = Vec_IntAlloc( 100 );
    vCex      = Tas_ReadModel( p );
    // solve for each output
    vPres = Vec_PtrAllocSimInfo( Gia_ManCiNum(pAig), 1 );
    Vec_PtrCleanSimInfo( vPres, 0, 1 );
    
    Gia_ManForEachCo( pAig, pRoot, i )
    {
        assert( !Gia_ObjIsConst0(Gia_ObjFanin0(pRoot)) );
        Vec_IntClear( vCex );
        clk = Abc_Clock();
        p->Pars.fUseHighest = 1;
        p->Pars.fUseLowest  = 0;
        status = Tas_ManSolve( p, Gia_ObjChild0(pRoot), NULL );
        Vec_StrPush( vStatus, (char)status );
        if ( status == -1 )
        {
            p->nSatUndec++;
            p->nConfUndec += p->Pars.nBTThis;
//            Cec_ManSatAddToStore( vCexStore, NULL, i ); // timeout
            p->timeSatUndec += Abc_Clock() - clk;

            i += Step;
            continue;
        }
        if ( status == 1 )
        {
            p->nSatUnsat++;
            p->nConfUnsat += p->Pars.nBTThis;
            p->timeSatUnsat += Abc_Clock() - clk;
            // record proved
            pOldRoot = (Gia_Obj_t *)Vec_PtrEntry( vOldRoots, i );
            assert( !Gia_ObjProved( pAigOld, Gia_ObjId(pAigOld, pOldRoot) ) );
            Gia_ObjSetProved( pAigOld, Gia_ObjId(pAigOld, pOldRoot) );

            i += Step;
            continue;
        }
        p->nSatSat++;
        p->nConfSat += p->Pars.nBTThis;
//        Gia_SatVerifyPattern( pAig, pRoot, vCex, vVisit );
//        Cec_ManSatAddToStore( vCexStore, vCex, i );

        // save pattern
        Tried++;
        Stored += Tas_StorePattern( vSimInfo, vPres, vCex );
        p->timeSatSat += Abc_Clock() - clk;
        i += Step;
    }
    printf( "Tried = %d  Stored = %d\n", Tried, Stored );
    Vec_IntFree( vVisit );
    p->nSatTotal = Gia_ManPoNum(pAig);
    p->timeTotal = Abc_Clock() - clkTotal;
    if ( fVerbose )
        Tas_ManSatPrintStats( p );
    Tas_ManStop( p );
    Vec_PtrFree( vPres );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

