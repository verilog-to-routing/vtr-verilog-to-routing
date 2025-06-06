/**CFile****************************************************************

  FileName    [giaHash.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Structural hashing.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaHash.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Returns the place where this node is stored (or should be stored).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Gia_ManHashOne( int iLit0, int iLit1, int iLitC, int TableSize ) 
{
    unsigned Key = iLitC * 2011;
    Key += Abc_Lit2Var(iLit0) * 7937;
    Key += Abc_Lit2Var(iLit1) * 2971;
    Key += Abc_LitIsCompl(iLit0) * 911;
    Key += Abc_LitIsCompl(iLit1) * 353;
    return (int)(Key % TableSize);
}
static inline int * Gia_ManHashFind( Gia_Man_t * p, int iLit0, int iLit1, int iLitC )
{
    int iThis, * pPlace = Vec_IntEntryP( &p->vHTable, Gia_ManHashOne( iLit0, iLit1, iLitC, Vec_IntSize(&p->vHTable) ) );
    assert( Vec_IntSize(&p->vHash) == Gia_ManObjNum(p) );
    assert( p->pMuxes || iLit0 < iLit1 );
    assert( iLit0 < iLit1 || (!Abc_LitIsCompl(iLit0) && !Abc_LitIsCompl(iLit1)) );
    assert( iLitC == -1 || !Abc_LitIsCompl(iLit1) );
    for ( ; (iThis = *pPlace); pPlace = Vec_IntEntryP(&p->vHash, iThis) )
    {
        Gia_Obj_t * pThis = Gia_ManObj( p, iThis );
        if ( Gia_ObjFaninLit0(pThis, iThis) == iLit0 && Gia_ObjFaninLit1(pThis, iThis) == iLit1 && (p->pMuxes == NULL || Gia_ObjFaninLit2p(p, pThis) == iLitC) )
            break;
    }
    return pPlace;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHashLookupInt( Gia_Man_t * p, int iLit0, int iLit1 )
{
    if ( iLit0 > iLit1 )
        iLit0 ^= iLit1, iLit1 ^= iLit0, iLit0 ^= iLit1;
    return Abc_Var2Lit( *Gia_ManHashFind( p, iLit0, iLit1, -1 ), 0 );
}
int Gia_ManHashLookup( Gia_Man_t * p, Gia_Obj_t * p0, Gia_Obj_t * p1 )
{
    int iLit0 = Gia_ObjToLit( p, p0 );
    int iLit1 = Gia_ObjToLit( p, p1 );
    return Gia_ManHashLookupInt( p, iLit0, iLit1 );
}

/**Function*************************************************************

  Synopsis    [Starts the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManHashAlloc( Gia_Man_t * p )  
{
    assert( Vec_IntSize(&p->vHTable) == 0 );
    Vec_IntFill( &p->vHTable, Abc_PrimeCudd( Gia_ManAndNum(p) ? Gia_ManAndNum(p) + 1000 : p->nObjsAlloc ), 0 );
    Vec_IntGrow( &p->vHash, Abc_MaxInt(Vec_IntSize(&p->vHTable), Gia_ManObjNum(p)) );
    Vec_IntFill( &p->vHash, Gia_ManObjNum(p), 0 );
//printf( "Alloced table with %d entries.\n", Vec_IntSize(&p->vHTable) );
}

/**Function*************************************************************

  Synopsis    [Starts the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManHashStart( Gia_Man_t * p )  
{
    Gia_Obj_t * pObj;
    int * pPlace, i;
    Gia_ManHashAlloc( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        pPlace = Gia_ManHashFind( p, Gia_ObjFaninLit0(pObj, i), Gia_ObjFaninLit1(pObj, i), Gia_ObjFaninLit2(p, i) );
        assert( *pPlace == 0 );
        *pPlace = i;
    }
}

/**Function*************************************************************

  Synopsis    [Stops the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManHashStop( Gia_Man_t * p )  
{
    Vec_IntErase( &p->vHTable );
    Vec_IntErase( &p->vHash );
}

/**Function*************************************************************

  Synopsis    [Resizes the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManHashResize( Gia_Man_t * p )
{
    int i, iThis, iNext, Counter, Counter2, * pPlace;
    Vec_Int_t vOld = p->vHTable;
    assert( Vec_IntSize(&vOld) > 0 );
    // replace the table
    Vec_IntZero( &p->vHTable );
    Vec_IntFill( &p->vHTable, Abc_PrimeCudd( 2 * Gia_ManAndNum(p) ), 0 ); 
    // rehash the entries from the old table
    Counter = 0;
    Vec_IntForEachEntry( &vOld, iThis, i )
        for ( iNext = Vec_IntEntry(&p->vHash, iThis);  
              iThis;  iThis = iNext,   
              iNext = Vec_IntEntry(&p->vHash, iThis)  )
        {
            Gia_Obj_t * pThis0 = Gia_ManObj( p, iThis );
            Vec_IntWriteEntry( &p->vHash, iThis, 0 );
            pPlace = Gia_ManHashFind( p, Gia_ObjFaninLit0(pThis0, iThis), Gia_ObjFaninLit1(pThis0, iThis), Gia_ObjFaninLit2p(p, pThis0) );
            assert( *pPlace == 0 ); // should not be there
            *pPlace = iThis;
            assert( *pPlace != 0 );
            Counter++;
        }
    Counter2 = Gia_ManAndNum(p) - Gia_ManBufNum(p);
    assert( Counter == Counter2 );
//    if ( p->fVerbose )
//        printf( "Resizing GIA hash table: %d -> %d.\n", Vec_IntSize(&vOld), Vec_IntSize(&p->vHTable) );
    Vec_IntErase( &vOld );
}

/**Function********************************************************************

  Synopsis    [Profiles the hash table.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Gia_ManHashProfile( Gia_Man_t * p )
{
    int iEntry;
    int i, Counter, Limit;
    printf( "Table size = %d. Entries = %d. ", Vec_IntSize(&p->vHTable), Gia_ManAndNum(p) );
    printf( "Hits = %d. Misses = %d.\n", (int)p->nHashHit, (int)p->nHashMiss );
    Limit = Abc_MinInt( 1000, Vec_IntSize(&p->vHTable) );
    for ( i = 0; i < Limit; i++ )
    {
        Counter = 0;
        for ( iEntry = Vec_IntEntry(&p->vHTable, i); 
              iEntry; 
              iEntry = iEntry? Vec_IntEntry(&p->vHash, iEntry) : 0 )
            Counter++;
        if ( Counter ) 
            printf( "%d ", Counter );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Recognizes what nodes are control and data inputs of a MUX.]

  Description [If the node is a MUX, returns the control variable C.
  Assigns nodes T and E to be the then and else variables of the MUX. 
  Node C is never complemented. Nodes T and E can be complemented.
  This function also recognizes EXOR/NEXOR gates as MUXes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Gia_ObjRecognizeMuxTwo( Gia_Obj_t * pNode0, Gia_Obj_t * pNode1, Gia_Obj_t ** ppNodeT, Gia_Obj_t ** ppNodeE )
{
    assert( !Gia_IsComplement(pNode0) );
    assert( !Gia_IsComplement(pNode1) );
    // find the control variable
    if ( Gia_ObjFanin1(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC1(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p2) )
        if ( Gia_ObjFaninC1(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            return Gia_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            return Gia_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    else if ( Gia_ObjFanin0(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC0(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p1) )
        if ( Gia_ObjFaninC0(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            return Gia_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            return Gia_ObjChild0(pNode0);//pNode1->p1;
        }
    }
    else if ( Gia_ObjFanin0(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC1(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p1) )
        if ( Gia_ObjFaninC0(pNode0) )
        { // pNode2->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            return Gia_ObjChild1(pNode1);//pNode2->p2;
        }
        else
        { // pNode1->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode0));//pNode1->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode1));//pNode2->p1);
            return Gia_ObjChild0(pNode0);//pNode1->p1;
        }
    }
    else if ( Gia_ObjFanin1(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC0(pNode1)) )
    {
//        if ( FrGia_IsComplement(pNode1->p2) )
        if ( Gia_ObjFaninC1(pNode0) )
        { // pNode2->p1 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            *ppNodeE = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            return Gia_ObjChild0(pNode1);//pNode2->p1;
        }
        else
        { // pNode1->p2 is positive phase of C
            *ppNodeT = Gia_Not(Gia_ObjChild0(pNode0));//pNode1->p1);
            *ppNodeE = Gia_Not(Gia_ObjChild1(pNode1));//pNode2->p2);
            return Gia_ObjChild1(pNode0);//pNode1->p2;
        }
    }
    assert( 0 ); // this is not MUX
    return NULL;
}


/**Function*************************************************************

  Synopsis    [Rehashes AIG with mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Gia_ManHashAndP( Gia_Man_t * p, Gia_Obj_t * p0, Gia_Obj_t * p1 )  
{ 
    return Gia_ObjFromLit( p, Gia_ManHashAnd( p, Gia_ObjToLit(p, p0), Gia_ObjToLit(p, p1) ) );
}

/**Function*************************************************************

  Synopsis    [Rehashes AIG with mapping.]

  Description [http://fmv.jku.at/papers/BrummayerBiere-MEMICS06.pdf]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Gia_Obj_t * Gia_ManAddStrash( Gia_Man_t * p, Gia_Obj_t * p0, Gia_Obj_t * p1 )  
{ 
    Gia_Obj_t * pNode0, * pNode1, * pFanA, * pFanB, * pFanC, * pFanD;
    assert( p->fAddStrash );
    pNode0 = Gia_Regular(p0);
    pNode1 = Gia_Regular(p1);
    if ( !Gia_ObjIsAnd(pNode0) && !Gia_ObjIsAnd(pNode1) )
        return NULL;
    pFanA = Gia_ObjIsAnd(pNode0) ? Gia_ObjChild0(pNode0) : NULL;
    pFanB = Gia_ObjIsAnd(pNode0) ? Gia_ObjChild1(pNode0) : NULL;
    pFanC = Gia_ObjIsAnd(pNode1) ? Gia_ObjChild0(pNode1) : NULL;
    pFanD = Gia_ObjIsAnd(pNode1) ? Gia_ObjChild1(pNode1) : NULL;
    if ( Gia_IsComplement(p0) )
    {
        if ( pFanA == Gia_Not(p1) || pFanB == Gia_Not(p1) )
            return p1;
        if ( pFanB == p1 )
            return Gia_ManHashAndP( p, Gia_Not(pFanA), pFanB );
        if ( pFanA == p1 )
            return Gia_ManHashAndP( p, Gia_Not(pFanB), pFanA );
    }
    else
    {
        if ( pFanA == Gia_Not(p1) || pFanB == Gia_Not(p1) )
            return Gia_ManConst0(p);
        if ( pFanA == p1 || pFanB == p1 )
            return p0;
    }
    if ( Gia_IsComplement(p1) )
    {
        if ( pFanC == Gia_Not(p0) || pFanD == Gia_Not(p0) )
            return p0;
        if ( pFanD == p0 )
            return Gia_ManHashAndP( p, Gia_Not(pFanC), pFanD );
        if ( pFanC == p0 )
            return Gia_ManHashAndP( p, Gia_Not(pFanD), pFanC );
    }
    else
    {
        if ( pFanC == Gia_Not(p0) || pFanD == Gia_Not(p0) )
            return Gia_ManConst0(p);
        if ( pFanC == p0 || pFanD == p0 )
            return p1;
    }
    if ( !Gia_IsComplement(p0) && !Gia_IsComplement(p1) ) 
    {
        if ( pFanA == Gia_Not(pFanC) || pFanA == Gia_Not(pFanD) || pFanB == Gia_Not(pFanC) || pFanB == Gia_Not(pFanD) )
            return Gia_ManConst0(p);
        if ( pFanA == pFanC || pFanB == pFanC )
            return Gia_ManHashAndP( p, p0, pFanD );
        if ( pFanB == pFanC || pFanB == pFanD )
            return Gia_ManHashAndP( p, pFanA, p1 );
        if ( pFanA == pFanD || pFanB == pFanD )
            return Gia_ManHashAndP( p, p0, pFanC );
        if ( pFanA == pFanC || pFanA == pFanD )
            return Gia_ManHashAndP( p, pFanB, p1 );
    }
    else if ( Gia_IsComplement(p0) && !Gia_IsComplement(p1) )
    {
        if ( pFanA == Gia_Not(pFanC) || pFanA == Gia_Not(pFanD) || pFanB == Gia_Not(pFanC) || pFanB == Gia_Not(pFanD) )
            return p1;
        if ( pFanB == pFanC || pFanB == pFanD )
            return Gia_ManHashAndP( p, Gia_Not(pFanA), p1 );
        if ( pFanA == pFanC || pFanA == pFanD )
            return Gia_ManHashAndP( p, Gia_Not(pFanB), p1 );
    }
    else if ( !Gia_IsComplement(p0) && Gia_IsComplement(p1) )
    {
        if ( pFanC == Gia_Not(pFanA) || pFanC == Gia_Not(pFanB) || pFanD == Gia_Not(pFanA) || pFanD == Gia_Not(pFanB) )
            return p0;
        if ( pFanD == pFanA || pFanD == pFanB )
            return Gia_ManHashAndP( p, Gia_Not(pFanC), p0 );
        if ( pFanC == pFanA || pFanC == pFanB )
            return Gia_ManHashAndP( p, Gia_Not(pFanD), p0 );
    }
    else // if ( Gia_IsComplement(p0) && Gia_IsComplement(p1) )
    {
        if ( pFanA == pFanD && pFanB == Gia_Not(pFanC) )
            return Gia_Not(pFanA);
        if ( pFanB == pFanC && pFanA == Gia_Not(pFanD) )
            return Gia_Not(pFanB);
        if ( pFanA == pFanC && pFanB == Gia_Not(pFanD) )
            return Gia_Not(pFanA);
        if ( pFanB == pFanD && pFanA == Gia_Not(pFanC) )
            return Gia_Not(pFanB);
    }
/*
    if ( !Gia_IsComplement(p0) || !Gia_IsComplement(p1) )
        return NULL;
    if ( !Gia_ObjIsAnd(pNode0) || !Gia_ObjIsAnd(pNode1) )
        return NULL;
    if ( (Gia_ObjFanin0(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC0(pNode1))) || 
         (Gia_ObjFanin0(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC0(pNode0) ^ Gia_ObjFaninC1(pNode1))) ||
         (Gia_ObjFanin1(pNode0) == Gia_ObjFanin0(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC0(pNode1))) ||
         (Gia_ObjFanin1(pNode0) == Gia_ObjFanin1(pNode1) && (Gia_ObjFaninC1(pNode0) ^ Gia_ObjFaninC1(pNode1))) )
    {
        Gia_Obj_t * pNodeC, * pNodeT, * pNodeE;
        int fCompl;
        pNodeC = Gia_ObjRecognizeMuxTwo( pNode0, pNode1, &pNodeT, &pNodeE );
        // using non-standard canonical rule for MUX (d0 is not compl; d1 may be compl)
        if ( (fCompl = Gia_IsComplement(pNodeE)) )
        {
            pNodeE = Gia_Not(pNodeE);
            pNodeT = Gia_Not(pNodeT);
        }
        pNode0 = Gia_ManHashAndP( p, Gia_Not(pNodeC), pNodeE );
        pNode1 = Gia_ManHashAndP( p, pNodeC,          pNodeT );
        p->fAddStrash = 0;
        pNodeC = Gia_NotCond( Gia_ManHashAndP( p, Gia_Not(pNode0), Gia_Not(pNode1) ), !fCompl );
        p->fAddStrash = 1;
        return pNodeC;
    }
*/
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Hashes XOR gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHashXorReal( Gia_Man_t * p, int iLit0, int iLit1 )  
{ 
    int fCompl = 0;
    assert( p->fAddStrash == 0 );
    if ( iLit0 < 2 )
        return iLit0 ? Abc_LitNot(iLit1) : iLit1;
    if ( iLit1 < 2 )
        return iLit1 ? Abc_LitNot(iLit0) : iLit0;
    if ( iLit0 == iLit1 )
        return 0;
    if ( iLit0 == Abc_LitNot(iLit1) )
        return 1;
    if ( (p->nObjs & 0xFF) == 0 && 2 * Vec_IntSize(&p->vHTable) < Gia_ManAndNum(p) )
        Gia_ManHashResize( p );
    if ( iLit0 < iLit1 )
        iLit0 ^= iLit1, iLit1 ^= iLit0, iLit0 ^= iLit1;
    if ( Abc_LitIsCompl(iLit0) )
        iLit0 = Abc_LitNot(iLit0), fCompl ^= 1;
    if ( Abc_LitIsCompl(iLit1) )
        iLit1 = Abc_LitNot(iLit1), fCompl ^= 1;
    {
        int *pPlace = Gia_ManHashFind( p, iLit0, iLit1, -1 );
        if ( *pPlace )
        {
            p->nHashHit++;
            return Abc_Var2Lit( *pPlace, fCompl );
        }
        p->nHashMiss++;
        if ( Vec_IntSize(&p->vHash) < Vec_IntCap(&p->vHash) )
            *pPlace = Abc_Lit2Var( Gia_ManAppendXorReal( p, iLit0, iLit1 ) );
        else
        {
            int iNode = Gia_ManAppendXorReal( p, iLit0, iLit1 );
            pPlace = Gia_ManHashFind( p, iLit0, iLit1, -1 );
            assert( *pPlace == 0 );
            *pPlace = Abc_Lit2Var( iNode );
        }
        return Abc_Var2Lit( *pPlace, fCompl );
    }
}

