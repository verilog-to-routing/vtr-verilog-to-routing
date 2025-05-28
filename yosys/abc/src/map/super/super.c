/**CFile****************************************************************

  FileName    [super.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Pre-computation of supergates.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 18, 2003.]

  Revision    [$Id: super.c,v 1.6 2004/10/30 20:51:11 satrajit Exp $]

***********************************************************************/

#include "superInt.h"
#include "base/main/mainInt.h"
#include "map/mio/mio.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Super_CommandSupergates   ( Abc_Frame_t * pAbc, int argc, char **argv );
static int Super_CommandSupergatesAnd( Abc_Frame_t * pAbc, int argc, char **argv );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_Init( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "SC mapping",  "super",   Super_CommandSupergates,    0 ); 
    Cmd_CommandAdd( pAbc, "SC mapping",  "super2",  Super_CommandSupergatesAnd, 0 ); 
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Super_End( Abc_Frame_t * pAbc )
{
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Super_CommandSupergatesAnd( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pOut, * pErr;
    int nVarsMax, nLevels;
    int fVerbose;
    int c;

    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    // set the defaults
    nVarsMax = 4;
    nLevels  = 3;
    fVerbose = 0;
    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "ILvh")) != EOF ) 
    {
        switch (c) 
        {
            case 'I':
                nVarsMax = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( nVarsMax < 0 ) 
                    goto usage;
                break;
            case 'L':
                nLevels = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( nLevels < 0 ) 
                    goto usage;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }

    Super2_Precompute( nVarsMax, nLevels, fVerbose );

    return 0;

usage:
    fprintf( pErr, "usage: super2 [-IL num] [-vh]\n");
    fprintf( pErr, "\t         precomputes the supergates composed of AND2s and INVs\n" );  
    fprintf( pErr, "\t-I num : the max number of inputs to the supergate [default = %d]\n", nVarsMax );
    fprintf( pErr, "\t-L num : the max number of logic levels of gates [default = %d]\n", nLevels );
    fprintf( pErr, "\t-v     : enable verbose output\n");
    fprintf( pErr, "\t-h     : print the help message\n");
    return 1;       /* error exit */
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Super_CommandSupergates( Abc_Frame_t * pAbc, int argc, char **argv )
{
    FILE * pFile;
    FILE * pOut, * pErr;
    Mio_Library_t * pLib;
    char * FileName, * ExcludeFile;
    float DelayLimit;
    float AreaLimit;
    int fSkipInvs;
    int fWriteOldFormat; 
    int nVarsMax, nLevels, nGatesMax, TimeLimit;
    int fVerbose;
    int c;

    pOut = Abc_FrameReadOut(pAbc);
    pErr = Abc_FrameReadErr(pAbc);

    // set the defaults
    nVarsMax   = 5;
    nLevels    = 2;
    DelayLimit = 0;
    AreaLimit  = 0;
    nGatesMax  = 0;
    TimeLimit  = 0;
    fSkipInvs  = 1;
    fVerbose   = 0;
    fWriteOldFormat = 0;
    ExcludeFile = 0;

    Extra_UtilGetoptReset();
    while ( (c = Extra_UtilGetopt(argc, argv, "ILNTDAEsovh")) != EOF ) 
    {
        switch (c) 
        {
            case 'I':
                nVarsMax = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( nVarsMax < 0 ) 
                    goto usage;
                break;
            case 'L':
                nLevels = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( nLevels < 0 ) 
                    goto usage;
                break;
            case 'N':
                nGatesMax = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( nGatesMax < 0 ) 
                    goto usage;
                break;
            case 'T':
                TimeLimit = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( TimeLimit < 0 ) 
                    goto usage;
                break;
            case 'D':
                DelayLimit = (float)atof(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( DelayLimit <= 0.0 ) 
                    goto usage;
                break;
            case 'A':
                AreaLimit = (float)atof(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( AreaLimit <= 0.0 ) 
                    goto usage;
                break;
            case 'E':
                ExcludeFile = argv[globalUtilOptind];
                if ( ExcludeFile == 0 )
                    goto usage;
                globalUtilOptind++;
                break;
            case 's':
                fSkipInvs ^= 1;
                break;
            case 'o':
                fWriteOldFormat ^= 1;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
                break;
            default:
                goto usage;
        }
    }


    if ( argc != globalUtilOptind + 1 )
    {
        fprintf( pErr, "The genlib library file should be given on the command line.\n" );
        goto usage;
    }

    if ( nVarsMax < 2 || nVarsMax > 6 )
    {
        fprintf( pErr, "The max number of variables (%d) should be more than 1 and less than 7.\n", nVarsMax );
        goto usage;
    }

    // get the input file name
    FileName = argv[globalUtilOptind];
    if ( (pFile = Io_FileOpen( FileName, "open_path", "r", 0 )) == NULL )
//    if ( (pFile = fopen( FileName, "r" )) == NULL )
    {
        fprintf( pErr, "Cannot open input file \"%s\". ", FileName );
        if (( FileName = Extra_FileGetSimilarName( FileName, ".genlib", ".lib", ".gen", ".g", NULL ) ))
            fprintf( pErr, "Did you mean \"%s\"?", FileName );
        fprintf( pErr, "\n" );
        return 1;
    }
    fclose( pFile );

    // set the new network
    pLib = Mio_LibraryRead( FileName, NULL, ExcludeFile, nVarsMax, fVerbose );
    if ( pLib == NULL )
    {
        fprintf( pErr, "Reading library has failed.\n" );
        goto usage;
    }

    // compute the gates
    FileName = Extra_FileNameGenericAppend(Mio_LibraryReadName(pLib), ".super");
    Super_Precompute( pLib, nVarsMax, nLevels, nGatesMax, DelayLimit, AreaLimit, TimeLimit, fSkipInvs, fVerbose, FileName );

    // delete the library
    Mio_LibraryDelete( pLib );
    return 0;

usage:
    fprintf( pErr, "usage: super [-ILNT num] [-DA float] [-E file] [-sovh] <genlib_file>\n");
    fprintf( pErr, "\t         precomputes the supergates for the given genlib library\n" );  
    fprintf( pErr, "\t-I num   : the max number of supergate inputs [default = %d]\n", nVarsMax );
    fprintf( pErr, "\t-L num   : the max number of levels of gates [default = %d]\n", nLevels );
    fprintf( pErr, "\t-N num   : the limit on the number of considered supergates [default = %d]\n", nGatesMax );
    fprintf( pErr, "\t-T num   : the approximate runtime limit in seconds [default = %d]\n", TimeLimit );
    fprintf( pErr, "\t-D float : the max delay of the supergates [default = %.2f]\n", DelayLimit );
    fprintf( pErr, "\t-A float : the max area of the supergates [default = %.2f]\n", AreaLimit );
    fprintf( pErr, "\t-E file  : file contains list of genlib gates to exclude\n" );
    fprintf( pErr, "\t-s       : toggle the use of inverters at the inputs [default = %s]\n", (fSkipInvs? "no": "yes") );
    fprintf( pErr, "\t-o       : toggle dumping the supergate library in old format [default = %s]\n", (fWriteOldFormat? "yes": "no") );
    fprintf( pErr, "\t-v       : enable verbose output [default = %s]\n", (fVerbose? "yes" : "no") );
    fprintf( pErr, "\t-h       : print the help message\n");
    fprintf( pErr, "\n");
    fprintf( pErr, "\tHere is a piece of advice on precomputing supergate libraries:\n");
    fprintf( pErr, "\t\n");
    fprintf( pErr, "\tStart with the number of inputs equal to 5 (-I 5), the number of \n");
    fprintf( pErr, "\tlevels equal to 2 (-L 2), the delay equal to 2-3 delays of inverter, \n");
    fprintf( pErr, "\tthe area equal to 2-3 areas of two input NAND, and runtime limit equal \n");
    fprintf( pErr, "\tto 10 seconds (-T 10). Run precomputation and learn from the result.\n");
    fprintf( pErr, "\tDetermine what parameter is most constraining and try to increase \n");
    fprintf( pErr, "\tthe value of that parameter. The goal is to have a well-balanced\n");
    fprintf( pErr, "\tset of constraints and the resulting supergate library containing\n");
    fprintf( pErr, "\tapproximately 5K-20K supergates. Typically, it is better to increase\n");
    fprintf( pErr, "\tdelay limit rather than area limit, because having large-area supergates\n");
    fprintf( pErr, "\tmay result in a considerable increase in area.\n");
    fprintf( pErr, "\t\n");
    fprintf( pErr, "\tNote that a good supergate library for experiments typically can be \n");
    fprintf( pErr, "\tprecomputed in 30 sec or less. Increasing runtime limit makes sense when\n");
    fprintf( pErr, "\tother parameters are well-balanced and it is needed to enumerate more\n");
    fprintf( pErr, "\tchoices to have a good result. In the end, to compute the final library\n");
    fprintf( pErr, "\tthe runtime can be set to 300 sec to ensure the ultimate quality.\n");
    fprintf( pErr, "\tIn some cases, the runtime has to be reduced if the supergate library\n");
    fprintf( pErr, "\tcontains too many supergates (> 500K).\n");
    fprintf( pErr, "\t\n");
    fprintf( pErr, "\tWhen precomputing libraries of 6 inputs (-i 6), start with even more \n");
    fprintf( pErr, "\trestricted parameters and gradually increase them until the goal is met.\n");
    fprintf( pErr, "\t\n");
    return 1;       /* error exit */
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

