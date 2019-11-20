/**CFile****************************************************************

  FileName    [main.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The main package.]

  Synopsis    [External declarations of the main package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: main.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                         TYPEDEFS                                 ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the framework containing all data
typedef struct Abc_Frame_t_      Abc_Frame_t;   

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

// this include should be the first one in the list
// it is used to catch memory leaks on Windows
#include "leaks.h"       

// data structure packages
#include "extra.h"
#include "vec.h"
#include "st.h"

// core packages
#include "abc.h"
#include "cmd.h"
#include "io.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== main.c ===========================================================*/
extern void            Abc_Start();
extern void            Abc_Stop();

/*=== mainFrame.c ===========================================================*/
extern Abc_Ntk_t *     Abc_FrameReadNtk( Abc_Frame_t * p );
extern FILE *          Abc_FrameReadOut( Abc_Frame_t * p );
extern FILE *          Abc_FrameReadErr( Abc_Frame_t * p );
extern bool            Abc_FrameReadMode( Abc_Frame_t * p );
extern bool            Abc_FrameSetMode( Abc_Frame_t * p, bool fNameMode );
extern void            Abc_FrameRestart( Abc_Frame_t * p );
extern bool            Abc_FrameShowProgress( Abc_Frame_t * p );

extern void            Abc_FrameSetCurrentNetwork( Abc_Frame_t * p, Abc_Ntk_t * pNet );
extern void            Abc_FrameSwapCurrentAndBackup( Abc_Frame_t * p );
extern void            Abc_FrameReplaceCurrentNetwork( Abc_Frame_t * p, Abc_Ntk_t * pNet );
extern void            Abc_FrameUnmapAllNetworks( Abc_Frame_t * p );
extern void            Abc_FrameDeleteAllNetworks( Abc_Frame_t * p );

extern void			   Abc_FrameSetGlobalFrame( Abc_Frame_t * p );
extern Abc_Frame_t *   Abc_FrameGetGlobalFrame();

extern Abc_Ntk_t *     Abc_FrameReadNtkStore();                  
extern int             Abc_FrameReadNtkStoreSize();              
extern void *          Abc_FrameReadLibLut();                    
extern void *          Abc_FrameReadLibGen();                    
extern void *          Abc_FrameReadLibSuper();                  
extern void *          Abc_FrameReadLibVer();                  
extern void *          Abc_FrameReadManDd();                     
extern void *          Abc_FrameReadManDec();                    
extern char *          Abc_FrameReadFlag( char * pFlag ); 
extern bool            Abc_FrameIsFlagEnabled( char * pFlag );

extern void            Abc_FrameSetNtkStore( Abc_Ntk_t * pNtk ); 
extern void            Abc_FrameSetNtkStoreSize( int nStored );  
extern void            Abc_FrameSetLibLut( void * pLib );        
extern void            Abc_FrameSetLibGen( void * pLib );        
extern void            Abc_FrameSetLibSuper( void * pLib );      
extern void            Abc_FrameSetLibVer( void * pLib );      
extern void            Abc_FrameSetFlag( char * pFlag, char * pValue );

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
