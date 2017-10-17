/**CFile****************************************************************

  FileName    [abcAuto.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Computation of autosymmetries.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcAuto.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

static void Abc_NtkAutoPrintAll( DdManager * dd, int nInputs, DdNode * pbOutputs[], int nOutputs, char * pInputNames[], char * pOutputNames[], int fNaive );
static void Abc_NtkAutoPrintOne( DdManager * dd, int nInputs, DdNode * pbOutputs[], int Output, char * pInputNames[], char * pOutputNames[], int fNaive );
 
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAutoPrint( Abc_Ntk_t * pNtk, int Output, int fNaive, int fVerbose )
{
	DdManager * dd;         // the BDD manager used to hold shared BDDs
	DdNode ** pbGlobal;     // temporary storage for global BDDs
	char ** pInputNames;    // pointers to the CI names
	char ** pOutputNames;   // pointers to the CO names
	int nOutputs, nInputs, i;
    Vec_Ptr_t * vFuncsGlob;
    Abc_Obj_t * pObj;

    // compute the global BDDs
    if ( Abc_NtkBuildGlobalBdds(pNtk, 10000000, 1, 1, fVerbose) == NULL )
        return;

    // get information about the network
    nInputs  = Abc_NtkCiNum(pNtk);
    nOutputs = Abc_NtkCoNum(pNtk);
//    dd       = pNtk->pManGlob;
    dd = (DdManager *)Abc_NtkGlobalBddMan( pNtk );

    // complement the global functions
    vFuncsGlob = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_PtrPush( vFuncsGlob, Abc_ObjGlobalBdd(pObj) );
    pbGlobal = (DdNode **)Vec_PtrArray( vFuncsGlob );

    // get the network names
    pInputNames = Abc_NtkCollectCioNames( pNtk, 0 );
    pOutputNames = Abc_NtkCollectCioNames( pNtk, 1 );

    // print the size of the BDDs
    if ( fVerbose )
        printf( "Shared BDD size = %6d nodes.\n", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );

	// allocate additional variables
	for ( i = 0; i < nInputs; i++ )
		Cudd_bddNewVar( dd );
	assert( Cudd_ReadSize(dd) == 2 * nInputs );

	// create ZDD variables in the manager
	Cudd_zddVarsFromBddVars( dd, 2 );

	// perform the analysis of the primary output functions for auto-symmetry
	if ( Output == -1 )
		Abc_NtkAutoPrintAll( dd, nInputs, pbGlobal, nOutputs, pInputNames, pOutputNames, fNaive );
	else
		Abc_NtkAutoPrintOne( dd, nInputs, pbGlobal, Output, pInputNames, pOutputNames, fNaive );

	// deref the PO functions
//    Abc_NtkFreeGlobalBdds( pNtk );
	// stop the global BDD manager
//    Extra_StopManager( pNtk->pManGlob );
//    pNtk->pManGlob = NULL;
    Abc_NtkFreeGlobalBdds( pNtk, 1 );
    ABC_FREE( pInputNames );
    ABC_FREE( pOutputNames );
    Vec_PtrFree( vFuncsGlob );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAutoPrintAll( DdManager * dd, int nInputs, DdNode * pbOutputs[], int nOutputs, char * pInputNames[], char * pOutputNames[], int fNaive )
{
	DdNode * bSpace1, * bSpace2, * bCanVars, * bReduced, * zEquations;
	double nMints; 
	int nSupp, SigCounter, o;

	int nAutos;
	int nAutoSyms;
	int nAutoSymsMax;
	int nAutoSymsMaxSupp;
	int nAutoSymOuts;
	int nSuppSizeMax;
	abctime clk;
	
	nAutoSymOuts = 0;
	nAutoSyms    = 0;
	nAutoSymsMax = 0;
	nAutoSymsMaxSupp = 0;
	nSuppSizeMax = 0;
	clk = Abc_Clock();

	SigCounter = 0;
	for ( o = 0; o < nOutputs; o++ )
	{
//		bSpace1  = Extra_bddSpaceFromFunctionFast( dd, pbOutputs[o] );           Cudd_Ref( bSpace1 );
		bSpace1  = Extra_bddSpaceFromFunction( dd, pbOutputs[o], pbOutputs[o] ); Cudd_Ref( bSpace1 );
		bCanVars = Extra_bddSpaceCanonVars( dd, bSpace1 );                       Cudd_Ref( bCanVars );
		bReduced = Extra_bddSpaceReduce( dd, pbOutputs[o], bCanVars );           Cudd_Ref( bReduced );
		zEquations = Extra_bddSpaceEquations( dd, bSpace1 );                     Cudd_Ref( zEquations );

		nSupp  = Cudd_SupportSize( dd, bSpace1 );
		nMints = Cudd_CountMinterm( dd, bSpace1, nSupp );
		nAutos = Extra_Base2LogDouble(nMints);
		printf( "Output #%3d: Inputs = %2d. AutoK = %2d.\n", o, nSupp, nAutos );

		if ( nAutos > 0 )
		{
			nAutoSymOuts++;
			nAutoSyms += nAutos;
			if ( nAutoSymsMax < nAutos )
			{
				nAutoSymsMax = nAutos;
				nAutoSymsMaxSupp = nSupp;
			}
		}
		if ( nSuppSizeMax < nSupp )
			nSuppSizeMax = nSupp;


//ABC_PRB( dd, bCanVars );
//ABC_PRB( dd, bReduced );
//Cudd_PrintMinterm( dd, bReduced );
//printf( "The equations are:\n" );
//Cudd_zddPrintCover( dd, zEquations );
//printf( "\n" );
//fflush( stdout );

		bSpace2 = Extra_bddSpaceFromMatrixPos( dd, zEquations );   Cudd_Ref( bSpace2 );
//ABC_PRB( dd, bSpace1 );
//ABC_PRB( dd, bSpace2 );
		if ( bSpace1 != bSpace2 )
			printf( "Spaces are NOT EQUAL!\n" );
//		else
//			printf( "Spaces are equal.\n" );
	
		Cudd_RecursiveDeref( dd, bSpace1 );
		Cudd_RecursiveDeref( dd, bSpace2 );
		Cudd_RecursiveDeref( dd, bCanVars );
		Cudd_RecursiveDeref( dd, bReduced );
		Cudd_RecursiveDerefZdd( dd, zEquations );
	}

	printf( "The cumulative statistics for all outputs:\n" );
	printf( "Ins=%3d ",      nInputs );
	printf( "InMax=%3d   ",  nSuppSizeMax );
	printf( "Outs=%3d ",     nOutputs );
	printf( "Auto=%3d   ",   nAutoSymOuts );
	printf( "SumK=%3d ",     nAutoSyms );
	printf( "KMax=%2d ",     nAutoSymsMax );
	printf( "Supp=%3d   ",   nAutoSymsMaxSupp );
	printf( "Time=%4.2f ", (float)(Abc_Clock() - clk)/(float)(CLOCKS_PER_SEC) );
	printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkAutoPrintOne( DdManager * dd, int nInputs, DdNode * pbOutputs[], int Output, char * pInputNames[], char * pOutputNames[], int fNaive )
{
	DdNode * bSpace1, * bCanVars, * bReduced, * zEquations;
	double nMints; 
	int nSupp, SigCounter;
	int nAutos;

	SigCounter = 0;
	bSpace1  = Extra_bddSpaceFromFunctionFast( dd, pbOutputs[Output] );                Cudd_Ref( bSpace1 );
//	bSpace1  = Extra_bddSpaceFromFunction( dd, pbOutputs[Output], pbOutputs[Output] ); Cudd_Ref( bSpace1 );
	bCanVars = Extra_bddSpaceCanonVars( dd, bSpace1 );                                 Cudd_Ref( bCanVars );
	bReduced = Extra_bddSpaceReduce( dd, pbOutputs[Output], bCanVars );                Cudd_Ref( bReduced );
	zEquations = Extra_bddSpaceEquations( dd, bSpace1 );                               Cudd_Ref( zEquations );

	nSupp  = Cudd_SupportSize( dd, bSpace1 );
	nMints = Cudd_CountMinterm( dd, bSpace1, nSupp );
	nAutos = Extra_Base2LogDouble(nMints);
	printf( "Output #%3d: Inputs = %2d. AutoK = %2d.\n", Output, nSupp, nAutos );

	Cudd_RecursiveDeref( dd, bSpace1 );
	Cudd_RecursiveDeref( dd, bCanVars );
	Cudd_RecursiveDeref( dd, bReduced );
	Cudd_RecursiveDerefZdd( dd, zEquations );
}

#else

void Abc_NtkAutoPrint( Abc_Ntk_t * pNtk, int Output, int fNaive, int fVerbose ) {}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

