/**CFile****************************************************************

  FileName    [kliveness.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Liveness property checking.]

  Synopsis    	[Main implementation module of the algorithm k-Liveness	] 
	      	[invented by Koen Claessen, Niklas Sorensson. Implements]
	      	[the code for 'kcs'. 'kcs' pre-processes based on switch]
		[and then runs the (absorber circuit >> pdr) loop  ]

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

//#define WITHOUT_CONSTRAINTS

ABC_NAMESPACE_IMPL_START

/***************** Declaration of standard ABC related functions ********************/
extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
extern Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan );
extern Abc_Ntk_t * Abc_NtkMakeOnePo( Abc_Ntk_t * pNtk, int Output, int nRange );
extern void Aig_ManDumpBlif( Aig_Man_t * p, char * pFileName, Vec_Ptr_t * vPiNames, Vec_Ptr_t * vPoNames );
/***********************************************************************************/

/***************** Declaration of kLiveness related functions **********************/
//function defined in kLiveConstraints.c
extern Aig_Man_t *generateWorkingAig( Aig_Man_t *pAig, Abc_Ntk_t *pNtk, int *pIndex0Live );

//function defined in arenaViolation.c
extern Aig_Man_t *generateWorkingAigWithDSC( Aig_Man_t *pAig, Abc_Ntk_t *pNtk, int *pIndex0Live, Vec_Ptr_t *vMasterBarriers );

//function defined in disjunctiveMonotone.c
extern Vec_Ptr_t *findDisjunctiveMonotoneSignals( Abc_Ntk_t *pNtk );
extern Vec_Int_t *createSingletonIntVector( int i );
/***********************************************************************************/
extern Aig_Man_t *generateDisjunctiveTester( Abc_Ntk_t *pNtk, Aig_Man_t *pAig, int combN, int combK );
extern Aig_Man_t *generateGeneralDisjunctiveTester( Abc_Ntk_t *pNtk, Aig_Man_t *pAig, int combK );

//Definition of some macros pertaining to modes/switches
#define SIMPLE_kCS 0
#define kCS_WITH_SAFETY_INVARIANTS 1
#define kCS_WITH_DISCOVER_MONOTONE_SIGNALS 2
#define kCS_WITH_SAFETY_AND_DCS_INVARIANTS 3
#define kCS_WITH_SAFETY_AND_USER_GIVEN_DCS_INVARIANTS 4

//unused function
#if 0
Aig_Obj_t *readTargetPinSignal(Aig_Man_t *pAig, Abc_Ntk_t *pNtk)
{
	Aig_Obj_t *pObj;
	int i;

	Saig_ManForEachPo( pAig, pObj, i )
	{
		if( strstr( Abc_ObjName(Abc_NtkPo( pNtk, i )), "0Liveness_" ) != NULL  )
		{
			//return Aig_ObjFanin0(pObj);
			return Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj), Aig_ObjFaninC0(pObj));
		}
	}	

	return NULL;
}
#endif

Aig_Obj_t *readLiveSignal_0( Aig_Man_t *pAig, int liveIndex_0 )
{
	Aig_Obj_t *pObj;

	pObj = Aig_ManCo( pAig, liveIndex_0 );
	return Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj), Aig_ObjFaninC0(pObj));
}

Aig_Obj_t *readLiveSignal_k( Aig_Man_t *pAig, int liveIndex_k )
{
	Aig_Obj_t *pObj;

	pObj = Aig_ManCo( pAig, liveIndex_k );
	return Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj), Aig_ObjFaninC0(pObj));
}

//unused funtion
#if 0
Aig_Obj_t *readTargetPoutSignal(Aig_Man_t *pAig, Abc_Ntk_t *pNtk, int nonFirstIteration)
{
	Aig_Obj_t *pObj;
	int i;

	if( nonFirstIteration == 0 )
		return NULL;
	else
		Saig_ManForEachPo( pAig, pObj, i )
		{
			if( strstr( Abc_ObjName(Abc_NtkPo( pNtk, i )), "kLiveness_" ) != NULL  )
			{
				//return Aig_ObjFanin0(pObj);
				return Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj), Aig_ObjFaninC0(pObj));
			}
		}	

	return NULL;
}
#endif

