/**CFile****************************************************************

  FileName    [covMinUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Mapping into network of SOPs/ESOPs.]

  Synopsis    [Utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: covMinUtil.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "covInt.h"

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
void Min_CubeCreate( Vec_Str_t * vCover, Min_Cube_t * pCube, char Type )
{
    int i;
    assert( (int)pCube->nLits == Min_CubeCountLits(pCube) );
    for ( i = 0; i < (int)pCube->nVars; i++ )
        if ( Min_CubeHasBit(pCube, i*2) )
        {
            if ( Min_CubeHasBit(pCube, i*2+1) )
//                fprintf( pFile, "-" );
                Vec_StrPush( vCover, '-' );
            else
//                fprintf( pFile, "0" );
                Vec_StrPush( vCover, '0' );
        }
        else
        {
            if ( Min_CubeHasBit(pCube, i*2+1) )
 //               fprintf( pFile, "1" );
                Vec_StrPush( vCover, '1' );
            else
//                fprintf( pFile, "?" );
                Vec_StrPush( vCover, '?' );
        }
//    fprintf( pFile, " 1\n" );
    Vec_StrPush( vCover, ' ' );
    Vec_StrPush( vCover, Type );
    Vec_StrPush( vCover, '\n' );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_CoverCreate( Vec_Str_t * vCover, Min_Cube_t * pCover, char Type )
{
    Min_Cube_t * pCube;
    assert( pCover != NULL );
    Vec_StrClear( vCover );
    Min_CoverForEachCube( pCover, pCube )
        Min_CubeCreate( vCover, pCube, Type );
    Vec_StrPush( vCover, 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_CubeWrite( FILE * pFile, Min_Cube_t * pCube )
{
    int i;
    assert( (int)pCube->nLits == Min_CubeCountLits(pCube) );
    for ( i = 0; i < (int)pCube->nVars; i++ )
        if ( Min_CubeHasBit(pCube, i*2) )
        {
            if ( Min_CubeHasBit(pCube, i*2+1) )
                fprintf( pFile, "-" );
            else
                fprintf( pFile, "0" );
        }
        else
        {
            if ( Min_CubeHasBit(pCube, i*2+1) )
                fprintf( pFile, "1" );
            else
                fprintf( pFile, "?" );
        }
    fprintf( pFile, " 1\n" );
//    fprintf( pFile, " %d\n", pCube->nLits );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_CoverWrite( FILE * pFile, Min_Cube_t * pCover )
{
    Min_Cube_t * pCube;
    Min_CoverForEachCube( pCover, pCube )
        Min_CubeWrite( pFile, pCube );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_CoverWriteStore( FILE * pFile, Min_Man_t * p )
{
    Min_Cube_t * pCube;
    int i;
    for ( i = 0; i <= p->nVars; i++ )
    {
        Min_CoverForEachCube( p->ppStore[i], pCube )
        {
            printf( "%2d : ", i );
            if ( pCube == p->pBubble )
            {
                printf( "Bubble\n" );
                continue;
            }
            Min_CubeWrite( pFile, pCube );
        }
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_CoverWriteFile( Min_Cube_t * pCover, char * pName, int fEsop )
{
    char Buffer[1000];
    Min_Cube_t * pCube;
    FILE * pFile;
    int i;
    sprintf( Buffer, "%s.%s", pName, fEsop? "esop" : "pla" );
    for ( i = strlen(Buffer) - 1; i >= 0; i-- )
        if ( Buffer[i] == '<' || Buffer[i] == '>' )
            Buffer[i] = '_';
    pFile = fopen( Buffer, "w" );
    fprintf( pFile, "# %s cover for output %s generated by ABC on %s\n", fEsop? "ESOP":"SOP", pName, Extra_TimeStamp() );
    fprintf( pFile, ".i %d\n", pCover? pCover->nVars : 0 );
    fprintf( pFile, ".o %d\n", 1 );
    fprintf( pFile, ".p %d\n", Min_CoverCountCubes(pCover) );
    if ( fEsop ) fprintf( pFile, ".type esop\n" );
    Min_CoverForEachCube( pCover, pCube )
        Min_CubeWrite( pFile, pCube );
    fprintf( pFile, ".e\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_CoverCheck( Min_Man_t * p )
{
    Min_Cube_t * pCube;
    int i;
    for ( i = 0; i <= p->nVars; i++ )
        Min_CoverForEachCube( p->ppStore[i], pCube )
            assert( i == (int)pCube->nLits );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Min_CubeCheck( Min_Cube_t * pCube )
{
    int i;
    for ( i = 0; i < (int)pCube->nVars; i++ )
        if ( Min_CubeGetVar( pCube, i ) == 0 )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Converts the cover from the sorted structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Min_Cube_t * Min_CoverCollect( Min_Man_t * p, int nSuppSize )
{
    Min_Cube_t * pCov = NULL, ** ppTail = &pCov;
    Min_Cube_t * pCube, * pCube2;
    int i;
    for ( i = 0; i <= nSuppSize; i++ )
    {
        Min_CoverForEachCubeSafe( p->ppStore[i], pCube, pCube2 )
        {
            assert( i == (int)pCube->nLits );
            *ppTail = pCube; 
            ppTail = &pCube->pNext;
            assert( pCube->uData[0] ); // not a bubble
        }
    }
    *ppTail = NULL;
    return pCov;
}

/**Function*************************************************************

  Synopsis    [Sorts the cover in the increasing number of literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_CoverExpand( Min_Man_t * p, Min_Cube_t * pCover )
{
    Min_Cube_t * pCube, * pCube2;
    Min_ManClean( p, p->nVars );
    Min_CoverForEachCubeSafe( pCover, pCube, pCube2 )
    {
        pCube->pNext = p->ppStore[pCube->nLits];
        p->ppStore[pCube->nLits] = pCube;
        p->nCubes++;
    }
}

/**Function*************************************************************

  Synopsis    [Sorts the cover in the increasing number of literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Min_CoverSuppVarNum( Min_Man_t * p, Min_Cube_t * pCover )
{
    Min_Cube_t * pCube;
    int i, Counter;
    if ( pCover == NULL )
        return 0;
    // clean the cube
    for ( i = 0; i < (int)pCover->nWords; i++ )
        p->pTemp->uData[i]  = ~((unsigned)0);
    // add the bit data
    Min_CoverForEachCube( pCover, pCube )
        for ( i = 0; i < (int)pCover->nWords; i++ )
            p->pTemp->uData[i] &= pCube->uData[i];
    // count the vars
    Counter = 0;
    for ( i = 0; i < (int)pCover->nVars; i++ )
        Counter += ( Min_CubeGetVar(p->pTemp, i) != 3 );
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

