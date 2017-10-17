/**CFile****************************************************************

  FileName    [ivyHaig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [HAIG management procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyHaig.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/* 
    HAIGing rules in working AIG:
    - Each node in the working AIG has a pointer to the corresponding node in HAIG 
      (this node is not necessarily the representative of the equivalence class of HAIG nodes)
    - This pointer is complemented if the AIG node and its corresponding HAIG node have different phase

    Choice node rules in HAIG:
    - Equivalent nodes are linked into a ring
    - Exactly one node in the ring has fanouts (this node is called the representative)
    - The pointer going from a node to the next node in the ring is complemented 
      if the first node is complemented, compared to the representative node of the equivalence class
    - (consequence of the above) The representative node always has non-complemented pointer to the next node
    - New nodes are inserted into the ring immediately after the representative node
*/

// returns the representative node of the given HAIG node
static inline Ivy_Obj_t * Ivy_HaigObjRepr( Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pTemp;
    assert( !Ivy_IsComplement(pObj) );
    // if the node has no equivalent node or has fanout, it is representative
    if ( pObj->pEquiv == NULL || Ivy_ObjRefs(pObj) > 0 )
        return pObj;
    // the node belongs to a class and is not a representative
    // complemented edge (pObj->pEquiv) tells if it is complemented w.r.t. the repr
    for ( pTemp = Ivy_Regular(pObj->pEquiv); pTemp != pObj; pTemp = Ivy_Regular(pTemp->pEquiv) )
        if ( Ivy_ObjRefs(pTemp) > 0 )
            break;
    // return the representative node
    assert( Ivy_ObjRefs(pTemp) > 0 );
    return Ivy_NotCond( pTemp, Ivy_IsComplement(pObj->pEquiv) );
}

