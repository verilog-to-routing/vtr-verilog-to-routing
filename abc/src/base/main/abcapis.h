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
 
#ifndef MINI_AIG__abc_apis_h
#define MINI_AIG__abc_apis_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

typedef struct Abc_Frame_t_      Abc_Frame_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#ifdef WIN32
  #ifdef WIN32_NO_DLL
    #define ABC_DLLEXPORT
    #define ABC_DLLIMPORT
  #else
    #define ABC_DLLEXPORT __declspec(dllexport)
    #define ABC_DLLIMPORT __declspec(dllimport)
  #endif
#else  /* defined(WIN32) */
#define ABC_DLLIMPORT
#endif /* defined(WIN32) */

#ifndef ABC_DLL
#define ABC_DLL ABC_DLLIMPORT
#endif

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

// procedures to start and stop the ABC framework
extern ABC_DLL void   Abc_Start();
extern ABC_DLL void   Abc_Stop();

// procedures to get the ABC framework (pAbc) and execute commands in it
extern ABC_DLL Abc_Frame_t * Abc_FrameGetGlobalFrame();
extern ABC_DLL int   Cmd_CommandExecute( Abc_Frame_t * pAbc, const char * pCommandLine );

// procedures to input/output 'mini AIG'
extern ABC_DLL void   Abc_NtkInputMiniAig( Abc_Frame_t * pAbc, void * pMiniAig );
extern ABC_DLL void * Abc_NtkOutputMiniAig( Abc_Frame_t * pAbc );
extern ABC_DLL void   Abc_FrameGiaInputMiniAig( Abc_Frame_t * pAbc, void * p );
extern ABC_DLL void * Abc_FrameGiaOutputMiniAig( Abc_Frame_t * pAbc );
extern ABC_DLL void   Abc_NtkSetFlopNum( Abc_Frame_t * pAbc, int nFlops );

// procedures to input/output 'mini LUT'
extern ABC_DLL void   Abc_FrameGiaInputMiniLut( Abc_Frame_t * pAbc, void * pMiniLut );
extern ABC_DLL void   Abc_FrameGiaInputMiniLut2( Abc_Frame_t * pAbc, void * pMiniLut );
extern ABC_DLL void * Abc_FrameGiaOutputMiniLut( Abc_Frame_t * pAbc );
extern ABC_DLL char * Abc_FrameGiaOutputMiniLutAttr( Abc_Frame_t * pAbc, void * pMiniLut );
extern ABC_DLL int *  Abc_FrameReadMiniLutSwitching( Abc_Frame_t * pAbc );
extern ABC_DLL int *  Abc_FrameReadMiniLutSwitchingPo( Abc_Frame_t * pAbc );

// procedures to input/output NDR data-structure
extern ABC_DLL void   Abc_FrameInputNdr( Abc_Frame_t * pAbc, void * pData );
extern ABC_DLL void * Abc_FrameOutputNdr( Abc_Frame_t * pAbc );
extern ABC_DLL int  * Abc_FrameOutputNdrArray( Abc_Frame_t * pAbc );

// procedures to set CI/CO arrival/required times
extern ABC_DLL void   Abc_NtkSetCiArrivalTime( Abc_Frame_t * pAbc, int iCi, float Rise, float Fall );
extern ABC_DLL void   Abc_NtkSetCoRequiredTime( Abc_Frame_t * pAbc, int iCo, float Rise, float Fall );

// procedure to set AND-gate delay to tech-independent synthesis and mapping
extern ABC_DLL void   Abc_NtkSetAndGateDelay( Abc_Frame_t * pAbc, float Delay );

// procedures to return the mapped network
extern ABC_DLL int *  Abc_NtkOutputMiniMapping( Abc_Frame_t * pAbc );
extern ABC_DLL void   Abc_NtkPrintMiniMapping( int * pArray );
extern ABC_DLL int *  Abc_FrameReadArrayMapping( Abc_Frame_t * pAbc );              
extern ABC_DLL int *  Abc_FrameReadBoxes( Abc_Frame_t * pAbc );

// procedures to access verifization status and a counter-example
extern ABC_DLL int    Abc_FrameReadProbStatus( Abc_Frame_t * pAbc );   
extern ABC_DLL void * Abc_FrameReadCex( Abc_Frame_t * pAbc );    

// procedure to return sequential equivalences
extern ABC_DLL int *  Abc_FrameReadMiniAigEquivClasses( Abc_Frame_t * pAbc );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

