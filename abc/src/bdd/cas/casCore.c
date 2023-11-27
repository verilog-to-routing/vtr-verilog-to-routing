/**CFile****************************************************************

  FileName    [casCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CASCADE: Decomposition of shared BDDs into a LUT cascade.]

  Synopsis    [Entrance into the implementation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Spring 2002.]

  Revision    [$Id: casCore.c,v 1.0 2002/01/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base/main/main.h"
#include "base/cmd/cmd.h"
#include "bdd/extrab/extraBdd.h"
#include "cas.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                      static functions                            ///
////////////////////////////////////////////////////////////////////////

DdNode * GetSingleOutputFunction( DdManager * dd, DdNode ** pbOuts, int nOuts, DdNode ** pbVarsEnc, int nVarsEnc, int fVerbose );
DdNode * GetSingleOutputFunctionRemapped( DdManager * dd, DdNode ** pOutputs, int nOuts, DdNode ** pbVarsEnc, int nVarsEnc );
DdNode * GetSingleOutputFunctionRemappedNewDD( DdManager * dd, DdNode ** pOutputs, int nOuts, DdManager ** DdNew );

extern int CreateDecomposedNetwork( DdManager * dd, DdNode * aFunc, char ** pNames, int nNames, char * FileName, int nLutSize, int fCheck, int fVerbose );

void WriteSingleOutputFunctionBlif( DdManager * dd, DdNode * aFunc, char ** pNames, int nNames, char * FileName );

DdNode * Cudd_bddTransferPermute( DdManager * ddSource, DdManager * ddDestination, DdNode * f, int * Permute );

////////////////////////////////////////////////////////////////////////
///                      static varibles                             ///
////////////////////////////////////////////////////////////////////////

//static FILE * pTable = NULL;
//static long s_RemappingTime = 0;

////////////////////////////////////////////////////////////////////////
///                      debugging macros                            ///
////////////////////////////////////////////////////////////////////////

#define PRD(p)       printf( "\nDECOMPOSITION TREE:\n\n" ); PrintDecEntry( (p), 0 ) 
#define PRB_(f)       printf( #f " = " ); Cudd_bddPrint(dd,f); printf( "\n" )
#define PRK(f,n)     Cudd_PrintKMap(stdout,dd,(f),Cudd_Not(f),(n),NULL,0); printf( "K-map for function" #f "\n\n" )
#define PRK2(f,g,n)  Cudd_PrintKMap(stdout,dd,(f),(g),(n),NULL,0); printf( "K-map for function <" #f ", " #g ">\n\n" ) 


////////////////////////////////////////////////////////////////////////
///                     EXTERNAL FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_CascadeExperiment( char * pFileGeneric, DdManager * dd, DdNode ** pOutputs, int nInputs, int nOutputs, int nLutSize, int fCheck, int fVerbose )
{
    int i;
    int nVars = nInputs;
    int nOuts = nOutputs;
    abctime clk1;

    int      nVarsEnc;              // the number of additional variables to encode outputs
    DdNode * pbVarsEnc[MAXOUTPUTS]; // the BDDs of the encoding vars

    int      nNames;               // the total number of all inputs
    char *   pNames[MAXINPUTS];     // the temporary storage for the input (and output encoding) names

    DdNode * aFunc;                 // the encoded 0-1 BDD containing all the outputs

    char FileNameIni[100];
    char FileNameFin[100];
    char Buffer[100];

    
//pTable = fopen( "stats.txt", "a+" );
//fprintf( pTable, "%s ", pFileGeneric );
//fprintf( pTable, "%d ", nVars );
//fprintf( pTable, "%d ", nOuts );


    // assign the file names
    strcpy( FileNameIni, pFileGeneric );
    strcat( FileNameIni, "_ENC.blif" );

    strcpy( FileNameFin, pFileGeneric );
    strcat( FileNameFin, "_LUT.blif" );


    // create the variables to encode the outputs
    nVarsEnc = Abc_Base2Log( nOuts );
    for ( i = 0; i < nVarsEnc; i++ )
        pbVarsEnc[i] = Cudd_bddNewVarAtLevel( dd, i );


    // store the input names
    nNames  = nVars + nVarsEnc;
    for ( i = 0; i < nVars; i++ )
    {
//      pNames[i] = Extra_UtilStrsav( pFunc->pInputNames[i] );
        sprintf( Buffer, "pi%03d", i );
        pNames[i] = Extra_UtilStrsav( Buffer );
    }
    // set the encoding variable name
    for ( ; i < nNames; i++ )
    {       
        sprintf( Buffer, "OutEnc_%02d", i-nVars );
        pNames[i] = Extra_UtilStrsav( Buffer );
    }


    // print the variable order
//  printf( "\n" );
//  printf( "Variable order is: " );
//  for ( i = 0; i < dd->size; i++ )
//      printf( " %d", dd->invperm[i] );
//  printf( "\n" );

    // derive the single-output function
    clk1 = Abc_Clock();
    aFunc = GetSingleOutputFunction( dd, pOutputs, nOuts, pbVarsEnc, nVarsEnc, fVerbose );  Cudd_Ref( aFunc );
//  aFunc = GetSingleOutputFunctionRemapped( dd, pOutputs, nOuts, pbVarsEnc, nVarsEnc );  Cudd_Ref( aFunc );
//  if ( fVerbose )
//  printf( "Single-output function computation time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );
 
//fprintf( pTable, "%d ", Cudd_SharingSize( pOutputs, nOutputs ) );
//fprintf( pTable, "%d ", Extra_ProfileWidthSharingMax(dd, pOutputs, nOutputs) );

    // dispose of the multiple-output function
//  Extra_Dissolve( pFunc );
 
    // reorder the single output function
//    if ( fVerbose )
//  printf( "Reordering variables...\n");
    clk1 = Abc_Clock();
//  if ( fVerbose )
//  printf( "Node count before = %6d\n", Cudd_DagSize( aFunc ) );
//  Cudd_ReduceHeap(dd, CUDD_REORDER_SIFT,1);
    Cudd_ReduceHeap(dd, CUDD_REORDER_SYMM_SIFT,1);
    Cudd_ReduceHeap(dd, CUDD_REORDER_SYMM_SIFT,1);
//  Cudd_ReduceHeap(dd, CUDD_REORDER_SYMM_SIFT,1);
//  Cudd_ReduceHeap(dd, CUDD_REORDER_SYMM_SIFT,1);
//  Cudd_ReduceHeap(dd, CUDD_REORDER_SYMM_SIFT,1);
//  Cudd_ReduceHeap(dd, CUDD_REORDER_SYMM_SIFT,1);
    if ( fVerbose )
    printf( "MTBDD reordered = %6d nodes\n", Cudd_DagSize( aFunc ) );
    if ( fVerbose )
    printf( "Variable reordering time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );
//  printf( "\n" );
//  printf( "Variable order is: " );
//  for ( i = 0; i < dd->size; i++ )
//      printf( " %d", dd->invperm[i] );
//  printf( "\n" );
//fprintf( pTable, "%d ", Cudd_DagSize( aFunc ) );
//fprintf( pTable, "%d ", Extra_ProfileWidthMax(dd, aFunc) );

    // write the single-output function into BLIF for verification
    clk1 = Abc_Clock();
    if ( fCheck )
    WriteSingleOutputFunctionBlif( dd, aFunc, pNames, nNames, FileNameIni );
//    if ( fVerbose )
//  printf( "Single-output function writing time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );

/*
    ///////////////////////////////////////////////////////////////////
    // verification of single output function
    clk1 = Abc_Clock();
    {
        BFunc g_Func;
        DdNode * aRes;

        g_Func.dd = dd;
        g_Func.FileInput = Extra_UtilStrsav(FileNameIni);

        if ( Extra_ReadFile( &g_Func ) == 0 )
        {
            printf( "\nSomething did not work out while reading the input file for verification\n");
            Extra_Dissolve( &g_Func );
            return;
        } 

        aRes = Cudd_BddToAdd( dd, g_Func.pOutputs[0] );  Cudd_Ref( aRes );

        if ( aRes != aFunc )
            printf( "\nVerification FAILED!\n");
        else
            printf( "\nVerification okay!\n");
        
        Cudd_RecursiveDeref( dd, aRes );

        // delocate
        Extra_Dissolve( &g_Func );
    }
    printf( "Preliminary verification time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );
    ///////////////////////////////////////////////////////////////////
*/

    if ( !CreateDecomposedNetwork( dd, aFunc, pNames, nNames, FileNameFin, nLutSize, fCheck, fVerbose ) )
        return 0;