// counts the number of nodes in the equivalence class
static inline int Ivy_HaigObjCountClass( Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pTemp;
    int Counter;
    assert( !Ivy_IsComplement(pObj) );
    assert( Ivy_ObjRefs(pObj) > 0 );
    if ( pObj->pEquiv == NULL )
        return 1;
    assert( !Ivy_IsComplement(pObj->pEquiv) );
    Counter = 1;
    for ( pTemp = pObj->pEquiv; pTemp != pObj; pTemp = Ivy_Regular(pTemp->pEquiv) )
        Counter++;
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts HAIG for the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManHaigStart( Ivy_Man_t * p, int fVerbose )
{
    Vec_Int_t * vLatches;
    Ivy_Obj_t * pObj;
    int i;
    assert( p->pHaig == NULL );
    p->pHaig = Ivy_ManDup( p );

    if ( fVerbose )
    {
        printf( "Starting : " );
        Ivy_ManPrintStats( p->pHaig );
    }

    // collect latches of design D and set their values to be DC
    vLatches = Vec_IntAlloc( 100 );
    Ivy_ManForEachLatch( p->pHaig, pObj, i )
    {
        pObj->Init = IVY_INIT_DC;
        Vec_IntPush( vLatches, pObj->Id );
    }
    p->pHaig->pData = vLatches;
/*
    {
        int x;
        Ivy_ManShow( p, 0, NULL );
        Ivy_ManShow( p->pHaig, 1, NULL );
        x = 0;
    }
*/
}

/**Function*************************************************************

  Synopsis    [Transfers the HAIG to the newly created manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManHaigTrasfer( Ivy_Man_t * p, Ivy_Man_t * pNew )
{
    Ivy_Obj_t * pObj;
    int i;
    assert( p->pHaig != NULL );
    Ivy_ManConst1(pNew)->pEquiv = Ivy_ManConst1(p)->pEquiv;
    Ivy_ManForEachPi( pNew, pObj, i )
        pObj->pEquiv = Ivy_ManPi( p, i )->pEquiv;
    pNew->pHaig = p->pHaig;
}

/**Function*************************************************************

  Synopsis    [Stops HAIG for the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManHaigStop( Ivy_Man_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    assert( p->pHaig != NULL );
    Vec_IntFree( (Vec_Int_t *)p->pHaig->pData );
    Ivy_ManStop( p->pHaig );
    p->pHaig = NULL;
    // remove dangling pointers to the HAIG objects
    Ivy_ManForEachObj( p, pObj, i )
        pObj->pEquiv = NULL;
}

/**Function*************************************************************

  Synopsis    [Creates a new node in HAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManHaigCreateObj( Ivy_Man_t * p, Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pEquiv0, * pEquiv1;
    assert( p->pHaig != NULL );
    assert( !Ivy_IsComplement(pObj) );
    if ( Ivy_ObjType(pObj) == IVY_BUF )
        pObj->pEquiv = Ivy_ObjChild0Equiv(pObj);
    else if ( Ivy_ObjType(pObj) == IVY_LATCH )
    {
//        pObj->pEquiv = Ivy_Latch( p->pHaig, Ivy_ObjChild0Equiv(pObj), pObj->Init );
        pEquiv0 = Ivy_ObjChild0Equiv(pObj);
        pEquiv0 = Ivy_NotCond( Ivy_HaigObjRepr(Ivy_Regular(pEquiv0)), Ivy_IsComplement(pEquiv0) );
        pObj->pEquiv = Ivy_Latch( p->pHaig, pEquiv0, (Ivy_Init_t)pObj->Init );
    }
    else if ( Ivy_ObjType(pObj) == IVY_AND )
    {
//        pObj->pEquiv = Ivy_And( p->pHaig, Ivy_ObjChild0Equiv(pObj), Ivy_ObjChild1Equiv(pObj) );
        pEquiv0 = Ivy_ObjChild0Equiv(pObj);
        pEquiv0 = Ivy_NotCond( Ivy_HaigObjRepr(Ivy_Regular(pEquiv0)), Ivy_IsComplement(pEquiv0) );
        pEquiv1 = Ivy_ObjChild1Equiv(pObj);
        pEquiv1 = Ivy_NotCond( Ivy_HaigObjRepr(Ivy_Regular(pEquiv1)), Ivy_IsComplement(pEquiv1) );
        pObj->pEquiv = Ivy_And( p->pHaig, pEquiv0, pEquiv1 );
    }
    else assert( 0 );
    // make sure the node points to the representative
//    pObj->pEquiv = Ivy_NotCond( Ivy_HaigObjRepr(Ivy_Regular(pObj->pEquiv)), Ivy_IsComplement(pObj->pEquiv) );
}

/**Function*************************************************************

  Synopsis    [Checks if the old node is in the TFI of the new node.]

  Description [] 
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ObjIsInTfi_rec( Ivy_Obj_t * pObjNew, Ivy_Obj_t * pObjOld, int Levels )
{
    if ( pObjNew == pObjOld )
        return 1;
    if ( Levels == 0 || Ivy_ObjIsCi(pObjNew) || Ivy_ObjIsConst1(pObjNew) )
        return 0;
    if ( Ivy_ObjIsInTfi_rec( Ivy_ObjFanin0(pObjNew), pObjOld, Levels - 1 ) )
        return 1;
    if ( Ivy_ObjIsNode(pObjNew) && Ivy_ObjIsInTfi_rec( Ivy_ObjFanin1(pObjNew), pObjOld, Levels - 1 ) )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sets the pair of equivalent nodes in HAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManHaigCreateChoice( Ivy_Man_t * p, Ivy_Obj_t * pObjOld, Ivy_Obj_t * pObjNew )
{
    Ivy_Obj_t * pObjOldHaig, * pObjNewHaig;
    Ivy_Obj_t * pObjOldHaigR, * pObjNewHaigR;
    int fCompl;
//printf( "\nCreating choice for %d and %d in AIG\n", pObjOld->Id, Ivy_Regular(pObjNew)->Id );

    assert( p->pHaig != NULL );
    assert( !Ivy_IsComplement(pObjOld) );
    // get pointers to the representatives of pObjOld and pObjNew
    pObjOldHaig = pObjOld->pEquiv;
    pObjNewHaig = Ivy_NotCond( Ivy_Regular(pObjNew)->pEquiv, Ivy_IsComplement(pObjNew) );
    // get the classes
    pObjOldHaig = Ivy_NotCond( Ivy_HaigObjRepr(Ivy_Regular(pObjOldHaig)), Ivy_IsComplement(pObjOldHaig) );
    pObjNewHaig = Ivy_NotCond( Ivy_HaigObjRepr(Ivy_Regular(pObjNewHaig)), Ivy_IsComplement(pObjNewHaig) );
    // get regular pointers
    pObjOldHaigR = Ivy_Regular(pObjOldHaig);
    pObjNewHaigR = Ivy_Regular(pObjNewHaig);
    // check if there is phase difference between them
    fCompl = (Ivy_IsComplement(pObjOldHaig) != Ivy_IsComplement(pObjNewHaig));
    // if the class is the same, nothing to do
    if ( pObjOldHaigR == pObjNewHaigR )
        return;
    // if the second node belongs to a class, do not merge classes (for the time being)
    if ( Ivy_ObjRefs(pObjOldHaigR) == 0 || pObjNewHaigR->pEquiv != NULL || 
        Ivy_ObjRefs(pObjNewHaigR) > 0 ) //|| Ivy_ObjIsInTfi_rec(pObjNewHaigR, pObjOldHaigR, 10) )
    {
/*
        if ( pObjNewHaigR->pEquiv != NULL )
            printf( "c" );
        if ( Ivy_ObjRefs(pObjNewHaigR) > 0 )
            printf( "f" );
        printf( " " );
*/
        p->pHaig->nClassesSkip++;
        return;
    }

    // add this node to the class of pObjOldHaig
    assert( Ivy_ObjRefs(pObjOldHaigR) > 0 );
    assert( !Ivy_IsComplement(pObjOldHaigR->pEquiv) );
    if ( pObjOldHaigR->pEquiv == NULL )
        pObjNewHaigR->pEquiv = Ivy_NotCond( pObjOldHaigR, fCompl );
    else
        pObjNewHaigR->pEquiv = Ivy_NotCond( pObjOldHaigR->pEquiv, fCompl );
    pObjOldHaigR->pEquiv = pObjNewHaigR;
