/**CFile****************************************************************

  FileName    [mainInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The main package.]

  Synopsis    [Internal declarations of the main package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mainInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef __MAIN_INT_H__
#define __MAIN_INT_H__

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "main.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// the current version
#define ABC_VERSION "UC Berkeley, ABC for VTR 7.0"

// the maximum length of an input line 
#define MAX_STR     32768

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

struct Abc_Frame_t_
{
    // general info
    char *          sVersion;    // the name of the current version
    // commands, aliases, etc
    st_table *      tCommands;   // the command table
    st_table *      tAliases;    // the alias table
    st_table *      tFlags;      // the flag table
    Vec_Ptr_t *     aHistory;    // the command history
    // the functionality
    Abc_Ntk_t *     pNtkCur;     // the current network
    int             nSteps;      // the counter of different network processed
    int             fAutoexac;   // marks the autoexec mode
	int				fBatchMode;	 // are we invoked in batch mode?
    // output streams
    FILE *          Out;
    FILE *          Err;
    FILE *          Hst;
    // used for runtime measurement
    int             TimeCommand; // the runtime of the last command
    int             TimeTotal;   // the total runtime of all commands
    // temporary storage for structural choices
    Abc_Ntk_t *     pStored;     // the stored networks
    int             nStored;     // the number of stored networks
    // decomposition package
    void *          pManDec;     // decomposition manager
    DdManager *     dd;          // temporary BDD package
    // libraries for mapping
    void *          pLibLut;     // the current LUT library
    void *          pLibGen;     // the current genlib
    void *          pLibSuper;   // the current supergate library
    void *          pLibVer;     // the current Verilog library
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== mvMain.c ===========================================================*/
extern int             main( int argc, char * argv[] );
/*=== mvInit.c ===================================================*/
extern void            Abc_FrameInit( Abc_Frame_t * pAbc );
extern void            Abc_FrameEnd( Abc_Frame_t * pAbc );
/*=== mvFrame.c =====================================================*/
extern Abc_Frame_t *   Abc_FrameAllocate();
extern void            Abc_FrameDeallocate( Abc_Frame_t * p );
/*=== mvUtils.c =====================================================*/
extern char *          Abc_UtilsGetVersion( Abc_Frame_t * pAbc );
extern char *          Abc_UtilsGetUsersInput( Abc_Frame_t * pAbc );
extern void            Abc_UtilsPrintHello( Abc_Frame_t * pAbc );
extern void            Abc_UtilsPrintUsage( Abc_Frame_t * pAbc, char * ProgName );
extern void            Abc_UtilsSource( Abc_Frame_t * pAbc );

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
