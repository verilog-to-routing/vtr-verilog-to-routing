/**CFile****************************************************************

  FileName    [arenaViolation.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Liveness property checking.]

  Synopsis    [module for addition of arena violator detector
		induced by stabilizing constraints]

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

//#define DISJUNCTIVE_CONSTRAINT_ENABLE_MODE
#define BARRIER_MONOTONE_TEST

ABC_NAMESPACE_IMPL_START

Vec_Ptr_t * createArenaLO( Aig_Man_t *pAigNew, Vec_Ptr_t *vBarriers )
{
	Vec_Ptr_t *vArenaLO;
	int barrierCount;
	Aig_Obj_t *pObj;
	int i;

	if( vBarriers == NULL )
		return NULL;

	barrierCount = Vec_PtrSize(vBarriers);
	if( barrierCount <= 0 )
		return NULL;

	vArenaLO = Vec_PtrAlloc(barrierCount);	
	for( i=0; i<barrierCount; i++ )
	{
		pObj = Aig_ObjCreateCi( pAigNew );
		Vec_PtrPush( vArenaLO, pObj );
	}

	return vArenaLO;
}

Vec_Ptr_t * createArenaLi( Aig_Man_t *pAigNew, Vec_Ptr_t *vBarriers, Vec_Ptr_t *vArenaSignal )
{
	Vec_Ptr_t *vArenaLi;
	int barrierCount;
	int i;
	Aig_Obj_t *pObj, *pObjDriver;

	if( vBarriers == NULL )
		return NULL;

	barrierCount = Vec_PtrSize(vBarriers);
	if( barrierCount <= 0 )
		return NULL;

	vArenaLi = Vec_PtrAlloc(barrierCount);	
	for( i=0; i<barrierCount; i++ )
	{
		pObjDriver = (Aig_Obj_t *)Vec_PtrEntry( vArenaSignal, i );
		pObj = Aig_ObjCreateCo( pAigNew, pObjDriver );
		Vec_PtrPush( vArenaLi, pObj );
	}

	return vArenaLi;
}

Vec_Ptr_t *createMonotoneBarrierLO( Aig_Man_t *pAigNew, Vec_Ptr_t *vBarriers )
{
	Vec_Ptr_t *vMonotoneLO;
	int barrierCount;
	Aig_Obj_t *pObj;
	int i;

	if( vBarriers == NULL )
		return NULL;

	barrierCount = Vec_PtrSize(vBarriers);
	if( barrierCount <= 0 )
		return NULL;

	vMonotoneLO = Vec_PtrAlloc(barrierCount);	
	for( i=0; i<barrierCount; i++ )
	{
		pObj = Aig_ObjCreateCi( pAigNew );
		Vec_PtrPush( vMonotoneLO, pObj );
	}

	return vMonotoneLO;
}

Aig_Obj_t *driverToPoNew( Aig_Man_t *pAig, Aig_Obj_t *pObjPo )
{
	Aig_Obj_t *poDriverOld;
	Aig_Obj_t *poDriverNew;	
	
	//Aig_ObjPrint( pAig, pObjPo );
	//printf("\n");

	assert( Aig_ObjIsCo(pObjPo) );
	poDriverOld = Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObjPo), Aig_ObjFaninC0(pObjPo));
	assert( !Aig_ObjIsCo(poDriverOld) );
	poDriverNew = !Aig_IsComplement(poDriverOld)? 
			(Aig_Obj_t *)(Aig_Regular(poDriverOld)->pData) : 
			Aig_Not((Aig_Obj_t *)(Aig_Regular(poDriverOld)->pData));
	//assert( !Aig_ObjIsCo(poDriverNew) );
	return poDriverNew;
}

Vec_Ptr_t *collectBarrierDisjunctions(Aig_Man_t *pAigOld, Aig_Man_t *pAigNew, Vec_Ptr_t *vBarriers)
{
	int barrierCount, i, j, jElem;
	Vec_Int_t *vIthBarrier;
	Aig_Obj_t *pObjBarrier, *pObjCurr, *pObjTargetPoOld;
	Vec_Ptr_t *vNewBarrierSignals;

	if( vBarriers == NULL )
		return NULL;
	barrierCount = Vec_PtrSize( vBarriers );
	if( barrierCount <= 0 )
		return NULL;

	vNewBarrierSignals = Vec_PtrAlloc( barrierCount );

	for( i=0; i<barrierCount; i++ )
	{
		vIthBarrier = (Vec_Int_t *)Vec_PtrEntry( vBarriers, i );
		assert( Vec_IntSize( vIthBarrier ) >= 1 );
		pObjBarrier = Aig_Not(Aig_ManConst1(pAigNew));
		Vec_IntForEachEntry( vIthBarrier, jElem, j )
		{
			pObjTargetPoOld = Aig_ManCo( pAigOld, jElem );
			//Aig_ObjPrint( pAigOld, pObjTargetPoOld );
			//printf("\n");
			pObjCurr = driverToPoNew( pAigOld, pObjTargetPoOld );
			pObjBarrier = Aig_Or( pAigNew, pObjCurr, pObjBarrier );
		}
		assert( pObjBarrier );
		Vec_PtrPush(vNewBarrierSignals, pObjBarrier);
	}
	assert( Vec_PtrSize( vNewBarrierSignals ) == barrierCount );

	return vNewBarrierSignals;
}

Aig_Obj_t *Aig_Xor( Aig_Man_t *pAig, Aig_Obj_t *pObj1, Aig_Obj_t *pObj2 )
{
	return Aig_Or( pAig, Aig_And( pAig, pObj1, Aig_Not(pObj2) ), Aig_And( pAig, Aig_Not(pObj1), pObj2 ) );
}

Aig_Obj_t *createArenaViolation(
		Aig_Man_t *pAigOld,
		Aig_Man_t *pAigNew, 
		Aig_Obj_t *pWindowBegins, 
		Aig_Obj_t *pWithinWindow, 
		Vec_Ptr_t *vMasterBarriers, 
		Vec_Ptr_t *vBarrierLo,
		Vec_Ptr_t *vBarrierLiDriver,
		Vec_Ptr_t *vMonotoneDisjunctionNodes
		)
{
	Aig_Obj_t *pWindowBeginsLocal = pWindowBegins;
	Aig_Obj_t *pWithinWindowLocal = pWithinWindow;
	int i;
	Aig_Obj_t *pObj, *pObjAnd1, *pObjOr1, *pObjAnd2, *pObjBarrierLo, *pObjBarrierSwitch, *pObjArenaViolation;
	Vec_Ptr_t *vBarrierSignals;

	assert( vBarrierLiDriver != NULL );
	assert( vMonotoneDisjunctionNodes != NULL );

	pObjArenaViolation = Aig_Not(Aig_ManConst1( pAigNew ));

	vBarrierSignals = collectBarrierDisjunctions(pAigOld, pAigNew, vMasterBarriers);
	assert( vBarrierSignals != NULL );

	assert( Vec_PtrSize( vMonotoneDisjunctionNodes ) == 0 );
	Vec_PtrForEachEntry( Aig_Obj_t *, vBarrierSignals, pObj, i )
		Vec_PtrPush( vMonotoneDisjunctionNodes, pObj );
	assert( Vec_PtrSize( vMonotoneDisjunctionNodes ) == Vec_PtrSize( vMasterBarriers ) );

	Vec_PtrForEachEntry( Aig_Obj_t *, vBarrierSignals, pObj, i )
	{
		//pObjNew = driverToPoNew( pAigOld, pObj );
		pObjAnd1 = Aig_And(pAigNew, pObj, pWindowBeginsLocal);
		pObjBarrierLo = (Aig_Obj_t *)Vec_PtrEntry( vBarrierLo, i );
		pObjOr1 = Aig_Or(pAigNew, pObjAnd1, pObjBarrierLo);
		Vec_PtrPush( vBarrierLiDriver, pObjOr1 );

		pObjBarrierSwitch = Aig_Xor( pAigNew, pObj, pObjBarrierLo );
		pObjAnd2 = Aig_And( pAigNew, pObjBarrierSwitch, pWithinWindowLocal );
		pObjArenaViolation = Aig_Or( pAigNew, pObjAnd2, pObjArenaViolation );
	}

	Vec_PtrFree(vBarrierSignals);
	return pObjArenaViolation;
}

Aig_Obj_t *createConstrained0LiveConeWithDSC( Aig_Man_t *pNewAig, Vec_Ptr_t *signalList )
{
	Aig_Obj_t *pConsequent, *pConsequentCopy, *pAntecedent, *p0LiveCone, *pObj;
	int i, numSigAntecedent;
	
	numSigAntecedent = Vec_PtrSize( signalList ) - 1;

	pAntecedent = Aig_ManConst1( pNewAig );
	pConsequent = (Aig_Obj_t *)Vec_PtrEntry( signalList, numSigAntecedent );
	pConsequentCopy = Aig_NotCond( (Aig_Obj_t *)(Aig_Regular(pConsequent)->pData), Aig_IsComplement( pConsequent ) );

	for(i=0; i<numSigAntecedent; i++ )
	{
		pObj = (Aig_Obj_t *)Vec_PtrEntry( signalList, i );
		assert( Aig_Regular(pObj)->pData );
		pAntecedent = Aig_And( pNewAig, pAntecedent, Aig_NotCond((Aig_Obj_t *)(Aig_Regular(pObj)->pData), Aig_IsComplement(pObj)) );
	}

	p0LiveCone = Aig_Or( pNewAig, Aig_Not(pAntecedent), pConsequentCopy );

	return p0LiveCone;
}

Vec_Ptr_t *collectCSSignalsWithDSC( Abc_Ntk_t *pNtk, Aig_Man_t *pAig )
{
	int i;
	Aig_Obj_t *pObj, *pConsequent = NULL;
	Vec_Ptr_t *vNodeArray;

	vNodeArray = Vec_PtrAlloc(1);

	Saig_ManForEachPo( pAig, pObj, i )
	{
		if( strstr( Abc_ObjName(Abc_NtkPo( pNtk, i )), "csLiveConst_" ) != NULL )
			Vec_PtrPush( vNodeArray, Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj), Aig_ObjFaninC0(pObj)) );	
		else if( strstr( Abc_ObjName(Abc_NtkPo( pNtk, i )), "csLiveTarget_" ) != NULL )
			pConsequent = Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj), Aig_ObjFaninC0(pObj));
	}
	assert( pConsequent );
	Vec_PtrPush( vNodeArray, pConsequent );	
	return vNodeArray;
}

int collectWindowBeginSignalWithDSC( Abc_Ntk_t *pNtk, Aig_Man_t *pAig )
{
	int i;
	Aig_Obj_t *pObj;

	Saig_ManForEachPo( pAig, pObj, i )
	{
		if( strstr( Abc_ObjName(Abc_NtkPo( pNtk, i )), "windowBegins_" ) != NULL )
		{
			return i;			
		}
	}
	
	return -1;
}

int collectWithinWindowSignalWithDSC( Abc_Ntk_t *pNtk, Aig_Man_t *pAig )
{
	int i;
	Aig_Obj_t *pObj;

	Saig_ManForEachPo( pAig, pObj, i )
	{
		if( strstr( Abc_ObjName(Abc_NtkPo( pNtk, i )), "withinWindow_" ) != NULL )
			return i;			
	}
	
	return -1;
}

int collectPendingSignalWithDSC( Abc_Ntk_t *pNtk, Aig_Man_t *pAig )
{
	int i;
	Aig_Obj_t *pObj;

	Saig_ManForEachPo( pAig, pObj, i )
	{
		if( strstr( Abc_ObjName(Abc_NtkPo( pNtk, i )), "pendingSignal" ) != NULL )
			return i;			
	}
	
	return -1;
}

Aig_Obj_t *createAndGateForMonotonicityVerification(
				Aig_Man_t *pNewAig,
				Vec_Ptr_t *vDisjunctionSignals,
				Vec_Ptr_t *vDisjunctionLo,
				Aig_Obj_t *pendingLo,
				Aig_Obj_t *pendingSignal
				)
{
	Aig_Obj_t *pObjBigAnd, *pObj, *pObjLo, *pObjImply;
	Aig_Obj_t *pObjPendingAndPendingLo;
	int i;

	pObjBigAnd = Aig_ManConst1( pNewAig );
	pObjPendingAndPendingLo = Aig_And( pNewAig, pendingLo, pendingSignal );
	Vec_PtrForEachEntry( Aig_Obj_t *, vDisjunctionSignals, pObj, i )
	{
		pObjLo = (Aig_Obj_t *)Vec_PtrEntry( vDisjunctionLo, i );	
		pObjImply = Aig_Or( pNewAig, Aig_Not(Aig_And( pNewAig, pObjPendingAndPendingLo, pObjLo)),
					pObj );
		pObjBigAnd = Aig_And( pNewAig, pObjBigAnd, pObjImply );
	}
	return pObjBigAnd;
}

Aig_Man_t *createNewAigWith0LivePoWithDSC( Aig_Man_t *pAig, Vec_Ptr_t *signalList, int *index0Live, int windowBeginIndex, int withinWindowIndex, int pendingSignalIndex, Vec_Ptr_t *vBarriers )
{
	Aig_Man_t *pNewAig;
	Aig_Obj_t *pObj, *pObjNewPoDriver;
	int i;
	int loCopied = 0, loCreated = 0, liCopied = 0, liCreated = 0;
	Aig_Obj_t *pObjWindowBeginsNew, *pObjWithinWindowNew, *pObjArenaViolation, *pObjTarget, *pObjArenaViolationLiDriver;
	Aig_Obj_t *pObjNewPoDriverArenaViolated, *pObjArenaViolationLo;
	Vec_Ptr_t *vBarrierLo, *vBarrierLiDriver, *vBarrierLi;
	Vec_Ptr_t *vMonotoneNodes;

#ifdef BARRIER_MONOTONE_TEST
	Aig_Obj_t *pObjPendingSignal;
	Aig_Obj_t *pObjPendingFlopLo;
	Vec_Ptr_t *vMonotoneBarrierLo;
	Aig_Obj_t *pObjPendingAndPendingSignal, *pObjMonotoneAnd, *pObjCurrMonotoneLo;
#endif

	//assert( Vec_PtrSize( signalList ) > 1 );

	//****************************************************************
	// Step1: create the new manager
	// Note: The new manager is created with "2 * Aig_ManObjNumMax(p)"
	// nodes, but this selection is arbitrary - need to be justified
	//****************************************************************
	pNewAig = Aig_ManStart( Aig_ManObjNumMax(pAig) );
	pNewAig->pName = (char *)malloc( strlen( pAig->pName ) + strlen("_0Live") + 1 );
	sprintf(pNewAig->pName, "%s_%s", pAig->pName, "0Live");
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
		pObj->pData = Aig_ObjCreateCi( pNewAig );
	}

	//****************************************************************
	// Step 4: create register outputs
	//****************************************************************
    	Saig_ManForEachLo( pAig, pObj, i )
    	{
		loCopied++;
		pObj->pData = Aig_ObjCreateCi( pNewAig );
    	}

	//****************************************************************
	// Step 4.a: create register outputs for the barrier flops
	//****************************************************************
	vBarrierLo = createArenaLO( pNewAig, vBarriers );
	loCreated = Vec_PtrSize(vBarrierLo);

	//****************************************************************
	// Step 4.b: create register output for arenaViolationFlop
	//****************************************************************
	pObjArenaViolationLo = Aig_ObjCreateCi( pNewAig );
	loCreated++;

#ifdef BARRIER_MONOTONE_TEST
	//****************************************************************
	// Step 4.c: create register output for pendingFlop
	//****************************************************************
	pObjPendingFlopLo = Aig_ObjCreateCi( pNewAig );
	loCreated++;

	//****************************************************************
	// Step 4.d: create register outputs for the barrier flops
	// for asserting monotonicity
	//****************************************************************
	vMonotoneBarrierLo = createMonotoneBarrierLO( pNewAig, vBarriers );
	loCreated = loCreated + Vec_PtrSize(vMonotoneBarrierLo);
#endif

	//********************************************************************
	// Step 5: create internal nodes
	//********************************************************************
    	Aig_ManForEachNode( pAig, pObj, i )
	{
		pObj->pData = Aig_And( pNewAig, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
	}

	//********************************************************************
	// Step 5.a: create internal nodes corresponding to arenaViolation
	//********************************************************************
	pObjTarget = Aig_ManCo( pAig, windowBeginIndex );
	pObjWindowBeginsNew = driverToPoNew( pAig, pObjTarget );

	pObjTarget = Aig_ManCo( pAig, withinWindowIndex );
	pObjWithinWindowNew = driverToPoNew( pAig, pObjTarget );

	vBarrierLiDriver = Vec_PtrAlloc( Vec_PtrSize(vBarriers) );
	vMonotoneNodes = Vec_PtrAlloc( Vec_PtrSize(vBarriers) );

	pObjArenaViolation = createArenaViolation( pAig, pNewAig, 
				pObjWindowBeginsNew, pObjWithinWindowNew, 
				vBarriers, vBarrierLo, vBarrierLiDriver, vMonotoneNodes ); 
	assert( Vec_PtrSize(vMonotoneNodes) == Vec_PtrSize(vBarriers) );

#ifdef ARENA_VIOLATION_CONSTRAINT

#endif

	pObjArenaViolationLiDriver = Aig_Or( pNewAig, pObjArenaViolation, pObjArenaViolationLo );

#ifdef BARRIER_MONOTONE_TEST
	//********************************************************************
	// Step 5.b: Create internal nodes for monotone testing
	//********************************************************************

	pObjTarget = Aig_ManCo( pAig, pendingSignalIndex );
	pObjPendingSignal = driverToPoNew( pAig, pObjTarget );
	
	pObjPendingAndPendingSignal = Aig_And( pNewAig, pObjPendingSignal, pObjPendingFlopLo );
	pObjMonotoneAnd = Aig_ManConst1( pNewAig );
	Vec_PtrForEachEntry( Aig_Obj_t *, vMonotoneNodes, pObj, i )
	{
		pObjCurrMonotoneLo = (Aig_Obj_t *)Vec_PtrEntry(vMonotoneBarrierLo, i);
		pObjMonotoneAnd = Aig_And( pNewAig, pObjMonotoneAnd,
			Aig_Or( pNewAig, 
			Aig_Not(Aig_And(pNewAig, pObjPendingAndPendingSignal, pObjCurrMonotoneLo)),
			pObj ) );
	}
#endif

	//********************************************************************
	// Step 6: create primary outputs
	//********************************************************************

	Saig_ManForEachPo( pAig, pObj, i )
	{
		pObj->pData = Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) ); 
	}

	pObjNewPoDriver = createConstrained0LiveConeWithDSC( pNewAig, signalList );
	pObjNewPoDriverArenaViolated = Aig_Or( pNewAig, pObjNewPoDriver, pObjArenaViolationLo );
#ifdef BARRIER_MONOTONE_TEST
	pObjNewPoDriverArenaViolated = Aig_And( pNewAig, pObjNewPoDriverArenaViolated, pObjMonotoneAnd );
#endif
	Aig_ObjCreateCo( pNewAig, pObjNewPoDriverArenaViolated );

	*index0Live = i;

	//********************************************************************
	// Step 7: create register inputs
	//********************************************************************

	Saig_ManForEachLi( pAig, pObj, i )
	{
		liCopied++;
		pObj->pData = Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) );
	}

	//********************************************************************
	// Step 7.a: create register inputs for barrier flops
	//********************************************************************
	assert( Vec_PtrSize(vBarrierLiDriver) == Vec_PtrSize(vBarriers) );
	vBarrierLi = createArenaLi( pNewAig, vBarriers, vBarrierLiDriver );
	liCreated = Vec_PtrSize( vBarrierLi );

	//********************************************************************
	// Step 7.b: create register inputs for arenaViolation flop
	//********************************************************************
	Aig_ObjCreateCo( pNewAig, pObjArenaViolationLiDriver );
	liCreated++;

#ifdef BARRIER_MONOTONE_TEST
	//****************************************************************
	// Step 7.c: create register input for pendingFlop
	//****************************************************************
	Aig_ObjCreateCo( pNewAig, pObjPendingSignal);
	liCreated++;

	//********************************************************************
	// Step 7.d: create register inputs for the barrier flops
	// for asserting monotonicity
	//********************************************************************
	Vec_PtrForEachEntry( Aig_Obj_t *, vMonotoneNodes, pObj, i )
	{
		Aig_ObjCreateCo( pNewAig, pObj );
		liCreated++;
	}
#endif

	assert(loCopied + loCreated == liCopied + liCreated);
	//next step should be changed
	Aig_ManSetRegNum( pNewAig, loCopied + loCreated );
	Aig_ManCleanup( pNewAig );

	assert( Aig_ManCheck( pNewAig ) );

	Vec_PtrFree(vBarrierLo);
	Vec_PtrFree(vMonotoneBarrierLo);
	Vec_PtrFree(vBarrierLiDriver);
	Vec_PtrFree(vBarrierLi);
	Vec_PtrFree(vMonotoneNodes);

	return pNewAig;
}

Aig_Man_t *generateWorkingAigWithDSC( Aig_Man_t *pAig, Abc_Ntk_t *pNtk, int *pIndex0Live, Vec_Ptr_t *vMasterBarriers )
{
	Vec_Ptr_t *vSignalVector;
	Aig_Man_t *pAigNew;
	int pObjWithinWindow;
	int pObjWindowBegin;
	int pObjPendingSignal;

	vSignalVector = collectCSSignalsWithDSC( pNtk, pAig );	

	pObjWindowBegin = collectWindowBeginSignalWithDSC( pNtk, pAig );
	pObjWithinWindow = collectWithinWindowSignalWithDSC( pNtk, pAig );
	pObjPendingSignal = collectPendingSignalWithDSC( pNtk, pAig );

	pAigNew = createNewAigWith0LivePoWithDSC( pAig, vSignalVector, pIndex0Live, pObjWindowBegin, pObjWithinWindow, pObjPendingSignal, vMasterBarriers );
	Vec_PtrFree(vSignalVector);

	return pAigNew;
}

ABC_NAMESPACE_IMPL_END
