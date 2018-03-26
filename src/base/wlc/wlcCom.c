/**CFile****************************************************************

  FileName    [wlcCom.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Command handlers.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcCom.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"
#include "base/main/mainInt.h"
#include "aig/miniaig/ndr.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int  Abc_CommandReadWlc    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandWriteWlc   ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandPs         ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandCone       ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandAbs        ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandPdrAbs     ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandAbs2       ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandBlast      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandGraft      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandProfile    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandShortNames ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandShow       ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandTest       ( Abc_Frame_t * pAbc, int argc, char ** argv );

static int  Abc_CommandInvPs      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandInvPrint   ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandInvCheck   ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandInvGet     ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandInvPut     ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandInvMin     ( Abc_Frame_t * pAbc, int argc, char ** argv );

static inline Wlc_Ntk_t * Wlc_AbcGetNtk( Abc_Frame_t * pAbc )                       { return (Wlc_Ntk_t *)pAbc->pAbcWlc;                      }
static inline void        Wlc_AbcFreeNtk( Abc_Frame_t * pAbc )                      { if ( pAbc->pAbcWlc ) Wlc_NtkFree(Wlc_AbcGetNtk(pAbc));  }
static inline void        Wlc_AbcUpdateNtk( Abc_Frame_t * pAbc, Wlc_Ntk_t * pNtk )  { Wlc_AbcFreeNtk(pAbc); pAbc->pAbcWlc = pNtk;             }

static inline Vec_Int_t * Wlc_AbcGetInv( Abc_Frame_t * pAbc )                       { return pAbc->pAbcWlcInv;                                }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Wlc_Init( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "Word level", "%read",        Abc_CommandReadWlc,    0 );
    Cmd_CommandAdd( pAbc, "Word level", "%write",       Abc_CommandWriteWlc,   0 );
    Cmd_CommandAdd( pAbc, "Word level", "%ps",          Abc_CommandPs,         0 );
    Cmd_CommandAdd( pAbc, "Word level", "%cone",        Abc_CommandCone,       0 );
    Cmd_CommandAdd( pAbc, "Word level", "%abs",         Abc_CommandAbs,        0 );
    Cmd_CommandAdd( pAbc, "Word level", "%pdra",        Abc_CommandPdrAbs,     0 );
    Cmd_CommandAdd( pAbc, "Word level", "%abs2",        Abc_CommandAbs2,       0 );
    Cmd_CommandAdd( pAbc, "Word level", "%blast",       Abc_CommandBlast,      0 );
    Cmd_CommandAdd( pAbc, "Word level", "%graft",       Abc_CommandGraft,      0 );
    Cmd_CommandAdd( pAbc, "Word level", "%profile",     Abc_CommandProfile,    0 );
    Cmd_CommandAdd( pAbc, "Word level", "%short_names", Abc_CommandShortNames, 0 );
    Cmd_CommandAdd( pAbc, "Word level", "%show",        Abc_CommandShow,       0 );
    Cmd_CommandAdd( pAbc, "Word level", "%test",        Abc_CommandTest,       0 );

    Cmd_CommandAdd( pAbc, "Word level", "inv_ps",       Abc_CommandInvPs,      0 );
    Cmd_CommandAdd( pAbc, "Word level", "inv_print",    Abc_CommandInvPrint,   0 );
    Cmd_CommandAdd( pAbc, "Word level", "inv_check",    Abc_CommandInvCheck,   0 );
    Cmd_CommandAdd( pAbc, "Word level", "inv_get",      Abc_CommandInvGet,     0 );
    Cmd_CommandAdd( pAbc, "Word level", "inv_put",      Abc_CommandInvPut,     0 );
    Cmd_CommandAdd( pAbc, "Word level", "inv_min",      Abc_CommandInvMin,     0 );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Wlc_End( Abc_Frame_t * pAbc )
{
    Wlc_AbcFreeNtk( pAbc );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Wlc_SetNtk( Abc_Frame_t * pAbc, Wlc_Ntk_t * pNtk )
{
    Wlc_AbcUpdateNtk( pAbc, pNtk );
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandReadWlc( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    FILE * pFile;
    Wlc_Ntk_t * pNtk = NULL;
    char * pFileName = NULL;
    int fOldParser   =    0;
    int fPrintTree   =    0;
    int c, fVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "opvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'o':
            fOldParser ^= 1;
            break;
        case 'p':
            fPrintTree ^= 1;
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
        if ( (pFileName = Extra_FileGetSimilarName( pFileName, ".v", ".smt", ".smt2", ".ndr", NULL )) )
            Abc_Print( 1, "Did you mean \"%s\"?", pFileName );
        Abc_Print( 1, "\n" );
        return 0;
    }
    fclose( pFile );

    // perform reading
    if ( !strcmp( Extra_FileNameExtension(pFileName), "v" )  )
        pNtk = Wlc_ReadVer( pFileName, NULL );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "smt" ) || !strcmp( Extra_FileNameExtension(pFileName), "smt2" )  )
        pNtk = Wlc_ReadSmt( pFileName, fOldParser, fPrintTree );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "ndr" )  )
        pNtk = Wlc_ReadNdr( pFileName );
    else
    {
        printf( "Abc_CommandReadWlc(): Unknown file extension.\n" );
        return 0;
    }
    Wlc_AbcUpdateNtk( pAbc, pNtk );
    return 0;
usage:
    Abc_Print( -2, "usage: %%read [-opvh] <file_name>\n" );
    Abc_Print( -2, "\t         reads word-level design from Verilog file\n" );
    Abc_Print( -2, "\t-o     : toggle using old SMT-LIB parser [default = %s]\n", fOldParser? "yes": "no" );
    Abc_Print( -2, "\t-p     : toggle printing parse SMT-LIB tree [default = %s]\n", fPrintTree? "yes": "no" );
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
int Abc_CommandWriteWlc( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
    char * pFileName = NULL;
    int fAddCos      =    0; 
    int fSplitNodes  =    0; 
    int fNoFlops     =    0;
    int c, fVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "anfvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'a':
            fAddCos ^= 1;
            break;
        case 'n':
            fSplitNodes ^= 1;
            break;
        case 'f':
            fNoFlops ^= 1;
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
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandWriteWlc(): There is no current design.\n" );
        return 0;
    }
    if ( argc == globalUtilOptind )
        pFileName = Extra_FileNameGenericAppend( pNtk->pName, "_out.v" );
    else if ( argc == globalUtilOptind + 1 )
        pFileName = argv[globalUtilOptind];
    else
    {
        printf( "Output file name should be given on the command line.\n" );
        return 0;
    }
    if ( !strcmp( Extra_FileNameExtension(pFileName), "ndr" )  )
        Wlc_WriteNdr( pNtk, pFileName );
    else if ( fSplitNodes )
    {
        pNtk = Wlc_NtkDupSingleNodes( pNtk );
        Wlc_WriteVer( pNtk, pFileName, fAddCos, fNoFlops );
        Wlc_NtkFree( pNtk );
    }
    else
        Wlc_WriteVer( pNtk, pFileName, fAddCos, fNoFlops );

    return 0;
usage:
    Abc_Print( -2, "usage: %%write [-anfvh]\n" );
    Abc_Print( -2, "\t         writes the design into a file\n" );
    Abc_Print( -2, "\t-a     : toggle adding a CO for each node [default = %s]\n",       fAddCos? "yes": "no" );
    Abc_Print( -2, "\t-n     : toggle splitting into individual nodes [default = %s]\n", fSplitNodes? "yes": "no" );
    Abc_Print( -2, "\t-f     : toggle skipping flops when writing file [default = %s]\n",fNoFlops? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n",    fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandPs( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
    int fShowCones   = 0;
    int fShowMulti   = 0;
    int fShowAdder   = 0;
    int fDistrib     = 0;
    int fTwoSides    = 0;
    int c, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "cmadtvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'c':
            fShowCones ^= 1;
            break;
        case 'm':
            fShowMulti ^= 1;
            break;
        case 'a':
            fShowAdder ^= 1;
            break;
        case 'd':
            fDistrib ^= 1;
            break;
        case 't':
            fTwoSides ^= 1;
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
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandPs(): There is no current design.\n" );
        return 0;
    }
    Wlc_NtkPrintStats( pNtk, fDistrib, fTwoSides, fVerbose );
    if ( fShowCones )
        Wlc_NtkProfileCones( pNtk ); 
    if ( fShowMulti )
        Wlc_NtkPrintNodes( pNtk, WLC_OBJ_ARI_MULTI );
    if ( fShowAdder )
        Wlc_NtkPrintNodes( pNtk, WLC_OBJ_ARI_ADD );
    return 0;
usage:
    Abc_Print( -2, "usage: %%ps [-cmadtvh]\n" );
    Abc_Print( -2, "\t         prints statistics\n" );
    Abc_Print( -2, "\t-c     : toggle printing cones [default = %s]\n",                 fShowCones? "yes": "no" );
    Abc_Print( -2, "\t-m     : toggle printing multipliers [default = %s]\n",           fShowMulti? "yes": "no" );
    Abc_Print( -2, "\t-a     : toggle printing adders [default = %s]\n",                fShowAdder? "yes": "no" );
    Abc_Print( -2, "\t-d     : toggle printing distrubition [default = %s]\n",          fDistrib? "yes": "no" );
    Abc_Print( -2, "\t-t     : toggle printing stats for LHS and RHS [default = %s]\n", fTwoSides? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n",   fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandCone( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
    int c, iOutput = -1, Range = 1, fAllPis = 0, fSeq = 0, fVerbose  = 0;
    char * pName;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ORisvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'O':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-O\" should be followed by an integer.\n" );
                goto usage;
            }
            iOutput = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( iOutput < 0 )
                goto usage;
            break;
        case 'R':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-R\" should be followed by an integer.\n" );
                goto usage;
            }
            Range = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( Range < 0 )
                goto usage;
            break;
        case 'i':
            fAllPis ^= 1;
            break;
        case 's':
            fSeq ^= 1;
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
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandCone(): There is no current design.\n" );
        return 0;
    }
    if ( iOutput < 0 || iOutput >= Wlc_NtkCoNum(pNtk) )
    {
        Abc_Print( 1, "Abc_CommandCone(): Illegal output index (%d) (should be 0 <= num < %d).\n", iOutput, Wlc_NtkCoNum(pNtk) );
        return 0;
    }
    printf( "Extracting output %d as a %s word-level network.\n", iOutput, fSeq ? "sequential" : "combinational" );
    pName = Wlc_NtkNewName( pNtk, iOutput, fSeq );
    Wlc_NtkMarkCone( pNtk, iOutput, Range, fSeq, fAllPis );
    pNtk = Wlc_NtkDupDfs( pNtk, 1, fSeq );
    ABC_FREE( pNtk->pName );
    pNtk->pName = Abc_UtilStrsav( pName );
    Wlc_AbcUpdateNtk( pAbc, pNtk );
    return 0;
usage:
    Abc_Print( -2, "usage: %%cone [-OR num] [-isvh]\n" );
    Abc_Print( -2, "\t         extracts logic cone of one or more word-level outputs\n" );
    Abc_Print( -2, "\t-O num : zero-based index of the first word-level output to extract [default = %d]\n", iOutput );
    Abc_Print( -2, "\t-R num : total number of word-level outputs to extract [default = %d]\n", Range );
    Abc_Print( -2, "\t-i     : toggle using support composed of all primary inputs [default = %s]\n", fAllPis? "yes": "no" );
    Abc_Print( -2, "\t-s     : toggle performing extracting sequential cones [default = %s]\n", fSeq? "yes": "no" );
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
int Abc_CommandPdrAbs( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
    Wlc_Par_t Pars, * pPars = &Pars;
    int c;
    Wlc_ManSetDefaultParams( pPars );
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "AMXFILabrcdilpqmstuxvwh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'A':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-A\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsAdd = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsAdd < 0 )
                goto usage;
            break;
        case 'M':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-M\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsMul = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsMul < 0 )
                goto usage;
            break;
        case 'X':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-X\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsMux = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsMux < 0 )
                goto usage;
            break;
        case 'F':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-F\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsFlop = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsFlop < 0 )
                goto usage;
            break;
        case 'I':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-I\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nIterMax = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nIterMax < 0 )
                goto usage;
            break;
        case 'L':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-L\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nLimit = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nLimit < 0 )
                goto usage;
            break;
        case 'a':
            pPars->fPdra ^= 1;
            break;
        case 'b':
            pPars->fProofRefine ^= 1;
            break;
        case 'r':
            pPars->fHybrid ^= 1;
            break;
        case 'x':
            pPars->fXorOutput ^= 1;
            break;
        case 'c':
            pPars->fCheckClauses ^= 1;
            break;
        case 'd':
            pPars->fAbs2 ^= 1;
            break;
        case 'i':
            pPars->fProofUsePPI ^= 1;
            break;
        case 'l':
            pPars->fLoadTrace ^= 1;
            break;
        case 'p':
            pPars->fPushClauses ^= 1;
            break;
        case 'q':
            pPars->fUseBmc3 ^= 1;
            break;
        case 'm':
            pPars->fMFFC ^= 1;
            break;
        case 's':
            pPars->fShrinkAbs ^= 1;
            break;
        case 't':
            pPars->fShrinkScratch ^= 1;
            break;
        case 'u':
            pPars->fCheckCombUnsat ^= 1;
            break;
        case 'v':
            pPars->fVerbose ^= 1;
            break;
        case 'w':
            pPars->fPdrVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandCone(): There is no current design.\n" );
        return 0;
    }
    Wlc_NtkPdrAbs( pNtk, pPars );
    return 0;
usage:
    Abc_Print( -2, "usage: %%pdra [-AMXFIL num] [-abrcdilpqmxstuvwh]\n" );
    Abc_Print( -2, "\t         abstraction for word-level networks\n" );
    Abc_Print( -2, "\t-A num : minimum bit-width of an adder/subtractor to abstract [default = %d]\n", pPars->nBitsAdd );
    Abc_Print( -2, "\t-M num : minimum bit-width of a multiplier to abstract [default = %d]\n",        pPars->nBitsMul );
    Abc_Print( -2, "\t-X num : minimum bit-width of a MUX operator to abstract [default = %d]\n",      pPars->nBitsMux );
    Abc_Print( -2, "\t-F num : minimum bit-width of a flip-flop to abstract [default = %d]\n",         pPars->nBitsFlop );
    Abc_Print( -2, "\t-I num : maximum number of CEGAR iterations [default = %d]\n",                   pPars->nIterMax );
    Abc_Print( -2, "\t-L num : maximum number of each type of signals [default = %d]\n",               pPars->nLimit );
    Abc_Print( -2, "\t-x     : toggle XORing outputs of word-level miter [default = %s]\n",            pPars->fXorOutput? "yes": "no" );
    Abc_Print( -2, "\t-a     : toggle running pdr with -nct [default = %s]\n",                         pPars->fPdra? "yes": "no" );
    Abc_Print( -2, "\t-b     : toggle using proof-based refinement [default = %s]\n",                  pPars->fProofRefine? "yes": "no" );
    Abc_Print( -2, "\t-r     : toggle using both cex-based and proof-based refinement [default = %s]\n",                  pPars->fHybrid? "yes": "no" );
    Abc_Print( -2, "\t-c     : toggle checking clauses in the reloaded trace [default = %s]\n",        pPars->fCheckClauses? "yes": "no" );
    Abc_Print( -2, "\t-d     : toggle using another way of creating abstractions [default = %s]\n",    pPars->fAbs2? "yes": "no" );
    Abc_Print( -2, "\t-i     : toggle using PPI values in proof-based refinement [default = %s]\n",    pPars->fProofUsePPI? "yes": "no" );
    Abc_Print( -2, "\t-l     : toggle loading previous PDR traces [default = %s]\n",                   pPars->fLoadTrace? "yes": "no" );
    Abc_Print( -2, "\t-s     : toggle shrinking abstractions with BMC [default = %s]\n",               pPars->fShrinkAbs? "yes": "no" );
    Abc_Print( -2, "\t-t     : toggle restarting pdr from scratch after shrinking abstractions with BMC [default = %s]\n",               pPars->fShrinkScratch? "yes": "no" );
    Abc_Print( -2, "\t-u     : toggle checking combinationally unsat [default = %s]\n",                pPars->fCheckCombUnsat? "yes": "no" );
    Abc_Print( -2, "\t-p     : toggle pushing clauses in the reloaded trace [default = %s]\n",         pPars->fPushClauses? "yes": "no" );
    Abc_Print( -2, "\t-q     : toggle running bmc3 in parallel for CEX [default = %s]\n",              pPars->fUseBmc3? "yes": "no" );
    Abc_Print( -2, "\t-m     : toggle refining the whole MFFC of a PPI [default = %s]\n",              pPars->fMFFC? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n",                  pPars->fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-w     : toggle printing verbose PDR output [default = %s]\n",                   pPars->fPdrVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandAbs( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
    Wlc_Par_t Pars, * pPars = &Pars;
    int c;
    Wlc_ManSetDefaultParams( pPars );
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "AMXFILdxvwh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'A':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-A\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsAdd = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsAdd < 0 )
                goto usage;
            break;
        case 'M':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-M\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsMul = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsMul < 0 )
                goto usage;
            break;
        case 'X':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-X\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsMux = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsMux < 0 )
                goto usage;
            break;
        case 'F':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-F\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsFlop = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsFlop < 0 )
                goto usage;
            break;
        case 'I':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-I\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nIterMax = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nIterMax < 0 )
                goto usage;
            break;
        case 'L':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-L\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nLimit = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nLimit < 0 )
                goto usage;
            break;
        case 'd':
            pPars->fAbs2 ^= 1;
            break;
        case 'x':
            pPars->fXorOutput ^= 1;
            break;
        case 'v':
            pPars->fVerbose ^= 1;
            break;
        case 'w':
            pPars->fPdrVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandCone(): There is no current design.\n" );
        return 0;
    }
    Wlc_NtkAbsCore( pNtk, pPars );
    return 0;
usage:
    Abc_Print( -2, "usage: %%abs [-AMXFIL num] [-dxvwh]\n" );
    Abc_Print( -2, "\t         abstraction for word-level networks\n" );
    Abc_Print( -2, "\t-A num : minimum bit-width of an adder/subtractor to abstract [default = %d]\n", pPars->nBitsAdd );
    Abc_Print( -2, "\t-M num : minimum bit-width of a multiplier to abstract [default = %d]\n",        pPars->nBitsMul );
    Abc_Print( -2, "\t-X num : minimum bit-width of a MUX operator to abstract [default = %d]\n",      pPars->nBitsMux );
    Abc_Print( -2, "\t-F num : minimum bit-width of a flip-flop to abstract [default = %d]\n",         pPars->nBitsFlop );
    Abc_Print( -2, "\t-I num : maximum number of CEGAR iterations [default = %d]\n",                   pPars->nIterMax );
    Abc_Print( -2, "\t-L num : maximum number of each type of signals [default = %d]\n",               pPars->nLimit );
    Abc_Print( -2, "\t-d     : toggle using another way of creating abstractions [default = %s]\n",    pPars->fAbs2? "yes": "no" );
    Abc_Print( -2, "\t-x     : toggle XORing outputs of word-level miter [default = %s]\n",            pPars->fXorOutput? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n",                  pPars->fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-w     : toggle printing verbose PDR output [default = %s]\n",                   pPars->fPdrVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandAbs2( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
    Wlc_Par_t Pars, * pPars = &Pars;
    int c;
    Wlc_ManSetDefaultParams( pPars );
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "AMXFIxvwh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'A':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-A\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsAdd = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsAdd < 0 )
                goto usage;
            break;
        case 'M':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-M\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsMul = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsMul < 0 )
                goto usage;
            break;
        case 'X':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-X\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsMux = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsMux < 0 )
                goto usage;
            break;
        case 'F':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-F\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nBitsFlop = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nBitsFlop < 0 )
                goto usage;
            break;
        case 'I':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-I\" should be followed by an integer.\n" );
                goto usage;
            }
            pPars->nIterMax = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPars->nIterMax < 0 )
                goto usage;
            break;
        case 'x':
            pPars->fXorOutput ^= 1;
            break;
        case 'v':
            pPars->fVerbose ^= 1;
            break;
        case 'w':
            pPars->fPdrVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandCone(): There is no current design.\n" );
        return 0;
    }
    Wlc_NtkAbsCore2( pNtk, pPars );
    return 0;
usage:
    Abc_Print( -2, "usage: %%abs2 [-AMXFI num] [-xvwh]\n" );
    Abc_Print( -2, "\t         abstraction for word-level networks\n" );
    Abc_Print( -2, "\t-A num : minimum bit-width of an adder/subtractor to abstract [default = %d]\n", pPars->nBitsAdd );
    Abc_Print( -2, "\t-M num : minimum bit-width of a multiplier to abstract [default = %d]\n",        pPars->nBitsMul );
    Abc_Print( -2, "\t-X num : minimum bit-width of a MUX operator to abstract [default = %d]\n",      pPars->nBitsMux );
    Abc_Print( -2, "\t-F num : minimum bit-width of a flip-flop to abstract [default = %d]\n",         pPars->nBitsFlop );
    Abc_Print( -2, "\t-I num : maximum number of CEGAR iterations [default = %d]\n",                   pPars->nIterMax );
    Abc_Print( -2, "\t-x     : toggle XORing outputs of word-level miter [default = %s]\n",            pPars->fXorOutput? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n",                  pPars->fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-w     : toggle printing verbose PDR output [default = %s]\n",                   pPars->fPdrVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandBlast( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
    Gia_Man_t * pNew = NULL; int c;
    Wlc_BstPar_t Par, * pPar = &Par;
    Wlc_BstParDefault( pPar );
    pPar->nOutputRange = 2;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "ORAMcombdsvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'O':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-O\" should be followed by an integer.\n" );
                goto usage;
            }
            pPar->iOutput = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPar->iOutput < 0 )
                goto usage;
            break;
        case 'R':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-R\" should be followed by an integer.\n" );
                goto usage;
            }
            pPar->nOutputRange = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPar->nOutputRange < 0 )
                goto usage;
            break;
        case 'A':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-A\" should be followed by an integer.\n" );
                goto usage;
            }
            pPar->nAdderLimit = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPar->nAdderLimit < 0 )
                goto usage;
            break;
        case 'M':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-M\" should be followed by an integer.\n" );
                goto usage;
            }
            pPar->nMultLimit = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( pPar->nMultLimit < 0 )
                goto usage;
            break;
        case 'c':
            pPar->fGiaSimple ^= 1;
            break;
        case 'o':
            pPar->fAddOutputs ^= 1;
            break;
        case 'm':
            pPar->fMulti ^= 1;
            break;
        case 'b':
            pPar->fBooth ^= 1;
            break;
        case 'd':
            pPar->fCreateMiter ^= 1;
            break;
        case 's':
            pPar->fDecMuxes ^= 1;
            break;
        case 'v':
            pPar->fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandBlast(): There is no current design.\n" );
        return 0;
    }
    if ( pPar->fMulti )
    {
        pPar->vBoxIds = Wlc_NtkCollectMultipliers( pNtk );
        if ( pPar->vBoxIds == NULL )
            Abc_Print( 1, "Warning:  There is no multipliers in the design.\n" );
    }
    else if ( pPar->nAdderLimit || pPar->nMultLimit )
    {
        int CountA, CountM;
        pPar->vBoxIds = Wlc_NtkCollectAddMult( pNtk, pPar, &CountA, &CountM );
        if ( pPar->vBoxIds == NULL )
            Abc_Print( 1, "Warning:  There is no adders and multipliers that will not be blasted.\n" );
        else 
            Abc_Print( 1, "Warning:  %d adders and %d multipliers will not be blasted.\n", CountA, CountM );
    }
    if ( pPar->iOutput >= 0 && pPar->iOutput + pPar->nOutputRange > Wlc_NtkPoNum(pNtk) )
    {
        Abc_Print( 1, "Abc_CommandBlast(): The output range [%d:%d] is incorrect.\n", pPar->iOutput, pPar->iOutput + pPar->nOutputRange - 1 );
        return 0;
    }
    // transform
    pNew = Wlc_NtkBitBlast( pNtk, pPar );
    Vec_IntFreeP( &pPar->vBoxIds );
    if ( pNew == NULL )
    {
        Abc_Print( 1, "Abc_CommandBlast(): Bit-blasting has failed.\n" );
        return 0;
    }
    Abc_FrameUpdateGia( pAbc, pNew );
    return 0;
usage:
    Abc_Print( -2, "usage: %%blast [-ORAM num] [-combdsvh]\n" );
    Abc_Print( -2, "\t         performs bit-blasting of the word-level design\n" );
    Abc_Print( -2, "\t-O num : zero-based index of the first word-level PO to bit-blast [default = %d]\n", pPar->iOutput );
    Abc_Print( -2, "\t-R num : the total number of word-level POs to bit-blast [default = %d]\n",          pPar->nOutputRange );
    Abc_Print( -2, "\t-A num : blast adders smaller than this (0 = unused) [default = %d]\n",              pPar->nAdderLimit );
    Abc_Print( -2, "\t-M num : blast multipliers smaller than this (0 = unused) [default = %d]\n",         pPar->nMultLimit );
    Abc_Print( -2, "\t-c     : toggle using AIG w/o const propagation and strashing [default = %s]\n",     pPar->fGiaSimple? "yes": "no" );
    Abc_Print( -2, "\t-o     : toggle using additional POs on the word-level boundaries [default = %s]\n", pPar->fAddOutputs? "yes": "no" );
    Abc_Print( -2, "\t-m     : toggle creating boxes for all multipliers in the design [default = %s]\n",  pPar->fMulti? "yes": "no" );
    Abc_Print( -2, "\t-b     : toggle generating radix-4 Booth multipliers [default = %s]\n",              pPar->fBooth? "yes": "no" );
    Abc_Print( -2, "\t-d     : toggle creating dual-output miter [default = %s]\n",                        pPar->fCreateMiter? "yes": "no" );
    Abc_Print( -2, "\t-s     : toggle creating decoded MUXes [default = %s]\n",                            pPar->fDecMuxes? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n",                      pPar->fVerbose? "yes": "no" );
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
    extern Wlc_Ntk_t * Wlc_NtkGraftMulti( Wlc_Ntk_t * p, int fVerbose );
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
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
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandGraft(): There is no current design.\n" );
        return 0;
    }
    pNtk = Wlc_NtkGraftMulti( pNtk, fVerbose );
    Wlc_AbcUpdateNtk( pAbc, pNtk );
    return 0;
usage:
    Abc_Print( -2, "usage: %%graft [-vh]\n" );
    Abc_Print( -2, "\t         detects multipliers in LHS of the miter and moves them to RHS\n" );
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
int Abc_CommandProfile( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
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
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandProfile(): There is no current design.\n" );
        return 0;
    }
    Wlc_WinProfileArith( pNtk );
    return 0;
usage:
    Abc_Print( -2, "usage: %%profile [-vh]\n" );
    Abc_Print( -2, "\t         profiles arithmetic components in the word-level networks\n" );
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
int Abc_CommandShortNames( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
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
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandProfile(): There is no current design.\n" );
        return 0;
    }
    Wlc_NtkShortNames( pNtk );
    return 0;
usage:
    Abc_Print( -2, "usage: %%short_names [-vh]\n" );
    Abc_Print( -2, "\t         derives short names for all objects of the network\n" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_CommandShow( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern void Wlc_NtkShow( Wlc_Ntk_t * p, Vec_Int_t * vBold );
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
    int c, fVerbose = 0;
    // set defaults
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'v':
            fVerbose ^= 1;
            break;
        default:
            goto usage;
        }
    }
    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    Wlc_NtkShow( pNtk, NULL );
    return 0;

usage:
    Abc_Print( -2, "usage: %%show [-h]\n" );
    Abc_Print( -2, "       visualizes the network structure using DOT and GSVIEW\n" );
#ifdef WIN32
    Abc_Print( -2, "       \"dot.exe\" and \"gsview32.exe\" should be set in the paths\n" );
    Abc_Print( -2, "       (\"gsview32.exe\" may be in \"C:\\Program Files\\Ghostgum\\gsview\\\")\n" );
#endif
    Abc_Print( -2, "\t-h    : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandTest( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern void Wlc_NtkSimulateTest( Wlc_Ntk_t * p );
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
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
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandTest(): There is no current design.\n" );
        return 0;
    }
    // transform
    //pNtk = Wlc_NtkUifNodePairs( pNtk, NULL );
    //pNtk = Wlc_NtkAbstractNodes( pNtk, NULL );
    //Wlc_AbcUpdateNtk( pAbc, pNtk );
    //Wlc_GenerateSmtStdout( pAbc );
    //Wlc_NtkSimulateTest( (Wlc_Ntk_t *)pAbc->pAbcWlc );
    //pNtk = Wlc_NtkDupSingleNodes( pNtk );
    //Wlc_AbcUpdateNtk( pAbc, pNtk );
    //Wlc_ReadNdrTest( pNtk );
    return 0;
usage:
    Abc_Print( -2, "usage: %%test [-vh]\n" );
    Abc_Print( -2, "\t         experiments with word-level networks\n" );
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
int Abc_CommandInvPs( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Vec_Int_t * Pdr_InvCounts( Vec_Int_t * vInv );
    extern void Wlc_NtkPrintInvStats( Wlc_Ntk_t * pNtk, Vec_Int_t * vInv, int fVerbose );
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
    Vec_Int_t * vCounts;
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
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandInvPs(): There is no current design.\n" );
        return 0;
    }
    if ( Wlc_AbcGetInv(pAbc) == NULL )
    {
        Abc_Print( 1, "Abc_CommandInvPs(): Invariant is not available.\n" );
        return 0;
    }
    vCounts = Pdr_InvCounts( Wlc_AbcGetInv(pAbc) );
    Wlc_NtkPrintInvStats( pNtk, vCounts, fVerbose );
    Vec_IntFree( vCounts );
    return 0;
usage:
    Abc_Print( -2, "usage: inv_ps [-vh]\n" );
    Abc_Print( -2, "\t         prints statistics for inductive invariant\n" );
    Abc_Print( -2, "\t         (in the case of \'sat\' or \'undecided\', inifity clauses are used)\n" );
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
int Abc_CommandInvPrint( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern void Pdr_InvPrint( Vec_Int_t * vInv, int fVerbose );
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
    if ( Wlc_AbcGetInv(pAbc) == NULL )
    {
        Abc_Print( 1, "Abc_CommandInvPs(): Invariant is not available.\n" );
        return 0;
    }
    Pdr_InvPrint( Wlc_AbcGetInv(pAbc), fVerbose );
    return 0;
usage:
    Abc_Print( -2, "usage: inv_print [-vh]\n" );
    Abc_Print( -2, "\t         prints the current inductive invariant\n" );
    Abc_Print( -2, "\t         (in the case of \'sat\' or \'undecided\', inifity clauses are used)\n" );
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
int Abc_CommandInvCheck( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    abctime clk = Abc_Clock();
    extern int Pdr_InvCheck( Gia_Man_t * p, Vec_Int_t * vInv, int fVerbose );
    int c, nFailed, fVerbose  = 0;
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
    if ( pAbc->pGia == NULL )
    {
        Abc_Print( 1, "Abc_CommandInvMin(): There is no current design.\n" );
        return 0;
    }
    if ( Wlc_AbcGetInv(pAbc) == NULL )
    {
        Abc_Print( 1, "Abc_CommandInvMin(): There is no saved invariant.\n" );
        return 0;
    }
    if ( Gia_ManRegNum(pAbc->pGia) != Vec_IntEntryLast(Wlc_AbcGetInv(pAbc)) )
    {
        Abc_Print( 1, "Abc_CommandInvMin(): The number of flops in the invariant and in GIA should be the same.\n" );
        return 0;
    }
    nFailed = Pdr_InvCheck( pAbc->pGia, Wlc_AbcGetInv(pAbc), fVerbose );
    if ( nFailed )
        printf( "Invariant verification failed for %d clauses (out of %d). ", nFailed, Vec_IntEntry(Wlc_AbcGetInv(pAbc),0) );
    else
        printf( "Invariant verification succeeded.    " );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return 0;
usage:
    Abc_Print( -2, "usage: inv_check [-vh]\n" );
    Abc_Print( -2, "\t         checks that the invariant is indeed an inductive invariant\n" );
    Abc_Print( -2, "\t         (AIG representing the design should be in the &-space)\n" );
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
int Abc_CommandInvGet( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Abc_Ntk_t * Wlc_NtkGetInv( Wlc_Ntk_t * pNtk, Vec_Int_t * vInv );
    Abc_Ntk_t * pMainNtk;
    Wlc_Ntk_t * pNtk = Wlc_AbcGetNtk(pAbc);
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
    if ( Wlc_AbcGetInv(pAbc) == NULL )
    {
        Abc_Print( 1, "Abc_CommandInvGet(): Invariant is not available.\n" );
        return 0;
    }
    // derive the network
    pMainNtk = Wlc_NtkGetInv( pNtk, Wlc_AbcGetInv(pAbc) );
    // replace the current network
    if ( pMainNtk )
        Abc_FrameReplaceCurrentNetwork( pAbc, pMainNtk );
    return 0;
usage:
    Abc_Print( -2, "usage: inv_get [-vh]\n" );
    Abc_Print( -2, "\t         places invariant found by PDR as the current network in the main-space\n" );
    Abc_Print( -2, "\t         (in the case of \'sat\' or \'undecided\', inifity clauses are used)\n" );
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
int Abc_CommandInvPut( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Vec_Int_t * Wlc_NtkGetPut( Abc_Ntk_t * pNtk, Gia_Man_t * pGia );
    Vec_Int_t * vInv = NULL;
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
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
    if ( pNtk == NULL )
    {
        Abc_Print( 1, "Abc_CommandInvPut(): There is no current design.\n" );
        return 0;
    }
    if ( pAbc->pGia == NULL )
    {
        Abc_Print( 1, "Abc_CommandInvPut(): There is no current AIG.\n" );
        return 0;
    }
    // derive the network
    vInv = Wlc_NtkGetPut( pNtk, pAbc->pGia );
    if ( vInv )
        Abc_FrameSetInv( vInv );
    return 0;
usage:
    Abc_Print( -2, "usage: inv_put [-vh]\n" );
    Abc_Print( -2, "\t         inputs the current network in the main-space as an invariant\n" );
    Abc_Print( -2, "\t         (AIG representing the design should be in the &-space)\n" );
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
int Abc_CommandInvMin( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern Vec_Int_t * Pdr_InvMinimize( Gia_Man_t * p, Vec_Int_t * vInv, int fVerbose );
    extern Vec_Int_t * Pdr_InvMinimizeLits( Gia_Man_t * p, Vec_Int_t * vInv, int fVerbose );
    Vec_Int_t * vInv, * vInv2;
    int c, fLits = 0, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "lvh" ) ) != EOF )
    {
        switch ( c )
        {
            case 'l':
                fLits ^= 1;
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
    if ( pAbc->pGia == NULL )
    {
        Abc_Print( 1, "Abc_CommandInvMin(): There is no current design.\n" );
        return 0;
    }
    if ( Wlc_AbcGetInv(pAbc) == NULL )
    {
        Abc_Print( 1, "Abc_CommandInvMin(): Invariant is not available.\n" );
        return 0;
    }
    vInv = Wlc_AbcGetInv(pAbc);
    if ( Gia_ManRegNum(pAbc->pGia) != Vec_IntEntryLast(vInv) )
    {
        Abc_Print( 1, "Abc_CommandInvMin(): The number of flops in the invariant and in GIA should be the same.\n" );
        return 0;
    }
    if ( fLits )
        vInv2 = Pdr_InvMinimizeLits( pAbc->pGia, vInv, fVerbose );
    else
        vInv2 = Pdr_InvMinimize( pAbc->pGia, vInv, fVerbose );
    if ( vInv2 )
        Abc_FrameSetInv( vInv2 );
    return 0;
usage:
    Abc_Print( -2, "usage: inv_min [-lvh]\n" );
    Abc_Print( -2, "\t         performs minimization of the current invariant\n" );
    Abc_Print( -2, "\t         (AIG representing the design should be in the &-space)\n" );
    Abc_Print( -2, "\t-l     : toggle minimizing literals rather than clauses [default = %s]\n", fLits? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

