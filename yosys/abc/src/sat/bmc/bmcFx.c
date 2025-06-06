/**CFile****************************************************************

  FileName    [bmcFx.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [INT-FX: Interpolation-based logic sharing extraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcFx.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "misc/vec/vecWec.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Create hash table to hash divisors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#define TAB_UNUSED 0x7FFF
typedef struct Tab_Obj_t_ Tab_Obj_t; // 16 bytes
struct Tab_Obj_t_
{
    int         Table;
    int         Next;
    unsigned    Cost : 17;
    unsigned    LitA : 15;
    unsigned    LitB : 15;
    unsigned    LitC : 15;
    unsigned    Func :  2;
};
typedef struct Tab_Tab_t_ Tab_Tab_t; // 16 bytes
struct Tab_Tab_t_
{
    int         SizeMask;
    int         nBins;
    Tab_Obj_t * pBins;
};
static inline Tab_Tab_t * Tab_TabAlloc( int LogSize )
{
    Tab_Tab_t * p = ABC_CALLOC( Tab_Tab_t, 1 );
    assert( LogSize >= 4 && LogSize <= 31 );
    p->SizeMask = (1 << LogSize) - 1;
    p->pBins = ABC_CALLOC( Tab_Obj_t, p->SizeMask + 1 );
    p->nBins = 1;
    return p;
}
static inline void Tab_TabFree( Tab_Tab_t * p )
{
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}
static inline Vec_Int_t * Tab_TabFindBest( Tab_Tab_t * p, int nDivs )
{
    char * pNames[5] = {"const1", "and", "xor", "mux", "none"};
    int * pOrder, i;
    Vec_Int_t * vDivs = Vec_IntAlloc( 100 ); 
    Vec_Int_t * vCosts = Vec_IntAlloc( p->nBins ); 
    Tab_Obj_t * pEnt, * pLimit = p->pBins + p->nBins;
    for ( pEnt = p->pBins; pEnt < pLimit; pEnt++ )
        Vec_IntPush( vCosts, -(int)pEnt->Cost );
    pOrder = Abc_MergeSortCost( Vec_IntArray(vCosts), Vec_IntSize(vCosts) );
    for ( i = 0; i < Vec_IntSize(vCosts); i++ )
    {
        pEnt = p->pBins + pOrder[i];
        if ( i == nDivs || pEnt->Cost == 1 )
            break;
        printf( "Lit0 = %5d.  Lit1 = %5d.  Lit2 = %5d.  Func = %s.  Cost = %3d.\n",
            pEnt->LitA, pEnt->LitB, pEnt->LitC, pNames[pEnt->Func], pEnt->Cost );
        Vec_IntPushTwo( vDivs, pEnt->Func, pEnt->LitA );
        Vec_IntPushTwo( vDivs, pEnt->LitB, pEnt->LitC );
    }
    Vec_IntFree( vCosts );
    ABC_FREE( pOrder );
    return vDivs;
}
static inline int Tab_Hash( int LitA, int LitB, int LitC, int Func, int Mask )
{
    return (LitA * 50331653 + LitB * 100663319 + LitC * 201326611 + Func * 402653189) & Mask;
}
static inline void Tab_TabRehash( Tab_Tab_t * p )
{
    Tab_Obj_t * pEnt, * pLimit, * pBin;
    assert( p->nBins == p->SizeMask + 1 );
    // realloc memory
    p->pBins = ABC_REALLOC( Tab_Obj_t, p->pBins, 2 * (p->SizeMask + 1) );
    memset( p->pBins + p->SizeMask + 1, 0, sizeof(Tab_Obj_t) * (p->SizeMask + 1) );
    // clean entries
    pLimit = p->pBins + p->SizeMask + 1;
    for ( pEnt = p->pBins; pEnt < pLimit; pEnt++ )
        pEnt->Table = pEnt->Next = 0;
    // rehash entries
    p->SizeMask = 2 * p->SizeMask + 1;
    for ( pEnt = p->pBins + 1; pEnt < pLimit; pEnt++ )
    {
        pBin = p->pBins + Tab_Hash( pEnt->LitA, pEnt->LitB, pEnt->LitC, pEnt->Func, p->SizeMask );
        pEnt->Next  = pBin->Table;
        pBin->Table = pEnt - p->pBins;
        assert( !pEnt->Next || pEnt->Next != pBin->Table );
    }
}
static inline Tab_Obj_t * Tab_TabEntry( Tab_Tab_t * p, int i ) { return i ? p->pBins + i : NULL; }
static inline int         Tab_TabHashAdd( Tab_Tab_t * p, int * pLits, int Func, int Cost )
{
    if ( p->nBins == p->SizeMask + 1 )
        Tab_TabRehash( p );
    assert( p->nBins < p->SizeMask + 1 );
    {
        Tab_Obj_t * pEnt, * pBin = p->pBins + Tab_Hash(pLits[0], pLits[1], pLits[2], Func, p->SizeMask);
        for ( pEnt = Tab_TabEntry(p, pBin->Table); pEnt; pEnt = Tab_TabEntry(p, pEnt->Next) )
            if ( (int)pEnt->LitA == pLits[0] && (int)pEnt->LitB == pLits[1] && (int)pEnt->LitC == pLits[2] && (int)pEnt->Func == Func )
                {  pEnt->Cost += Cost; return 1; }
        pEnt = p->pBins + p->nBins;
        pEnt->LitA  = pLits[0];
        pEnt->LitB  = pLits[1];
        pEnt->LitC  = pLits[2];
        pEnt->Func  = Func;
        pEnt->Cost  = Cost;
        pEnt->Next  = pBin->Table;
        pBin->Table = p->nBins++;
        assert( !pEnt->Next || pEnt->Next != pBin->Table );
        return 0;
    }
}


/**Function*************************************************************

  Synopsis    [Input is four literals. Output is type, polarity and fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// name types
typedef enum { 
    DIV_CST = 0,   // 0: constant 1
    DIV_AND,       // 1: and (ordered fanins)
    DIV_XOR,       // 2: xor (ordered fanins)
    DIV_MUX,       // 3: mux (c, d1, d0)
    DIV_NONE       // 4: not used
} Div_Type_t; 

static inline Div_Type_t Bmc_FxDivOr( int LitA, int LitB, int * pLits, int * pPhase )
{
    assert( LitA != LitB );
    if ( Abc_Lit2Var(LitA) == Abc_Lit2Var(LitB) )
        return DIV_CST;
    if ( LitA > LitB )
        ABC_SWAP( int, LitA, LitB );
    pLits[0] = Abc_LitNot(LitA);
    pLits[1] = Abc_LitNot(LitB);
    *pPhase = 1;
    return DIV_AND;
}
static inline Div_Type_t Bmc_FxDivXor( int LitA, int LitB, int * pLits, int * pPhase )
{
    assert( LitA != LitB );
    *pPhase ^= Abc_LitIsCompl(LitA);
    *pPhase ^= Abc_LitIsCompl(LitB);
    pLits[0] = Abc_LitRegular(LitA);
    pLits[1] = Abc_LitRegular(LitB);
    return DIV_XOR;
}
static inline Div_Type_t Bmc_FxDivMux( int LitC, int LitCn, int LitT, int LitE, int * pLits, int * pPhase )
{
    assert( LitC != LitCn );
    assert( Abc_Lit2Var(LitC) == Abc_Lit2Var(LitCn) );
    assert( Abc_Lit2Var(LitC) != Abc_Lit2Var(LitT) );
    assert( Abc_Lit2Var(LitC) != Abc_Lit2Var(LitE) );
    assert( Abc_Lit2Var(LitC) != Abc_Lit2Var(LitE) );
    if ( Abc_LitIsCompl(LitC) )
    {
        LitC = Abc_LitRegular(LitC);
        ABC_SWAP( int, LitT, LitE );
    }
    if ( Abc_LitIsCompl(LitT) )
    {
        *pPhase ^= 1;
        LitT = Abc_LitNot(LitT);
        LitE = Abc_LitNot(LitE);
    }
    pLits[0] = LitC;
    pLits[1] = LitT;
    pLits[2] = LitE;
    return DIV_MUX;
}
static inline Div_Type_t Div_FindType( int LitA[2], int LitB[2], int * pLits, int * pPhase )
{
    *pPhase = 0;
    pLits[0] = pLits[1] = pLits[2] = TAB_UNUSED;
    if ( LitA[0] == -1 && LitA[1] == -1 ) return DIV_CST;
    if ( LitB[0] == -1 && LitB[1] == -1 ) return DIV_CST;
    assert( LitA[0] >= 0 && LitB[0] >= 0 );
    assert( LitA[0] != LitB[0] );
    if ( LitA[1] == -1 && LitB[1] == -1 )
        return Bmc_FxDivOr( LitA[0], LitB[0], pLits, pPhase );
    assert( LitA[1] != LitB[1] );
    if ( LitA[1] == -1 || LitB[1] == -1 )
    {
        if ( LitA[1] == -1 )
        {
            ABC_SWAP( int, LitA[0], LitB[0] );
            ABC_SWAP( int, LitA[1], LitB[1] );
        }
        assert( LitA[0] >= 0 && LitA[1] >= 0 );
        assert( LitB[0] >= 0 && LitB[1] == -1 );
        if ( Abc_Lit2Var(LitB[0]) == Abc_Lit2Var(LitA[0]) )
            return Bmc_FxDivOr( LitB[0], LitA[1], pLits, pPhase );
        if ( Abc_Lit2Var(LitB[0]) == Abc_Lit2Var(LitA[1]) )
            return Bmc_FxDivOr( LitB[0], LitA[0], pLits, pPhase );
        return DIV_NONE;
    }
    if ( Abc_Lit2Var(LitA[0]) == Abc_Lit2Var(LitB[0]) && Abc_Lit2Var(LitA[1]) == Abc_Lit2Var(LitB[1]) )
        return Bmc_FxDivXor( LitA[0], LitB[1], pLits, pPhase );
    if ( Abc_Lit2Var(LitA[0]) == Abc_Lit2Var(LitB[0]) )
        return Bmc_FxDivMux( LitA[0], LitB[0], LitA[1], LitB[1], pLits, pPhase );
    if ( Abc_Lit2Var(LitA[0]) == Abc_Lit2Var(LitB[1]) )
        return Bmc_FxDivMux( LitA[0], LitB[1], LitA[1], LitB[0], pLits, pPhase );
    if ( Abc_Lit2Var(LitA[1]) == Abc_Lit2Var(LitB[0]) )
        return Bmc_FxDivMux( LitA[1], LitB[0], LitA[0], LitB[1], pLits, pPhase );
    if ( Abc_Lit2Var(LitA[1]) == Abc_Lit2Var(LitB[1]) )
        return Bmc_FxDivMux( LitA[1], LitB[1], LitA[0], LitB[0], pLits, pPhase );
    return DIV_NONE;
}

/**Function*************************************************************

  Synopsis    [Returns the number of shared variables, or -1 if failed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Div_AddLit( int Lit, int pLits[2] )
{
    if ( pLits[0] == -1 )
        pLits[0] = Lit;
    else if ( pLits[1] == -1 )
        pLits[1] = Lit;
    else return 1;
    return 0;
}
int Div_FindDiv( Vec_Int_t * vA, Vec_Int_t * vB, int pLitsA[2], int pLitsB[2] )
{
    int Counter = 0;
    int * pBegA = vA->pArray, * pEndA = vA->pArray + vA->nSize;
    int * pBegB = vB->pArray, * pEndB = vB->pArray + vB->nSize; 
    pLitsA[0] = pLitsA[1] = pLitsB[0] = pLitsB[1] = -1;
    while ( pBegA < pEndA && pBegB < pEndB )
    {
        if ( *pBegA == *pBegB )
            pBegA++, pBegB++, Counter++;
        else if ( *pBegA < *pBegB )
        {
            if ( Div_AddLit( *pBegA++, pLitsA ) )
                return -1;
        }
        else  
        {
            if ( Div_AddLit( *pBegB++, pLitsB ) )
                return -1;
        }
    }
    while ( pBegA < pEndA )
        if ( Div_AddLit( *pBegA++, pLitsA ) )
            return -1;
    while ( pBegB < pEndB )
        if ( Div_AddLit( *pBegB++, pLitsB ) )
            return -1;
    return Counter;
}

void Div_CubePrintOne( Vec_Int_t * vCube, Vec_Str_t * vStr, int nVars )
{
    int i, Lit;
    Vec_StrFill( vStr, nVars, '-' );
    Vec_IntForEachEntry( vCube, Lit, i )
        Vec_StrWriteEntry( vStr, Abc_Lit2Var(Lit), (char)(Abc_LitIsCompl(Lit) ? '0' : '1') );
    printf( "%s\n", Vec_StrArray(vStr) );
}
void Div_CubePrint( Vec_Wec_t * p, int nVars )
{
    Vec_Int_t * vCube; int i;
    Vec_Str_t * vStr = Vec_StrStart( nVars + 1 );
    Vec_WecForEachLevel( p, vCube, i )
        Div_CubePrintOne( vCube, vStr, nVars );
    Vec_StrFree( vStr );
}

Vec_Int_t * Div_CubePairs( Vec_Wec_t * p, int nVars, int nDivs )
{
    int fVerbose = 0;
    char * pNames[5] = {"const1", "and", "xor", "mux", "none"};
    Vec_Int_t * vCube1, * vCube2, * vDivs; 
    int i1, i2, i, k, pLitsA[2], pLitsB[2], pLits[4], Type, Phase, nBase, Count = 0;
    Vec_Str_t * vStr = Vec_StrStart( nVars + 1 );
    Tab_Tab_t * pTab = Tab_TabAlloc( 5 );
    Vec_WecForEachLevel( p, vCube1, i )
    {
        // add lit pairs
        pLits[2] = TAB_UNUSED;
        Vec_IntForEachEntry( vCube1, pLits[0], i1 )
        Vec_IntForEachEntryStart( vCube1, pLits[1], i2, i1+1 )
        {
            Tab_TabHashAdd( pTab, pLits, DIV_AND, 1 );
        }

        Vec_WecForEachLevelStart( p, vCube2, k, i+1 )
        {
            nBase = Div_FindDiv( vCube1, vCube2, pLitsA, pLitsB );
            if ( nBase == -1 )
                continue;
            Type = Div_FindType(pLitsA, pLitsB, pLits, &Phase);
            if ( Type >= DIV_AND && Type <= DIV_MUX )
                Tab_TabHashAdd( pTab, pLits, Type, nBase );

            if ( fVerbose )
            {
                printf( "Pair %d:\n", Count++ );
                Div_CubePrintOne( vCube1, vStr, nVars );
                Div_CubePrintOne( vCube2, vStr, nVars );
                printf( "Result = %d   ", nBase );
                assert( nBase > 0 );

                printf( "Type = %s  ", pNames[Type] );
                printf( "LitA = %d ",  pLits[0] );
                printf( "LitB = %d ",  pLits[1] );
                printf( "LitC = %d ",  pLits[2] );
                printf( "Phase = %d ", Phase );
                printf( "\n" );
            }
        }
    }
    // print the table
    printf( "Divisors = %d.\n", pTab->nBins );
    vDivs = Tab_TabFindBest( pTab, nDivs );
    // cleanup
    Vec_StrFree( vStr );
    Tab_TabFree( pTab );
    return vDivs;
}

/**Function*************************************************************

  Synopsis    [Solve the enumeration problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_FxSolve( sat_solver * pSat, int iOut, int iAuxVar, Vec_Int_t * vVars, int fDumpPla, int fVerbose, int * pCounter, Vec_Wec_t * vCubes )
{
    int nBTLimit = 1000000;
    int fUseOrder = 1;
    Vec_Int_t * vLevel  = NULL;
    Vec_Int_t * vLits   = Vec_IntAlloc( Vec_IntSize(vVars) );
    Vec_Int_t * vLits2  = Vec_IntAlloc( Vec_IntSize(vVars) );
    Vec_Str_t * vCube   = Vec_StrStart( Vec_IntSize(vVars)+1 );
    int status, i, k, n, Lit, Lit2, iVar, nFinal, * pFinal, pLits[2], nIter = 0, RetValue = 0;
    int Before, After, Total = 0, nLits = 0;
    pLits[0] = Abc_Var2Lit( iOut + 1, 0 ); // F = 1
    pLits[1] = Abc_Var2Lit( iAuxVar, 0 );  // iNewLit
    if ( vCubes )
        Vec_WecClear( vCubes );
    if ( fDumpPla )
    printf( ".i %d\n", Vec_IntSize(vVars) );
    if ( fDumpPla )
    printf( ".o %d\n", 1 );
    while ( 1 ) 
    {
        // find onset minterm
        status = sat_solver_solve( pSat, pLits, pLits + 2, nBTLimit, 0, 0, 0 );
        if ( status == l_Undef )
            { RetValue = -1; break; }
        if ( status == l_False )
            { RetValue = 1; break; }
        assert( status == l_True );
        // collect divisor literals
        Vec_IntClear( vLits );
        Vec_IntPush( vLits, Abc_LitNot(pLits[0]) ); // F = 0
//        Vec_IntForEachEntryReverse( vVars, iVar, i )
        Vec_IntForEachEntry( vVars, iVar, i )
            Vec_IntPush( vLits, sat_solver_var_literal(pSat, iVar) );

        if ( fUseOrder )
        {
            //////////////////////////////////////////////////////////////
            // save these literals
            Vec_IntClear( vLits2 );
            Vec_IntAppend( vLits2, vLits );
            Before = Vec_IntSize(vLits2);

            // try removing literals from the cube
            Vec_IntForEachEntry( vLits2, Lit2, k )
            {
                if ( Lit2 == Abc_LitNot(pLits[0]) )
                    continue;
                Vec_IntClear( vLits );
                Vec_IntForEachEntry( vLits2, Lit, n )
                    if ( Lit != -1 && Lit != Lit2 )
                        Vec_IntPush( vLits, Lit );
                // call sat
                status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), nBTLimit, 0, 0, 0 );
                if ( status == l_Undef )
                    assert( 0 );
                if ( status == l_True ) // SAT
                    continue;
                // Lit2 can be removed
                Vec_IntWriteEntry( vLits2, k, -1 );
            }

            // make one final run
            Vec_IntClear( vLits );
            Vec_IntForEachEntry( vLits2, Lit2, k )
                if ( Lit2 != -1 )
                    Vec_IntPush( vLits, Lit2 );
            status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), nBTLimit, 0, 0, 0 );
            assert( status == l_False );

            // get subset of literals
            nFinal = sat_solver_final( pSat, &pFinal );
            //////////////////////////////////////////////////////////////
        }
        else
        {
            ///////////////////////////////////////////////////////////////
            // check against offset
            status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), nBTLimit, 0, 0, 0 );
            if ( status == l_Undef )
                { RetValue = -1; break; }
            if ( status == l_True )
                break;
            assert( status == l_False );
            // get subset of literals
            nFinal = sat_solver_final( pSat, &pFinal );
            Before = nFinal;
            //printf( "Before %d. ", nFinal );

/*
            // save these literals
            Vec_IntClear( vLits );
            for ( i = 0; i < nFinal; i++ )
                Vec_IntPush( vLits, Abc_LitNot(pFinal[i]) );
            Vec_IntReverseOrder( vLits );

            // make one final run
            status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), nBTLimit, 0, 0, 0 );
            assert( status == l_False );
            nFinal = sat_solver_final( pSat, &pFinal );
*/

            // save these literals
            Vec_IntClear( vLits2 );
            for ( i = 0; i < nFinal; i++ )
                Vec_IntPush( vLits2, Abc_LitNot(pFinal[i]) );
            Vec_IntSort( vLits2, 1 );

            // try removing literals from the cube
            Vec_IntForEachEntry( vLits2, Lit2, k )
            {
                if ( Lit2 == Abc_LitNot(pLits[0]) )
                    continue;
                Vec_IntClear( vLits );
                Vec_IntForEachEntry( vLits2, Lit, n )
                    if ( Lit != -1 && Lit != Lit2 )
                        Vec_IntPush( vLits, Lit );
                // call sat
                status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), nBTLimit, 0, 0, 0 );
                if ( status == l_Undef )
                    assert( 0 );
                if ( status == l_True ) // SAT
                    continue;
                // Lit2 can be removed
                Vec_IntWriteEntry( vLits2, k, -1 );
            }

            // make one final run
            Vec_IntClear( vLits );
            Vec_IntForEachEntry( vLits2, Lit2, k )
                if ( Lit2 != -1 )
                    Vec_IntPush( vLits, Lit2 );
            status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), nBTLimit, 0, 0, 0 );
            assert( status == l_False );

            // put them back
            nFinal = 0;
            Vec_IntForEachEntry( vLits2, Lit2, k )
                if ( Lit2 != -1 )
                    pFinal[nFinal++] = Abc_LitNot(Lit2);        
            /////////////////////////////////////////////////////////
        }


        //printf( "After %d. \n", nFinal );
        After  = nFinal;
        Total += Before - After;

        // get subset of literals
        //nFinal = sat_solver_final( pSat, &pFinal );
        // compute cube and add clause
        Vec_IntClear( vLits );
        Vec_IntPush( vLits, Abc_LitNot(pLits[1]) ); // NOT(iNewLit)
        if ( fDumpPla )
            Vec_StrFill( vCube, Vec_IntSize(vVars), '-' );
        if ( vCubes )
            vLevel = Vec_WecPushLevel( vCubes );
        for ( i = 0; i < nFinal; i++ )
        {
            if ( pFinal[i] == pLits[0] )
                continue;
            Vec_IntPush( vLits, pFinal[i] );
            iVar = Vec_IntFind( vVars, Abc_Lit2Var(pFinal[i]) );   
            assert( iVar >= 0 && iVar < Vec_IntSize(vVars) );
            //printf( "%s%d ", Abc_LitIsCompl(pFinal[i]) ? "+":"-", iVar );
            if ( fDumpPla )
                Vec_StrWriteEntry( vCube, iVar, (char)(!Abc_LitIsCompl(pFinal[i]) ? '0' : '1') );
            if ( vLevel )
                Vec_IntPush( vLevel, Abc_Var2Lit(iVar, !Abc_LitIsCompl(pFinal[i])) );
        }
        if ( vCubes )
            Vec_IntSort( vLevel, 0 );
        if ( fDumpPla )
            printf( "%s 1\n", Vec_StrArray(vCube) );
        status = sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) );
        assert( status );
        nLits += Vec_IntSize(vLevel);
        nIter++;
    }
    if ( fDumpPla )
    printf( ".e\n" );
    if ( fDumpPla )
    printf( ".p %d\n\n", nIter );
    if ( fVerbose )
    printf( "Cubes = %d.  Reduced = %d.  Lits = %d\n", nIter, Total, nLits );
    if ( pCounter )
        *pCounter = nIter;
