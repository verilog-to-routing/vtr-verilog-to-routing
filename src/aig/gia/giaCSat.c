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

typedef struct Cbs_Par_t_ Cbs_Par_t;
struct Cbs_Par_t_
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
    // decision heuristics
    int           fUseHighest;  // use node with the highest ID
    int           fUseLowest;   // use node with the highest ID
    int           fUseMaxFF;    // use node with the largest fanin fanout
    // other
    int           fVerbose;
};

typedef struct Cbs_Que_t_ Cbs_Que_t;
struct Cbs_Que_t_
{
    int           iHead;        // beginning of the queue
    int           iTail;        // end of the queue
    int           nSize;        // allocated size
    Gia_Obj_t **  pData;        // nodes stored in the queue
};
 
//typedef struct Cbs_Man_t_ Cbs_Man_t;
struct Cbs_Man_t_
{
    Cbs_Par_t     Pars;         // parameters
    Gia_Man_t *   pAig;         // AIG manager
    Cbs_Que_t     pProp;        // propagation queue
    Cbs_Que_t     pJust;        // justification queue
    Cbs_Que_t     pClauses;     // clause queue
    Gia_Obj_t **  pIter;        // iterator through clause vars
    Vec_Int_t *   vLevReas;     // levels and decisions
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
    // runtime stats
    abctime       timeSatUnsat; // unsat
    abctime       timeSatSat;   // sat
    abctime       timeSatUndec; // undecided
    abctime       timeTotal;    // total runtime
};

static inline int   Cbs_VarIsAssigned( Gia_Obj_t * pVar )      { return pVar->fMark0;                        }
static inline void  Cbs_VarAssign( Gia_Obj_t * pVar )          { assert(!pVar->fMark0); pVar->fMark0 = 1;    }
static inline void  Cbs_VarUnassign( Gia_Obj_t * pVar )        { assert(pVar->fMark0);  pVar->fMark0 = 0; pVar->fMark1 = 0; pVar->Value = ~0; }
static inline int   Cbs_VarValue( Gia_Obj_t * pVar )           { assert(pVar->fMark0);  return pVar->fMark1; }
static inline void  Cbs_VarSetValue( Gia_Obj_t * pVar, int v ) { assert(pVar->fMark0);  pVar->fMark1 = v;    }
static inline int   Cbs_VarIsJust( Gia_Obj_t * pVar )          { return Gia_ObjIsAnd(pVar) && !Cbs_VarIsAssigned(Gia_ObjFanin0(pVar)) && !Cbs_VarIsAssigned(Gia_ObjFanin1(pVar)); } 
static inline int   Cbs_VarFanin0Value( Gia_Obj_t * pVar )     { return !Cbs_VarIsAssigned(Gia_ObjFanin0(pVar)) ? 2 : (Cbs_VarValue(Gia_ObjFanin0(pVar)) ^ Gia_ObjFaninC0(pVar)); }
static inline int   Cbs_VarFanin1Value( Gia_Obj_t * pVar )     { return !Cbs_VarIsAssigned(Gia_ObjFanin1(pVar)) ? 2 : (Cbs_VarValue(Gia_ObjFanin1(pVar)) ^ Gia_ObjFaninC1(pVar)); }

static inline int         Cbs_VarDecLevel( Cbs_Man_t * p, Gia_Obj_t * pVar )  { assert( pVar->Value != ~0 ); return Vec_IntEntry(p->vLevReas, 3*pVar->Value);          }
static inline Gia_Obj_t * Cbs_VarReason0( Cbs_Man_t * p, Gia_Obj_t * pVar )   { assert( pVar->Value != ~0 ); return pVar + Vec_IntEntry(p->vLevReas, 3*pVar->Value+1); }
static inline Gia_Obj_t * Cbs_VarReason1( Cbs_Man_t * p, Gia_Obj_t * pVar )   { assert( pVar->Value != ~0 ); return pVar + Vec_IntEntry(p->vLevReas, 3*pVar->Value+2); }
static inline int         Cbs_ClauseDecLevel( Cbs_Man_t * p, int hClause )    { return Cbs_VarDecLevel( p, p->pClauses.pData[hClause] );                               }

