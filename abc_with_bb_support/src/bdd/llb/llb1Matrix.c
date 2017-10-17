/**CFile****************************************************************

  FileName    [llb1Matrix.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Partition clustering as a matrix problem.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb1Matrix.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

//              0123              nCols
//             +--------------------->  
// pi        0 |  111             row0   pRowSums[0]  
// pi        1 | 1     11         row1   pRowSums[1] 
// pi        2 |   1  11          row2   pRowSums[2] 
// CS          |1       1
// CS          |1      111
// CS          |111    111
// int         |  11111           
// int         |     111          
// int         |       111        
// int         |         111      
// NS          |           11 11
// NS          |            11 1
// NS          |             111
//       nRows |
//             v    
//              cccc   pColSums[0]
//              oooo   pColSums[1]
//              llll   pColSums[2]
//              0123   pColSums[3]

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Verify columns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrVerifyRowsAll( Llb_Mtr_t * p )
{
    int iRow, iCol, Counter;
    for ( iCol = 0; iCol < p->nCols; iCol++ )
    {
        Counter = 0;
        for ( iRow = 0; iRow < p->nRows; iRow++ )
            if ( p->pMatrix[iCol][iRow] == 1 )
                Counter++;
        assert( Counter == p->pColSums[iCol] );
    }
}

/**Function*************************************************************

  Synopsis    [Verify columns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrVerifyColumnsAll( Llb_Mtr_t * p )
{
    int iRow, iCol, Counter;
    for ( iRow = 0; iRow < p->nRows; iRow++ )
    {
        Counter = 0;
        for ( iCol = 0; iCol < p->nCols; iCol++ )
            if ( p->pMatrix[iCol][iRow] == 1 )
                Counter++;
        assert( Counter == p->pRowSums[iRow] );
    }
}

/**Function*************************************************************

  Synopsis    [Verify columns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrVerifyMatrix( Llb_Mtr_t * p )
{
    Llb_MtrVerifyRowsAll( p );
    Llb_MtrVerifyColumnsAll( p );
}

/**Function*************************************************************

  Synopsis    [Sort variables in the order of removal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Llb_MtrFindVarOrder( Llb_Mtr_t * p )
{
    int * pOrder, * pLast;
    int i, k, fChanges, Temp;
    pOrder = ABC_CALLOC( int, p->nRows );
    pLast  = ABC_CALLOC( int, p->nRows );
    for ( i = 0; i < p->nRows; i++ )
    {
        pOrder[i] = i;
        for ( k = p->nCols - 1; k >= 0; k-- )
            if ( p->pMatrix[k][i] )
            {
                pLast[i] = k;
                break;
            }
    }
    do
    {
        fChanges = 0;
        for ( i = 0; i < p->nRows - 1; i++ )
            if ( pLast[i] > pLast[i+1] )
            {
                Temp = pOrder[i];
                pOrder[i] = pOrder[i+1];
                pOrder[i+1] = Temp;

                Temp = pLast[i];
                pLast[i] = pLast[i+1];
                pLast[i+1] = Temp;

                fChanges = 1;
            }
    }
    while ( fChanges );
    ABC_FREE( pLast );
    return pOrder;
}

/**Function*************************************************************

  Synopsis    [Returns type of a variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Llb_MtrVarName( Llb_Mtr_t * p, int iVar )
{
    static char Buffer[10];
    if ( iVar < p->nPis )
        strcpy( Buffer, "pi" );
    else if ( iVar < p->nPis + p->nFfs )
        strcpy( Buffer, "CS" );
    else if ( iVar >= p->nRows - p->nFfs )
        strcpy( Buffer, "NS" );
    else 
        strcpy( Buffer, "int" );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Creates one column with vars in the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrPrint( Llb_Mtr_t * p, int fOrder )
{
    int * pOrder = NULL;
    int i, iRow, iCol;
    if ( fOrder )
        pOrder = Llb_MtrFindVarOrder( p );
    for ( i = 0; i < p->nRows; i++ )
    {
        iRow = pOrder ? pOrder[i] : i;
        printf( "%3d : ", iRow );
        printf( "%3d ", p->pRowSums[iRow] );
        printf( "%3s ", Llb_MtrVarName(p, iRow) );
        for ( iCol = 0; iCol < p->nCols; iCol++ )
            printf( "%c", p->pMatrix[iCol][iRow] ? '*' : ' ' );
        printf( "\n" );
    }
    ABC_FREE( pOrder );
}

/**Function*************************************************************

  Synopsis    [Verify columns.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrPrintMatrixStats( Llb_Mtr_t * p )
{
    int iVar, iGrp, iGrp1, iGrp2, Span = 0, nCutSize = 0, nCutSizeMax = 0;
    int * pGrp1 = ABC_CALLOC( int, p->nRows ); 
    int * pGrp2 = ABC_CALLOC( int, p->nRows ); 
    for ( iVar = 0; iVar < p->nRows; iVar++ )
    {
        if ( p->pRowSums[iVar] == 0 )
            continue;
        for ( iGrp1 = 0; iGrp1 < p->nCols; iGrp1++ )
            if ( p->pMatrix[iGrp1][iVar] == 1 )
                break;
        for ( iGrp2 = p->nCols - 1; iGrp2 >= 0; iGrp2-- )
            if ( p->pMatrix[iGrp2][iVar] == 1 )
                break;
        assert( iGrp1 <= iGrp2 );
        pGrp1[iVar] = iGrp1;
        pGrp2[iVar] = iGrp2;
        Span += iGrp2 - iGrp1;
    }
    // compute span
    for ( iGrp = 0; iGrp < p->nCols; iGrp++ )
    {
        for ( iVar = 0; iVar < p->nRows; iVar++ )
            if ( pGrp1[iVar] == iGrp )
                nCutSize++;
        if ( nCutSizeMax < nCutSize )
            nCutSizeMax = nCutSize;
        for ( iVar = 0; iVar < p->nRows; iVar++ )
            if ( pGrp2[iVar] == iGrp )
                nCutSize--;
    }
    ABC_FREE( pGrp1 );
    ABC_FREE( pGrp2 );
    printf( "[%4d x %4d]  Life-span =%6.2f  Max-cut =%5d\n", 
        p->nCols, p->nRows, 1.0*Span/p->nRows, nCutSizeMax );
    if ( nCutSize )
        Abc_Print( -1, "Cut size is not zero (%d).\n", nCutSize );
}



/**Function*************************************************************

  Synopsis    [Starts the matrix representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Llb_Mtr_t * Llb_MtrAlloc( int nPis, int nFfs, int nCols, int nRows )
{
    Llb_Mtr_t * p;
    int i;
    p = ABC_CALLOC( Llb_Mtr_t, 1 );
    p->nPis  = nPis;
    p->nFfs  = nFfs;
    p->nRows = nRows;
    p->nCols = nCols;
    p->pRowSums = ABC_CALLOC( int, nRows );
    p->pColSums = ABC_CALLOC( int, nCols );
    p->pColGrps = ABC_CALLOC( Llb_Grp_t *, nCols );
    p->pMatrix  = ABC_CALLOC( char *, nCols );
    for ( i = 0; i < nCols; i++ )
        p->pMatrix[i] = ABC_CALLOC( char, nRows );
    // partial product
    p->pProdVars = ABC_CALLOC( char, nRows );  // variables in the partial product
    p->pProdNums = ABC_CALLOC( int, nRows );   // var counts in the remaining partitions
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the matrix representation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrFree( Llb_Mtr_t * p )
{
    int i;
    ABC_FREE( p->pProdVars );
    ABC_FREE( p->pProdNums );
    for ( i = 0; i < p->nCols; i++ )
        ABC_FREE( p->pMatrix[i] );
    ABC_FREE( p->pRowSums );
    ABC_FREE( p->pColSums );
    ABC_FREE( p->pMatrix );
    ABC_FREE( p->pColGrps );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Creates one column with vars in the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrAddColumn( Llb_Mtr_t * p, Llb_Grp_t * pGrp )
{
    Aig_Obj_t * pVar;
    int i, iRow, iCol = pGrp->Id;
    assert( iCol >= 0 && iCol < p->nCols );
    p->pColGrps[iCol] = pGrp;
    Vec_PtrForEachEntry( Aig_Obj_t *, pGrp->vIns, pVar, i )
    {
        iRow = Vec_IntEntry( pGrp->pMan->vObj2Var, Aig_ObjId(pVar) );
        assert( iRow >= 0 && iRow < p->nRows );
        p->pMatrix[iCol][iRow] = 1;
        p->pColSums[iCol]++;
        p->pRowSums[iRow]++;
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, pGrp->vOuts, pVar, i )
    {
        iRow = Vec_IntEntry( pGrp->pMan->vObj2Var, Aig_ObjId(pVar) );
        assert( iRow >= 0 && iRow < p->nRows );
        p->pMatrix[iCol][iRow] = 1;
        p->pColSums[iCol]++;
        p->pRowSums[iRow]++;
    }
}

/**Function*************************************************************

  Synopsis    [Matrix reduce.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_MtrRemoveSingletonRows( Llb_Mtr_t * p )
{
    int i, k;
    for ( i = 0; i < p->nRows; i++ )
        if ( p->pRowSums[i] < 2 )
        {
            p->pRowSums[i] = 0;
            for ( k = 0; k < p->nCols; k++ )
            {
                if ( p->pMatrix[k][i] == 1 )
                {
                    p->pMatrix[k][i] = 0;
                    p->pColSums[k]--;
                }
            }
        }
}

/**Function*************************************************************

  Synopsis    [Matrix reduce.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Llb_Mtr_t * Llb_MtrCreate( Llb_Man_t * p )
{
    Llb_Mtr_t * pMatrix;
    Llb_Grp_t * pGroup;
    int i;
    pMatrix = Llb_MtrAlloc( Saig_ManPiNum(p->pAig), Saig_ManRegNum(p->pAig), 
        Vec_PtrSize(p->vGroups), Vec_IntSize(p->vVar2Obj) );
    Vec_PtrForEachEntry( Llb_Grp_t *, p->vGroups, pGroup, i )
        Llb_MtrAddColumn( pMatrix, pGroup );
//    Llb_MtrRemoveSingletonRows( pMatrix );
    return pMatrix;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