//    assert( status == l_True );
    Vec_IntFree( vLits );
    Vec_IntFree( vLits2 );
    Vec_StrFree( vCube );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_FxCompute( Gia_Man_t * p )
{
    // create dual-output circuit with on-set/off-set
    extern Gia_Man_t * Gia_ManDupOnsetOffset( Gia_Man_t * p );
    Gia_Man_t * p2 = Gia_ManDupOnsetOffset( p );
    // create SAT solver
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p2, 8, 0, 0, 0, 0 );
    sat_solver * pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    // compute parameters
    int nOuts = Gia_ManCoNum(p);
    int nCiVars = Gia_ManCiNum(p), iCiVarBeg = pCnf->nVars - nCiVars;// - 1;
    int o, i, n, RetValue, nCounter, iAuxVarStart = sat_solver_nvars( pSat );
    int nCubes[2][2] = {{0}};
    // create variables
    Vec_Int_t * vVars = Vec_IntAlloc( nCiVars );
    for ( n = 0; n < nCiVars; n++ )
        Vec_IntPush( vVars, iCiVarBeg + n );
    sat_solver_setnvars( pSat, iAuxVarStart + 4*nOuts );
    // iterate through outputs
    for ( o = 0; o < nOuts; o++ )
    for ( i = 0; i < 2; i++ )
    for ( n = 0; n < 2; n++ ) // 0=onset, 1=offset
