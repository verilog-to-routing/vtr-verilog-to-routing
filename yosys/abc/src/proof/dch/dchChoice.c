/**CFile****************************************************************

  FileName    [dchChoice.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Choice computation for tech-mapping.]

  Synopsis    [Contrustion of choices.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: dchChoice.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dchInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Counts support nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dch_ObjCountSupp_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    int Count;
    if ( Aig_ObjIsTravIdCurrent( p, pObj ) )
        return 0;
    Aig_ObjSetTravIdCurrent( p, pObj );
    if ( Aig_ObjIsCi(pObj) )
        return 1;
    assert( Aig_ObjIsNode(pObj) );
    Count  = Dch_ObjCountSupp_rec( p, Aig_ObjFanin0(pObj) );
    Count += Dch_ObjCountSupp_rec( p, Aig_ObjFanin1(pObj) );
    return Count;
}
int Dch_ObjCountSupp( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_ManIncrementTravId( p );
    return Dch_ObjCountSupp_rec( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Counts the number of representatives.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dch_DeriveChoiceCountReprs( Aig_Man_t * pAig )
{
    Aig_Obj_t * pObj, * pRepr;
    int i, nReprs = 0;
    Aig_ManForEachObj( pAig, pObj, i )
    {
        pRepr = Aig_ObjRepr( pAig, pObj );
        if ( pRepr == NULL )
            continue;
        assert( pRepr->Id < pObj->Id );
        nReprs++;
    }
    return nReprs;
}
int Dch_DeriveChoiceCountEquivs( Aig_Man_t * pAig )
{
    Aig_Obj_t * pObj, * pEquiv;
    int i, nEquivs = 0;
    Aig_ManForEachObj( pAig, pObj, i )
    {
        pEquiv = Aig_ObjEquiv( pAig, pObj );
        if ( pEquiv == NULL )
            continue;
        assert( pEquiv->Id < pObj->Id );
        nEquivs++;
    }
    return nEquivs;
}

/**Function*************************************************************

  Synopsis    [Marks the TFI of the node.]

  Description [Returns 1 if there is a CI not marked with previous ID.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dch_ObjMarkTfi_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    int RetValue;
    if ( pObj == NULL )
        return 0;
    if ( Aig_ObjIsTravIdCurrent( p, pObj ) )
        return 0;
    if ( Aig_ObjIsCi(pObj) )
    {
        RetValue = !Aig_ObjIsTravIdPrevious( p, pObj );
        Aig_ObjSetTravIdCurrent( p, pObj );
        return RetValue;
    }
    assert( Aig_ObjIsNode(pObj) );
    Aig_ObjSetTravIdCurrent( p, pObj );
    RetValue  = Dch_ObjMarkTfi_rec( p, Aig_ObjFanin0(pObj) );
    RetValue += Dch_ObjMarkTfi_rec( p, Aig_ObjFanin1(pObj) );
//    RetValue += Dch_ObjMarkTfi_rec( p, Aig_ObjEquiv(p, pObj) );
    return (RetValue > 0);
}
int Dch_ObjCheckSuppRed( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pRepr )
{
    // mark support of the representative node (pRepr)
    Aig_ManIncrementTravId( p );
    Dch_ObjMarkTfi_rec( p, pRepr );
    // detect if the new node (pObj) depends on additional variables
    Aig_ManIncrementTravId( p );
    if ( Dch_ObjMarkTfi_rec( p, pObj ) )
        return 1;//, printf( "1" );
    // detect if the representative node (pRepr) depends on additional variables
    Aig_ManIncrementTravId( p );
    if ( Dch_ObjMarkTfi_rec( p, pRepr ) )
        return 1;//, printf( "2" );
    // skip the choice if this is what is happening
    return 0;
}

/**Function*************************************************************

  Synopsis    [Make sure reprsentative nodes do not have representatives.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManCheckReprs( Aig_Man_t * p )
{
    int fPrintConst = 0;
    Aig_Obj_t * pObj;
    int i, fProb = 0;
    int Class0 = 0, ClassCi = 0;
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjRepr(p, pObj) == NULL )
            continue;
        if ( !Aig_ObjIsNode(pObj) )
            printf( "Obj %d is not an AND but it has a repr %d.\n", i, Aig_ObjId(Aig_ObjRepr(p, pObj)) ), fProb = 1;
        else if ( Aig_ObjRepr(p, Aig_ObjRepr(p, pObj)) )
            printf( "Obj %d has repr %d with a repr %d.\n", i, Aig_ObjId(Aig_ObjRepr(p, pObj)), Aig_ObjId(Aig_ObjRepr(p, Aig_ObjRepr(p, pObj))) ), fProb = 1;
    }
    if ( !fProb )
        printf( "Representive verification successful.\n" );
    else
        printf( "Representive verification FAILED.\n" );
    if ( !fPrintConst )
        return;
    // count how many representatives are const0 or a CI
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjRepr(p, pObj) == Aig_ManConst1(p) )
            Class0++;
        if ( Aig_ObjRepr(p, pObj) && Aig_ObjIsCi(Aig_ObjRepr(p, pObj)) )
            ClassCi++;
    }
    printf( "Const0 nodes = %d.  ConstCI nodes = %d.\n", Class0, ClassCi );
}

/**Function*************************************************************

  Synopsis    [Verify correctness of choices.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_CheckChoices( Aig_Man_t * p, int fSkipRedSupps )
{
    Aig_Obj_t * pObj;
    int i, fProb = 0;
    Aig_ManCleanMarkA( p );
    Aig_ManForEachNode( p, pObj, i )
    {
        if ( p->pEquivs[i] != NULL )
        {
            if ( pObj->fMarkA == 1 )
                printf( "node %d participates in more than one choice class\n", i ), fProb = 1;
            pObj->fMarkA = 1;
            // check redundancy
            if ( fSkipRedSupps && Dch_ObjCheckSuppRed( p, pObj, p->pEquivs[i]) )
                printf( "node %d and repr %d have diff supports\n", pObj->Id, p->pEquivs[i]->Id );
            // consider the next one
            pObj = p->pEquivs[i];
            if ( p->pEquivs[Aig_ObjId(pObj)] == NULL )
            {
                if ( pObj->fMarkA == 1 )
                    printf( "repr %d has final node %d participates in more than one choice class\n", i, pObj->Id ), fProb = 1;
                pObj->fMarkA = 1;
            }
            // consider the non-head ones
            if ( pObj->nRefs > 0 )
                printf( "node %d belonging to choice has fanout %d\n", pObj->Id, pObj->nRefs );
        }
        if ( p->pReprs && p->pReprs[i] != NULL )
        {
            if ( pObj->nRefs > 0 )
                printf( "node %d has representative %d and fanout count %d\n", pObj->Id, p->pReprs[i]->Id, pObj->nRefs ), fProb = 1;
        }
    }
    Aig_ManCleanMarkA( p );
    if ( !fProb )
        printf( "Verification of choice AIG succeeded.\n" );
    else
        printf( "Verification of choice AIG FAILED.\n" );
}

/**Function*************************************************************

  Synopsis    [Checks for combinational loops in the AIG.]

  Description [Returns 1 if combinational loop is detected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManCheckAcyclic_rec( Aig_Man_t * p, Aig_Obj_t * pNode, int fVerbose )
{
    Aig_Obj_t * pFanin;
    int fAcyclic;
    if ( Aig_ObjIsCi(pNode) || Aig_ObjIsConst1(pNode) )
        return 1;
    assert( Aig_ObjIsNode(pNode) );
    // make sure the node is not visited
    assert( !Aig_ObjIsTravIdPrevious(p, pNode) );
    // check if the node is part of the combinational loop
    if ( Aig_ObjIsTravIdCurrent(p, pNode) )
    {
        if ( fVerbose )
            Abc_Print( 1, "Network \"%s\" contains combinational loop!\n", p->pSpec? p->pSpec : NULL );
        if ( fVerbose )
        Abc_Print( 1, "Node \"%d\" is encountered twice on the following path to the COs:\n", Aig_ObjId(pNode) );
        return 0;
    }
    // mark this node as a node on the current path
    Aig_ObjSetTravIdCurrent( p, pNode );

    // visit the transitive fanin
    pFanin = Aig_ObjFanin0(pNode);
    // check if the fanin is visited
    if ( !Aig_ObjIsTravIdPrevious(p, pFanin) ) 
    {
        // traverse the fanin's cone searching for the loop
        if ( !(fAcyclic = Aig_ManCheckAcyclic_rec(p, pFanin, fVerbose)) )
        {
            // return as soon as the loop is detected
            if ( fVerbose )
            Abc_Print( 1, " %d ->", Aig_ObjId(pFanin) );
            return 0;
        }
    }

    // visit the transitive fanin
    pFanin = Aig_ObjFanin1(pNode);
    // check if the fanin is visited
    if ( !Aig_ObjIsTravIdPrevious(p, pFanin) ) 
    {
        // traverse the fanin's cone searching for the loop
        if ( !(fAcyclic = Aig_ManCheckAcyclic_rec(p, pFanin, fVerbose)) )
        {
            // return as soon as the loop is detected
            if ( fVerbose )
            Abc_Print( 1, " %d ->", Aig_ObjId(pFanin) );
            return 0;
        }
    }

    // visit choices
    if ( Aig_ObjRepr(p, pNode) == NULL && Aig_ObjEquiv(p, pNode) != NULL )
    {
        for ( pFanin = Aig_ObjEquiv(p, pNode); pFanin; pFanin = Aig_ObjEquiv(p, pFanin) )
        {
            // check if the fanin is visited
            if ( Aig_ObjIsTravIdPrevious(p, pFanin) ) 
                continue;
            // traverse the fanin's cone searching for the loop
            if ( (fAcyclic = Aig_ManCheckAcyclic_rec(p, pFanin, fVerbose)) )
                continue;
            // return as soon as the loop is detected
            if ( fVerbose )
            Abc_Print( 1, " %d", Aig_ObjId(pFanin) );
            if ( fVerbose )
            Abc_Print( 1, " (choice of %d) -> ", Aig_ObjId(pNode) );
            return 0;
        }
    }
    // mark this node as a visited node
    Aig_ObjSetTravIdPrevious( p, pNode );
    return 1;
}
int Aig_ManCheckAcyclic( Aig_Man_t * p, int fVerbose )
{
    Aig_Obj_t * pNode;
    int fAcyclic;
    int i;
    // set the traversal ID for this DFS ordering
    Aig_ManIncrementTravId( p );   
    Aig_ManIncrementTravId( p );   
    // pNode->TravId == pNet->nTravIds      means "pNode is on the path"
    // pNode->TravId == pNet->nTravIds - 1  means "pNode is visited but is not on the path"
    // pNode->TravId <  pNet->nTravIds - 1  means "pNode is not visited"
    // traverse the network to detect cycles
    fAcyclic = 1;
    Aig_ManForEachCo( p, pNode, i )
    {
        pNode = Aig_ObjFanin0(pNode);
        if ( Aig_ObjIsTravIdPrevious(p, pNode) )
            continue;
        // traverse the output logic cone
        if ( (fAcyclic = Aig_ManCheckAcyclic_rec(p, pNode, fVerbose)) )
            continue;
        // stop as soon as the first loop is detected
        if ( fVerbose )
        Abc_Print( 1, " CO %d\n", i );
        break;
    }
    return fAcyclic;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the choice node of pRepr is in the TFI of pObj.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dch_ObjCheckTfi_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    // check the trivial cases
    if ( pObj == NULL )
        return 0;
    if ( Aig_ObjIsCi(pObj) )
        return 0;
    if ( pObj->fMarkA )
        return 1;
    // skip the visited node
    if ( Aig_ObjIsTravIdCurrent( p, pObj ) )
        return 0;
    Aig_ObjSetTravIdCurrent( p, pObj );
    // check the children
    if ( Dch_ObjCheckTfi_rec( p, Aig_ObjFanin0(pObj) ) )
        return 1;
    if ( Dch_ObjCheckTfi_rec( p, Aig_ObjFanin1(pObj) ) )
        return 1;
    // check equivalent nodes
    return Dch_ObjCheckTfi_rec( p, Aig_ObjEquiv(p, pObj) );
}
int Dch_ObjCheckTfi( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pRepr )
{
    Aig_Obj_t * pTemp;
    int RetValue;
    assert( !Aig_IsComplement(pObj) );
    assert( !Aig_IsComplement(pRepr) );
    // mark nodes of the choice node
    for ( pTemp = pRepr; pTemp; pTemp = Aig_ObjEquiv(p, pTemp) )
        pTemp->fMarkA = 1;
    // traverse the new node
    Aig_ManIncrementTravId( p );
    RetValue = Dch_ObjCheckTfi_rec( p, pObj );
    // unmark nodes of the choice node
    for ( pTemp = pRepr; pTemp; pTemp = Aig_ObjEquiv(p, pTemp) )
        pTemp->fMarkA = 0;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Returns representatives of fanin in approapriate polarity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Aig_Obj_t * Aig_ObjGetRepr( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t * pRepr;
    if ( (pRepr = Aig_ObjRepr(p, Aig_Regular(pObj))) )
        return Aig_NotCond( pRepr, Aig_Regular(pObj)->fPhase ^ pRepr->fPhase ^ Aig_IsComplement(pObj) );
    return pObj;
}
static inline Aig_Obj_t * Aig_ObjChild0CopyRepr( Aig_Man_t * p, Aig_Obj_t * pObj ) { return Aig_ObjGetRepr( p, Aig_ObjChild0Copy(pObj) ); }
static inline Aig_Obj_t * Aig_ObjChild1CopyRepr( Aig_Man_t * p, Aig_Obj_t * pObj ) { return Aig_ObjGetRepr( p, Aig_ObjChild1Copy(pObj) ); }

/**Function*************************************************************

  Synopsis    [Derives the AIG with choices from representatives.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dch_DeriveChoiceAigNode( Aig_Man_t * pAigNew, Aig_Man_t * pAigOld, Aig_Obj_t * pObj, int fSkipRedSupps )
{
    Aig_Obj_t * pRepr, * pObjNew, * pReprNew;
    assert( !Aig_IsComplement(pObj) );
    // get the representative
    pRepr = Aig_ObjRepr( pAigOld, pObj );
    if ( pRepr != NULL && (Aig_ObjIsConst1(pRepr) || Aig_ObjIsCi(pRepr)) )
    {
        assert( pRepr->pData != NULL );
        pObj->pData = Aig_NotCond( (Aig_Obj_t *)pRepr->pData, pObj->fPhase ^ pRepr->fPhase );
        return;
    }
    // get the new node
    pObjNew = Aig_And( pAigNew, 
        Aig_ObjChild0CopyRepr(pAigNew, pObj), 
        Aig_ObjChild1CopyRepr(pAigNew, pObj) );
//    pObjNew = Aig_ObjGetRepr( pAigNew, pObjNew );
    while ( 1 ) 
    {
        Aig_Obj_t * pObjNew2 = pObjNew;
        pObjNew = Aig_ObjGetRepr( pAigNew, pObjNew2 );
        if ( pObjNew == pObjNew2 )
            break;
    }
//    assert( Aig_ObjRepr( pAigNew, pObjNew ) == NULL );
    // assign the copy
    assert( pObj->pData == NULL );
    pObj->pData = pObjNew;
    // skip those without reprs
    if ( pRepr == NULL )
        return;
    assert( pRepr->Id < pObj->Id );
    assert( Aig_ObjIsNode(pRepr) );
    // get the corresponding new nodes
    pObjNew  = Aig_Regular((Aig_Obj_t *)pObj->pData);
    pReprNew = Aig_Regular((Aig_Obj_t *)pRepr->pData);
    // skip earlier nodes
    if ( pReprNew->Id >= pObjNew->Id )
        return;
    assert( pReprNew->Id < pObjNew->Id );
    // set the representatives
    Aig_ObjSetRepr( pAigNew, pObjNew, pReprNew );
    // skip used nodes
    if ( pObjNew->nRefs > 0 )
        return;
    assert( pObjNew->nRefs == 0 );
    // skip choices that can lead to combo loops
    if ( Dch_ObjCheckTfi( pAigNew, pObjNew, pReprNew ) )
        return;
    // don't add choice if structural support of pObjNew and pReprNew differ
    if ( fSkipRedSupps && Dch_ObjCheckSuppRed(pAigNew, pObjNew, pReprNew) )
        return;
    // add choice to the end of the list
    while ( pAigNew->pEquivs[pReprNew->Id] != NULL )
        pReprNew = pAigNew->pEquivs[pReprNew->Id];
    assert( pAigNew->pEquivs[pReprNew->Id] == NULL );
    pAigNew->pEquivs[pReprNew->Id] = pObjNew;
}
Aig_Man_t * Dch_DeriveChoiceAigInt( Aig_Man_t * pAig, int fSkipRedSupps )
{
    Aig_Man_t * pChoices;
    Aig_Obj_t * pObj;
    int i;
    // start recording equivalences
    pChoices = Aig_ManStart( Aig_ManObjNumMax(pAig) );
    pChoices->pEquivs = ABC_CALLOC( Aig_Obj_t *, Aig_ManObjNumMax(pAig) );
    pChoices->pReprs  = ABC_CALLOC( Aig_Obj_t *, Aig_ManObjNumMax(pAig) );
    // map constants and PIs
    Aig_ManCleanData( pAig );
    Aig_ManConst1(pAig)->pData = Aig_ManConst1(pChoices);
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pChoices );
    // construct choices for the internal nodes
    assert( pAig->pReprs != NULL );
    Aig_ManForEachNode( pAig, pObj, i ) 
        Dch_DeriveChoiceAigNode( pChoices, pAig, pObj, fSkipRedSupps );
    Aig_ManForEachCo( pAig, pObj, i )
        Aig_ObjCreateCo( pChoices, Aig_ObjChild0CopyRepr(pChoices, pObj) );
    Aig_ManSetRegNum( pChoices, Aig_ManRegNum(pAig) );
    return pChoices;
}
Aig_Man_t * Dch_DeriveChoiceAig( Aig_Man_t * pAig, int fSkipRedSupps )
{
    int fCheck = 0;
    Aig_Man_t * pChoices, * pTemp;
    // verify
    if ( fCheck )
        Aig_ManCheckReprs( pAig ); 
    // compute choices
    pChoices = Dch_DeriveChoiceAigInt( pAig, fSkipRedSupps );
    ABC_FREE( pChoices->pReprs );
    // verify
    if ( fCheck )
        Dch_CheckChoices( pChoices, fSkipRedSupps ); 
    // find correct topo order with choices
    pChoices = Aig_ManDupDfs( pTemp = pChoices );
    Aig_ManStop( pTemp );
    // verify
    if ( fCheck )
        Dch_CheckChoices( pChoices, fSkipRedSupps ); 
    return pChoices;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

