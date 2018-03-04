/**CFile****************************************************************

  FileName    [cmdInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Internal declarations of the command package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cmdInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__cmd__cmdInt_h
#define ABC__base__cmd__cmdInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/main/mainInt.h"
#include "cmd.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct MvCommand
{
    char *        sName;       // the command name  
    char *        sGroup;      // the group name  
    Cmd_CommandFuncType        pFunc;       // the function to execute the command
    int           fChange;     // set to 1 to mark that the network is changed
};

struct MvAlias
{
    char *        sName;       // the alias name
    int           argc;        // the number of alias parts
    char **       argv;        // the alias parts
};

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                        ///
////////////////////////////////////////////////////////////////////////

/*=== cmdAlias.c =============-========================================*/
extern void       CmdCommandAliasAdd( Abc_Frame_t * pAbc, char * sName, int argc, char ** argv );
extern void       CmdCommandAliasPrint( Abc_Frame_t * pAbc, Abc_Alias * pAlias );
extern char *     CmdCommandAliasLookup( Abc_Frame_t * pAbc, char * sCommand );
extern void       CmdCommandAliasFree( Abc_Alias * p );
/*=== cmdUtils.c =======================================================*/
extern int        CmdCommandDispatch( Abc_Frame_t * pAbc, int * argc, char *** argv );
extern const char *     CmdSplitLine( Abc_Frame_t * pAbc, const char * sCommand, int * argc, char *** argv );
extern int        CmdApplyAlias( Abc_Frame_t * pAbc, int * argc, char *** argv, int * loop );
extern char *     CmdHistorySubstitution( Abc_Frame_t * pAbc, char * line, int * changed );
extern FILE *     CmdFileOpen( Abc_Frame_t * pAbc, char * sFileName, char * sMode, char ** pFileNameReal, int silent );
extern void       CmdFreeArgv( int argc, char ** argv );
extern char **    CmdAddToArgv( int argc, char ** argv );
extern void       CmdCommandFree( Abc_Command * pCommand );
extern void       CmdCommandPrint( Abc_Frame_t * pAbc, int fPrintAll, int fDetails );
extern void       CmdPrintTable( st__table * tTable, int fAliases );

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_END

#endif

