/**CFile****************************************************************

  FileName    [cmdAlias.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures dealing with aliases in the command package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cmdAlias.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
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
void CmdCommandAliasAdd( Abc_Frame_t * pAbc, char * sName, int argc, char ** argv )
{
    Abc_Alias * pAlias;
    int fStatus, i;

    pAlias = ABC_ALLOC(Abc_Alias, 1);
    pAlias->sName = Extra_UtilStrsav(sName);
    pAlias->argc = argc;
    pAlias->argv = ABC_ALLOC(char *, pAlias->argc);
    for(i = 0; i < argc; i++) 
        pAlias->argv[i] = Extra_UtilStrsav(argv[i]);
    fStatus = st__insert( pAbc->tAliases, pAlias->sName, (char *) pAlias );
    assert(!fStatus);  
}

/**Function********************************************************************

  Synopsis    [required]

  Description [optional]

  SideEffects [required]

  SeeAlso     [optional]

******************************************************************************/
void CmdCommandAliasPrint( Abc_Frame_t * pAbc, Abc_Alias * pAlias )
{
    int i;
    fprintf(pAbc->Out, "%-15s", pAlias->sName);
    for(i = 0; i < pAlias->argc; i++) 
        fprintf( pAbc->Out, " %s", pAlias->argv[i] );
    fprintf( pAbc->Out, "\n" );
}

/**Function********************************************************************

  Synopsis    [required]

  Description [optional]

  SideEffects [required]

  SeeAlso     [optional]

******************************************************************************/
char * CmdCommandAliasLookup( Abc_Frame_t * pAbc, char * sCommand )
{
  Abc_Alias * pAlias;
  char * value;
  if (! st__lookup( pAbc->tAliases, sCommand, &value)) 
    return sCommand;
  pAlias = (Abc_Alias *) value;
  return pAlias->argv[0];
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void CmdCommandAliasFree( Abc_Alias * pAlias )
{
    CmdFreeArgv( pAlias->argc, pAlias->argv );
    ABC_FREE(pAlias->sName);    
    ABC_FREE(pAlias);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

