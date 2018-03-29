/**CFile****************************************************************

  FileName    [disjunctiveMonotone.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Liveness property checking.]

  Synopsis    [Constraint analysis module for the k-Liveness algorithm
        invented by Koen Classen, Niklas Sorensson.]

  Author      [Sayak Ray]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 31, 2012.]

***********************************************************************/

#include <stdio.h>
#include "base/main/main.h"
#include "aig/aig/aig.h"
#include "aig/saig/saig.h"
#include <string.h>
#include "base/main/mainInt.h"
#include "proof/pdr/pdr.h"
#include <time.h>

ABC_NAMESPACE_IMPL_START

struct aigPoIndices
{
    int attrPendingSignalIndex;
    int attrHintSingalBeginningMarker;
    int attrHintSingalEndMarker;
    int attrSafetyInvarIndex;
};

extern struct aigPoIndices *allocAigPoIndices();
extern void deallocAigPoIndices(struct aigPoIndices *toBeDeletedAigPoIndices);
extern int collectSafetyInvariantPOIndex(Abc_Ntk_t *pNtk);

struct antecedentConsequentVectorsStruct
{
    Vec_Int_t *attrAntecedents;
    Vec_Int_t *attrConsequentCandidates;
};

struct antecedentConsequentVectorsStruct *allocAntecedentConsequentVectorsStruct()
{
    struct antecedentConsequentVectorsStruct *newStructure;

    newStructure = (struct antecedentConsequentVectorsStruct *)malloc(sizeof (struct antecedentConsequentVectorsStruct));

    newStructure->attrAntecedents = NULL;
    newStructure->attrConsequentCandidates = NULL;
        
    assert( newStructure != NULL );
    return newStructure;
}

void deallocAntecedentConsequentVectorsStruct(struct antecedentConsequentVectorsStruct *toBeDeleted)
{
    assert( toBeDeleted != NULL );
    if(toBeDeleted->attrAntecedents) 
        Vec_IntFree( toBeDeleted->attrAntecedents );
    if(toBeDeleted->attrConsequentCandidates)
        Vec_IntFree( toBeDeleted->attrConsequentCandidates );
    free( toBeDeleted );
}

Aig_Man_t *createDisjunctiveMonotoneTester(Aig_Man_t *pAig, struct aigPoIndices *aigPoIndicesArg, 
                    struct antecedentConsequentVectorsStruct *anteConseVectors, int *startMonotonePropPo)
{
    Aig_Man_t *pNewAig;
    int iElem, i, nRegCount;
    int piCopied = 0, liCopied = 0, liCreated = 0, loCopied = 0, loCreated = 0;
    int poCopied = 0, poCreated = 0;
    Aig_Obj_t *pObj, *pObjPo, *pObjDriver, *pObjDriverNew, *pObjPendingDriverNew, *pObjPendingAndNextPending;
    Aig_Obj_t *pPendingFlop, *pObjConseCandFlop, *pObjSafetyInvariantPoDriver;
    //Vec_Ptr_t *vHintMonotoneLocalDriverNew;
    Vec_Ptr_t *vConseCandFlopOutput;
    //Vec_Ptr_t *vHintMonotoneLocalProp;

    Aig_Obj_t *pObjAnteDisjunction, *pObjConsecDriver, *pObjConsecDriverNew, *pObjCandMonotone, *pObjPrevCandMonotone, *pObjMonotonePropDriver;
    Vec_Ptr_t *vCandMonotoneProp;
    Vec_Ptr_t *vCandMonotoneFlopInput;

    int pendingSignalIndexLocal = aigPoIndicesArg->attrPendingSignalIndex;

    Vec_Int_t *vAntecedentsLocal = anteConseVectors->attrAntecedents;
    Vec_Int_t *vConsequentCandidatesLocal = anteConseVectors->attrConsequentCandidates;

    if( vConsequentCandidatesLocal == NULL )
        return NULL; //no candidates for consequent is provided, hence no need to generate a new AIG

    
    //****************************************************************
    // Step1: create the new manager
    // Note: The new manager is created with "2 * Aig_ManObjNumMax(p)"
    // nodes, but this selection is arbitrary - need to be justified
    //****************************************************************
    pNewAig = Aig_ManStart( Aig_ManObjNumMax(pAig) );
    pNewAig->pName = (char *)malloc( strlen( pAig->pName ) + strlen("_monotone") + 2 );
    sprintf(pNewAig->pName, "%s_%s", pAig->pName, "monotone");
        pNewAig->pSpec = NULL;

