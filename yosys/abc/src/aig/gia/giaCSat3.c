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


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Cbs3_Par_t_ Cbs3_Par_t;
struct Cbs3_Par_t_
{
    // conflict limits
    int           nBTLimit;     // limit on the number of conflicts
    int           nJustLimit;   // limit on the size of justification queue
    int           nRestLimit;   // limit on the number of restarts
    // current parameters
    int           nBTThis;      // number of conflicts
    int           nJustThis;    // max size of the frontier
    int           nBTTotal;     // total number of conflicts
    int           nJustTotal;   // total size of the frontier
    // other
    int           fVerbose;
};

typedef struct Cbs3_Que_t_ Cbs3_Que_t;
struct Cbs3_Que_t_
{
    int           iHead;        // beginning of the queue
    int           iTail;        // end of the queue
    int           nSize;        // allocated size
    int *         pData;        // nodes stored in the queue
};
 
typedef struct Cbs3_Man_t_ Cbs3_Man_t;
struct Cbs3_Man_t_
{
    Cbs3_Par_t    Pars;         // parameters
    Gia_Man_t *   pAig;         // AIG manager
    Cbs3_Que_t    pProp;        // propagation queue
    Cbs3_Que_t    pJust;        // justification queue
    Cbs3_Que_t    pClauses;     // clause queue
    Vec_Int_t *   vModel;       // satisfying assignment
    Vec_Int_t *   vTemp;        // temporary storage
    // circuit structure
    int           nVars;
    int           nVarsAlloc;
    int           var_inc;
    Vec_Int_t     vMap;
    Vec_Int_t     vRef;
    Vec_Int_t     vFans;
    Vec_Wec_t     vImps;
    // internal data
    Vec_Str_t     vAssign;
    Vec_Str_t     vMark;
    Vec_Int_t     vLevReason;
    Vec_Int_t     vActs;
    Vec_Int_t     vWatches;
    Vec_Int_t     vWatchUpds;
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
    abctime       timeJFront;
    abctime       timeSatLoad;  // SAT solver loading time
    abctime       timeSatUnsat; // unsat
    abctime       timeSatSat;   // sat
    abctime       timeSatUndec; // undecided
    abctime       timeTotal;    // total runtime
    // other statistics
    int           nPropCalls[3];
    int           nFails[2];
    int           nClauseConf;
    int           nDecs;
};

static inline int   Cbs3_VarUnused( Cbs3_Man_t * p, int iVar )                     { return Vec_IntEntry(&p->vLevReason, 3*iVar) == -1; }
static inline void  Cbs3_VarSetUnused( Cbs3_Man_t * p, int iVar )                  { Vec_IntWriteEntry(&p->vLevReason, 3*iVar, -1);     } 

static inline int   Cbs3_VarMark0( Cbs3_Man_t * p, int iVar )                      { return Vec_StrEntry(&p->vMark, iVar);              }
static inline void  Cbs3_VarSetMark0( Cbs3_Man_t * p, int iVar, int Value )        { Vec_StrWriteEntry(&p->vMark, iVar, (char)Value);   } 

static inline int   Cbs3_VarIsAssigned( Cbs3_Man_t * p, int iVar )                 { return Vec_StrEntry(&p->vAssign, iVar) < 2;        }
static inline void  Cbs3_VarUnassign( Cbs3_Man_t * p, int iVar )                   { assert( Cbs3_VarIsAssigned(p, iVar)); Vec_StrWriteEntry(&p->vAssign, iVar, (char)(2+Vec_StrEntry(&p->vAssign, iVar))); Cbs3_VarSetUnused(p, iVar); }

static inline int   Cbs3_VarValue( Cbs3_Man_t * p, int iVar )                      { return Vec_StrEntry(&p->vAssign, iVar);            }
static inline void  Cbs3_VarSetValue( Cbs3_Man_t * p, int iVar, int v )            { assert( !Cbs3_VarIsAssigned(p, iVar)); Vec_StrWriteEntry(&p->vAssign, iVar, (char)v);                                  }

static inline int   Cbs3_VarLit0( Cbs3_Man_t * p, int iVar )                       { return Vec_IntEntry( &p->vFans, Abc_Var2Lit(iVar, 0) );                             } 
static inline int   Cbs3_VarLit1( Cbs3_Man_t * p, int iVar )                       { return Vec_IntEntry( &p->vFans, Abc_Var2Lit(iVar, 1) );                             } 
static inline int   Cbs3_VarIsPi( Cbs3_Man_t * p, int iVar )                       { return Vec_IntEntry( &p->vFans, Abc_Var2Lit(iVar, 0) ) == 0;                        } 
static inline int   Cbs3_VarIsJust( Cbs3_Man_t * p, int iVar )                     { int * pLits = Vec_IntEntryP(&p->vFans, Abc_Var2Lit(iVar, 0)); return pLits[0] > 0 && Cbs3_VarValue(p, Abc_Lit2Var(pLits[0])) >= 2 && Cbs3_VarValue(p, Abc_Lit2Var(pLits[1])) >= 2; } 

static inline int   Cbs3_VarDecLevel( Cbs3_Man_t * p, int iVar )                   { assert( !Cbs3_VarUnused(p, iVar) ); return Vec_IntEntry(&p->vLevReason, 3*iVar);    }
static inline int   Cbs3_VarReason0( Cbs3_Man_t * p, int iVar )                    { assert( !Cbs3_VarUnused(p, iVar) ); return Vec_IntEntry(&p->vLevReason, 3*iVar+1);  }
static inline int   Cbs3_VarReason1( Cbs3_Man_t * p, int iVar )                    { assert( !Cbs3_VarUnused(p, iVar) ); return Vec_IntEntry(&p->vLevReason, 3*iVar+2);  }
static inline int * Cbs3_VarReasonP( Cbs3_Man_t * p, int iVar )                    { assert( !Cbs3_VarUnused(p, iVar) ); return Vec_IntEntryP(&p->vLevReason, 3*iVar+1); }
static inline int   Cbs3_ClauseDecLevel( Cbs3_Man_t * p, int hClause )             { return Cbs3_VarDecLevel( p, p->pClauses.pData[hClause] );                           }

static inline int   Cbs3_ClauseSize( Cbs3_Man_t * p, int hClause )                 { return p->pClauses.pData[hClause];                                                  }
static inline int * Cbs3_ClauseLits( Cbs3_Man_t * p, int hClause )                 { return p->pClauses.pData+hClause+1;                                                 }
static inline int   Cbs3_ClauseLit( Cbs3_Man_t * p, int hClause, int i )           { return p->pClauses.pData[hClause+1+i];                                              }
static inline int * Cbs3_ClauseNext1p( Cbs3_Man_t * p, int hClause )               { return p->pClauses.pData+hClause+Cbs3_ClauseSize(p, hClause)+2;                     }

