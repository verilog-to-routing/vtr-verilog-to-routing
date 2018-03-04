#include <stdio.h>
#include "base/main/main.h"
#include "aig/aig/aig.h"
#include "aig/saig/saig.h"
#include <string.h>
#include "base/main/mainInt.h"
#include "proof/pdr/pdr.h"
#include <time.h>

ABC_NAMESPACE_IMPL_START

long countCombination(long n, long k)
{
	assert( n >= k );
	if( n == k ) return 1;
	if( k == 1 ) return n;
	return countCombination( n-1, k-1 ) + countCombination( n-1, k );
}	

void listCombination(int n, int t)
{
	Vec_Int_t *vC;
	int i, j, combCounter = 0;

	//Initialization
	vC = Vec_IntAlloc(t+3);
	for(i=-1; i<t; i++)
		Vec_IntPush( vC, i );
	Vec_IntPush( vC, n );
	Vec_IntPush( vC, 0 );

	while(1)
	{
		//visit combination
		printf("Comb-%3d : ", ++combCounter);
		for( i=t; i>0; i--)
			printf("vC[%d] = %d ", i, Vec_IntEntry(vC, i));
		printf("\n");

		j = 1;
		while( Vec_IntEntry( vC, j ) + 1 == Vec_IntEntry( vC, j+1 ) )
		{
			//printf("\nGochon = %d, %d\n", Vec_IntEntry( vC, j ) + 1, Vec_IntEntry( vC, j+1 ));
			Vec_IntWriteEntry( vC, j, j-1 );
			j = j + 1;
		}
		if( j > t ) break;
		Vec_IntWriteEntry( vC, j, Vec_IntEntry( vC, j ) + 1 );
	}
	
	Vec_IntFree(vC);	
}

int generateCombinatorialStabil( Aig_Man_t *pAigNew, Aig_Man_t *pAigOld, 
				Vec_Int_t *vCandidateMonotoneSignals_,
				Vec_Ptr_t *vDisj_nCk_,
				int combN, int combK )
{
	Aig_Obj_t *pObjMonoCand, *pObj;
	int targetPoIndex;

	//Knuth's Data Strcuture
	int totalCombination_KNUTH = 0;
	Vec_Int_t *vC_KNUTH;
	int i_KNUTH, j_KNUTH;

	//Knuth's Data Structure Initialization
	vC_KNUTH = Vec_IntAlloc(combK+3);
	for(i_KNUTH=-1; i_KNUTH<combK; i_KNUTH++)
		Vec_IntPush( vC_KNUTH, i_KNUTH );
	Vec_IntPush( vC_KNUTH, combN );
	Vec_IntPush( vC_KNUTH, 0 );

	while(1)
	{
		totalCombination_KNUTH++;
		pObjMonoCand = Aig_Not(Aig_ManConst1(pAigNew));
		for( i_KNUTH=combK; i_KNUTH>0; i_KNUTH--)
		{
			targetPoIndex = Vec_IntEntry( vCandidateMonotoneSignals_, Vec_IntEntry(vC_KNUTH, i_KNUTH));
			pObj = Aig_ObjChild0Copy(Aig_ManCo( pAigOld, targetPoIndex ));
			pObjMonoCand = Aig_Or( pAigNew, pObj, pObjMonoCand );
		}
		Vec_PtrPush(vDisj_nCk_, pObjMonoCand );

		j_KNUTH = 1;
		while( Vec_IntEntry( vC_KNUTH, j_KNUTH ) + 1 == Vec_IntEntry( vC_KNUTH, j_KNUTH+1 ) )
		{
			Vec_IntWriteEntry( vC_KNUTH, j_KNUTH, j_KNUTH-1 );
			j_KNUTH = j_KNUTH + 1;
		}
		if( j_KNUTH > combK ) break;
		Vec_IntWriteEntry( vC_KNUTH, j_KNUTH, Vec_IntEntry( vC_KNUTH, j_KNUTH ) + 1 );
	}

	Vec_IntFree(vC_KNUTH);

	return totalCombination_KNUTH;
}