/*
    ///////////////////////////////////////////////////////////////////
    // verification of the decomposed LUT network
    clk1 = Abc_Clock();
    {
        BFunc g_Func;
        DdNode * aRes;

        g_Func.dd = dd;
        g_Func.FileInput = Extra_UtilStrsav(FileNameFin);

        if ( Extra_ReadFile( &g_Func ) == 0 )
        {
            printf( "\nSomething did not work out while reading the input file for verification\n");
            Extra_Dissolve( &g_Func );
            return;
        } 

        aRes = Cudd_BddToAdd( dd, g_Func.pOutputs[0] );  Cudd_Ref( aRes );

        if ( aRes != aFunc )
            printf( "\nFinal verification FAILED!\n");
        else
            printf( "\nFinal verification okay!\n");
        
        Cudd_RecursiveDeref( dd, aRes );
 
        // delocate 
        Extra_Dissolve( &g_Func );
    }
    printf( "Final verification time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );
    ///////////////////////////////////////////////////////////////////
*/
 
    // verify the results
    if ( fCheck )
    {
        char Command[300];
        sprintf( Command, "cec %s %s", FileNameIni, FileNameFin );
        Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), Command );
    }

    Cudd_RecursiveDeref( dd, aFunc );

    // release the names
    for ( i = 0; i < nNames; i++ )
        ABC_FREE( pNames[i] );


//fprintf( pTable, "\n" );
//fclose( pTable );

    return 1;
}

