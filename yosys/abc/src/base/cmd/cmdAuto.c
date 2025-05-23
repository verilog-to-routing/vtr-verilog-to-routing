/**CFile****************************************************************

  FileName    [cmdAuto.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Autotuner.]

  Author      [Alan Mishchenko, Bruno Schmitt]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cmdAuto.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "misc/util/abc_global.h"
#include "misc/extra/extra.h"
#include "aig/gia/gia.h"
#include "sat/satoko/satoko.h"

#ifdef ABC_USE_PTHREADS

#ifdef _WIN32
#include "../lib/pthread.h"
#else
#include <pthread.h>
#include <unistd.h>
#endif

#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
#define CMD_AUTO_LINE_MAX 1000  // max number of chars in the string
#define CMD_AUTO_ARG_MAX   100  // max number of arguments in the call

extern int Gia_ManSatokoCallOne( Gia_Man_t * p, satoko_opts_t * opts, int iOutput );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Printing option structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cmd_RunAutoTunerPrintOptions( satoko_opts_t * pOpts )
{
    printf( "-C %d  ",   (int)pOpts->conf_limit );
    printf( "-V %.3f  ", (float)pOpts->var_decay );
    printf( "-W %.3f  ", (float)pOpts->clause_decay );
    if ( pOpts->verbose )
        printf( "-v" );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [The main evaluation procedure for an array of AIGs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cmd_RunAutoTunerEvalSimple( Vec_Ptr_t * vAigs, satoko_opts_t * pOpts )
{
    Gia_Man_t * pGia;
    int i, TotalCost = 0;
    //printf( "Tuning with options: " );
    //Cmd_RunAutoTunerPrintOptions( pOpts );
    Vec_PtrForEachEntry( Gia_Man_t *, vAigs, pGia, i )
        TotalCost += Gia_ManSatokoCallOne( pGia, pOpts, -1 );
    return TotalCost;
}

/**Function*************************************************************

  Synopsis    [The main evaluation procedure for the set of AIGs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#ifndef ABC_USE_PTHREADS

int Cmd_RunAutoTunerEval( Vec_Ptr_t * vAigs, satoko_opts_t * pOpts, int nCores )
{
    return Cmd_RunAutoTunerEvalSimple( vAigs, pOpts );
}

#else // pthreads are used


#define CMD_THR_MAX 100
typedef struct Cmd_AutoData_t_
{
    Gia_Man_t *     pGia;
    satoko_opts_t * pOpts;
    int             iThread;
    int             nTimeOut;
    int             fWorking;
    int             Result;
} Cmd_AutoData_t;

void * Cmd_RunAutoTunerEvalWorkerThread( void * pArg )
{
    Cmd_AutoData_t * pThData = (Cmd_AutoData_t *)pArg;
    volatile int * pPlace = &pThData->fWorking;
    while ( 1 )
    {
        while ( *pPlace == 0 );
        assert( pThData->fWorking );
        if ( pThData->pGia == NULL ) // kills itself when there is nothing to do
        {
            pthread_exit( NULL );
            assert( 0 );
            return NULL;
        }
        pThData->Result = Gia_ManSatokoCallOne( pThData->pGia, pThData->pOpts, -1 );
        pThData->fWorking = 0;
    }
    assert( 0 );
    return NULL;
}
int Cmd_RunAutoTunerEval( Vec_Ptr_t * vAigs, satoko_opts_t * pOpts, int nProcs )
{
    Cmd_AutoData_t ThData[CMD_THR_MAX];
    pthread_t WorkerThread[CMD_THR_MAX];
    int i, status, fWorkToDo = 1, TotalCost = 0;
    Vec_Ptr_t * vStack;
    if ( nProcs == 1 )
        return Cmd_RunAutoTunerEvalSimple( vAigs, pOpts );
    // subtract manager thread
    nProcs--;
    assert( nProcs >= 1 && nProcs <= CMD_THR_MAX );
    // start threads
    for ( i = 0; i < nProcs; i++ )
    {
        ThData[i].pGia     = NULL;
        ThData[i].pOpts    = pOpts;
        ThData[i].iThread  = i;
        ThData[i].nTimeOut = -1;
        ThData[i].fWorking = 0;
        ThData[i].Result   = -1;
        status = pthread_create( WorkerThread + i, NULL,Cmd_RunAutoTunerEvalWorkerThread, (void *)(ThData + i) );  assert( status == 0 );
    }
    // look at the threads
    vStack = Vec_PtrDup(vAigs);
    while ( fWorkToDo )
    {
        fWorkToDo = (int)(Vec_PtrSize(vStack) > 0);
        for ( i = 0; i < nProcs; i++ )
        {
            // check if this thread is working
            if ( ThData[i].fWorking )
            {
                fWorkToDo = 1;
                continue;
            }
            // check if this thread has recently finished
            if ( ThData[i].pGia != NULL )
            {
                assert( ThData[i].Result >= 0 );
                TotalCost += ThData[i].Result;
                ThData[i].pGia = NULL;
            }
            if ( Vec_PtrSize(vStack) == 0 )
                continue;
            // give this thread a new job
            assert( ThData[i].pGia == NULL );
            ThData[i].pGia = (Gia_Man_t *)Vec_PtrPop( vStack );
            ThData[i].fWorking = 1;
        }
    }
    Vec_PtrFree( vStack );
    // stop threads
    for ( i = 0; i < nProcs; i++ )
    {
        assert( !ThData[i].fWorking );
        // stop
        ThData[i].pGia = NULL;
        ThData[i].fWorking = 1;
    }
    return TotalCost;
}

#endif // pthreads are used


/**Function*************************************************************

  Synopsis    [Derives all possible param stucts according to the config file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Cmd_DeriveConvertIntoString( int argc, char ** argv )
{
    char pBuffer[CMD_AUTO_LINE_MAX] = {0};
    int i;
    for ( i = 0; i < argc; i++ )
    {
        strcat( pBuffer, argv[i] );
        strcat( pBuffer, " " );
    }
    return Abc_UtilStrsav(pBuffer);
}
satoko_opts_t * Cmd_DeriveOptionFromSettings( int argc, char ** argv )
{
    int c;
    satoko_opts_t opts, * pOpts;
    satoko_default_opts(&opts);
    Extra_UtilGetoptReset();
#ifdef SATOKO_ACT_VAR_FIXED
    while ( ( c = Extra_UtilGetopt( argc, argv, "CPDEFGHIJKLMNOQRSTUhv" ) ) != EOF )
#else
    while ( ( c = Extra_UtilGetopt( argc, argv, "CPDEFGHIJKLMNOQRShv" ) ) != EOF )
#endif
    {
        switch ( c )
        {
        case 'C':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-C\" should be followed by an integer.\n" );
                return NULL;
            }
            opts.conf_limit = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( opts.conf_limit < 0 )
                return NULL;
            break;
         case 'P':
                if ( globalUtilOptind >= argc )
                {
                    Abc_Print( -1, "Command line switch \"-P\" should be followed by an integer.\n" );
                    return NULL;
                }
                opts.prop_limit = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                if ( opts.prop_limit < 0 )
                    return NULL;
                break;
         case 'D':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-D\" should be followed by an float.\n" );
                   return NULL;
               }
               opts.f_rst = atof(argv[globalUtilOptind]);
               globalUtilOptind++;
               if ( opts.f_rst < 0 )
                   return NULL;
               break;
         case 'E':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-E\" should be followed by an float.\n" );
                   return NULL;
               }
               opts.b_rst = atof(argv[globalUtilOptind]);
               globalUtilOptind++;
               if ( opts.b_rst < 0 )
                   return NULL;
               break;
         case 'F':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-F\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.fst_block_rst = (unsigned)atoi(argv[globalUtilOptind]);
               globalUtilOptind++;
               break;
         case 'G':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-G\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.sz_lbd_bqueue = (unsigned)atoi(argv[globalUtilOptind]);
               globalUtilOptind++;
               break;
         case 'H':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-H\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.sz_trail_bqueue = (unsigned)atoi(argv[globalUtilOptind]);
               globalUtilOptind++;
               break;
         case 'I':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-I\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.n_conf_fst_reduce = (unsigned)atoi(argv[globalUtilOptind]);
               globalUtilOptind++;
               break;
         case 'J':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-J\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.inc_reduce = (unsigned)atoi(argv[globalUtilOptind]);
               globalUtilOptind++;
               break;
         case 'K':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-K\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.inc_special_reduce = (unsigned)atoi(argv[globalUtilOptind]);
               globalUtilOptind++;
               break;
         case 'L':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-L\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.lbd_freeze_clause = (unsigned)atoi(argv[globalUtilOptind]);
               globalUtilOptind++;
               break;
         case 'M':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-M\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.learnt_ratio = atof(argv[globalUtilOptind]) / 100;
               globalUtilOptind++;
               if ( opts.learnt_ratio < 0 )
                   return NULL;
               break;
         case 'N':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-M\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.garbage_max_ratio = atof(argv[globalUtilOptind]) / 100;
               globalUtilOptind++;
               if ( opts.garbage_max_ratio < 0 )
                   return NULL;
               break;
         case 'O':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-O\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.clause_max_sz_bin_resol = (unsigned)atoi(argv[globalUtilOptind]);
               globalUtilOptind++;
               break;
         case 'Q':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-O\" should be followed by an integer.\n" );
                   return NULL;
               }
               opts.clause_min_lbd_bin_resol = (unsigned)atoi(argv[globalUtilOptind]);
               globalUtilOptind++;
               break;
         case 'R':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-R\" should be followed by an float.\n" );
                   return NULL;
               }
               opts.clause_decay = atof(argv[globalUtilOptind]);
               globalUtilOptind++;
               if ( opts.clause_decay < 0 )
                   return NULL;
               break;
         case 'S':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-S\" should be followed by an float.\n" );
                   return NULL;
               }
               opts.var_decay = atof(argv[globalUtilOptind]);
               globalUtilOptind++;
               if ( opts.var_decay < 0 )
                   return NULL;
               break;
#ifdef SATOKO_ACT_VAR_FIXED
         case 'T':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-T\" should be followed by an float.\n" );
                   return NULL;
               }
               opts.var_act_limit = (unsigned)strtol(argv[globalUtilOptind], NULL, 16);
               globalUtilOptind++;
               break;
         case 'U':
               if ( globalUtilOptind >= argc )
               {
                   Abc_Print( -1, "Command line switch \"-U\" should be followed by an float.\n" );
                   return NULL;
               }
               opts.var_act_rescale = (unsigned)strtol(argv[globalUtilOptind], NULL, 16);
               globalUtilOptind++;
               break;
#endif
        case 'h':
            return NULL;
        case 'v':
            opts.verbose ^= 1;
            break;

        default:
            return NULL;
        }
    }
    // return a copy of this parameter structure
    pOpts = ABC_ALLOC( satoko_opts_t, 1 );
    memcpy( pOpts, &opts, sizeof(satoko_opts_t) );
    return pOpts;
}
void Cmf_CreateOptions_rec( Vec_Wec_t * vPars, int iPar, char Argv[CMD_AUTO_ARG_MAX][20], int Argc, Vec_Ptr_t * vOpts )
{
    Vec_Int_t * vLine;
    int Symb, Num, i;
    assert( Argc <= CMD_AUTO_ARG_MAX );
    if ( Vec_WecSize(vPars) == iPar )
    {
        satoko_opts_t * pOpts;
        char * pArgv[CMD_AUTO_ARG_MAX];
        for ( i = 0; i < Argc; i++ )
            pArgv[i] = Argv[i];
        pOpts = Cmd_DeriveOptionFromSettings( Argc, pArgv );
        if ( pOpts == NULL )
            printf( "Cannot parse command line options...\n" );
        else
        {
            Vec_PtrPush( vOpts, pOpts );
            Vec_PtrPush( vOpts, Cmd_DeriveConvertIntoString(Argc, pArgv) );
            printf( "Adding settings %s\n", (char *)Vec_PtrEntryLast(vOpts) );
        }
        return;
    }
    vLine = Vec_WecEntry( vPars, iPar );
    // consider binary option
    if ( Vec_IntSize(vLine) == 2 )
    {
        Symb = Vec_IntEntry( vLine, 0 );
        Num = Vec_IntEntry( vLine, 1 );
        assert( Abc_Int2Float(Num) == -1.0 );
        // create one setting without this option
        Cmf_CreateOptions_rec( vPars, iPar+1, Argv, Argc, vOpts );
        // create another setting with this option
        sprintf( Argv[Argc], "-%c", Symb );
        Cmf_CreateOptions_rec( vPars, iPar+1, Argv, Argc+1, vOpts );
        return;
    }
    // consider numeric option
    Vec_IntForEachEntryDouble( vLine, Symb, Num, i )
    {
        float NumF = Abc_Int2Float(Num);
        // create setting with this option
        assert( NumF >= 0 );
        sprintf( Argv[Argc], "-%c", Symb );
        if ( NumF == (float)(int)NumF )
            sprintf( Argv[Argc+1], "%d", (int)NumF );
        else
            sprintf( Argv[Argc+1], "%.3f", NumF );
        Cmf_CreateOptions_rec( vPars, iPar+1, Argv, Argc+2, vOpts );
    }
}
Vec_Ptr_t * Cmf_CreateOptions( Vec_Wec_t * vPars )
{
    char Argv[CMD_AUTO_ARG_MAX][20];
    int Symb, Num, i, Argc = 0;
    Vec_Ptr_t * vOpts = Vec_PtrAlloc( 100 );
    Vec_Int_t * vLine = Vec_WecEntry( vPars, 0 );
    printf( "Creating all possible settings to be used by the autotuner:\n" );
    sprintf( Argv[Argc++], "autotuner" );
    Vec_IntForEachEntryDouble( vLine, Symb, Num, i )
    {
        float NumF = Abc_Int2Float(Num);
        sprintf( Argv[Argc++], "-%c", Symb );
        if ( NumF < 0.0 )
            continue;
        if ( NumF == (float)(int)NumF )
            sprintf( Argv[Argc++], "%d", (int)NumF );
        else
            sprintf( Argv[Argc++], "%.3f", NumF );
    }
    Cmf_CreateOptions_rec( vPars, 1, Argv, Argc, vOpts );
    printf( "Finished creating %d settings.\n\n", Vec_PtrSize(vOpts)/2 );
    return vOpts;
}


/**Function*************************************************************

  Synopsis    [Parses config file and derives AIGs listed in file list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cmf_IsSpace( char p )          { return p == ' ' || p == '\t' || p == '\n' ||  p == '\r'; }
static inline int Cmf_IsLowerCaseChar( char p )  { return p >= 'a' && p <= 'z';                             }
static inline int Cmf_IsUpperCaseChar( char p )  { return p >= 'A' && p <= 'Z';                             }
static inline int Cmf_IsDigit( char p )          { return (p >= '0' && p <= '9') || p == '.';               }

Vec_Wec_t * Cmd_ReadParamChoices( char * pConfig )
{
    Vec_Wec_t * vPars;
    Vec_Int_t * vLine;
    char * pThis, pBuffer[CMD_AUTO_LINE_MAX];
    FILE * pFile = fopen( pConfig, "rb" );
    if ( pFile == NULL )
        { printf( "File containing list of files \"%s\" cannot be opened.\n", pConfig ); return NULL; }
    vPars = Vec_WecAlloc( 100 );
    while ( fgets( pBuffer, CMD_AUTO_LINE_MAX, pFile ) != NULL )
    {
        // get the command from the file
        if ( Cmf_IsSpace(pBuffer[0]) || pBuffer[0] == '#')
            continue;
        // skip trailing spaces
        while ( Cmf_IsSpace(pBuffer[strlen(pBuffer)-1]) )
            pBuffer[strlen(pBuffer)-1] = 0;
        // read the line
        vLine = Vec_WecPushLevel( vPars );
        for ( pThis = pBuffer; *pThis;  )
        {
            if ( Cmf_IsLowerCaseChar(*pThis) )
            {
                Vec_IntPushTwo( vLine, (int)*pThis, Abc_Float2Int((float)-1.0) );
                pThis++;
                while ( Cmf_IsSpace(*pThis) )
                    pThis++;
                continue;
            }
            if ( Cmf_IsUpperCaseChar(*pThis) )
            {
                char Param = *pThis++;
                if ( !Cmf_IsDigit(*pThis) )
                    { printf( "Upper-case character (%c) should be followed by a number without space in line \"%s\".\n", Param, pBuffer ); return NULL; }
                Vec_IntPushTwo( vLine, (int)Param, Abc_Float2Int(atof(pThis)) );
                while ( Cmf_IsDigit(*pThis) )
                    pThis++;
                while ( Cmf_IsSpace(*pThis) )
                    pThis++;
                continue;
            }
            printf( "Expecting a leading lower-case or upper-case digit in line \"%s\".\n", pBuffer ); 
            return NULL;
        }
    }
    fclose( pFile );
    return vPars;
}
Vec_Ptr_t * Cmd_ReadFiles( char * pFileList )
{
    Gia_Man_t * pGia;
    Vec_Ptr_t * vAigs;
    char pBuffer[CMD_AUTO_LINE_MAX];
    FILE * pFile = fopen( pFileList, "rb" );
    if ( pFile == NULL )
        { printf( "File containing list of files \"%s\" cannot be opened.\n", pFileList ); return NULL; }
    vAigs = Vec_PtrAlloc( 100 );
    while ( fgets( pBuffer, CMD_AUTO_LINE_MAX, pFile ) != NULL )
    {
        // get the command from the file
        if ( Cmf_IsSpace(pBuffer[0]) || pBuffer[0] == '#')
            continue;
        // skip trailing spaces
        while ( Cmf_IsSpace(pBuffer[strlen(pBuffer)-1]) )
            pBuffer[strlen(pBuffer)-1] = 0;
        // read the file
        pGia = Gia_AigerRead( pBuffer, 0, 0, 0 );
        if ( pGia == NULL )
        {
            printf( "Cannot read AIG from file \"%s\".\n", pBuffer );
            continue;
        }
        Vec_PtrPush( vAigs, pGia );
    }
    fclose( pFile );
    return vAigs;
}

/**Function*************************************************************

  Synopsis    [Autotuner for SAT solver "satoko".]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cmd_RunAutoTuner( char * pConfig, char * pFileList, int nCores )
{
    abctime clk = Abc_Clock();
    Vec_Wec_t * vPars = Cmd_ReadParamChoices( pConfig );
    Vec_Ptr_t * vAigs = Cmd_ReadFiles( pFileList );
    Vec_Ptr_t * vOpts = vPars ? Cmf_CreateOptions( vPars ) : NULL;
    int i; char * pString, * pStringBest = NULL;
    satoko_opts_t * pOpts, * pOptsBest = NULL;
    int Result, ResultBest = 0x7FFFFFFF;
    Gia_Man_t * pGia; 
    // iterate through all possible option settings
    if ( vAigs && vOpts )
    {
        Vec_PtrForEachEntryDouble( satoko_opts_t *, char *, vOpts, pOpts, pString, i )
        {
            abctime clk = Abc_Clock();
            printf( "Evaluating settings: %20s...  \n", pString );
            Result = Cmd_RunAutoTunerEval( vAigs, pOpts, nCores );
            printf( "Cost = %6d.  ", Result );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            if ( ResultBest > Result )
            {
                ResultBest = Result;
                pStringBest = pString;
                pOptsBest = pOpts;
            }
        }
        printf( "The best settings are: %20s    \n", pStringBest );
        printf( "Best cost = %6d.  ", ResultBest );
        Abc_PrintTime( 1, "Total time", Abc_Clock() - clk );
    }
    // cleanup
    if ( vPars ) Vec_WecFree( vPars );
    if ( vOpts ) Vec_PtrFreeFree( vOpts );
    if ( vAigs )
    {
        Vec_PtrForEachEntry( Gia_Man_t *, vAigs, pGia, i )
            Gia_ManStop( pGia );
        Vec_PtrFree( vAigs );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

