/**CFile****************************************************************

  FileName    [main.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The main package.]

  Synopsis    [External declarations of the main package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: main.h,v 1.1 2008/05/14 22:13:13 wudenni Exp $]

***********************************************************************/

#ifndef ABC__base__main__main_h
#define ABC__base__main__main_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

// core packages
#include "base/abc/abc.h"
#include "aig/gia/gia.h"

// data structure packages
#include "misc/vec/vec.h"
#include "misc/st/st.h"

// the framework containing all data is defined here
#include "abcapis.h"

#include "base/cmd/cmd.h"
#include "base/io/ioAbc.h"

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         TYPEDEFS                                 ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== main.c ===========================================================*/
extern ABC_DLL void            Abc_Start();
extern ABC_DLL void            Abc_Stop();

/*=== mainFrame.c ===========================================================*/
extern ABC_DLL Abc_Ntk_t *     Abc_FrameReadNtk( Abc_Frame_t * p );
extern ABC_DLL Gia_Man_t *     Abc_FrameReadGia( Abc_Frame_t * p );
extern ABC_DLL FILE *          Abc_FrameReadOut( Abc_Frame_t * p );
extern ABC_DLL FILE *          Abc_FrameReadErr( Abc_Frame_t * p );
extern ABC_DLL int             Abc_FrameReadMode( Abc_Frame_t * p );
extern ABC_DLL int             Abc_FrameSetMode( Abc_Frame_t * p, int fNameMode );
extern ABC_DLL void            Abc_FrameRestart( Abc_Frame_t * p );
extern ABC_DLL int             Abc_FrameShowProgress( Abc_Frame_t * p );
extern ABC_DLL void            Abc_FrameClearVerifStatus( Abc_Frame_t * p );
extern ABC_DLL void            Abc_FrameUpdateGia( Abc_Frame_t * p, Gia_Man_t * pNew );
extern ABC_DLL Gia_Man_t *     Abc_FrameGetGia( Abc_Frame_t * p );

extern ABC_DLL void            Abc_FrameSetCurrentNetwork( Abc_Frame_t * p, Abc_Ntk_t * pNet );
extern ABC_DLL void            Abc_FrameSwapCurrentAndBackup( Abc_Frame_t * p );
extern ABC_DLL void            Abc_FrameReplaceCurrentNetwork( Abc_Frame_t * p, Abc_Ntk_t * pNet );
extern ABC_DLL void            Abc_FrameUnmapAllNetworks( Abc_Frame_t * p );
extern ABC_DLL void            Abc_FrameDeleteAllNetworks( Abc_Frame_t * p );

extern ABC_DLL void            Abc_FrameSetGlobalFrame( Abc_Frame_t * p );
extern ABC_DLL Abc_Frame_t *   Abc_FrameGetGlobalFrame();
extern ABC_DLL Abc_Frame_t *   Abc_FrameReadGlobalFrame();

extern ABC_DLL Vec_Ptr_t *     Abc_FrameReadStore();                  
extern ABC_DLL int             Abc_FrameReadStoreSize();              
extern ABC_DLL void *          Abc_FrameReadLibLut();                    
extern ABC_DLL void *          Abc_FrameReadLibBox();                    
extern ABC_DLL void *          Abc_FrameReadLibGen();                    
extern ABC_DLL void *          Abc_FrameReadLibGen2();                    
extern ABC_DLL void *          Abc_FrameReadLibSuper();                  
extern ABC_DLL void *          Abc_FrameReadLibScl();                  
extern ABC_DLL void *          Abc_FrameReadManDd();                     
extern ABC_DLL void *          Abc_FrameReadManDec();                    
extern ABC_DLL void *          Abc_FrameReadManDsd();           
extern ABC_DLL void *          Abc_FrameReadManDsd2();           
extern ABC_DLL Vec_Ptr_t *     Abc_FrameReadSignalNames();
extern ABC_DLL char *          Abc_FrameReadSpecName();
         
extern ABC_DLL char *          Abc_FrameReadFlag( char * pFlag ); 
extern ABC_DLL int             Abc_FrameIsFlagEnabled( char * pFlag );
extern ABC_DLL int             Abc_FrameIsBatchMode();
extern ABC_DLL void            Abc_FrameSetBatchMode( int Mode );
extern ABC_DLL int             Abc_FrameIsBridgeMode();
extern ABC_DLL void            Abc_FrameSetBridgeMode();

