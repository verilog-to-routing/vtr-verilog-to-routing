/**CFile****************************************************************

  FileName    [acbPush.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Implementation of logic pushing.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 21, 2015.]

  Revision    [$Id: acbPush.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acb.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Pushing logic to the fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_ObjPushToFanout( Acb_Ntk_t * p, int iObj, int iFaninIndex, int iFanout )
{
    word c0, uTruthObjNew = 0, uTruthObj = Acb_ObjTruth( p, iObj ), Gate;
    word c1, uTruthFanNew = 0, uTruthFan = Acb_ObjTruth( p, iFanout );
    int DecType = Abc_Tt6CheckOutDec( uTruthObj, iFaninIndex, &uTruthObjNew );
    int iFanin = Acb_ObjFanin( p, iObj, iFaninIndex );
    int iFanoutObjIndex = Acb_ObjWhatFanin( p, iFanout, iObj );
    int iFanoutFaninIndex = Acb_ObjWhatFanin( p, iFanout, iFanin );
    if ( iFanoutFaninIndex == -1 )
        iFanoutFaninIndex = Acb_ObjFaninNum(p, iFanout); 
    assert( !Acb_ObjIsCio(p, iObj) );
    assert( !Acb_ObjIsCio(p, iFanout) );
    assert( iFanoutFaninIndex >= 0 );
    assert( iFaninIndex < Acb_ObjFaninNum(p, iObj) );
    assert( Acb_ObjFanoutNum(p, iObj) == 1 );
    // compute new function of the fanout
    c0 = Abc_Tt6Cofactor0( uTruthFan, iFanoutObjIndex );
    c1 = Abc_Tt6Cofactor1( uTruthFan, iFanoutObjIndex );
    if ( DecType == 0 ) //  F = i * G
        Gate =  s_Truths6[iFanoutFaninIndex] & s_Truths6[iFanoutObjIndex];
    else if ( DecType == 1 ) //  F = ~i * G
        Gate = ~s_Truths6[iFanoutFaninIndex] & s_Truths6[iFanoutObjIndex];
    else if ( DecType == 2 ) //  F = ~i + G
        Gate = ~s_Truths6[iFanoutFaninIndex] | s_Truths6[iFanoutObjIndex];
    else if ( DecType == 3 ) //  F = i + G
        Gate =  s_Truths6[iFanoutFaninIndex] | s_Truths6[iFanoutObjIndex];
    else if ( DecType == 4 ) //  F = i # G
        Gate =  s_Truths6[iFanoutFaninIndex] ^ s_Truths6[iFanoutObjIndex];
    else assert( 0 );
    uTruthFanNew = (~Gate & c0) | (Gate & c1);
    // update functions
    Vec_WrdWriteEntry( &p->vObjTruth, iObj, Abc_Tt6RemoveVar(uTruthObjNew, iFaninIndex) );
    Vec_WrdWriteEntry( &p->vObjTruth, iFanout, uTruthFanNew );
    // update fanins
    Acb_ObjRemoveFaninFanoutOne( p, iObj, iFanin );
    if ( iFanoutFaninIndex == Acb_ObjFaninNum(p, iFanout) ) // adding new
        Acb_ObjAddFaninFanoutOne( p, iFanout, iFanin );
}

/**Function*************************************************************

  Synopsis    [Pushing logic to the fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_ObjPushToFanin( Acb_Ntk_t * p, int iObj, int iFaninIndex2, int iFanin )
{
    word uTruthObjNew = 0, uTruthObj = Acb_ObjTruth( p, iObj );
    word uTruthFanNew = 0, uTruthFan = Acb_ObjTruth( p, iFanin );
    int iFaninIndex = Acb_ObjWhatFanin( p, iObj, iFanin );
    int DecType = Abc_TtCheckDsdAnd( uTruthObj, iFaninIndex, iFaninIndex2, &uTruthObjNew );
    int iFanin2 = Acb_ObjFanin( p, iObj, iFaninIndex2 );
    int iFaninFaninIndex = Acb_ObjWhatFanin( p, iFanin, iFanin2 );
    if ( iFaninFaninIndex == -1 )
        iFaninFaninIndex = Acb_ObjFaninNum(p, iFanin); 
    assert( !Acb_ObjIsCio(p, iObj) );
    assert( !Acb_ObjIsCio(p, iFanin) );
    assert( iFaninIndex  < Acb_ObjFaninNum(p, iObj) );
    assert( iFaninIndex2 < Acb_ObjFaninNum(p, iObj) );
    assert( iFaninIndex != iFaninIndex2 );
    assert( Acb_ObjFanoutNum(p, iFanin) == 1 );
    // compute new function of the fanout
    if ( DecType == 0 )      //  i *  j
        uTruthFanNew =  uTruthFan &  s_Truths6[iFaninFaninIndex];
    else if ( DecType == 1 ) //  i * !j
        uTruthFanNew = ~uTruthFan &  s_Truths6[iFaninFaninIndex];
    else if ( DecType == 2 ) // !i *  j
        uTruthFanNew =  uTruthFan & ~s_Truths6[iFaninFaninIndex];
    else if ( DecType == 3 ) // !i * !j
        uTruthFanNew = ~uTruthFan & ~s_Truths6[iFaninFaninIndex];
    else if ( DecType == 4 ) //  i #  j
        uTruthFanNew =  uTruthFan ^  s_Truths6[iFaninFaninIndex];
    else assert( 0 );
    // update functions
    Vec_WrdWriteEntry( &p->vObjTruth, iObj, Abc_Tt6RemoveVar(uTruthObjNew, iFaninIndex2) );
    Vec_WrdWriteEntry( &p->vObjTruth, iFanin, uTruthFanNew );
    // update fanins
    Acb_ObjRemoveFaninFanoutOne( p, iObj, iFanin2 );
    if ( iFaninFaninIndex == Acb_ObjFaninNum(p, iFanin) ) // adding new
        Acb_ObjAddFaninFanoutOne( p, iFanin, iFanin2 );
}

/**Function*************************************************************

  Synopsis    [Removing constants, buffers, duplicated fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Acb_ObjFindNodeFanout( Acb_Ntk_t * p, int iObj )
{
    int i, iFanout;
    Acb_ObjForEachFanout( p, iObj, iFanout, i )
        if ( !Acb_ObjIsCio(p, iFanout) )
            return iFanout;
    return -1;
}
int Acb_ObjSuppMin_int( Acb_Ntk_t * p, int iObj )
{
    int k, iFanin, * pFanins;
    word uTruth = Acb_ObjTruth( p, iObj );
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
    {
        if ( Abc_Tt6HasVar(uTruth, k) )
            continue;
        Acb_ObjDeleteFaninIndex( p, iObj, k );
        Vec_IntRemove( Vec_WecEntry(&p->vFanouts, iFanin), iObj );
        Vec_WrdWriteEntry( &p->vObjTruth, iObj, Abc_Tt6RemoveVar(uTruth, k) );
        return 1;
    }
    return 0;
}
void Acb_ObjSuppMin( Acb_Ntk_t * p, int iObj )
{
    while ( Acb_ObjSuppMin_int(p, iObj) );
}
void Acb_ObjRemoveDup( Acb_Ntk_t * p, int iObj, int i, int j )
{
    word c00, c11, uTruthNew, uTruth = Acb_ObjTruth( p, iObj );
    assert( !Acb_ObjIsCio(p, iObj) );
    assert( Acb_ObjFanin(p, iObj, i) == Acb_ObjFanin(p, iObj, j) );
    c00 = Abc_Tt6Cofactor0( Abc_Tt6Cofactor0(uTruth, i), j );
    c11 = Abc_Tt6Cofactor1( Abc_Tt6Cofactor1(uTruth, i), j );
    uTruthNew = (~s_Truths6[i] & c00) | (s_Truths6[i] & c11);
    Vec_WrdWriteEntry( &p->vObjTruth, iObj, Abc_Tt6RemoveVar(uTruthNew, j) );
    Acb_ObjDeleteFaninIndex( p, iObj, j );
    Vec_IntRemove( Vec_WecEntry(&p->vFanouts, iObj), Acb_ObjFanin(p, iObj, j) );
    Acb_ObjSuppMin( p, iObj );
}
int Acb_ObjRemoveDupFanins_int( Acb_Ntk_t * p, int iObj )
{
    int i, k, * pFanins = Acb_ObjFanins( p, iObj );
    for ( i = 0; i < pFanins[0]; i++ )
    for ( k = i+1; k < pFanins[0]; k++ )
    {
        if ( pFanins[1+i] != pFanins[1+k] )
            continue;
        Acb_ObjRemoveDup( p, iObj, i, k );
        return 1;
    }
    return 0;
}
void Acb_ObjRemoveDupFanins( Acb_Ntk_t * p, int iObj )
{
    assert( !Acb_ObjIsCio(p, iObj) );
    while ( Acb_ObjRemoveDupFanins_int(p, iObj) );
}
void Acb_ObjRemoveConst( Acb_Ntk_t * p, int iObj )
{
    int iFanout;
    word uTruth = Acb_ObjTruth( p, iObj );
    assert( !Acb_ObjIsCio(p, iObj) );
    assert( Acb_ObjFaninNum(p, iObj) == 0 );
    assert( uTruth == 0 || ~uTruth == 0 );
    while ( (iFanout = Acb_ObjFindNodeFanout(p, iObj)) >= 0 )
    {
        int iObjIndex = Acb_ObjWhatFanin( p, iFanout, iObj );
        word uTruthF = Acb_ObjTruth( p, iFanout );
        Acb_ObjRemoveFaninFanoutOne( p, iFanout, iObj );
        uTruthF = (uTruth & 1) ? Abc_Tt6Cofactor1(uTruthF, iObjIndex) : Abc_Tt6Cofactor0(uTruthF, iObjIndex);
        Vec_WrdWriteEntry( &p->vObjTruth, iFanout, Abc_Tt6RemoveVar(uTruthF, iObjIndex) );
        Acb_ObjSuppMin( p, iFanout );
    }
    if ( Acb_ObjFanoutNum(p, iObj) == 0 )
        Acb_ObjCleanType( p, iObj );
}
void Acb_ObjRemoveBufInv( Acb_Ntk_t * p, int iObj )
{
    int iFanout;
    word uTruth = Acb_ObjTruth( p, iObj );
    assert( !Acb_ObjIsCio(p, iObj) );
    assert( Acb_ObjFaninNum(p, iObj) == 1 );
    assert( uTruth == s_Truths6[0] || ~uTruth == s_Truths6[0] );
    while ( (iFanout = Acb_ObjFindNodeFanout(p, iObj)) >= 0 )
    {
        int iFanin = Acb_ObjFanin( p, iObj, 0 );
        int iObjIndex = Acb_ObjWhatFanin( p, iFanout, iObj );
        Acb_ObjPatchFanin( p, iFanout, iObj, iFanin );
        if ( uTruth & 1 ) // inv
        {
            word uTruthF = Acb_ObjTruth( p, iFanout );
            Vec_WrdWriteEntry( &p->vObjTruth, iFanout, Abc_Tt6Flip(uTruthF, iObjIndex) );
        }
        Acb_ObjRemoveDupFanins( p, iFanout );
    }
    while ( (uTruth & 1) == 0 && Acb_ObjFanoutNum(p, iObj) > 0 )
    {
        int iFanin = Acb_ObjFanin( p, iObj, 0 );
        int iFanout = Acb_ObjFanout( p, iObj, 0 );
        assert( Acb_ObjIsCo(p, iFanout) );
        Acb_ObjPatchFanin( p, iFanout, iObj, iFanin );
    }
    if ( Acb_ObjFanoutNum(p, iObj) == 0 )
    {
        Acb_ObjRemoveFaninFanout( p, iObj );
        Acb_ObjRemoveFanins( p, iObj );
        Acb_ObjCleanType( p, iObj );
    }
}

/**Function*************************************************************

  Synopsis    [Check if the node can have its logic pushed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Acb_ObjFindFaninPushableIndex( Acb_Ntk_t * p, int iObj, int iFanIndex )
{
    int k, iFanin, * pFanins;
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
        if ( k != iFanIndex && Abc_TtCheckDsdAnd(Acb_ObjTruth(p, iObj), k, iFanIndex, NULL) >= 0 )
            return k;
    return -1;
}
static inline int Acb_ObjFindFanoutPushableIndex( Acb_Ntk_t * p, int iObj )
{
    int k, iFanin, * pFanins;
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
        if ( Abc_Tt6CheckOutDec(Acb_ObjTruth(p, iObj), k, NULL) >= 0 )
            return k;
    return -1;
}
int Acb_ObjPushToFanins( Acb_Ntk_t * p, int iObj, int nLutSize )
{
    int k, k2, iFanin, * pFanins;
    if ( Acb_ObjFaninNum(p, iObj) < 2 )
        return 0;
    Acb_ObjForEachFaninFast( p, iObj, pFanins, iFanin, k )
    {
        if ( Acb_ObjIsCi(p, iFanin) )
            continue;
        if ( Acb_ObjFanoutNum(p, iFanin) > 1 )
            continue;
        if ( Acb_ObjFaninNum(p, iFanin) == nLutSize )
            continue;
        if ( (k2 = Acb_ObjFindFaninPushableIndex(p, iObj, k)) == -1 )
            continue;
        //printf( "Object %4d : Pushing fanin %d (%d) into fanin %d.\n", iObj, Acb_ObjFanin(p, iObj, k2), k2, iFanin );
        Acb_ObjPushToFanin( p, iObj, k2, iFanin );
        return 1;
    }
    if ( Acb_ObjFaninNum(p, iObj) == 2 && Acb_ObjFanoutNum(p, iObj) == 1 )
    {
        int iFanout = Acb_ObjFanout( p, iObj, 0 );
        if ( !Acb_ObjIsCo(p, iFanout) && Acb_ObjFaninNum(p, iFanout) < nLutSize )
        {
            k2 = Acb_ObjFindFanoutPushableIndex( p, iObj );
            //printf( "Object %4d : Pushing fanin %d (%d) into fanout %d.\n", iObj, Acb_ObjFanin(p, iObj, k2), k2, iFanout );
            Acb_ObjPushToFanout( p, iObj, k2, iFanout );
            return 1;
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_NtkPushLogic( Acb_Ntk_t * p, int nLutSize, int fVerbose )
{
    int n = 0, iObj, nNodes = Acb_NtkNodeNum(p), nPushes = 0;
    Acb_NtkCreateFanout( p );  // fanout data structure
    Acb_NtkForEachNodeSupp( p, iObj, 0 )
        Acb_ObjRemoveConst( p, iObj );
    Acb_NtkForEachNodeSupp( p, iObj, 1 )
        Acb_ObjRemoveBufInv( p, iObj );
    for ( n = 2; n <= nLutSize; n++ )
        Acb_NtkForEachNodeSupp( p, iObj, n )
        {
            while ( Acb_ObjPushToFanins(p, iObj, nLutSize) )
                nPushes++;
            if ( Acb_ObjFaninNum(p, iObj) == 1 )
                Acb_ObjRemoveBufInv( p, iObj );
        }
    printf( "Saved %d nodes after %d pushes.\n", nNodes - Acb_NtkNodeNum(p), nPushes );
}

/**Function*************************************************************

  Synopsis    [Pushing logic to the fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acb_NtkPushLogic2( Acb_Ntk_t * p, int nLutSize, int fVerbose )
{
    int iObj;
    Acb_NtkCreateFanout( p );  // fanout data structure
    Acb_NtkForEachObj( p, iObj )
        if ( !Acb_ObjIsCio(p, iObj) )
            break;
    Acb_ObjPushToFanout( p, iObj, Acb_ObjFaninNum(p, iObj)-1, Acb_ObjFanout(p, iObj, 0) );
//    Acb_ObjPushToFanin( p, Acb_ObjFanout(p, iObj, 0), Acb_ObjFaninNum(p, iObj)-1, iObj );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

