/**CFile****************************************************************

  FileName    [dauNpn2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Functional enumeration.]

  Synopsis    [Functional enumeration.]

  Author      [Siang-Yun Lee]
  
  Affiliation [National Taiwan University]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauNpn2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "misc/util/utilTruth.h"
#include "misc/extra/extra.h"
#include "aig/gia/gia.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Dtt_Man_t_ Dtt_Man_t;
struct Dtt_Man_t_
{
    int            nVars;          // variable number
    int            nPerms;         // number of permutations
    int            nComps;         // number of complementations
    int *          pPerms;         // permutations
    int *          pComps;         // complementations
    word *         pPres;          // function marks
    Vec_Int_t *    vFanins;        // node fanins
    Vec_Int_t *    vTruths;        // node truth tables
    Vec_Int_t *    vConfigs;       // configurations
    Vec_Int_t *    vClasses;       // node NPN classes
    Vec_Int_t *    vTruthNpns;     // truth tables of the classes
    Vec_Wec_t *    vFunNodes;      // nodes by NPN class
    Vec_Int_t *    vTemp;          // temporary
    Vec_Int_t *    vTemp2;         // temporary
    unsigned       FunMask;        // function mask
    unsigned       CmpMask;        // function mask
    unsigned       BinMask;        // hash mask
    unsigned *     pBins;          // hash bins
    Vec_Int_t *    vUsedBins;      // used bins
    int            Counts[32];     // node counts
    int            nClasses;       // count of classes
    unsigned *     pTable;         // mapping of funcs into their classes
    int *          pNodes;         // the number of nodes in min-node network
    int *          pTimes;         // the number of different min-node networks
    char *         pVisited;       // visited classes
    Vec_Int_t *    vVisited;       // the number of visited classes
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Parse one formula into a truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Dau_ParseFormulaEndToken( char * pForm )
{
    int Counter = 0;
    char * pThis;
    for ( pThis = pForm; *pThis; pThis++ )
    {
        if ( *pThis == '~' )
            continue;
        if ( *pThis == '(' )
            Counter++;
        else if ( *pThis == ')' )
            Counter--;
        if ( Counter == 0 )
            return pThis + 1;
    }
    assert( 0 );
    return NULL;
}
word Dau_ParseFormula_rec( char * pBeg, char * pEnd )
{
    word iFans[2], Res, Oper = -1;
    char * pEndNew;
    int fCompl = 0;
    while ( pBeg[0] == '~' )
    {
        pBeg++;
        fCompl ^= 1;
    }
    if ( pBeg + 1 == pEnd )
    {
        if ( pBeg[0] >= 'a' && pBeg[0] <= 'f' )
            return fCompl ? ~s_Truths6[pBeg[0] - 'a'] : s_Truths6[pBeg[0] - 'a'];
        assert( 0 );
        return -1;
    }
    if ( pBeg[0] == '(' )
    {
        pEndNew = Dau_ParseFormulaEndToken( pBeg );
        if ( pEndNew == pEnd )
        {
            assert( pBeg[0] == '(' );
            assert( pBeg[pEnd-pBeg-1] == ')' );
            Res = Dau_ParseFormula_rec( pBeg + 1, pEnd - 1 );
            return fCompl ? ~Res : Res;
        }
    }
    // get first part
    pEndNew  = Dau_ParseFormulaEndToken( pBeg );
    iFans[0] = Dau_ParseFormula_rec( pBeg, pEndNew );
    iFans[0] = fCompl ? ~iFans[0] : iFans[0];
    Oper     = pEndNew[0];
    // get second part
    pBeg     = pEndNew + 1;
    pEndNew  = Dau_ParseFormulaEndToken( pBeg );
    iFans[1] = Dau_ParseFormula_rec( pBeg, pEndNew );
    // derive the formula
    if ( Oper == '&' )
        return iFans[0] & iFans[1];
    if ( Oper == '^' )
        return iFans[0] ^ iFans[1];
    assert( 0 );
    return -1;
}
word Dau_ParseFormula( char * p )
{
    return Dau_ParseFormula_rec( p, p + strlen(p) );
}
void Dau_ParseFormulaTest()
{
    char * p = "~((~~d&~(~~b&c))^(~(~a&~d)&~(~c^~b)))";
    word r = ABC_CONST(0x037d037d037d037d);
    word t = Dau_ParseFormula( p );
    assert( r == t );
}

 
/**Function*************************************************************

  Synopsis    [Parse one formula into a AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_ParseFormulaAig_rec( Gia_Man_t * p, char * pBeg, char * pEnd )
{
    int iFans[2], Res, Oper = -1;
    char * pEndNew;
    int fCompl = 0;
    while ( pBeg[0] == '~' )
    {
        pBeg++;
        fCompl ^= 1;
    }
    if ( pBeg + 1 == pEnd )
    {
        if ( pBeg[0] >= 'a' && pBeg[0] <= 'f' )
            return Abc_Var2Lit( 1 + pBeg[0] - 'a', fCompl );
        assert( 0 );
        return -1;
    }
    if ( pBeg[0] == '(' )
    {
        pEndNew = Dau_ParseFormulaEndToken( pBeg );
        if ( pEndNew == pEnd )
        {
            assert( pBeg[0] == '(' );
            assert( pBeg[pEnd-pBeg-1] == ')' );
            Res = Dau_ParseFormulaAig_rec( p, pBeg + 1, pEnd - 1 );
            return Abc_LitNotCond( Res, fCompl );
        }
    }
    // get first part
    pEndNew  = Dau_ParseFormulaEndToken( pBeg );
    iFans[0] = Dau_ParseFormulaAig_rec( p, pBeg, pEndNew );
    iFans[0] = Abc_LitNotCond( iFans[0], fCompl );
    Oper     = pEndNew[0];
    // get second part
    pBeg     = pEndNew + 1;
    pEndNew  = Dau_ParseFormulaEndToken( pBeg );
    iFans[1] = Dau_ParseFormulaAig_rec( p, pBeg, pEndNew );
    // derive the formula
    if ( Oper == '&' )
        return Gia_ManHashAnd( p, iFans[0], iFans[1] );
    if ( Oper == '^' )
        return Gia_ManHashXor( p, iFans[0], iFans[1] );
    assert( 0 );
    return -1;
}
int Dau_ParseFormulaAig( Gia_Man_t * p, char * pStr )
{
    return Dau_ParseFormulaAig_rec( p, pStr, pStr + strlen(pStr) );
}
Gia_Man_t * Dau_ParseFormulaAigTest()
{
    int i;
    char * pStr = "~((~~d&~(~~b&c))^(~(~a&~d)&~(~c^~b)))";
    Gia_Man_t * p = Gia_ManStart( 1000 );
    p->pName = Abc_UtilStrsav( "func_enum_aig" );
    Gia_ManHashAlloc( p );
    for ( i = 0; i < 5; i++ )
        Gia_ManAppendCi( p );
    Gia_ManAppendCo( p, Dau_ParseFormulaAig(p, pStr) );
    return p;
}


/**Function*************************************************************

  Synopsis    [Verify one file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_VerifyFile( char * pFileName )
{
    char Buffer[1000];
    unsigned uTruth, uTruth2; 
    int nFails = 0, nLines = 0;
    FILE * pFile = fopen( pFileName, "rb" );
    while ( fgets( Buffer, 1000, pFile ) )
    {
        if ( Buffer[strlen(Buffer)-1] == '\n' )
            Buffer[strlen(Buffer)-1] = 0;
        if ( Buffer[strlen(Buffer)-1] == '\r' )
            Buffer[strlen(Buffer)-1] = 0;
        Extra_ReadHexadecimal( &uTruth2, Buffer, 5 );
        uTruth = (unsigned)Dau_ParseFormula( Buffer + 11 );
        if ( uTruth != uTruth2 )
            printf( "Verification failed in line %d:  %s\n", nLines, Buffer ), nFails++;
        nLines++;
    }
    printf( "Verification succeeded for %d functions and failed for %d functions.\n", nLines-nFails, nFails );
}
void Dau_VerifyFileTest()
{
    char * pFileName = "lib4var.txt";
    Dau_VerifyFile( pFileName );
}


/**Function*************************************************************

  Synopsis    [Create AIG for one file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Dau_ConstructAigFromFile( char * pFileName )
{
    char Buffer[1000];
    int i, nLines = 0;
    FILE * pFile = fopen( pFileName, "rb" );
    Gia_Man_t * p = Gia_ManStart( 1000 );
    p->pName = Abc_UtilStrsav( "func_enum_aig" );
    Gia_ManHashAlloc( p );
    for ( i = 0; i < 5; i++ )
        Gia_ManAppendCi( p );
    while ( fgets( Buffer, 1000, pFile ) )
    {
        if ( Buffer[strlen(Buffer)-1] == '\n' )
            Buffer[strlen(Buffer)-1] = 0;
        if ( Buffer[strlen(Buffer)-1] == '\r' )
            Buffer[strlen(Buffer)-1] = 0;
        Gia_ManAppendCo( p, Dau_ParseFormulaAig(p, Buffer + 11) );
        nLines++;
    }
    printf( "Finish constructing AIG for %d structures.\n", nLines );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Dau_ReadFile2( char * pFileName, int nSizeW )
{
    abctime clk = Abc_Clock();
    unsigned * p;
    int RetValue;
    FILE * pFile = fopen( pFileName, "rb" );
    if (pFile == NULL) return NULL;
    p = (unsigned *)ABC_CALLOC(word, nSizeW);
    RetValue = pFile ? fread( p, sizeof(word), nSizeW, pFile ) : 0;
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
void Dtt_ManRenum( int nVars, unsigned * pTable, int * pnClasses )
{
    unsigned i, Limit = 1 << ((1 << nVars)-1), Count = 0;
    for ( i = 0; i < Limit; i++ )
        if ( pTable[i] == i )
            pTable[i] = Count++;
        else 
        {
            assert( pTable[i] < i );
            pTable[i] = pTable[pTable[i]];
        }
    printf( "The total number of NPN classes = %d.\n", Count );
    *pnClasses = Count;
}
unsigned * Dtt_ManLoadClasses( int nVars, int * pnClasses )
{
    extern void Dau_TruthEnum(int nVars);
    unsigned * pTable = NULL;
    int nSizeLog = (1<<nVars) -2;
    int nSizeW = 1 << nSizeLog;
    char pFileName[200];
    sprintf( pFileName, "tableW%d.data", nSizeLog );
    pTable = Dau_ReadFile2( pFileName, nSizeW );
    if (pTable == NULL)
    {
        Dau_TruthEnum(nVars);
        pTable = Dau_ReadFile2( pFileName, nSizeW );
    }
    Dtt_ManRenum( nVars, pTable, pnClasses );
    return pTable;
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dtt_ManAddVisited( Dtt_Man_t * p, unsigned Truth2, int n )
{
    unsigned Truth = Truth2 & p->CmpMask ? ~Truth2 : Truth2;
    unsigned Class = p->pTable[Truth & p->FunMask];
    assert( Class < (unsigned)p->nClasses );
    if ( p->pNodes[Class] < n )
        return;
    assert( p->pNodes[Class] == n );
    if ( p->pVisited[Class] )
        return;
    p->pVisited[Class] = 1;
    Vec_IntPush( p->vVisited, Class );
}
void Dtt_ManProcessVisited( Dtt_Man_t * p )
{
    int i, Class;
    Vec_IntForEachEntry( p->vVisited, Class, i )
    {
        assert( p->pVisited[Class] );
        p->pVisited[Class] = 0;
        p->pTimes[Class]++;
    }
    Vec_IntClear( p->vVisited );
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dtt_Man_t * Dtt_ManAlloc( int nVars, int fMulti )
{
    Dtt_Man_t * p = ABC_CALLOC( Dtt_Man_t, 1 );
    p->nVars      = nVars;
    p->nPerms     = Extra_Factorial( nVars );
    p->nComps     = 1 << nVars;
    p->pPerms     = Extra_PermSchedule( nVars );
    p->pComps     = Extra_GreyCodeSchedule( nVars );
    p->pPres      = ABC_CALLOC( word, 1 << (p->nComps - 7) );
    p->vFanins    = Vec_IntAlloc( 2*617000 );
    p->vTruths    = Vec_IntAlloc( 617000 );
    p->vConfigs   = Vec_IntAlloc( 617000 );
    p->vClasses   = Vec_IntAlloc( 617000 );
    p->vTruthNpns = Vec_IntAlloc( 617000 );
    p->vFunNodes  = Vec_WecStart( 16 );
    p->vTemp      = Vec_IntAlloc( 4000 );
    p->vTemp2     = Vec_IntAlloc( 4000 );
    p->FunMask    = nVars == 5 ?      ~0 : (nVars == 4 ?  0xFFFF :   0xFF);
    p->CmpMask    = nVars == 5 ? 1 << 31 : (nVars == 4 ? 1 << 15 : 1 << 7);
    p->BinMask    = 0x3FFF;
    p->pBins      = ABC_FALLOC( unsigned, p->BinMask + 1 );
    p->vUsedBins  = Vec_IntAlloc( 4000 );
    if ( !fMulti ) return p;
    p->pTable     = Dtt_ManLoadClasses( p->nVars, &p->nClasses );
    p->pNodes     = ABC_CALLOC( int, p->nClasses );
    p->pTimes     = ABC_CALLOC( int, p->nClasses );
    p->pVisited   = ABC_CALLOC( char, p->nClasses );
    p->vVisited   = Vec_IntAlloc( 1000 );
    return p;
}
void Dtt_ManFree( Dtt_Man_t * p )
{
    Vec_IntFreeP( &p->vVisited );
    ABC_FREE( p->pTable );
    ABC_FREE( p->pNodes );
    ABC_FREE( p->pTimes );
    ABC_FREE( p->pVisited );
    Vec_IntFreeP( &p->vFanins );
    Vec_IntFreeP( &p->vTruths );
    Vec_IntFreeP( &p->vConfigs );
    Vec_IntFreeP( &p->vClasses );
    Vec_IntFreeP( &p->vTruthNpns );
    Vec_WecFreeP( &p->vFunNodes );
    Vec_IntFreeP( &p->vTemp );
    Vec_IntFreeP( &p->vTemp2 );
    Vec_IntFreeP( &p->vUsedBins );
    ABC_FREE( p->pPerms );
    ABC_FREE( p->pComps );
    ABC_FREE( p->pPres );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Collect representatives of the same class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Dtt_ManHashKey( Dtt_Man_t * p, unsigned Truth )
{
    static unsigned s_P[4] = { 1699, 5147, 7103, 8147 };
    unsigned char * pD = (unsigned char*)&Truth;
    return pD[0] * s_P[0] + pD[1] * s_P[1] + pD[2] * s_P[2] + pD[3] * s_P[3];
}
int Dtt_ManCheckHash( Dtt_Man_t * p, unsigned Truth )
{
    unsigned Hash = Dtt_ManHashKey(p, Truth);
    unsigned * pSpot = p->pBins + (Hash & p->BinMask);
    for ( ; ~*pSpot; Hash++, pSpot = p->pBins + (Hash & p->BinMask) )
        if ( *pSpot == Truth ) // equal
            return 0;
    Vec_IntPush( p->vUsedBins, pSpot - p->pBins );
    *pSpot = Truth;
    return 1;
}
Vec_Int_t * Dtt_ManCollect( Dtt_Man_t * p, unsigned Truth, Vec_Int_t * vFuns )
{
    int i, k, Entry;
    word tCur = ((word)Truth << 32) | (word)Truth;
    Vec_IntClear( vFuns );
    for ( i = 0; i < p->nPerms; i++ )
    {
        for ( k = 0; k < p->nComps; k++ )
        {
//            unsigned tTemp = (unsigned)(tCur & 1 ? ~tCur : tCur);
            unsigned tTemp = (unsigned)(tCur & p->CmpMask ? ~tCur : tCur);
            if ( Dtt_ManCheckHash( p, tTemp ) )
                Vec_IntPush( vFuns, tTemp );
            tCur = Abc_Tt6Flip( tCur, p->pComps[k] );
        }
        tCur = Abc_Tt6SwapAdjacent( tCur, p->pPerms[i] );
    }
    assert( tCur == (((word)Truth << 32) | (word)Truth) );
    // clean hash table
    Vec_IntForEachEntry( p->vUsedBins, Entry, i )
        p->pBins[Entry] = ~0;
    Vec_IntClear( p->vUsedBins );
    //printf( "%d ", Vec_IntSize(vFuns) );
    return vFuns;
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dtt_ManGetFun( Dtt_Man_t * p, unsigned tFun, int n )
{
    unsigned Class;
    tFun = tFun & p->CmpMask ? ~tFun : tFun;
    //return Abc_TtGetBit( p->pPres, tFun & p->FunMask );
    if ( !Abc_TtGetBit( p->pPres, tFun & p->FunMask ) ) return 0;
    if ( p->pTable == NULL ) return 1;

    Class = p->pTable[tFun & p->FunMask];
    assert( Class < (unsigned)p->nClasses );
    if ( p->pNodes[Class] < n )
        return 1;
    assert( p->pNodes[Class] == n );
    if ( p->pVisited[Class] )
        return 1;
    
    return 0;
}
static inline void Dtt_ManSetFun( Dtt_Man_t * p, unsigned tFun )
{
    tFun = tFun & p->CmpMask ? ~tFun : tFun;
    //assert( !Dtt_ManGetFun(p, fFun & p->FunMask );
    Abc_TtSetBit( p->pPres, tFun & p->FunMask );
}
void Dtt_ManAddFunction( Dtt_Man_t * p, int n, int FanI, int FanJ, int Type, unsigned Truth )
{
    Vec_Int_t * vFuncs = Dtt_ManCollect( p, Truth, p->vTemp2 );
    unsigned Min = Vec_IntFindMin( vFuncs ); 
    int i, nObjs = Vec_IntSize(p->vFanins)/2;
    int nNodesI = 0xF & (Vec_IntEntry(p->vConfigs, FanI) >> 3);
    int nNodesJ = 0xF & (Vec_IntEntry(p->vConfigs, FanJ) >> 3);
    int nNodes  = nNodesI + nNodesJ + 1;
    assert( nObjs == Vec_IntSize(p->vTruths) );
    assert( nObjs == Vec_IntSize(p->vConfigs) );
    assert( nObjs == Vec_IntSize(p->vClasses) );
    Vec_WecPush( p->vFunNodes, n, nObjs );
    Vec_IntPushTwo( p->vFanins, FanI, FanJ );
    Vec_IntPush( p->vTruths, Truth );
    Vec_IntPush( p->vConfigs, (nNodes << 3) | Type );
    Vec_IntPush( p->vClasses, Vec_IntSize(p->vTruthNpns) );
    Vec_IntPush( p->vTruthNpns, Min );
    Vec_IntForEachEntry( vFuncs, Min, i )
        Dtt_ManSetFun( p, Min );
    assert( nNodes < 32 );
    p->Counts[nNodes]++;        

    if ( p->pTable == NULL )
        return;
    Truth = Truth & p->CmpMask ? ~Truth : Truth;
    Truth &= p->FunMask;
    //assert( p->pNodes[p->pTable[Truth]] == 0 );
    p->pNodes[p->pTable[Truth]] = n;
}

int Dtt_PrintStats( int nNodes, int nVars, Vec_Wec_t * vFunNodes, word nSteps, abctime clk, int fDelay, word nMultis )
{
    int nNew = Vec_IntSize(Vec_WecEntry(vFunNodes, nNodes));
    printf("%c =%2d  |  ",     fDelay ? 'D':'N', nNodes );
    printf("C =%12.0f  |  ",   (double)(iword)nSteps );
    printf("New%d =%10d   ",   nVars, nNew + (int)(nNodes==0) );
    printf("All%d =%10d  |  ", nVars, Vec_WecSizeSize(vFunNodes)+1 );
    printf("Multi =%10d  |  ", (int)nMultis );
    Abc_PrintTime( 1, "Time",  Abc_Clock() - clk );
    //Abc_Print(1, "%9.2f sec\n", 1.0*(Abc_Clock() - clk)/(CLOCKS_PER_SEC));
    fflush(stdout);
    return nNew;
}
void Dtt_PrintDistrib( Dtt_Man_t * p )
{
    int i;
    printf( "NPN classes for each node count (N):\n" );
    for ( i = 0; i < 32; i++ )
        if ( p->Counts[i] )
            printf( "N = %2d : NPN = %6d\n", i, p->Counts[i] );
}
void Dtt_PrintMulti2( Dtt_Man_t * p )
{
    int i, n;
    for ( n = 0; n <= 7; n++ )
    {
        printf( "n=%d : ", n);
        for ( i = 0; i < p->nClasses; i++ )
            if ( p->pNodes[i] == n )
                printf( "%d ", p->pTimes[i] );
        printf( "\n" );
    }
}
void Dtt_PrintMulti1( Dtt_Man_t * p )
{
    int i, n, Entry, Count, Prev;
    for ( n = 0; n < 16; n++ )
    {
        Vec_Int_t * vTimes = Vec_IntAlloc( 100 );
        Vec_Int_t * vUsed  = Vec_IntAlloc( 100 );
        for ( i = 0; i < p->nClasses; i++ )
            if ( p->pNodes[i] == n )
                Vec_IntPush( vTimes, p->pTimes[i] );
        if ( Vec_IntSize(vTimes) == 0 )
        {
            Vec_IntFree(vTimes);
            Vec_IntFree(vUsed);
            break;
        }
        Vec_IntSort( vTimes, 0 );
        Count = 1;
        Prev = Vec_IntEntry( vTimes, 0 );
        Vec_IntForEachEntryStart( vTimes, Entry, i, 1 )
            if ( Prev == Entry )
                Count++;
            else
            {
                assert( Prev < Entry );
                Vec_IntPushTwo( vUsed, Prev, Count );
                Count = 1;
                Prev = Entry;
            }
        if ( Count > 0 )
            Vec_IntPushTwo( vUsed, Prev, Count );
        printf( "n=%d : ", n);
        Vec_IntForEachEntryDouble( vUsed, Prev, Entry, i )
            printf( "%d=%d ", Prev, Entry );
        printf( "\n" );
        Vec_IntFree( vTimes );
        Vec_IntFree( vUsed );
    }
}
void Dtt_PrintMulti( Dtt_Man_t * p )
{
    int n, Counts[13][15] = {{0}};
    for ( n = 0; n < 13; n++ )
    {
        int i, Total = 0, Count = 0;
        for ( i = 0; i < p->nClasses; i++ )
            if ( p->pNodes[i] == n )
            {
                int Log = Abc_Base2Log(p->pTimes[i]);
                assert( Log < 15 );
                if ( p->pTimes[i] < 2 )
                    Counts[n][0]++;
                else
                    Counts[n][Log]++;
                Total += p->pTimes[i];
                Count++;
            }
        if ( Count == 0 )
            break;
        printf( "n=%2d : ", n );
        printf( "All = %7d  ", Count );
        printf( "Ave = %6.2f  ", 1.0*Total/Count );
        for ( i = 0; i < 15; i++ )
            if ( Counts[n][i] )
                printf( "%6d", Counts[n][i] );
            else
                printf( "%6s", "" );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
typedef struct Dtt_FunImpl_t_ Dtt_FunImpl_t;
struct Dtt_FunImpl_t_
{
    int Type;
    int NP1;     // 4 bits for an input (NPPP)
    int FI1;     // the id (in vLibFun) of the first fanin function
    int NP2;
    int FI2;     // the id (in vLibFun) of the second fanin function
};

void Dtt_FunImplFI2Str( int FI, int NP, Vec_Int_t* vLibFun, char* str )
{
    int i, P[5], N[5];
    for ( i = 0; i < 5; i++ )
    {
        P[i] = NP & 0x7;
        NP = NP >> 3;
        N[i] = NP & 0x1;
        NP = NP >> 1 ;
    }
    sprintf( str, "[%08x(%03d),%d%d%d%d%d,%d%d%d%d%d]", Vec_IntEntry( vLibFun, FI ), FI, P[0], P[1], P[2], P[3], P[4], N[0], N[1], N[2], N[3], N[4] ); 
}

void Dtt_FunImpl2Str( int Type, char* sFI1, char* sFI2, char* str )
{
    // 0: 1&2, 1: ~1&2, 2: 1&~2, 3: 1|2 = ~(~1&~2), 4: 1^2
    // 5: ~(1&2), 6: ~(~1&2), 7: ~(1&~2), 8: ~1&~2, 9: ~(1^2)
    switch( Type )
    {
        case 0: sprintf( str, "(%s&%s)", sFI1, sFI2 ); break;
        case 1: sprintf( str, "(~%s&%s)", sFI1, sFI2 ); break;
        case 2: sprintf( str, "(%s&~%s)", sFI1, sFI2 ); break;
        case 3: sprintf( str, "~(~%s&~%s)", sFI1, sFI2 ); break;
        case 4: sprintf( str, "(%s^%s)", sFI1, sFI2 ); break;
        case 5: sprintf( str, "~(%s&%s)", sFI1, sFI2 ); break;
        case 6: sprintf( str, "~(~%s&%s)", sFI1, sFI2 ); break;
        case 7: sprintf( str, "~(%s&~%s)", sFI1, sFI2 ); break;
        case 8: sprintf( str, "(~%s&~%s)", sFI1, sFI2 ); break;
        case 9: sprintf( str, "~(%s^%s)", sFI1, sFI2 ); break;
        /*case 0: sprintf( str, " ( %s& %s)", sFI1, sFI2 ); break;
        case 1: sprintf( str, " (~%s& %s)", sFI1, sFI2 ); break;
        case 2: sprintf( str, " ( %s&~%s)", sFI1, sFI2 ); break;
        case 3: sprintf( str, "~(~%s&~%s)", sFI1, sFI2 ); break;
        case 4: sprintf( str, " ( %s^ %s)", sFI1, sFI2 ); break;
        case 5: sprintf( str, "~( %s& %s)", sFI1, sFI2 ); break;
        case 6: sprintf( str, "~(~%s& %s)", sFI1, sFI2 ); break;
        case 7: sprintf( str, "~( %s&~%s)", sFI1, sFI2 ); break;
        case 8: sprintf( str, " (~%s&~%s)", sFI1, sFI2 ); break;
        case 9: sprintf( str, "~( %s^ %s)", sFI1, sFI2 ); break;*/
    }
}