extern ABC_DLL int             Abc_FrameReadBmcFrames( Abc_Frame_t * p );              
extern ABC_DLL int             Abc_FrameReadProbStatus( Abc_Frame_t * p );              
extern ABC_DLL void *          Abc_FrameReadCex( Abc_Frame_t * p );              
extern ABC_DLL Vec_Ptr_t *     Abc_FrameReadCexVec( Abc_Frame_t * p );  
extern ABC_DLL Vec_Int_t *     Abc_FrameReadStatusVec( Abc_Frame_t * p );  
extern ABC_DLL Vec_Ptr_t *     Abc_FrameReadPoEquivs( Abc_Frame_t * p );  
extern ABC_DLL Vec_Int_t *     Abc_FrameReadPoStatuses( Abc_Frame_t * p );  
extern ABC_DLL Vec_Int_t *     Abc_FrameReadObjIds( Abc_Frame_t * p );
extern ABC_DLL Abc_Nam_t *     Abc_FrameReadJsonStrs( Abc_Frame_t * p );  
extern ABC_DLL Vec_Wec_t *     Abc_FrameReadJsonObjs( Abc_Frame_t * p );

extern ABC_DLL int             Abc_FrameReadCexPiNum( Abc_Frame_t * p );              
extern ABC_DLL int             Abc_FrameReadCexRegNum( Abc_Frame_t * p );              
extern ABC_DLL int             Abc_FrameReadCexPo( Abc_Frame_t * p );              
extern ABC_DLL int             Abc_FrameReadCexFrame( Abc_Frame_t * p );              

extern ABC_DLL void            Abc_FrameSetNtkStore( Abc_Ntk_t * pNtk ); 
extern ABC_DLL void            Abc_FrameSetNtkStoreSize( int nStored );  
extern ABC_DLL void            Abc_FrameSetLibLut( void * pLib );        
extern ABC_DLL void            Abc_FrameSetLibBox( void * pLib );        
extern ABC_DLL void            Abc_FrameSetLibGen( void * pLib );        
extern ABC_DLL void            Abc_FrameSetLibGen2( void * pLib );        
extern ABC_DLL void            Abc_FrameSetLibSuper( void * pLib );      
extern ABC_DLL void            Abc_FrameSetLibVer( void * pLib );      
extern ABC_DLL void            Abc_FrameSetFlag( char * pFlag, char * pValue );
extern ABC_DLL void            Abc_FrameSetCex( Abc_Cex_t * pCex );
extern ABC_DLL void            Abc_FrameSetNFrames( int nFrames );
extern ABC_DLL void            Abc_FrameSetStatus( int Status );
extern ABC_DLL void            Abc_FrameSetManDsd( void * pMan );
extern ABC_DLL void            Abc_FrameSetManDsd2( void * pMan );
extern ABC_DLL void            Abc_FrameSetInv( Vec_Int_t * vInv );
extern ABC_DLL void            Abc_FrameSetCnf( Vec_Int_t * vInv );
extern ABC_DLL void            Abc_FrameSetStr( Vec_Str_t * vInv );
extern ABC_DLL void            Abc_FrameSetJsonStrs( Abc_Nam_t * pStrs );
extern ABC_DLL void            Abc_FrameSetJsonObjs( Vec_Wec_t * vObjs );
extern ABC_DLL void            Abc_FrameSetSignalNames( Vec_Ptr_t * vNames );
extern ABC_DLL void            Abc_FrameSetSpecName( char * pFileName );

extern ABC_DLL int             Abc_FrameCheckPoConst( Abc_Frame_t * p, int iPoNum );

extern ABC_DLL void            Abc_FrameReplaceCex( Abc_Frame_t * pAbc, Abc_Cex_t ** ppCex );
extern ABC_DLL void            Abc_FrameReplaceCexVec( Abc_Frame_t * pAbc, Vec_Ptr_t ** pvCexVec );
extern ABC_DLL void            Abc_FrameReplacePoEquivs( Abc_Frame_t * pAbc, Vec_Ptr_t ** pvPoEquivs );
extern ABC_DLL void            Abc_FrameReplacePoStatuses( Abc_Frame_t * pAbc, Vec_Int_t ** pvStatuses );

extern ABC_DLL char *          Abc_FrameReadDrivingCell();              
extern ABC_DLL float           Abc_FrameReadMaxLoad();
extern ABC_DLL void            Abc_FrameSetDrivingCell( char * pName );
extern ABC_DLL void            Abc_FrameSetMaxLoad( float Load );

extern ABC_DLL void            Abc_FrameSetArrayMapping( int * p );
extern ABC_DLL void            Abc_FrameSetBoxes( int * p );

ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