//unused function
#if 0
void updateNewNetworkNameManager_kCS( Abc_Ntk_t *pNtk, Aig_Man_t *pAig, Vec_Ptr_t *vPiNames, 
			Vec_Ptr_t *vLoNames, Vec_Ptr_t *vPoNames, Vec_Ptr_t *vLiNames )
{
	Aig_Obj_t *pObj;
	Abc_Obj_t *pNode;
	int i, ntkObjId;

	pNtk->pManName = Nm_ManCreate( Abc_NtkCiNum( pNtk ) );

	if( vPiNames )
	{
		Saig_ManForEachPi( pAig, pObj, i )
		{
			ntkObjId = Abc_NtkCi( pNtk, i )->Id;
			Nm_ManStoreIdName( pNtk->pManName, ntkObjId, Aig_ObjType(pObj), (char *)Vec_PtrEntry(vPiNames, i), NULL );
		}
	}
	if( vLoNames )
	{
		Saig_ManForEachLo( pAig, pObj, i )
		{
			ntkObjId = Abc_NtkCi( pNtk, Saig_ManPiNum( pAig ) + i )->Id;
			Nm_ManStoreIdName( pNtk->pManName, ntkObjId, Aig_ObjType(pObj), (char *)Vec_PtrEntry(vLoNames, i), NULL );
		}
	}

	if( vPoNames )
	{
		Saig_ManForEachPo( pAig, pObj, i )
		{
			ntkObjId = Abc_NtkCo( pNtk, i )->Id;
			Nm_ManStoreIdName( pNtk->pManName, ntkObjId, Aig_ObjType(pObj), (char *)Vec_PtrEntry(vPoNames, i), NULL );	
		}
	}

	if( vLiNames )
	{
		Saig_ManForEachLi( pAig, pObj, i )
		{
			ntkObjId = Abc_NtkCo( pNtk, Saig_ManPoNum( pAig ) + i )->Id;
			Nm_ManStoreIdName( pNtk->pManName, ntkObjId, Aig_ObjType(pObj), (char *)Vec_PtrEntry(vLiNames, i), NULL );
		}
	}

    // assign latch input names
	Abc_NtkForEachLatch(pNtk, pNode, i)
        if ( Nm_ManFindNameById(pNtk->pManName, Abc_ObjFanin0(pNode)->Id) == NULL )
            Abc_ObjAssignName( Abc_ObjFanin0(pNode), Abc_ObjName(Abc_ObjFanin0(pNode)), NULL );
}
#endif

