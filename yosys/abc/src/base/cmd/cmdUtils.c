/**CFile****************************************************************

  FileName    [cmdUtils.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Various utilities of the command package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cmdUtils.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/mainInt.h"
#include "misc/util/utilSignal.h"
#include "cmdInt.h"
#include <ctype.h>

ABC_NAMESPACE_IMPL_START
    // proper declaration of isspace

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int CmdCommandPrintCompare( Abc_Command ** ppC1, Abc_Command ** ppC2 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int cmdCheckShellEscape( Abc_Frame_t * pAbc, int argc, char ** argv)
{
    int RetValue;
    if (argv[0][0] == '!') 
    {
#if defined(__wasm)
        RetValue = -1;
#else
        const int size = 4096;
        int i;
        char * buffer = ABC_ALLOC(char, 10000);
        strncpy (buffer, &argv[0][1], size);
        for (i = 1; i < argc; ++i)
        {
                strncat (buffer, " ", size);
                strncat (buffer, argv[i], size);
        }
        if (buffer[0] == 0) 
            strncpy (buffer, "/bin/sh", size);
        RetValue = system (buffer);
        ABC_FREE( buffer );

        // NOTE: Since we reconstruct the cmdline by concatenating
        // the parts, we lose information. So a command like
        // `!ls "file name"` will be sent to the system as
        // `ls file name` which is a BUG
#endif
        return 1;
    }
    else
    {
        return 0;
    }
}

/**Function*************************************************************

  Synopsis    [Executes one command.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int CmdCommandDispatch( Abc_Frame_t * pAbc, int * pargc, char *** pargv )
{
    int argc = *pargc;
    char ** argv = *pargv;
    char ** argv2;

    Abc_Ntk_t * pNetCopy;
    int (*pFunc) ( Abc_Frame_t *, int, char ** );
    Abc_Command * pCommand;
    char * value;
    int fError;
    double clk;

    if ( argc == 0 )
        return 0;

    if ( cmdCheckShellEscape( pAbc, argc, argv ) == 1 )
        return 0;

    // get the command
    if ( ! st__lookup( pAbc->tCommands, argv[0], (char **)&pCommand ) )
    {   // the command is not in the table
        // if there is only one word with an extension, assume this is file to be read
        if ( argc == 1 && strstr( argv[0], "." ) )
        {
            // add command 'read' assuming that this is the file name
            argv2 = CmdAddToArgv( argc, argv );
            CmdFreeArgv( argc, argv );
            argc = argc+1;
            argv = argv2;
            *pargc = argc;
            *pargv = argv;
            if ( ! st__lookup( pAbc->tCommands, argv[0], (char **)&pCommand ) )
                assert( 0 );
        }
        else
        {
            fprintf( pAbc->Err, "** cmd error: unknown command '%s'\n", argv[0] );
            fprintf( pAbc->Err, "(this is likely caused by using an alias defined in \"abc.rc\"\n" );
            fprintf( pAbc->Err, "without having this file in the current or parent directory)\n" );
            return 1;
        }
    }

    // get the backup network if the command is going to change the network
    if ( pCommand->fChange ) 
    {
        if ( pAbc->pNtkCur && Abc_FrameIsFlagEnabled( "backup" ) )
        {
            pNetCopy = Abc_NtkDup( pAbc->pNtkCur );
            Abc_FrameSetCurrentNetwork( pAbc, pNetCopy );
            // swap the current network and the backup network 
            // to prevent the effect of resetting the short names
            Abc_FrameSwapCurrentAndBackup( pAbc );
        }
    }

    // execute the command
    clk = Extra_CpuTimeDouble();
    pFunc = (int (*)(Abc_Frame_t *, int, char **))pCommand->pFunc;
    fError = (*pFunc)( pAbc, argc, argv );
    pAbc->TimeCommand += Extra_CpuTimeDouble() - clk;

    // automatic execution of arbitrary command after each command 
    // usually this is a passive command ... 
    if ( fError == 0 && !pAbc->fAutoexac )
    {
        if ( st__lookup( pAbc->tFlags, "autoexec", &value ) )
        {
            pAbc->fAutoexac = 1;
            fError = Cmd_CommandExecute( pAbc, value );
            pAbc->fAutoexac = 0;
        }
    }
    return fError;
}

/**Function*************************************************************

  Synopsis    [Splits the command line string into individual commands.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
const char * CmdSplitLine( Abc_Frame_t * pAbc, const char *sCommand, int *argc, char ***argv )
{
    const char *p, *start;
    char c;
    int i, j;
    char *new_arg;
    Vec_Ptr_t * vArgs;
    int single_quote, double_quote;

    vArgs = Vec_PtrAlloc( 10 );

    p = sCommand;
    for ( ;; )
    {
        // skip leading white space 
        while ( isspace( ( int ) *p ) )
        {
            p++;
        }

        // skip until end of this token 
        single_quote = double_quote = 0;
        for ( start = p; ( c = *p ) != '\0'; p++ )
        {
            if ( c == ';' || c == '#' || isspace( ( int ) c ) )
            {
                if ( !single_quote && !double_quote )
                {
                    break;
                }
            }
            if ( c == '\'' )
            {
                single_quote = !single_quote;
            }
            if ( c == '"' )
            {
                double_quote = !double_quote;
            }
        }
        if ( single_quote || double_quote )
        {
            ( void ) fprintf( pAbc->Err, "** cmd warning: ignoring unbalanced quote ...\n" );
        }
        if ( start == p )
            break;

        new_arg = ABC_ALLOC( char, p - start + 1 );
        j = 0;
        for ( i = 0; i < p - start; i++ )
        {
            c = start[i];
            if ( ( c != '\'' ) && ( c != '\"' ) )
            {
                new_arg[j++] = isspace( ( int ) c ) ? ' ' : start[i];
            }
        }
        new_arg[j] = '\0';
        Vec_PtrPush( vArgs, new_arg );
    }

    *argc = vArgs->nSize;
    *argv = (char **)Vec_PtrReleaseArray( vArgs );
    Vec_PtrFree( vArgs );
    if ( *p == ';' )
    {
        p++;
    }
    else if ( *p == '#' )
    {
        for ( ; *p != 0; p++ ); // skip to end of line 
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Replaces parts of the command line string by aliases if given.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int CmdApplyAlias( Abc_Frame_t * pAbc, int *argcp, char ***argvp, int *loop )
{
    int i, argc, stopit, added, offset, did_subst, subst, fError, newc, j;
    const char *arg;
    char **argv, **newv;
    Abc_Alias *alias;

    argc = *argcp;
    argv = *argvp;
    stopit = 0;
    for ( ; *loop < 200; ( *loop )++ )
    {
        if ( argc == 0 )
            return 0;
        if ( stopit != 0 || st__lookup( pAbc->tAliases, argv[0],  (char **) &alias ) == 0 )
        {
            return 0;
        }
        if ( strcmp( argv[0], alias->argv[0] ) == 0 )
        {
            stopit = 1;
        }
        ABC_FREE( argv[0] );
        added = alias->argc - 1;

        /* shift all the arguments to the right */
        if ( added != 0 )
        {
            argv = ABC_REALLOC( char *, argv, argc + added );
            for ( i = argc - 1; i >= 1; i-- )
            {
                argv[i + added] = argv[i];
            }
            for ( i = 1; i <= added; i++ )
            {
                argv[i] = NULL;
            }
            argc += added;
        }
        subst = 0;
        for ( i = 0, offset = 0; i < alias->argc; i++, offset++ )
        {
            arg = CmdHistorySubstitution( pAbc, alias->argv[i], &did_subst );
            if ( arg == NULL )
            {
                *argcp = argc;
                *argvp = argv;
                return ( 1 );
            }
            if ( did_subst != 0 )
            {
                subst = 1;
            }
            fError = 0;
            do
            {
                arg = CmdSplitLine( pAbc, arg, &newc, &newv );
                /*
                 * If there's a complete `;' terminated command in `arg',
                 * when split_line() returns arg[0] != '\0'.
                 */
                if ( arg[0] == '\0' )
                { /* just a bunch of words */
                    break;
                }
                fError = CmdApplyAlias( pAbc, &newc, &newv, loop );
                if ( fError == 0 )
                {
                       fError = CmdCommandDispatch( pAbc, &newc, &newv );
                }
                CmdFreeArgv( newc, newv );
            }
            while ( fError == 0 );
            if ( fError != 0 )
            {
                *argcp = argc;
                *argvp = argv;
                return ( 1 );
            }
            added = newc - 1;
            if ( added != 0 )
            {
                argv = ABC_REALLOC( char *, argv, argc + added );
                for ( j = argc - 1; j > offset; j-- )
                {
                    argv[j + added] = argv[j];
                }
                argc += added;
            }
            for ( j = 0; j <= added; j++ )
            {
                argv[j + offset] = newv[j];
            }
            ABC_FREE( newv );
            offset += added;
        }
        if ( subst == 1 )
        {
            for ( i = offset; i < argc; i++ )
            {
                ABC_FREE( argv[i] );
            }
            argc = offset;
        }
        *argcp = argc;
        *argvp = argv;
    }

    fprintf( pAbc->Err, "** cmd warning: alias loop\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs history substitution (now, disabled).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * CmdHistorySubstitution( Abc_Frame_t * pAbc, char *line, int *changed )
{
    // as of today, no history substitution 
    *changed = 0;
    return line;
}

/**Function*************************************************************

  Synopsis    [Opens the file with path (now, disabled).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
FILE * CmdFileOpen( Abc_Frame_t * pAbc, char *sFileName, char *sMode, char **pFileNameReal, int silent )
{
    char * sRealName, * sPathUsr, * sPathLib, * sPathAll;
    FILE * pFile;
    
    if (strcmp(sFileName, "-") == 0) {
        if (strcmp(sMode, "w") == 0) {
            sRealName = Extra_UtilStrsav( "stdout" );
            pFile = stdout;
        }
        else {
            sRealName = Extra_UtilStrsav( "stdin" );
            pFile = stdin;
        }
    }
    else {
        sRealName = NULL;
        if (strcmp(sMode, "r") == 0) {
            
            /* combine both pathes if exist */
            sPathUsr = Cmd_FlagReadByName(pAbc,"open_path");
            sPathLib = Cmd_FlagReadByName(pAbc,"lib_path");
            
            if ( sPathUsr == NULL && sPathLib == NULL ) {
                sPathAll = NULL;
            }
            else if ( sPathUsr == NULL ) {
                sPathAll = Extra_UtilStrsav( sPathLib );
            }
            else if ( sPathLib == NULL ) {
                sPathAll = Extra_UtilStrsav( sPathUsr );
            }
            else {
                sPathAll = ABC_ALLOC( char, strlen(sPathLib)+strlen(sPathUsr)+5 );
                sprintf( sPathAll, "%s:%s",sPathUsr, sPathLib );
            }
            if ( sPathAll != NULL ) {
                sRealName = Extra_UtilFileSearch(sFileName, sPathAll, "r");
                ABC_FREE( sPathAll );
            }
        }
        if (sRealName == NULL) {
            sRealName = Extra_UtilTildeExpand(sFileName);
        }

        if ((pFile = fopen(sRealName, sMode)) == NULL) {
            if (! silent) {
//                perror(sRealName);
                Abc_Print( 1, "Cannot open file \"%s\".\n", sRealName );
            }
        }
        else
        {
            // print the path/name of the resource file 'abc.rc' that is being loaded
            if ( !silent && strlen(sRealName) >= 6 && strcmp( sRealName + strlen(sRealName) - 6, "abc.rc" ) == 0 )
                Abc_Print( 1, "Loading resource file \"%s\".\n", sRealName );
        }
    }
    if ( pFileNameReal )
        *pFileNameReal = sRealName;
    else
        ABC_FREE(sRealName);
    
    return pFile;
}

