/**CFile****************************************************************

  FileName    [abcapis.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Include this file in the external code calling ABC.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 29, 2012.]

  Revision    [$Id: abcapis.h,v 1.00 2012/09/29 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef MINI_AIG__abc_apis_old_h
#define MINI_AIG__abc_apis_old_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

// procedures to start and stop the ABC framework
extern void   Abc_Start();
extern void   Abc_Stop();

// procedures to get the ABC framework (pAbc) and execute commands in it
extern void * Abc_FrameGetGlobalFrame();
extern int    Cmd_CommandExecute( void * pAbc, char * pCommandLine );

// procedures to input/output 'mini AIG'
extern void   Abc_NtkInputMiniAig( void * pAbc, void * pMiniAig );
extern void * Abc_NtkOutputMiniAig( void * pAbc );
extern void   Abc_FrameGiaInputMiniAig( void * pAbc, void * p );
extern void * Abc_FrameGiaOutputMiniAig( void * pAbc );
extern void   Abc_NtkSetFlopNum( void * pAbc, int nFlops );

// procedures to input/output 'mini LUT'
extern void   Abc_FrameGiaInputMiniLut( void * pAbc, void * pMiniLut );
extern void * Abc_FrameGiaOutputMiniLut( void * pAbc );

// procedures to set CI/CO arrival/required times
extern void   Abc_NtkSetCiArrivalTime( void * pAbc, int iCi, float Rise, float Fall );
extern void   Abc_NtkSetCoRequiredTime( void * pAbc, int iCo, float Rise, float Fall );

// procedure to set AND-gate delay to tech-independent synthesis and mapping
extern void   Abc_NtkSetAndGateDelay( void * pAbc, float Delay );

// procedures to return the mapped network
extern int *  Abc_NtkOutputMiniMapping( void * pAbc );
extern void   Abc_NtkPrintMiniMapping( int * pArray );

// procedures to access verifization status and a counter-example
extern int    Abc_FrameReadProbStatus( void * pAbc );   
extern void * Abc_FrameReadCex( void * pAbc );    


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

