/**CFile****************************************************************

  FileName    [bmcFault.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Checking for functional faults.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcFault.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define FFTEST_MAX_VARS 2
#define FFTEST_MAX_PARS 8

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ParFfSetDefault( Bmc_ParFf_t * p )
{
    memset( p, 0, sizeof(Bmc_ParFf_t) );
    p->pFileName     =  NULL; 
    p->Algo          =     0; 
    p->fStartPats    =     0; 
    p->nTimeOut      =     0; 
    p->nIterCheck    =     0;
    p->fBasic        =     0; 
    p->fFfOnly       =     0;
    p->fDump         =     0; 
    p->fDumpUntest   =     0; 
    p->fVerbose      =     0; 
}

/**Function*************************************************************

  Synopsis    [Adds a general cardinality constraint in terms of vVars.]

  Description [Strict is "exactly K out of N". Non-strict is 
  "less or equal than K out of N".]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cnf_AddSorder( sat_solver * p, int * pVars, int i, int k, int * pnVars )
{
    int iVar1 = (*pnVars)++;
    int iVar2 = (*pnVars)++;
    sat_solver_add_and( p, iVar1, pVars[i], pVars[k], 1, 1, 1 );
    sat_solver_add_and( p, iVar2, pVars[i], pVars[k], 0, 0, 0 );
    pVars[i] = iVar1;
    pVars[k] = iVar2;
}
static inline void Cnf_AddCardinConstrMerge( sat_solver * p, int * pVars, int lo, int hi, int r, int * pnVars )
{
    int i, step = r * 2;
    if ( step < hi - lo )
    {
        Cnf_AddCardinConstrMerge( p, pVars, lo, hi-r, step, pnVars );
        Cnf_AddCardinConstrMerge( p, pVars, lo+r, hi, step, pnVars );
        for ( i = lo+r; i < hi-r; i += step )
            Cnf_AddSorder( p, pVars, i, i+r, pnVars );
    }
}
static inline void Cnf_AddCardinConstrRange( sat_solver * p, int * pVars, int lo, int hi, int * pnVars )
{
    if ( hi - lo >= 1 )
    {
        int i, mid = lo + (hi - lo) / 2;
        for ( i = lo; i <= mid; i++ )
            Cnf_AddSorder( p, pVars, i, i + (hi - lo + 1) / 2, pnVars );
        Cnf_AddCardinConstrRange( p, pVars, lo, mid, pnVars );
        Cnf_AddCardinConstrRange( p, pVars, mid+1, hi, pnVars );
        Cnf_AddCardinConstrMerge( p, pVars, lo, hi, 1, pnVars );
    }
}
void Cnf_AddCardinConstrPairWise( sat_solver * p, Vec_Int_t * vVars, int K, int fStrict )
{
    int nVars = sat_solver_nvars(p);
    int nSizeOld = Vec_IntSize(vVars);
    int i, iVar, nSize, Lit, nVarsOld;
    assert( nSizeOld >= 2 );
    // check that vars are ok
    Vec_IntForEachEntry( vVars, iVar, i )
        assert( iVar >= 0 && iVar < nVars );
    // make the size a degree of two
    for ( nSize = 1; nSize < nSizeOld; nSize *= 2 );
    // extend
    sat_solver_setnvars( p, nVars + 1 + nSize * nSize / 2 );
    if ( nSize != nSizeOld )
    {
        // fill in with const0
        Vec_IntFillExtra( vVars, nSize, nVars );
        // add const0 variable
        Lit = Abc_Var2Lit( nVars++, 1 );
        if ( !sat_solver_addclause( p, &Lit, &Lit+1 ) )
            assert( 0 );
    }
    // construct recursively
    nVarsOld = nVars;
    Cnf_AddCardinConstrRange( p, Vec_IntArray(vVars), 0, nSize - 1, &nVars );
    // add final constraint
    assert( K > 0 && K < nSizeOld );
    Lit = Abc_Var2Lit( Vec_IntEntry(vVars, K), 1 );
    if ( !sat_solver_addclause( p, &Lit, &Lit+1 ) )
        assert( 0 );
    if ( fStrict )
    {
        Lit = Abc_Var2Lit( Vec_IntEntry(vVars, K-1), 0 );
        if ( !sat_solver_addclause( p, &Lit, &Lit+1 ) )
            assert( 0 );
    }
    // return to the old size
    Vec_IntShrink( vVars, 0 ); // make it unusable
    //printf( "The %d input sorting network contains %d sorters.\n", nSize, (nVars - nVarsOld)/2 );
}

/**Function*************************************************************

  Synopsis    [Adds a general cardinality constraint in terms of vVars.]

  Description [Strict is "exactly K out of N". Non-strict is 
  "less or equal than K out of N".]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cnf_AddCardinVar( int Level, int Base, Vec_Int_t * vVars, int k )
{
    return Level ? Base + k : Vec_IntEntry( vVars, k );
}
void Cnf_AddCardinConstrGeneral( sat_solver * p, Vec_Int_t * vVars, int K, int fStrict )
{
    int i, k, iCur, iNext, iVar, Lit;
    int nVars = sat_solver_nvars(p);
    int nBits = Vec_IntSize(vVars);
    Vec_IntForEachEntry( vVars, iVar, i )
        assert( iVar >= 0 && iVar < nVars );
    sat_solver_setnvars( p, nVars + nBits * nBits );
    for ( i = 0; i < nBits; i++ )
    {
        iCur = nVars + nBits * (i-1);
        iNext = nVars + nBits * i;
        if ( i & 1 )
            sat_solver_add_buffer( p, iNext, Cnf_AddCardinVar(i, iCur, vVars, 0), 0 );
        for ( k = i & 1; k + 1 < nBits; k += 2 )
        {
            sat_solver_add_and( p, iNext+k  , Cnf_AddCardinVar(i, iCur, vVars, k), Cnf_AddCardinVar(i, iCur, vVars, k+1), 1, 1, 1 );
            sat_solver_add_and( p, iNext+k+1, Cnf_AddCardinVar(i, iCur, vVars, k), Cnf_AddCardinVar(i, iCur, vVars, k+1), 0, 0, 0 );
        }
        if ( k == nBits - 1 )
            sat_solver_add_buffer( p, iNext + nBits-1, Cnf_AddCardinVar(i, iCur, vVars, nBits-1), 0 );
    }
    // add final constraint
    assert( K > 0 && K < nBits );
    Lit = Abc_Var2Lit( nVars + nBits * (nBits - 1) + K, 1 );
    if ( !sat_solver_addclause( p, &Lit, &Lit+1 ) )
        assert( 0 );
    if ( fStrict )
    {
        Lit = Abc_Var2Lit( nVars + nBits * (nBits - 1) + K - 1, 0 );
        if ( !sat_solver_addclause( p, &Lit, &Lit+1 ) )
            assert( 0 );
    }
}
/**Function*************************************************************

  Synopsis    [Add constraint that no more than 1 variable is 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cnf_AddCardinConstr( sat_solver * p, Vec_Int_t * vVars )
{
    int i, k, pLits[2], iVar, nVars = sat_solver_nvars(p);
    Vec_IntForEachEntry( vVars, iVar, i )
        assert( iVar >= 0 && iVar < nVars );
    iVar = nVars;
    sat_solver_setnvars( p, nVars + Vec_IntSize(vVars) - 1 );
    while ( Vec_IntSize(vVars) > 1 )
    {
        for ( k = i = 0; i < Vec_IntSize(vVars)/2; i++ )
        {
            pLits[0] = Abc_Var2Lit( Vec_IntEntry(vVars, 2*i), 1 );
            pLits[1] = Abc_Var2Lit( Vec_IntEntry(vVars, 2*i+1), 1 );
            sat_solver_addclause( p, pLits, pLits + 2 );
            sat_solver_add_and( p, iVar, Vec_IntEntry(vVars, 2*i), Vec_IntEntry(vVars, 2*i+1), 1, 1, 1 );
            Vec_IntWriteEntry( vVars, k++, iVar++ );
        }
        if ( Vec_IntSize(vVars) & 1 )
            Vec_IntWriteEntry( vVars, k++, Vec_IntEntryLast(vVars) );
        Vec_IntShrink( vVars, k );
    }
    return iVar;
}
void Cnf_AddCardinConstrTest()
{
    int i, status, Count = 1, nVars = 8;
    Vec_Int_t * vVars = Vec_IntStartNatural( nVars );
    sat_solver * pSat = sat_solver_new();
    sat_solver_setnvars( pSat, nVars );
    //Cnf_AddCardinConstr( pSat, vVars );
    Cnf_AddCardinConstrPairWise( pSat, vVars, 2, 1 );
    while ( 1 )
    {
        status = sat_solver_solve( pSat, NULL, NULL, 0, 0, 0, 0 );
        if ( status != l_True )
            break;
        Vec_IntClear( vVars );
        printf( "%3d : ", Count++ );
        for ( i = 0; i < nVars; i++ )
        {
            Vec_IntPush( vVars, Abc_Var2Lit(i, sat_solver_var_value(pSat, i)) );
            printf( "%d", sat_solver_var_value(pSat, i) );
        }
        printf( "\n" );
        status = sat_solver_addclause( pSat, Vec_IntArray(vVars), Vec_IntArray(vVars) + Vec_IntSize(vVars) );
        if ( status == 0 )
            break;
    }
    sat_solver_delete( pSat );
    Vec_IntFree( vVars );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Cnf_Dat_t * Cnf_DeriveGiaRemapped( Gia_Man_t * p )
{
    Cnf_Dat_t * pCnf;
    Aig_Man_t * pAig = Gia_ManToAigSimple( p );
    pAig->nRegs = 0;
    pCnf = Cnf_Derive( pAig, Aig_ManCoNum(pAig) );
    Aig_ManStop( pAig );
    return pCnf;
//    return (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, 0, 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cnf_DataLiftGia( Cnf_Dat_t * p, Gia_Man_t * pGia, int nVarsPlus )
{
    Gia_Obj_t * pObj;
    int v;
    Gia_ManForEachObj( pGia, pObj, v )
        if ( p->pVarNums[Gia_ObjId(pGia, pObj)] >= 0 )
            p->pVarNums[Gia_ObjId(pGia, pObj)] += nVarsPlus;
    for ( v = 0; v < p->nLiterals; v++ )
        p->pClauses[0][v] += 2*nVarsPlus;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFaultUnfold( Gia_Man_t * p, int fUseMuxes, int fFfOnly )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, iCtrl, iThis;
    pNew = Gia_ManStart( (2 + 3 * fUseMuxes) * Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    // add first timeframe
    Gia_ManForEachRo( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);
    // add second timeframe
    Gia_ManForEachRo( p, pObj, i )
        pObj->Value = Gia_ObjRoToRi(p, pObj)->Value;
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    if ( fFfOnly )
    {
        Gia_ManForEachAnd( p, pObj, i )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachPo( p, pObj, i )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
        Gia_ManForEachRi( p, pObj, i )
        {
            iCtrl = Gia_ManAppendCi(pNew);
            iThis = Gia_ObjFanin0Copy(pObj);
            if ( fUseMuxes )
                pObj->Value = Gia_ManHashMux( pNew, iCtrl, pObj->Value, iThis );
            else
                pObj->Value = iThis;
            pObj->Value = Gia_ManAppendCo( pNew, pObj->Value );
        }
    }
    else
    {
        Gia_ManForEachAnd( p, pObj, i )
        {
            iCtrl = Gia_ManAppendCi(pNew);
            iThis = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
            if ( fUseMuxes )
                pObj->Value = Gia_ManHashMux( pNew, iCtrl, pObj->Value, iThis );
            else
                pObj->Value = iThis;
        }
        Gia_ManForEachCo( p, pObj, i )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    assert( Gia_ManPiNum(pNew) == Gia_ManRegNum(p) + 2 * Gia_ManPiNum(p) + (fFfOnly ? Gia_ManRegNum(p) : Gia_ManAndNum(p)) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManStuckAtUnfold( Gia_Man_t * p, Vec_Int_t * vMap )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, iFuncVars = 0;
    pNew = Gia_ManStart( 3 * Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
    {
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );

        if ( Vec_IntEntry(vMap, iFuncVars++) )
            pObj->Value = Gia_ManHashAnd( pNew, Abc_LitNot(Gia_ManAppendCi(pNew)), pObj->Value );
        else
            Gia_ManAppendCi(pNew);

        if ( Vec_IntEntry(vMap, iFuncVars++) )
            pObj->Value = Gia_ManHashOr( pNew, Gia_ManAppendCi(pNew), pObj->Value );
        else
            Gia_ManAppendCi(pNew);
    }
    assert( iFuncVars == Vec_IntSize(vMap) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    assert( Gia_ManPiNum(pNew) == Gia_ManCiNum(p) + 2 * Gia_ManAndNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFlipUnfold( Gia_Man_t * p, Vec_Int_t * vMap )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, iFuncVars = 0;
    pNew = Gia_ManStart( 4 * Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
    {
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( Vec_IntEntry(vMap, iFuncVars++) )
            pObj->Value = Gia_ManHashXor( pNew, Gia_ManAppendCi(pNew), pObj->Value );
        else
            Gia_ManAppendCi(pNew);
    }
    assert( iFuncVars == Vec_IntSize(vMap) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    assert( Gia_ManPiNum(pNew) == Gia_ManCiNum(p) + Gia_ManAndNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFOFUnfold( Gia_Man_t * p, Vec_Int_t * vMap )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, iCtrl0, iCtrl1, iCtrl2, iCtrl3, iMuxA, iMuxB, iFuncVars = 0;
    pNew = Gia_ManStart( 9 * Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Vec_IntEntry(vMap, iFuncVars++) )
            iCtrl0 = Gia_ManAppendCi(pNew);
        else
            iCtrl0 = 0, Gia_ManAppendCi(pNew);

        if ( Vec_IntEntry(vMap, iFuncVars++) )
            iCtrl1 = Gia_ManAppendCi(pNew);
        else
            iCtrl1 = 0, Gia_ManAppendCi(pNew);

        if ( Vec_IntEntry(vMap, iFuncVars++) )
            iCtrl2 = Gia_ManAppendCi(pNew);
        else
            iCtrl2 = 0, Gia_ManAppendCi(pNew);

        if ( Vec_IntEntry(vMap, iFuncVars++) )
            iCtrl3 = Gia_ManAppendCi(pNew);
        else
            iCtrl3 = 0, Gia_ManAppendCi(pNew);

        if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
            iCtrl0 = Abc_LitNot(iCtrl0);
        else if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
            iCtrl1 = Abc_LitNot(iCtrl1);
        else if ( Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) )
            iCtrl2 = Abc_LitNot(iCtrl2);
        else //if ( !Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) )
            iCtrl3 = Abc_LitNot(iCtrl3);

        iMuxA       = Gia_ManHashMux( pNew, Gia_ObjFanin0(pObj)->Value, iCtrl1, iCtrl0 );
        iMuxB       = Gia_ManHashMux( pNew, Gia_ObjFanin0(pObj)->Value, iCtrl3, iCtrl2 );
        pObj->Value = Gia_ManHashMux( pNew, Gia_ObjFanin1(pObj)->Value, iMuxB,  iMuxA );
    }
    assert( iFuncVars == Vec_IntSize(vMap) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    assert( Gia_ManPiNum(pNew) == Gia_ManCiNum(p) + 4 * Gia_ManAndNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_FormStrCount( char * pStr, int * pnVars, int * pnPars )
{
    int i, Counter = 0;
    if ( pStr[0] != '(' )
    {
        printf( "The first symbol should be the opening parenthesis \"(\".\n" );
        return 1;
    }
    if ( pStr[strlen(pStr)-1] != ')' )
    {
        printf( "The last symbol should be the closing parenthesis \")\".\n" );
        return 1;
    }
    for ( i = 0; pStr[i]; i++ )
        if (  pStr[i] == '(' )
            Counter++;
        else if (  pStr[i] == ')' )
            Counter--;
    if ( Counter != 0 )
    {
        printf( "The number of opening and closing parentheses is not equal.\n" );
        return 1;
    }
    *pnVars = 0;
    *pnPars = 0;
    for ( i = 0; pStr[i]; i++ )
    {
        if ( pStr[i] >= 'a' && pStr[i] <= 'b' )
            *pnVars = Abc_MaxInt( *pnVars, pStr[i] - 'a' + 1 );
        else if ( pStr[i] >= 'p' && pStr[i] <= 's' )
            *pnPars = Abc_MaxInt( *pnPars, pStr[i] - 'p' + 1 );
        else if ( pStr[i] == '(' || pStr[i] == ')' )
        {}
        else if ( pStr[i] == '&' || pStr[i] == '|' || pStr[i] == '^' )
        {}
        else if ( pStr[i] == '?' || pStr[i] == ':' )
        {}
        else if ( pStr[i] == '~' )
        {
            if ( pStr[i+1] < 'a' || pStr[i+1] > 'z' )
            {
                printf( "Expecting alphabetic symbol (instead of \"%c\") after negation (~)\n",  pStr[i+1] );
                return 1;
            }
        }
        else 
        {
            printf( "Unknown symbol (%c) in the formula (%s)\n", pStr[i], pStr );
            return 1;
        }
    }
    if ( *pnVars != FFTEST_MAX_VARS )
        { printf( "The number of input variables (%d) should be 2\n", *pnVars ); return 1; }
    if ( *pnPars < 1 && *pnPars > FFTEST_MAX_PARS )
        { printf( "The number of parameters should be between 1 and %d\n", *pnPars ); return 1; }
    return 0;
}
void Gia_FormStrTransform( char * pStr, char * pForm )
{
    int i, k;
    for ( k = i = 0; pForm[i]; i++ )
    {
        if ( pForm[i] == '~' )
        {
            i++;
            assert( pForm[i] >= 'a' && pForm[i] <= 'z' );
            pStr[k++] = 'A' + pForm[i] - 'a';
        }
        else
            pStr[k++] = pForm[i];
    }
    pStr[k] = 0; 
}   


/**Function*************************************************************

  Synopsis    [Implements fault model formula using functional/parameter vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Gia_ManFormulaEndToken( char * pForm )
{
    int Counter = 0;
    char * pThis;
    for ( pThis = pForm; *pThis; pThis++ )
    {
        assert( *pThis != '~' );
        if ( *pThis == '(' )
            Counter++;
        else if ( *pThis == ')' )
            Counter--;
        if ( Counter == 0 )
            return pThis + 1;
    }
    assert( 0 );
    return NULL;
}
int Gia_ManRealizeFormula_rec( Gia_Man_t * p, int * pVars, int * pPars, char * pBeg, char * pEnd, int nPars )
{
    int iFans[3], Oper = -1;
    char * pEndNew;
    if ( pBeg + 1 == pEnd )
    {
        if ( pBeg[0] >= 'a' && pBeg[0] <= 'b' )
            return pVars[pBeg[0] - 'a'];
        if ( pBeg[0] >= 'A' && pBeg[0] <= 'B' )
            return Abc_LitNot( pVars[pBeg[0] - 'A'] );
        if ( pBeg[0] >= 'p' && pBeg[0] <= 'w' ) // pqrstuvw
            return pPars[pBeg[0] - 'p'];
        if ( pBeg[0] >= 'P' && pBeg[0] <= 'W' )
            return Abc_LitNot( pPars[pBeg[0] - 'P'] );
        assert( 0 );
        return -1;
    }
    if ( pBeg[0] == '(' )
    {
        pEndNew = Gia_ManFormulaEndToken( pBeg );
        if ( pEndNew == pEnd )
        {
            assert( pBeg[0] == '(' );
            assert( pBeg[pEnd-pBeg-1] == ')' );
            return Gia_ManRealizeFormula_rec( p, pVars, pPars, pBeg + 1, pEnd - 1, nPars );
        }
    }
    // get first part
    pEndNew  = Gia_ManFormulaEndToken( pBeg );
    iFans[0] = Gia_ManRealizeFormula_rec( p, pVars, pPars, pBeg, pEndNew, nPars );
    Oper     = pEndNew[0];
    // get second part
    pBeg     = pEndNew + 1;
    pEndNew  = Gia_ManFormulaEndToken( pBeg );
    iFans[1] = Gia_ManRealizeFormula_rec( p, pVars, pPars, pBeg, pEndNew, nPars );
    // derive the formula
    if ( Oper == '&' )
        return Gia_ManHashAnd( p, iFans[0], iFans[1] );
    if ( Oper == '|' )
        return Gia_ManHashOr( p, iFans[0], iFans[1] );
    if ( Oper == '^' )
        return Gia_ManHashXor( p, iFans[0], iFans[1] );
    // get third part
    assert( Oper == '?' );
    assert( pEndNew[0] == ':' );
    pBeg     = pEndNew + 1;
    pEndNew  = Gia_ManFormulaEndToken( pBeg );
    iFans[2] = Gia_ManRealizeFormula_rec( p, pVars, pPars, pBeg, pEndNew, nPars );
    return Gia_ManHashMux( p, iFans[0], iFans[1], iFans[2] );
}
int Gia_ManRealizeFormula( Gia_Man_t * p, int * pVars, int * pPars, char * pStr, int nPars )
{
    return Gia_ManRealizeFormula_rec( p, pVars, pPars, pStr, pStr + strlen(pStr), nPars );
}
Gia_Man_t * Gia_ManFormulaUnfold( Gia_Man_t * p, char * pForm, int fFfOnly )
{
    char pStr[100];
    int Count = 0;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, k, iCtrl[FFTEST_MAX_PARS], iFans[FFTEST_MAX_VARS];
    int nVars, nPars;
    assert( strlen(pForm) < 100 );
    Gia_FormStrCount( pForm, &nVars, &nPars );
    assert( nVars == 2 );
    Gia_FormStrTransform( pStr, pForm );
    pNew = Gia_ManStart( 5 * Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    if ( fFfOnly )
    {
        Gia_ManCleanMark0( p );
        Gia_ManForEachRi( p, pObj, i )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
        Gia_ManForEachAnd( p, pObj, i )
        {
            if ( pObj->fMark0 )
            {
                for ( k = 0; k < nPars; k++ )
                    iCtrl[k] = Gia_ManAppendCi(pNew);
                iFans[0] = Gia_ObjFanin0Copy(pObj);
                iFans[1] = Gia_ObjFanin1Copy(pObj);
                pObj->Value = Gia_ManRealizeFormula( pNew, iFans, iCtrl, pStr, nPars );
                Count++;
            }
            else
                pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        }
        Gia_ManForEachRi( p, pObj, i )
            Gia_ObjFanin0(pObj)->fMark0 = 0;
    }
    else
    {
        Gia_ManForEachAnd( p, pObj, i )
        {
            for ( k = 0; k < nPars; k++ )
                iCtrl[k] = Gia_ManAppendCi(pNew);
            iFans[0] = Gia_ObjFanin0Copy(pObj);
            iFans[1] = Gia_ObjFanin1Copy(pObj);
            pObj->Value = Gia_ManRealizeFormula( pNew, iFans, iCtrl, pStr, nPars );
        }
    }
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    assert( Gia_ManPiNum(pNew) == Gia_ManCiNum(p) + nPars * (fFfOnly ? Count : Gia_ManAndNum(p)) );
//    if ( fUseFaults )
//        Gia_AigerWrite( pNew, "newfault.aig", 0, 0 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFaultCofactor( Gia_Man_t * p, Vec_Int_t * vValues )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachPi( p, pObj, i )
    {
        pObj->Value = Gia_ManAppendCi( pNew );
        if ( i < Vec_IntSize(vValues) )
            pObj->Value = Vec_IntEntry( vValues, i );
    }
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    assert( Gia_ManPiNum(pNew) == Gia_ManPiNum(p) );
    return pNew;

}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDumpTests( Vec_Int_t * vTests, int nIter, char * pFileName )
{
    FILE * pFile = fopen( pFileName, "wb" );
    int i, k, v, nVars = Vec_IntSize(vTests) / nIter;
    assert( Vec_IntSize(vTests) % nIter == 0 );
    for ( v = i = 0; i < nIter; i++, fprintf(pFile, "\n") )
        for ( k = 0; k < nVars; k++ )
            fprintf( pFile, "%d", Vec_IntEntry(vTests, v++) );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDumpTestsSimulate( Gia_Man_t * p, Vec_Int_t * vValues )
{
    Gia_Obj_t * pObj; int k;
    assert( Vec_IntSize(vValues) == Gia_ManCiNum(p) );
    Gia_ManConst0(p)->fMark0 = 0;
    Gia_ManForEachCi( p, pObj, k )
        pObj->fMark0 = Vec_IntEntry( vValues, k );
    Gia_ManForEachAnd( p, pObj, k )
        pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                       (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
    Gia_ManForEachCo( p, pObj, k )
        pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
    // collect flop input values
    Vec_IntClear( vValues );
    Gia_ManForEachRi( p, pObj, k )
        Vec_IntPush( vValues, pObj->fMark0 );
    assert( Vec_IntSize(vValues) == Gia_ManRegNum(p) );
}
void Gia_ManDumpTestsDelay( Vec_Int_t * vTests, int nIter, char * pFileName, Gia_Man_t * p )
{
    FILE * pFile = fopen( pFileName, "wb" );
    Vec_Int_t * vValues = Vec_IntAlloc( Gia_ManCiNum(p) );
    int i, v, nVars = Vec_IntSize(vTests) / nIter;
    assert( Vec_IntSize(vTests) % nIter == 0 );
    assert( nVars == 2 * Gia_ManPiNum(p) + Gia_ManRegNum(p) );
    for ( i = 0; i < nIter; i++ )
    {
        // collect PIs followed by flops
        Vec_IntClear( vValues );
        for ( v = Gia_ManRegNum(p); v < Gia_ManCiNum(p); v++ )
        {
            fprintf( pFile, "%d", Vec_IntEntry(vTests, i * nVars + v) );
            Vec_IntPush( vValues, Vec_IntEntry(vTests, i * nVars + v) );
        }
        for ( v = 0; v < Gia_ManRegNum(p); v++ )
        {
            fprintf( pFile, "%d", Vec_IntEntry(vTests, i * nVars + v) );
            Vec_IntPush( vValues, Vec_IntEntry(vTests, i * nVars + v) );
        }
        fprintf( pFile, "\n" );
        // derive next-state values
        Gia_ManDumpTestsSimulate( p, vValues );
        // collect PIs followed by flops
        for ( v = Gia_ManCiNum(p); v < nVars; v++ )
            fprintf( pFile, "%d", Vec_IntEntry(vTests, i * nVars + v) );
        for ( v = 0; v < Vec_IntSize(vValues); v++ )
            fprintf( pFile, "%d", Vec_IntEntry(vValues, v) );
        fprintf( pFile, "\n" );
    }
    Gia_ManCleanMark0(p);
    fclose( pFile );
    Vec_IntFree( vValues );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintResults( Gia_Man_t * p, sat_solver * pSat, int nIter, abctime clk )
{
    FILE * pTable = fopen( "fault_stats.txt", "a+" );

    fprintf( pTable, "%s ", Gia_ManName(p) );
    fprintf( pTable, "%d ", Gia_ManPiNum(p) );
    fprintf( pTable, "%d ", Gia_ManPoNum(p) );
    fprintf( pTable, "%d ", Gia_ManRegNum(p) );
    fprintf( pTable, "%d ", Gia_ManAndNum(p) );

    fprintf( pTable, "%d ", sat_solver_nvars(pSat) );
    fprintf( pTable, "%d ", sat_solver_nclauses(pSat) );
    fprintf( pTable, "%d ", sat_solver_nconflicts(pSat) );

    fprintf( pTable, "%d ", nIter );
    fprintf( pTable, "%.2f", 1.0*clk/CLOCKS_PER_SEC );
    fprintf( pTable, "\n" );
    fclose( pTable );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFaultAddOne( Gia_Man_t * pM, Cnf_Dat_t * pCnf, sat_solver * pSat, Vec_Int_t * vLits, int nFuncVars, int fAddOr, Gia_Man_t * pGiaCnf )
{
    Gia_Man_t * pC;//, * pTemp;
    Cnf_Dat_t * pCnf2;
    Gia_Obj_t * pObj;
    int i, Lit;
    // derive the cofactor
    pC = Gia_ManFaultCofactor( pM, vLits );
    // derive new CNF
    pCnf2 = Cnf_DeriveGiaRemapped( pC );
    Cnf_DataLiftGia( pCnf2, pC, sat_solver_nvars(pSat) );
    // add timeframe clauses
    for ( i = 0; i < pCnf2->nClauses; i++ )
        if ( !sat_solver_addclause( pSat, pCnf2->pClauses[i], pCnf2->pClauses[i+1] ) )
        {
            Cnf_DataFree( pCnf2 );
            Gia_ManStop( pC );
            return 0;
        }
    // add constraint clauses
    if ( fAddOr )
    {
        // add one OR gate to assert difference of at least one output of the miter
        Vec_Int_t * vOrGate = Vec_IntAlloc( Gia_ManPoNum(pC) );
        Gia_ManForEachPo( pC, pObj, i )
        {
            Lit = Abc_Var2Lit( pCnf2->pVarNums[Gia_ObjId(pC, pObj)], 0 );
            Vec_IntPush( vOrGate, Lit );
        }
        if ( !sat_solver_addclause( pSat, Vec_IntArray(vOrGate), Vec_IntArray(vOrGate) + Vec_IntSize(vOrGate) ) )
            assert( 0 );
        Vec_IntFree( vOrGate );
    }
    else
    {
        // add negative literals to assert equality of all outputs of the miter
        Gia_ManForEachPo( pC, pObj, i )
        {
            Lit = Abc_Var2Lit( pCnf2->pVarNums[Gia_ObjId(pC, pObj)], 1 );
            if ( !sat_solver_addclause( pSat, &Lit, &Lit+1 ) )
            {
                Cnf_DataFree( pCnf2 );
                Gia_ManStop( pC );
                return 0;
            }
        }
    }
    // add connection clauses
    if ( pGiaCnf )
    {
        Gia_ManForEachPi( pGiaCnf, pObj, i )
            if ( i >= nFuncVars )
                sat_solver_add_buffer( pSat, pCnf->pVarNums[Gia_ObjId(pGiaCnf, pObj)], pCnf2->pVarNums[Gia_ObjId(pC, Gia_ManPi(pC, i))], 0 );
    }
    Cnf_DataFree( pCnf2 );
    Gia_ManStop( pC );
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManDumpUntests( Gia_Man_t * pM, Cnf_Dat_t * pCnf, sat_solver * pSat, int nFuncVars, char * pFileName, int fVerbose )
{
    FILE * pFile = fopen( pFileName, "wb" );
    Vec_Int_t * vLits;
    Gia_Obj_t * pObj;
    int nItersMax = 10000;
    int i, nIters, status, Value, Count = 0;
    vLits = Vec_IntAlloc( Gia_ManPiNum(pM) - nFuncVars );
    for ( nIters = 0; nIters < nItersMax; nIters++ )
    {
        status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        if ( status == l_Undef )
            printf( "Timeout reached after dumping %d untestable faults.\n", nIters );
        if ( status == l_Undef )
            break;
        if ( status == l_False )
            break;
        // collect literals
        Vec_IntClear( vLits );
        Gia_ManForEachPi( pM, pObj, i )
            if ( i >= nFuncVars )
                Vec_IntPush( vLits, Abc_Var2Lit(pCnf->pVarNums[Gia_ObjId(pM, pObj)], sat_solver_var_value(pSat, pCnf->pVarNums[Gia_ObjId(pM, pObj)])) );
        // dump the fault
        Vec_IntForEachEntry( vLits, Value, i )
            if ( Abc_LitIsCompl(Value) )
                break;
        if ( i < Vec_IntSize(vLits) )
        {
            if ( fVerbose )
            {
                printf( "Untestable fault %4d : ", ++Count );
                Vec_IntForEachEntry( vLits, Value, i )
                    if ( Abc_LitIsCompl(Value) )
                        printf( "%d ", i );
                printf( "\n" );
            }
            Vec_IntForEachEntry( vLits, Value, i )
                if ( Abc_LitIsCompl(Value) )
                    fprintf( pFile, "%d ", i );
            fprintf( pFile, "\n" );
        }
        // add this clause
        if ( !sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) ) )
            break;
    }
    Vec_IntFree( vLits );
    fclose( pFile );
    return nIters;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManGetTestPatterns( char * pFileName )
{
    FILE * pFile = fopen( pFileName, "rb" );
    Vec_Int_t * vTests; int c;
    if ( pFile == NULL )
    {
        printf( "Cannot open input file \"%s\".\n", pFileName );
        return NULL;
    }
    vTests = Vec_IntAlloc( 10000 );
    while ( (c = fgetc(pFile)) != EOF )
    {
        if ( c == ' ' || c == '\t' || c == '\r' || c == '\n' )
            continue;
        if ( c != '0' && c != '1' )
        {
            printf( "Wrong symbol (%c) in the input file.\n", c );
            Vec_IntFreeP( &vTests );
            break;
        }
        Vec_IntPush( vTests, c - '0' );
    }
    fclose( pFile );
    return vTests;
}

/**Function*************************************************************

  Synopsis    [Derive the second AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDeriveDup( Gia_Man_t * p, int nPisNew )
{
    int i;
    Gia_Man_t * pNew = Gia_ManDup(p);
    for ( i = 0; i < nPisNew; i++ )
        Gia_ManAppendCi( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFaultAnalyze( sat_solver * pSat, Vec_Int_t * vPars, Vec_Int_t * vMap, Vec_Int_t * vLits, int Iter )
{
    int nConfLimit = 100;
    int status, i, v, iVar, Lit;
    int nUnsats = 0, nRuns = 0;
    abctime clk = Abc_Clock();
    assert( Vec_IntSize(vPars) == Vec_IntSize(vMap) );
    // check presence of each variable
    Vec_IntClear( vLits );
    Vec_IntAppend( vLits, vMap );
    for ( v = 0; v < Vec_IntSize(vPars); v++ )
    {
        if ( !Vec_IntEntry(vLits, v) )
            continue;
        assert( Vec_IntEntry(vLits, v) == 1 );
        nRuns++;
        Lit = Abc_Var2Lit( Vec_IntEntry(vPars, v), 0 );
        status = sat_solver_solve( pSat, &Lit, &Lit+1, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        if ( status == l_Undef )
            continue;
        if ( status == l_False )
        {
            nUnsats++;
            assert( Vec_IntEntry(vMap, v) == 1 );
            Vec_IntWriteEntry( vMap, v, 0 );
            Lit = Abc_LitNot(Lit);
            //status = sat_solver_addclause( pSat, &Lit, &Lit+1 );
            //assert( status );
            continue;
        }
        Vec_IntForEachEntry( vPars, iVar, i )
            if ( Vec_IntEntry(vLits, i) && sat_solver_var_value(pSat, iVar) )
                Vec_IntWriteEntry( vLits, i, 0 );
        assert( Vec_IntEntry(vLits, v) == 0 );
    }
    printf( "Iteration %3d has determined %5d (out of %5d) parameters after %6d SAT calls.  ", Iter, Vec_IntSize(vMap) - Vec_IntCountPositive(vMap), Vec_IntSize(vPars), nRuns );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return nUnsats;
}

/**Function*************************************************************

  Synopsis    [Dump faults detected by the new test, which are not detected by previous tests.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFaultDumpNewFaults( Gia_Man_t * pM, int nFuncVars, Vec_Int_t * vTests, Vec_Int_t * vTestNew, Bmc_ParFf_t * pPars )
{
    char * pFileName = "newfaults.txt";
    abctime clk;
    Gia_Man_t * pC;
    Cnf_Dat_t * pCnf2;
    sat_solver * pSat;
    Vec_Int_t * vLits;
    int i, Iter, IterMax, nNewFaults;

    // derive the cofactor
    pC = Gia_ManFaultCofactor( pM, vTestNew );
    // derive new CNF
    pCnf2 = Cnf_DeriveGiaRemapped( pC );

    // create SAT solver
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, 1 );
    sat_solver_set_runtime_limit( pSat, pPars->nTimeOut ? pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0 );
    // create the first cofactor
    Gia_ManFaultAddOne( pM, NULL, pSat, vTestNew, nFuncVars, 1, NULL );

    // add other test patterns
    assert( Vec_IntSize(vTests) % nFuncVars == 0 );
    IterMax = Vec_IntSize(vTests) / nFuncVars;
    vLits = Vec_IntAlloc( nFuncVars );
    for ( Iter = 0; Iter < IterMax; Iter++ )
    {
        // get pattern
        Vec_IntClear( vLits );
        for ( i = 0; i < nFuncVars; i++ )
            Vec_IntPush( vLits, Vec_IntEntry(vTests, Iter*nFuncVars + i) );
        // the resulting problem cannot be UNSAT, because the new test is guaranteed 
        // to detect something that the given test set does not detect
        if ( !Gia_ManFaultAddOne( pM, pCnf2, pSat, vLits, nFuncVars, 0, pC ) )
            assert( 0 );
    }
    Vec_IntFree( vLits );

    // dump the new faults
    clk = Abc_Clock();
    nNewFaults = Gia_ManDumpUntests( pC, pCnf2, pSat, nFuncVars, pFileName, pPars->fVerbose );
    printf( "Dumped %d new multiple faults into file \"%s\".  ", nNewFaults, pFileName );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    // cleanup
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf2 );
    Gia_ManStop( pC );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Generate miter, CNF and solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFaultPrepare( Gia_Man_t * p, Gia_Man_t * pG, Bmc_ParFf_t * pPars, int nFuncVars, Vec_Int_t * vMap, Vec_Int_t * vTests, Vec_Int_t * vLits, Gia_Man_t ** ppMiter, Cnf_Dat_t ** ppCnf, sat_solver ** ppSat, int fWarmUp )
{
    Gia_Man_t * p0 = NULL, * p1 = NULL, * pM;
    Gia_Obj_t * pObj;
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    int i, Iter, status;
    abctime clkSat = 0;

    if ( Vec_IntSize(vTests) && (Vec_IntSize(vTests) % nFuncVars != 0) )
    {
        printf( "The number of symbols in the input patterns (%d) does not divide evenly on the number of test variables (%d).\n", Vec_IntSize(vTests), nFuncVars );
        Vec_IntFree( vTests );
        return 0;
    }

    // select algorithm
    if ( pPars->Algo == 0 )
        p1 = Gia_ManFormulaUnfold( p, pPars->pFormStr, pPars->fFfOnly );
    else if ( pPars->Algo == 1 )
    {
        assert( Gia_ManRegNum(p) > 0 );
        p0 = Gia_ManFaultUnfold( pG, 0, pPars->fFfOnly );
        p1 = Gia_ManFaultUnfold( p, 1, pPars->fFfOnly );
    }
    else if ( pPars->Algo == 2 )
        p1 = Gia_ManStuckAtUnfold( p, vMap );
    else if ( pPars->Algo == 3 )
        p1 = Gia_ManFlipUnfold( p, vMap );
    else if ( pPars->Algo == 4 )
        p1 = Gia_ManFOFUnfold( p, vMap );
    if ( pPars->Algo != 1 )
        p0 = Gia_ManDeriveDup( pG, Gia_ManCiNum(p1) - Gia_ManCiNum(pG) );
//    Gia_AigerWrite( p1, "newfault.aig", 0, 0 );
//    printf( "Dumped circuit with fault parameters into file \"newfault.aig\".\n" );

    // create miter
    pM = Gia_ManMiter( p0, p1, 0, 0, 0, 0, 0 );
    pCnf = Cnf_DeriveGiaRemapped( pM );
    Gia_ManStop( p0 );
    Gia_ManStop( p1 );

    // start the SAT solver
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, pCnf->nVars );
    sat_solver_set_runtime_limit( pSat, pPars->nTimeOut ? pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0 );
    // add timeframe clauses
    for ( i = 0; i < pCnf->nClauses; i++ )
        if ( !sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
            assert( 0 );

    // add one large OR clause
    Vec_IntClear( vLits );
    Gia_ManForEachCo( pM, pObj, i )
        Vec_IntPush( vLits, Abc_Var2Lit(pCnf->pVarNums[Gia_ObjId(pM, pObj)], 0) );
    sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) );

    // save return data
    *ppMiter = pM;
    *ppCnf = pCnf;
    *ppSat = pSat;

    // add cardinality constraint
    if ( pPars->fBasic )
    {
        Vec_IntClear( vLits );
        Gia_ManForEachPi( pM, pObj, i )
            if ( i >= nFuncVars )
                Vec_IntPush( vLits, pCnf->pVarNums[Gia_ObjId(pM, pObj)] );
        Cnf_AddCardinConstr( pSat, vLits );
    }
    else if ( pPars->nCardConstr )
    {
        Vec_IntClear( vLits );
        Gia_ManForEachPi( pM, pObj, i )
            if ( i >= nFuncVars )
                Vec_IntPush( vLits, pCnf->pVarNums[Gia_ObjId(pM, pObj)] );
        Cnf_AddCardinConstrGeneral( pSat, vLits, pPars->nCardConstr, !pPars->fNonStrict );
    }

    // add available test-patterns
    if ( Vec_IntSize(vTests) > 0 )
    {
        int nTests = Vec_IntSize(vTests) / nFuncVars;
        assert( Vec_IntSize(vTests) % nFuncVars == 0 );
        if ( pPars->pFileName )
            printf( "Reading %d pre-computed test patterns from file \"%s\".\n", Vec_IntSize(vTests) / nFuncVars, pPars->pFileName );
        else
            printf( "Reading %d pre-computed test patterns from previous rounds.\n", Vec_IntSize(vTests) / nFuncVars );
        for ( Iter = 0; Iter < nTests; Iter++ )
        {
            if ( fWarmUp )
            {
                abctime clk = Abc_Clock();
                status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
                clkSat += Abc_Clock() - clk;
                if ( status == l_Undef )
                {
                    if ( pPars->fVerbose )
                        printf( "\n" );
                    printf( "Timeout reached after %d seconds and adding %d tests.\n", pPars->nTimeOut, Iter );
                    Vec_IntShrink( vTests, Iter * nFuncVars );
                    return 0;
                }
                if ( status == l_False )
                {
                    if ( pPars->fVerbose )
                        printf( "\n" );
                    printf( "The problem is UNSAT after adding %d tests.\n", Iter );
                    Vec_IntShrink( vTests, Iter * nFuncVars );
                    return 0;
                }
            }
            // get pattern
            Vec_IntClear( vLits );
            for ( i = 0; i < nFuncVars; i++ )
                Vec_IntPush( vLits, Vec_IntEntry(vTests, Iter*nFuncVars + i) );
            if ( !Gia_ManFaultAddOne( pM, pCnf, pSat, vLits, nFuncVars, 0, pM ) )
            {
                if ( pPars->fVerbose )
                    printf( "\n" );
                printf( "The problem is UNSAT after adding %d tests.\n", Iter );
                Vec_IntShrink( vTests, Iter * nFuncVars );
                return 0;
            }
            if ( pPars->fVerbose )
            {
                printf( "Iter%6d : ",       Iter );
                printf( "Var =%10d  ",      sat_solver_nvars(pSat) );
                printf( "Clause =%10d  ",   sat_solver_nclauses(pSat) );
                printf( "Conflict =%10d  ", sat_solver_nconflicts(pSat) );
                //Abc_PrintTime( 1, "Time", clkSat );
                ABC_PRTr( "Solver time", clkSat );
            }
        }
    }
    else if ( pPars->fStartPats )
    {
        for ( Iter = 0; Iter < 2; Iter++ )
        {
            status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
            if ( status == l_Undef )
            {
                if ( pPars->fVerbose )
                    printf( "\n" );
                printf( "Timeout reached after %d seconds and %d iterations.\n", pPars->nTimeOut, Iter );
                Vec_IntShrink( vTests, Iter * nFuncVars );
                return 0;
            }
            if ( status == l_False )
            {
                if ( pPars->fVerbose )
                    printf( "\n" );
                printf( "The problem is UNSAT after %d iterations.\n", Iter );
                Vec_IntShrink( vTests, Iter * nFuncVars );
                return 0;
            }
            // initialize simple pattern
            Vec_IntFill( vLits, nFuncVars, Iter );
            Vec_IntAppend( vTests, vLits );
            if ( !Gia_ManFaultAddOne( pM, pCnf, pSat, vLits, nFuncVars, 0, pM ) )
            {
                if ( pPars->fVerbose )
                    printf( "\n" );
                printf( "The problem is UNSAT after adding %d tests.\n", Iter );
                Vec_IntShrink( vTests, Iter * nFuncVars );
                return 0;
            }
        }
    }

    printf( "Using miter with:  AIG nodes = %6d.  CNF variables = %6d.  CNF clauses = %8d.\n", Gia_ManAndNum(pM), pCnf->nVars, pCnf->nClauses );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFaultTest( Gia_Man_t * p, Gia_Man_t * pG, Bmc_ParFf_t * pPars )
{
    int nIterMax = 1000000, nVars, nPars;
    int i, Iter, Iter2, status, nFuncVars = -1;
    abctime clk, clkSat = 0, clkTotal = Abc_Clock();
    Vec_Int_t * vLits, * vMap = NULL, * vTests, * vPars = NULL;
    Gia_Man_t * pM;
    Gia_Obj_t * pObj;
    Cnf_Dat_t * pCnf;
    sat_solver * pSat = NULL;

    if ( pPars->Algo == 0 && Gia_FormStrCount( pPars->pFormStr, &nVars, &nPars ) )
        return;

    // select algorithm
    if ( pPars->Algo == 0 )
        printf( "FFTEST is computing test patterns for fault model \"%s\"...\n", pPars->pFormStr );
    else if ( pPars->Algo == 1 )
        printf( "FFTEST is computing test patterns for %sdelay faults...\n", pPars->fBasic ? "single " : "" );
    else if ( pPars->Algo == 2 )
        printf( "FFTEST is computing test patterns for %sstuck-at faults...\n", pPars->fBasic ? "single " : "" );
    else if ( pPars->Algo == 3 )
        printf( "FFTEST is computing test patterns for %scomplement faults...\n", pPars->fBasic ? "single " : "" );
    else if ( pPars->Algo == 4 )
        printf( "FFTEST is computing test patterns for %sfunctionally observable faults...\n", pPars->fBasic ? "single " : "" );
    else
    {
        printf( "Unrecognized algorithm (%d).\n", pPars->Algo );
        return;
    }

    // select algorithm
    if ( pPars->Algo == 0 )
        nFuncVars = Gia_ManCiNum(p);
    else if ( pPars->Algo == 1 )
        nFuncVars = Gia_ManRegNum(p) + 2 * Gia_ManPiNum(p);
    else if ( pPars->Algo == 2 )
        nFuncVars = Gia_ManCiNum(p);
    else if ( pPars->Algo == 3 )
        nFuncVars = Gia_ManCiNum(p);
    else if ( pPars->Algo == 4 )
        nFuncVars = Gia_ManCiNum(p);

    // collect test patterns from file
    if ( pPars->pFileName )
        vTests = Gia_ManGetTestPatterns( pPars->pFileName );
    else
        vTests = Vec_IntAlloc( 10000 );
    if ( vTests == NULL )
        return;

    // select algorithm
    vMap = Vec_IntAlloc( 0 );
    if ( pPars->Algo == 2 )
        Vec_IntFill( vMap, 2 * Gia_ManAndNum(p), 1 );
    else if ( pPars->Algo == 3 )
        Vec_IntFill( vMap,     Gia_ManAndNum(p), 1 );
    else if ( pPars->Algo == 4 )
        Vec_IntFill( vMap, 4 * Gia_ManAndNum(p), 1 );

    // prepare SAT solver
    vLits = Vec_IntAlloc( Gia_ManCoNum(p) );

    // add user-speicified test-vectors (if given) and create missing ones (if needed)
    if ( Gia_ManFaultPrepare(p, pG, pPars, nFuncVars, vMap, vTests, vLits, &pM, &pCnf, &pSat, 1) )
    for ( Iter = pPars->fStartPats ? 2 : Vec_IntSize(vTests) / nFuncVars; Iter < nIterMax; Iter++ )
    {
        // collect parameter variables
        if ( pPars->nIterCheck && vPars == NULL )
        {
            vPars = Vec_IntAlloc( Gia_ManPiNum(pM) - nFuncVars );
            Gia_ManForEachPi( pM, pObj, i )
                if ( i >= nFuncVars )
                    Vec_IntPush( vPars, pCnf->pVarNums[Gia_ObjId(pM, pObj)] );
            assert( Vec_IntSize(vPars) == Gia_ManPiNum(pM) - nFuncVars );
        }
        // derive unit parameter variables
        if ( Iter && pPars->nIterCheck && (Iter % pPars->nIterCheck) == 0 )
        {
            Gia_ManFaultAnalyze( pSat, vPars, vMap, vLits, Iter );
            // cleanup
            Gia_ManStop( pM );
            Cnf_DataFree( pCnf );
            sat_solver_delete( pSat );
            // recompute
            if ( !Gia_ManFaultPrepare(p, pG, pPars, nFuncVars, vMap, vTests, vLits, &pM, &pCnf, &pSat, 0) )
            {
                printf( "This should never happen.\n" );
                return;
            }
            Vec_IntFreeP( &vPars );
        }
        // solve
        clk = Abc_Clock();
        status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        clkSat += Abc_Clock() - clk;
        if ( pPars->fVerbose )
        {
            printf( "Iter%6d : ",       Iter );
            printf( "Var =%10d  ",      sat_solver_nvars(pSat) );
            printf( "Clause =%10d  ",   sat_solver_nclauses(pSat) );
            printf( "Conflict =%10d  ", sat_solver_nconflicts(pSat) );
            //Abc_PrintTime( 1, "Time", clkSat );
            ABC_PRTr( "Solver time", clkSat );
        }
        if ( status == l_Undef )
        {
            if ( pPars->fVerbose )
                printf( "\n" );
            printf( "Timeout reached after %d seconds and %d iterations.\n", pPars->nTimeOut, Iter );
            goto finish;
        }
        if ( status == l_False )
        {
            if ( pPars->fVerbose )
                printf( "\n" );
            printf( "The problem is UNSAT after %d iterations.  ", Iter );
            break;
        }
        assert( status == l_True );
        // collect SAT assignment
        Vec_IntClear( vLits );
        Gia_ManForEachPi( pM, pObj, i )
            if ( i < nFuncVars )
                Vec_IntPush( vLits, sat_solver_var_value(pSat, pCnf->pVarNums[Gia_ObjId(pM, pObj)]) );
        // if the user selected to generate new faults detected by this test (vLits)
        // and not detected by the given test set (vTests), we compute the new faults here and quit
        if ( pPars->fDumpNewFaults )
        {
            if ( Vec_IntSize(vTests) == 0 )
                printf( "The input test patterns are not given.\n" );
            else
                Gia_ManFaultDumpNewFaults( pM, nFuncVars, vTests, vLits, pPars );
            goto finish_all;
        }
        Vec_IntAppend( vTests, vLits );
        // add constraint
        if ( !Gia_ManFaultAddOne( pM, pCnf, pSat, vLits, nFuncVars, 0, pM ) )
        {
            if ( pPars->fVerbose )
                printf( "\n" );
            printf( "The problem is UNSAT after adding %d tests.\n", Iter );
            break;
        }
    }
    else Iter = Vec_IntSize(vTests) / nFuncVars;
finish:
    // print results
//    if ( status == l_False )
//        Gia_ManPrintResults( p, pSat, Iter, Abc_Clock() - clkTotal );
    // cleanup
    Abc_PrintTime( 1, "Testing runtime", Abc_Clock() - clkTotal );
    // dump the test suite
    if ( pPars->fDump )
    {
        char * pFileName = "tests.txt";
        if ( pPars->fDumpDelay && pPars->Algo == 1 )
        {
            Gia_ManDumpTestsDelay( vTests, Iter, pFileName, p );
            printf( "Dumping %d pairs of test patterns (total %d pattern) into file \"%s\".\n", Vec_IntSize(vTests) / nFuncVars, 2*Vec_IntSize(vTests) / nFuncVars, pFileName );
        }
        else
        {
            Gia_ManDumpTests( vTests, Iter, pFileName );
            printf( "Dumping %d test patterns into file \"%s\".\n", Vec_IntSize(vTests) / nFuncVars, pFileName );
        }
    }

    // compute untestable faults
    if ( Iter && (p != pG || pPars->fDumpUntest) )
    {
        abctime clkTotal = Abc_Clock();
        // restart the SAT solver
        sat_solver_delete( pSat );
        pSat = sat_solver_new();
        sat_solver_setnvars( pSat, pCnf->nVars );
        sat_solver_set_runtime_limit( pSat, pPars->nTimeOut ? pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0 );
        // add timeframe clauses
        for ( i = 0; i < pCnf->nClauses; i++ )
            if ( !sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
                assert( 0 );
        // add constraint to rule out no fault
//        if ( p == pG )
        {
            Vec_IntClear( vLits );
            Gia_ManForEachPi( pM, pObj, i )
                if ( i >= nFuncVars )
                    Vec_IntPush( vLits, Abc_Var2Lit(pCnf->pVarNums[Gia_ObjId(pM, pObj)], 0) );
            sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) );
        }
        // add cardinality constraint
        if ( pPars->fBasic )
        {
            Vec_IntClear( vLits );
            Gia_ManForEachPi( pM, pObj, i )
                if ( i >= nFuncVars )
                    Vec_IntPush( vLits, pCnf->pVarNums[Gia_ObjId(pM, pObj)] );
            Cnf_AddCardinConstr( pSat, vLits );
        }
        // add output clauses
        Gia_ManForEachCo( pM, pObj, i )
        {
            int Lit = Abc_Var2Lit( pCnf->pVarNums[Gia_ObjId(pM, pObj)], 1 );
            sat_solver_addclause( pSat, &Lit, &Lit + 1 );
        }
        // simplify the SAT solver
        status = sat_solver_simplify( pSat );
        assert( status );

        // add test patterns
        assert( Vec_IntSize(vTests) == Iter * nFuncVars );
        for ( Iter2 = 0; ; Iter2++ )
        {
            abctime clk = Abc_Clock();
            status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
            clkSat += Abc_Clock() - clk;
            if ( pPars->fVerbose )
            {
                printf( "Iter%6d : ",       Iter2 );
                printf( "Var =%10d  ",      sat_solver_nvars(pSat) );
                printf( "Clause =%10d  ",   sat_solver_nclauses(pSat) );
                printf( "Conflict =%10d  ", sat_solver_nconflicts(pSat) );
                //Abc_PrintTime( 1, "Time", clkSat );
                ABC_PRTr( "Solver time", clkSat );
            }
            if ( status == l_Undef )
            {
                if ( pPars->fVerbose )
                    printf( "\n" );
                printf( "Timeout reached after %d seconds and %d iterations.\n", pPars->nTimeOut, Iter2 );
                goto finish;
            }
            if ( Iter2 == Iter )
                break;
            assert( status == l_True );
            // get pattern
            Vec_IntClear( vLits );
            for ( i = 0; i < nFuncVars; i++ )
                Vec_IntPush( vLits, Vec_IntEntry(vTests, Iter2*nFuncVars + i) );
            if ( !Gia_ManFaultAddOne( pM, pCnf, pSat, vLits, nFuncVars, 0, pM ) )
            {
                printf( "The problem is UNSAT after adding %d tests.\n", Iter2 );
                goto finish;
            }
        }
        assert( Iter2 == Iter );
        if ( pPars->fVerbose )
            printf( "\n" );
        if ( p == pG )
        {
            if ( status == l_True )
                printf( "There are untestable faults.  " );
            else if ( status == l_False )
                printf( "There is no untestable faults.  " );
            else assert( 0 );
            Abc_PrintTime( 1, "Fault computation runtime", Abc_Clock() - clkTotal );
        }
        else
        {
            if ( status == l_True )
                printf( "The circuit is rectifiable.  " );
            else if ( status == l_False )
                printf( "The circuit is not rectifiable (or equivalent to the golden one).  " );
            else assert( 0 );
            Abc_PrintTime( 1, "Rectification runtime", Abc_Clock() - clkTotal );
        }
        // dump untestable faults
        if ( pPars->fDumpUntest && status == l_True )
        {
            abctime clk = Abc_Clock();
            char * pFileName = "untest.txt";
            int nUntests = Gia_ManDumpUntests( pM, pCnf, pSat, nFuncVars, pFileName, pPars->fVerbose );
            if ( p == pG )
                printf( "Dumped %d untestable multiple faults into file \"%s\".  ", nUntests, pFileName );
            else
                printf( "Dumped %d ways of rectifying the circuit into file \"%s\".  ", nUntests, pFileName );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }
    }
finish_all:
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    Gia_ManStop( pM );
    Vec_IntFree( vTests );
    Vec_IntFree( vMap );
    Vec_IntFree( vLits );
    Vec_IntFreeP( &vPars );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