Aig_Man_t *introduceAbsorberLogic( Aig_Man_t *pAig, int *pLiveIndex_0, int *pLiveIndex_k, int nonFirstIteration )
{
	Aig_Man_t *pNewAig;
	Aig_Obj_t *pObj, *pObjAbsorberLo, *pPInNewArg, *pPOutNewArg;
	Aig_Obj_t *pPIn = NULL, *pPOut = NULL, *pPOutCo = NULL;
	Aig_Obj_t *pFirstAbsorberOr, *pSecondAbsorberOr;
	int i;
	int piCopied = 0, loCreated = 0, loCopied = 0, liCreated = 0, liCopied = 0; 
	int nRegCount;

	assert(*pLiveIndex_0 != -1);
	if(nonFirstIteration == 0)
		assert( *pLiveIndex_k == -1 );
	else
		assert( *pLiveIndex_k != -1  );

	//****************************************************************
	// Step1: create the new manager
	// Note: The new manager is created with "2 * Aig_ManObjNumMax(p)"
	// nodes, but this selection is arbitrary - need to be justified
	//****************************************************************
	pNewAig = Aig_ManStart( Aig_ManObjNumMax(pAig) );
	pNewAig->pName = (char *)malloc( strlen( pAig->pName ) + strlen("_kCS") + 1 );
	sprintf(pNewAig->pName, "%s_%s", pAig->pName, "kCS");
    	pNewAig->pSpec = NULL;

	//****************************************************************
	// reading the signal pIn, and pOut
	//****************************************************************

	pPIn = readLiveSignal_0( pAig, *pLiveIndex_0 );
	if( *pLiveIndex_k == -1 )
		pPOut = NULL;
	else
		pPOut = readLiveSignal_k( pAig, *pLiveIndex_k );
    
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
	// Step 6: create "D" register output for the ABSORBER logic
	//****************************************************************
	loCreated++;
	pObjAbsorberLo = Aig_ObjCreateCi( pNewAig );

	nRegCount = loCreated + loCopied;

	//********************************************************************
	// Step 7: create internal nodes
	//********************************************************************
    	Aig_ManForEachNode( pAig, pObj, i )
	{
		pObj->pData = Aig_And( pNewAig, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
	}

	//****************************************************************
	// Step 8: create the two OR gates of the OBSERVER logic
	//****************************************************************
	if( nonFirstIteration == 0 )
	{
		assert(pPIn);
		
		pPInNewArg = !Aig_IsComplement(pPIn)? 
				(Aig_Obj_t *)((Aig_Regular(pPIn))->pData) : 
				Aig_Not((Aig_Obj_t *)((Aig_Regular(pPIn))->pData));

		pFirstAbsorberOr = Aig_Or( pNewAig, Aig_Not(pPInNewArg), pObjAbsorberLo );
		pSecondAbsorberOr = Aig_Or( pNewAig, pPInNewArg, Aig_Not(pObjAbsorberLo) );
	}
	else
	{
		assert( pPOut );

		pPInNewArg = !Aig_IsComplement(pPIn)? 
				(Aig_Obj_t *)((Aig_Regular(pPIn))->pData) : 
				Aig_Not((Aig_Obj_t *)((Aig_Regular(pPIn))->pData));
		pPOutNewArg = !Aig_IsComplement(pPOut)? 
				(Aig_Obj_t *)((Aig_Regular(pPOut))->pData) : 
				Aig_Not((Aig_Obj_t *)((Aig_Regular(pPOut))->pData));
		
		pFirstAbsorberOr = Aig_Or( pNewAig, Aig_Not(pPOutNewArg), pObjAbsorberLo );
		pSecondAbsorberOr = Aig_Or( pNewAig, pPInNewArg, Aig_Not(pObjAbsorberLo) );
	}	
	
	//********************************************************************
	// Step 9: create primary outputs 
	//********************************************************************
	Saig_ManForEachPo( pAig, pObj, i )
	{
		pObj->pData = Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) ); 
		if( i == *pLiveIndex_k )
			pPOutCo = (Aig_Obj_t *)(pObj->pData);
	}

	//create new po
	if( nonFirstIteration == 0 )
	{
		assert(pPOutCo == NULL);
		pPOutCo = Aig_ObjCreateCo( pNewAig, pSecondAbsorberOr ); 	

		*pLiveIndex_k = i;
	}	
	else
	{
		assert( pPOutCo != NULL );
		//pPOutCo = Aig_ObjCreateCo( pNewAig, pSecondAbsorberOr ); 	
		//*pLiveIndex_k = Saig_ManPoNum(pAig);

		Aig_ObjPatchFanin0( pNewAig, pPOutCo, pSecondAbsorberOr );
	}

	Saig_ManForEachLi( pAig, pObj, i )
	{
		liCopied++;
		Aig_ObjCreateCo( pNewAig, Aig_ObjChild0Copy(pObj) );
	}

	//create new li
	liCreated++;
	Aig_ObjCreateCo( pNewAig, pFirstAbsorberOr );

	Aig_ManSetRegNum( pNewAig, nRegCount );
	Aig_ManCleanup( pNewAig );
	
	assert( Aig_ManCheck( pNewAig ) );

	assert( *pLiveIndex_k != - 1);
	return pNewAig;
}

void modifyAigToApplySafetyInvar(Aig_Man_t *pAig, int csTarget, int safetyInvarPO)
{
	Aig_Obj_t *pObjPOSafetyInvar, *pObjSafetyInvar;
	Aig_Obj_t *pObjPOCSTarget, *pObjCSTarget;
	Aig_Obj_t *pObjCSTargetNew;

	pObjPOSafetyInvar = Aig_ManCo( pAig, safetyInvarPO );
	pObjSafetyInvar =  Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObjPOSafetyInvar), Aig_ObjFaninC0(pObjPOSafetyInvar));	
	pObjPOCSTarget = Aig_ManCo( pAig, csTarget );
	pObjCSTarget = Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObjPOCSTarget), Aig_ObjFaninC0(pObjPOCSTarget));	

	pObjCSTargetNew = Aig_And( pAig, pObjSafetyInvar, pObjCSTarget );
	Aig_ObjPatchFanin0( pAig, pObjPOCSTarget, pObjCSTargetNew );
}