    //****************************************************************
    // Step 2: map constant nodes
    //****************************************************************
        pObj = Aig_ManConst1( pAig );
        pObj->pData = Aig_ManConst1( pNewAig );

    //****************************************************************
        // Step 3: create true PIs
    //****************************************************************
        Saig_ManForEachPi( pAig, pObj, i )
    {
        piCopied++;
        pObj->pData = Aig_ObjCreateCi(pNewAig);
    }

    //****************************************************************
    // Step 5: create register outputs
    //****************************************************************
        Saig_ManForEachLo( pAig, pObj, i )
        {
        loCopied++;
        pObj->pData = Aig_ObjCreateCi(pNewAig);
        }

    //****************************************************************
    // Step 6: create "D" register output for PENDING flop 
    //****************************************************************
    loCreated++;
    pPendingFlop = Aig_ObjCreateCi( pNewAig );

    //****************************************************************
    // Step 6.a: create "D" register output for HINT_MONOTONE flop 
    //****************************************************************
    vConseCandFlopOutput = Vec_PtrAlloc(Vec_IntSize(vConsequentCandidatesLocal));
    Vec_IntForEachEntry( vConsequentCandidatesLocal, iElem, i )
    {
        loCreated++;
        pObjConseCandFlop = Aig_ObjCreateCi( pNewAig );
        Vec_PtrPush( vConseCandFlopOutput, pObjConseCandFlop );
    }

    nRegCount = loCreated + loCopied;

    //********************************************************************
    // Step 7: create internal nodes
    //********************************************************************
        Aig_ManForEachNode( pAig, pObj, i )
    {
        pObj->pData = Aig_And( pNewAig, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    }
    
    //********************************************************************
    // Step 8: mapping appropriate new flop drivers 
    //********************************************************************

    if( aigPoIndicesArg->attrSafetyInvarIndex != -1 )
    {
        pObjPo = Aig_ManCo( pAig, aigPoIndicesArg->attrSafetyInvarIndex );
        pObjDriver = Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObjPo), Aig_ObjFaninC0(pObjPo));
        pObjDriverNew = !Aig_IsComplement(pObjDriver)? 
                (Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData) : 
                Aig_Not((Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData));
        pObjSafetyInvariantPoDriver = pObjDriverNew;
    }
    else
        pObjSafetyInvariantPoDriver = Aig_ManConst1(pNewAig);

    pObjPo = Aig_ManCo( pAig, pendingSignalIndexLocal );
    pObjDriver = Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObjPo), Aig_ObjFaninC0(pObjPo));
    pObjPendingDriverNew = !Aig_IsComplement(pObjDriver)? 
                (Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData) : 
                Aig_Not((Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData));

    pObjPendingAndNextPending = Aig_And( pNewAig, pObjPendingDriverNew, pPendingFlop );

    pObjAnteDisjunction = Aig_Not(Aig_ManConst1( pNewAig ));
    if( vAntecedentsLocal )
    {
        Vec_IntForEachEntry( vAntecedentsLocal, iElem, i )
        {
            pObjPo = Aig_ManCo( pAig, iElem );
            pObjDriver = Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObjPo), Aig_ObjFaninC0(pObjPo));
            pObjDriverNew = !Aig_IsComplement(pObjDriver)? 
                    (Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData) : 
                    Aig_Not((Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData));

            pObjAnteDisjunction = Aig_Or( pNewAig, pObjDriverNew, pObjAnteDisjunction );
        }
    }

    vCandMonotoneProp = Vec_PtrAlloc( Vec_IntSize(vConsequentCandidatesLocal) );
    vCandMonotoneFlopInput = Vec_PtrAlloc( Vec_IntSize(vConsequentCandidatesLocal) );
    Vec_IntForEachEntry( vConsequentCandidatesLocal, iElem, i )
    {
        pObjPo = Aig_ManCo( pAig, iElem );
        pObjConsecDriver = Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObjPo), Aig_ObjFaninC0(pObjPo));
        pObjConsecDriverNew = !Aig_IsComplement(pObjConsecDriver)? 
                    (Aig_Obj_t *)(Aig_Regular(pObjConsecDriver)->pData) : 
                    Aig_Not((Aig_Obj_t *)(Aig_Regular(pObjConsecDriver)->pData));

        pObjCandMonotone = Aig_Or( pNewAig, pObjConsecDriverNew, pObjAnteDisjunction );
        pObjPrevCandMonotone = (Aig_Obj_t *)(Vec_PtrEntry( vConseCandFlopOutput, i ));
        pObjMonotonePropDriver = Aig_Or( pNewAig, Aig_Not( Aig_And( pNewAig, pObjPendingAndNextPending, pObjPrevCandMonotone ) ), 
                        pObjCandMonotone );

        //Conjunting safety invar
        pObjMonotonePropDriver = Aig_And( pNewAig, pObjMonotonePropDriver, pObjSafetyInvariantPoDriver );
                        
        Vec_PtrPush( vCandMonotoneFlopInput, pObjCandMonotone );
        Vec_PtrPush( vCandMonotoneProp, pObjMonotonePropDriver );
    }

    //********************************************************************
    // Step 9: create primary outputs 
    //********************************************************************
    Saig_ManForEachPo( pAig, pObj, i )
    {
        poCopied++;
        pObj->pData = Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) ); 
    }

    *startMonotonePropPo = i;
    Vec_PtrForEachEntry( Aig_Obj_t *, vCandMonotoneProp, pObj, i )
    {
        poCreated++;
        pObjPo = Aig_ObjCreateCo( pNewAig, pObj ); 
    }

    //********************************************************************
    // Step 9: create latch inputs 
    //********************************************************************

    Saig_ManForEachLi( pAig, pObj, i )
    {
        liCopied++;
        Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) );
    }
    
    //********************************************************************
    // Step 9.a: create latch input for PENDING_FLOP
    //********************************************************************

    liCreated++;
    Aig_ObjCreateCo( pNewAig, pObjPendingDriverNew );

    //********************************************************************
    // Step 9.b: create latch input for MONOTONE_FLOP
    //********************************************************************

    Vec_PtrForEachEntry( Aig_Obj_t *, vCandMonotoneFlopInput, pObj, i )
    {
        liCreated++;
        Aig_ObjCreateCo( pNewAig, pObj );
    }

    Aig_ManSetRegNum( pNewAig, nRegCount );
    Aig_ManCleanup( pNewAig );
    
    assert( Aig_ManCheck( pNewAig ) );
    assert( loCopied + loCreated == liCopied + liCreated );

    Vec_PtrFree(vConseCandFlopOutput);
    Vec_PtrFree(vCandMonotoneProp);
    Vec_PtrFree(vCandMonotoneFlopInput);
    return pNewAig;
}

