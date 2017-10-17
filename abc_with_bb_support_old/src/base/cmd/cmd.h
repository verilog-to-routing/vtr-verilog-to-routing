/**CFile****************************************************************

  FileName    [cmd.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [External declarations of the command package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cmd.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef __CMD_H__
#define __CMD_H__

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct MvCommand    Abc_Command;  // one command
typedef struct MvAlias      Abc_Alias;    // one alias

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== cmd.c ===========================================================*/
extern void        Cmd_Init();
extern void        Cmd_End();
/*=== cmdApi.c ========================================================*/
extern void        Cmd_CommandAdd( Abc_Frame_t * pAbc, char * sGroup, char * sName, void * pFunc, int fChanges );
extern int         Cmd_CommandExecute( Abc_Frame_t * pAbc, char * sCommand );
/*=== cmdFlag.c ========================================================*/
extern char *      Cmd_FlagReadByName( Abc_Frame_t * pAbc, char * flag );
extern void        Cmd_FlagDeleteByName( Abc_Frame_t * pAbc, char * key );
extern void        Cmd_FlagUpdateValue( Abc_Frame_t * pAbc, char * key, char * value );
/*=== cmdHist.c ========================================================*/
extern void   	   Cmd_HistoryAddCommand( Abc_Frame_t * pAbc, char * command );

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

