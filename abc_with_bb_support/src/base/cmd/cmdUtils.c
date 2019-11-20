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

#include "mainInt.h"
#include "abc.h"
#include "cmdInt.h"
#include <ctype.h>	// proper declaration of isspace

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
	if (argv[0][0] == '!') 
	{
		const int size = 4096;
		int i;
		char buffer[4096];
		strncpy (buffer, &argv[0][1], size);
		for (i = 1; i < argc; ++i)
		{
				strncat (buffer, " ", size);
				strncat (buffer, argv[i], size);
		}
		if (buffer[0] == 0) 
			strncpy (buffer, "/bin/sh", size);
		system (buffer);

		// NOTE: Since we reconstruct the cmdline by concatenating
		// the parts, we lose information. So a command like
		// `!ls "file name"` will be sent to the system as
		// `ls file name` which is a BUG

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
int CmdCommandDispatch( Abc_Frame_t * pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNetCopy;
    int (*pFunc) ( Abc_Frame_t *, int, char ** );
    Abc_Command * pCommand;
    char * value;
    int fError;
    int clk;

    if ( argc == 0 )
        return 0;

	if ( cmdCheckShellEscape( pAbc, argc, argv ) == 1 )
		return 0;

    // get the command
    if ( !st_lookup( pAbc->tCommands, argv[0], (char **)&pCommand ) )
    {   // the command is not in the table
        fprintf( pAbc->Err, "** cmd error: unknown command '%s'\n", argv[0] );
        return 1;
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
    clk = Extra_CpuTime();
    pFunc = (int (*)(Abc_Frame_t *, int, char **))pCommand->pFunc;
    fError = (*pFunc)( pAbc, argc, argv );
    pAbc->TimeCommand += (Extra_CpuTime() - clk);

    // automatic execution of arbitrary command after each command 
    // usually this is a passive command ... 
    if ( fError == 0 && !pAbc->fAutoexac )
    {
        if ( st_lookup( pAbc->tFlags, "autoexec", &value ) )
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
char * CmdSplitLine( Abc_Frame_t * pAbc, char *sCommand, int *argc, char ***argv )
{
    char *p, *start, c;
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

        new_arg = ALLOC( char, p - start + 1 );
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
    char *arg, **argv, **newv;
    Abc_Alias *alias;

    argc = *argcp;
    argv = *argvp;
    stopit = 0;
    for ( ; *loop < 200; ( *loop )++ )
    {
        if ( argc == 0 )
            return 0;
        if ( stopit != 0 || st_lookup( pAbc->tAliases, argv[0],  (char **) &alias ) == 0 )
        {
            return 0;
        }
        if ( strcmp( argv[0], alias->argv[0] ) == 0 )
        {
            stopit = 1;
        }
        FREE( argv[0] );
        added = alias->argc - 1;

        /* shift all the arguments to the right */
        if ( added != 0 )
        {
            argv = REALLOC( char *, argv, argc + added );
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
                   	fError = CmdCommandDispatch( pAbc, newc, newv );
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
                argv = REALLOC( char *, argv, argc + added );
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
            FREE( newv );
            offset += added;
        }
        if ( subst == 1 )
        {
            for ( i = offset; i < argc; i++ )
            {
                FREE( argv[i] );
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
                sPathAll = ALLOC( char, strlen(sPathLib)+strlen(sPathUsr)+5 );
                sprintf( sPathAll, "%s:%s",sPathUsr, sPathLib );
            }
            if ( sPathAll != NULL ) {
                sRealName = Extra_UtilFileSearch(sFileName, sPathAll, "r");
                FREE( sPathAll );
            }
        }
        if (sRealName == NULL) {
            sRealName = Extra_UtilTildeExpand(sFileName);
        }
        if ((pFile = fopen(sRealName, sMode)) == NULL) {
            if (! silent) {
                perror(sRealName);
            }
        }
    }
    if ( pFileNameReal )
        *pFileNameReal = sRealName;
    else
        FREE(sRealName);
    
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
        FREE( argv[i] );
    FREE( argv );
}

/**Function*************************************************************

  Synopsis    [Frees the previously allocated command.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdCommandFree( Abc_Command * pCommand )
{
    free( pCommand->sGroup );
    free( pCommand->sName );
    free( pCommand );
}


/**Function*************************************************************

  Synopsis    [Prints commands alphabetically by group.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdCommandPrint( Abc_Frame_t * pAbc, bool fPrintAll )
{
    char *key, *value;
    st_generator * gen;
    Abc_Command ** ppCommands;
    Abc_Command * pCommands;
    int nCommands, i;
    char * sGroupCur;
    int LenghtMax, nColumns, iCom = 0;

    // put all commands into one array
    nCommands = st_count( pAbc->tCommands );
    ppCommands = ALLOC( Abc_Command *, nCommands );
    i = 0;
    st_foreach_item( pAbc->tCommands, gen, &key, &value )
    {
        pCommands = (Abc_Command *)value;
        if ( fPrintAll || pCommands->sName[0] != '_' )
            ppCommands[i++] = pCommands;
    }
    nCommands = i;

    // sort command by group and then by name, alphabetically
    qsort( (void *)ppCommands, nCommands, sizeof(Abc_Command *), 
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
    fprintf( pAbc->Out, "                        Welcome to ABC!" );

    // print the command by group
    sGroupCur = NULL;
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
    fprintf( pAbc->Out, "\n" );
    FREE( ppCommands );
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
void CmdPrintTable( st_table * tTable, int fAliases )
{
    st_generator * gen;
    char ** ppNames;
    char * key, * value;
    int nNames, i;

    // collect keys in the array
    ppNames = ALLOC( char *, st_count(tTable) );
    nNames = 0;
    st_foreach_item( tTable, gen, &key, &value )
        ppNames[nNames++] = key;

    // sort array by name
    qsort( (void *)ppNames, nNames, sizeof(char *), 
        (int (*)(const void *, const void *))CmdNamePrintCompare );

    // print in this order
    for ( i = 0; i < nNames; i++ )
    {
        st_lookup( tTable, ppNames[i], &value );
        if ( fAliases )
            CmdCommandAliasPrint( Abc_FrameGetGlobalFrame(), (Abc_Alias *)value );
        else
            fprintf( stdout, "%-15s %-15s\n", ppNames[i], value );
    }
    free( ppNames );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