int flipConePdr( Aig_Man_t *pAig, int directive, int targetCSPropertyIndex, int safetyInvariantPOIndex, int absorberCount )
{
	int RetValue, i;
	Aig_Obj_t *pObjTargetPo;
	Aig_Man_t *pAigDupl;
	Pdr_Par_t Pars, * pPars = &Pars;
	Abc_Cex_t * pCex = NULL;

	char *fileName;
	
	fileName = (char *)malloc(sizeof(char) * 50);
	sprintf(fileName, "%s_%d.%s", "kLive", absorberCount, "blif" );

	if( directive == kCS_WITH_SAFETY_INVARIANTS || directive == kCS_WITH_SAFETY_AND_DCS_INVARIANTS || directive == kCS_WITH_SAFETY_AND_USER_GIVEN_DCS_INVARIANTS )
	{
		assert( safetyInvariantPOIndex != -1 );
		modifyAigToApplySafetyInvar(pAig, targetCSPropertyIndex, safetyInvariantPOIndex);
	}

	pAigDupl = pAig;
	pAig = Aig_ManDupSimple( pAigDupl );

	for( i=0; i<Saig_ManPoNum(pAig); i++ )
	{
		pObjTargetPo = Aig_ManCo( pAig, i );
		Aig_ObjChild0Flip( pObjTargetPo );
	}

	Pdr_ManSetDefaultParams( pPars );
	pPars->fVerbose = 1;
	pPars->fNotVerbose = 1;
	pPars->fSolveAll = 1;
	pAig->vSeqModelVec = NULL;

	Aig_ManCleanup( pAig );
	assert( Aig_ManCheck( pAig ) );

	Pdr_ManSolve( pAig, pPars );	

	if( pAig->vSeqModelVec )
	{
		pCex = (Abc_Cex_t *)Vec_PtrEntry( pAig->vSeqModelVec, targetCSPropertyIndex );
		if( pCex == NULL )
		{
			RetValue = 1;
		}
		else
			RetValue = 0;
	}
	else
	{
		RetValue = -1;
		exit(0);
	}

	free(fileName);

	for( i=0; i<Saig_ManPoNum(pAig); i++ )
	{
		pObjTargetPo = Aig_ManCo( pAig, i );
		Aig_ObjChild0Flip( pObjTargetPo );
	}
	
	Aig_ManStop( pAig );
	return RetValue;
}

//unused function
#if 0
int read0LiveIndex( Abc_Ntk_t *pNtk )
{
	Abc_Obj_t *pObj;
	int i;

	Abc_NtkForEachPo( pNtk, pObj, i )
	{
		if( strstr( Abc_ObjName( pObj ), "0Liveness_" ) != NULL )
			return i;
	}		

	return -1;
}
#endif

int collectSafetyInvariantPOIndex(Abc_Ntk_t *pNtk)
{
	Abc_Obj_t *pObj;
	int i;

	Abc_NtkForEachPo( pNtk, pObj, i )
	{
		if( strstr( Abc_ObjName( pObj ), "csSafetyInvar_" ) != NULL )
			return i;
	}		

	return -1;
}

Vec_Ptr_t *collectUserGivenDisjunctiveMonotoneSignals( Abc_Ntk_t *pNtk )
{
	Abc_Obj_t *pObj;
	int i;
	Vec_Ptr_t *monotoneVector;
	Vec_Int_t *newIntVector;

	monotoneVector = Vec_PtrAlloc(0);	
	Abc_NtkForEachPo( pNtk, pObj, i )
	{
		if( strstr( Abc_ObjName( pObj ), "csLevel1Stabil_" ) != NULL )
		{
			newIntVector = createSingletonIntVector(i);
			Vec_PtrPush(monotoneVector, newIntVector);
		}
	}

	if( Vec_PtrSize(monotoneVector) > 0 )
		return monotoneVector;
	else
		return NULL;

}

