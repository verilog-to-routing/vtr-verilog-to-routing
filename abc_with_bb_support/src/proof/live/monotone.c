/**CFile****************************************************************

  FileName    [kLiveConstraints.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Liveness property checking.]

  Synopsis    [Constraint analysis module for the k-Liveness algorithm
		invented by Koen Classen, Niklas Sorensson.]

  Author      [Sayak Ray]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 31, 2012.]

  Revision    [$Id: liveness.c,v 1.00 2009/01/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include "base/main/main.h"
#include "aig/aig/aig.h"
#include "aig/saig/saig.h"
#include <string.h>
#include "base/main/mainInt.h"
#include "proof/pdr/pdr.h"

ABC_NAMESPACE_IMPL_START

extern Aig_Man_t *Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
//extern Aig_Man_t *createDisjunctiveMonotoneTester(Aig_Man_t *pAig, struct aigPoIndices *aigPoIndicesArg, struct monotoneVectorsStruct *monotoneVectorArg, int *startMonotonePropPo);

struct aigPoIndices
{
	int attrPendingSignalIndex;
	int attrHintSingalBeginningMarker;
	int attrHintSingalEndMarker;
	int attrSafetyInvarIndex;
};

struct aigPoIndices *allocAigPoIndices()
{
	struct aigPoIndices *newAigPoIndices;

	newAigPoIndices = (struct aigPoIndices *)malloc(sizeof (struct aigPoIndices));
	newAigPoIndices->attrPendingSignalIndex = -1;
	newAigPoIndices->attrHintSingalBeginningMarker = -1;
	newAigPoIndices->attrHintSingalEndMarker = -1;
	newAigPoIndices->attrSafetyInvarIndex = -1;

	assert( newAigPoIndices != NULL );
	return newAigPoIndices;
}

void deallocAigPoIndices(struct aigPoIndices *toBeDeletedAigPoIndices)
{
	assert(toBeDeletedAigPoIndices != NULL );
	free(toBeDeletedAigPoIndices);
}

struct monotoneVectorsStruct
{
	Vec_Int_t *attrKnownMonotone;
	Vec_Int_t *attrCandMonotone;
	Vec_Int_t *attrHintMonotone;
};

struct monotoneVectorsStruct *allocPointersToMonotoneVectors()
{
	struct monotoneVectorsStruct *newPointersToMonotoneVectors;

	newPointersToMonotoneVectors = (struct monotoneVectorsStruct *)malloc(sizeof (struct monotoneVectorsStruct));

	newPointersToMonotoneVectors->attrKnownMonotone = NULL;
	newPointersToMonotoneVectors->attrCandMonotone = NULL;
	newPointersToMonotoneVectors->attrHintMonotone = NULL;
		
	assert( newPointersToMonotoneVectors != NULL );
	return newPointersToMonotoneVectors;
}

void deallocPointersToMonotoneVectors(struct monotoneVectorsStruct *toBeDeleted)
{
	assert( toBeDeleted != NULL );
	free( toBeDeleted );
}

Vec_Int_t *findHintOutputs(Abc_Ntk_t *pNtk)
{
	int i, numElementPush = 0;
	Abc_Obj_t *pNode;
	Vec_Int_t *vHintPoIntdex;

	vHintPoIntdex = Vec_IntAlloc(0);
	Abc_NtkForEachPo( pNtk, pNode, i )
	{
		if( strstr( Abc_ObjName( pNode ), "hint_" ) != NULL )
		{
			Vec_IntPush( vHintPoIntdex, i );
			numElementPush++;
		}
	}	
	
	if( numElementPush == 0 )
		return NULL;
	else
		return vHintPoIntdex;
}

int findPendingSignal(Abc_Ntk_t *pNtk)
{
	int i, pendingSignalIndex = -1;
	Abc_Obj_t *pNode;

	Abc_NtkForEachPo( pNtk, pNode, i )
	{
		if( strstr( Abc_ObjName( pNode ), "pendingSignal" ) != NULL )
		{
			pendingSignalIndex = i;
			break;
		}
	}	
	
	return pendingSignalIndex;
}

int checkSanityOfKnownMonotone( Vec_Int_t *vKnownMonotone, Vec_Int_t *vCandMonotone, Vec_Int_t *vHintMonotone )
{
	int iElem, i;

	Vec_IntForEachEntry( vKnownMonotone, iElem, i )
		printf("%d ", iElem);
	printf("\n");
	Vec_IntForEachEntry( vCandMonotone, iElem, i )
		printf("%d ", iElem);
	printf("\n");
	Vec_IntForEachEntry( vHintMonotone, iElem, i )
		printf("%d ", iElem);
	printf("\n");
	return 1;
}

Aig_Man_t *createMonotoneTester(Aig_Man_t *pAig, struct aigPoIndices *aigPoIndicesArg, struct monotoneVectorsStruct *monotoneVectorArg, int *startMonotonePropPo)
{
	Aig_Man_t *pNewAig;
	int iElem, i, nRegCount, oldPoNum, poSerialNum, iElemHint;
	int piCopied = 0, liCopied = 0, liCreated = 0, loCopied = 0, loCreated = 0;
	int poCopied = 0, poCreated = 0;
	Aig_Obj_t *pObj, *pObjPo, *pObjDriver, *pObjDriverNew, *pObjPendingDriverNew, *pObjPendingAndNextPending;
	Aig_Obj_t *pPendingFlop, *pHintSignalLo, *pHintMonotoneFlop, *pObjTemp1, *pObjTemp2, *pObjKnownMonotoneAnd;
	Vec_Ptr_t *vHintMonotoneLocalDriverNew;
	Vec_Ptr_t *vHintMonotoneLocalFlopOutput;
	Vec_Ptr_t *vHintMonotoneLocalProp;

	int pendingSignalIndexLocal = aigPoIndicesArg->attrPendingSignalIndex;
	int hintSingalBeginningMarkerLocal = aigPoIndicesArg->attrHintSingalBeginningMarker;
	//int hintSingalEndMarkerLocal = aigPoIndicesArg->attrHintSingalEndMarker;

	Vec_Int_t *vKnownMonotoneLocal = monotoneVectorArg->attrKnownMonotone;
	Vec_Int_t *vCandMonotoneLocal = monotoneVectorArg->attrCandMonotone;
	Vec_Int_t *vHintMonotoneLocal = monotoneVectorArg->attrHintMonotone;
	
	//****************************************************************
	// Step1: create the new manager
	// Note: The new manager is created with "2 * Aig_ManObjNumMax(p)"
	// nodes, but this selection is arbitrary - need to be justified
	//****************************************************************
	pNewAig = Aig_ManStart( Aig_ManObjNumMax(pAig) );
	pNewAig->pName = (char *)malloc( strlen( pAig->pName ) + strlen("_monotone") + 1 );
	sprintf(pNewAig->pName, "%s_%s", pAig->pName, "_monotone");
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
	vHintMonotoneLocalFlopOutput = Vec_PtrAlloc(Vec_IntSize(vHintMonotoneLocal));
	Vec_IntForEachEntry( vHintMonotoneLocal, iElem, i )
	{
		loCreated++;
		pHintMonotoneFlop = Aig_ObjCreateCi( pNewAig );
		Vec_PtrPush( vHintMonotoneLocalFlopOutput, pHintMonotoneFlop );
	}

	nRegCount = loCreated + loCopied;
	printf("\nnRegCount = %d\n", nRegCount);

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

	pObjPo = Aig_ManCo( pAig, pendingSignalIndexLocal );
	pObjDriver = Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObjPo), Aig_ObjFaninC0(pObjPo));
	pObjPendingDriverNew = !Aig_IsComplement(pObjDriver)? 
				(Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData) : 
				Aig_Not((Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData));

	pObjPendingAndNextPending = Aig_And( pNewAig, pObjPendingDriverNew, pPendingFlop );

	oldPoNum = Aig_ManCoNum(pAig) - Aig_ManRegNum(pAig);
	pObjKnownMonotoneAnd = Aig_ManConst1( pNewAig );
	#if 1
	if( vKnownMonotoneLocal )
	{
		assert( checkSanityOfKnownMonotone( vKnownMonotoneLocal, vCandMonotoneLocal, vHintMonotoneLocal ) );

		Vec_IntForEachEntry( vKnownMonotoneLocal, iElemHint, i )
		{
			iElem = (iElemHint - hintSingalBeginningMarkerLocal) + 1 + pendingSignalIndexLocal;
			printf("\nProcessing knownMonotone = %d\n", iElem);
			pObjPo = Aig_ManCo( pAig, iElem );
			pObjDriver = Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObjPo), Aig_ObjFaninC0(pObjPo));
			pObjDriverNew = !Aig_IsComplement(pObjDriver)? 
					(Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData) : 
					Aig_Not((Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData));
			pHintSignalLo = (Aig_Obj_t *)Vec_PtrEntry(vHintMonotoneLocalFlopOutput, iElem - oldPoNum);
			pObjTemp1 = Aig_Or( pNewAig, Aig_And(pNewAig, pObjDriverNew, pHintSignalLo), 
						Aig_And(pNewAig, Aig_Not(pObjDriverNew), Aig_Not(pHintSignalLo)) );
			
			pObjKnownMonotoneAnd = Aig_And( pNewAig, pObjKnownMonotoneAnd, pObjTemp1 );
		}
		pObjPendingAndNextPending = Aig_And( pNewAig, pObjPendingAndNextPending, pObjKnownMonotoneAnd );
	}
	#endif

	vHintMonotoneLocalDriverNew = Vec_PtrAlloc(Vec_IntSize(vHintMonotoneLocal));
	vHintMonotoneLocalProp = Vec_PtrAlloc(Vec_IntSize(vHintMonotoneLocal));
	Vec_IntForEachEntry( vHintMonotoneLocal, iElem, i )
	{
		pObjPo = Aig_ManCo( pAig, iElem );
		pObjDriver = Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObjPo), Aig_ObjFaninC0(pObjPo));
		pObjDriverNew = !Aig_IsComplement(pObjDriver)? 
				(Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData) : 
				Aig_Not((Aig_Obj_t *)(Aig_Regular(pObjDriver)->pData));

		if( vKnownMonotoneLocal != NULL && Vec_IntFind( vKnownMonotoneLocal, iElem ) != -1 )
		{
			Vec_PtrPush(vHintMonotoneLocalDriverNew, pObjDriverNew);
		}
		else
		{
			poSerialNum = Vec_IntFind( vHintMonotoneLocal, iElem );
			pHintSignalLo = (Aig_Obj_t *)Vec_PtrEntry(vHintMonotoneLocalFlopOutput, poSerialNum );
			pObjTemp1 = Aig_And( pNewAig, pObjPendingAndNextPending, pHintSignalLo);
			pObjTemp2 = Aig_Or( pNewAig, Aig_Not(pObjTemp1), pObjDriverNew );
			//pObjTemp2 = Aig_Or( pNewAig, Aig_Not(pObjTemp1), Aig_ManConst1( pNewAig ));
			//pObjTemp2 = Aig_ManConst1( pNewAig );
			Vec_PtrPush(vHintMonotoneLocalDriverNew, pObjDriverNew);
			Vec_PtrPush(vHintMonotoneLocalProp, pObjTemp2); 
		}
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
	Vec_PtrForEachEntry( Aig_Obj_t *, vHintMonotoneLocalProp, pObj, i )
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

	Vec_PtrForEachEntry( Aig_Obj_t *, vHintMonotoneLocalDriverNew, pObj, i )
	{
		liCreated++;
		Aig_ObjCreateCo( pNewAig, pObj );
	}

	printf("\npoCopied = %d, poCreated = %d\n", poCopied, poCreated);
	printf("\nliCreated++ = %d\n", liCreated );
	Aig_ManSetRegNum( pNewAig, nRegCount );
	Aig_ManCleanup( pNewAig );
	
	assert( Aig_ManCheck( pNewAig ) );
	assert( loCopied + loCreated == liCopied + liCreated );

	printf("\nSaig_ManPoNum = %d\n", Saig_ManPoNum(pNewAig));
	return pNewAig;
}


Vec_Int_t *findNewMonotone( Aig_Man_t *pAig, struct aigPoIndices *aigPoIndicesArg, struct monotoneVectorsStruct *monotoneVectorArg )
{
	Aig_Man_t *pAigNew;
	Aig_Obj_t *pObjTargetPo;
	int poMarker, oldPoNum;
	int i, RetValue;
	Pdr_Par_t Pars, * pPars = &Pars;
	Abc_Cex_t * pCex = NULL;
	Vec_Int_t *vMonotoneIndex;

	int pendingSignalIndexLocal = aigPoIndicesArg->attrPendingSignalIndex;
	int hintSingalBeginningMarkerLocal = aigPoIndicesArg->attrHintSingalBeginningMarker;
	//int hintSingalEndMarkerLocal = aigPoIndicesArg->attrHintSingalEndMarker;

	//Vec_Int_t *vKnownMonotoneLocal = monotoneVectorArg->attrKnownMonotone;
	//Vec_Int_t *vCandMonotoneLocal = monotoneVectorArg->attrCandMonotone;
	//Vec_Int_t *vHintMonotoneLocal = monotoneVectorArg->attrHintMonotone;

	pAigNew = createMonotoneTester(pAig, aigPoIndicesArg, monotoneVectorArg, &poMarker );
	oldPoNum = Aig_ManCoNum(pAig) - Aig_ManRegNum(pAig);

	vMonotoneIndex = Vec_IntAlloc(0);
	printf("\nSaig_ManPoNum(pAigNew) = %d, poMarker = %d\n", Saig_ManPoNum(pAigNew), poMarker);
	for( i=poMarker; i<Saig_ManPoNum(pAigNew); i++ )
	{
		pObjTargetPo = Aig_ManCo( pAigNew, i );
		Aig_ObjChild0Flip( pObjTargetPo );

		Pdr_ManSetDefaultParams( pPars );
		pCex = NULL;
		pPars->fVerbose = 0;
		//pPars->iOutput = i;
		//RetValue = Pdr_ManSolve( pAigNew, pPars, &pCex );	
		RetValue = Pdr_ManSolve( pAigNew, pPars );	
		if( RetValue == 1 )
		{
			printf("\ni = %d, RetValue = %d : %s (Frame %d)\n", i - oldPoNum + hintSingalBeginningMarkerLocal, RetValue, "Property Proved", pCex? (pCex)->iFrame : -1 );
			Vec_IntPush( vMonotoneIndex, i - (pendingSignalIndexLocal + 1) + hintSingalBeginningMarkerLocal);
		}
		Aig_ObjChild0Flip( pObjTargetPo );
	}
	
	if( Vec_IntSize( vMonotoneIndex ) > 0 )
		return vMonotoneIndex;
	else
		return NULL;
}

Vec_Int_t *findRemainingMonotoneCandidates(Vec_Int_t *vKnownMonotone, Vec_Int_t *vHintMonotone)
{
	Vec_Int_t *vCandMonotone;
	int iElem, i;

	if( vKnownMonotone == NULL || Vec_IntSize(vKnownMonotone) <= 0 )
		return vHintMonotone;
	vCandMonotone = Vec_IntAlloc(0);
	Vec_IntForEachEntry( vHintMonotone, iElem, i )
	{
		if( Vec_IntFind( vKnownMonotone, iElem ) == -1 )
			Vec_IntPush( vCandMonotone, iElem );
	}
	
	return vCandMonotone;
}

Vec_Int_t *findMonotoneSignals( Abc_Ntk_t *pNtk )
{
	Aig_Man_t *pAig;
	Vec_Int_t *vCandidateMonotoneSignals;
	Vec_Int_t *vKnownMonotoneSignals;
	//Vec_Int_t *vKnownMonotoneSignalsNew;
	//Vec_Int_t *vRemainingCanMonotone;
	int i, iElem;
	int pendingSignalIndex;
	Abc_Ntk_t *pNtkTemp;
	int hintSingalBeginningMarker;
	int hintSingalEndMarker;
	struct aigPoIndices *aigPoIndicesInstance;
	struct monotoneVectorsStruct *monotoneVectorsInstance;
	
	/*******************************************/	
	//Finding the PO index of the pending signal 
	/*******************************************/	
	pendingSignalIndex = findPendingSignal(pNtk);
	if( pendingSignalIndex == -1 )
	{
		printf("\nNo Pending Signal Found\n");
		return NULL;
	}
	else
		printf("Po[%d] = %s\n", pendingSignalIndex, Abc_ObjName( Abc_NtkPo(pNtk, pendingSignalIndex) ) );

	/*******************************************/	
	//Finding the PO indices of all hint signals
	/*******************************************/	
	vCandidateMonotoneSignals = findHintOutputs(pNtk);
	if( vCandidateMonotoneSignals == NULL )
		return NULL;
	else
	{
		Vec_IntForEachEntry( vCandidateMonotoneSignals, iElem, i )
			printf("Po[%d] = %s\n", iElem, Abc_ObjName( Abc_NtkPo(pNtk, iElem) ) );
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

	/****************************************************/
	//Allocating "struct" with necessary monotone vectors
	/****************************************************/
	monotoneVectorsInstance = allocPointersToMonotoneVectors();
	monotoneVectorsInstance->attrCandMonotone = vCandidateMonotoneSignals;
	monotoneVectorsInstance->attrHintMonotone = vCandidateMonotoneSignals;

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
	vKnownMonotoneSignals = findNewMonotone( pAig, aigPoIndicesInstance, monotoneVectorsInstance );
	monotoneVectorsInstance->attrKnownMonotone = vKnownMonotoneSignals;

	/*******************************************/	
	//finding LEVEL >1 monotone signals
	/*******************************************/	
	#if 0
	if( vKnownMonotoneSignals )
	{
		printf("\nsize = %d\n", Vec_IntSize(vKnownMonotoneSignals) );
		vRemainingCanMonotone = findRemainingMonotoneCandidates(vKnownMonotoneSignals, vCandidateMonotoneSignals);
		monotoneVectorsInstance->attrCandMonotone = vRemainingCanMonotone; 
		vKnownMonotoneSignalsNew = findNewMonotone( pAig, aigPoIndicesInstance, monotoneVectorsInstance );
	}
	#endif 

	deallocAigPoIndices(aigPoIndicesInstance);
	deallocPointersToMonotoneVectors(monotoneVectorsInstance);
	return NULL;
}

ABC_NAMESPACE_IMPL_END
