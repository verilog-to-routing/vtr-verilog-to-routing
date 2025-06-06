/**CFile****************************************************************

  FileName    [fxuCreate.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Create matrix from covers and covers from matrix.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxuCreate.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h"
#include "fxu.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void        Fxu_CreateMatrixAddCube( Fxu_Matrix * p, Fxu_Cube * pCube, char * pSopCube, Vec_Int_t * vFanins, int * pOrder );
static int         Fxu_CreateMatrixLitCompare( int * ptrX, int * ptrY );
static void        Fxu_CreateCoversNode( Fxu_Matrix * p, Fxu_Data_t * pData, int iNode, Fxu_Cube * pCubeFirst, Fxu_Cube * pCubeNext );
static Fxu_Cube *  Fxu_CreateCoversFirstCube( Fxu_Matrix * p, Fxu_Data_t * pData, int iNode );
static int * s_pLits;

extern int         Fxu_PreprocessCubePairs( Fxu_Matrix * p, Vec_Ptr_t * vCovers, int nPairsTotal, int nPairsMax );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the sparse matrix from the array of SOPs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Matrix * Fxu_CreateMatrix( Fxu_Data_t * pData )
{
    Fxu_Matrix * p;
    Fxu_Var * pVar;
    Fxu_Cube * pCubeFirst, * pCubeNew;
    Fxu_Cube * pCube1, * pCube2;
    Vec_Int_t * vFanins;
    char * pSopCover;
    char * pSopCube;
    int * pOrder, nBitsMax;
    int i, v, c;
    int nCubesTotal;
    int nPairsTotal;
    int nPairsStore;
    int nCubes;
    int iCube, iPair;
    int nFanins;

    // collect all sorts of statistics
    nCubesTotal =  0;
    nPairsTotal =  0;
    nPairsStore =  0;
    nBitsMax    = -1; 
    for ( i = 0; i < pData->nNodesOld; i++ )
        if ( (pSopCover = (char *)pData->vSops->pArray[i]) )
        {
            nCubes       = Abc_SopGetCubeNum( pSopCover );
            nFanins      = Abc_SopGetVarNum( pSopCover );
            assert( nFanins > 1 && nCubes > 0 );

            nCubesTotal += nCubes;
            nPairsTotal += nCubes * (nCubes - 1) / 2;
            nPairsStore += nCubes * nCubes;
            if ( nBitsMax < nFanins )
                nBitsMax = nFanins;
        }
    if ( nBitsMax <= 0 )
    {
        printf( "The current network does not have SOPs to perform extraction.\n" );
        return NULL;
    }

    if ( nPairsStore > 50000000 )
    {
        printf( "The problem is too large to be solved by \"fxu\" (%d cubes and %d cube pairs)\n", nCubesTotal, nPairsStore );
        return NULL;
    }

    // start the matrix
    p = Fxu_MatrixAllocate();
    // create the column labels 
    p->ppVars = ABC_ALLOC( Fxu_Var *, 2 * (pData->nNodesOld + pData->nNodesExt) );
    for ( i = 0; i < 2 * pData->nNodesOld; i++ )
        p->ppVars[i] = Fxu_MatrixAddVar( p );

    // allocate storage for all cube pairs at once
    p->pppPairs = ABC_ALLOC( Fxu_Pair **, nCubesTotal + 100 );
    p->ppPairs  = ABC_ALLOC( Fxu_Pair *,  nPairsStore + 100 );
    memset( p->ppPairs, 0, sizeof(Fxu_Pair *) * nPairsStore );
    iCube = 0;
    iPair = 0;
    for ( i = 0; i < pData->nNodesOld; i++ )
        if ( (pSopCover = (char *)pData->vSops->pArray[i]) )
        {
            // get the number of cubes
            nCubes = Abc_SopGetCubeNum( pSopCover );
            // get the new var in the matrix
            pVar = p->ppVars[2*i+1];
            // assign the pair storage
            pVar->nCubes     = nCubes;
            if ( nCubes > 0 )
            {
                pVar->ppPairs    = p->pppPairs + iCube;
                pVar->ppPairs[0] = p->ppPairs  + iPair;
                for ( v = 1; v < nCubes; v++ )
                    pVar->ppPairs[v] = pVar->ppPairs[v-1] + nCubes;
            }
            // update
            iCube += nCubes;
            iPair += nCubes * nCubes;
        }
    assert( iCube == nCubesTotal );
    assert( iPair == nPairsStore );


    // allocate room for the reordered literals
    pOrder = ABC_ALLOC( int, nBitsMax );
    // create the rows
    for ( i = 0; i < pData->nNodesOld; i++ )
    if ( (pSopCover = (char *)pData->vSops->pArray[i]) )
    {
        // get the new var in the matrix
        pVar = p->ppVars[2*i+1];
        // here we sort the literals of the cover
        // in the increasing order of the numbers of the corresponding nodes
        // because literals should be added to the matrix in this order
        vFanins = (Vec_Int_t *)pData->vFanins->pArray[i];
        s_pLits = vFanins->pArray;
        // start the variable order
        nFanins = Abc_SopGetVarNum( pSopCover );
        for ( v = 0; v < nFanins; v++ )
            pOrder[v] = v;
        // reorder the fanins
        qsort( (void *)pOrder, (size_t)nFanins, sizeof(int),(int (*)(const void *, const void *))Fxu_CreateMatrixLitCompare);
        assert( s_pLits[ pOrder[0] ] < s_pLits[ pOrder[nFanins-1] ] );
        // create the corresponding cubes in the matrix
        pCubeFirst = NULL;
        c = 0;
        Abc_SopForEachCube( pSopCover, nFanins, pSopCube )
        {
            // create the cube
            pCubeNew = Fxu_MatrixAddCube( p, pVar, c++ );
            Fxu_CreateMatrixAddCube( p, pCubeNew, pSopCube, vFanins, pOrder );
            if ( pCubeFirst == NULL )
                pCubeFirst = pCubeNew;
            pCubeNew->pFirst = pCubeFirst;
        }
        // set the first cube of this var
        pVar->pFirst = pCubeFirst;
        // create the divisors without preprocessing
        if ( nPairsTotal <= pData->nPairsMax )
        {
            for ( pCube1 = pCubeFirst; pCube1; pCube1 = pCube1->pNext )
                for ( pCube2 = pCube1? pCube1->pNext: NULL; pCube2; pCube2 = pCube2->pNext )
                    Fxu_MatrixAddDivisor( p, pCube1, pCube2 );
        }
    }
    ABC_FREE( pOrder );

    // consider the case when cube pairs should be preprocessed
    // before adding them to the set of divisors
//    if ( pData->fVerbose )
//        printf( "The total number of cube pairs is %d.\n", nPairsTotal );
    if ( nPairsTotal > 10000000 )
    {
        printf( "The total number of cube pairs of the network is more than 10,000,000.\n" );
        printf( "Command \"fx\" takes a long time to run in such cases. It is suggested\n" );
        printf( "that the user changes the network by reducing the size of logic node and\n" );
        printf( "consequently the number of cube pairs to be processed by this command.\n" );
        printf( "It can be achieved as follows: \"st; if -K <num>\" or \"st; renode -s -K <num>\"\n" );
        printf( "as a proprocessing step, while selecting <num> as approapriate.\n" );
        return NULL;
    }
    if ( nPairsTotal > pData->nPairsMax )
        if ( !Fxu_PreprocessCubePairs( p, pData->vSops, nPairsTotal, pData->nPairsMax ) )
            return NULL;
//    if ( pData->fVerbose )
//        printf( "Only %d best cube pairs will be used by the fast extract command.\n", pData->nPairsMax );

    if ( p->lVars.nItems > 1000000 )
    {
        printf( "The total number of variables is more than 1,000,000.\n" );
        printf( "Command \"fx\" takes a long time to run in such cases. It is suggested\n" );
        printf( "that the user changes the network by reducing the size of logic node and\n" );
        printf( "consequently the number of cube pairs to be processed by this command.\n" );
        printf( "It can be achieved as follows: \"st; if -K <num>\" or \"st; renode -s -K <num>\"\n" );
        printf( "as a proprocessing step, while selecting <num> as approapriate.\n" );
        return NULL;
    }


    // add the var pairs to the heap
    Fxu_MatrixComputeSingles( p, pData->fUse0, pData->nSingleMax );

    // print stats
    if ( pData->fVerbose )
    {
        double Density;
        Density = ((double)p->nEntries) / p->lVars.nItems / p->lCubes.nItems;
        fprintf( stdout, "Matrix: [vars x cubes] = [%d x %d]  ",
            p->lVars.nItems, p->lCubes.nItems );
        fprintf( stdout, "Lits = %d  Density = %.5f%%\n", 
            p->nEntries, Density );
        fprintf( stdout, "1-cube divs = %6d. (Total = %6d)  ",  p->lSingles.nItems, p->nSingleTotal );
        fprintf( stdout, "2-cube divs = %6d. (Total = %6d)",  p->nDivsTotal, nPairsTotal );
        fprintf( stdout, "\n" );
    }
//    Fxu_MatrixPrint( stdout, p );
    return p;
}

/**Function*************************************************************

  Synopsis    [Adds one cube with literals to the matrix.]

  Description [Create the cube and literals in the matrix corresponding 
  to the given cube in the SOP cover. Co-singleton transform is performed here.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_CreateMatrixAddCube( Fxu_Matrix * p, Fxu_Cube * pCube, char * pSopCube, Vec_Int_t * vFanins, int * pOrder )
{
    Fxu_Var * pVar;
    int Value, i;
    // add literals to the matrix
    Abc_CubeForEachVar( pSopCube, Value, i )
    {
        Value = pSopCube[pOrder[i]];
        if ( Value == '0' )
        {
            pVar = p->ppVars[ 2 * vFanins->pArray[pOrder[i]] + 1 ];  // CST
            Fxu_MatrixAddLiteral( p, pCube, pVar );
        }
        else if ( Value == '1' )
        {
            pVar = p->ppVars[ 2 * vFanins->pArray[pOrder[i]] ];  // CST
            Fxu_MatrixAddLiteral( p, pCube, pVar );
        }
    }
}


/**Function*************************************************************

  Synopsis    [Creates the new array of Sop covers from the sparse matrix.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_CreateCovers( Fxu_Matrix * p, Fxu_Data_t * pData )
{
    Fxu_Cube * pCube, * pCubeFirst, * pCubeNext;
    char * pSopCover;
    int iNode, n;

    // get the first cube of the first internal node 
    pCubeFirst = Fxu_CreateCoversFirstCube( p, pData, 0 );

    // go through the internal nodes
    for ( n = 0; n < pData->nNodesOld; n++ )
    if ( (pSopCover = (char *)pData->vSops->pArray[n]) )
    {
        // get the number of this node
        iNode = n;
        // get the next first cube
        pCubeNext = Fxu_CreateCoversFirstCube(  p, pData, iNode + 1 );
        // check if there any new variables in these cubes
        for ( pCube = pCubeFirst; pCube != pCubeNext; pCube = pCube->pNext )
            if ( pCube->lLits.pTail && pCube->lLits.pTail->iVar >= 2 * pData->nNodesOld )
                break;
        if ( pCube != pCubeNext )
            Fxu_CreateCoversNode( p, pData, iNode, pCubeFirst, pCubeNext );
        // update the first cube
        pCubeFirst = pCubeNext;
    }

    // add the covers for the extracted nodes
    for ( n = 0; n < pData->nNodesNew; n++ )
    {
        // get the number of this node
        iNode = pData->nNodesOld + n;
        // get the next first cube
        pCubeNext = Fxu_CreateCoversFirstCube( p, pData, iNode + 1 );
        // the node should be added
        Fxu_CreateCoversNode( p, pData, iNode, pCubeFirst, pCubeNext );
        // update the first cube
        pCubeFirst = pCubeNext;
    }
}

/**Function*************************************************************

  Synopsis    [Create Sop covers for one node that has changed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_CreateCoversNode( Fxu_Matrix * p, Fxu_Data_t * pData, int iNode, Fxu_Cube * pCubeFirst, Fxu_Cube * pCubeNext )
{
    Vec_Int_t * vInputsNew;
    char * pSopCover, * pSopCube;
    Fxu_Var * pVar;
    Fxu_Cube * pCube;
    Fxu_Lit * pLit;
    int iNum, nCubes, v;

    // collect positive polarity variable in the cubes between pCubeFirst and pCubeNext
    Fxu_MatrixRingVarsStart( p );
    for ( pCube = pCubeFirst; pCube != pCubeNext; pCube = pCube->pNext )
        for ( pLit = pCube->lLits.pHead; pLit; pLit = pLit->pHNext )
        {
            pVar = p->ppVars[ 2 * (pLit->pVar->iVar/2) + 1 ];
            if ( pVar->pOrder == NULL )
                Fxu_MatrixRingVarsAdd( p, pVar );
        }
    Fxu_MatrixRingVarsStop( p );

    // collect the variable numbers
    vInputsNew = Vec_IntAlloc( 4 );
    Fxu_MatrixForEachVarInRing( p, pVar )
        Vec_IntPush( vInputsNew, pVar->iVar / 2 );
    Fxu_MatrixRingVarsUnmark( p );

    // sort the vars by their number
    Vec_IntSort( vInputsNew, 0 );

    // mark the vars with their numbers in the sorted array
    for ( v = 0; v < vInputsNew->nSize; v++ )
    {
        p->ppVars[ 2 * vInputsNew->pArray[v] + 0 ]->lLits.nItems = v; // hack - reuse lLits.nItems
        p->ppVars[ 2 * vInputsNew->pArray[v] + 1 ]->lLits.nItems = v; // hack - reuse lLits.nItems
    }

    // count the number of cubes
    nCubes = 0;
    for ( pCube = pCubeFirst; pCube != pCubeNext; pCube = pCube->pNext )
        if ( pCube->lLits.nItems )
            nCubes++;

    // allocate room for the new cover
    pSopCover = Abc_SopStart( pData->pManSop, nCubes, vInputsNew->nSize );
    // set the correct polarity of the cover
    if ( iNode < pData->nNodesOld && Abc_SopGetPhase( (char *)pData->vSops->pArray[iNode] ) == 0 )
        Abc_SopComplement( pSopCover );

    // add the cubes
    nCubes = 0;
    for ( pCube = pCubeFirst; pCube != pCubeNext; pCube = pCube->pNext )
    {
        if ( pCube->lLits.nItems == 0 )
            continue;
        // get hold of the SOP cube
        pSopCube = pSopCover + nCubes * (vInputsNew->nSize + 3);
        // insert literals
        for ( pLit = pCube->lLits.pHead; pLit; pLit = pLit->pHNext )
        {
            iNum = pLit->pVar->lLits.nItems; // hack - reuse lLits.nItems
            assert( iNum < vInputsNew->nSize );
            if ( pLit->pVar->iVar / 2 < pData->nNodesOld )
                pSopCube[iNum] = (pLit->pVar->iVar & 1)? '0' : '1';   // reverse CST
            else
                pSopCube[iNum] = (pLit->pVar->iVar & 1)? '1' : '0';   // no CST
        }
        // count the cube
        nCubes++;
    }
    assert( nCubes == Abc_SopGetCubeNum(pSopCover) );

    // set the new cover and the array of fanins
    pData->vSopsNew->pArray[iNode]   = pSopCover;
    pData->vFaninsNew->pArray[iNode] = vInputsNew;
}


/**Function*************************************************************

  Synopsis    [Adds the var to storage.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxu_Cube * Fxu_CreateCoversFirstCube( Fxu_Matrix * p, Fxu_Data_t * pData, int iVar )
{
    int v;
    for ( v = iVar; v < pData->nNodesOld + pData->nNodesNew; v++ )
        if ( p->ppVars[ 2*v + 1 ]->pFirst )
            return p->ppVars[ 2*v + 1 ]->pFirst;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Compares the vars by their number.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_CreateMatrixLitCompare( int * ptrX, int * ptrY )
{
    return s_pLits[*ptrX] - s_pLits[*ptrY];
} 

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

