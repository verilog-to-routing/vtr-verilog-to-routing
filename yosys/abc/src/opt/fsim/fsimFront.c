/**CFile****************************************************************

  FileName    [fsimFront.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast sequential AIG simulator.]

  Synopsis    [Simulation frontier.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: fsimFront.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fsimInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManStoreNum( Fsim_Man_t * p, int Num )
{
    unsigned x = (unsigned)Num;
    assert( Num >= 0 );
    while ( x & ~0x7f )
    {
        *p->pDataCur++ = (x & 0x7f) | 0x80;
        x >>= 7;
    }
    *p->pDataCur++ = x;
    assert( p->pDataCur - p->pDataAig < p->nDataAig );
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Fsim_ManRestoreNum( Fsim_Man_t * p )
{
    int ch, i, x = 0;
    for ( i = 0; (ch = *p->pDataCur++) & 0x80; i++ )
        x |= (ch & 0x7f) << (7 * i);
    assert( p->pDataCur - p->pDataAig < p->nDataAig );
    return x | (ch << (7 * i));
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Fsim_ManStoreObj( Fsim_Man_t * p, Fsim_Obj_t * pObj )
{
    if ( p->pDataAig2 )
    {
        *p->pDataCur2++ = pObj->iNode;
        *p->pDataCur2++ = pObj->iFan0;
        *p->pDataCur2++ = pObj->iFan1;
        return;
    }
    if ( pObj->iFan0 && pObj->iFan1 ) // and
    {
        assert( pObj->iNode );
        assert( pObj->iNode >= p->iNodePrev );
        assert( (pObj->iNode << 1) > pObj->iFan0 );
        assert( pObj->iFan0 > pObj->iFan1 );
        Fsim_ManStoreNum( p, ((pObj->iNode - p->iNodePrev) << 2) | 3 );
        Fsim_ManStoreNum( p, (pObj->iNode << 1) - pObj->iFan0 );
        Fsim_ManStoreNum( p, pObj->iFan0 - pObj->iFan1 );
        p->iNodePrev = pObj->iNode;
    }
    else if ( !pObj->iFan0 && !pObj->iFan1 ) // ci
    {
        assert( pObj->iNode );
        assert( pObj->iNode >= p->iNodePrev );
        Fsim_ManStoreNum( p, ((pObj->iNode - p->iNodePrev) << 2) | 1 );
        p->iNodePrev = pObj->iNode;
    }
    else // if ( !pObj->iFan0 && pObj->iFan1 ) // co
    {
        assert( pObj->iNode == 0 );
        assert( pObj->iFan0 != 0 );
        assert( pObj->iFan1 == 0 );
        assert( ((p->iNodePrev << 1) | 1) >= pObj->iFan0 );
        Fsim_ManStoreNum( p, (((p->iNodePrev << 1) | 1) - pObj->iFan0) << 1 );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Fsim_ManRestoreObj( Fsim_Man_t * p, Fsim_Obj_t * pObj )
{
    int iValue = Fsim_ManRestoreNum( p );
    if ( (iValue & 3) == 3 ) // and
    {
        pObj->iNode = (iValue >> 2) + p->iNodePrev;
        pObj->iFan0 = (pObj->iNode << 1) - Fsim_ManRestoreNum( p );
        pObj->iFan1 = pObj->iFan0 - Fsim_ManRestoreNum( p );
        p->iNodePrev = pObj->iNode;
    }
    else if ( (iValue & 3) == 1 ) // ci
    {
        pObj->iNode = (iValue >> 2) + p->iNodePrev;
        pObj->iFan0 = 0;
        pObj->iFan1 = 0;
        p->iNodePrev = pObj->iNode;
    }
    else // if ( (iValue & 1) == 0 ) // co
    {
        pObj->iNode = 0;
        pObj->iFan0 = ((p->iNodePrev << 1) | 1) - (iValue >> 1);
        pObj->iFan1 = 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Determine the frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Fsim_ManFrontFindNext( Fsim_Man_t * p, char * pFront )
{
    assert( p->iNumber < (1 << 30) - p->nFront );
    while ( 1 )
    {
        if ( p->iNumber % p->nFront == 0 )
            p->iNumber++;
        if ( pFront[p->iNumber % p->nFront] == 0 )
        {
            pFront[p->iNumber % p->nFront] = 1;
            return p->iNumber;
        }
        p->iNumber++;
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Verifies the frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fsim_ManVerifyFront( Fsim_Man_t * p )
{
    Fsim_Obj_t * pObj;
    int * pFans0, * pFans1;  // representation of fanins
    int * pFrontToId;        // mapping of nodes into frontier variables
    int i, iVar0, iVar1;
    pFans0 = ABC_ALLOC( int, p->nObjs );
    pFans1 = ABC_ALLOC( int, p->nObjs );
    pFans0[0] = pFans1[0] = 0;
    pFans0[1] = pFans1[1] = 0;
    pFrontToId = ABC_CALLOC( int, p->nFront );
    if ( Aig_ObjRefs(Aig_ManConst1(p->pAig)) )
        pFrontToId[1] = 1;
    Fsim_ManForEachObj( p, pObj, i )
    {
        if ( pObj->iNode )
            pFrontToId[pObj->iNode % p->nFront] = i;
        iVar0 = Fsim_Lit2Var(pObj->iFan0);
        iVar1 = Fsim_Lit2Var(pObj->iFan1);
        pFans0[i] = Fsim_Var2Lit(pFrontToId[iVar0 % p->nFront], Fsim_LitIsCompl(pObj->iFan0));
        pFans1[i] = Fsim_Var2Lit(pFrontToId[iVar1 % p->nFront], Fsim_LitIsCompl(pObj->iFan1));
    }
    for ( i = 0; i < p->nObjs; i++ )
    {
        assert( pFans0[i] == p->pFans0[i] );
        assert( pFans1[i] == p->pFans1[i] );
    }
    ABC_FREE( pFrontToId );
    ABC_FREE( pFans0 );
    ABC_FREE( pFans1 );
}

/**Function*************************************************************

  Synopsis    [Determine the frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fsim_ManFront( Fsim_Man_t * p, int fCompressAig )
{
    Fsim_Obj_t Obj, * pObj = &Obj;
    char * pFront;    // places used for the frontier
    int * pIdToFront; // mapping of nodes into frontier places
    int i, iVar0, iVar1, nCrossCut = 0, nCrossCutMax = 0;
    // start the frontier
    pFront = ABC_CALLOC( char, p->nFront );
    pIdToFront = ABC_ALLOC( int, p->nObjs );
    pIdToFront[0] = -1;
    pIdToFront[1] = -1;
    // add constant node
    p->iNumber = 1;
    if ( p->pRefs[1] )
    {
        pIdToFront[1] = Fsim_ManFrontFindNext( p, pFront );
        nCrossCut = 1;
    }
    // allocate room for data
    if ( fCompressAig )
    {
        p->nDataAig = p->nObjs * 6;
        p->pDataAig = ABC_ALLOC( unsigned char, p->nDataAig );
        p->pDataCur = p->pDataAig;
        p->iNodePrev = 0;
    }
    else
    {
        p->pDataAig2 = ABC_ALLOC( int, 3 * p->nObjs );
        p->pDataCur2 = p->pDataAig2 + 6;
    }
    // iterate through the objects
    for ( i = 2; i < p->nObjs; i++ )
    {
        if ( p->pFans0[i] == 0 ) // ci
        {
            // store node
            pIdToFront[i] = Fsim_ManFrontFindNext( p, pFront );
            pObj->iNode = pIdToFront[i];
            pObj->iFan0 = 0;
            pObj->iFan1 = 0;
            Fsim_ManStoreObj( p, pObj );
            // handle CIs without fanout
            if ( p->pRefs[i] == 0 )
            {
                pFront[pIdToFront[i] % p->nFront] = 0;
                pIdToFront[i] = -1;
            }
        }
        else if ( p->pFans1[i] == 0 ) // co
        {
            assert( p->pRefs[i] == 0 );
            // get the fanin
            iVar0 = Fsim_Lit2Var(p->pFans0[i]);
            assert( pIdToFront[iVar0] > 0 );
            // store node
            pObj->iNode = 0;
            pObj->iFan0 = Fsim_Var2Lit(pIdToFront[iVar0], Fsim_LitIsCompl(p->pFans0[i]));
            pObj->iFan1 = 0;
            Fsim_ManStoreObj( p, pObj );
            // deref the fanin
            if ( --p->pRefs[iVar0] == 0 )
            {
                pFront[pIdToFront[iVar0] % p->nFront] = 0;
                pIdToFront[iVar0] = -1;
                nCrossCut--;
            }
        }
        else
        {
            // get the fanins
            iVar0 = Fsim_Lit2Var(p->pFans0[i]);
            assert( pIdToFront[iVar0] > 0 );
            iVar1 = Fsim_Lit2Var(p->pFans1[i]);
            assert( pIdToFront[iVar1] > 0 );
            // store node
            pIdToFront[i] = Fsim_ManFrontFindNext( p, pFront );
            pObj->iNode = pIdToFront[i];
            pObj->iFan0 = Fsim_Var2Lit(pIdToFront[iVar0], Fsim_LitIsCompl(p->pFans0[i]));
            pObj->iFan1 = Fsim_Var2Lit(pIdToFront[iVar1], Fsim_LitIsCompl(p->pFans1[i]));
            Fsim_ManStoreObj( p, pObj );
            // deref the fanins
            if ( --p->pRefs[iVar0] == 0 )
            {
                pFront[pIdToFront[iVar0] % p->nFront] = 0;
                pIdToFront[iVar0] = -1;
                nCrossCut--;
            }
            if ( --p->pRefs[iVar1] == 0 )
            {
                pFront[pIdToFront[iVar1] % p->nFront] = 0;
                pIdToFront[iVar1] = -1;
                nCrossCut--;
            }
            // handle nodes without fanout (choice nodes)
            if ( p->pRefs[i] == 0 )
            {
                pFront[pIdToFront[i] % p->nFront] = 0;
                pIdToFront[i] = -1;
            }
        }
        if ( p->pRefs[i] )
            if ( nCrossCutMax < ++nCrossCut )
                nCrossCutMax = nCrossCut;
    }
    assert( p->pDataAig2 == NULL || p->pDataCur2 - p->pDataAig2 == (3 * p->nObjs) );
    assert( nCrossCut == 0 );
    assert( nCrossCutMax == p->nCrossCutMax );
    for ( i = 0; i < p->nFront; i++ )
        assert( pFront[i] == 0 );
    ABC_FREE( pFront );
    ABC_FREE( pIdToFront );
//    Fsim_ManVerifyFront( p );
    ABC_FREE( p->pFans0 );
    ABC_FREE( p->pFans1 );
    ABC_FREE( p->pRefs );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

