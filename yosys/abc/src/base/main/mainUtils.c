/**CFile****************************************************************

  FileName    [mainUtils.c]

  SystemName  [ABC: Logic synthesis and verification system.]
 
  PackageName [The main package.]

  Synopsis    [Miscellaneous utilities.]
 
  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mainUtils.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "mainInt.h"

#ifdef ABC_USE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static char * DateReadFromDateString( char * datestr );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_UtilsGetVersion( Abc_Frame_t * pAbc )
{
    static char Version[1000];
#if __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdate-time"
#endif
    sprintf(Version, "%s (compiled %s %s)", ABC_VERSION, __DATE__, __TIME__);
#if __GNUC__
    #pragma GCC diagnostic pop
#endif
    return Version;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_UtilsGetUsersInput( Abc_Frame_t * pAbc )
{
    static char Prompt[5000];
    sprintf( Prompt, "abc %02d> ", pAbc->nSteps );
#ifdef ABC_USE_READLINE
    {
    static char * line = NULL;
    if (line != NULL) ABC_FREE(line);
    line = readline(Prompt);  
    if (line == NULL){ printf("***EOF***\n"); exit(0); }
    add_history(line);
    return line;
    }
#else
    {
    char * pRetValue;
    fprintf( pAbc->Out, "%s", Prompt );
    pRetValue = fgets( Prompt, 5000, stdin );
    return Prompt;
    }
#endif
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_UtilsPrintHello( Abc_Frame_t * pAbc )
{
    fprintf( pAbc->Out, "%s\n", pAbc->sVersion );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_UtilsPrintUsage( Abc_Frame_t * pAbc, char * ProgName )
{
    fprintf( pAbc->Err, "\n" );
    fprintf( pAbc->Err,
             "usage: %s [-c cmd] [-q cmd] [-C cmd] [-Q cmd] [-f script] [-h] [-o file] [-s] [-t type] [-T type] [-x] [-b] [file]\n",
             ProgName);
    fprintf( pAbc->Err, "    -c cmd\texecute commands `cmd'\n");
    fprintf( pAbc->Err, "    -q cmd\texecute commands `cmd' quietly\n");
    fprintf( pAbc->Err, "    -C cmd\texecute commands `cmd', then continue in interactive mode\n");
    fprintf( pAbc->Err, "    -Q cmd\texecute commands `cmd' quietly, then continue in interactive mode\n");
    fprintf( pAbc->Err, "    -F script\texecute commands from a script file and echo commands\n");
    fprintf( pAbc->Err, "    -f script\texecute commands from a script file\n");
    fprintf( pAbc->Err, "    -h\t\tprint the command usage\n");
    fprintf( pAbc->Err, "    -o file\tspecify output filename to store the result\n");
    fprintf( pAbc->Err, "    -s\t\tdo not read any initialization file\n");
    fprintf( pAbc->Err, "    -t type\tspecify input type (blif_mv (default), blif_mvs, blif, or none)\n");
    fprintf( pAbc->Err, "    -T type\tspecify output type (blif_mv (default), blif_mvs, blif, or none)\n");
    fprintf( pAbc->Err, "    -x\t\tequivalent to '-t none -T none'\n");
    fprintf( pAbc->Err, "    -b\t\trunning in bridge mode\n");
    fprintf( pAbc->Err, "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_UtilsSource( Abc_Frame_t * pAbc )
{  
#ifdef WIN32
    if ( Cmd_CommandExecute(pAbc, "source abc.rc") )
    {
        if ( Cmd_CommandExecute(pAbc, "source ..\\abc.rc") == 0 )
            printf( "Loaded \"abc.rc\" from the parent directory.\n" );
        else if ( Cmd_CommandExecute(pAbc, "source ..\\..\\abc.rc") == 0 )
            printf( "Loaded \"abc.rc\" from the grandparent directory.\n" );
    }
#else

#if 0
    {
        char * sPath1, * sPath2;

         // If .rc is present in both the home and current directories, then read
         // it from the home directory.  Otherwise, read it from wherever it's located.
        sPath1 = Extra_UtilFileSearch(".rc", "~/", "r");
        sPath2 = Extra_UtilFileSearch(".rc", ".",  "r");
  
        if ( sPath1 && sPath2 ) {
            /* ~/.rc == .rc : Source the file only once */
            (void) Cmd_CommandExecute(pAbc, "source -s ~/.rc");
        }
        else {
            if (sPath1) {
                (void) Cmd_CommandExecute(pAbc, "source -s ~/.rc");
            }
            if (sPath2) {
                (void) Cmd_CommandExecute(pAbc, "source -s .rc");
            }
        }
        if ( sPath1 ) ABC_FREE(sPath1);
        if ( sPath2 ) ABC_FREE(sPath2);
    
        /* execute the abc script which can be open with the "open_path" */
        Cmd_CommandExecute( pAbc, "source -s abc.rc" );
    }