int generateCombinatorialStabilExhaust( Aig_Man_t *pAigNew, Aig_Man_t *pAigOld, 
				Vec_Ptr_t *vDisj_nCk_,
				int combN, int combK )
{
	Aig_Obj_t *pObjMonoCand, *pObj;
	int targetPoIndex;

	//Knuth's Data Strcuture
	int totalCombination_KNUTH = 0;
	Vec_Int_t *vC_KNUTH;
	int i_KNUTH, j_KNUTH;

	//Knuth's Data Structure Initialization
	vC_KNUTH = Vec_IntAlloc(combK+3);
	for(i_KNUTH=-1; i_KNUTH<combK; i_KNUTH++)
		Vec_IntPush( vC_KNUTH, i_KNUTH );
	Vec_IntPush( vC_KNUTH, combN );
	Vec_IntPush( vC_KNUTH, 0 );

	while(1)
	{
		totalCombination_KNUTH++;
		pObjMonoCand = Aig_Not(Aig_ManConst1(pAigNew));
		for( i_KNUTH=combK; i_KNUTH>0; i_KNUTH--)
		{
			//targetPoIndex = Vec_IntEntry( vCandidateMonotoneSignals_, Vec_IntEntry(vC_KNUTH, i_KNUTH));
			targetPoIndex = Vec_IntEntry(vC_KNUTH, i_KNUTH);
			pObj = (Aig_Obj_t *)(Aig_ManLo( pAigOld, targetPoIndex )->pData);
			pObjMonoCand = Aig_Or( pAigNew, pObj, pObjMonoCand );
		}
		Vec_PtrPush(vDisj_nCk_, pObjMonoCand );

		j_KNUTH = 1;
		while( Vec_IntEntry( vC_KNUTH, j_KNUTH ) + 1 == Vec_IntEntry( vC_KNUTH, j_KNUTH+1 ) )
		{
			Vec_IntWriteEntry( vC_KNUTH, j_KNUTH, j_KNUTH-1 );
			j_KNUTH = j_KNUTH + 1;
		}
		if( j_KNUTH > combK ) break;
		Vec_IntWriteEntry( vC_KNUTH, j_KNUTH, Vec_IntEntry( vC_KNUTH, j_KNUTH ) + 1 );
	}

	Vec_IntFree(vC_KNUTH);

	return totalCombination_KNUTH;
}