int Dtt_ComposeNP( int NP1, int NP2 )
{
    // NP[i] = NP1[NP2[i]]
    int NP = 0, i;
    for ( i = 0; i < 5; i++ )
    {
        NP |= ( ( NP1 >> ((NP2&0x7)<<2) ) & 0x7 ) << (i<<2); // P'[i] = P1[P2[i]]
        NP |= ( ( NP1 >> ((NP2&0x7)<<2) ^ NP2 ) & 0x8 ) << (i<<2); // N'[i] = N1[P2[i]] ^ N2[i]
        NP2 = NP2 >> 4;
    }
    return NP; 
}

void Dtt_MakePI( int NP, char* str )
{
    // apply P'[i], find the i s.t. P'[i]==0, correspond to N'[i]
    int i;
    for ( i = 0; i < 5; i++ )
    {
        if ( ( NP & 0x7 ) == 0 )
        {
            if ( NP & 0x8 ) 
                sprintf( str, "~%c", 'a'+i );
            else
                sprintf( str, "%c", 'a'+i );
            return;
        }
        NP = NP >> 4;
    }
    assert(0);
}

void Dtt_MakeFormula( unsigned tFun, Dtt_FunImpl_t* pFun, Vec_Vec_t* vLibImpl, int NP, char* sF, int fPrint, FILE* pFile );
void Dtt_MakeFormulaFI2( unsigned tFun, Dtt_FunImpl_t* pFun, Vec_Vec_t* vLibImpl, int NP, char* sFI1, char* sF, int fPrint, FILE* pFile )
{
    int j;
    Dtt_FunImpl_t* pImpl2;
    char sFI2[100] = {0}; //sprintf( sFI2, "" );

    if ( pFun->FI2 == 0 ) // PI
    {
        Dtt_MakePI( Dtt_ComposeNP( pFun->NP2, NP ), sFI2 );
        Dtt_FunImpl2Str( pFun->Type, sFI1, sFI2, sF );
        if (fPrint)
            fprintf(pFile, "%08x = %s\n", tFun, sF);
    }
    else
    {
        Vec_VecForEachEntryLevel( Dtt_FunImpl_t*, vLibImpl, pImpl2, j, pFun->FI2 )
        {
            Dtt_MakeFormula( tFun, pImpl2, vLibImpl, Dtt_ComposeNP( pFun->NP2, NP ), sFI2, 0, pFile );
            Dtt_FunImpl2Str( pFun->Type, sFI1, sFI2, sF );
            if (fPrint)
                fprintf(pFile, "%08x = %s\n", tFun, sF);
        }
    }   
}

