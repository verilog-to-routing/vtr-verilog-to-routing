/**CFile****************************************************************

  FileName    [bar.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Progress bar.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bar.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "misc/util/abc_global.h"
#include "base/main/main.h"
#include "bar.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Bar_Progress_t_
{
    int              nItemsNext;   // the number of items for the next update of the progress bar
    int              nItemsTotal;  // the total number of items
    int              posTotal;     // the total number of positions
    int              posCur;       // the current position
    FILE *           pFile;        // the output stream 
};

static void Bar_ProgressShow( Bar_Progress_t * p, char * pString );
static void Bar_ProgressClean( Bar_Progress_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the progress bar.]

  Description [The first parameter is the output stream (pFile), where
  the progress is printed. The current printing position should be the
  first one on the given line. The second parameters is the total
  number of items that correspond to 100% position of the progress bar.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bar_Progress_t * Bar_ProgressStart( FILE * pFile, int nItemsTotal )
{
    Bar_Progress_t * p;
    Abc_Frame_t * pFrame;
    pFrame = Abc_FrameReadGlobalFrame();
    if ( pFrame == NULL )
        return NULL;
    if ( !Abc_FrameShowProgress(pFrame) ) return NULL;
    p = ABC_ALLOC( Bar_Progress_t, 1 );
    memset( p, 0, sizeof(Bar_Progress_t) );
    p->pFile       = pFile;
    p->nItemsTotal = nItemsTotal;
    p->posTotal    = 78;
    p->posCur      = 1;
    p->nItemsNext  = (int)((7.0+p->posCur)*p->nItemsTotal/p->posTotal);
    Bar_ProgressShow( p, NULL );
    return p;
}

/**Function*************************************************************

  Synopsis    [Updates the progress bar.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bar_ProgressUpdate_int( Bar_Progress_t * p, int nItemsCur, char * pString )
{
    if ( p == NULL ) return;
    if ( nItemsCur < p->nItemsNext )
        return;
    if ( nItemsCur >= p->nItemsTotal )
    {
        p->posCur = 78;
        p->nItemsNext = 0x7FFFFFFF;
    }
    else
    {
        p->posCur += 7;
        p->nItemsNext = (int)((7.0+p->posCur)*p->nItemsTotal/p->posTotal);
    }
    Bar_ProgressShow( p, pString );
}


/**Function*************************************************************

  Synopsis    [Stops the progress bar.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bar_ProgressStop( Bar_Progress_t * p )
{
    if ( p == NULL ) return;
    Bar_ProgressClean( p );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints the progress bar of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bar_ProgressShow( Bar_Progress_t * p, char * pString )
{
    int i;
    if ( p == NULL ) 
        return;
    if ( Abc_FrameIsBatchMode() )
        return;
    if ( pString )
        fprintf( p->pFile, "%s ", pString );
    for ( i = (pString? strlen(pString) + 1 : 0); i < p->posCur; i++ )
        fprintf( p->pFile, "-" );
    if ( i == p->posCur )
        fprintf( p->pFile, ">" );
    for ( i++  ; i <= p->posTotal; i++ )
        fprintf( p->pFile, " " );
    fprintf( p->pFile, "\r" );
    fflush( stdout );
}

/**Function*************************************************************

  Synopsis    [Cleans the progress bar before quitting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bar_ProgressClean( Bar_Progress_t * p )
{
    int i;
    if ( p == NULL ) 
        return;
    if ( Abc_FrameIsBatchMode() )
        return;
    for ( i = 0; i <= p->posTotal; i++ )
        fprintf( p->pFile, " " );
    fprintf( p->pFile, "\r" );
    fflush( stdout );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

