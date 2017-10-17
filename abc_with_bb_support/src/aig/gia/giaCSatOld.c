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

typedef struct Cbs0_Par_t_ Cbs0_Par_t;
struct Cbs0_Par_t_
{
    // conflict limits
    int           nBTLimit;     // limit on the number of conflicts
    int           nJustLimit;   // limit on the size of justification queue
    // current parameters
    int           nBTThis;      // number of conflicts
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

typedef struct Cbs0_Que_t_ Cbs0_Que_t;
struct Cbs0_Que_t_
{
    int           iHead;        // beginning of the queue
    int           iTail;        // end of the queue
    int           nSize;        // allocated size
    Gia_Obj_t **  pData;        // nodes stored in the queue
};
 
typedef struct Cbs0_Man_t_ Cbs0_Man_t;
struct Cbs0_Man_t_
{
    Cbs0_Par_t    Pars;         // parameters
    Gia_Man_t *   pAig;         // AIG manager
    Cbs0_Que_t    pProp;        // propagation queue
    Cbs0_Que_t    pJust;        // justification queue
    Vec_Int_t *   vModel;       // satisfying assignment
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

static inline int   Cbs0_VarIsAssigned( Gia_Obj_t * pVar )      { return pVar->fMark0;                        }
static inline void  Cbs0_VarAssign( Gia_Obj_t * pVar )          { assert(!pVar->fMark0); pVar->fMark0 = 1;    }
static inline void  Cbs0_VarUnassign( Gia_Obj_t * pVar )        { assert(pVar->fMark0);  pVar->fMark0 = 0; pVar->fMark1 = 0;         }
static inline int   Cbs0_VarValue( Gia_Obj_t * pVar )           { assert(pVar->fMark0);  return pVar->fMark1; }
static inline void  Cbs0_VarSetValue( Gia_Obj_t * pVar, int v ) { assert(pVar->fMark0);  pVar->fMark1 = v;    }
static inline int   Cbs0_VarIsJust( Gia_Obj_t * pVar )          { return Gia_ObjIsAnd(pVar) && !Cbs0_VarIsAssigned(Gia_ObjFanin0(pVar)) && !Cbs0_VarIsAssigned(Gia_ObjFanin1(pVar)); } 
static inline int   Cbs0_VarFanin0Value( Gia_Obj_t * pVar )     { return !Cbs0_VarIsAssigned(Gia_ObjFanin0(pVar)) ? 2 : (Cbs0_VarValue(Gia_ObjFanin0(pVar)) ^ Gia_ObjFaninC0(pVar)); }
static inline int   Cbs0_VarFanin1Value( Gia_Obj_t * pVar )     { return !Cbs0_VarIsAssigned(Gia_ObjFanin1(pVar)) ? 2 : (Cbs0_VarValue(Gia_ObjFanin1(pVar)) ^ Gia_ObjFaninC1(pVar)); }

#define Cbs0_QueForEachEntry( Que, pObj, i )                    \
    for ( i = (Que).iHead; (i < (Que).iTail) && ((pObj) = (Que).pData[i]); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sets default values of the parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cbs0_SetDefaultParams( Cbs0_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Cbs0_Par_t) );
    pPars->nBTLimit    =  1000;   // limit on the number of conflicts
    pPars->nJustLimit  =   100;   // limit on the size of justification queue
    pPars->fUseHighest =     1;   // use node with the highest ID
    pPars->fUseLowest  =     0;   // use node with the highest ID
    pPars->fUseMaxFF   =     0;   // use node with the largest fanin fanout
    pPars->fVerbose    =     1;   // print detailed statistics
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cbs0_Man_t * Cbs0_ManAlloc()
{
    Cbs0_Man_t * p;
    p = ABC_CALLOC( Cbs0_Man_t, 1 );
    p->pProp.nSize = p->pJust.nSize = 10000;
    p->pProp.pData = ABC_ALLOC( Gia_Obj_t *, p->pProp.nSize );
    p->pJust.pData = ABC_ALLOC( Gia_Obj_t *, p->pJust.nSize );
    p->vModel = Vec_IntAlloc( 1000 );
    Cbs0_SetDefaultParams( &p->Pars );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cbs0_ManStop( Cbs0_Man_t * p )
{
    Vec_IntFree( p->vModel );
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
Vec_Int_t * Cbs0_ReadModel( Cbs0_Man_t * p )
{
    return p->vModel;
}




/**Function*************************************************************

  Synopsis    [Returns 1 if the solver is out of limits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs0_ManCheckLimits( Cbs0_Man_t * p )
{
    return p->Pars.nJustThis > p->Pars.nJustLimit || p->Pars.nBTThis > p->Pars.nBTLimit;
}

/**Function*************************************************************

  Synopsis    [Saves the satisfying assignment as an array of literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs0_ManSaveModel( Cbs0_Man_t * p, Vec_Int_t * vCex )
{
    Gia_Obj_t * pVar;
    int i;
    Vec_IntClear( vCex );
    p->pProp.iHead = 0;
    Cbs0_QueForEachEntry( p->pProp, pVar, i )
        if ( Gia_ObjIsCi(pVar) )
//            Vec_IntPush( vCex, Abc_Var2Lit(Gia_ObjId(p->pAig,pVar), !Cbs0_VarValue(pVar)) );
            Vec_IntPush( vCex, Abc_Var2Lit(Gia_ObjCioId(pVar), !Cbs0_VarValue(pVar)) );
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs0_QueIsEmpty( Cbs0_Que_t * p )
{
    return p->iHead == p->iTail;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs0_QuePush( Cbs0_Que_t * p, Gia_Obj_t * pObj )
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
static inline int Cbs0_QueHasNode( Cbs0_Que_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pTemp;
    int i;
    Cbs0_QueForEachEntry( *p, pTemp, i )
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
static inline void Cbs0_QueStore( Cbs0_Que_t * p, int * piHeadOld, int * piTailOld )
{
    int i;
    *piHeadOld = p->iHead;
    *piTailOld = p->iTail;
    for ( i = *piHeadOld; i < *piTailOld; i++ )
        Cbs0_QuePush( p, p->pData[i] );
    p->iHead = *piTailOld;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs0_QueRestore( Cbs0_Que_t * p, int iHeadOld, int iTailOld )
{
    p->iHead = iHeadOld;
    p->iTail = iTailOld;
}


/**Function*************************************************************

  Synopsis    [Max number of fanins fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs0_VarFaninFanoutMax( Cbs0_Man_t * p, Gia_Obj_t * pObj )
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
static inline Gia_Obj_t * Cbs0_ManDecideHighest( Cbs0_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMax = NULL;
    int i;
    Cbs0_QueForEachEntry( p->pJust, pObj, i )
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
static inline Gia_Obj_t * Cbs0_ManDecideLowest( Cbs0_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMin = NULL;
    int i;
    Cbs0_QueForEachEntry( p->pJust, pObj, i )
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
static inline Gia_Obj_t * Cbs0_ManDecideMaxFF( Cbs0_Man_t * p )
{
    Gia_Obj_t * pObj, * pObjMax = NULL;
    int i, iMaxFF = 0, iCurFF;
    assert( p->pAig->pRefs != NULL );
    Cbs0_QueForEachEntry( p->pJust, pObj, i )
    {
        iCurFF = Cbs0_VarFaninFanoutMax( p, pObj );
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
static inline void Cbs0_ManCancelUntil( Cbs0_Man_t * p, int iBound )
{
    Gia_Obj_t * pVar;
    int i;
    assert( iBound <= p->pProp.iTail );
    p->pProp.iHead = iBound;
    Cbs0_QueForEachEntry( p->pProp, pVar, i )
        Cbs0_VarUnassign( pVar );
    p->pProp.iTail = iBound;
}

/**Function*************************************************************

  Synopsis    [Assigns the variables a value.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cbs0_ManAssign( Cbs0_Man_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pObjR = Gia_Regular(pObj);
    assert( Gia_ObjIsCand(pObjR) );
    assert( !Cbs0_VarIsAssigned(pObjR) );
    Cbs0_VarAssign( pObjR );
    Cbs0_VarSetValue( pObjR, !Gia_IsComplement(pObj) );
    Cbs0_QuePush( &p->pProp, pObjR );
}

/**Function*************************************************************

  Synopsis    [Propagates a variable.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs0_ManPropagateOne( Cbs0_Man_t * p, Gia_Obj_t * pVar )
{
    int Value0, Value1;
    assert( !Gia_IsComplement(pVar) );
    assert( Cbs0_VarIsAssigned(pVar) );
    if ( Gia_ObjIsCi(pVar) )
        return 0;
    assert( Gia_ObjIsAnd(pVar) );
    Value0 = Cbs0_VarFanin0Value(pVar);
    Value1 = Cbs0_VarFanin1Value(pVar);
    if ( Cbs0_VarValue(pVar) )
    { // value is 1
        if ( Value0 == 0 || Value1 == 0 ) // one is 0
            return 1;
        if ( Value0 == 2 ) // first is unassigned
            Cbs0_ManAssign( p, Gia_ObjChild0(pVar) );
        if ( Value1 == 2 ) // first is unassigned
            Cbs0_ManAssign( p, Gia_ObjChild1(pVar) );
        return 0;
    }
    // value is 0
    if ( Value0 == 0 || Value1 == 0 ) // one is 0
        return 0;
    if ( Value0 == 1 && Value1 == 1 ) // both are 1
        return 1;
    if ( Value0 == 1 || Value1 == 1 ) // one is 1 
    {
        if ( Value0 == 2 ) // first is unassigned
            Cbs0_ManAssign( p, Gia_Not(Gia_ObjChild0(pVar)) );
        if ( Value1 == 2 ) // first is unassigned
            Cbs0_ManAssign( p, Gia_Not(Gia_ObjChild1(pVar)) );
        return 0;
    }
    assert( Cbs0_VarIsJust(pVar) );
    assert( !Cbs0_QueHasNode( &p->pJust, pVar ) );
    Cbs0_QuePush( &p->pJust, pVar );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Propagates a variable.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cbs0_ManPropagateTwo( Cbs0_Man_t * p, Gia_Obj_t * pVar )
{
    int Value0, Value1;
    assert( !Gia_IsComplement(pVar) );
    assert( Gia_ObjIsAnd(pVar) );
    assert( Cbs0_VarIsAssigned(pVar) );
    assert( !Cbs0_VarValue(pVar) );
    Value0 = Cbs0_VarFanin0Value(pVar);
    Value1 = Cbs0_VarFanin1Value(pVar);
    // value is 0
    if ( Value0 == 0 || Value1 == 0 ) // one is 0
        return 0;
    if ( Value0 == 1 && Value1 == 1 ) // both are 1
        return 1;
    assert( Value0 == 1 || Value1 == 1 );
    if ( Value0 == 2 ) // first is unassigned
        Cbs0_ManAssign( p, Gia_Not(Gia_ObjChild0(pVar)) );
    if ( Value1 == 2 ) // first is unassigned
        Cbs0_ManAssign( p, Gia_Not(Gia_ObjChild1(pVar)) );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Propagates all variables.]

  Description [Returns 1 if conflict; 0 if no conflict.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cbs0_ManPropagate( Cbs0_Man_t * p )
{
    Gia_Obj_t * pVar;
    int i, k;
    while ( 1 )
    {
        Cbs0_QueForEachEntry( p->pProp, pVar, i )
        {
            if ( Cbs0_ManPropagateOne( p, pVar ) )
                return 1;
        }
        p->pProp.iHead = p->pProp.iTail;
        k = p->pJust.iHead;
        Cbs0_QueForEachEntry( p->pJust, pVar, i )
        {
            if ( Cbs0_VarIsJust( pVar ) )
                p->pJust.pData[k++] = pVar;
            else if ( Cbs0_ManPropagateTwo( p, pVar ) )
                return 1;
        }
        if ( k == p->pJust.iTail )
            break;
        p->pJust.iTail = k;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Solve the problem recursively.]

  Description [Returns 1 if unsat or undecided; 0 if satisfiable.]
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
int Cbs0_ManSolve_rec( Cbs0_Man_t * p )
{
    Gia_Obj_t * pVar = NULL, * pDecVar;
    int iPropHead, iJustHead, iJustTail;
    // propagate assignments
    assert( !Cbs0_QueIsEmpty(&p->pProp) );
    if ( Cbs0_ManPropagate( p ) )
        return 1;
    // check for satisfying assignment
    assert( Cbs0_QueIsEmpty(&p->pProp) );
    if ( Cbs0_QueIsEmpty(&p->pJust) )
        return 0;
    // quit using resource limits
    p->Pars.nJustThis = Abc_MaxInt( p->Pars.nJustThis, p->pJust.iTail - p->pJust.iHead );
    if ( Cbs0_ManCheckLimits( p ) )
        return 0;
    // remember the state before branching
    iPropHead = p->pProp.iHead;
    Cbs0_QueStore( &p->pJust, &iJustHead, &iJustTail );
    // find the decision variable
    if ( p->Pars.fUseHighest )
        pVar = Cbs0_ManDecideHighest( p );
    else if ( p->Pars.fUseLowest )
        pVar = Cbs0_ManDecideLowest( p );
    else if ( p->Pars.fUseMaxFF )
        pVar = Cbs0_ManDecideMaxFF( p );
    else assert( 0 );
    assert( Cbs0_VarIsJust( pVar ) );
    // chose decision variable using fanout count
    if ( Gia_ObjRefNum(p->pAig, Gia_ObjFanin0(pVar)) > Gia_ObjRefNum(p->pAig, Gia_ObjFanin1(pVar)) )
        pDecVar = Gia_Not(Gia_ObjChild0(pVar));
    else
        pDecVar = Gia_Not(Gia_ObjChild1(pVar));
    // decide on first fanin
    Cbs0_ManAssign( p, pDecVar );
    if ( !Cbs0_ManSolve_rec( p ) )
        return 0;
    Cbs0_ManCancelUntil( p, iPropHead );
    Cbs0_QueRestore( &p->pJust, iJustHead, iJustTail );
    // decide on second fanin
    Cbs0_ManAssign( p, Gia_Not(pDecVar) );
    if ( !Cbs0_ManSolve_rec( p ) )
        return 0;
    p->Pars.nBTThis++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Looking for a satisfying assignment of the node.]

  Description [Assumes that each node has flag pObj->fMark0 set to 0.
  Returns 1 if unsatisfiable, 0 if satisfiable, and -1 if undecided.
  The node may be complemented. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cbs0_ManSolve( Cbs0_Man_t * p, Gia_Obj_t * pObj )
{
    int RetValue;
    assert( !p->pProp.iHead && !p->pProp.iTail );
    assert( !p->pJust.iHead && !p->pJust.iTail );
    p->Pars.nBTThis = p->Pars.nJustThis = 0;
    Cbs0_ManAssign( p, pObj );
    RetValue = Cbs0_ManSolve_rec( p );
    if ( RetValue == 0 && !Cbs0_ManCheckLimits(p) )
        Cbs0_ManSaveModel( p, p->vModel );
    Cbs0_ManCancelUntil( p, 0 );
    p->pJust.iHead = p->pJust.iTail = 0;
    p->Pars.nBTTotal += p->Pars.nBTThis;
    p->Pars.nJustTotal = Abc_MaxInt( p->Pars.nJustTotal, p->Pars.nJustThis );
    if ( Cbs0_ManCheckLimits( p ) )
        RetValue = -1;
//    printf( "Outcome = %2d.  Confs = %6d.  Decision level max = %3d.\n", 
//        RetValue, p->Pars.nBTThis, p->DecLevelMax );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Prints statistics of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cbs0_ManSatPrintStats( Cbs0_Man_t * p )
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
Vec_Int_t * Cbs_ManSolveMiter( Gia_Man_t * pAig, int nConfs, Vec_Str_t ** pvStatus, int fVerbose )
{
    extern void Cec_ManSatAddToStore( Vec_Int_t * vCexStore, Vec_Int_t * vCex, int Out );
    Cbs0_Man_t * p; 
    Vec_Int_t * vCex, * vVisit, * vCexStore;
    Vec_Str_t * vStatus;
    Gia_Obj_t * pRoot; 
    int i, status;
    abctime clk, clkTotal = Abc_Clock();
    assert( Gia_ManRegNum(pAig) == 0 );
    // prepare AIG
    Gia_ManCreateRefs( pAig );
    Gia_ManCleanMark0( pAig );
    Gia_ManCleanMark1( pAig );
    // create logic network
    p = Cbs0_ManAlloc();
    p->Pars.nBTLimit = nConfs;
    p->pAig   = pAig;
    // create resulting data-structures
    vStatus   = Vec_StrAlloc( Gia_ManPoNum(pAig) );
    vCexStore = Vec_IntAlloc( 10000 );
    vVisit    = Vec_IntAlloc( 100 );
    vCex      = Cbs0_ReadModel( p );
    // solve for each output
    Gia_ManForEachCo( pAig, pRoot, i )
    {
        Vec_IntClear( vCex );
        if ( Gia_ObjIsConst0(Gia_ObjFanin0(pRoot)) )
        {
            if ( Gia_ObjFaninC0(pRoot) )
            {
                printf( "Constant 1 output of SRM!!!\n" );
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
        status = Cbs0_ManSolve( p, Gia_ObjChild0(pRoot) );
/*
        if ( status == -1 )
        {
            p->Pars.fUseHighest = 0;
            p->Pars.fUseLowest  = 1;
            status = Cbs0_ManSolve( p, Gia_ObjChild0(pRoot) );
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
        Cbs0_ManSatPrintStats( p );
    Cbs0_ManStop( p );
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

