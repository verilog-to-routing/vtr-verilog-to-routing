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

#include "mainInt.h"
/* jluu
#ifndef _WIN32
#include "readline/readline.h"
#endif
*/
////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
static char * DateReadFromDateString(char * datestr);

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
    sprintf(Version, "%s (compiled %s %s)", ABC_VERSION, __DATE__, __TIME__);
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
    static char Buffer[1000], Prompt[1000];
    sprintf( Prompt, "abc %02d> ", pAbc->nSteps );
	
/* jluu #ifdef _WIN32  */
#ifndef CMD_READLINE
    fprintf( pAbc->Out, "%s", Prompt );
    fgets( Buffer, 999, stdin );
    return Buffer;
#else
    static char* line = NULL;
    if (line != NULL) free(line);
    line = readline(Prompt);  
    if (line == NULL){ printf("***EOF***\n"); exit(0); }
    add_history(line);
    return line;
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
             "usage: %s [-c cmd] [-f script] [-h] [-o file] [-s] [-t type] [-T type] [-x] [file]\n", 
             ProgName);
    fprintf( pAbc->Err, "    -c cmd\texecute commands `cmd'\n");
    fprintf( pAbc->Err, "    -F script\texecute commands from a script file and echo commands\n");
    fprintf( pAbc->Err, "    -f script\texecute commands from a script file\n");
    fprintf( pAbc->Err, "    -h\t\tprint the command usage\n");
    fprintf( pAbc->Err, "    -o file\tspecify output filename to store the result\n");
    fprintf( pAbc->Err, "    -s\t\tdo not read any initialization file\n");
    fprintf( pAbc->Err, "    -t type\tspecify input type (blif_mv (default), blif_mvs, blif, or none)\n");
    fprintf( pAbc->Err, "    -T type\tspecify output type (blif_mv (default), blif_mvs, blif, or none)\n");
    fprintf( pAbc->Err, "    -x\t\tequivalent to '-t none -T none'\n");
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
    if ( sPath1 ) FREE(sPath1);
    if ( sPath2 ) FREE(sPath2);
    
    /* execute the abc script which can be open with the "open_path" */
    Cmd_CommandExecute( pAbc, "source -s abc.rc" );
    }
#endif //WIN32
    {
        // reset command history
        char * pName; 
        int i;
        Vec_PtrForEachEntry( pAbc->aHistory, pName, i )
            free( pName );
        pAbc->aHistory->nSize = 0;
    }
}

/**Function********************************************************************

  Synopsis    [Returns the date in a brief format assuming its coming from
  the program `date'.]

  Description [optional]

  SideEffects []

******************************************************************************/
char *
DateReadFromDateString(
  char * datestr)
{
  static char result[25];
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



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


