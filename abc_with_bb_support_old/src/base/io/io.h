/**CFile****************************************************************

  FileName    [io.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: io.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef __IO_H__
#define __IO_H__

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "abc.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// network functionality
typedef enum { 
    IO_FILE_NONE = 0, 
    IO_FILE_AIGER,      
    IO_FILE_BAF,      
    IO_FILE_BLIF,      
    IO_FILE_BLIFMV,      
    IO_FILE_BENCH,      
    IO_FILE_CNF,      
    IO_FILE_DOT,      
    IO_FILE_EDIF,      
    IO_FILE_EQN,      
    IO_FILE_GML,      
    IO_FILE_LIST,      
    IO_FILE_PLA,      
    IO_FILE_VERILOG,    
    IO_FILE_UNKNOWN       
} Io_FileType_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define  IO_WRITE_LINE_LENGTH    78    // the output line length

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== abcReadAiger.c ==========================================================*/
extern Abc_Ntk_t *        Io_ReadAiger( char * pFileName, int fCheck );
/*=== abcReadBaf.c ============================================================*/
extern Abc_Ntk_t *        Io_ReadBaf( char * pFileName, int fCheck );
/*=== abcReadBlif.c ===========================================================*/
extern Abc_Ntk_t *        Io_ReadBlif( char * pFileName, int fCheck );
/*=== abcReadBlifMv.c =========================================================*/
extern Abc_Ntk_t *        Io_ReadBlifMv( char * pFileName, int fBlifMv, int fCheck );
/*=== abcReadBench.c ==========================================================*/
extern Abc_Ntk_t *        Io_ReadBench( char * pFileName, int fCheck );
/*=== abcReadEdif.c ===========================================================*/
extern Abc_Ntk_t *        Io_ReadEdif( char * pFileName, int fCheck );
/*=== abcReadEqn.c ============================================================*/
extern Abc_Ntk_t *        Io_ReadEqn( char * pFileName, int fCheck );
/*=== abcReadPla.c ============================================================*/
extern Abc_Ntk_t *        Io_ReadPla( char * pFileName, int fCheck );
/*=== abcReadVerilog.c ========================================================*/
extern Abc_Ntk_t *        Io_ReadVerilog( char * pFileName, int fCheck );
/*=== abcWriteAiger.c =========================================================*/
extern void               Io_WriteAiger( Abc_Ntk_t * pNtk, char * pFileName, int fWriteSymbols );
/*=== abcWriteBaf.c ===========================================================*/
extern void               Io_WriteBaf( Abc_Ntk_t * pNtk, char * pFileName );
/*=== abcWriteBlif.c ==========================================================*/
extern void               Io_WriteBlifLogic( Abc_Ntk_t * pNtk, char * pFileName, int fWriteLatches );
extern void               Io_WriteBlif( Abc_Ntk_t * pNtk, char * pFileName, int fWriteLatches );
extern void               Io_WriteTimingInfo( FILE * pFile, Abc_Ntk_t * pNtk );
/*=== abcWriteBlifMv.c ==========================================================*/ 
extern void               Io_WriteBlifMv( Abc_Ntk_t * pNtk, char * FileName );
/*=== abcWriteBench.c =========================================================*/
extern int                Io_WriteBench( Abc_Ntk_t * pNtk, char * FileName );
extern int                Io_WriteBenchLut( Abc_Ntk_t * pNtk, char * FileName );
/*=== abcWriteCnf.c ===========================================================*/
extern int                Io_WriteCnf( Abc_Ntk_t * pNtk, char * FileName, int fAllPrimes );
/*=== abcWriteDot.c ===========================================================*/
extern void               Io_WriteDot( Abc_Ntk_t * pNtk, char * FileName );
extern void               Io_WriteDotNtk( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vNodesShow, char * pFileName, int fGateNames, int fUseReverse );
extern void               Io_WriteDotSeq( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vNodesShow, char * pFileName, int fGateNames, int fUseReverse );
/*=== abcWriteEqn.c ===========================================================*/
extern void               Io_WriteEqn( Abc_Ntk_t * pNtk, char * pFileName );
/*=== abcWriteGml.c ===========================================================*/
extern void               Io_WriteGml( Abc_Ntk_t * pNtk, char * pFileName );
/*=== abcWriteList.c ==========================================================*/
extern void               Io_WriteList( Abc_Ntk_t * pNtk, char * pFileName, int fUseHost );
/*=== abcWritePla.c ===========================================================*/
extern int                Io_WritePla( Abc_Ntk_t * pNtk, char * FileName );
/*=== abcWriteVerilog.c =======================================================*/
extern void               Io_WriteVerilog( Abc_Ntk_t * pNtk, char * FileName );
/*=== abcUtil.c ===============================================================*/
extern Io_FileType_t      Io_ReadFileType( char * pFileName );
extern Abc_Ntk_t *        Io_ReadNetlist( char * pFileName, Io_FileType_t FileType, int fCheck );
extern Abc_Ntk_t *        Io_Read( char * pFileName, Io_FileType_t FileType, int fCheck );
extern void               Io_Write( Abc_Ntk_t * pNtk, char * pFileName, Io_FileType_t FileType );
extern void               Io_WriteHie( Abc_Ntk_t * pNtk, char * pBaseName, char * pFileName );
extern Abc_Obj_t *        Io_ReadCreatePi( Abc_Ntk_t * pNtk, char * pName );
extern Abc_Obj_t *        Io_ReadCreatePo( Abc_Ntk_t * pNtk, char * pName );
extern Abc_Obj_t *        Io_ReadCreateAssert( Abc_Ntk_t * pNtk, char * pName );
extern Abc_Obj_t *        Io_ReadCreateLatch( Abc_Ntk_t * pNtk, char * pNetLI, char * pNetLO );
extern Abc_Obj_t *        Io_ReadCreateResetLatch( Abc_Ntk_t * pNtk, int fBlifMv );
extern Abc_Obj_t *        Io_ReadCreateResetMux( Abc_Ntk_t * pNtk, char * pResetLO, char * pDataLI, int fBlifMv );
extern Abc_Obj_t *        Io_ReadCreateNode( Abc_Ntk_t * pNtk, char * pNameOut, char * pNamesIn[], int nInputs );
extern Abc_Obj_t *        Io_ReadCreateConst( Abc_Ntk_t * pNtk, char * pName, bool fConst1 );
extern Abc_Obj_t *        Io_ReadCreateInv( Abc_Ntk_t * pNtk, char * pNameIn, char * pNameOut );
extern Abc_Obj_t *        Io_ReadCreateBuf( Abc_Ntk_t * pNtk, char * pNameIn, char * pNameOut );
extern FILE *             Io_FileOpen( const char * FileName, const char * PathVar, const char * Mode, int fVerbose );


#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

