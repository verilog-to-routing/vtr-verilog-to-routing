/**CFile****************************************************************

  FileName    [cecIso.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Detection of structural isomorphism.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecIso.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cecInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline unsigned * Cec_ManIsoInfo( unsigned * pStore, int nWords, int Id ) { return pStore + nWords * Id; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes simulation info for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManIsoSimulate( Gia_Obj_t * pObj, int Id, unsigned * pStore, int nWords )
{
    unsigned * pInfo  = Cec_ManIsoInfo( pStore, nWords, Id );
    unsigned * pInfo0 = Cec_ManIsoInfo( pStore, nWords, Gia_ObjFaninId0(pObj, Id) );
    unsigned * pInfo1 = Cec_ManIsoInfo( pStore, nWords, Gia_ObjFaninId1(pObj, Id) );
    int w;
    if ( Gia_ObjFaninC0(pObj) )
    {
        if (  Gia_ObjFaninC1(pObj) )
            for ( w = 0; w < nWords; w++ )
                pInfo[w] = ~(pInfo0[w] | pInfo1[w]);
        else 
            for ( w = 0; w < nWords; w++ )
                pInfo[w] = ~pInfo0[w] & pInfo1[w];
    }
    else 
    {
        if (  Gia_ObjFaninC1(pObj) )
            for ( w = 0; w < nWords; w++ )
                pInfo[w] = pInfo0[w] & ~pInfo1[w];
        else 
            for ( w = 0; w < nWords; w++ )
                pInfo[w] = pInfo0[w] & pInfo1[w];
    }
}

/**Function*************************************************************

  Synopsis    [Copies simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManIsoCopy( int IdDest, int IdSour, unsigned * pStore, int nWords )
{
    unsigned * pInfo0 = Cec_ManIsoInfo( pStore, nWords, IdDest );
    unsigned * pInfo1 = Cec_ManIsoInfo( pStore, nWords, IdSour );
    int w;
    for ( w = 0; w < nWords; w++ )
        pInfo0[w] = pInfo1[w];
}

/**Function*************************************************************

  Synopsis    [Compares simulation info of two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManIsoEqual( int Id0, int Id1, unsigned * pStore, int nWords )
{
    unsigned * pInfo0 = Cec_ManIsoInfo( pStore, nWords, Id0 );
    unsigned * pInfo1 = Cec_ManIsoInfo( pStore, nWords, Id1 );
    int w;
    for ( w = 0; w < nWords; w++ )
        if ( pInfo0[w] != pInfo1[w] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Generates random simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManIsoRandom( int Id, unsigned * pStore, int nWords )
{
    unsigned * pInfo0 = Cec_ManIsoInfo( pStore, nWords, Id );
    int w;
    for ( w = 0; w < nWords; w++ )
        pInfo0[w] = Gia_ManRandom( 0 );
}

/**Function*************************************************************

  Synopsis    [Computes hash key of the simuation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManIsoHashKey( int Id, unsigned * pStore, int nWords, int nTableSize )
{
    static int s_Primes[16] = { 
        1291, 1699, 1999, 2357, 2953, 3313, 3907, 4177, 
        4831, 5147, 5647, 6343, 6899, 7103, 7873, 8147 };
    unsigned * pInfo0 = Cec_ManIsoInfo( pStore, nWords, Id );
    unsigned uHash = 0;
    int i;
    for ( i = 0; i < nWords; i++ )
        uHash ^= pInfo0[i] * s_Primes[i & 0xf];
    return (int)(uHash % nTableSize);

}

/**Function*************************************************************

  Synopsis    [Adds node to the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManIsoTableAdd( Gia_Man_t * p, int Id, unsigned * pStore, int nWords, int * pTable, int nTableSize )
{
    Gia_Obj_t * pTemp;
    int Key, Ent, Color = Gia_ObjColors( p, Id );
    assert( Color == 1 || Color == 2 );
    Key = Gia_ManIsoHashKey( Id, pStore, nWords, nTableSize );
    for ( Ent = pTable[Key],  pTemp = (Ent ?  Gia_ManObj(p, Ent) : NULL);  pTemp;
          Ent = pTemp->Value, pTemp = (Ent ?  Gia_ManObj(p, Ent) : NULL) )
    {
        if ( Gia_ObjColors( p, Ent ) != Color )
            continue;
        if ( !Gia_ManIsoEqual( Id, Ent, pStore, nWords ) )
            continue;
        // found node with the same color and signature - mark it and do not add new node
        pTemp->fMark0 = 1;
        return;
    }
    // did not find the node with the same color and signature - add new node
    pTemp = Gia_ManObj( p, Id );
    assert( pTemp->Value == 0 );
    assert( pTemp->fMark0 == 0 );
    pTemp->Value = pTable[Key];
    pTable[Key] = Id;
}

/**Function*************************************************************

  Synopsis    [Extracts equivalence class candidates from one bin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManIsoExtractClasses( Gia_Man_t * p, int Bin, unsigned * pStore, int nWords, Vec_Int_t * vNodesA, Vec_Int_t * vNodesB )
{
    Gia_Obj_t * pTemp;
    int Ent;
    Vec_IntClear( vNodesA );
    Vec_IntClear( vNodesB );
    for ( Ent = Bin,          pTemp = (Ent ?  Gia_ManObj(p, Ent) : NULL);  pTemp;
          Ent = pTemp->Value, pTemp = (Ent ?  Gia_ManObj(p, Ent) : NULL) )
    {
        if ( pTemp->fMark0 )
        {
            pTemp->fMark0 = 0;
            continue;
        }
        if ( Gia_ObjColors( p, Ent ) == 1 )
            Vec_IntPush( vNodesA, Ent );
        else
            Vec_IntPush( vNodesB, Ent );
    }
    return Vec_IntSize(vNodesA) > 0 && Vec_IntSize(vNodesB) > 0;    
}

/**Function*************************************************************

  Synopsis    [Matches nodes in the extacted classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManIsoMatchNodes( int * pIso, unsigned * pStore, int nWords, Vec_Int_t * vNodesA, Vec_Int_t * vNodesB )
{
    int k0, k1, IdA, IdB;
    Vec_IntForEachEntry( vNodesA, IdA, k0 )
    Vec_IntForEachEntry( vNodesB, IdB, k1 )
    {
        if ( Gia_ManIsoEqual( IdA, IdB, pStore, nWords ) )
        {
            assert( pIso[IdA] == 0 );
            assert( pIso[IdB] == 0 );
            assert( IdA != IdB );
            pIso[IdA] = IdB;
            pIso[IdB] = IdA;
            continue;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Transforms iso into equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_ManTransformClasses( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    assert( p->pReprs && p->pNexts && p->pIso );
    memset( p->pReprs, 0, sizeof(int) * Gia_ManObjNum(p) );
    memset( p->pNexts, 0, sizeof(int) * Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        p->pReprs[i].iRepr = GIA_VOID;
        if ( p->pIso[i] && p->pIso[i] < i )
        {
            p->pReprs[i].iRepr = p->pIso[i];
            p->pNexts[p->pIso[i]] = i;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Finds node correspondences in the miter.]

  Description [Assumes that the colors are assigned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Cec_ManDetectIsomorphism( Gia_Man_t * p )
{
    int nWords = 2;
    Gia_Obj_t * pObj;
    Vec_Int_t * vNodesA, * vNodesB;
    unsigned * pStore, Counter;
    int i, * pIso, * pTable, nTableSize;
    // start equivalence classes
    pIso = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsCo(pObj) )
        {
            assert( Gia_ObjColors(p, i) == 0 );
            continue;
        }
        assert( Gia_ObjColors(p, i) );
        if ( Gia_ObjColors(p, i) == 3 )
            pIso[i] = i;
    }
    // start simulation info
    pStore = ABC_ALLOC( unsigned, Gia_ManObjNum(p) * nWords );
    // simulate and create table
    nTableSize = Abc_PrimeCudd( 100 + Gia_ManObjNum(p)/2 );
    pTable = ABC_CALLOC( int, nTableSize );
    Gia_ManCleanValue( p );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCo(pObj) )
            continue;
        if ( pIso[i] == 0 ) // simulate
            Gia_ManIsoSimulate( pObj, i, pStore, nWords );
        else if ( pIso[i] < i ) // copy
            Gia_ManIsoCopy( i, pIso[i], pStore, nWords );
        else // generate
            Gia_ManIsoRandom( i, pStore, nWords );
        if ( pIso[i] == 0 )
            Gia_ManIsoTableAdd( p, i, pStore, nWords, pTable, nTableSize );
    }
    // create equivalence classes
    vNodesA = Vec_IntAlloc( 100 );
    vNodesB = Vec_IntAlloc( 100 );
    for ( i = 0; i < nTableSize; i++ )
        if ( Gia_ManIsoExtractClasses( p, pTable[i], pStore, nWords, vNodesA, vNodesB ) )
            Gia_ManIsoMatchNodes( pIso, pStore, nWords, vNodesA, vNodesB );
    Vec_IntFree( vNodesA );
    Vec_IntFree( vNodesB );
    // collect info
    Counter = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        Counter += (pIso[i] && pIso[i] < i);
/*
        if ( pIso[i] && pIso[i] < i )
        {
            if ( (Gia_ObjIsHead(p,pIso[i]) && Gia_ObjRepr(p,i)==pIso[i]) || 
                 (Gia_ObjIsClass(p,pIso[i]) && Gia_ObjRepr(p,i)==Gia_ObjRepr(p,pIso[i])) )
                 Abc_Print( 1, "1" );
            else
                Abc_Print( 1, "0" );
        }
*/
    }
    Abc_Print( 1, "Computed %d pairs of structurally equivalent nodes.\n", Counter );
//    p->pIso = pIso;
//    Cec_ManTransformClasses( p );

    ABC_FREE( pTable );
    ABC_FREE( pStore );
    return pIso;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