/**Function*************************************************************

  Synopsis    [Frees the previously allocated argv array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdFreeArgv( int argc, char **argv )
{
    int i;
    for ( i = 0; i < argc; i++ )
        ABC_FREE( argv[i] );
    ABC_FREE( argv );
}
char ** CmdDupArgv( int argc, char **argv )
{
    char ** argvNew = ABC_ALLOC( char *, argc );
    int i;
    for ( i = 0; i < argc; i++ )
        argvNew[i] = Abc_UtilStrsav( argv[i] );
    return argvNew;
}

/**Function*************************************************************

  Synopsis    [Frees the previously allocated argv array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char ** CmdAddToArgv( int argc, char ** argv )
{
    char ** argv2;
    int i;
    argv2 = ABC_ALLOC( char *, argc + 1 ); 
    argv2[0] = Extra_UtilStrsav( "read" ); 
//    argv2[0] = Extra_UtilStrsav( "&r" ); 
    for ( i = 0; i < argc; i++ )
        argv2[i+1] = Extra_UtilStrsav( argv[i] ); 
    return argv2;
}

/**Function*************************************************************

  Synopsis    [Frees the previously allocated command.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdCommandFree( Abc_Command * pCommand )
{
    ABC_FREE( pCommand->sGroup );
    ABC_FREE( pCommand->sName );
    ABC_FREE( pCommand );
}


/**Function*************************************************************

  Synopsis    [Prints commands alphabetically by group.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdCommandPrint( Abc_Frame_t * pAbc, int fPrintAll, int fDetails )
{
    const char *key;
    char *value;
    st__generator * gen;
    Abc_Command ** ppCommands;
    Abc_Command * pCommands;
    int nCommands, iGroupStart, i, j;
    char * sGroupCur;
    int LenghtMax, nColumns, iCom = 0;
    FILE *backupErr = pAbc->Err;

    // put all commands into one array
    nCommands = st__count( pAbc->tCommands );
    ppCommands = ABC_ALLOC( Abc_Command *, nCommands );
    i = 0;
    st__foreach_item( pAbc->tCommands, gen, &key, &value )
    {
        pCommands = (Abc_Command *)value;
        if ( fPrintAll || pCommands->sName[0] != '_' )
            ppCommands[i++] = pCommands;
    }
    nCommands = i;

    // sort command by group and then by name, alphabetically
    qsort( (void *)ppCommands, (size_t)nCommands, sizeof(Abc_Command *), 
            (int (*)(const void *, const void *)) CmdCommandPrintCompare );
    assert( CmdCommandPrintCompare( ppCommands, ppCommands + nCommands - 1 ) <= 0 );

    // get the longest command name
    LenghtMax = 0;
    for ( i = 0; i < nCommands; i++ )
        if ( LenghtMax < (int)strlen(ppCommands[i]->sName) )
             LenghtMax = (int)strlen(ppCommands[i]->sName); 
    // get the number of columns
    nColumns = 79 / (LenghtMax + 2);

    // print the starting message 
    fprintf( pAbc->Out, "      Welcome to ABC compiled on %s %s!", __DATE__, __TIME__ );

    // print the command by group
    sGroupCur = NULL;
    iGroupStart = 0;
    pAbc->Err = pAbc->Out;
    for ( i = 0; i < nCommands; i++ )
        if ( sGroupCur && strcmp( sGroupCur, ppCommands[i]->sGroup ) == 0 )
        { // this command belongs to the same group as the previous one
            if ( iCom++ % nColumns == 0 )
                fprintf( pAbc->Out, "\n" ); 
            // print this command
            fprintf( pAbc->Out, " %-*s", LenghtMax, ppCommands[i]->sName );
        }
        else
        { // this command starts the new group of commands
            // start the new group
            if ( fDetails && i != iGroupStart )
            { // print help messages for all commands in the previous groups
                fprintf( pAbc->Out, "\n" );
                for ( j = iGroupStart; j < i; j++ )
                {
                    char *tmp_cmd;
                    fprintf( pAbc->Out, "\n" );
                    // fprintf( pAbc->Out, "--- %s ---\n", ppCommands[j]->sName );
                    tmp_cmd = ABC_ALLOC(char, strlen(ppCommands[j]->sName)+4);
                    (void) sprintf(tmp_cmd, "%s -h", ppCommands[j]->sName);
                    (void) Cmd_CommandExecute( pAbc, tmp_cmd );
                    ABC_FREE(tmp_cmd);
                }
                fprintf( pAbc->Out, "\n" );
                fprintf( pAbc->Out, "   ----------------------------------------------------------------------" );
                iGroupStart = i;
            }
            fprintf( pAbc->Out, "\n" );
            fprintf( pAbc->Out, "\n" );
            fprintf( pAbc->Out, "%s commands:\n", ppCommands[i]->sGroup );
            // print this command
            fprintf( pAbc->Out, " %-*s", LenghtMax, ppCommands[i]->sName );
            // remember current command group
            sGroupCur = ppCommands[i]->sGroup;
            // reset the command counter
            iCom = 1;
        }
    if ( fDetails && i != iGroupStart )
    { // print help messages for all commands in the previous groups
        fprintf( pAbc->Out, "\n" );
        for ( j = iGroupStart; j < i; j++ )
        {
            char *tmp_cmd;
            fprintf( pAbc->Out, "\n" );
            // fprintf( pAbc->Out, "--- %s ---\n", ppCommands[j]->sName );
            tmp_cmd = ABC_ALLOC(char, strlen(ppCommands[j]->sName)+4);
            (void) sprintf(tmp_cmd, "%s -h", ppCommands[j]->sName);
            (void) Cmd_CommandExecute( pAbc, tmp_cmd );
            ABC_FREE(tmp_cmd);
        }
    }
    pAbc->Err = backupErr;
    fprintf( pAbc->Out, "\n" );
    ABC_FREE( ppCommands );
}
 
/**Function*************************************************************

  Synopsis    [Comparision function used for sorting commands.]

  Description []
                
  SideEffects []

  SeeAlso     []

***********************************************************************/
int CmdCommandPrintCompare( Abc_Command ** ppC1, Abc_Command ** ppC2 )
{
    Abc_Command * pC1 = *ppC1;
    Abc_Command * pC2 = *ppC2;
    int RetValue;

    RetValue = strcmp( pC1->sGroup, pC2->sGroup );
    if ( RetValue < 0 )
        return -1;
    if ( RetValue > 0 )
        return 1;
    // the command belong to the same group

    // put commands with "_" at the end of the list
    if ( pC1->sName[0] != '_' && pC2->sName[0] == '_' )
        return -1;
    if ( pC1->sName[0] == '_' && pC2->sName[0] != '_' )
        return 1;

    RetValue = strcmp( pC1->sName, pC2->sName );
    if ( RetValue < 0 )
        return -1;
    if ( RetValue > 0 )
        return 1;
     // should not be two indentical commands
    assert( 0 );
    return 0;
}
 
