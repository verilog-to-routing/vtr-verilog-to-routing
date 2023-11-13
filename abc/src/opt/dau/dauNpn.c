/**CFile****************************************************************

  FileName    [dau.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dau.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "misc/util/utilTruth.h"
#include "misc/extra/extra.h"
#include "bool/lucky/lucky.h"

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
void Dau_TruthEnum(int nVars)
{
    int fUseTable = 1;
    abctime clk = Abc_Clock();
    int nSizeLog = (1<<nVars) -2;
    int nSizeW = 1 << nSizeLog;
    int nPerms  = Extra_Factorial( nVars );
    int nMints  = 1 << nVars;
    int * pPerm = Extra_PermSchedule( nVars );
    int * pComp = Extra_GreyCodeSchedule( nVars );
    word nFuncs = ((word)1 << (((word)1 << nVars)-1));
    word * pPres = ABC_CALLOC( word, 1 << ((1<<nVars)-7) );
    unsigned * pTable = fUseTable ? (unsigned *)ABC_CALLOC(word, nSizeW) : NULL;
    Vec_Int_t * vNpns = Vec_IntAlloc( 1000 );
    word tMask = Abc_Tt6Mask( 1 << nVars );
    word tTemp, tCur;
    int i, k;
    if ( pPres == NULL )
    {
        printf( "Cannot alloc memory for marks.\n" );
        return;
    }
    if ( pTable == NULL )
        printf( "Cannot alloc memory for table.\n" );
    for ( tCur = 0; tCur < nFuncs; tCur++ )
    {
        if ( (tCur & 0x3FFFF) == 0 )
        {
            printf( "Finished %08x.  Classes = %6d.  ", (int)tCur, Vec_IntSize(vNpns) );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            fflush(stdout);
        }
        if ( Abc_TtGetBit(pPres, (int)tCur) )
            continue;
        //Extra_PrintBinary( stdout, (unsigned *)&tCur, 16 ); printf( " %04x\n", (int)tCur );
        //Dau_DsdPrintFromTruth( &tCur, 4 ); printf( "\n" );
        Vec_IntPush( vNpns, (int)tCur );
        tTemp = tCur;
        for ( i = 0; i < nPerms; i++ )
        {
            for ( k = 0; k < nMints; k++ )
            {
                if ( tCur < nFuncs )
                {
                    if ( pTable ) pTable[(int)tCur] = tTemp;
                    Abc_TtSetBit( pPres, (int)tCur );
                }
                if ( (tMask & ~tCur) < nFuncs )
                {
                    if ( pTable ) pTable[(int)(tMask & ~tCur)] = tTemp;
                    Abc_TtSetBit( pPres, (int)(tMask & ~tCur) );
                }
                tCur = Abc_Tt6Flip( tCur, pComp[k] );
            }
            tCur = Abc_Tt6SwapAdjacent( tCur, pPerm[i] );
        }
        assert( tTemp == tCur );
    }
    printf( "Computed %d NPN classes of %d variables.  ", Vec_IntSize(vNpns), nVars );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    fflush(stdout);
    Vec_IntFree( vNpns );
    ABC_FREE( pPres );
    ABC_FREE( pPerm );
    ABC_FREE( pComp );
    // write into file
    if ( pTable )
    {
        FILE * pFile;
        int RetValue;
        char pFileName[200];
        sprintf( pFileName, "tableW%d.data", nSizeLog );
        pFile = fopen( pFileName, "wb" );
        RetValue = fwrite( pTable, 8, nSizeW, pFile );
        RetValue = 0;
        fclose( pFile );
        ABC_FREE( pTable );
    }
}

 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Dau_ReadFile( char * pFileName, int nSizeW )
{
    abctime clk = Abc_Clock();
    FILE * pFile = fopen( pFileName, "rb" );
    unsigned * p = (unsigned *)ABC_CALLOC(word, nSizeW);
    int RetValue = pFile ? fread( p, sizeof(word), nSizeW, pFile ) : 0;
    RetValue = 0;
    if ( pFile )
    {
        printf( "Finished reading file \"%s\".\n", pFileName );
        fclose( pFile );
    }
    else
        printf( "Cannot open input file \"%s\".\n", pFileName );
    Abc_PrintTime( 1, "File reading", Abc_Clock() - clk );
    return p;
}
int Dau_AddFunction( word tCur, int nVars, unsigned * pTable, Vec_Int_t * vNpns, Vec_Int_t * vNpns_ )
{
    int Digit  = (1 << nVars)-1;
    word tMask = Abc_Tt6Mask( 1 << nVars );
    word tNorm = (tCur >> Digit) & 1 ? ~tCur : tCur;
    unsigned t = (unsigned)(tNorm & tMask);
    unsigned tRep = pTable[t] & 0x7FFFFFFF;
    unsigned tRep2 = pTable[tRep];
    assert( ((tNorm >> Digit) & 1) == 0 );
    if ( (tRep2 >> 31) == 0 ) // first time
    {
        Vec_IntPush( vNpns, tRep2 );
        if ( Abc_TtSupportSize(&tCur, nVars) < nVars )
            Vec_IntPush( vNpns_, tRep2 );
        pTable[tRep] = tRep2 | (1 << 31);
        return tRep2;
    }
    return 0;
}
void Dau_NetworkEnum(int nVars)
{
    abctime clk = Abc_Clock();
    int Limit = 2;
    int UseTwo = 0;
    int nSizeLog = (1<<nVars) -2;
    int nSizeW = 1 << nSizeLog;
    char pFileName[200];
    unsigned * pTable;
    Vec_Wec_t * vNpns  = Vec_WecStart( 32 );
    Vec_Wec_t * vNpns_ = Vec_WecStart( 32 );
    int i, v, u, g, k, m, n, Res, Entry;
    unsigned Inv = (unsigned)Abc_Tt6Mask(1 << (nVars-1));
    sprintf( pFileName, "tableW%d.data", nSizeLog );
    pTable  = Dau_ReadFile( pFileName, nSizeW );
    // create constant function and buffer/inverter function
    pTable[0]   |= (1 << 31);
    pTable[Inv] |= (1 << 31);
    Vec_IntPushTwo( Vec_WecEntry(vNpns,  0), 0, Inv );
    Vec_IntPushTwo( Vec_WecEntry(vNpns_, 0), 0, Inv );
    printf("Nodes = %2d.   New = %6d. Total = %6d.   New = %6d. Total = %6d.  ", 
        0, Vec_IntSize(Vec_WecEntry(vNpns,  0)), Vec_WecSizeSize(vNpns), 
           Vec_IntSize(Vec_WecEntry(vNpns_, 0)), Vec_WecSizeSize(vNpns_) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // numerate other functions based on how many nodes they have
    for ( n = 1; n < 32; n++ )
    {
        Vec_Int_t * vFuncsN2 = n > 1 ? Vec_WecEntry( vNpns, n-2 ) : NULL;
        Vec_Int_t * vFuncsN1 = Vec_WecEntry( vNpns, n-1 );
        Vec_Int_t * vFuncsN  = Vec_WecEntry( vNpns, n   );
        Vec_Int_t * vFuncsN_ = Vec_WecEntry( vNpns_,n   );
        Vec_IntForEachEntry( vFuncsN1, Entry, i )
        {
            word uTruth = (((word)Entry) << 32) | (word)Entry;
            int nSupp = Abc_TtSupportSize( &uTruth, nVars );
            assert( nSupp == 6 || !Abc_Tt6HasVar(uTruth, nVars-1-nSupp) );
            //printf( "Exploring function %4d with %d vars: ", i, nSupp );
            //printf( " %04x\n", (int)uTruth );
            //Dau_DsdPrintFromTruth( &uTruth, 4 );
            for ( v = 0; v < nSupp; v++ )
            {
                word tGate, tCur;
                word Cof0 = Abc_Tt6Cofactor0( uTruth, nVars-1-v );
                word Cof1 = Abc_Tt6Cofactor1( uTruth, nVars-1-v );
                for ( g = 0; g < Limit; g++ )
                {
                    if ( nSupp < nVars )
                    {
                        if ( g == 0 )
                        {
                            tGate = s_Truths6[nVars-1-v] & s_Truths6[nVars-1-nSupp];
                            tCur  = (tGate & Cof1) | (~tGate & Cof0);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tCur  = (tGate & Cof0) | (~tGate & Cof1);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );
                        }
                        else
                        {
                            tGate = s_Truths6[nVars-1-v] ^ s_Truths6[nVars-1-nSupp];
                            tCur  = (tGate & Cof1) | (~tGate & Cof0);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );
                        }
                    }
                }
                for ( g = 0; g < Limit; g++ )
                {
                    // add one cross bar
                    for ( k = 0; k < nSupp; k++ ) if ( k != v )
                    {
                        if ( g == 0 )
                        {
                            tGate = s_Truths6[nVars-1-v] & s_Truths6[nVars-1-k];
                            tCur  = (tGate & Cof1) | (~tGate & Cof0);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tCur  = (tGate & Cof0) | (~tGate & Cof1);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tGate = s_Truths6[nVars-1-v] & ~s_Truths6[nVars-1-k];
                            tCur  = (tGate & Cof1) | (~tGate & Cof0);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tCur  = (tGate & Cof0) | (~tGate & Cof1);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );
                        }
                        else
                        {
                            tGate = s_Truths6[nVars-1-v] ^ s_Truths6[nVars-1-k];
                            tCur  = (tGate & Cof1) | (~tGate & Cof0);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );
                        }
                    }
                }
                for ( g = 0; g < Limit; g++ )
                {
                    // add two cross bars
                    for ( k = 0;   k < nSupp; k++ ) if ( k != v )
                    for ( m = k+1; m < nSupp; m++ ) if ( m != v )
                    {
                        if ( g == 0 )
                        {
                            tGate = s_Truths6[nVars-1-m] & s_Truths6[nVars-1-k];
                            tCur  = (tGate & Cof1) | (~tGate & Cof0);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tCur  = (tGate & Cof0) | (~tGate & Cof1);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tGate = s_Truths6[nVars-1-m] & ~s_Truths6[nVars-1-k];
                            tCur  = (tGate & Cof1) | (~tGate & Cof0);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tCur  = (tGate & Cof0) | (~tGate & Cof1);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tGate = ~s_Truths6[nVars-1-m] & s_Truths6[nVars-1-k];
                            tCur  = (tGate & Cof1) | (~tGate & Cof0);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tCur  = (tGate & Cof0) | (~tGate & Cof1);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tGate = ~s_Truths6[nVars-1-m] & ~s_Truths6[nVars-1-k];
                            tCur  = (tGate & Cof1) | (~tGate & Cof0);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );

                            tCur  = (tGate & Cof0) | (~tGate & Cof1);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );
                        }
                        else
                        {
                            tGate = s_Truths6[nVars-1-m] ^ s_Truths6[nVars-1-k];
                            tCur  = (tGate & Cof1) | (~tGate & Cof0);
                            Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );
                        }
                    }
                }
            }
        }
        if ( UseTwo && vFuncsN2 )
        Vec_IntForEachEntry( vFuncsN2, Entry, i )
        {
            word uTruth = (((word)Entry) << 32) | (word)Entry;
            int nSupp = Abc_TtSupportSize( &uTruth, nVars );
            assert( nSupp == 6 || !Abc_Tt6HasVar(uTruth, nVars-1-nSupp) );
            //printf( "Exploring function %4d with %d vars: ", i, nSupp );
            //printf( " %04x\n", (int)uTruth );
            //Dau_DsdPrintFromTruth( &uTruth, 4 );
            for ( v = 0; v < nSupp; v++ )
//            for ( u = v+1; u < nSupp; u++ ) if ( u != v )
            for ( u = 0; u < nSupp; u++ ) if ( u != v )
            {
                word tGate1, tGate2, tCur;
                word Cof0 = Abc_Tt6Cofactor0( uTruth, nVars-1-v );
                word Cof1 = Abc_Tt6Cofactor1( uTruth, nVars-1-v );

                word Cof00 = Abc_Tt6Cofactor0( Cof0, nVars-1-u );
                word Cof01 = Abc_Tt6Cofactor1( Cof0, nVars-1-u );
                word Cof10 = Abc_Tt6Cofactor0( Cof1, nVars-1-u );
                word Cof11 = Abc_Tt6Cofactor1( Cof1, nVars-1-u );

                tGate1 = s_Truths6[nVars-1-v] & s_Truths6[nVars-1-u];
                tGate2 = s_Truths6[nVars-1-v] ^ s_Truths6[nVars-1-u];

                Cof0  = (tGate2 & Cof00) | (~tGate2 & Cof01);
                Cof1  = (tGate2 & Cof10) | (~tGate2 & Cof11);

                tCur  = (tGate1 & Cof1)  | (~tGate1 & Cof0);
                Res = Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );
                if ( Res )
                    printf( "Found function %d\n", Res );

                tCur  = (tGate1 & Cof0)  | (~tGate1 & Cof1);
                Res = Dau_AddFunction( tCur, nVars, pTable, vFuncsN, vFuncsN_ );
                if ( Res )
                    printf( "Found function %d\n", Res );
            }
        }
        printf("Nodes = %2d.   New = %6d. Total = %6d.   New = %6d. Total = %6d.  ", 
            n, Vec_IntSize(vFuncsN), Vec_WecSizeSize(vNpns), Vec_IntSize(vFuncsN_), Vec_WecSizeSize(vNpns_) );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        fflush(stdout);
        if ( Vec_IntSize(vFuncsN) == 0 )
            break;
    }
//    printf( "Functions with 7 nodes:\n" );
//    Vec_IntForEachEntry( Vec_WecEntry(vNpns_,7), Entry, i )
//        printf( "%04x ", Entry );
//    printf( "\n" );

    Vec_WecFree( vNpns );
    Vec_WecFree( vNpns_ );
    ABC_FREE( pTable );
    Abc_PrintTime( 1, "Total time", Abc_Clock() - clk );
    fflush(stdout);
}
void Dau_NetworkEnumTest()
{
    //Dau_TruthEnum(3);
    Dau_NetworkEnum(4);
}



/**Function*************************************************************

  Synopsis    [Count the number of symmetric pairs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_CountSymms( word t, int nVars )
{
    word Cof0, Cof1;
    int i, j, nPairs = 0;
    for ( i = 0; i < nVars; i++ )
    for ( j = i+1; j < nVars; j++ )
        nPairs += Abc_TtVarsAreSymmetric(&t, nVars, i, j, &Cof0, &Cof1);
    return nPairs;
}
int Dau_CountSymms2( word t, int nVars )
{
    word Cof0, Cof1;
    int i, j, SymVars = 0;
    for ( i = 0; i < nVars; i++ )
    for ( j = i+1; j < nVars; j++ )
        if ( Abc_TtVarsAreSymmetric(&t, nVars, i, j, &Cof0, &Cof1) )
            SymVars |= (1 << j);
    return SymVars;
}
int Dau_CountCompl1( word t, int v, int nVars )
{
    word tNew = Abc_Tt6Flip(t, v);
    int k;
    if ( tNew == ~t )
        return 1;
    for ( k = 0; k < nVars; k++ ) if ( k != v )
        if ( tNew == Abc_Tt6Flip(t, k) )
            return 1;
    return 0;
}
int Dau_CountCompl( word t, int nVars )
{
    int i, nPairs = 0;
    for ( i = 0; i < nVars; i++ )
        nPairs += Dau_CountCompl1(t, i, nVars);
    return nPairs;
}

/**Function*************************************************************

  Synopsis    [Performs exact canonicization of semi-canonical classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Dau_ExactNpnForClasses( Vec_Mem_t * vTtMem, Vec_Int_t * vNodSup, int nVars, int nInputs )
{
    Vec_Wrd_t * vCanons = Vec_WrdStart( Vec_IntSize(vNodSup) );
    word pAuxWord[1024], pAuxWord1[1024];
    word uTruth; int i, Entry;
    permInfo * pi = setPermInfoPtr(nVars);
    Vec_IntForEachEntry( vNodSup, Entry, i )
    {
        if ( (Entry & 0xF) > nVars )
            continue;
        uTruth = *Vec_MemReadEntry( vTtMem, i );
        simpleMinimal(&uTruth, pAuxWord, pAuxWord1, pi, nVars);
        Vec_WrdWriteEntry( vCanons, i, uTruth );
    }
    freePermInfoPtr(pi);
    return vCanons;
}
void Dau_ExactNpnPrint( Vec_Mem_t * vTtMem, Vec_Int_t * vNodSup, int nVars, int nInputs, int nNodesMax )
{
    abctime clk = Abc_Clock(); int n, nTotal = 0;
    Vec_Wrd_t * vCanons = Dau_ExactNpnForClasses( vTtMem, vNodSup, nVars, nInputs );
    Vec_Mem_t * vTtMem2  = Vec_MemAlloc( Vec_MemEntrySize(vTtMem), 10 );
    Vec_MemHashAlloc( vTtMem2, 1<<10 );
    Abc_PrintTime( 1, "Exact NPN computation time", Abc_Clock() - clk );
    printf( "Final results:\n" );
    for ( n = 0; n <= nNodesMax; n++ )
    {
        int i, Entry, Entry2, nEntries2, Counter = 0, Counter2 = 0;
        Vec_IntForEachEntry( vNodSup, Entry, i )
        {
            if ( (Entry & 0xF) > nVars || (Entry >> 16) != n )
                continue;
            Counter++;
            nEntries2 = Vec_MemEntryNum(vTtMem2);
            Entry2 = Vec_MemHashInsert( vTtMem2, Vec_WrdEntryP(vCanons, i) );
            if ( nEntries2 == Vec_MemEntryNum(vTtMem2) ) // found in the table - not new
                continue;
            Counter2++;
        }
        nTotal += Counter2;
        printf( "Nodes = %2d.  ", n );
        printf( "Semi-canonical = %8d.  ", Counter );
        printf( "Canonical = %8d.  ", Counter2 );
        printf( "Total = %8d.", nTotal );
        printf( "\n" );
    }
    Vec_MemHashFree( vTtMem2 );
    Vec_MemFreeP( &vTtMem2 );
    Vec_WrdFree( vCanons );
    fflush(stdout);
}

/**Function*************************************************************

  Synopsis    [Saving hash tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_TablesSave( int nInputs, int nVars, Vec_Mem_t * vTtMem, Vec_Int_t * vNodSup, int nFronts, abctime clk )
{
    FILE * pFile; 
    char FileName[100];
    int i, nWords = Abc_TtWordNum(nInputs);
    // NPN classes
    sprintf( FileName, "npn%d%d.ttd", nInputs, nVars );
    pFile = fopen( FileName, "wb" );
    for ( i = 0; i < Vec_MemEntryNum(vTtMem); i++ )
        fwrite( Vec_MemReadEntry(vTtMem, i), 8, nWords, pFile );
    fwrite( Vec_IntArray(vNodSup), 4, Vec_IntSize(vNodSup), pFile );
    fclose( pFile );
//    printf( "Dumped files with %10d classes after exploring %10d frontiers.\n", 
//        Vec_IntSize(vNodSup), nFronts );
    printf( "Dumped file \"%s\" with %10d classes after exploring %10d frontiers.  ", 
        FileName, Vec_IntSize(vNodSup), nFronts );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    fflush(stdout);
}

/**Function*************************************************************

  Synopsis    [Dump functions by the number of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DumpFuncs( Vec_Mem_t * vTtMem, Vec_Int_t * vNodSup, int nVars, int nMax )
{
    FILE * pFile[20];
    int Counters[20] = {0};
    int n, i;
    assert( nVars == 4 || nVars == 5 );
    for ( n = 0; n <= nMax; n++ )
    {
        char FileName[100];
        sprintf( FileName, "func%d_min%d.tt", nVars, n );
        pFile[n] = fopen( FileName, "wb" );
    }
    for ( i = 0; i < Vec_MemEntryNum(vTtMem); i++ )
    {
        word * pTruth = Vec_MemReadEntry( vTtMem, i );
        int NodSup = Vec_IntEntry( vNodSup, i );
        if ( (NodSup & 0xF) != nVars )
            continue;
        Counters[NodSup >> 16]++;
        if ( nVars == 4 )
            fprintf( pFile[NodSup >> 16], "%04x\n", (int)(0xFFFF & pTruth[0]) );
        else if ( nVars == 5 )
            fprintf( pFile[NodSup >> 16], "%08x\n", (int)(0xFFFFFFFF & pTruth[0]) );
    }
    for ( n = 0; n <= nMax; n++ )
    {
        printf( "Dumped %8d  %d-node %d-input functions into file.\n", Counters[n], n, nVars );
        fclose( pFile[n] );
    }
}

/**Function*************************************************************

  Synopsis    [Function enumeration.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_CountFuncs( Vec_Int_t * vNodSup, int iStart, int iStop, int nVars )
{
    int i, Entry, Count = 0;
    Vec_IntForEachEntryStartStop( vNodSup, Entry, i, iStart, iStop )
        Count += ((Entry & 0xF) <= nVars);
    return Count;
}
int Dau_PrintStats( int nNodes, int nInputs, int nVars, Vec_Int_t * vNodSup, int iStart, int iStop, word nSteps, int Count2, abctime clk )
{
    int nNew;
    printf("N =%2d | ",      nNodes );
    printf("C =%12.0f  ",    (double)(iword)nSteps );
    printf("New%d =%10d  ",  nInputs, iStop-iStart );
    printf("All%d =%10d | ", nInputs, iStop );
    printf("New%d =%8d  ",   nVars, nNew = Dau_CountFuncs(vNodSup, iStart, iStop, nVars) );
    printf("All%d =%8d  ",   nVars,        Dau_CountFuncs(vNodSup,      0, iStop, nVars) );
    printf("Two =%6d ",      Count2 );
    //Abc_PrintTime( 1, "T",   Abc_Clock() - clk );
    Abc_Print(1, "%9.2f sec\n", 1.0*(Abc_Clock() - clk)/(CLOCKS_PER_SEC));
    fflush(stdout);
    return nNew;
}


int Dau_InsertFunction( Abc_TtHieMan_t * pMan, word * pCur, int nNodes, int nInputs, int nVars0, int nVars, 
    Vec_Mem_t * vTtMem, Vec_Int_t * vNodSup, int nFronts, abctime clk )
{
    int DumpDelta = 1000000;
    char Perm[16] = {0};
    int nVarsNew = Abc_TtMinBase( pCur, NULL, nVars, nInputs );
    //unsigned Phase = Abc_TtCanonicizeHie( pMan, pCur, nVarsNew, Perm, 1 );
    unsigned Phase = Abc_TtCanonicizeWrap( Abc_TtCanonicizeAda, pMan, pCur, nVarsNew, Perm, 99 );
    int nEntries = Vec_MemEntryNum(vTtMem);
    int Entry = Vec_MemHashInsert( vTtMem, pCur );
    if ( nEntries == Vec_MemEntryNum(vTtMem) ) // found in the table - not new
        return 0;
    Entry = 0;
    Phase = 0;
    // this is a new class
    Vec_IntPush( vNodSup, (nNodes << 16) | nVarsNew );
    assert( Vec_MemEntryNum(vTtMem) == Vec_IntSize(vNodSup) );
    if ( Vec_IntSize(vNodSup) % DumpDelta == 0 )
        Dau_TablesSave( nInputs, nVars0, vTtMem, vNodSup, nFronts, clk );
    return 1;
}
void Dau_FunctionEnum( int nInputs, int nVars, int nNodeMax, int fUseTwo, int fReduce, int fVerbose )
{
    abctime clk = Abc_Clock();
    int nWords = Abc_TtWordNum(nInputs); word nSteps = 0;
    Abc_TtHieMan_t * pMan = Abc_TtHieManStart( nInputs, 5 );
    Vec_Mem_t * vTtMem  = Vec_MemAlloc( nWords, 16 );
    Vec_Int_t * vNodSup = Vec_IntAlloc( 1 << 16 );
    int v, u, k, m, n, Entry, nNew, Limit[32] = {1, 2};
    word Truth[4] = {0};
    assert( nVars >= 3 && nVars <= nInputs && nInputs <= 6 );
    Vec_MemHashAlloc( vTtMem,  1<<16 );
    // add constant 0
    Vec_MemHashInsert( vTtMem,  Truth );
    Vec_IntPush( vNodSup, 0 ); // nodes=0, supp=0
    // add buffer/inverter
    Abc_TtIthVar( Truth, 0, nInputs );
    Abc_TtNot( Truth, nWords );
    Vec_MemHashInsert( vTtMem,  Truth );
    Vec_IntPush( vNodSup, 1 ); // nodes=0, supp=1
    Dau_PrintStats( 0, nInputs, nVars, vNodSup, 0, 2, nSteps, 0, clk );
    // numerate other functions based on how many nodes they have
    for ( n = 1; n <= nNodeMax; n++ )
    {
        int Count2 = 0;
        int fExpand = !(fReduce && n == nNodeMax);
        for ( Entry = Limit[n-1]; Entry < Limit[n]; Entry++ )
        {
            word * pTruth = Vec_MemReadEntry( vTtMem, Entry );
            int NodSup = Vec_IntEntry(vNodSup, Entry);
            int nSupp = 0xF & NodSup;
            int SymVars = Dau_CountSymms2( pTruth[0], nSupp );
            assert( n-1 == (NodSup >> 16) );
            assert( !Abc_Tt6HasVar(*pTruth, nSupp) );
            //printf( "Exploring function %4d with %d vars: ", i, nSupp );
            //printf( " %04x\n", (int)uTruth );
            //Dau_DsdPrintFromTruth( &uTruth, 4 );
            for ( v = 0; v < nSupp; v++ ) if ( (SymVars & (1 << v)) == 0 )
            {
                word tGate, tCur;
                word Cof0 = Abc_Tt6Cofactor0( *pTruth, v );
                word Cof1 = Abc_Tt6Cofactor1( *pTruth, v );
                // add one extra variable to support
                if ( nSupp < nInputs && fExpand )
                {
                    tGate = s_Truths6[v] & s_Truths6[nSupp];
                    tCur  = (tGate & Cof1) | (~tGate & Cof0);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp+1, vTtMem, vNodSup, Entry, clk );

                    tCur  = (tGate & Cof0) | (~tGate & Cof1);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp+1, vTtMem, vNodSup, Entry, clk );


                    tGate = s_Truths6[v] ^ s_Truths6[nSupp];
                    tCur  = (tGate & Cof1) | (~tGate & Cof0);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp+1, vTtMem, vNodSup, Entry, clk );

                    nSteps += 3;
                }
                // add one cross bar
                if ( fExpand )
                for ( k = 0; k < nSupp; k++ ) if ( k != v && ((SymVars & (1 << k)) == 0 || k == v+1) )
                {
                    tGate = s_Truths6[v] & s_Truths6[k];
                    tCur  = (tGate & Cof1) | (~tGate & Cof0);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tCur  = (tGate & Cof0) | (~tGate & Cof1);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tGate = s_Truths6[v] & ~s_Truths6[k];
                    tCur  = (tGate & Cof1) | (~tGate & Cof0);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tCur  = (tGate & Cof0) | (~tGate & Cof1);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );


                    tGate = s_Truths6[v] ^ s_Truths6[k];
                    tCur  = (tGate & Cof1) | (~tGate & Cof0);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    nSteps += 5;
                }
                // add two cross bars
                for ( k = 0;   k < nSupp; k++ ) if ( k != v )//&& ((SymVars & (1 << k)) == 0) )
                for ( m = k+1; m < nSupp; m++ ) if ( m != v )//&& ((SymVars & (1 << m)) == 0 || m == k+1) )
                {
                    tGate = s_Truths6[m] & s_Truths6[k];
                    tCur  = (tGate & Cof1) | (~tGate & Cof0);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tCur  = (tGate & Cof0) | (~tGate & Cof1);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tGate = s_Truths6[m] & ~s_Truths6[k];
                    tCur  = (tGate & Cof1) | (~tGate & Cof0);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tCur  = (tGate & Cof0) | (~tGate & Cof1);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tGate = ~s_Truths6[m] & s_Truths6[k];
                    tCur  = (tGate & Cof1) | (~tGate & Cof0);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tCur  = (tGate & Cof0) | (~tGate & Cof1);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tGate = ~s_Truths6[m] & ~s_Truths6[k];
                    tCur  = (tGate & Cof1) | (~tGate & Cof0);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tCur  = (tGate & Cof0) | (~tGate & Cof1);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    
                    tGate = s_Truths6[m] ^ s_Truths6[k];
                    tCur  = (tGate & Cof1) | (~tGate & Cof0);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    tGate = s_Truths6[m] ^ s_Truths6[k];
                    tCur  = (tGate & Cof0) | (~tGate & Cof1);
                    Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );

                    nSteps += 10;
                }
            }
        } 
        if ( fUseTwo && n > 2 && fExpand )
        for ( Entry = Limit[n-2]; Entry < Limit[n-1]; Entry++ )
        {
            word * pTruth = Vec_MemReadEntry( vTtMem, Entry );
            int NodSup = Vec_IntEntry(vNodSup, Entry);
            int nSupp = 0xF & NodSup; int g1, g2;
            assert( n-2 == (NodSup >> 16) );
            assert( !Abc_Tt6HasVar(*pTruth, nSupp) );
            for ( v = 0; v < nSupp; v++ )
            for ( u = 0; u < nSupp; u++ ) if ( u != v )
            {
                word Cof0 = Abc_Tt6Cofactor0( *pTruth, v );
                word Cof1 = Abc_Tt6Cofactor1( *pTruth, v );

                word Cof00 = Abc_Tt6Cofactor0( Cof0, u );
                word Cof01 = Abc_Tt6Cofactor1( Cof0, u );
                word Cof10 = Abc_Tt6Cofactor0( Cof1, u );
                word Cof11 = Abc_Tt6Cofactor1( Cof1, u );

                word tGates[5], tCur;
                tGates[0] =  s_Truths6[v] &  s_Truths6[u];
                tGates[1] =  s_Truths6[v] & ~s_Truths6[u];
                tGates[2] = ~s_Truths6[v] &  s_Truths6[u];
                tGates[3] =  s_Truths6[v] |  s_Truths6[u];
                tGates[4] =  s_Truths6[v] ^  s_Truths6[u];

                for ( g1 = 0;    g1 < 5; g1++ )
                for ( g2 = g1+1; g2 < 5; g2++ )
                {
                    Cof0  = (tGates[g1] & Cof01) | (~tGates[g1] & Cof00);
                    Cof1  = (tGates[g1] & Cof11) | (~tGates[g1] & Cof10);

                    tCur  = (tGates[g2] & Cof1)  | (~tGates[g2] & Cof0);
                    Count2 += Dau_InsertFunction( pMan, &tCur, n, nInputs, nVars, nSupp, vTtMem, vNodSup, Entry, clk );
                }
            }
        }
        Limit[n+1] = Vec_IntSize(vNodSup);
        nNew = Dau_PrintStats( n, nInputs, nVars, vNodSup, Limit[n], Limit[n+1], nSteps, Count2, clk );
        if ( nNew == 0 )
            break;
    }
    Dau_TablesSave( nInputs, nVars, vTtMem, vNodSup, Vec_IntSize(vNodSup), clk );
    Abc_PrintTime( 1, "Total time", Abc_Clock() - clk );
    //Dau_DumpFuncs( vTtMem, vNodSup, nVars, nNodeMax );
    //Dau_ExactNpnPrint( vTtMem, vNodSup, nVars, nInputs, n );
    Abc_TtHieManStop( pMan );
    Vec_MemHashFree( vTtMem );
    Vec_MemFreeP( &vTtMem );
    Vec_IntFree( vNodSup );
    fflush(stdout);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

