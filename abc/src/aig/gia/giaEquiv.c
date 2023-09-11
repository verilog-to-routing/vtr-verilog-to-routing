/**CFile****************************************************************

  FileName    [giaEquiv.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Manipulation of equivalence classes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaEquiv.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "proof/cec/cec.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Manipulating original IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManOrigIdsInit( Gia_Man_t * p )
{
    Vec_IntFreeP( &p->vIdsOrig );
    p->vIdsOrig = Vec_IntStartNatural( Gia_ManObjNum(p) );
}
void Gia_ManOrigIdsStart( Gia_Man_t * p )
{
    Vec_IntFreeP( &p->vIdsOrig );
    p->vIdsOrig = Vec_IntStartFull( Gia_ManObjNum(p) );
}
void Gia_ManOrigIdsRemap( Gia_Man_t * p, Gia_Man_t * pNew )
{
    Gia_Obj_t * pObj; int i;
    if ( p->vIdsOrig == NULL )
        return;
    Gia_ManOrigIdsStart( pNew );
    Vec_IntWriteEntry( pNew->vIdsOrig, 0, 0 );
    Gia_ManForEachObj1( p, pObj, i )
        if ( ~pObj->Value && Abc_Lit2Var(pObj->Value) && Vec_IntEntry(p->vIdsOrig, i) != -1 && Vec_IntEntry(pNew->vIdsOrig, Abc_Lit2Var(pObj->Value)) == -1 )
            Vec_IntWriteEntry( pNew->vIdsOrig, Abc_Lit2Var(pObj->Value), Vec_IntEntry(p->vIdsOrig, i) );
    Gia_ManForEachObj( pNew, pObj, i )
        assert( Vec_IntEntry(pNew->vIdsOrig, i) >= 0 );
}
// input is a set of equivalent node pairs in any order
// output is the mapping of each node into the equiv node with the smallest ID
void Gia_ManOrigIdsRemapPairsInsert( Vec_Int_t * vMap, int One, int Two )
{
    int Smo = One < Two ? One : Two;
    int Big = One < Two ? Two : One;
    assert( Smo != Big );
    if ( Vec_IntEntry(vMap, Big) == -1 )
        Vec_IntWriteEntry( vMap, Big, Smo );
    else
        Gia_ManOrigIdsRemapPairsInsert( vMap, Smo, Vec_IntEntry(vMap, Big) );
}
int Gia_ManOrigIdsRemapPairsExtract( Vec_Int_t * vMap, int One )
{
    if ( Vec_IntEntry(vMap, One) == -1 )
        return One;
    return Gia_ManOrigIdsRemapPairsExtract( vMap, Vec_IntEntry(vMap, One) );
}
Vec_Int_t * Gia_ManOrigIdsRemapPairs( Vec_Int_t * vEquivPairs, int nObjs )
{
    Vec_Int_t * vMapResult;
    Vec_Int_t * vMap2Smaller;
    int i, One, Two;
    // map bigger into smaller one
    vMap2Smaller = Vec_IntStartFull( nObjs );
    Vec_IntForEachEntryDouble( vEquivPairs, One, Two, i )
        Gia_ManOrigIdsRemapPairsInsert( vMap2Smaller, One, Two );
    // collect results in the topo order
    vMapResult = Vec_IntStartFull( nObjs );
    Vec_IntForEachEntry( vMap2Smaller, One, i )
        if ( One >= 0 )
            Vec_IntWriteEntry( vMapResult, i, Gia_ManOrigIdsRemapPairsExtract(vMap2Smaller, One) );
    Vec_IntFree( vMap2Smaller );
    return vMapResult;
}
// remap the AIG using the equivalent pairs proved
// returns the reduced AIG and the equivalence classes of the original AIG
Gia_Man_t * Gia_ManOrigIdsReduce( Gia_Man_t * p, Vec_Int_t * vPairs )
{
    Gia_Man_t * pNew = NULL;
    Gia_Obj_t * pObj, * pRepr; int i;
    Vec_Int_t * vMap = Gia_ManOrigIdsRemapPairs( vPairs, Gia_ManObjNum(p) );
    Gia_ManSetPhase( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManFillValue(p);
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Vec_IntEntry(vMap, i) == -1 )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else
        {
            pRepr = Gia_ManObj( p, Vec_IntEntry(vMap, i) );
            pObj->Value = Abc_LitNotCond( pRepr->Value, pRepr->fPhase ^ pObj->fPhase );
        }
    }
    Gia_ManHashStop( pNew );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Vec_IntFree( vMap );
    // compute equivalences
    assert( !p->pReprs && !p->pNexts );
    p->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        Gia_ObjSetRepr( p, i, GIA_VOID );
    Gia_ManFillValue(pNew);
    Gia_ManForEachAnd( p, pObj, i )
    {
        int iRepr = Abc_Lit2Var(pObj->Value);
        if ( iRepr == 0 )
        {
            Gia_ObjSetRepr( p, i, 0 );
            continue;
        }
        pRepr = Gia_ManObj( pNew, iRepr );
        if ( !~pRepr->Value ) // first time
        {
            pRepr->Value = i;
            continue;
        }
        // add equivalence
        Gia_ObjSetRepr( p, i, pRepr->Value );
    }
    p->pNexts = Gia_ManDeriveNexts( p );
    return pNew;
}
Gia_Man_t * Gia_ManOrigIdsReduceTest( Gia_Man_t * p, Vec_Int_t * vPairs )
{
    Gia_Man_t * pTemp, * pNew = Gia_ManOrigIdsReduce( p, vPairs );
    Gia_ManPrintStats( p, NULL );
    Gia_ManPrintStats( pNew, NULL );
    //Gia_ManStop( pNew );
    // cleanup the resulting one
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Compute equivalence classes of nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManComputeGiaEquivs( Gia_Man_t * pGia, int nConfs, int fVerbose )
{
    Gia_Man_t * pTemp;
    Cec_ParFra_t ParsFra, * pPars = &ParsFra;
    Cec_ManFraSetDefaultParams( pPars );
    pPars->nItersMax = 100;
    pPars->fUseOrigIds = 1;
    pPars->fSatSweeping = 1;
    pPars->nBTLimit = nConfs;
    pPars->fVerbose = fVerbose;
    pTemp = Cec_ManSatSweeping( pGia, pPars, 0 );
    Gia_ManStop( pTemp );
    return Gia_ManOrigIdsReduce( pGia, pGia->vIdsEquiv );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if AIG is not in the required topo order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCheckTopoOrder_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pRepr;
    if ( pObj->Value == 0 )
        return 1;
    pObj->Value = 0;
    assert( Gia_ObjIsAnd(pObj) );
    if ( !Gia_ManCheckTopoOrder_rec( p, Gia_ObjFanin0(pObj) ) )
        return 0;
    if ( !Gia_ManCheckTopoOrder_rec( p, Gia_ObjFanin1(pObj) ) )
        return 0;
    pRepr = p->pReprs ? Gia_ObjReprObj( p, Gia_ObjId(p,pObj) ) : NULL;
    return pRepr == NULL || pRepr->Value == 0;
}

/**Function*************************************************************

  Synopsis    [Returns 0 if AIG is not in the required topo order.]

  Description [AIG should be in such an order that the representative
  is always traversed before the node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCheckTopoOrder( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, RetValue = 1;
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = 0;
    Gia_ManForEachCo( p, pObj, i )
        RetValue &= Gia_ManCheckTopoOrder_rec( p, Gia_ObjFanin0(pObj) );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Given representatives, derives pointers to the next objects.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Gia_ManDeriveNexts( Gia_Man_t * p )
{
    unsigned * pNexts, * pTails;
    int i;
    assert( p->pReprs != NULL );
    assert( p->pNexts == NULL );
    pNexts = ABC_CALLOC( unsigned, Gia_ManObjNum(p) );
    pTails = ABC_ALLOC( unsigned, Gia_ManObjNum(p) );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        pTails[i] = i;
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
    {
        //if ( p->pReprs[i].iRepr == GIA_VOID )
        if ( !p->pReprs[i].iRepr || p->pReprs[i].iRepr == GIA_VOID )
            continue;
        pNexts[ pTails[p->pReprs[i].iRepr] ] = i;
        pTails[p->pReprs[i].iRepr] = i;
    }
    ABC_FREE( pTails );
    return (int *)pNexts;
}

/**Function*************************************************************

  Synopsis    [Given points to the next objects, derives representatives.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDeriveReprs( Gia_Man_t * p )
{
    int i, iObj;
    assert( p->pReprs == NULL );
    assert( p->pNexts != NULL );
    p->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        Gia_ObjSetRepr( p, i, GIA_VOID );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
    {
        if ( p->pNexts[i] == 0 )
            continue;
        if ( p->pReprs[i].iRepr != GIA_VOID )
            continue;
        // next is set, repr is not set
        for ( iObj = p->pNexts[i]; iObj; iObj = p->pNexts[iObj] )
            p->pReprs[iObj].iRepr = i;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManEquivCountLitsAll( Gia_Man_t * p )
{
    int i, nLits = 0;
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        nLits += (Gia_ObjRepr(p, i) != GIA_VOID);
    return nLits;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManEquivCountClasses( Gia_Man_t * p )
{
    int i, Counter = 0;
    if ( p->pReprs == NULL )
        return 0;
    for ( i = 1; i < Gia_ManObjNum(p); i++ )
        Counter += Gia_ObjIsHead(p, i);
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManEquivCheckLits( Gia_Man_t * p, int nLits )
{
    int nLitsReal = Gia_ManEquivCountLitsAll( p );
    if ( nLitsReal != nLits )
        Abc_Print( 1, "Detected a mismatch in counting equivalence classes (%d).\n", nLitsReal - nLits );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintStatsClasses( Gia_Man_t * p )
{
    int i, Counter = 0, Counter0 = 0, CounterX = 0, Proved = 0, nLits;
    for ( i = 1; i < Gia_ManObjNum(p); i++ )
    {
        if ( Gia_ObjIsHead(p, i) )
            Counter++;
        else if ( Gia_ObjIsConst(p, i) )
            Counter0++;
        else if ( Gia_ObjIsNone(p, i) )
            CounterX++;
        if ( Gia_ObjProved(p, i) )
            Proved++;
    }
    CounterX -= Gia_ManCoNum(p);
    nLits = Gia_ManCiNum(p) + Gia_ManAndNum(p) - Counter - CounterX;

//    Abc_Print( 1, "i/o/ff =%5d/%5d/%5d  ", Gia_ManPiNum(p), Gia_ManPoNum(p), Gia_ManRegNum(p) );
//    Abc_Print( 1, "and =%5d  ", Gia_ManAndNum(p) );
//    Abc_Print( 1, "lev =%3d  ", Gia_ManLevelNum(p) );
    Abc_Print( 1, "cst =%3d  cls =%6d  lit =%8d\n", Counter0, Counter, nLits );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManEquivCountLits( Gia_Man_t * p )
{
    int i, Counter = 0, Counter0 = 0, CounterX = 0;
    if ( p->pReprs == NULL || p->pNexts == NULL )
        return 0;
    for ( i = 1; i < Gia_ManObjNum(p); i++ )
    {
        if ( Gia_ObjIsHead(p, i) )
            Counter++;
        else if ( Gia_ObjIsConst(p, i) )
            Counter0++;
        else if ( Gia_ObjIsNone(p, i) )
            CounterX++;
    }
    CounterX -= Gia_ManCoNum(p);
    return Gia_ManCiNum(p) + Gia_ManAndNum(p) - Counter - CounterX;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManEquivCountOne( Gia_Man_t * p, int i )
{
    int Ent, nLits = 1;
    Gia_ClassForEachObj1( p, i, Ent )
    {
        assert( Gia_ObjRepr(p, Ent) == i );
        nLits++;
    }
    return nLits;
}
void Gia_ManEquivPrintOne( Gia_Man_t * p, int i, int Counter )
{
    int Ent;
    Abc_Print( 1, "Class %4d :  Num = %2d  {", Counter, Gia_ManEquivCountOne(p, i) );
    Gia_ClassForEachObj( p, i, Ent )
    {
        Abc_Print( 1," %d", Ent );
        if ( p->pReprs[Ent].fColorA || p->pReprs[Ent].fColorB )
            Abc_Print( 1," <%d%d>", p->pReprs[Ent].fColorA, p->pReprs[Ent].fColorB );
    }
    Abc_Print( 1, " }\n" );
}
void Gia_ManEquivPrintClasses( Gia_Man_t * p, int fVerbose, float Mem )
{
    int i, Counter = 0, Counter0 = 0, CounterX = 0, Proved = 0, nLits;
    for ( i = 1; i < Gia_ManObjNum(p); i++ )
    {
        if ( Gia_ObjIsHead(p, i) )
            Counter++;
        else if ( Gia_ObjIsConst(p, i) )
            Counter0++;
        else if ( Gia_ObjIsNone(p, i) )
            CounterX++;
        if ( Gia_ObjProved(p, i) )
            Proved++;
    }
    CounterX -= Gia_ManCoNum(p);
    nLits = Gia_ManCiNum(p) + Gia_ManAndNum(p) - Counter - CounterX;
//    Abc_Print( 1, "cst =%8d  cls =%7d  lit =%8d  unused =%8d  proof =%6d  mem =%5.2f MB\n",
//        Counter0, Counter, nLits, CounterX, Proved, (Mem == 0.0) ? 8.0*Gia_ManObjNum(p)/(1<<20) : Mem );
    Abc_Print( 1, "cst =%8d  cls =%7d  lit =%8d  unused =%8d  proof =%6d\n",
        Counter0, Counter, nLits, CounterX, Proved );
    assert( Gia_ManEquivCheckLits( p, nLits ) );
    if ( fVerbose )
    {
//        int Ent;
        Abc_Print( 1, "Const0 (%d) = ", Counter0 );
        Gia_ManForEachConst( p, i )
            Abc_Print( 1, "%d ", i );
        Abc_Print( 1, "\n" );
        Counter = 0;
        Gia_ManForEachClass( p, i )
            Gia_ManEquivPrintOne( p, i, ++Counter );
/*
        Gia_ManLevelNum( p );
        Gia_ManForEachClass( p, i )
            if ( i % 100 == 0 )
            {
//                Abc_Print( 1, "%d ", Gia_ManEquivCountOne(p, i) );
                Gia_ClassForEachObj( p, i, Ent )
                {
                    Abc_Print( 1, "%d ", Gia_ObjLevel( p, Gia_ManObj(p, Ent) ) );
                }
                Abc_Print( 1, "\n" );
            }
*/
    }
}


