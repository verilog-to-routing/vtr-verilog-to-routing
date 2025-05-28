/**CFile****************************************************************

  FileName    [hopOper.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [AIG operations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: hopOper.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "hop.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// procedure to detect an EXOR gate
static inline int Hop_ObjIsExorType( Hop_Obj_t * p0, Hop_Obj_t * p1, Hop_Obj_t ** ppFan0, Hop_Obj_t ** ppFan1 )
{
    if ( !Hop_IsComplement(p0) || !Hop_IsComplement(p1) )
        return 0;
    p0 = Hop_Regular(p0);
    p1 = Hop_Regular(p1);
    if ( !Hop_ObjIsAnd(p0) || !Hop_ObjIsAnd(p1) )
        return 0;
    if ( Hop_ObjFanin0(p0) != Hop_ObjFanin0(p1) || Hop_ObjFanin1(p0) != Hop_ObjFanin1(p1) )
        return 0;
    if ( Hop_ObjFaninC0(p0) == Hop_ObjFaninC0(p1) || Hop_ObjFaninC1(p0) == Hop_ObjFaninC1(p1) )
        return 0;
    *ppFan0 = Hop_ObjChild0(p0);
    *ppFan1 = Hop_ObjChild1(p0);
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns i-th elementary variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_IthVar( Hop_Man_t * p, int i )
{
    int v;
    for ( v = Hop_ManPiNum(p); v <= i; v++ )
        Hop_ObjCreatePi( p );
    assert( i < Vec_PtrSize(p->vPis) );
    return Hop_ManPi( p, i );
}

/**Function*************************************************************

  Synopsis    [Perform one operation.]

  Description [The argument nodes can be complemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Oper( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1, Hop_Type_t Type )
{
    if ( Type == AIG_AND )
        return Hop_And( p, p0, p1 );
    if ( Type == AIG_EXOR )
        return Hop_Exor( p, p0, p1 );
    assert( 0 );
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Performs canonicization step.]

  Description [The argument nodes can be complemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_And( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1 )
{
    Hop_Obj_t * pGhost, * pResult;
//    Hop_Obj_t * pFan0, * pFan1;
    // check trivial cases
    if ( p0 == p1 )
        return p0;
    if ( p0 == Hop_Not(p1) )
        return Hop_Not(p->pConst1);
    if ( Hop_Regular(p0) == p->pConst1 )
        return p0 == p->pConst1 ? p1 : Hop_Not(p->pConst1);
    if ( Hop_Regular(p1) == p->pConst1 )
        return p1 == p->pConst1 ? p0 : Hop_Not(p->pConst1);
    // check if it can be an EXOR gate
//    if ( Hop_ObjIsExorType( p0, p1, &pFan0, &pFan1 ) )
//        return Hop_Exor( p, pFan0, pFan1 );
    // check the table
    pGhost = Hop_ObjCreateGhost( p, p0, p1, AIG_AND );
    if ( (pResult = Hop_TableLookup( p, pGhost )) )
        return pResult;
    return Hop_ObjCreate( p, pGhost );
}

/**Function*************************************************************

  Synopsis    [Performs canonicization step.]

  Description [The argument nodes can be complemented.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Exor( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1 )
{
/*
    Hop_Obj_t * pGhost, * pResult;
    // check trivial cases
    if ( p0 == p1 )
        return Hop_Not(p->pConst1);
    if ( p0 == Hop_Not(p1) )
        return p->pConst1;
    if ( Hop_Regular(p0) == p->pConst1 )
        return Hop_NotCond( p1, p0 == p->pConst1 );
    if ( Hop_Regular(p1) == p->pConst1 )
        return Hop_NotCond( p0, p1 == p->pConst1 );
    // check the table
    pGhost = Hop_ObjCreateGhost( p, p0, p1, AIG_EXOR );
    if ( pResult = Hop_TableLookup( p, pGhost ) )
        return pResult;
    return Hop_ObjCreate( p, pGhost );
*/
    return Hop_Or( p, Hop_And(p, p0, Hop_Not(p1)), Hop_And(p, Hop_Not(p0), p1) );
}

