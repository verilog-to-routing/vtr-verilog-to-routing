/**CFile****************************************************************

  FileName    [giaFront.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Frontier representation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaFront.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Find the next place on the frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManFrontFindNext( char * pFront, int nFront, int iFront )
{
    assert( iFront < nFront );
    for ( ; pFront[iFront]; iFront = (iFront + 1) % nFront );
    assert( pFront[iFront] == 0 );
    pFront[iFront] = 1;
    return iFront;
}

/**Function*************************************************************

  Synopsis    [Transforms the frontier manager to its initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFrontTransform( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, * pFrontToId; // mapping of nodes into frontier variables
    assert( p->nFront > 0 );
    pFrontToId = ABC_FALLOC( int, p->nFront );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( Gia_ObjIsCo(pObj) )
        {
            assert( pObj->Value == GIA_NONE );
            pObj->iDiff0 = i - pFrontToId[Gia_ObjDiff0(pObj)];
        }
        else if ( Gia_ObjIsAnd(pObj) )
        {
            assert( (int)pObj->Value < p->nFront );
            pObj->iDiff0 = i - pFrontToId[Gia_ObjDiff0(pObj)];
            pObj->iDiff1 = i - pFrontToId[Gia_ObjDiff1(pObj)];
            pFrontToId[pObj->Value] = i;
        }
        else
        {
            assert( (int)pObj->Value < p->nFront );
            pFrontToId[pObj->Value] = i;
        }
        pObj->Value = 0;
    }
    ABC_FREE( pFrontToId );
}

/**Function*************************************************************

  Synopsis    [Determine the frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCrossCutSimple( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, nCutCur = 0, nCutMax = 0;
    Gia_ManCreateValueRefs( p );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( pObj->Value )
            nCutCur++;
        if ( nCutMax < nCutCur )
            nCutMax = nCutCur;
        if ( Gia_ObjIsAnd(pObj) )
        {
            if ( --Gia_ObjFanin0(pObj)->Value == 0 )
                nCutCur--;
            if ( --Gia_ObjFanin1(pObj)->Value == 0 )
                nCutCur--;
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            if ( --Gia_ObjFanin0(pObj)->Value == 0 )
                nCutCur--;
        }
    }
//    Gia_ManForEachObj( p, pObj, i )
//        assert( pObj->Value == 0 );
    return nCutMax;
}


/**Function*************************************************************

  Synopsis    [Determine the frontier.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFront( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj, * pFanin0New, * pFanin1New, * pObjNew;
    char * pFront;    // places used for the frontier
    int i, iLit, nCrossCut = 0, nCrossCutMax = 0;
    int nCrossCutMaxInit = Gia_ManCrossCutSimple( p );
    int iFront = 0;//, clk = Abc_Clock(); 
    // set references for all objects
    Gia_ManCreateValueRefs( p );
    // start the new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->nFront = 1 + (int)((float)1.1 * nCrossCutMaxInit); 
    // start the frontier
    pFront = ABC_CALLOC( char, pNew->nFront );
    // add constant node
    Gia_ManConst0(pNew)->Value = iFront = Gia_ManFrontFindNext( pFront, pNew->nFront, iFront );
    if ( Gia_ObjValue(Gia_ManConst0(p)) == 0 )
        pFront[iFront] = 0;
    else
        nCrossCut = 1;
    // iterate through the objects
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
        {
            if ( Gia_ObjValue(pObj) && nCrossCutMax < ++nCrossCut )
                nCrossCutMax = nCrossCut;
            // create new node
            iLit = Gia_ManAppendCi( pNew );
            pObjNew = Gia_ManObj( pNew, Abc_Lit2Var(iLit) );
            assert( Gia_ObjId(pNew, pObjNew) == Gia_ObjId(p, pObj) );
            pObjNew->Value = iFront = Gia_ManFrontFindNext( pFront, pNew->nFront, iFront );
            // handle CIs without fanout
            if ( Gia_ObjValue(pObj) == 0 )
                pFront[iFront] = 0;
            continue;
        }
        if ( Gia_ObjIsCo(pObj) )
        {
            assert( Gia_ObjValue(pObj) == 0 );
            // create new node
            iLit = Gia_ManAppendCo( pNew, 0 );
            pObjNew = Gia_ManObj( pNew, Abc_Lit2Var(iLit) );
            assert( Gia_ObjId(pNew, pObjNew) == Gia_ObjId(p, pObj) );
            // get the fanin
            pFanin0New = Gia_ManObj( pNew, Gia_ObjFaninId0(pObj, i) );
            assert( pFanin0New->Value != GIA_NONE );
            pObjNew->Value = GIA_NONE;
            pObjNew->iDiff0 = pFanin0New->Value;
            pObjNew->fCompl0 = Gia_ObjFaninC0(pObj);
            // deref the fanin
            if ( --Gia_ObjFanin0(pObj)->Value == 0 )
            {
                pFront[pFanin0New->Value] = 0;
                nCrossCut--;
            }
            continue;
        }
        if ( Gia_ObjValue(pObj) && nCrossCutMax < ++nCrossCut )
            nCrossCutMax = nCrossCut;
        // create new node
        pObjNew = Gia_ManAppendObj( pNew );
        assert( Gia_ObjId(pNew, pObjNew) == Gia_ObjId(p, pObj) );
        // assign the first fanin
        pFanin0New = Gia_ManObj( pNew, Gia_ObjFaninId0(pObj, i) );
        assert( pFanin0New->Value != GIA_NONE );
        pObjNew->iDiff0 = pFanin0New->Value;
        pObjNew->fCompl0 = Gia_ObjFaninC0(pObj);
        // assign the second fanin
        pFanin1New = Gia_ManObj( pNew, Gia_ObjFaninId1(pObj, i) );
        assert( pFanin1New->Value != GIA_NONE );
        pObjNew->iDiff1 = pFanin1New->Value;
        pObjNew->fCompl1 = Gia_ObjFaninC1(pObj);
        // assign the frontier number
        pObjNew->Value = iFront = Gia_ManFrontFindNext( pFront, pNew->nFront, iFront );
        // deref the fanins
        if ( --Gia_ObjFanin0(pObj)->Value == 0 )
        {
            pFront[pFanin0New->Value] = 0;
            nCrossCut--;
        }
        if ( --Gia_ObjFanin1(pObj)->Value == 0 )
        {
            pFront[pFanin1New->Value] = 0;
            nCrossCut--;
        }
        // handle nodes without fanout (choice nodes)
        if ( Gia_ObjValue(pObj) == 0 )
            pFront[iFront] = 0;
    }
    assert( pNew->nObjs == p->nObjs );
    assert( nCrossCut == 0 || nCrossCutMax == nCrossCutMaxInit );
    for ( i = 0; i < pNew->nFront; i++ )
        assert( pFront[i] == 0 );
    ABC_FREE( pFront );
//printf( "Crosscut = %6d. Frontier = %6d. ", nCrossCutMaxInit, pNew->nFront );
//ABC_PRT( "Time", Abc_Clock() - clk );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManFrontTest( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    pNew  = Gia_ManFront( p );
    Gia_ManFrontTransform( pNew );
//    Gia_ManCleanValue( p );
//    Gia_ManCleanValue( pNew );
    if ( memcmp( pNew->pObjs, p->pObjs, sizeof(Gia_Obj_t) * p->nObjs ) )
    {
/*
        Gia_Obj_t * pObj, * pObjNew;
        int i;
        Gia_ManForEachObj( p, pObj, i )
        {
            pObjNew = Gia_ManObj( pNew, i );
            printf( "%5d %5d   %5d %5d\n", 
                pObj->iDiff0, pObjNew->iDiff0, 
                pObj->iDiff1, pObjNew->iDiff1 );
        }
*/
        printf( "Verification failed.\n" );
    }
    else
        printf( "Verification successful.\n" );
    Gia_ManStop( pNew );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