void Dtt_MakeFormula( unsigned tFun, Dtt_FunImpl_t* pFun, Vec_Vec_t* vLibImpl, int NP, char* sF, int fPrint, FILE* pFile )
{
    int j;
    Dtt_FunImpl_t* pImpl1;
    char sFI1[100], sCopy[100] = {0}; //sprintf( sFI1, "" );

    if ( pFun->FI1 == 0 ) // PI
    {
        Dtt_MakePI( Dtt_ComposeNP( pFun->NP1, NP ), sFI1 );
        sprintf( sCopy, "%s", sF );
        Dtt_MakeFormulaFI2( tFun, pFun, vLibImpl, NP, sFI1, sF, fPrint, pFile );
    }
    else
    {
        Vec_VecForEachEntryLevel( Dtt_FunImpl_t*, vLibImpl, pImpl1, j, pFun->FI1 )
        {
            Dtt_MakeFormula( tFun, pImpl1, vLibImpl, Dtt_ComposeNP( pFun->NP1, NP ), sFI1, 0, pFile );
            sprintf( sCopy, "%s", sF );
            Dtt_MakeFormulaFI2( tFun, pFun, vLibImpl, NP, sFI1, sF, fPrint, pFile );
        }
    }
}

void Dtt_ProcessType( int* Type, int nFanin )
{
    // 0: 1&2, 1: ~1&2, 2: 1&~2, 3: 1|2 = ~(~1&~2), 4: 1^2
    // 5: ~(1&2), 6: ~(~1&2), 7: ~(1&~2), 8: ~1&~2, 9: ~(1^2)
    if ( nFanin == 3 ) // for output negation
        *Type += (*Type<5)? 5: -5;
    else if ( *Type == 0 || *Type == 5 )
        *Type += nFanin;
    else if ( *Type == nFanin ) // 1,1 ; 2,2
        *Type = 0;
    else if ( *Type + nFanin == 3 ) // 1,2 ; 2,1
        *Type = 8;
    else if ( *Type == 3 )
        *Type = (nFanin==1)? 7: 6;
    else if ( *Type == 4 )
        *Type = 9;
    else if ( *Type == nFanin+5 ) // 6,1 ; 7,2
        *Type = 5;
    else if ( *Type + nFanin == 8 ) // 6,2 ; 7,1
        *Type = 3;
    else if ( *Type == 8 )
        *Type = (nFanin==1)? 2: 1;
    else if ( *Type == 9 )
        *Type = 4;
    else
        assert( 0 );
}