/**Function*************************************************************

  Synopsis    [Hashes MUX gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHashMuxReal( Gia_Man_t * p, int iLitC, int iLit1, int iLit0 )  
{
    int fCompl = 0;
    assert( p->fAddStrash == 0 );
    if ( iLitC < 2 )
        return iLitC ? iLit1 : iLit0;
    if ( iLit0 < 2 )
        return iLit0 ? Gia_ManHashOr(p, Abc_LitNot(iLitC), iLit1) : Gia_ManHashAnd(p, iLitC, iLit1);
    if ( iLit1 < 2 )
        return iLit1 ? Gia_ManHashOr(p, iLitC, iLit0) : Gia_ManHashAnd(p, Abc_LitNot(iLitC), iLit0);
    assert( iLit0 > 1 && iLit1 > 1 && iLitC > 1 );
    if ( iLit0 == iLit1 )
        return iLit0;
    if ( iLitC == iLit0 || iLitC == Abc_LitNot(iLit1) )
        return Gia_ManHashAnd(p, iLit0, iLit1);
    if ( iLitC == iLit1 || iLitC == Abc_LitNot(iLit0) )
        return Gia_ManHashOr(p, iLit0, iLit1);
    if ( Abc_Lit2Var(iLit0) == Abc_Lit2Var(iLit1) )
        return Gia_ManHashXorReal( p, iLitC, iLit0 );
    if ( iLit0 > iLit1 )
        iLit0 ^= iLit1, iLit1 ^= iLit0, iLit0 ^= iLit1, iLitC = Abc_LitNot(iLitC);
    if ( Abc_LitIsCompl(iLit1) )
        iLit0 = Abc_LitNot(iLit0), iLit1 = Abc_LitNot(iLit1), fCompl = 1;
    {
        int *pPlace = Gia_ManHashFind( p, iLit0, iLit1, iLitC );
        if ( *pPlace )
        {
            p->nHashHit++;
            return Abc_Var2Lit( *pPlace, fCompl );
        }
        p->nHashMiss++;
        if ( Vec_IntSize(&p->vHash) < Vec_IntCap(&p->vHash) )
            *pPlace = Abc_Lit2Var( Gia_ManAppendMuxReal( p, iLitC, iLit1, iLit0 ) );
        else
        {
            int iNode = Gia_ManAppendMuxReal( p, iLitC, iLit1, iLit0 );
            pPlace = Gia_ManHashFind( p, iLit0, iLit1, iLitC );
            assert( *pPlace == 0 );
            *pPlace = Abc_Lit2Var( iNode );
        }
        return Abc_Var2Lit( *pPlace, fCompl );
    }
}

/**Function*************************************************************

  Synopsis    [Hashes AND gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHashAnd( Gia_Man_t * p, int iLit0, int iLit1 )  
{ 
    if ( iLit0 < 2 )
        return iLit0 ? iLit1 : 0;
    if ( iLit1 < 2 )
        return iLit1 ? iLit0 : 0;
    if ( iLit0 == iLit1 )
        return iLit1;
    if ( iLit0 == Abc_LitNot(iLit1) )
        return 0;
    if ( p->fGiaSimple )
    {
        assert( Vec_IntSize(&p->vHTable) == 0 );
        return Gia_ManAppendAnd( p, iLit0, iLit1 );
    }
    if ( (p->nObjs & 0xFF) == 0 && 2 * Vec_IntSize(&p->vHTable) < Gia_ManAndNum(p) )
        Gia_ManHashResize( p );
    if ( p->fAddStrash )
    {
        Gia_Obj_t * pObj = Gia_ManAddStrash( p, Gia_ObjFromLit(p, iLit0), Gia_ObjFromLit(p, iLit1) );
        if ( pObj != NULL )
            return Gia_ObjToLit( p, pObj );
    }
    if ( iLit0 > iLit1 )
        iLit0 ^= iLit1, iLit1 ^= iLit0, iLit0 ^= iLit1;
    {
        int * pPlace = Gia_ManHashFind( p, iLit0, iLit1, -1 );
        if ( *pPlace )
        {
            p->nHashHit++;
            return Abc_Var2Lit( *pPlace, 0 );
        }
        p->nHashMiss++;
        if ( Vec_IntSize(&p->vHash) < Vec_IntCap(&p->vHash) )
            *pPlace = Abc_Lit2Var( Gia_ManAppendAnd( p, iLit0, iLit1 ) );
        else
        {
            int iNode = Gia_ManAppendAnd( p, iLit0, iLit1 );
            pPlace = Gia_ManHashFind( p, iLit0, iLit1, -1 );
            assert( *pPlace == 0 );
            *pPlace = Abc_Lit2Var( iNode );
        }
        return Abc_Var2Lit( *pPlace, 0 );
    }
}
int Gia_ManHashOr( Gia_Man_t * p, int iLit0, int iLit1 )  
{ 
    return Abc_LitNot(Gia_ManHashAnd( p, Abc_LitNot(iLit0), Abc_LitNot(iLit1) ));
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHashAndTry( Gia_Man_t * p, int iLit0, int iLit1 )  
{ 
    if ( iLit0 < 2 )
        return iLit0 ? iLit1 : 0;
    if ( iLit1 < 2 )
        return iLit1 ? iLit0 : 0;
    if ( iLit0 == iLit1 )
        return iLit1;
    if ( iLit0 == Abc_LitNot(iLit1) )
        return 0;
    if ( iLit0 > iLit1 )
        iLit0 ^= iLit1, iLit1 ^= iLit0, iLit0 ^= iLit1;
    {
        int * pPlace = Gia_ManHashFind( p, iLit0, iLit1, -1 );
        if ( *pPlace ) 
            return Abc_Var2Lit( *pPlace, 0 );
        return -1;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHashXor( Gia_Man_t * p, int iLit0, int iLit1 )  
{ 
    if ( p->fGiaSimple )
        return Gia_ManHashOr(p, Gia_ManHashAnd(p, iLit0, Abc_LitNot(iLit1)), Gia_ManHashAnd(p, Abc_LitNot(iLit0), iLit1) );
    else
    {
        int fCompl = Abc_LitIsCompl(iLit0) ^ Abc_LitIsCompl(iLit1);
        int iTemp0 = Gia_ManHashAnd( p, Abc_LitRegular(iLit0), Abc_LitNot(Abc_LitRegular(iLit1)) );
        int iTemp1 = Gia_ManHashAnd( p, Abc_LitRegular(iLit1), Abc_LitNot(Abc_LitRegular(iLit0)) );
        return Abc_LitNotCond( Gia_ManHashAnd( p, Abc_LitNot(iTemp0), Abc_LitNot(iTemp1) ), !fCompl );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHashMux( Gia_Man_t * p, int iCtrl, int iData1, int iData0 )  
{ 
    if ( p->fGiaSimple )
        return Gia_ManHashOr(p, Gia_ManHashAnd(p, iCtrl, iData1), Gia_ManHashAnd(p, Abc_LitNot(iCtrl), iData0) );
    else
    {
        int iTemp0, iTemp1, fCompl = 0;
        if ( iData0 > iData1 )
            iData0 ^= iData1, iData1 ^= iData0, iData0 ^= iData1, iCtrl = Abc_LitNot(iCtrl);
        if ( Abc_LitIsCompl(iData1) )
            iData0 = Abc_LitNot(iData0), iData1 = Abc_LitNot(iData1), fCompl = 1;
        iTemp0 = Gia_ManHashAnd( p, Abc_LitNot(iCtrl), iData0 );
        iTemp1 = Gia_ManHashAnd( p, iCtrl, iData1 );
        return Abc_LitNotCond( Gia_ManHashAnd( p, Abc_LitNot(iTemp0), Abc_LitNot(iTemp1) ), !fCompl );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHashMaj( Gia_Man_t * p, int iData0, int iData1, int iData2 )  
{ 
    int iTemp0 = Gia_ManHashOr( p, iData1, iData2 );
    int iTemp1 = Gia_ManHashAnd( p, iData0, iTemp0 );
    int iTemp2 = Gia_ManHashAnd( p, iData1, iData2 );
    return Gia_ManHashOr( p, iTemp1, iTemp2 );
}

/**Function*************************************************************

  Synopsis    [Rehashes AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManRehash( Gia_Man_t * p, int fAddStrash )  
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->fAddStrash = fAddStrash;
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj( p, pObj, i )
    {
        //if ( Gia_ObjIsBuf(pObj) )
        //    pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) );
        //else 
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManHashStop( pNew );
    pNew->fAddStrash = 0;
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
//    printf( "Top gate is %s\n", Gia_ObjFaninC0(Gia_ManCo(pNew, 0))? "OR" : "AND" );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Creates well-balanced AND gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManHashAndMulti( Gia_Man_t * p, Vec_Int_t * vLits )
{
    if ( Vec_IntSize(vLits) == 0 )
        return 0;
    while ( Vec_IntSize(vLits) > 1 )
    {
        int i, k = 0, Lit1, Lit2, LitRes;
        Vec_IntForEachEntryDouble( vLits, Lit1, Lit2, i )
        {
            LitRes = Gia_ManHashAnd( p, Lit1, Lit2 );
            Vec_IntWriteEntry( vLits, k++, LitRes );
        }
        if ( Vec_IntSize(vLits) & 1 )
            Vec_IntWriteEntry( vLits, k++, Vec_IntEntryLast(vLits) );
        Vec_IntShrink( vLits, k );
    }
    assert( Vec_IntSize(vLits) == 1 );
    return Vec_IntEntry(vLits, 0);
}
int Gia_ManHashAndMulti2( Gia_Man_t * p, Vec_Int_t * vLits )
{
    int i, iLit, iRes = 1;
    Vec_IntForEachEntry( vLits, iLit, i )
        iRes = Gia_ManHashAnd( p, iRes, iLit );
    return iRes;
}
int Gia_ManHashDualMiter( Gia_Man_t * p, Vec_Int_t * vOuts )
{
    int i, iLit0, iLit1, iRes = 0;
    Vec_IntForEachEntryDouble( vOuts, iLit0, iLit1, i )
        iRes = Gia_ManHashOr( p, iRes, Gia_ManHashXor(p, iLit0, iLit1) );
    return iRes;
}



/**Function*************************************************************

  Synopsis    [Create multi-input tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Gia_ManCollectLiterals( int nVars )
{
    int i, * pRes = ABC_CALLOC( int, nVars );
    for ( i = 0; i < nVars; i++ )
        pRes[i] = Abc_Var2Lit( i+1, 0 );
    return pRes;
}
int * Gia_ManGenZero( int nBits )
{
    return ABC_CALLOC( int, nBits );
}
int * Gia_ManGenPerm( int nBits )
{
    int i, * pRes = ABC_CALLOC( int, nBits );
    srand( time(NULL) );
    for ( i = 0; i < nBits; i++ )
        pRes[i] = i;
    for ( i = 0; i < nBits; i++ )
    {
        int iPerm = rand() % nBits;
        ABC_SWAP( int, pRes[i], pRes[iPerm] );
    }
    return pRes;
}
int * Gia_ManGenPerm2( int nBits )
{
    int i, * pRes = ABC_CALLOC( int, nBits );
    srand( time(NULL) );
    for ( i = 0; i < nBits; i++ )
        pRes[i] = rand() % nBits;
    return pRes;
}
int Gia_ManMultiCheck( int * pPerm, int nPerm )
{
    int i;
    for ( i = 1; i < nPerm; i++ )
        if ( pPerm[i-1] <= pPerm[i] )
            return 0;
    return 1;
}
int Gia_ManMultiInputPerm( Gia_Man_t * pNew, int * pVars, int nVars, int * pPerm, int fOr, int fXor )
{
    int fPrint = 1;
    int i, iLit;
    if ( fPrint )
    {
        for ( i = 0; i < nVars; i++ )
            printf( "%d ", pPerm[i] );
        printf( "\n" );
    }
    while ( 1 )
    {
        for ( i = 1; i < nVars; i++ )
            if ( pPerm[i-1] >= pPerm[i] )
                break;
        if ( i == nVars )
            break;
        assert( pPerm[i-1] >= pPerm[i] );
        if ( pPerm[i-1] > pPerm[i] )
        {
            ABC_SWAP( int, pPerm[i-1], pPerm[i] );
            ABC_SWAP( int, pVars[i-1], pVars[i] );
        }
        else
        {
            assert( pPerm[i-1] == pPerm[i] );
            pPerm[i-1]++;
            if ( fXor )
                pVars[i-1] = Gia_ManHashXor( pNew, pVars[i-1], pVars[i] );
            else if ( fOr )
                pVars[i-1] = Gia_ManHashOr( pNew, pVars[i-1], pVars[i] );
            else
                pVars[i-1] = Gia_ManHashAnd( pNew, pVars[i-1], pVars[i] );
            for ( i = i+1; i < nVars; i++ )
            {
                pPerm[i-1] = pPerm[i];
                pVars[i-1] = pVars[i];
            }
            nVars--;
        }
        if ( fPrint )
        {
            for ( i = 0; i < nVars; i++ )
                printf( "%d ", pPerm[i] );
            printf( "\n" );
        }
    }
    iLit = pVars[0];
    for ( i = 1; i < nVars; i++ )
        if ( fXor )
            iLit = Gia_ManHashXor( pNew, iLit, pVars[i] );
        else if ( fOr )
            iLit = Gia_ManHashOr( pNew, iLit, pVars[i] );
        else
            iLit = Gia_ManHashAnd( pNew, iLit, pVars[i] );
    return iLit;
}
Gia_Man_t * Gia_ManMultiInputTest( int nBits )
{
    Gia_Man_t * pNew;
    int i, iRes, * pPerm;
    int * pMulti = Gia_ManCollectLiterals( nBits );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "multi" );
    for ( i = 0; i < nBits; i++ )
        Gia_ManAppendCi( pNew );
    Gia_ManHashAlloc( pNew );
    pPerm = Gia_ManGenPerm2( nBits );
    //pPerm = Gia_ManGenZero( nBits );
    iRes = Gia_ManMultiInputPerm( pNew, pMulti, nBits, pPerm, 0, 0 );
    Gia_ManAppendCo( pNew, iRes );
    ABC_FREE( pPerm );
    ABC_FREE( pMulti );
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Create MUX tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCube( Gia_Man_t * pNew, int Vars, int nVars, int * pLits )
{
    int i, iLit = 1;
    for ( i = 0; i < nVars; i++ )
        iLit = Gia_ManHashAnd( pNew, iLit, Abc_LitNotCond(pLits[i], !((Vars >> i) & 1)) );
    return iLit;
}
int Gia_ManMuxTree_rec( Gia_Man_t * pNew, int * pCtrl, int nCtrl, int * pData )
{
    int iLit0, iLit1;
    if ( nCtrl == 0 )
        return pData[0];
    iLit0 = Gia_ManMuxTree_rec( pNew, pCtrl, nCtrl-1, pData );
    iLit1 = Gia_ManMuxTree_rec( pNew, pCtrl, nCtrl-1, pData + (1<<(nCtrl-1)) );
    return Gia_ManHashMux( pNew, pCtrl[nCtrl-1], iLit1, iLit0 );
}
void Gia_ManUsePerm( int * pTree, int nBits, int * pPerm )
{
    int fPrint = 0;
    int i, k, m, nVars = nBits + (1 << nBits);
    if ( fPrint )
    {
        for ( i = 0; i < nVars; i++ )
            printf( "%d ", pPerm[i] );
        printf( "\n" );
    }
    for ( i = 0; i < nBits; i++ )
    {
        for ( k = i+1; k < nBits; k++ )
            if ( pPerm[i] > pPerm[k] )
                break;
        if ( k == nBits )
            break;
        assert( pPerm[i] > pPerm[k] );
        ABC_SWAP( int, pPerm[i], pPerm[k] );
        ABC_SWAP( int, pTree[i], pTree[k] );
        for ( m = 0; m < (1 << nBits); m++ )
            if ( ((m >> i) & 1) && !((m >> k) & 1) )
            {
                ABC_SWAP( int, pTree[nBits+m], pTree[nBits+(m^(1<<i)^(1<<k))] );
                ABC_SWAP( int, pPerm[nBits+m], pPerm[nBits+(m^(1<<i)^(1<<k))] );
            }
    }
    if ( fPrint )
    {
        for ( i = 0; i < nVars; i++ )
            printf( "%d ", pPerm[i] );
        printf( "\n" );
    }
}
int Gia_ManFindCond( int * pLits, int nBits, int iLate1, int iLate2 )
{
    int i;
    assert( iLate1 != iLate2 );
    for ( i = 0; i < nBits; i++ )
        if ( (((iLate1 ^ iLate2) >> i) & 1) )
            return Abc_LitNotCond( pLits[i], (iLate1 >> i) & 1 );
    return -1;
}
int Gia_ManLatest( int * pPerm, int nVars, int iPrev1, int iPrev2, int iPrev3 )
{
    int i, Value = -1, iLate = -1;
    for ( i = 0; i < nVars; i++ )
        if ( Value < pPerm[i] && i != iPrev1 && i != iPrev2 && i != iPrev3 )
        {
            Value = pPerm[i];
            iLate = i;
        }
    return iLate;
}
int Gia_ManEarliest( int * pPerm, int nVars )
{
    int i, Value = ABC_INFINITY, iLate = -1;
    for ( i = 0; i < nVars; i++ )
        if ( Value > pPerm[i] )
        {
            Value = pPerm[i];
            iLate = i;
        }
    return iLate;
}
int Gia_ManDecompOne( Gia_Man_t * pNew, int * pTree, int nBits, int * pPerm, int iLate )
{
    int iRes, iData;
    assert( iLate >= 0 && iLate < (1<<nBits) );
    iData = pTree[nBits+iLate];
    pTree[nBits+iLate] = pTree[nBits+(iLate^1)];
    iRes = Gia_ManMuxTree_rec( pNew, pTree, nBits, pTree+nBits );
    return Gia_ManHashMux( pNew, Gia_ManCube(pNew, iLate, nBits, pTree), iData, iRes );
}
int Gia_ManDecompTwo( Gia_Man_t * pNew, int * pTree, int nBits, int * pPerm, int iLate1, int iLate2 )
{
    int iRes, iData1, iData2, iData, iCond, iCond2;
    assert( iLate1 != iLate2 );
    assert( iLate1 >= 0 && iLate1 < (1<<nBits) );
    assert( iLate2 >= 0 && iLate2 < (1<<nBits) );
    iData1 = pTree[nBits+iLate1];
    iData2 = pTree[nBits+iLate2];
    pTree[nBits+iLate1] = pTree[nBits+(iLate1^1)];
    pTree[nBits+iLate2] = pTree[nBits+(iLate2^1)];
    iRes   = Gia_ManMuxTree_rec( pNew, pTree, nBits, pTree+nBits );
    iCond  = Gia_ManHashOr( pNew, Gia_ManCube(pNew, iLate1, nBits, pTree), Gia_ManCube(pNew, iLate2, nBits, pTree) ); 
    iCond2 = Gia_ManFindCond( pTree, nBits, iLate1, iLate2 );
    iData  = Gia_ManHashMux( pNew, iCond2, iData2, iData1 );
    return Gia_ManHashMux( pNew, iCond, iData, iRes );
}
int Gia_ManDecompThree( Gia_Man_t * pNew, int * pTree, int nBits, int * pPerm, int iLate1, int iLate2, int iLate3 )
{
    int iRes, iData1, iData2, iData3, iCube1, iCube2, iCube3, iCtrl0, iCtrl1, iMux10, iMux11;
    assert( iLate1 != iLate2 );
    assert( iLate1 != iLate3 );
    assert( iLate2 != iLate3 );
    assert( iLate1 >= 0 && iLate1 < (1<<nBits) );
    assert( iLate2 >= 0 && iLate2 < (1<<nBits) );
    assert( iLate3 >= 0 && iLate3 < (1<<nBits) );
    iData1 = pTree[nBits+iLate1];
    iData2 = pTree[nBits+iLate2];
    iData3 = pTree[nBits+iLate3];
    pTree[nBits+iLate1] = pTree[nBits+(iLate1^1)];
    pTree[nBits+iLate2] = pTree[nBits+(iLate2^1)];
    pTree[nBits+iLate3] = pTree[nBits+(iLate3^1)];
    iRes   = Gia_ManMuxTree_rec( pNew, pTree, nBits, pTree+nBits );
    iCube1 = Gia_ManCube( pNew, iLate1, nBits, pTree );
    iCube2 = Gia_ManCube( pNew, iLate2, nBits, pTree );
    iCube3 = Gia_ManCube( pNew, iLate3, nBits, pTree );
    iCtrl0 = Gia_ManHashOr( pNew, iCube1, iCube3 );
    iCtrl1 = Gia_ManHashOr( pNew, iCube2, iCube3 );
    iMux10 = Gia_ManHashMux( pNew, iCtrl0, iData1, iRes );
    iMux11 = Gia_ManHashMux( pNew, iCtrl0, iData3, iData2 );
    return Gia_ManHashMux( pNew, iCtrl1, iMux11, iMux10 );
}
int Gia_ManDecomp( Gia_Man_t * pNew, int * pTree, int nBits, int * pPerm )
{
    if ( nBits == 2 )
        return Gia_ManMuxTree_rec( pNew, pTree, nBits, pTree+nBits );
    else
    {
        int iBase  = Gia_ManEarliest( pPerm+nBits, 1<<nBits ), BaseValue = pPerm[nBits+iBase];
        int iLate1 = Gia_ManLatest( pPerm+nBits, 1<<nBits, -1, -1, -1 );
        int iLate2 = Gia_ManLatest( pPerm+nBits, 1<<nBits, iLate1, -1, -1 );
        int iLate3 = Gia_ManLatest( pPerm+nBits, 1<<nBits, iLate1, iLate2, -1 );
        int iLate4 = Gia_ManLatest( pPerm+nBits, 1<<nBits, iLate1, iLate2, iLate3 );
        if ( 0 )
        {
            int i;
            for ( i = 0; i < (1<<nBits); i++ )
                printf( "%d ", pPerm[nBits+i] );
            printf( "\n" );
        }
        if ( pPerm[nBits+iLate1] > BaseValue && pPerm[nBits+iLate2] > BaseValue && pPerm[nBits+iLate3] > BaseValue && pPerm[nBits+iLate4] == BaseValue )
            return Gia_ManDecompThree( pNew, pTree, nBits, pPerm, iLate1, iLate2, iLate3 );
        if ( pPerm[nBits+iLate1] > BaseValue && pPerm[nBits+iLate2] > BaseValue && pPerm[nBits+iLate3] == BaseValue )
            return Gia_ManDecompTwo( pNew, pTree, nBits, pPerm, iLate1, iLate2 );
        if ( pPerm[nBits+iLate1] > BaseValue && pPerm[nBits+iLate2] == BaseValue )
            return Gia_ManDecompOne( pNew, pTree, nBits, pPerm, iLate1 );
        return Gia_ManMuxTree_rec( pNew, pTree, nBits, pTree+nBits );
    }
}
Gia_Man_t * Gia_ManMuxTreeTest( int nBits )
{
    Gia_Man_t * pNew;
    int i, iLit, nVars = nBits + (1 << nBits);
    int * pPerm, * pTree = Gia_ManCollectLiterals( nVars );
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "mux_tree" );
    for ( i = 0; i < nVars; i++ )
        Gia_ManAppendCi( pNew );
    Gia_ManHashAlloc( pNew );
    pPerm = Gia_ManGenPerm( nVars );
    //pPerm = Gia_ManGenZero( nVars );
    pPerm[nBits+1] = 100;
    pPerm[nBits+5] = 100;
    pPerm[nBits+4] = 100;
    Gia_ManUsePerm( pTree, nBits, pPerm );
    iLit = Gia_ManDecomp( pNew, pTree, nBits, pPerm );
    Gia_ManAppendCo( pNew, iLit );
    ABC_FREE( pPerm );
    ABC_FREE( pTree );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