Vec_Int_t *findNewDisjunctiveMonotone( Aig_Man_t *pAig, struct aigPoIndices *aigPoIndicesArg, struct antecedentConsequentVectorsStruct *anteConseVectors )
{
    Aig_Man_t *pAigNew;
    Aig_Obj_t *pObjTargetPo;
    int poMarker;
    //int i, RetValue, poSerialNum;
    int i, poSerialNum;
    Pdr_Par_t Pars, * pPars = &Pars;
    //Abc_Cex_t * pCex = NULL;
    Vec_Int_t *vMonotoneIndex;
    //char fileName[20];
    Abc_Cex_t * cexElem;

    int pendingSignalIndexLocal = aigPoIndicesArg->attrPendingSignalIndex;

    pAigNew = createDisjunctiveMonotoneTester(pAig, aigPoIndicesArg, anteConseVectors, &poMarker );

    //printf("enter an integer : ");
    //waitForInteger = getchar();
    //putchar(waitForInteger);

    vMonotoneIndex = Vec_IntAlloc(0);

    for( i=0; i<Saig_ManPoNum(pAigNew); i++ )
    {
        pObjTargetPo = Aig_ManCo( pAigNew, i );
        Aig_ObjChild0Flip( pObjTargetPo );
    }

    Pdr_ManSetDefaultParams( pPars );
    pPars->fVerbose = 0;
    pPars->fNotVerbose = 1;
    pPars->fSolveAll = 1;
    pAigNew->vSeqModelVec = NULL;
    Pdr_ManSolve( pAigNew, pPars );    

    if( pAigNew->vSeqModelVec )
    {
        Vec_PtrForEachEntry( Abc_Cex_t *, pAigNew->vSeqModelVec, cexElem, i )
        {
            if( cexElem == NULL  && i >= pendingSignalIndexLocal + 1)
            {
                poSerialNum = i - (pendingSignalIndexLocal + 1);
                Vec_IntPush( vMonotoneIndex, Vec_IntEntry( anteConseVectors->attrConsequentCandidates, poSerialNum ));
            }
        }
    }
    for( i=0; i<Saig_ManPoNum(pAigNew); i++ )
    {
        pObjTargetPo = Aig_ManCo( pAigNew, i );
        Aig_ObjChild0Flip( pObjTargetPo );
    }
    
    //if(pAigNew->vSeqModelVec)
    //    Vec_PtrFree(pAigNew->vSeqModelVec);

    Aig_ManStop(pAigNew);
    
    if( Vec_IntSize( vMonotoneIndex ) > 0 )
    {
        return vMonotoneIndex;
    }
    else
    {
        Vec_IntFree(vMonotoneIndex);
        return NULL;
    }
}