//    for ( n = 1; n >= 0; n-- ) // 0=onset, 1=offset
    {
        printf( "Out %3d %sset ", o, n ? "off" : " on" );
        RetValue = Bmc_FxSolve( pSat, Abc_Var2Lit(o, n), iAuxVarStart + 2*i*nOuts + Abc_Var2Lit(o, n), vVars, 0, 0, &nCounter, NULL );
        if ( RetValue == 0 )
            printf( "Mismatch\n" );
        if ( RetValue == -1 )
            printf( "Timeout\n" );
        nCubes[i][n] += nCounter;
    }
    // cleanup
    Vec_IntFree( vVars );
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    Gia_ManStop( p2 );
    printf( "Onset = %5d.   Offset = %5d.      Onset = %5d.   Offset = %5d.\n", nCubes[0][0], nCubes[0][1], nCubes[1][0], nCubes[1][1] );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_FxAddClauses( sat_solver * pSat, Vec_Int_t * vDivs, int iCiVarBeg, int iVarStart )
{
    int i, Func, pLits[3], nDivs = Vec_IntSize(vDivs)/4;
    assert( Vec_IntSize(vDivs) % 4 == 0 );
    // create new var for each divisor
    for ( i = 0; i < nDivs; i++ )
    {
        Func     = Vec_IntEntry(vDivs, 4*i+0);
        pLits[0] = Vec_IntEntry(vDivs, 4*i+1);
        pLits[1] = Vec_IntEntry(vDivs, 4*i+2);
        pLits[2] = Vec_IntEntry(vDivs, 4*i+3);
        //printf( "Adding clause with vars %d %d -> %d\n", iCiVarBeg + Abc_Lit2Var(pLits[0]), iCiVarBeg + Abc_Lit2Var(pLits[1]), iVarStart + nDivs - 1 - i );
        if ( Func == DIV_AND )
            sat_solver_add_and( pSat, 
                iVarStart + nDivs - 1 - i, iCiVarBeg + Abc_Lit2Var(pLits[0]), iCiVarBeg + Abc_Lit2Var(pLits[1]), 
                Abc_LitIsCompl(pLits[0]), Abc_LitIsCompl(pLits[1]), 0 );
        else if ( Func == DIV_XOR )
            sat_solver_add_xor( pSat, 
                iVarStart + nDivs - 1 - i, iCiVarBeg + Abc_Lit2Var(pLits[0]), iCiVarBeg + Abc_Lit2Var(pLits[1]), 0 );
        else if ( Func == DIV_MUX )
            sat_solver_add_mux( pSat, 
                iVarStart + nDivs - 1 - i, iCiVarBeg + Abc_Lit2Var(pLits[0]), iCiVarBeg + Abc_Lit2Var(pLits[1]), iCiVarBeg + Abc_Lit2Var(pLits[2]), 
                Abc_LitIsCompl(pLits[0]), Abc_LitIsCompl(pLits[1]), Abc_LitIsCompl(pLits[2]), 0 );
        else assert( 0 );
    }
}
int Bmc_FxComputeOne( Gia_Man_t * p, int nIterMax, int nDiv2Add )
{
    int Extra    = 1000;
    // create SAT solver
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, 0, 0 );
    sat_solver * pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    // compute parameters
    int nCiVars   = Gia_ManCiNum(p);             // PI count
    int iCiVarBeg = pCnf->nVars - nCiVars;//- 1;   // first PI var
    int iCiVarCur = iCiVarBeg + nCiVars;         // current unused PI var
    int n, Iter, RetValue;
    // create variables
    int iAuxVarStart = sat_solver_nvars(pSat) + Extra; // the aux var
    sat_solver_setnvars( pSat, iAuxVarStart + 1 + nIterMax );
    for ( Iter = 0; Iter < nIterMax; Iter++ )
    {
        Vec_Wec_t * vCubes = Vec_WecAlloc( 1000 );
        // collect variables
        Vec_Int_t * vVar2Sat = Vec_IntAlloc( iCiVarCur-iCiVarBeg ), * vDivs;
//        for ( n = iCiVarCur - 1; n >= iCiVarBeg; n-- )
        for ( n = iCiVarBeg; n < iCiVarCur; n++ )
            Vec_IntPush( vVar2Sat, n );
        // iterate through outputs
        printf( "\nIteration %d (Aux = %d)\n", Iter, iAuxVarStart + Iter );
        RetValue = Bmc_FxSolve( pSat, 0, iAuxVarStart + Iter, vVar2Sat, 1, 1, NULL, vCubes );
        if ( RetValue == 0 )
            printf( "Mismatch\n" );
        if ( RetValue == -1 )
            printf( "Timeout\n" );
        // print cubes
        //Div_CubePrint( vCubes, nCiVars );
        vDivs = Div_CubePairs( vCubes, nCiVars, nDiv2Add );
        Vec_WecFree( vCubes );
        // add clauses and update variables
        Bmc_FxAddClauses( pSat, vDivs, iCiVarBeg, iCiVarCur );
        iCiVarCur += Vec_IntSize(vDivs)/4;
        Vec_IntFree( vDivs );
        // cleanup
        assert( Vec_IntSize(vVar2Sat) <= nCiVars + Extra );
        Vec_IntFree( vVar2Sat );
    }
    // cleanup
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