void deallocateMasterBarrierDisjunctInt(Vec_Ptr_t *vMasterBarrierDisjunctsArg)
{
	Vec_Int_t *vInt;
	int i;

	if(vMasterBarrierDisjunctsArg)
	{
		Vec_PtrForEachEntry(Vec_Int_t *, vMasterBarrierDisjunctsArg, vInt, i)
		{	
			Vec_IntFree(vInt);
		}
		Vec_PtrFree(vMasterBarrierDisjunctsArg);
	}
}

void deallocateMasterBarrierDisjunctVecPtrVecInt(Vec_Ptr_t *vMasterBarrierDisjunctsArg)
{
	Vec_Int_t *vInt;
	Vec_Ptr_t *vPtr;
	int i, j, k, iElem;

	if(vMasterBarrierDisjunctsArg)
	{
		Vec_PtrForEachEntry(Vec_Ptr_t *, vMasterBarrierDisjunctsArg, vPtr, i)
		{	
			assert(vPtr);
			Vec_PtrForEachEntry( Vec_Int_t *, vPtr, vInt, j )
			{
				//Vec_IntFree(vInt);
				Vec_IntForEachEntry( vInt, iElem, k )
					printf("%d - ", iElem);
				//printf("Chung Chang j = %d\n", j);
			}
			Vec_PtrFree(vPtr);
		}
		Vec_PtrFree(vMasterBarrierDisjunctsArg);
	}
}

Vec_Ptr_t *getVecOfVecFairness(FILE *fp)
{
	Vec_Ptr_t *masterVector = Vec_PtrAlloc(0);
	//Vec_Ptr_t *currSignalVector;
	char stringBuffer[100];
	//int i;
	
	while(fgets(stringBuffer, 50, fp))
	{
		if(strstr(stringBuffer, ":"))
		{

		}
		else
		{
				
		}
	}

	return masterVector;
}


