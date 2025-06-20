/**CFile****************************************************************

  FileName    [cmdFlag.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures working with flags.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cmdFlag.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function********************************************************************

  Synopsis    [Looks up value of flag in table of named values.]

  Description [The command parser maintains a table of named values.  These
  are manipulated using the 'set' and 'unset' commands.  The value of the
  named flag is returned, or NULL is returned if the flag has not been set.]

  SideEffects []

******************************************************************************/
char * Cmd_FlagReadByName( Abc_Frame_t * pAbc, char * flag )
{
    char * value;
    if ( st__lookup(pAbc->tFlags, flag, &value) )
        return value;
    return NULL;
}


/**Function********************************************************************

  Synopsis    [Updates a set value by calling instead of set command.]

  Description [Updates a set value by calling instead of set command.]

  SideEffects []

******************************************************************************/
void Cmd_FlagUpdateValue( Abc_Frame_t * pAbc, const char * key, char * value )
{
    char * oldValue, * newValue;
    if ( !key )
        return;
    if ( value )
        newValue = Extra_UtilStrsav(value);
    else
        newValue = Extra_UtilStrsav("");
//        newValue = NULL;
    if ( st__delete(pAbc->tFlags, &key, &oldValue) )
        ABC_FREE(oldValue);
    st__insert( pAbc->tFlags, key, newValue );
}


/**Function********************************************************************

  Synopsis    [Deletes a set value by calling instead of unset command.]

  Description [Deletes a set value by calling instead of unset command.]

  SideEffects []

******************************************************************************/
void Cmd_FlagDeleteByName( Abc_Frame_t * pAbc, const char * key )
{
    char *value;
    if ( !key )
        return;
    if ( st__delete( pAbc->tFlags, &key, &value ) ) 
    {
        ABC_FREE(key);
        ABC_FREE(value);
    }
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