Aig_Man_t *generateDisjunctiveTester( Abc_Ntk_t *pNtk, Aig_Man_t *pAig, int combN, int combK )
{
	//AIG creation related data structure
	Aig_Man_t *pNewAig;
	int piCopied = 0, loCopied = 0, loCreated = 0, liCopied = 0, liCreated = 0, poCopied = 0;
	//int i, iElem, nRegCount, hintSingalBeginningMarker, hintSingalEndMarker;
	int i, nRegCount, hintSingalBeginningMarker, hintSingalEndMarker;
	int combN_internal, combK_internal; //, targetPoIndex;
	long longI, lCombinationCount;
	//Aig_Obj_t *pObj, *pObjMonoCand, *pObjLO_nCk, *pObjDisj_nCk;
	Aig_Obj_t *pObj, *pObjLO_nCk, *pObjDisj_nCk;
	Vec_Ptr_t *vLO_nCk, *vPODriver_nCk, *vDisj_nCk;
	Vec_Int_t *vCandidateMonotoneSignals;

	extern Vec_Int_t *findHintOutputs(Abc_Ntk_t *pNtk);
	
	//Knuth's Data Strcuture
	//Vec_Int_t *vC_KNUTH;
	//int i_KNUTH, j_KNUTH, combCounter_KNUTH = 0;

	//Collecting target HINT signals
	vCandidateMonotoneSignals = findHintOutputs(pNtk);
	if( vCandidateMonotoneSignals == NULL )
	{
		printf("\nTraget Signal Set is Empty: Duplicating given AIG\n");
		combN_internal = 0;
	}
	else
	{
		//Vec_IntForEachEntry( vCandidateMonotoneSignals, iElem, i )
		//	printf("Po[%d] = %s\n", iElem, Abc_ObjName( Abc_NtkPo(pNtk, iElem) ) );
		hintSingalBeginningMarker = Vec_IntEntry( vCandidateMonotoneSignals, 0 );
		hintSingalEndMarker = Vec_IntEntry( vCandidateMonotoneSignals, Vec_IntSize(vCandidateMonotoneSignals) - 1 );
		combN_internal = hintSingalEndMarker - hintSingalBeginningMarker + 1;
	}

	//combK_internal = combK;

	//Knuth's Data Structure Initialization
	//vC_KNUTH = Vec_IntAlloc(combK_internal+3);
	//for(i_KNUTH=-1; i_KNUTH<combK_internal; i_KNUTH++)
	//	Vec_IntPush( vC_KNUTH, i_KNUTH );
	//Vec_IntPush( vC_KNUTH, combN_internal );
	//Vec_IntPush( vC_KNUTH, 0 );

	//Standard AIG duplication begins
	//Standard AIG Manager Creation
	pNewAig = Aig_ManStart( Aig_ManObjNumMax(pAig) );
	pNewAig->pName = (char *)malloc( strlen( pAig->pName ) + strlen("_nCk") + 1 );
	sprintf(pNewAig->pName, "%s_%s", pAig->pName, "nCk");
    	pNewAig->pSpec = NULL;

	//Standard Mapping of Constant Nodes
	pObj = Aig_ManConst1( pAig );
    	pObj->pData = Aig_ManConst1( pNewAig );

	//Standard AIG PI duplication
	Saig_ManForEachPi( pAig, pObj, i )
	{
		piCopied++;
		pObj->pData = Aig_ObjCreateCi(pNewAig);
	}

	//Standard AIG LO duplication
	Saig_ManForEachLo( pAig, pObj, i )
    	{
		loCopied++;
		pObj->pData = Aig_ObjCreateCi(pNewAig);
    	}

	//nCk LO creation
	lCombinationCount = 0;
	for(combK_internal=1; combK_internal<=combK; combK_internal++)
		lCombinationCount += countCombination( combN_internal, combK_internal );
	assert( lCombinationCount > 0 );
	vLO_nCk = Vec_PtrAlloc(lCombinationCount);
	for( longI = 0; longI < lCombinationCount; longI++ )
	{
		loCreated++;
		pObj = Aig_ObjCreateCi(pNewAig);
		Vec_PtrPush( vLO_nCk, pObj );
	}

	//Standard Node duplication
	Aig_ManForEachNode( pAig, pObj, i )
	{
		pObj->pData = Aig_And( pNewAig, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
	}
	
	//nCk specific logic creation (i.e. nCk number of OR gates)
	vDisj_nCk = Vec_PtrAlloc(lCombinationCount);

	

	//while(1)
	//{
	//	//visit combination
	//	//printf("Comb-%3d : ", ++combCounter_KNUTH);
	//	pObjMonoCand = Aig_Not(Aig_ManConst1(pNewAig));
	//	for( i_KNUTH=combK_internal; i_KNUTH>0; i_KNUTH--)
	//	{
	//		//printf("vC[%d] = %d ", i_KNUTH, Vec_IntEntry(vC_KNUTH, i_KNUTH));
	//		targetPoIndex = Vec_IntEntry( vCandidateMonotoneSignals, Vec_IntEntry(vC_KNUTH, i_KNUTH));
	//		pObj = Aig_ObjChild0Copy(Aig_ManCo( pAig, targetPoIndex ));
	//		pObjMonoCand = Aig_Or( pNewAig, pObj, pObjMonoCand );
	//	}
	//	Vec_PtrPush(vDisj_nCk, pObjMonoCand );
	//	//printf("\n");

	//	j_KNUTH = 1;
	//	while( Vec_IntEntry( vC_KNUTH, j_KNUTH ) + 1 == Vec_IntEntry( vC_KNUTH, j_KNUTH+1 ) )
	//	{
	//		Vec_IntWriteEntry( vC_KNUTH, j_KNUTH, j_KNUTH-1 );
	//		j_KNUTH = j_KNUTH + 1;
	//	}
	//	if( j_KNUTH > combK_internal ) break;
	//	Vec_IntWriteEntry( vC_KNUTH, j_KNUTH, Vec_IntEntry( vC_KNUTH, j_KNUTH ) + 1 );
	//}
	for( combK_internal=1; combK_internal<=combK; combK_internal++ )
		generateCombinatorialStabil( pNewAig, pAig, vCandidateMonotoneSignals,
				vDisj_nCk, combN_internal, combK_internal );


	//creation of implication logic
	vPODriver_nCk = Vec_PtrAlloc(lCombinationCount);
	for( longI = 0; longI < lCombinationCount; longI++ )
	{
		pObjLO_nCk = (Aig_Obj_t *)(Vec_PtrEntry( vLO_nCk, longI ));
		pObjDisj_nCk = (Aig_Obj_t *)(Vec_PtrEntry( vDisj_nCk, longI ));

		pObj = Aig_Or( pNewAig, Aig_Not(pObjDisj_nCk), pObjLO_nCk);
		Vec_PtrPush(vPODriver_nCk, pObj);
	}
	
	//Standard PO duplication
	Saig_ManForEachPo( pAig, pObj, i )
	{
		poCopied++;
		pObj->pData = Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) ); 
	}

	//nCk specific PO creation
	for( longI = 0; longI < lCombinationCount; longI++ )
	{
		Aig_ObjCreateCo( pNewAig, (Aig_Obj_t *)(Vec_PtrEntry( vPODriver_nCk, longI )) );
	}

	//Standard LI duplication
	Saig_ManForEachLi( pAig, pObj, i )
	{
		liCopied++;
		Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) );
	}

	//nCk specific LI creation
	for( longI = 0; longI < lCombinationCount; longI++ )
	{
		liCreated++;
		Aig_ObjCreateCo( pNewAig, (Aig_Obj_t *)(Vec_PtrEntry( vPODriver_nCk, longI )) );
	}

	//clean-up, book-keeping
	assert( liCopied + liCreated == loCopied + loCreated );	
	nRegCount = loCopied + loCreated;

	Aig_ManSetRegNum( pNewAig, nRegCount );
	Aig_ManCleanup( pNewAig );
	assert( Aig_ManCheck( pNewAig ) );
	
	//Vec_IntFree(vC_KNUTH);	
	return pNewAig;
}

