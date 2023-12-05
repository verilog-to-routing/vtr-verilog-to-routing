/**CFile****************************************************************

  FileName    [giaGen.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaGen.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/util/utilTruth.h"
#include "misc/extra/extra.h"

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
Gia_Man_t * Gia_DeriveAig( Vec_Wrd_t * vSims, Vec_Str_t * vSimsOut )
{
    int nInputs = 32*32*24;
    int nWords  = nInputs/64;
    int nExamps = 64;
    int i, e, iLitOut[10] = {0};
    Gia_Man_t * pNew;
    assert( Vec_WrdSize(vSims) % nInputs == 0 );
    pNew = Gia_ManStart( nInputs * nExamps + 10000 );
    for ( i = 0; i < nInputs; i++ )
        Gia_ManAppendCi( pNew );
    Gia_ManHashStart( pNew );
    for ( e = 0; e < nExamps; e++ )
    {
        int Class = Vec_StrEntry( vSimsOut, e );
        int This = 1;
        word * pSim = Vec_WrdEntryP( vSims, e*nWords );
        for ( i = 0; i < nInputs; i++ )
            This = Gia_ManHashAnd( pNew, This, Abc_Var2Lit(i+1, !Abc_TtGetBit(pSim, i)) );
        assert( Class >= 0 && Class <= 9 );
        iLitOut[Class] = Gia_ManHashOr( pNew, iLitOut[Class], This );
        //printf( "Finished example %d\n", e );
    }
    for ( i = 0; i < 10; i++ )
        Gia_ManAppendCo( pNew, iLitOut[i] );
    //pNew = Gia_ManCleanup( pTemp = pNew );
    //Gia_ManStop( pTemp );
    return pNew;
}
void Gia_DeriveAigTest()
{
    extern int Gia_ManReadCifar10File( char * pFileName, Vec_Wrd_t ** pvSimsIn, Vec_Str_t ** pvSimsOut, int * pnExamples );
    char pFileName[100] = "test";
    Vec_Wrd_t * vSimsIn;
    Vec_Str_t * vSimsOut;
    int nExamples = 0;
    int nInputs = Gia_ManReadCifar10File( pFileName, &vSimsIn, &vSimsOut, &nExamples );
    Gia_Man_t * pThis = Gia_DeriveAig( vSimsIn, vSimsOut );
    Gia_AigerWrite( pThis, "examples64.aig", 0, 0, 0 );
    printf( "Dumped file \"%s\".\n", "examples64.aig" );
    Gia_ManStop( pThis );
    Vec_WrdFree( vSimsIn );
    Vec_StrFree( vSimsOut );
    nInputs = 0;
}


/**Function*************************************************************

  Synopsis    [Populate internal simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Gia_ManObjSim( Gia_Man_t * p, int iObj )
{
    return Vec_WrdEntryP( p->vSims, p->nSimWords * iObj );
}
static inline void Gia_ManObjSimPi( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSim = Gia_ManObjSim( p, iObj );
    for ( w = 0; w < p->nSimWords; w++ )
        pSim[w] = Gia_ManRandomW( 0 );
//    pSim[0] <<= 1;
}
static inline void Gia_ManObjSimPo( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSimCo  = Gia_ManObjSim( p, iObj );
    word * pSimDri = Gia_ManObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] = ~pSimDri[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] =  pSimDri[w];
}
static inline void Gia_ManObjSimAnd( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSim  = Gia_ManObjSim( p, iObj );
    word * pSim0 = Gia_ManObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    word * pSim1 = Gia_ManObjSim( p, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & ~pSim1[w];
    else if ( Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & pSim1[w];
    else if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & ~pSim1[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & pSim1[w];
}
int Gia_ManSimulateWords( Gia_Man_t * p, int nWords )
{
    Gia_Obj_t * pObj; int i;
    // allocate simulation info for one timeframe
    Vec_WrdFreeP( &p->vSims );
    p->vSims = Vec_WrdStart( Gia_ManObjNum(p) * nWords );
    p->nSimWords = nWords;
    // perform simulation
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Gia_ManObjSimAnd( p, i );
        else if ( Gia_ObjIsCi(pObj) )
            Gia_ManObjSimPi( p, i );
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ManObjSimPo( p, i );
        else assert( 0 );
    }
    return 1;
}

int Gia_ManSimulateWordsInit( Gia_Man_t * p, Vec_Wrd_t * vSimsIn )
{
    Gia_Obj_t * pObj; int i, Id;
    int nWords = Vec_WrdSize(vSimsIn) / Gia_ManCiNum(p);
    assert( Vec_WrdSize(vSimsIn) == nWords * Gia_ManCiNum(p) );
    // allocate simulation info for one timeframe
    Vec_WrdFreeP( &p->vSims );
    p->vSims = Vec_WrdStart( Gia_ManObjNum(p) * nWords );
    p->nSimWords = nWords;
    // set input sim info
    Gia_ManForEachCiId( p, Id, i )
        memcpy( Vec_WrdEntryP(p->vSims, Id*nWords), Vec_WrdEntryP(vSimsIn, i*nWords), sizeof(word)*nWords );
    // perform simulation
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Gia_ManObjSimAnd( p, i );
        else if ( Gia_ObjIsCi(pObj) )
            continue;
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ManObjSimPo( p, i );
        else assert( 0 );
    }
    return 1;
}

Vec_Wrd_t * Gia_ManSimulateWordsOut( Gia_Man_t * p, Vec_Wrd_t * vSimsIn )
{
    Gia_Obj_t * pObj; int i, Id;
    int nWords = Vec_WrdSize(vSimsIn) / Gia_ManCiNum(p);
    Vec_Wrd_t * vSimsOut = Vec_WrdStart( nWords * Gia_ManCoNum(p) );
    assert( Vec_WrdSize(vSimsIn) == nWords * Gia_ManCiNum(p) );
    // allocate simulation info for one timeframe
    Vec_WrdFreeP( &p->vSims );
    p->vSims = Vec_WrdStart( Gia_ManObjNum(p) * nWords );
    p->nSimWords = nWords;
    // set input sim info
    Gia_ManForEachCiId( p, Id, i )
        memcpy( Vec_WrdEntryP(p->vSims, Id*nWords), Vec_WrdEntryP(vSimsIn, i*nWords), sizeof(word)*nWords );
    // perform simulation
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Gia_ManObjSimAnd( p, i );
        else if ( Gia_ObjIsCi(pObj) )
            continue;
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ManObjSimPo( p, i );
        else assert( 0 );
    }
    // set output sim info
    Gia_ManForEachCoId( p, Id, i )
        memcpy( Vec_WrdEntryP(vSimsOut, i*nWords), Vec_WrdEntryP(p->vSims, Id*nWords), sizeof(word)*nWords );
    Vec_WrdFreeP( &p->vSims );
    p->nSimWords = -1;
    return vSimsOut;
}

/**Function*************************************************************

  Synopsis    [Dump data files.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDumpFiles( Gia_Man_t * p, int nCexesT, int nCexesV, int Seed, char * pFileName )
{
    int n, nSize[2] = {nCexesT*64, nCexesV*64};

    char pFileNameOutTX[100];
    char pFileNameOutTY[100];
    char pFileNameOutVX[100];
    char pFileNameOutVY[100];
    char pFileNameOut[100];
    
    //sprintf( pFileNameOutTX, "train_%s_%d_%d.data", pFileName ? pFileName : Gia_ManName(p), nSize[0], Gia_ManCiNum(p) );
    //sprintf( pFileNameOutTY, "train_%s_%d_%d.data", pFileName ? pFileName : Gia_ManName(p), nSize[0], Gia_ManCoNum(p) );
    //sprintf( pFileNameOutVX, "test_%s_%d_%d.data",  pFileName ? pFileName : Gia_ManName(p), nSize[1], Gia_ManCiNum(p) );
    //sprintf( pFileNameOutVY, "test_%s_%d_%d.data",  pFileName ? pFileName : Gia_ManName(p), nSize[1], Gia_ManCoNum(p) );
    
    sprintf( pFileNameOutTX, "%s_x.train.data", pFileName ? pFileName : Gia_ManName(p) );
    sprintf( pFileNameOutTY, "%s_y.train.data", pFileName ? pFileName : Gia_ManName(p) );
    sprintf( pFileNameOutVX, "%s_x.test.data",  pFileName ? pFileName : Gia_ManName(p) );
    sprintf( pFileNameOutVY, "%s_y.test.data",  pFileName ? pFileName : Gia_ManName(p) );

    Gia_ManRandomW( 1 );
    for ( n = 0; n < Seed; n++ )
        Gia_ManRandomW( 0 );
    for ( n = 0; n < 2; n++ )
    {
        int Res = Gia_ManSimulateWords( p, nSize[n] );

        Vec_Bit_t * vBitX = Vec_BitAlloc( nSize[n] * Gia_ManCiNum(p) );
        Vec_Bit_t * vBitY = Vec_BitAlloc( nSize[n] * Gia_ManCoNum(p) );

        FILE * pFileOutX  = fopen( n ? pFileNameOutVX : pFileNameOutTX, "wb" );
        FILE * pFileOutY  = fopen( n ? pFileNameOutVY : pFileNameOutTY, "wb" );

        int i, k, Id, Num, Value, nBytes;
        for ( k = 0; k < nSize[n]; k++ )
        {
            Gia_ManForEachCiId( p, Id, i )
            {
                Vec_BitPush( vBitX, Abc_TtGetBit(Gia_ManObjSim(p, Id), k) );
                //printf( "%d", Abc_TtGetBit(Gia_ManObjSim(p, Id), k) );
            }
            //printf( " " );
            Gia_ManForEachCoId( p, Id, i )
            {
                Vec_BitPush( vBitY, Abc_TtGetBit(Gia_ManObjSim(p, Id), k) );
                //printf( "%d", Abc_TtGetBit(Gia_ManObjSim(p, Id), k) );
            }
            //printf( "\n" );
        }
        assert( Vec_BitSize(vBitX) <= Vec_BitCap(vBitX) );
        assert( Vec_BitSize(vBitY) <= Vec_BitCap(vBitY) );

        Num = 2;               Value = fwrite( &Num, 1, 4, pFileOutX ); assert( Value == 4 );
        Num = nSize[n];        Value = fwrite( &Num, 1, 4, pFileOutX ); assert( Value == 4 );
        Num = Gia_ManCiNum(p); Value = fwrite( &Num, 1, 4, pFileOutX ); assert( Value == 4 );

        nBytes = nSize[n] * Gia_ManCiNum(p) / 8;
        assert( nSize[n] * Gia_ManCiNum(p) % 8 == 0 );
        Value = fwrite( Vec_BitArray(vBitX), 1, nBytes, pFileOutX );
        assert( Value == nBytes );

        Num = 2;               Value = fwrite( &Num, 1, 4, pFileOutY ); assert( Value == 4 );
        Num = nSize[n];        Value = fwrite( &Num, 1, 4, pFileOutY ); assert( Value == 4 );
        Num = Gia_ManCoNum(p); Value = fwrite( &Num, 1, 4, pFileOutY ); assert( Value == 4 );

        nBytes = nSize[n] * Gia_ManCoNum(p) / 8;
        assert( nSize[n] * Gia_ManCoNum(p) % 8 == 0 );
        Value = fwrite( Vec_BitArray(vBitY), 1, nBytes, pFileOutY );
        assert( Value == nBytes );

        fclose( pFileOutX );
        fclose( pFileOutY );

        Vec_BitFree( vBitX );
        Vec_BitFree( vBitY );

        Res = 0;
    }
    printf( "Finished dumping files \"%s\" and \"%s\".\n", pFileNameOutTX, pFileNameOutTY );
    printf( "Finished dumping files \"%s\" and \"%s\".\n", pFileNameOutVX, pFileNameOutVY );

    sprintf( pFileNameOut, "%s.flist", pFileName ? pFileName : Gia_ManName(p) );
    {
        FILE * pFile = fopen( pFileNameOut, "wb" );
        fprintf( pFile, "%s\n", pFileNameOutTX );
        fprintf( pFile, "%s\n", pFileNameOutTY );
        fprintf( pFile, "%s\n", pFileNameOutVX );
        fprintf( pFile, "%s\n", pFileNameOutVY );
        fclose( pFile );
        printf( "Finished dumping file list \"%s\".\n", pFileNameOut );
    }
}

/**Function*************************************************************

  Synopsis    [Dump data files.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDumpPlaFiles( Gia_Man_t * p, int nCexesT, int nCexesV, int Seed, char * pFileName )
{
    int n, nSize[3] = {nCexesT, nCexesV, nCexesV};

    char pFileNameOut[3][100];
    
    sprintf( pFileNameOut[0], "%s.train.pla", pFileName ? pFileName : Gia_ManName(p) );
    sprintf( pFileNameOut[1], "%s.valid.pla", pFileName ? pFileName : Gia_ManName(p) );
    sprintf( pFileNameOut[2], "%s.test.pla",  pFileName ? pFileName : Gia_ManName(p) );

    Gia_ManRandomW( 1 );
    for ( n = 0; n < Seed; n++ )
        Gia_ManRandomW( 0 );
    for ( n = 0; n < 3; n++ )
    {
        int Res = Gia_ManSimulateWords( p, nSize[n] );
        int i, k, Id;

        FILE * pFileOut  = fopen( pFileNameOut[n], "wb" );

        fprintf( pFileOut, ".i %d\n", Gia_ManCiNum(p) );
        fprintf( pFileOut, ".o %d\n", Gia_ManCoNum(p) );
        fprintf( pFileOut, ".p %d\n", nSize[n]*64 );
        fprintf( pFileOut, ".type fr\n" );
        for ( k = 0; k < nSize[n]*64; k++ )
        {
            Gia_ManForEachCiId( p, Id, i )
            {
                //Vec_BitPush( vBitX, Abc_TtGetBit(Gia_ManObjSim(p, Id), k) );
                fprintf( pFileOut, "%d", Abc_TtGetBit(Gia_ManObjSim(p, Id), k) );
            }
            fprintf( pFileOut, " " );
            Gia_ManForEachCoId( p, Id, i )
            {
                //Vec_BitPush( vBitY, Abc_TtGetBit(Gia_ManObjSim(p, Id), k) );
                fprintf( pFileOut, "%d", Abc_TtGetBit(Gia_ManObjSim(p, Id), k) );
            }
            fprintf( pFileOut, "\n" );
        }
        fprintf( pFileOut, ".e\n" );

        fclose( pFileOut );

        Res = 0;
    }
    printf( "Finished dumping files: \"%s.{train, valid, test}.pla\".\n", pFileName ? pFileName : Gia_ManName(p) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSimLogStats( Gia_Man_t * p, char * pDumpFile, int Total, int Correct, int Guess )
{
    FILE * pTable = fopen( pDumpFile, "wb" );
    fprintf( pTable, "{\n" );
    fprintf( pTable, "    \"name\" : \"%s\",\n", p->pName );
    fprintf( pTable, "    \"input\" : %d,\n",    Gia_ManCiNum(p) );
    fprintf( pTable, "    \"output\" : %d,\n",   Gia_ManCoNum(p) );
    fprintf( pTable, "    \"and\" : %d,\n",      Gia_ManAndNum(p) );
    fprintf( pTable, "    \"level\" : %d,\n",    Gia_ManLevelNum(p) );
    fprintf( pTable, "    \"total\" : %d,\n",    Total );
    fprintf( pTable, "    \"correct\" : %d,\n",  Correct );
    fprintf( pTable, "    \"guess\" : %d\n",     Guess );
    fprintf( pTable, "}\n" );
    fclose( pTable );
}
int Gia_ManSimParamRead( char * pFileName, int * pnIns, int * pnWords )
{
    int c, nIns = -1, nLines = 0, Count = 0, fReadDot = 0;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return 0;
    }
    while ( (c = fgetc(pFile)) != EOF )
    {
        if ( c == '.' )
            fReadDot = 1;
        if ( c == '\n' )
        {
            if ( !fReadDot )
            {
                if ( nIns == -1 )
                    nIns = Count;
                else if ( nIns != Count )
                {
                    printf( "The number of symbols (%d) does not match other lines (%d).\n", Count, nIns );
                    fclose( pFile );
                    return 0;
                }
                Count = 0;
                nLines++;
            }
            fReadDot = 0;
        }
        if ( fReadDot )
            continue;
        if ( c != '0' && c != '1' )
            continue;
        Count++;
    }
    if ( nLines % 64 > 0 )
    {
        printf( "The number of lines (%d) is not divisible by 64.\n", nLines );
        fclose( pFile );
        return 0;
    }
    *pnIns = nIns - 1;
    *pnWords = nLines / 64;
    //printf( "Expecting %d inputs and %d words of simulation data.\n", *pnIns, *pnWords );
    fclose( pFile );
    return 1;
}
void Gia_ManSimFileRead( char * pFileName, int nIns, int nWords, Vec_Wrd_t * vSimsIn, Vec_Int_t * vValues )
{
    int c, nPats = 0, Count = 0, fReadDot = 0;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return;
    }
    assert( Vec_WrdSize(vSimsIn) % nWords == 0 );
    while ( (c = fgetc(pFile)) != EOF )
    {
        if ( c == '.' )
            fReadDot = 1;
        if ( c == '\n' )
            fReadDot = 0;
        if ( fReadDot )
            continue;
        if ( c != '0' && c != '1' )
            continue;
        if ( Count == nIns )
        {
            Vec_IntPush( vValues, c - '0' );
            Count = 0;
            nPats++;
        }
        else
        {
            if ( c == '1' )
                Abc_TtSetBit( Vec_WrdEntryP(vSimsIn, Count * nWords), nPats );
            Count++;
        }
    }
    assert( nPats == 64*nWords );
    fclose( pFile );
    printf( "Finished reading %d simulation patterns for %d inputs. Probability of 1 at the output is %6.2f %%.\n", 64*nWords, nIns, 100.0*Vec_IntSum(vValues)/nPats );
}
void Gia_ManCompareValues( Gia_Man_t * p, Vec_Wrd_t * vSimsIn, Vec_Int_t * vValues, char * pDumpFile )
{
    int i, Value, Guess, Count = 0, nWords = Vec_WrdSize(vSimsIn) / Gia_ManCiNum(p);
    word * pSims;
    assert( Vec_IntSize(vValues) == nWords * 64 );
    Gia_ManSimulateWordsInit( p, vSimsIn );
    assert( p->nSimWords == nWords );
    pSims = Gia_ManObjSim( p, Gia_ObjId(p, Gia_ManCo(p, 0)) );
    Vec_IntForEachEntry( vValues, Value, i )
        if ( Abc_TtGetBit(pSims, i) == Value )
            Count++;
    Guess = (Vec_IntSum(vValues) > nWords * 32) ? Vec_IntSum(vValues) : nWords * 64 - Vec_IntSum(vValues);
    printf( "Total = %6d.  Errors = %6d.  Correct = %6d.  (%6.2f %%)   Naive guess = %6d.  (%6.2f %%)\n", 
        Vec_IntSize(vValues), Vec_IntSize(vValues) - Count, 
        Count, 100.0*Count/Vec_IntSize(vValues),
        Guess, 100.0*Guess/Vec_IntSize(vValues));
    if ( pDumpFile == NULL )
        return;
    Gia_ManSimLogStats( p, pDumpFile, Vec_IntSize(vValues), Count, Guess );
    printf( "Finished dumping statistics into file \"%s\".\n", pDumpFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManReadSimFile( char * pFileName, int * pnIns, int * pnOuts, int * pnPats, Vec_Wrd_t ** pvSimsIn, Vec_Wrd_t ** pvSimsOut )
{
    char * pTemp, pBuffer[1000];
    Vec_Wrd_t * vSimsIn = NULL, * vSimsOut = NULL;
    int i, iPat = 0, nWordsI, nWordsO, nIns = -1, nOuts = -1, nPats = -1;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return;
    }
    while ( fgets( pBuffer, 1000, pFile ) != NULL )
    {
        pTemp = pBuffer;
        if ( pTemp[0] == '\0' || pTemp[0] == '#' || pTemp[0] == ' ' )
            continue;
        if ( pTemp[0] != '.' )
            break;
        if ( pTemp[1] == 'i' )
            nIns = atoi(pTemp+2);
        else if ( pTemp[1] == 'o' )
            nOuts = atoi(pTemp+2);
        else if ( pTemp[1] == 'p' )
        {
            if ( atoi(pTemp+2) % 64 == 0 )
                printf( "Expecting the number of patterns divisible by 64.\n" );
            nPats = atoi(pTemp+2) / 64;
        }
    }
    if ( nIns == -1 || nOuts == -1 || nPats == -1 )
    {
        printf( "Some of the parameters (inputs, outputs, patterns) is not specified.\n" );
        fclose( pFile );
        return;
    }
    nWordsI = (nIns  + 63) / 64;
    nWordsO = (nOuts + 63) / 64;

    vSimsIn  = Vec_WrdStart( nPats * nWordsI );
    vSimsOut = Vec_WrdStart( nPats * nWordsO );
    rewind(pFile);
    while ( fgets( pBuffer, 1000, pFile ) != NULL )
    {
        if ( pTemp[0] == '\0' || pTemp[0] == '.' )
            continue;
        for ( i = 0, pTemp = pBuffer; *pTemp != '\n'; pTemp++ )
            if ( *pTemp == '0' || *pTemp == '1' )
            {
                if ( *pTemp == '1' )
                {
                    if ( i < nIns )
                        Abc_TtSetBit( Vec_WrdEntryP(vSimsIn,  nWordsI*iPat), i );
                    else
                        Abc_TtSetBit( Vec_WrdEntryP(vSimsOut, nWordsO*iPat), i-nIns );
                }
                i++;
            }
        iPat++;
    }
    if ( iPat != nPats )
        printf( "The number of patterns does not match.\n" );
    fclose( pFile );
    *pnIns = nIns;
    *pnOuts = nOuts;
    *pnPats  = nPats;
    *pvSimsIn = vSimsIn;
    *pvSimsOut = vSimsOut;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManReadBinaryFile( char * pFileName, Vec_Wrd_t ** pvSimsIn, Vec_Str_t ** pvSimsOut )
{
    extern void Extra_BitMatrixTransposeP( Vec_Wrd_t * vSimsIn, int nWordsIn, Vec_Wrd_t * vSimsOut, int nWordsOut );
    int nFileSize = Extra_FileSize( pFileName );
    int nExamples = 1 << 16;
    int nInputs   = nFileSize / nExamples - 1;
    int nWords    = (8*nInputs + 63)/64, i;
    char * pContents = Extra_FileReadContents( pFileName );
    Vec_Wrd_t * vSimsIn  = Vec_WrdStart( nExamples * nWords );
    Vec_Wrd_t * vSimsIn2 = Vec_WrdStart( nExamples * nWords );
    Vec_Str_t * vSimsOut = Vec_StrAlloc( nExamples );
    assert( nFileSize % nExamples == 0 );
    for ( i = 0; i < nExamples; i++ )
    {
        memcpy( (void *)Vec_WrdEntryP(vSimsIn,  i*nWords), (void *)(pContents + i*(nInputs+1)), nInputs );
        Vec_StrPush( vSimsOut, pContents[i*(nInputs+1) + nInputs] );
    }
    Extra_BitMatrixTransposeP( vSimsIn, nWords, vSimsIn2, nExamples/64 );
    Vec_WrdShrink( vSimsIn2, 8*nInputs * nExamples/64 );
    Vec_WrdFree( vSimsIn );
    *pvSimsIn  = vSimsIn2;
    *pvSimsOut = vSimsOut;
    ABC_FREE( pContents );
    return nInputs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSimLogStats2( Gia_Man_t * p, char * pDumpFile, int Total, int nPositives, float ErrorTotal, float GuessTotal )
{
    FILE * pTable = fopen( pDumpFile, "wb" );
    fprintf( pTable, "{\n" );
    fprintf( pTable, "    \"name\" : \"%s\",\n", p->pName );
    fprintf( pTable, "    \"input\" : %d,\n",    Gia_ManCiNum(p) );
    fprintf( pTable, "    \"output\" : %d,\n",   Gia_ManCoNum(p) );
    fprintf( pTable, "    \"and\" : %d,\n",      Gia_ManAndNum(p) );
    fprintf( pTable, "    \"level\" : %d,\n",    Gia_ManLevelNum(p) );
    fprintf( pTable, "    \"total\" : %d,\n",    Total );
    fprintf( pTable, "    \"positive\" : %d,\n", nPositives );
    fprintf( pTable, "    \"error\" : %e,\n",    ErrorTotal );
    fprintf( pTable, "    \"guess\" : %e\n",     GuessTotal );
    fprintf( pTable, "}\n" );
    fclose( pTable );
}
int Gia_ManGetExampleValue( word ** ppSims, int nSims, int iExample )
{
    int o, Sign = 0, ValueSim = 0;
    for ( o = 0; o < nSims; o++ )
        if ( (Sign = Abc_TtGetBit(ppSims[o], iExample)) )
            ValueSim |= (1 << o);
    if ( Sign )
        ValueSim |= ~0 << nSims;
    return ValueSim;
}
void Gia_ManCompareValues2( int nInputs, Gia_Man_t * p, Vec_Wrd_t * vSimsIn, Vec_Str_t * vValues, char * pDumpFile )
{
    float Error1, ErrorTotal = 0, Guess1, GuessTotal = 0;
    int i, o, nPositives = 0, nWords = Vec_WrdSize(vSimsIn) / Gia_ManCiNum(p);
    word ** ppSims = ABC_CALLOC( word *, Gia_ManCoNum(p) );
    Gia_Obj_t * pObj;
    assert( nWords == (1<<10) );
    assert( Vec_WrdSize(vSimsIn) % Gia_ManCiNum(p) == 0 );
    assert( Vec_StrSize(vValues) == (1 << 16) );
    assert( nWords*64 == (1 << 16) );
    // simulate examples given in vSimsIn
    Gia_ManSimulateWordsInit( p, vSimsIn );
    // collect simulation info for the outputs
    assert( p->nSimWords == nWords );
    Gia_ManForEachCo( p, pObj, o )
        ppSims[o] = Gia_ManObjSim( p, Gia_ObjId(p, pObj) );
    // compare the output for each example
    for ( i = 0; i < nWords*64; i++ )
    {
        int ValueGold = (int)Vec_StrEntry( vValues, i );
        int ValueImpl = Gia_ManGetExampleValue( ppSims, Gia_ManCoNum(p), i );
        // compute error for this example
        Error1      = (float)(ValueGold - ValueImpl)/256;
        ErrorTotal += Error1 * Error1;
        // compute error of zero-output
        Guess1      = ValueGold > 0 ? Abc_AbsInt(ValueImpl) : 0;
        GuessTotal += Guess1 * Guess1;
        // count positive values (disregard negative values due to Leaky ReLU)
        nPositives += (int)(ValueGold > 0);
    }
    ABC_FREE( ppSims );
    printf( "Total = %6d.  Positive = %6d.  (%6.2f %%)     Errors = %e.  Guess = %e.  (%6.2f %%)\n", 
        Vec_StrSize(vValues), nPositives, 100.0*nPositives/Vec_StrSize(vValues),
        ErrorTotal,           GuessTotal, 100.0*ErrorTotal/GuessTotal   );
    if ( pDumpFile == NULL )
        return;
    Gia_ManSimLogStats2( p, pDumpFile, Vec_StrSize(vValues), nPositives, ErrorTotal, GuessTotal );
    printf( "Finished dumping statistics into file \"%s\".\n", pDumpFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTestWordFileUnused( Gia_Man_t * p, char * pFileName, char * pDumpFile )
{
    Vec_Wrd_t * vSimsIn;
    Vec_Str_t * vSimsOut;
    int nInputs = Gia_ManReadBinaryFile( pFileName, &vSimsIn, &vSimsOut );
    if ( Gia_ManCiNum(p) == 8*nInputs )
        Gia_ManCompareValues2( nInputs, p, vSimsIn, vSimsOut, pDumpFile );
    else
        printf( "The number of inputs in the AIG (%d) and in the file (%d) does not match.\n", Gia_ManCiNum(p), 8*nInputs );
    Vec_WrdFree( vSimsIn );
    Vec_StrFree( vSimsOut );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTestOneFile( Gia_Man_t * p, char * pFileName, char * pDumpFile )
{
    Vec_Wrd_t * vSimsIn;
    Vec_Int_t * vValues;
    int nIns, nWords;
    if ( !Gia_ManSimParamRead( pFileName, &nIns, &nWords ) )
        return;
    if ( nIns != Gia_ManCiNum(p) )
    {
        printf( "The number of inputs in the file \"%s\" (%d) does not match the AIG (%d).\n", pFileName, nIns, Gia_ManCiNum(p) );
        return;
    }
    vSimsIn = Vec_WrdStart( nIns * nWords );
    vValues = Vec_IntAlloc( nWords * 64 );
    Gia_ManSimFileRead( pFileName, nIns, nWords, vSimsIn, vValues );
    Gia_ManCompareValues( p, vSimsIn, vValues, pDumpFile );
    Vec_WrdFree( vSimsIn );
    Vec_IntFree( vValues );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManReadCifar10File( char * pFileName, Vec_Wrd_t ** pvSimsIn, Vec_Str_t ** pvSimsOut, int * pnExamples )
{
    int nPixels   = 32*32*3;
    int nFileSize = Extra_FileSize( pFileName );
    int nExamples = nFileSize / (nPixels + 1);
    int nWordsIn  = nPixels / 8;
    int nWordsOut = (nExamples + 63) / 64; int e;
    if ( nFileSize % (nPixels + 1) )
    {
        printf( "The input file \"%s\" with image data does not appear to be in CIFAR10 format.\n", pFileName );
        return 0;
    }
    else
    {
        Vec_Wrd_t * vSimsIn  = Vec_WrdStart( 64 * nWordsOut * nWordsIn );
        Vec_Str_t * vSimsOut = Vec_StrAlloc( 64 * nWordsOut );
        unsigned char * pBuffer = ABC_ALLOC( unsigned char, nFileSize );
        FILE * pFile = fopen( pFileName, "rb" );
        int Value = fread( pBuffer, 1, nFileSize, pFile );
        fclose( pFile );
        assert( Value == nFileSize );
        printf( "Successfully read %5.2f MB (%d images) from file \"%s\".\n", (float)nFileSize/(1<<20), nExamples, pFileName );
        for ( e = 0; e < nExamples; e++ )
        {
            Vec_StrPush( vSimsOut, (char)pBuffer[e*(nPixels + 1)] );
            memcpy( Vec_WrdEntryP(vSimsIn, e*nWordsIn), pBuffer + e*(nPixels + 1) + 1, nPixels );
        }
        ABC_FREE( pBuffer );
        for ( ; e < 64 * nWordsOut; e++ )
            Vec_StrPush( vSimsOut, (char)0 );
        memset( Vec_WrdEntryP(vSimsIn, nExamples*nWordsIn), 0, (64*nWordsOut - nExamples)*nWordsIn );
        *pvSimsIn   = vSimsIn;
        *pvSimsOut  = vSimsOut;
        *pnExamples = nExamples;
        return 8*nPixels;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSimulateBatch( Gia_Man_t * p, Vec_Wrd_t * vSimsIn, Vec_Str_t * vSimsOut, Vec_Str_t * vSimsOut2, int b, int Limit )
{
    Gia_Obj_t * pObj; 
    word * ppSims[10];
    int i, o, Count = 0;
    assert( Gia_ManCiNum(p) == Vec_WrdSize(vSimsIn) );
    assert( Gia_ManCoNum(p) == 10 );
    Gia_ManSimulateWordsInit( p, vSimsIn );
    Gia_ManForEachCo( p, pObj, o )
        ppSims[o] = Gia_ManObjSim( p, Gia_ObjId(p, pObj) );
    for ( i = 0; i < Limit; i++ )
    {
        int Value = 0;
        for ( o = 0; o < 10; o++ )
            if ( Abc_TtGetBit(ppSims[o], i) )
            {
                Value = o;
                break;
            }
        Vec_StrPush( vSimsOut, (char)Value );
        Count += Value == (int)Vec_StrEntry( vSimsOut2, 64*b+i );
    }
    return Count;
}
Vec_Str_t * Gia_ManSimulateAll( Gia_Man_t * p, Vec_Wrd_t * vSimsIn, Vec_Str_t * vSimsOut, int nExamples, int fVerbose )
{
    extern void Extra_BitMatrixTransposeP( Vec_Wrd_t * vSimsIn, int nWordsIn, Vec_Wrd_t * vSimsOut, int nWordsOut );
    Vec_Str_t * vRes = Vec_StrAlloc( 100 ); int b, Count;
    int nWordsIn  = 32*32*24/64; // one image
    int nWordsOut = Vec_WrdSize(vSimsIn)/(nWordsIn*64); 
    assert( Vec_WrdSize(vSimsIn) % nWordsIn == 0 );
    for ( b = 0; b < nWordsOut; b++ )
    {
        int Limit = b == nWordsOut-1 ? nExamples-b*64 : 64;
        Vec_Wrd_t * vSimsIn1 = Vec_WrdStart( nWordsIn*64 );
        Vec_Wrd_t * vSimsIn2 = Vec_WrdStart( nWordsIn*64 );
        memcpy( Vec_WrdArray(vSimsIn1), Vec_WrdEntryP(vSimsIn, b*nWordsIn*64), sizeof(word)*nWordsIn*64 );
        Extra_BitMatrixTransposeP( vSimsIn1, nWordsIn, vSimsIn2, 1 );
        Vec_WrdFree( vSimsIn1 );
        Count = Gia_ManSimulateBatch( p, vSimsIn2, vRes, vSimsOut, b, Limit );
        Vec_WrdFree( vSimsIn2 );
        if ( fVerbose )
        printf( "Finished simulating word %4d (out of %4d). Correct = %2d. (Limit = %2d.)\n", b, nWordsOut, Count, Limit );
    }
    assert( Vec_StrSize(vRes) == nExamples );
    return vRes;
}
void Gia_ManCompareCifar10Values( Gia_Man_t * p, Vec_Str_t * vRes, Vec_Str_t * vSimsOut, char * pDumpFile, int nExamples )
{
    int i, Guess = (nExamples+9)/10, Count = 0;
    for ( i = 0; i < nExamples; i++ )
    {
        char ValueReal = Vec_StrEntry(vRes,     i);
        char ValueGold = Vec_StrEntry(vSimsOut, i);
        if ( ValueReal == ValueGold )
            Count++;
    }
    printf( "Summary: Total = %6d.  Errors = %6d.  Correct = %6d. (%6.2f %%)   Naive guess = %6d. (%6.2f %%)\n", 
        nExamples, nExamples - Count, 
        Count, 100.0*Count/nExamples,
        Guess, 100.0*Guess/nExamples);
    if ( pDumpFile == NULL )
        return;
    Gia_ManSimLogStats( p, pDumpFile, nExamples, Count, Guess );
    printf( "Finished dumping statistics into file \"%s\".\n", pDumpFile );
}
void Gia_ManTestWordFile( Gia_Man_t * p, char * pFileName, char * pDumpFile, int fVerbose )
{
    abctime clk = Abc_Clock();
    Vec_Wrd_t * vSimsIn;
    Vec_Str_t * vSimsOut;
    int i, nExamples = 0;
    int nInputs = Gia_ManReadCifar10File( pFileName, &vSimsIn, &vSimsOut, &nExamples );
    char * pKnownFileNames[3] = {"small.aig", "medium.aig", "large.aig"};
    int pLimitFileSizes[3] = {10000, 100000, 1000000};
    for ( i = 0; i < 3; i++ )
        if ( p->pSpec && !strncmp(p->pSpec, pKnownFileNames[i], 5) && Gia_ManAndNum(p) > pLimitFileSizes[i] )
            printf( "Warning: The input file \"%s\" contains more than %d internal and-nodes.\n", pKnownFileNames[i], pLimitFileSizes[i] );
    if ( nInputs == Gia_ManCiNum(p) )
    {
        Vec_Str_t * vRes = Gia_ManSimulateAll( p, vSimsIn, vSimsOut, nExamples, fVerbose );
        Gia_ManCompareCifar10Values( p, vRes, vSimsOut, pDumpFile, nExamples );
        Vec_StrFree( vRes );
    }
    else
        printf( "The primary input counts in the AIG (%d) and in the image data (%d) do not match.\n", Gia_ManCiNum(p), nInputs );
    Vec_WrdFree( vSimsIn );
    Vec_StrFree( vSimsOut );
    Abc_PrintTime( 1, "Total checking time", Abc_Clock() - clk );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