Vec_Int_t *updateAnteConseVectors(struct antecedentConsequentVectorsStruct *anteConse )
{
    Vec_Int_t *vCandMonotone;
    int iElem, i;

    //if( vKnownMonotone == NULL || Vec_IntSize(vKnownMonotone) <= 0 )
    //    return vHintMonotone;
    if( anteConse->attrAntecedents == NULL  || Vec_IntSize(anteConse->attrAntecedents) <= 0 )
        return anteConse->attrConsequentCandidates;
    vCandMonotone = Vec_IntAlloc(0);
    Vec_IntForEachEntry( anteConse->attrConsequentCandidates, iElem, i )
    {
        if( Vec_IntFind( anteConse->attrAntecedents, iElem ) == -1 )
            Vec_IntPush( vCandMonotone, iElem );
    }
    
    return vCandMonotone;
}

Vec_Int_t *vectorDifference(Vec_Int_t *A, Vec_Int_t *B)
{
    Vec_Int_t *C;
    int iElem, i;

    C = Vec_IntAlloc(0);
    Vec_IntForEachEntry( A, iElem, i )
    {
        if( Vec_IntFind( B, iElem ) == -1 )
        {
            Vec_IntPush( C, iElem );
        }
    }

    return C;
}

Vec_Int_t *createSingletonIntVector( int iElem )
{
    Vec_Int_t *myVec = Vec_IntAlloc(1);
    
    Vec_IntPush(myVec, iElem);
    return myVec;
}

#if 0
Vec_Ptr_t *generateDisjuntiveMonotone_rec()
{
    nextLevelSignals = ;
    if level is not exhausted
    for all x \in nextLevelSignals
    {
        append x in currAntecendent
        recond it as a monotone predicate
        resurse with level - 1 
    }
}
#endif

#if 0
Vec_Ptr_t *generateDisjuntiveMonotoneLevels(Aig_Man_t *pAig, 
            struct aigPoIndices *aigPoIndicesInstance, 
            struct antecedentConsequentVectorsStruct *anteConsecInstanceOrig,
            int level )
{
    Vec_Int_t *firstLevelMonotone;
    Vec_Int_t *currVecInt;
    Vec_Ptr_t *hierarchyList;
    int iElem, i;

    assert( level >= 1 );
    firstLevelMonotone = findNewDisjunctiveMonotone( pAig, aigPoIndicesInstance, anteConsecInstance );
    if( firstLevelMonotone == NULL || Vec_IntSize(firstLevelMonotone) <= 0 )
        return NULL;
    
    hierarchyList = Vec_PtrAlloc(Vec_IntSize(firstLevelMonotone));
    
    Vec_IntForEachEntry( firstLevelMonotone, iElem, i )
    {
        currVecInt = createSingletonIntVector( iElem );
        Vec_PtrPush( hierarchyList, currVecInt );
    }

    if( level > 1 )
    {
        Vec_IntForEachEntry( firstLevelMonotone, iElem, i )
        {
            currVecInt = (Vec_Int_t *)Vec_PtrEntry( hierarchyList, i );
            
            
        }
    }

    return hierarchyList;
}
#endif

int Vec_IntPushUniqueLocal( Vec_Int_t * p, int Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( p->pArray[i] == Entry )
            return 1;
    Vec_IntPush( p, Entry );
    return 0;
}

