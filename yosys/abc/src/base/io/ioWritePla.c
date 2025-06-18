/**CFile****************************************************************

  FileName    [ioWritePla.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write the network in BENCH format.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWritePla.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the network in PLA format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WritePlaOne( FILE * pFile, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode, * pFanin, * pDriver;
    char * pCubeIn, * pCubeOut, * pCube;
    int i, k, nProducts, nInputs, nOutputs, nFanins;

    nProducts = 0;
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pDriver = Abc_ObjFanin0Ntk( Abc_ObjFanin0(pNode) );
        if ( !Abc_ObjIsNode(pDriver) )
        {
            nProducts++;
            continue;
        }
        if ( Abc_NodeIsConst(pDriver) )
        {
            if ( Abc_NodeIsConst1(pDriver) )
                nProducts++;
            continue;
        }
        nProducts += Abc_SopGetCubeNum((char *)pDriver->pData);
    }

    // collect the parameters
    nInputs  = Abc_NtkCiNum(pNtk);
    nOutputs = Abc_NtkCoNum(pNtk);
    pCubeIn  = ABC_ALLOC( char, nInputs + 1 );
    pCubeOut = ABC_ALLOC( char, nOutputs + 1 );
    memset( pCubeIn,  '-', (size_t)nInputs );     pCubeIn[nInputs]   = 0;
    memset( pCubeOut, '0', (size_t)nOutputs );    pCubeOut[nOutputs] = 0;

    // write the header
    fprintf( pFile, ".i %d\n", nInputs );
    fprintf( pFile, ".o %d\n", nOutputs );
    fprintf( pFile, ".ilb" );
    Abc_NtkForEachCi( pNtk, pNode, i )
        fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanout0(pNode)) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".ob" );
    Abc_NtkForEachCo( pNtk, pNode, i )
        fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin0(pNode)) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".p %d\n", nProducts );

    // mark the CI nodes
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)i;

    // write the cubes
    pProgress = Extra_ProgressBarStart( stdout, nOutputs );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        // prepare the output cube
        if ( i - 1 >= 0 )
            pCubeOut[i-1] = '0';
        pCubeOut[i] = '1';

        // consider special cases of nodes
        pDriver = Abc_ObjFanin0Ntk( Abc_ObjFanin0(pNode) );
        if ( !Abc_ObjIsNode(pDriver) )
        {
            assert( Abc_ObjIsCi(pDriver) );
            pCubeIn[(int)(ABC_PTRUINT_T)pDriver->pCopy] = '1' - Abc_ObjFaninC0(pNode);
            fprintf( pFile, "%s %s\n", pCubeIn, pCubeOut );
            pCubeIn[(int)(ABC_PTRUINT_T)pDriver->pCopy] = '-';
            continue;
        }
        if ( Abc_NodeIsConst(pDriver) )
        {
            if ( Abc_NodeIsConst1(pDriver) )
                fprintf( pFile, "%s %s\n", pCubeIn, pCubeOut );
            continue;
        }

        // make sure the cover is not complemented
        assert( !Abc_SopIsComplement( (char *)pDriver->pData ) );

        // write the cubes
        nFanins = Abc_ObjFaninNum(pDriver);
        Abc_SopForEachCube( (char *)pDriver->pData, nFanins, pCube )
        {
            Abc_ObjForEachFanin( pDriver, pFanin, k )
            {
                pFanin = Abc_ObjFanin0Ntk(pFanin);
                assert( (int)(ABC_PTRUINT_T)pFanin->pCopy < nInputs );
                pCubeIn[(int)(ABC_PTRUINT_T)pFanin->pCopy] = pCube[k];
            }
            fprintf( pFile, "%s %s\n", pCubeIn, pCubeOut );
        }
        // clean the cube for future writing
        Abc_ObjForEachFanin( pDriver, pFanin, k )
        {
            pFanin = Abc_ObjFanin0Ntk(pFanin);
            assert( Abc_ObjIsCi(pFanin) );
            pCubeIn[(int)(ABC_PTRUINT_T)pFanin->pCopy] = '-';
        }
        Extra_ProgressBarUpdate( pProgress, i, NULL );
    }
    Extra_ProgressBarStop( pProgress );
    fprintf( pFile, ".e\n" );

    // clean the CI nodes
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy = NULL;
    ABC_FREE( pCubeIn );
    ABC_FREE( pCubeOut );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the network in PLA format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WritePla( Abc_Ntk_t * pNtk, char * pFileName )
{
    Abc_Ntk_t * pExdc;
    FILE * pFile;

    assert( Abc_NtkIsSopNetlist(pNtk) );
    assert( Abc_NtkLevel(pNtk) == 1 );

    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WritePla(): Cannot open the output file.\n" );
        return 0;
    }
    fprintf( pFile, "# Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    // write the network
    Io_WritePlaOne( pFile, pNtk );
    // write EXDC network if it exists
    pExdc = Abc_NtkExdc( pNtk );
    if ( pExdc )
        printf( "Io_WritePla: EXDC is not written (warning).\n" );
    // finalize the file
    fclose( pFile );
    return 1;
}

#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Writes the network in PLA format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteMoPlaOneInt( FILE * pFile, Abc_Ntk_t * pNtk, DdManager * dd, Vec_Ptr_t * vFuncs )
{
    Abc_Obj_t * pNode;
    DdNode * bOnset, * bOffset, * bCube, * bFunc, * bTemp, * zCover;
    int i, k, nInputs, nOutputs;
    int nCubes, fPhase;

    assert( Vec_PtrSize(vFuncs) == Abc_NtkCoNum(pNtk) );
    assert( dd->size == Abc_NtkCiNum(pNtk) );
    assert( dd->size <= 1000 );

    // collect the parameters
    nInputs   = Abc_NtkCiNum(pNtk);
    nOutputs  = Abc_NtkCoNum(pNtk);
    assert( nOutputs > 1 );

    // create extra variables
    for ( i = 0; i < nOutputs; i++ )
        Cudd_bddNewVarAtLevel( dd, i );
    assert( dd->size == nInputs + nOutputs );

    // create ON and OFF sets
    bOnset = Cudd_ReadLogicZero( dd );  Cudd_Ref(bOnset);
    bOffset = Cudd_ReadLogicZero( dd ); Cudd_Ref(bOffset);
    for ( i = 0; i < nOutputs; i++ )
    {
        bFunc = (DdNode *)Vec_PtrEntry(vFuncs, i);
        // create onset
        bCube = Cudd_bddAnd( dd, Cudd_bddIthVar(dd, nInputs+i), bFunc );  Cudd_Ref(bCube);
        for ( k = 0; k < nOutputs; k++ ) 
            if ( k != i )
            {
                bCube = Cudd_bddAnd( dd, bTemp = bCube, Cudd_Not(Cudd_bddIthVar(dd, nInputs+k)) );  Cudd_Ref(bCube);
                Cudd_RecursiveDeref( dd, bTemp );
            }
        bOnset = Cudd_bddOr( dd, bTemp = bOnset, bCube );   Cudd_Ref(bOnset);
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
        // create offset
        bCube = Cudd_bddAnd( dd, Cudd_bddIthVar(dd, nInputs+i), Cudd_Not(bFunc) );  Cudd_Ref(bCube);
        bOffset = Cudd_bddOr( dd, bTemp = bOffset, bCube );                         Cudd_Ref(bOffset);
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );

        printf( "Trying %d output.\n", i );
        printf( "Onset = %d nodes.\n", Cudd_DagSize(bOnset) );
        printf( "Offset = %d nodes.\n", Cudd_DagSize(bOffset) );
    }

    Cudd_zddVarsFromBddVars( dd, 2 );

    // derive ISOP
    {
        extern int Abc_CountZddCubes( DdManager * dd, DdNode * zCover );
        DdNode * bCover, * zCover0, * zCover1;
        int nCubes0, nCubes1;
        // get the ZDD of the negative polarity
        bCover = Cudd_zddIsop( dd, bOffset, Cudd_Not(bOnset), &zCover0 );
        Cudd_Ref( zCover0 );
        Cudd_Ref( bCover );
        Cudd_RecursiveDeref( dd, bCover );
        nCubes0 = Abc_CountZddCubes( dd, zCover0 );

        // get the ZDD of the positive polarity
        bCover = Cudd_zddIsop( dd, bOnset, Cudd_Not(bOffset), &zCover1 );
        Cudd_Ref( zCover1 );
        Cudd_Ref( bCover );
        Cudd_RecursiveDeref( dd, bCover );
        nCubes1 = Abc_CountZddCubes( dd, zCover1 );

        // compare the number of cubes
        if ( nCubes1 <= nCubes0 )
        { // use positive polarity
            nCubes = nCubes1;
            zCover = zCover1;
            Cudd_RecursiveDerefZdd( dd, zCover0 );
            fPhase = 1;
        }
        else
        { // use negative polarity
            nCubes = nCubes0;
            zCover = zCover0;
            Cudd_RecursiveDerefZdd( dd, zCover1 );
            fPhase = 0;
        }
    }
    Cudd_RecursiveDeref( dd, bOnset );
    Cudd_RecursiveDeref( dd, bOffset );
    Cudd_RecursiveDerefZdd( dd, zCover );
    printf( "Cover = %d nodes.\n", Cudd_DagSize(zCover) );
    printf( "ISOP = %d\n", nCubes );

    // write the header
    fprintf( pFile, ".i %d\n", nInputs );
    fprintf( pFile, ".o %d\n", nOutputs );
    fprintf( pFile, ".ilb" );
    Abc_NtkForEachCi( pNtk, pNode, i )
        fprintf( pFile, " %s", Abc_ObjName(pNode) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".ob" );
    Abc_NtkForEachCo( pNtk, pNode, i )
        fprintf( pFile, " %s", Abc_ObjName(pNode) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".p %d\n", nCubes );


    fprintf( pFile, ".e\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the network in PLA format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteMoPlaOneIntMinterms( FILE * pFile, Abc_Ntk_t * pNtk, DdManager * dd, Vec_Ptr_t * vFuncs )
{
    int pValues[1000];
    Abc_Obj_t * pNode;
    int i, k, nProducts, nInputs, nOutputs;
    assert( Vec_PtrSize(vFuncs) == Abc_NtkCoNum(pNtk) );
    assert( dd->size == Abc_NtkCiNum(pNtk) );
    assert( dd->size <= 1000 );

    // collect the parameters
    nInputs   = Abc_NtkCiNum(pNtk);
    nOutputs  = Abc_NtkCoNum(pNtk);
    nProducts = (1 << nInputs);

    // write the header
    fprintf( pFile, ".i %d\n", nInputs );
    fprintf( pFile, ".o %d\n", nOutputs );
    fprintf( pFile, ".ilb" );
    Abc_NtkForEachCi( pNtk, pNode, i )
        fprintf( pFile, " %s", Abc_ObjName(pNode) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".ob" );
    Abc_NtkForEachCo( pNtk, pNode, i )
        fprintf( pFile, " %s", Abc_ObjName(pNode) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".p %d\n", nProducts );

    // iterate through minterms
    for ( k = 0; k < nProducts; k++ )
    {
        for ( i = 0; i < nInputs; i++ )
            fprintf( pFile, "%c", '0' + (pValues[i] = ((k >> i) & 1)) );
        fprintf( pFile, " " );
        for ( i = 0; i < nOutputs; i++ )
            fprintf( pFile, "%c", '0' + (Cudd_ReadOne(dd) == Cudd_Eval(dd, (DdNode *)Vec_PtrEntry(vFuncs, i), pValues)) );
        fprintf( pFile, "\n" );
    }

    fprintf( pFile, ".e\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the network in PLA format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteMoPlaOne( FILE * pFile, Abc_Ntk_t * pNtk )
{
    int fVerbose = 0;
    DdManager * dd;
    DdNode * bFunc;
    Vec_Ptr_t * vFuncsGlob;
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    dd = (DdManager *)Abc_NtkBuildGlobalBdds( pNtk, 10000000, 1, 1, 0, fVerbose );
    if ( dd == NULL )
        return 0;
    if ( fVerbose )
        printf( "Shared BDD size = %6d nodes.\n", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );

    // complement the global functions
    vFuncsGlob = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_PtrPush( vFuncsGlob, Abc_ObjGlobalBdd(pObj) );

    // consider minterms
    Io_WriteMoPlaOneIntMinterms( pFile, pNtk, dd, vFuncsGlob );
    Abc_NtkFreeGlobalBdds( pNtk, 0 );

    // cleanup
    Vec_PtrForEachEntry( DdNode *, vFuncsGlob, bFunc, i )
        Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrFree( vFuncsGlob );
    Extra_StopManager( dd );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the network in PLA format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteMoPla( Abc_Ntk_t * pNtk, char * pFileName )
{
    FILE * pFile;
    assert( Abc_NtkIsStrash(pNtk) );
    if ( Abc_NtkCiNum(pNtk) > 16 )
    {
        printf( "Cannot write multi-output PLA for more than 16 inputs.\n" );
        return 0;
    }    
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WritePla(): Cannot open the output file.\n" );
        return 0;
    }
    fprintf( pFile, "# Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    Io_WriteMoPlaOne( pFile, pNtk );
    fclose( pFile );
    return 1;
}



/**Function*************************************************************

  Synopsis    [Writes the network in PLA format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteMoPlaOneIntMintermsM( FILE * pFile, Abc_Ntk_t * pNtk, DdManager * dd, DdNode * bFunc, int nMints )
{
    Abc_Obj_t * pNode;
    int * pArray = ABC_CALLOC( int, dd->size );
    DdNode ** pbMints = Cudd_bddPickArbitraryMinterms( dd, bFunc, dd->vars, dd->size, nMints );
    int i, k, nInputs = Abc_NtkCiNum(pNtk);
    assert( dd->size == Abc_NtkCiNum(pNtk) );

    // write the header
    fprintf( pFile, ".i %d\n", nInputs );
    fprintf( pFile, ".o %d\n", 1 );
    fprintf( pFile, ".ilb" );
    Abc_NtkForEachCi( pNtk, pNode, i )
        fprintf( pFile, " %s", Abc_ObjName(pNode) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".ob" );
    fprintf( pFile, " %s", Abc_ObjName(Abc_NtkCo(pNtk, 0)) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".p %d\n", nMints );

    // iterate through minterms
    for ( k = 0; k < nMints; k++ )
    {
        Cudd_BddToCubeArray( dd, pbMints[k], pArray );
        for ( i = 0; i < Abc_NtkCiNum(pNtk); i++ )
            if ( pArray[i] == 0 )
                fprintf( pFile, "%c", '0' );
            else if ( pArray[i] == 1 )
                fprintf( pFile, "%c", '1' );
            else if ( pArray[i] == 2 )
                fprintf( pFile, "%c", '-' );
        fprintf( pFile, " " );
        fprintf( pFile, "%c", '1' );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, ".e\n" );

    //for ( k = 0; k < nMints; k++ )
    //    Cudd_RecursiveDeref( dd, pbMints[k] );
    ABC_FREE( pbMints );
    ABC_FREE( pArray );
    return 1;
}
int Io_WriteMoPlaOneM( FILE * pFile, Abc_Ntk_t * pNtk, int nMints )
{
    int fVerbose = 0;
    DdManager * dd;
    DdNode * bFunc;
    Vec_Ptr_t * vFuncsGlob;
    Abc_Obj_t * pObj;
    int i;
    if ( Abc_NtkIsStrash(pNtk) )
    {
        assert( Abc_NtkIsStrash(pNtk) );
        dd = (DdManager *)Abc_NtkBuildGlobalBdds( pNtk, 10000000, 1, 1, 0, fVerbose );
        if ( dd == NULL )
            return 0;
        if ( fVerbose )
            printf( "Shared BDD size = %6d nodes.\n", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );

        // complement the global functions
        vFuncsGlob = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
        Abc_NtkForEachCo( pNtk, pObj, i )
            Vec_PtrPush( vFuncsGlob, Abc_ObjGlobalBdd(pObj) );

        // get the output function
        bFunc = (DdNode *)Vec_PtrEntry(vFuncsGlob, 0);
        if ( bFunc == Cudd_ReadOne(dd) )
            printf( "First primary output has constant 1 function.\n" );
        else if ( Cudd_Not(bFunc) == Cudd_ReadOne(dd) )
            printf( "First primary output has constant 0 function.\n" );
        else
            Io_WriteMoPlaOneIntMintermsM( pFile, pNtk, dd, bFunc, nMints );
        Abc_NtkFreeGlobalBdds( pNtk, 0 );

        // cleanup
        Vec_PtrForEachEntry( DdNode *, vFuncsGlob, bFunc, i )
            Cudd_RecursiveDeref( dd, bFunc );
        Vec_PtrFree( vFuncsGlob );
        //Extra_StopManager( dd );
        Cudd_Quit( dd );
    }
    else if ( Abc_NtkIsBddLogic(pNtk) )
    {
        DdNode * bFunc = (DdNode *)Abc_ObjFanin0(Abc_NtkCo(pNtk, 0))->pData;
        dd = (DdManager *)pNtk->pManFunc;
        if ( dd->size == Abc_NtkCiNum(pNtk) )
            Io_WriteMoPlaOneIntMintermsM( pFile, pNtk, dd, bFunc, nMints );
        else
        {
            printf( "Cannot write minterms because the size of the manager for local BDDs is not equal to\n" );
            printf( "the number of primary inputs. (It is likely that the current network is not collapsed.)\n" );
        }
    }
    return 1;
}
int Io_WriteMoPlaM( Abc_Ntk_t * pNtk, char * pFileName, int nMints )
{
    FILE * pFile;
    assert( Abc_NtkIsStrash(pNtk) || Abc_NtkIsBddLogic(pNtk) );
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WriteMoPlaM(): Cannot open the output file.\n" );
        return 0;
    }
    fprintf( pFile, "# Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    Io_WriteMoPlaOneM( pFile, pNtk, nMints );
    fclose( pFile );
    return 1;
}


#else

int Io_WriteMoPla( Abc_Ntk_t * pNtk, char * pFileName )              { return 1; }
int Io_WriteMoPlaM( Abc_Ntk_t * pNtk, char * pFileName, int nMints ) { return 1; }

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