int Abc_CommandCS_kLiveness( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    	Abc_Ntk_t * pNtk, * pNtkTemp;
	Aig_Man_t * pAig, *pAigCS, *pAigCSNew;
	int absorberCount;
	int absorberLimit = 500;
	int RetValue;
	int liveIndex_0 = -1, liveIndex_k = -1;
	int fVerbose = 1;
	int directive = -1;
	int c;
	int safetyInvariantPO = -1;
	abctime beginTime, endTime;
	double time_spent;
	Vec_Ptr_t *vMasterBarrierDisjuncts = NULL;
	Aig_Man_t *pWorkingAig;
	//FILE *fp;

	pNtk = Abc_FrameReadNtk(pAbc);

	//fp = fopen("propFile.txt", "r");
	//if( fp )
	//	getVecOfVecFairness(fp);
	//exit(0);

	/*************************************************
	Extraction of Command Line Argument	
	*************************************************/
	if( argc == 1 )
	{
		assert( directive == -1 );
		directive = SIMPLE_kCS;
	}
	else
	{
		Extra_UtilGetoptReset();
		while ( ( c = Extra_UtilGetopt( argc, argv, "cmCgh" ) ) != EOF )
		{
			switch( c )
			{
			case 'c':
				directive = kCS_WITH_SAFETY_INVARIANTS; 
				break;
			case 'm':
				directive = kCS_WITH_DISCOVER_MONOTONE_SIGNALS; 
				break;
			case 'C':
				directive = kCS_WITH_SAFETY_AND_DCS_INVARIANTS;
				break;
			case 'g':
				directive = kCS_WITH_SAFETY_AND_USER_GIVEN_DCS_INVARIANTS;
				break;
			case 'h':
				goto usage;
				break;
			default:
				goto usage;
			}
		}
	}
	/*************************************************
	Extraction of Command Line Argument Ends	
	*************************************************/

	if( !Abc_NtkIsStrash( pNtk ) )
	{
		printf("The input network was not strashed, strashing....\n");
		pNtkTemp = Abc_NtkStrash( pNtk, 0, 0, 0 );
		pAig = Abc_NtkToDar( pNtkTemp, 0, 1 );
	}
	else
	{
		pAig = Abc_NtkToDar( pNtk, 0, 1 );
		pNtkTemp = pNtk;
	}

	if( directive == kCS_WITH_SAFETY_INVARIANTS )
	{
		safetyInvariantPO = collectSafetyInvariantPOIndex(pNtkTemp);			
		assert( safetyInvariantPO != -1 );
	}

	if(directive == kCS_WITH_DISCOVER_MONOTONE_SIGNALS)
	{
		beginTime = Abc_Clock();
		vMasterBarrierDisjuncts = findDisjunctiveMonotoneSignals( pNtk );
		endTime = Abc_Clock();
		time_spent = (double)(endTime - beginTime)/CLOCKS_PER_SEC;
		printf("pre-processing time = %f\n",time_spent); 
		return 0;
	}

	if(directive == kCS_WITH_SAFETY_AND_DCS_INVARIANTS)
	{
		safetyInvariantPO = collectSafetyInvariantPOIndex(pNtkTemp);			
		assert( safetyInvariantPO != -1 );

		beginTime = Abc_Clock();
		vMasterBarrierDisjuncts = findDisjunctiveMonotoneSignals( pNtk );
		endTime = Abc_Clock();
		time_spent = (double)(endTime - beginTime)/CLOCKS_PER_SEC;
		printf("pre-processing time = %f\n",time_spent); 

		assert( vMasterBarrierDisjuncts != NULL );
		assert( Vec_PtrSize(vMasterBarrierDisjuncts) > 0 );
	}

	if(directive == kCS_WITH_SAFETY_AND_USER_GIVEN_DCS_INVARIANTS)
	{
		safetyInvariantPO = collectSafetyInvariantPOIndex(pNtkTemp);			
		assert( safetyInvariantPO != -1 );

		beginTime = Abc_Clock();
		vMasterBarrierDisjuncts = collectUserGivenDisjunctiveMonotoneSignals( pNtk );
		endTime = Abc_Clock();
		time_spent = (double)(endTime - beginTime)/CLOCKS_PER_SEC;
		printf("pre-processing time = %f\n",time_spent); 

		assert( vMasterBarrierDisjuncts != NULL );
		assert( Vec_PtrSize(vMasterBarrierDisjuncts) > 0 );
	}

	if(directive == kCS_WITH_SAFETY_AND_DCS_INVARIANTS || directive == kCS_WITH_SAFETY_AND_USER_GIVEN_DCS_INVARIANTS)
	{
		assert( vMasterBarrierDisjuncts != NULL );
		pWorkingAig = generateWorkingAigWithDSC( pAig, pNtk, &liveIndex_0, vMasterBarrierDisjuncts );
		pAigCS = introduceAbsorberLogic(pWorkingAig, &liveIndex_0, &liveIndex_k, 0);
	}
	else
	{
		pWorkingAig = generateWorkingAig( pAig, pNtk, &liveIndex_0 );
		pAigCS = introduceAbsorberLogic(pWorkingAig, &liveIndex_0, &liveIndex_k, 0);
	}

	Aig_ManStop(pWorkingAig);

	for( absorberCount=1; absorberCount<absorberLimit; absorberCount++ )
	{
		//printf( "\nindex of the liveness output = %d\n", liveIndex_k );
		RetValue = flipConePdr( pAigCS, directive, liveIndex_k, safetyInvariantPO, absorberCount );

		if ( RetValue == 1 )
		{
        		Abc_Print( 1, "k = %d, Property proved\n", absorberCount );
			break;
		}
    		else if ( RetValue == 0 )
		{
			if( fVerbose )
			{
				Abc_Print( 1, "k = %d, Property DISPROVED\n", absorberCount );
			}
		}
    		else if ( RetValue == -1 )
		{
        		Abc_Print( 1, "Property UNDECIDED with k = %d.\n", absorberCount );
		}
    		else
        		assert( 0 );

		pAigCSNew = introduceAbsorberLogic(pAigCS, &liveIndex_0, &liveIndex_k, absorberCount);
		Aig_ManStop(pAigCS);
		pAigCS = pAigCSNew;
	}

	Aig_ManStop(pAigCS);
	Aig_ManStop(pAig);

	if(directive == kCS_WITH_SAFETY_AND_USER_GIVEN_DCS_INVARIANTS)
	{
		deallocateMasterBarrierDisjunctInt(vMasterBarrierDisjuncts);
	}
	else
	{
		//if(vMasterBarrierDisjuncts)
		//	Vec_PtrFree(vMasterBarrierDisjuncts);
		//deallocateMasterBarrierDisjunctVecPtrVecInt(vMasterBarrierDisjuncts);
		deallocateMasterBarrierDisjunctInt(vMasterBarrierDisjuncts);
	}
	return 0;

	usage:
		fprintf( stdout, "usage: kcs [-cmgCh]\n" );
    		fprintf( stdout, "\timplements Claessen-Sorensson's k-Liveness algorithm\n" );
		fprintf( stdout, "\t-c : verification with constraints, looks for POs prefixed with csSafetyInvar_\n");
		fprintf( stdout, "\t-m : discovers monotone signals\n");
    		fprintf( stdout, "\t-g : verification with user-supplied barriers, looks for POs prefixed with csLevel1Stabil_\n");
		fprintf( stdout, "\t-C : verification with discovered monotone signals\n");
		fprintf( stdout, "\t-h : print usage\n");
    		return 1;

}

