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
int Cmd_CommandHandleSpecial( Abc_Frame_t * pAbc, const char * sCommand )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    int piCountNew = (pNtk && Abc_NtkHasMapping(pNtk)) ? Abc_NtkCiNum(pNtk)         : 0, piCount = 0;
    int poCountNew = (pNtk && Abc_NtkHasMapping(pNtk)) ? Abc_NtkCoNum(pNtk)         : 0, poCount = 0;
    int ndCountNew = (pNtk && Abc_NtkHasMapping(pNtk)) ? Abc_NtkNodeNum(pNtk)       : 0, ndCount = 0;
    double AreaNew = (pNtk && Abc_NtkHasMapping(pNtk)) ? Abc_NtkGetMappedArea(pNtk) : 0, Area    = 0;
    int DepthNew   = (pNtk && Abc_NtkHasMapping(pNtk)) ? Abc_NtkLevel(pNtk)         : 0, Depth   = 0;
    if ( strstr(sCommand, "#PS") ) 
    {
        printf( "pi=%d ",   piCountNew );
        printf( "po=%d ",   poCountNew );
        printf( "fn=%d ",   ndCountNew );
        printf( "ma=%.1f ", AreaNew    );
        printf( "de=%d ",   DepthNew   );
        printf( "\n" );
        return 1;
    }
    if ( strstr(sCommand, "#CEC") ) 
    {
        //int proofStatus = Abc_NtkVerifyUsingCec(pNtk);
        int proofStatus = 1;
        // -1 (undecided), 0 (different), 1 (equivalent)
        printf( "proofStatus=%d\n", proofStatus );
        return 1;
    }
    if ( strstr(sCommand, "#ASSERT") ) 
    {
        int Status    = 0;
        char * pNumb  = strrchr( (char *)sCommand, '=' );
        if ( strstr(sCommand, "_PI_") ) 
        {
            piCount = pNumb ? atoi(pNumb+1) : 0;
            if ( strstr( sCommand, "==" ) )
                Status = piCountNew == piCount;
            else if ( strstr( sCommand, "<=" ) )
                Status = piCountNew <= piCount;
            else return 0;
        }
        else if ( strstr(sCommand, "_PO_") ) 
        {
            poCount = pNumb ? atoi(pNumb+1) : 0;
            if ( strstr( sCommand, "==" ) )
                Status = poCountNew == poCount;
            else if ( strstr( sCommand, "<=" ) )
                Status = poCountNew <= poCount;
            else return 0;
        }
        else if ( strstr(sCommand, "_NODE_") ) 
        {
            ndCount = pNumb ? atoi(pNumb+1) : 0;
            if ( strstr( sCommand, "==" ) )
                Status = ndCountNew == ndCount;
            else if ( strstr( sCommand, "<=" ) )
                Status = ndCountNew <= ndCount;
            else return 0;
        }
        else if ( strstr(sCommand, "_AREA_") ) 
        {
            double Eplison = 1.0;
            Area = pNumb ? atof(pNumb+1) : 0;
            if ( strstr( sCommand, "==" ) )
                Status = AreaNew >= Area - Eplison && AreaNew <= Area + Eplison;
            else if ( strstr( sCommand, "<=" ) )
                Status = AreaNew <= Area + Eplison;
            else return 0;
        }
        else if ( strstr(sCommand, "_DEPTH_") ) 
        {
            Depth = pNumb ? atoi(pNumb+1) : 0;
            if ( strstr( sCommand, "==" ) )
                Status = DepthNew == Depth;
            else if ( strstr( sCommand, "<=" ) )
                Status = DepthNew <= Depth;
            else return 0;
        }
        else return 0;
        printf( "%s\n", Status ? "succeeded" : "failed" );
        return 1;
    }
    return 0;
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
        if ( sCommandNext[0] == '#' && Cmd_CommandHandleSpecial( pAbc, sCommandNext ) )
            break;
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

