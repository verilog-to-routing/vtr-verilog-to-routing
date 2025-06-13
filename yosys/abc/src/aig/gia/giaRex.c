/**CFile****************************************************************

  FileName    [giaRex.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Regular expressions.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaRex.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Simulate AIG with the given sequence.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAutomSimulate( Gia_Man_t * p, Vec_Int_t * vAlpha, char * pSim )
{
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int nInputs = Vec_IntSize(vAlpha);
    int nFrames = strlen(pSim);
    int i, k;
    assert( Gia_ManPiNum(p) == nInputs );
    printf( "Simulating string \"%s\":\n", pSim );
    Gia_ManCleanMark0(p);
    Gia_ManForEachRo( p, pObj, i )
        pObj->fMark0 = 0;
    for ( i = 0; i < nFrames; i++ )
    {
        Gia_ManForEachPi( p, pObj, k )
            pObj->fMark0 = (int)(Vec_IntFind(vAlpha, pSim[i]) == k);
        Gia_ManForEachAnd( p, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( p, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;

        printf( "Frame %d : %c %d\n", i, pSim[i], Gia_ManPo(p, 0)->fMark0 );
    }
    Gia_ManCleanMark0(p);
}

/**Function*************************************************************

  Synopsis    [Builds 1-hotness contraint.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManBuild1Hot_rec( Gia_Man_t * p, int * pLits, int nLits, int * pZero, int * pOne )
{
    int Zero0, One0, Zero1, One1;
    if ( nLits == 1 )
    {
        *pZero = Abc_LitNot(pLits[0]);
        *pOne  = pLits[0];
        return;
    }
    Gia_ManBuild1Hot_rec( p, pLits,           nLits/2,         &Zero0, &One0 );
    Gia_ManBuild1Hot_rec( p, pLits + nLits/2, nLits - nLits/2, &Zero1, &One1 );
    *pZero = Gia_ManHashAnd( p, Zero0, Zero1 );
    *pOne  = Gia_ManHashOr( p, Gia_ManHashAnd(p, Zero0, One1), Gia_ManHashAnd(p, Zero1, One0) );
}
int Gia_ManBuild1Hot( Gia_Man_t * p, Vec_Int_t * vLits )
{
    int Zero, One;
    assert( Vec_IntSize(vLits) > 0 );
    Gia_ManBuild1Hot_rec( p, Vec_IntArray(vLits), Vec_IntSize(vLits), &Zero, &One );
    return One;
}

/**Function*************************************************************

  Synopsis    [Converting regular expressions into sequential AIGs.]

  Description [http://algs4.cs.princeton.edu/lectures/54RegularExpressions.pdf]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_SymbSpecial( char c ) { return c == '(' || c == ')' || c == '*' || c == '|'; }
// collects info about input alphabet and state of the automaton
int Gia_ManRexNumInputs( char * pStr, Vec_Int_t ** pvAlphas, Vec_Int_t ** pvStr2Sta )
{
    int i, nStates = 0, Length = strlen(pStr);
    Vec_Int_t * vAlphas  = Vec_IntAlloc( 100 );            // alphabet
    Vec_Int_t * vStr2Sta = Vec_IntStartFull( Length + 1 ); // symbol to state
    for ( i = 0; i < Length; i++ )
    {
        if ( Gia_SymbSpecial(pStr[i]) )
            continue;
        if ( Vec_IntFind(vAlphas, pStr[i]) == -1 )
            Vec_IntPush( vAlphas, pStr[i] );
        Vec_IntWriteEntry( vStr2Sta, i, nStates++ );
    }
    Vec_IntWriteEntry( vStr2Sta, i, nStates );
    *pvAlphas  = vAlphas;
    *pvStr2Sta = vStr2Sta;
    return nStates;
}
// prints automaton
void Gia_ManPrintAutom( char * pStr, Vec_Int_t * vStaTrans )
{
    int i = 0, nLength = strlen(pStr);
    for ( i = 0; i < nLength; i++ )
    {
        printf( "%d \'%c\' ", i, pStr[i] );
        if ( Vec_IntEntry(vStaTrans, i) >= 0 )
            printf( "-> %d \'%c\' ", Vec_IntEntry(vStaTrans, i), pStr[Vec_IntEntry(vStaTrans, i)] );
        printf( "\n" );
    }
}
// prints states reachable through e-transitions
void Gia_ManPrintReached( char * pStr, int iState, Vec_Int_t * vReached )
{
    int i, Entry;
    printf( "Reached from state %d \'%c\':  ", iState, pStr[iState] );
    Vec_IntForEachEntry( vReached, Entry, i )
        printf( "%d \'%c\'  ", Entry, pStr[Entry] );
    printf( "\n" );
}
// collect states reachable from the given one by e-transitions
void Gia_ManPrintReached_rec( char * pStr, Vec_Int_t * vStaTrans, int iState, Vec_Int_t * vReached, Vec_Int_t * vVisited, int TravId )
{
    if ( Vec_IntEntry(vVisited, iState) == TravId )
        return;
    Vec_IntWriteEntry( vVisited, iState, TravId );
    if ( !Gia_SymbSpecial(pStr[iState]) ) // read state
        Vec_IntPush( vReached, iState );
    if ( pStr[iState] == '\0' )
        return;
    if ( Gia_SymbSpecial(pStr[iState]) && pStr[iState] != '|' ) // regular e-transition
       Gia_ManPrintReached_rec( pStr, vStaTrans, iState + 1, vReached, vVisited, TravId );
    if ( Vec_IntEntry(vStaTrans, iState) >= 0 ) // additional e-transition
       Gia_ManPrintReached_rec( pStr, vStaTrans, Vec_IntEntry(vStaTrans, iState), vReached, vVisited, TravId );
}
void Gia_ManCollectReached( char * pStr, Vec_Int_t * vStaTrans, int iState, Vec_Int_t * vReached, Vec_Int_t * vVisited, int TravId )
{
    assert( iState == 0 || !Gia_SymbSpecial(pStr[iState]) );
    assert( Vec_IntEntry(vVisited, iState) != TravId );
    Vec_IntClear( vReached );
    Gia_ManPrintReached_rec( pStr, vStaTrans, iState + 1, vReached, vVisited, TravId );
}
// preprocesses the regular expression
char * Gia_ManRexPreprocess( char * pStr )
{
    char * pCopy = ABC_CALLOC( char, strlen(pStr) * 2 + 10 );
    int i, k = 0;
    pCopy[k++] = '(';
    pCopy[k++] = '(';
    for ( i = 0; pStr[i]; i++ )
    {
        if ( pStr[i] == '(' )
            pCopy[k++] = '(';
        else if ( pStr[i] == ')' )
            pCopy[k++] = ')';
        if ( pStr[i] != ' ' && pStr[i] != '\t' && pStr[i] != '\n' && pStr[i] != '\r' )
            pCopy[k++] = pStr[i];
    }
    pCopy[k++] = ')';
    pCopy[k++] = ')';
    pCopy[k++] = '\0';
    return pCopy;
}
// construct sequential AIG for the automaton
Gia_Man_t * Gia_ManRex2Gia( char * pStrInit, int fOrder, int fVerbose )
{
    Gia_Man_t * pNew = NULL, * pTemp; 
    Vec_Int_t * vAlphas, * vStr2Sta, * vStaLits;
    Vec_Int_t * vStaTrans, * vStack, * vVisited;
    Vec_Str_t * vInit;
    char * pStr = Gia_ManRexPreprocess( pStrInit );
    int nStates = Gia_ManRexNumInputs( pStr, &vAlphas, &vStr2Sta );
    int i, k, iLit, Entry, nLength = strlen(pStr), nTravId = 1;
    if ( fOrder )
        Vec_IntSort( vAlphas, 0 );
//    if ( fVerbose )
    {
        printf( "Input variable order: " );
        Vec_IntForEachEntry( vAlphas, Entry, k )
            printf( "%c", (char)Entry );
        printf( "\n" );
    }
    // start AIG
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( pStrInit );
    for ( i = 0; i < Vec_IntSize(vAlphas) + nStates; i++ )
        Gia_ManAppendCi( pNew );
    // prepare automaton
    vStaLits  = Vec_IntStart( nStates + 1 );
    vStaTrans = Vec_IntStartFull( nLength );
    vStack    = Vec_IntAlloc( nLength );
    vVisited  = Vec_IntStartFull( nLength + 1 );
    for ( i = 0; i < nLength; i++ )
    {
        int Lp = i;
        if ( pStr[i] == '(' || pStr[i] == '|' )
            Vec_IntPush( vStack, i );
        else if ( pStr[i] == ')' )
        {
            int Or = Vec_IntPop( vStack );
            if ( pStr[Or] == '|' )
            {
                Lp = Vec_IntPop( vStack );
                Vec_IntWriteEntry( vStaTrans, Lp, Or + 1 );
                Vec_IntWriteEntry( vStaTrans, Or, i );
            }
            else
                Lp = Or;
        }
        if ( i < nLength - 1 && pStr[i+1] == '*' )
        {
            Vec_IntWriteEntry( vStaTrans, Lp, i+1 );
            Vec_IntWriteEntry( vStaTrans, i+1, Lp );
        }
    }
    assert( Vec_IntSize(vStack) == 0 );
    if ( fVerbose )
        Gia_ManPrintAutom( pStr, vStaTrans );

    // create next-state functions for each state
    Gia_ManHashAlloc( pNew );
    for ( i = 1; i < nLength; i++ )
    {
        int iThis, iThat, iThisLit, iInputLit;
        if ( Gia_SymbSpecial(pStr[i]) )
            continue;
        Gia_ManCollectReached( pStr, vStaTrans, i, vStack, vVisited, nTravId++ );
        if ( fVerbose )
            Gia_ManPrintReached( pStr, i, vStack );
        // create transitions from this state under this input
        iThis = Vec_IntEntry(vStr2Sta, i);
        iThisLit = Gia_Obj2Lit(pNew, Gia_ManPi(pNew, Vec_IntSize(vAlphas) + iThis));
        iInputLit = Gia_Obj2Lit(pNew, Gia_ManPi(pNew, Vec_IntFind(vAlphas, pStr[i])));
        iLit = Gia_ManHashAnd( pNew, iThisLit, iInputLit );
        Vec_IntForEachEntry( vStack, Entry, k )
        {
            iThat = Vec_IntEntry(vStr2Sta, Entry);
            iLit = Gia_ManHashOr( pNew, iLit, Vec_IntEntry(vStaLits, iThat) );
            Vec_IntWriteEntry( vStaLits, iThat, iLit );
        }
    }
    // create one-hotness
    Vec_IntClear( vStack );
    for ( i = 0; i < Vec_IntSize(vAlphas); i++ )
        Vec_IntPush( vStack, Gia_Obj2Lit(pNew, Gia_ManPi(pNew, i)) );
    iLit = Gia_ManBuild1Hot( pNew, vStack );
    // combine with outputs
    Vec_IntForEachEntry( vStaLits, Entry, k )
        Vec_IntWriteEntry( vStaLits, k, Gia_ManHashAnd(pNew, iLit, Entry) );
    Gia_ManHashStop( pNew );

    // collect initial state
    Gia_ManCollectReached( pStr, vStaTrans, 0, vStack, vVisited, nTravId++ );
    if ( fVerbose )
        Gia_ManPrintReached( pStr, 0, vStack );
    vInit = Vec_StrStart( nStates + 1 );
    Vec_StrFill( vInit, nStates, '0' );
    Vec_IntForEachEntry( vStack, Entry, k )
        if ( pStr[Entry] != '\0' )
            Vec_StrWriteEntry( vInit, Vec_IntEntry(vStr2Sta, Entry), '1' );
    if ( fVerbose )
        printf( "Init state = %s\n", Vec_StrArray(vInit) );

    // add outputs
    Vec_IntPushFirst( vStaLits, Vec_IntPop(vStaLits) );
    assert( Vec_IntSize(vStaLits) == nStates + 1 );
    Vec_IntForEachEntry( vStaLits, iLit, i )
        Gia_ManAppendCo( pNew, iLit );
    Gia_ManSetRegNum( pNew, nStates );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );

    // add initial state
    pNew = Gia_ManDupZeroUndc( pTemp = pNew, Vec_StrArray(vInit), 0, 0, 0 );
    Gia_ManStop( pTemp );
    Vec_StrFree( vInit );
/*
    Gia_ManAutomSimulate( pNew, vAlphas, "0" );
    Gia_ManAutomSimulate( pNew, vAlphas, "01" );
    Gia_ManAutomSimulate( pNew, vAlphas, "110" );
    Gia_ManAutomSimulate( pNew, vAlphas, "011" );
    Gia_ManAutomSimulate( pNew, vAlphas, "111" );
    Gia_ManAutomSimulate( pNew, vAlphas, "1111" );
    Gia_ManAutomSimulate( pNew, vAlphas, "1010" );

    Gia_ManAutomSimulate( pNew, vAlphas, "A" );
    Gia_ManAutomSimulate( pNew, vAlphas, "AD" );
    Gia_ManAutomSimulate( pNew, vAlphas, "ABCD" );
    Gia_ManAutomSimulate( pNew, vAlphas, "BCD" );
    Gia_ManAutomSimulate( pNew, vAlphas, "CD" );
*/

    // cleanup
    Vec_IntFree( vAlphas );
    Vec_IntFree( vStr2Sta );
    Vec_IntFree( vStaLits );
    Vec_IntFree( vStaTrans );
    Vec_IntFree( vStack );
    Vec_IntFree( vVisited );
    ABC_FREE( pStr );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Transposing 64-bit matrix.]

  Description [Borrowed from "Hacker's Delight", by Henry Warren.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAutomTranspose64( word A[64] )
{
    int j, k;
    word t, m = 0x00000000FFFFFFFF;
    for ( j = 32; j != 0; j = j >> 1, m = m ^ (m << j) )
    {
        for ( k = 0; k < 64; k = (k + j + 1) & ~j )
        {
            t = (A[k] ^ (A[k+j] >> j)) & m;
            A[k] = A[k] ^ t;
            A[k+j] = A[k+j] ^ (t << j);
        }
    }
}

/**Function*************************************************************

  Synopsis    [Simulate AIG with the given sequence.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Gia_ManAutomSim0( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Wrd_t * vTemp )
{
    return Gia_ObjFaninC0(pObj) ? ~Vec_WrdEntry(vTemp, Gia_ObjFaninId0p(p, pObj)) : Vec_WrdEntry(vTemp, Gia_ObjFaninId0p(p, pObj));
}
static inline word Gia_ManAutomSim1( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Wrd_t * vTemp )
{
    return Gia_ObjFaninC1(pObj) ? ~Vec_WrdEntry(vTemp, Gia_ObjFaninId1p(p, pObj)) : Vec_WrdEntry(vTemp, Gia_ObjFaninId1p(p, pObj));
}
word Gia_ManAutomStep( Gia_Man_t * p, word Cur, word * pNext, Vec_Wrd_t * vTemp )
{
    Gia_Obj_t * pObj; int i;
    assert( Gia_ManPoNum(p) == 1 );
    assert( Vec_WrdSize(vTemp) >= Gia_ManObjNum(p) );
    Vec_WrdWriteEntry( vTemp, 0, 0 );
    Gia_ManForEachPi( p, pObj, i )
        Vec_WrdWriteEntry( vTemp, Gia_ObjId(p, pObj), ((word)1) << (63-i) );
    Gia_ManForEachRo( p, pObj, i )
        Vec_WrdWriteEntry( vTemp, Gia_ObjId(p, pObj), ((Cur >> (63-i)) & 1) ? ~((word)0) : 0 );
    Gia_ManForEachAnd( p, pObj, i )
        Vec_WrdWriteEntry( vTemp, i, Gia_ManAutomSim0(p, pObj, vTemp) & Gia_ManAutomSim1(p, pObj, vTemp) );
    Gia_ManForEachRi( p, pObj, i )
        pNext[i] = Gia_ManAutomSim0(p, pObj, vTemp);
    for ( ; i < 64; i++ )
        pNext[i] = 0;
    // transpose
//    for ( i = 0; i < 64; i++ )
//        Extra_PrintBinary( stdout, (unsigned *)&pNext[i], 64 ), Abc_Print( 1, "\n" );
//    printf( "\n" );
    Gia_ManAutomTranspose64( pNext );
//    for ( i = 0; i < 64; i++ )
//        Extra_PrintBinary( stdout, (unsigned *)&pNext[i], 64 ), Abc_Print( 1, "\n" );
//    printf( "\n" );
    // return output values
    return Gia_ManAutomSim0(p, Gia_ManPo(p, 0), vTemp);
}
void Gia_ManAutomWalkOne( Gia_Man_t * p, int nSteps, Vec_Wrd_t * vStates, Vec_Int_t * vCounts, Vec_Wrd_t * vTemp, word Init )
{
    word iState = 0, Output, pNext[64]; 
    int i, k, kMin, Index, IndexMin;
    int Count, CountMin; 
    for ( i = 0; i < nSteps; i++ )
    {
        Output = Gia_ManAutomStep( p, iState, pNext, vTemp );
        // check visited states
        kMin = -1;
        IndexMin = -1;
        CountMin = ABC_INFINITY;  
        for ( k = 0; k < Gia_ManPiNum(p); k++ )
        {
            if ( pNext[k] == Init )
                continue;
            Index = Vec_WrdFind( vStates, pNext[k] );
            Count = Index == -1 ? 0 : Vec_IntEntry( vCounts, Index );
            if ( CountMin > Count || (CountMin != ABC_INFINITY && Count && ((float)CountMin / Count) > (float)rand()/RAND_MAX ) )
            {
                CountMin = Count;
                IndexMin = Index;
                kMin = k;
            }
            if ( CountMin == 0 )
                break;
        }
        // choose the best state
        if ( CountMin == ABC_INFINITY )
        {
            for ( k = 0; k < Gia_ManPiNum(p); k++ )
                if ( (Output >> (63-k)) & 1 )
                {
                    printf( "%c", 'a' + k );
                    printf( "!" );
                }            
            break;
        }
        assert( CountMin < ABC_INFINITY );
        if ( IndexMin == -1 )
        {
            assert( CountMin == 0 );
            IndexMin = Vec_IntSize(vCounts);
            Vec_IntPush( vCounts, 0 );
            Vec_WrdPush( vStates, pNext[kMin] );
        }
        Vec_IntAddToEntry( vCounts, IndexMin, 1 );
        iState = pNext[kMin];
//Extra_PrintBinary( stdout, (unsigned *)&iState, 64 ); printf( "\n" );
        // print the transition
        printf( "%c", 'a' + kMin );
        if ( (Output >> (63-kMin)) & 1 )
            printf( "!" );
    }
    printf( "\n" );
}
// find flop variables pointed to by negative edges
word Gia_ManAutomInit( Gia_Man_t * p )
{
    Gia_Obj_t * pObj; 
    int i, Index;
    word Init = 0;
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_ObjFaninC0(pObj) && Gia_ObjIsCi(Gia_ObjFanin0(pObj)) )
        {
            Index = Gia_ObjCioId(Gia_ObjFanin0(pObj)) - Gia_ManPiNum(p);
            if ( Index >= 0 )
                Init |= ((word)1 << (63-Index));
        }
        if ( Gia_ObjFaninC1(pObj) && Gia_ObjIsCi(Gia_ObjFanin1(pObj)) )
        {
            Index = Gia_ObjCioId(Gia_ObjFanin1(pObj)) - Gia_ManPiNum(p);
            if ( Index >= 0 )
                Init |= ((word)1 << (63-Index));
        }
    }
    return Init;
}
void Gia_ManAutomWalk( Gia_Man_t * p, int nSteps, int nWalks, int fVerbose )
{
    Vec_Wrd_t * vTemp, * vStates;
    Vec_Int_t * vCounts; int i; word Init;
    if ( Gia_ManPoNum(p) != 1 )
    {
        printf( "AIG should have one primary output.\n" );
        return;
    }
    if ( Gia_ManPiNum(p) > 64 )
    {
        printf( "Cannot simulate an automaton with more than 64 inputs.\n" );
        return;
    }
    if ( Gia_ManRegNum(p) > 64 )
    {
        printf( "Cannot simulate an automaton with more than 63 states.\n" );
        return;
    }
    vTemp   = Vec_WrdStart( Gia_ManObjNum(p) );
    vStates = Vec_WrdAlloc( 1000 );
    vCounts = Vec_IntAlloc( 1000 );
    Vec_WrdPush( vStates, 0 );
    Vec_IntPush( vCounts, 1 );
    Init = Gia_ManAutomInit( p );
    for ( i = 0; i < nWalks; i++ )
        Gia_ManAutomWalkOne( p, nSteps, vStates, vCounts, vTemp, Init );
    if ( fVerbose )
    {
        word State;
        Vec_WrdForEachEntry( vStates, State, i )
        {
            State ^= Init;
            printf( "%3d : ", i );
            Extra_PrintBinary( stdout, (unsigned *)&State, 64 ); 
            printf( " %d  ", Vec_IntEntry(vCounts, i) );
            printf( "\n" );
        }
        printf( "\n" );
    }
    Vec_WrdFree( vTemp );
    Vec_WrdFree( vStates );
    Vec_IntFree( vCounts );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

