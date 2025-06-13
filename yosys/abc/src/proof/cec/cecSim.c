/**CFile****************************************************************

  FileName    [cecSim.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Simulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecSim.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cecInt.h"
#include "aig/gia/giaAig.h"
#include "misc/util/utilTruth.h"


ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define SIM_RANDS    113

typedef struct Cec_ManS_t_ Cec_ManS_t;
struct Cec_ManS_t_
{
    int              nWords;
    int              nLevelMax;
    int              nLevelMin;
    int              iRand;
    Gia_Man_t *      pAig;       
    Vec_Int_t *      vInputs;
    Vec_Wec_t *      vLevels;
    Vec_Wrd_t *      vSims;
    word *           pTemp[4];
    word             Rands[SIM_RANDS];
    int              nSkipped;
    int              nVisited;
    int              nCexes;
    abctime          clkSat;
    abctime          clkUnsat;
};

static inline word * Cec_ManSSim( Cec_ManS_t * p, int iNode, int Value ) { return Vec_WrdEntryP(p->vSims, p->nWords*(iNode+iNode+Value)); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec_ManS_t * Cec_ManSStart( Gia_Man_t * pAig, int nWords )
{
    Cec_ManS_t * p; int i;
    p = ABC_ALLOC( Cec_ManS_t, 1 );
    memset( p, 0, sizeof(Cec_ManS_t) );
    p->nWords    = nWords;
    p->pAig      = pAig;
    p->vInputs   = Vec_IntAlloc( 100 );
    p->vLevels   = Vec_WecStart( Gia_ManLevelNum(pAig) + 1 );
    p->vSims     = Vec_WrdStart( Gia_ManObjNum(pAig) * nWords * 2 );
    p->pTemp[0]  = ABC_ALLOC( word, 4*nWords );
    for ( i = 1; i < 4; i++ )
        p->pTemp[i] = p->pTemp[i-1] + nWords;
    for ( i = 0; i < SIM_RANDS; i++ )
        p->Rands[i] = Gia_ManRandomW(0);
    return p;
}
void Cec_ManSStop( Cec_ManS_t * p )
{
    Vec_IntFree( p->vInputs );
    Vec_WecFree( p->vLevels );
    Vec_WrdFree( p->vSims );
    ABC_FREE( p->pTemp[0] );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Verify counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec_ManSVerify_rec( Gia_Man_t * p, int iObj )
{
    int Value0, Value1;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( iObj == 0 ) return 0;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return pObj->fMark1;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
        return pObj->fMark1;
    assert( Gia_ObjIsAnd(pObj) );
    Value0 = Cec_ManSVerify_rec( p, Gia_ObjFaninId0(pObj, iObj) ) ^ Gia_ObjFaninC0(pObj);
    Value1 = Cec_ManSVerify_rec( p, Gia_ObjFaninId1(pObj, iObj) ) ^ Gia_ObjFaninC1(pObj);
    return pObj->fMark1 = Gia_ObjIsXor(pObj) ? Value0 ^ Value1 : Value0 & Value1;
}
void Cec_ManSVerifyTwo( Gia_Man_t * p, int iObj0, int iObj1 )
{
    int Value0, Value1;
    Gia_ManIncrementTravId( p );
    Value0 = Cec_ManSVerify_rec( p, iObj0 );
    Value1 = Cec_ManSVerify_rec( p, iObj1 );
    if ( (Value0 ^ Value1) == Gia_ObjPhaseDiff(p, iObj0, iObj1) )
        printf( "CEX verification FAILED for obj %d and obj %d.\n", iObj0, iObj1 );
//    else
//        printf( "CEX verification succeeded for obj %d and obj %d.\n", iObj0, iObj1 );;
}
void Cec_ManSVerify( Cec_ManS_t * p, int iObj0, int iObj1 )
{
    int fDoVerify = 0;
    int w, i, iObj, nCares;
    word * pCare = Vec_WrdArray(p->vSims);
    if ( Vec_IntSize(p->vInputs) == 0 )
    {
        printf( "No primary inputs.\n" );
        return;
    }
    Vec_IntForEachEntry( p->vInputs, iObj, i )
    {
        word * pSim_0 = Cec_ManSSim( p, iObj, 0 );
        word * pSim_1 = Cec_ManSSim( p, iObj, 1 );
        if ( p->nWords == 1 )
            pCare[0] |= pSim_0[0] & pSim_1[0];
        else
            Abc_TtOrAnd( pCare, pSim_0, pSim_1, p->nWords );
    }
    nCares = Abc_TtCountOnesVec( pCare, p->nWords );
    if ( nCares == 64*p->nWords )
    {
        printf( "No CEXes.\n" );
        return;
    }
    assert( Vec_IntSize(p->vInputs) > 0 );
    for ( w = 0; w < 64*p->nWords; w++ )
    {
        if ( Abc_TtGetBit(pCare, w) )
            continue;
        if ( !fDoVerify )
            continue;
        Vec_IntForEachEntry( p->vInputs, iObj, i )
            Gia_ManObj(p->pAig, iObj)->fMark1 = Abc_TtGetBit( Cec_ManSSim(p, iObj, 1), w );
        Cec_ManSVerifyTwo( p->pAig, iObj0, iObj1 );
    }
    printf( "Considered %d CEXes of nodes %d and %d.\n", 64*p->nWords - nCares, iObj0, iObj1 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManSRunImply( Cec_ManS_t * p, int iNode )
{
    Gia_Obj_t * pNode = Gia_ManObj( p->pAig, iNode );
    if ( Gia_ObjIsAnd(pNode) )
    {
        int iFan0 = Gia_ObjFaninId0( pNode, iNode );
        int iFan1 = Gia_ObjFaninId1( pNode, iNode );
        word * pSim__ = Cec_ManSSim( p, 0, 0 );
        word * pSim_0 = Cec_ManSSim( p, iNode, 0 );
        word * pSim_1 = Cec_ManSSim( p, iNode, 1 );
        word * pSim00 = Cec_ManSSim( p, iFan0,  Gia_ObjFaninC0(pNode) );
        word * pSim01 = Cec_ManSSim( p, iFan0, !Gia_ObjFaninC0(pNode) );
        word * pSim10 = Cec_ManSSim( p, iFan1,  Gia_ObjFaninC1(pNode) );
        word * pSim11 = Cec_ManSSim( p, iFan1, !Gia_ObjFaninC1(pNode) );
        if ( p->nWords == 1 )
        {
            pSim_0[0] |= pSim00[0] | pSim10[0];
            pSim_1[0] |= pSim01[0] & pSim11[0];
            pSim__[0] |= pSim_0[0] & pSim_1[0];
            pSim_0[0] &= ~pSim__[0];
            pSim_1[0] &= ~pSim__[0];
        }
        else
        {
            Abc_TtOr( pSim_0, pSim_0, pSim00, p->nWords );
            Abc_TtOr( pSim_0, pSim_0, pSim10, p->nWords );
            Abc_TtOrAnd( pSim_1, pSim01, pSim11, p->nWords );
            Abc_TtOrAnd( pSim__, pSim_0, pSim_1, p->nWords );
            Abc_TtAndSharp( pSim_0, pSim_0, pSim__, p->nWords, 1 );
            Abc_TtAndSharp( pSim_1, pSim_1, pSim__, p->nWords, 1 );
        }
    }
}
int Cec_ManSRunPropagate( Cec_ManS_t * p, int iNode )
{
    Gia_Obj_t * pNode = Gia_ManObj( p->pAig, iNode );
    int iFan0 = Gia_ObjFaninId0( pNode, iNode );
    int iFan1 = Gia_ObjFaninId1( pNode, iNode );
    word * pSim_0 = Cec_ManSSim( p, iNode, 0 );
    word * pSim_1 = Cec_ManSSim( p, iNode, 1 );
    if ( Abc_TtIsConst0(pSim_0, p->nWords) && Abc_TtIsConst0(pSim_1, p->nWords) )
    {
        p->nSkipped++;
        return 0;
    }
    p->nVisited++;
    //Cec_ManSRunImply( p, iFan0 );
    //Cec_ManSRunImply( p, iFan1 );
    {
        word * pSim__ = Cec_ManSSim( p, 0, 0 );
        word * pSim00 = Cec_ManSSim( p, iFan0,  Gia_ObjFaninC0(pNode) );
        word * pSim01 = Cec_ManSSim( p, iFan0, !Gia_ObjFaninC0(pNode) );
        word * pSim10 = Cec_ManSSim( p, iFan1,  Gia_ObjFaninC1(pNode) );
        word * pSim11 = Cec_ManSSim( p, iFan1, !Gia_ObjFaninC1(pNode) );
        p->iRand = p->iRand == SIM_RANDS-1 ? 0 : p->iRand + 1;
        if ( p->nWords == 1 )
        {
            pSim01[0] |= pSim_1[0];
            pSim11[0] |= pSim_1[0];

            pSim00[0] |= pSim_0[0] & (pSim11[0] | ~p->Rands[p->iRand]);
            pSim10[0] |= pSim_0[0] & (pSim01[0] |  p->Rands[p->iRand]);

            pSim__[0] |= pSim00[0] & pSim01[0];
            pSim__[0] |= pSim10[0] & pSim11[0];

            pSim00[0] &= ~pSim__[0];
            pSim01[0] &= ~pSim__[0];
            pSim10[0] &= ~pSim__[0];
            pSim11[0] &= ~pSim__[0];
        }
        else
        {
            int w;
            for ( w = 0; w < p->nWords; w++ )
                p->pTemp[0][w] = ~p->Rands[(p->iRand + w) % SIM_RANDS];

            Abc_TtOr( pSim01, pSim01, pSim_1, p->nWords );
            Abc_TtOr( pSim11, pSim11, pSim_1, p->nWords );

            Abc_TtOr( p->pTemp[1], pSim11, p->pTemp[0], p->nWords );
            Abc_TtOrAnd( pSim00, pSim_0, p->pTemp[1], p->nWords );

            Abc_TtNot( p->pTemp[0], p->nWords );
            Abc_TtOr( p->pTemp[1], pSim01, p->pTemp[0], p->nWords );
            Abc_TtOrAnd( pSim10, pSim_0, p->pTemp[1], p->nWords );

            Abc_TtOrAnd( pSim__, pSim00, pSim01, p->nWords );
            Abc_TtOrAnd( pSim__, pSim10, pSim11, p->nWords );

            Abc_TtAndSharp( pSim00, pSim00, pSim__, p->nWords, 1 );
            Abc_TtAndSharp( pSim01, pSim01, pSim__, p->nWords, 1 );
            Abc_TtAndSharp( pSim10, pSim10, pSim__, p->nWords, 1 );
            Abc_TtAndSharp( pSim11, pSim11, pSim__, p->nWords, 1 );
        }
    }
    return 1;
}
void Cec_ManSInsert( Cec_ManS_t * p, int iNode )
{
    Gia_Obj_t * pNode; int Level;
    assert( iNode > 0 );
    if ( Gia_ObjIsTravIdCurrentId(p->pAig, iNode) )
        return;
    Gia_ObjSetTravIdCurrentId(p->pAig, iNode);
    pNode = Gia_ManObj( p->pAig, iNode );
    if ( Gia_ObjIsCi(pNode) )
    {
        Vec_IntPush( p->vInputs, iNode );
        return;
    }
    assert( Gia_ObjIsAnd(pNode) );
    Level = Gia_ObjLevelId( p->pAig, iNode );
    assert( Level > 0 );
    Vec_WecPush( p->vLevels, Level, iNode );
    p->nLevelMax = Abc_MaxInt( p->nLevelMax, Level );
    p->nLevelMin = Abc_MinInt( p->nLevelMin, Level );
    assert( p->nLevelMin <= p->nLevelMax );
}
int Cec_ManSRunSimInt( Cec_ManS_t * p )
{
    Vec_Int_t * vLevel; 
    int i, k, iNode, fSolved = 0;
    Vec_WecForEachLevelReverseStartStop( p->vLevels, vLevel, i, p->nLevelMax+1, p->nLevelMin )
    {
        Vec_IntForEachEntry( vLevel, iNode, k )
        {
            Gia_Obj_t * pNode = Gia_ManObj( p->pAig, iNode );
            if ( !fSolved && Cec_ManSRunPropagate( p, iNode ) )
            {
                Cec_ManSInsert( p, Gia_ObjFaninId0(pNode, iNode) );
                Cec_ManSInsert( p, Gia_ObjFaninId1(pNode, iNode) );
                if ( Abc_TtIsConst1(Vec_WrdArray(p->vSims), p->nWords) )
                    fSolved = 1;
            }
            Abc_TtClear( Cec_ManSSim(p, iNode, 0), 2*p->nWords );
        }
        Vec_IntClear( vLevel );
    }
    //Vec_WecForEachLevel( p->vLevels, vLevel, i )
    //    assert( Vec_IntSize(vLevel) == 0 );
    return fSolved;
}
int Cec_ManSRunSim( Cec_ManS_t * p, int iNode0, int iNode1 )
{
    abctime clk = Abc_Clock();
    //Vec_Int_t * vLevel; 
    //int pNodes[2] = { iNode0, iNode1 };
    int i, iNode, Status, fDiff = Gia_ObjPhaseDiff( p->pAig, iNode0, iNode1 );
    word * pSim00 = Cec_ManSSim( p, iNode0, 0 );
    word * pSim01 = Cec_ManSSim( p, iNode0, 1 );
    word * pSim10 = Cec_ManSSim( p, iNode1,  fDiff );
    word * pSim11 = Cec_ManSSim( p, iNode1, !fDiff );
    Abc_TtClear( Vec_WrdArray(p->vSims), p->nWords );
    //for ( i = 0; i < Vec_WrdSize(p->vSims); i++ )
    //    assert( p->vSims->pArray[i] == 0 );
    assert( Vec_IntSize(p->vInputs) == 0 );
    if ( iNode0 == 0 )
        Abc_TtFill( pSim11, p->nWords );
    else
    {
        if ( p->nWords == 1 )
        {
            pSim00[0] = (word)0xFFFFFFFF;
            pSim11[0] = (word)0xFFFFFFFF;
            pSim01[0] = pSim00[0] << 32;
            pSim10[0] = pSim11[0] << 32;
        }
        else
        {
            assert( p->nWords % 2 == 0 );
            Abc_TtFill( pSim00,               p->nWords/2 );
            Abc_TtFill( pSim11,               p->nWords/2 );
            Abc_TtFill( pSim01 + p->nWords/2, p->nWords/2 );
            Abc_TtFill( pSim10 + p->nWords/2, p->nWords/2 );
        }
    }
    p->nLevelMin = ABC_INFINITY;
    p->nLevelMax = 0;
    Gia_ManIncrementTravId( p->pAig );
    if ( iNode0 )
    Cec_ManSInsert( p, iNode0 );
    Cec_ManSInsert( p, iNode1 );
    p->nSkipped = p->nVisited = 0;
    Status = Cec_ManSRunSimInt( p );
    if ( Status == 0 )
        p->clkSat += Abc_Clock() - clk;
    else
        p->clkUnsat += Abc_Clock() - clk;
//    if ( Status == 0 )
//        printf( "Solving %6d and %6d.  Skipped = %6d.  Visited = %6d.  Cone = %6d.  Min = %3d.  Max = %3d.\n", 
//            iNode0, iNode1, p->nSkipped, p->nVisited, Gia_ManConeSize(p->pAig, pNodes, 2), p->nLevelMin, p->nLevelMax );
    if ( Status == 0 )
        Cec_ManSVerify( p, iNode0, iNode1 ), p->nCexes++;
    Vec_IntForEachEntry( p->vInputs, iNode, i )
        Abc_TtClear( Cec_ManSSim(p, iNode, 0), 2*p->nWords );
    Vec_IntClear( p->vInputs );
    return Status;
}
void Cec_ManSRunTest( Gia_Man_t * pAig )
{
    abctime clk = Abc_Clock();
    Cec_ManS_t * p;
    int i, k, nWords = 1;
    Gia_ManRandomW( 1 );
    p = Cec_ManSStart( pAig, nWords );
    Gia_ManForEachClass0( p->pAig, i )
        Gia_ClassForEachObj1( p->pAig, i, k )
            Cec_ManSRunSim( p, i, k );
    printf( "Detected %d CEXes.  ", p->nCexes );
    Abc_PrintTime( 1, "Time ", Abc_Clock() - clk );
    Abc_PrintTime( 1, "Sat  ", p->clkSat );
    Abc_PrintTime( 1, "Unsat", p->clkUnsat );
    Cec_ManSStop( p );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