#if 0

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Experiment2( BFunc * pFunc )
{
    int i, x, RetValue;
    int nVars = pFunc->nInputs;
    int nOuts = pFunc->nOutputs;
    DdManager * dd = pFunc->dd;
    long clk1;

//  int      nVarsEnc;              // the number of additional variables to encode outputs
//  DdNode * pbVarsEnc[MAXOUTPUTS]; // the BDDs of the encoding vars

    int      nNames;               // the total number of all inputs
    char *   pNames[MAXINPUTS];     // the temporary storage for the input (and output encoding) names

    DdNode * aFunc;                 // the encoded 0-1 BDD containing all the outputs

    char FileNameIni[100];
    char FileNameFin[100];
    char Buffer[100];

    DdManager * DdNew;

//pTable = fopen( "stats.txt", "a+" );
//fprintf( pTable, "%s ", pFunc->FileGeneric );
//fprintf( pTable, "%d ", nVars );
//fprintf( pTable, "%d ", nOuts );


    // assign the file names
    strcpy( FileNameIni, pFunc->FileGeneric );
    strcat( FileNameIni, "_ENC.blif" );

    strcpy( FileNameFin, pFunc->FileGeneric );
    strcat( FileNameFin, "_LUT.blif" );

    // derive the single-output function IN THE NEW MANAGER
    clk1 = Abc_Clock();
//  aFunc = GetSingleOutputFunction( dd, pFunc->pOutputs, nOuts, pbVarsEnc, nVarsEnc );  Cudd_Ref( aFunc );
    aFunc = GetSingleOutputFunctionRemappedNewDD( dd, pFunc->pOutputs, nOuts, &DdNew );  Cudd_Ref( aFunc );
    printf( "Single-output function derivation time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );
//  s_RemappingTime = Abc_Clock() - clk1;

    // dispose of the multiple-output function
    Extra_Dissolve( pFunc );

    // reorder the single output function
    printf( "\nReordering variables in the new manager...\n");
    clk1 = Abc_Clock();
    printf( "Node count before = %d\n", Cudd_DagSize( aFunc ) );
//  Cudd_ReduceHeap(DdNew, CUDD_REORDER_SIFT,1);
    Cudd_ReduceHeap(DdNew, CUDD_REORDER_SYMM_SIFT,1);
//  Cudd_ReduceHeap(DdNew, CUDD_REORDER_SYMM_SIFT,1);
    printf( "Node count after  = %d\n", Cudd_DagSize( aFunc ) );
    printf( "Variable reordering time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );
    printf( "\n" );

//fprintf( pTable, "%d ", Cudd_DagSize( aFunc ) );
//fprintf( pTable, "%d ", Extra_ProfileWidthMax(DdNew, aFunc) );


    // create the names to be used with the new manager
    nNames = DdNew->size;
    for ( x = 0; x < nNames; x++ )
    {
        sprintf( Buffer, "v%02d", x );
        pNames[x] = Extra_UtilStrsav( Buffer );
    }



    // write the single-output function into BLIF for verification
    clk1 = Abc_Clock();
    WriteSingleOutputFunctionBlif( DdNew, aFunc, pNames, nNames, FileNameIni );
    printf( "Single-output function writing time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );


    ///////////////////////////////////////////////////////////////////
    // verification of single output function
    clk1 = Abc_Clock();
    {
        BFunc g_Func;
        DdNode * aRes;

        g_Func.dd = DdNew;
        g_Func.FileInput = Extra_UtilStrsav(FileNameIni);

        if ( Extra_ReadFile( &g_Func ) == 0 )
        {
            printf( "\nSomething did not work out while reading the input file for verification\n");
            Extra_Dissolve( &g_Func );
            return;
        } 

        aRes = Cudd_BddToAdd( DdNew, g_Func.pOutputs[0] );  Cudd_Ref( aRes );

        if ( aRes != aFunc )
            printf( "\nVerification FAILED!\n");
        else
            printf( "\nVerification okay!\n");
        
        Cudd_RecursiveDeref( DdNew, aRes );

        // delocate
        Extra_Dissolve( &g_Func );
    }
    printf( "Preliminary verification time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );
    ///////////////////////////////////////////////////////////////////


    CreateDecomposedNetwork( DdNew, aFunc, pNames, nNames, FileNameFin, nLutSize, 0 );

/*
    ///////////////////////////////////////////////////////////////////
    // verification of the decomposed LUT network
    clk1 = Abc_Clock();
    {
        BFunc g_Func;
        DdNode * aRes;

        g_Func.dd = DdNew;
        g_Func.FileInput = Extra_UtilStrsav(FileNameFin);

        if ( Extra_ReadFile( &g_Func ) == 0 )
        {
            printf( "\nSomething did not work out while reading the input file for verification\n");
            Extra_Dissolve( &g_Func );
            return;
        } 

        aRes = Cudd_BddToAdd( DdNew, g_Func.pOutputs[0] );  Cudd_Ref( aRes );

        if ( aRes != aFunc )
            printf( "\nFinal verification FAILED!\n");
        else
            printf( "\nFinal verification okay!\n");
        
        Cudd_RecursiveDeref( DdNew, aRes );

        // delocate
        Extra_Dissolve( &g_Func );
    }
    printf( "Final verification time = %.2f sec\n", (float)(Abc_Clock() - clk1)/(float)(CLOCKS_PER_SEC) );
    ///////////////////////////////////////////////////////////////////
*/


    Cudd_RecursiveDeref( DdNew, aFunc );

    // release the names
    for ( i = 0; i < nNames; i++ )
        ABC_FREE( pNames[i] );


    
    /////////////////////////////////////////////////////////////////////
    // check for remaining references in the package
    RetValue = Cudd_CheckZeroRef( DdNew );
    printf( "\nThe number of referenced nodes in the new manager = %d\n", RetValue );
    Cudd_Quit( DdNew );

//fprintf( pTable, "\n" );
//fclose( pTable );

} 

#endif

////////////////////////////////////////////////////////////////////////
///                       SINGLE OUTPUT FUNCTION                     ///
////////////////////////////////////////////////////////////////////////

// the bit count for the first 256 integer numbers
//static unsigned char BitCount8[256] = {
//    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
//    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
//    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
//    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
//    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
//    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
//    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
//    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
//};

/////////////////////////////////////////////////////////////
static int s_SuppSize[MAXOUTPUTS];
int CompareSupports( int *ptrX, int *ptrY )
{
    return ( s_SuppSize[*ptrY] - s_SuppSize[*ptrX] );
}
/////////////////////////////////////////////////////////////

 
/////////////////////////////////////////////////////////////
static int s_MintOnes[MAXOUTPUTS];
int CompareMinterms( int *ptrX, int *ptrY )
{
    return ( s_MintOnes[*ptrY] - s_MintOnes[*ptrX] );
}
/////////////////////////////////////////////////////////////

int GrayCode ( int BinCode )
{ 
  return BinCode ^ ( BinCode >> 1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * GetSingleOutputFunction( DdManager * dd, DdNode ** pbOuts, int nOuts, DdNode ** pbVarsEnc, int nVarsEnc, int fVerbose )
{
    int i;
    DdNode * bResult, * aResult;
    DdNode * bCube, * bTemp, * bProd;

    int Order[MAXOUTPUTS];
//  int OrderMint[MAXOUTPUTS];
 
    // sort the output according to their support size
    for ( i = 0; i < nOuts; i++ )
    {
        s_SuppSize[i] = Cudd_SupportSize( dd, pbOuts[i] );
//      s_MintOnes[i] = BitCount8[i];
        Order[i]      = i;
//      OrderMint[i]  = i;
    }
    
    // order the outputs
    qsort( (void*)Order,     (size_t)nOuts, sizeof(int), (int(*)(const void*, const void*)) CompareSupports );
    // order the outputs
//  qsort( (void*)OrderMint, (size_t)nOuts, sizeof(int), (int(*)(const void*, const void*)) CompareMinterms );


    bResult = b0;   Cudd_Ref( bResult );
    for ( i = 0; i < nOuts; i++ )
    {
//      bCube   = Cudd_bddBitsToCube( dd, OrderMint[i], nVarsEnc, pbVarsEnc );   Cudd_Ref( bCube );
//      bProd   = Cudd_bddAnd( dd, bCube, pbOuts[Order[nOuts-1-i]] );         Cudd_Ref( bProd );
        bCube   = Extra_bddBitsToCube( dd, i, nVarsEnc, pbVarsEnc, 1 );   Cudd_Ref( bCube );
        bProd   = Cudd_bddAnd( dd, bCube, pbOuts[Order[i]] );         Cudd_Ref( bProd );
        Cudd_RecursiveDeref( dd, bCube );

        bResult = Cudd_bddOr( dd, bProd, bTemp = bResult );           Cudd_Ref( bResult );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bProd );
    }

    // convert to the ADD
if ( fVerbose )
printf( "Single BDD size = %6d nodes\n", Cudd_DagSize(bResult) );
    aResult = Cudd_BddToAdd( dd, bResult );  Cudd_Ref( aResult );
    Cudd_RecursiveDeref( dd, bResult );
if ( fVerbose )
printf( "MTBDD           = %6d nodes\n", Cudd_DagSize(aResult) );
    Cudd_Deref( aResult );
    return aResult;
}
/*
DdNode * GetSingleOutputFunction( DdManager * dd, DdNode ** pbOuts, int nOuts, DdNode ** pbVarsEnc, int nVarsEnc )
{
    int i;
    DdNode * bResult, * aResult;
    DdNode * bCube, * bTemp, * bProd;

    bResult = b0;   Cudd_Ref( bResult );
    for ( i = 0; i < nOuts; i++ )
    {
//      bCube   = Extra_bddBitsToCube( dd, i, nVarsEnc, pbVarsEnc );   Cudd_Ref( bCube );
        bCube   = Extra_bddBitsToCube( dd, nOuts-1-i, nVarsEnc, pbVarsEnc );   Cudd_Ref( bCube );
        bProd   = Cudd_bddAnd( dd, bCube, pbOuts[i] );                Cudd_Ref( bProd );
        Cudd_RecursiveDeref( dd, bCube );

        bResult = Cudd_bddOr( dd, bProd, bTemp = bResult );           Cudd_Ref( bResult );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bProd );
    }

    // conver to the ADD
    aResult = Cudd_BddToAdd( dd, bResult );  Cudd_Ref( aResult );
    Cudd_RecursiveDeref( dd, bResult );

    Cudd_Deref( aResult );
    return aResult;
}
*/


////////////////////////////////////////////////////////////////////////
///                        INPUT REMAPPING                           ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * GetSingleOutputFunctionRemapped( DdManager * dd, DdNode ** pOutputs, int nOuts, DdNode ** pbVarsEnc, int nVarsEnc )
// returns the ADD of the remapped function
{
    static int Permute[MAXINPUTS];
    static DdNode * pRemapped[MAXOUTPUTS];

    DdNode * bSupp, * bTemp;
    int i, Counter;
    DdNode * bFunc;
    DdNode * aFunc;

    Cudd_AutodynDisable(dd);

    // perform the remapping
    for ( i = 0; i < nOuts; i++ )
    {
        // get support
        bSupp = Cudd_Support( dd, pOutputs[i] );    Cudd_Ref( bSupp );

        // create the variable map
        Counter = 0;
        for ( bTemp = bSupp; bTemp != dd->one; bTemp = cuddT(bTemp) )
            Permute[bTemp->index] = Counter++;

        // transfer the BDD and remap it
        pRemapped[i] = Cudd_bddPermute( dd, pOutputs[i], Permute );  Cudd_Ref( pRemapped[i] );

        // remove support
        Cudd_RecursiveDeref( dd, bSupp );
    }
    
    // perform the encoding
    bFunc = Extra_bddEncodingBinary( dd, pRemapped, nOuts, pbVarsEnc, nVarsEnc );   Cudd_Ref( bFunc );

    // convert to ADD
    aFunc = Cudd_BddToAdd( dd, bFunc );  Cudd_Ref( aFunc );
    Cudd_RecursiveDeref( dd, bFunc );

    // deref the intermediate results
    for ( i = 0; i < nOuts; i++ )
        Cudd_RecursiveDeref( dd, pRemapped[i] );

    Cudd_Deref( aFunc );
    return aFunc;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * GetSingleOutputFunctionRemappedNewDD( DdManager * dd, DdNode ** pOutputs, int nOuts, DdManager ** DdNew )
// returns the ADD of the remapped function
{
    static int Permute[MAXINPUTS];
    static DdNode * pRemapped[MAXOUTPUTS];

    static DdNode * pbVarsEnc[MAXINPUTS];
    int nVarsEnc;

    DdManager * ddnew;

    DdNode * bSupp, * bTemp;
    int i, v, Counter;
    DdNode * bFunc;

    // these are in the new manager
    DdNode * bFuncNew;
    DdNode * aFuncNew;

    int nVarsMax = 0;

    // perform the remapping and write the DDs into the new manager
    for ( i = 0; i < nOuts; i++ )
    {
        // get support
        bSupp = Cudd_Support( dd, pOutputs[i] );    Cudd_Ref( bSupp );

        // create the variable map
        // to remap the DD into the upper part of the manager
        Counter = 0;
        for ( bTemp = bSupp; bTemp != dd->one; bTemp = cuddT(bTemp) )
            Permute[bTemp->index] = dd->invperm[Counter++];

        // transfer the BDD and remap it
        pRemapped[i] = Cudd_bddPermute( dd, pOutputs[i], Permute );  Cudd_Ref( pRemapped[i] );

        // remove support
        Cudd_RecursiveDeref( dd, bSupp );


        // determine the largest support size
        if ( nVarsMax < Counter )
            nVarsMax = Counter;
    }
    
    // select the encoding variables to follow immediately after the original variables
    nVarsEnc = Abc_Base2Log(nOuts);
/*
    for ( v = 0; v < nVarsEnc; v++ )
        if ( nVarsMax + v < dd->size )
            pbVarsEnc[v] = dd->var[ dd->invperm[nVarsMax+v] ];
        else
            pbVarsEnc[v] = Cudd_bddNewVar( dd );
*/
    // create the new variables on top of the manager
    for ( v = 0; v < nVarsEnc; v++ )
        pbVarsEnc[v] = Cudd_bddNewVarAtLevel( dd, v );

//fprintf( pTable, "%d ", Cudd_SharingSize( pRemapped, nOuts ) );
//fprintf( pTable, "%d ", Extra_ProfileWidthSharingMax(dd, pRemapped, nOuts) );


    // perform the encoding
    bFunc = Extra_bddEncodingBinary( dd, pRemapped, nOuts, pbVarsEnc, nVarsEnc );   Cudd_Ref( bFunc );


    // find the cross-manager permutation
    // the variable from the level v in the old manager 
    // should become a variable number v in the new manager
    for ( v = 0; v < nVarsMax + nVarsEnc; v++ )
        Permute[dd->invperm[v]] = v;


    ///////////////////////////////////////////////////////////////////////////////
    // start the new manager
    ddnew = Cudd_Init( nVarsMax + nVarsEnc, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
//  Cudd_AutodynDisable(ddnew);
    Cudd_AutodynEnable(dd, CUDD_REORDER_SYMM_SIFT);

    // transfer it to the new manager
    bFuncNew = Cudd_bddTransferPermute( dd, ddnew, bFunc, Permute );      Cudd_Ref( bFuncNew );
    ///////////////////////////////////////////////////////////////////////////////


    // deref the intermediate results in the old manager
    Cudd_RecursiveDeref( dd, bFunc );
    for ( i = 0; i < nOuts; i++ )
        Cudd_RecursiveDeref( dd, pRemapped[i] );


    ///////////////////////////////////////////////////////////////////////////////
    // convert to ADD in the new manager
    aFuncNew = Cudd_BddToAdd( ddnew, bFuncNew );  Cudd_Ref( aFuncNew );
    Cudd_RecursiveDeref( ddnew, bFuncNew );

    // return the manager
    *DdNew = ddnew;
    ///////////////////////////////////////////////////////////////////////////////

    Cudd_Deref( aFuncNew );
    return aFuncNew;
}

////////////////////////////////////////////////////////////////////////
///                        BLIF WRITING FUNCTIONS                    ///
////////////////////////////////////////////////////////////////////////

void WriteDDintoBLIFfile( FILE * pFile, DdNode * Func, char * OutputName, char * Prefix, char ** InputNames );


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void WriteSingleOutputFunctionBlif( DdManager * dd, DdNode * aFunc, char ** pNames, int nNames, char * FileName )
{
    int i;
    FILE * pFile;

    // start the file
    pFile = fopen( FileName, "w" );
    fprintf( pFile, ".model %s\n", FileName );

    fprintf( pFile, ".inputs" );
    for ( i = 0; i < nNames; i++ )
        fprintf( pFile, " %s", pNames[i] );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs F" );
    fprintf( pFile, "\n" );

    // write the DD into the file
    WriteDDintoBLIFfile( pFile, aFunc, "F", "", pNames );

    fprintf( pFile, ".end\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void WriteDDintoBLIFfile( FILE * pFile, DdNode * Func, char * OutputName, char * Prefix, char ** InputNames )
// writes the main part of the BLIF file 
// Func is a BDD or a 0-1 ADD to be written
// OutputName is the name of the output
// Prefix is attached to each intermendiate signal to make it unique
// InputNames are the names of the input signals
// (some part of the code is borrowed from Cudd_DumpDot())
{
    int i;
    st__table * visited;
    st__generator * gen = NULL;
    long refAddr, diff, mask;
    DdNode * Node, * Else, * ElseR, * Then;

    /* Initialize symbol table for visited nodes. */
    visited = st__init_table( st__ptrcmp, st__ptrhash );

    /* Collect all the nodes of this DD in the symbol table. */
    cuddCollectNodes( Cudd_Regular(Func), visited );

    /* Find how many most significant hex digits are identical
       ** in the addresses of all the nodes. Build a mask based
       ** on this knowledge, so that digits that carry no information
       ** will not be printed. This is done in two steps.
       **  1. We scan the symbol table to find the bits that differ
       **     in at least 2 addresses.
       **  2. We choose one of the possible masks. There are 8 possible
       **     masks for 32-bit integer, and 16 possible masks for 64-bit
       **     integers.
     */

    /* Find the bits that are different. */
    refAddr = ( long )Cudd_Regular(Func);
    diff = 0;
    gen = st__init_gen( visited );
    while ( st__gen( gen, ( const char ** ) &Node, NULL ) )
    {
        diff |= refAddr ^ ( long ) Node;
    }
    st__free_gen( gen );
    gen = NULL;

    /* Choose the mask. */
    for ( i = 0; ( unsigned ) i < 8 * sizeof( long ); i += 4 )
    {
        mask = ( 1 << i ) - 1;
        if ( diff <= mask )
            break;
    }


    // write the buffer for the output
    fprintf( pFile, ".names %s%lx %s\n", Prefix, ( mask & (long)Cudd_Regular(Func) ) / sizeof(DdNode), OutputName ); 
    fprintf( pFile, "%s 1\n", (Cudd_IsComplement(Func))? "0": "1" );


    gen = st__init_gen( visited );
    while ( st__gen( gen, ( const char ** ) &Node, NULL ) )
    {
        if ( Node->index == CUDD_MAXINDEX )
        {
            // write the terminal node
            fprintf( pFile, ".names %s%lx\n", Prefix, ( mask & (long)Node ) / sizeof(DdNode) );
            fprintf( pFile, " %s\n", (cuddV(Node) == 0.0)? "0": "1" );
            continue;
        }

        Else  = cuddE(Node);
        ElseR = Cudd_Regular(Else);
        Then  = cuddT(Node);

        assert( InputNames[Node->index] );
        if ( Else == ElseR )
        { // no inverter
            fprintf( pFile, ".names %s %s%lx %s%lx %s%lx\n", InputNames[Node->index],                           
                              Prefix, ( mask & (long)ElseR ) / sizeof(DdNode),
                              Prefix, ( mask & (long)Then  ) / sizeof(DdNode),
                              Prefix, ( mask & (long)Node  ) / sizeof(DdNode)   );
            fprintf( pFile, "01- 1\n" );
            fprintf( pFile, "1-1 1\n" );
        }
        else
        { // inverter
            int * pSlot;
            fprintf( pFile, ".names %s %s%lx_i %s%lx %s%lx\n", InputNames[Node->index],                         
                              Prefix, ( mask & (long)ElseR ) / sizeof(DdNode),
                              Prefix, ( mask & (long)Then  ) / sizeof(DdNode),
                              Prefix, ( mask & (long)Node  ) / sizeof(DdNode)   );
            fprintf( pFile, "01- 1\n" );
            fprintf( pFile, "1-1 1\n" );

            // if the inverter is written, skip
            if ( ! st__find( visited, (char *)ElseR, (char ***)&pSlot ) )
                assert( 0 );
            if ( *pSlot )
                continue;
            *pSlot = 1;

            fprintf( pFile, ".names %s%lx %s%lx_i\n",  
                              Prefix, ( mask & (long)ElseR  ) / sizeof(DdNode),
                              Prefix, ( mask & (long)ElseR  ) / sizeof(DdNode)   );
            fprintf( pFile, "0 1\n" );
        }
    }
    st__free_gen( gen );
    gen = NULL;
    st__free_table( visited );
}




static DdManager * s_ddmin;

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void WriteDDintoBLIFfileReorder( DdManager * dd, FILE * pFile, DdNode * Func, char * OutputName, char * Prefix, char ** InputNames )
// writes the main part of the BLIF file 
// Func is a BDD or a 0-1 ADD to be written
// OutputName is the name of the output
// Prefix is attached to each intermendiate signal to make it unique
// InputNames are the names of the input signals
// (some part of the code is borrowed from Cudd_DumpDot())
{
    int i;
    st__table * visited;
    st__generator * gen = NULL;
    long refAddr, diff, mask;
    DdNode * Node, * Else, * ElseR, * Then;


    ///////////////////////////////////////////////////////////////
    DdNode * bFmin;
    abctime clk1;

    if ( s_ddmin == NULL )
        s_ddmin = Cudd_Init( dd->size, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);

    clk1 = Abc_Clock();
    bFmin = Cudd_bddTransfer( dd, s_ddmin, Func );  Cudd_Ref( bFmin );

    // reorder
    printf( "Nodes before = %d.   ", Cudd_DagSize(bFmin) ); 
    Cudd_ReduceHeap(s_ddmin,CUDD_REORDER_SYMM_SIFT,1);
//  Cudd_ReduceHeap(s_ddmin,CUDD_REORDER_SYMM_SIFT_CONV,1);
    printf( "Nodes after  = %d.  \n", Cudd_DagSize(bFmin) ); 
    ///////////////////////////////////////////////////////////////



    /* Initialize symbol table for visited nodes. */
    visited = st__init_table( st__ptrcmp, st__ptrhash );

    /* Collect all the nodes of this DD in the symbol table. */
    cuddCollectNodes( Cudd_Regular(bFmin), visited );

    /* Find how many most significant hex digits are identical
       ** in the addresses of all the nodes. Build a mask based
       ** on this knowledge, so that digits that carry no information
       ** will not be printed. This is done in two steps.
       **  1. We scan the symbol table to find the bits that differ
       **     in at least 2 addresses.
       **  2. We choose one of the possible masks. There are 8 possible
       **     masks for 32-bit integer, and 16 possible masks for 64-bit
       **     integers.
     */

    /* Find the bits that are different. */
    refAddr = ( long )Cudd_Regular(bFmin);
    diff = 0;
    gen = st__init_gen( visited );
    while ( st__gen( gen, ( const char ** ) &Node, NULL ) )
    {
        diff |= refAddr ^ ( long ) Node;
    }
    st__free_gen( gen );
    gen = NULL;

    /* Choose the mask. */
    for ( i = 0; ( unsigned ) i < 8 * sizeof( long ); i += 4 )
    {
        mask = ( 1 << i ) - 1;
        if ( diff <= mask )
            break;
    }


    // write the buffer for the output
    fprintf( pFile, ".names %s%lx %s\n", Prefix, ( mask & (long)Cudd_Regular(bFmin) ) / sizeof(DdNode), OutputName ); 
    fprintf( pFile, "%s 1\n", (Cudd_IsComplement(bFmin))? "0": "1" );


    gen = st__init_gen( visited );
    while ( st__gen( gen, ( const char ** ) &Node, NULL ) )
    {
        if ( Node->index == CUDD_MAXINDEX )
        {
            // write the terminal node
            fprintf( pFile, ".names %s%lx\n", Prefix, ( mask & (long)Node ) / sizeof(DdNode) );
            fprintf( pFile, " %s\n", (cuddV(Node) == 0.0)? "0": "1" );
            continue;
        }

        Else  = cuddE(Node);
        ElseR = Cudd_Regular(Else);
        Then  = cuddT(Node);

        assert( InputNames[Node->index] );
        if ( Else == ElseR )
        { // no inverter
            fprintf( pFile, ".names %s %s%lx %s%lx %s%lx\n", InputNames[Node->index],                           
                              Prefix, ( mask & (long)ElseR ) / sizeof(DdNode),
                              Prefix, ( mask & (long)Then  ) / sizeof(DdNode),
                              Prefix, ( mask & (long)Node  ) / sizeof(DdNode)   );
            fprintf( pFile, "01- 1\n" );
            fprintf( pFile, "1-1 1\n" );
        }
        else
        { // inverter
            fprintf( pFile, ".names %s %s%lx_i %s%lx %s%lx\n", InputNames[Node->index],                         
                              Prefix, ( mask & (long)ElseR ) / sizeof(DdNode),
                              Prefix, ( mask & (long)Then  ) / sizeof(DdNode),
                              Prefix, ( mask & (long)Node  ) / sizeof(DdNode)   );
            fprintf( pFile, "01- 1\n" );
            fprintf( pFile, "1-1 1\n" );

            fprintf( pFile, ".names %s%lx %s%lx_i\n",  
                              Prefix, ( mask & (long)ElseR  ) / sizeof(DdNode),
                              Prefix, ( mask & (long)ElseR  ) / sizeof(DdNode)   );
            fprintf( pFile, "0 1\n" );
        }
    }
    st__free_gen( gen );
    gen = NULL;
    st__free_table( visited );


    //////////////////////////////////////////////////
    Cudd_RecursiveDeref( s_ddmin, bFmin );
    //////////////////////////////////////////////////
}




////////////////////////////////////////////////////////////////////////
///                    TRANSFER WITH MAPPING                         ///
////////////////////////////////////////////////////////////////////////
static DdNode * cuddBddTransferPermuteRecur
ARGS((DdManager * ddS, DdManager * ddD, DdNode * f, st__table * table, int * Permute ));

static DdNode * cuddBddTransferPermute
ARGS((DdManager * ddS, DdManager * ddD, DdNode * f, int * Permute));

/**Function********************************************************************

  Synopsis    [Convert a BDD from a manager to another one.]

  Description [Convert a BDD from a manager to another one. The orders of the
  variables in the two managers may be different. Returns a
  pointer to the BDD in the destination manager if successful; NULL
  otherwise. The i-th entry in the array Permute tells what is the index
  of the i-th variable from the old manager in the new manager.]

  SideEffects [None]

  SeeAlso     []

******************************************************************************/
DdNode *
Cudd_bddTransferPermute( DdManager * ddSource,
                  DdManager * ddDestination, DdNode * f, int * Permute )
{
    DdNode *res;
    do
    {
        ddDestination->reordered = 0;
        res = cuddBddTransferPermute( ddSource, ddDestination, f, Permute );
    }
    while ( ddDestination->reordered == 1 );
    return ( res );

}                               /* end of Cudd_bddTransferPermute */


/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**Function********************************************************************

  Synopsis    [Convert a BDD from a manager to another one.]

  Description [Convert a BDD from a manager to another one. Returns a
  pointer to the BDD in the destination manager if successful; NULL
  otherwise.]

  SideEffects [None]

  SeeAlso     [Cudd_bddTransferPermute]

******************************************************************************/
DdNode *
cuddBddTransferPermute( DdManager * ddS, DdManager * ddD, DdNode * f, int * Permute )
{
    DdNode *res;
    st__table *table = NULL;
    st__generator *gen = NULL;
    DdNode *key, *value;

    table = st__init_table( st__ptrcmp, st__ptrhash );
    if ( table == NULL )
        goto failure;
    res = cuddBddTransferPermuteRecur( ddS, ddD, f, table, Permute );
    if ( res != NULL )
        cuddRef( res );

    /* Dereference all elements in the table and dispose of the table.
       ** This must be done also if res is NULL to avoid leaks in case of
       ** reordering. */
    gen = st__init_gen( table );
    if ( gen == NULL )
        goto failure;
    while ( st__gen( gen, ( const char ** ) &key, ( char ** ) &value ) )
    {
        Cudd_RecursiveDeref( ddD, value );
    }
    st__free_gen( gen );
    gen = NULL;
    st__free_table( table );
    table = NULL;

    if ( res != NULL )
        cuddDeref( res );
    return ( res );

  failure:
    if ( table != NULL )
        st__free_table( table );
    if ( gen != NULL )
        st__free_gen( gen );
    return ( NULL );

}                               /* end of cuddBddTransferPermute */


/**Function********************************************************************

  Synopsis    [Performs the recursive step of Cudd_bddTransferPermute.]

  Description [Performs the recursive step of Cudd_bddTransferPermute.
  Returns a pointer to the result if successful; NULL otherwise.]

  SideEffects [None]

  SeeAlso     [cuddBddTransferPermute]

******************************************************************************/
static DdNode *
cuddBddTransferPermuteRecur( DdManager * ddS,
                      DdManager * ddD, DdNode * f, st__table * table, int * Permute )
{
    DdNode *ft, *fe, *t, *e, *var, *res;
    DdNode *one, *zero;
    int index;
    int comple = 0;

    statLine( ddD );
    one = DD_ONE( ddD );
    comple = Cudd_IsComplement( f );

    /* Trivial cases. */
    if ( Cudd_IsConstant( f ) )
        return ( Cudd_NotCond( one, comple ) );

    /* Make canonical to increase the utilization of the cache. */
    f = Cudd_NotCond( f, comple );
    /* Now f is a regular pointer to a non-constant node. */

    /* Check the cache. */
    if ( st__lookup( table, ( char * ) f, ( char ** ) &res ) )
        return ( Cudd_NotCond( res, comple ) );

    /* Recursive step. */
    index = Permute[f->index];
    ft = cuddT( f );
    fe = cuddE( f );

    t = cuddBddTransferPermuteRecur( ddS, ddD, ft, table, Permute );
    if ( t == NULL )
    {
        return ( NULL );
    }
    cuddRef( t );

    e = cuddBddTransferPermuteRecur( ddS, ddD, fe, table, Permute );
    if ( e == NULL )
    {
        Cudd_RecursiveDeref( ddD, t );
        return ( NULL );
    }
    cuddRef( e );

    zero = Cudd_Not( one );
    var = cuddUniqueInter( ddD, index, one, zero );
    if ( var == NULL )
    {
        Cudd_RecursiveDeref( ddD, t );
        Cudd_RecursiveDeref( ddD, e );
        return ( NULL );
    }
    res = cuddBddIteRecur( ddD, var, t, e );
    if ( res == NULL )
    {
        Cudd_RecursiveDeref( ddD, t );
        Cudd_RecursiveDeref( ddD, e );
        return ( NULL );
    }
    cuddRef( res );
    Cudd_RecursiveDeref( ddD, t );
    Cudd_RecursiveDeref( ddD, e );

    if ( st__add_direct( table, ( char * ) f, ( char * ) res ) ==
         st__OUT_OF_MEM )
    {
        Cudd_RecursiveDeref( ddD, res );
        return ( NULL );
    }
    return ( Cudd_NotCond( res, comple ) );

}                               /* end of cuddBddTransferPermuteRecur */

////////////////////////////////////////////////////////////////////////
///                           END OF FILE                            ///
////////////////////////////////////////////////////////////////////////




ABC_NAMESPACE_IMPL_END