/**Function*************************************************************

  Synopsis    [Implements Boolean OR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Or( Hop_Man_t * p, Hop_Obj_t * p0, Hop_Obj_t * p1 )
{
    return Hop_Not( Hop_And( p, Hop_Not(p0), Hop_Not(p1) ) );
}

/**Function*************************************************************

  Synopsis    [Implements ITE operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Mux( Hop_Man_t * p, Hop_Obj_t * pC, Hop_Obj_t * p1, Hop_Obj_t * p0 )
{
/*    
    Hop_Obj_t * pTempA1, * pTempA2, * pTempB1, * pTempB2, * pTemp;
    int Count0, Count1;
    // consider trivial cases
    if ( p0 == Hop_Not(p1) )
        return Hop_Exor( p, pC, p0 );
    // other cases can be added
    // implement the first MUX (F = C * x1 + C' * x0)

    // check for constants here!!!

    pTempA1 = Hop_TableLookup( p, Hop_ObjCreateGhost(p, pC,          p1, AIG_AND) );
    pTempA2 = Hop_TableLookup( p, Hop_ObjCreateGhost(p, Hop_Not(pC), p0, AIG_AND) );
    if ( pTempA1 && pTempA2 )
    {
        pTemp = Hop_TableLookup( p, Hop_ObjCreateGhost(p, Hop_Not(pTempA1), Hop_Not(pTempA2), AIG_AND) );
        if ( pTemp ) return Hop_Not(pTemp);
    }
    Count0 = (pTempA1 != NULL) + (pTempA2 != NULL);
    // implement the second MUX (F' = C * x1' + C' * x0')
    pTempB1 = Hop_TableLookup( p, Hop_ObjCreateGhost(p, pC,          Hop_Not(p1), AIG_AND) );
    pTempB2 = Hop_TableLookup( p, Hop_ObjCreateGhost(p, Hop_Not(pC), Hop_Not(p0), AIG_AND) );
    if ( pTempB1 && pTempB2 )
    {
        pTemp = Hop_TableLookup( p, Hop_ObjCreateGhost(p, Hop_Not(pTempB1), Hop_Not(pTempB2), AIG_AND) );
        if ( pTemp ) return pTemp;
    }
    Count1 = (pTempB1 != NULL) + (pTempB2 != NULL);
    // compare and decide which one to implement
    if ( Count0 >= Count1 )
    {
        pTempA1 = pTempA1? pTempA1 : Hop_And(p, pC,          p1);
        pTempA2 = pTempA2? pTempA2 : Hop_And(p, Hop_Not(pC), p0);
        return Hop_Or( p, pTempA1, pTempA2 );
    }
    pTempB1 = pTempB1? pTempB1 : Hop_And(p, pC,          Hop_Not(p1));
    pTempB2 = pTempB2? pTempB2 : Hop_And(p, Hop_Not(pC), Hop_Not(p0));
    return Hop_Not( Hop_Or( p, pTempB1, pTempB2 ) );
*/
    return Hop_Or( p, Hop_And(p, pC, p1), Hop_And(p, Hop_Not(pC), p0) );
}

/**Function*************************************************************

  Synopsis    [Implements ITE operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Maj( Hop_Man_t * p, Hop_Obj_t * pA, Hop_Obj_t * pB, Hop_Obj_t * pC )
{
    return Hop_Or( p, Hop_Or(p, Hop_And(p, pA, pB), Hop_And(p, pA, pC)), Hop_And(p, pB, pC) );
}

/**Function*************************************************************

  Synopsis    [Constructs the well-balanced tree of gates.]

  Description [Disregards levels and possible logic sharing.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Multi_rec( Hop_Man_t * p, Hop_Obj_t ** ppObjs, int nObjs, Hop_Type_t Type )
{
    Hop_Obj_t * pObj1, * pObj2;
    if ( nObjs == 1 )
        return ppObjs[0];
    pObj1 = Hop_Multi_rec( p, ppObjs,           nObjs/2,         Type );
    pObj2 = Hop_Multi_rec( p, ppObjs + nObjs/2, nObjs - nObjs/2, Type );
    return Hop_Oper( p, pObj1, pObj2, Type );
}

/**Function*************************************************************

  Synopsis    [Old code.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Multi( Hop_Man_t * p, Hop_Obj_t ** pArgs, int nArgs, Hop_Type_t Type )
{
    assert( Type == AIG_AND || Type == AIG_EXOR );
    assert( nArgs > 0 );
    return Hop_Multi_rec( p, pArgs, nArgs, Type );
}

/**Function*************************************************************

  Synopsis    [Implements the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Miter( Hop_Man_t * p, Vec_Ptr_t * vPairs )
{
    int i;
    assert( vPairs->nSize > 0 );
    assert( vPairs->nSize % 2 == 0 );
    // go through the cubes of the node's SOP
    for ( i = 0; i < vPairs->nSize; i += 2 )
        vPairs->pArray[i/2] = Hop_Not( Hop_Exor( p, (Hop_Obj_t *)vPairs->pArray[i], (Hop_Obj_t *)vPairs->pArray[i+1] ) );
    vPairs->nSize = vPairs->nSize/2;
    return Hop_Not( Hop_Multi_rec( p, (Hop_Obj_t **)vPairs->pArray, vPairs->nSize, AIG_AND ) );
}

/**Function*************************************************************

  Synopsis    [Creates AND function with nVars inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_CreateAnd( Hop_Man_t * p, int nVars )
{
    Hop_Obj_t * pFunc;
    int i;
    pFunc = Hop_ManConst1( p );
    for ( i = 0; i < nVars; i++ )
        pFunc = Hop_And( p, pFunc, Hop_IthVar(p, i) );
    return pFunc;
}

/**Function*************************************************************

  Synopsis    [Creates AND function with nVars inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_CreateOr( Hop_Man_t * p, int nVars )
{
    Hop_Obj_t * pFunc;
    int i;
    pFunc = Hop_ManConst0( p );
    for ( i = 0; i < nVars; i++ )
        pFunc = Hop_Or( p, pFunc, Hop_IthVar(p, i) );
    return pFunc;
}

/**Function*************************************************************

  Synopsis    [Creates AND function with nVars inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_CreateExor( Hop_Man_t * p, int nVars )
{
    Hop_Obj_t * pFunc;
    int i;
    pFunc = Hop_ManConst0( p );
    for ( i = 0; i < nVars; i++ )
        pFunc = Hop_Exor( p, pFunc, Hop_IthVar(p, i) );
    return pFunc;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