/**Function*************************************************************

  Synopsis    [Map representatives into class members with minimum level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManChoiceMinLevel_rec( Gia_Man_t * p, int iPivot, int fDiveIn, Vec_Int_t * vMap )
{
    int Level0, Level1, LevelMax;
    Gia_Obj_t * pPivot = Gia_ManObj( p, iPivot );
    if ( Gia_ObjIsCi(pPivot) || iPivot == 0 )
        return 0;
    if ( Gia_ObjLevel(p, pPivot) )
        return Gia_ObjLevel(p, pPivot);
    if ( fDiveIn && Gia_ObjIsClass(p, iPivot) )
    {
        int iObj, ObjMin = -1, iRepr = Gia_ObjRepr(p, iPivot), LevMin = ABC_INFINITY;
        Gia_ClassForEachObj( p, iRepr, iObj )
        {
            int LevCur = Gia_ManChoiceMinLevel_rec( p, iObj, 0, vMap );
            if ( LevMin > LevCur )
            {
                LevMin = LevCur;
                ObjMin = iObj;
            }
        }
        assert( LevMin > 0 );
        Vec_IntWriteEntry( vMap, iRepr, ObjMin );
        Gia_ClassForEachObj( p, iRepr, iObj )
            Gia_ObjSetLevelId( p, iObj, LevMin );
        return LevMin;
    }
    assert( Gia_ObjIsAnd(pPivot) );
    Level0 = Gia_ManChoiceMinLevel_rec( p, Gia_ObjFaninId0(pPivot, iPivot), 1, vMap );
    Level1 = Gia_ManChoiceMinLevel_rec( p, Gia_ObjFaninId1(pPivot, iPivot), 1, vMap );
    LevelMax = 1 + Abc_MaxInt(Level0, Level1);
    Gia_ObjSetLevel( p, pPivot, LevelMax );
    return LevelMax;
}
Vec_Int_t * Gia_ManChoiceMinLevel( Gia_Man_t * p )
{
    Vec_Int_t * vMap = Vec_IntStartFull( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj;
    int i, LevelCur, LevelMax = 0;
//    assert( Gia_ManRegNum(p) == 0 );
    Gia_ManCleanLevels( p, Gia_ManObjNum(p) );
    Gia_ManForEachCo( p, pObj, i )
    {
        LevelCur = Gia_ManChoiceMinLevel_rec( p, Gia_ObjFaninId0p(p, pObj), 1, vMap );
        LevelMax = Abc_MaxInt(LevelMax, LevelCur);
    }
    //printf( "Max level %d\n", LevelMax );
    return vMap;
} 

/**Function*************************************************************

  Synopsis    [Returns representative node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Gia_ManEquivRepr( Gia_Man_t * p, Gia_Obj_t * pObj, int fUseAll, int fDualOut )
{
    if ( fUseAll )
    {
        if ( Gia_ObjRepr(p, Gia_ObjId(p,pObj)) == GIA_VOID )
            return NULL;
    }
    else
    {
        if ( !Gia_ObjProved(p, Gia_ObjId(p,pObj)) )
            return NULL;
    }
//    if ( fDualOut && !Gia_ObjDiffColors( p, Gia_ObjId(p, pObj), Gia_ObjRepr(p, Gia_ObjId(p,pObj)) ) )
    if ( fDualOut && !Gia_ObjDiffColors2( p, Gia_ObjId(p, pObj), Gia_ObjRepr(p, Gia_ObjId(p,pObj)) ) )
        return NULL;
    return Gia_ManObj( p, Gia_ObjRepr(p, Gia_ObjId(p,pObj)) );
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivReduce_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj, int fUseAll, int fDualOut )
{
    Gia_Obj_t * pRepr;
    if ( (pRepr = Gia_ManEquivRepr(p, pObj, fUseAll, fDualOut)) )
    {
        Gia_ManEquivReduce_rec( pNew, p, pRepr, fUseAll, fDualOut );
        pObj->Value = Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) );
        return;
    }
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManEquivReduce_rec( pNew, p, Gia_ObjFanin0(pObj), fUseAll, fDualOut );
    Gia_ManEquivReduce_rec( pNew, p, Gia_ObjFanin1(pObj), fUseAll, fDualOut );
    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Reduces AIG using equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManEquivReduce( Gia_Man_t * p, int fUseAll, int fDualOut, int fSkipPhase, int fVerbose )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    if ( !p->pReprs && p->pSibls )
    {
        int * pMap = ABC_FALLOC( int, Gia_ManObjNum(p) );
        p->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
            Gia_ObjSetRepr( p, i, GIA_VOID );
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
            if ( p->pSibls[i] > 0 )
            {
                if ( pMap[p->pSibls[i]] == -1 )
                    pMap[p->pSibls[i]] = p->pSibls[i];
                pMap[i] = pMap[p->pSibls[i]];
            }
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
            if ( p->pSibls[i] > 0 )
                Gia_ObjSetRepr( p, i, pMap[i] );
        //printf( "Created equivalence classes.\n" );
        ABC_FREE( p->pNexts );
        p->pNexts = Gia_ManDeriveNexts( p );
        ABC_FREE( pMap );
    }
    if ( !p->pReprs )
    {
        Abc_Print( 1, "Gia_ManEquivReduce(): Equivalence classes are not available.\n" );
        return NULL;
    }
    if ( fDualOut && (Gia_ManPoNum(p) & 1) )
    {
        Abc_Print( 1, "Gia_ManEquivReduce(): Dual-output miter should have even number of POs.\n" );
        return NULL;
    }
    // check if there are any equivalences defined
    Gia_ManForEachObj( p, pObj, i )
        if ( Gia_ObjReprObj(p, i) != NULL )
            break;
    if ( i == Gia_ManObjNum(p) )
    {
//        Abc_Print( 1, "Gia_ManEquivReduce(): There are no equivalences to reduce.\n" );
//        return NULL;
        return Gia_ManDup( p );
    }
/*
    if ( !Gia_ManCheckTopoOrder( p ) )
    {
        Abc_Print( 1, "Gia_ManEquivReduce(): AIG is not in a correct topological order.\n" );
        return NULL;
    }
*/
    if ( !fSkipPhase )
        Gia_ManSetPhase( p );
    if ( fDualOut )
        Gia_ManEquivSetColors( p, fVerbose );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManEquivReduce_rec( pNew, p, Gia_ObjFanin0(pObj), fUseAll, fDualOut );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivReduce2_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vMap, int fDiveIn )
{
    Gia_Obj_t * pRepr;
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    if ( fDiveIn && (pRepr = Gia_ManEquivRepr(p, pObj, 1, 0)) )
    {
        int iTemp, iRepr = Gia_ObjId(p, pRepr);
        Gia_Obj_t * pRepr2 = Gia_ManObj( p, Vec_IntEntry(vMap, iRepr) );
        Gia_ManEquivReduce2_rec( pNew, p, pRepr2, vMap, 0 );
        Gia_ClassForEachObj( p, iRepr, iTemp )
        {
            Gia_Obj_t * pTemp = Gia_ManObj(p, iTemp);
            pTemp->Value = Abc_LitNotCond( pRepr2->Value, Gia_ObjPhaseReal(pRepr2) ^ Gia_ObjPhaseReal(pTemp) );
        }
        assert( ~pObj->Value );
        assert( ~pRepr->Value );
        assert( ~pRepr2->Value );
        return;
    }
    Gia_ManEquivReduce2_rec( pNew, p, Gia_ObjFanin0(pObj), vMap, 1 );
    Gia_ManEquivReduce2_rec( pNew, p, Gia_ObjFanin1(pObj), vMap, 1 );
    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}
