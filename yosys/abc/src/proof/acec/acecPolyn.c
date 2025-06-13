/**CFile****************************************************************

  FileName    [acecPolyn.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Polynomial extraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecPolyn.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecWec.h"
#include "misc/vec/vecHsh.h"
#include "misc/vec/vecQue.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/*
!a            ->   1 - a
a & b         ->   ab
a | b         ->   a + b - ab
a ^ b         ->   a + b - 2ab
MUX(a, b, c)  ->   ab | (1 - a)c = ab + (1-a)c - ab(1-a)c = ab + c - ac

!a & b        ->   (1 - a)b = b - ab
 a & !b       ->   a(1 - b) = a - ab
!a & !b       ->   1 - a - b + ab
*/

typedef struct Pln_Man_t_ Pln_Man_t;
struct Pln_Man_t_
{
    Gia_Man_t *    pGia;      // AIG manager
    Hsh_VecMan_t * pHashC;    // hash table for constants
    Hsh_VecMan_t * pHashM;    // hash table for monomials
    Vec_Que_t *    vQue;      // queue by largest node
    Vec_Flt_t *    vCounts;   // largest node
    Vec_Int_t *    vCoefs;    // coefficients for each monomial
    Vec_Int_t *    vTempC[2]; // polynomial representation
    Vec_Int_t *    vTempM[4]; // polynomial representation
    Vec_Int_t *    vOrder;    // order of collapsing
    int            nBuilds;   // built monomials
    int            nUsed;     // used monomials
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Pln_Man_t * Pln_ManAlloc( Gia_Man_t * pGia, Vec_Int_t * vOrder )
{
    Pln_Man_t * p = ABC_CALLOC( Pln_Man_t, 1 );
    p->pGia      = pGia;
    p->pHashC    = Hsh_VecManStart( 1000 );
    p->pHashM    = Hsh_VecManStart( 1000 );
    p->vQue      = Vec_QueAlloc( 1000 );
    p->vCounts   = Vec_FltAlloc( 1000 );
    p->vCoefs    = Vec_IntAlloc( 1000 );
    p->vTempC[0] = Vec_IntAlloc( 100 );
    p->vTempC[1] = Vec_IntAlloc( 100 );
    p->vTempM[0] = Vec_IntAlloc( 100 );
    p->vTempM[1] = Vec_IntAlloc( 100 );
    p->vTempM[2] = Vec_IntAlloc( 100 );
    p->vTempM[3] = Vec_IntAlloc( 100 );
    p->vOrder    = vOrder ? Vec_IntDup(vOrder) : Vec_IntStartNatural( Gia_ManObjNum(pGia) );
    assert( Vec_IntSize(p->vOrder) == Gia_ManObjNum(pGia) );
    Vec_QueSetPriority( p->vQue, Vec_FltArrayP(p->vCounts) );
    // add 0-constant and 1-monomial
    Hsh_VecManAdd( p->pHashC, p->vTempC[0] );
    Hsh_VecManAdd( p->pHashM, p->vTempM[0] );
    Vec_FltPush( p->vCounts, 0 );
    Vec_IntPush( p->vCoefs, 0 );
    return p;
}
void Pln_ManStop( Pln_Man_t * p )
{
    Hsh_VecManStop( p->pHashC );
    Hsh_VecManStop( p->pHashM );
    Vec_QueFree( p->vQue );
    Vec_FltFree( p->vCounts );
    Vec_IntFree( p->vCoefs );
    Vec_IntFree( p->vTempC[0] );
    Vec_IntFree( p->vTempC[1] );
    Vec_IntFree( p->vTempM[0] );
    Vec_IntFree( p->vTempM[1] );
    Vec_IntFree( p->vTempM[2] );
    Vec_IntFree( p->vTempM[3] );
    Vec_IntFree( p->vOrder );
    ABC_FREE( p );
}
int Pln_ManCompare3( int * pData0, int * pData1 )
{
    if ( pData0[0] < pData1[0] ) return -1;
    if ( pData0[0] > pData1[0] ) return  1;
    if ( pData0[1] < pData1[1] ) return -1;
    if ( pData0[1] > pData1[1] ) return  1;
    return 0;
}
void Pln_ManPrintFinal( Pln_Man_t * p, int fVerbose, int fVeryVerbose )
{
    Vec_Int_t * vArray;
    int i, k, Entry, iMono, iConst;
    // collect triples
    Vec_Int_t * vPairs = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( p->vCoefs, iConst, iMono )
    {
        if ( iConst == 0 ) 
            continue;
        vArray = Hsh_VecReadEntry( p->pHashC, iConst );
        Vec_IntPush( vPairs, Vec_IntEntry(vArray, 0) );
        vArray = Hsh_VecReadEntry( p->pHashM, iMono );
        Vec_IntPush( vPairs, Vec_IntSize(vArray) ? Vec_IntEntry(vArray, 0) : 0 );
        Vec_IntPushTwo( vPairs, iConst, iMono );
    }
    // sort triples
    qsort( Vec_IntArray(vPairs), (size_t)(Vec_IntSize(vPairs)/4), 16, (int (*)(const void *, const void *))Pln_ManCompare3 );
    // print
    if ( fVerbose )
    Vec_IntForEachEntryDouble( vPairs, iConst, iMono, i )
    {
        if ( i % 4 == 0 )
            continue;
        printf( "%-6d : ", i/4 );
        vArray = Hsh_VecReadEntry( p->pHashC, iConst );
        Vec_IntForEachEntry( vArray, Entry, k )
            printf( "%s%d", Entry < 0 ? "-" : "+", (1 << (Abc_AbsInt(Entry)-1)) );
        vArray = Hsh_VecReadEntry( p->pHashM, iMono );
        Vec_IntForEachEntry( vArray, Entry, k )
            printf( " * %d", Entry );
        printf( "\n" );
    }
    printf( "HashC = %d. HashM = %d.  Total = %d. Used = %d.  ", Hsh_VecSize(p->pHashC), Hsh_VecSize(p->pHashM), p->nBuilds, Vec_IntSize(vPairs)/4 );
    Vec_IntFree( vPairs );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntAppendMinus2x( Vec_Int_t * vVec1, Vec_Int_t * vVec2 )
{
    int Entry, i;
    Vec_IntClear( vVec1 );
    Vec_IntForEachEntry( vVec2, Entry, i )
        Vec_IntPush( vVec1, Entry > 0 ? -Entry-1 : -Entry+1 );
}

/**Function*************************************************************

  Synopsis    []

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
    Vec_IntPushUniqueOrder( vConst, New );
}
static inline void Gia_PolynMergeConst( Vec_Int_t * vConst, Pln_Man_t * p, int iConstAdd )
{
    int i, New;
    Vec_Int_t * vConstAdd = Hsh_VecReadEntry( p->pHashC, iConstAdd );
    Vec_IntForEachEntry( vConstAdd, New, i )
    {
        Gia_PolynMergeConstOne( vConst, New );
        vConstAdd = Hsh_VecReadEntry( p->pHashC, iConstAdd );
    }
}
static inline void Gia_PolynBuildAdd( Pln_Man_t * p, Vec_Int_t * vTempC, Vec_Int_t * vTempM )
{
    int iConst, iConstNew, iMono = vTempM ? Hsh_VecManAdd(p->pHashM, vTempM) : 0;
    p->nBuilds++;
    if ( iMono == Vec_IntSize(p->vCoefs) ) // new monomial
    {
        iConst = Hsh_VecManAdd( p->pHashC, vTempC );
        Vec_IntPush( p->vCoefs, iConst );
//        Vec_FltPush( p->vCounts, Vec_IntEntryLast(vTempM) );
        Vec_FltPush( p->vCounts, (float)Vec_IntEntry(p->vOrder, Vec_IntEntryLast(vTempM)) );
        Vec_QuePush( p->vQue, iMono );
//        Vec_QueUpdate( p->vQue, iMono );
        if ( iConst )
            p->nUsed++;
        return;
    }
    // this monomial exists
    iConst = Vec_IntEntry( p->vCoefs, iMono );
    if ( iConst )
        Gia_PolynMergeConst( vTempC, p, iConst );
    iConstNew = Hsh_VecManAdd( p->pHashC, vTempC );
    Vec_IntWriteEntry( p->vCoefs, iMono, iConstNew );
    if ( iConst && !iConstNew )
        p->nUsed--;
    else if ( !iConst && iConstNew )
        p->nUsed++;
    //assert( p->nUsed == Vec_IntSize(p->vCoefs) - Vec_IntCountZero(p->vCoefs) );
}
void Gia_PolynBuildOne( Pln_Man_t * p, int iMono )
{
    Gia_Obj_t * pObj; 
    Vec_Int_t * vArray, * vConst;
    int iFan0, iFan1;
    int k, iConst, iDriver;

    assert( Vec_IntSize(p->vCoefs) == Hsh_VecSize(p->pHashM) );
    vArray  = Hsh_VecReadEntry( p->pHashM, iMono );
    iDriver = Vec_IntEntryLast( vArray );
    pObj    = Gia_ManObj( p->pGia, iDriver );
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    assert( !Gia_ObjIsMux(p->pGia, pObj) );

    iConst = Vec_IntEntry( p->vCoefs, iMono );
    if ( iConst == 0 )
        return;
    Vec_IntWriteEntry( p->vCoefs, iMono, 0 );
    p->nUsed--;

    iFan0 = Gia_ObjFaninId0p(p->pGia, pObj);
    iFan1 = Gia_ObjFaninId1p(p->pGia, pObj);
    for ( k = 0; k < 4; k++ )
    {
        Vec_IntClear( p->vTempM[k] );
        Vec_IntAppend( p->vTempM[k], vArray );
        Vec_IntPop( p->vTempM[k] );
        if ( k == 1 || k == 3 )
            Vec_IntPushUniqueOrderCost( p->vTempM[k], iFan0, p->vOrder );    // x
//            Vec_IntPushUniqueOrder( p->vTempM[k], iFan0 );    // x
        if ( k == 2 || k == 3 )
            Vec_IntPushUniqueOrderCost( p->vTempM[k], iFan1, p->vOrder );    // y
//            Vec_IntPushUniqueOrder( p->vTempM[k], iFan1 );    // y
    }

    vConst = Hsh_VecReadEntry( p->pHashC, iConst );

    if ( !Gia_ObjIsXor(pObj) )
        for ( k = 0; k < 2; k++ )
            Vec_IntAppendMinus( p->vTempC[k], vConst, k );

    if ( Gia_ObjIsXor(pObj) )
    {
        vConst = Hsh_VecReadEntry( p->pHashC, iConst );
        Vec_IntAppendMinus( p->vTempC[0], vConst, 0 );
        Gia_PolynBuildAdd( p, p->vTempC[0], p->vTempM[1] );   //  C * x 

        vConst = Hsh_VecReadEntry( p->pHashC, iConst );
        Vec_IntAppendMinus( p->vTempC[0], vConst, 0 );
        Gia_PolynBuildAdd( p, p->vTempC[0], p->vTempM[2] );   //  C * y 

        //if ( !p->pGia->vXors || Vec_IntFind(p->pGia->vXors, iDriver) == -1 || Vec_IntFind(p->pGia->vXors, iDriver) == 5 )
        { 
            vConst = Hsh_VecReadEntry( p->pHashC, iConst );
            Vec_IntAppendMinus2x( p->vTempC[0], vConst );
            Gia_PolynBuildAdd( p, p->vTempC[0], p->vTempM[3] );   // -C * x * y 
        }
    }
    else if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )  //  C * (1 - x) * (1 - y)
    {
        Gia_PolynBuildAdd( p, p->vTempC[0], p->vTempM[0] );   //  C * 1
        Gia_PolynBuildAdd( p, p->vTempC[1], p->vTempM[1] );   // -C * x
        vConst = Hsh_VecReadEntry( p->pHashC, iConst );
        for ( k = 0; k < 2; k++ )
            Vec_IntAppendMinus( p->vTempC[k], vConst, k );
        Gia_PolynBuildAdd( p, p->vTempC[1], p->vTempM[2] );   // -C * y 
        Gia_PolynBuildAdd( p, p->vTempC[0], p->vTempM[3] );   //  C * x * y
    }
    else if ( Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) ) //  C * (1 - x) * y
    {
        Gia_PolynBuildAdd( p, p->vTempC[0], p->vTempM[2] );   //  C * y 
        Gia_PolynBuildAdd( p, p->vTempC[1], p->vTempM[3] );   // -C * x * y
    }
    else if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) ) //  C * x * (1 - y)
    {
        Gia_PolynBuildAdd( p, p->vTempC[0], p->vTempM[1] );   //  C * x 
        Gia_PolynBuildAdd( p, p->vTempC[1], p->vTempM[3] );   // -C * x * y
    }
    else   
        Gia_PolynBuildAdd( p, p->vTempC[0], p->vTempM[3] );   //  C * x * y
}
void Gia_PolynBuild( Gia_Man_t * pGia, Vec_Int_t * vOrder, int fSigned, int fVerbose, int fVeryVerbose )
{
    abctime clk = Abc_Clock();//, clk2 = 0;
    Gia_Obj_t * pObj; 
    Vec_Bit_t * vPres = Vec_BitStart( Gia_ManObjNum(pGia) );
    int i, iMono, iDriver, LevPrev, LevCur, Iter, Line = 0;
    Pln_Man_t * p = Pln_ManAlloc( pGia, vOrder );
    Gia_ManForEachCoReverse( pGia, pObj, i )
    {
        Vec_IntFill( p->vTempC[0], 1,  i+1 );      //  2^i
        Vec_IntFill( p->vTempC[1], 1, -i-1 );      // -2^i

        iDriver = Gia_ObjFaninId0p( pGia, pObj );
        Vec_IntFill( p->vTempM[0], 1, iDriver );   //  Driver

        if ( fSigned && i == Gia_ManCoNum(pGia)-1 )
        {
            if ( Gia_ObjFaninC0(pObj) )
            {
                Gia_PolynBuildAdd( p, p->vTempC[1], NULL );           // -C
                Gia_PolynBuildAdd( p, p->vTempC[0], p->vTempM[0] );   //  C * Driver
            }
            else
                Gia_PolynBuildAdd( p, p->vTempC[1], p->vTempM[0] );   // -C * Driver
        }
        else 
        {
            if ( Gia_ObjFaninC0(pObj) )
            {
                Gia_PolynBuildAdd( p, p->vTempC[0], NULL );           //  C
                Gia_PolynBuildAdd( p, p->vTempC[1], p->vTempM[0] );   // -C * Driver
            }
            else
                Gia_PolynBuildAdd( p, p->vTempC[0], p->vTempM[0] );   //  C * Driver
        }
    }
    LevPrev = -1;
    for ( Iter = 0; ; Iter++ )
    {
        Vec_Int_t * vTempM;
        //abctime temp = Abc_Clock();
        if ( Vec_QueSize(p->vQue) == 0 )
            break;
        iMono = Vec_QuePop(p->vQue);

        // report
        vTempM = Hsh_VecReadEntry( p->pHashM, iMono );
        //printf( "Removing var %d\n", Vec_IntEntryLast(vTempM) );
        LevCur = Vec_IntEntryLast(vTempM);
        if ( !Gia_ObjIsAnd(Gia_ManObj(pGia, LevCur)) )
            continue;

        if ( LevPrev != LevCur )
        {
            if ( Vec_BitEntry( vPres, LevCur ) && fVerbose )
                printf( "Repeating entry %d\n", LevCur );
            else
                Vec_BitSetEntry( vPres, LevCur, 1 );

            if ( fVeryVerbose )
                printf( "Line%5d   Iter%10d : Obj =%6d.  Order =%6d.  HashC =%6d. HashM =%10d.  Total =%10d. Used =%10d.\n", 
                    Line++, Iter, LevCur, Vec_IntEntry(p->vOrder, LevCur), Hsh_VecSize(p->pHashC), Hsh_VecSize(p->pHashM), p->nBuilds, p->nUsed );
        }
        LevPrev = LevCur;

        Gia_PolynBuildOne( p, iMono );
        //clk2 += Abc_Clock() - temp;
    }
    //Abc_PrintTime( 1, "Time2", clk2 );
    Pln_ManPrintFinal( p, fVerbose, fVeryVerbose );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Pln_ManStop( p );
    Vec_BitFree( vPres );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_PolynBuild2( Gia_Man_t * pGia, int fSigned, int fVerbose, int fVeryVerbose )
{
    Hsh_VecMan_t * pHashC = Hsh_VecManStart( 1000 );    // hash table for constants
    Hsh_VecMan_t * pHashM = Hsh_VecManStart( 1000 );    // hash table for monomials
    //Vec_Wec_t * vLit2Mono = Vec_WecStart( Gia_ManObjNum(pGia) * 2 );

    Hsh_VecManStop( pHashC );
    Hsh_VecManStop( pHashM );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

