/**CFile****************************************************************

  FileName    [cnfUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG-to-CNF conversion.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: cnfUtil.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cnf.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManScanMapping_rec( Cnf_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vMapped )
{
    Aig_Obj_t * pLeaf;
    Dar_Cut_t * pCutBest;
    int aArea, i;
    if ( pObj->nRefs++ || Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
        return 0;
    assert( Aig_ObjIsAnd(pObj) );
    // collect the node first to derive pre-order
    if ( vMapped )
        Vec_PtrPush( vMapped, pObj );
    // visit the transitive fanin of the selected cut
    if ( pObj->fMarkB )
    {
        Vec_Ptr_t * vSuper = Vec_PtrAlloc( 100 );
        Aig_ObjCollectSuper( pObj, vSuper );
        aArea = Vec_PtrSize(vSuper) + 1;
        Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pLeaf, i )
            aArea += Aig_ManScanMapping_rec( p, Aig_Regular(pLeaf), vMapped );
        Vec_PtrFree( vSuper );
        ////////////////////////////
        pObj->fMarkB = 1;
    }
    else
    {
        pCutBest = Dar_ObjBestCut( pObj );
        aArea = Cnf_CutSopCost( p, pCutBest );
        Dar_CutForEachLeaf( p->pManAig, pCutBest, pLeaf, i )
            aArea += Aig_ManScanMapping_rec( p, pLeaf, vMapped );
    }
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description [Collects the nodes in reverse topological order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManScanMapping( Cnf_Man_t * p, int fCollect )
{
    Vec_Ptr_t * vMapped = NULL;
    Aig_Obj_t * pObj;
    int i;
    // clean all references
    Aig_ManForEachObj( p->pManAig, pObj, i )
        pObj->nRefs = 0;
    // allocate the array
    if ( fCollect )
        vMapped = Vec_PtrAlloc( 1000 );
    // collect nodes reachable from POs in the DFS order through the best cuts
    p->aArea = 0;
    Aig_ManForEachCo( p->pManAig, pObj, i )
        p->aArea += Aig_ManScanMapping_rec( p, Aig_ObjFanin0(pObj), vMapped );
//    printf( "Variables = %6d. Clauses = %8d.\n", vMapped? Vec_PtrSize(vMapped) + Aig_ManCiNum(p->pManAig) + 1 : 0, p->aArea + 2 );
    return vMapped;
}

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_ManScanMapping_rec( Cnf_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vMapped, int fPreorder )
{
    Aig_Obj_t * pLeaf;
    Cnf_Cut_t * pCutBest;
    int aArea, i;
    if ( pObj->nRefs++ || Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj) )
        return 0;
    assert( Aig_ObjIsAnd(pObj) );
    assert( pObj->pData != NULL );
    // add the node to the mapping
    if ( vMapped && fPreorder )
         Vec_PtrPush( vMapped, pObj );
    // visit the transitive fanin of the selected cut
    if ( pObj->fMarkB )
    {
        Vec_Ptr_t * vSuper = Vec_PtrAlloc( 100 );
        Aig_ObjCollectSuper( pObj, vSuper );
        aArea = Vec_PtrSize(vSuper) + 1;
        Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pLeaf, i )
            aArea += Cnf_ManScanMapping_rec( p, Aig_Regular(pLeaf), vMapped, fPreorder );
        Vec_PtrFree( vSuper );
        ////////////////////////////
        pObj->fMarkB = 1;
    }
    else
    {
        pCutBest = (Cnf_Cut_t *)pObj->pData;
//        assert( pCutBest->nFanins > 0 );
        assert( pCutBest->Cost < 127 );
        aArea = pCutBest->Cost;
        Cnf_CutForEachLeaf( p->pManAig, pCutBest, pLeaf, i )
            aArea += Cnf_ManScanMapping_rec( p, pLeaf, vMapped, fPreorder );
    }
    // add the node to the mapping
    if ( vMapped && !fPreorder )
         Vec_PtrPush( vMapped, pObj );
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description [Collects the nodes in reverse topological order.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Cnf_ManScanMapping( Cnf_Man_t * p, int fCollect, int fPreorder )
{
    Vec_Ptr_t * vMapped = NULL;
    Aig_Obj_t * pObj;
    int i;
    // clean all references
    Aig_ManForEachObj( p->pManAig, pObj, i )
        pObj->nRefs = 0;
    // allocate the array
    if ( fCollect )
        vMapped = Vec_PtrAlloc( 1000 );
    // collect nodes reachable from POs in the DFS order through the best cuts
    p->aArea = 0;
    Aig_ManForEachCo( p->pManAig, pObj, i )
        p->aArea += Cnf_ManScanMapping_rec( p, Aig_ObjFanin0(pObj), vMapped, fPreorder );
//    printf( "Variables = %6d. Clauses = %8d.\n", vMapped? Vec_PtrSize(vMapped) + Aig_ManCiNum(p->pManAig) + 1 : 0, p->aArea + 2 );
    return vMapped;
}

/**Function*************************************************************

  Synopsis    [Returns the array of CI IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Cnf_DataCollectCiSatNums( Cnf_Dat_t * pCnf, Aig_Man_t * p )
{
    Vec_Int_t * vCiIds;
    Aig_Obj_t * pObj;
    int i;
    vCiIds = Vec_IntAlloc( Aig_ManCiNum(p) );
    Aig_ManForEachCi( p, pObj, i )
        Vec_IntPush( vCiIds, pCnf->pVarNums[pObj->Id] );
    return vCiIds;
}

/**Function*************************************************************

  Synopsis    [Returns the array of CI IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Cnf_DataCollectCoSatNums( Cnf_Dat_t * pCnf, Aig_Man_t * p )
{
    Vec_Int_t * vCoIds;
    Aig_Obj_t * pObj;
    int i;
    vCoIds = Vec_IntAlloc( Aig_ManCoNum(p) );
    Aig_ManForEachCo( p, pObj, i )
        Vec_IntPush( vCoIds, pCnf->pVarNums[pObj->Id] );
    return vCoIds;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned char * Cnf_DataDeriveLitPolarities( Cnf_Dat_t * p )
{
    int i, c, iClaBeg, iClaEnd, * pLit;
    unsigned * pPols0 = ABC_CALLOC( unsigned, Aig_ManObjNumMax(p->pMan) );
    unsigned * pPols1 = ABC_CALLOC( unsigned, Aig_ManObjNumMax(p->pMan) );
    unsigned char * pPres = ABC_CALLOC( unsigned char, p->nClauses );
    for ( i = 0; i < Aig_ManObjNumMax(p->pMan); i++ )
    {
        if ( p->pObj2Count[i] == 0 )
            continue;
        iClaBeg = p->pObj2Clause[i];
        iClaEnd = p->pObj2Clause[i] + p->pObj2Count[i];
        // go through the negative polarity clauses
        for ( c = iClaBeg; c < iClaEnd; c++ )
            for ( pLit = p->pClauses[c]+1; pLit < p->pClauses[c+1]; pLit++ )
                if ( Abc_LitIsCompl(p->pClauses[c][0]) )
                    pPols0[Abc_Lit2Var(*pLit)] |= (unsigned)(2 - Abc_LitIsCompl(*pLit));  // taking the opposite (!) -- not the case
                else
                    pPols1[Abc_Lit2Var(*pLit)] |= (unsigned)(2 - Abc_LitIsCompl(*pLit));  // taking the opposite (!) -- not the case
        // record these clauses
        for ( c = iClaBeg; c < iClaEnd; c++ )
            for ( pLit = p->pClauses[c]+1; pLit < p->pClauses[c+1]; pLit++ )
                if ( Abc_LitIsCompl(p->pClauses[c][0]) )
                    pPres[c] = (unsigned char)( (unsigned)pPres[c] | (pPols0[Abc_Lit2Var(*pLit)] << (2*(pLit-p->pClauses[c]-1))) );
                else
                    pPres[c] = (unsigned char)( (unsigned)pPres[c] | (pPols1[Abc_Lit2Var(*pLit)] << (2*(pLit-p->pClauses[c]-1))) );
        // clean negative polarity
        for ( c = iClaBeg; c < iClaEnd; c++ )
            for ( pLit = p->pClauses[c]+1; pLit < p->pClauses[c+1]; pLit++ )
                pPols0[Abc_Lit2Var(*pLit)] = pPols1[Abc_Lit2Var(*pLit)] = 0;
    }
    ABC_FREE( pPols0 );
    ABC_FREE( pPols1 );
/*
//    for ( c = 0; c < p->nClauses; c++ )
    for ( c = 0; c < 100; c++ )
    {
        printf( "Clause %6d : ", c );
        for ( i = 0; i < 4; i++ )
            printf( "%d ", ((unsigned)pPres[c] >> (2*i)) & 3 );
        printf( "  " );
        for ( pLit = p->pClauses[c]; pLit < p->pClauses[c+1]; pLit++ )
            printf( "%6d ", *pLit );
        printf( "\n" );
    }
*/
    return pPres;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Dat_t * Cnf_DataReadFromFile( char * pFileName )
{
    int MaxLine = 1000000;
    int Var, Lit, nVars = -1, nClas = -1, i, Entry, iLine = 0;
    Cnf_Dat_t * pCnf = NULL;
    Vec_Int_t * vClas = NULL;
    Vec_Int_t * vLits = NULL;
    char * pBuffer, * pToken;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return NULL;
    }
    pBuffer = ABC_ALLOC( char, MaxLine );
    while ( fgets(pBuffer, MaxLine, pFile) != NULL )
    {
        iLine++;
        if ( pBuffer[0] == 'c' )
            continue;
        if ( pBuffer[0] == 'p' )
        {
            pToken = strtok( pBuffer+1, " \t" );
            if ( strcmp(pToken, "cnf") )
            {
                printf( "Incorrect input file.\n" );
                goto finish;
            }
            pToken = strtok( NULL, " \t" );
            nVars = atoi( pToken );
            pToken = strtok( NULL, " \t" );
            nClas = atoi( pToken );
            if ( nVars <= 0 || nClas <= 0 )
            {
                printf( "Incorrect parameters.\n" );
                goto finish;
            }
            // temp storage
            vClas = Vec_IntAlloc( nClas+1 );
            vLits = Vec_IntAlloc( nClas*8 );
            continue;
        }
        pToken = strtok( pBuffer, " \t\r\n" );
        if ( pToken == NULL )
            continue;
        Vec_IntPush( vClas, Vec_IntSize(vLits) );
        while ( pToken )
        {
            Var = atoi( pToken );
            if ( Var == 0 )
                break;
            Lit = (Var > 0) ? Abc_Var2Lit(Var-1, 0) : Abc_Var2Lit(-Var-1, 1);
            if ( Lit >= 2*nVars )
            {
                printf( "Literal %d is out-of-bound for %d variables.\n", Lit, nVars );
                goto finish;
            }
            Vec_IntPush( vLits, Lit );
            pToken = strtok( NULL, " \t\r\n" );
        }
        if ( Var != 0 )
        {
            printf( "There is no zero-terminator in line %d.\n", iLine );
            goto finish;
        }
    }
    // finalize
    if ( Vec_IntSize(vClas) != nClas )
        printf( "Warning! The number of clauses (%d) is different from declaration (%d).\n", Vec_IntSize(vClas), nClas );
    Vec_IntPush( vClas, Vec_IntSize(vLits) );
    // create
    pCnf = ABC_CALLOC( Cnf_Dat_t, 1 );
    pCnf->nVars     = nVars;
    pCnf->nClauses  = Vec_IntSize(vClas)-1;
    pCnf->nLiterals = Vec_IntSize(vLits);
    pCnf->pClauses  = ABC_ALLOC( int *, Vec_IntSize(vClas) );
    pCnf->pClauses[0] = Vec_IntReleaseArray(vLits);
    Vec_IntForEachEntry( vClas, Entry, i )
        pCnf->pClauses[i] = pCnf->pClauses[0] + Entry;