Aig_Man_t *generateGeneralDisjunctiveTester( Abc_Ntk_t *pNtk, Aig_Man_t *pAig, int combK )
{
	//AIG creation related data structure
	Aig_Man_t *pNewAig;
	int piCopied = 0, loCopied = 0, loCreated = 0, liCopied = 0, liCreated = 0, poCopied = 0;
	//int i, iElem, nRegCount, hintSingalBeginningMarker, hintSingalEndMarker;
	int i, nRegCount;
	int combN_internal, combK_internal; //, targetPoIndex;
	long longI, lCombinationCount;
	//Aig_Obj_t *pObj, *pObjMonoCand, *pObjLO_nCk, *pObjDisj_nCk;
	Aig_Obj_t *pObj, *pObjLO_nCk, *pObjDisj_nCk;
	Vec_Ptr_t *vLO_nCk, *vPODriver_nCk, *vDisj_nCk;

	extern Vec_Int_t *findHintOutputs(Abc_Ntk_t *pNtk);
	
	//Knuth's Data Strcuture
	//Vec_Int_t *vC_KNUTH;
	//int i_KNUTH, j_KNUTH, combCounter_KNUTH = 0;

	//Collecting target HINT signals
	//vCandidateMonotoneSignals = findHintOutputs(pNtk);
	//if( vCandidateMonotoneSignals == NULL )
	//{
	//	printf("\nTraget Signal Set is Empty: Duplicating given AIG\n");
	//	combN_internal = 0;
	//}
	//else
	//{
		//Vec_IntForEachEntry( vCandidateMonotoneSignals, iElem, i )
		//	printf("Po[%d] = %s\n", iElem, Abc_ObjName( Abc_NtkPo(pNtk, iElem) ) );
	//	hintSingalBeginningMarker = Vec_IntEntry( vCandidateMonotoneSignals, 0 );
	//	hintSingalEndMarker = Vec_IntEntry( vCandidateMonotoneSignals, Vec_IntSize(vCandidateMonotoneSignals) - 1 );
	//	combN_internal = hintSingalEndMarker - hintSingalBeginningMarker + 1;
	//}

	combN_internal = Aig_ManRegNum(pAig);

	pNewAig = Aig_ManStart( Aig_ManObjNumMax(pAig) );
	pNewAig->pName = (char *)malloc( strlen( pAig->pName ) + strlen("_nCk") + 1 );
	sprintf(pNewAig->pName, "%s_%s", pAig->pName, "nCk");
    	pNewAig->pSpec = NULL;

	//Standard Mapping of Constant Nodes
	pObj = Aig_ManConst1( pAig );
    	pObj->pData = Aig_ManConst1( pNewAig );

	//Standard AIG PI duplication
	Saig_ManForEachPi( pAig, pObj, i )
	{
		piCopied++;
		pObj->pData = Aig_ObjCreateCi(pNewAig);
	}

	//Standard AIG LO duplication
	Saig_ManForEachLo( pAig, pObj, i )
    	{
		loCopied++;
		pObj->pData = Aig_ObjCreateCi(pNewAig);
    	}

	//nCk LO creation
	lCombinationCount = 0;
	for(combK_internal=1; combK_internal<=combK; combK_internal++)
		lCombinationCount += countCombination( combN_internal, combK_internal );
	assert( lCombinationCount > 0 );
	vLO_nCk = Vec_PtrAlloc(lCombinationCount);
	for( longI = 0; longI < lCombinationCount; longI++ )
	{
		loCreated++;
		pObj = Aig_ObjCreateCi(pNewAig);
		Vec_PtrPush( vLO_nCk, pObj );
	}

	//Standard Node duplication
	Aig_ManForEachNode( pAig, pObj, i )
	{
		pObj->pData = Aig_And( pNewAig, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
	}
	
	//nCk specific logic creation (i.e. nCk number of OR gates)
	vDisj_nCk = Vec_PtrAlloc(lCombinationCount);

	for( combK_internal=1; combK_internal<=combK; combK_internal++ )
	{
		generateCombinatorialStabilExhaust( pNewAig, pAig,
				vDisj_nCk, combN_internal, combK_internal );
	}


	//creation of implication logic
	vPODriver_nCk = Vec_PtrAlloc(lCombinationCount);
	for( longI = 0; longI < lCombinationCount; longI++ )
	{
		pObjLO_nCk = (Aig_Obj_t *)(Vec_PtrEntry( vLO_nCk, longI ));
		pObjDisj_nCk = (Aig_Obj_t *)(Vec_PtrEntry( vDisj_nCk, longI ));

		pObj = Aig_Or( pNewAig, Aig_Not(pObjDisj_nCk), pObjLO_nCk);
		Vec_PtrPush(vPODriver_nCk, pObj);
	}
	
	//Standard PO duplication
	Saig_ManForEachPo( pAig, pObj, i )
	{
		poCopied++;
		pObj->pData = Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) ); 
	}

	//nCk specific PO creation
	for( longI = 0; longI < lCombinationCount; longI++ )
	{
		Aig_ObjCreateCo( pNewAig, (Aig_Obj_t *)(Vec_PtrEntry( vPODriver_nCk, longI )) );
	}

	//Standard LI duplication
	Saig_ManForEachLi( pAig, pObj, i )
	{
		liCopied++;
		Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) );
	}

	//nCk specific LI creation
	for( longI = 0; longI < lCombinationCount; longI++ )
	{
		liCreated++;
		Aig_ObjCreateCo( pNewAig, (Aig_Obj_t *)(Vec_PtrEntry( vPODriver_nCk, longI )) );
	}

	//clean-up, book-keeping
	assert( liCopied + liCreated == loCopied + loCreated );	
	nRegCount = loCopied + loCreated;

	Aig_ManSetRegNum( pNewAig, nRegCount );
	Aig_ManCleanup( pNewAig );
	assert( Aig_ManCheck( pNewAig ) );
	
	Vec_PtrFree(vLO_nCk);
	Vec_PtrFree(vPODriver_nCk);
	Vec_PtrFree(vDisj_nCk);
	//Vec_IntFree(vC_KNUTH);	
	return pNewAig;
}

ABC_NAMESPACE_IMPL_END
