/**CFile****************************************************************

  FileName    [cmdHist.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures working with history.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cmdHist.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/mainInt.h"
#include "cmd.h"
#include "cmdInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define ABC_USE_HISTORY 1

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cmd_HistoryAddCommand(    Abc_Frame_t * p, const char * command )
{
    int nLastLooked =   10;  // do not add history if the same entry appears among the last entries
    int nLastSaved  = 1000;  // when saving a file, save no more than this number of last entries
    char Buffer[ABC_MAX_STR];
    int Len;
    if ( p->fBatchMode )
        return;
    Len = strlen(command);
    strcpy( Buffer, command );
    if ( Len > 0 && Buffer[Len-1] == '\n' )
        Buffer[Len-1] = 0;
    if ( strlen(Buffer) > 3 &&
         strncmp(Buffer,"set",3) && 
         strncmp(Buffer,"unset",5) && 
         strncmp(Buffer,"time",4) && 
         strncmp(Buffer,"quit",4) && 
         strncmp(Buffer,"alias",5) && 
//         strncmp(Buffer,"source",6) && 
         strncmp(Buffer,"history",7) && strncmp(Buffer,"hi ", 3) && strcmp(Buffer,"hi") &&
         Buffer[strlen(Buffer)-1] != '?' )
    {
        char * pStr = NULL;
        int i, Start = Abc_MaxInt( 0, Vec_PtrSize(p->aHistory) - nLastLooked );
        // do not enter if the same command appears among nLastLooked commands
        Vec_PtrForEachEntryStart( char *, p->aHistory, pStr, i, Start )
            if ( !strcmp(pStr, Buffer) )
                break;
        if ( i == Vec_PtrSize(p->aHistory) )
        { // add new entry
            Vec_PtrPush( p->aHistory, Extra_UtilStrsav(Buffer) );
            Cmd_HistoryWrite( p, nLastSaved );
        }
        else
        { // put at the end
            Vec_PtrRemove( p->aHistory, pStr );
            Vec_PtrPush( p->aHistory, pStr );
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cmd_HistoryRead( Abc_Frame_t * p )
{
#if defined(WIN32) && defined(ABC_USE_HISTORY)
    char Buffer[ABC_MAX_STR];
    FILE * pFile;
    assert( Vec_PtrSize(p->aHistory) == 0 );
    pFile = fopen( "abc.history", "rb" );
    if ( pFile == NULL )
        return;
    while ( fgets( Buffer, ABC_MAX_STR, pFile ) != NULL )
    {
        int Len = strlen(Buffer);
        if ( Buffer[Len-1] == '\n' )
            Buffer[Len-1] = 0;
        Vec_PtrPush( p->aHistory, Extra_UtilStrsav(Buffer) );
    }
    fclose( pFile );
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cmd_HistoryWrite( Abc_Frame_t * p, int Limit )
{
#if defined(WIN32) && defined(ABC_USE_HISTORY)
    FILE * pFile;
    char * pStr; 
    int i;
    pFile = fopen( "abc.history", "wb" );
    if ( pFile == NULL )
    {
        Abc_Print( 0, "Cannot open file \"abc.history\" for writing.\n" );
        return;
    }
    Limit = Abc_MaxInt( 0, Vec_PtrSize(p->aHistory)-Limit );
    Vec_PtrForEachEntryStart( char *, p->aHistory, pStr, i, Limit )
        fprintf( pFile, "%s\n", pStr );
    fclose( pFile );
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cmd_HistoryPrint( Abc_Frame_t * p, int Limit )
{
#if defined(WIN32) && defined(ABC_USE_HISTORY) 
    char * pStr; 
    int i;
    Limit = Abc_MaxInt( 0, Vec_PtrSize(p->aHistory)-Limit );
    printf( "================== Command history ==================\n" );
    Vec_PtrForEachEntryStart( char *, p->aHistory, pStr, i, Limit )
        printf( "%s\n", pStr );
    printf( "=====================================================\n" );
#endif
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

