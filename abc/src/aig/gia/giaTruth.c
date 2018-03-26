/**CFile****************************************************************

  FileName    [giaTruth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Truth table computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaTruth.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/vec/vecMem.h"
#include "misc/vec/vecWec.h"
#include "misc/util/utilTruth.h"
#include "opt/dau/dau.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static word s_Truth6[6] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000)
};

static inline word * Gla_ObjTruthElem( Gia_Man_t * p, int i )            { return (word *)Vec_PtrEntry( p->vTtInputs, i );                                           }
static inline word * Gla_ObjTruthNodeId( Gia_Man_t * p, int Id )         { return Vec_WrdArray(p->vTtMemory) + p->nTtWords * Id;                                     }
static inline word * Gla_ObjTruthNode( Gia_Man_t * p, Gia_Obj_t * pObj ) { return Vec_WrdArray(p->vTtMemory) + p->nTtWords * Gia_ObjNum(p, pObj);                    }
static inline word * Gla_ObjTruthFree1( Gia_Man_t * p )                  { return Vec_WrdArray(p->vTtMemory) + Vec_WrdSize(p->vTtMemory) - p->nTtWords * 1;          }
static inline word * Gla_ObjTruthFree2( Gia_Man_t * p )                  { return Vec_WrdArray(p->vTtMemory) + Vec_WrdSize(p->vTtMemory) - p->nTtWords * 2;          }
static inline word * Gla_ObjTruthConst0( Gia_Man_t * p, word * pDst )                   { int w; for ( w = 0; w < p->nTtWords; w++ ) pDst[w] = 0; return pDst;                      }
static inline word * Gla_ObjTruthDup( Gia_Man_t * p, word * pDst, word * pSrc, int c )  { int w; for ( w = 0; w < p->nTtWords; w++ ) pDst[w] = c ? ~pSrc[w] : pSrc[w]; return pDst; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Compute truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Gia_LutComputeTruth6_rec( Gia_Man_t * p, int iNode, Vec_Wrd_t * vTruths )
{
    Gia_Obj_t * pObj;
    word Truth0, Truth1;
    if ( Gia_ObjIsTravIdCurrentId(p, iNode) )
        return Vec_WrdEntry(vTruths, iNode);
    Gia_ObjSetTravIdCurrentId(p, iNode);
    pObj = Gia_ManObj( p, iNode );
    assert( Gia_ObjIsAnd(pObj) );
    Truth0 = Gia_LutComputeTruth6_rec( p, Gia_ObjFaninId0p(p, pObj), vTruths );
    Truth1 = Gia_LutComputeTruth6_rec( p, Gia_ObjFaninId1p(p, pObj), vTruths );
    if ( Gia_ObjFaninC0(pObj) )
        Truth0 = ~Truth0;
    if ( Gia_ObjFaninC1(pObj) )
        Truth1 = ~Truth1;
    Vec_WrdWriteEntry( vTruths, iNode, Truth0 & Truth1 );
    return Truth0 & Truth1;
}
word Gia_LutComputeTruth6( Gia_Man_t * p, int iObj, Vec_Wrd_t * vTruths )
{
    int k, iFan;
    assert( Gia_ObjIsLut(p, iObj) );
    Gia_ManIncrementTravId( p );
    Gia_LutForEachFanin( p, iObj, iFan, k )
    {
        Vec_WrdWriteEntry( vTruths, iFan, s_Truths6[k] );
        Gia_ObjSetTravIdCurrentId( p, iFan );
    }
    return Gia_LutComputeTruth6_rec( p, iObj, vTruths );
}

/**Function*************************************************************

  Synopsis    [Computes truth table of a 6-LUT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjComputeTruthTable6Lut_rec( Gia_Man_t * p, int iObj, Vec_Wrd_t * vTemp )
{
    word uTruth0, uTruth1;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ObjComputeTruthTable6Lut_rec( p, Gia_ObjFaninId0p(p, pObj), vTemp );
    Gia_ObjComputeTruthTable6Lut_rec( p, Gia_ObjFaninId1p(p, pObj), vTemp );
    uTruth0 = Vec_WrdEntry( vTemp, Gia_ObjFaninId0p(p, pObj) );
    uTruth0 = Gia_ObjFaninC0(pObj) ? ~uTruth0 : uTruth0;
    uTruth1 = Vec_WrdEntry( vTemp, Gia_ObjFaninId1p(p, pObj) );
    uTruth1 = Gia_ObjFaninC1(pObj) ? ~uTruth1 : uTruth1;
    Vec_WrdWriteEntry( vTemp, iObj, uTruth0 & uTruth1 );
}
word Gia_ObjComputeTruthTable6Lut( Gia_Man_t * p, int iObj, Vec_Wrd_t * vTemp )
{
    int i, Fanin;
    assert( Vec_WrdSize(vTemp) == Gia_ManObjNum(p) );
    assert( Gia_ObjIsLut(p, iObj) );
    Gia_ManIncrementTravId( p );
    Gia_LutForEachFanin( p, iObj, Fanin, i )
    {
        Gia_ObjSetTravIdCurrentId( p, Fanin );
        Vec_WrdWriteEntry( vTemp, Fanin, s_Truth6[i] );
    }
    assert( i <= 6 );
    Gia_ObjComputeTruthTable6Lut_rec( p, iObj, vTemp );
    return Vec_WrdEntry( vTemp, iObj );
}

/**Function*************************************************************

  Synopsis    [Computes truth table up to 6 inputs in terms of CIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Gia_ObjComputeTruth6( Gia_Man_t * p, int iObj, Vec_Int_t * vSupp, Vec_Wrd_t * vTemp )
{
    int i, Fanin;
    assert( Vec_WrdSize(vTemp) == Gia_ManObjNum(p) );
    assert( Vec_IntSize(vSupp) <= 6 );
    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vSupp, Fanin, i )
    {
        Gia_ObjSetTravIdCurrentId( p, Fanin );
        Vec_WrdWriteEntry( vTemp, Fanin, s_Truth6[i] );
    }
    Gia_ObjComputeTruthTable6Lut_rec( p, iObj, vTemp );
    return Vec_WrdEntry( vTemp, iObj );
}
void Gia_ObjComputeTruth6CisSupport_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vSupp )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPushOrder( vSupp, iObj );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ObjComputeTruth6CisSupport_rec( p, Gia_ObjFaninId0p(p, pObj), vSupp );
    Gia_ObjComputeTruth6CisSupport_rec( p, Gia_ObjFaninId1p(p, pObj), vSupp );
}
word Gia_ObjComputeTruth6Cis( Gia_Man_t * p, int iLit, Vec_Int_t * vSupp, Vec_Wrd_t * vTemp )
{
    int iObj = Abc_Lit2Var(iLit); 
    Vec_IntClear( vSupp );
    if ( !iObj ) return Abc_LitIsCompl(iLit) ? ~(word)0 : (word)0;
    Gia_ManIncrementTravId( p );
    Gia_ObjComputeTruth6CisSupport_rec( p, iObj, vSupp );
    if ( Vec_IntSize(vSupp) > 6 )
        return 0;
    Gia_ObjComputeTruth6( p, iObj, vSupp, vTemp );
    return Abc_LitIsCompl(iLit) ? ~Vec_WrdEntry(vTemp, iObj) : Vec_WrdEntry(vTemp, iObj);
}

/**Function*************************************************************

  Synopsis    [Computes truth table up to 6 inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjComputeTruthTable6_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Wrd_t * vTruths )
{
    word uTruth0, uTruth1;
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    assert( !pObj->fMark0 );
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ObjComputeTruthTable6_rec( p, Gia_ObjFanin0(pObj), vTruths );
    Gia_ObjComputeTruthTable6_rec( p, Gia_ObjFanin1(pObj), vTruths );
    uTruth0 = Vec_WrdEntry( vTruths, Gia_ObjFanin0(pObj)->Value );
    uTruth0 = Gia_ObjFaninC0(pObj) ? ~uTruth0 : uTruth0;
    uTruth1 = Vec_WrdEntry( vTruths, Gia_ObjFanin1(pObj)->Value );
    uTruth1 = Gia_ObjFaninC1(pObj) ? ~uTruth1 : uTruth1;
    pObj->Value = Vec_WrdSize(vTruths);
    Vec_WrdPush( vTruths, uTruth0 & uTruth1 );
}
word Gia_ObjComputeTruthTable6( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp, Vec_Wrd_t * vTruths )
{
    Gia_Obj_t * pLeaf;
    int i;
    assert( Vec_IntSize(vSupp) <= 6 );
    assert( Gia_ObjIsAnd(pObj) );
    assert( !pObj->fMark0 );
    Vec_WrdClear( vTruths );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachObjVec( vSupp, p, pLeaf, i )
    {
        assert( pLeaf->fMark0 || Gia_ObjIsRo(p, pLeaf) );
        pLeaf->Value = Vec_WrdSize(vTruths);
        Vec_WrdPush( vTruths, s_Truth6[i] );
        Gia_ObjSetTravIdCurrent(p, pLeaf);
    }
    Gia_ObjComputeTruthTable6_rec( p, pObj, vTruths );
    return Vec_WrdEntryLast( vTruths );
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes reachable from the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjCollectInternal_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( !Gia_ObjIsAnd(pObj) )
        return;
    if ( pObj->fMark0 )
        return;
    pObj->fMark0 = 1;
    Gia_ObjCollectInternal_rec( p, Gia_ObjFanin0(pObj) );
    Gia_ObjCollectInternal_rec( p, Gia_ObjFanin1(pObj) );
    Gia_ObjSetNum( p, pObj, Vec_IntSize(p->vTtNodes) );
    Vec_IntPush( p->vTtNodes, Gia_ObjId(p, pObj) );
}
void Gia_ObjCollectInternal( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    Vec_IntClear( p->vTtNodes );
    Gia_ObjCollectInternal_rec( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Computing the truth table for GIA object.]

  Description [The truth table should be used by the calling application
  (or saved into the user's storage) before this procedure is called again.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word * Gia_ObjComputeTruthTable( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    Gia_Obj_t * pTemp, * pRoot;
    word * pTruth, * pTruthL, * pTruth0, * pTruth1;
    int i;
    if ( p->vTtMemory == NULL )
    {
        p->nTtVars   = Gia_ManPiNum( p );
        p->nTtWords  = Abc_Truth6WordNum( p->nTtVars );
        p->vTtNums   = Vec_IntStart( Gia_ManObjNum(p) + 1000 );
        p->vTtNodes  = Vec_IntAlloc( 256 );
        p->vTtInputs = Vec_PtrAllocTruthTables( Abc_MaxInt(6, p->nTtVars) );
        p->vTtMemory = Vec_WrdStart( p->nTtWords * 256 );
    }
    else
    {
        // make sure the number of primary inputs did not change 
        // since the truth table computation storage was prepared
        assert( p->nTtVars == Gia_ManPiNum(p) );
    }
    // extend ID numbers
    if ( Vec_IntSize(p->vTtNums) < Gia_ManObjNum(p) )
        Vec_IntFillExtra( p->vTtNums, Gia_ManObjNum(p), 0 );
    // collect internal nodes
    pRoot = Gia_ObjIsCo(pObj) ? Gia_ObjFanin0(pObj) : pObj;
    Gia_ObjCollectInternal( p, pRoot );
    // extend TT storage
    if ( Vec_WrdSize(p->vTtMemory) < p->nTtWords * (Vec_IntSize(p->vTtNodes) + 2) )
        Vec_WrdFillExtra( p->vTtMemory, p->nTtWords * (Vec_IntSize(p->vTtNodes) + 2), 0 );
    // compute the truth table for internal nodes
    Gia_ManForEachObjVec( p->vTtNodes, p, pTemp, i )
    {
        pTemp->fMark0 = 0; // unmark nodes marked by Gia_ObjCollectInternal()
        pTruth  = Gla_ObjTruthNode(p, pTemp);
        pTruthL = pTruth + p->nTtWords;
        pTruth0 = Gia_ObjIsAnd(Gia_ObjFanin0(pTemp)) ? Gla_ObjTruthNode(p, Gia_ObjFanin0(pTemp)) : Gla_ObjTruthElem(p, Gia_ObjCioId(Gia_ObjFanin0(pTemp)) );
        pTruth1 = Gia_ObjIsAnd(Gia_ObjFanin1(pTemp)) ? Gla_ObjTruthNode(p, Gia_ObjFanin1(pTemp)) : Gla_ObjTruthElem(p, Gia_ObjCioId(Gia_ObjFanin1(pTemp)) );
        if ( Gia_ObjFaninC0(pTemp) )
        {
            if ( Gia_ObjFaninC1(pTemp) )
                while ( pTruth < pTruthL )
                    *pTruth++ = ~*pTruth0++ & ~*pTruth1++;
            else
                while ( pTruth < pTruthL )
                    *pTruth++ = ~*pTruth0++ &  *pTruth1++;
        }
        else
        {
            if ( Gia_ObjFaninC1(pTemp) )
                while ( pTruth < pTruthL )
                    *pTruth++ =  *pTruth0++ & ~*pTruth1++;
            else
                while ( pTruth < pTruthL )
                    *pTruth++ =  *pTruth0++ &  *pTruth1++;
        }
    }
    // compute the final table
    if ( Gia_ObjIsConst0(pRoot) )
        pTruth = Gla_ObjTruthConst0( p, Gla_ObjTruthFree1(p) );
    else if ( Gia_ObjIsPi(p, pRoot) )
        pTruth = Gla_ObjTruthElem( p, Gia_ObjCioId(pRoot) );
    else if ( Gia_ObjIsAnd(pRoot) )
        pTruth = Gla_ObjTruthNode( p, pRoot );
    else
        pTruth = NULL;
    return Gla_ObjTruthDup( p, Gla_ObjTruthFree2(p), pTruth, Gia_ObjIsCo(pObj) && Gia_ObjFaninC0(pObj) );
}

/**Function*************************************************************

  Synopsis    [Testing truth table computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjComputeTruthTableTest( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    unsigned * pTruth;
    abctime clk = Abc_Clock();
    int i;
    Gia_ManForEachPo( p, pObj, i )
    {
        pTruth = (unsigned *)Gia_ObjComputeTruthTable( p, pObj );
//        Extra_PrintHex( stdout, pTruth, Gia_ManPiNum(p) ); printf( "\n" );
    }
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjComputeTruthTableStart( Gia_Man_t * p, int nVarsMax )
{
    assert( p->vTtMemory == NULL );
    p->nTtVars   = nVarsMax;
    p->nTtWords  = Abc_Truth6WordNum( p->nTtVars );
    p->vTtNodes  = Vec_IntAlloc( 256 );
    p->vTtInputs = Vec_PtrAllocTruthTables( Abc_MaxInt(6, p->nTtVars) );
    p->vTtMemory = Vec_WrdStart( p->nTtWords * 64 );
    p->vTtNums   = Vec_IntAlloc( Gia_ManObjNum(p) + 1000 );
    Vec_IntFill( p->vTtNums, Vec_IntCap(p->vTtNums), -ABC_INFINITY );
}
void Gia_ObjComputeTruthTableStop( Gia_Man_t * p )
{
    p->nTtVars   = 0;
    p->nTtWords  = 0;
    Vec_IntFreeP( &p->vTtNums );
    Vec_IntFreeP( &p->vTtNodes );
    Vec_PtrFreeP( &p->vTtInputs );
    Vec_WrdFreeP( &p->vTtMemory );
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes reachable from the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ObjCollectInternalCut_rec( Gia_Man_t * p, int iObj )
{
    if ( Gia_ObjHasNumId(p, iObj) )
        return;
    assert( Gia_ObjIsAnd(Gia_ManObj(p, iObj)) );
    Gia_ObjCollectInternalCut_rec( p, Gia_ObjFaninId0(Gia_ManObj(p, iObj), iObj) );
    Gia_ObjCollectInternalCut_rec( p, Gia_ObjFaninId1(Gia_ManObj(p, iObj), iObj) );
    Gia_ObjSetNumId( p, iObj, Vec_IntSize(p->vTtNodes) );
    Vec_IntPush( p->vTtNodes, iObj );
}
void Gia_ObjCollectInternalCut( Gia_Man_t * p, int iRoot, Vec_Int_t * vLeaves )
{
    int i, iObj;
    assert( !Gia_ObjHasNumId(p, iRoot) );
    assert( Gia_ObjIsAnd(Gia_ManObj(p, iRoot)) );
    Vec_IntForEachEntry( vLeaves, iObj, i )
    {
        if ( Gia_ObjHasNumId(p, iObj) ) // if cuts have repeated variables, skip 
            continue;
        assert( !Gia_ObjHasNumId(p, iObj) );
        Gia_ObjSetNumId( p, iObj, -i );
    }
    assert( !Gia_ObjHasNumId(p, iRoot) ); // the root cannot be one of the leaves
    Vec_IntClear( p->vTtNodes );
    Vec_IntPush( p->vTtNodes, -1 );
    Gia_ObjCollectInternalCut_rec( p, iRoot );
}

/**Function*************************************************************

  Synopsis    [Computes the truth table of pRoot in terms of leaves.]

  Description [The root cannot be one of the leaves.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word * Gia_ObjComputeTruthTableCut( Gia_Man_t * p, Gia_Obj_t * pRoot, Vec_Int_t * vLeaves )
{
    Gia_Obj_t * pTemp;
    word * pTruth, * pTruthL, * pTruth0, * pTruth1;
    int i, iObj, Id0, Id1;
    assert( p->vTtMemory != NULL );
    assert( Vec_IntSize(vLeaves) <= p->nTtVars );
    // extend ID numbers
    if ( Vec_IntSize(p->vTtNums) < Gia_ManObjNum(p) )
        Vec_IntFillExtra( p->vTtNums, Gia_ManObjNum(p), -ABC_INFINITY );
    // collect internal nodes
    Gia_ObjCollectInternalCut( p, Gia_ObjId(p, pRoot), vLeaves );
    // extend TT storage
    if ( Vec_WrdSize(p->vTtMemory) < p->nTtWords * (Vec_IntSize(p->vTtNodes) + 2) )
        Vec_WrdFillExtra( p->vTtMemory, p->nTtWords * (Vec_IntSize(p->vTtNodes) + 2), 0 );
    // compute the truth table for internal nodes
    Vec_IntForEachEntryStart( p->vTtNodes, iObj, i, 1 )
    {
        assert( i == Gia_ObjNumId(p, iObj) );
        pTemp   = Gia_ManObj( p, iObj );
        pTruth  = Gla_ObjTruthNodeId( p, i );  
        pTruthL = pTruth + p->nTtWords;
        Id0 = Gia_ObjNumId( p, Gia_ObjFaninId0(pTemp, iObj) );
        Id1 = Gia_ObjNumId( p, Gia_ObjFaninId1(pTemp, iObj) );
        pTruth0 = (Id0 > 0) ? Gla_ObjTruthNodeId(p, Id0) : Gla_ObjTruthElem(p, -Id0);
        pTruth1 = (Id1 > 0) ? Gla_ObjTruthNodeId(p, Id1) : Gla_ObjTruthElem(p, -Id1);
        if ( Gia_ObjFaninC0(pTemp) )
        {
            if ( Gia_ObjFaninC1(pTemp) )
                while ( pTruth < pTruthL )
                    *pTruth++ = ~*pTruth0++ & ~*pTruth1++;
            else
                while ( pTruth < pTruthL )
                    *pTruth++ = ~*pTruth0++ &  *pTruth1++;
        }
        else
        {
            if ( Gia_ObjFaninC1(pTemp) )
                while ( pTruth < pTruthL )
                    *pTruth++ =  *pTruth0++ & ~*pTruth1++;
            else
                while ( pTruth < pTruthL )
                    *pTruth++ =  *pTruth0++ &  *pTruth1++;
        }
    }
    pTruth = Gla_ObjTruthNode( p, pRoot );
    // unmark leaves marked by Gia_ObjCollectInternal()
    Vec_IntForEachEntry( vLeaves, iObj, i )
        Gia_ObjResetNumId( p, iObj );
    Vec_IntForEachEntryStart( p->vTtNodes, iObj, i, 1 )
        Gia_ObjResetNumId( p, iObj );
    return pTruth;
}

/**Function*************************************************************

  Synopsis    [Reduces GIA to contain isomorphic POs.]

  Description [The root cannot be one of the leaves.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManIsoNpnReduce( Gia_Man_t * p, Vec_Ptr_t ** pvPosEquivs, int fVerbose )
{
    char pCanonPerm[16];
    int i, iObj, uCanonPhase, nVars, lastId, truthId;
    int IndexCon = -1, IndexVar = -1;
    Vec_Wec_t * vPosEquivs = Vec_WecAlloc( 100 );
    word * pTruth;
    Gia_Obj_t * pObj;
    Vec_Mem_t * vTtMem[17];   // truth table memory and hash table
    Gia_Man_t * pNew = NULL;
    Vec_Int_t * vLeaves = Vec_IntAlloc( 16 );
    Vec_Int_t * vFirsts;
    Vec_Int_t * vTt2Class[17];
    for ( i = 0; i < 17; i++ )
    {
        vTtMem[i] = Vec_MemAlloc( Abc_TtWordNum(i), 10 );
        Vec_MemHashAlloc( vTtMem[i], 1000 );
        vTt2Class[i] = Vec_IntStartFull( Gia_ManCoNum(p)+1 );
    }
    Gia_ObjComputeTruthTableStart( p, 16 );
    Gia_ManForEachPo( p, pObj, i )
    {
        iObj = Gia_ObjId(p, pObj);
        Gia_ManCollectCis( p, &iObj, 1, vLeaves );
        if ( Vec_IntSize(vLeaves) > 16 )
        {
            Vec_IntPush( Vec_WecPushLevel(vPosEquivs), i );
            continue;
        }
        pObj = Gia_ObjFanin0(pObj);
        if ( Gia_ObjIsConst0(pObj) )
        {
            if ( IndexCon == -1 )
            {
                IndexCon = Vec_WecSize(vPosEquivs);
                Vec_WecPushLevel(vPosEquivs);
            }
            Vec_WecPush( vPosEquivs, IndexCon, i );
            continue;
        }
        if ( Gia_ObjIsCi(pObj) )
        {
            if ( IndexVar == -1 )
            {
                IndexVar = Vec_WecSize(vPosEquivs);
                Vec_WecPushLevel(vPosEquivs);
            }
            Vec_WecPush( vPosEquivs, IndexVar, i );
            continue;
        }
        assert( Gia_ObjIsAnd(pObj) );
        pTruth = Gia_ObjComputeTruthTableCut( p, pObj, vLeaves );
        Abc_TtMinimumBase( pTruth, NULL, Vec_IntSize(vLeaves), &nVars );
        if ( nVars == 0 )
        {
            if ( IndexCon == -1 )
            {
                IndexCon = Vec_WecSize(vPosEquivs);
                Vec_WecPushLevel(vPosEquivs);
            }
            Vec_WecPush( vPosEquivs, IndexCon, i );
            continue;
        }
        if ( nVars == 1 )
        {
            if ( IndexVar == -1 )
            {
                IndexVar = Vec_WecSize(vPosEquivs);
                Vec_WecPushLevel(vPosEquivs);
            }
            Vec_WecPush( vPosEquivs, IndexVar, i );
            continue;
        }
        uCanonPhase = Abc_TtCanonicize( pTruth, nVars, pCanonPerm );
        lastId = Vec_MemEntryNum( vTtMem[nVars] );
        truthId = Vec_MemHashInsert( vTtMem[nVars], pTruth );
        if ( lastId != Vec_MemEntryNum( vTtMem[nVars] ) ) // new one
        {
            assert( Vec_IntEntry(vTt2Class[nVars], truthId) == -1 );
            Vec_IntWriteEntry( vTt2Class[nVars], truthId, Vec_WecSize(vPosEquivs) );
            Vec_WecPushLevel(vPosEquivs);
        }
        assert( Vec_IntEntry(vTt2Class[nVars], truthId) >= 0 );
        Vec_WecPush( vPosEquivs, Vec_IntEntry(vTt2Class[nVars], truthId), i );
    }
    Gia_ObjComputeTruthTableStop( p );
    Vec_IntFree( vLeaves );
    for ( i = 0; i < 17; i++ )
    {
        Vec_MemHashFree( vTtMem[i] );
        Vec_MemFree( vTtMem[i] );
        Vec_IntFree( vTt2Class[i] );
    }

    // find the first outputs and derive GIA
    vFirsts = Vec_WecCollectFirsts( vPosEquivs );
    pNew = Gia_ManDupCones( p, Vec_IntArray(vFirsts), Vec_IntSize(vFirsts), 0 );
    Vec_IntFree( vFirsts );
    // report and return    
    if ( fVerbose )
    { 
        printf( "Nontrivial classes:\n" );
        Vec_WecPrint( vPosEquivs, 1 );
    }
    if ( pvPosEquivs )
        *pvPosEquivs = Vec_WecConvertToVecPtr( vPosEquivs );
    Vec_WecFree( vPosEquivs );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

