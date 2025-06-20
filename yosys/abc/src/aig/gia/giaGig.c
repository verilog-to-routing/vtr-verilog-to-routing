/**CFile****************************************************************

  FileName    [giaGig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Parser for Gate-Inverter Graph by Niklas Een.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaGig.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/extra/extra.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAX_LINE 1000000

// network types
enum { 
    GLS_NONE = -1,  // not used
    GLS_ZERO =  0,  // zero
    GLS_ONE  =  1,  // one
    GLS_PI   =  2,  // primary input
    GLS_PO   =  3,  // primary output
    GLS_BAR  =  4,  // barrier
    GLS_SEQ  =  5,  // sequential
    GLS_SEL  =  6,  // fan
    GLS_LUT4 =  7,  // LUT4
    GLS_LUT6 =  8,  // LUT6
    GLS_BOX  =  9,  // sequential box
    GLS_DEL  = 10,  // delay box
    GLS_FINAL
};

static char * s_Strs[GLS_FINAL] =
{
    "0",     // GLS_ZERO =  0,  // zero
    "1",     // GLS_ONE  =  1,  // one
    "PI",    // GLS_PI   =  2,  // primary input
    "PO",    // GLS_PO   =  3,  // primary output
    "Bar",   // GLS_BAR  =  4,  // barrier
    "Seq",   // GLS_SEQ  =  5,  // sequential
    "Sel",   // GLS_SEL  =  6,  // fan
    "Lut4",  // GLS_LUT4 =  7,  // LUT4
    "Lut6",  // GLS_LUT6 =  8,  // LUT6
    "Box",   // GLS_BOX  =  9,  // sequential box
    "Del"    // GLS_DEL  = 10,  // delay box
};

typedef struct Gls_Man_t_ Gls_Man_t;
struct Gls_Man_t_
{
    // general
    Vec_Str_t *    vLines;       // line types
    Vec_Str_t *    vTypes;       // gate types
    Vec_Int_t *    vIndexes;     // gate indexes
    // specific types
    Vec_Int_t *    vLut4s;       // 4-LUTs (4-tuples)
    Vec_Int_t *    vLut4TTs;     // truth tables
    Vec_Int_t *    vLut6s;       // 6-LUTs (6-tuples)
    Vec_Wrd_t *    vLut6TTs;     // truth tables
    Vec_Int_t *    vBoxes;       // boxes (5-tuples)
    Vec_Wec_t *    vDelayIns;    // delay fanins
    Vec_Wec_t *    vDelayOuts;   // delay fanouts
    Vec_Int_t *    vDelays;      // delay values
    // ordering
    Vec_Int_t *    vOrderPis;  
    Vec_Int_t *    vOrderPos;  
    Vec_Int_t *    vOrderBoxes;  
    Vec_Int_t *    vOrderDelays;  
    Vec_Int_t *    vOrderLuts;  
    Vec_Int_t *    vOrderSeqs;  
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gls_Man_t * Gls_ManAlloc( Vec_Str_t * vLines, int * pCounts )
{
    Gls_Man_t * p = ABC_CALLOC( Gls_Man_t, 1 );
    p->vLines     = vLines;
    p->vTypes     = Vec_StrStart( Vec_StrSize(vLines)+100 ); 
    p->vIndexes   = Vec_IntStart( Vec_StrSize(vLines)+100 ); 
    p->vLut4s     = Vec_IntAlloc( 4 * pCounts[GLS_LUT4] );
    p->vLut4TTs   = Vec_IntAlloc( pCounts[GLS_LUT4] );
    p->vLut6s     = Vec_IntAlloc( 6 * pCounts[GLS_LUT6] );
    p->vLut6TTs   = Vec_WrdAlloc( pCounts[GLS_LUT6] );
    p->vBoxes     = Vec_IntAlloc( 5 * pCounts[GLS_BOX] );
    p->vDelays    = Vec_IntAlloc( pCounts[GLS_DEL] );
    p->vDelayIns  = Vec_WecAlloc( pCounts[GLS_DEL] );
    p->vDelayOuts = Vec_WecAlloc( pCounts[GLS_DEL] );
    // ordering
    p->vOrderPis    = Vec_IntAlloc( pCounts[GLS_PI] );
    p->vOrderPos    = Vec_IntAlloc( pCounts[GLS_PO] );
    p->vOrderBoxes  = Vec_IntAlloc( pCounts[GLS_BOX] );
    p->vOrderDelays = Vec_IntAlloc( pCounts[GLS_DEL] );
    p->vOrderLuts   = Vec_IntAlloc( pCounts[GLS_LUT4] + pCounts[GLS_LUT6] + 2*pCounts[GLS_BAR] );
    p->vOrderSeqs   = Vec_IntAlloc( pCounts[GLS_SEQ] );
    return p;
}
void Gls_ManStop( Gls_Man_t * p )
{
    Vec_StrFree( p->vLines );
    Vec_StrFree( p->vTypes );
    Vec_IntFree( p->vIndexes );
    Vec_IntFree( p->vLut4s );
    Vec_IntFree( p->vLut4TTs );
    Vec_IntFree( p->vLut6s );
    Vec_WrdFree( p->vLut6TTs );
    Vec_IntFree( p->vBoxes );
    Vec_IntFree( p->vDelays );
    Vec_WecFree( p->vDelayIns );
    Vec_WecFree( p->vDelayOuts );
    // ordering
    Vec_IntFree( p->vOrderPis );
    Vec_IntFree( p->vOrderPos );
    Vec_IntFree( p->vOrderBoxes );
    Vec_IntFree( p->vOrderDelays );
    Vec_IntFree( p->vOrderLuts );
    Vec_IntFree( p->vOrderSeqs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Str_t * Gls_ManCount( FILE * pFile, int pCounts[GLS_FINAL] )
{
    char * pLine, * pBuffer = ABC_ALLOC(char, MAX_LINE); int Type;
    Vec_Str_t * vLines = Vec_StrAlloc( 10000 );
    memset( pCounts, 0, sizeof(int)*GLS_FINAL );
    while ( fgets( pBuffer, MAX_LINE, pFile ) != NULL )
    {
        pLine = pBuffer;
        while ( *pLine )
            if ( *pLine++ == '=' )
                break;
        while ( *pLine == ' ' )
            pLine++;
        if ( *pLine == 'L' )
        {
            if ( pLine[3] == '4' )
                Type = GLS_LUT4;
            else if ( pLine[3] == '6' )
                Type = GLS_LUT6;
            else assert( 0 );
        }
        else if ( *pLine == 'P' )
        {
            if ( pLine[1] == 'I' )
                Type = GLS_PI;
            else if ( pLine[1] == 'O' )
                Type = GLS_PO;
            else assert( 0 );
        }
        else if ( *pLine == 'B' )
        {
            if ( pLine[1] == 'o' )
                Type = GLS_BOX;
            else if ( pLine[1] == 'a' )
                Type = GLS_BAR;
            else assert( 0 );
        }
        else if ( *pLine == 'S' )
        {
            if ( pLine[2] == 'l' )
                Type = GLS_SEL;
            else if ( pLine[2] == 'q' )
                Type = GLS_SEQ;
            else assert( 0 );
        }
        else if ( *pLine == 'D' )
            Type = GLS_DEL;
        else assert( 0 );
        Vec_StrPush( vLines, (char)Type );
        pCounts[Type]++;
    }
    ABC_FREE( pBuffer );
    return vLines;
}
int Gls_ManParseOne( char ** ppLine )
{
    int Entry;
    char * pLine = *ppLine;
    while ( *pLine == ' ' )   pLine++;
    if ( *pLine == '-' )
        Entry = GLS_NONE;
    else if ( *pLine == '0' )
        Entry = 0;
    else if ( *pLine == '1' )
        Entry = 1;
    else if ( *pLine == 'w' )
        Entry = atoi(++pLine);
    else assert( 0 );
    while ( *pLine == '-' || (*pLine >= '0' && *pLine <= '9') )   pLine++;
    while ( *pLine == ' ' )   pLine++;
    *ppLine = pLine;
    return Entry;
}
int Gls_ManParse( FILE * pFile, Gls_Man_t * p )
{
    char * pLine, * pBuffer = ABC_ALLOC(char, MAX_LINE); 
    int i, k, Type, iObj, Entry, iItem;  word Truth;
    for ( i = 0; fgets( pBuffer, MAX_LINE, pFile ) != NULL; i++ )
    {
        pLine = pBuffer;
        Type = Vec_StrEntry( p->vLines, i );
        iObj = Gls_ManParseOne( &pLine );
        Vec_StrWriteEntry( p->vTypes, iObj, (char)Type );
        if ( Type == GLS_PI )
        {
            Vec_IntPush( p->vOrderPis, iObj );
            Vec_IntWriteEntry( p->vIndexes, iObj, -1 );
            continue;
        }
        while ( *pLine )
            if ( *pLine++ == '(' )
                break;
        Entry = Gls_ManParseOne( &pLine );
        if ( Type == GLS_PO || Type == GLS_BAR || Type == GLS_SEQ || Type == GLS_SEL )
        {
            if ( Type == GLS_PO )
                Vec_IntPush( p->vOrderPos, iObj );
            else if ( Type == GLS_BAR )
                Vec_IntPush( p->vOrderLuts, iObj );
            else if ( Type == GLS_SEQ )
                Vec_IntPush( p->vOrderSeqs, iObj );
            else if ( Type == GLS_SEL )
            {
                if ( (int)Vec_StrEntry(p->vTypes, Entry) == GLS_DEL )
                {
                    Vec_Int_t * vOuts = Vec_WecEntry( p->vDelayOuts, Vec_IntEntry(p->vIndexes, Entry) );
                    Vec_IntPush( vOuts, iObj );
                }
                else if ( (int)Vec_StrEntry(p->vTypes, Entry) == GLS_BAR )
                    Vec_IntPush( p->vOrderLuts, iObj );
                else assert( 0 );
            }
            Vec_IntWriteEntry( p->vIndexes, iObj, Entry );
            continue;
        }
        if ( Type == GLS_LUT4 )
        {
            Vec_IntWriteEntry( p->vIndexes, iObj, Vec_IntSize(p->vLut4TTs) );
            Vec_IntPush( p->vLut4s, Entry );
            for ( k = 1; ; k++ )
            {
                if ( *pLine != ',' )     break;
                pLine++;
                Entry = Gls_ManParseOne( &pLine );
                Vec_IntPush( p->vLut4s, Entry );
            }
            assert( *pLine == ')' );
            assert( k == 4 );
            pLine++;
            while ( *pLine )
                if ( *pLine++ == '[' )
                    break;
            Abc_TtReadHex( &Truth, pLine );
            Vec_IntPush( p->vLut4TTs, (unsigned)Truth );
            Vec_IntPush( p->vOrderLuts, iObj );
        }
        else if ( Type == GLS_LUT6 )
        {
            Vec_IntWriteEntry( p->vIndexes, iObj, Vec_WrdSize(p->vLut6TTs) );
            Vec_IntPush( p->vLut6s, Entry );
            for ( k = 1; ; k++ )
            {
                if ( *pLine != ',' )     break;
                pLine++;
                Entry = Gls_ManParseOne( &pLine );
                Vec_IntPush( p->vLut6s, Entry );
            }
            assert( *pLine == ')' );
            assert( k == 4 );
            pLine++;
            while ( *pLine )
                if ( *pLine++ == '[' )
                    break;
            Abc_TtReadHex( &Truth, pLine );
            Vec_WrdPush( p->vLut6TTs, Truth );
            Vec_IntPush( p->vOrderLuts, iObj );
        }
        else if ( Type == GLS_BOX )
        {
            Vec_IntWriteEntry( p->vIndexes, iObj, Vec_IntSize(p->vBoxes)/5 );
            Vec_IntPush( p->vBoxes, Entry );
            for ( k = 1; ; k++ )
            {
                if ( *pLine != ',' )     break;
                pLine++;
                Entry = Gls_ManParseOne( &pLine );
                Vec_IntPush( p->vBoxes, Entry );
            }
            assert( *pLine == ')' );
            assert( k == 4 || k == 5 );
            if ( k == 4 )
                Vec_IntPush( p->vBoxes, GLS_NONE );
            Vec_IntPush( p->vOrderBoxes, iObj );
        }
        else if ( Type == GLS_DEL )
        {
            Vec_Int_t * vIns  = Vec_WecPushLevel( p->vDelayIns );
            Vec_Int_t * vOuts = Vec_WecPushLevel( p->vDelayOuts );
            Vec_IntWriteEntry( p->vIndexes, iObj, Vec_IntSize(p->vDelays) );
            Vec_IntPush( vIns, Entry );
            if ( *pLine != ')' )
            {
                for ( k = 1; ; k++ )
                {
                    if ( *pLine != ',' )     break;
                    pLine++;
                    Entry = Gls_ManParseOne( &pLine );
                    Vec_IntPush( vIns, Entry );
                }
            }
            assert( *pLine == ')' );
            pLine++;
            while ( *pLine )
                if ( *pLine++ == '[' )
                    break;
            iItem = atoi(pLine);
            Vec_IntPush( p->vDelays, iItem );
            Vec_IntPush( p->vOrderDelays, iObj );
            vOuts = vIns; // harmless use to prevent a compiler warning
        }
        else assert( 0 );
    }
    ABC_FREE( pBuffer );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gls_ManConstruct( Gls_Man_t * p, char * pFileName )
{
    extern int Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash );
    Gia_Man_t * pGia = NULL;  
    Vec_Int_t * vMap, * vArray;
    Vec_Int_t * vCover = Vec_IntAlloc(0);
    Vec_Int_t * vLeaves = Vec_IntAlloc(6);
    int  k, iObj, iLit, Index;  char Type;
    // create new manager
    pGia = Gia_ManStart( Vec_StrSize(p->vTypes) );
    pGia->pName = Abc_UtilStrsav( pFileName );
    pGia->pSpec = Abc_UtilStrsav( pFileName );
    // create constants
    vMap = Vec_IntStartFull( Vec_StrSize(p->vTypes) );
    Vec_IntWriteEntry( vMap, 0, 0 );
    Vec_IntWriteEntry( vMap, 1, 1 );
    // create primary inputs
    Vec_IntForEachEntry( p->vOrderPis, iObj, k )
        Vec_IntWriteEntry( vMap, iObj, Gia_ManAppendCi(pGia) );
    // create box outputs
    Vec_IntForEachEntry( p->vOrderBoxes, iObj, k )
        Vec_IntWriteEntry( vMap, iObj, Gia_ManAppendCi(pGia) );
    // create delay outputs
    Vec_IntForEachEntry( p->vOrderDelays, iObj, Index )
    {
        assert( Index == Vec_IntEntry(p->vIndexes, iObj) );
        vArray = Vec_WecEntry(p->vDelayOuts, Index);
        if ( Vec_IntSize(vArray) == 0 )
            Vec_IntWriteEntry( vMap, iObj, Gia_ManAppendCi(pGia) );
        else
            Vec_IntForEachEntry( vArray, iObj, k )
                Vec_IntWriteEntry( vMap, iObj, Gia_ManAppendCi(pGia) );
    }
    // construct LUTs
    Vec_IntForEachEntry( p->vOrderLuts, iObj, Index )
    {
        Type = Vec_StrEntry( p->vTypes, iObj );
        if ( Type == GLS_LUT4 || Type == GLS_LUT6 )
        {
            int Limit = Type == GLS_LUT4 ? 4 : 6;
            int Index = Vec_IntEntry(p->vIndexes, iObj);
            int * pFanins = Type == GLS_LUT4 ? Vec_IntEntryP(p->vLut4s, 4*Index) : Vec_IntEntryP(p->vLut6s, 6*Index);
            word Truth = Type == GLS_LUT4 ? (word)Vec_IntEntry(p->vLut4TTs, Index) : Vec_WrdEntry(p->vLut6TTs, Index);
            Vec_IntClear( vLeaves );
            for ( k = 0; k < Limit; k++ )
                Vec_IntPush( vLeaves, pFanins[k] == GLS_NONE ? 0 : Vec_IntEntry(vMap, pFanins[k]) );
            iLit = Kit_TruthToGia( pGia, (unsigned *)&Truth, Vec_IntSize(vLeaves), vCover, vLeaves, 0 );
            Vec_IntWriteEntry( vMap, iObj, iLit );
        }
        else if ( Type == GLS_BAR || Type == GLS_SEL )
        {
            iLit = Vec_IntEntry( vMap, Vec_IntEntry(p->vIndexes, iObj) );
            Vec_IntWriteEntry( vMap, iObj, iLit );
        }
    }
    // delay inputs
    Vec_IntForEachEntry( p->vOrderDelays, iObj, Index )
    {
        vArray = Vec_WecEntry(p->vDelayIns, Index);
        assert( Vec_IntSize(vArray) > 0 );
        Vec_IntForEachEntry( vArray, iObj, k )
            Gia_ManAppendCo( pGia, Vec_IntEntry(vMap, iObj) );
    }
    // create primary outputs
    Vec_IntForEachEntry( p->vOrderPos, iObj, k )
        Gia_ManAppendCo( pGia, Vec_IntEntry(vMap, Vec_IntEntry(p->vIndexes, iObj)) );
    // create sequential nodes
    Vec_IntForEachEntry( p->vOrderSeqs, iObj, k )
        Gia_ManAppendCo( pGia, Vec_IntEntry(vMap, Vec_IntEntry(p->vIndexes, iObj)) );
    Vec_IntFree( vMap );
    Vec_IntFree( vCover );
    Vec_IntFree( vLeaves );
    // print delay boxes
//    for ( k = 0; k < Vec_IntSize(p->vDelays); k++ )
//        printf( "%d:%d  ", Vec_IntSize(Vec_WecEntry(p->vDelayIns, k)), Vec_IntSize(Vec_WecEntry(p->vDelayOuts, k)) );
//    printf( "\n" );
    return pGia;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManReadGig( char * pFileName )
{
    abctime clk = Abc_Clock();
    Gls_Man_t * p = NULL;
    Gia_Man_t * pGia = NULL;
    Vec_Str_t * vLines;
    int i, pCounts[GLS_FINAL];
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot read file \"%s\".\n", pFileName );
        return NULL;
    }
    vLines = Gls_ManCount( pFile, pCounts );
    rewind( pFile );
    // statistics
    for ( i = 0; i < GLS_FINAL; i++ )
        if ( pCounts[i] )
            printf( "%s=%d  ", s_Strs[i], pCounts[i] );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // collect data and derive AIG
    p = Gls_ManAlloc( vLines, pCounts );
    if ( Gls_ManParse( pFile, p ) )
        pGia = Gls_ManConstruct( p, pFileName );
    Gls_ManStop( p );
    fclose( pFile );
    //printf( "\n" );
    return pGia;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