#endif

    {
        char * sPath1, * sPath2;
        char * home;

         // If .rc is present in both the home and current directories, then read
         // it from the home directory.  Otherwise, read it from wherever it's located.
        home = getenv("HOME");
        if (home){
            char * sPath3 = ABC_ALLOC(char, strlen(home) + 2);
            (void) sprintf(sPath3, "%s/", home);
            sPath1 = Extra_UtilFileSearch(".abc.rc", sPath3, "r");
            ABC_FREE(sPath3);
        }else
            sPath1 = NULL;

        sPath2 = Extra_UtilFileSearch(".abc.rc", ".",  "r");

        if ( sPath1 && sPath2 ) {
            /* ~/.rc == .rc : Source the file only once */
            char *tmp_cmd = ABC_ALLOC(char, strlen(sPath1)+12);
            (void) sprintf(tmp_cmd, "source -s %s", sPath1);
            // (void) Cmd_CommandExecute(pAbc, "source -s ~/.abc.rc");
            (void) Cmd_CommandExecute(pAbc, tmp_cmd);
            ABC_FREE(tmp_cmd);
        }
        else {
            if (sPath1) {
                char *tmp_cmd = ABC_ALLOC(char, strlen(sPath1)+12);
                (void) sprintf(tmp_cmd, "source -s %s", sPath1);
                // (void) Cmd_CommandExecute(pAbc, "source -s ~/.abc.rc");
                (void) Cmd_CommandExecute(pAbc, tmp_cmd);
                ABC_FREE(tmp_cmd);
            }
            if (sPath2) {
                char *tmp_cmd = ABC_ALLOC(char, strlen(sPath2)+12);
                (void) sprintf(tmp_cmd, "source -s %s", sPath2);
                // (void) Cmd_CommandExecute(pAbc, "source -s .abc.rc");
                (void) Cmd_CommandExecute(pAbc, tmp_cmd);
                ABC_FREE(tmp_cmd);
            }
        }
        if ( sPath1 ) ABC_FREE(sPath1);
        if ( sPath2 ) ABC_FREE(sPath2);

        /* execute the abc script which can be open with the "open_path" */
        Cmd_CommandExecute( pAbc, "source -s abc.rc" );
    }
    
#endif //WIN32
}

/**Function********************************************************************

  Synopsis    [Returns the date in a brief format assuming its coming from
  the program `date'.]

  Description [optional]

  SideEffects []

******************************************************************************/
char * DateReadFromDateString( char * datestr )
{
  static char result[100];
  char        day[10];
  char        month[10];
  char        zone[10];
  char       *at;
  int         date;
  int         hour;
  int         minute;
  int         second;
  int         year;

  if (sscanf(datestr, "%s %s %2d %2d:%2d:%2d %s %4d",
             day, month, &date, &hour, &minute, &second, zone, &year) == 8) {
    if (hour >= 12) {
      if (hour >= 13) hour -= 12;
      at = "PM";
    }
    else {
      if (hour == 0) hour = 12;
      at = "AM";
    }
    (void) sprintf(result, "%d-%3s-%02d at %d:%02d %s", 
                   date, month, year % 100, hour, minute, at);
    return result;
  }
  else {
    return datestr;
  }
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

******************************************************************************/
void Abc_FrameStoreStop( Abc_Frame_t * pAbc )
{
    if ( pAbc->pHash ) 
        Hsh_VecManStop( pAbc->pHash );
    pAbc->pHash = NULL;
}
void Abc_FrameStoreStart( Abc_Frame_t * pAbc )
{
    Abc_FrameStoreStop( pAbc );
    pAbc->pHash = Hsh_VecManStart( 1000 );
    pAbc->pHash->vEntry = Vec_IntAlloc( 1000 );
    pAbc->pHash->vValue = Vec_IntAlloc( 1000 );    
}
void Abc_FrameStoreAdd( Abc_Frame_t * pAbc, Gia_Man_t * pGia )
{
    if ( pAbc->pHash == NULL )
        Abc_FrameStoreStart( pAbc );
    Gia_Man_t * p = Gia_ManIsoCanonicize( pGia, 0 );
    Vec_IntClear( pAbc->pHash->vEntry );
    Vec_IntPush( pAbc->pHash->vEntry, 0 );
    Vec_IntPush( pAbc->pHash->vEntry, Gia_ManPiNum(p) );
    Vec_IntPush( pAbc->pHash->vEntry, Gia_ManPoNum(p) );
    Vec_IntPush( pAbc->pHash->vEntry, Gia_ManRegNum(p) );
    Gia_Obj_t * pObj; int i;
    Gia_ManForEachAnd( p, pObj, i )
        Vec_IntPushTwo( pAbc->pHash->vEntry, Gia_ObjFaninLit0(pObj, i), Gia_ObjFaninLit1(pObj, i) );
    int iEntry = Hsh_VecManAdd( pAbc->pHash, pAbc->pHash->vEntry );
    if ( Vec_IntSize(pAbc->pHash->vValue) == iEntry )
        Vec_IntPush( pAbc->pHash->vValue, 0 );
    Vec_IntAddToEntry( pAbc->pHash->vValue, iEntry, 1 );
    assert( Vec_IntSize(pAbc->pHash->vValue) == Hsh_VecSize(pAbc->pHash) );
    Gia_ManStop( p );
}
void Abc_FrameStorePrint( Abc_Frame_t * pAbc )
{
    if ( pAbc->pHash == NULL )
        Abc_FrameStoreStart( pAbc );
    int i, Entry, Max = Vec_IntFindMax( pAbc->pHash->vValue );
    Vec_Int_t * vCounts = Vec_IntStart( Max+1 );
    Vec_IntForEachEntry( pAbc->pHash->vValue, Entry, i )
        Vec_IntAddToEntry( vCounts, Entry, 1 );
    printf( "Distribution of %d stored items by reference count (<ref_count>=<num_items>): ", Hsh_VecSize(pAbc->pHash) );
    Vec_IntForEachEntry( vCounts, Entry, i )
        if ( Entry )
            printf( "%d=%d ", i, Entry );
    printf( "\n" );
    Vec_IntFree( vCounts );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
