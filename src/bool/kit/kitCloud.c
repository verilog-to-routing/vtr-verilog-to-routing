/**CFile****************************************************************

  FileName    [kitCloud.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Computation kit.]

  Synopsis    [Procedures using BDD package CLOUD.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 6, 2006.]

  Revision    [$Id: kitCloud.c,v 1.00 2006/12/06 00:00:00 alanmi Exp $]

***********************************************************************/

#include "kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// internal representation of the function to be decomposed
typedef struct Kit_Mux_t_ Kit_Mux_t;
struct Kit_Mux_t_
{
    unsigned      v  :  5;          // variable 
    unsigned      t  : 12;          // then edge
    unsigned      e  : 12;          // else edge
    unsigned      c  :  1;          // complemented attr of else edge
    unsigned      i  :  1;          // complemented attr of top node
};

static inline int        Kit_Mux2Int( Kit_Mux_t m )  { union { Kit_Mux_t x; int y; } v; v.x = m; return v.y;  }
static inline Kit_Mux_t  Kit_Int2Mux( int m )        { union { Kit_Mux_t x; int y; } v; v.y = m; return v.x;  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derive BDD from the truth table for 5 variable functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
CloudNode * Kit_TruthToCloud5_rec( CloudManager * dd, unsigned uTruth, int nVars, int nVarsAll )
{
    static unsigned uVars[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    CloudNode * pCof0, * pCof1;
    unsigned uCof0, uCof1;
    assert( nVars <= 5 );
    if ( uTruth == 0 )
        return dd->zero;
    if ( uTruth == ~0 )
        return dd->one;
    if ( nVars == 1 )
    {
        if ( uTruth == uVars[0] )
            return dd->vars[nVarsAll-1];
        if ( uTruth == ~uVars[0] )
            return Cloud_Not(dd->vars[nVarsAll-1]);
        assert( 0 );
    }
//    Count++;
    assert( nVars > 1 );
    uCof0 = uTruth & ~uVars[nVars-1];
    uCof1 = uTruth &  uVars[nVars-1];
    uCof0 |= uCof0 << (1<<(nVars-1));
    uCof1 |= uCof1 >> (1<<(nVars-1));
    if ( uCof0 == uCof1 )
        return Kit_TruthToCloud5_rec( dd, uCof0, nVars - 1, nVarsAll );
    if ( uCof0 == ~uCof1 )
    {
        pCof0 = Kit_TruthToCloud5_rec( dd, uCof0, nVars - 1, nVarsAll );
        pCof1 = Cloud_Not( pCof0 );
    }
    else
    {
        pCof0 = Kit_TruthToCloud5_rec( dd, uCof0, nVars - 1, nVarsAll );
        pCof1 = Kit_TruthToCloud5_rec( dd, uCof1, nVars - 1, nVarsAll );
    }
    return Cloud_MakeNode( dd, nVarsAll - nVars, pCof1, pCof0 );
}

/**Function********************************************************************

  Synopsis    [Compute BDD for the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
CloudNode * Kit_TruthToCloud_rec( CloudManager * dd, unsigned * pTruth, int nVars, int nVarsAll )
{
    CloudNode * pCof0, * pCof1;
    unsigned * pTruth0, * pTruth1;
    if ( nVars <= 5 )
        return Kit_TruthToCloud5_rec( dd, pTruth[0], nVars, nVarsAll );
    if ( Kit_TruthIsConst0(pTruth, nVars) )
        return dd->zero;
    if ( Kit_TruthIsConst1(pTruth, nVars) )
        return dd->one;
//    Count++;
    pTruth0 = pTruth;
    pTruth1 = pTruth + Kit_TruthWordNum(nVars-1);
    if ( Kit_TruthIsEqual( pTruth0, pTruth1, nVars - 1 ) )
        return Kit_TruthToCloud_rec( dd, pTruth0, nVars - 1, nVarsAll );
    if ( Kit_TruthIsOpposite( pTruth0, pTruth1, nVars - 1 ) )
    {
        pCof0 = Kit_TruthToCloud_rec( dd, pTruth0, nVars - 1, nVarsAll );
        pCof1 = Cloud_Not( pCof0 );
    }
    else
    {
        pCof0 = Kit_TruthToCloud_rec( dd, pTruth0, nVars - 1, nVarsAll );
        pCof1 = Kit_TruthToCloud_rec( dd, pTruth1, nVars - 1, nVarsAll );
    }
    return Cloud_MakeNode( dd, nVarsAll - nVars, pCof1, pCof0 );
}

/**Function********************************************************************

  Synopsis    [Compute BDD for the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
CloudNode * Kit_TruthToCloud( CloudManager * dd, unsigned * pTruth, int nVars )
{
    CloudNode * pRes;
    pRes = Kit_TruthToCloud_rec( dd, pTruth, nVars, nVars );
//    printf( "%d/%d  ", Count, Cloud_DagSize(dd, pRes) );
    return pRes;
}

/**Function********************************************************************

  Synopsis    [Transforms the array of BDDs into the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
int Kit_CreateCloud( CloudManager * dd, CloudNode * pFunc, Vec_Int_t * vNodes )
{
    Kit_Mux_t Mux;
    int nNodes, i;
    // collect BDD nodes
    nNodes = Cloud_DagCollect( dd, pFunc );
    if ( nNodes >= (1<<12) ) // because in Kit_Mux_t edge is 12 bit
        return 0;
    assert( nNodes == Cloud_DagSize( dd, pFunc ) );
    assert( nNodes < dd->nNodesLimit );
    Vec_IntClear( vNodes );
    Vec_IntPush( vNodes, 0 ); // const1 node
    dd->ppNodes[0]->s = 0;
    for ( i = 1; i < nNodes; i++ )
    {
        dd->ppNodes[i]->s = i;
        Mux.v = dd->ppNodes[i]->v;
        Mux.t = dd->ppNodes[i]->t->s;
        Mux.e = Cloud_Regular(dd->ppNodes[i]->e)->s;
        Mux.c = Cloud_IsComplement(dd->ppNodes[i]->e); 
        Mux.i = (i == nNodes - 1)? Cloud_IsComplement(pFunc) : 0;
        // put the MUX into the array
        Vec_IntPush( vNodes, Kit_Mux2Int(Mux) );
    }
    assert( Vec_IntSize(vNodes) == nNodes );
    // reset signatures
    for ( i = 0; i < nNodes; i++ )
        dd->ppNodes[i]->s = dd->nSignCur;
    return 1;
}

/**Function********************************************************************

  Synopsis    [Transforms the array of BDDs into the integer array.]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
int Kit_CreateCloudFromTruth( CloudManager * dd, unsigned * pTruth, int nVars, Vec_Int_t * vNodes )
{
    CloudNode * pFunc;
    Cloud_Restart( dd );
    pFunc = Kit_TruthToCloud( dd, pTruth, nVars );
    Vec_IntClear( vNodes );
    return Kit_CreateCloud( dd, pFunc, vNodes );
}

/**Function*************************************************************

  Synopsis    [Computes composition of truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Kit_CloudToTruth( Vec_Int_t * vNodes, int nVars, Vec_Ptr_t * vStore, int fInv )
{
    unsigned * pThis, * pFan0, * pFan1;
    Kit_Mux_t Mux;
    int i, Entry;
    assert( Vec_IntSize(vNodes) <= Vec_PtrSize(vStore) );
    pThis = (unsigned *)Vec_PtrEntry( vStore, 0 );
    Kit_TruthFill( pThis, nVars );
    Vec_IntForEachEntryStart( vNodes, Entry, i, 1 )
    {
        Mux = Kit_Int2Mux(Entry);
        assert( (int)Mux.e < i && (int)Mux.t < i && (int)Mux.v < nVars );          
        pFan0 = (unsigned *)Vec_PtrEntry( vStore, Mux.e );
        pFan1 = (unsigned *)Vec_PtrEntry( vStore, Mux.t );
        pThis = (unsigned *)Vec_PtrEntry( vStore, i );
        Kit_TruthMuxVarPhase( pThis, pFan0, pFan1, nVars, fInv? Mux.v : nVars-1-Mux.v, Mux.c );
    } 
    // complement the result
    if ( Mux.i )
        Kit_TruthNot( pThis, pThis, nVars );
    return pThis;
}

/**Function*************************************************************

  Synopsis    [Computes composition of truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Kit_TruthCompose( CloudManager * dd, unsigned * pTruth, int nVars, 
    unsigned ** pInputs, int nVarsAll, Vec_Ptr_t * vStore, Vec_Int_t * vNodes )
{
    CloudNode * pFunc;
    unsigned * pThis, * pFan0, * pFan1;
    Kit_Mux_t Mux;
    int i, Entry, RetValue;
    // derive BDD from truth table
    Cloud_Restart( dd );
    pFunc = Kit_TruthToCloud( dd, pTruth, nVars );
    // convert it into nodes
    RetValue = Kit_CreateCloud( dd, pFunc, vNodes );
    if ( RetValue == 0 )
        printf( "Kit_TruthCompose(): Internal failure!!!\n" );
    // verify the result
//    pFan0 = Kit_CloudToTruth( vNodes, nVars, vStore, 0 );
//    if ( !Kit_TruthIsEqual( pTruth, pFan0, nVars ) )
//        printf( "Failed!\n" );
    // compute truth table from the BDD
    assert( Vec_IntSize(vNodes) <= Vec_PtrSize(vStore) );
    pThis = (unsigned *)Vec_PtrEntry( vStore, 0 );
    Kit_TruthFill( pThis, nVarsAll );
    Vec_IntForEachEntryStart( vNodes, Entry, i, 1 )
    {
        Mux = Kit_Int2Mux(Entry);
        pFan0 = (unsigned *)Vec_PtrEntry( vStore, Mux.e );
        pFan1 = (unsigned *)Vec_PtrEntry( vStore, Mux.t );
        pThis = (unsigned *)Vec_PtrEntry( vStore, i );
        Kit_TruthMuxPhase( pThis, pFan0, pFan1, pInputs[nVars-1-Mux.v], nVarsAll, Mux.c );
    }
    // complement the result
    if ( Mux.i )
        Kit_TruthNot( pThis, pThis, nVarsAll );
    return pThis;
}

/**Function********************************************************************

  Synopsis    [Compute BDD for the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

******************************************************************************/
void Kit_TruthCofSupports( Vec_Int_t * vBddDir, Vec_Int_t * vBddInv, int nVars, Vec_Int_t * vMemory, unsigned * puSupps )
{
    Kit_Mux_t Mux;
    unsigned * puSuppAll;
    unsigned * pThis = NULL; // Suppress "might be used uninitialized"
    unsigned * pFan0, * pFan1;
    int i, v, Var, Entry, nSupps;
    nSupps = 2 * nVars;

    // extend storage
    if ( Vec_IntSize( vMemory ) < nSupps * Vec_IntSize(vBddDir) )
        Vec_IntGrow( vMemory, nSupps * Vec_IntSize(vBddDir) );
    puSuppAll = (unsigned *)Vec_IntArray( vMemory );
    // clear storage for the const node
    memset( puSuppAll, 0, sizeof(unsigned) * nSupps );
    // compute supports from nodes
    Vec_IntForEachEntryStart( vBddDir, Entry, i, 1 )
    {
        Mux = Kit_Int2Mux(Entry);
        Var = nVars - 1 - Mux.v;
        pFan0 = puSuppAll + nSupps * Mux.e;
        pFan1 = puSuppAll + nSupps * Mux.t;
        pThis = puSuppAll + nSupps * i;
        for ( v = 0; v < nSupps; v++ )
            pThis[v] = pFan0[v] | pFan1[v] | (1<<Var);
        assert( pFan0[2*Var + 0] == pFan0[2*Var + 1] );
        assert( pFan1[2*Var + 0] == pFan1[2*Var + 1] );
        pThis[2*Var + 0] = pFan0[2*Var + 0];// | pFan0[2*Var + 1];
        pThis[2*Var + 1] = pFan1[2*Var + 0];// | pFan1[2*Var + 1];
    }
    // copy the result
    memcpy( puSupps, pThis, sizeof(unsigned) * nSupps );
    // compute the inverse

    // extend storage
    if ( Vec_IntSize( vMemory ) < nSupps * Vec_IntSize(vBddInv) )
        Vec_IntGrow( vMemory, nSupps * Vec_IntSize(vBddInv) );
    puSuppAll = (unsigned *)Vec_IntArray( vMemory );
    // clear storage for the const node
    memset( puSuppAll, 0, sizeof(unsigned) * nSupps );
    // compute supports from nodes
    Vec_IntForEachEntryStart( vBddInv, Entry, i, 1 )
    {
        Mux = Kit_Int2Mux(Entry);
//        Var = nVars - 1 - Mux.v;
        Var = Mux.v;
        pFan0 = puSuppAll + nSupps * Mux.e;
        pFan1 = puSuppAll + nSupps * Mux.t;
        pThis = puSuppAll + nSupps * i;
        for ( v = 0; v < nSupps; v++ )
            pThis[v] = pFan0[v] | pFan1[v] | (1<<Var);
        assert( pFan0[2*Var + 0] == pFan0[2*Var + 1] );
        assert( pFan1[2*Var + 0] == pFan1[2*Var + 1] );
        pThis[2*Var + 0] = pFan0[2*Var + 0];// | pFan0[2*Var + 1];
        pThis[2*Var + 1] = pFan1[2*Var + 0];// | pFan1[2*Var + 1];
    }

    // merge supports
    for ( Var = 0; Var < nSupps; Var++ )
        puSupps[Var] = (puSupps[Var] & Kit_BitMask(Var/2)) | (pThis[Var] & ~Kit_BitMask(Var/2));
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

