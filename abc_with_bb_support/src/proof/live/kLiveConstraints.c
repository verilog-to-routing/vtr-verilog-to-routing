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

Aig_Obj_t *createConstrained0LiveCone( Aig_Man_t *pNewAig, Vec_Ptr_t *signalList )
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

Vec_Ptr_t *collectCSSignals( Abc_Ntk_t *pNtk, Aig_Man_t *pAig )
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

Aig_Man_t *createNewAigWith0LivePo( Aig_Man_t *pAig, Vec_Ptr_t *signalList, int *index0Live )
{
	Aig_Man_t *pNewAig;
	Aig_Obj_t *pObj, *pObjNewPoDriver;
	int i;

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
	// Step 5: create register outputs
	//****************************************************************
    	Saig_ManForEachLo( pAig, pObj, i )
    	{
		pObj->pData = Aig_ObjCreateCi( pNewAig );
    	}

	//********************************************************************
	// Step 7: create internal nodes
	//********************************************************************
    	Aig_ManForEachNode( pAig, pObj, i )
	{
		pObj->pData = Aig_And( pNewAig, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
	}

	Saig_ManForEachPo( pAig, pObj, i )
	{
		pObj->pData = Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) ); 
	}

	pObjNewPoDriver = createConstrained0LiveCone( pNewAig, signalList );
	Aig_ObjCreateCo( pNewAig, pObjNewPoDriver );
	*index0Live = i;

	Saig_ManForEachLi( pAig, pObj, i )
	{
		pObj->pData = Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) );
	}

	Aig_ManSetRegNum( pNewAig, Aig_ManRegNum(pAig) );
	Aig_ManCleanup( pNewAig );

	assert( Aig_ManCheck( pNewAig ) );
	return pNewAig;
}

Vec_Ptr_t *checkMonotoneSignal()
{
	return NULL;
}

Vec_Ptr_t *gatherMonotoneSignals(Aig_Man_t *pAig)
{
	int i;
	Aig_Obj_t *pObj;

	Aig_ManForEachNode( pAig, pObj, i )
	{
		Aig_ObjPrint( pAig, pObj );
		printf("\n");
	}
	
	return NULL;
}

Aig_Man_t *generateWorkingAig( Aig_Man_t *pAig, Abc_Ntk_t *pNtk, int *pIndex0Live )
{
	Vec_Ptr_t *vSignalVector;
	Aig_Man_t *pAigNew;

	vSignalVector = collectCSSignals( pNtk, pAig );	
	assert(vSignalVector);
	pAigNew = createNewAigWith0LivePo( pAig, vSignalVector, pIndex0Live );
	Vec_PtrFree(vSignalVector);

	return pAigNew;
}

ABC_NAMESPACE_IMPL_END

