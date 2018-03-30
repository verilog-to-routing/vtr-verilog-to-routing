/**CFile****************************************************************

  FileName    [pdrInv.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Property driven reachability.]

  Synopsis    [Invariant computation, printing, verification.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 20, 2010.]

  Revision    [$Id: pdrInv.c,v 1.00 2010/11/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "pdrInt.h"
#include "base/abc/abc.h"      // for Abc_NtkCollectCioNames()
#include "base/main/main.h"    // for Abc_FrameReadGlobalFrame()
#include "aig/ioa/ioa.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pdr_ManPrintProgress( Pdr_Man_t * p, int fClose, abctime Time )
{
    Vec_Ptr_t * vVec;
    int i, ThisSize, Length, LengthStart;
    if ( Vec_PtrSize(p->vSolvers) < 2 )
    {
        Abc_Print(1, "Frame " );
        Abc_Print(1, "Clauses                                                     " );
        Abc_Print(1, "Max Queue " );
        Abc_Print(1, "Flops " );
        Abc_Print(1, "Cex      " );
        Abc_Print(1, "Time" );
        Abc_Print(1, "\n" );
        return;
    }
    if ( Abc_FrameIsBatchMode() && !fClose )
        return;
    // count the total length of the printout
    Length = 0;
    Vec_VecForEachLevel( p->vClauses, vVec, i )
        Length += 1 + Abc_Base10Log(Vec_PtrSize(vVec)+1);
    // determine the starting point
    LengthStart = Abc_MaxInt( 0, Length - 60 );
    Abc_Print( 1, "%3d :", Vec_PtrSize(p->vSolvers)-1 );
    ThisSize = 5;
    if ( LengthStart > 0 )
    {
        Abc_Print( 1, " ..." );
        ThisSize += 4;
    }
    Length = 0;
    Vec_VecForEachLevel( p->vClauses, vVec, i )
    {
        if ( Length < LengthStart )
        {
            Length += 1 + Abc_Base10Log(Vec_PtrSize(vVec)+1);
            continue;
        }
        Abc_Print( 1, " %d", Vec_PtrSize(vVec) );
        Length += 1 + Abc_Base10Log(Vec_PtrSize(vVec)+1);
        ThisSize += 1 + Abc_Base10Log(Vec_PtrSize(vVec)+1);
    }
    for ( i = ThisSize; i < 70; i++ )
        Abc_Print( 1, " " );
    Abc_Print( 1, "%5d", p->nQueMax );
    Abc_Print( 1, "%6d", p->vAbsFlops ? Vec_IntCountPositive(p->vAbsFlops) : p->nAbsFlops );
    if ( p->pPars->fUseAbs )
    Abc_Print( 1, "%4d", p->nCexes );
    Abc_Print( 1, "%10.2f sec", 1.0*Time/CLOCKS_PER_SEC );
    if ( p->pPars->fSolveAll )
        Abc_Print( 1, "  CEX =%4d", p->pPars->nFailOuts );
    if ( p->pPars->nTimeOutOne )
        Abc_Print( 1, "  T/O =%3d", p->pPars->nDropOuts );
    Abc_Print( 1, "%s", fClose ? "\n":"\r" );
    if ( fClose )
        p->nQueMax = 0, p->nCexes = 0;
    fflush( stdout );
}

/**Function*************************************************************

  Synopsis    [Counts how many times each flop appears in the set of cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Pdr_ManCountFlops( Pdr_Man_t * p, Vec_Ptr_t * vCubes )
{
    Vec_Int_t * vFlopCount;
    Pdr_Set_t * pCube;
    int i, n;
    vFlopCount = Vec_IntStart( Aig_ManRegNum(p->pAig) );
    Vec_PtrForEachEntry( Pdr_Set_t *, vCubes, pCube, i )
    {
        if ( pCube->nRefs == -1 )
            continue;
        for ( n = 0; n < pCube->nLits; n++ )
        {
            assert( pCube->Lits[n] >= 0 && pCube->Lits[n] < 2*Aig_ManRegNum(p->pAig) );
            Vec_IntAddToEntry( vFlopCount, pCube->Lits[n] >> 1, 1 );
        }
    }
    return vFlopCount;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pdr_ManFindInvariantStart( Pdr_Man_t * p )
{
    Vec_Ptr_t * vArrayK;
    int k, kMax = Vec_PtrSize(p->vSolvers)-1;
    Vec_VecForEachLevelStartStop( p->vClauses, vArrayK, k, 1, kMax+1 )
        if ( Vec_PtrSize(vArrayK) == 0 )
            return k;
//    return -1;
    // if there is no starting point (as in case of SAT or undecided), return the last frame
//    Abc_Print( 1, "The last timeframe contains %d clauses.\n", Vec_PtrSize(Vec_VecEntry(p->vClauses, kMax)) );
    return kMax;
}

/**Function*************************************************************

  Synopsis    [Counts the number of variables used in the clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Pdr_ManCollectCubes( Pdr_Man_t * p, int kStart )
{
    Vec_Ptr_t * vResult;
    Vec_Ptr_t * vArrayK;
    Pdr_Set_t * pSet;
    int i, j;
    vResult = Vec_PtrAlloc( 100 );
    Vec_VecForEachLevelStart( p->vClauses, vArrayK, i, kStart )
        Vec_PtrForEachEntry( Pdr_Set_t *, vArrayK, pSet, j )
            Vec_PtrPush( vResult, pSet );
    return vResult;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Pdr_ManCountFlopsInv( Pdr_Man_t * p )
{
    int kStart = Pdr_ManFindInvariantStart(p);
    Vec_Ptr_t *vCubes = Pdr_ManCollectCubes(p, kStart);
    Vec_Int_t * vInv = Pdr_ManCountFlops( p, vCubes );
    Vec_PtrFree(vCubes);
    return vInv;
}

/**Function*************************************************************

  Synopsis    [Counts the number of variables used in the clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pdr_ManCountVariables( Pdr_Man_t * p, int kStart )
{
    Vec_Int_t * vFlopCounts;
    Vec_Ptr_t * vCubes;
    int i, Entry, Counter = 0;
    if ( p->vInfCubes == NULL )
        vCubes = Pdr_ManCollectCubes( p, kStart );
    else
        vCubes = Vec_PtrDup( p->vInfCubes );
    vFlopCounts = Pdr_ManCountFlops( p, vCubes );
    Vec_IntForEachEntry( vFlopCounts, Entry, i )
        Counter += (Entry > 0);
    Vec_IntFreeP( &vFlopCounts );
    Vec_PtrFree( vCubes );
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pdr_ManPrintClauses( Pdr_Man_t * p, int kStart )
{
    Vec_Ptr_t * vArrayK;
    Pdr_Set_t * pCube;
    int i, k, Counter = 0;
    Vec_VecForEachLevelStart( p->vClauses, vArrayK, k, kStart )
    {
        Vec_PtrSort( vArrayK, (int (*)(void))Pdr_SetCompare );
        Vec_PtrForEachEntry( Pdr_Set_t *, vArrayK, pCube, i )
        {
            Abc_Print( 1, "C=%4d. F=%4d ", Counter++, k );
            Pdr_SetPrint( stdout, pCube, Aig_ManRegNum(p->pAig), NULL );  
            Abc_Print( 1, "\n" ); 
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
void Pdr_SetPrintOne( Pdr_Set_t * p )
{
    int i;
    Abc_Print(1, "Clause: {" );
    for ( i = 0; i < p->nLits; i++ )
        Abc_Print(1, " %s%d", Abc_LitIsCompl(p->Lits[i])? "!":"", Abc_Lit2Var(p->Lits[i]) );
    Abc_Print(1, " }\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
Aig_Man_t * Pdr_ManDupAigWithClauses( Aig_Man_t * p, Vec_Ptr_t * vCubes )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pObjNew, * pLit;
    Pdr_Set_t * pCube;
    int i, n;
    // create the new manager
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // create the PIs
    Aig_ManCleanData( p );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    // create outputs for each cube
    Vec_PtrForEachEntry( Pdr_Set_t *, vCubes, pCube, i )
    {
//        Pdr_SetPrintOne( pCube );
        pObjNew = Aig_ManConst1(pNew);
        for ( n = 0; n < pCube->nLits; n++ )
        {
            assert( Abc_Lit2Var(pCube->Lits[n]) < Saig_ManRegNum(p) );
            pLit = Aig_NotCond( Aig_ManCi(pNew, Saig_ManPiNum(p) + Abc_Lit2Var(pCube->Lits[n])), Abc_LitIsCompl(pCube->Lits[n]) );
            pObjNew = Aig_And( pNew, pObjNew, pLit );
        }
        Aig_ObjCreateCo( pNew, pObjNew );
    }
    // duplicate internal nodes
    Aig_ManForEachNode( p, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // add the POs
    Saig_ManForEachLi( p, pObj, i )
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    Aig_ManCleanup( pNew );
    Aig_ManSetRegNum( pNew, Aig_ManRegNum(p) );
    // check the resulting network
    if ( !Aig_ManCheck(pNew) )
        Abc_Print(1, "Aig_ManDupSimple(): The check has failed.\n" );
    return pNew;
}
void Pdr_ManDumpAig( Aig_Man_t * p, Vec_Ptr_t * vCubes )
{
    Aig_Man_t * pNew = Pdr_ManDupAigWithClauses( p, vCubes );
    Ioa_WriteAiger( pNew, "aig_with_clauses.aig", 0, 0 );
    Aig_ManStop( pNew );
    Abc_Print(1, "Dumped modified AIG into file \"aig_with_clauses.aig\".\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
void Pdr_ManDumpClauses( Pdr_Man_t * p, char * pFileName, int fProved )
{
    FILE * pFile;
    Vec_Int_t * vFlopCounts;
    Vec_Ptr_t * vCubes;
    Pdr_Set_t * pCube;
    char ** pNamesCi;
    int i, kStart, Count = 0;
    // create file
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        Abc_Print( 1, "Cannot open file \"%s\" for writing invariant.\n", pFileName );
        return;
    } 
    // collect cubes
    kStart = Pdr_ManFindInvariantStart( p );
    if ( fProved )
        vCubes = Pdr_ManCollectCubes( p, kStart );
    else
        vCubes = Vec_PtrDup( p->vInfCubes );
    Vec_PtrSort( vCubes, (int (*)(void))Pdr_SetCompare );
//    Pdr_ManDumpAig( p->pAig, vCubes );
    // count cubes
    Count = 0;
    Vec_PtrForEachEntry( Pdr_Set_t *, vCubes, pCube, i )
    {
        if ( pCube->nRefs == -1 )
            continue;
        Count++;
    }
    // collect variable appearances
    vFlopCounts = p->pPars->fUseSupp ? Pdr_ManCountFlops( p, vCubes ) : NULL; 
    // output the header
    if ( fProved )
        fprintf( pFile, "# Inductive invariant for \"%s\"\n", p->pAig->pName );
    else
        fprintf( pFile, "# Clauses of the last timeframe for \"%s\"\n", p->pAig->pName );
    fprintf( pFile, "# generated by PDR in ABC on %s\n", Aig_TimeStamp() );
    fprintf( pFile, ".i %d\n", p->pPars->fUseSupp ? Pdr_ManCountVariables(p, kStart) : Aig_ManRegNum(p->pAig) );
    fprintf( pFile, ".o 1\n" );
    fprintf( pFile, ".p %d\n", Count );
    // output flop names
    pNamesCi = Abc_NtkCollectCioNames( Abc_FrameReadNtk( Abc_FrameReadGlobalFrame() ), 0 );
    if ( pNamesCi )
    {
        fprintf( pFile, ".ilb" );
        for ( i = 0; i < Aig_ManRegNum(p->pAig); i++ )
            if ( !p->pPars->fUseSupp || Vec_IntEntry( vFlopCounts, i ) )
                fprintf( pFile, " %s", pNamesCi[Saig_ManPiNum(p->pAig) + i] );
        fprintf( pFile, "\n" );
        ABC_FREE( pNamesCi );
        fprintf( pFile, ".ob inv\n" );
    }
    // output cubes
    Vec_PtrForEachEntry( Pdr_Set_t *, vCubes, pCube, i )
    {
        if ( pCube->nRefs == -1 )
            continue;
        Pdr_SetPrint( pFile, pCube, Aig_ManRegNum(p->pAig), vFlopCounts );  
        fprintf( pFile, " 1\n" ); 
    }
    fprintf( pFile, ".e\n\n" );
    fclose( pFile );
    Vec_IntFreeP( &vFlopCounts );
    Vec_PtrFree( vCubes );
    if ( fProved )
        Abc_Print( 1, "Inductive invariant was written into file \"%s\".\n", pFileName );
    else
        Abc_Print( 1, "Clauses of the last timeframe were written into file \"%s\".\n", pFileName );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
Vec_Str_t * Pdr_ManDumpString( Pdr_Man_t * p )
{
    Vec_Str_t * vStr;
    Vec_Int_t * vFlopCounts;
    Vec_Ptr_t * vCubes;
    Pdr_Set_t * pCube;
    int i, kStart;
    vStr = Vec_StrAlloc( 1000 );
    // collect cubes
    kStart = Pdr_ManFindInvariantStart( p );
    if ( p->vInfCubes == NULL )
        vCubes = Pdr_ManCollectCubes( p, kStart );
    else
        vCubes = Vec_PtrDup( p->vInfCubes );
    Vec_PtrSort( vCubes, (int (*)(void))Pdr_SetCompare );
    // collect variable appearances
    vFlopCounts = p->pPars->fUseSupp ? Pdr_ManCountFlops( p, vCubes ) : NULL; 
    // output cubes
    Vec_PtrForEachEntry( Pdr_Set_t *, vCubes, pCube, i )
    {
        if ( pCube->nRefs == -1 )
            continue;
        Pdr_SetPrintStr( vStr, pCube, Aig_ManRegNum(p->pAig), vFlopCounts );  
    }
    Vec_IntFreeP( &vFlopCounts );
    Vec_PtrFree( vCubes );
    Vec_StrPush( vStr, '\0' );
    return vStr;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pdr_ManReportInvariant( Pdr_Man_t * p )
{
    Vec_Ptr_t * vCubes;
    int kStart = Pdr_ManFindInvariantStart( p );
    vCubes = Pdr_ManCollectCubes( p, kStart );
    Abc_Print( 1, "Invariant F[%d] : %d clauses with %d flops (out of %d) (cex = %d, ave = %.2f)\n", 
        kStart, Vec_PtrSize(vCubes), Pdr_ManCountVariables(p, kStart), Aig_ManRegNum(p->pAig), p->nCexesTotal, 1.0*p->nXsimLits/p->nXsimRuns );
//    Abc_Print( 1, "Invariant F[%d] : %d clauses with %d flops (out of %d)\n", 
//        kStart, Vec_PtrSize(vCubes), Pdr_ManCountVariables(p, kStart), Aig_ManRegNum(p->pAig) );
    Vec_PtrFree( vCubes );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pdr_ManVerifyInvariant( Pdr_Man_t * p )
{
    sat_solver * pSat;
    Vec_Int_t * vLits;
    Vec_Ptr_t * vCubes;
    Pdr_Set_t * pCube;
    int i, kStart, kThis, RetValue, Counter = 0;
    abctime clk = Abc_Clock();
    // collect cubes used in the inductive invariant
    kStart = Pdr_ManFindInvariantStart( p );
    vCubes = Pdr_ManCollectCubes( p, kStart );
    // create solver with the cubes
    kThis = Vec_PtrSize(p->vSolvers);
    pSat  = Pdr_ManCreateSolver( p, kThis );
    // add the property output
//    Pdr_ManSetPropertyOutput( p, kThis );
    // add the clauses
    Vec_PtrForEachEntry( Pdr_Set_t *, vCubes, pCube, i )
    {
        vLits = Pdr_ManCubeToLits( p, kThis, pCube, 1, 0 );
        RetValue = sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) );
        assert( RetValue );
        sat_solver_compress( pSat );
    }
    // check each clause
    Vec_PtrForEachEntry( Pdr_Set_t *, vCubes, pCube, i )
    {
        vLits = Pdr_ManCubeToLits( p, kThis, pCube, 0, 1 );
        RetValue = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), 0, 0, 0, 0 );
        if ( RetValue != l_False )
        {
            Abc_Print( 1, "Verification of clause %d failed.\n", i );
            Counter++;
        }
    }
    if ( Counter )
        Abc_Print( 1, "Verification of %d clauses has failed.\n", Counter );
    else
    {
        Abc_Print( 1, "Verification of invariant with %d clauses was successful.  ", Vec_PtrSize(vCubes) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
//    sat_solver_delete( pSat );
    Vec_PtrFree( vCubes );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pdr_ManDeriveMarkNonInductive( Pdr_Man_t * p, Vec_Ptr_t * vCubes )
{
    sat_solver * pSat;
    Vec_Int_t * vLits;
    Pdr_Set_t * pCube;
    int i, kThis, RetValue, fChanges = 0, Counter = 0;
    // create solver with the cubes
    kThis = Vec_PtrSize(p->vSolvers);
    pSat  = Pdr_ManCreateSolver( p, kThis );
    // add the clauses
    Vec_PtrForEachEntry( Pdr_Set_t *, vCubes, pCube, i )
    {
        if ( pCube->nRefs == -1 ) // skip non-inductive
            continue;
        vLits = Pdr_ManCubeToLits( p, kThis, pCube, 1, 0 );
        RetValue = sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) );
        assert( RetValue );
        sat_solver_compress( pSat );
    }
    // check each clause
    Vec_PtrForEachEntry( Pdr_Set_t *, vCubes, pCube, i )
    {
        if ( pCube->nRefs == -1 ) // skip non-inductive
            continue;
        vLits = Pdr_ManCubeToLits( p, kThis, pCube, 0, 1 );
        RetValue = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), 0, 0, 0, 0 );
        if ( RetValue != l_False ) // mark as non-inductive
        {
            pCube->nRefs = -1;
            fChanges = 1;
        }
        else
            Counter++;
    }
    //Abc_Print(1, "Clauses = %d.\n", Counter );
    //sat_solver_delete( pSat );
    return fChanges;
}
Vec_Int_t * Pdr_ManDeriveInfinityClauses( Pdr_Man_t * p, int fReduce )
{
    Vec_Int_t * vResult;
    Vec_Ptr_t * vCubes;
    Pdr_Set_t * pCube;
    int i, v, kStart;
    // collect cubes used in the inductive invariant
    kStart = Pdr_ManFindInvariantStart( p );
    vCubes = Pdr_ManCollectCubes( p, kStart );
    // refine as long as there are changes
    if ( fReduce )
        while ( Pdr_ManDeriveMarkNonInductive(p, vCubes) );
    // collect remaining clauses
    vResult = Vec_IntAlloc( 1000 );
    Vec_IntPush( vResult, 0 );
    Vec_PtrForEachEntry( Pdr_Set_t *, vCubes, pCube, i )
    {
        if ( pCube->nRefs == -1 ) // skip non-inductive
            continue;
        Vec_IntAddToEntry( vResult, 0, 1 );
        Vec_IntPush( vResult, pCube->nLits );
        for ( v = 0; v < pCube->nLits; v++ )
            Vec_IntPush( vResult, pCube->Lits[v] );
    }
    //Vec_PtrFree( vCubes );
    Vec_PtrFreeP( &p->vInfCubes );
    p->vInfCubes = vCubes;
    Vec_IntPush( vResult, Aig_ManRegNum(p->pAig) );
    return vResult;
}



/**Function*************************************************************

  Synopsis    [Remove clauses while maintaining the invariant.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#define Pdr_ForEachCube( pList, pCut, i ) for ( i = 0, pCut = pList + 1; i < pList[0]; i++, pCut += pCut[0] + 1 )

Vec_Int_t * Pdr_InvMap( Vec_Int_t * vCounts )
{
    int i, k = 0, Count;
    Vec_Int_t * vMap = Vec_IntStart( Vec_IntSize(vCounts) );
    Vec_IntForEachEntry( vCounts, Count, i )
        if ( Count )
            Vec_IntWriteEntry( vMap, i, k++ );
    return vMap;
}
Vec_Int_t * Pdr_InvCounts( Vec_Int_t * vInv )
{
    int i, k, * pCube, * pList = Vec_IntArray(vInv);
    Vec_Int_t * vCounts = Vec_IntStart( Vec_IntEntryLast(vInv) );
    Pdr_ForEachCube( pList, pCube, i )
        for ( k = 0; k < pCube[0]; k++ )
            Vec_IntAddToEntry( vCounts, Abc_Lit2Var(pCube[k+1]), 1 );
    return vCounts;
}
int Pdr_InvUsedFlopNum( Vec_Int_t * vInv )
{
    Vec_Int_t * vCounts = Pdr_InvCounts( vInv );
    int nZeros = Vec_IntCountZero( vCounts );
    Vec_IntFree( vCounts );
    return Vec_IntEntryLast(vInv) - nZeros;
}

Vec_Str_t * Pdr_InvPrintStr( Vec_Int_t * vInv, Vec_Int_t * vCounts )
{
    Vec_Str_t * vStr = Vec_StrAlloc( 1000 );
    Vec_Int_t * vMap = Pdr_InvMap( vCounts );
    int nVars = Vec_IntSize(vCounts) - Vec_IntCountZero(vCounts);
    int i, k, * pCube, * pList = Vec_IntArray(vInv);
    char * pBuffer = ABC_ALLOC( char, nVars );
    for ( i = 0; i < nVars; i++ )
        pBuffer[i] = '-';
    Pdr_ForEachCube( pList, pCube, i )
    {
        for ( k = 0; k < pCube[0]; k++ )
            pBuffer[Vec_IntEntry(vMap, Abc_Lit2Var(pCube[k+1]))] = '0' + !Abc_LitIsCompl(pCube[k+1]);
        for ( k = 0; k < nVars; k++ )
            Vec_StrPush( vStr, pBuffer[k] );
        Vec_StrPush( vStr, ' ' );
        Vec_StrPush( vStr, '1' );
        Vec_StrPush( vStr, '\n' );
        for ( k = 0; k < pCube[0]; k++ )
            pBuffer[Vec_IntEntry(vMap, Abc_Lit2Var(pCube[k+1]))] = '-';
    }
    Vec_StrPush( vStr, '\0' );
    ABC_FREE( pBuffer );
    Vec_IntFree( vMap );
    return vStr;
}
void Pdr_InvPrint( Vec_Int_t * vInv, int fVerbose )
{
    Abc_Print(1, "Invariant contains %d clauses with %d literals and %d flops (out of %d).\n", Vec_IntEntry(vInv, 0), Vec_IntSize(vInv)-Vec_IntEntry(vInv, 0)-2, Pdr_InvUsedFlopNum(vInv), Vec_IntEntryLast(vInv) );
    if ( fVerbose )
    {
        Vec_Int_t * vCounts = Pdr_InvCounts( vInv );
        Vec_Str_t * vStr = Pdr_InvPrintStr( vInv, vCounts );
        Abc_Print(1, "%s", Vec_StrArray( vStr ) );
        Vec_IntFree( vCounts );
        Vec_StrFree( vStr );
    }
}

int Pdr_InvCheck_int( Gia_Man_t * p, Vec_Int_t * vInv, int fVerbose, sat_solver * pSat, int fSkip )
{
    int nBTLimit = 0;
    int fCheckProperty = 1;
    int i, k, status, nFailed = 0, nFailedOuts = 0; 
    // collect cubes
    int * pCube, * pList = Vec_IntArray(vInv);
    // create variables
    Vec_Int_t * vLits = Vec_IntAlloc(100);
    int iFoVarBeg = sat_solver_nvars(pSat) - Gia_ManRegNum(p);
    int iFiVarBeg = 1 + Gia_ManPoNum(p);
    // add cubes
    Pdr_ForEachCube( pList, pCube, i )
    {
        // collect literals
        Vec_IntClear( vLits );
        for ( k = 0; k < pCube[0]; k++ )
            if ( pCube[k+1] != -1 )
                Vec_IntPush( vLits, Abc_Var2Lit(iFoVarBeg + Abc_Lit2Var(pCube[k+1]), !Abc_LitIsCompl(pCube[k+1])) );
        if ( Vec_IntSize(vLits) == 0 )
        {
            Vec_IntFree( vLits );
            return 1;
        }
        // add it to the solver
        status = sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits) );
        assert( status == 1 );
    }
    // verify property output
    if ( fCheckProperty )
    {
        for ( i = 0; i < Gia_ManPoNum(p); i++ )
        {
            Vec_IntFill( vLits, 1, Abc_Var2Lit(1+i, 0) );
            status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits), nBTLimit, 0, 0, 0 );
            if ( status == l_Undef ) // timeout
                break;
            if ( status == l_True ) // sat - property fails
            {
                if ( fVerbose )
                    Abc_Print(1, "Coverage check failed for output %d.\n", i );
                nFailedOuts++;
                if ( fSkip )
                {
                    Vec_IntFree( vLits );
                    return 1;
                }
                continue;
            }
            assert( status == l_False ); // unsat - property holds
        }
    }
    // iterate through cubes in the direct order
    Pdr_ForEachCube( pList, pCube, i )
    {
        // collect cube
        Vec_IntClear( vLits );
        for ( k = 0; k < pCube[0]; k++ )
            if ( pCube[k+1] != -1 )
                Vec_IntPush( vLits, Abc_Var2Lit(iFiVarBeg + Abc_Lit2Var(pCube[k+1]), Abc_LitIsCompl(pCube[k+1])) );
        // check if this cube intersects with the complement of other cubes in the solver
        // if it does not intersect, then it is redundant and can be skipped
        status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits), nBTLimit, 0, 0, 0 );
        if ( status != l_True && fVerbose )
            Abc_Print(1, "Finished checking clause %d (out of %d)...\r", i, pList[0] );
        if ( status == l_Undef ) // timeout
            break;
        if ( status == l_False ) // unsat -- correct
            continue;
        assert( status == l_True );
        if ( fVerbose )
            Abc_Print(1, "Inductiveness check failed for clause %d.\n", i );
        nFailed++;
        if ( fSkip )
        {
            Vec_IntFree( vLits );
            return 1;
        }
    }
    Vec_IntFree( vLits );
    return nFailed + nFailedOuts;
}

int Pdr_InvCheck( Gia_Man_t * p, Vec_Int_t * vInv, int fVerbose )
{
    int RetValue;
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, 0, 0 );
    sat_solver * pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    assert( sat_solver_nvars(pSat) == pCnf->nVars );
    Cnf_DataFree( pCnf );
    RetValue = Pdr_InvCheck_int( p, vInv, fVerbose, pSat, 0 );
    sat_solver_delete( pSat );
    return RetValue;
}

Vec_Int_t * Pdr_InvMinimize( Gia_Man_t * p, Vec_Int_t * vInv, int fVerbose )
{
    int nBTLimit = 0;
    int fCheckProperty = 1;
    abctime clk = Abc_Clock();
    int n, i, k, status, nLits, fFailed = 0, fCannot = 0, nRemoved = 0; 
    Vec_Int_t * vRes = NULL;
    // create SAT solver
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, 0, 0 );
    sat_solver * pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    int * pCube, * pList = Vec_IntArray(vInv), nCubes = pList[0];
    // create variables
    Vec_Int_t * vLits = Vec_IntAlloc(100);
    Vec_Bit_t * vRemoved = Vec_BitStart( nCubes );
    int iFoVarBeg = pCnf->nVars - Gia_ManRegNum(p);
    int iFiVarBeg = 1 + Gia_ManPoNum(p);
    int iAuxVarBeg = sat_solver_nvars(pSat);
    // allocate auxiliary variables
    assert( sat_solver_nvars(pSat) == pCnf->nVars );
    sat_solver_setnvars( pSat, sat_solver_nvars(pSat) + nCubes );
    // add clauses
    Pdr_ForEachCube( pList, pCube, i )
    {
        // collect literals
        Vec_IntFill( vLits, 1, Abc_Var2Lit(iAuxVarBeg + i, 1) ); // neg aux literal
        for ( k = 0; k < pCube[0]; k++ )
            Vec_IntPush( vLits, Abc_Var2Lit(iFoVarBeg + Abc_Lit2Var(pCube[k+1]), !Abc_LitIsCompl(pCube[k+1])) );
        // add it to the solver
        status = sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits) );
        assert( status == 1 );
    }
    // iterate through clauses 
    Pdr_ForEachCube( pList, pCube, i )
    {
        if ( Vec_BitEntry(vRemoved, i) )
            continue;
        // collect aux literals for remaining clauses
        Vec_IntClear( vLits );
        for ( k = 0; k < nCubes; k++ )
            if ( k != i && !Vec_BitEntry(vRemoved, k) ) // skip this cube and already removed cubes
                Vec_IntPush( vLits, Abc_Var2Lit(iAuxVarBeg + k, 0) ); // pos aux literal
        nLits = Vec_IntSize( vLits );
        // check if the property still holds
        if ( fCheckProperty )
        {
            for ( k = 0; k < Gia_ManPoNum(p); k++ )
            {
                Vec_IntShrink( vLits, nLits );
                Vec_IntPush( vLits, Abc_Var2Lit(1+k, 0) );
                status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits), nBTLimit, 0, 0, 0 );
                if ( status == l_Undef ) // timeout
                {
                    fFailed = 1;
                    break;
                }
                if ( status == l_True ) // sat - property fails
                    break;
                assert( status == l_False ); // unsat - property holds
            }
            if ( fFailed )
                break;
            if ( k < Gia_ManPoNum(p) )
                continue;
        }
        // check other clauses
        fCannot = 0;
        Pdr_ForEachCube( pList, pCube, n )
        {
            if ( Vec_BitEntry(vRemoved, n) || n == i )
                continue;
            // collect cube
            Vec_IntShrink( vLits, nLits );
            for ( k = 0; k < pCube[0]; k++ )
               Vec_IntPush( vLits, Abc_Var2Lit(iFiVarBeg + Abc_Lit2Var(pCube[k+1]), Abc_LitIsCompl(pCube[k+1])) );
            // check if this cube intersects with the complement of other cubes in the solver
            // if it does not intersect, then it is redundant and can be skipped
            status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits), nBTLimit, 0, 0, 0 );
            if ( status == l_Undef ) // timeout
            {
                fFailed = 1;
                break;
            }
            if ( status == l_False ) // unsat -- correct
                continue;
            assert( status == l_True );
            // cannot remove
            fCannot = 1;
            break;
        }
        if ( fFailed )
            break;
        if ( fCannot )
            continue;
        if ( fVerbose )
        Abc_Print(1, "Removing clause %d.\n", i );
        Vec_BitWriteEntry( vRemoved, i, 1 );
        nRemoved++;
    }
    if ( nRemoved )
        Abc_Print(1, "Invariant minimization reduced %d clauses (out of %d).  ", nRemoved, nCubes );
    else
        Abc_Print(1, "Invariant minimization did not change the invariant.  " ); 
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // cleanup cover
    if ( !fFailed && nRemoved > 0 ) // finished without timeout and removed some cubes
    {
        vRes = Vec_IntAlloc( 1000 );
        Vec_IntPush( vRes, nCubes-nRemoved );
        Pdr_ForEachCube( pList, pCube, i )
            if ( !Vec_BitEntry(vRemoved, i) )
                for ( k = 0; k <= pCube[0]; k++ )
                    Vec_IntPush( vRes, pCube[k] );
        Vec_IntPush( vRes, Vec_IntEntryLast(vInv) );
    }
    Cnf_DataFree( pCnf );
    sat_solver_delete( pSat );
    Vec_BitFree( vRemoved );
    Vec_IntFree( vLits );
    return vRes;
}

Vec_Int_t * Pdr_InvMinimizeLits( Gia_Man_t * p, Vec_Int_t * vInv, int fVerbose )
{
    Vec_Int_t * vRes = NULL;
    abctime clk = Abc_Clock();
    int i, k, nLits = 0, * pCube, * pList = Vec_IntArray(vInv), nRemoved = 0;
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, 0, 0 );
    sat_solver * pSat;
//    sat_solver * pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    Pdr_ForEachCube( pList, pCube, i )
    {
        nLits += pCube[0];
        for ( k = 0; k < pCube[0]; k++ )
        {
            int Save = pCube[k+1];
            pCube[k+1] = -1;
            //sat_solver_bookmark( pSat );
            pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
            if ( Pdr_InvCheck_int(p, vInv, 0, pSat, 1) )
                pCube[k+1] = Save;
            else
            {
                if ( fVerbose )
                Abc_Print(1, "Removing lit %d from clause %d.\n", k, i );
                nRemoved++;
            }
            sat_solver_delete( pSat );
            //sat_solver_rollback( pSat );
            //sat_solver_bookmark( pSat );
        }
    }
    Cnf_DataFree( pCnf );
    //sat_solver_delete( pSat );
    if ( nRemoved )
        Abc_Print(1, "Invariant minimization reduced %d literals (out of %d).  ", nRemoved, nLits );
    else
        Abc_Print(1, "Invariant minimization did not change the invariant.  " ); 
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( nRemoved > 0 ) // finished without timeout and removed some lits
    {
        vRes = Vec_IntAlloc( 1000 );
        Vec_IntPush( vRes, pList[0] );
        Pdr_ForEachCube( pList, pCube, i )
        {
            int nLits = 0;
            for ( k = 0; k < pCube[0]; k++ )
                if ( pCube[k+1] != -1 )
                    nLits++;
            Vec_IntPush( vRes, nLits );
            for ( k = 0; k < pCube[0]; k++ )
                if ( pCube[k+1] != -1 )
                    Vec_IntPush( vRes, pCube[k+1] );
        }
        Vec_IntPush( vRes, Vec_IntEntryLast(vInv) );
    }
    return vRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