//printf( "Setting choice node %d -> %d.\n", pObjOldHaigR->Id, pObjNewHaigR->Id );
    // update the class of the new node
//    Ivy_Regular(pObjNew)->pEquiv = Ivy_NotCond( pObjOldHaigR, fCompl ^ Ivy_IsComplement(pObjNew) );
//printf( "Creating choice for %d and %d in HAIG\n", pObjOldHaigR->Id, pObjNewHaigR->Id );

//    if ( pObjOldHaigR->Id == 13 )
//    {
//        Ivy_ManShow( p, 0 );
//        Ivy_ManShow( p->pHaig, 1 );
//    }
//    if ( !Ivy_ManIsAcyclic( p->pHaig ) )
//        printf( "HAIG contains a cycle\n" );
}

/**Function*************************************************************

  Synopsis    [Count the number of choices and choice nodes in HAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManHaigCountChoices( Ivy_Man_t * p, int * pnChoices )
{
    Ivy_Obj_t * pObj;
    int nChoices, nChoiceNodes, Counter, i;
    assert( p->pHaig != NULL );
    nChoices = nChoiceNodes = 0;
    Ivy_ManForEachObj( p->pHaig, pObj, i )
    {
        if ( Ivy_ObjIsTerm(pObj) || i == 0 )
            continue;
        if ( Ivy_ObjRefs(pObj) == 0 )
            continue;
        Counter = Ivy_HaigObjCountClass( pObj );
        nChoiceNodes += (int)(Counter > 1);
        nChoices += Counter - 1;
//        if ( Counter > 1 )
//            printf( "Choice node %d %s\n", pObj->Id, Ivy_ObjIsLatch(pObj)? "(latch)": "" );
    }
    *pnChoices = nChoices;
    return nChoiceNodes;
} 

/**Function*************************************************************

  Synopsis    [Prints statistics of the HAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManHaigPostprocess( Ivy_Man_t * p, int fVerbose )
{
    int nChoices, nChoiceNodes;

    assert( p->pHaig != NULL );

    if ( fVerbose )
    {
        printf( "Final    : " );
        Ivy_ManPrintStats( p );
        printf( "HAIG     : " );
        Ivy_ManPrintStats( p->pHaig );

        // print choice node stats
        nChoiceNodes = Ivy_ManHaigCountChoices( p, &nChoices );
        printf( "Total choice nodes = %d. Total choices = %d. Skipped classes = %d.\n", 
            nChoiceNodes, nChoices, p->pHaig->nClassesSkip ); 
    }

    if ( Ivy_ManIsAcyclic( p->pHaig ) )
    {
        if ( fVerbose )
            printf( "HAIG is acyclic\n" );
    }
    else
        printf( "HAIG contains a cycle\n" );

//    if ( fVerbose )
//        Ivy_ManHaigSimulate( p );
}


/**Function*************************************************************

  Synopsis    [Applies the simulation rules.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Ivy_Init_t Ivy_ManHaigSimulateAnd( Ivy_Init_t In0, Ivy_Init_t In1 )
{
    assert( In0 != IVY_INIT_NONE && In1 != IVY_INIT_NONE );
    if ( In0 == IVY_INIT_DC || In1 == IVY_INIT_DC )
        return IVY_INIT_DC;
    if ( In0 == IVY_INIT_1 && In1 == IVY_INIT_1 )
        return IVY_INIT_1;
    return IVY_INIT_0;
}

/**Function*************************************************************

  Synopsis    [Applies the simulation rules.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Ivy_Init_t Ivy_ManHaigSimulateChoice( Ivy_Init_t In0, Ivy_Init_t In1 )
{
    assert( In0 != IVY_INIT_NONE && In1 != IVY_INIT_NONE );
    if ( (In0 == IVY_INIT_0 && In1 == IVY_INIT_1) || (In0 == IVY_INIT_1 && In1 == IVY_INIT_0) )
    {
         printf( "Compatibility fails.\n" );
         return IVY_INIT_0;
    }
    if ( In0 == IVY_INIT_DC && In1 == IVY_INIT_DC )
        return IVY_INIT_DC;
    if ( In0 != IVY_INIT_DC )
        return In0;
    return In1;
}

/**Function*************************************************************

  Synopsis    [Simulate HAIG using modified 3-valued simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManHaigSimulate( Ivy_Man_t * p )
{
    Vec_Int_t * vNodes, * vLatches, * vLatchesD;
    Ivy_Obj_t * pObj, * pTemp;
    Ivy_Init_t In0, In1;
    int i, k, Counter;
    int fVerbose = 0;

    // check choices
    Ivy_ManCheckChoices( p );

    // switch to HAIG
    assert( p->pHaig != NULL );
    p = p->pHaig;

if ( fVerbose )
Ivy_ManForEachPi( p, pObj, i )
printf( "Setting PI %d\n", pObj->Id );

    // collect latches and nodes in the DFS order
    vNodes = Ivy_ManDfsSeq( p, &vLatches );

if ( fVerbose )
Ivy_ManForEachNodeVec( p, vNodes, pObj, i )
printf( "Collected node %d with fanins %d and %d\n", pObj->Id, Ivy_ObjFanin0(pObj)->Id, Ivy_ObjFanin1(pObj)->Id );

    // set the PI values
    Ivy_ManConst1(p)->Init = IVY_INIT_1;
    Ivy_ManForEachPi( p, pObj, i )
        pObj->Init = IVY_INIT_0;

    // set the latch values
    Ivy_ManForEachNodeVec( p, vLatches, pObj, i )
        pObj->Init = IVY_INIT_DC;
    // set the latches of D to be determinate
    vLatchesD = (Vec_Int_t *)p->pData;
    Ivy_ManForEachNodeVec( p, vLatchesD, pObj, i )
        pObj->Init = IVY_INIT_0;

    // perform several rounds of simulation
    for ( k = 0; k < 10; k++ )
    {
        // count the number of non-determinate values
        Counter = 0;
        Ivy_ManForEachNodeVec( p, vLatches, pObj, i )
            Counter += ( pObj->Init == IVY_INIT_DC );
        printf( "Iter %d : Non-determinate = %d\n", k, Counter );    
        
        // simulate the internal nodes
        Ivy_ManForEachNodeVec( p, vNodes, pObj, i )
        {
if ( fVerbose )
printf( "Processing node %d with fanins %d and %d\n", pObj->Id, Ivy_ObjFanin0(pObj)->Id, Ivy_ObjFanin1(pObj)->Id );
            In0 = Ivy_InitNotCond( (Ivy_Init_t)Ivy_ObjFanin0(pObj)->Init, Ivy_ObjFaninC0(pObj) );
            In1 = Ivy_InitNotCond( (Ivy_Init_t)Ivy_ObjFanin1(pObj)->Init, Ivy_ObjFaninC1(pObj) );
            pObj->Init = Ivy_ManHaigSimulateAnd( In0, In1 );
            // simulate the equivalence class if the node is a representative
            if ( pObj->pEquiv && Ivy_ObjRefs(pObj) > 0 )
            {
if ( fVerbose )
printf( "Processing choice node %d\n", pObj->Id );
                In0 = (Ivy_Init_t)pObj->Init;
                assert( !Ivy_IsComplement(pObj->pEquiv) );
                for ( pTemp = pObj->pEquiv; pTemp != pObj; pTemp = Ivy_Regular(pTemp->pEquiv) )
                {
if ( fVerbose )
printf( "Processing secondary node %d\n", pTemp->Id );
                    In1 = Ivy_InitNotCond( (Ivy_Init_t)pTemp->Init, Ivy_IsComplement(pTemp->pEquiv) );
                    In0 = Ivy_ManHaigSimulateChoice( In0, In1 );
                }
                pObj->Init = In0;
            }
        }

        // simulate the latches
        Ivy_ManForEachNodeVec( p, vLatches, pObj, i )
        {
            pObj->Level = Ivy_ObjFanin0(pObj)->Init;
if ( fVerbose )
printf( "Using latch %d with fanin %d\n", pObj->Id, Ivy_ObjFanin0(pObj)->Id );
        }
        Ivy_ManForEachNodeVec( p, vLatches, pObj, i )
            pObj->Init = pObj->Level, pObj->Level = 0;
    }
    // free arrays
    Vec_IntFree( vNodes );
    Vec_IntFree( vLatches );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

