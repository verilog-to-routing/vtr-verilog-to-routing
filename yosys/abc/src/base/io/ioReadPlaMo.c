/**CFile****************************************************************

  FileName    [ioReadPlaMo.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedure to read network from file.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioReadPlaMo.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Mop_Man_t_ Mop_Man_t;
struct Mop_Man_t_
{
    int              nIns;
    int              nOuts;
    int              nWordsIn;
    int              nWordsOut;
    Vec_Wrd_t *      vWordsIn;
    Vec_Wrd_t *      vWordsOut;
    Vec_Int_t *      vCubes; 
    Vec_Int_t *      vFree;
};

static inline int    Mop_ManIsSopSymb( char c ) {  return c == '0' || c == '1' || c == '-'; }
static inline int    Mop_ManIsSpace( char c )   {  return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r'; }

static inline word * Mop_ManCubeIn( Mop_Man_t * p, int i )  { return Vec_WrdEntryP(p->vWordsIn,   p->nWordsIn  * i); }
static inline word * Mop_ManCubeOut( Mop_Man_t * p, int i ) { return Vec_WrdEntryP(p->vWordsOut,  p->nWordsOut * i); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mop_Man_t * Mop_ManAlloc( int nIns, int nOuts, int nCubes )
{
    Mop_Man_t * p   = ABC_CALLOC( Mop_Man_t, 1 );
    p->nIns         = nIns;
    p->nOuts        = nOuts;
    p->nWordsIn     = Abc_Bit6WordNum( 2 * nIns );
    p->nWordsOut    = Abc_Bit6WordNum( nOuts );
    p->vWordsIn     = Vec_WrdStart( 2 * p->nWordsIn * nCubes );
    p->vWordsOut    = Vec_WrdStart( 2 * p->nWordsOut * nCubes );
    p->vCubes       = Vec_IntAlloc( 2 * nCubes );
    p->vFree        = Vec_IntAlloc( 2 * nCubes );
    return p;
}
void Mop_ManStop( Mop_Man_t * p )
{
    Vec_WrdFree( p->vWordsIn );
    Vec_WrdFree( p->vWordsOut );
    Vec_IntFree( p->vCubes );
    Vec_IntFree( p->vFree );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Reads the file into a character buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Mop_ManLoadFile( char * pFileName )
{
    FILE * pFile;
    int nFileSize, RetValue;
    char * pContents;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        Abc_Print( -1, "Mop_ManLoadFile(): The file is unavailable (absent or open).\n" );
        return NULL;
    }
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile ); 
    if ( nFileSize == 0 )
    {
        Abc_Print( -1, "Mop_ManLoadFile(): The file is empty.\n" );
        return NULL;
    }
    pContents = ABC_ALLOC( char, nFileSize + 10 );
    rewind( pFile );
    RetValue = fread( pContents, nFileSize, 1, pFile );
    fclose( pFile );
    strcpy( pContents + nFileSize, "\n" );
    return pContents;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mop_ManReadParams( char * pBuffer, int * pnIns, int * pnOuts )
{
    char * pIns  = strstr( pBuffer, ".i " );
    char * pOuts = strstr( pBuffer, ".o " );
    char * pStr  = pBuffer; int nCubes = 0;
    if ( pIns == NULL || pOuts == NULL )
        return -1;
    *pnIns  = atoi( pIns + 2 );
    *pnOuts = atoi( pOuts + 2 );
    while ( *pStr )
        nCubes += (*pStr++ == '\n');
    return nCubes;
}
Mop_Man_t * Mop_ManRead( char * pFileName )
{
    Mop_Man_t * p;
    int nIns, nOuts, nCubes, iCube;
    char * pToken, * pBuffer = Mop_ManLoadFile( pFileName );
    if ( pBuffer == NULL )
        return NULL;
    nCubes = Mop_ManReadParams( pBuffer, &nIns, &nOuts );
    if ( nCubes == -1 )
        return NULL;
    p = Mop_ManAlloc( nIns, nOuts, nCubes );
    // get the first cube
    pToken = strtok( pBuffer, "\n" );
    while ( pToken )
    {
        while ( Mop_ManIsSpace(*pToken) )
            pToken++;
        if ( Mop_ManIsSopSymb(*pToken) )
            break;
        pToken = strtok( NULL, "\n" );
    }
    // read cubes
    for ( iCube = 0; pToken && Mop_ManIsSopSymb(*pToken); iCube++ )
    {
        char * pTokenCopy = pToken;
        int i, o, nVars[2] = {nIns, nOuts};
        word * pCube[2] = { Mop_ManCubeIn(p, iCube), Mop_ManCubeOut(p, iCube) }; 
        for ( o = 0; o < 2; o++ )
        {
            while ( Mop_ManIsSpace(*pToken) )
                pToken++;
            for ( i = 0; i < nVars[o]; i++, pToken++ )
            {
                if ( !Mop_ManIsSopSymb(*pToken) )
                {
                    printf( "Cannot read cube %d (%s).\n", iCube+1, pTokenCopy );
                    ABC_FREE( pBuffer );
                    Mop_ManStop( p );
                    return NULL;
                }
                if ( o == 1 )
                {
                    if ( *pToken == '1' )
                        Abc_TtSetBit( pCube[o], i );
                }
                else if ( *pToken == '0' )
                    Abc_TtSetBit( pCube[o], 2*i );
                else if ( *pToken == '1' )
                    Abc_TtSetBit( pCube[o], 2*i+1 );
            }
        }
        assert( iCube < nCubes );
        Vec_IntPush( p->vCubes, iCube );
        pToken = strtok( NULL, "\n" );
    }
    for ( ; iCube < 2 * nCubes; iCube++ )
        Vec_IntPush( p->vFree, iCube );
    ABC_FREE( pBuffer );
    return p;
}
void Mop_ManPrintOne( Mop_Man_t * p, int iCube )
{
    int k;
    char Symb[4] = { '-', '0', '1', '?' };
    word * pCubeIn  = Mop_ManCubeIn( p, iCube );
    word * pCubeOut = Mop_ManCubeOut( p, iCube );
    for ( k = 0; k < p->nIns; k++ )
        printf( "%c", Symb[Abc_TtGetQua(pCubeIn, k)] );
    printf( " " );
    for ( k = 0; k < p->nOuts; k++ )
        printf( "%d", Abc_TtGetBit(pCubeOut, k) );
    printf( "\n" );
}
void Mop_ManPrint( Mop_Man_t * p )
{
    int i, iCube;
    printf( ".%d\n", p->nIns );
    printf( ".%d\n", p->nOuts );
    Vec_IntForEachEntry( p->vCubes, iCube, i )
        Mop_ManPrintOne( p, iCube );
    printf( ".e\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Mop_ManCountOnes( word * pCube, int nWords )
{
    int w, Count = 0;
    for ( w = 0; w < nWords; w++ )
        Count += Abc_TtCountOnes( pCube[w] );
    return Count;
}
static inline int Mop_ManCountOutputLits( Mop_Man_t * p )
{
    int i, iCube, nOutLits = 0;
    Vec_IntForEachEntry( p->vCubes, iCube, i )
        nOutLits += Mop_ManCountOnes( Mop_ManCubeOut(p, iCube), p->nWordsOut );
    return nOutLits;
}
static inline Vec_Wec_t * Mop_ManCreateGroups( Mop_Man_t * p )
{
    int i, iCube;
    Vec_Wec_t * vGroups = Vec_WecStart( p->nIns );
    Vec_IntForEachEntry( p->vCubes, iCube, i )
        Vec_WecPush( vGroups, Mop_ManCountOnes(Mop_ManCubeIn(p, iCube), p->nWordsIn), iCube );
    return vGroups;
}
static inline int Mop_ManUnCreateGroups( Mop_Man_t * p, Vec_Wec_t * vGroups )
{
    int i, c1, iCube1;
    int nBefore = Vec_IntSize(p->vCubes);
    Vec_Int_t * vGroup; 
    Vec_IntClear( p->vCubes );
    Vec_WecForEachLevel( vGroups, vGroup, i )
        Vec_IntForEachEntry( vGroup, iCube1, c1 )
            if ( iCube1 != -1 )
                Vec_IntPush( p->vCubes, iCube1 );
    return nBefore - Vec_IntSize(p->vCubes);
}
static inline int Mop_ManCheckContain( word * pBig, word * pSmall, int nWords )
{
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( pSmall[w] != (pSmall[w] & pBig[w]) )
            return 0;
    return 1;
}
static inline void Mop_ManRemoveEmpty( Mop_Man_t * p )
{
    int w, i, k = 0, iCube;
    Vec_IntForEachEntry( p->vCubes, iCube, i )
    {
        word * pCube = Mop_ManCubeOut( p, iCube );
        for ( w = 0; w < p->nWordsOut; w++ )
            if ( pCube[w] )
                break;
        if ( w < p->nWordsOut )
            Vec_IntWriteEntry( p->vCubes, k++, iCube );
    }
    Vec_IntShrink( p->vCubes, k );
}

/**Function*************************************************************

  Synopsis    [Count how many times each variable appears in the input parts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Mop_ManCollectStats( Mop_Man_t * p )
{
    int i, v, iCube, nVars = 32 * p->nWordsIn;
    Vec_Int_t * vStats = Vec_IntStart( nVars );
    Vec_IntForEachEntry( p->vCubes, iCube, i )
    {
        word * pCube = Mop_ManCubeIn(p, iCube);
        int nOutLits = Mop_ManCountOnes( Mop_ManCubeOut(p, iCube), p->nWordsOut );
        for ( v = 0; v < nVars; v++ )
            if ( Abc_TtGetQua(pCube, v) )
                Vec_IntAddToEntry( vStats, v, nOutLits );
    }
    return vStats;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// knowing that cubes are distance-1, find the different input variable
static inline int Mop_ManFindDiffVar( word * pCube1, word * pCube2, int nWords )
{
    int w, i;
    for ( w = 0; w < nWords; w++ )
    {
        word Xor = pCube1[w] ^ pCube2[w];
        for ( i = 0; i < 32; i++ )
            if ( (Xor >> (i << 1)) & 0x3 )
                return w * 32 + i;
    }
    assert( 0 );
    return -1;
}
// check containment of input parts of two cubes
static inline int Mop_ManCheckDist1( word * pCube1, word * pCube2, int nWords )
{
    int w, fFound1 = 0;
    for ( w = 0; w < nWords; w++ )
    {
        word Xor = pCube1[w] ^ pCube2[w];
        if ( Xor == 0 ) // equal
            continue;
        if ( (Xor ^ (Xor >> 1)) & ABC_CONST(0x5555555555555555) ) // not pairs
            return 0;
        Xor &= (Xor >> 1) & ABC_CONST(0x5555555555555555);
        if ( Xor == 0 ) // not equal and not distance-1
            return 0;
        if ( fFound1 ) // distance-2 or more
            return 0;
        if ( (Xor & (Xor-1)) ) // distance-2 or more
            return 0;
        fFound1 = 1; // distance 1 so far
    }
    return fFound1;
}
// compresses cubes in the group
static inline void Map_ManGroupCompact( Vec_Int_t * vGroup )
{
    int i, Entry, k = 0;
    Vec_IntForEachEntry( vGroup, Entry, i )
        if ( Entry != -1 )
            Vec_IntWriteEntry( vGroup, k++, Entry );
    Vec_IntShrink( vGroup, k );
}
// takes cubes with identical literal count and removes duplicates
int Mop_ManRemoveIdentical( Mop_Man_t * p, Vec_Int_t * vGroup )
{
    int w, c1, c2, iCube1, iCube2, nEqual = 0;
    Vec_IntForEachEntry( vGroup, iCube1, c1 )
    if ( iCube1 != -1 )
    {
        word * pCube1Out, * pCube1 = Mop_ManCubeIn( p, iCube1 );
        Vec_IntForEachEntryStart( vGroup, iCube2, c2, c1+1 )
        if ( iCube2 != -1 )
        {
            word * pCube2Out, * pCube2 = Mop_ManCubeIn( p, iCube2 );
            if ( memcmp(pCube1, pCube2, sizeof(word)*p->nWordsIn) )
                continue;
            // merge cubes
            pCube1Out = Mop_ManCubeOut( p, iCube1 );
            pCube2Out = Mop_ManCubeOut( p, iCube2 );
            for ( w = 0; w < p->nWordsOut; w++ )
                pCube1Out[w] |= pCube2Out[w];
            Vec_IntWriteEntry( vGroup, c2, -1 );
            Vec_IntPush( p->vFree, iCube2 );
            nEqual++;
        }
    }
    if ( nEqual )
        Map_ManGroupCompact( vGroup );
    return nEqual;
}
// reduces the set of pairs
Vec_Int_t * Mop_ManCompatiblePairs( Vec_Int_t * vPairs, int nObjs )
{
    int i, Entry, Entry2;
    Vec_Int_t * vCounts = Vec_IntStart( nObjs );
    Vec_Int_t * vPairsNew = Vec_IntAlloc( Vec_IntSize(vPairs) );
    Vec_IntForEachEntry( vPairs, Entry, i )
        Vec_IntAddToEntry( vCounts, Entry, 1 );
    // include pairs which have those that appear only once
    Vec_IntForEachEntryDouble( vPairs, Entry, Entry2, i )
        if ( Vec_IntEntry(vCounts, Entry) == 1 || Vec_IntEntry(vCounts, Entry2) == 1 )
        {
            if ( Vec_IntEntry(vCounts, Entry) == 1 )
                Vec_IntPushTwo( vPairsNew, Entry, Entry2 );
            else
                Vec_IntPushTwo( vPairsNew, Entry2, Entry );
            Vec_IntWriteEntry( vCounts, Entry, -1 );
            Vec_IntWriteEntry( vCounts, Entry2, -1 );
        }
    // add those remaining pairs that are both present
    Vec_IntForEachEntryDouble( vPairs, Entry, Entry2, i )
        if ( Vec_IntEntry(vCounts, Entry) > 0 && Vec_IntEntry(vCounts, Entry2) > 0 )
        {
            Vec_IntPushTwo( vPairsNew, Entry, Entry2 );
            Vec_IntWriteEntry( vCounts, Entry, -1 );
            Vec_IntWriteEntry( vCounts, Entry2, -1 );
        }
    // add remaining pairs
    Vec_IntForEachEntryDouble( vPairs, Entry, Entry2, i )
        if ( Vec_IntEntry(vCounts, Entry) > 0 || Vec_IntEntry(vCounts, Entry2) > 0 )
        {
            if ( Vec_IntEntry(vCounts, Entry) > 0 )
                Vec_IntPushTwo( vPairsNew, Entry, Entry2 );
            else
                Vec_IntPushTwo( vPairsNew, Entry2, Entry );
            Vec_IntWriteEntry( vCounts, Entry, -1 );
            Vec_IntWriteEntry( vCounts, Entry2, -1 );
        }
    Vec_IntFree( vCounts );
    // verify the result
    if ( 0 )
    {
        Vec_Int_t * vTemp1 = Vec_IntDup( vPairs );
        Vec_Int_t * vTemp2 = Vec_IntDup( vPairsNew );
        Vec_IntUniqify( vTemp1 );
        Vec_IntUniqify( vTemp2 );
        assert( Vec_IntEqual( vTemp1, vTemp2 ) );
        Vec_IntFree( vTemp1 );
        Vec_IntFree( vTemp2 );
    }
    return vPairsNew;
}
// detects pairs of distance-1 cubes with identical outputs
Vec_Int_t * Mop_ManFindDist1Pairs( Mop_Man_t * p, Vec_Int_t * vGroup )
{
    int c1, c2, iCube1, iCube2;
    Vec_Int_t * vPairs = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vGroup, iCube1, c1 )
    {
        word * pCube1Out, * pCube1 = Mop_ManCubeIn( p, iCube1 );
        Vec_IntForEachEntryStart( vGroup, iCube2, c2, c1+1 )
        {
            word * pCube2Out, * pCube2 = Mop_ManCubeIn( p, iCube2 );
            if ( !Mop_ManCheckDist1(pCube1, pCube2, p->nWordsIn) )
                continue;
            pCube1Out = Mop_ManCubeOut( p, iCube1 );
            pCube2Out = Mop_ManCubeOut( p, iCube2 );
            if ( !memcmp(pCube1Out, pCube2Out, sizeof(word)*p->nWordsOut) )
                Vec_IntPushTwo( vPairs, c1, c2 );
        }
    }
    return vPairs;
}
// merge distance-1 with identical output part
int Mop_ManMergeDist1Pairs( Mop_Man_t * p, Vec_Int_t * vGroup, Vec_Int_t * vGroupPrev, Vec_Int_t * vStats, int nLimit )
{
    Vec_Int_t * vPairs    = Mop_ManFindDist1Pairs( p, vGroup );
    Vec_Int_t * vPairsNew = Mop_ManCompatiblePairs( vPairs, Vec_IntSize(vGroup) );
    int nCubes = Vec_IntSize(vGroup) + Vec_IntSize(vGroupPrev);
    int w, i, c1, c2, iCubeNew, iVar;
    // move cubes to the previous group
    word * pCube, * pCube1, * pCube2;
    Vec_Int_t * vToFree = Vec_IntAlloc( Vec_IntSize(vPairsNew) );
    Vec_IntForEachEntryDouble( vPairsNew, c1, c2, i )
    {
        pCube1 = Mop_ManCubeIn( p, Vec_IntEntry(vGroup, c1) );
        pCube2 = Mop_ManCubeIn( p, Vec_IntEntry(vGroup, c2) );
        assert( Mop_ManCheckDist1(pCube1, pCube2, p->nWordsIn) );

        // skip those cubes that have frequently appearing variables
        iVar = Mop_ManFindDiffVar( pCube1, pCube2, p->nWordsIn );
        if ( Vec_IntEntry( vStats, iVar ) > nLimit )
            continue;
        Vec_IntPush( vToFree, c1 );
        Vec_IntPush( vToFree, c2 );

        iCubeNew  = Vec_IntPop( p->vFree );
        pCube  = Mop_ManCubeIn( p, iCubeNew );
        for ( w = 0; w < p->nWordsIn; w++ )
            pCube[w] = pCube1[w] & pCube2[w];

        pCube  = Mop_ManCubeOut( p, iCubeNew );
        pCube1 = Mop_ManCubeOut( p, Vec_IntEntry(vGroup, c1) );
        pCube2 = Mop_ManCubeOut( p, Vec_IntEntry(vGroup, c2) );

        assert( !memcmp(pCube1, pCube2, sizeof(word)*p->nWordsOut) );
        for ( w = 0; w < p->nWordsOut; w++ )
            pCube[w] = pCube1[w];

        Vec_IntPush( vGroupPrev, iCubeNew );
    }
//    Vec_IntForEachEntry( vPairsNew, c1, i )
    Vec_IntForEachEntry( vToFree, c1, i )
    {
        if ( Vec_IntEntry(vGroup, c1) == -1 )
            continue;
        Vec_IntPush( p->vFree, Vec_IntEntry(vGroup, c1) );
        Vec_IntWriteEntry( vGroup, c1, -1 );
    }
    Vec_IntFree( vToFree );
    if ( Vec_IntSize(vPairsNew) > 0 )
        Map_ManGroupCompact( vGroup );
    Vec_IntFree( vPairs );
    Vec_IntFree( vPairsNew );
    return nCubes - Vec_IntSize(vGroup) - Vec_IntSize(vGroupPrev);
}
// merge distance-1 with contained output part
int Mop_ManMergeDist1Pairs2( Mop_Man_t * p, Vec_Int_t * vGroup, Vec_Int_t * vGroupPrev )
{
    int w, c1, c2, iCube1, iCube2, Count = 0;
    Vec_IntForEachEntry( vGroup, iCube1, c1 )
    if ( iCube1 != -1 )
    {
        word * pCube1Out, * pCube1 = Mop_ManCubeIn( p, iCube1 );
        Vec_IntForEachEntryStart( vGroup, iCube2, c2, c1+1 )
        if ( iCube2 != -1 )
        {
            word * pCube2Out, * pCube2 = Mop_ManCubeIn( p, iCube2 );
            if ( !Mop_ManCheckDist1(pCube1, pCube2, p->nWordsIn) )
                continue;
            pCube1Out = Mop_ManCubeOut( p, iCube1 );
            pCube2Out = Mop_ManCubeOut( p, iCube2 );
            assert( memcmp(pCube1Out, pCube2Out, sizeof(word)*p->nWordsOut) );
            if ( Mop_ManCheckContain(pCube1Out, pCube2Out, p->nWordsOut) ) // pCube1 has more outputs
            {
                // update the input part
                for ( w = 0; w < p->nWordsIn; w++ )
                    pCube2[w] &= pCube1[w];
                // sharp the output part
                for ( w = 0; w < p->nWordsOut; w++ )
                    pCube1Out[w] &= ~pCube2Out[w];
                // move to another group
                Vec_IntPush( vGroupPrev, iCube2 );
                Vec_IntWriteEntry( vGroup, c2, -1 );
                Count++;
            }
            else if ( Mop_ManCheckContain(pCube2Out, pCube1Out, p->nWordsOut) ) // pCube2 has more outputs
            {
                // update the input part
                for ( w = 0; w < p->nWordsIn; w++ )
                    pCube1[w] &= pCube2[w];
                // sharp the output part
                for ( w = 0; w < p->nWordsOut; w++ )
                    pCube2Out[w] &= ~pCube1Out[w];
                // move to another group
                Vec_IntPush( vGroupPrev, iCube1 );
                Vec_IntWriteEntry( vGroup, c1, -1 );
                Count++;
            }
        }
    }
    if ( Count )
        Map_ManGroupCompact( vGroup );
    return Count;
}
int Mop_ManMergeDist1All( Mop_Man_t * p, Vec_Wec_t * vGroups, Vec_Int_t * vStats, int nLimit )
{
    Vec_Int_t * vGroup;
    int i, nEqual, nReduce, Count = 0;
    Vec_WecForEachLevelReverse( vGroups, vGroup, i )
    {
        if ( Vec_IntSize(vGroup) == 0 )
            continue;
        if ( i == 0 )
        {
            printf( "Detected constant-1 cover.\n" );
            fflush( stdout );
            return -1;
        }
        nEqual  = Mop_ManRemoveIdentical( p, vGroup );
        nReduce = Mop_ManMergeDist1Pairs( p, vGroup, Vec_WecEntry(vGroups, i-1), vStats, nLimit );
        //Mop_ManMergeDist1Pairs2( p, vGroup, Vec_WecEntry(vGroups, i-1) );
        Count  += nEqual + nReduce;
        //printf( "Group %3d : Equal =%5d. Reduce =%5d.\n", i, nEqual, nReduce );
    }
    return Count;
}
// reduce contained cubes
int Mop_ManMergeContainTwo( Mop_Man_t * p, Vec_Int_t * vGroup, Vec_Int_t * vGroup2 )
{
    int w, c1, c2, iCube1, iCube2, Count = 0;
    Vec_IntForEachEntry( vGroup, iCube1, c1 )
    {
        word * pCube1Out, * pCube1 = Mop_ManCubeIn( p, iCube1 );
        Vec_IntForEachEntry( vGroup2, iCube2, c2 )
        if ( iCube2 != -1 )
        {
            word * pCube2Out, * pCube2 = Mop_ManCubeIn( p, iCube2 );
            if ( !Mop_ManCheckContain(pCube2, pCube1, p->nWordsIn) )
                continue;
            pCube1Out = Mop_ManCubeOut( p, iCube1 );
            pCube2Out = Mop_ManCubeOut( p, iCube2 );
            for ( w = 0; w < p->nWordsOut; w++ )
                pCube2Out[w] &= ~pCube1Out[w];
            for ( w = 0; w < p->nWordsOut; w++ )
                if ( pCube2Out[w] )
                    break;
            if ( w < p->nWordsOut ) // has output literals
                continue;
            // remove larger cube
            Vec_IntWriteEntry( vGroup2, c2, -1 );
            Vec_IntPush( p->vFree, iCube2 );
            Count++;
        }
    }
    if ( Count )
        Map_ManGroupCompact( vGroup2 );
    return Count;
}
int Mop_ManMergeContainAll( Mop_Man_t * p, Vec_Wec_t * vGroups )
{
    Vec_Int_t * vGroup, * vGroup2;
    int i, k, Count = 0;
    Vec_WecForEachLevel( vGroups, vGroup, i )
    {
        Count += Mop_ManRemoveIdentical( p, vGroup );
        Vec_WecForEachLevelStart( vGroups, vGroup2, k, i+1 )
            Count += Mop_ManMergeContainTwo( p, vGroup, vGroup2 );
    }
    return Count;
}
void Mop_ManReduce2( Mop_Man_t * p )
{
    abctime clk = Abc_Clock();
    int nCubes  = Vec_IntSize(p->vCubes);
    Vec_Int_t * vStats = Mop_ManCollectStats( p );
    Vec_Wec_t * vGroups = Mop_ManCreateGroups( p );
    int nLimit    = ABC_INFINITY; // 5 * Vec_IntSum(vStats) / Vec_IntSize(vStats) + 1;
    int nOutLits  = Mop_ManCountOutputLits( p );
    int Count1    = Mop_ManMergeContainAll( p, vGroups );
    int Count2    = Mop_ManMergeDist1All( p, vGroups, vStats, nLimit );
    int Count3    = Mop_ManMergeContainAll( p, vGroups );
    int Count4    = Mop_ManMergeDist1All( p, vGroups, vStats, nLimit );
    int Count5    = Mop_ManMergeContainAll( p, vGroups );
    int Removed   = Mop_ManUnCreateGroups( p, vGroups );
    int nOutLits2 = Mop_ManCountOutputLits( p );
    Vec_WecFree( vGroups );
//Vec_IntPrint( vStats );
    Vec_IntFree( vStats );
    assert( Removed == Count1 + Count2 + Count3 );
    // report
    printf( "Cubes: %d -> %d.  C = %d.  M = %d.  C = %d.  M = %d.  C = %d.  Output lits: %d -> %d.   ", 
        nCubes, Vec_IntSize(p->vCubes), Count1, Count2, Count3, Count4, Count5, nOutLits, nOutLits2 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mop_ManReduce( Mop_Man_t * p )
{
    abctime clk = Abc_Clock();
    Vec_Int_t * vGroup, * vGroup2;
    int i, k, nOutLits, nOutLits2, nEqual = 0, nContain = 0;
    Vec_Wec_t * vGroups = Mop_ManCreateGroups( p );
    // initial stats
    nOutLits = Mop_ManCountOutputLits( p );
    // check identical cubes within each group
    Vec_WecForEachLevel( vGroups, vGroup, i )
        nEqual += Mop_ManRemoveIdentical( p, vGroup );
    // check contained cubes
    Vec_WecForEachLevel( vGroups, vGroup, i )
        Vec_WecForEachLevelStart( vGroups, vGroup2, k, i+1 )
            nContain += Mop_ManMergeContainTwo( p, vGroup, vGroup2 );
    // final stats
    nOutLits2 = Mop_ManCountOutputLits( p );
    Mop_ManUnCreateGroups( p, vGroups );
    Vec_WecFree( vGroups );
    // report
    printf( "Total = %d. Reduced %d equal and %d contained cubes. Output lits: %d -> %d.   ", Vec_IntSize(p->vCubes), nEqual, nContain, nOutLits, nOutLits2 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Mop_ManCubeCount( Mop_Man_t * p )
{
    int i, k, iCube;
    Vec_Wec_t * vOuts = Vec_WecStart( p->nOuts );
    Vec_IntForEachEntry( p->vCubes, iCube, i )
    if ( iCube != -1 )
    {
        word * pCube = Mop_ManCubeOut( p, iCube );
        for ( k = 0; k < p->nOuts; k++ )
            if ( Abc_TtGetBit( pCube, k ) )
                Vec_WecPush( vOuts, k, iCube );
    }
    return vOuts;
}
Abc_Ntk_t * Mop_ManDerive( Mop_Man_t * p, char * pFileName )
{
    int i, k, c, iCube; 
    char Symb[4] = { '-', '0', '1', '?' };     // cube symbols
    Vec_Str_t * vSop  = Vec_StrAlloc( 1000 );  // storage for one SOP
    Vec_Wec_t * vOuts = Mop_ManCubeCount( p ); // cube count for each output
    Abc_Ntk_t * pNtk  = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    pNtk->pName = Extra_UtilStrsav( pFileName );
    pNtk->pSpec = Extra_UtilStrsav( pFileName );
    for ( i = 0; i < p->nIns; i++ )
        Abc_NtkCreatePi(pNtk);
    for ( i = 0; i < p->nOuts; i++ )
    {
        Vec_Int_t * vThis = Vec_WecEntry( vOuts, i );
        Abc_Obj_t * pPo   = Abc_NtkCreatePo(pNtk);
        Abc_Obj_t * pNode = Abc_NtkCreateNode(pNtk);
        Abc_ObjAddFanin( pPo, pNode );
        if ( Vec_IntSize(vThis) == 0 )
        {
            pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, " 0\n" );
            continue;
        }
        for ( k = 0; k < p->nIns; k++ )
            Abc_ObjAddFanin( pNode, Abc_NtkPi(pNtk, k) );
        Vec_StrClear( vSop );
        Vec_IntForEachEntry( vThis, iCube, c )
        {
            word * pCube = Mop_ManCubeIn( p, iCube );
            for ( k = 0; k < p->nIns; k++ )
                Vec_StrPush( vSop, Symb[Abc_TtGetQua(pCube, k)] );
            Vec_StrAppend( vSop, " 1\n" );
        }
        Vec_StrPush( vSop, '\0' );
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, Vec_StrArray(vSop) );
    }
    Vec_StrFree( vSop );
    Vec_WecFree( vOuts );
    Abc_NtkAddDummyPiNames( pNtk );
    Abc_NtkAddDummyPoNames( pNtk );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Mop_ManTest( char * pFileName, int fMerge, int fVerbose )
{
    Abc_Ntk_t * pNtk = NULL;
    Mop_Man_t * p = Mop_ManRead( pFileName );
    if ( p == NULL )
        return NULL;
    Mop_ManRemoveEmpty( p );
    //Mop_ManPrint( p );
    if ( fMerge )
        Mop_ManReduce2( p );
    else
        Mop_ManReduce( p );
    //Mop_ManPrint( p );
    pNtk = Mop_ManDerive( p, pFileName );
    Mop_ManStop( p );
    return pNtk;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_IMPL_END