int Dtt_Check( unsigned tFun, unsigned tGoal, unsigned tCur, int* pType )
{
    if (!tGoal) //for Fanin2 and output
        return ( tCur == tFun || ~tCur == tFun );
    //for Fanin1: check if tFun(Type)tCur==tGoal
    switch (*pType)
    {
    // 0: 1&2, 1: ~1&2, 2: 1&~2, 3: 1|2 = ~(~1&~2), 4: 1^2
    // 5: ~(1&2), 6: ~(~1&2), 7: ~(1&~2), 8: ~1&~2, 9: ~(1^2)
        case 0: case 5: if ( (~tCur & tFun) == tGoal ) { Dtt_ProcessType( pType, 1 ); return 1; }
                else return ( (tCur & tFun) == tGoal );
        case 1: case 6: if ( (tCur & tFun) == tGoal ) { Dtt_ProcessType( pType, 1 ); return 1; }
                else return ( (~tCur & tFun) == tGoal );
        case 2: case 7: if ( (~tCur & ~tFun) == tGoal ) { Dtt_ProcessType( pType, 1 ); return 1; }
                else return ( (tCur & ~tFun) == tGoal );
        case 3: case 8: if ( (~tCur | tFun) == tGoal ) { Dtt_ProcessType( pType, 1 ); return 1; }
                else return ( (tCur | tFun) == tGoal );
        case 4: case 9: if ( (~tCur ^ tFun) == tGoal ) { Dtt_ProcessType( pType, 1 ); return 1; }
                else return ( (tCur ^ tFun) == tGoal );
        default: assert(0);
    }
    return -1;
}