Vec_Ptr_t *findNextLevelDisjunctiveMonotone( 
        Aig_Man_t *pAig, 
        struct aigPoIndices *aigPoIndicesInstance, 
        struct antecedentConsequentVectorsStruct *anteConsecInstance, 
        Vec_Ptr_t *previousMonotoneVectors )
{
    Vec_Ptr_t *newLevelPtrVec;
    Vec_Int_t *vElem, *vNewDisjunctVector, *newDisjunction;
    int i, j, iElem;
    struct antecedentConsequentVectorsStruct *anteConsecInstanceLocal;
    Vec_Int_t *vUnionPrevMonotoneVector, *vDiffVector;

    newLevelPtrVec = Vec_PtrAlloc(0);
    vUnionPrevMonotoneVector = Vec_IntAlloc(0);
    Vec_PtrForEachEntry(Vec_Int_t *, previousMonotoneVectors, vElem, i)
        Vec_IntForEachEntry( vElem, iElem, j )
            Vec_IntPushUniqueLocal( vUnionPrevMonotoneVector, iElem );

    Vec_PtrForEachEntry(Vec_Int_t *, previousMonotoneVectors, vElem, i)
    {
        anteConsecInstanceLocal = allocAntecedentConsequentVectorsStruct();

        anteConsecInstanceLocal->attrAntecedents = Vec_IntDup(vElem);    
        vDiffVector = vectorDifference( anteConsecInstance->attrConsequentCandidates, vUnionPrevMonotoneVector);
        anteConsecInstanceLocal->attrConsequentCandidates = vDiffVector;
        assert( vDiffVector );

        //printf("Calling target function %d\n", i);
        vNewDisjunctVector = findNewDisjunctiveMonotone( pAig, aigPoIndicesInstance, anteConsecInstanceLocal );

        if( vNewDisjunctVector )
        {
            Vec_IntForEachEntry(vNewDisjunctVector, iElem, j)
            {
                newDisjunction = Vec_IntDup(vElem);
                Vec_IntPush( newDisjunction, iElem );
                Vec_PtrPush( newLevelPtrVec, newDisjunction );
            }
            Vec_IntFree(vNewDisjunctVector);
        }
        deallocAntecedentConsequentVectorsStruct( anteConsecInstanceLocal );
    }

    Vec_IntFree(vUnionPrevMonotoneVector);

    return newLevelPtrVec;
}

void printAllIntVectors(Vec_Ptr_t *vDisjunctions, Abc_Ntk_t *pNtk, char *fileName)
{
    Vec_Int_t *vElem;
    int i, j, iElem;
    char *name, *hintSubStr;
    FILE *fp;

    fp = fopen( fileName, "a" );

    Vec_PtrForEachEntry(Vec_Int_t *, vDisjunctions, vElem, i)
    {    
        fprintf(fp, "( ");
        Vec_IntForEachEntry( vElem, iElem, j )
        {
            name = Abc_ObjName( Abc_NtkPo(pNtk, iElem));
            hintSubStr = strstr( name, "hint");
            assert( hintSubStr );
            fprintf(fp, "%s", hintSubStr);
            if( j < Vec_IntSize(vElem) - 1 )
            {
                fprintf(fp, " || ");
            }
            else
            {
                fprintf(fp, " )\n");
            }
        }
    }
    fclose(fp);
}

void printAllIntVectorsStabil(Vec_Ptr_t *vDisjunctions, Abc_Ntk_t *pNtk, char *fileName)
{
    Vec_Int_t *vElem;
    int i, j, iElem;
    char *name, *hintSubStr;
    FILE *fp;

    fp = fopen( fileName, "a" );

    Vec_PtrForEachEntry(Vec_Int_t *, vDisjunctions, vElem, i)
    {    
        printf("INT[%d] : ( ", i);
        fprintf(fp, "( ");
        Vec_IntForEachEntry( vElem, iElem, j )
        {
            name = Abc_ObjName( Abc_NtkPo(pNtk, iElem));
            hintSubStr = strstr( name, "csLevel1Stabil");
            assert( hintSubStr );
            printf("%s", hintSubStr);
            fprintf(fp, "%s", hintSubStr);
            if( j < Vec_IntSize(vElem) - 1 )
            {
                printf(" || ");
                fprintf(fp, " || ");
            }
            else
            {
                printf(" )\n");
                fprintf(fp, " )\n");
            }
        }
        //printf(")\n");
    }
    fclose(fp);
}


