/**CFile****************************************************************

  FileName    [amapUniq.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [Checks if the structural node already exists.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapUniq.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks if the entry exists and returns value.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Vec_IntCheckWithMask( Vec_Int_t * p, int Entry )
{
    int i;
    for ( i = 0; i < p->nSize; i++ )
        if ( (0xffff & p->pArray[i]) == (0xffff & Entry) )
            return p->pArray[i] >> 16;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Pushes entry in the natural order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntPushOrderWithMask( Vec_Int_t * p, int Entry )
{
    int i;
    if ( p->nSize == p->nCap )
        Vec_IntGrow( p, 2 * p->nCap );
    p->nSize++;
    for ( i = p->nSize-2; i >= 0; i-- )
        if ( (0xffff & p->pArray[i]) > (0xffff & Entry) )
            p->pArray[i+1] = p->pArray[i];
        else
            break;
    p->pArray[i+1] = Entry;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibFindNode( Amap_Lib_t * pLib, int iFan0, int iFan1, int fXor )
{
    if ( fXor )
        return Vec_IntCheckWithMask( (Vec_Int_t *)Vec_PtrEntry(pLib->vRulesX, iFan0), iFan1 );
    else
        return Vec_IntCheckWithMask( (Vec_Int_t *)Vec_PtrEntry(pLib->vRules, iFan0), iFan1 );
}

/**Function*************************************************************

  Synopsis    [Checks if the three-argument rule exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibFindMux( Amap_Lib_t * p, int iFan0, int iFan1, int iFan2 )
{
    int x;
    for ( x = 0; x < Vec_IntSize(p->vRules3); x += 4 )
    {
        if ( Vec_IntEntry(p->vRules3, x)   == iFan0 &&
             Vec_IntEntry(p->vRules3, x+1) == iFan1 &&
             Vec_IntEntry(p->vRules3, x+2) == iFan2 )
        {
            return Vec_IntEntry(p->vRules3, x+3);
        }
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Nod_t * Amap_LibCreateObj( Amap_Lib_t * p )
{
    Amap_Nod_t * pNode;
    if ( p->nNodes == p->nNodesAlloc )
    {
        p->pNodes = ABC_REALLOC( Amap_Nod_t, p->pNodes, 2*p->nNodesAlloc );
        p->nNodesAlloc *= 2;
    }
    pNode = Amap_LibNod( p, p->nNodes );
    memset( pNode, 0, sizeof(Amap_Nod_t) );
    pNode->Id = p->nNodes++;
    Vec_PtrPush( p->vRules, Vec_IntAlloc(8) );
    Vec_PtrPush( p->vRules, Vec_IntAlloc(8) );
    Vec_PtrPush( p->vRulesX, Vec_IntAlloc(8) );
    Vec_PtrPush( p->vRulesX, Vec_IntAlloc(8) );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibCreateVar( Amap_Lib_t * p )
{
    Amap_Nod_t * pNode;
    // start the manager
    assert( p->pNodes == NULL );
    p->nNodesAlloc = 256;
    p->pNodes = ABC_ALLOC( Amap_Nod_t, p->nNodesAlloc );
    // create the first node
    pNode = Amap_LibCreateObj( p );
    p->pNodes->Type = AMAP_OBJ_PI;
    p->pNodes->nSuppSize = 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibCreateNode( Amap_Lib_t * p, int iFan0, int iFan1, int fXor )
{
    Amap_Nod_t * pNode;
    int iFan;
    if ( iFan0 < iFan1 )
    {
        iFan  = iFan0;
        iFan0 = iFan1;
        iFan1 = iFan;
    }
    pNode = Amap_LibCreateObj( p );
    pNode->Type  = fXor? AMAP_OBJ_XOR : AMAP_OBJ_AND;
    pNode->nSuppSize = p->pNodes[Abc_Lit2Var(iFan0)].nSuppSize + p->pNodes[Abc_Lit2Var(iFan1)].nSuppSize;
    pNode->iFan0 = iFan0;
    pNode->iFan1 = iFan1;
if ( p->fVerbose )
printf( "Creating node %5d %c :  iFan0 = %5d%c  iFan1 = %5d%c\n", 
pNode->Id, (fXor?'x':' '), 
Abc_Lit2Var(iFan0), (Abc_LitIsCompl(iFan0)?'-':'+'), 
Abc_Lit2Var(iFan1), (Abc_LitIsCompl(iFan1)?'-':'+') );

    if ( fXor )
    {
        if ( iFan0 == iFan1 )
            Vec_IntPushOrderWithMask( (Vec_Int_t *)Vec_PtrEntry(p->vRulesX, iFan0), (pNode->Id << 16) | iFan1 );
        else
        {
            Vec_IntPushOrderWithMask( (Vec_Int_t *)Vec_PtrEntry(p->vRulesX, iFan0), (pNode->Id << 16) | iFan1 );
            Vec_IntPushOrderWithMask( (Vec_Int_t *)Vec_PtrEntry(p->vRulesX, iFan1), (pNode->Id << 16) | iFan0 );
        }
    }
    else
    {
        if ( iFan0 == iFan1 )
            Vec_IntPushOrderWithMask( (Vec_Int_t *)Vec_PtrEntry(p->vRules, iFan0), (pNode->Id << 16) | iFan1 );
        else
        {
            Vec_IntPushOrderWithMask( (Vec_Int_t *)Vec_PtrEntry(p->vRules, iFan0), (pNode->Id << 16) | iFan1 );
            Vec_IntPushOrderWithMask( (Vec_Int_t *)Vec_PtrEntry(p->vRules, iFan1), (pNode->Id << 16) | iFan0 );
        }
    }
    return pNode->Id;
}

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_LibCreateMux( Amap_Lib_t * p, int iFan0, int iFan1, int iFan2 )
{
    Amap_Nod_t * pNode;
    pNode = Amap_LibCreateObj( p );
    pNode->Type  = AMAP_OBJ_MUX;
    pNode->nSuppSize = p->pNodes[Abc_Lit2Var(iFan0)].nSuppSize + p->pNodes[Abc_Lit2Var(iFan1)].nSuppSize + p->pNodes[Abc_Lit2Var(iFan2)].nSuppSize;
    pNode->iFan0 = iFan0;
    pNode->iFan1 = iFan1;
    pNode->iFan2 = iFan2;
if ( p->fVerbose )
printf( "Creating node %5d %c :  iFan0 = %5d%c  iFan1 = %5d%c  iFan2 = %5d%c\n", 
pNode->Id, 'm', 
Abc_Lit2Var(iFan0), (Abc_LitIsCompl(iFan0)?'-':'+'), 
Abc_Lit2Var(iFan1), (Abc_LitIsCompl(iFan1)?'-':'+'), 
Abc_Lit2Var(iFan2), (Abc_LitIsCompl(iFan2)?'-':'+') );

    Vec_IntPush( p->vRules3, iFan0 );
    Vec_IntPush( p->vRules3, iFan1 );
    Vec_IntPush( p->vRules3, iFan2 );
    Vec_IntPush( p->vRules3, pNode->Id );
    return pNode->Id;
}

/**Function*************************************************************

  Synopsis    [Allocates triangular lookup table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int ** Amap_LibLookupTableAlloc( Vec_Ptr_t * vVec, int fVerbose )
{
    Vec_Int_t * vOne;
    int ** pRes;
    int i, k, nTotal, nSize, nEntries, Value;
    // count the total size
    nEntries = nSize = Vec_PtrSize( vVec );
    Vec_PtrForEachEntry( Vec_Int_t *, vVec, vOne, i )
        nEntries += Vec_IntSize(vOne);
    pRes = (int **)ABC_ALLOC( char, nSize * sizeof(void *) + nEntries * sizeof(int) );
    pRes[0] = (int *)((char *)pRes + nSize * sizeof(void *));
    nTotal = 0;
    Vec_PtrForEachEntry( Vec_Int_t *, vVec, vOne, i )
    {
        pRes[i] = pRes[0] + nTotal;
        nTotal += Vec_IntSize(vOne) + 1;
        if ( fVerbose )
        printf( "%d : ", i );
        Vec_IntForEachEntry( vOne, Value, k )
        {
            pRes[i][k] = Value;
            if ( fVerbose )
            printf( "%d(%d) ", Value&0xffff, Value>>16 );
        }
        if ( fVerbose )
        printf( "\n" );
        pRes[i][k] = 0;
    }
    assert( nTotal == nEntries );
    return pRes;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