Gia_Man_t * Gia_ManEquivReduce2( Gia_Man_t * p )
{
    Vec_Int_t * vMap;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    if ( !p->pReprs && p->pSibls )
    {
        int * pMap = ABC_FALLOC( int, Gia_ManObjNum(p) );
        p->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
            Gia_ObjSetRepr( p, i, GIA_VOID );
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
            if ( p->pSibls[i] > 0 )
            {
                if ( pMap[p->pSibls[i]] == -1 )
                    pMap[p->pSibls[i]] = p->pSibls[i];
                pMap[i] = pMap[p->pSibls[i]];
            }
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
            if ( p->pSibls[i] > 0 )
                Gia_ObjSetRepr( p, i, pMap[i] );
        //printf( "Created equivalence classes.\n" );
        ABC_FREE( p->pNexts );
        p->pNexts = Gia_ManDeriveNexts( p );
        ABC_FREE( pMap );
    }
    if ( !p->pReprs )
    {
        Abc_Print( 1, "Gia_ManEquivReduce(): Equivalence classes are not available.\n" );
        return NULL;
    }
    // check if there are any equivalences defined
    Gia_ManForEachObj( p, pObj, i )
        if ( Gia_ObjReprObj(p, i) != NULL )
            break;
    if ( i == Gia_ManObjNum(p) )
        return Gia_ManDup( p );
    vMap = Gia_ManChoiceMinLevel( p );
    Gia_ManSetPhase( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManEquivReduce2_rec( pNew, p, Gia_ObjFanin0(pObj), vMap, 1 );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Vec_IntFree( vMap );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Reduces AIG using equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivFixOutputPairs( Gia_Man_t * p )
{
    Gia_Obj_t * pObj0, * pObj1;
    int i;
    assert( (Gia_ManPoNum(p) & 1) == 0 );
    Gia_ManForEachPo( p, pObj0, i )
    {
        pObj1 = Gia_ManPo( p, ++i );
        if ( Gia_ObjChild0(pObj0) != Gia_ObjChild0(pObj1) )
            continue;
        pObj0->iDiff0  = Gia_ObjId(p, pObj0);
        pObj0->fCompl0 = 0;
        pObj1->iDiff0  = Gia_ObjId(p, pObj1);
        pObj1->fCompl0 = 0;
    }
}

/**Function*************************************************************

  Synopsis    [Removes pointers to the unmarked nodes..]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivUpdatePointers( Gia_Man_t * p, Gia_Man_t * pNew )
{
    Gia_Obj_t * pObj, * pObjNew;
    int i;
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( !~pObj->Value )
            continue;
        pObjNew = Gia_ManObj( pNew, Abc_Lit2Var(pObj->Value) );
        if ( pObjNew->fMark0 )
            pObj->Value = ~0;
    }
}

/**Function*************************************************************

  Synopsis    [Removes pointers to the unmarked nodes..]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivDeriveReprs( Gia_Man_t * p, Gia_Man_t * pNew, Gia_Man_t * pFinal )
{
    Vec_Int_t * vClass;
    Gia_Obj_t * pObj, * pObjNew;
    int i, k, iNode, iRepr, iPrev;
    // start representatives
    pFinal->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(pFinal) );
    for ( i = 0; i < Gia_ManObjNum(pFinal); i++ )
        Gia_ObjSetRepr( pFinal, i, GIA_VOID );
    // iterate over constant candidates
    Gia_ManForEachConst( p, i )
    {
        pObj = Gia_ManObj( p, i );
        if ( !~pObj->Value )
            continue;
        pObjNew = Gia_ManObj( pNew, Abc_Lit2Var(pObj->Value) );
        if ( Abc_Lit2Var(pObjNew->Value) == 0 )
            continue;
        Gia_ObjSetRepr( pFinal, Abc_Lit2Var(pObjNew->Value), 0 );
    }
    // iterate over class candidates
    vClass = Vec_IntAlloc( 100 );
    Gia_ManForEachClass( p, i )
    {
        Vec_IntClear( vClass );
        Gia_ClassForEachObj( p, i, k )
        {
            pObj = Gia_ManObj( p, k );
            if ( !~pObj->Value )
                continue;
            pObjNew = Gia_ManObj( pNew, Abc_Lit2Var(pObj->Value) );
            Vec_IntPushUnique( vClass, Abc_Lit2Var(pObjNew->Value) );
        }
        if ( Vec_IntSize( vClass ) < 2 )
            continue;
        Vec_IntSort( vClass, 0 );
        iRepr = iPrev = Vec_IntEntry( vClass, 0 );
        Vec_IntForEachEntryStart( vClass, iNode, k, 1 )
        {
            Gia_ObjSetRepr( pFinal, iNode, iRepr );
            assert( iPrev < iNode );
            iPrev = iNode;
        }
    }
    Vec_IntFree( vClass );
    pFinal->pNexts = Gia_ManDeriveNexts( pFinal );
}

/**Function*************************************************************

  Synopsis    [Removes pointers to the unmarked nodes..]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManEquivRemapDfs( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Vec_Int_t * vClass;
    int i, k, iNode, iRepr, iPrev;
    pNew = Gia_ManDupDfs( p );
    // start representatives
    pNew->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(pNew) );
    for ( i = 0; i < Gia_ManObjNum(pNew); i++ )
        Gia_ObjSetRepr( pNew, i, GIA_VOID );
    // iterate over constant candidates
    Gia_ManForEachConst( p, i )
        Gia_ObjSetRepr( pNew, Abc_Lit2Var(Gia_ManObj(p, i)->Value), 0 );
    // iterate over class candidates
    vClass = Vec_IntAlloc( 100 );
    Gia_ManForEachClass( p, i )
    {
        Vec_IntClear( vClass );
        Gia_ClassForEachObj( p, i, k )
            Vec_IntPushUnique( vClass, Abc_Lit2Var(Gia_ManObj(p, k)->Value) );
        assert( Vec_IntSize( vClass ) > 1 );
        Vec_IntSort( vClass, 0 );
        iRepr = iPrev = Vec_IntEntry( vClass, 0 );
        Vec_IntForEachEntryStart( vClass, iNode, k, 1 )
        {
            Gia_ObjSetRepr( pNew, iNode, iRepr );
            assert( iPrev < iNode );
            iPrev = iNode;
        }
    }
    Vec_IntFree( vClass );
    pNew->pNexts = Gia_ManDeriveNexts( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Reduces AIG while remapping equivalence classes.]

  Description [Drops the pairs of outputs if they are proved equivalent.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManEquivReduceAndRemap( Gia_Man_t * p, int fSeq, int fMiterPairs )
{
    Gia_Man_t * pNew, * pFinal;
    pNew = Gia_ManEquivReduce( p, 0, 0, 0, 0 );
    if ( pNew == NULL )
        return NULL;
    Gia_ManOrigIdsRemap( p, pNew );
    if ( fMiterPairs )
        Gia_ManEquivFixOutputPairs( pNew );
    if ( fSeq )
        Gia_ManSeqMarkUsed( pNew );
    else
        Gia_ManCombMarkUsed( pNew );
    Gia_ManEquivUpdatePointers( p, pNew );
    pFinal = Gia_ManDupMarked( pNew );
    Gia_ManOrigIdsRemap( pNew, pFinal );
    Gia_ManEquivDeriveReprs( p, pNew, pFinal );
    Gia_ManStop( pNew );
    pFinal = Gia_ManEquivRemapDfs( pNew = pFinal );
    Gia_ManOrigIdsRemap( pNew, pFinal );
    Gia_ManStop( pNew );
    return pFinal;
}

/**Function*************************************************************

  Synopsis    [Marks CIs/COs/ANDs unreachable from POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManEquivSetColor_rec( Gia_Man_t * p, Gia_Obj_t * pObj, int fOdds )
{
    if ( Gia_ObjVisitColor( p, Gia_ObjId(p,pObj), fOdds ) )
        return 0;
    if ( Gia_ObjIsRo(p, pObj) )
        return 1 + Gia_ManEquivSetColor_rec( p, Gia_ObjFanin0(Gia_ObjRoToRi(p, pObj)), fOdds );
    assert( Gia_ObjIsAnd(pObj) );
    return 1 + Gia_ManEquivSetColor_rec( p, Gia_ObjFanin0(pObj), fOdds )
             + Gia_ManEquivSetColor_rec( p, Gia_ObjFanin1(pObj), fOdds );
}

/**Function*************************************************************

  Synopsis    [Marks CIs/COs/ANDs unreachable from POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManEquivSetColors( Gia_Man_t * p, int fVerbose )
{
    Gia_Obj_t * pObj;
    int i, nNodes[2], nDiffs[2];
    assert( (Gia_ManPoNum(p) & 1) == 0 );
    Gia_ObjSetColors( p, 0 );
    Gia_ManForEachPi( p, pObj, i )
        Gia_ObjSetColors( p, Gia_ObjId(p,pObj) );
    nNodes[0] = nNodes[1] = Gia_ManPiNum(p);
    Gia_ManForEachPo( p, pObj, i )
        nNodes[i&1] += Gia_ManEquivSetColor_rec( p, Gia_ObjFanin0(pObj), i&1 );
//    Gia_ManForEachObj( p, pObj, i )
//        if ( Gia_ObjIsCi(pObj) || Gia_ObjIsAnd(pObj) )
//            assert( Gia_ObjColors(p, i) );
    nDiffs[0] = Gia_ManCandNum(p) - nNodes[0];
    nDiffs[1] = Gia_ManCandNum(p) - nNodes[1];
    if ( fVerbose )
    {
        Abc_Print( 1, "CI+AND = %7d  A = %7d  B = %7d  Ad = %7d  Bd = %7d  AB = %7d.\n",
            Gia_ManCandNum(p), nNodes[0], nNodes[1], nDiffs[0], nDiffs[1],
            Gia_ManCandNum(p) - nDiffs[0] - nDiffs[1] );
    }
    return (nDiffs[0] + nDiffs[1]) / 2;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSpecBuild( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vXorLits, int fDualOut, int fSpeculate, Vec_Int_t * vTrace, Vec_Int_t * vGuide, Vec_Int_t * vMap )
{
    Gia_Obj_t * pRepr;
    unsigned iLitNew;
    pRepr = Gia_ObjReprObj( p, Gia_ObjId(p,pObj) );
    if ( pRepr == NULL )
        return;
//    if ( fDualOut && !Gia_ObjDiffColors( p, Gia_ObjId(p, pObj), Gia_ObjId(p, pRepr) ) )
    if ( fDualOut && !Gia_ObjDiffColors2( p, Gia_ObjId(p, pObj), Gia_ObjId(p, pRepr) ) )
        return;
    iLitNew = Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) );
    if ( pObj->Value != iLitNew && !Gia_ObjProved(p, Gia_ObjId(p,pObj)) )
    {
        if ( vTrace )
            Vec_IntPush( vTrace, 1 );
        if ( vGuide == NULL || Vec_IntEntry( vGuide, Vec_IntSize(vTrace)-1 ) )
        {
            if ( vMap )  
                Vec_IntPush( vMap, Gia_ObjId(p, pObj) );
            Vec_IntPush( vXorLits, Gia_ManHashXor(pNew, pObj->Value, iLitNew) );
        }
    }
    else
    {
        if ( vTrace )
            Vec_IntPush( vTrace, 0 );
    }
    if ( fSpeculate )
        pObj->Value = iLitNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSpecReduce_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vXorLits, int fDualOut, int fSpeculate, Vec_Int_t * vTrace, Vec_Int_t * vGuide, Vec_Int_t * vMap )
{
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManSpecReduce_rec( pNew, p, Gia_ObjFanin0(pObj), vXorLits, fDualOut, fSpeculate, vTrace, vGuide, vMap );
    Gia_ManSpecReduce_rec( pNew, p, Gia_ObjFanin1(pObj), vXorLits, fDualOut, fSpeculate, vTrace, vGuide, vMap );
    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManSpecBuild( pNew, p, pObj, vXorLits, fDualOut, fSpeculate, vTrace, vGuide, vMap );
}

/**Function*************************************************************

  Synopsis    [Reduces AIG using equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManSpecReduceTrace( Gia_Man_t * p, Vec_Int_t * vTrace, Vec_Int_t * vMap )
{
    Vec_Int_t * vXorLits;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, iLitNew;
    if ( !p->pReprs )
    {
        Abc_Print( 1, "Gia_ManSpecReduce(): Equivalence classes are not available.\n" );
        return NULL;
    }
    Vec_IntClear( vTrace );
    vXorLits = Vec_IntAlloc( 1000 );
    Gia_ManSetPhase( p );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachRo( p, pObj, i )
        Gia_ManSpecBuild( pNew, p, pObj, vXorLits, 0, 1, vTrace, NULL, vMap );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManSpecReduce_rec( pNew, p, Gia_ObjFanin0(pObj), vXorLits, 0, 1, vTrace, NULL, vMap );
    Gia_ManForEachPo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Vec_IntForEachEntry( vXorLits, iLitNew, i )
        Gia_ManAppendCo( pNew, iLitNew );
    if ( Vec_IntSize(vXorLits) == 0 )
    {
        Abc_Print( 1, "Speculatively reduced model has no primary outputs.\n" );
        Gia_ManAppendCo( pNew, 0 );
    }
    Gia_ManForEachRi( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Vec_IntFree( vXorLits );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Reduces AIG using equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManSpecReduce( Gia_Man_t * p, int fDualOut, int fSynthesis, int fSpeculate, int fSkipSome, int fVerbose )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    Vec_Int_t * vXorLits;
    int i, iLitNew;
    Vec_Int_t * vTrace = NULL, * vGuide = NULL;
    if ( !p->pReprs )
    {
        Abc_Print( 1, "Gia_ManSpecReduce(): Equivalence classes are not available.\n" );
        return NULL;
    }
    if ( fDualOut && (Gia_ManPoNum(p) & 1) )
    {
        Abc_Print( 1, "Gia_ManSpecReduce(): Dual-output miter should have even number of POs.\n" );
        return NULL;
    }
    if ( fSkipSome )
    {
        vGuide = Vec_IntAlloc( 100 );
        pTemp  = Gia_ManSpecReduceTrace( p, vGuide, NULL );
        Gia_ManStop( pTemp );
        assert( Vec_IntSize(vGuide) == Gia_ManEquivCountLitsAll(p) );
        vTrace = Vec_IntAlloc( 100 );
    }
    vXorLits = Vec_IntAlloc( 1000 );
    Gia_ManSetPhase( p );
    Gia_ManFillValue( p );
    if ( fDualOut )
        Gia_ManEquivSetColors( p, fVerbose );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachRo( p, pObj, i )
        Gia_ManSpecBuild( pNew, p, pObj, vXorLits, fDualOut, fSpeculate, vTrace, vGuide, NULL );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManSpecReduce_rec( pNew, p, Gia_ObjFanin0(pObj), vXorLits, fDualOut, fSpeculate, vTrace, vGuide, NULL );
    if ( !fSynthesis )
    {
        Gia_ManForEachPo( p, pObj, i )
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Vec_IntForEachEntry( vXorLits, iLitNew, i )
        Gia_ManAppendCo( pNew, iLitNew );
    if ( Vec_IntSize(vXorLits) == 0 )
    {
        Abc_Print( 1, "Speculatively reduced model has no primary outputs.\n" );
        Gia_ManAppendCo( pNew, 0 );
    }
    Gia_ManForEachRi( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Vec_IntFree( vXorLits );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );

    // update using trace
    if ( fSkipSome )
    {
        // count the number of non-zero entries
        int iLit, nAddPos = 0;
        Vec_IntForEachEntry( vGuide, iLit, i )
            if ( iLit )
                nAddPos++;
        if ( nAddPos )
            assert( Gia_ManPoNum(pNew) == Gia_ManPoNum(p) + nAddPos );
    }
    Vec_IntFreeP( &vTrace );
    Vec_IntFreeP( &vGuide );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSpecBuildInit( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vXorLits, int f, int fDualOut )
{
    Gia_Obj_t * pRepr;
    int iLitNew;
    pRepr = Gia_ObjReprObj( p, Gia_ObjId(p,pObj) );
    if ( pRepr == NULL )
        return;
//    if ( fDualOut && !Gia_ObjDiffColors( p, Gia_ObjId(p, pObj), Gia_ObjId(p, pRepr) ) )
    if ( fDualOut && !Gia_ObjDiffColors2( p, Gia_ObjId(p, pObj), Gia_ObjId(p, pRepr) ) )
        return;
    iLitNew = Abc_LitNotCond( Gia_ObjCopyF(p, f, pRepr), Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) );
    if ( Gia_ObjCopyF(p, f, pObj) != iLitNew && !Gia_ObjProved(p, Gia_ObjId(p,pObj)) )
        Vec_IntPush( vXorLits, Gia_ManHashXor(pNew, Gia_ObjCopyF(p, f, pObj), iLitNew) );
    Gia_ObjSetCopyF( p, f, pObj, iLitNew );
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSpecReduceInit_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vXorLits, int f, int fDualOut )
{
    if ( ~Gia_ObjCopyF(p, f, pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManSpecReduceInit_rec( pNew, p, Gia_ObjFanin0(pObj), vXorLits, f, fDualOut );
    Gia_ManSpecReduceInit_rec( pNew, p, Gia_ObjFanin1(pObj), vXorLits, f, fDualOut );
    Gia_ObjSetCopyF( p, f, pObj, Gia_ManHashAnd(pNew, Gia_ObjFanin0CopyF(p, f, pObj), Gia_ObjFanin1CopyF(p, f, pObj)) );
    Gia_ManSpecBuildInit( pNew, p, pObj, vXorLits, f, fDualOut );
}

/**Function*************************************************************

  Synopsis    [Creates initialized SRM with the given number of frames.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManSpecReduceInit( Gia_Man_t * p, Abc_Cex_t * pInit, int nFrames, int fDualOut )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    Vec_Int_t * vXorLits;
    int f, i, iLitNew;
    if ( !p->pReprs )
    {
        Abc_Print( 1, "Gia_ManSpecReduceInit(): Equivalence classes are not available.\n" );
        return NULL;
    }
    if ( Gia_ManRegNum(p) == 0 )
    {
        Abc_Print( 1, "Gia_ManSpecReduceInit(): Circuit is not sequential.\n" );
        return NULL;
    }
    if ( Gia_ManRegNum(p) != pInit->nRegs )
    {
        Abc_Print( 1, "Gia_ManSpecReduceInit(): Mismatch in the number of registers.\n" );
        return NULL;
    }
    if ( fDualOut && (Gia_ManPoNum(p) & 1) )
    {
        Abc_Print( 1, "Gia_ManSpecReduceInit(): Dual-output miter should have even number of POs.\n" );
        return NULL;
    }

/*
    if ( !Gia_ManCheckTopoOrder( p ) )
    {
        Abc_Print( 1, "Gia_ManSpecReduceInit(): AIG is not in a correct topological order.\n" );
        return NULL;
    }
*/
    assert( pInit->nRegs == Gia_ManRegNum(p) && pInit->nPis == 0 );
    Vec_IntFill( &p->vCopies, nFrames * Gia_ManObjNum(p), -1 );
    vXorLits = Vec_IntAlloc( 1000 );
    Gia_ManSetPhase( p );
    if ( fDualOut )
        Gia_ManEquivSetColors( p, 0 );
    pNew = Gia_ManStart( nFrames * Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachRo( p, pObj, i )
        Gia_ObjSetCopyF( p, 0, pObj, Abc_InfoHasBit(pInit->pData, i) );
    for ( f = 0; f < nFrames; f++ )
    {
        Gia_ObjSetCopyF( p, f, Gia_ManConst0(p), 0 );
        Gia_ManForEachPi( p, pObj, i )
            Gia_ObjSetCopyF( p, f, pObj, Gia_ManAppendCi(pNew) );
        Gia_ManForEachRo( p, pObj, i )
            Gia_ManSpecBuildInit( pNew, p, pObj, vXorLits, f, fDualOut );
        Gia_ManForEachCo( p, pObj, i )
        {
            Gia_ManSpecReduceInit_rec( pNew, p, Gia_ObjFanin0(pObj), vXorLits, f, fDualOut );
            Gia_ObjSetCopyF( p, f, pObj, Gia_ObjFanin0CopyF(p, f, pObj) );
        }
        if ( f == nFrames - 1 )
            break;
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, i )
            Gia_ObjSetCopyF( p, f+1, pObjRo, Gia_ObjCopyF(p, f, pObjRi) );
    }
    Vec_IntForEachEntry( vXorLits, iLitNew, i )
        Gia_ManAppendCo( pNew, iLitNew );
    if ( Vec_IntSize(vXorLits) == 0 )
    {
//        Abc_Print( 1, "Speculatively reduced model has no primary outputs.\n" );
        Gia_ManAppendCo( pNew, 0 );
    }
    Vec_IntErase( &p->vCopies );
    Vec_IntFree( vXorLits );
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Creates initialized SRM with the given number of frames.]

  Description [Uses as many frames as needed to create the number of 
  output not less than the number of equivalence literals.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManSpecReduceInitFrames( Gia_Man_t * p, Abc_Cex_t * pInit, int nFramesMax, int * pnFrames, int fDualOut, int nMinOutputs )
{
    Gia_Man_t * pFrames;
    int f, nLits;
    nLits = Gia_ManEquivCountLits( p );
    for ( f = 1; ; f++ )
    {
        pFrames = Gia_ManSpecReduceInit( p, pInit, f, fDualOut );
        if ( (nMinOutputs == 0 && Gia_ManPoNum(pFrames) >= nLits/2+1) ||
             (nMinOutputs != 0 && Gia_ManPoNum(pFrames) >= nMinOutputs) )
            break;
        if ( f == nFramesMax )
            break;
        if ( Gia_ManAndNum(pFrames) > 500000 )
        {
            Gia_ManStop( pFrames );
            return NULL;
        }
        Gia_ManStop( pFrames );
        pFrames = NULL;
    }
    if ( f == nFramesMax )
        Abc_Print( 1, "Stopped unrolling after %d frames.\n", nFramesMax );
    if ( pnFrames )
        *pnFrames = f;
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Transforms equiv classes by removing the AB nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivTransform( Gia_Man_t * p, int fVerbose )
{
    extern void Cec_ManSimClassCreate( Gia_Man_t * p, Vec_Int_t * vClass );
    Vec_Int_t * vClass, * vClassNew;
    int iRepr, iNode, Ent, k;
    int nRemovedLits = 0, nRemovedClas = 0;
    int nTotalLits = 0, nTotalClas = 0;
    Gia_Obj_t * pObj;
    int i;
    assert( p->pReprs && p->pNexts );
    vClass = Vec_IntAlloc( 100 );
    vClassNew = Vec_IntAlloc( 100 );
    Gia_ManForEachObj( p, pObj, i )
        if ( Gia_ObjIsCi(pObj) || Gia_ObjIsAnd(pObj) )
            assert( Gia_ObjColors(p, i) );
    Gia_ManForEachClassReverse( p, iRepr )
    {
        nTotalClas++;
        Vec_IntClear( vClass );
        Vec_IntClear( vClassNew );
        Gia_ClassForEachObj( p, iRepr, iNode )
        {
            nTotalLits++;
            Vec_IntPush( vClass, iNode );
            assert( Gia_ObjColors(p, iNode) );
            if ( Gia_ObjColors(p, iNode) != 3 )
                Vec_IntPush( vClassNew, iNode );
            else
                nRemovedLits++;
        }
        Vec_IntForEachEntry( vClass, Ent, k )
        {
            p->pReprs[Ent].fFailed = p->pReprs[Ent].fProved = 0;
            p->pReprs[Ent].iRepr = GIA_VOID;
            p->pNexts[Ent] = 0;
        }
        if ( Vec_IntSize(vClassNew) < 2 )
        {
            nRemovedClas++;
            continue;
        }
        Cec_ManSimClassCreate( p, vClassNew );
    }
    Vec_IntFree( vClass );
    Vec_IntFree( vClassNew );
    if ( fVerbose )
    Abc_Print( 1, "Removed classes = %6d (out of %6d). Removed literals = %6d (out of %6d).\n",
        nRemovedClas, nTotalClas, nRemovedLits, nTotalLits );
}

/**Function*************************************************************

  Synopsis    [Marks proved equivalences.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivMark( Gia_Man_t * p, char * pFileName, int fSkipSome, int fVerbose )
{
    Gia_Man_t * pMiter, * pTemp;
    Gia_Obj_t * pObj;
    int i, iLit, nAddPos, nLits = 0;
    int nLitsAll, Counter = 0;
    nLitsAll = Gia_ManEquivCountLitsAll( p );
    if ( nLitsAll == 0 )
    {
        Abc_Print( 1, "Gia_ManEquivMark(): Current AIG does not have equivalences.\n" );
        return;
    }
    // read AIGER file
    pMiter = Gia_AigerRead( pFileName, 0, 0, 0 );
    if ( pMiter == NULL )
    {
        Abc_Print( 1, "Gia_ManEquivMark(): Input file %s could not be read.\n", pFileName );
        return;
    }
    if ( fSkipSome )
    {
        Vec_Int_t * vTrace = Vec_IntAlloc( 100 );
        pTemp = Gia_ManSpecReduceTrace( p, vTrace, NULL );
        Gia_ManStop( pTemp );
        assert( Vec_IntSize(vTrace) == nLitsAll );
        // count the number of non-zero entries
        nAddPos = 0;
        Vec_IntForEachEntry( vTrace, iLit, i )
            if ( iLit )
                nAddPos++;
        // check the number
        if ( Gia_ManPoNum(pMiter) != Gia_ManPoNum(p) + nAddPos )
        {
            Abc_Print( 1, "Gia_ManEquivMark(): The number of POs is not correct: MiterPONum(%d) != AIGPONum(%d) + AIGFilteredEquivNum(%d).\n",
                Gia_ManPoNum(pMiter), Gia_ManPoNum(p), nAddPos );
            Gia_ManStop( pMiter );
            Vec_IntFreeP( &vTrace );
            return;
        }
        // mark corresponding POs as solved
        nLits = iLit = Counter = 0;
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
        {
            if ( Gia_ObjRepr(p, i) == GIA_VOID )
                continue;
            if ( Vec_IntEntry( vTrace, nLits++ ) == 0 )
                continue;
            pObj = Gia_ManPo( pMiter, Gia_ManPoNum(p) + iLit++ );
            if ( Gia_ObjFaninLit0p(pMiter, pObj) == 0 ) // const 0 - proven
            {
                Gia_ObjSetProved(p, i);
                Counter++;
            }
        }
        assert( nLits == nLitsAll );
        assert( iLit == nAddPos );
        Vec_IntFreeP( &vTrace );
    }
    else
    {
        if ( Gia_ManPoNum(pMiter) != Gia_ManPoNum(p) + nLitsAll )
        {
            Abc_Print( 1, "Gia_ManEquivMark(): The number of POs is not correct: MiterPONum(%d) != AIGPONum(%d) + AIGEquivNum(%d).\n",
                Gia_ManPoNum(pMiter), Gia_ManPoNum(p), nLitsAll );
            Gia_ManStop( pMiter );
            return;
        }
        // mark corresponding POs as solved
        nLits = 0;
        for ( i = 0; i < Gia_ManObjNum(p); i++ )
        {
            if ( Gia_ObjRepr(p, i) == GIA_VOID )
                continue;
            pObj = Gia_ManPo( pMiter, Gia_ManPoNum(p) + nLits++ );
            if ( Gia_ObjFaninLit0p(pMiter, pObj) == 0 ) // const 0 - proven
            {
                Gia_ObjSetProved(p, i);
                Counter++;
            }
        }
        assert( nLits == nLitsAll );
    }
    if ( fVerbose )
        Abc_Print( 1, "Set %d equivalences as proved.\n", Counter );
    Gia_ManStop( pMiter );
}

/**Function*************************************************************

  Synopsis    [Transforms equiv classes by filtering those that correspond to disproved outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivFilter( Gia_Man_t * p, Vec_Int_t * vPoIds, int fVerbose )
{
    Gia_Man_t * pSrm;
    Vec_Int_t * vTrace, * vMap;
    int i, iObjId, Entry, Prev = -1;
    // check if there are equivalences
    if ( p->pReprs == NULL || p->pNexts == NULL )
    {
        Abc_Print( 1, "Gia_ManEquivFilter(): Equivalence classes are not defined.\n" );
        return;
    }
    // check if PO indexes are available
    if ( vPoIds == NULL )
    {
        Abc_Print( 1, "Gia_ManEquivFilter(): Array of disproved POs is not available.\n" );
        return;
    }
    if ( Vec_IntSize(vPoIds) == 0 )
        return;
    // create SRM with mapping into POs
    vMap = Vec_IntAlloc( 1000 );
    vTrace = Vec_IntAlloc( 1000 );
    pSrm = Gia_ManSpecReduceTrace( p, vTrace, vMap );
    Vec_IntFree( vTrace );
    // the resulting array (vMap) maps PO indexes of the SRM into object IDs
    assert( Gia_ManPoNum(pSrm) == Gia_ManPoNum(p) + Vec_IntSize(vMap) );
    Gia_ManStop( pSrm );
    if ( fVerbose )
        printf( "Design POs = %d. SRM POs = %d. Spec POs = %d. Disproved POs = %d.\n", 
            Gia_ManPoNum(p), Gia_ManPoNum(p) + Vec_IntSize(vMap), Vec_IntSize(vMap), Vec_IntSize(vPoIds) );
    // check if disproved POs satisfy the range
    Vec_IntSort( vPoIds, 0 );
    Vec_IntForEachEntry( vPoIds, Entry, i )
    {
        if ( Entry < 0 || Entry >= Gia_ManPoNum(p) + Vec_IntSize(vMap) )
        {
            Abc_Print( 1, "Gia_ManEquivFilter(): Array of disproved POs contains PO index (%d),\n", Entry );
            Abc_Print( 1, "which does not fit into the range of available PO indexes of the SRM: [%d; %d].\n", 0, Gia_ManPoNum(p) + Vec_IntSize(vMap)-1 );
            Vec_IntFree( vMap );
            return;
        }
        if ( Entry < Gia_ManPoNum(p) )
            Abc_Print( 0, "Gia_ManEquivFilter(): One of the original POs (%d) have failed.\n", Entry );
        if ( Prev == Entry )
        {
            Abc_Print( 1, "Gia_ManEquivFilter(): Array of disproved POs contains at least one duplicate entry (%d),\n", Entry );
            Vec_IntFree( vMap );
            return;
        }
        Prev = Entry;
    }
    // perform the reduction of the equivalence classes
    Vec_IntForEachEntry( vPoIds, Entry, i )
    {
        if ( Entry < Gia_ManPoNum(p) )
            continue;
        iObjId = Vec_IntEntry( vMap, Entry - Gia_ManPoNum(p) );
        Gia_ObjUnsetRepr( p, iObjId );
//        Gia_ObjSetNext( p, iObjId, 0 );
    }
    Vec_IntFree( vMap );
    ABC_FREE( p->pNexts );
    p->pNexts = Gia_ManDeriveNexts( p );
}

/**Function*************************************************************

  Synopsis    [Transforms equiv classes by filtering those that correspond to disproved outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivFilterTest( Gia_Man_t * p )
{
    Vec_Int_t * vPoIds;
    int i;
    vPoIds = Vec_IntAlloc( 1000 );
    for ( i = 0; i < 10; i++ )
    {
        Vec_IntPush( vPoIds, Gia_ManPoNum(p) + 2 * i + 2 );
        printf( "%d ", Gia_ManPoNum(p) + 2*i + 2 );
    }
    printf( "\n" );
    Gia_ManEquivFilter( p, vPoIds, 1 );
    Vec_IntFree( vPoIds );
}

/**Function*************************************************************

  Synopsis    [Transforms equiv classes by setting a good representative.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivImprove( Gia_Man_t * p )
{
    Vec_Int_t * vClass;
    int i, k, iNode, iRepr;
    int iReprBest, iLevelBest, iLevelCur, iMffcBest, iMffcCur;
    assert( p->pReprs != NULL && p->pNexts != NULL );
    Gia_ManLevelNum( p );
    Gia_ManCreateRefs( p );
    // iterate over class candidates
    vClass = Vec_IntAlloc( 100 );
    Gia_ManForEachClass( p, i )
    {
        Vec_IntClear( vClass );
        iReprBest = -1;
        iLevelBest = iMffcBest = ABC_INFINITY;
        Gia_ClassForEachObj( p, i, k )
        {
            iLevelCur = Gia_ObjLevel( p,Gia_ManObj(p, k) );
            iMffcCur  = Gia_NodeMffcSize( p, Gia_ManObj(p, k) );
            if ( iLevelBest > iLevelCur || (iLevelBest == iLevelCur && iMffcBest > iMffcCur) )
            {
                iReprBest  = k;
                iLevelBest = iLevelCur;
                iMffcBest  = iMffcCur;
            }
            Vec_IntPush( vClass, k );
        }
        assert( Vec_IntSize( vClass ) > 1 );
        assert( iReprBest > 0 );
        if ( i == iReprBest )
            continue;
/*
        Abc_Print( 1, "Repr/Best = %6d/%6d. Lev = %3d/%3d. Mffc = %3d/%3d.\n", 
            i, iReprBest, Gia_ObjLevel( p,Gia_ManObj(p, i) ), Gia_ObjLevel( p,Gia_ManObj(p, iReprBest) ),
            Gia_NodeMffcSize( p, Gia_ManObj(p, i) ), Gia_NodeMffcSize( p, Gia_ManObj(p, iReprBest) ) );
*/
        iRepr = iReprBest;
        Gia_ObjSetRepr( p, iRepr, GIA_VOID );
        Gia_ObjSetProved( p, i );
        Gia_ObjUnsetProved( p, iRepr );
        Vec_IntForEachEntry( vClass, iNode, k )
            if ( iNode != iRepr )
                Gia_ObjSetRepr( p, iNode, iRepr );
    }
    Vec_IntFree( vClass );
    ABC_FREE( p->pNexts );
//    p->pNexts = Gia_ManDeriveNexts( p );
}


/**Function*************************************************************

  Synopsis    [Returns 1 if pOld is in the TFI of pNode.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjCheckTfi_rec( Gia_Man_t * p, Gia_Obj_t * pOld, Gia_Obj_t * pNode, Vec_Ptr_t * vVisited )
{
    // check the trivial cases
    if ( pNode == NULL )
        return 0;
    if ( Gia_ObjIsCi(pNode) )
        return 0;
//    if ( pNode->Id < pOld->Id ) // cannot use because of choices of pNode
//        return 0;
    if ( pNode == pOld )
        return 1;
    // skip the visited node
    if ( pNode->fMark0 )
        return 0;
    pNode->fMark0 = 1;
    Vec_PtrPush( vVisited, pNode );
    // check the children
    if ( Gia_ObjCheckTfi_rec( p, pOld, Gia_ObjFanin0(pNode), vVisited ) )
        return 1;
    if ( Gia_ObjCheckTfi_rec( p, pOld, Gia_ObjFanin1(pNode), vVisited ) )
        return 1;
    // check equivalent nodes
    return Gia_ObjCheckTfi_rec( p, pOld, Gia_ObjNextObj(p, Gia_ObjId(p, pNode)), vVisited );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pOld is in the TFI of pNode.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjCheckTfi( Gia_Man_t * p, Gia_Obj_t * pOld, Gia_Obj_t * pNode )
{
    Vec_Ptr_t * vVisited;
    Gia_Obj_t * pObj;
    int RetValue, i;
    assert( !Gia_IsComplement(pOld) );
    assert( !Gia_IsComplement(pNode) );
    vVisited = Vec_PtrAlloc( 100 );
    RetValue = Gia_ObjCheckTfi_rec( p, pOld, pNode, vVisited );
    Vec_PtrForEachEntry( Gia_Obj_t *, vVisited, pObj, i )
        pObj->fMark0 = 0;
    Vec_PtrFree( vVisited );
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Adds the next entry while making choices.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManAddNextEntry_rec( Gia_Man_t * p, Gia_Obj_t * pOld, Gia_Obj_t * pNode )
{
    if ( Gia_ObjNext(p, Gia_ObjId(p, pOld)) == 0 )
    {
        Gia_ObjSetNext( p, Gia_ObjId(p, pOld), Gia_ObjId(p, pNode) );
        return;
    }
    Gia_ManAddNextEntry_rec( p, Gia_ObjNextObj(p, Gia_ObjId(p, pOld)), pNode );
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG in the DFS order.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManEquivToChoices_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pRepr, * pReprNew, * pObjNew;
    if ( ~pObj->Value )
        return;
    if ( (pRepr = Gia_ObjReprObj(p, Gia_ObjId(p, pObj))) && !Gia_ObjFailed(p,Gia_ObjId(p,pObj))  )
    {
        if ( Gia_ObjIsConst0(pRepr) )
        {
            pObj->Value = Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) );
            return;
        }
        Gia_ManEquivToChoices_rec( pNew, p, pRepr );
        assert( Gia_ObjIsAnd(pObj) );
        Gia_ManEquivToChoices_rec( pNew, p, Gia_ObjFanin0(pObj) );
        Gia_ManEquivToChoices_rec( pNew, p, Gia_ObjFanin1(pObj) );
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( Abc_LitRegular(pObj->Value) == Abc_LitRegular(pRepr->Value) )
        {
            assert( (int)pObj->Value == Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) ) );
            return;
        }
        if ( pRepr->Value > pObj->Value ) // should never happen with high resource limit
            return;
        assert( pRepr->Value < pObj->Value );
        pReprNew = Gia_ManObj( pNew, Abc_Lit2Var(pRepr->Value) );
        pObjNew  = Gia_ManObj( pNew, Abc_Lit2Var(pObj->Value) );
        if ( Gia_ObjReprObj( pNew, Gia_ObjId(pNew, pObjNew) ) )
        {
//            assert( Gia_ObjReprObj( pNew, Gia_ObjId(pNew, pObjNew) ) == pReprNew );
            if ( Gia_ObjReprObj( pNew, Gia_ObjId(pNew, pObjNew) ) != pReprNew )
                return;
            pObj->Value = Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) );
            return;
        }
        if ( !Gia_ObjCheckTfi( pNew, pReprNew, pObjNew ) )
        {
            assert( Gia_ObjNext(pNew, Gia_ObjId(pNew, pObjNew)) == 0 );
            Gia_ObjSetRepr( pNew, Gia_ObjId(pNew, pObjNew), Gia_ObjId(pNew, pReprNew) );
            Gia_ManAddNextEntry_rec( pNew, pReprNew, pObjNew );
        }
        pObj->Value = Abc_LitNotCond( pRepr->Value, Gia_ObjPhaseReal(pRepr) ^ Gia_ObjPhaseReal(pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManEquivToChoices_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManEquivToChoices_rec( pNew, p, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Removes choices, which contain fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManRemoveBadChoices( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, iObj, iPrev, Counter = 0;
    // mark nodes with fanout
    Gia_ManForEachObj( p, pObj, i )
    {
        pObj->fMark0 = 0;
        if ( Gia_ObjIsAnd(pObj) )
        {
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( Gia_ObjIsCo(pObj) )
            Gia_ObjFanin0(pObj)->fMark0 = 1;
    }
    // go through the classes and remove 
    Gia_ManForEachClass( p, i )
    {
        for ( iPrev = i, iObj = Gia_ObjNext(p, i); iObj; iObj = Gia_ObjNext(p, iPrev) )
        {
            if ( !Gia_ManObj(p, iObj)->fMark0 )
            {
                iPrev = iObj;
                continue;
            }
            Gia_ObjSetRepr( p, iObj, GIA_VOID );
            Gia_ObjSetNext( p, iPrev, Gia_ObjNext(p, iObj) );
            Gia_ObjSetNext( p, iObj, 0 );
            Counter++;
        }
    }
    // remove the marks
    Gia_ManCleanMark0( p );
//    Abc_Print( 1, "Removed %d bad choices.\n", Counter );
}

/**Function*************************************************************

  Synopsis    [Reduces AIG using equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManEquivToChoices( Gia_Man_t * p, int nSnapshots )
{
    Vec_Int_t * vNodes;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pRepr;
    int i;
//Gia_ManEquivPrintClasses( p, 0, 0 );
    assert( (Gia_ManCoNum(p) % nSnapshots) == 0 );
    Gia_ManSetPhase( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
    pNew->pNexts = ABC_CALLOC( int, Gia_ManObjNum(p) );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        pNew->pReprs[i].iRepr = GIA_VOID;
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachRo( p, pObj, i )
        if ( (pRepr = Gia_ObjReprObj(p, Gia_ObjId(p, pObj))) )
        {
            assert( Gia_ObjIsConst0(pRepr) || Gia_ObjIsRo(p, pRepr) );
            pObj->Value = pRepr->Value;
        }
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManEquivToChoices_rec( pNew, p, Gia_ObjFanin0(pObj) );
    vNodes = Gia_ManGetDangling( p );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        Gia_ManEquivToChoices_rec( pNew, p, pObj );
    Vec_IntFree( vNodes );
    Gia_ManForEachCo( p, pObj, i )
        if ( i % nSnapshots == 0 )
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Gia_ManRemoveBadChoices( pNew );
//Gia_ManEquivPrintClasses( pNew, 0, 0 );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
//Gia_ManEquivPrintClasses( pNew, 0, 0 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Counts the number of choice nodes]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCountChoiceNodes( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    if ( p->pReprs == NULL || p->pNexts == NULL )
        return 0;
    Gia_ManForEachObj( p, pObj, i )
        Counter += Gia_ObjIsHead( p, i );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of choices]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCountChoices( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    if ( p->pReprs == NULL || p->pNexts == NULL )
        return 0;
    Gia_ManForEachObj( p, pObj, i )
        Counter += (int)(Gia_ObjNext( p, i ) > 0);
    return Counter;
}

ABC_NAMESPACE_IMPL_END

#include "aig/aig/aig.h"
#include "aig/saig/saig.h"
#include "proof/cec/cec.h"
#include "giaAig.h"

ABC_NAMESPACE_IMPL_START


/**Function*************************************************************

  Synopsis    [Returns 1 if AIG has dangling nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHasNoEquivs( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    if ( p->pReprs == NULL )
        return 1;
    Gia_ManForEachObj( p, pObj, i )
        if ( Gia_ObjReprObj(p, i) != NULL )
            break;
    return i == Gia_ManObjNum(p);
}

/**Function*************************************************************

  Synopsis    [Implements iteration during speculation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_CommandSpecI( Gia_Man_t * pGia, int nFramesInit, int nBTLimitInit, int fStart, int fCheckMiter, int fVerbose )
{
//    extern int Cec_ManCheckNonTrivialCands( Gia_Man_t * pAig );
    Aig_Man_t * pTemp;
    Gia_Man_t * pSrm, * pReduce, * pAux;
    int nIter, nStart = 0;
    if ( pGia->pReprs == NULL || pGia->pNexts == NULL )
    {
        Abc_Print( 1, "Gia_CommandSpecI(): Equivalence classes are not defined.\n" );
        return 0;
    }
    // (spech)*  where spech = &srm; restore save3; bmc2 -F 100 -C 25000; &resim
    Gia_ManCleanMark0( pGia );
    Gia_ManPrintStats( pGia, NULL );
    for ( nIter = 0; ; nIter++ )
    {
        if ( Gia_ManHasNoEquivs(pGia) )
        {
            Abc_Print( 1, "Gia_CommandSpecI: No equivalences left.\n" );
            break;
        }
        Abc_Print( 1, "ITER %3d : ", nIter );
//      if ( fVerbose )
//            Abc_Print( 1, "Starting BMC from frame %d.\n", nStart );
//      if ( fVerbose )
//            Gia_ManPrintStats( pGia, 0 );
            Gia_ManPrintStatsClasses( pGia );
        // perform speculative reduction
//        if ( Gia_ManPoNum(pSrm) <= Gia_ManPoNum(pGia) )
        if ( !Cec_ManCheckNonTrivialCands(pGia) )
        {
            Abc_Print( 1, "Gia_CommandSpecI: There are only trivial equiv candidates left (PO drivers). Quitting.\n" );
            break;
        }
        pSrm = Gia_ManSpecReduce( pGia, 0, 0, 1, 0, 0 );
        // bmc2 -F 100 -C 25000
        {
            Abc_Cex_t * pCex;
            int nFrames     = nFramesInit; // different from default
            int nNodeDelta  = 2000;
            int nBTLimit    = nBTLimitInit; // different from default
            int nBTLimitAll = 2000000;
            pTemp = Gia_ManToAig( pSrm, 0 );
//            Aig_ManPrintStats( pTemp );
            Gia_ManStop( pSrm );
            Saig_BmcPerform( pTemp, nStart, nFrames, nNodeDelta, 0, nBTLimit, nBTLimitAll, fVerbose, 0, NULL, 0, 0 );
            pCex = pTemp->pSeqModel; pTemp->pSeqModel = NULL;
            Aig_ManStop( pTemp );
            if ( pCex == NULL )
            {
                Abc_Print( 1, "Gia_CommandSpecI(): Internal BMC could not find a counter-example.\n" );
                break;
            }
            if ( fStart )
                nStart = pCex->iFrame;
            // perform simulation
            {
                Cec_ParSim_t Pars, * pPars = &Pars;
                Cec_ManSimSetDefaultParams( pPars );
                pPars->fCheckMiter = fCheckMiter;
                if ( Cec_ManSeqResimulateCounter( pGia, pPars, pCex ) )
                {
                    ABC_FREE( pCex );
                    break;
                }
                ABC_FREE( pCex );
            }
        }
        // write equivalence classes
        Gia_AigerWrite( pGia, "gore.aig", 0, 0, 0 );
        // reduce the model
        pReduce = Gia_ManSpecReduce( pGia, 0, 0, 1, 0, 0 );
        if ( pReduce )
        {
            pReduce = Gia_ManSeqStructSweep( pAux = pReduce, 1, 1, 0 );
            Gia_ManStop( pAux );
            Gia_AigerWrite( pReduce, "gsrm.aig", 0, 0, 0 );
//            Abc_Print( 1, "Speculatively reduced model was written into file \"%s\".\n", "gsrm.aig" );
//          Gia_ManPrintStatsShort( pReduce );
            Gia_ManStop( pReduce );
        }
    }
    return 1;
}




/**Function*************************************************************

  Synopsis    [Reduces AIG using equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFilterEquivsForSpeculation( Gia_Man_t * pGia, char * pName1, char * pName2, int fLatchA, int fLatchB )
{
    Gia_Man_t * pGia1, * pGia2, * pMiter;
    Gia_Obj_t * pObj1, * pObj2, * pObjM, * pObj;
    int i, iObj, iNext, Counter = 0;
    if ( pGia->pReprs == NULL || pGia->pNexts == NULL )
    {
        Abc_Print( 1, "Equivalences are not defined.\n" );
        return 0;
    }
    pGia1 = Gia_AigerRead( pName1, 0, 0, 0 );
    if ( pGia1 == NULL )
    {
        Abc_Print( 1, "Cannot read first file %s.\n", pName1 );
        return 0;
    }
    pGia2 = Gia_AigerRead( pName2, 0, 0, 0 );
    if ( pGia2 == NULL )
    {
        Gia_ManStop( pGia2 );
        Abc_Print( 1, "Cannot read second file %s.\n", pName2 );
        return 0;
    }
    pMiter = Gia_ManMiter( pGia1, pGia2, 0, 0, 1, 0, 0 );
    if ( pMiter == NULL )
    {
        Gia_ManStop( pGia1 );
        Gia_ManStop( pGia2 );
        Abc_Print( 1, "Cannot create sequential miter.\n" );
        return 0;
    }
    // make sure the miter is isomorphic
    if ( Gia_ManObjNum(pGia) != Gia_ManObjNum(pMiter) )
    {
        Gia_ManStop( pGia1 );
        Gia_ManStop( pGia2 );
        Gia_ManStop( pMiter );
        Abc_Print( 1, "The number of objects in different.\n" );
        return 0;
    }
    if ( memcmp( pGia->pObjs, pMiter->pObjs, sizeof(Gia_Obj_t) *  Gia_ManObjNum(pGia) ) )
    {
        Gia_ManStop( pGia1 );
        Gia_ManStop( pGia2 );
        Gia_ManStop( pMiter );
        Abc_Print( 1, "The AIG structure of the miter does not match.\n" );
        return 0;
    }
    // transfer copies
    Gia_ManCleanMark0( pGia );
    Gia_ManForEachObj( pGia1, pObj1, i )
    {
        if ( pObj1->Value == ~0 )
            continue;
        pObjM = Gia_ManObj( pMiter, Abc_Lit2Var(pObj1->Value) );
        pObj = Gia_ManObj( pGia, Gia_ObjId(pMiter, pObjM) );
        pObj->fMark0 = 1;
    }
    Gia_ManCleanMark1( pGia );
    Gia_ManForEachObj( pGia2, pObj2, i )
    {
        if ( pObj2->Value == ~0 )
            continue;
        pObjM = Gia_ManObj( pMiter, Abc_Lit2Var(pObj2->Value) );
        pObj = Gia_ManObj( pGia, Gia_ObjId(pMiter, pObjM) );
        pObj->fMark1 = 1;
    }

    // filter equivalences
    Gia_ManForEachConst( pGia, i )
    {
        Gia_ObjUnsetRepr( pGia, i );
        assert( pGia->pNexts[i] == 0 );
    }
    Gia_ManForEachClass( pGia, i )
    {
        // find the first colorA and colorB
        int ClassA = -1, ClassB = -1;
        Gia_ClassForEachObj( pGia, i, iObj )
        {
            pObj = Gia_ManObj( pGia, iObj );
            if ( ClassA == -1 && pObj->fMark0 && !pObj->fMark1 )
            {
                if ( fLatchA && !Gia_ObjIsRo(pGia, pObj) )
                    continue;
                ClassA = iObj;
            }
            if ( ClassB == -1 && pObj->fMark1 && !pObj->fMark0 )
            {
                if ( fLatchB && !Gia_ObjIsRo(pGia, pObj) )
                    continue;
                ClassB = iObj;
            }
        }
        // undo equivalence classes
        for ( iObj = i, iNext = Gia_ObjNext(pGia, iObj); iObj;
              iObj = iNext, iNext = Gia_ObjNext(pGia, iObj) )
        {
            Gia_ObjUnsetRepr( pGia, iObj );
            Gia_ObjSetNext( pGia, iObj, 0 );
        }
        assert( !Gia_ObjIsHead(pGia, i) );
        if ( ClassA > 0 && ClassB > 0 )
        {
            if ( ClassA > ClassB )
            {
                ClassA ^= ClassB;
                ClassB ^= ClassA;
                ClassA ^= ClassB;
            }
            assert( ClassA < ClassB );
            Gia_ObjSetNext( pGia, ClassA, ClassB );
            Gia_ObjSetRepr( pGia, ClassB, ClassA );
            Counter++;
            assert( Gia_ObjIsHead(pGia, ClassA) );
        }
    }
    Abc_Print( 1, "The number of two-node classes after filtering = %d.\n", Counter );
//Gia_ManEquivPrintClasses( pGia, 1, 0 );

    Gia_ManCleanMark0( pGia );
    Gia_ManCleanMark1( pGia );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Reduces AIG using equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFilterEquivsUsingParts( Gia_Man_t * pGia, char * pName1, char * pName2 )
{
    Vec_Int_t * vNodes;
    Gia_Man_t * pGia1, * pGia2, * pMiter;
    Gia_Obj_t * pObj1, * pObj2, * pObjM, * pObj = NULL;
    int i, k, iObj, iNext, iPrev, iRepr;
    int iLitsOld, iLitsNew;
    if ( pGia->pReprs == NULL || pGia->pNexts == NULL )
    {
        Abc_Print( 1, "Equivalences are not defined.\n" );
        return 0;
    }
    pGia1 = Gia_AigerRead( pName1, 0, 0, 0 );
    if ( pGia1 == NULL )
    {
        Abc_Print( 1, "Cannot read first file %s.\n", pName1 );
        return 0;
    }
    pGia2 = Gia_AigerRead( pName2, 0, 0, 0 );
    if ( pGia2 == NULL )
    {
        Gia_ManStop( pGia2 );
        Abc_Print( 1, "Cannot read second file %s.\n", pName2 );
        return 0;
    }
    pMiter = Gia_ManMiter( pGia1, pGia2, 0, 0, 1, 0, 0 );
    if ( pMiter == NULL )
    {
        Gia_ManStop( pGia1 );
        Gia_ManStop( pGia2 );
        Abc_Print( 1, "Cannot create sequential miter.\n" );
        return 0;
    }
    // make sure the miter is isomorphic
    if ( Gia_ManObjNum(pGia) != Gia_ManObjNum(pMiter) )
    {
        Gia_ManStop( pGia1 );
        Gia_ManStop( pGia2 );
        Gia_ManStop( pMiter );
        Abc_Print( 1, "The number of objects in different.\n" );
        return 0;
    }
    if ( memcmp( pGia->pObjs, pMiter->pObjs, sizeof(Gia_Obj_t) *  Gia_ManObjNum(pGia) ) )
    {
        Gia_ManStop( pGia1 );
        Gia_ManStop( pGia2 );
        Gia_ManStop( pMiter );
        Abc_Print( 1, "The AIG structure of the miter does not match.\n" );
        return 0;
    }
    // transfer copies
    Gia_ManCleanMark0( pGia );
    Gia_ManForEachObj( pGia1, pObj1, i )
    {
        if ( pObj1->Value == ~0 )
            continue;
        pObjM = Gia_ManObj( pMiter, Abc_Lit2Var(pObj1->Value) );
        pObj = Gia_ManObj( pGia, Gia_ObjId(pMiter, pObjM) );
        pObj->fMark0 = 1;
    }
    Gia_ManCleanMark1( pGia );
    Gia_ManForEachObj( pGia2, pObj2, i )
    {
        if ( pObj2->Value == ~0 )
            continue;
        pObjM = Gia_ManObj( pMiter, Abc_Lit2Var(pObj2->Value) );
        pObj = Gia_ManObj( pGia, Gia_ObjId(pMiter, pObjM) );
        pObj->fMark1 = 1;
    }

    // filter equivalences
    iLitsOld = iLitsNew = 0;
    Gia_ManForEachConst( pGia, i )
    {
        iLitsOld++;
        pObj = Gia_ManObj( pGia, i );
        assert( pGia->pNexts[i] == 0 );
        assert( pObj->fMark0 || pObj->fMark1 );
        if ( pObj->fMark0 && pObj->fMark1 ) // belongs to both A and B
            Gia_ObjUnsetRepr( pGia, i );
        else
            iLitsNew++;
    }
    // filter equivalences
    vNodes = Vec_IntAlloc( 100 );
    Gia_ManForEachClass( pGia, i )
    {
        int fSeenA = 0, fSeenB = 0;
        assert( pObj->fMark0 || pObj->fMark1 );
        Vec_IntClear( vNodes );
        Gia_ClassForEachObj( pGia, i, iObj )
        {
            pObj = Gia_ManObj( pGia, iObj );
            if ( pObj->fMark0 && !pObj->fMark1 )
            {
                fSeenA = 1;
                Vec_IntPush( vNodes, iObj );
            }
            if ( !pObj->fMark0 && pObj->fMark1 )
            {
                fSeenB = 1;
                Vec_IntPush( vNodes, iObj );
            }
            iLitsOld++;
        }
        iLitsOld--;
        // undo equivalence classes
        for ( iObj = i, iNext = Gia_ObjNext(pGia, iObj); iObj;
              iObj = iNext, iNext = Gia_ObjNext(pGia, iObj) )
        {
            Gia_ObjUnsetRepr( pGia, iObj );
            Gia_ObjSetNext( pGia, iObj, 0 );
        }
        assert( !Gia_ObjIsHead(pGia, i) );
        if ( fSeenA && fSeenB && Vec_IntSize(vNodes) > 1 )
        {
            // create new class
            iPrev = iRepr = Vec_IntEntry( vNodes, 0 );
            Vec_IntForEachEntryStart( vNodes, iObj, k, 1 )
            {
                Gia_ObjSetRepr( pGia, iObj, iRepr );
                Gia_ObjSetNext( pGia, iPrev, iObj );
                iPrev = iObj;
                iLitsNew++;
            }
            assert( Gia_ObjNext(pGia, iPrev) == 0 );
        }
    }
    Vec_IntFree( vNodes );
    Abc_Print( 1, "The number of literals: Before = %d. After = %d.\n", iLitsOld, iLitsNew );
//Gia_ManEquivPrintClasses( pGia, 1, 0 );

    Gia_ManCleanMark0( pGia );
    Gia_ManCleanMark1( pGia );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFilterEquivsUsingLatches( Gia_Man_t * pGia, int fFlopsOnly, int fFlopsWith, int fUseRiDrivers )
{
    Gia_Obj_t * pObjR;
    Vec_Int_t * vNodes, * vFfIds;
    int i, k, iObj, iNext, iPrev, iRepr;
    int iLitsOld = 0, iLitsNew = 0;
    assert( fFlopsOnly ^ fFlopsWith );
    vNodes = Vec_IntAlloc( 100 );
    // select nodes "flop" node IDs
    vFfIds = Vec_IntStart( Gia_ManObjNum(pGia) );
    if ( fUseRiDrivers )
    {
        Gia_ManForEachRi( pGia, pObjR, i )
            Vec_IntWriteEntry( vFfIds, Gia_ObjFaninId0p(pGia, pObjR), 1 );
    }
    else
    {
        Gia_ManForEachRo( pGia, pObjR, i )
            Vec_IntWriteEntry( vFfIds, Gia_ObjId(pGia, pObjR), 1 );
    }
    // remove all non-flop constants
    Gia_ManForEachConst( pGia, i )
    {
        iLitsOld++;
        assert( pGia->pNexts[i] == 0 );
        if ( !Vec_IntEntry(vFfIds, i) )
            Gia_ObjUnsetRepr( pGia, i );
        else
            iLitsNew++;
    }
    // clear the classes
    if ( fFlopsOnly )
    {
        Gia_ManForEachClass( pGia, i )
        {
            Vec_IntClear( vNodes );
            Gia_ClassForEachObj( pGia, i, iObj )
            {
                if ( Vec_IntEntry(vFfIds, iObj) )
                    Vec_IntPush( vNodes, iObj );
                iLitsOld++;
            }
            iLitsOld--;
            // undo equivalence classes
            for ( iObj = i, iNext = Gia_ObjNext(pGia, iObj); iObj;
                  iObj = iNext, iNext = Gia_ObjNext(pGia, iObj) )
            {
                Gia_ObjUnsetRepr( pGia, iObj );
                Gia_ObjSetNext( pGia, iObj, 0 );
            }
            assert( !Gia_ObjIsHead(pGia, i) );
            if ( Vec_IntSize(vNodes) > 1 )
            {
                // create new class
                iPrev = iRepr = Vec_IntEntry( vNodes, 0 );
                Vec_IntForEachEntryStart( vNodes, iObj, k, 1 )
                {
                    Gia_ObjSetRepr( pGia, iObj, iRepr );
                    Gia_ObjSetNext( pGia, iPrev, iObj );
                    iPrev = iObj;
                    iLitsNew++;
                }
                assert( Gia_ObjNext(pGia, iPrev) == 0 );
            }
        }
    }
    else
    {
        Gia_ManForEachClass( pGia, i )
        {
            int fSeenFlop = 0;
            Gia_ClassForEachObj( pGia, i, iObj )
            {
                if ( Vec_IntEntry(vFfIds, iObj) )
                    fSeenFlop = 1;
                iLitsOld++;
                iLitsNew++;
            }
            iLitsOld--;
            iLitsNew--;
            if ( fSeenFlop )
                continue;
            // undo equivalence classes
            for ( iObj = i, iNext = Gia_ObjNext(pGia, iObj); iObj;
                  iObj = iNext, iNext = Gia_ObjNext(pGia, iObj) )
            {
                Gia_ObjUnsetRepr( pGia, iObj );
                Gia_ObjSetNext( pGia, iObj, 0 );
                iLitsNew--;
            }
            iLitsNew++;
            assert( !Gia_ObjIsHead(pGia, i) );
        }
    }
    Vec_IntFree( vNodes );
    Vec_IntFree( vFfIds );
    Abc_Print( 1, "The number of literals: Before = %d. After = %d.\n", iLitsOld, iLitsNew );
}

/**Function*************************************************************

  Synopsis    [Changing node order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManChangeOrder_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return pObj->Value;
    if ( Gia_ObjIsCi(pObj) )
        return pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManChangeOrder_rec( pNew, p, Gia_ObjFanin0(pObj) );
    if ( Gia_ObjIsCo(pObj) )
        return pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManChangeOrder_rec( pNew, p, Gia_ObjFanin1(pObj) );
    return pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
} 
Gia_Man_t * Gia_ManChangeOrder( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i, k;
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachClass( p, i )
        Gia_ClassForEachObj( p, i, k )
            Gia_ManChangeOrder_rec( pNew, p, Gia_ManObj(p, k) );
    Gia_ManForEachConst( p, k )
        Gia_ManChangeOrder_rec( pNew, p, Gia_ManObj(p, k) );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManChangeOrder_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    assert( Gia_ManObjNum(pNew) == Gia_ManObjNum(p) );
    return pNew;
}
void Gia_ManTransferEquivs( Gia_Man_t * p, Gia_Man_t * pNew )
{
    Vec_Int_t * vClass;
    int i, k, iNode, iRepr;
    assert( Gia_ManObjNum(p) == Gia_ManObjNum(pNew) );
    assert( p->pReprs != NULL );
    assert( p->pNexts != NULL );
    assert( pNew->pReprs == NULL );
    assert( pNew->pNexts == NULL );
    // start representatives
    pNew->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(pNew) );
    for ( i = 0; i < Gia_ManObjNum(pNew); i++ )
        Gia_ObjSetRepr( pNew, i, GIA_VOID );
    // iterate over constant candidates
    Gia_ManForEachConst( p, i )
        Gia_ObjSetRepr( pNew, Abc_Lit2Var(Gia_ManObj(p, i)->Value), 0 );
    // iterate over class candidates
    vClass = Vec_IntAlloc( 100 );
    Gia_ManForEachClass( p, i )
    {
        Vec_IntClear( vClass );
        Gia_ClassForEachObj( p, i, k )
            Vec_IntPushUnique( vClass, Abc_Lit2Var(Gia_ManObj(p, k)->Value) );
        assert( Vec_IntSize( vClass ) > 1 );
        Vec_IntSort( vClass, 0 );
        iRepr = Vec_IntEntry( vClass, 0 );
        Vec_IntForEachEntryStart( vClass, iNode, k, 1 )
            Gia_ObjSetRepr( pNew, iNode, iRepr );
    }
    Vec_IntFree( vClass );
    pNew->pNexts = Gia_ManDeriveNexts( pNew );
}
void Gia_ManTransferTest( Gia_Man_t * p )
{
    Gia_Obj_t * pObj; int i;
    Gia_Rpr_t * pReprs = p->pReprs;  // representatives (for CIs and ANDs)
    int *       pNexts = p->pNexts;  // next nodes in the equivalence classes
    Gia_Man_t * pNew = Gia_ManChangeOrder(p);
    //Gia_ManEquivPrintClasses( p, 1, 0 );
    assert( Gia_ManObjNum(p) == Gia_ManObjNum(pNew) );
    Gia_ManTransferEquivs( p, pNew );
    p->pReprs = NULL;
    p->pNexts = NULL;
    // make new point to old
    Gia_ManForEachObj( p, pObj, i )
    {
        assert( !Abc_LitIsCompl(pObj->Value) );
        Gia_ManObj(pNew, Abc_Lit2Var(pObj->Value))->Value = Abc_Var2Lit(i, 0);
    }
    Gia_ManTransferEquivs( pNew, p );
    //Gia_ManEquivPrintClasses( p, 1, 0 );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        pReprs[i].fProved = 0;
        //printf( "%5d :    %5d %5d    %5d %5d\n", i, *(int*)&p->pReprs[i], *(int*)&pReprs[i], (int)p->pNexts[i], (int)pNexts[i] );
    if ( memcmp(p->pReprs, pReprs, sizeof(int)*Gia_ManObjNum(p)) )
        printf( "Verification of reprs failed.\n" );
    else
        printf( "Verification of reprs succeeded.\n" );
    if ( memcmp(p->pNexts, pNexts, sizeof(int)*Gia_ManObjNum(p)) )
        printf( "Verification of nexts failed.\n" );
    else
        printf( "Verification of nexts succeeded.\n" );
    ABC_FREE( pNew->pReprs );
    ABC_FREE( pNew->pNexts );
    ABC_FREE( pReprs );
    ABC_FREE( pNexts );
    Gia_ManStop( pNew );
}


/**Function*************************************************************

  Synopsis    [Converting AIG after SAT sweeping into AIG with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec4_ManMarkIndependentClasses_rec( Gia_Man_t * p, int iObj )
{
    Gia_Obj_t * pObj;
    assert( iObj > 0 );
    if ( Gia_ObjIsTravIdPreviousId(p, iObj) ) // failed
        return 0;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) ) // passed
        return 1;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsCi(pObj) )
        return 1;
    assert( Gia_ObjIsAnd(pObj) );
    if ( Cec4_ManMarkIndependentClasses_rec( p, Gia_ObjFaninId0(pObj, iObj) ) && 
         Cec4_ManMarkIndependentClasses_rec( p, Gia_ObjFaninId1(pObj, iObj) ) )
        return 1;
    Gia_ObjSetTravIdPreviousId(p, iObj);
    return 0;
}
int Cec4_ManMarkIndependentClasses( Gia_Man_t * p, Gia_Man_t * pNew )
{
    int iObjNew, iRepr, iObj, Res, fHaveChoices = 0;
    Gia_ManCleanMark01(p);
    Gia_ManForEachClass( p, iRepr )
    {
        Gia_ManIncrementTravId( pNew );
        Gia_ManIncrementTravId( pNew );
        iObjNew = Abc_Lit2Var( Gia_ManObj(p, iRepr)->Value );
        Res = Cec4_ManMarkIndependentClasses_rec( pNew, iObjNew );
        assert( Res == 1 );
        Gia_ObjSetTravIdPreviousId( pNew, iObjNew );
        p->pReprs[iRepr].fColorA = 1;
        Gia_ClassForEachObj1( p, iRepr, iObj )
        {
            assert( p->pReprs[iObj].iRepr == (unsigned)iRepr );
            iObjNew = Abc_Lit2Var( Gia_ManObj(p, iObj)->Value );
            if ( Cec4_ManMarkIndependentClasses_rec( pNew, iObjNew ) )
            {
                p->pReprs[iObj].fColorA = 1;
                fHaveChoices = 1;
            }
            Gia_ObjSetTravIdPreviousId( pNew, iObjNew );
        }
    }
    return fHaveChoices;
}
int Cec4_ManSatSolverAnd_rec( Gia_Man_t * pCho, Gia_Man_t * p, Gia_Man_t * pNew, int iObj )
{
    return 0;
}
int Cec4_ManSatSolverChoices_rec( Gia_Man_t * pCho, Gia_Man_t * p, Gia_Man_t * pNew, int iObj )
{
    if ( !Gia_ObjIsClass(p, iObj) )
        return Cec4_ManSatSolverAnd_rec( pCho, p, pNew, iObj );
    else
    {
        Vec_Int_t * vLits = Vec_IntAlloc( 100 );
        int i, iHead, iNext, iRepr = Gia_ObjIsHead(p, iObj) ? iObj : Gia_ObjRepr(p, iObj);
        Gia_ClassForEachObj( p, iRepr, iObj )
            if ( p->pReprs[iObj].fColorA )
                Vec_IntPush( vLits, Cec4_ManSatSolverAnd_rec( pCho, p, pNew, iObj ) );
        Vec_IntSort( vLits, 1 );
        iHead = Abc_Lit2Var( Vec_IntEntry(vLits, 0) );
        if ( Vec_IntSize(vLits) > 1 )
        {
            Vec_IntForEachEntryStart( vLits, iNext, i, 1 )
            {
                pCho->pSibls[iHead] = Abc_Lit2Var(iNext);  
                iHead = Abc_Lit2Var(iNext);
            }
        }
        return Abc_LitNotCond( Vec_IntEntry(vLits, 0), Gia_ManObj(p, iHead)->fPhase );
    }
}
Gia_Man_t * Cec4_ManSatSolverChoices( Gia_Man_t * p, Gia_Man_t * pNew )
{
    Gia_Man_t * pCho;
    Gia_Obj_t * pObj; 
    int i, DriverId;
    // mark topologically dependent equivalent nodes
    if ( !Cec4_ManMarkIndependentClasses( p, pNew ) )
        return Gia_ManDup( pNew );
    // rebuild AIG in a different order with choices
    pCho = Gia_ManStart( Gia_ManObjNum(pNew) );
    pCho->pName = Abc_UtilStrsav( p->pName );
    pCho->pSpec = Abc_UtilStrsav( p->pSpec );
    pCho->pSibls = ABC_CALLOC( int, Gia_ManObjNum(pNew) );
    Gia_ManFillValue(pNew);
    Gia_ManConst0(pNew)->Value = 0;
    for ( i = 0; i < Gia_ManCiNum(pNew); i++ )
        Gia_ManCi(pNew, i)->Value = Gia_ManAppendCi( pCho );
    Gia_ManForEachCoDriverId( p, DriverId, i )
        Cec4_ManSatSolverChoices_rec( pCho, p, pNew, DriverId );
    Gia_ManForEachCo( pNew, pObj, i )
        pObj->Value = Gia_ManAppendCo( pCho, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pCho, Gia_ManRegNum(p) );
    return pCho;
}

/**Function*************************************************************

  Synopsis    [Converting AIG after SAT sweeping into AIG with choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManCombSpecReduce( Gia_Man_t * p )
{
    Gia_Obj_t * pObj, * pRepr; int i, iLit;
    Vec_Int_t * vXors = Vec_IntAlloc( 100 );
    Gia_Man_t * pTemp, * pNew = Gia_ManStart( Gia_ManObjNum(p) );
    assert( p->pReprs && p->pNexts );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManLevelNum(p);
    Gia_ManSetPhase(p);
    Gia_ManFillValue(p);
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachAnd( p, pObj, i )
    {
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        pRepr = Gia_ObjReprObj( p, i );
        if ( pRepr && Abc_Lit2Var(pObj->Value) != Abc_Lit2Var(pRepr->Value) )
        {
            //if ( Gia_ObjLevel(p, pRepr) > Gia_ObjLevel(p, pObj) + 50 )
            //printf( "%d %d  ", Gia_ObjLevel(p, pRepr), Gia_ObjLevel(p, pObj) );
            iLit = Abc_LitNotCond( pRepr->Value, pObj->fPhase ^ pRepr->fPhase );
            Vec_IntPush( vXors, Gia_ManHashXor( pNew, pObj->Value, iLit ) );
            pObj->Value = iLit;
        }
    }
    Gia_ManHashStop( pNew );
    if ( Vec_IntSize(vXors) == 0 )
        Vec_IntPush( vXors, 0 );
    Vec_IntForEachEntry( vXors, iLit, i )
        Gia_ManAppendCo( pNew, iLit );
    Vec_IntFree( vXors );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
void Gia_ManCombSpecReduceTest( Gia_Man_t * p, char * pFileName )
{
    Gia_Man_t * pSrm = Gia_ManCombSpecReduce( p );
    if ( pFileName == NULL )
        pFileName = "test.aig";
    Gia_AigerWrite( pSrm, pFileName, 0, 0, 0 );
    Abc_Print( 1, "Speculatively reduced model was written into file \"%s\".\n", pFileName );
    Gia_ManStop( pSrm );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