void appendVecToMasterVecInt(Vec_Ptr_t *masterVec, Vec_Ptr_t *candVec )
{
    int i;
    Vec_Int_t *vCand;
    Vec_Int_t *vNewIntVec;

    assert(masterVec != NULL);
    assert(candVec != NULL);
    Vec_PtrForEachEntry( Vec_Int_t *, candVec, vCand, i )
    {
        vNewIntVec = Vec_IntDup(vCand);    
        Vec_PtrPush(masterVec, vNewIntVec);
    }
}

void deallocateVecOfIntVec( Vec_Ptr_t *vecOfIntVec )
{
    Vec_Int_t *vInt;
    int i;

    if( vecOfIntVec )
    {
        Vec_PtrForEachEntry( Vec_Int_t *, vecOfIntVec, vInt, i )
        {
            Vec_IntFree( vInt );
        }
        Vec_PtrFree(vecOfIntVec);
    }    
}

Vec_Ptr_t *findDisjunctiveMonotoneSignals( Abc_Ntk_t *pNtk )
{
    Aig_Man_t *pAig;
    Vec_Int_t *vCandidateMonotoneSignals;
    Vec_Int_t *vKnownMonotoneSignals;
    //Vec_Int_t *vKnownMonotoneSignalsRoundTwo;
    //Vec_Int_t *vOldConsequentVector;
    //Vec_Int_t *vRemainingConsecVector;
    int i;
    int iElem;
    int pendingSignalIndex;
    Abc_Ntk_t *pNtkTemp;
    int hintSingalBeginningMarker;
    int hintSingalEndMarker;
    struct aigPoIndices *aigPoIndicesInstance;
    //struct monotoneVectorsStruct *monotoneVectorsInstance;
    struct antecedentConsequentVectorsStruct *anteConsecInstance;
    //Aig_Obj_t *safetyDriverNew;
    Vec_Int_t *newIntVec;
    Vec_Ptr_t *levelOneMonotne, *levelTwoMonotne;
    //Vec_Ptr_t *levelThreeMonotne;

    Vec_Ptr_t *vMasterDisjunctions;

    extern int findPendingSignal(Abc_Ntk_t *pNtk);
    extern Vec_Int_t *findHintOutputs(Abc_Ntk_t *pNtk);    
    extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );

    //system("rm monotone.dat");
    
    /*******************************************/    
    //Finding the PO index of the pending signal 
    /*******************************************/    
    pendingSignalIndex = findPendingSignal(pNtk);
    if( pendingSignalIndex == -1 )
    {
        printf("\nNo Pending Signal Found\n");
        return NULL;
    }
    //else
        //printf("Po[%d] = %s\n", pendingSignalIndex, Abc_ObjName( Abc_NtkPo(pNtk, pendingSignalIndex) ) );

    /*******************************************/    
    //Finding the PO indices of all hint signals
    /*******************************************/    
    vCandidateMonotoneSignals = findHintOutputs(pNtk);
    if( vCandidateMonotoneSignals == NULL )
        return NULL;
    else
    {
        //Vec_IntForEachEntry( vCandidateMonotoneSignals, iElem, i )
        //    printf("Po[%d] = %s\n", iElem, Abc_ObjName( Abc_NtkPo(pNtk, iElem) ) );
        hintSingalBeginningMarker = Vec_IntEntry( vCandidateMonotoneSignals, 0 );
        hintSingalEndMarker = Vec_IntEntry( vCandidateMonotoneSignals, Vec_IntSize(vCandidateMonotoneSignals) - 1 );
    }

    /**********************************************/
    //Allocating "struct" with necessary parameters
    /**********************************************/
    aigPoIndicesInstance = allocAigPoIndices();
    aigPoIndicesInstance->attrPendingSignalIndex = pendingSignalIndex;
    aigPoIndicesInstance->attrHintSingalBeginningMarker = hintSingalBeginningMarker;
    aigPoIndicesInstance->attrHintSingalEndMarker = hintSingalEndMarker;
    aigPoIndicesInstance->attrSafetyInvarIndex = collectSafetyInvariantPOIndex(pNtk);

    /****************************************************/
    //Allocating "struct" with necessary monotone vectors
    /****************************************************/
    anteConsecInstance = allocAntecedentConsequentVectorsStruct();
    anteConsecInstance->attrAntecedents = NULL;
    anteConsecInstance->attrConsequentCandidates = vCandidateMonotoneSignals;

    /*******************************************/    
    //Generate AIG from Ntk
    /*******************************************/    
    if( !Abc_NtkIsStrash( pNtk ) )
    {
        pNtkTemp = Abc_NtkStrash( pNtk, 0, 0, 0 );
        pAig = Abc_NtkToDar( pNtkTemp, 0, 1 );
    }
    else
    {
        pAig = Abc_NtkToDar( pNtk, 0, 1 );
        pNtkTemp = pNtk;
    }

    /*******************************************/    
    //finding LEVEL 1 monotone signals
    /*******************************************/    
    //printf("Calling target function outside loop\n");
    vKnownMonotoneSignals = findNewDisjunctiveMonotone( pAig, aigPoIndicesInstance, anteConsecInstance );
    levelOneMonotne = Vec_PtrAlloc(0);
    Vec_IntForEachEntry( vKnownMonotoneSignals, iElem, i )
    {
        newIntVec = createSingletonIntVector( iElem );
        Vec_PtrPush( levelOneMonotne, newIntVec );
        //printf("Monotone Po[%d] = %s\n", iElem, Abc_ObjName( Abc_NtkPo(pNtk, iElem) ) );
    }
    //printAllIntVectors( levelOneMonotne, pNtk, "monotone.dat" );

    vMasterDisjunctions = Vec_PtrAlloc( Vec_PtrSize( levelOneMonotne ));
    appendVecToMasterVecInt(vMasterDisjunctions, levelOneMonotne );

    /*******************************************/    
    //finding LEVEL >1 monotone signals
    /*******************************************/    
    #if 0
    if( vKnownMonotoneSignals )
    {
        Vec_IntForEachEntry( vKnownMonotoneSignals, iElem, i )
        {
            printf("\n**************************************************************\n");
            printf("Exploring Second Layer : Reference Po[%d] = %s", iElem, Abc_ObjName( Abc_NtkPo(pNtk, iElem) ));
            printf("\n**************************************************************\n");
            anteConsecInstance->attrAntecedents = createSingletonIntVector( iElem );
            vOldConsequentVector = anteConsecInstance->attrConsequentCandidates;
            vRemainingConsecVector = updateAnteConseVectors(anteConsecInstance);
            if( anteConsecInstance->attrConsequentCandidates != vRemainingConsecVector )
            {
                anteConsecInstance->attrConsequentCandidates = vRemainingConsecVector;
            }
            vKnownMonotoneSignalsRoundTwo = findNewDisjunctiveMonotone( pAig, aigPoIndicesInstance, anteConsecInstance );
            Vec_IntForEachEntry( vKnownMonotoneSignalsRoundTwo, iElemTwo, iTwo )
            {
                printf("Monotone Po[%d] = %s, (%d, %d)\n", iElemTwo, Abc_ObjName( Abc_NtkPo(pNtk, iElemTwo) ), iElem, iElemTwo );
            }
            Vec_IntFree(vKnownMonotoneSignalsRoundTwo);
            Vec_IntFree(anteConsecInstance->attrAntecedents);
            if(anteConsecInstance->attrConsequentCandidates != vOldConsequentVector)
            {
                Vec_IntFree(anteConsecInstance->attrConsequentCandidates);
                anteConsecInstance->attrConsequentCandidates = vOldConsequentVector;
            }
        }
    }
    #endif 

