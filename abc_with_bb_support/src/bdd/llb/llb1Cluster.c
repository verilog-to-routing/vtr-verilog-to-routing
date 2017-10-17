/**CFile****************************************************************

  FileName    [llb1Cluster.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Clustering algorithm.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]
 
  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb1Cluster.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManComputeCommonQuant( Llb_Mtr_t * p, int iCol1, int iCol2 )
{
    int iVar, Weight = 0;
    for ( iVar = 0; iVar < p->nRows - p->nFfs; iVar++ )
    {
        // count each removed variable as 2
        if ( p->pMatrix[iCol1][iVar] == 1 && p->pMatrix[iCol2][iVar] == 1 && p->pRowSums[iVar] == 2 )
            Weight += 2;
        // count each added variale as -1
        else if ( (p->pMatrix[iCol1][iVar] == 1 && p->pMatrix[iCol2][iVar] == 0) || 
                  (p->pMatrix[iCol1][iVar] == 0 && p->pMatrix[iCol2][iVar] == 1) )
            Weight--;
    }
    return Weight;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManComputeBestQuant( Llb_Mtr_t * p )
{
    int i, k, WeightBest = -100000, WeightCur, RetValue = -1;
    for ( i = 1; i < p->nCols-1; i++ )
    for ( k = i+1; k < p->nCols-1; k++ )
    {
        if ( p->pColSums[i] == 0 || p->pColSums[i] > p->pMan->pPars->nClusterMax )
            continue;
        if ( p->pColSums[k] == 0 || p->pColSums[k] > p->pMan->pPars->nClusterMax )
            continue;

        WeightCur = Llb_ManComputeCommonQuant( p, i, k );
        if ( WeightCur <= 0 )
            continue;
        if ( WeightBest < WeightCur )
        {
            WeightBest = WeightCur;
            RetValue   = (i << 16) | k;
        }
    }
//    printf( "Choosing best quant Weight %4d\n", WeightCur );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float ** Llb_ManComputeQuant( Llb_Mtr_t * p )
{
    float ** pCosts;
    int i, k;
    // alloc and clean
    pCosts = (float **)Extra_ArrayAlloc( p->nCols, p->nCols, sizeof(float) );
    for ( i = 0; i < p->nCols; i++ )
    for ( k = 0; k < p->nCols; k++ )
        pCosts[i][i] = 0.0;
    // fill up
    for ( i = 1; i < p->nCols-1; i++ )
    for ( k = i+1; k < p->nCols-1; k++ )
        pCosts[i][k] = pCosts[k][i] = Llb_ManComputeCommonQuant( p, i, k );
    return pCosts;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Llb_ManComputeCommonAttr( Llb_Mtr_t * p, int iCol1, int iCol2 )
{
    int iVar, CountComm = 0, CountDiff = 0;
    for ( iVar = 0; iVar < p->nRows - p->nFfs; iVar++ )
    {
        if ( p->pMatrix[iCol1][iVar] == 1 && p->pMatrix[iCol2][iVar] == 1 )
            CountComm++;
        else if ( p->pMatrix[iCol1][iVar] == 1 || p->pMatrix[iCol2][iVar] == 1 )
            CountDiff++;
    }
/*
    printf( "Attr cost for %4d and %4d:  %4d %4d (%5.2f)\n", 
        iCol1, iCol2, 
        CountDiff, CountComm, 
        -1.0 * CountDiff / ( CountComm + CountDiff ) );
*/
    return -1.0 * CountDiff / ( CountComm + CountDiff );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManComputeBestAttr( Llb_Mtr_t * p )
{
    float WeightBest = -100000, WeightCur;
    int i, k, RetValue = -1;
    for ( i = 1; i < p->nCols-1; i++ )
    for ( k = i+1; k < p->nCols-1; k++ )
    {
        if ( p->pColSums[i] == 0 || p->pColSums[i] > p->pMan->pPars->nClusterMax )
            continue;
        if ( p->pColSums[k] == 0 || p->pColSums[k] > p->pMan->pPars->nClusterMax )
            continue;
        WeightCur = Llb_ManComputeCommonAttr( p, i, k );
        if ( WeightBest < WeightCur )
        {
            WeightBest = WeightCur;
            RetValue   = (i << 16) | k;
        }
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float ** Llb_ManComputeAttr( Llb_Mtr_t * p )
{
    float ** pCosts;
    int i, k;
    // alloc and clean
    pCosts = (float **)Extra_ArrayAlloc( p->nCols, p->nCols, sizeof(float) );
    for ( i = 0; i < p->nCols; i++ )
    for ( k = 0; k < p->nCols; k++ )
        pCosts[i][i] = 0.0;
    // fill up
    for ( i = 1; i < p->nCols-1; i++ )
    for ( k = i+1; k < p->nCols-1; k++ )
        pCosts[i][k] = pCosts[k][i] = Llb_ManComputeCommonAttr( p, i, k );
    return pCosts;
}


/**Function*************************************************************

  Synopsis    [Returns the number of variables that will be saved.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrCombineSelectedColumns( Llb_Mtr_t * p, int iGrp1, int iGrp2 )
{
    int iVar;
    assert( iGrp1 >= 1 && iGrp1 < p->nCols - 1 );
    assert( iGrp2 >= 1 && iGrp2 < p->nCols - 1 );
    assert( p->pColGrps[iGrp1] != NULL );
    assert( p->pColGrps[iGrp2] != NULL );
    for ( iVar = 0; iVar < p->nRows; iVar++ )
    {
        if ( p->pMatrix[iGrp1][iVar] == 1 && p->pMatrix[iGrp2][iVar] == 1 )
            p->pRowSums[iVar]--;
        if ( p->pMatrix[iGrp1][iVar] == 0 && p->pMatrix[iGrp2][iVar] == 1 )
        {
            p->pMatrix[iGrp1][iVar] = 1;
            p->pColSums[iGrp1]++;
        }
        if ( p->pMatrix[iGrp2][iVar] == 1 )
            p->pMatrix[iGrp2][iVar] = 0;
    }
    p->pColSums[iGrp2] = 0;
}


/**Function*************************************************************

  Synopsis    [Combines one pair of columns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManClusterOne( Llb_Mtr_t * p, int iCol1, int iCol2 )
{
    int fVerbose = 0;
    Llb_Grp_t * pGrp;
    int iVar;

    if ( fVerbose )
    {
        printf( "Combining %d and %d\n", iCol1, iCol2 );
        for ( iVar = 0; iVar < p->nRows; iVar++ )
        {
            if ( p->pMatrix[iCol1][iVar] == 0 && p->pMatrix[iCol2][iVar] == 0 )
                continue;
            printf( "%3d : %c%c\n", iVar, 
                p->pMatrix[iCol1][iVar]? '*':' ', 
                p->pMatrix[iCol2][iVar]? '*':' ' );
        }
    }
    pGrp = Llb_ManGroupsCombine( p->pColGrps[iCol1], p->pColGrps[iCol2] );
    Llb_MtrCombineSelectedColumns( p, iCol1, iCol2 );
    p->pColGrps[iCol1] = pGrp;
    p->pColGrps[iCol2] = NULL;
}

/**Function*************************************************************

  Synopsis    [Removes empty columns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManClusterCompress( Llb_Mtr_t * p )
{
    int i, k = 0;
    for ( i = 0; i < p->nCols; i++ )
    {
        if ( p->pColGrps[i] == NULL )
        {
            assert( p->pColSums[i] == 0 );
            assert( p->pMatrix[i] != NULL );
            ABC_FREE( p->pMatrix[i] );
            continue;
        }
        p->pMatrix[k]  = p->pMatrix[i];
        p->pColGrps[k] = p->pColGrps[i];
        p->pColSums[k] = p->pColSums[i];
        k++;
    }
    p->nCols = k;
}

/**Function*************************************************************

  Synopsis    [Combines one pair of columns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManCluster( Llb_Mtr_t * p )
{
    int RetValue;
    do
    {
        do {
            RetValue = Llb_ManComputeBestQuant( p );
            if ( RetValue > 0 )
                Llb_ManClusterOne( p, RetValue >> 16, RetValue & 0xffff );
        }
        while ( RetValue > 0 );

        RetValue = Llb_ManComputeBestAttr( p );
        if ( RetValue > 0 )
            Llb_ManClusterOne( p, RetValue >> 16, RetValue & 0xffff );

        Llb_MtrVerifyMatrix( p );
    }
    while ( RetValue > 0 );

    Llb_ManClusterCompress( p );

    Llb_MtrVerifyMatrix( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

