/**CFile****************************************************************

  FileName    [abcAttach.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Attaches the library gates to the current network.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcAttach.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
#define    ATTACH_FULL             (~((unsigned)0))
#define    ATTACH_MASK(n)         ((~((unsigned)0)) >> (32-(n)))

static void Abc_AttachSetupTruthTables( unsigned uTruths[][2] );
static void Abc_AttachComputeTruth( char * pSop, unsigned uTruthsIn[][2], unsigned * uTruthNode );
static Mio_Gate_t * Abc_AttachFind( Mio_Gate_t ** ppGates, unsigned ** puTruthGates, int nGates, unsigned * uTruthNode, int * Perm );
static int Abc_AttachCompare( unsigned ** puTruthGates, int nGates, unsigned * uTruthNode );
static int Abc_NodeAttach( Abc_Obj_t * pNode, Mio_Gate_t ** ppGates, unsigned ** puTruthGates, int nGates, unsigned uTruths[][2] );
static void Abc_TruthPermute( char * pPerm, int nVars, unsigned * uTruthNode, unsigned * uTruthPerm );

static char ** s_pPerms = NULL;
static int s_nPerms;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Attaches gates from the current library to the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkAttach( Abc_Ntk_t * pNtk )
{
    Mio_Library_t * pGenlib;
    unsigned ** puTruthGates;
    unsigned uTruths[6][2];
    Abc_Obj_t * pNode;
    Mio_Gate_t ** ppGates;
    int nGates, nFanins, i;

    assert( Abc_NtkIsSopLogic(pNtk) );

    // check that the library is available
    pGenlib = (Mio_Library_t *)Abc_FrameReadLibGen();
    if ( pGenlib == NULL )
    {
        printf( "The current library is not available.\n" );
        return 0;
    }

    // start the truth tables
    Abc_AttachSetupTruthTables( uTruths );
    
    // collect all the gates
    ppGates = Mio_CollectRoots( pGenlib, 6, (float)1.0e+20, 1, &nGates, 0 );

    // derive the gate truth tables
    puTruthGates    = ABC_ALLOC( unsigned *, nGates );
    puTruthGates[0] = ABC_ALLOC( unsigned, 2 * nGates );
    for ( i = 1; i < nGates; i++ )
        puTruthGates[i] = puTruthGates[i-1] + 2;
    for ( i = 0; i < nGates; i++ )
        Mio_DeriveTruthTable( ppGates[i], uTruths, Mio_GateReadPinNum(ppGates[i]), 6, puTruthGates[i] );

    // assign the gates to pNode->pCopy
    Abc_NtkCleanCopy( pNtk );
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        nFanins = Abc_ObjFaninNum(pNode);
        if ( nFanins == 0 )
        {
            if ( Abc_SopIsConst1((char *)pNode->pData) )
                pNode->pCopy = (Abc_Obj_t *)Mio_LibraryReadConst1(pGenlib);
            else
                pNode->pCopy = (Abc_Obj_t *)Mio_LibraryReadConst0(pGenlib);
        }
        else if ( nFanins == 1 )
        {
            if ( Abc_SopIsBuf((char *)pNode->pData) )
                pNode->pCopy = (Abc_Obj_t *)Mio_LibraryReadBuf(pGenlib);
            else
                pNode->pCopy = (Abc_Obj_t *)Mio_LibraryReadInv(pGenlib);
        }
        else if ( nFanins > 6 )
        {
            printf( "Cannot attach gate with more than 6 inputs to node %s.\n", Abc_ObjName(pNode) );
            ABC_FREE( puTruthGates[0] );
            ABC_FREE( puTruthGates );
            ABC_FREE( ppGates );
            return 0;
        }
        else if ( !Abc_NodeAttach( pNode, ppGates, puTruthGates, nGates, uTruths ) )
        {
            printf( "Could not attach the library gate to node %s.\n", Abc_ObjName(pNode) );
            ABC_FREE( puTruthGates[0] );
            ABC_FREE( puTruthGates );
            ABC_FREE( ppGates );
            return 0;
        }
    }
    ABC_FREE( puTruthGates[0] );
    ABC_FREE( puTruthGates );
    ABC_FREE( ppGates );
    ABC_FREE( s_pPerms );

    // perform the final transformation
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if ( pNode->pCopy == NULL )
        {
            printf( "Some elementary gates (constant, buffer, or inverter) are missing in the library.\n" );
            return 0;
        }
    }

    // replace SOP representation by the gate representation
    Abc_NtkForEachNode( pNtk, pNode, i )
        pNode->pData = pNode->pCopy, pNode->pCopy = NULL;
    pNtk->ntkFunc = ABC_FUNC_MAP;
    Extra_MmFlexStop( (Extra_MmFlex_t *)pNtk->pManFunc );
    pNtk->pManFunc = pGenlib;

    printf( "Library gates are successfully attached to the nodes.\n" );

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkAttach: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeAttach( Abc_Obj_t * pNode, Mio_Gate_t ** ppGates, unsigned ** puTruthGates, int nGates, unsigned uTruths[][2] )
{
    int Perm[10];
    int pTempInts[10];
    unsigned uTruthNode[2];
    Abc_Obj_t * pFanin;
    Mio_Gate_t * pGate;
    int nFanins, i;

    // compute the node's truth table
    Abc_AttachComputeTruth( (char *)pNode->pData, uTruths, uTruthNode );
    // find the matching gate and permutation
    pGate = Abc_AttachFind( ppGates, puTruthGates, nGates, uTruthNode, Perm );
    if ( pGate == NULL )
        return 0;
    // permute the fanins
    nFanins = Abc_ObjFaninNum(pNode);
    Abc_ObjForEachFanin( pNode, pFanin, i )
        pTempInts[i] = pFanin->Id;
    for ( i = 0; i < nFanins; i++ )
        pNode->vFanins.pArray[Perm[i]] = pTempInts[i];
    // set the gate
    pNode->pCopy = (Abc_Obj_t *)pGate;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sets up the truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_AttachSetupTruthTables( unsigned uTruths[][2] )
{
    int m, v;
    for ( v = 0; v < 5; v++ )
        uTruths[v][0] = 0;
    // set up the truth tables
    for ( m = 0; m < 32; m++ )
        for ( v = 0; v < 5; v++ )
            if ( m & (1 << v) )
                uTruths[v][0] |= (1 << m);
    // make adjustments for the case of 6 variables
    for ( v = 0; v < 5; v++ )
        uTruths[v][1] = uTruths[v][0];
    uTruths[5][0] = 0;
    uTruths[5][1] = ATTACH_FULL;
}

/**Function*************************************************************

  Synopsis    [Compute the truth table of the node's cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_AttachComputeTruth( char * pSop, unsigned uTruthsIn[][2], unsigned * uTruthRes )
{
//    Mvc_Cube_t * pCube;
    unsigned uSignCube[2];
    int Value;
//    int nInputs = pCover->nBits/2;
    int nInputs = 6;
    int nFanins = Abc_SopGetVarNum(pSop);
    char * pCube;
    int k;

    // make sure that the number of input truth tables in equal to the number of gate inputs
    assert( nInputs < 7 );

    // clean the resulting truth table
    uTruthRes[0] = 0;
    uTruthRes[1] = 0;
    if ( nInputs < 6 )
    {
        // consider the case when only one unsigned can be used
//        Mvc_CoverForEachCube( pCover, pCube )
        Abc_SopForEachCube( pSop, nFanins, pCube )
        {
            uSignCube[0] = ATTACH_FULL;
//            Mvc_CubeForEachVarValue( pCover, pCube, Var, Value )
            Abc_CubeForEachVar( pCube, Value, k )
            {
                if ( Value == '0' )
                    uSignCube[0] &= ~uTruthsIn[k][0];
                else if ( Value == '1' )
                    uSignCube[0] &=  uTruthsIn[k][0];
            }
            uTruthRes[0] |= uSignCube[0];
        }
        if ( Abc_SopGetPhase(pSop) == 0 )
            uTruthRes[0] = ~uTruthRes[0];
        if ( nInputs < 5 )
            uTruthRes[0] &= ATTACH_MASK(1<<nInputs);
    }
    else
    {
        // consider the case when two unsigneds should be used
//        Mvc_CoverForEachCube( pCover, pCube )
        Abc_SopForEachCube( pSop, nFanins, pCube )
        {
            uSignCube[0] = ATTACH_FULL;
            uSignCube[1] = ATTACH_FULL;
//            Mvc_CubeForEachVarValue( pCover, pCube, Var, Value )
            Abc_CubeForEachVar( pCube, Value, k )
            {
                if ( Value == '0' )
                {
                    uSignCube[0] &= ~uTruthsIn[k][0];
                    uSignCube[1] &= ~uTruthsIn[k][1];
                }
                else if ( Value == '1' )
                {
                    uSignCube[0] &=  uTruthsIn[k][0];
                    uSignCube[1] &=  uTruthsIn[k][1];
                }
            }
            uTruthRes[0] |= uSignCube[0];
            uTruthRes[1] |= uSignCube[1];
        }

        // complement if the SOP is complemented
        if ( Abc_SopGetPhase(pSop) == 0 )
        {
            uTruthRes[0] = ~uTruthRes[0];
            uTruthRes[1] = ~uTruthRes[1];
        }
    }
}

/**Function*************************************************************

  Synopsis    [Find the gate by truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Gate_t * Abc_AttachFind( Mio_Gate_t ** ppGates, unsigned ** puTruthGates, int nGates, unsigned * uTruthNode, int * Perm )
{
    unsigned uTruthPerm[2];
    int i, v, iNum;

    // try the gates without permutation
    if ( (iNum = Abc_AttachCompare( puTruthGates, nGates, uTruthNode )) >= 0 )
    {
        for ( v = 0; v < 6; v++ )
            Perm[v] = v;
        return ppGates[iNum];
    }
    // get permutations
    if ( s_pPerms == NULL )
    {
        s_pPerms = Extra_Permutations( 6 );
        s_nPerms = Extra_Factorial( 6 );
    }
    // try permutations
    for ( i = 0; i < s_nPerms; i++ )
    {
        Abc_TruthPermute( s_pPerms[i], 6, uTruthNode, uTruthPerm );
        if ( (iNum = Abc_AttachCompare( puTruthGates, nGates, uTruthPerm )) >= 0 )
        {
            for ( v = 0; v < 6; v++ )
                Perm[v] = (int)s_pPerms[i][v];
            return ppGates[iNum];
        }
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Find the gate by truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_AttachCompare( unsigned ** puTruthGates, int nGates, unsigned * uTruthNode )
{
    int i;
    for ( i = 0; i < nGates; i++ )
        if ( puTruthGates[i][0] == uTruthNode[0] && puTruthGates[i][1] == uTruthNode[1] )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Permutes the 6-input truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_TruthPermute( char * pPerm, int nVars, unsigned * uTruthNode, unsigned * uTruthPerm )
{
    int nMints, iMintPerm, iMint, v;
    uTruthPerm[0] = uTruthPerm[1] = 0;
    nMints = (1 << nVars);
    for ( iMint = 0; iMint < nMints; iMint++ )
    {
        if ( (uTruthNode[iMint>>5] & (1 << (iMint&31))) == 0 )
            continue;
        iMintPerm = 0;
        for ( v = 0; v < nVars; v++ )
            if ( iMint & (1 << v) )
                iMintPerm |= (1 << pPerm[v]);
        uTruthPerm[iMintPerm>>5] |= (1 << (iMintPerm&31));     
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