void Dtt_FindNP( Dtt_Man_t * p, unsigned tFun, unsigned tGoal, unsigned tNpn, int * NP, int* pType, int NPout )
{
    int i, k, j;
    int P[5] = { 0, 1, 2, 3, 4 };
    int N[5] = { 0, 0, 0, 0, 0 };
    int temp;
    
    word tCur = ((word)tNpn << 32) | (word)tNpn;
    for ( i = 0; i < p->nPerms; i++ )
    {
        for ( k = 0; k < p->nComps; k++ )
        {
            if ( Dtt_Check( tFun, tGoal, (unsigned)tCur, pType ))
            {
                if ( !tGoal && ( tFun == (unsigned)~tCur ) ) // !tGoal: not FI1
                {
                    if (!NPout) Dtt_ProcessType( pType, 3 );
                    else Dtt_ProcessType( pType, 2 );
                }
                *NP = 0;
                if (!NPout)
                {
                    for ( j = 0; j < 5; j++ )
                        *NP |= ( ( ( N[j] & 0x1 ) << 3 ) | ( P[j] & 0x7 ) ) << (j<<2) ;
                }
                else
                {
                    for ( j = 0; j < 5; j++ )
                    {
                        *NP |= P[NPout&0x7] << (j<<2); // P'[j] = P[Pout[j]]
                        *NP |= ( N[NPout&0x7] ^ ( (NPout>>3) & 0x1 )) << (j<<2) << 3; // N'[i] = N1[P2[i]] ^ N2[i]
                        NPout = NPout >> 4;
                    }
                }
                
                return;
            }
            tCur = Abc_Tt6Flip( tCur, p->pComps[k] );
            N[p->pComps[k]] ^= 0x1;
        }
        tCur = Abc_Tt6SwapAdjacent( tCur, p->pPerms[i] );
        temp = P[p->pPerms[i]]; P[p->pPerms[i]] = P[p->pPerms[i]+1]; P[p->pPerms[i]+1] = temp;
    }
    assert(0);
}

