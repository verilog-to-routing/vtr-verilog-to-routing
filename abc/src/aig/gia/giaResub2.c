/**CFile****************************************************************

  FileName    [giaResub2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaResub2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/util/utilTruth.h"
#include "misc/vec/vecHsh.h"
#include "opt/dau/dau.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
typedef struct Gia_Rsb2Man_t_ Gia_Rsb2Man_t;
struct Gia_Rsb2Man_t_
{
    // hyper-parameters
    int            nDivsMax;
    int            nLevelIncrease;
    int            fUseXor;
    int            fUseZeroCost;
    int            fDebug;
    int            fVerbose;
    // input AIG
    int            nObjs;
    int            nPis;
    int            nNodes;
    int            nPos;
    int            iFirstPo;
    int            Level;
    int            nMffc;
    // intermediate data
    Vec_Int_t      vObjs;
    Vec_Wrd_t      vSims;
    Vec_Ptr_t      vpDivs;
    Vec_Int_t      vDivs;
    Vec_Int_t      vLevels;
    Vec_Int_t      vRefs;
    Vec_Int_t      vCopies;
    Vec_Int_t      vTried;
    word           Truth0;
    word           Truth1;
    word           CareSet;
};

extern void Abc_ResubPrepareManager( int nWords );
extern int Abc_ResubComputeFunction( void ** ppDivs, int nDivs, int nWords, int nLimit, int nDivsMax, int iChoice, int fUseXor, int fDebug, int fVerbose, int ** ppArray );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Rsb2Man_t * Gia_Rsb2ManAlloc()
{
    Gia_Rsb2Man_t * p = ABC_CALLOC( Gia_Rsb2Man_t, 1 );
    return p;
}
void Gia_Rsb2ManFree( Gia_Rsb2Man_t * p )
{
    Vec_IntErase( &p->vObjs   );
    Vec_WrdErase( &p->vSims   );
    Vec_PtrErase( &p->vpDivs  );
    Vec_IntErase( &p->vDivs   );
    Vec_IntErase( &p->vLevels );
    Vec_IntErase( &p->vRefs   );
    Vec_IntErase( &p->vCopies );
    Vec_IntErase( &p->vTried );
    ABC_FREE( p );
}
void Gia_Rsb2ManStart( Gia_Rsb2Man_t * p, int * pObjs, int nObjs, int nDivsMax, int nLevelIncrease, int fUseXor, int fUseZeroCost, int fDebug, int fVerbose )
{
    int i;
    // hyper-parameters
    p->nDivsMax       = nDivsMax; 
    p->nLevelIncrease = nLevelIncrease; 
    p->fUseXor        = fUseXor; 
    p->fUseZeroCost   = fUseZeroCost; 
    p->fDebug         = fDebug; 
    p->fVerbose       = fVerbose; 
    // user data
    Vec_IntClear( &p->vObjs );
    Vec_IntPushArray( &p->vObjs, pObjs, 2*nObjs );
    assert( pObjs[0] == 0 );
    assert( pObjs[1] == 0 );
    p->nObjs    = nObjs;
    p->nPis     = 0;
    p->nNodes   = 0;
    p->nPos     = 0;
    p->iFirstPo = 0;
    for ( i = 1; i < nObjs; i++ )
    {
        if ( pObjs[2*i+0] == 0 && pObjs[2*i+1] == 0 )
            p->nPis++;
        else if ( pObjs[2*i+0] == pObjs[2*i+1] )
            p->nPos++;
        else
            p->nNodes++;
    }
    assert( nObjs == 1 + p->nPis + p->nNodes + p->nPos );
    p->iFirstPo = nObjs - p->nPos;
    Vec_WrdClear( &p->vSims );
    Vec_WrdGrow( &p->vSims, 2*nObjs );
    Vec_WrdPush( &p->vSims, 0 );
    Vec_WrdPush( &p->vSims, 0 );
    for ( i = 0; i < p->nPis; i++ )
    {
        Vec_WrdPush( &p->vSims,  s_Truths6[i] );
        Vec_WrdPush( &p->vSims, ~s_Truths6[i] );
    }
    p->vSims.nSize = 2*p->nObjs;
    Vec_IntClear( &p->vDivs   );
    Vec_IntClear( &p->vLevels );
    Vec_IntClear( &p->vRefs   );
    Vec_IntClear( &p->vCopies );
    Vec_IntClear( &p->vTried  );
    Vec_PtrClear( &p->vpDivs  );
    Vec_IntGrow( &p->vDivs,   nObjs );
    Vec_IntGrow( &p->vLevels, nObjs );
    Vec_IntGrow( &p->vRefs,   nObjs );
    Vec_IntGrow( &p->vCopies, nObjs );
    Vec_IntGrow( &p->vTried,  nObjs );
    Vec_PtrGrow( &p->vpDivs,  nObjs );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_Rsb2ManPrint( Gia_Rsb2Man_t * p )
{
    int i, * pObjs = Vec_IntArray( &p->vObjs );
    printf( "PI = %d.  PO = %d.  Obj = %d.\n", p->nPis, p->nPos, p->nObjs );
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
        printf( "%2d = %c%2d & %c%2d;\n", i, 
            Abc_LitIsCompl(pObjs[2*i+0]) ? '!' : ' ', Abc_Lit2Var(pObjs[2*i+0]),
            Abc_LitIsCompl(pObjs[2*i+1]) ? '!' : ' ', Abc_Lit2Var(pObjs[2*i+1]) );
    for ( i = p->iFirstPo; i < p->nObjs; i++ )
        printf( "%2d = %c%2d;\n", i, 
            Abc_LitIsCompl(pObjs[2*i+0]) ? '!' : ' ', Abc_Lit2Var(pObjs[2*i+0]) );
}

int Gia_Rsb2ManLevel( Gia_Rsb2Man_t * p )
{
    int i, * pLevs, Level = 0;
    Vec_IntClear( &p->vLevels );
    Vec_IntGrow( &p->vLevels, p->nObjs );
    pLevs = Vec_IntArray( &p->vLevels );
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
        pLevs[i] = 1 + Abc_MaxInt( pLevs[2*i+0]/2, pLevs[2*i+1]/2 );
    for ( i = p->iFirstPo; i < p->nObjs; i++ )
        Level = Abc_MaxInt( Level, pLevs[i] = pLevs[2*i+0]/2 );
    return Level;
}
word Gia_Rsb2ManOdcs( Gia_Rsb2Man_t * p, int iNode )
{
    int i; word Res = 0;
    int  * pObjs = Vec_IntArray( &p->vObjs );
    word * pSims = Vec_WrdArray( &p->vSims );
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
    {
        if ( pObjs[2*i+0] < pObjs[2*i+1] )
            pSims[2*i+0] = pSims[pObjs[2*i+0]] & pSims[pObjs[2*i+1]];
        else if ( pObjs[2*i+0] > pObjs[2*i+1] )
            pSims[2*i+0] = pSims[pObjs[2*i+0]] ^ pSims[pObjs[2*i+1]];
        else assert( 0 );
        pSims[2*i+1] = ~pSims[2*i+0];        
    }
    for ( i = p->iFirstPo; i < p->nObjs; i++ )
        pSims[2*i+0] = pSims[pObjs[2*i+0]];
    ABC_SWAP( word, pSims[2*iNode+0], pSims[2*iNode+1] );
    for ( i = iNode + 1; i < p->iFirstPo; i++ )
    {
        if ( pObjs[2*i+0] < pObjs[2*i+1] )
            pSims[2*i+0] = pSims[pObjs[2*i+0]] & pSims[pObjs[2*i+1]];
        else if ( pObjs[2*i+0] < pObjs[2*i+1] )
            pSims[2*i+0] = pSims[pObjs[2*i+0]] ^ pSims[pObjs[2*i+1]];
        else assert( 0 );
        pSims[2*i+1] = ~pSims[2*i+0];        
    }
    for ( i = p->iFirstPo; i < p->nObjs; i++ )
        Res |= pSims[2*i+0] ^ pSims[pObjs[2*i+0]];
    ABC_SWAP( word, pSims[2*iNode+0], pSims[2*iNode+1] );
    return Res;
}
// marks MFFC and returns its size
int Gia_Rsb2ManDeref_rec( Gia_Rsb2Man_t * p, int * pObjs, int * pRefs, int iNode )
{
    int Counter = 1;
    if ( iNode <= p->nPis )
        return 0;
    if ( --pRefs[Abc_Lit2Var(pObjs[2*iNode+0])] == 0 )
        Counter += Gia_Rsb2ManDeref_rec( p, pObjs, pRefs, Abc_Lit2Var(pObjs[2*iNode+0]) );
    if ( --pRefs[Abc_Lit2Var(pObjs[2*iNode+1])] == 0 )
        Counter += Gia_Rsb2ManDeref_rec( p, pObjs, pRefs, Abc_Lit2Var(pObjs[2*iNode+1]) );
    return Counter;
}
int Gia_Rsb2ManMffc( Gia_Rsb2Man_t * p, int iNode )
{
    int i, * pRefs, * pObjs;
    Vec_IntFill( &p->vRefs, p->nObjs, 0 );
    pRefs = Vec_IntArray( &p->vRefs );
    pObjs = Vec_IntArray( &p->vObjs );
    assert( pObjs[2*iNode+0] != pObjs[2*iNode+1] );
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
        pRefs[Abc_Lit2Var(pObjs[2*i+0])]++, 
        pRefs[Abc_Lit2Var(pObjs[2*i+1])]++;
    for ( i = p->iFirstPo; i < p->nObjs; i++ )
        pRefs[Abc_Lit2Var(pObjs[2*i+0])]++;
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
        assert( pRefs[i] );
    pRefs[iNode] = 0;
    for ( i = iNode + 1; i < p->iFirstPo; i++ )
        if ( !pRefs[Abc_Lit2Var(pObjs[2*i+0])] || !pRefs[Abc_Lit2Var(pObjs[2*i+1])] )
            pRefs[i] = 0;
    return Gia_Rsb2ManDeref_rec( p, pObjs, pRefs, iNode );
}
// collects divisors and maps them into nodes
// assumes MFFC is already marked
int Gia_Rsb2ManDivs( Gia_Rsb2Man_t * p, int iNode )
{
    int i, iNodeLevel = 0;
    int * pRefs = Vec_IntArray( &p->vRefs );
    p->CareSet = Gia_Rsb2ManOdcs( p, iNode );
    p->Truth1 = p->CareSet & Vec_WrdEntry(&p->vSims, 2*iNode);
    p->Truth0 = p->CareSet & ~p->Truth1;
    Vec_PtrClear( &p->vpDivs );
    Vec_PtrPush( &p->vpDivs, &p->Truth0 );
    Vec_PtrPush( &p->vpDivs, &p->Truth1 );
    Vec_IntClear( &p->vDivs );
    Vec_IntPushTwo( &p->vDivs, -1, -1 );
    for ( i = 1; i <= p->nPis; i++ )
    {
        Vec_PtrPush( &p->vpDivs, Vec_WrdEntryP(&p->vSims, 2*i) );
        Vec_IntPush( &p->vDivs, i );
    }
    p->nMffc = Gia_Rsb2ManMffc( p, iNode );
    if ( p->nLevelIncrease >= 0 )
    {
        p->Level = Gia_Rsb2ManLevel(p);
        iNodeLevel = Vec_IntEntry(&p->vLevels, iNode);
    }
    for ( i = p->nPis + 1; i < p->iFirstPo; i++ )
    {
        if ( !pRefs[i] || (p->nLevelIncrease >= 0 && Vec_IntEntry(&p->vLevels, i) > iNodeLevel + p->nLevelIncrease) )
            continue;
        Vec_PtrPush( &p->vpDivs, Vec_WrdEntryP(&p->vSims, 2*i) );
        Vec_IntPush( &p->vDivs, i );
    }
    assert( Vec_IntSize(&p->vDivs) == Vec_PtrSize(&p->vpDivs) );
    return Vec_IntSize(&p->vDivs);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_Rsb2AddNode( Vec_Int_t * vRes, int iLit0, int iLit1, int iRes0, int iRes1 )
{
    int iLitMin = iRes0 < iRes1 ? Abc_LitNotCond(iRes0, Abc_LitIsCompl(iLit0)) : Abc_LitNotCond(iRes1, Abc_LitIsCompl(iLit1));
    int iLitMax = iRes0 < iRes1 ? Abc_LitNotCond(iRes1, Abc_LitIsCompl(iLit1)) : Abc_LitNotCond(iRes0, Abc_LitIsCompl(iLit0));
    int iLitRes = Vec_IntSize(vRes);
    if ( iLit0 < iLit1 ) // and
    {
        if ( iLitMin == 0 )
            return 0;
        if ( iLitMin == 1 )
            return iLitMax;
        if ( iLitMin == Abc_LitNot(iLitMax) )
            return 0;
    }
    else if ( iLit0 > iLit1 ) // xor
    {
        if ( iLitMin == 0 )
            return iLitMax;
        if ( iLitMin == 1 )
            return Abc_LitNot(iLitMax);
        if ( iLitMin == Abc_LitNot(iLitMax) )
            return 1;
    }
    else assert( 0 );
    assert( iLitMin >= 2 && iLitMax >= 2 );
    if ( iLit0 < iLit1 ) // and
        Vec_IntPushTwo( vRes, iLitMin, iLitMax );
    else if ( iLit0 > iLit1 ) // xor
    {
        assert( !Abc_LitIsCompl(iLit0) );
        assert( !Abc_LitIsCompl(iLit1) );
        Vec_IntPushTwo( vRes, iLitMax, iLitMin );
    }
    else assert( 0 );
    return iLitRes;
}
int Gia_Rsb2ManInsert_rec( Vec_Int_t * vRes, int nPis, Vec_Int_t * vObjs, int iNode, Vec_Int_t * vResub, Vec_Int_t * vDivs, Vec_Int_t * vCopies, int iObj )
{
    if ( Vec_IntEntry(vCopies, iObj) >= 0 )
        return Vec_IntEntry(vCopies, iObj);
    assert( iObj > nPis );
    if ( iObj == iNode )
    {
        int nVars = Vec_IntSize(vDivs);
        int iLitRes = -1, iTopLit = Vec_IntEntryLast( vResub );
        if ( Abc_Lit2Var(iTopLit) == 0 )
            iLitRes = 0;
        else if ( Abc_Lit2Var(iTopLit) < nVars )
            iLitRes = Gia_Rsb2ManInsert_rec( vRes, nPis, vObjs, -1, vResub, vDivs, vCopies, Vec_IntEntry(vDivs, Abc_Lit2Var(iTopLit)) );
        else
        {
            Vec_Int_t * vCopy = Vec_IntAlloc( 10 );
            int k, iLit, iLit0, iLit1;
            Vec_IntForEachEntryStop( vResub, iLit, k, Vec_IntSize(vResub)-1 )
                if ( Abc_Lit2Var(iLit) < nVars )
                    Gia_Rsb2ManInsert_rec( vRes, nPis, vObjs, -1, vResub, vDivs, vCopies, Vec_IntEntry(vDivs, Abc_Lit2Var(iLit)) );
            Vec_IntForEachEntryDouble( vResub, iLit0, iLit1, k )
            {
                int iVar0 = Abc_Lit2Var(iLit0);
                int iVar1 = Abc_Lit2Var(iLit1);
                int iRes0 = iVar0 < nVars ? Vec_IntEntry(vCopies, Vec_IntEntry(vDivs, iVar0)) : Vec_IntEntry(vCopy, iVar0 - nVars);
                int iRes1 = iVar1 < nVars ? Vec_IntEntry(vCopies, Vec_IntEntry(vDivs, iVar1)) : Vec_IntEntry(vCopy, iVar1 - nVars);
                iLitRes   = Gia_Rsb2AddNode( vRes, iLit0, iLit1, iRes0, iRes1 );
                Vec_IntPush( vCopy, iLitRes );
            }
            Vec_IntFree( vCopy );
        }
        iLitRes = Abc_LitNotCond( iLitRes, Abc_LitIsCompl(iTopLit) );
        Vec_IntWriteEntry( vCopies, iObj, iLitRes );
        return iLitRes;
    }
    else
    {
        int iLit0 = Vec_IntEntry( vObjs, 2*iObj+0 );
        int iLit1 = Vec_IntEntry( vObjs, 2*iObj+1 );
        int iRes0 = Gia_Rsb2ManInsert_rec( vRes, nPis, vObjs, iNode, vResub, vDivs, vCopies, Abc_Lit2Var(iLit0) );
        int iRes1 = Gia_Rsb2ManInsert_rec( vRes, nPis, vObjs, iNode, vResub, vDivs, vCopies, Abc_Lit2Var(iLit1) );
        int iLitRes = Gia_Rsb2AddNode( vRes, iLit0, iLit1, iRes0, iRes1 );
        Vec_IntWriteEntry( vCopies, iObj, iLitRes );
        return iLitRes;
    }
}
Vec_Int_t * Gia_Rsb2ManInsert( int nPis, int nPos, Vec_Int_t * vObjs, int iNode, Vec_Int_t * vResub, Vec_Int_t * vDivs, Vec_Int_t * vCopies )
{
    int i, nObjs = Vec_IntSize(vObjs)/2, iFirstPo = nObjs - nPos;
    Vec_Int_t * vRes = Vec_IntAlloc( Vec_IntSize(vObjs) );
//Vec_IntPrint( vDivs );
//Vec_IntPrint( vResub );
    Vec_IntFill( vCopies, Vec_IntSize(vObjs), -1 );
    Vec_IntFill( vRes, 2*(nPis + 1), 0 );
    for ( i = 0; i <= nPis; i++ )
        Vec_IntWriteEntry( vCopies, i, 2*i );
    for ( i = iFirstPo; i < nObjs; i++ )
        Gia_Rsb2ManInsert_rec( vRes, nPis, vObjs, iNode, vResub, vDivs, vCopies, Abc_Lit2Var( Vec_IntEntry(vObjs, 2*i) ) );
    for ( i = iFirstPo; i < nObjs; i++ )
    {
        int iLitNew = Abc_Lit2LitL( Vec_IntArray(vCopies), Vec_IntEntry(vObjs, 2*i) );
        Vec_IntPushTwo( vRes, iLitNew, iLitNew );
    }
    return vRes;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ResubPrintDivs( void ** ppDivs, int nDivs )
{
    word ** pDivs = (word **)ppDivs;
    int i;
    for ( i = 0; i < nDivs; i++ )
    {
        printf( "Div %2d : ", i );
        Dau_DsdPrintFromTruth( pDivs[i], 6 );
    }
}
int Abc_ResubNodeToTry( Vec_Int_t * vTried, int iFirst, int iLast )
{
    int iNode;
    //for ( iNode = iFirst; iNode < iLast; iNode++ )
    for ( iNode = iLast - 1; iNode >= iFirst; iNode-- )
        if ( Vec_IntFind(vTried, iNode) == -1 )
            return iNode;
    return -1;
}
int Abc_ResubComputeWindow( int * pObjs, int nObjs, int nDivsMax, int nLevelIncrease, int fUseXor, int fUseZeroCost, int fDebug, int fVerbose, int ** ppArray, int * pnResubs )
{
    int iNode, nChanges = 0, RetValue = 0;
    Gia_Rsb2Man_t * p = Gia_Rsb2ManAlloc(); 
    Gia_Rsb2ManStart( p, pObjs, nObjs, nDivsMax, nLevelIncrease, fUseXor, fUseZeroCost, fDebug, fVerbose );
    *ppArray = NULL;
    while ( (iNode = Abc_ResubNodeToTry(&p->vTried, p->nPis+1, p->iFirstPo)) > 0 )
    {
        int nDivs = Gia_Rsb2ManDivs( p, iNode );
        int * pResub, nResub = Abc_ResubComputeFunction( Vec_PtrArray(&p->vpDivs), nDivs, 1, p->nMffc-1, nDivsMax, 0, fUseXor, fDebug, fVerbose, &pResub );
        if ( nResub == 0 )
            Vec_IntPush( &p->vTried, iNode );
        else
        {
            int i, k = 0, iTried;
            Vec_Int_t vResub = { nResub, nResub, pResub };
            Vec_Int_t * vRes = Gia_Rsb2ManInsert( p->nPis, p->nPos, &p->vObjs, iNode, &vResub, &p->vDivs, &p->vCopies );
            //printf( "\nResubing node %d:\n", iNode );
            //Gia_Rsb2ManPrint( p );
            p->nObjs    = Vec_IntSize(vRes)/2;
            p->iFirstPo = p->nObjs - p->nPos;
            Vec_IntClear( &p->vObjs );
            Vec_IntAppend( &p->vObjs, vRes );
            Vec_IntFree( vRes );
            Vec_IntForEachEntry( &p->vTried, iTried, i )
                if ( Vec_IntEntry(&p->vCopies, iTried) > Abc_Var2Lit(p->nPis, 0) ) // internal node
                    Vec_IntWriteEntry( &p->vTried, k++, Abc_Lit2Var(Vec_IntEntry(&p->vCopies, iTried)) );
            Vec_IntShrink( &p->vTried, k );
            nChanges++;
            //Gia_Rsb2ManPrint( p );
        }
    }
    if ( nChanges )
    {
        RetValue = p->nObjs;
        *ppArray = p->vObjs.pArray;
        Vec_IntZero( &p->vObjs );
    }
    Gia_Rsb2ManFree( p );
    if ( pnResubs )
        *pnResubs = nChanges;
    return RetValue;
}
int Abc_ResubComputeWindow2( int * pObjs, int nObjs, int nDivsMax, int nLevelIncrease, int fUseXor, int fUseZeroCost, int fDebug, int fVerbose, int ** ppArray, int * pnResubs )
{
    *ppArray = ABC_ALLOC( int, 2*nObjs );
    memmove( *ppArray, pObjs, 2*nObjs * sizeof(int) );
    if ( pnResubs )
        *pnResubs = 0;
    return nObjs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Gia_ManToResub( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, * pObjs = ABC_CALLOC( int, 2*Gia_ManObjNum(p) );
    assert( Gia_ManIsNormalized(p) );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
            continue;
        pObjs[2*i+0] = Gia_ObjFaninLit0(Gia_ManObj(p, i), i);
        if ( Gia_ObjIsCo(pObj) )
            pObjs[2*i+1] = pObjs[2*i+0];
        else if ( Gia_ObjIsAnd(pObj) )
            pObjs[2*i+1] = Gia_ObjFaninLit1(Gia_ManObj(p, i), i);
        else assert( 0 );
    }
    return pObjs;
}
Gia_Man_t * Gia_ManFromResub( int * pObjs, int nObjs, int nIns )
{
    Gia_Man_t * pNew = Gia_ManStart( nObjs );
    int i;
    for ( i = 1; i < nObjs; i++ )
    {
        if ( pObjs[2*i] == 0 && i <= nIns ) // pi
            Gia_ManAppendCi( pNew );
        else if ( pObjs[2*i] == pObjs[2*i+1] ) // po
            Gia_ManAppendCo( pNew, pObjs[2*i] );
        else if ( pObjs[2*i] < pObjs[2*i+1] )
            Gia_ManAppendAnd( pNew, pObjs[2*i], pObjs[2*i+1] );
        else if ( pObjs[2*i] > pObjs[2*i+1] )
            Gia_ManAppendXor( pNew, pObjs[2*i], pObjs[2*i+1] );
        else assert( 0 );
    }
    return pNew;
}
Gia_Man_t * Gia_ManResub2Test( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    int nResubs, nObjsNew, * pObjsNew, * pObjs = Gia_ManToResub( p );
//Gia_ManPrint( p );
    Abc_ResubPrepareManager( 1 );
    nObjsNew = Abc_ResubComputeWindow( pObjs, Gia_ManObjNum(p), 1000, -1, 0, 0, 0, 0, &pObjsNew, &nResubs );
    //printf( "Performed resub %d times.  Reduced %d nodes.\n", nResubs, nObjsNew ? Gia_ManObjNum(p) - nObjsNew : 0 );
    Abc_ResubPrepareManager( 0 );
    if ( nObjsNew )
    {
        pNew = Gia_ManFromResub( pObjsNew, nObjsNew, Gia_ManCiNum(p) );
        pNew->pName = Abc_UtilStrsav( p->pName );
    }
    else 
        pNew = Gia_ManDup( p );
    ABC_FREE( pObjs );
    ABC_FREE( pObjsNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Creating a window with support composed of primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// returns the number of nodes added to the window when is iPivot is added
// the window nodes in vNodes are labeled with the current traversal ID
// the new node iPivot and its fanout are temporarily labeled and then unlabeled
int Gia_WinTryAddingNode( Gia_Man_t * p, int iPivot, int iPivot2, Vec_Wec_t * vLevels, Vec_Int_t * vNodes )
{
    Vec_Int_t * vLevel; 
    Gia_Obj_t * pObj, * pFanout;
    int k, i, f, Count = 0;
    // precondition:  the levelized structure is empty
    assert( Vec_WecSizeSize(vLevels) == 0 );
    // precondition:  the new object to be added (iPivot) is not in the window
    assert( !Gia_ObjIsTravIdCurrentId(p, iPivot) );
    // add the object to the window and to the levelized structure
    Gia_ObjSetTravIdCurrentId( p, iPivot );
    Vec_WecPush( vLevels, Gia_ObjLevelId(p, iPivot), iPivot );
    // the same about the second node if it is given
    if ( iPivot2 != -1 )
    {
        // precondition:  the new object to be added (iPivot2) is not in the window
        assert( !Gia_ObjIsTravIdCurrentId(p, iPivot2) );
        // add the object to the window and to the levelized structure
        Gia_ObjSetTravIdCurrentId( p, iPivot2 );
        Vec_WecPush( vLevels, Gia_ObjLevelId(p, iPivot2), iPivot2 );
    }
    // iterate through all objects and explore their fanouts
    Vec_WecForEachLevel( vLevels, vLevel, k )
        Gia_ManForEachObjVec( vLevel, p, pObj, i )
            Gia_ObjForEachFanoutStatic( p, pObj, pFanout, f )
            {
                if ( f == 5 ) // explore first 5 fanouts of the node
                    break;
                if ( Gia_ObjIsAnd(pFanout) && // internal node
                    !Gia_ObjIsTravIdCurrent(p, pFanout) && // not in the window
                     Gia_ObjIsTravIdCurrent(p, Gia_ObjFanin0(pFanout)) && // but fanins are
                     Gia_ObjIsTravIdCurrent(p, Gia_ObjFanin1(pFanout)) )  // in the window
                {
                    // add fanout to the window and to the levelized structure
                    Gia_ObjSetTravIdCurrent( p, pFanout );
                    Vec_WecPush( vLevels, Gia_ObjLevel(p, pFanout), Gia_ObjId(p, pFanout) );
                    // count the number of nodes in the structure
                    Count++;
                }
            }
    // iterate through the nodes in the levelized structure
    Vec_WecForEachLevel( vLevels, vLevel, k )
    {
        Gia_ManForEachObjVec( vLevel, p, pObj, i )
            if ( vNodes == NULL ) // it was a test run - unmark the node
                Gia_ObjSetTravIdPrevious( p, pObj );
            else // it was a real run - permanently add to the node to the window
                Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
        // clean the levelized structure
        Vec_IntClear( vLevel );
    }
    // return the number of nodes to be added
    return Count;
}
// find the first PI to add based on the fanout count
int Gia_WinAddCiWithMaxFanouts( Gia_Man_t * p )
{
    int i, Id, nMaxFan = -1, iMaxFan = -1;
    Gia_ManForEachCiId( p, Id, i )
        if ( nMaxFan < Gia_ObjFanoutNumId(p, Id) )
        {
            nMaxFan = Gia_ObjFanoutNumId(p, Id);
            iMaxFan = Id;
        }
    assert( iMaxFan >= 0 );
    return iMaxFan;
}
// find the next PI to add based on how many nodes will be added to the window
int Gia_WinAddCiWithMaxDivisors( Gia_Man_t * p, Vec_Wec_t * vLevels )
{
    int i, Id, nCurFan, nMaxFan = -1, iMaxFan = -1;
    Gia_ManForEachCiId( p, Id, i )
    {
        if ( Gia_ObjIsTravIdCurrentId( p, Id ) )
            continue;
        nCurFan = Gia_WinTryAddingNode( p, Id, -1, vLevels, NULL );
        if ( nMaxFan < nCurFan )
        {
            nMaxFan = nCurFan;
            iMaxFan = Id;
        }
    }
    assert( iMaxFan >= 0 );
    return iMaxFan;
}
// check if the node has unmarked fanouts
int Gia_WinNodeHasUnmarkedFanouts( Gia_Man_t * p, int iPivot )
{
    int f, iFan;
    Gia_ObjForEachFanoutStaticId( p, iPivot, iFan, f )
        if ( !Gia_ObjIsTravIdCurrentId(p, iFan) )
            return 1;
    return 0;
}
// this is a translation procedure, which converts the array of node IDs (vObjs) 
// into the internal represnetation for the resub engine, which is returned
// (this procedure is not needed when we simply construct a window)
Vec_Int_t * Gia_RsbCiTranslate( Gia_Man_t * p, Vec_Int_t * vObjs, Vec_Int_t * vMap )
{
    int i, iObj, Lit0, Lit1, Fan0, Fan1;
    Vec_Int_t * vNodes = Vec_IntAlloc( 100 );
    assert( Vec_IntSize(vMap) == Gia_ManObjNum(p) );
    Vec_IntPushTwo( vNodes, 0, 0 ); // const0 node
    Vec_IntForEachEntry( vObjs, iObj, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj(p, iObj);
        assert( Gia_ObjIsTravIdCurrentId(p, iObj) );
        Fan0 = Gia_ObjIsCi(pObj) ? 0 : Vec_IntEntry(vMap, Gia_ObjFaninId0(pObj, iObj));
        Fan1 = Gia_ObjIsCi(pObj) ? 0 : Vec_IntEntry(vMap, Gia_ObjFaninId1(pObj, iObj));
        Lit0 = Gia_ObjIsCi(pObj) ? 0 : Abc_LitNotCond( Fan0, Gia_ObjFaninC0(pObj) );
        Lit1 = Gia_ObjIsCi(pObj) ? 0 : Abc_LitNotCond( Fan1, Gia_ObjFaninC1(pObj) );
        Vec_IntWriteEntry( vMap, iObj, Vec_IntSize(vNodes) );
        Vec_IntPushTwo( vNodes, Lit0, Lit1 );
    }
    Vec_IntForEachEntry( vObjs, iObj, i )
        if ( Gia_WinNodeHasUnmarkedFanouts( p, iObj ) )
            Vec_IntPushTwo( vNodes, Vec_IntEntry(vMap, iObj), Vec_IntEntry(vMap, iObj) );
    return vNodes;
}
// construct a high-volume window support by the given number (nPis) of primary inputs
Vec_Int_t * Gia_RsbCiWindow( Gia_Man_t * p, int nPis )
{
    Vec_Int_t * vRes;  int i, iMaxFan;
    Vec_Int_t * vNodes  = Vec_IntAlloc( 100 );
    Vec_Int_t * vMap    = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_Wec_t * vLevels = Vec_WecStart( Gia_ManLevelNum(p)+1 );
    Gia_ManStaticFanoutStart( p );
    Gia_ManIncrementTravId(p);
    // add the first one
    iMaxFan = Gia_WinAddCiWithMaxFanouts( p );
    Gia_ObjSetTravIdCurrentId( p, iMaxFan );
    Vec_IntPush( vNodes, iMaxFan );
    // add remaining ones
    for ( i = 1; i < nPis; i++ )
    {
        iMaxFan = Gia_WinAddCiWithMaxDivisors( p, vLevels );
        Gia_WinTryAddingNode( p, iMaxFan, -1, vLevels, vNodes );
    }
    Vec_IntSort( vNodes, 0 );
    vRes = Gia_RsbCiTranslate( p, vNodes, vMap );
    Gia_ManStaticFanoutStop( p );
    Vec_WecFree( vLevels );
    Vec_IntFree( vMap );
//Vec_IntPrint( vNodes );
    Vec_IntFree( vNodes );
    return vRes;
}
void Gia_RsbCiWindowTest( Gia_Man_t * p )
{
    Vec_Int_t * vWin = Gia_RsbCiWindow( p, 6 );
    //Vec_IntPrint( vWin );
    Vec_IntFree( vWin );
}




/**Function*************************************************************

  Synopsis    [Return initial window for the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
//static inline int  Gia_ObjFaninId( Gia_Obj_t * pObj, int iObj, int n )          { return n ? Gia_ObjFaninId1(pObj, iObj) : Gia_ObjFaninId0(pObj, iObj);  }

static inline int  Gia_ObjTravIsTopTwo( Gia_Man_t * p, int iNodeA )            { return (p->pTravIds[iNodeA] >= p->nTravIds - 1);       }
static inline int  Gia_ObjTravIsSame( Gia_Man_t * p, int iNodeA, int iNodeB )  { return (p->pTravIds[iNodeA] == p->pTravIds[iNodeB]);   }
static inline void Gia_ObjTravSetSame( Gia_Man_t * p, int iNodeA, int iNodeB ) { p->pTravIds[iNodeA] = p->pTravIds[iNodeB];             }

// collect nodes on the path from the meeting point to the root node, excluding the meeting point
void Gia_RsbWindowGather( Gia_Man_t * p, Vec_Int_t * vPaths, int iNode, Vec_Int_t * vVisited )
{
    int iPrev;
    if ( iNode == 0 )
        return;
    Vec_IntPush( vVisited, iNode );
    iPrev = Vec_IntEntry( vPaths, iNode );
    if ( iPrev == 0 )
        return;
    assert( Gia_ObjTravIsSame(p, iPrev, iNode) );
    Gia_RsbWindowGather( p, vPaths, iPrev, vVisited );
}
// explore the frontier of nodes in the breadth-first traversal
int Gia_RsbWindowExplore( Gia_Man_t * p, Vec_Int_t * vVisited, int iStart, Vec_Int_t * vPaths, int * piMeet, int * piNode )
{
    int i, n, iObj, iLimit = Vec_IntSize( vVisited );
    *piMeet = *piNode = 0;
    Vec_IntForEachEntryStartStop( vVisited, iObj, i, iStart, iLimit )
    {
        Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
        if ( !Gia_ObjIsAnd(pObj) )
            continue;
        for ( n = 0; n < 2; n++ )
        {
            int iFan = Gia_ObjFaninId( pObj, iObj, n );
            // if the node was visited on the paths to both fanins, collect it
            if ( Gia_ObjTravIsTopTwo(p, iObj) && Gia_ObjTravIsTopTwo(p, iFan) && !Gia_ObjTravIsSame(p, iObj, iFan) )
            {
                *piMeet = iFan;
                *piNode = iObj;
                return 1;
            }
            // if the node was visited already on this path, skip it
            if ( Gia_ObjTravIsTopTwo(p, iFan) )
            {
                assert( Gia_ObjTravIsSame(p, iObj, iFan) );
                continue;
            }
            // label the node as visited
            Gia_ObjTravSetSame( p, iFan, iObj );
            Vec_IntWriteEntry( vPaths, iFan, iObj ); 
            Vec_IntPush( vVisited, iFan );
        }
    }
    return 0;
}
Vec_Int_t * Gia_RsbWindowInit( Gia_Man_t * p, Vec_Int_t * vPaths, int iPivot, int nIter )
{
    Vec_Int_t * vVisited = Vec_IntAlloc( 100 );
    Gia_Obj_t * pPivot = Gia_ManObj( p, iPivot ); 
    int i, n, iStart = 0;
    assert( Gia_ObjIsAnd(pPivot) );
    // start paths for both fanins of the pivot node
    for ( n = 0; n < 2; n++ )
    {
        int iFan = Gia_ObjFaninId( pPivot, iPivot, n );
        Gia_ManIncrementTravId(p);
        Vec_IntPush( vVisited, iFan );
        Vec_IntWriteEntry( vPaths, iFan, 0 ); 
        Gia_ObjSetTravIdCurrentId( p, iFan );
    }
    // perform several iterations of breadth-first search
    for ( i = 0; i < nIter; i++ )
    {
        int iMeet, iNode, iNext = Vec_IntSize(vVisited);
        if ( Gia_RsbWindowExplore( p, vVisited, iStart, vPaths, &iMeet, &iNode ) )
        {
            // found the shared path
            if ( Gia_ObjIsTravIdCurrentId(p, iMeet) )
                assert( Gia_ObjIsTravIdPreviousId(p, iNode) );
            else if ( Gia_ObjIsTravIdPreviousId(p, iMeet) )
                assert( Gia_ObjIsTravIdCurrentId(p, iNode) );
            else assert( 0 );
            // collect the initial window
            Vec_IntClear( vVisited );
            Gia_RsbWindowGather( p, vPaths, Vec_IntEntry(vPaths, iMeet), vVisited );
            Gia_RsbWindowGather( p, vPaths, iNode, vVisited );
            Vec_IntPush( vVisited, iPivot );
            break;
        }
        iStart = iNext;
    }
    // if no meeting point is found, make sure to return NULL
    if ( i == nIter )
        Vec_IntFreeP( &vVisited );
    return vVisited;
}
Vec_Int_t * Gia_RsbCreateWindowInputs( Gia_Man_t * p, Vec_Int_t * vWin )
{
    Vec_Int_t * vInputs = Vec_IntAlloc(10);
    Gia_Obj_t * pObj; int i, n, iObj;
    Gia_ManIncrementTravId(p);
    Gia_ManForEachObjVec( vWin, p, pObj, i )
        Gia_ObjSetTravIdCurrent(p, pObj);
    Gia_ManForEachObjVec( vWin, p, pObj, i )
    {
        assert( Gia_ObjIsAnd(pObj) );
        for ( n = 0; n < 2; n++ )
        {
            int iFan = n ? Gia_ObjFaninId1p(p, pObj) : Gia_ObjFaninId0p(p, pObj);
            if ( !Gia_ObjIsTravIdCurrentId(p, iFan) )
                Vec_IntPushUnique( vInputs, iFan );
        }
    }
    Vec_IntForEachEntry( vInputs, iObj, i )
    {
        Gia_ObjSetTravIdCurrentId( p, iObj );
        Vec_IntPush( vWin, iObj );
    }
    return vInputs;
}

/**Function*************************************************************

  Synopsis    [Grow window for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_RsbAddSideInputs( Gia_Man_t * p, Vec_Wec_t * vLevels, Vec_Int_t * vWin )
{
    Vec_Int_t * vLevel; 
    Gia_Obj_t * pObj, * pFanout;
    int k, i, f, iObj;
    // precondition: the levelized structure is empty
    assert( Vec_WecSizeSize(vLevels) == 0 );
    // precondition: window nodes are labeled with the current ID
    Vec_IntForEachEntry( vWin, iObj, i )
    {
        assert( Gia_ObjIsTravIdCurrentId(p, iObj) );
        Vec_WecPush( vLevels, Gia_ObjLevelId(p, iObj), iObj );
    }
    // iterate through all objects and explore their fanouts
    Vec_WecForEachLevel( vLevels, vLevel, k )
        Gia_ManForEachObjVec( vLevel, p, pObj, i )
            Gia_ObjForEachFanoutStatic( p, pObj, pFanout, f )
            {
                if ( f == 5 ) // explore first 5 fanouts of the node
                    break;
                if ( Gia_ObjIsAnd(pFanout) && // internal node
                    !Gia_ObjIsTravIdCurrent(p, pFanout) && // not in the window
                     Gia_ObjIsTravIdCurrent(p, Gia_ObjFanin0(pFanout)) && // but fanins are
                     Gia_ObjIsTravIdCurrent(p, Gia_ObjFanin1(pFanout)) )  // in the window
                {
                    // add fanout to the window and to the levelized structure
                    Gia_ObjSetTravIdCurrent( p, pFanout );
                    Vec_WecPush( vLevels, Gia_ObjLevel(p, pFanout), Gia_ObjId(p, pFanout) );
                    Vec_IntPush( vWin, Gia_ObjId(p, pFanout) );
                }
            }
    // iterate through the nodes in the levelized structure
    Vec_WecForEachLevel( vLevels, vLevel, k )
        Vec_IntClear( vLevel );
}
// expland inputs until saturation while adding the side-fanouts
void Gia_RsbExpandInputs( Gia_Man_t * p, Vec_Wec_t * vLevels, Vec_Int_t * vWin, Vec_Int_t * vInputs )
{
    Gia_Obj_t * pObj; 
    int i, n, iFans[2], fChange = 1;
    while ( fChange )
    {
        fChange = 0;
        Gia_ManForEachObjVec( vInputs, p, pObj, i )
        {
            if ( !Gia_ObjIsAnd(pObj) )
                continue;
            iFans[0] = Gia_ObjFaninId0p(p, pObj);
            iFans[1] = Gia_ObjFaninId1p(p, pObj);
            if ( !Gia_ObjIsTravIdCurrentId(p, iFans[0]) && !Gia_ObjIsTravIdCurrentId(p, iFans[1]) )
                continue;
            Vec_IntRemove( vInputs, Gia_ObjId(p, pObj) );
            assert( Vec_IntFind(vWin, Gia_ObjId(p, pObj)) >= 0 );
            for ( n = 0; n < 2; n++ )
            {
                if ( Gia_ObjIsTravIdCurrentId(p, iFans[n]) )
                    continue;
                assert( Vec_IntFind(vInputs, iFans[n]) == -1 );
                Vec_IntPush( vInputs, iFans[n] );
                Gia_WinTryAddingNode( p, iFans[n], -1, vLevels, vWin );
                assert( Gia_ObjIsTravIdCurrentId(p, iFans[n]) );
            }
            fChange = 1;
        }
    }
}
// select the best input to expand, based on its contribution to the window size
int Gia_RsbSelectOneInput( Gia_Man_t * p, Vec_Wec_t * vLevels, Vec_Int_t * vIns )
{
    int i, iNode = 0, WeightThis, WeightBest = -1;
    Gia_Obj_t * pObj; 
    Gia_ManForEachObjVec( vIns, p, pObj, i )
        if ( Gia_ObjIsAnd(pObj) )
        {
            int iFan0 = Gia_ObjFaninId0p(p, pObj);
            int iFan1 = Gia_ObjFaninId1p(p, pObj);
            assert( !Gia_ObjIsTravIdCurrentId(p, iFan0) && !Gia_ObjIsTravIdCurrentId(p, iFan1) );
            WeightThis = Gia_WinTryAddingNode( p, iFan0, iFan1, vLevels, NULL );
            if ( WeightBest < WeightThis )
            {
                WeightBest = WeightThis;
                iNode = Gia_ObjId(p, pObj);
            }
        }
    return iNode;
}
// grow the initial window as long as it fits the input count limit
void Gia_RsbWindowGrow( Gia_Man_t * p, Vec_Wec_t * vLevels, Vec_Int_t * vWin, Vec_Int_t * vIns, int nInputsMax )
{
    int iNode;
    Gia_RsbAddSideInputs( p, vLevels, vWin );
    Gia_RsbExpandInputs( p, vLevels, vWin, vIns );
    while ( Vec_IntSize(vIns) < nInputsMax && (iNode = Gia_RsbSelectOneInput(p, vLevels, vIns)) )
    {
        int iFan0 = Gia_ObjFaninId0p(p, Gia_ManObj(p, iNode));
        int iFan1 = Gia_ObjFaninId1p(p, Gia_ManObj(p, iNode));
        assert( !Gia_ObjIsTravIdCurrentId(p, iFan0) && !Gia_ObjIsTravIdCurrentId(p, iFan1) );
        Gia_WinTryAddingNode( p, iFan0, iFan1, vLevels, vWin );
        assert(  Gia_ObjIsTravIdCurrentId(p, iFan0) &&  Gia_ObjIsTravIdCurrentId(p, iFan1) );
        Vec_IntRemove( vIns, iNode );
        Vec_IntPushTwo( vIns, iFan0, iFan1 );
        Gia_RsbExpandInputs( p, vLevels, vWin, vIns );
    }
}

/**Function*************************************************************

  Synopsis    [Grow window for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_WinCreateFromCut_rec( Gia_Man_t * p, int iObj, Vec_Int_t * vWin )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    pObj = Gia_ManObj( p, iObj );
    assert( Gia_ObjIsAnd(pObj) );
    Gia_WinCreateFromCut_rec( p, Gia_ObjFaninId0(pObj, iObj), vWin );
    Gia_WinCreateFromCut_rec( p, Gia_ObjFaninId1(pObj, iObj), vWin );
    Vec_IntPush( vWin, iObj );
}
// uses levelized structure (vLevels) to collect in array vWin divisors supported by the cut (vIn)
void Gia_WinCreateFromCut( Gia_Man_t * p, int iPivot, Vec_Int_t * vIn, Vec_Wec_t * vLevels, Vec_Int_t * vWin )
{
    Vec_Int_t * vLevel; 
    Gia_Obj_t * pObj, * pFanout;
    int k, i, f, iObj, Level;
    Vec_Int_t * vUsed = Vec_IntAlloc( 100 );
    // precondition:  the levelized structure is empty
    assert( Vec_WecSizeSize(vLevels) == 0 );
    // clean the resulting array
    Vec_IntClear( vWin );
    // collect leaves
    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vIn, iObj, i )
    {
        Gia_ObjSetTravIdCurrentId( p, iObj );
        Vec_IntPush( vWin, iObj );
    }
    // collect internal cone
    Gia_WinCreateFromCut_rec( p, iPivot, vWin );
    // add nodes to the levelized structure
    Vec_IntForEachEntry( vWin, iObj, i )
    {
        Vec_WecPush( vLevels, Gia_ObjLevelId(p, iObj), iObj );
        Vec_IntPushUniqueOrder( vUsed, Gia_ObjLevelId(p, iObj) );
    }
    // iterate through all objects and explore their fanouts
    //Vec_WecForEachLevel( vLevels, vLevel, k )
    Vec_IntForEachEntry( vUsed, Level, k )
    {
        vLevel = Vec_WecEntry( vLevels, Level );
        Gia_ManForEachObjVec( vLevel, p, pObj, i )
            Gia_ObjForEachFanoutStatic( p, pObj, pFanout, f )
            {
                if ( f == 5 ) // explore first 5 fanouts of the node
                    break;
                if ( Gia_ObjIsAnd(pFanout) && // internal node
                    !Gia_ObjIsTravIdCurrent(p, pFanout) && // not in the window
                     Gia_ObjIsTravIdCurrent(p, Gia_ObjFanin0(pFanout)) && // but fanins are
                     Gia_ObjIsTravIdCurrent(p, Gia_ObjFanin1(pFanout)) )  // in the window
                {
                    // add fanout to the window and to the levelized structure
                    Gia_ObjSetTravIdCurrent( p, pFanout );
                    Vec_WecPush( vLevels, Gia_ObjLevel(p, pFanout), Gia_ObjId(p, pFanout) );
                    Vec_IntPush( vWin, Gia_ObjId(p, pFanout) );
                    Vec_IntPushUniqueOrder( vUsed, Gia_ObjLevel(p, pFanout) );
                }
            }
        Vec_IntClear( vLevel );
    }
    Vec_IntSort( vWin, 0 );
    Vec_IntFree( vUsed );
}
// update the cut until both fanins of AND nodes are not in the cut
int Gia_RsbExpandCut( Gia_Man_t * p, Vec_Int_t * vIns )
{
    int fOnlyPis = 0, fChange = 1, nSize = Vec_IntSize(vIns);
    while ( fChange )
    {
        Gia_Obj_t * pObj;
        int i, iFan0, iFan1, fHave0, fHave1;
        fOnlyPis = 1;
        fChange = 0;
        // check if some nodes can be expanded without increasing cut size
        Gia_ManForEachObjVec( vIns, p, pObj, i )
        {
            assert( Gia_ObjIsTravIdCurrent(p, pObj) );
            if ( !Gia_ObjIsAnd(pObj) )
                continue;
            fOnlyPis = 0;
            iFan0 = Gia_ObjFaninId0p(p, pObj);
            iFan1 = Gia_ObjFaninId1p(p, pObj);
            fHave0 = Gia_ObjIsTravIdCurrentId(p, iFan0);
            fHave1 = Gia_ObjIsTravIdCurrentId(p, iFan1);
            if ( !fHave0 && !fHave1 )
                continue;
            // can expand because one of the fanins is already in the cut
            // remove current cut node
            Vec_IntDrop( vIns, i );
            // add missing fanin
            if ( !fHave0 )  
            {
                Vec_IntPush( vIns, iFan0 );
                Gia_ObjSetTravIdCurrentId( p, iFan0 );
            }
            if ( !fHave1 )  
            {
                Vec_IntPush( vIns, iFan1 );
                Gia_ObjSetTravIdCurrentId( p, iFan1 );
            }
            fChange = 1;
            break;
        }
    }
    assert( Vec_IntSize(vIns) <= nSize );
    return fOnlyPis;
}
int Gia_RsbFindFaninAdd( int iFan, int pFanins[32], int pFaninCounts[32], int nFanins )
{
    int i;
    for ( i = 0; i < nFanins; i++ )
        if ( pFanins[i] == iFan )
            break;
    pFanins[i] = iFan;
    pFaninCounts[i]++;
    return nFanins + (i == nFanins);
}                        
int Gia_RsbFindFaninToAddToCut( Gia_Man_t * p, Vec_Int_t * vIns )
{
    Gia_Obj_t * pObj;
    int nFanins = 0, pFanins[64] = {0}, pFaninCounts[64] = {0};
    int i, iFan0, iFan1, iFanMax = -1, CountMax = 0;
    Gia_ManForEachObjVec( vIns, p, pObj, i )
    {
        if ( !Gia_ObjIsAnd(pObj) )
            continue;
        iFan0 = Gia_ObjFaninId0p(p, pObj);
        iFan1 = Gia_ObjFaninId1p(p, pObj);
        assert( !Gia_ObjIsTravIdCurrentId(p, iFan0) );
        assert( !Gia_ObjIsTravIdCurrentId(p, iFan1) );
        nFanins = Gia_RsbFindFaninAdd( iFan0, pFanins, pFaninCounts, nFanins );
        nFanins = Gia_RsbFindFaninAdd( iFan1, pFanins, pFaninCounts, nFanins );
        assert( nFanins < 64 );
    }
    // find fanin with the highest count
    if ( p->vFanoutNums != NULL )
    {
        for ( i = 0; i < nFanins; i++ )
            if ( CountMax < pFaninCounts[i] || (CountMax == pFaninCounts[i] && (Gia_ObjFanoutNumId(p, iFanMax) < Gia_ObjFanoutNumId(p, pFanins[i]))) )
            {
                CountMax = pFaninCounts[i];
                iFanMax  = pFanins[i];
            }
    }
    else
    {
        for ( i = 0; i < nFanins; i++ )
            if ( CountMax < pFaninCounts[i] || (CountMax == pFaninCounts[i] && (Gia_ObjRefNumId(p, iFanMax) < Gia_ObjRefNumId(p, pFanins[i]))) )
            {
                CountMax = pFaninCounts[i];
                iFanMax  = pFanins[i];
            }
    }
    return iFanMax;
}
// precondition: nodes in vWin and in vIns are marked with the current ID
void Gia_RsbWindowGrow2( Gia_Man_t * p, int iObj, Vec_Wec_t * vLevels, Vec_Int_t * vWin, Vec_Int_t * vIns, int nInputsMax )
{
    // window will be recomputed later
    Vec_IntClear( vWin );
    // expand the cut without increasing its cost
    if ( !Gia_RsbExpandCut( p, vIns ) )
    {
        // save it as the best cut
        Vec_Int_t * vBest = Vec_IntSize(vIns) <= nInputsMax ? Vec_IntDup(vIns) : NULL;
        int fOnlyPis = 0, Iter = 0;
        // iterate expansion until  
        // (1) the cut cannot be expanded because all leaves are PIs
        // (2) the cut size exceeded the limit for 5 consecutive iterations
        while ( !fOnlyPis && (Vec_IntSize(vIns) <= nInputsMax || Iter < 5) )
        {
            int iFanBest = Gia_RsbFindFaninToAddToCut( p, vIns );
            Vec_IntPush( vIns, iFanBest );
            Gia_ObjSetTravIdCurrentId( p, iFanBest );
            fOnlyPis = Gia_RsbExpandCut( p, vIns );
            if ( Vec_IntSize(vIns) > nInputsMax )
                Iter++;
            else
                Iter = 0;                
            if ( Vec_IntSize(vIns) <= nInputsMax && (!vBest || Vec_IntSize(vBest) <= Vec_IntSize(vIns)) )
            {
                if ( vBest )
                    Vec_IntClear(vBest);
                else
                    vBest = Vec_IntAlloc( 10 );
                Vec_IntAppend( vBest, vIns );
            }
        }
        if ( vBest )
        {
            Vec_IntClear( vIns );
            Vec_IntAppend( vIns, vBest );
            Vec_IntFree( vBest );
        }
        else
            assert( Vec_IntSize(vIns) > nInputsMax );
    }
    if ( vLevels && Vec_IntSize(vIns) <= nInputsMax )
    {
        Vec_IntSort( vIns, 0 );
        Gia_WinCreateFromCut( p, iObj, vIns, vLevels, vWin );
    }
}

/**Function*************************************************************

  Synopsis    [Create window for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_RsbWindowCompute( Gia_Man_t * p, int iObj, int nInputsMax, int nLevelsMax, Vec_Wec_t * vLevels, Vec_Int_t * vPaths, Vec_Int_t ** pvWin, Vec_Int_t ** pvIns )
{
    Vec_Int_t * vWin, * vIns;
    *pvWin = *pvIns = NULL;
    vWin = Gia_RsbWindowInit( p, vPaths, iObj, nLevelsMax );
    if ( vWin == NULL )
        return 0;
    vIns = Gia_RsbCreateWindowInputs( p, vWin ); 
    // vWin and vIns are labeled with the current trav ID
    //Vec_IntPrint( vWin );    
    //Vec_IntPrint( vIns );    
    if ( Vec_IntSize(vIns) <= nInputsMax + 3 ) // consider windows, which initially has a larger input space
        Gia_RsbWindowGrow2( p, iObj, vLevels, vWin, vIns, nInputsMax );
    if ( Vec_IntSize(vIns) <= nInputsMax )
    {
        Vec_IntSort( vWin, 0 );
        Vec_IntSort( vIns, 0 );
        *pvWin = vWin;
        *pvIns = vIns;
        return 1;
    }
    else
    {
        Vec_IntFree( vWin );
        Vec_IntFree( vIns );
        return 0;
    }
}

/**Function*************************************************************

  Synopsis    [Derive GIA from the window]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_RsbFindOutputs( Gia_Man_t * p, Vec_Int_t * vWin, Vec_Int_t * vIns, Vec_Int_t * vRefs )
{
    Vec_Int_t * vOuts = Vec_IntAlloc( 100 );
    Gia_Obj_t * pObj; int i;
    Gia_ManIncrementTravId( p );
    Gia_ManForEachObjVec( vIns, p, pObj, i ) 
        Gia_ObjSetTravIdCurrent( p, pObj );
    Gia_ManForEachObjVec( vWin, p, pObj, i ) 
        if ( !Gia_ObjIsTravIdCurrent(p, pObj) && Gia_ObjIsAnd(pObj) )
        {
            Vec_IntAddToEntry( vRefs, Gia_ObjFaninId0p(p, pObj), 1 );
            Vec_IntAddToEntry( vRefs, Gia_ObjFaninId1p(p, pObj), 1 );
        }
    Gia_ManForEachObjVec( vWin, p, pObj, i )
        if ( !Gia_ObjIsTravIdCurrent(p, pObj) && Gia_ObjFanoutNum(p, pObj) != Vec_IntEntry(vRefs, Gia_ObjId(p, pObj)) )
            Vec_IntPush( vOuts, Gia_ObjId(p, pObj) );
    Gia_ManForEachObjVec( vWin, p, pObj, i )
        if ( !Gia_ObjIsTravIdCurrent(p, pObj) && Gia_ObjIsAnd(pObj) )
        {
            Vec_IntAddToEntry( vRefs, Gia_ObjFaninId0p(p, pObj), -1 );
            Vec_IntAddToEntry( vRefs, Gia_ObjFaninId1p(p, pObj), -1 );
        }
    return vOuts;
}

Gia_Man_t * Gia_RsbDeriveGiaFromWindows( Gia_Man_t * p, Vec_Int_t * vWin, Vec_Int_t * vIns, Vec_Int_t * vOuts )
{
    Gia_Man_t * pNew; 
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObjVec( vIns, p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachObjVec( vWin, p, pObj, i )
        if ( !~pObj->Value )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachObjVec( vOuts, p, pObj, i )
        Gia_ManAppendCo( pNew, pObj->Value );
    Gia_ManHashStop( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Naive truth-table-based verification.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Gia_LutComputeTruth66_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    word Truth0, Truth1;
    if ( Gia_ObjIsCi(pObj) )
        return s_Truths6[Gia_ObjCioId(pObj)];
    if ( Gia_ObjIsConst0(pObj) )
        return 0;
    assert( Gia_ObjIsAnd(pObj) );
    Truth0 = Gia_LutComputeTruth66_rec( p, Gia_ObjFanin0(pObj) );
    Truth1 = Gia_LutComputeTruth66_rec( p, Gia_ObjFanin1(pObj) );
    if ( Gia_ObjFaninC0(pObj) )
        Truth0 = ~Truth0;
    if ( Gia_ObjFaninC1(pObj) )
        Truth1 = ~Truth1;
    return Truth0 & Truth1;
}
int Gia_ManVerifyTwoTruths( Gia_Man_t * p1, Gia_Man_t * p2 )
{
    int i, fFailed = 0;
    assert( Gia_ManCoNum(p1) == Gia_ManCoNum(p2) );
    for ( i = 0; i < Gia_ManCoNum(p1); i++ )
    {
        Gia_Obj_t * pPo1 = Gia_ManCo(p1, i);
        Gia_Obj_t * pPo2 = Gia_ManCo(p2, i);
        word word1 = Gia_LutComputeTruth66_rec( p1, Gia_ObjFanin0(pPo1) );
        word word2 = Gia_LutComputeTruth66_rec( p2, Gia_ObjFanin0(pPo2) );
        if ( Gia_ObjFaninC0(pPo1) )
            word1 = ~word1;
        if ( Gia_ObjFaninC0(pPo2) )
            word2 = ~word2;
        if ( word1 != word2 )
        {
            //Dau_DsdPrintFromTruth( &word1, 6 );
            //Dau_DsdPrintFromTruth( &word2, 6 );
            printf( "Verification failed for output %d (out of %d).\n", i, Gia_ManCoNum(p1) );
            fFailed = 1;
        }
    }
//    if ( !fFailed )
//        printf( "Verification succeeded for %d outputs.\n", Gia_ManCoNum(p1) );
    return !fFailed;
}



/**Function*************************************************************

  Synopsis    [Enumerate windows of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_RsbEnumerateWindows( Gia_Man_t * p, int nInputsMax, int nLevelsMax )
{
    int fVerbose = 0;
    int fUseHash = 0;
    int i, nWins = 0, nWinSize = 0, nInsSize = 0, nOutSize = 0, nNodeGain = 0;
    Vec_Wec_t * vLevels = Vec_WecStart( Gia_ManLevelNum(p)+1 );
    Vec_Int_t * vPaths = Vec_IntStart( Gia_ManObjNum(p) );
    Vec_Int_t * vRefs = Vec_IntStart( Gia_ManObjNum(p) );
    Hsh_VecMan_t * pHash = Hsh_VecManStart( 1000 );
    Gia_Obj_t * pObj;
    Gia_Man_t * pIn, * pOut;
    abctime clk = Abc_Clock();
    Gia_ManStaticFanoutStart( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        Vec_Int_t * vWin, * vIns, * vOuts;
        if ( !Gia_RsbWindowCompute( p, i, nInputsMax, nLevelsMax, vLevels, vPaths, &vWin, &vIns ) )
            continue;
        vOuts = Gia_RsbFindOutputs( p, vWin, vIns, vRefs );
        nWins++;
        nWinSize += Vec_IntSize(vWin);
        nInsSize += Vec_IntSize(vIns);
        nOutSize += Vec_IntSize(vOuts);
   

        if ( fVerbose )
        {
            printf( "\n\nObj %d\n", i );
            Vec_IntPrint( vWin );    
            Vec_IntPrint( vIns );    
            Vec_IntPrint( vOuts );    
            printf( "\n" );
        }
        else if ( Vec_IntSize(vWin) > 1000 )
            printf( "Obj %d.   Window: Ins = %d. Ands = %d. Outs = %d.\n", 
                i, Vec_IntSize(vIns), Vec_IntSize(vWin)-Vec_IntSize(vIns), Vec_IntSize(vOuts) );

        if ( fUseHash )
        {
            int nEntries = Hsh_VecSize(pHash);
            Hsh_VecManAdd( pHash, vWin );
            if ( nEntries == Hsh_VecSize(pHash) )
            {
                Vec_IntFree( vWin );
                Vec_IntFree( vIns );
                Vec_IntFree( vOuts );
                continue;
            }
        }

        pIn = Gia_RsbDeriveGiaFromWindows( p, vWin, vIns, vOuts );
        pOut = Gia_ManResub2Test( pIn );
        //pOut = Gia_ManDup( pIn );
        if ( !Gia_ManVerifyTwoTruths( pIn, pOut ) )
        {
            Gia_ManPrint( pIn );
            Gia_ManPrint( pOut );
            pOut = pOut;
        }

        nNodeGain += Gia_ManAndNum(pIn) - Gia_ManAndNum(pOut);
        Gia_ManStop( pIn );
        Gia_ManStop( pOut );

        Vec_IntFree( vWin );
        Vec_IntFree( vIns );
        Vec_IntFree( vOuts );
    }
    Gia_ManStaticFanoutStop( p );
    Vec_WecFree( vLevels );
    Vec_IntFree( vPaths );
    Vec_IntFree( vRefs );
    printf( "Computed windows for %d nodes (out of %d). Unique = %d. Ave inputs = %.2f. Ave outputs = %.2f. Ave volume = %.2f.  Gain = %d. ", 
        nWins, Gia_ManAndNum(p), Hsh_VecSize(pHash), 1.0*nInsSize/Abc_MaxInt(1,nWins), 
        1.0*nOutSize/Abc_MaxInt(1,nWins), 1.0*nWinSize/Abc_MaxInt(1,nWins), nNodeGain );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Hsh_VecManStop( pHash );
}

/**Function*************************************************************

  Synopsis    [Apply k-resub to one AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_RsbTryOneWindow( Gia_Man_t * p )
{
    return Gia_ManResub2Test( p );
}

/**Function*************************************************************

  Synopsis    [Apply k-resub to one AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_RsbTestArray()
{
    int Array[1000] = { 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 6, 3, 7, 15, 17, 8, 19, 
        5, 20, 5, 12, 8, 24, 4, 12, 9, 28, 27, 31, 23, 32, 4, 13, 8, 36, 5, 
        13, 18, 40, 9, 18, 5, 44, 19, 36, 9, 48, 47, 51, 10, 18, 40, 54, 8, 
        56, 25, 37, 44, 61, 59, 63, 8, 28, 8, 18, 25, 68, 66, 70, 64, 73, 11, 
        19, 8, 13, 76, 78, 10, 19, 40, 82, 9, 84, 81, 87, 20, 61, 19, 28, 30, 
        92, 91, 95, 88, 96, 74, 98, 9, 40, 49, 103, 27, 104, 10, 107, 8, 40, 
        9, 24, 111, 113, 11, 115, 109, 117, 11, 66, 51, 121, 118, 122, 18, 36, 
        18, 110, 93, 127, 10, 131, 129, 133, 11, 38, 32, 137, 103, 138, 19, 141, 
        134, 143, 28, 76, 9, 146, 11, 110, 19, 150, 149, 153, 87, 95, 9, 19, 10, 
        159, 61, 160, 18, 30, 61, 158, 9, 12, 25, 169, 19, 171, 111, 173, 10, 175, 
        167, 177, 18, 102, 4, 20, 18, 171, 183, 185, 11, 187, 181, 189, 178, 190, 
        24, 44, 11, 194, 8, 54, 4, 198, 197, 201, 45, 49, 10, 39, 9, 126, 73, 209, 
        11, 211, 54, 168, 213, 215, 43, 167, 67, 218, 10, 221, 26, 54, 18, 18, 34, 
        34, 38, 38, 40, 40, 42, 42, 52, 52, 100, 100, 124, 124, 126, 126, 144, 144, 
        148, 148, 154, 154, 156, 156, 162, 162, 164, 164, 192, 192, 70, 70, 202, 
        202, 204, 204, 206, 206, 216, 216, 222, 222, 224, 224
    };
    int i, iFan0, iFan1, nResubs;
    int * pRes;
    // create the internal array
    Vec_Int_t * vArray = Vec_IntAlloc( 100 );
    for ( i = 0; i < 50 || Array[i] > 0; i++ )
        Vec_IntPush( vArray, Array[i] );
    Vec_IntPrint( vArray );
    // check the nodes
    printf( "Constant0 and primary inputs:\n" );
    Vec_IntForEachEntryDouble( vArray, iFan0, iFan1, i )
    {
        if ( iFan0 != iFan1 )
            break;
        printf( "%2d = %c%2d & %c%2d;\n", i, 
            Abc_LitIsCompl(iFan0) ? '!' : ' ', Abc_Lit2Var(iFan0),
            Abc_LitIsCompl(iFan1) ? '!' : ' ', Abc_Lit2Var(iFan1) );
    }
    printf( "Primary outputs:\n" );
    Vec_IntForEachEntryDoubleStart( vArray, iFan0, iFan1, i, 14 )
    {
        if ( iFan0 != iFan1 )
            continue;
        printf( "%2d = %c%2d & %c%2d;\n", i, 
            Abc_LitIsCompl(iFan0) ? '!' : ' ', Abc_Lit2Var(iFan0),
            Abc_LitIsCompl(iFan1) ? '!' : ' ', Abc_Lit2Var(iFan1) );
    }
    // run the resub
    Abc_ResubPrepareManager( 1 );
    Abc_ResubComputeWindow( Vec_IntArray(vArray), Vec_IntSize(vArray)/2, 10, -1, 0, 0, 1, 1, &pRes, &nResubs );
    Abc_ResubPrepareManager( 0 );
    Vec_IntFree( vArray );
}

/**Function*************************************************************

  Synopsis    [Computing cuts of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wec_t * Gia_ManExtractCuts2( Gia_Man_t * p, int nCutSize, int nCuts, int fVerbose )
{
    int c, nLevelMax = 8;
    abctime clk = Abc_Clock();
    Vec_Wec_t * vCuts = Vec_WecStart( nCuts );
    Vec_Int_t * vPaths = Vec_IntStart( Gia_ManObjNum(p) );
    srand( time(NULL) );
    for ( c = 0; c < nCuts; )
    {
        Vec_Int_t * vCut, * vWin = NULL;
        while ( vWin == NULL )
        {
            int iPivot = 1 + Gia_ManCiNum(p) + rand() % Gia_ManAndNum(p);
            assert( Gia_ObjIsAnd(Gia_ManObj(p, iPivot)) );
            vWin = Gia_RsbWindowInit( p, vPaths, iPivot, nLevelMax );
        }
        vCut = Gia_RsbCreateWindowInputs( p, vWin );
        if ( Vec_IntSize(vCut) >= nCutSize - 2 && Vec_IntSize(vCut) <= nCutSize )
        {
            Vec_IntPush( Vec_WecEntry(vCuts, c), Vec_IntSize(vCut) );
            Vec_IntAppend( Vec_WecEntry(vCuts, c++), vCut );
        }
        Vec_IntFree( vCut );
        Vec_IntFree( vWin );
    }
    Vec_IntFree( vPaths );
    Abc_PrintTime( 0, "Computing cuts  ", Abc_Clock() - clk );
    return vCuts;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

