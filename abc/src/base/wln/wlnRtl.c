/**CFile****************************************************************

  FileName    [wlnRtl.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Word-level network.]

  Synopsis    [Constructing WLN network from Rtl data structure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 23, 2018.]

  Revision    [$Id: wlnRtl.c,v 1.00 2018/09/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wln.h"
#include "base/main/main.h"

#ifdef WIN32
#include <process.h> 
#define unlink _unlink
#else
#include <unistd.h>
#endif

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
#define MAX_LINE 1000000

void Rtl_NtkCleanFile( char * pFileName )
{
    char * pBuffer, * pFileName2 = "_temp__.rtlil"; 
    FILE * pFile = fopen( pFileName, "rb" );
    FILE * pFile2;
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return;
    }
    pFile2 = fopen( pFileName2, "wb" );
    if ( pFile2 == NULL )
    {
        fclose( pFile );
        printf( "Cannot open file \"%s\" for writing.\n", pFileName2 );
        return;
    }
    pBuffer = ABC_ALLOC( char, MAX_LINE );
    while ( fgets( pBuffer, MAX_LINE, pFile ) != NULL )
        if ( !strstr(pBuffer, "attribute \\src") )
            fputs( pBuffer, pFile2 );
    ABC_FREE( pBuffer );
    fclose( pFile );
    fclose( pFile2 );
}

void Rtl_NtkCleanFile2( char * pFileName )
{
    char * pBuffer, * pFileName2 = "_temp__.v"; 
    FILE * pFile = fopen( pFileName, "rb" );
    FILE * pFile2;
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return;
    }
    pFile2 = fopen( pFileName2, "wb" );
    if ( pFile2 == NULL )
    {
        fclose( pFile );
        printf( "Cannot open file \"%s\" for writing.\n", pFileName2 );
        return;
    }
    pBuffer = ABC_ALLOC( char, MAX_LINE );
    while ( fgets( pBuffer, MAX_LINE, pFile ) != NULL )
        if ( !strstr(pBuffer, "//") )
            fputs( pBuffer, pFile2 );
    ABC_FREE( pBuffer );
    fclose( pFile );
    fclose( pFile2 );
}

char * Wln_GetYosysName()
{
    char * pYosysName = NULL;
    char * pYosysNameWin = "yosys.exe";
    char * pYosysNameUnix = "yosys";
    if ( Abc_FrameReadFlag("yosyswin") )
        pYosysNameWin = Abc_FrameReadFlag("yosyswin");
    if ( Abc_FrameReadFlag("yosysunix") )
        pYosysNameUnix = Abc_FrameReadFlag("yosysunix");
#ifdef WIN32
    pYosysName = pYosysNameWin;
#else
    pYosysName = pYosysNameUnix;
#endif
    return pYosysName;
}
int Wln_ConvertToRtl( char * pCommand, char * pFileTemp )
{
    FILE * pFile;
    if ( system( pCommand ) == -1 )
    {
        fprintf( stdout, "Cannot execute \"%s\".\n", pCommand );
        return 0;
    }
    if ( (pFile = fopen(pFileTemp, "r")) == NULL )
    {
        fprintf( stdout, "Cannot open intermediate file \"%s\".\n", pFileTemp );
        return 0;
    }
    fclose( pFile );
    return 1;
}
Rtl_Lib_t * Wln_ReadSystemVerilog( char * pFileName, char * pTopModule, char * pDefines, int fCollapse, int fVerbose )
{
    Rtl_Lib_t * pNtk = NULL;
    char Command[1000];
    char * pFileTemp = "_temp_.rtlil";
    int fSVlog = strstr(pFileName, ".sv") != NULL;
    if ( strstr(pFileName, ".rtl") )
        return Rtl_LibReadFile( pFileName, pFileName );
    sprintf( Command, "%s -qp \"read_verilog %s%s %s%s; hierarchy %s%s; %sproc; write_rtlil %s\"",
        Wln_GetYosysName(), 
        pDefines   ? "-D"       : "",
        pDefines   ? pDefines   : "",
        fSVlog     ? "-sv "     : "",
        pFileName,
        pTopModule ? "-top "    : "", 
        pTopModule ? pTopModule : "", 
        fCollapse  ? "flatten; ": "",
        pFileTemp );
    if ( fVerbose )
    printf( "%s\n", Command );
    if ( !Wln_ConvertToRtl(Command, pFileTemp) )
        return NULL;
    pNtk = Rtl_LibReadFile( pFileTemp, pFileName );
    if ( pNtk == NULL )
    {
        printf( "Dumped the design into file \"%s\".\n", pFileTemp );
        return NULL;
    }
    Rtl_NtkCleanFile( pFileTemp );
    unlink( pFileTemp );
    return pNtk;
}
Gia_Man_t * Wln_BlastSystemVerilog( char * pFileName, char * pTopModule, char * pDefines, int fSkipStrash, int fInvert, int fTechMap, int fVerbose )
{
    Gia_Man_t * pGia = NULL;
    char Command[1000];
    char * pFileTemp = "_temp_.aig";
    int fRtlil = strstr(pFileName, ".rtl") != NULL;
    int fSVlog = strstr(pFileName, ".sv")  != NULL;
    sprintf( Command, "%s -qp \"%s %s%s %s%s; hierarchy %s%s; flatten; proc; %saigmap; write_aiger %s\"",
        Wln_GetYosysName(), 
        fRtlil ? "read_rtlil"   : "read_verilog",
        pDefines  ? "-D"        : "",
        pDefines  ? pDefines    : "",
        fSVlog    ? "-sv "      : "",
        pFileName,
        pTopModule ? "-top "    : "-auto-top",
        pTopModule ? pTopModule : "", 
        fTechMap ? "techmap; setundef -zero; " : "", pFileTemp );
    if ( fVerbose )
    printf( "%s\n", Command );
    if ( !Wln_ConvertToRtl(Command, pFileTemp) )
        return NULL;
    pGia = Gia_AigerRead( pFileTemp, 0, fSkipStrash, 0 );
    if ( pGia == NULL )
    {
        printf( "Converting to AIG has failed.\n" );
        return NULL;
    }
    ABC_FREE( pGia->pName );
    pGia->pName = pTopModule ? Abc_UtilStrsav(pTopModule) :
        Extra_FileNameGeneric( Extra_FileNameWithoutPath(pFileName) );
    unlink( pFileTemp );
    // complement the outputs
    if ( fInvert )
    {
        Gia_Obj_t * pObj; int i;
        Gia_ManForEachPo( pGia, pObj, i )
            Gia_ObjFlipFaninC0( pObj );
    }
    return pGia;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