int Abc_CommandNChooseK( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	Abc_Ntk_t * pNtk, * pNtkTemp, *pNtkCombStabil;
	Aig_Man_t * pAig, *pAigCombStabil;
	int directive = -1;
	int c;
	int parameterizedCombK;

	pNtk = Abc_FrameReadNtk(pAbc);


	/*************************************************
	Extraction of Command Line Argument	
	*************************************************/
	if( argc == 1 )
	{
		assert( directive == -1 );
		directive = SIMPLE_kCS;
	}
	else
	{
		Extra_UtilGetoptReset();
		while ( ( c = Extra_UtilGetopt( argc, argv, "cmCgh" ) ) != EOF )
		{
			switch( c )
			{
			case 'c':
				directive = kCS_WITH_SAFETY_INVARIANTS; 
				break;
			case 'm':
				directive = kCS_WITH_DISCOVER_MONOTONE_SIGNALS; 
				break;
			case 'C':
				directive = kCS_WITH_SAFETY_AND_DCS_INVARIANTS;
				break;
			case 'g':
				directive = kCS_WITH_SAFETY_AND_USER_GIVEN_DCS_INVARIANTS;
				break;
			case 'h':
				goto usage;
				break;
			default:
				goto usage;
			}
		}
	}
	/*************************************************
	Extraction of Command Line Argument Ends	
	*************************************************/

	if( !Abc_NtkIsStrash( pNtk ) )
	{
		printf("The input network was not strashed, strashing....\n");
		pNtkTemp = Abc_NtkStrash( pNtk, 0, 0, 0 );
		pAig = Abc_NtkToDar( pNtkTemp, 0, 1 );
	}
	else
	{
		pAig = Abc_NtkToDar( pNtk, 0, 1 );
		pNtkTemp = pNtk;
	}

/**********************Code for generation of nCk outputs**/
	//combCount = countCombination(1000, 3);
	//pAigCombStabil = generateDisjunctiveTester( pNtk, pAig, 7, 2 );
	printf("Enter parameterizedCombK = " );
	if( scanf("%d", &parameterizedCombK) != 1 )
	{
		printf("\nFailed to read integer!\n");
		return 0;
	}
	printf("\n");

	pAigCombStabil = generateGeneralDisjunctiveTester( pNtk, pAig, parameterizedCombK );
	Aig_ManPrintStats(pAigCombStabil);
	pNtkCombStabil = Abc_NtkFromAigPhase( pAigCombStabil );
	pNtkCombStabil->pName = Abc_UtilStrsav( pAigCombStabil->pName );
	if ( !Abc_NtkCheck( pNtkCombStabil ) )
	        fprintf( stdout, "Abc_NtkCreateCone(): Network check has failed.\n" );
	Abc_FrameSetCurrentNetwork( pAbc, pNtkCombStabil );

	Aig_ManStop( pAigCombStabil );
	Aig_ManStop( pAig );

	return 1;
	//printf("\ncombCount = %d\n", combCount);
	//exit(0);
/**********************************************************/

	usage:
		fprintf( stdout, "usage: nck [-cmgCh]\n" );
    		fprintf( stdout, "\tgenerates combinatorial signals for stabilization\n" );
		fprintf( stdout, "\t-h : print usage\n");
    		return 1;

}


ABC_NAMESPACE_IMPL_END