#if 1
    levelTwoMonotne = findNextLevelDisjunctiveMonotone( pAig, aigPoIndicesInstance, anteConsecInstance, levelOneMonotne );
    //printAllIntVectors( levelTwoMonotne, pNtk, "monotone.dat" );
    appendVecToMasterVecInt(vMasterDisjunctions, levelTwoMonotne );
#endif

    //levelThreeMonotne = findNextLevelDisjunctiveMonotone( pAig, aigPoIndicesInstance, anteConsecInstance, levelTwoMonotne );
    //printAllIntVectors( levelThreeMonotne );
    //printAllIntVectors( levelTwoMonotne, pNtk, "monotone.dat" );
    //appendVecToMasterVecInt(vMasterDisjunctions, levelThreeMonotne );

    deallocAigPoIndices(aigPoIndicesInstance);
    deallocAntecedentConsequentVectorsStruct(anteConsecInstance);
    //deallocPointersToMonotoneVectors(monotoneVectorsInstance);

    deallocateVecOfIntVec( levelOneMonotne );
    deallocateVecOfIntVec( levelTwoMonotne );

    Aig_ManStop(pAig);
    Vec_IntFree(vKnownMonotoneSignals);

    return vMasterDisjunctions;
}

ABC_NAMESPACE_IMPL_END