/**Function*************************************************************

  Synopsis    [Comparision function used for sorting commands.]

  Description []
                
  SideEffects []

  SeeAlso     []

***********************************************************************/
int CmdNamePrintCompare( char ** ppC1, char ** ppC2 )
{
    return strcmp( *ppC1, *ppC2 );
}

/**Function*************************************************************

  Synopsis    [Comparision function used for sorting commands.]

  Description []
                
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdPrintTable( st__table * tTable, int fAliases )
{
    st__generator * gen;
    const char ** ppNames;
    const char * key;
    char* value;
    int nNames, i;

    // collect keys in the array
    ppNames = ABC_ALLOC( const char *, st__count(tTable) );
    nNames = 0;
    st__foreach_item( tTable, gen, &key, &value )
        ppNames[nNames++] = key;

    // sort array by name
    qsort( (void *)ppNames, (size_t)nNames, sizeof(char *), 
        (int (*)(const void *, const void *))CmdNamePrintCompare );

    // print in this order
    for ( i = 0; i < nNames; i++ )
    {
        st__lookup( tTable, ppNames[i], &value );
        if ( fAliases )
            CmdCommandAliasPrint( Abc_FrameGetGlobalFrame(), (Abc_Alias *)value );
        else
            fprintf( stdout, "%-15s %-15s\n", ppNames[i], value );
    }
    ABC_FREE( ppNames );
}

/**Function*************************************************************

  Synopsis    []

  Description []
                
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManKissatCall( Abc_Frame_t * pAbc, char * pFileName, char * pArgs, int nConfs, int nTimeLimit, int fSat, int fUnsat, int fPrintCex, int fVerbose )
{
    char Command[1000], Buffer[100];
    char * pNameWin = "kissat.exe";
    char * pNameUnix = "kissat";
    char * pKissatName = NULL;
    //FILE * pFile = NULL;

    // get the names from the resource file
    if ( Cmd_FlagReadByName(pAbc, "kissatwin") )
        pNameWin = Cmd_FlagReadByName(pAbc, "kissatwin");
    if ( Cmd_FlagReadByName(pAbc, "kissatunix") )
        pNameUnix = Cmd_FlagReadByName(pAbc, "kissatunix");
/*
    // check if the binary is available
    if ( (pFile = fopen( pNameWin, "r" )) )
        pKissatName = pNameWin;
    else if ( (pFile = fopen( pNameUnix, "r" )) )
        pKissatName = pNameUnix;
    else if ( pFile == NULL )
    {
        fprintf( stdout, "Cannot find Kissat binary \"%s\" or \"%s\" in the current directory.\n", pNameWin, pNameUnix );
        return;
    }
    fclose( pFile );
*/