static inline void  Cbs3_ClauseSetSize( Cbs3_Man_t * p, int hClause, int x )       { p->pClauses.pData[hClause] = x;                                                     }
static inline void  Cbs3_ClauseSetLit( Cbs3_Man_t * p, int hClause, int i, int x ) { p->pClauses.pData[hClause+i+1] = x;                                                 }
static inline void  Cbs3_ClauseSetNext( Cbs3_Man_t * p, int hClause, int n, int x ){ p->pClauses.pData[hClause+Cbs3_ClauseSize(p, hClause)+1+n] = x;                     }


#define Cbs3_QueForEachEntry( Que, iObj, i )                         \
    for ( i = (Que).iHead; (i < (Que).iTail) && ((iObj) = (Que).pData[i]); i++ )

#define Cbs3_ClauseForEachEntry( p, hClause, iObj, i )               \
    for ( i = 1; i <= Cbs3_ClauseSize(p, hClause) && (iObj = (p)->pClauses.pData[hClause+i]); i++ )
#define Cbs3_ClauseForEachEntry1( p, hClause, iObj, i )              \
    for ( i = 2; i <= Cbs3_ClauseSize(p, hClause) && (iObj = (p)->pClauses.pData[hClause+i]); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sets default values of the parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cbs3_SetDefaultParams( Cbs3_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Cbs3_Par_t) );
    pPars->nBTLimit    =  1000;   // limit on the number of conflicts
    pPars->nJustLimit  =   500;   // limit on the size of justification queue
    pPars->nRestLimit  =    10;   // limit on the number of restarts
    pPars->fVerbose    =     1;   // print detailed statistics
}
void Cbs3_ManSetConflictNum( Cbs3_Man_t * p, int Num )
{
    p->Pars.nBTLimit = Num;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cbs3_Man_t * Cbs3_ManAlloc( Gia_Man_t * pGia )
{
    Cbs3_Man_t * p;
    p = ABC_CALLOC( Cbs3_Man_t, 1 );
    p->pProp.nSize = p->pJust.nSize = p->pClauses.nSize = 10000;
    p->pProp.pData = ABC_ALLOC( int, p->pProp.nSize );
    p->pJust.pData = ABC_ALLOC( int, p->pJust.nSize );
    p->pClauses.pData = ABC_ALLOC( int, p->pClauses.nSize );
    p->pClauses.iHead = p->pClauses.iTail = 1;
    p->vModel   = Vec_IntAlloc( 1000 );
    p->vTemp    = Vec_IntAlloc( 1000 );
    p->pAig     = pGia;
    Cbs3_SetDefaultParams( &p->Pars );
    // circuit structure
    Vec_IntPush( &p->vMap, -1 );
    Vec_IntPush( &p->vRef, -1 );
    Vec_IntPushTwo( &p->vFans, -1, -1 );
    Vec_WecPushLevel( &p->vImps );
    Vec_WecPushLevel( &p->vImps );
    p->nVars = 1;
    // internal data
    p->nVarsAlloc = 1000;
    Vec_StrFill( &p->vAssign,      p->nVarsAlloc, 2 );
    Vec_StrFill( &p->vMark,        p->nVarsAlloc, 0 );
    Vec_IntFill( &p->vLevReason, 3*p->nVarsAlloc, -1 );
    Vec_IntFill( &p->vActs,        p->nVarsAlloc, 0 );
    Vec_IntFill( &p->vWatches,   2*p->nVarsAlloc, 0 );
    Vec_IntGrow( &p->vWatchUpds, 1000 );
    return p;
}
static inline void Cbs3_ManReset( Cbs3_Man_t * p )
{
    assert( p->nVars == Vec_IntSize(&p->vMap) );
    Vec_IntShrink( &p->vMap, 1 );
    Vec_IntShrink( &p->vRef, 1 );
    Vec_IntShrink( &p->vFans, 2 );
    Vec_WecShrink( &p->vImps, 2 );
    p->nVars = 1;
}
static inline void Cbs3_ManGrow( Cbs3_Man_t * p )
{
    if ( p->nVarsAlloc < p->nVars )
    {
        p->nVarsAlloc = 2*p->nVars;
        Vec_StrFill( &p->vAssign,      p->nVarsAlloc, 2 );
        Vec_StrFill( &p->vMark,        p->nVarsAlloc, 0 );
        Vec_IntFill( &p->vLevReason, 3*p->nVarsAlloc, -1 );
        Vec_IntFill( &p->vActs,        p->nVarsAlloc, 0 );
        Vec_IntFill( &p->vWatches,   2*p->nVarsAlloc, 0 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cbs3_ManStop( Cbs3_Man_t * p )
{
    // circuit structure
    Vec_IntErase( &p->vMap );
    Vec_IntErase( &p->vRef );
    Vec_IntErase( &p->vFans );
    Vec_WecErase( &p->vImps );
    // internal data
    Vec_StrErase( &p->vAssign );
    Vec_StrErase( &p->vMark );
    Vec_IntErase( &p->vLevReason );
    Vec_IntErase( &p->vActs );
    Vec_IntErase( &p->vWatches );
    Vec_IntErase( &p->vWatchUpds );
    Vec_IntFree( p->vModel );
    Vec_IntFree( p->vTemp );
    ABC_FREE( p->pClauses.pData );
    ABC_FREE( p->pProp.pData );
    ABC_FREE( p->pJust.pData );
    ABC_FREE( p );
}
int Cbs3_ManMemory( Cbs3_Man_t * p )
{
    int nMem = sizeof(Cbs3_Man_t);
    nMem += (int)Vec_IntMemory( &p->vMap );
    nMem += (int)Vec_IntMemory( &p->vRef );
    nMem += (int)Vec_IntMemory( &p->vFans );
    nMem += (int)Vec_WecMemory( &p->vImps );
    nMem += (int)Vec_StrMemory( &p->vAssign );
    nMem += (int)Vec_StrMemory( &p->vMark );
    nMem += (int)Vec_IntMemory( &p->vActs );
    nMem += (int)Vec_IntMemory( &p->vWatches );
    nMem += (int)Vec_IntMemory( &p->vWatchUpds );
    nMem += (int)Vec_IntMemory( p->vModel );
    nMem += (int)Vec_IntMemory( p->vTemp );
    nMem += 4*p->pClauses.nSize;
    nMem += 4*p->pProp.nSize;
    nMem += 4*p->pJust.nSize;
    return nMem;
}

/**Function*************************************************************

  Synopsis    [Returns satisfying assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Cbs3_ReadModel( Cbs3_Man_t * p )
{
    return p->vModel;
}


/**Function*************************************************************

  Synopsis    [Activity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
//#define USE_ACTIVITY

#ifdef USE_ACTIVITY
static inline void Cbs3_ActReset( Cbs3_Man_t * p ) 
{
    int i, * pAct = Vec_IntArray(&p->vActs);
    for ( i = 0; i < p->nVars; i++ )
        pAct[i] = (1 << 10);
    p->var_inc = (1 << 5);
}
static inline void Cbs3_ActRescale( Cbs3_Man_t * p ) 
{
    int i, * pAct = Vec_IntArray(&p->vActs);
    for ( i = 0; i < p->nVars; i++ )
        pAct[i] >>= 19;
    p->var_inc >>= 19;
    p->var_inc = Abc_MaxInt( (unsigned)p->var_inc, (1<<5) );
}
static inline void Cbs3_ActBumpVar( Cbs3_Man_t * p, int iVar ) 
{
    int * pAct = Vec_IntArray(&p->vActs);
    pAct[iVar] += p->var_inc;
    if ((unsigned)pAct[iVar] & 0x80000000)
        Cbs3_ActRescale(p);
}
static inline void Cbs3_ActDecay( Cbs3_Man_t * p ) 
{
    p->var_inc += (p->var_inc >>  4); 
}
#else
static inline void Cbs3_ActReset( Cbs3_Man_t * p )              {}
static inline void Cbs3_ActRescale( Cbs3_Man_t * p )            {}
static inline void Cbs3_ActBumpVar( Cbs3_Man_t * p, int iVar )  {}
static inline void Cbs3_ActDecay( Cbs3_Man_t * p )              {}
#endif


/**Function*************************************************************

  Synopsis    [Returns 1 if the solver is out of limits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs3_ManCheckLimits( Cbs3_Man_t * p )
{
    p->nFails[0] += p->Pars.nJustThis > p->Pars.nJustLimit;
    p->nFails[1] += p->Pars.nBTThis > p->Pars.nBTLimit;
    return p->Pars.nJustThis > p->Pars.nJustLimit || p->Pars.nBTThis > p->Pars.nBTLimit;
}

/**Function*************************************************************

  Synopsis    [Saves the satisfying assignment as an array of literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs3_ManSaveModel( Cbs3_Man_t * p, Vec_Int_t * vCex )
{
    int i, iLit;
    Vec_IntClear( vCex );
    p->pProp.iHead = 0;
    Cbs3_QueForEachEntry( p->pProp, iLit, i )
        if ( Cbs3_VarIsPi(p, Abc_Lit2Var(iLit)) )
            Vec_IntPush( vCex, Abc_Lit2LitV(Vec_IntArray(&p->vMap), iLit)-2 );
} 
static inline void Cbs3_ManSaveModelAll( Cbs3_Man_t * p, Vec_Int_t * vCex )
{
    int i, iLit;
    Vec_IntClear( vCex );
    p->pProp.iHead = 0;
    Cbs3_QueForEachEntry( p->pProp, iLit, i )
    {
        int iVar = Abc_Lit2Var(iLit);
        Vec_IntPush( vCex, Abc_Var2Lit(iVar, !Cbs3_VarValue(p, iVar)) );
    }
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs3_QueIsEmpty( Cbs3_Que_t * p )
{
    return p->iHead == p->iTail;
}
static inline int Cbs3_QueSize( Cbs3_Que_t * p )
{
    return p->iTail - p->iHead;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs3_QuePush( Cbs3_Que_t * p, int iObj )
{
    if ( p->iTail == p->nSize )
    {
        p->nSize *= 2;
        p->pData = ABC_REALLOC( int, p->pData, p->nSize ); 
    }
    p->pData[p->iTail++] = iObj;
}
static inline void Cbs3_QueGrow( Cbs3_Que_t * p, int Plus )
{
    if ( p->iTail + Plus > p->nSize )
    {
        p->nSize *= 2;
        p->pData = ABC_REALLOC( int, p->pData, p->nSize ); 
    }
    assert( p->iTail + Plus <= p->nSize );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the object in the queue.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs3_QueHasNode( Cbs3_Que_t * p, int iObj )
{
    int i, iTemp;
    Cbs3_QueForEachEntry( *p, iTemp, i )
        if ( iTemp == iObj )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs3_QueStore( Cbs3_Que_t * p, int * piHeadOld, int * piTailOld )
{
    int i;
    *piHeadOld = p->iHead;
    *piTailOld = p->iTail;
    for ( i = *piHeadOld; i < *piTailOld; i++ )
        Cbs3_QuePush( p, p->pData[i] );
    p->iHead = *piTailOld;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs3_QueRestore( Cbs3_Que_t * p, int iHeadOld, int iTailOld )
{
    p->iHead = iHeadOld;
    p->iTail = iTailOld;
}

/**Function*************************************************************

  Synopsis    [Find variable with the highest ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs3_ManDecide( Cbs3_Man_t * p )
{
    int i, iObj, iObjMax = 0;
#ifdef USE_ACTIVITY
    Cbs3_QueForEachEntry( p->pJust, iObj, i )
        if ( iObjMax == 0 || 
             Vec_IntEntry(&p->vActs, iObjMax) <  Vec_IntEntry(&p->vActs, iObj) || 
            (Vec_IntEntry(&p->vActs, iObjMax) == Vec_IntEntry(&p->vActs, iObj) && Vec_IntEntry(&p->vMap, iObjMax) < Vec_IntEntry(&p->vMap, iObj)) )
             iObjMax = iObj;
#else
    Cbs3_QueForEachEntry( p->pJust, iObj, i )
//       if ( iObjMax == 0 || iObjMax < iObj )
       if ( iObjMax == 0 || Vec_IntEntry(&p->vMap, iObjMax) < Vec_IntEntry(&p->vMap, iObj) )
            iObjMax = iObj;
#endif
    return iObjMax;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs3_ManCancelUntil( Cbs3_Man_t * p, int iBound )
{
    int i, iLit;
    assert( iBound <= p->pProp.iTail );
    p->pProp.iHead = iBound;
    Cbs3_QueForEachEntry( p->pProp, iLit, i )
        Cbs3_VarUnassign( p, Abc_Lit2Var(iLit) );
    p->pProp.iTail = iBound;
}

/**Function*************************************************************

  Synopsis    [Assigns the variables a value.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs3_ManAssign( Cbs3_Man_t * p, int iLit, int Level, int iRes0, int iRes1 )
{
    int iObj = Abc_Lit2Var(iLit);
    assert( Cbs3_VarUnused(p, iObj) );
    assert( !Cbs3_VarIsAssigned(p, iObj) );
    Cbs3_VarSetValue( p, iObj, !Abc_LitIsCompl(iLit) );
    Cbs3_QuePush( &p->pProp, iLit );
    Vec_IntWriteEntry( &p->vLevReason, 3*iObj,   Level );
    Vec_IntWriteEntry( &p->vLevReason, 3*iObj+1, iRes0 );
    Vec_IntWriteEntry( &p->vLevReason, 3*iObj+2, iRes1 );
}




/**Function*************************************************************

  Synopsis    [Prints conflict clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs3_ManPrintClause( Cbs3_Man_t * p, int Level, int hClause )
{
    int i, iLit;
    assert( Cbs3_QueIsEmpty( &p->pClauses ) );
    printf( "Level %2d : ", Level );
    Cbs3_ClauseForEachEntry( p, hClause, iLit, i )
        printf( "%c%d ", Abc_LitIsCompl(iLit) ? '-':'+', Abc_Lit2Var(iLit) );
//        printf( "%d=%d(%d) ", iObj, Cbs3_VarValue(p, Abc_Lit2Var(iLit)), Cbs3_VarDecLevel(p, Abc_Lit2Var(iLit)) );
    printf( "\n" );
}
static inline void Cbs3_ManPrintCube( Cbs3_Man_t * p, int Level, int hClause )
{
    int i, iObj;
    assert( Cbs3_QueIsEmpty( &p->pClauses ) );
    printf( "Level %2d : ", Level );
    Cbs3_ClauseForEachEntry( p, hClause, iObj, i )
        printf( "%c%d ", Cbs3_VarValue(p, iObj)? '+':'-', iObj );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Finalized the clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs3_ManCleanWatch( Cbs3_Man_t * p )
{
    int i, iLit;
    Vec_IntForEachEntry( &p->vWatchUpds, iLit, i )
        Vec_IntWriteEntry( &p->vWatches, iLit, 0 );
    Vec_IntClear( &p->vWatchUpds );
    //Vec_IntForEachEntry( &p->vWatches, iLit, i )
    //    assert( iLit == 0 );
}
static inline void Cbs3_ManWatchClause( Cbs3_Man_t * p, int hClause, int Lit )
{
    int * pLits = Cbs3_ClauseLits( p, hClause );
    int * pPlace = Vec_IntEntryP( &p->vWatches, Abc_LitNot(Lit) );
    if ( *pPlace == 0 )
        Vec_IntPush( &p->vWatchUpds, Abc_LitNot(Lit) );
/*
    if ( pClause->pLits[0] == Lit )
        pClause->pNext0 = p->pWatches[lit_neg(Lit)];  
    else
    {
        assert( pClause->pLits[1] == Lit );
        pClause->pNext1 = p->pWatches[lit_neg(Lit)];  
    }
    p->pWatches[lit_neg(Lit)] = pClause;
*/
    assert( Lit == pLits[0] || Lit == pLits[1] );
    Cbs3_ClauseSetNext( p, hClause, Lit == pLits[1], *pPlace );
    *pPlace = hClause;
}
static inline int Cbs3_QueFinish( Cbs3_Man_t * p, int Level )
{
    Cbs3_Que_t * pQue = &(p->pClauses); 
    int i, iObj, hClauseC, hClause = pQue->iHead, Size = pQue->iTail - pQue->iHead - 1;
    assert( pQue->iHead+1 < pQue->iTail );
    Cbs3_ClauseSetSize( p, pQue->iHead, Size );
    hClauseC = pQue->iHead = pQue->iTail;
    //printf( "Adding cube: " ); Cbs3_ManPrintCube(p, Level, hClause);
    if ( Size == 1 )
        return hClause;
    // create watched clause
    pQue->iHead = hClause;
    Cbs3_QueForEachEntry( p->pClauses, iObj, i )
    {
        if ( i == hClauseC )
            break;
        else if ( i == hClause ) // nlits
            Cbs3_QuePush( pQue, iObj );
        else // literals
            Cbs3_QuePush( pQue, Abc_Var2Lit(iObj, Cbs3_VarValue(p, iObj)) ); // complement
    }
    Cbs3_QuePush( pQue, 0 ); // next0
    Cbs3_QuePush( pQue, 0 ); // next1
    pQue->iHead = pQue->iTail;
    Cbs3_ManWatchClause( p, hClauseC, Cbs3_ClauseLit(p, hClauseC, 0) );
    Cbs3_ManWatchClause( p, hClauseC, Cbs3_ClauseLit(p, hClauseC, 1) );
    //printf( "Adding clause %d: ", hClauseC ); Cbs3_ManPrintClause(p, Level, hClauseC);
    Cbs3_ActDecay( p );
    return hClause;
}

/**Function*************************************************************

  Synopsis    [Returns conflict clause.]

  Description [Performs conflict analysis.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs3_ManDeriveReason( Cbs3_Man_t * p, int Level )
{
    Cbs3_Que_t * pQue = &(p->pClauses);
    int i, k, iObj, iLitLevel, * pReason;
    assert( pQue->pData[pQue->iHead] == 0 );
    assert( pQue->pData[pQue->iHead+1] == 0 );
    assert( pQue->iHead + 2 < pQue->iTail );
    //for ( i = pQue->iHead + 2; i < pQue->iTail; i++ )
    //    assert( !Cbs3_VarMark0(p, pQue->pData[i]) );
    // compact literals
    Vec_IntClear( p->vTemp );
    for ( i = k = pQue->iHead + 2; i < pQue->iTail; i++ )
    {
        iObj = pQue->pData[i];
        if ( Cbs3_VarMark0(p, iObj) ) // unassigned - seen again
            continue;
        //if ( Vec_IntEntry(&p->vActivity, iObj) == 0 )
        //    Vec_IntPush( &p->vActStore, iObj );
        //Vec_IntAddToEntry( &p->vActivity, iObj, 1 );
        // assigned - seen first time
        Cbs3_VarSetMark0(p, iObj, 1);
        Cbs3_ActBumpVar(p, iObj);
        Vec_IntPush( p->vTemp, iObj );
        // check decision level
        iLitLevel = Cbs3_VarDecLevel( p, iObj );
        if ( iLitLevel < Level )
        {
            pQue->pData[k++] = iObj;
            continue;
        }
        assert( iLitLevel == Level );
        pReason = Cbs3_VarReasonP( p, iObj );
        if ( pReason[0] == 0 && pReason[1] == 0 ) // no reason
        {
            assert( pQue->pData[pQue->iHead+1] == 0 );
            pQue->pData[pQue->iHead+1] = iObj;
        }
        else if ( pReason[0] != 0 ) // circuit reason
        {
            Cbs3_QuePush( pQue, pReason[0] );
            if ( pReason[1] )
            Cbs3_QuePush( pQue, pReason[1] );
        }
        else // clause reason
        {
            int i, * pLits, nLits = Cbs3_ClauseSize( p, pReason[1] );
            assert( pReason[1] );
            Cbs3_QueGrow( pQue, nLits );
            pLits = Cbs3_ClauseLits( p, pReason[1] );
            assert( iObj == Abc_Lit2Var(pLits[0]) );
            for ( i = 1; i < nLits; i++ )
                Cbs3_QuePush( pQue, Abc_Lit2Var(pLits[i]) );
        }
    }
    assert( pQue->pData[pQue->iHead] == 0 );
    assert( pQue->pData[pQue->iHead+1] != 0 );
    pQue->iTail = k;
    // clear the marks
    Vec_IntForEachEntry( p->vTemp, iObj, i )
        Cbs3_VarSetMark0(p, iObj, 0);
    return Cbs3_QueFinish( p, Level );
}
static inline int Cbs3_ManAnalyze( Cbs3_Man_t * p, int Level, int iVar, int iFan0, int iFan1 )
{
    Cbs3_Que_t * pQue = &(p->pClauses); 
    assert( Cbs3_VarIsAssigned(p, iVar) );
    assert( Cbs3_QueIsEmpty( pQue ) );
    Cbs3_QuePush( pQue, 0 );
    Cbs3_QuePush( pQue, 0 );
    if ( iFan0 ) // circuit conflict
    {
        assert( Cbs3_VarIsAssigned(p, iFan0) );
        assert( iFan1 == 0 || Cbs3_VarIsAssigned(p, iFan1) );
        Cbs3_QuePush( pQue, iVar );
        Cbs3_QuePush( pQue, iFan0 );
        if ( iFan1 )
        Cbs3_QuePush( pQue, iFan1 );
    }
    else // clause conflict
    {
        int i, * pLits, nLits = Cbs3_ClauseSize( p, iFan1 );
        assert( iFan1 );
        Cbs3_QueGrow( pQue, nLits );
        pLits = Cbs3_ClauseLits( p, iFan1 );
        assert( iVar == Abc_Lit2Var(pLits[0]) );
        assert( Cbs3_VarValue(p, iVar) == Abc_LitIsCompl(pLits[0]) );
        for ( i = 0; i < nLits; i++ )
            Cbs3_QuePush( pQue, Abc_Lit2Var(pLits[i]) );
    }
    return Cbs3_ManDeriveReason( p, Level );
}


/**Function*************************************************************

  Synopsis    [Propagate one assignment.]

  Description [Returns handle of the conflict clause, if conflict occurs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs3_ManPropagateClauses( Cbs3_Man_t * p, int Level, int Lit )
{
    int i, Value, Cur, LitF = Abc_LitNot(Lit);
    int * pPrev = Vec_IntEntryP( &p->vWatches, Lit );
    //for ( pCur = p->pWatches[Lit]; pCur; pCur = *ppPrev )
    for ( Cur = *pPrev; Cur; Cur = *pPrev )
    {
        int nLits = Cbs3_ClauseSize( p, Cur );
        int * pLits = Cbs3_ClauseLits( p, Cur );
        p->nPropCalls[1]++;
//printf( "  Watching literal %c%d on level %d.\n", Abc_LitIsCompl(Lit) ? '-':'+', Abc_Lit2Var(Lit), Level );
        // make sure the false literal is in the second literal of the clause
        //if ( pCur->pLits[0] == LitF )
        if ( pLits[0] == LitF )
        {
            //pCur->pLits[0] = pCur->pLits[1];
            pLits[0] = pLits[1];
            //pCur->pLits[1] = LitF;
            pLits[1] = LitF;
            //pTemp = pCur->pNext0;
            //pCur->pNext0 = pCur->pNext1;
            //pCur->pNext1 = pTemp;
            ABC_SWAP( int, pLits[nLits], pLits[nLits+1] );
        }
        //assert( pCur->pLits[1] == LitF );
        assert( pLits[1] == LitF );

        // if the first literal is true, the clause is satisfied
        //if ( pCur->pLits[0] == p->pAssigns[lit_var(pCur->pLits[0])] )
        if ( Cbs3_VarValue(p, Abc_Lit2Var(pLits[0])) == !Abc_LitIsCompl(pLits[0]) )
        {
            //ppPrev = &pCur->pNext1;
            pPrev = Cbs3_ClauseNext1p(p, Cur);
            continue;
        }

        // look for a new literal to watch
        for ( i = 2; i < nLits; i++ )
        {
            // skip the case when the literal is false
            //if ( lit_neg(pCur->pLits[i]) == p->pAssigns[lit_var(pCur->pLits[i])] )
            if ( Cbs3_VarValue(p, Abc_Lit2Var(pLits[i])) == Abc_LitIsCompl(pLits[i]) )
                continue;
            // the literal is either true or unassigned - watch it
            //pCur->pLits[1] = pCur->pLits[i];
            //pCur->pLits[i] = LitF;
            pLits[1] = pLits[i];
            pLits[i] = LitF;
            // remove this clause from the watch list of Lit
            //*ppPrev = pCur->pNext1;
            *pPrev = *Cbs3_ClauseNext1p(p, Cur);
            // add this clause to the watch list of pCur->pLits[i] (now it is pCur->pLits[1])
            //Intb_ManWatchClause( p, pCur, pCur->pLits[1] );
            Cbs3_ManWatchClause( p, Cur, Cbs3_ClauseLit(p, Cur, 1) );
            break;
        }
        if ( i < nLits ) // found new watch
            continue;

        // clause is unit - enqueue new implication
        //if ( Inta_ManEnqueue(p, pCur->pLits[0], pCur) )
        //{
        //    ppPrev = &pCur->pNext1;
        //    continue;
        //}

        // clause is unit - enqueue new implication
        Value = Cbs3_VarValue(p, Abc_Lit2Var(pLits[0]));
        if ( Value >= 2 ) // unassigned
        {
            Cbs3_ManAssign( p, pLits[0], Level, 0, Cur );
            pPrev = Cbs3_ClauseNext1p(p, Cur);
            continue;
        }

        // conflict detected - return the conflict clause
        //return pCur;
        if ( Value == Abc_LitIsCompl(pLits[0]) )
        {
            p->nClauseConf++;
            return Cbs3_ManAnalyze( p, Level, Abc_Lit2Var(pLits[0]), 0, Cur );
        }
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Performs resolution of two clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs3_ManResolve( Cbs3_Man_t * p, int Level, int hClause0, int hClause1 )
{
    Cbs3_Que_t * pQue = &(p->pClauses);
    int i, iObj, LevelMax = -1, LevelCur;
    assert( pQue->pData[hClause0+1] != 0 );
    assert( pQue->pData[hClause0+1] == pQue->pData[hClause1+1] );
    //Cbs3_ClauseForEachEntry1( p, hClause0, iObj, i )
    //    assert( !Cbs3_VarMark0(p, iObj) );
    //Cbs3_ClauseForEachEntry1( p, hClause1, iObj, i )
    //    assert( !Cbs3_VarMark0(p, iObj) );
    assert( Cbs3_QueIsEmpty( pQue ) );
    Cbs3_QuePush( pQue, 0 );
    Cbs3_QuePush( pQue, 0 );
//    for ( i = hClause0 + 1; (iObj = pQue->pData[i]); i++ )
    Cbs3_ClauseForEachEntry1( p, hClause0, iObj, i )
    {
        if ( Cbs3_VarMark0(p, iObj) ) // unassigned - seen again
            continue;
        //if ( Vec_IntEntry(&p->vActivity, iObj) == 0 )
        //    Vec_IntPush( &p->vActStore, iObj );
        //Vec_IntAddToEntry( &p->vActivity, iObj, 1 );
        // assigned - seen first time
        Cbs3_VarSetMark0(p, iObj, 1);
        Cbs3_ActBumpVar(p, iObj);
        Cbs3_QuePush( pQue, iObj );
        LevelCur = Cbs3_VarDecLevel( p, iObj );
        if ( LevelMax < LevelCur )
            LevelMax = LevelCur;
    }
//    for ( i = hClause1 + 1; (iObj = pQue->pData[i]); i++ )
    Cbs3_ClauseForEachEntry1( p, hClause1, iObj, i )
    {
        if ( Cbs3_VarMark0(p, iObj) ) // unassigned - seen again
            continue;
        //if ( Vec_IntEntry(&p->vActivity, iObj) == 0 )
        //    Vec_IntPush( &p->vActStore, iObj );
        //Vec_IntAddToEntry( &p->vActivity, iObj, 1 );
        // assigned - seen first time
        Cbs3_VarSetMark0(p, iObj, 1);
        Cbs3_ActBumpVar(p, iObj);
        Cbs3_QuePush( pQue, iObj );
        LevelCur = Cbs3_VarDecLevel( p, iObj );
        if ( LevelMax < LevelCur )
            LevelMax = LevelCur;
    }
    for ( i = pQue->iHead + 2; i < pQue->iTail; i++ )
        Cbs3_VarSetMark0(p, pQue->pData[i], 0);
    return Cbs3_ManDeriveReason( p, LevelMax );
}

/**Function*************************************************************

  Synopsis    [Propagates a variable.]

  Description [Returns clause handle if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cbs3_ManUpdateJFrontier( Cbs3_Man_t * p )
{
    //abctime clk = Abc_Clock();
    int iVar, iLit, i, k = p->pJust.iTail;
    Cbs3_QueGrow( &p->pJust, Cbs3_QueSize(&p->pJust) + Cbs3_QueSize(&p->pProp) );
    Cbs3_QueForEachEntry( p->pJust, iVar, i )
        if ( Cbs3_VarIsJust(p, iVar) )
            p->pJust.pData[k++] = iVar;
    Cbs3_QueForEachEntry( p->pProp, iLit, i )
        if ( Cbs3_VarIsJust(p, Abc_Lit2Var(iLit)) )
            p->pJust.pData[k++] = Abc_Lit2Var(iLit);
    p->pJust.iHead = p->pJust.iTail;
    p->pJust.iTail = k;
    //p->timeJFront += Abc_Clock() - clk;
}
int Cbs3_ManPropagateNew( Cbs3_Man_t * p, int Level )
{
    int i, k, iLit, hClause, nLits, * pLits;
    p->nPropCalls[0]++;
    Cbs3_QueForEachEntry( p->pProp, iLit, i )
    {
        if ( (hClause = Cbs3_ManPropagateClauses(p, Level, iLit)) )
            return hClause;
        p->nPropCalls[2]++;
        nLits = Vec_IntSize(Vec_WecEntry(&p->vImps, iLit));
        pLits = Vec_IntArray(Vec_WecEntry(&p->vImps, iLit)); 
        for ( k = 0; k < nLits; k += 2 )
        {
            int Value0 = Cbs3_VarValue(p, Abc_Lit2Var(pLits[k]));
            int Value1 = pLits[k+1] ? Cbs3_VarValue(p, Abc_Lit2Var(pLits[k+1])) : -1;
            if ( Value1 == -1 || Value1 == Abc_LitIsCompl(pLits[k+1]) ) // pLits[k+1] is false
            {
                if ( Value0 >= 2 ) // pLits[k] is unassigned
                    Cbs3_ManAssign( p, pLits[k], Level, Abc_Lit2Var(iLit), Abc_Lit2Var(pLits[k+1]) );
                else if ( Value0 == Abc_LitIsCompl(pLits[k]) ) // pLits[k] is false
                    return Cbs3_ManAnalyze( p, Level, Abc_Lit2Var(iLit), Abc_Lit2Var(pLits[k]), Abc_Lit2Var(pLits[k+1]) );
            }
            if ( Value1 != -1 && Value0 == Abc_LitIsCompl(pLits[k]) ) // pLits[k] is false
            {
                if ( Value1 >= 2 ) // pLits[k+1] is unassigned
                    Cbs3_ManAssign( p, pLits[k+1], Level, Abc_Lit2Var(iLit), Abc_Lit2Var(pLits[k]) );
                else if ( Value1 == Abc_LitIsCompl(pLits[k+1]) ) // pLits[k+1] is false
                    return Cbs3_ManAnalyze( p, Level, Abc_Lit2Var(iLit), Abc_Lit2Var(pLits[k]), Abc_Lit2Var(pLits[k+1]) );
            }
        }
    }
    Cbs3_ManUpdateJFrontier( p );
    // finalize propagation queue
    p->pProp.iHead = p->pProp.iTail;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Solve the problem recursively.]

  Description [Returns learnt clause if unsat, NULL if sat or undecided.]
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
int Cbs3_ManSolve2_rec( Cbs3_Man_t * p, int Level )
{ 
    Cbs3_Que_t * pQue = &(p->pClauses);
    int iPropHead, iJustHead, iJustTail;
    int hClause, hLearn0, hLearn1, iVar, iDecLit;
    int nRef0, nRef1;
    // propagate assignments
    assert( !Cbs3_QueIsEmpty(&p->pProp) );
    //if ( (hClause = Cbs3_ManPropagate( p, Level )) )
    if ( (hClause = Cbs3_ManPropagateNew( p, Level )) )
        return hClause;
    // check for satisfying assignment
    assert( Cbs3_QueIsEmpty(&p->pProp) );
    if ( Cbs3_QueIsEmpty(&p->pJust) )
        return 0;
    // quit using resource limits
    p->Pars.nJustThis = Abc_MaxInt( p->Pars.nJustThis, p->pJust.iTail - p->pJust.iHead );
    if ( Cbs3_ManCheckLimits( p ) )
        return 0;
    // remember the state before branching
    iPropHead = p->pProp.iHead;
    iJustHead = p->pJust.iHead;
    iJustTail = p->pJust.iTail;
    // find the decision variable
    p->nDecs++;
    iVar = Cbs3_ManDecide( p );
    assert( !Cbs3_VarIsPi(p, iVar) );
    assert( Cbs3_VarIsJust(p, iVar) );
    // chose decision variable using fanout count
    nRef0 = Vec_IntEntry(&p->vRef, Abc_Lit2Var(Cbs3_VarLit0(p, iVar)));
    nRef1 = Vec_IntEntry(&p->vRef, Abc_Lit2Var(Cbs3_VarLit1(p, iVar)));
//    if ( nRef0 >= nRef1 || (nRef0 == nRef1) && (Abc_Random(0) & 1) )
    if ( nRef0 >= nRef1 )
        iDecLit = Abc_LitNot(Cbs3_VarLit0(p, iVar));
    else
        iDecLit = Abc_LitNot(Cbs3_VarLit1(p, iVar));
    // decide on first fanin
    Cbs3_ManAssign( p, iDecLit, Level+1, 0, 0 );
    if ( !(hLearn0 = Cbs3_ManSolve2_rec( p, Level+1 )) )
        return 0;
    if ( pQue->pData[hLearn0+1] != Abc_Lit2Var(iDecLit) )
        return hLearn0;
    Cbs3_ManCancelUntil( p, iPropHead );
    Cbs3_QueRestore( &p->pJust, iJustHead, iJustTail );
    // decide on second fanin
    Cbs3_ManAssign( p, Abc_LitNot(iDecLit), Level+1, 0, 0 );
    if ( !(hLearn1 = Cbs3_ManSolve2_rec( p, Level+1 )) )
        return 0;
    if ( pQue->pData[hLearn1+1] != Abc_Lit2Var(iDecLit) )
        return hLearn1;
    hClause = Cbs3_ManResolve( p, Level, hLearn0, hLearn1 );
    p->Pars.nBTThis++;
    return hClause;
}

/**Function*************************************************************

  Synopsis    [Looking for a satisfying assignment of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs3_ManSolveInt( Cbs3_Man_t * p, int iLit )
{
    int RetValue = 0;
    assert( !p->pProp.iHead && !p->pProp.iTail );
    assert( !p->pJust.iHead && !p->pJust.iTail );
    p->Pars.nBTThis = p->Pars.nJustThis = 0;
    Cbs3_ManAssign( p, iLit, 0, 0, 0 );
    if ( !Cbs3_ManSolve2_rec(p, 0) && !Cbs3_ManCheckLimits(p) )
        Cbs3_ManSaveModel( p, p->vModel );
    else
        RetValue = 1;
    Cbs3_ManCancelUntil( p, 0 );
    p->pJust.iHead = p->pJust.iTail = 0;
    p->Pars.nBTTotal += p->Pars.nBTThis;
    p->Pars.nJustTotal = Abc_MaxInt( p->Pars.nJustTotal, p->Pars.nJustThis );
    if ( Cbs3_ManCheckLimits( p ) )
        RetValue = -1;
    return RetValue;
}
int Cbs3_ManSolve( Cbs3_Man_t * p, int iLit, int nRestarts )
{
    int i, RetValue = -1;
    assert( p->pClauses.iHead == 1 && p->pClauses.iTail == 1 );
    for ( i = 0; i < nRestarts; i++ )
        if ( (RetValue = Cbs3_ManSolveInt(p, iLit)) != -1 )
            break;
    Cbs3_ManCleanWatch( p );
    p->pClauses.iHead = p->pClauses.iTail = 1;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Prints statistics of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cbs3_ManSatPrintStats( Cbs3_Man_t * p )
{
    printf( "CO = %8d  ", Gia_ManCoNum(p->pAig) );
    printf( "AND = %8d  ", Gia_ManAndNum(p->pAig) );
    printf( "Conf = %6d  ", p->Pars.nBTLimit );
    printf( "Restart = %2d  ", p->Pars.nRestLimit );
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

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs3_ManAddVar( Cbs3_Man_t * p, int iGiaObj )
{
    assert( Vec_IntSize(&p->vMap) == p->nVars );
    Vec_IntPush( &p->vMap, iGiaObj );
    Vec_IntPush( &p->vRef, Gia_ObjRefNumId(p->pAig, iGiaObj) );
    Vec_IntPushTwo( &p->vFans, 0, 0 );
    Vec_WecPushLevel(&p->vImps);
    Vec_WecPushLevel(&p->vImps);
    return Abc_Var2Lit( p->nVars++, 0 );
}
static inline void Cbs3_ManAddConstr( Cbs3_Man_t * p, int x, int x0, int x1 )
{
    Vec_WecPushTwo( &p->vImps,   x ,   x0,   0  ); // ~x +  x0
    Vec_WecPushTwo( &p->vImps,   x ,   x1,   0  ); // ~x +  x1
    Vec_WecPushTwo( &p->vImps, 1^x0, 1^x ,   0  ); // ~x +  x0
    Vec_WecPushTwo( &p->vImps, 1^x1, 1^x ,   0  ); // ~x +  x1
    Vec_WecPushTwo( &p->vImps, 1^x , 1^x0, 1^x1 ); //  x + ~x0 + ~x1
    Vec_WecPushTwo( &p->vImps,   x0,   x , 1^x1 ); //  x + ~x0 + ~x1
    Vec_WecPushTwo( &p->vImps,   x1,   x , 1^x0 ); //  x + ~x0 + ~x1
}
static inline void Cbs3_ManAddAnd( Cbs3_Man_t * p, int x, int x0, int x1 )
{
    assert( x > 0 && x0 > 0 && x1 > 0 );
    Vec_IntWriteEntry( &p->vFans, x,   x0 );
    Vec_IntWriteEntry( &p->vFans, x+1, x1 );
    Cbs3_ManAddConstr( p, x, x0, x1 );
}
static inline int Cbs3_ManToSolver1_rec( Cbs3_Man_t * pSol, Gia_Man_t * p, int iObj, int Depth )
{
    Gia_Obj_t * pObj = Gia_ManObj(p, iObj); int Lit0, Lit1;
    if ( Gia_ObjUpdateTravIdCurrentId(p, iObj) )
        return pObj->Value;
    pObj->Value = Cbs3_ManAddVar( pSol, iObj );
    if ( Gia_ObjIsCi(pObj) || Depth == 0 )
        return pObj->Value;
    assert( Gia_ObjIsAnd(pObj) );
    Lit0 = Cbs3_ManToSolver1_rec( pSol, p, Gia_ObjFaninId0(pObj, iObj), Depth - Gia_ObjFaninC0(pObj) );
    Lit1 = Cbs3_ManToSolver1_rec( pSol, p, Gia_ObjFaninId1(pObj, iObj), Depth - Gia_ObjFaninC1(pObj) );
    Cbs3_ManAddAnd( pSol, pObj->Value, Lit0 ^ Gia_ObjFaninC0(pObj), Lit1 ^ Gia_ObjFaninC1(pObj) );
    return pObj->Value;
}
static inline int Cbs3_ManToSolver1( Cbs3_Man_t * pSol, Gia_Man_t * p, Gia_Obj_t * pRoot, int nRestarts, int Depth )
{
    //abctime clk = Abc_Clock();
    assert( Gia_ObjIsCo(pRoot) );
    Cbs3_ManReset( pSol );
    Gia_ManIncrementTravId( p );
    Cbs3_ManToSolver1_rec( pSol, p, Gia_ObjFaninId0p(p, pRoot), Depth );
    Cbs3_ManGrow( pSol );
    Cbs3_ActReset( pSol );
    //pSol->timeSatLoad += Abc_Clock() - clk;
    return Cbs3_ManSolve( pSol, Gia_ObjFanin0Copy(pRoot), nRestarts );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs3_ManPrepare( Cbs3_Man_t * p )
{
    int x, x0, x1;
    Vec_WecInit( &p->vImps, Abc_Var2Lit(p->nVars, 0) );
    Vec_IntForEachEntryDoubleStart( &p->vFans, x0, x1, x, 2 )
        if ( x0 ) Cbs3_ManAddConstr( p, x, x0, x1 );
}
static inline int Cbs3_ManAddNode( Cbs3_Man_t * p, int iGiaObj, int iLit0, int iLit1 )
{
    assert( Vec_IntSize(&p->vMap) == p->nVars );
    Vec_IntPush( &p->vMap, iGiaObj );
    Vec_IntPush( &p->vRef, Gia_ObjRefNumId(p->pAig, iGiaObj) );
    Vec_IntPushTwo( &p->vFans, iLit0, iLit1 );
    return Abc_Var2Lit( p->nVars++, 0 );
}
static inline int Cbs3_ManToSolver2_rec( Cbs3_Man_t * pSol, Gia_Man_t * p, int iObj, int Depth )
{
    Gia_Obj_t * pObj = Gia_ManObj(p, iObj); int Lit0, Lit1;
    if ( Gia_ObjUpdateTravIdCurrentId(p, iObj) )
        return pObj->Value;
    if ( Gia_ObjIsCi(pObj) || Depth == 0 )
        return pObj->Value = Cbs3_ManAddNode(pSol, iObj, 0, 0);
    assert( Gia_ObjIsAnd(pObj) );
    Lit0 = Cbs3_ManToSolver2_rec( pSol, p, Gia_ObjFaninId0(pObj, iObj), Depth - Gia_ObjFaninC0(pObj) );
    Lit1 = Cbs3_ManToSolver2_rec( pSol, p, Gia_ObjFaninId1(pObj, iObj), Depth - Gia_ObjFaninC1(pObj) );
    return pObj->Value = Cbs3_ManAddNode(pSol, iObj, Lit0 ^ Gia_ObjFaninC0(pObj), Lit1 ^ Gia_ObjFaninC1(pObj));
}
static inline int Cbs3_ManToSolver2( Cbs3_Man_t * pSol, Gia_Man_t * p, Gia_Obj_t * pRoot, int nRestarts, int Depth )
{
    //abctime clk = Abc_Clock();
    assert( Gia_ObjIsCo(pRoot) );
    Cbs3_ManReset( pSol );
    Gia_ManIncrementTravId( p );
    Cbs3_ManToSolver2_rec( pSol, p, Gia_ObjFaninId0p(p, pRoot), Depth );
    Cbs3_ManGrow( pSol );
    Cbs3_ManPrepare( pSol );
    Cbs3_ActReset( pSol );
    //pSol->timeSatLoad += Abc_Clock() - clk;
    return Cbs3_ManSolve( pSol, Gia_ObjFanin0Copy(pRoot), nRestarts );
}


/**Function*************************************************************

  Synopsis    [Procedure to test the new SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Cbs3_ManSolveMiterNc( Gia_Man_t * pAig, int nConfs, int nRestarts, Vec_Str_t ** pvStatus, int fVerbose )
{
    extern void Cec_ManSatAddToStore( Vec_Int_t * vCexStore, Vec_Int_t * vCex, int Out );
    Cbs3_Man_t * p; 
    Vec_Int_t * vCex, * vVisit, * vCexStore;
    Vec_Str_t * vStatus;
    Gia_Obj_t * pRoot; 
    int i, status; // 1 = unsat, 0 = sat, -1 = undec
    abctime clk, clkTotal = Abc_Clock();
    //assert( Gia_ManRegNum(pAig) == 0 );
    Gia_ManCreateRefs( pAig );
    //Gia_ManLevelNum( pAig );
    // create logic network
    p = Cbs3_ManAlloc( pAig );
    p->Pars.nBTLimit = nConfs;
    p->Pars.nRestLimit = nRestarts;
    // create resulting data-structures
    vStatus   = Vec_StrAlloc( Gia_ManPoNum(pAig) );
    vCexStore = Vec_IntAlloc( 10000 );
    vVisit    = Vec_IntAlloc( 100 );
    vCex      = Cbs3_ReadModel( p );
    // solve for each output
    Gia_ManForEachCo( pAig, pRoot, i )
    {
        if ( Gia_ObjIsConst0(Gia_ObjFanin0(pRoot)) )
        {
            Vec_IntClear( vCex );
            Vec_StrPush( vStatus, (char)(!Gia_ObjFaninC0(pRoot)) );
            if ( Gia_ObjFaninC0(pRoot) ) // const1
                Cec_ManSatAddToStore( vCexStore, vCex, i ); // trivial counter-example
            continue;
        }
        clk = Abc_Clock();
        status = Cbs3_ManToSolver2( p, pAig, pRoot, p->Pars.nRestLimit, 10000 );
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
        //Gia_SatVerifyPattern( pAig, pRoot, vCex, vVisit );
        Cec_ManSatAddToStore( vCexStore, vCex, i );
        p->timeSatSat += Abc_Clock() - clk;
    }
    Vec_IntFree( vVisit );
    p->nSatTotal = Gia_ManPoNum(pAig);
    p->timeTotal = Abc_Clock() - clkTotal;
    if ( fVerbose )
        Cbs3_ManSatPrintStats( p );
    if ( fVerbose )
    {
        printf( "Prop1 = %d.  Prop2 = %d.  Prop3 = %d.  ClaConf = %d.   FailJ = %d.  FailC = %d.   ", p->nPropCalls[0], p->nPropCalls[1], p->nPropCalls[2], p->nClauseConf, p->nFails[0], p->nFails[1] );
        printf( "Mem usage %.2f MB.\n", 1.0*Cbs3_ManMemory(p)/(1<<20) );
        //Abc_PrintTime( 1, "JFront", p->timeJFront );
        //Abc_PrintTime( 1, "Loading", p->timeSatLoad );
        //printf( "Decisions = %d.\n", p->nDecs );
    }
    Cbs3_ManStop( p );
    *pvStatus = vStatus;
    return vCexStore;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

