/**CFile****************************************************************

  FileName    [abcUnate.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcUnate.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

static void Abc_NtkPrintUnateBdd( Abc_Ntk_t * pNtk, int fUseNaive, int fVerbose );
static void Abc_NtkPrintUnateSat( Abc_Ntk_t * pNtk, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Detects unate variables of the multi-output function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintUnate( Abc_Ntk_t * pNtk, int fUseBdds, int fUseNaive, int fVerbose )
{
    if ( fUseBdds || fUseNaive )
        Abc_NtkPrintUnateBdd( pNtk, fUseNaive, fVerbose );
    else
        Abc_NtkPrintUnateSat( pNtk, fVerbose );
}

/**Function*************************************************************

  Synopsis    [Detects unate variables using BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintUnateBdd( Abc_Ntk_t * pNtk, int fUseNaive, int fVerbose )
{
    Abc_Obj_t * pNode;
    Extra_UnateInfo_t * p;
    DdManager * dd;         // the BDD manager used to hold shared BDDs
//    DdNode ** pbGlobal;     // temporary storage for global BDDs
    int TotalSupps = 0;
    int TotalUnate = 0;
    int i;
    abctime clk = Abc_Clock();
    abctime clkBdd, clkUnate;

    // compute the global BDDs
    dd = (DdManager *)Abc_NtkBuildGlobalBdds(pNtk, 10000000, 1, 1, fVerbose);
    if ( dd == NULL )
        return;
clkBdd = Abc_Clock() - clk;

    // get information about the network
//    dd       = pNtk->pManGlob;
//    dd       = (DdManager *)Abc_NtkGlobalBddMan( pNtk );
//    pbGlobal = (DdNode **)Vec_PtrArray( pNtk->vFuncsGlob );

    // print the size of the BDDs
    printf( "Shared BDD size = %6d nodes.\n", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );

    // perform naive BDD-based computation
    if ( fUseNaive )
    {
        Abc_NtkForEachCo( pNtk, pNode, i )
        {
//            p = Extra_UnateComputeSlow( dd, pbGlobal[i] );
            p = Extra_UnateComputeSlow( dd, (DdNode *)Abc_ObjGlobalBdd(pNode) );
            if ( fVerbose )
            {
                printf( "Out%4d : ", i );
                Extra_UnateInfoPrint( p );
            }
            TotalSupps += p->nVars;
            TotalUnate += p->nUnate;
            Extra_UnateInfoDissolve( p );
        }
    }
    // perform smart BDD-based computation
    else 
    {
        // create ZDD variables in the manager
        Cudd_zddVarsFromBddVars( dd, 2 );
        Abc_NtkForEachCo( pNtk, pNode, i )
        {
//            p = Extra_UnateComputeFast( dd, pbGlobal[i] );
            p = Extra_UnateComputeFast( dd, (DdNode *)Abc_ObjGlobalBdd(pNode) );
            if ( fVerbose )
            {
                printf( "Out%4d : ", i );
                Extra_UnateInfoPrint( p );
            }
            TotalSupps += p->nVars;
            TotalUnate += p->nUnate;
            Extra_UnateInfoDissolve( p );
        }
    }
clkUnate = Abc_Clock() - clk - clkBdd;

    // print stats
    printf( "Ins/Outs = %4d/%4d.  Total supp = %5d.  Total unate = %5d.\n",
        Abc_NtkCiNum(pNtk), Abc_NtkCoNum(pNtk), TotalSupps, TotalUnate );
    ABC_PRT( "Glob BDDs", clkBdd );
    ABC_PRT( "Unateness", clkUnate );
    ABC_PRT( "Total    ", Abc_Clock() - clk );

    // deref the PO functions
//    Abc_NtkFreeGlobalBdds( pNtk );
    // stop the global BDD manager
//    Extra_StopManager( pNtk->pManGlob );
//    pNtk->pManGlob = NULL;
    Abc_NtkFreeGlobalBdds( pNtk, 1 );
}

/**Function*************************************************************

  Synopsis    [Detects unate variables using SAT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkPrintUnateSat( Abc_Ntk_t * pNtk, int fVerbose )
{
}

#else

void Abc_NtkPrintUnate( Abc_Ntk_t * pNtk, int fUseBdds, int fUseNaive, int fVerbose ){}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

