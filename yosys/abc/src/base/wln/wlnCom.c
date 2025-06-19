/**CFile****************************************************************

  FileName    [wlnCom.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Word-level network.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 23, 2018.]

  Revision    [$Id: wlnCom.c,v 1.00 2018/09/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wln.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int  Abc_CommandYosys      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandGraft      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandHierarchy  ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandCollapse   ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandSolve      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandPrint      ( Abc_Frame_t * pAbc, int argc, char ** argv );

static inline Rtl_Lib_t * Wln_AbcGetRtl( Abc_Frame_t * pAbc )                       { return (Rtl_Lib_t *)pAbc->pAbcRtl;                      }
static inline void        Wln_AbcFreeRtl( Abc_Frame_t * pAbc )                      { if ( pAbc->pAbcRtl ) Rtl_LibFree(Wln_AbcGetRtl(pAbc));  }
static inline void        Wln_AbcUpdateRtl( Abc_Frame_t * pAbc, Rtl_Lib_t * pLib )  { Wln_AbcFreeRtl(pAbc); pAbc->pAbcRtl = pLib;             }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wln_Init( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "Word level", "%yosys",       Abc_CommandYosys,      0 );
    Cmd_CommandAdd( pAbc, "Word level", "%graft",       Abc_CommandGraft,      0 );
    Cmd_CommandAdd( pAbc, "Word level", "%hierarchy",   Abc_CommandHierarchy,   0 );
    Cmd_CommandAdd( pAbc, "Word level", "%collapse",    Abc_CommandCollapse,   0 );
    //Cmd_CommandAdd( pAbc, "Word level", "%solve",       Abc_CommandSolve,      0 );
    Cmd_CommandAdd( pAbc, "Word level", "%print",       Abc_CommandPrint,      0 );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Wln_End( Abc_Frame_t * pAbc )
{
    Wln_AbcUpdateRtl( pAbc, NULL );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandYosys( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Gia_Man_t * Wln_BlastSystemVerilog( char * pFileName, char * pTopModule, char * pDefines, int fSkipStrash, int fInvert, int fTechMap, int fLibInDir, int fVerbose );
    extern Rtl_Lib_t * Wln_ReadSystemVerilog( char * pFileName, char * pTopModule, char * pDefines, int fCollapse, int fVerbose );

    FILE * pFile;
    char * pFileName = NULL;
    char * pTopModule= NULL;
    char * pDefines  = NULL;
    int fBlast       =    0;
    int fInvert      =    0;
    int fTechMap     =    1;
    int fLibInDir    =    0;
    int fSkipStrash  =    0;
    int fCollapse    =    0;
    int c, fVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "TDbismlcvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'T':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-T\" should be followed by a file name.\n" );
                goto usage;
            }
            pTopModule = argv[globalUtilOptind];
            globalUtilOptind++;
            break;
        case 'D':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-D\" should be followed by a file name.\n" );
                goto usage;
            }
            pDefines = argv[globalUtilOptind];
            globalUtilOptind++;
            break;
        case 'b':
            fBlast ^= 1;
            break;
        case 'i':
            fInvert ^= 1;
            break;
        case 's':
            fSkipStrash ^= 1;
            break;
        case 'm':
            fTechMap ^= 1;
            break;
        case 'l':
            fLibInDir ^= 1;
            break;            
        case 'c':
            fCollapse ^= 1;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
    {
        printf( "Abc_CommandReadWlc(): Input file name should be given on the command line.\n" );
        return 0;
    }
    // get the file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "r" )) == NULL )
    {
        Abc_Print( 1, "Cannot open input file \"%s\". ", pFileName );
        if ( (pFileName = Extra_FileGetSimilarName( pFileName, ".v", ".sv", NULL, NULL, NULL )) )
            Abc_Print( 1, "Did you mean \"%s\"?", pFileName );
        Abc_Print( 1, "\n" );
        return 0;
    }
    fclose( pFile );

    // perform reading
    if ( fBlast )
    {
        Gia_Man_t * pNew = NULL;
        if ( !strcmp( Extra_FileNameExtension(pFileName), "v" )  )
            pNew = Wln_BlastSystemVerilog( pFileName, pTopModule, pDefines, fSkipStrash, fInvert, fTechMap, fLibInDir, fVerbose );
        else if ( !strcmp( Extra_FileNameExtension(pFileName), "sv" )  )
            pNew = Wln_BlastSystemVerilog( pFileName, pTopModule, pDefines, fSkipStrash, fInvert, fTechMap, fLibInDir, fVerbose );
        else if ( !strcmp( Extra_FileNameExtension(pFileName), "rtlil" )  )
            pNew = Wln_BlastSystemVerilog( pFileName, pTopModule, pDefines, fSkipStrash, fInvert, fTechMap, fLibInDir, fVerbose );
        else
        {
            printf( "Abc_CommandYosys(): Unknown file extension.\n" );
            return 0;
        }
        Abc_FrameUpdateGia( pAbc, pNew );
    }
    else
    {
        Rtl_Lib_t * pLib = NULL;
        if ( !strcmp( Extra_FileNameExtension(pFileName), "v" )  )
            pLib = Wln_ReadSystemVerilog( pFileName, pTopModule, pDefines, fCollapse, fVerbose );
        else if ( !strcmp( Extra_FileNameExtension(pFileName), "sv" )  )
            pLib = Wln_ReadSystemVerilog( pFileName, pTopModule, pDefines, fCollapse, fVerbose );
        else if ( !strcmp( Extra_FileNameExtension(pFileName), "rtlil" )  )
            pLib = Wln_ReadSystemVerilog( pFileName, pTopModule, pDefines, fCollapse, fVerbose );
        else
        {
            printf( "Abc_CommandYosys(): Unknown file extension.\n" );
            return 0;
        }
        Wln_AbcUpdateRtl( pAbc, pLib );
    }
    return 0;
usage:
    Abc_Print( -2, "usage: %%yosys [-T <module>] [-D <defines>] [-bismlcvh] <file_name>\n" );
    Abc_Print( -2, "\t         reads Verilog or SystemVerilog using Yosys\n" );
    Abc_Print( -2, "\t-T     : specify the top module name (default uses \"-auto-top\")\n" );
    Abc_Print( -2, "\t-D     : specify defines to be used by Yosys (default \"not used\")\n" );
    Abc_Print( -2, "\t-b     : toggle bit-blasting the design into an AIG using Yosys [default = %s]\n", fBlast? "yes": "no" );
    Abc_Print( -2, "\t-i     : toggle inverting the outputs (useful for miters) [default = %s]\n", fInvert? "yes": "no" );
    Abc_Print( -2, "\t-s     : toggle no structural hashing during bit-blasting [default = %s]\n", fSkipStrash? "no strash": "strash" );
    Abc_Print( -2, "\t-m     : toggle using \"techmap\" to blast operators [default = %s]\n", fTechMap? "yes": "no" );
    Abc_Print( -2, "\t-l     : toggle looking for \"techmap.v\" in the current directory [default = %s]\n", fLibInDir? "yes": "no" );
    Abc_Print( -2, "\t-c     : toggle collapsing design hierarchy using Yosys [default = %s]\n", fCollapse? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandGraft( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern void Wln_LibGraftOne( Rtl_Lib_t * p, char ** pModules, int nModules, int fInv, int fVerbose );
    Rtl_Lib_t * pLib = Wln_AbcGetRtl(pAbc);
    char ** pArgvNew; int nArgcNew;
    int c, fInv = 0, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ivh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'i':
            fInv ^= 1;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( pLib == NULL )
    {
        printf( "The design is not entered.\n" );
        return 1;
    }
    pArgvNew = argv + globalUtilOptind;
    nArgcNew = argc - globalUtilOptind;
    if ( nArgcNew != 0 && nArgcNew != 2 )
    {
        Abc_Print( -1, "Abc_CommandGraft(): This command expects one AIG file name on the command line.\n" );
        return 1;
    }
    Wln_LibGraftOne( pLib, pArgvNew, nArgcNew, fInv, fVerbose );
    return 0;
usage:
    Abc_Print( -2, "usage: %%graft [-ivh] <module1_name> <module2_name>\n" );
    Abc_Print( -2, "\t         replace instances of module1 by those of module2\n" );
    Abc_Print( -2, "\t-i     : toggle using inverse grafting [default = %s]\n", fInv? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandHierarchy( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern void Wln_LibMarkHierarchy( Rtl_Lib_t * p, char ** ppModule, int nModules, int fVerbose );
    Rtl_Lib_t * pLib = Wln_AbcGetRtl(pAbc);
    char ** pArgvNew; int nArgcNew;
    int c,  fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( pLib == NULL )
    {
        printf( "The design is not entered.\n" );
        return 1;
    }
    pArgvNew = argv + globalUtilOptind;
    nArgcNew = argc - globalUtilOptind;
    if ( nArgcNew < 0 )
    {
        Abc_Print( -1, "Abc_CommandHierarchy(): This command expects one AIG file name on the command line.\n" );
        return 1;
    }
    Wln_LibMarkHierarchy( pLib, pArgvNew, nArgcNew, fVerbose );
    return 0;
usage:
    Abc_Print( -2, "usage: %%hierarchy [-vh] <module_name>\n" );
    Abc_Print( -2, "\t         marks the module whose instances may later be treated as black boxes\n" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandCollapse( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Gia_Man_t * Rtl_LibCollapse( Rtl_Lib_t * p, char * pTopModule, int fRev, int fVerbose );
    Gia_Man_t * pNew = NULL;
    Rtl_Lib_t * pLib = Wln_AbcGetRtl(pAbc);
    char * pTopModule = NULL;
    int c, fInv = 0, fRev = 0, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "Tcrvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'T':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-T\" should be followed by a file name.\n" );
                goto usage;
            }
            pTopModule = argv[globalUtilOptind];
            globalUtilOptind++;
            break;
        case 'c':
            fInv ^= 1;
            break;
        case 'r':
            fRev ^= 1;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( pLib == NULL )
    {
        printf( "The design is not entered.\n" );
        return 1;
    }
    pNew = Rtl_LibCollapse( pLib, pTopModule, fRev, fVerbose );
    if ( fInv )
        Gia_ManInvertPos( pNew );
    Abc_FrameUpdateGia( pAbc, pNew );
    return 0;
usage:
    Abc_Print( -2, "usage: %%collapse [-T <module>] [-crvh] <file_name>\n" );
    Abc_Print( -2, "\t         collapse hierarchical design into an AIG\n" );
    Abc_Print( -2, "\t-T     : specify the top module of the design [default = none]\n" );
    Abc_Print( -2, "\t-c     : toggle complementing miter outputs after collapsing [default = %s]\n", fInv? "yes": "no" );
    Abc_Print( -2, "\t-r     : toggle bit order reversal in the word-level IO [default = %s]\n", fRev? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandPrint( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern void Rtl_LibPrintStats( Rtl_Lib_t * p );
    extern void Rtl_LibPrintHieStats( Rtl_Lib_t * p );
    extern void Rtl_LibPrint( char * pFileName, Rtl_Lib_t * p );
    Rtl_Lib_t * pLib = Wln_AbcGetRtl(pAbc);
    int c, fHie = 0, fDesign = 0, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "pdvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'p':
            fHie ^= 1;
            break;
        case 'd':
            fDesign ^= 1;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( pLib == NULL )
    {
        printf( "The design is not entered.\n" );
        return 1;
    }
    Rtl_LibPrintStats( pLib );
    if ( fHie )
        Rtl_LibPrintHieStats( pLib );
    if ( fDesign )
        Rtl_LibPrint( NULL, pLib );
    return 0;
usage:
    Abc_Print( -2, "usage: %%print [-pdvh]\n" );
    Abc_Print( -2, "\t         print statistics about the hierarchical design\n" );
    Abc_Print( -2, "\t-p     : toggle printing of the hierarchy [default = %s]\n", fHie? "yes": "no" );
    Abc_Print( -2, "\t-d     : toggle printing of the design [default = %s]\n", fDesign? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    Abc_Print( -2, "\t<file> : text file name with guidance for solving\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandSolve( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern void Rtl_LibBlast( Rtl_Lib_t * pLib );
    extern void Rtl_LibBlast2( Rtl_Lib_t * pLib, Vec_Int_t * vRoots, int fInv );
    extern void Rtl_LibSolve( Rtl_Lib_t * pLib, void * pNtk );
    extern void Rtl_LibPreprocess( Rtl_Lib_t * pLib );
    extern void Wln_SolveWithGuidance( char * pFileName, Rtl_Lib_t * p );

    Rtl_Lib_t * pLib = Wln_AbcGetRtl(pAbc);
    int c, fOldBlast = 0, fPrepro = 0, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "opvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'o':
            fOldBlast ^= 1;
            break;
        case 'p':
            fPrepro ^= 1;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( pLib == NULL )
    {
        printf( "The design is not entered.\n" );
        return 1;
    }
    if ( argc == globalUtilOptind + 1 )
    {
        char * pFileName = argv[globalUtilOptind];
        FILE * pFile = fopen( pFileName, "r" );
        if ( pFile == NULL )
        {
            Abc_Print( -1, "Cannot open file \"%s\" with the input test patterns.\n", pFileName );
            return 0;
        }
        fclose( pFile );
        printf( "Using guidance from file \"%s\".\n", pFileName );
        Wln_SolveWithGuidance( pFileName, pLib );
    }
    else
    {
        printf( "Solving the miter without guidance.\n" );
        if ( fOldBlast )
            Rtl_LibBlast( pLib );
        else
            Rtl_LibBlast2( pLib, NULL, 0 );
        if ( fPrepro )
            Rtl_LibPreprocess( pLib );
        Rtl_LibSolve( pLib, NULL );
    }
    return 0;
usage:
    Abc_Print( -2, "usage: %%solve [-opvh] <file>\n" );
    Abc_Print( -2, "\t         solving properties for the hierarchical design\n" );
    Abc_Print( -2, "\t-o     : toggle using old bit-blasting procedure [default = %s]\n", fOldBlast? "yes": "no" );
    Abc_Print( -2, "\t-p     : toggle preprocessing for verification [default = %s]\n", fPrepro? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    Abc_Print( -2, "\t<file> : text file name with guidance for solving\n");
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