finish:
    fclose( pFile );
    Vec_IntFreeP( &vClas );
    Vec_IntFreeP( &vLits );
    ABC_FREE( pBuffer );
    return pCnf;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_DataSolveFromFile( char * pFileName, int nConfLimit, int nLearnedStart, int nLearnedDelta, int nLearnedPerce, int fVerbose, int fShowPattern, int ** ppModel, int nPis )
{
    abctime clk = Abc_Clock();
    Cnf_Dat_t * pCnf = Cnf_DataReadFromFile( pFileName );
    sat_solver * pSat;
    int i, status, RetValue = -1;
    if ( pCnf == NULL )
        return -1;
    if ( fVerbose )
    {
        printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    // convert into SAT solver
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    if ( pSat == NULL )
    {
        printf( "The problem is trivially UNSAT.\n" );
        Cnf_DataFree( pCnf );
        return 1;
    }
    if ( nLearnedStart )
        pSat->nLearntStart = pSat->nLearntMax = nLearnedStart;
    if ( nLearnedDelta )
        pSat->nLearntDelta = nLearnedDelta;
    if ( nLearnedPerce )
        pSat->nLearntRatio = nLearnedPerce;
    if ( fVerbose )
        pSat->fVerbose = fVerbose;

//sat_solver_start_cardinality( pSat, 100 );

    // solve the miter
    status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfLimit, 0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( status == l_Undef )
        RetValue = -1;
    else if ( status == l_True )
        RetValue = 0;
    else if ( status == l_False )
        RetValue = 1;
    else
        assert( 0 );
    if ( fVerbose )
        Sat_SolverPrintStats( stdout, pSat );
    if ( RetValue == -1 )
        Abc_Print( 1, "UNDECIDED      " );
    else if ( RetValue == 0 )
        Abc_Print( 1, "SATISFIABLE    " );
    else
        Abc_Print( 1, "UNSATISFIABLE  " );
    //Abc_Print( -1, "\n" );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // derive SAT assignment
    if ( RetValue == 0 && nPis > 0 )
    {
        *ppModel = ABC_ALLOC( int, nPis );
        for ( i = 0; i < nPis; i++ )
            (*ppModel)[i] = sat_solver_var_value( pSat, pCnf->nVars - nPis + i );
    }
    if ( RetValue == 0 && fShowPattern )
    {
        for ( i = 0; i < pCnf->nVars; i++ )
            printf( "%d", sat_solver_var_value(pSat,i) );
        printf( "\n" );
    }
    Cnf_DataFree( pCnf );
    sat_solver_delete( pSat );
    return RetValue;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

