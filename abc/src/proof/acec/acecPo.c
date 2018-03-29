/**CFile****************************************************************

  FileName    [acecPo.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecPo.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecWec.h"
#include "misc/vec/vecHsh.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks that items are unique and in order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntPushOrderAbs( Vec_Int_t * p, int Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        assert( Entry != p->pArray[i] );
    if ( p->nSize == p->nCap )
    {
        if ( p->nCap < 16 )
            Vec_IntGrow( p, 16 );
        else
            Vec_IntGrow( p, 2 * p->nCap );
    }
    p->nSize++;
    for ( i = p->nSize-2; i >= 0; i-- )
        if ( Abc_AbsInt(p->pArray[i]) < Abc_AbsInt(Entry) )
            p->pArray[i+1] = p->pArray[i];
        else
            break;
    p->pArray[i+1] = Entry;
}
static inline void Vec_IntAppendMinusAbs( Vec_Int_t * vVec1, Vec_Int_t * vVec2, int fMinus )
{
    int Entry, i;
    Vec_IntClear( vVec1 );
    Vec_IntForEachEntry( vVec2, Entry, i )
        Vec_IntPushOrderAbs( vVec1, fMinus ? -Entry : Entry );
}
static inline void Vec_IntCheckUniqueOrderAbs( Vec_Int_t * p )
{
    int i;
    for ( i = 1; i < p->nSize; i++ )
        assert( Abc_AbsInt(p->pArray[i-1]) > Abc_AbsInt(p->pArray[i]) );
}
static inline void Vec_IntCheckUniqueOrder( Vec_Int_t * p )
{
    int i;
    for ( i = 1; i < p->nSize; i++ )
        assert( p->pArray[i-1] < p->pArray[i] );
}

/**Function*************************************************************

  Synopsis    [Prints polynomial.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_PolynPrintMono( Vec_Int_t * vConst, Vec_Int_t * vMono, int Prev )
{
    int k, Entry;
    printf( "%c ", Prev != Abc_AbsInt(Vec_IntEntry(vConst, 0)) ? '|' : ' ' );
    Vec_IntForEachEntry( vConst, Entry, k )
        printf( "%s2^%d", Entry < 0 ? "-" : "+", Abc_AbsInt(Entry)-1 );
    Vec_IntForEachEntry( vMono, Entry, k )
        printf( " * %d", Entry-1 );
    printf( "\n" );
}
void Gia_PolynPrint( Vec_Wec_t * vPolyn )
{
    Vec_Int_t * vConst, * vMono;  
    int i, Prev = -1;
    printf( "Polynomial with %d monomials:\n", Vec_WecSize(vPolyn)/2 );
    for ( i = 0; i < Vec_WecSize(vPolyn)/2; i++ )
    {
        vConst = Vec_WecEntry( vPolyn, 2*i+0 );
        vMono  = Vec_WecEntry( vPolyn, 2*i+1 );
        Gia_PolynPrintMono( vConst, vMono, Prev );
        Prev = Abc_AbsInt( Vec_IntEntry(vConst, 0) );
    }
}
void Gia_PolynPrintStats( Vec_Wec_t * vPolyn )
{
    Vec_Int_t * vConst, * vCountsP, * vCountsN;
    int i, Entry, Max = 0;
    printf( "Polynomial with %d monomials:\n", Vec_WecSize(vPolyn)/2 );
    for ( i = 0; i < Vec_WecSize(vPolyn)/2; i++ )
    {
        vConst = Vec_WecEntry( vPolyn, 2*i+0 );
        Max = Abc_MaxInt( Max, Abc_AbsInt(Abc_AbsInt(Vec_IntEntry(vConst, 0))) );
    }
    vCountsP = Vec_IntStart( Max + 1 );
    vCountsN = Vec_IntStart( Max + 1 );
    for ( i = 0; i < Vec_WecSize(vPolyn)/2; i++ )
    {
        vConst = Vec_WecEntry( vPolyn, 2*i+0 );
        Entry = Vec_IntEntry(vConst, 0);
        if ( Entry > 0 )
            Vec_IntAddToEntry( vCountsP, Entry, 1 );
        else
            Vec_IntAddToEntry( vCountsN, -Entry, 1 );
    }
    Vec_IntForEachEntry( vCountsN, Entry, i )
        if ( Entry )
            printf( "-2^%d appears %d times\n", Abc_AbsInt(i)-1, Entry );
    Vec_IntForEachEntry( vCountsP, Entry, i )
        if ( Entry )
            printf( "+2^%d appears %d times\n", Abc_AbsInt(i)-1, Entry );
    Vec_IntFree( vCountsP );
    Vec_IntFree( vCountsN );
}

/**Function*************************************************************

  Synopsis    [Collects polynomial.]

  Description [Collects non-trivial monomials in the increasing order 
  of the absolute value of the their first coefficients.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_PolynGetResultCompare( int * p0, int * p1 )
{
    if ( p0[2] < p1[2] ) return -1;
    if ( p0[2] > p1[2] ) return  1;
    return 0;
}
Vec_Wec_t * Gia_PolynGetResult( Hsh_VecMan_t * pHashC, Hsh_VecMan_t * pHashM, Vec_Int_t * vCoefs )
{
    Vec_Int_t * vClass, * vLevel, * vArray;
    Vec_Wec_t * vPolyn, * vSorted;
    int i, k, iConst, iMono, iFirst;
    // find the largest 
    int nLargest = 0, nNonConst = 0;
    Vec_IntForEachEntry( vCoefs, iConst, iMono )
    {
        //Vec_IntPrint( Hsh_VecReadEntry(pHashM, iMono) );
        if ( iConst == 0 ) 
            continue;
        vArray = Hsh_VecReadEntry( pHashC, iConst );
        nLargest = Abc_MaxInt( nLargest, Abc_AbsInt(Vec_IntEntry(vArray, 0)) );
        nNonConst++;
    }
    // sort by the size of the largest coefficient
    vSorted = Vec_WecStart( nLargest+1 );
    Vec_IntForEachEntry( vCoefs, iConst, iMono )
    {
        if ( iConst == 0 ) 
            continue;
        vArray = Hsh_VecReadEntry( pHashC, iConst );
        vLevel = Vec_WecEntry( vSorted, Abc_AbsInt(Vec_IntEntry(vArray, 0)) );
        vArray = Hsh_VecReadEntry( pHashM, iMono );
        iFirst = Vec_IntSize(vArray) ? Vec_IntEntry(vArray, 0) : -1;
        Vec_IntPushThree( vLevel, iConst, iMono, iFirst );
    }
    // reload in the given order
    vPolyn = Vec_WecAlloc( 2*nNonConst );
    Vec_WecForEachLevel( vSorted, vClass, i )
    {
        // sort monomials by the index of the first variable
        qsort( Vec_IntArray(vClass), Vec_IntSize(vClass)/3, 12, (int (*)(const void *, const void *))Gia_PolynGetResultCompare );
        Vec_IntForEachEntryTriple( vClass, iConst, iMono, iFirst, k )
        {
            vArray = Hsh_VecReadEntry( pHashC, iConst );
            Vec_IntCheckUniqueOrderAbs( vArray );
            vLevel = Vec_WecPushLevel( vPolyn );
            Vec_IntGrow( vLevel, Vec_IntSize(vArray) );
            Vec_IntAppend( vLevel, vArray );

            vArray = Hsh_VecReadEntry( pHashM, iMono );
            Vec_IntCheckUniqueOrder( vArray );
            vLevel = Vec_WecPushLevel( vPolyn );
            Vec_IntGrow( vLevel, Vec_IntSize(vArray) );
            Vec_IntAppend( vLevel, vArray );
        }
    }
    assert( Vec_WecSize(vPolyn) == 2*nNonConst );
    Vec_WecFree( vSorted );
    return vPolyn;
}


/**Function*************************************************************

  Synopsis    [Derives new constant.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_PolynMergeConstOne( Vec_Int_t * vConst, int New )
{
    int i, Old;
    assert( New != 0 );
    Vec_IntForEachEntry( vConst, Old, i )
    {
        assert( Old != 0 );
        if ( Old == New ) // A == B
        {
            Vec_IntDrop( vConst, i );
            Gia_PolynMergeConstOne( vConst, New > 0 ? New + 1 : New - 1 );
            return;
        }
        if ( Abc_AbsInt(Old) == Abc_AbsInt(New) ) // A == -B
        {
            Vec_IntDrop( vConst, i );
            return;
        }
        if ( Old + New == 1 || Old + New == -1 )  // sign(A) != sign(B)  &&  abs(abs(A)-abs(B)) == 1 
        {
            int Value = Abc_MinInt( Abc_AbsInt(Old), Abc_AbsInt(New) );
            Vec_IntDrop( vConst, i );
            Gia_PolynMergeConstOne( vConst, (Old + New == 1) ? Value : -Value );
            return;
        }
    }
    Vec_IntPushOrderAbs( vConst, New );
}
static inline void Gia_PolynMergeConst( Vec_Int_t * vTempC, Hsh_VecMan_t * pHashC, int iConstAdd )
{
    int i, New;
    Vec_Int_t * vConstAdd = Hsh_VecReadEntry( pHashC, iConstAdd );
    Vec_IntForEachEntry( vConstAdd, New, i )
    {
        Gia_PolynMergeConstOne( vTempC, New );
        vConstAdd = Hsh_VecReadEntry( pHashC, iConstAdd );
    }
    Vec_IntCheckUniqueOrderAbs( vConstAdd );
    //Vec_IntPrint( vConstAdd );
}
static inline int Gia_PolynBuildAdd( Hsh_VecMan_t * pHashC, Hsh_VecMan_t * pHashM, Vec_Int_t * vCoefs, 
                                     Vec_Wec_t * vLit2Mono, Vec_Int_t * vTempC, Vec_Int_t * vTempM )
{
    int i, iLit, iConst, iConstNew;
    int iMono = Hsh_VecManAdd(pHashM, vTempM);
    if ( iMono == Vec_IntSize(vCoefs) ) // new monomial
    {
        // map monomial into a constant
        assert( Vec_IntSize(vTempC) > 0 );
        iConst = Hsh_VecManAdd( pHashC, vTempC );
        Vec_IntPush( vCoefs, iConst );
        // map literals into monomial
        assert( Vec_IntSize(vTempM) > 0 );
        Vec_IntForEachEntry( vTempM, iLit, i )
            Vec_WecPush( vLit2Mono, iLit, iMono ); 
        //printf( "New monomial: \n" );
        //Gia_PolynPrintMono( vTempC, vTempM );
        return 1;
    }
    // this monomial exists
    iConst = Vec_IntEntry( vCoefs, iMono );
    if ( iConst )
        Gia_PolynMergeConst( vTempC, pHashC, iConst );
    iConstNew = Hsh_VecManAdd( pHashC, vTempC );
    Vec_IntWriteEntry( vCoefs, iMono, iConstNew );
    //printf( "Old monomial: \n" );
    //Gia_PolynPrintMono( vTempC, vTempM );
    if ( iConst && !iConstNew )
        return -1;
    if ( !iConst && iConstNew )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Computing for literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_PolynHandleOne( Hsh_VecMan_t * pHashC, Hsh_VecMan_t * pHashM, Vec_Int_t * vCoefs, 
                                      Vec_Wec_t * vLit2Mono, Vec_Int_t * vTempC, Vec_Int_t * vTempM, 
                                      int iMono, int iLitOld, int iLitNew0, int iLitNew1 )
{
    int status, iConst = Vec_IntEntry( vCoefs, iMono );
    Vec_Int_t * vArrayC = Hsh_VecReadEntry( pHashC, iConst );
    Vec_Int_t * vArrayM = Hsh_VecReadEntry( pHashM, iMono );
    // create new monomial
    Vec_IntClear( vTempM );
    Vec_IntAppend( vTempM, vArrayM );
    status = Vec_IntRemove( vTempM, iLitOld );
    assert( status );
    // create new monomial
    if ( iLitNew0 == -1 && iLitNew1 == -1 )     // no new lit - the same const
        Vec_IntAppendMinusAbs( vTempC, vArrayC, 0 );
    else if ( iLitNew0 > -1 && iLitNew1 == -1 ) // one new lit - opposite const
    {
        Vec_IntAppendMinusAbs( vTempC, vArrayC, 1 );
        Vec_IntPushUniqueOrder( vTempM, iLitNew0 );
    }
    else if ( iLitNew0 > -1 && iLitNew1 > -1 )  // both new lit - the same const
    {
        Vec_IntAppendMinusAbs( vTempC, vArrayC, 0 );
        Vec_IntPushUniqueOrder( vTempM, iLitNew0 );
        Vec_IntPushUniqueOrder( vTempM, iLitNew1 );
    }
    else assert( 0 );
    return Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC, vTempM );
}

Vec_Wec_t * Gia_PolynBuildNew2( Gia_Man_t * pGia, Vec_Int_t * vRootLits, int nExtra, Vec_Int_t * vLeaves, Vec_Int_t * vNodes, int fSigned, int fVerbose, int fVeryVerbose )
{
    abctime clk = Abc_Clock();
    Vec_Wec_t * vPolyn;
    Vec_Wec_t * vLit2Mono = Vec_WecStart( 2 * Gia_ManObjNum(pGia) ); // mapping AIG literals into monomials
    Hsh_VecMan_t * pHashC = Hsh_VecManStart( 1000 );    // hash table for constants
    Hsh_VecMan_t * pHashM = Hsh_VecManStart( 1000 );    // hash table for monomials
    Vec_Int_t * vCoefs    = Vec_IntAlloc( 1000 );       // monomial coefficients
    Vec_Int_t * vTempC    = Vec_IntAlloc( 10 );         // temporary array
    Vec_Int_t * vTempM    = Vec_IntAlloc( 10 );         // temporary array
    int i, k, iObj, iLit, iMono, nMonos = 0, nBuilds = 0;

    // add 0-constant and 1-monomial
    Hsh_VecManAdd( pHashC, vTempC );
    Hsh_VecManAdd( pHashM, vTempM );
    Vec_IntPush( vCoefs, 0 );

    // create output signature
    Vec_IntForEachEntry( vRootLits, iLit, i )
    {
        int Value = 1 + Abc_MinInt( i, Vec_IntSize(vRootLits)-nExtra );
        Vec_IntFill( vTempC, 1, (fSigned && i == Vec_IntSize(vRootLits)-1-nExtra) ? -Value : Value );
        Vec_IntFill( vTempM, 1, iLit );
        nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC, vTempM );
        nBuilds++;
    }

    // perform construction for internal nodes
    Vec_IntForEachEntryReverse( vNodes, iObj, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj( pGia, iObj );
        int iLits[2] = { Abc_Var2Lit(iObj, 0),         Abc_Var2Lit(iObj, 1)         };
        int iFans[2] = { Gia_ObjFaninLit0(pObj, iObj), Gia_ObjFaninLit1(pObj, iObj) };
        // add inverter
        Vec_Int_t * vArray = Vec_WecEntry( vLit2Mono, iLits[1] );
        Vec_IntForEachEntry( vArray, iMono, k )
            if ( Vec_IntEntry(vCoefs, iMono) > 0 )
            {
                nMonos += Gia_PolynHandleOne( pHashC, pHashM, vCoefs, vLit2Mono, vTempC, vTempM, iMono, iLits[1], -1, -1 );
                nMonos += Gia_PolynHandleOne( pHashC, pHashM, vCoefs, vLit2Mono, vTempC, vTempM, iMono, iLits[1], iLits[0], -1 );
                Vec_IntWriteEntry( vCoefs, iMono, 0 );
                nMonos--;
                nBuilds++;
                nBuilds++;
            }
        // add AND gate
        vArray = Vec_WecEntry( vLit2Mono, iLits[0] );
        Vec_IntForEachEntry( vArray, iMono, k )
            if ( Vec_IntEntry(vCoefs, iMono) > 0 )
            {
                nMonos += Gia_PolynHandleOne( pHashC, pHashM, vCoefs, vLit2Mono, vTempC, vTempM, iMono, iLits[0], iFans[0], iFans[1] );
                Vec_IntWriteEntry( vCoefs, iMono, 0 );
                nMonos--;
                nBuilds++;
            }
        //printf( "Obj %5d : nMonos = %6d  nUsed = %6d\n", iObj, nBuilds, nMonos );
    }

    // complement leave nodes
    Vec_IntForEachEntry( vLeaves, iObj, i )
    {
        int iLits[2] = { Abc_Var2Lit(iObj, 0),  Abc_Var2Lit(iObj, 1) };
        // add inverter
        Vec_Int_t * vArray = Vec_WecEntry( vLit2Mono, iLits[1] );
        Vec_IntForEachEntry( vArray, iMono, k )
            if ( Vec_IntEntry(vCoefs, iMono) > 0 )
            {
                nMonos += Gia_PolynHandleOne( pHashC, pHashM, vCoefs, vLit2Mono, vTempC, vTempM, iMono, iLits[1], -1, -1 );
                nMonos += Gia_PolynHandleOne( pHashC, pHashM, vCoefs, vLit2Mono, vTempC, vTempM, iMono, iLits[1], iLits[0], -1 );
                Vec_IntWriteEntry( vCoefs, iMono, 0 );
                nMonos--;
                nBuilds++;
            }
    }

    // get the results
    vPolyn = Gia_PolynGetResult( pHashC, pHashM, vCoefs );

    printf( "HashC = %d. HashM = %d.  Total = %d. Left = %d.  Used = %d.  ", 
        Hsh_VecSize(pHashC), Hsh_VecSize(pHashM), nBuilds, nMonos, Vec_WecSize(vPolyn)/2 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    Vec_IntFree( vTempC );
    Vec_IntFree( vTempM );
    Vec_IntFree( vCoefs );
    Vec_WecFree( vLit2Mono );
    Hsh_VecManStop( pHashC );
    Hsh_VecManStop( pHashM );
    return vPolyn;
}


/**Function*************************************************************

  Synopsis    [Computing for objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_PolynPrepare2( Vec_Int_t * vTempC[2], Vec_Int_t * vTempM[2], int iObj, int iCst )
{
    Vec_IntFill( vTempC[0], 1, iCst );
    Vec_IntFill( vTempC[1], 1, -iCst );
    Vec_IntClear( vTempM[0] );
    Vec_IntFill( vTempM[1], 1, iObj );
}
static inline void Gia_PolynPrepare4( Vec_Int_t * vTempC[4], Vec_Int_t * vTempM[4], Vec_Int_t * vConst, Vec_Int_t * vMono, int iObj, int iFan0, int iFan1 )
{
    int i, k, Entry;
    for ( i = 0; i < 4; i++ )
        Vec_IntAppendMinusAbs( vTempC[i], vConst, i & 1 );
    for ( i = 0; i < 4; i++ )
        Vec_IntClear( vTempM[i] );
    Vec_IntForEachEntry( vMono, Entry, k )
        if ( Entry != iObj )
            for ( i = 0; i < 4; i++ )
                Vec_IntPush( vTempM[i], Entry );
    Vec_IntPushUniqueOrder( vTempM[1], iFan0 );
    Vec_IntPushUniqueOrder( vTempM[2], iFan1 );
    Vec_IntPushUniqueOrder( vTempM[3], iFan0 );
    Vec_IntPushUniqueOrder( vTempM[3], iFan1 );
}

Vec_Wec_t * Gia_PolynBuildNew( Gia_Man_t * pGia, Vec_Int_t * vRootLits, int nExtra, Vec_Int_t * vLeaves, Vec_Int_t * vNodes, int fSigned, int fVerbose, int fVeryVerbose )
{
    abctime clk = Abc_Clock();
    Vec_Wec_t * vPolyn;
    Vec_Wec_t * vLit2Mono = Vec_WecStart( Gia_ManObjNum(pGia) ); // mapping AIG literals into monomials
    Hsh_VecMan_t * pHashC = Hsh_VecManStart( 1000 );    // hash table for constants
    Hsh_VecMan_t * pHashM = Hsh_VecManStart( 1000 );    // hash table for monomials
    Vec_Int_t * vCoefs    = Vec_IntAlloc( 1000 );       // monomial coefficients
    Vec_Int_t * vTempC[4],  * vTempM[4];                // temporary array
    int i, k, iObj, iLit, iMono, iConst, nMonos = 0, nBuilds = 0;
    for ( i = 0; i < 4; i++ )
        vTempC[i] = Vec_IntAlloc( 10 );
    for ( i = 0; i < 4; i++ )
        vTempM[i] = Vec_IntAlloc( 10 );

    // add 0-constant and 1-monomial
    Hsh_VecManAdd( pHashC, vTempC[0] );
    Hsh_VecManAdd( pHashM, vTempM[0] );
    Vec_IntPush( vCoefs, 0 );

    if ( nExtra )
        printf( "Assigning %d outputs from %d to %d rank %d.\n", nExtra, Vec_IntSize(vRootLits)-nExtra, Vec_IntSize(vRootLits)-1, Vec_IntSize(vRootLits)-nExtra );

    // create output signature
    Vec_IntForEachEntry( vRootLits, iLit, i )
    {
        int Value = 1 + Abc_MinInt( i, Vec_IntSize(vRootLits)-nExtra );
        Gia_PolynPrepare2( vTempC, vTempM, Abc_Lit2Var(iLit), Value );
        if ( fSigned && i >= Vec_IntSize(vRootLits)-nExtra-1 )
        {
            if ( fVeryVerbose ) printf( "Out %d : Negative   Value = %d\n", i, Value-1 );
            if ( Abc_LitIsCompl(iLit) )
            {
                nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[1], vTempM[0] );   // -C
                nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[0], vTempM[1] );   //  C * Driver
                nBuilds++;
            }
            else
                nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[1], vTempM[1] );   // -C * Driver
        }
        else 
        {
            if ( fVeryVerbose ) printf( "Out %d : Positive   Value = %d\n", i, Value-1 );
            if ( Abc_LitIsCompl(iLit) )
            {
                nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[0], vTempM[0] );   //  C
                nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[1], vTempM[1] );   // -C * Driver
                nBuilds++;
            }
            else
                nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[0], vTempM[1] );   //  C * Driver
        }
        nBuilds++;
    }

    // perform construction for internal nodes
    Vec_IntForEachEntryReverse( vNodes, iObj, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj( pGia, iObj );
        Vec_Int_t * vArray = Vec_WecEntry( vLit2Mono, iObj );
        Vec_IntForEachEntry( vArray, iMono, k )
            if ( (iConst = Vec_IntEntry(vCoefs, iMono)) > 0 )
            {
                Vec_Int_t * vArrayC = Hsh_VecReadEntry( pHashC, iConst );
                Vec_Int_t * vArrayM = Hsh_VecReadEntry( pHashM, iMono );
                Gia_PolynPrepare4( vTempC, vTempM, vArrayC, vArrayM, iObj, Gia_ObjFaninId0(pObj, iObj), Gia_ObjFaninId1(pObj, iObj) );
                if ( Gia_ObjIsXor(pObj) )
                {
                }
                else if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )  //  C * (1 - x) * (1 - y)
                {
                    nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[0], vTempM[0] );   //  C * 1
                    nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[1], vTempM[1] );   // -C * x
                    nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[3], vTempM[2] );   // -C * y 
                    nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[2], vTempM[3] );   //  C * x * y
                    nBuilds += 3;
                }
                else if ( Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) ) //  C * (1 - x) * y
                {
                    nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[0], vTempM[2] );   //  C * y 
                    nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[1], vTempM[3] );   // -C * x * y
                    nBuilds += 2;
                }
                else if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) ) //  C * x * (1 - y)
                {
                    nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[0], vTempM[1] );   //  C * x 
                    nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[1], vTempM[3] );   // -C * x * y
                    nBuilds++;
                }
                else   
                    nMonos += Gia_PolynBuildAdd( pHashC, pHashM, vCoefs, vLit2Mono, vTempC[0], vTempM[3] ); //  C * x * y
                Vec_IntWriteEntry( vCoefs, iMono, 0 );
                nMonos--;
                nBuilds++;
            }
        //printf( "Obj %5d : nMonos = %6d  nUsed = %6d\n", iObj, nBuilds, nMonos );
    }

    // get the results
    vPolyn = Gia_PolynGetResult( pHashC, pHashM, vCoefs );

    printf( "HashC = %d. HashM = %d.  Total = %d. Left = %d.  Used = %d.  ", 
        Hsh_VecSize(pHashC), Hsh_VecSize(pHashM), nBuilds, nMonos, Vec_WecSize(vPolyn)/2 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    for ( i = 0; i < 4; i++ )
        Vec_IntFree( vTempC[i] );
    for ( i = 0; i < 4; i++ )
        Vec_IntFree( vTempM[i] );
    Vec_IntFree( vCoefs );
    Vec_WecFree( vLit2Mono );
    Hsh_VecManStop( pHashC );
    Hsh_VecManStop( pHashM );
    return vPolyn;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_PolynBuild2Test( Gia_Man_t * pGia, int nExtra, int fSigned, int fVerbose, int fVeryVerbose )
{
    Vec_Wec_t * vPolyn;
    Vec_Int_t * vRootLits = Vec_IntAlloc( Gia_ManCoNum(pGia) );
    Vec_Int_t * vLeaves   = Vec_IntAlloc( Gia_ManCiNum(pGia) );
    Vec_Int_t * vNodes    = Vec_IntAlloc( Gia_ManAndNum(pGia) );
    Gia_Obj_t * pObj;
    int i;

    // print logic level
    if ( nExtra == -1 )
    {
        int LevelMax = -1, iMax = -1;
        Gia_ManLevelNum( pGia );
        Gia_ManForEachCo( pGia, pObj, i )
            if ( LevelMax < Gia_ObjLevel(pGia, pObj) )
            {
                LevelMax = Gia_ObjLevel(pGia, pObj);
                iMax = i;
            }
        nExtra = Gia_ManCoNum(pGia) - iMax - 1;
        printf( "Determined the number of extra outputs to be %d.\n", nExtra );
    }

    Gia_ManForEachObj( pGia, pObj, i )
        if ( Gia_ObjIsCi(pObj) )
            Vec_IntPush( vLeaves, i );
        else if ( Gia_ObjIsAnd(pObj) )
            Vec_IntPush( vNodes, i );
        else if ( Gia_ObjIsCo(pObj) )
            Vec_IntPush( vRootLits, Gia_ObjFaninLit0p(pGia, pObj) );

    vPolyn = Gia_PolynBuildNew( pGia, vRootLits, nExtra, vLeaves, vNodes, fSigned, fVerbose, fVeryVerbose );
    //printf( "Polynomial has %d monomials.\n", Vec_WecSize(vPolyn)/2 );
    if ( fVerbose || fVeryVerbose )
        Gia_PolynPrintStats( vPolyn );
    if ( fVeryVerbose )
        Gia_PolynPrint( vPolyn );
    Vec_WecFree( vPolyn );

    Vec_IntFree( vRootLits );
    Vec_IntFree( vLeaves );
    Vec_IntFree( vNodes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

