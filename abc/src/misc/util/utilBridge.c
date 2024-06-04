/**CFile****************************************************************

  FileName    [utilBridge.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName []

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: utilBridge.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <misc/util/abc_global.h>

#if defined(LIN) || defined(LIN64)
#include <unistd.h>
#endif

#include "aig/gia/gia.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define BRIDGE_TEXT_MESSAGE 999996

#define BRIDGE_ABORT        5
#define BRIDGE_PROGRESS     3
#define BRIDGE_RESULTS      101
#define BRIDGE_BAD_ABS      105
//#define BRIDGE_NETLIST      106
//#define BRIDGE_ABS_NETLIST  107

#define BRIDGE_VALUE_X 0
#define BRIDGE_VALUE_0 2
#define BRIDGE_VALUE_1 3


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Gia_ManToBridgeVec( Gia_Man_t * p )
{
    Vec_Str_t * vStr;
    Gia_Obj_t * pObj;
    int i, uLit0, uLit1, nNodes;
    assert( Gia_ManPoNum(p) > 0 );

    // start with const1 node (number 1)
    nNodes = 1;
    // assign literals(!!!) to the value field
    Gia_ManConst0(p)->Value = Abc_Var2Lit( nNodes++, 1 );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Abc_Var2Lit( nNodes++, 0 );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Abc_Var2Lit( nNodes++, 0 );

    // write header
    vStr = Vec_StrAlloc( 1000 );
    Gia_AigerWriteUnsigned( vStr, Gia_ManPiNum(p) );
    Gia_AigerWriteUnsigned( vStr, Gia_ManRegNum(p) );
    Gia_AigerWriteUnsigned( vStr, Gia_ManAndNum(p) );

    // write the nodes 
    Gia_ManForEachAnd( p, pObj, i )
    {
        uLit0 = Gia_ObjFanin0Copy( pObj );
        uLit1 = Gia_ObjFanin1Copy( pObj );
        assert( uLit0 != uLit1 );
        Gia_AigerWriteUnsigned( vStr, uLit0 << 1 );
        Gia_AigerWriteUnsigned( vStr, uLit1 );
    }

    // write latch drivers
    Gia_ManForEachRi( p, pObj, i )
    {
        uLit0 = Gia_ObjFanin0Copy( pObj );
        Gia_AigerWriteUnsigned( vStr, (uLit0 << 2) | BRIDGE_VALUE_0 );
    }

    // write PO drivers
    Gia_AigerWriteUnsigned( vStr, Gia_ManPoNum(p) );
    Gia_ManForEachPo( p, pObj, i )
    {
        uLit0 = Gia_ObjFanin0Copy( pObj );
        // complement property output!!!
        Gia_AigerWriteUnsigned( vStr, Abc_LitNot(uLit0) );
    }
    return vStr;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_CreateHeader( FILE * pFile, int Type, int Size, unsigned char * pBuffer )
{
    fprintf( pFile, "%.6d", Type );
    fprintf( pFile, " " );
    fprintf( pFile, "%.16d", Size );
    fprintf( pFile, " " );
#if !defined(LIN) && !defined(LIN64)
    {
    int RetValue;
    RetValue = fwrite( pBuffer, Size, 1, pFile );
    assert( RetValue == 1 || Size == 0);
    fflush( pFile );
    }
#else
    fflush(pFile);
    int fd = fileno(pFile);

    ssize_t bytes_written = 0;
    while (bytes_written < Size){
        ssize_t n = write(fd, &pBuffer[bytes_written], Size - bytes_written);
        if (n < 0){
            fprintf(stderr, "BridgeMode: failed to send package; aborting\n"); fflush(stderr);
            _exit(255);
        }
        bytes_written += n;
    }
#endif
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManToBridgeText( FILE * pFile, int Size, unsigned char * pBuffer )
{
    Gia_CreateHeader( pFile, BRIDGE_TEXT_MESSAGE, Size, pBuffer );
    return 1;
}


int Gia_ManToBridgeAbort( FILE * pFile, int Size, unsigned char * pBuffer )
{
    Gia_CreateHeader( pFile, BRIDGE_ABORT, Size, pBuffer );
    return 1;
}


int Gia_ManToBridgeProgress( FILE * pFile, int Size, unsigned char * pBuffer )
{
    Gia_CreateHeader( pFile, BRIDGE_PROGRESS, Size, pBuffer );
    return 1;
}


int Gia_ManToBridgeAbsNetlist( FILE * pFile, void * p, int pkg_type )
{
    Vec_Str_t * vBuffer;
    vBuffer = Gia_ManToBridgeVec( (Gia_Man_t *)p );
    Gia_CreateHeader( pFile, pkg_type, Vec_StrSize(vBuffer), (unsigned char *)Vec_StrArray(vBuffer) );
    Vec_StrFree( vBuffer );
    return 1;
}


int Gia_ManToBridgeBadAbs( FILE * pFile )
{
    Gia_CreateHeader( pFile, BRIDGE_BAD_ABS, 0, NULL );
    return 1;
}


static int aigerNumSize( unsigned x )
{
    int sz = 1;
    while (x & ~0x7f)
    {
        sz++;
        x >>= 7;
    }
    return sz;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFromBridgeHolds( FILE * pFile, int iPoProved )
{
    fprintf( pFile, "%.6d", 101 /*message type = Result*/);
    fprintf( pFile, " " );
    fprintf( pFile, "%.16d", 3 + aigerNumSize(iPoProved) /*size in bytes*/);
    fprintf( pFile, " " );

    fputc( (char)BRIDGE_VALUE_1, pFile ); // true
    fputc( (char)1, pFile ); // size of vector (Armin's encoding)
    Gia_AigerWriteUnsignedFile( pFile, iPoProved ); // number of the property (Armin's encoding)
    fputc( (char)0, pFile ); // no invariant
    fflush(pFile);
}
void Gia_ManFromBridgeUnknown( FILE * pFile, int iPoUnknown )
{
    fprintf( pFile, "%.6d", 101 /*message type = Result*/);
    fprintf( pFile, " " );
    fprintf( pFile, "%.16d", 2 + aigerNumSize(iPoUnknown) /*size in bytes*/);
    fprintf( pFile, " " );

    fputc( (char)BRIDGE_VALUE_X, pFile ); // undef
    fputc( (char)1, pFile ); // size of vector (Armin's encoding)
    Gia_AigerWriteUnsignedFile( pFile, iPoUnknown ); // number of the property (Armin's encoding)
    fflush(pFile);
}
void Gia_ManFromBridgeCex( FILE * pFile, Abc_Cex_t * pCex )
{
    int i, f, iBit;//, RetValue;
    Vec_Str_t * vStr = Vec_StrAlloc( 1000 );
    Vec_StrPush( vStr, (char)BRIDGE_VALUE_0 ); // false
    Vec_StrPush( vStr, (char)1 ); // size of vector (Armin's encoding)
    Gia_AigerWriteUnsigned( vStr, pCex->iPo ); // number of the property (Armin's encoding)
    Vec_StrPush( vStr, (char)1 ); // size of vector (Armin's encoding)
    Gia_AigerWriteUnsigned( vStr, pCex->iFrame ); // depth

    Gia_AigerWriteUnsigned( vStr, 1 ); // concrete
    Gia_AigerWriteUnsigned( vStr, pCex->iFrame + 1 ); // number of frames (1 more than depth)
    iBit = pCex->nRegs;
    for ( f = 0; f <= pCex->iFrame; f++ )
    {
        Gia_AigerWriteUnsigned( vStr, pCex->nPis ); // num of inputs
        for ( i = 0; i < pCex->nPis; i++, iBit++ )
            Vec_StrPush( vStr, (char)(Abc_InfoHasBit(pCex->pData, iBit) ? BRIDGE_VALUE_1:BRIDGE_VALUE_0) ); // value
    }
    assert( iBit == pCex->nBits );
    Vec_StrPush( vStr, (char)1 ); // the number of frames (for a concrete counter-example)
    Gia_AigerWriteUnsigned( vStr, pCex->nRegs ); // num of flops
    for ( i = 0; i < pCex->nRegs; i++ )
        Vec_StrPush( vStr, (char)BRIDGE_VALUE_0 ); // always zero!!!
//    RetValue = fwrite( Vec_StrArray(vStr), Vec_StrSize(vStr), 1, pFile );
    Gia_CreateHeader(pFile, 101/*type=Result*/, Vec_StrSize(vStr), (unsigned char*)Vec_StrArray(vStr));

    Vec_StrFree( vStr );
    fflush(pFile);
}
int Gia_ManToBridgeResult( FILE * pFile, int Result, Abc_Cex_t * pCex, int iPoProved )
{
    if ( Result == 0 ) // sat
        Gia_ManFromBridgeCex( pFile, pCex );
    else if ( Result == 1 ) // unsat
        Gia_ManFromBridgeHolds( pFile, iPoProved );
    else if ( Result == -1 ) // undef
        Gia_ManFromBridgeUnknown( pFile, iPoProved );
    else assert( 0 );
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t *  Gia_ManFromBridgeReadBody( int Size, unsigned char * pBuffer, Vec_Int_t ** pvInits )
{
    int fHash = 0;
    Vec_Int_t * vLits, * vInits;
    Gia_Man_t * p = NULL;
    unsigned char * pBufferPivot, * pBufferEnd = pBuffer + Size;
    int i, nInputs, nFlops, nGates, nProps;
    int verFairness, nFairness, nConstraints;
    unsigned iFan0, iFan1;

    nInputs = Gia_AigerReadUnsigned( &pBuffer );
    nFlops  = Gia_AigerReadUnsigned( &pBuffer );
    nGates  = Gia_AigerReadUnsigned( &pBuffer );

    vLits = Vec_IntAlloc( 1000 );
    Vec_IntPush( vLits, -999 );
    Vec_IntPush( vLits,  1 );

    // start the AIG package
    p = Gia_ManStart( nInputs + nFlops * 2 + nGates + 1 + 1 ); // PI+FO+FI+AND+PO+1
    p->pName = Abc_UtilStrsav( "temp" );

    // create PIs
    for ( i = 0; i < nInputs; i++ )
        Vec_IntPush( vLits, Gia_ManAppendCi( p ) );

    // create flop outputs
    for ( i = 0; i < nFlops; i++ )
        Vec_IntPush( vLits, Gia_ManAppendCi( p ) );

    // create nodes
    if ( fHash )
        Gia_ManHashAlloc( p );
    for ( i = 0; i < nGates; i++ )
    {
        iFan0 = Gia_AigerReadUnsigned( &pBuffer );
        iFan1 = Gia_AigerReadUnsigned( &pBuffer );
        assert( !(iFan0 & 1) );
        iFan0 >>= 1;
        iFan0 = Abc_LitNotCond( Vec_IntEntry(vLits, iFan0 >> 1), iFan0 & 1 );
        iFan1 = Abc_LitNotCond( Vec_IntEntry(vLits, iFan1 >> 1), iFan1 & 1 );
        if ( fHash )
            Vec_IntPush( vLits, Gia_ManHashAnd(p, iFan0, iFan1) );
        else
            Vec_IntPush( vLits, Gia_ManAppendAnd(p, iFan0, iFan1) );

    }
    if ( fHash )
        Gia_ManHashStop( p );

    // remember where flops begin
    pBufferPivot = pBuffer;
    // scroll through flops
    for ( i = 0; i < nFlops; i++ )
        Gia_AigerReadUnsigned( &pBuffer );

    // create POs
    nProps = Gia_AigerReadUnsigned( &pBuffer );
//    assert( nProps == 1 );
    for ( i = 0; i < nProps; i++ )
    {
        iFan0 = Gia_AigerReadUnsigned( &pBuffer );
        iFan0 = Abc_LitNotCond( Vec_IntEntry(vLits, iFan0 >> 1), iFan0 & 1 );
        // complement property output!!!
        Gia_ManAppendCo( p, Abc_LitNot(iFan0) );
    }

    verFairness = Gia_AigerReadUnsigned( &pBuffer );
    assert( verFairness == 1 );

    nFairness = Gia_AigerReadUnsigned( &pBuffer );
    assert( nFairness == 0 );

    nConstraints = Gia_AigerReadUnsigned( &pBuffer );
    assert( nConstraints == 0);

    // make sure the end of buffer is reached
    assert( pBufferEnd == pBuffer );

    // resetting to flops
    pBuffer = pBufferPivot;
    vInits = Vec_IntAlloc( nFlops );
    for ( i = 0; i < nFlops; i++ )
    {
        iFan0 = Gia_AigerReadUnsigned( &pBuffer );
        assert( (iFan0 & 3) == BRIDGE_VALUE_0 );
        Vec_IntPush( vInits, iFan0 & 3 ); // 0 = X value; 1 = not used; 2 = false; 3 = true
        iFan0 >>= 2;
        iFan0 = Abc_LitNotCond( Vec_IntEntry(vLits, iFan0 >> 1), iFan0 & 1 );
        Gia_ManAppendCo( p, iFan0 );
    }
    Gia_ManSetRegNum( p, nFlops );
    Vec_IntFree( vLits );

    // remove wholes in the node list
    if ( fHash )
    {
        Gia_Man_t * pTemp;
        p = Gia_ManCleanup( pTemp = p );
        Gia_ManStop( pTemp );
    }

    // return
    if ( pvInits )
        *pvInits = vInits;
    else
        Vec_IntFree( vInits );
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFromBridgeReadPackage( FILE * pFile, int * pType, int * pSize, unsigned char ** ppBuffer )
{
    char Temp[24];
    int RetValue;
    RetValue = fread( Temp, 24, 1, pFile );
    if ( RetValue != 1 )
    {
        printf( "Gia_ManFromBridgeReadPackage();  Error 1: Something is wrong!\n" );
        return 0;
    }
    Temp[6] = 0;
    Temp[23]= 0;

    *pType = atoi( Temp );
    *pSize = atoi( Temp + 7 );

    *ppBuffer = ABC_ALLOC( unsigned char, *pSize );
    RetValue = fread( *ppBuffer, *pSize, 1, pFile );
    if ( RetValue != 1 && *pSize != 0 )
    {
        ABC_FREE( *ppBuffer );
        printf( "Gia_ManFromBridgeReadPackage();  Error 2: Something is wrong!\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFromBridge( FILE * pFile, Vec_Int_t ** pvInit )
{
    unsigned char * pBuffer;
    int Type, Size, RetValue;
    Gia_Man_t * p = NULL;

    RetValue = Gia_ManFromBridgeReadPackage( pFile, &Type, &Size, &pBuffer );
    ABC_FREE( pBuffer );
    if ( !RetValue )
        return NULL;

    RetValue = Gia_ManFromBridgeReadPackage( pFile, &Type, &Size, &pBuffer );
    if ( !RetValue )
        return NULL;

    p = Gia_ManFromBridgeReadBody( Size, pBuffer, pvInit );
    ABC_FREE( pBuffer );
    if ( p == NULL )
        return NULL;

    RetValue = Gia_ManFromBridgeReadPackage( pFile, &Type, &Size, &pBuffer );
    ABC_FREE( pBuffer );
    if ( !RetValue )
        return NULL;

    return p;
}

/*
    {
        extern void Gia_ManFromBridgeTest( char * pFileName );
        Gia_ManFromBridgeTest( "C:\\_projects\\abc\\_TEST\\bug\\65\\par.dump" );
    }
*/

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManToBridgeAbsNetlistTest( char * pFileName, Gia_Man_t * p, int msg_type )
{
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }
    Gia_ManToBridgeAbsNetlist( pFile, p, msg_type );
    fclose ( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFromBridgeTest( char * pFileName )
{
    Gia_Man_t * p;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open input file \"%s\".\n", pFileName );
        return;
    }

    p = Gia_ManFromBridge( pFile, NULL );
    fclose ( pFile );

    Gia_ManPrintStats( p, NULL );
    Gia_AigerWrite( p, "temp.aig", 0, 0, 0 );

    Gia_ManToBridgeAbsNetlistTest( "par_.dump", p, BRIDGE_ABS_NETLIST );
    Gia_ManStop( p );
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