#define Cbs_QueForEachEntry( Que, pObj, i )                         \
    for ( i = (Que).iHead; (i < (Que).iTail) && ((pObj) = (Que).pData[i]); i++ )

#define Cbs_ClauseForEachVar( p, hClause, pObj )                    \
    for ( (p)->pIter = (p)->pClauses.pData + hClause; (pObj = *pIter); (p)->pIter++ )
#define Cbs_ClauseForEachVar1( p, hClause, pObj )                   \
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
void Cbs_SetDefaultParams( Cbs_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Cbs_Par_t) );
    pPars->nBTLimit    =  1000;   // limit on the number of conflicts
    pPars->nJustLimit  =   100;   // limit on the size of justification queue
    pPars->fUseHighest =     1;   // use node with the highest ID
    pPars->fUseLowest  =     0;   // use node with the highest ID
    pPars->fUseMaxFF   =     0;   // use node with the largest fanin fanout
    pPars->fVerbose    =     1;   // print detailed statistics
}
void Cbs_ManSetConflictNum( Cbs_Man_t * p, int Num )
{
    p->Pars.nBTLimit = Num;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cbs_Man_t * Cbs_ManAlloc( Gia_Man_t * pGia )
{
    Cbs_Man_t * p;
    p = ABC_CALLOC( Cbs_Man_t, 1 );
    p->pProp.nSize = p->pJust.nSize = p->pClauses.nSize = 10000;
    p->pProp.pData = ABC_ALLOC( Gia_Obj_t *, p->pProp.nSize );
    p->pJust.pData = ABC_ALLOC( Gia_Obj_t *, p->pJust.nSize );
    p->pClauses.pData = ABC_ALLOC( Gia_Obj_t *, p->pClauses.nSize );
    p->pClauses.iHead = p->pClauses.iTail = 1;
    p->vModel   = Vec_IntAlloc( 1000 );
    p->vLevReas = Vec_IntAlloc( 1000 );
    p->vTemp    = Vec_PtrAlloc( 1000 );
    p->pAig     = pGia;
    Cbs_SetDefaultParams( &p->Pars );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cbs_ManStop( Cbs_Man_t * p )
{
    Vec_IntFree( p->vLevReas );
    Vec_IntFree( p->vModel );
    Vec_PtrFree( p->vTemp );
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
Vec_Int_t * Cbs_ReadModel( Cbs_Man_t * p )
{
    return p->vModel;
}




/**Function*************************************************************

  Synopsis    [Returns 1 if the solver is out of limits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs_ManCheckLimits( Cbs_Man_t * p )
{
    return p->Pars.nJustThis > p->Pars.nJustLimit || p->Pars.nBTThis > p->Pars.nBTLimit;
}

/**Function*************************************************************

  Synopsis    [Saves the satisfying assignment as an array of literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs_ManSaveModel( Cbs_Man_t * p, Vec_Int_t * vCex )
{
    Gia_Obj_t * pVar;
    int i;
    Vec_IntClear( vCex );
    p->pProp.iHead = 0;
    Cbs_QueForEachEntry( p->pProp, pVar, i )
        if ( Gia_ObjIsCi(pVar) )
//            Vec_IntPush( vCex, Abc_Var2Lit(Gia_ObjId(p->pAig,pVar), !Cbs_VarValue(pVar)) );
            Vec_IntPush( vCex, Abc_Var2Lit(Gia_ObjCioId(pVar), !Cbs_VarValue(pVar)) );
} 
static inline void Cbs_ManSaveModelAll( Cbs_Man_t * p, Vec_Int_t * vCex )
{
    Gia_Obj_t * pVar;
    int i;
    Vec_IntClear( vCex );
    p->pProp.iHead = 0;
    Cbs_QueForEachEntry( p->pProp, pVar, i )
        Vec_IntPush( vCex, Abc_Var2Lit(Gia_ObjId(p->pAig,pVar), !Cbs_VarValue(pVar)) );
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs_QueIsEmpty( Cbs_Que_t * p )
{
    return p->iHead == p->iTail;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs_QuePush( Cbs_Que_t * p, Gia_Obj_t * pObj )
{
    assert( !Gia_IsComplement(pObj) );
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
static inline int Cbs_QueHasNode( Cbs_Que_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pTemp;
    int i;
    Cbs_QueForEachEntry( *p, pTemp, i )
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
static inline void Cbs_QueStore( Cbs_Que_t * p, int * piHeadOld, int * piTailOld )
{
    int i;
    *piHeadOld = p->iHead;
    *piTailOld = p->iTail;
    for ( i = *piHeadOld; i < *piTailOld; i++ )
        Cbs_QuePush( p, p->pData[i] );
    p->iHead = *piTailOld;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs_QueRestore( Cbs_Que_t * p, int iHeadOld, int iTailOld )
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
static inline int Cbs_QueFinish( Cbs_Que_t * p )
{
    int iHeadOld = p->iHead;
    assert( p->iHead < p->iTail );
    Cbs_QuePush( p, NULL );
    p->iHead = p->iTail;
    return iHeadOld;
}


/**Function*************************************************************

  Synopsis    [Max number of fanins fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs_VarFaninFanoutMax( Cbs_Man_t * p, Gia_Obj_t * pObj )
{
    int Count0, Count1;
    assert( !Gia_IsComplement(pObj) );
    assert( Gia_ObjIsAnd(pObj) );
    Count0 = Gia_ObjRefNum( p->pAig, Gia_ObjFanin0(pObj) );
    Count1 = Gia_ObjRefNum( p->pAig, Gia_ObjFanin1(pObj) );
    return Abc_MaxInt( Count0, Count1 );
}

/**Function*************************************************************

  Synopsis    [Find variable with the highest ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Cbs_ManDecideHighest( Cbs_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMax = NULL;
    int i;
    Cbs_QueForEachEntry( p->pJust, pObj, i )
        if ( pObjMax == NULL || pObjMax < pObj )
            pObjMax = pObj;
    return pObjMax;
}

/**Function*************************************************************

  Synopsis    [Find variable with the lowest ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Cbs_ManDecideLowest( Cbs_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMin = NULL;
    int i;
    Cbs_QueForEachEntry( p->pJust, pObj, i )
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
static inline Gia_Obj_t * Cbs_ManDecideMaxFF( Cbs_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMax = NULL;
    int i, iMaxFF = 0, iCurFF;
    assert( p->pAig->pRefs != NULL );
    Cbs_QueForEachEntry( p->pJust, pObj, i )
    {
        iCurFF = Cbs_VarFaninFanoutMax( p, pObj );
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
static inline void Cbs_ManCancelUntil( Cbs_Man_t * p, int iBound )
{
    Gia_Obj_t * pVar;
    int i;
    assert( iBound <= p->pProp.iTail );
    p->pProp.iHead = iBound;
    Cbs_QueForEachEntry( p->pProp, pVar, i )
        Cbs_VarUnassign( pVar );
    p->pProp.iTail = iBound;
    Vec_IntShrink( p->vLevReas, 3*iBound );
}

int s_Counter = 0;

/**Function*************************************************************

  Synopsis    [Assigns the variables a value.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs_ManAssign( Cbs_Man_t * p, Gia_Obj_t * pObj, int Level, Gia_Obj_t * pRes0, Gia_Obj_t * pRes1 )
{
    Gia_Obj_t * pObjR = Gia_Regular(pObj);
    assert( Gia_ObjIsCand(pObjR) );
    assert( !Cbs_VarIsAssigned(pObjR) );
    Cbs_VarAssign( pObjR );
    Cbs_VarSetValue( pObjR, !Gia_IsComplement(pObj) );
    assert( pObjR->Value == ~0 );
    pObjR->Value = p->pProp.iTail;
    Cbs_QuePush( &p->pProp, pObjR );
    Vec_IntPush( p->vLevReas, Level );
    Vec_IntPush( p->vLevReas, pRes0 ? pRes0-pObjR : 0 );
    Vec_IntPush( p->vLevReas, pRes1 ? pRes1-pObjR : 0 );
    assert( Vec_IntSize(p->vLevReas) == 3 * p->pProp.iTail );
//    s_Counter++;
//    s_Counter = Abc_MaxIntInt( s_Counter, Vec_IntSize(p->vLevReas)/3 );
}


/**Function*************************************************************

  Synopsis    [Returns clause size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs_ManClauseSize( Cbs_Man_t * p, int hClause )
{
    Cbs_Que_t * pQue = &(p->pClauses);
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
static inline void Cbs_ManPrintClause( Cbs_Man_t * p, int Level, int hClause )
{
    Cbs_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pObj;
    int i;
    assert( Cbs_QueIsEmpty( pQue ) );
    printf( "Level %2d : ", Level );
    for ( i = hClause; (pObj = pQue->pData[i]); i++ )
        printf( "%d=%d(%d) ", Gia_ObjId(p->pAig, pObj), Cbs_VarValue(pObj), Cbs_VarDecLevel(p, pObj) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints conflict clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs_ManPrintClauseNew( Cbs_Man_t * p, int Level, int hClause )
{
    Cbs_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pObj;
    int i;
    assert( Cbs_QueIsEmpty( pQue ) );
    printf( "Level %2d : ", Level );
    for ( i = hClause; (pObj = pQue->pData[i]); i++ )
        printf( "%c%d ", Cbs_VarValue(pObj)? '+':'-', Gia_ObjId(p->pAig, pObj) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Returns conflict clause.]

  Description [Performs conflict analysis.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs_ManDeriveReason( Cbs_Man_t * p, int Level )
{
    Cbs_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pObj, * pReason;
    int i, k, iLitLevel;
    assert( pQue->pData[pQue->iHead] == NULL );
    assert( pQue->iHead + 1 < pQue->iTail );
/*
    for ( i = pQue->iHead + 1; i < pQue->iTail; i++ )
    {
        pObj = pQue->pData[i];
        assert( pObj->fMark0 == 1 );
    }
*/
    // compact literals
    Vec_PtrClear( p->vTemp );
    for ( i = k = pQue->iHead + 1; i < pQue->iTail; i++ )
    {
        pObj = pQue->pData[i];
        if ( !pObj->fMark0 ) // unassigned - seen again
            continue;
        // assigned - seen first time
        pObj->fMark0 = 0;
        Vec_PtrPush( p->vTemp, pObj );
        // check decision level
        iLitLevel = Cbs_VarDecLevel( p, pObj );
        if ( iLitLevel < Level )
        {
            pQue->pData[k++] = pObj;
            continue;
        }
        assert( iLitLevel == Level );
        pReason = Cbs_VarReason0( p, pObj );
        if ( pReason == pObj ) // no reason
        {
            //assert( pQue->pData[pQue->iHead] == NULL );
            pQue->pData[pQue->iHead] = pObj;
            continue;
        }
        Cbs_QuePush( pQue, pReason );
        pReason = Cbs_VarReason1( p, pObj );
        if ( pReason != pObj ) // second reason
            Cbs_QuePush( pQue, pReason );
    }
    assert( pQue->pData[pQue->iHead] != NULL );
    pQue->iTail = k;
    // clear the marks
    Vec_PtrForEachEntry( Gia_Obj_t *, p->vTemp, pObj, i )
        pObj->fMark0 = 1;
}

/**Function*************************************************************

  Synopsis    [Returns conflict clause.]

  Description [Performs conflict analysis.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs_ManAnalyze( Cbs_Man_t * p, int Level, Gia_Obj_t * pVar, Gia_Obj_t * pFan0, Gia_Obj_t * pFan1 )
{
    Cbs_Que_t * pQue = &(p->pClauses);
    assert( Cbs_VarIsAssigned(pVar) );
    assert( Cbs_VarIsAssigned(pFan0) );
    assert( pFan1 == NULL || Cbs_VarIsAssigned(pFan1) );
    assert( Cbs_QueIsEmpty( pQue ) );
    Cbs_QuePush( pQue, NULL );
    Cbs_QuePush( pQue, pVar );
    Cbs_QuePush( pQue, pFan0 );
    if ( pFan1 )
        Cbs_QuePush( pQue, pFan1 );
    Cbs_ManDeriveReason( p, Level );
    return Cbs_QueFinish( pQue );
}


/**Function*************************************************************

  Synopsis    [Performs resolution of two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs_ManResolve( Cbs_Man_t * p, int Level, int hClause0, int hClause1 )
{
    Cbs_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pObj;
    int i, LevelMax = -1, LevelCur;
    assert( pQue->pData[hClause0] != NULL );
    assert( pQue->pData[hClause0] == pQue->pData[hClause1] );
/*
    for ( i = hClause0 + 1; (pObj = pQue->pData[i]); i++ )
        assert( pObj->fMark0 == 1 );
    for ( i = hClause1 + 1; (pObj = pQue->pData[i]); i++ )
        assert( pObj->fMark0 == 1 );
*/
    assert( Cbs_QueIsEmpty( pQue ) );
    Cbs_QuePush( pQue, NULL );
    for ( i = hClause0 + 1; (pObj = pQue->pData[i]); i++ )
    {
        if ( !pObj->fMark0 ) // unassigned - seen again
            continue;
        // assigned - seen first time
        pObj->fMark0 = 0;
        Cbs_QuePush( pQue, pObj );
        LevelCur = Cbs_VarDecLevel( p, pObj );
        if ( LevelMax < LevelCur )
            LevelMax = LevelCur;
    }
    for ( i = hClause1 + 1; (pObj = pQue->pData[i]); i++ )
    {
        if ( !pObj->fMark0 ) // unassigned - seen again
            continue;
        // assigned - seen first time
        pObj->fMark0 = 0;
        Cbs_QuePush( pQue, pObj );
        LevelCur = Cbs_VarDecLevel( p, pObj );
        if ( LevelMax < LevelCur )
            LevelMax = LevelCur;
    }
    for ( i = pQue->iHead + 1; i < pQue->iTail; i++ )
        pQue->pData[i]->fMark0 = 1;
    Cbs_ManDeriveReason( p, LevelMax );
    return Cbs_QueFinish( pQue );
}

/**Function*************************************************************

  Synopsis    [Propagates a variable.]

  Description [Returns clause handle if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs_ManPropagateOne( Cbs_Man_t * p, Gia_Obj_t * pVar, int Level )
{
    int Value0, Value1;
    assert( !Gia_IsComplement(pVar) );
    assert( Cbs_VarIsAssigned(pVar) );
    if ( Gia_ObjIsCi(pVar) )
        return 0;
    assert( Gia_ObjIsAnd(pVar) );
    Value0 = Cbs_VarFanin0Value(pVar);
    Value1 = Cbs_VarFanin1Value(pVar);
    if ( Cbs_VarValue(pVar) )
    { // value is 1
        if ( Value0 == 0 || Value1 == 0 ) // one is 0
        {
            if ( Value0 == 0 && Value1 != 0 )
                return Cbs_ManAnalyze( p, Level, pVar, Gia_ObjFanin0(pVar), NULL );
            if ( Value0 != 0 && Value1 == 0 )
                return Cbs_ManAnalyze( p, Level, pVar, Gia_ObjFanin1(pVar), NULL );
            assert( Value0 == 0 && Value1 == 0 );
            return Cbs_ManAnalyze( p, Level, pVar, Gia_ObjFanin0(pVar), Gia_ObjFanin1(pVar) );
        }
        if ( Value0 == 2 ) // first is unassigned
            Cbs_ManAssign( p, Gia_ObjChild0(pVar), Level, pVar, NULL );
        if ( Value1 == 2 ) // first is unassigned
            Cbs_ManAssign( p, Gia_ObjChild1(pVar), Level, pVar, NULL );
        return 0;
    }
    // value is 0
    if ( Value0 == 0 || Value1 == 0 ) // one is 0
        return 0;
    if ( Value0 == 1 && Value1 == 1 ) // both are 1
        return Cbs_ManAnalyze( p, Level, pVar, Gia_ObjFanin0(pVar), Gia_ObjFanin1(pVar) );
    if ( Value0 == 1 || Value1 == 1 ) // one is 1 
    {
        if ( Value0 == 2 ) // first is unassigned
            Cbs_ManAssign( p, Gia_Not(Gia_ObjChild0(pVar)), Level, pVar, Gia_ObjFanin1(pVar) );
        if ( Value1 == 2 ) // second is unassigned
            Cbs_ManAssign( p, Gia_Not(Gia_ObjChild1(pVar)), Level, pVar, Gia_ObjFanin0(pVar) );
        return 0;
    }
    assert( Cbs_VarIsJust(pVar) );
    assert( !Cbs_QueHasNode( &p->pJust, pVar ) );
    Cbs_QuePush( &p->pJust, pVar );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Propagates a variable.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs_ManPropagateTwo( Cbs_Man_t * p, Gia_Obj_t * pVar, int Level )
{
    int Value0, Value1;
    assert( !Gia_IsComplement(pVar) );
    assert( Gia_ObjIsAnd(pVar) );
    assert( Cbs_VarIsAssigned(pVar) );
    assert( !Cbs_VarValue(pVar) );
    Value0 = Cbs_VarFanin0Value(pVar);
    Value1 = Cbs_VarFanin1Value(pVar);
    // value is 0
    if ( Value0 == 0 || Value1 == 0 ) // one is 0
        return 0;
    if ( Value0 == 1 && Value1 == 1 ) // both are 1
        return Cbs_ManAnalyze( p, Level, pVar, Gia_ObjFanin0(pVar), Gia_ObjFanin1(pVar) );
    assert( Value0 == 1 || Value1 == 1 );
    if ( Value0 == 2 ) // first is unassigned
        Cbs_ManAssign( p, Gia_Not(Gia_ObjChild0(pVar)), Level, pVar, Gia_ObjFanin1(pVar) );
    if ( Value1 == 2 ) // first is unassigned
        Cbs_ManAssign( p, Gia_Not(Gia_ObjChild1(pVar)), Level, pVar, Gia_ObjFanin0(pVar) );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Propagates all variables.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cbs_ManPropagate( Cbs_Man_t * p, int Level )
{
    int hClause;
    Gia_Obj_t * pVar;
    int i, k;
    while ( 1 )
    {
        Cbs_QueForEachEntry( p->pProp, pVar, i )
        {
            if ( (hClause = Cbs_ManPropagateOne( p, pVar, Level )) )
                return hClause;
        }
        p->pProp.iHead = p->pProp.iTail;
        k = p->pJust.iHead;
        Cbs_QueForEachEntry( p->pJust, pVar, i )
        {
            if ( Cbs_VarIsJust( pVar ) )
                p->pJust.pData[k++] = pVar;
            else if ( (hClause = Cbs_ManPropagateTwo( p, pVar, Level )) )
                return hClause;
        }
        if ( k == p->pJust.iTail )
            break;
        p->pJust.iTail = k;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Solve the problem recursively.]

  Description [Returns learnt clause if unsat, NULL if sat or undecided.]
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
int Cbs_ManSolve_rec( Cbs_Man_t * p, int Level )
{ 
    Cbs_Que_t * pQue = &(p->pClauses);
    Gia_Obj_t * pVar = NULL, * pDecVar;
    int hClause, hLearn0, hLearn1;
    int iPropHead, iJustHead, iJustTail;
    // propagate assignments
    assert( !Cbs_QueIsEmpty(&p->pProp) );
    if ( (hClause = Cbs_ManPropagate( p, Level )) )
        return hClause;
    // check for satisfying assignment
    assert( Cbs_QueIsEmpty(&p->pProp) );
    if ( Cbs_QueIsEmpty(&p->pJust) )
        return 0;
    // quit using resource limits
    p->Pars.nJustThis = Abc_MaxInt( p->Pars.nJustThis, p->pJust.iTail - p->pJust.iHead );
    if ( Cbs_ManCheckLimits( p ) )
        return 0;
    // remember the state before branching
    iPropHead = p->pProp.iHead;
    Cbs_QueStore( &p->pJust, &iJustHead, &iJustTail );
    // find the decision variable
    if ( p->Pars.fUseHighest )
        pVar = Cbs_ManDecideHighest( p );
    else if ( p->Pars.fUseLowest )
        pVar = Cbs_ManDecideLowest( p );
    else if ( p->Pars.fUseMaxFF )
        pVar = Cbs_ManDecideMaxFF( p );
    else assert( 0 );
    assert( Cbs_VarIsJust( pVar ) );
    // chose decision variable using fanout count
    if ( Gia_ObjRefNum(p->pAig, Gia_ObjFanin0(pVar)) > Gia_ObjRefNum(p->pAig, Gia_ObjFanin1(pVar)) )
        pDecVar = Gia_Not(Gia_ObjChild0(pVar));
    else
        pDecVar = Gia_Not(Gia_ObjChild1(pVar));
//    pDecVar = Gia_NotCond( Gia_Regular(pDecVar), Gia_Regular(pDecVar)->fPhase );
//    pDecVar = Gia_Not(pDecVar);
    // decide on first fanin
    Cbs_ManAssign( p, pDecVar, Level+1, NULL, NULL );
    if ( !(hLearn0 = Cbs_ManSolve_rec( p, Level+1 )) )
        return 0;
    if ( pQue->pData[hLearn0] != Gia_Regular(pDecVar) )
        return hLearn0;
    Cbs_ManCancelUntil( p, iPropHead );
    Cbs_QueRestore( &p->pJust, iJustHead, iJustTail );
    // decide on second fanin
    Cbs_ManAssign( p, Gia_Not(pDecVar), Level+1, NULL, NULL );
    if ( !(hLearn1 = Cbs_ManSolve_rec( p, Level+1 )) )
        return 0;
    if ( pQue->pData[hLearn1] != Gia_Regular(pDecVar) )
        return hLearn1;
    hClause = Cbs_ManResolve( p, Level, hLearn0, hLearn1 );
//    Cbs_ManPrintClauseNew( p, Level, hClause );
//    if ( Level > Cbs_ClauseDecLevel(p, hClause) )
//        p->Pars.nBTThisNc++;
    p->Pars.nBTThis++;
    return hClause;
}

/**Function*************************************************************

  Synopsis    [Looking for a satisfying assignment of the node.]

  Description [Assumes that each node has flag pObj->fMark0 set to 0.
  Returns 1 if unsatisfiable, 0 if satisfiable, and -1 if undecided.
  The node may be complemented. ]
               
  SideEffects [The two procedures differ in the CEX format.]

  SeeAlso     []

***********************************************************************/
int Cbs_ManSolve( Cbs_Man_t * p, Gia_Obj_t * pObj )
{
    int RetValue = 0;
    s_Counter = 0;
    assert( !p->pProp.iHead && !p->pProp.iTail );
    assert( !p->pJust.iHead && !p->pJust.iTail );
    assert( p->pClauses.iHead == 1 && p->pClauses.iTail == 1 );
    p->Pars.nBTThis = p->Pars.nJustThis = p->Pars.nBTThisNc = 0;
    Cbs_ManAssign( p, pObj, 0, NULL, NULL );
    if ( !Cbs_ManSolve_rec(p, 0) && !Cbs_ManCheckLimits(p) )
        Cbs_ManSaveModel( p, p->vModel );
    else
        RetValue = 1;
    Cbs_ManCancelUntil( p, 0 );
    p->pJust.iHead = p->pJust.iTail = 0;
    p->pClauses.iHead = p->pClauses.iTail = 1;
    p->Pars.nBTTotal += p->Pars.nBTThis;
    p->Pars.nJustTotal = Abc_MaxInt( p->Pars.nJustTotal, p->Pars.nJustThis );
    if ( Cbs_ManCheckLimits( p ) )
        RetValue = -1;
//    printf( "%d ", s_Counter );
    return RetValue;
}
int Cbs_ManSolve2( Cbs_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pObj2 )
{
    int RetValue = 0;
    s_Counter = 0;
    assert( !p->pProp.iHead && !p->pProp.iTail );
    assert( !p->pJust.iHead && !p->pJust.iTail );
    assert( p->pClauses.iHead == 1 && p->pClauses.iTail == 1 );
    p->Pars.nBTThis = p->Pars.nJustThis = p->Pars.nBTThisNc = 0;
    Cbs_ManAssign( p, pObj, 0, NULL, NULL );
    if ( pObj2 ) 
    Cbs_ManAssign( p, pObj2, 0, NULL, NULL );
    if ( !Cbs_ManSolve_rec(p, 0) && !Cbs_ManCheckLimits(p) )
        Cbs_ManSaveModelAll( p, p->vModel );
    else
        RetValue = 1;
    Cbs_ManCancelUntil( p, 0 );
    p->pJust.iHead = p->pJust.iTail = 0;
    p->pClauses.iHead = p->pClauses.iTail = 1;
    p->Pars.nBTTotal += p->Pars.nBTThis;
    p->Pars.nJustTotal = Abc_MaxInt( p->Pars.nJustTotal, p->Pars.nJustThis );
    if ( Cbs_ManCheckLimits( p ) )
        RetValue = -1;
//    printf( "%d ", s_Counter );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Prints statistics of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cbs_ManSatPrintStats( Cbs_Man_t * p )
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
Vec_Int_t * Cbs_ManSolveMiterNc( Gia_Man_t * pAig, int nConfs, Vec_Str_t ** pvStatus, int fVerbose )
{
    extern void Gia_ManCollectTest( Gia_Man_t * pAig );
    extern void Cec_ManSatAddToStore( Vec_Int_t * vCexStore, Vec_Int_t * vCex, int Out );
    Cbs_Man_t * p; 
    Vec_Int_t * vCex, * vVisit, * vCexStore;
    Vec_Str_t * vStatus;
    Gia_Obj_t * pRoot; 
    int i, status;
    abctime clk, clkTotal = Abc_Clock();
    assert( Gia_ManRegNum(pAig) == 0 );
//    Gia_ManCollectTest( pAig );
    // prepare AIG
    Gia_ManCreateRefs( pAig );
    Gia_ManCleanMark0( pAig );
    Gia_ManCleanMark1( pAig );
    Gia_ManFillValue( pAig ); // maps nodes into trail ids
    Gia_ManSetPhase( pAig ); // maps nodes into trail ids
    // create logic network
    p = Cbs_ManAlloc( pAig );
    p->Pars.nBTLimit = nConfs;
    // create resulting data-structures
    vStatus   = Vec_StrAlloc( Gia_ManPoNum(pAig) );
    vCexStore = Vec_IntAlloc( 10000 );
    vVisit    = Vec_IntAlloc( 100 );
    vCex      = Cbs_ReadModel( p );
    // solve for each output
    Gia_ManForEachCo( pAig, pRoot, i )
    {
//        printf( "\n" );

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
        p->Pars.fUseHighest = 1;
        p->Pars.fUseLowest  = 0;
        status = Cbs_ManSolve( p, Gia_ObjChild0(pRoot) );
//        printf( "\n" );
/*
        if ( status == -1 )
        {
            p->Pars.fUseHighest = 0;
            p->Pars.fUseLowest  = 1;
            status = Cbs_ManSolve( p, Gia_ObjChild0(pRoot) );
        }
*/
        Vec_StrPush( vStatus, (char)status );
        if ( status == -1 )
        {
            p->nSatUndec++;
            p->nConfUndec += p->Pars.nBTThis;
            Cec_ManSatAddToStore( vCexStore, NULL, i ); // timeout
            p->timeSatUndec += Abc_Clock() - clk;
            continue;
        }
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
    }
    Vec_IntFree( vVisit );
    p->nSatTotal = Gia_ManPoNum(pAig);
    p->timeTotal = Abc_Clock() - clkTotal;
    if ( fVerbose )
        Cbs_ManSatPrintStats( p );
//    printf( "RecCalls = %8d.  RecClause = %8d.  RecNonChro = %8d.\n", p->nRecCall, p->nRecClause, p->nRecNonChro );
    Cbs_ManStop( p );
    *pvStatus = vStatus;

//    printf( "Total number of cex literals = %d. (Ave = %d)\n", 
//         Vec_IntSize(vCexStore)-2*p->nSatUndec-2*p->nSatSat, 
//        (Vec_IntSize(vCexStore)-2*p->nSatUndec-2*p->nSatSat)/p->nSatSat );
    return vCexStore;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

