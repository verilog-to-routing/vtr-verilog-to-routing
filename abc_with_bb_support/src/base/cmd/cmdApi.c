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

#include "mainInt.h"
#include "cmdInt.h"
#include "abc.h"

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
void Cmd_CommandAdd( Abc_Frame_t * pAbc, char * sGroup, char * sName, void * pFunc, int fChanges )
{
    char * key, * value;
    Abc_Command * pCommand;
    int fStatus;

    key = sName;
    if ( st_delete( pAbc->tCommands, &key, &value ) ) 
    {
        // delete existing definition for this command 
        fprintf( pAbc->Err, "Cmd warning: redefining '%s'\n", sName );
        CmdCommandFree( (Abc_Command *)value );
    }

    // create the new command
    pCommand = ALLOC( Abc_Command, 1 );
    pCommand->sName   = Extra_UtilStrsav( sName );
    pCommand->sGroup  = Extra_UtilStrsav( sGroup );
    pCommand->pFunc   = pFunc;
    pCommand->fChange = fChanges;
    fStatus = st_insert( pAbc->tCommands, sName, (char *)pCommand );
    assert( !fStatus );  // the command should not be in the table
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cmd_CommandExecute( Abc_Frame_t * pAbc, char * sCommand )
{
    int fStatus = 0, argc, loop;
    char * sCommandNext, **argv;

    if ( !pAbc->fAutoexac ) 
	    Cmd_HistoryAddCommand(pAbc, sCommand);
    sCommandNext = sCommand;
    do 
    {
       	sCommandNext = CmdSplitLine( pAbc, sCommandNext, &argc, &argv );
		loop = 0;
		fStatus = CmdApplyAlias( pAbc, &argc, &argv, &loop );
		if ( fStatus == 0 ) 
			fStatus = CmdCommandDispatch( pAbc, argc, argv );
       	CmdFreeArgv( argc, argv );
    } 
    while ( fStatus == 0 && *sCommandNext != '\0' );
    return fStatus;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


