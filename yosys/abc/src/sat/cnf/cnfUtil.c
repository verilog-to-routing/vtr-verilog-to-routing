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

#ifdef _MSC_VER
#define unlink _unlink
#else
#include <unistd.h>
#endif

#ifdef ABC_USE_PTHREADS

#ifdef _WIN32
#include "../lib/pthread.h"
#else
#include <pthread.h>
#endif

#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Vec_Int_t *Exa4_ManParse(char *pFileName);

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Solving problems using one core.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t *Cnf_RunSolverOnce(int Id, int Rand, int TimeOut, int fVerbose)
{
    int fVerboseSolver = 0;
    Vec_Int_t *vRes = NULL;
    abctime clkTotal = Abc_Clock();
    char FileNameIn[100], FileNameOut[100];
    sprintf(FileNameIn,  "%02d.cnf", Id);
    sprintf(FileNameOut, "%02d.txt", Id);
#ifdef _WIN32
    char *pKissat = "kissat.exe";
#else
    char *pKissat = "kissat";
#endif
    char Command[1000], *pCommand = (char *)&Command;
    if (TimeOut)
        sprintf(pCommand, "%s --seed=%d --time=%d %s %s > %s", pKissat, Rand, TimeOut, fVerboseSolver ? "" : "-q", FileNameIn, FileNameOut);
    else
        sprintf(pCommand, "%s --seed=%d %s %s > %s", pKissat, Rand, fVerboseSolver ? "" : "-q", FileNameIn, FileNameOut);
    //printf( "Thread command: %s\n", pCommand);
    FILE * pFile = fopen(FileNameIn, "rb");
    if ( pFile != NULL ) {
        fclose( pFile );
#if defined(__wasm)
        if ( 1 ) {
#else
        if (system(pCommand) == -1) {
#endif
            fprintf(stdout, "Command \"%s\" did not succeed.\n", pCommand);
            return 0;
        }
        vRes = Exa4_ManParse(FileNameOut); // FileNameOut is removed here
    }
    if (fVerbose) {
        double SolvingTime = ((double)(Abc_Clock() - clkTotal))/((double)CLOCKS_PER_SEC);
        if (vRes)
            printf("Problem %2d has a solution. ", Id);
        else if (vRes == NULL && (TimeOut == 0 || SolvingTime < (double)TimeOut))
            printf("Problem %2d has no solution. ", Id);
        else if (vRes == NULL)
            printf("Problem %2d has no solution or timed out after %d sec. ", Id, TimeOut);
        Abc_PrintTime(1, "Solving time", Abc_Clock() - clkTotal );
    }
    else if (vRes) {   
        printf("Problem %2d has a solution. ", Id);
        Abc_PrintTime(1, "Solving time", Abc_Clock() - clkTotal );
        printf("(Currently waiting for %d sec for other threads to finish.)\n", TimeOut);
    }
    return vRes;
}
Vec_Int_t * Cnf_RunSolverArray(int nProcs, int TimeOut, int fVerbose)
{
    Vec_Int_t *vRes = NULL; int i;
    for ( i = 0; i < nProcs; i++ )
        if ((vRes = Cnf_RunSolverOnce(i, 0, TimeOut, fVerbose))) 
            break;
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Solving problems using many cores.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
#ifndef ABC_USE_PTHREADS

Vec_Int_t *Cnf_RunSolver(int nProcs, int TimeOut, int fVerbose)
{
    return Cnf_RunSolverArray(nProcs, TimeOut, fVerbose);
}

#else // pthreads are used

#define PAR_THR_MAX 100
typedef struct Cnf_ThData_t_
{
    Vec_Int_t *vRes;
    int Index;
    int Rand;
    int nTimeOut;
    int fWorking;
    int fVerbose;
} Cnf_ThData_t;

void *Cnf_WorkerThread(void *pArg)
{
    Cnf_ThData_t *pThData = (Cnf_ThData_t *)pArg;
    volatile int *pPlace = &pThData->fWorking;
    while (1)
    {
        while (*pPlace == 0)
            ;
        assert(pThData->fWorking);
        if (pThData->Index == -1)
        {
            pthread_exit(NULL);
            assert(0);
            return NULL;
        }
        pThData->vRes = Cnf_RunSolverOnce(pThData->Index, pThData->Rand, pThData->nTimeOut, pThData->fVerbose);
        pThData->fWorking = 0;
    }
    assert(0);
    return NULL;
}

Vec_Int_t *Cnf_RunSolver(int nProcs, int TimeOut, int fVerbose)
{
    Vec_Int_t *vRes = NULL;
    Cnf_ThData_t ThData[PAR_THR_MAX];
    pthread_t WorkerThread[PAR_THR_MAX];
    int i, k, status;
    if (fVerbose)
        printf("Running concurrent solving with %d processes.\n", nProcs);
    fflush(stdout);
    if (nProcs < 2)
        return Cnf_RunSolverArray(nProcs, TimeOut, fVerbose);
    // subtract manager thread
    // nProcs--;
    assert(nProcs >= 1 && nProcs <= PAR_THR_MAX);
    // start threads
    for (i = 0; i < nProcs; i++)
    {
        ThData[i].vRes     = NULL;
        ThData[i].Index    = -1;
        ThData[i].Rand     = Abc_Random(0) % 0x1000000;
        ThData[i].nTimeOut = TimeOut;
        ThData[i].fWorking = 0;
        ThData[i].fVerbose = fVerbose;
        status = pthread_create(WorkerThread + i, NULL, Cnf_WorkerThread, (void *)(ThData + i));
        assert(status == 0);
    }
    // look at the threads
    for (k = 0; k < nProcs;)
    {
        for (i = 0; i < nProcs; i++)
        {
            if (ThData[i].fWorking)
                continue;
            if (ThData[i].vRes)
            {
                k = nProcs;
                break;
            }
            ThData[i].Index = k++;
            ThData[i].fWorking = 1;
            break;
        }
    }
    // wait till threads finish
    for (i = 0; i < nProcs; i++)
        if (ThData[i].fWorking)
            i = -1;
    // stop threads
    for (i = 0; i < nProcs; i++)
    {
        assert(!ThData[i].fWorking);
        if (ThData[i].vRes && vRes == NULL)
        {
            vRes = ThData[i].vRes;
            ThData[i].vRes = NULL;
        }
        Vec_IntFreeP(&ThData[i].vRes);
        // stop
        ThData[i].Index = -1;
        ThData[i].fWorking = 1;
    }
    return vRes;
}

#endif // pthreads are used



/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManScanMapping_rec(Cnf_Man_t *p, Aig_Obj_t *pObj, Vec_Ptr_t *vMapped)
{
    Aig_Obj_t *pLeaf;
    Dar_Cut_t *pCutBest;
    int aArea, i;
    if (pObj->nRefs++ || Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj))
        return 0;
    assert(Aig_ObjIsAnd(pObj));
    // collect the node first to derive pre-order
    if (vMapped)
        Vec_PtrPush(vMapped, pObj);
    // visit the transitive fanin of the selected cut
    if (pObj->fMarkB)
    {
        Vec_Ptr_t *vSuper = Vec_PtrAlloc(100);
        Aig_ObjCollectSuper(pObj, vSuper);
        aArea = Vec_PtrSize(vSuper) + 1;
        Vec_PtrForEachEntry(Aig_Obj_t *, vSuper, pLeaf, i)
            aArea += Aig_ManScanMapping_rec(p, Aig_Regular(pLeaf), vMapped);
        Vec_PtrFree(vSuper);
        ////////////////////////////
        pObj->fMarkB = 1;
    }
    else
    {
        pCutBest = Dar_ObjBestCut(pObj);
        aArea = Cnf_CutSopCost(p, pCutBest);
        Dar_CutForEachLeaf(p->pManAig, pCutBest, pLeaf, i)
            aArea += Aig_ManScanMapping_rec(p, pLeaf, vMapped);
    }
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description [Collects the nodes in reverse topological order.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t *Aig_ManScanMapping(Cnf_Man_t *p, int fCollect)
{
    Vec_Ptr_t *vMapped = NULL;
    Aig_Obj_t *pObj;
    int i;
    // clean all references
    Aig_ManForEachObj(p->pManAig, pObj, i)
        pObj->nRefs = 0;
    // allocate the array
    if (fCollect)
        vMapped = Vec_PtrAlloc(1000);
    // collect nodes reachable from POs in the DFS order through the best cuts
    p->aArea = 0;
    Aig_ManForEachCo(p->pManAig, pObj, i)
        p->aArea += Aig_ManScanMapping_rec(p, Aig_ObjFanin0(pObj), vMapped);
    //    printf( "Variables = %6d. Clauses = %8d.\n", vMapped? Vec_PtrSize(vMapped) + Aig_ManCiNum(p->pManAig) + 1 : 0, p->aArea + 2 );
    return vMapped;
}

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_ManScanMapping_rec(Cnf_Man_t *p, Aig_Obj_t *pObj, Vec_Ptr_t *vMapped, int fPreorder)
{
    Aig_Obj_t *pLeaf;
    Cnf_Cut_t *pCutBest;
    int aArea, i;
    if (pObj->nRefs++ || Aig_ObjIsCi(pObj) || Aig_ObjIsConst1(pObj))
        return 0;
    assert(Aig_ObjIsAnd(pObj));
    assert(pObj->pData != NULL);
    // add the node to the mapping
    if (vMapped && fPreorder)
        Vec_PtrPush(vMapped, pObj);
    // visit the transitive fanin of the selected cut
    if (pObj->fMarkB)
    {
        Vec_Ptr_t *vSuper = Vec_PtrAlloc(100);
        Aig_ObjCollectSuper(pObj, vSuper);
        aArea = Vec_PtrSize(vSuper) + 1;
        Vec_PtrForEachEntry(Aig_Obj_t *, vSuper, pLeaf, i)
            aArea += Cnf_ManScanMapping_rec(p, Aig_Regular(pLeaf), vMapped, fPreorder);
        Vec_PtrFree(vSuper);
        ////////////////////////////
        pObj->fMarkB = 1;
    }
    else
    {
        pCutBest = (Cnf_Cut_t *)pObj->pData;
        //        assert( pCutBest->nFanins > 0 );
        assert(pCutBest->Cost < 127);
        aArea = pCutBest->Cost;
        Cnf_CutForEachLeaf(p->pManAig, pCutBest, pLeaf, i)
            aArea += Cnf_ManScanMapping_rec(p, pLeaf, vMapped, fPreorder);
    }
    // add the node to the mapping
    if (vMapped && !fPreorder)
        Vec_PtrPush(vMapped, pObj);
    return aArea;
}

/**Function*************************************************************

  Synopsis    [Computes area, references, and nodes used in the mapping.]

  Description [Collects the nodes in reverse topological order.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t *Cnf_ManScanMapping(Cnf_Man_t *p, int fCollect, int fPreorder)
{
    Vec_Ptr_t *vMapped = NULL;
    Aig_Obj_t *pObj;
    int i;
    // clean all references
    Aig_ManForEachObj(p->pManAig, pObj, i)
        pObj->nRefs = 0;
    // allocate the array
    if (fCollect)
        vMapped = Vec_PtrAlloc(1000);
    // collect nodes reachable from POs in the DFS order through the best cuts
    p->aArea = 0;
    Aig_ManForEachCo(p->pManAig, pObj, i)
        p->aArea += Cnf_ManScanMapping_rec(p, Aig_ObjFanin0(pObj), vMapped, fPreorder);
    //    printf( "Variables = %6d. Clauses = %8d.\n", vMapped? Vec_PtrSize(vMapped) + Aig_ManCiNum(p->pManAig) + 1 : 0, p->aArea + 2 );
    return vMapped;
}

/**Function*************************************************************

  Synopsis    [Returns the array of CI IDs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t *Cnf_DataCollectCiSatNums(Cnf_Dat_t *pCnf, Aig_Man_t *p)
{
    Vec_Int_t *vCiIds;
    Aig_Obj_t *pObj;
    int i;
    vCiIds = Vec_IntAlloc(Aig_ManCiNum(p));
    Aig_ManForEachCi(p, pObj, i)
        Vec_IntPush(vCiIds, pCnf->pVarNums[pObj->Id]);
    return vCiIds;
}

/**Function*************************************************************

  Synopsis    [Returns the array of CI IDs.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t *Cnf_DataCollectCoSatNums(Cnf_Dat_t *pCnf, Aig_Man_t *p)
{
    Vec_Int_t *vCoIds;
    Aig_Obj_t *pObj;
    int i;
    vCoIds = Vec_IntAlloc(Aig_ManCoNum(p));
    Aig_ManForEachCo(p, pObj, i)
        Vec_IntPush(vCoIds, pCnf->pVarNums[pObj->Id]);
    return vCoIds;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned char *Cnf_DataDeriveLitPolarities(Cnf_Dat_t *p)
{
    int i, c, iClaBeg, iClaEnd, *pLit;
    unsigned *pPols0 = ABC_CALLOC(unsigned, Aig_ManObjNumMax(p->pMan));
    unsigned *pPols1 = ABC_CALLOC(unsigned, Aig_ManObjNumMax(p->pMan));
    unsigned char *pPres = ABC_CALLOC(unsigned char, p->nClauses);
    for (i = 0; i < Aig_ManObjNumMax(p->pMan); i++)
    {
        if (p->pObj2Count[i] == 0)
            continue;
        iClaBeg = p->pObj2Clause[i];
        iClaEnd = p->pObj2Clause[i] + p->pObj2Count[i];
        // go through the negative polarity clauses
        for (c = iClaBeg; c < iClaEnd; c++)
            for (pLit = p->pClauses[c] + 1; pLit < p->pClauses[c + 1]; pLit++)
                if (Abc_LitIsCompl(p->pClauses[c][0]))
                    pPols0[Abc_Lit2Var(*pLit)] |= (unsigned)(2 - Abc_LitIsCompl(*pLit)); // taking the opposite (!) -- not the case
                else
                    pPols1[Abc_Lit2Var(*pLit)] |= (unsigned)(2 - Abc_LitIsCompl(*pLit)); // taking the opposite (!) -- not the case
        // record these clauses
        for (c = iClaBeg; c < iClaEnd; c++)
            for (pLit = p->pClauses[c] + 1; pLit < p->pClauses[c + 1]; pLit++)
                if (Abc_LitIsCompl(p->pClauses[c][0]))
                    pPres[c] = (unsigned char)((unsigned)pPres[c] | (pPols0[Abc_Lit2Var(*pLit)] << (2 * (pLit - p->pClauses[c] - 1))));
                else
                    pPres[c] = (unsigned char)((unsigned)pPres[c] | (pPols1[Abc_Lit2Var(*pLit)] << (2 * (pLit - p->pClauses[c] - 1))));
        // clean negative polarity
        for (c = iClaBeg; c < iClaEnd; c++)
            for (pLit = p->pClauses[c] + 1; pLit < p->pClauses[c + 1]; pLit++)
                pPols0[Abc_Lit2Var(*pLit)] = pPols1[Abc_Lit2Var(*pLit)] = 0;
    }
    ABC_FREE(pPols0);
    ABC_FREE(pPols1);
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
Cnf_Dat_t *Cnf_DataReadFromFile(char *pFileName)
{
    int MaxLine = 1000000;
    int Var, Lit, nVars = -1, nClas = -1, i, Entry, iLine = 0;
    Cnf_Dat_t *pCnf = NULL;
    Vec_Int_t *vClas = NULL;
    Vec_Int_t *vLits = NULL;
    char *pBuffer, *pToken;
    FILE *pFile = fopen(pFileName, "rb");
    if (pFile == NULL)
    {
        printf("Cannot open file \"%s\" for writing.\n", pFileName);
        return NULL;
    }
    pBuffer = ABC_ALLOC(char, MaxLine);
    while (fgets(pBuffer, MaxLine, pFile) != NULL)
    {
        iLine++;
        if (pBuffer[0] == 'c')
            continue;
        if (pBuffer[0] == 'p')
        {
            pToken = strtok(pBuffer + 1, " \t");
            if (strcmp(pToken, "cnf"))
            {
                printf("Incorrect input file.\n");
                goto finish;
            }
            pToken = strtok(NULL, " \t");
            nVars = atoi(pToken);
            pToken = strtok(NULL, " \t");
            nClas = atoi(pToken);
            if (nVars <= 0 || nClas <= 0)
            {
                printf("Incorrect parameters.\n");
                goto finish;
            }
            // temp storage
            vClas = Vec_IntAlloc(nClas + 1);
            vLits = Vec_IntAlloc(nClas * 8);
            continue;
        }
        pToken = strtok(pBuffer, " \t\r\n");
        if (pToken == NULL)
            continue;
        Vec_IntPush(vClas, Vec_IntSize(vLits));
        while (pToken)
        {
            Var = atoi(pToken);
            if (Var == 0)
                break;
            Lit = (Var > 0) ? Abc_Var2Lit(Var - 1, 0) : Abc_Var2Lit(-Var - 1, 1);
            if (Lit >= 2 * nVars)
            {
                printf("Literal %d is out-of-bound for %d variables.\n", Lit, nVars);
                goto finish;
            }
            Vec_IntPush(vLits, Lit);
            pToken = strtok(NULL, " \t\r\n");
        }
        if (Var != 0)
        {
            printf("There is no zero-terminator in line %d.\n", iLine);
            goto finish;
        }
    }
    // finalize
    if (Vec_IntSize(vClas) != nClas)
        printf("Warning! The number of clauses (%d) is different from declaration (%d).\n", Vec_IntSize(vClas), nClas);
    Vec_IntPush(vClas, Vec_IntSize(vLits));
    // create
    pCnf = ABC_CALLOC(Cnf_Dat_t, 1);
    pCnf->nVars = nVars;
    pCnf->nClauses = Vec_IntSize(vClas) - 1;
    pCnf->nLiterals = Vec_IntSize(vLits);
    pCnf->pClauses = ABC_ALLOC(int *, Vec_IntSize(vClas));
    pCnf->pClauses[0] = Vec_IntReleaseArray(vLits);
    Vec_IntForEachEntry(vClas, Entry, i)
        pCnf->pClauses[i] = pCnf->pClauses[0] + Entry;
finish:
    fclose(pFile);
    Vec_IntFreeP(&vClas);
    Vec_IntFreeP(&vLits);
    ABC_FREE(pBuffer);
    return pCnf;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_DataSolveFromFile(char *pFileName, int nConfLimit, int nLearnedStart, int nLearnedDelta, int nLearnedPerce, int fVerbose, int fShowPattern, int **ppModel, int nPis)
{
    abctime clk = Abc_Clock();
    Cnf_Dat_t *pCnf = Cnf_DataReadFromFile(pFileName);
    sat_solver *pSat;
    int i, status, RetValue = -1;
    if (pCnf == NULL)
        return -1;
    if (fVerbose)
    {
        printf("CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals);
        Abc_PrintTime(1, "Time", Abc_Clock() - clk);
    }
    // convert into SAT solver
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver(pCnf, 1, 0);
    if (pSat == NULL)
    {
        printf("The problem is trivially UNSAT.\n");
        Cnf_DataFree(pCnf);
        return 1;
    }
    if (nLearnedStart)
        pSat->nLearntStart = pSat->nLearntMax = nLearnedStart;
    if (nLearnedDelta)
        pSat->nLearntDelta = nLearnedDelta;
    if (nLearnedPerce)
        pSat->nLearntRatio = nLearnedPerce;
    if (fVerbose)
        pSat->fVerbose = fVerbose;

    // sat_solver_start_cardinality( pSat, 100 );

    // solve the miter
    status = sat_solver_solve(pSat, NULL, NULL, (ABC_INT64_T)nConfLimit, 0, (ABC_INT64_T)0, (ABC_INT64_T)0);
    if (status == l_Undef)
        RetValue = -1;
    else if (status == l_True)
        RetValue = 0;
    else if (status == l_False)
        RetValue = 1;
    else
        assert(0);
    if (fVerbose)
        Sat_SolverPrintStats(stdout, pSat);
    if (RetValue == -1)
        Abc_Print(1, "UNDECIDED      ");
    else if (RetValue == 0)
        Abc_Print(1, "SATISFIABLE    ");
    else
        Abc_Print(1, "UNSATISFIABLE  ");
    // Abc_Print( -1, "\n" );
    Abc_PrintTime(1, "Time", Abc_Clock() - clk);
    // derive SAT assignment
    if (RetValue == 0 && nPis > 0)
    {
        *ppModel = ABC_ALLOC(int, nPis);
        for (i = 0; i < nPis; i++)
            (*ppModel)[i] = sat_solver_var_value(pSat, pCnf->nVars - nPis + i);
    }
    if (RetValue == 0 && fShowPattern)
    {
        for (i = 0; i < pCnf->nVars; i++)
            printf("%d", sat_solver_var_value(pSat, i));
        printf("\n");
    }
    Cnf_DataFree(pCnf);
    sat_solver_delete(pSat);
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_DataBestVar(Cnf_Dat_t *p, int *pSkip)
{
    int *pNums = ABC_CALLOC(int, p->nVars);
    int i, *pLit, NumMax = -1, iVarMax = -1;
    for (i = 0; i < p->nClauses; i++)
        for (pLit = p->pClauses[i]; pLit < p->pClauses[i + 1]; pLit++)
            pNums[Abc_Lit2Var(*pLit)]++;
    for (i = 0; i < p->nVars; i++)
        if ((!pSkip || !pSkip[i]) && NumMax < pNums[i])
            NumMax = pNums[i], iVarMax = i;
    ABC_FREE(pNums);
    return iVarMax;
}
void Cnf_Experiment1()
{
    Cnf_Dat_t *pTemp, *p = Cnf_DataReadFromFile("../166b.cnf");
    int i, *pSkip = ABC_CALLOC(int, p->nVars);
    for (i = 0; i < 100; i++)
    {
        int iVar = Cnf_DataBestVar(p, pSkip);
        char FileName[100];
        sprintf(FileName, "cnf/%03d.cnf", i);
        Cnf_DataWriteIntoFile(p, FileName, 0, NULL, NULL);
        printf("Dumped file \"%s\".\n", FileName);
        p = Cnf_DataDupCof(pTemp = p, Abc_Var2Lit(iVar, 0));
        Cnf_DataFree(pTemp);
        pSkip[iVar] = 1;
    }
    Cnf_DataFree(p);
    ABC_FREE(pSkip);
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t *Cnf_GenRandLits(int iVarBeg, int iVarEnd, int nLits, int Value, int Rand, int fVerbose)
{
    Vec_Int_t *vLits = Vec_IntAlloc(nLits);
    assert(iVarBeg < iVarEnd && nLits < iVarEnd - iVarBeg);
    while (Vec_IntSize(vLits) < nLits)
    {
        int iVar = iVarBeg + (Abc_Random(0) ^ Rand) % (iVarEnd - iVarBeg);
        assert(iVar >= iVarBeg && iVar < iVarEnd);
        if (Vec_IntFind(vLits, Abc_Var2Lit(iVar, 0)) == -1 && Vec_IntFind(vLits, Abc_Var2Lit(iVar, 1)) == -1)
        {
            if (Value == 0)
                Vec_IntPush(vLits, Abc_Var2Lit(iVar, 1));
            else if (Value == 1)
                Vec_IntPush(vLits, Abc_Var2Lit(iVar, 0));
            else
                Vec_IntPush(vLits, Abc_Var2Lit(iVar, Abc_Random(0) & 1));
        }
    }
    Vec_IntSort(vLits, 0);
    if ( fVerbose )
    Vec_IntPrint(vLits);
    fflush(stdout);
    return vLits;
}
void Cnf_SplitCnfFile(char * pFileName, int nParts, int iVarBeg, int iVarEnd, int nLits, int Value, int Rand, int fPrepro, int fVerbose)
{
    Cnf_Dat_t *p = Cnf_DataReadFromFile(pFileName); int k;
    if (iVarEnd == ABC_INFINITY)
        iVarEnd = p->nVars;
    for (k = 0; k < nParts; k++)
    {
        Vec_Int_t *vLits = Cnf_GenRandLits(iVarBeg, iVarEnd, nLits, Value, Rand, fVerbose);
        Cnf_Dat_t *pCnf = Cnf_DataDupCofArray(p, vLits);
        char FileName[100];  sprintf(FileName, "%02d.cnf", k);
        if ( fPrepro ) {
            char Command[1000];   
            sprintf(Command, "satelite --verbosity=0 -pre temp.cnf %s", FileName);
            Cnf_DataWriteIntoFile(pCnf, "temp.cnf", 0, NULL, NULL);
#if defined(__wasm)
            if ( 1 ) {
#else
            if (system(Command) == -1) {
#endif
                fprintf(stdout, "Command \"%s\" did not succeed. Preprocessing skipped.\n", Command);
                Cnf_DataWriteIntoFile(pCnf, FileName, 0, NULL, NULL);
            }
            unlink("temp.cnf");
        }
        else 
            Cnf_DataWriteIntoFile(pCnf, FileName, 0, NULL, NULL);
        Cnf_DataFree(pCnf);
        Vec_IntFree(vLits);
    }    
    Cnf_DataFree(p);
}
void Cnf_SplitCnfCleanup(int nParts)
{
    char FileName[100];  int k;
    for (k = 0; k < nParts; k++) {
        sprintf(FileName, "%02d.cnf", k);
        unlink(FileName);
    }   
}
void Cnf_SplitSat(char *pFileName, int iVarBeg, int iVarEnd, int nLits, int Value, int TimeOut, int nProcs, int nIters, int Seed, int fPrepro, int fVerbose)
{
    abctime clk = Abc_Clock();
    Vec_Int_t *vSol = NULL;
    int i, Rand = 0;
    if ( nIters == 0 )
        nIters = ABC_INFINITY;
    Abc_Random(1);
    for ( i = 0; i < Seed; i++ )
        Abc_Random(0);
    Rand = Abc_Random(0);
    for (i = 0; i < nIters && !vSol; i++)
    {
        abctime clk2 = Abc_Clock();
        Cnf_SplitCnfFile(pFileName, nProcs, iVarBeg, iVarEnd, nLits, Value, Rand, fPrepro, fVerbose);
        vSol = Cnf_RunSolver(nProcs, TimeOut, fVerbose);
        Cnf_SplitCnfCleanup(nProcs);
        if (fVerbose) {
            printf( "Finished iteration %d.  ", i);    
            Abc_PrintTime(0, "Time", Abc_Clock() - clk2);
        }
    }
    printf("%solution is found.  ", vSol ? "S" : "No s");
    Abc_PrintTime(0, "Total time", Abc_Clock() - clk);
    Vec_IntFreeP(&vSol);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END
