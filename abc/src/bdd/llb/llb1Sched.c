/**CFile****************************************************************

  FileName    [llb1Sched.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Partition scheduling algorithm.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb1Sched.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Swaps two rows.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrSwapColumns( Llb_Mtr_t * p, int iCol1, int iCol2 )
{
    Llb_Grp_t * pGemp;
    char * pTemp;
    int iTemp;
    assert( iCol1 >= 0 && iCol1 < p->nCols );
    assert( iCol2 >= 0 && iCol2 < p->nCols );
    if ( iCol1 == iCol2 )
        return;
    assert( iCol1 != iCol2 );
    // swap col groups
    pGemp = p->pColGrps[iCol1];
    p->pColGrps[iCol1] = p->pColGrps[iCol2];
    p->pColGrps[iCol2] = pGemp;
    // swap col vectors
    pTemp = p->pMatrix[iCol1];
    p->pMatrix[iCol1] = p->pMatrix[iCol2];
    p->pMatrix[iCol2] = pTemp;
    // swap col sums
    iTemp = p->pColSums[iCol1];
    p->pColSums[iCol1] = p->pColSums[iCol2];
    p->pColSums[iCol2] = iTemp;
}

/**Function*************************************************************

  Synopsis    [Find columns which brings as few vars as possible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_MtrFindBestColumn( Llb_Mtr_t * p, int iGrpStart )
{
    int Cost, Cost2, CostBest = ABC_INFINITY, Cost2Best = ABC_INFINITY;
    int WeightCur, WeightBest = -ABC_INFINITY, iGrp = -1, iGrpBest = -1;
    int k, c, iVar, Counter;
    // find partition that reduces partial product as much as possible
    for ( iVar = 0; iVar < p->nRows - p->nFfs; iVar++ )
    { 
        if ( p->pRowSums[iVar] < 2 )
            continue;
        // look at present variables that can be quantified
        if ( !(p->pProdVars[iVar] == 1 && p->pProdNums[iVar] == 1) )
            continue;
        // check that it appears in one partition only
        Counter = 0;
        for ( c = iGrpStart; c < p->nCols-1; c++ )
            if ( p->pMatrix[c][iVar] == 1 )
            {
                iGrp = c;
                Counter++;
            }
        assert( Counter == 1 );
        if ( Counter != 1 )
            Abc_Print( -1, "Llb_MtrFindBestColumn() Internal error!\n" );
        // find weight of this column
        WeightCur = 0;
        for ( k = 0; k < p->nRows; k++ )
        {
            // increase weight if variable k will be quantified from partial product
            if ( p->pProdVars[k] == 1 && p->pMatrix[iGrp][k] == 1 && p->pProdNums[k] == 1 ) 
                WeightCur += 2;
            // decrease weight if variable k will be added to partial product
            if ( p->pProdVars[k] == 0 && p->pMatrix[iGrp][k] == 1 )
                WeightCur--;
        }
        if ( WeightCur > 0 && WeightBest < WeightCur )
        {
            WeightBest = WeightCur;
            iGrpBest   = iGrp;
        }
    }
    if ( iGrpBest >= 0 )
        return iGrpBest;
    // could not find the group with any vars to quantify
    // select the group that contains as few extra variables as possible
    // if there is a tie, select variables that appear in less groups than others
    for ( iGrp = iGrpStart; iGrp < p->nCols-1; iGrp++ )
    {
        Cost = Cost2 = 0;
        for ( k = 0; k < p->nRows; k++ )
            if ( p->pProdVars[k] == 0 && p->pMatrix[iGrp][k] == 1 )
            {
                Cost++;
                Cost2 += p->pProdNums[k];
            }
        if ( CostBest >  Cost || 
            (CostBest == Cost && Cost2 > Cost2Best) )
        {
            CostBest  = Cost;
            Cost2Best = Cost2;
            iGrpBest  = iGrp;
        }
    }
    return iGrpBest;
}

/**Function*************************************************************

  Synopsis    [Returns the number of variables that will be saved.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrUseSelectedColumn( Llb_Mtr_t * p, int iCol )
{
    int iVar;
    assert( iCol >= 1 && iCol < p->nCols - 1 );
    for ( iVar = 0; iVar < p->nRows; iVar++ )
    {
        if ( p->pMatrix[iCol][iVar] == 0 )
            continue;
        if ( p->pProdVars[iVar] == 1 && p->pProdNums[iVar] == 1 )
        {
            p->pProdVars[iVar] = 0;
            p->pProdNums[iVar] = 0;
            continue;
        }
        if ( p->pProdVars[iVar] == 0 )
        {
            p->pProdVars[iVar] = 1;
            p->pProdNums[iVar] = p->pRowSums[iVar];
        }
        p->pProdNums[iVar]--;
        assert( p->pProdNums[iVar] >= 0 );
        if ( p->pProdNums[iVar] < 0 )
            Abc_Print( -1, "Llb_MtrUseSelectedColumn() Internal error!\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Verify columns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrVerifyColumns( Llb_Mtr_t * p, int iGrpStart )
{
    int iVar, iGrp, Counter;
    for ( iVar = 0; iVar < p->nRows; iVar++ )
    {
        if ( p->pProdVars[iVar] == 0 )
            continue;
        Counter = 0;
        for ( iGrp = iGrpStart; iGrp < p->nCols; iGrp++ )
            if ( p->pMatrix[iGrp][iVar] == 1 )
                Counter++;
        assert( Counter == p->pProdNums[iVar] );
        if ( Counter != p->pProdNums[iVar] )
            Abc_Print( -1, "Llb_MtrVerifyColumns(): Internal error.\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Matrix reduce.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrSchedule( Llb_Mtr_t * p )
{
    int iGrp, iGrpBest, i;
    // start partial product
    for ( i = 0; i < p->nRows; i++ )
    {
        if ( i >= p->nPis && i < p->nPis + p->nFfs )
        {
            p->pProdVars[i] = 1;
            p->pProdNums[i] = p->pRowSums[i] - 1;
        }
        else 
        {
            p->pProdVars[i] = 0;
            p->pProdNums[i] = p->pRowSums[i];
        }
    }
    // order the partitions
    Llb_MtrVerifyMatrix( p );
    for ( iGrp = 1; iGrp < p->nCols-1; iGrp++ )
    {
        Llb_MtrVerifyColumns( p, iGrp );
        iGrpBest = Llb_MtrFindBestColumn( p, iGrp );
        Llb_MtrUseSelectedColumn( p, iGrpBest );
        Llb_MtrSwapColumns( p, iGrp, iGrpBest );
    }
    Llb_MtrVerifyMatrix( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