void Dtt_DumpLibrary( Dtt_Man_t * p, char* FileName )
{
    FILE * pFile;
    char str[100], sFI1[50], sFI2[50];
    int i, j, Entry, fRepeat;
    Dtt_FunImpl_t * pFun, * pFun2;
    Vec_Int_t * vLibFun = Vec_IntDup( p->vTruthNpns );  // none-duplicating vector of NPN representitives
    Vec_Vec_t * vLibImpl;
    Vec_IntUniqify( vLibFun );
    vLibImpl = Vec_VecStart( Vec_IntSize( vLibFun ) );
    Vec_IntForEachEntry( p->vTruths, Entry, i )
    {
        int NP, Fanin2, Fanin1Npn, Fanin2Npn;
        if (i<2) continue; // skip const 0
        pFun = ABC_CALLOC( Dtt_FunImpl_t, 1 );
        pFun->Type = (int)( 0x7 & Vec_IntEntry(p->vConfigs, i) );
        //word Fanin1 = Vec_IntEntry( p->vTruths, Vec_IntEntry( p->vFanins, i*2 ) );
        Fanin2 = Vec_IntEntry( p->vTruths, Vec_IntEntry( p->vFanins, i*2+1 ) );
        Fanin1Npn = Vec_IntEntry( p->vTruthNpns, Vec_IntEntry( p->vFanins, i*2 ) );
        Fanin2Npn = Vec_IntEntry( p->vTruthNpns, Vec_IntEntry( p->vFanins, i*2+1 ) );
        pFun->FI1 = Vec_IntFind( vLibFun, Fanin1Npn );
        pFun->FI2 = Vec_IntFind( vLibFun, Fanin2Npn );
        
        fRepeat = 0;
        Vec_VecForEachEntryLevel( Dtt_FunImpl_t*, vLibImpl, pFun2, j, Vec_IntFind( vLibFun, Vec_IntEntry( p->vTruthNpns, i ) ) )
        {
            if ( ( pFun2->FI1 == pFun->FI1 && pFun2->FI2 == pFun->FI2 ) || ( pFun2->FI2 == pFun->FI1 && pFun2->FI1 == pFun->FI2 ) )
            {
                fRepeat = 1;
                break;
            }
        }
        if (fRepeat) 
        {
            ABC_FREE( pFun );
            continue;
        }

        Dtt_FindNP( p, Vec_IntEntry( p->vTruthNpns, i ), 0, Entry, &NP, &(pFun->Type), 0 ); //out: tGoal=0, NPout=0
        Dtt_FindNP( p, Fanin2, Entry, Fanin1Npn, &(pFun->NP1), &(pFun->Type), NP ); //FI1
        Dtt_FindNP( p, Fanin2, 0, Fanin2Npn, &(pFun->NP2), &(pFun->Type), NP ); //FI2: tGoal=0
        
        Vec_VecPush( vLibImpl, Vec_IntFind( vLibFun, Vec_IntEntry( p->vTruthNpns, i ) ), pFun );
    }

    // print to file
    pFile = fopen( FileName, "wb" );

    if (0)
    Vec_IntForEachEntry( vLibFun, Entry, i )
    {
        if (!Entry) continue; // skip const 0
        fprintf( pFile, "%08x: ", (unsigned)Entry );
        Vec_VecForEachEntryLevel( Dtt_FunImpl_t*, vLibImpl, pFun, j, i )
        {
            Dtt_FunImplFI2Str( pFun->FI1, pFun->NP1, vLibFun, sFI1 ); 
            Dtt_FunImplFI2Str( pFun->FI2, pFun->NP2, vLibFun, sFI2 );
            Dtt_FunImpl2Str( pFun->Type, sFI1, sFI2, str );
            fprintf( pFile, "%s, ", str );
        }
        fprintf( pFile, "\n" );
    }

    // formula format
    Vec_IntForEachEntry( vLibFun, Entry, i )
    {
        if ( i<2 ) continue; // skip const 0 and buffer
        Vec_VecForEachEntryLevel( Dtt_FunImpl_t*, vLibImpl, pFun, j, i )
        {
            str[0] = 0; //sprintf( str, "" );
            Dtt_MakeFormula( (unsigned)Entry, pFun, vLibImpl, (4<<16)+(3<<12)+(2<<8)+(1<<4), str, 1, pFile );
        }
    }

    fclose( pFile );
    printf( "Dumped file \"%s\". \n", FileName );
    fflush( stdout );
}

