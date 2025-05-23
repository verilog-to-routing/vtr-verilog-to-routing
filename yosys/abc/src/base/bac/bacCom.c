/**CFile****************************************************************

  FileName    [bacCom.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Command handlers.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacCom.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"
#include "bacPrs.h"
#include "proof/cec/cec.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int  Bac_CommandRead     ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Bac_CommandWrite    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Bac_CommandPs       ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Bac_CommandPut      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Bac_CommandGet      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Bac_CommandClp      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Bac_CommandCec      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Bac_CommandTest     ( Abc_Frame_t * pAbc, int argc, char ** argv );

static inline Bac_Man_t * Bac_AbcGetMan( Abc_Frame_t * pAbc )                       { return (Bac_Man_t *)pAbc->pAbcBac;                        }
static inline void        Bac_AbcFreeMan( Abc_Frame_t * pAbc )                      { if ( pAbc->pAbcBac ) Bac_ManFree(Bac_AbcGetMan(pAbc));    }
static inline void        Bac_AbcUpdateMan( Abc_Frame_t * pAbc, Bac_Man_t * p )     { Bac_AbcFreeMan(pAbc); pAbc->pAbcBac = p;                  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    [Accessing current Bac_Ntk_t.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Abc_FrameImportPtr( Vec_Ptr_t * vPtr )
{
    Bac_Man_t * p;
    if ( Abc_FrameGetGlobalFrame() == NULL )
    {
        printf( "ABC framework is not started.\n" );
        return;
    }
    p = Bac_PtrTransformToCba( vPtr );
    if ( p == NULL )
        printf( "Converting from Ptr failed.\n" );
    Bac_AbcUpdateMan( Abc_FrameGetGlobalFrame(), p );
}
Vec_Ptr_t * Abc_FrameExportPtr()
{
    Vec_Ptr_t * vPtr;
    Bac_Man_t * p;
    if ( Abc_FrameGetGlobalFrame() == NULL )
    {
        printf( "ABC framework is not started.\n" );
        return NULL;
    }
    p = Bac_AbcGetMan( Abc_FrameGetGlobalFrame() );
    if ( p == NULL )
        printf( "There is no CBA design present.\n" );
    vPtr = Bac_PtrDeriveFromCba( p );
    if ( vPtr == NULL )
        printf( "Converting to Ptr has failed.\n" );
    return vPtr;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Bac_Init( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "New word level", "@_read",       Bac_CommandRead,      0 );
    Cmd_CommandAdd( pAbc, "New word level", "@_write",      Bac_CommandWrite,     0 );
    Cmd_CommandAdd( pAbc, "New word level", "@_ps",         Bac_CommandPs,        0 );
    Cmd_CommandAdd( pAbc, "New word level", "@_put",        Bac_CommandPut,       0 );
    Cmd_CommandAdd( pAbc, "New word level", "@_get",        Bac_CommandGet,       0 );
    Cmd_CommandAdd( pAbc, "New word level", "@_clp",        Bac_CommandClp,       0 );
    Cmd_CommandAdd( pAbc, "New word level", "@_cec",        Bac_CommandCec,       0 );
    Cmd_CommandAdd( pAbc, "New word level", "@_test",       Bac_CommandTest,      0 );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Bac_End( Abc_Frame_t * pAbc )
{
    Bac_AbcFreeMan( pAbc );
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Bac_CommandRead( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    FILE * pFile;
    Bac_Man_t * p = NULL;
    Vec_Ptr_t * vDes = NULL;
    char * pFileName = NULL;
    int c, fUseAbc = 0, fUsePtr = 0, fVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "apvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'a':
            fUseAbc ^= 1;
            break;
        case 'p':
            fUsePtr ^= 1;
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
        printf( "Bac_CommandRead(): Input file name should be given on the command line.\n" );
        return 0;
    }
    // get the file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "r" )) == NULL )
    {
        Abc_Print( 1, "Cannot open input file \"%s\". ", pFileName );
        if ( (pFileName = Extra_FileGetSimilarName( pFileName, ".v", ".blif", ".smt", ".bac", NULL )) )
            Abc_Print( 1, "Did you mean \"%s\"?", pFileName );
        Abc_Print( 1, "\n" );
        return 0;
    }
    fclose( pFile );
    // perform reading
    if ( fUseAbc || fUsePtr )
    {
        extern Vec_Ptr_t * Ptr_AbcDeriveDes( Abc_Ntk_t * pNtk );
        Abc_Ntk_t * pAbcNtk = Io_ReadNetlist( pFileName, Io_ReadFileType(pFileName), 0 );
        Vec_Ptr_t * vDes = Ptr_AbcDeriveDes( pAbcNtk );
        p = Bac_PtrTransformToCba( vDes );
        Bac_PtrFree( vDes ); // points to names in pAbcNtk
        if ( p )
        {
            ABC_FREE( p->pSpec );
            p->pSpec = Abc_UtilStrsav( pAbcNtk->pSpec );
        }
        Abc_NtkDelete( pAbcNtk );
    }
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "blif" )  )
    {
        vDes = Psr_ManReadBlif( pFileName );
        if ( vDes && Vec_PtrSize(vDes) )
            p = Psr_ManBuildCba( pFileName, vDes );
        if ( vDes )
            Psr_ManVecFree( vDes );
    }
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "v" )  )
    {
        vDes = Psr_ManReadVerilog( pFileName );
        if ( vDes && Vec_PtrSize(vDes) )
            p = Psr_ManBuildCba( pFileName, vDes );
        if ( vDes )
            Psr_ManVecFree( vDes );
    }
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "smt" )  )
    {
        vDes = NULL;//Psr_ManReadSmt( pFileName );
        if ( vDes && Vec_PtrSize(vDes) )
            p = Psr_ManBuildCba( pFileName, vDes );
        if ( vDes )
            Psr_ManVecFree( vDes );
    }
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "bac" )  )
    {
        p = Bac_ManReadBac( pFileName );
    }
    else 
    {
        printf( "Unrecognized input file extension.\n" );
        return 0;
    }
    Bac_AbcUpdateMan( pAbc, p );
    return 0;
usage:
    Abc_Print( -2, "usage: @_read [-apvh] <file_name>\n" );
    Abc_Print( -2, "\t         reads hierarchical design in BLIF or Verilog\n" );
    Abc_Print( -2, "\t-a     : toggle using old ABC parser [default = %s]\n", fUseAbc? "yes": "no" );
    Abc_Print( -2, "\t-p     : toggle using Ptr construction [default = %s]\n", fUsePtr? "yes": "no" );
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
int Bac_CommandWrite( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Bac_Man_t * p = Bac_AbcGetMan(pAbc);
    char * pFileName = NULL;
    int fUseAssign   =    1;
    int fUsePtr      =    0; 
    int c, fVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "apvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'a':
            fUseAssign ^= 1;
            break;
        case 'p':
            fUsePtr ^= 1;
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
    if ( p == NULL )
    {
        Abc_Print( 1, "Bac_CommandWrite(): There is no current design.\n" );
        return 0;
    }
    if ( argc == globalUtilOptind + 1 )
        pFileName = argv[globalUtilOptind];
    else if ( argc == globalUtilOptind && p )
        pFileName = Extra_FileNameGenericAppend( Bac_ManName(p), "_out.v" );
    else 
    {
        printf( "Output file name should be given on the command line.\n" );
        return 0;
    }
    // perform writing
    if ( !strcmp( Extra_FileNameExtension(pFileName), "blif" )  )
        Bac_ManWriteBlif( pFileName, p );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "v" )  )
    {
        if ( fUsePtr )
        {
            Vec_Ptr_t * vPtr = Bac_PtrDeriveFromCba( p );
            if ( vPtr == NULL )
                printf( "Converting to Ptr has failed.\n" );
            else
            {
                Bac_PtrDumpVerilog( pFileName, vPtr );
                Bac_PtrFree( vPtr ); 
            }
        }
        else
            Bac_ManWriteVerilog( pFileName, p, fUseAssign );        
    }
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "bac" )  )
        Bac_ManWriteBac( pFileName, p );
    else 
    {
        printf( "Unrecognized output file extension.\n" );
        return 0;
    }
    return 0;
usage:
    Abc_Print( -2, "usage: @_write [-apvh]\n" );
    Abc_Print( -2, "\t         writes the design into a file in BLIF or Verilog\n" );
    Abc_Print( -2, "\t-a     : toggle using assign-statement for primitives [default = %s]\n",  fUseAssign? "yes": "no" );
    Abc_Print( -2, "\t-p     : toggle using Ptr construction (mapped Verilog only) [default = %s]\n", fUsePtr? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n",  fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Bac_CommandPs( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Bac_Man_t * p = Bac_AbcGetMan(pAbc);
    int c, nModules = 0, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "Mvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'M':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-M\" should be followed by an integer.\n" );
                goto usage;
            }
            nModules = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nModules < 0 )
                goto usage;
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
    if ( p == NULL )
    {
        Abc_Print( 1, "Bac_CommandPs(): There is no current design.\n" );
        return 0;
    }
    Bac_ManPrintStats( p, nModules, fVerbose );
    return 0;
usage:
    Abc_Print( -2, "usage: @_ps [-M num] [-vh]\n" );
    Abc_Print( -2, "\t         prints statistics\n" );
    Abc_Print( -2, "\t-M num : the number of first modules to report [default = %d]\n", nModules );
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
int Bac_CommandPut( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Bac_Man_t * p = Bac_AbcGetMan(pAbc);
    Gia_Man_t * pGia = NULL;
    int c, fBarBufs = 1, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "bvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'b':
            fBarBufs ^= 1;
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
    if ( p == NULL )
    {
        Abc_Print( 1, "Bac_CommandPut(): There is no current design.\n" );
        return 0;
    }
    pGia = Bac_ManExtract( p, fBarBufs, fVerbose );
    if ( pGia == NULL )
    {
        Abc_Print( 1, "Bac_CommandPut(): Conversion to AIG has failed.\n" );
        return 0;
    }
    Abc_FrameUpdateGia( pAbc, pGia );
    return 0;
usage:
    Abc_Print( -2, "usage: @_put [-bvh]\n" );
    Abc_Print( -2, "\t         extracts AIG from the hierarchical design\n" );
    Abc_Print( -2, "\t-b     : toggle using barrier buffers [default = %s]\n", fBarBufs? "yes": "no" );
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
int Bac_CommandGet( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Bac_Man_t * pNew = NULL, * p = Bac_AbcGetMan(pAbc);
    int c, fMapped = 0, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'm':
            fMapped ^= 1;
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
    if ( p == NULL )
    {
        Abc_Print( 1, "Bac_CommandGet(): There is no current design.\n" );
        return 0;
    }
    if ( fMapped )
    {
        if ( pAbc->pNtkCur == NULL )
        {
            Abc_Print( 1, "Bac_CommandGet(): There is no current mapped design.\n" );
            return 0;
        }
        pNew = (Bac_Man_t *)Bac_ManInsertAbc( p, pAbc->pNtkCur );
    }
    else
    {
        if ( pAbc->pGia == NULL )
        {
            Abc_Print( 1, "Bac_CommandGet(): There is no current AIG.\n" );
            return 0;
        }
        pNew = Bac_ManInsertGia( p, pAbc->pGia );
    }
    Bac_AbcUpdateMan( pAbc, pNew );
    return 0;
usage:
    Abc_Print( -2, "usage: @_get [-mvh]\n" );
    Abc_Print( -2, "\t         inserts AIG or mapped network into the hierarchical design\n" );
    Abc_Print( -2, "\t-m     : toggle using mapped network from main-space [default = %s]\n", fMapped? "yes": "no" );
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
int Bac_CommandClp( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Bac_Man_t * pNew = NULL, * p = Bac_AbcGetMan(pAbc);
    int c, fVerbose  = 0;
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
    if ( p == NULL )
    {
        Abc_Print( 1, "Bac_CommandGet(): There is no current design.\n" );
        return 0;
    }
    pNew = Bac_ManCollapse( p );
    Bac_AbcUpdateMan( pAbc, pNew );
    return 0;
usage:
    Abc_Print( -2, "usage: @_clp [-vh]\n" );
    Abc_Print( -2, "\t         collapses the current hierarchical design\n" );
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
int Bac_CommandCec( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Bac_Man_t * p = Bac_AbcGetMan(pAbc);
    Gia_Man_t * pFirst, * pSecond, * pMiter;
    Cec_ParCec_t ParsCec, * pPars = &ParsCec;
    Vec_Ptr_t * vDes;
    char * FileName, * pStr, ** pArgvNew;
    int c, nArgcNew, fDumpMiter = 0;
    FILE * pFile;
    Cec_ManCecSetDefaultParams( pPars );
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'v':
            pPars->fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( p == NULL )
    {
        Abc_Print( 1, "Bac_CommandCec(): There is no current design.\n" );
        return 0;
    }
    pArgvNew = argv + globalUtilOptind;
    nArgcNew = argc - globalUtilOptind;
    if ( nArgcNew != 1 )
    {
        if ( p->pSpec == NULL )
        {
            Abc_Print( -1, "File name is not given on the command line.\n" );
            return 1;
        }
        FileName = p->pSpec;
    }
    else
        FileName = pArgvNew[0];
    // fix the wrong symbol
    for ( pStr = FileName; *pStr; pStr++ )
        if ( *pStr == '>' )
            *pStr = '\\';
    if ( (pFile = fopen( FileName, "r" )) == NULL )
    {
        Abc_Print( -1, "Cannot open input file \"%s\". ", FileName );
        if ( (FileName = Extra_FileGetSimilarName( FileName, ".v", ".blif", NULL, NULL, NULL )) )
            Abc_Print( 1, "Did you mean \"%s\"?", FileName );
        Abc_Print( 1, "\n" );
        return 1;
    }
    fclose( pFile );

    // extract AIG from the current design
    pFirst = Bac_ManExtract( p, 0, 0 );
    if ( pFirst == NULL )
    {
        Abc_Print( -1, "Extracting AIG from the current design has failed.\n" );
        return 0;
    }
    // extract AIG from the second design
    if ( !strcmp( Extra_FileNameExtension(FileName), "blif" )  )
        vDes = Psr_ManReadBlif( FileName );
    else if ( !strcmp( Extra_FileNameExtension(FileName), "v" )  )
        vDes = Psr_ManReadVerilog( FileName );
    else assert( 0 );
    p = Psr_ManBuildCba( FileName, vDes );
    Psr_ManVecFree( vDes );
    pSecond = Bac_ManExtract( p, 0, 0 );
    Bac_ManFree( p );
    if ( pSecond == NULL )
    {
        Gia_ManStop( pFirst );
        Abc_Print( -1, "Extracting AIG from the original design has failed.\n" );
        return 0;
    }
    // compute the miter
    pMiter = Gia_ManMiter( pFirst, pSecond, 0, 1, 0, 0, pPars->fVerbose );
    if ( pMiter )
    {
        if ( fDumpMiter )
        {
            Abc_Print( 0, "The verification miter is written into file \"%s\".\n", "cec_miter.aig" );
            Gia_AigerWrite( pMiter, "cec_miter.aig", 0, 0, 0 );
        }
        pAbc->Status = Cec_ManVerify( pMiter, pPars );
        //Abc_FrameReplaceCex( pAbc, &pAbc->pGia->pCexComb );
        Gia_ManStop( pMiter );
    }
    Gia_ManStop( pFirst );
    Gia_ManStop( pSecond );
    return 0;
usage:
    Abc_Print( -2, "usage: @_cec [-vh]\n" );
    Abc_Print( -2, "\t         combinational equivalence checking\n" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", pPars->fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Bac_CommandTest( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern void Psr_ManReadBlifTest();
    extern void Psr_ManReadVerilogTest();
    extern void Psr_SmtReadSmtTest();
    //Bac_Man_t * p = Bac_AbcGetMan(pAbc);
    int c, fVerbose  = 0;
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
/*
    if ( p == NULL )
    {
        Abc_Print( 1, "Bac_CommandTest(): There is no current design.\n" );
        return 0;
    }
*/
    //Bac_PtrTransformTestTest();
    //Psr_ManReadVerilogTest();
    //Psr_SmtReadSmtTest();
    return 0;
usage:
    Abc_Print( -2, "usage: @_test [-vh]\n" );
    Abc_Print( -2, "\t         experiments with word-level networks\n" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

