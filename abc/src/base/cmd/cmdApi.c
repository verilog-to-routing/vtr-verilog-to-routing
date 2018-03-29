/**CFile****************************************************************

  FileName    [cmdApi.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [External procedures of the command package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cmdApi.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/mainInt.h"
#include "cmdInt.h"

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
int Cmd_CommandIsDefined( Abc_Frame_t * pAbc, const char * sName )
{
    return st__is_member( pAbc->tCommands, sName );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cmd_CommandAdd( Abc_Frame_t * pAbc, const char * sGroup, const char * sName, Cmd_CommandFuncType pFunc, int fChanges )
{
    const char * key;
    char * value;
    Abc_Command * pCommand;
    int fStatus;

    key = sName;
    if ( st__delete( pAbc->tCommands, &key, &value ) ) 
    {
        // delete existing definition for this command 
        fprintf( pAbc->Err, "Cmd warning: redefining '%s'\n", sName );
        CmdCommandFree( (Abc_Command *)value );
    }

    // create the new command
    pCommand = ABC_ALLOC( Abc_Command, 1 );
    pCommand->sName   = Extra_UtilStrsav( sName );
    pCommand->sGroup  = Extra_UtilStrsav( sGroup );
    pCommand->pFunc   = pFunc;
    pCommand->fChange = fChanges;
    fStatus = st__insert( pAbc->tCommands, pCommand->sName, (char *)pCommand );
    assert( !fStatus );  // the command should not be in the table
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cmd_CommandExecute( Abc_Frame_t * pAbc, const char * sCommand )
{
    int fStatus = 0, argc, loop;
    const char * sCommandNext;
    char **argv;

    if ( !pAbc->fAutoexac && !pAbc->fSource ) 
        Cmd_HistoryAddCommand(pAbc, sCommand);
    sCommandNext = sCommand;
    do 
    {
           sCommandNext = CmdSplitLine( pAbc, sCommandNext, &argc, &argv );
        loop = 0;
        fStatus = CmdApplyAlias( pAbc, &argc, &argv, &loop );
        if ( fStatus == 0 ) 
            fStatus = CmdCommandDispatch( pAbc, &argc, &argv );
           CmdFreeArgv( argc, argv );
    } 
    while ( fStatus == 0 && *sCommandNext != '\0' );
    return fStatus;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