void Dtt_EnumerateLf( int nVars, int nNodeMax, int fDelay, int fMulti, int fVerbose, char* pFileName )
{
    abctime clk = Abc_Clock(); word nSteps = 0, nMultis = 0;
    Dtt_Man_t * p = Dtt_ManAlloc( nVars, fMulti ); int n, i, j;

    // constant zero class
    Vec_IntPushTwo( p->vFanins, 0, 0 );
    Vec_IntPush( p->vTruths, 0 );
    Vec_IntPush( p->vConfigs, 0 );
    Vec_IntPush( p->vClasses, Vec_IntSize(p->vTruthNpns) );
    Vec_IntPush( p->vTruthNpns, 0 );
    Dtt_ManSetFun( p, 0 );
    // buffer class
    Vec_WecPush( p->vFunNodes, 0, Vec_IntSize(p->vFanins)/2 );
    Vec_IntPushTwo( p->vFanins, 0, 0 );
    Vec_IntPush( p->vTruths, (unsigned)s_Truths6[0] );
    Vec_IntPush( p->vConfigs, 0 );
    Vec_IntPush( p->vClasses, Vec_IntSize(p->vTruthNpns) );
    Vec_IntPush( p->vTruthNpns, (unsigned)s_Truths6[0] );
    for ( i = 0; i < nVars; i++ )
        Dtt_ManSetFun( p, (unsigned)s_Truths6[i] );
    p->Counts[0] = 2;
    // enumerate
    Dtt_PrintStats(0, nVars, p->vFunNodes, nSteps, clk, fDelay, 0);
    for ( n = 1; n <= nNodeMax; n++ )
    {
        for ( i = 0, j = n - 1; i < n; i++, j = j - 1 + fDelay ) if ( i <= j )
        {
            Vec_Int_t * vFaninI = Vec_WecEntry( p->vFunNodes, i );
            Vec_Int_t * vFaninJ = Vec_WecEntry( p->vFunNodes, j );
            int k, i0, j0, EntryI, EntryJ;
            Vec_IntForEachEntry( vFaninI, EntryI, i0 )
            {
                unsigned TruthI = Vec_IntEntry(p->vTruths, EntryI);
                Vec_Int_t * vFuns = Dtt_ManCollect( p, TruthI, p->vTemp );
                int Start = i == j ? i0 : 0;
                Vec_IntForEachEntryStart( vFaninJ, EntryJ, j0, Start )
                {
                    unsigned Truth, TruthJ = Vec_IntEntry(p->vTruths, EntryJ);
                    Vec_IntForEachEntry( vFuns, Truth, k )
                    {
                        if ( !Dtt_ManGetFun(p,  TruthJ &  Truth, n) )
                            Dtt_ManAddFunction( p, n, EntryI, EntryJ, 0,  TruthJ &  Truth );
                        if ( !Dtt_ManGetFun(p,  TruthJ & ~Truth, n) )
                            Dtt_ManAddFunction( p, n, EntryI, EntryJ, 1,  TruthJ & ~Truth );
                        if ( !Dtt_ManGetFun(p, ~TruthJ &  Truth, n) )
                            Dtt_ManAddFunction( p, n, EntryI, EntryJ, 2, ~TruthJ &  Truth );
                        if ( !Dtt_ManGetFun(p,  TruthJ |  Truth, n) )
                            Dtt_ManAddFunction( p, n, EntryI, EntryJ, 3,  TruthJ |  Truth );
                        if ( !Dtt_ManGetFun(p,  TruthJ ^  Truth, n) )
                            Dtt_ManAddFunction( p, n, EntryI, EntryJ, 4,  TruthJ ^  Truth );
                        nSteps += 5;

                        if ( p->pTable ) Dtt_ManAddVisited( p,  TruthJ &  Truth, n );
                        if ( p->pTable ) Dtt_ManAddVisited( p,  TruthJ & ~Truth, n );
                        if ( p->pTable ) Dtt_ManAddVisited( p, ~TruthJ &  Truth, n );
                        if ( p->pTable ) Dtt_ManAddVisited( p,  TruthJ |  Truth, n );
                        if ( p->pTable ) Dtt_ManAddVisited( p,  TruthJ ^  Truth, n );
                    }
                    nMultis++;
                    if ( p->pTable ) Dtt_ManProcessVisited( p );
                }
            }
        }
        if ( Dtt_PrintStats(n, nVars, p->vFunNodes, nSteps, clk, fDelay, nMultis) == 0 )
            break;
    }
    if ( fDelay )
        Dtt_PrintDistrib( p );
    //if ( p->pTable ) 
        //Dtt_PrintMulti( p );
    if ( !fDelay && pFileName!=NULL )
        Dtt_DumpLibrary( p, pFileName );
    Dtt_ManFree( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

