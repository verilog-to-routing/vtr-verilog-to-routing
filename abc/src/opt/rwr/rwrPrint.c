/**CFile****************************************************************

  FileName    [rwrCut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting package.]

  Synopsis    [Cut computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rwrCut.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rwr.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Adds one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_Trav2_rec( Rwr_Man_t * p, Rwr_Node_t * pNode, int * pVolume )
{
    if ( pNode->fUsed || pNode->TravId == p->nTravIds )
        return;
    pNode->TravId = p->nTravIds;
    (*pVolume)++;
    Rwr_Trav2_rec( p, Rwr_Regular(pNode->p0), pVolume );
    Rwr_Trav2_rec( p, Rwr_Regular(pNode->p1), pVolume );
}

/**Function*************************************************************

  Synopsis    [Adds the node to the end of the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_GetBushVolume( Rwr_Man_t * p, int Entry, int * pVolume, int * pnFuncs )
{
    Rwr_Node_t * pNode;
    int Volume = 0;
    int nFuncs = 0;
    Rwr_ManIncTravId( p );
    for ( pNode = p->pTable[Entry]; pNode; pNode = pNode->pNext )
    {
        if ( pNode->uTruth != p->puCanons[pNode->uTruth] )
            continue;
        nFuncs++;
        Rwr_Trav2_rec( p, pNode, &Volume );
    }
    *pVolume = Volume;
    *pnFuncs = nFuncs;
}

/**Function*************************************************************

  Synopsis    [Adds the node to the end of the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rwr_GetBushSumOfVolumes( Rwr_Man_t * p, int Entry )
{
    Rwr_Node_t * pNode;
    int Volume, VolumeTotal = 0;
    for ( pNode = p->pTable[Entry]; pNode; pNode = pNode->pNext )
    {
        if ( pNode->uTruth != p->puCanons[pNode->uTruth] )
            continue;
        Volume = 0;
        Rwr_ManIncTravId( p );
        Rwr_Trav2_rec( p, pNode, &Volume );
        VolumeTotal += Volume;
    }
    return VolumeTotal;
}

/**Function*************************************************************

  Synopsis    [Prints one rwr node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_NodePrint_rec( FILE * pFile, Rwr_Node_t * pNode )
{
    assert( !Rwr_IsComplement(pNode) );

    if ( pNode->Id == 0 )
    {
        fprintf( pFile, "Const1" );
        return;
    }

    if ( pNode->Id < 5 )
    {
        fprintf( pFile, "%c", 'a' + pNode->Id - 1 );
        return;
    }

    if ( Rwr_IsComplement(pNode->p0) )
    {
        if ( Rwr_Regular(pNode->p0)->Id < 5 )
        {
            Rwr_NodePrint_rec( pFile, Rwr_Regular(pNode->p0) );
            fprintf( pFile, "\'" );
        }
        else
        {
            fprintf( pFile, "(" );
            Rwr_NodePrint_rec( pFile, Rwr_Regular(pNode->p0) );
            fprintf( pFile, ")\'" );
        }
    }
    else
    {
        if ( Rwr_Regular(pNode->p0)->Id < 5 )
        {
            Rwr_NodePrint_rec( pFile, Rwr_Regular(pNode->p0) );
        }
        else
        {
            fprintf( pFile, "(" );
            Rwr_NodePrint_rec( pFile, Rwr_Regular(pNode->p0) );
            fprintf( pFile, ")" );
        }
    }

    if ( pNode->fExor )
        fprintf( pFile, "+" );

    if ( Rwr_IsComplement(pNode->p1) )
    {
        if ( Rwr_Regular(pNode->p1)->Id < 5 )
        {
            Rwr_NodePrint_rec( pFile, Rwr_Regular(pNode->p1) );
            fprintf( pFile, "\'" );
        }
        else
        {
            fprintf( pFile, "(" );
            Rwr_NodePrint_rec( pFile, Rwr_Regular(pNode->p1) );
            fprintf( pFile, ")\'" );
        }
    }
    else
    {
        if ( Rwr_Regular(pNode->p1)->Id < 5 )
        {
            Rwr_NodePrint_rec( pFile, Rwr_Regular(pNode->p1) );
        }
        else
        {
            fprintf( pFile, "(" );
            Rwr_NodePrint_rec( pFile, Rwr_Regular(pNode->p1) );
            fprintf( pFile, ")" );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Prints one rwr node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_NodePrint( FILE * pFile, Rwr_Man_t * p, Rwr_Node_t * pNode )
{
    unsigned uTruth;
    fprintf( pFile, "%5d : ", pNode->Id );
    uTruth = pNode->uTruth;
    Extra_PrintHex( pFile, &uTruth, 4 );
    fprintf( pFile, " tt=" );
    Extra_PrintBinary( pFile, &uTruth, 16 );
//    fprintf( pFile, " cn=", pNode->Id );
//    uTruth = p->puCanons[pNode->uTruth];
//    Extra_PrintBinary( pFile, &uTruth, 16 );
    fprintf( pFile, " lev=%d", pNode->Level );
    fprintf( pFile, " vol=%d", pNode->Volume );
    fprintf( pFile, "  " );
    Rwr_NodePrint_rec( pFile, pNode );
    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints one rwr node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_ManPrint( Rwr_Man_t * p )
{
    FILE * pFile;
    Rwr_Node_t * pNode;
    unsigned uTruth;
    int Limit, Counter, Volume, nFuncs, i;
    pFile = fopen( "graph_lib.txt", "w" );
    Counter = 0;
    Limit = (1 << 16);
    for ( i = 0; i < Limit; i++ )
    {
        if ( p->pTable[i] == NULL )
            continue;
        if ( i != p->puCanons[i] )
            continue;
        fprintf( pFile, "\nClass %3d. Func %6d.  ", p->pMap[i], Counter++ );
        Rwr_GetBushVolume( p, i, &Volume, &nFuncs );
        fprintf( pFile, "Roots = %3d. Vol = %3d. Sum = %3d.  ", nFuncs, Volume, Rwr_GetBushSumOfVolumes(p, i) );
        uTruth = i;
        Extra_PrintBinary( pFile, &uTruth, 16 );
        fprintf( pFile, "\n" );
        for ( pNode = p->pTable[i]; pNode; pNode = pNode->pNext )
            if ( pNode->uTruth == p->puCanons[pNode->uTruth] )
                Rwr_NodePrint( pFile, p, pNode );
    }
    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