#ifdef _WIN32
    pKissatName = pNameWin;
#else
    pKissatName = pNameUnix;
#endif

    sprintf( Command, "%s",  pKissatName );
    if ( pArgs )
    {
        strcat( Command, " " );
        strcat( Command, pArgs );
    }
    if ( !pArgs || (strcmp(pArgs, "-h") && strcmp(pArgs, "--help")) )
    {
        if ( !fVerbose )
            strcat( Command, " -q" );
        if ( !fPrintCex )
            strcat( Command, " -n" );
        if ( fSat )
            strcat( Command, " --sat" );
        if ( fUnsat )
            strcat( Command, " --unsat" );
        if ( nConfs )
        {
            sprintf( Buffer, " --conflicts=%d", nConfs );
            strcat( Command, Buffer );
        }
        if ( nTimeLimit )
        {
            sprintf( Buffer, " --time=%d", nTimeLimit );
            strcat( Command, Buffer );
        }
        strcat( Command, " " );
        strcat( Command, pFileName );
    }
    if ( fVerbose )
        printf( "Running command:  %s\n", Command );
    if ( Util_SignalSystem( Command ) == -1 )
    {
        fprintf( stdout, "The following command has returned a strange exit status:\n" );
        fprintf( stdout, "\"%s\"\n", Command );
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
                
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Cmd_GenScript( char ** pComms, int nComms, int nParts )
{
    static char pScript[1000]; int c;
    pScript[0] = 0;
    for ( c = 0; c < nParts; c++ ) {
        strcat( pScript, pComms[rand() % nComms] );
        strcat( pScript, "; " );
    }
    strcat( pScript, "print_stats" );    
    return pScript;
}
void Cmd_CommandSGen( Abc_Frame_t * pAbc, int nParts, int nIters, int fVerbose )
{
    Abc_Ntk_t * pCopy = Abc_NtkDup( Abc_FrameReadNtk(pAbc) );
    Abc_Ntk_t * pBest = Abc_NtkDup( Abc_FrameReadNtk(pAbc) );
    Abc_Ntk_t * pCur = NULL;  int i;
    char * pComms[6] = { "balance", "rewrite", "rewrite -z", "refactor", "refactor -z", "resub" };
    srand( time(NULL) );
    for ( i = 0; i < nIters; i++ )
    {
        char * pScript = Cmd_GenScript( pComms, 6, nParts );
        Abc_FrameSetCurrentNetwork( pAbc, Abc_NtkDup(pCopy) );
        if ( Abc_FrameIsBatchMode() )
        {
            if ( Cmd_CommandExecute(pAbc, pScript) )
            {
                Abc_Print( 1, "Something did not work out with the command \"%s\".\n", pScript );
                return;
            }
        }
        else
        {
            Abc_FrameSetBatchMode( 1 );
            if ( Cmd_CommandExecute(pAbc, pScript) )
            {
                Abc_Print( 1, "Something did not work out with the command \"%s\".\n", pScript );
                Abc_FrameSetBatchMode( 0 );
                return;
            }
            Abc_FrameSetBatchMode( 0 );
        }
        pCur = Abc_FrameReadNtk(pAbc);
        if ( Abc_NtkNodeNum(pCur) < Abc_NtkNodeNum(pBest) ) {
            Abc_Obj_t * pObj; int k;
            Abc_NtkForEachObj( pBest, pObj, k )
                pObj->fMarkA = pObj->fMarkB = pObj->fMarkC = 0;
            Abc_NtkDelete( pBest );
            pBest = Abc_NtkDup( pCur );
        }
    }
    Abc_FrameSetCurrentNetwork( pAbc, pBest );
    Abc_NtkDelete( pCopy );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

