/**CFile****************************************************************

  FileName    [plaCom.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SOP manager.]

  Synopsis    [Scalable SOP transformations.]

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - March 18, 2015.]

  Revision    [$Id: plaCom.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "pla.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int  Abc_CommandReadPla  ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandWritePla ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandPs       ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandGen      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandMerge    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Abc_CommandTest     ( Abc_Frame_t * pAbc, int argc, char ** argv );

static inline Pla_Man_t * Pla_AbcGetMan( Abc_Frame_t * pAbc )                    { return (Pla_Man_t *)pAbc->pAbcPla;                      }
static inline void        Pla_AbcFreeMan( Abc_Frame_t * pAbc )                   { if ( pAbc->pAbcPla ) Pla_ManFree(Pla_AbcGetMan(pAbc));  }
static inline void        Pla_AbcUpdateMan( Abc_Frame_t * pAbc, Pla_Man_t * p )  { Pla_AbcFreeMan(pAbc); pAbc->pAbcPla = p;                }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Pla_Init( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "Two-level", "|read",       Abc_CommandReadPla,   0 );
    Cmd_CommandAdd( pAbc, "Two-level", "|write",      Abc_CommandWritePla,  0 );
    Cmd_CommandAdd( pAbc, "Two-level", "|ps",         Abc_CommandPs,        0 );
    Cmd_CommandAdd( pAbc, "Two-level", "|gen",        Abc_CommandGen,       0 );
    Cmd_CommandAdd( pAbc, "Two-level", "|merge",      Abc_CommandMerge,     0 );
    Cmd_CommandAdd( pAbc, "Two-level", "|test",       Abc_CommandTest,      0 );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Pla_End( Abc_Frame_t * pAbc )
{
    Pla_AbcFreeMan( pAbc );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Pla_SetMan( Abc_Frame_t * pAbc, Pla_Man_t * p )
{
    Pla_AbcUpdateMan( pAbc, p );
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Abc_CommandReadPla( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    FILE * pFile;
    Pla_Man_t * p = NULL;
    char * pFileName = NULL;
    int c, fVerbose  =    0;
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
    if ( argc != globalUtilOptind + 1 )
    {
        printf( "Abc_CommandReadPla(): Input file name should be given on the command line.\n" );
        return 0;
    }
    // get the file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "r" )) == NULL )
    {
        Abc_Print( 1, "Cannot open input file \"%s\". ", pFileName );
        if ( (pFileName = Extra_FileGetSimilarName( pFileName, ".pla", NULL, NULL, NULL, NULL )) )
            Abc_Print( 1, "Did you mean \"%s\"?", pFileName );
        Abc_Print( 1, "\n" );
        return 0;
    }
    fclose( pFile );

    // perform reading
    if ( !strcmp( Extra_FileNameExtension(pFileName), "pla" )  )
        p = Pla_ReadPla( pFileName );
    else
    {
        printf( "Abc_CommandReadPla(): Unknown file extension.\n" );
        return 0;
    }
    Pla_AbcUpdateMan( pAbc, p );
    return 0;
usage:
    Abc_Print( -2, "usage: |read [-vh] <file_name>\n" );
    Abc_Print( -2, "\t         reads the SOP from a PLA file\n" );
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
int Abc_CommandWritePla( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Pla_Man_t * p = Pla_AbcGetMan(pAbc);
    char * pFileName = NULL;
    int c, fVerbose  =    0;
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
        Abc_Print( 1, "Abc_CommandWritePla(): There is no current design.\n" );
        return 0;
    }
    if ( argc == globalUtilOptind )
        pFileName = Extra_FileNameGenericAppend( p->pName, "_out.v" );
    else if ( argc == globalUtilOptind + 1 )
        pFileName = argv[globalUtilOptind];
    else
    {
        printf( "Output file name should be given on the command line.\n" );
        return 0;
    }
    Pla_WritePla( p, pFileName );
    return 0;
usage:
    Abc_Print( -2, "usage: |write [-vh]\n" );
    Abc_Print( -2, "\t         writes the SOP into a PLA file\n" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n",        fVerbose? "yes": "no" );
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
    Pla_Man_t * p = Pla_AbcGetMan(pAbc);
    int fShowMulti   = 0;
    int fShowAdder   = 0;
    int fDistrib     = 0;
    int c, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "madvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'm':
            fShowMulti ^= 1;
            break;
        case 'a':
            fShowAdder ^= 1;
            break;
        case 'd':
            fDistrib ^= 1;
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
        Abc_Print( 1, "Abc_CommandPs(): There is no current design.\n" );
        return 0;
    }
    Pla_ManPrintStats( p, fVerbose );
    return 0;
usage:
    Abc_Print( -2, "usage: |ps [-madvh]\n" );
    Abc_Print( -2, "\t         prints statistics\n" );
    Abc_Print( -2, "\t-m     : toggle printing multipliers [default = %s]\n",         fShowMulti? "yes": "no" );
    Abc_Print( -2, "\t-a     : toggle printing adders [default = %s]\n",              fShowAdder? "yes": "no" );
    Abc_Print( -2, "\t-d     : toggle printing distrubition [default = %s]\n",        fDistrib? "yes": "no" );
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
int Abc_CommandGen( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern void Pla_GenSorter( int nVars );
    Pla_Man_t * p  = NULL;
    int nInputs      =  8;
    int nOutputs     =  1;
    int nCubes       = 20;
    int Seed         =  0;
    int fSorter      =  0;
    int fPrimes      =  0;
    int c, fVerbose  =  0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "IOPSspvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'I':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-I\" should be followed by an integer.\n" );
                goto usage;
            }
            nInputs = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nInputs < 0 )
                goto usage;
            break;
        case 'O':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-O\" should be followed by an integer.\n" );
                goto usage;
            }
            nOutputs = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nOutputs < 0 )
                goto usage;
            break;
        case 'P':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-P\" should be followed by an integer.\n" );
                goto usage;
            }
            nCubes = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nCubes < 0 )
                goto usage;
            break;
        case 'S':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-S\" should be followed by an integer.\n" );
                goto usage;
            }
            Seed = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( Seed < 0 )
                goto usage;
            break;
        case 's':
            fSorter ^= 1;
            break;
        case 'p':
            fPrimes ^= 1;
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
    if ( fSorter )
        Pla_GenSorter( nInputs );
    else if ( fPrimes )
        p = Pla_ManPrimesDetector( nInputs );
    else
    {
        Gia_ManRandom( 1 );
        for ( c = 0; c < Seed; c++ )
            Gia_ManRandom( 0 );
        p = Pla_ManGenerate( nInputs, nOutputs, nCubes, fVerbose );
    }
    Pla_AbcUpdateMan( pAbc, p );
    return 0;
usage:
    Abc_Print( -2, "usage: |gen [-IOPS num] [-spvh]\n" );
    Abc_Print( -2, "\t         generate random or specialized SOP\n" );
    Abc_Print( -2, "\t-I num : the number of inputs [default = %d]\n", nInputs );
    Abc_Print( -2, "\t-O num : the number of outputs [default = %d]\n", nOutputs );
    Abc_Print( -2, "\t-P num : the number of products [default = %d]\n", nCubes );
    Abc_Print( -2, "\t-S num : ramdom seed (0 <= num <= 1000) [default = %d]\n", Seed );
    Abc_Print( -2, "\t-s     : toggle generating sorter as a PLA file [default = %s]\n", fSorter? "yes": "no" );
    Abc_Print( -2, "\t-p     : toggle generating prime detector [default = %s]\n", fPrimes? "yes": "no" );
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
int Abc_CommandMerge( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Pla_Man_t * p = Pla_AbcGetMan(pAbc);
    int c, fMulti = 0, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'm':
            fMulti ^= 1;
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
        Abc_Print( 1, "Abc_CommandMerge(): There is no current design.\n" );
        return 0;
    }
    // transform
    Pla_ManDist1Merge( p );
    return 0;
usage:
    Abc_Print( -2, "usage: |merge [-mvh]\n" );
    Abc_Print( -2, "\t         performs distance-1 merge using cube hashing\n" );
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
int Abc_CommandTest( Abc_Frame_t * pAbc, int argc, char ** argv )
{
//    Pla_Man_t * p = Pla_AbcGetMan(pAbc);
    int c, nVars = 4, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "Nvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'N':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-N\" should be followed by an integer.\n" );
                goto usage;
            }
            nVars = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nVars < 0 )
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
/*
    if ( p == NULL )
    {
        Abc_Print( 1, "Abc_CommandTest(): There is no current design.\n" );
        return 0;
    }
*/
    //Pla_ManFxPerformSimple( nVars );
    //Pla_ManConvertFromBits( p );
    //Pla_ManConvertToBits( p );
    //Pla_ManPerformFxch( p );
    return 0;
usage:
    Abc_Print( -2, "usage: |test [-N num] [-vh]\n" );
    Abc_Print( -2, "\t         experiments with SOPs\n" );
    Abc_Print( -2, "\t-N num : the number of variables [default = %d]\n", nVars );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
