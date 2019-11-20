/**CFile****************************************************************

  FileName    [abcQbf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Implementation of a simple QBF solver.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcQbf.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

/*
   Implementation of a simple QBF solver along the lines of
   A. Solar-Lezama, L. Tancau, R. Bodik, V. Saraswat, and S. Seshia, 
   "Combinatorial sketching for finite programs", 12th International 
   Conference on Architectural Support for Programming Languages and 
   Operating Systems (ASPLOS 2006), San Jose, CA, October 2006.
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Abc_NtkModelToVector( Abc_Ntk_t * pNtk, Vec_Int_t * vPiValues );
static void Abc_NtkVectorClearPars( Vec_Int_t * vPiValues, int nPars );
static void Abc_NtkVectorClearVars( Abc_Ntk_t * pNtk, Vec_Int_t * vPiValues, int nPars );
static void Abc_NtkVectorPrintPars( Vec_Int_t * vPiValues, int nPars );
static void Abc_NtkVectorPrintVars( Abc_Ntk_t * pNtk, Vec_Int_t * vPiValues, int nPars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Solve the QBF problem EpAx[M(p,x)].]

  Description [Variables p go first, followed by variable x.
  The number of parameters is nPars. The miter is in pNtk.
  The miter expresses EQUALITY of the implementation and spec.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkQbf( Abc_Ntk_t * pNtk, int nPars, int fVerbose )
{
    Abc_Ntk_t * pNtkVer, * pNtkSyn, * pNtkSyn2, * pNtkTemp;
    Vec_Int_t * vPiValues;
    int clkTotal = clock(), clkS, clkV;
    int nIters, nIterMax = 500, nInputs, RetValue, fFound = 0;

    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkIsComb(pNtk) );
    assert( Abc_NtkPoNum(pNtk) == 1 );
    assert( nPars > 0 && nPars < Abc_NtkPiNum(pNtk) );
    assert( Abc_NtkPiNum(pNtk)-nPars < 32 );
    nInputs = Abc_NtkPiNum(pNtk) - nPars;

    // initialize the synthesized network with 0000-combination
    vPiValues = Vec_IntStart( Abc_NtkPiNum(pNtk) );
    Abc_NtkVectorClearPars( vPiValues, nPars );
    pNtkSyn = Abc_NtkMiterCofactor( pNtk, vPiValues );
    if ( fVerbose )
    {
        printf( "Iter %2d : ", 0 );
        printf( "AIG = %6d  ", Abc_NtkNodeNum(pNtkSyn) );
        Abc_NtkVectorPrintVars( pNtk, vPiValues, nPars );
        printf( "\n" );
    }

    // iteratively solve
    for ( nIters = 0; nIters < nIterMax; nIters++ )
    {
        // solve the synthesis instance
clkS = clock();
        RetValue = Abc_NtkMiterSat( pNtkSyn, 0, 0, 0, NULL, NULL );
clkS = clock() - clkS;
        if ( RetValue == 0 )
            Abc_NtkModelToVector( pNtkSyn, vPiValues );
        if ( RetValue == 1 )
        {
            break;
        }
        if ( RetValue == -1 )
        {
            printf( "Synthesis timed out.\n" );
            break;
        }
        // there is a counter-example

        // construct the verification instance
        Abc_NtkVectorClearVars( pNtk, vPiValues, nPars );
        pNtkVer = Abc_NtkMiterCofactor( pNtk, vPiValues );
        // complement the output
        Abc_ObjXorFaninC( Abc_NtkPo(pNtkVer,0), 0 );

        // solve the verification instance
clkV = clock();
        RetValue = Abc_NtkMiterSat( pNtkVer, 0, 0, 0, NULL, NULL );
clkV = clock() - clkV;
        if ( RetValue == 0 )
            Abc_NtkModelToVector( pNtkVer, vPiValues );
        Abc_NtkDelete( pNtkVer );
        if ( RetValue == 1 )
        {
            fFound = 1;
            break;
        }
        if ( RetValue == -1 )
        {
            printf( "Verification timed out.\n" );
            break;
        }
        // there is a counter-example

        // create a new synthesis network
        Abc_NtkVectorClearPars( vPiValues, nPars );
        pNtkSyn2 = Abc_NtkMiterCofactor( pNtk, vPiValues );
        // add to the synthesis instance
        pNtkSyn = Abc_NtkMiterAnd( pNtkTemp = pNtkSyn, pNtkSyn2, 0, 0 );
        Abc_NtkDelete( pNtkSyn2 );
        Abc_NtkDelete( pNtkTemp );

        if ( fVerbose )
        {
            printf( "Iter %2d : ", nIters+1 );
            printf( "AIG = %6d  ", Abc_NtkNodeNum(pNtkSyn) );
            Abc_NtkVectorPrintVars( pNtk, vPiValues, nPars );
            printf( "  " );
            PRTn( "Syn", clkS );
            PRT( "Ver", clkV );
        }
    }
    Abc_NtkDelete( pNtkSyn );
    // report the results
    if ( fFound )
    {
        printf( "Parameters: " );
        Abc_NtkVectorPrintPars( vPiValues, nPars );
        printf( "\n" );
        printf( "Solved after %d interations.  ", nIters );
    }
    else if ( nIters == nIterMax )
        printf( "Unsolved after %d interations.  ", nIters );
    else 
        printf( "Implementation does not exist.  " );
    PRT( "Total runtime", clock() - clkTotal );
    Vec_IntFree( vPiValues );
}


/**Function*************************************************************

  Synopsis    [Translates model into the vector of values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkModelToVector( Abc_Ntk_t * pNtk, Vec_Int_t * vPiValues )
{
    int * pModel, i;
    pModel = pNtk->pModel;
    for ( i = 0; i < Abc_NtkPiNum(pNtk); i++ )
        Vec_IntWriteEntry( vPiValues, i, pModel[i] );
}

/**Function*************************************************************

  Synopsis    [Clears parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkVectorClearPars( Vec_Int_t * vPiValues, int nPars )
{
    int i;
    for ( i = 0; i < nPars; i++ )
        Vec_IntWriteEntry( vPiValues, i, -1 );
}

/**Function*************************************************************

  Synopsis    [Clears variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkVectorClearVars( Abc_Ntk_t * pNtk, Vec_Int_t * vPiValues, int nPars )
{
    int i;
    for ( i = nPars; i < Abc_NtkPiNum(pNtk); i++ )
        Vec_IntWriteEntry( vPiValues, i, -1 );
}

/**Function*************************************************************

  Synopsis    [Clears variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkVectorPrintPars( Vec_Int_t * vPiValues, int nPars )
{
    int i;
    for ( i = 0; i < nPars; i++ )
        printf( "%d", Vec_IntEntry(vPiValues,i) );
}

/**Function*************************************************************

  Synopsis    [Clears variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkVectorPrintVars( Abc_Ntk_t * pNtk, Vec_Int_t * vPiValues, int nPars )
{
    int i;
    for ( i = nPars; i < Abc_NtkPiNum(pNtk); i++ )
        printf( "%d", Vec_IntEntry(vPiValues,i) );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


