/**CFile****************************************************************

  FileName    [reoSwap.c]

  PackageName [REO: A specialized DD reordering engine.]

  Synopsis    [Implementation of the two-variable swap.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 15, 2002.]

  Revision    [$Id: reoSwap.c,v 1.0 2002/15/10 03:00:00 alanmi Exp $]

***********************************************************************/

#include "reo.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define AddToLinkedList( ppList, pLink )   (((pLink)->Next = *(ppList)), (*(ppList) = (pLink)))

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description [Takes the level (lev0) of the plane, which should be swapped 
  with the next plane. Returns the gain using the current cost function.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
double reoReorderSwapAdjacentVars( reo_man * p, int lev0, int fMovingUp )
{
    // the levels in the decision diagram
    int lev1 = lev0 + 1, lev2 = lev0 + 2;
    // the new nodes on lev0
    reo_unit * pLoop, * pUnit;
    // the new nodes on lev1
    reo_unit * pNewPlane20 = NULL, * pNewPlane21 = NULL; // Suppress "might be used uninitialized"
        reo_unit * pNewPlane20R;
    reo_unit * pUnitE, * pUnitER, * pUnitT;
    // the nodes below lev1
    reo_unit * pNew1E = NULL, * pNew1T = NULL, * pNew2E = NULL, * pNew2T = NULL;
    reo_unit * pNew1ER = NULL, * pNew2ER = NULL;
    // the old linked lists
    reo_unit * pListOld0 = p->pPlanes[lev0].pHead;
    reo_unit * pListOld1 = p->pPlanes[lev1].pHead;
    // working planes and one more temporary plane
    reo_unit * pListNew0 = NULL, ** ppListNew0 = &pListNew0;
    reo_unit * pListNew1 = NULL, ** ppListNew1 = &pListNew1; 
    reo_unit * pListTemp = NULL, ** ppListTemp = &pListTemp; 
    // various integer variables
    int fComp, fCompT, fFound, HKey, fInteract, temp, c;
        int nWidthCofs = -1; // Suppress "might be used uninitialized"
    // statistical variables
    int nNodesUpMovedDown  = 0;
    int nNodesDownMovedUp  = 0;
    int nNodesUnrefRemoved = 0;
    int nNodesUnrefAdded   = 0;
    int nWidthReduction    = 0;
    double AplWeightTotalLev0 = 0.0; // Suppress "might be used uninitialized"
    double AplWeightTotalLev1 = 0.0; // Suppress "might be used uninitialized"
    double AplWeightHalf      = 0.0; // Suppress "might be used uninitialized"
    double AplWeightPrev      = 0.0; // Suppress "might be used uninitialized"
    double AplWeightAfter     = 0.0; // Suppress "might be used uninitialized"
    double nCostGain;     

    // set the old lists
    assert( lev0 >= 0 && lev1 < p->nSupp );
    pListOld0 = p->pPlanes[lev0].pHead;
    pListOld1 = p->pPlanes[lev1].pHead;

    // make sure the planes have nodes
    assert( p->pPlanes[lev0].statsNodes && p->pPlanes[lev1].statsNodes );
    assert( pListOld0 && pListOld1 );

    if ( p->fMinWidth )
    {
        // verify that the width parameters are set correctly
        reoProfileWidthVerifyLevel( p->pPlanes + lev0, lev0 );
        reoProfileWidthVerifyLevel( p->pPlanes + lev1, lev1 );
        // start the storage for cofactors
        nWidthCofs = 0;
    }
    else if ( p->fMinApl )
    {
        AplWeightPrev      = p->nAplCur;
        AplWeightAfter     = p->nAplCur;
        AplWeightTotalLev0 = 0.0;
        AplWeightTotalLev1 = 0.0;
    }

    // check if the planes interact
    fInteract = 0; // assume that they do not interact
    for ( pUnit = pListOld0; pUnit; pUnit = pUnit->Next )
    {
        if ( pUnit->pT->lev == lev1 || Unit_Regular(pUnit->pE)->lev == lev1 )
        {
            fInteract = 1;
            break;
        }
        // change the level now, this is done for efficiency reasons
        pUnit->lev = lev1;
    }

    // set the new signature for hashing
    p->nSwaps++;
    if ( !fInteract )
//    if ( 0 )
    {
        // perform the swap without interaction
        p->nNISwaps++;

        // change the levels
        if ( p->fMinWidth )
        {
            // go through the current lower level, which will become upper
            for ( pUnit = pListOld1; pUnit; pUnit = pUnit->Next )
            {
                pUnit->lev = lev0;

                pUnitER = Unit_Regular(pUnit->pE);
                if ( pUnitER->TopRef > lev0 )
                {
                    if ( pUnitER->Sign != p->nSwaps )
                    {
                        if ( pUnitER->TopRef == lev2 )
                        {
                            pUnitER->TopRef = lev1;
                            nWidthReduction--;
                        }
                        else
                        {
                            assert( pUnitER->TopRef == lev1 );
                        }
                        pUnitER->Sign   = p->nSwaps;
                    }
                }

                pUnitT  = pUnit->pT;
                if ( pUnitT->TopRef > lev0 )
                {
                    if ( pUnitT->Sign != p->nSwaps )
                    {
                        if ( pUnitT->TopRef == lev2 )
                        {
                            pUnitT->TopRef = lev1;
                            nWidthReduction--;
                        }
                        else
                        {
                            assert( pUnitT->TopRef == lev1 );
                        }
                        pUnitT->Sign   = p->nSwaps;
                    }
                }

            }

            // go through the current upper level, which will become lower
            for ( pUnit = pListOld0; pUnit; pUnit = pUnit->Next )
            {
                pUnit->lev = lev1;

                pUnitER = Unit_Regular(pUnit->pE);
                if ( pUnitER->TopRef > lev0 )
                {
                    if ( pUnitER->Sign != p->nSwaps )
                    {
                        assert( pUnitER->TopRef == lev1 );
                        pUnitER->TopRef = lev2;
                        pUnitER->Sign   = p->nSwaps;
                        nWidthReduction++;
                    }
                }

                pUnitT  = pUnit->pT;
                if ( pUnitT->TopRef > lev0 )
                {
                    if ( pUnitT->Sign != p->nSwaps )
                    {
                        assert( pUnitT->TopRef == lev1 );
                        pUnitT->TopRef = lev2;
                        pUnitT->Sign   = p->nSwaps;
                        nWidthReduction++;
                    }
                }
            }
        }
        else
        {
//            for ( pUnit = pListOld0; pUnit; pUnit = pUnit->Next )
//                pUnit->lev = lev1;
            for ( pUnit = pListOld1; pUnit; pUnit = pUnit->Next )
                pUnit->lev = lev0;
        }

        // set the new linked lists, which will be attached to the planes
        pListNew0 = pListOld1;
        pListNew1 = pListOld0;

        if ( p->fMinApl )
        {
            AplWeightTotalLev0 = p->pPlanes[lev1].statsCost;
            AplWeightTotalLev1 = p->pPlanes[lev0].statsCost;
        }
        
        // set the changes in terms of nodes
        nNodesUpMovedDown = p->pPlanes[lev0].statsNodes; 
        nNodesDownMovedUp = p->pPlanes[lev1].statsNodes; 
        goto finish;
    }
    p->Signature++;


    // two-variable swap is done in three easy steps
    // previously I thought that steps (1) and (2) can be merged into one step
    // now it is clear that this cannot be done without changing a lot of other stuff...

    // (1) walk through the upper level, find units without cofactors in the lower level 
    //     and move them to the new lower level (while adding to the cache)
    // (2) walk through the uppoer level, and tranform all the remaning nodes 
    //     while employing cache for the new lower level
    // (3) walk through the old lower level, find those nodes whose ref counters are not zero, 
    //     and move them to the new uppoer level, free other nodes

    // (1) walk through the upper level, find units without cofactors in the lower level 
    //     and move them to the new lower level (while adding to the cache)
    for ( pLoop = pListOld0; pLoop; )
    {
        pUnit = pLoop;
        pLoop = pLoop->Next;

        pUnitE  = pUnit->pE;
        pUnitER = Unit_Regular(pUnitE);
        pUnitT  = pUnit->pT;

        if ( pUnitER->lev != lev1 && pUnitT->lev != lev1 )
        {
            //                before                        after
            //
            //                 <p1>                                 .
            //              0 /    \ 1                              .
            //               /      \                               .
            //              /        \                              .
            //             /          \                     <p2n>   .
            //            /            \                  0 /  \ 1  .
            //           /              \                  /    \   .
            //          /                \                /      \  .
            //         F0                F1              F0      F1

            // move to plane-2-new
            // nothing changes in the process (cofactors, ref counter, APL weight)
            pUnit->lev = lev1;
            AddToLinkedList( ppListNew1, pUnit );
            if ( p->fMinApl )
                AplWeightTotalLev1 += pUnit->Weight;

            // add to cache - find the cell with different signature (not the current one!)
            for (  HKey = hashKey3(p->Signature, pUnitE, pUnitT, p->nTableSize);
                   p->HTable[HKey].Sign == p->Signature;
                   HKey = (HKey+1) % p->nTableSize );
            assert( p->HTable[HKey].Sign != p->Signature );
            p->HTable[HKey].Sign = p->Signature;
            p->HTable[HKey].Arg1 = pUnitE;
            p->HTable[HKey].Arg2 = pUnitT;
            p->HTable[HKey].Arg3 = pUnit;

            nNodesUpMovedDown++;

            if ( p->fMinWidth )
            {
                // update the cofactors's top ref
                if ( pUnitER->TopRef > lev0 ) // the cofactor's top ref level is one of the current two levels
                {
                    assert( pUnitER->TopRef == lev1 );
                    pUnitER->TopRefNew = lev2;
                    if ( pUnitER->Sign != p->nSwaps )
                    {
                        pUnitER->Sign      = p->nSwaps;  // set the current signature
                        p->pWidthCofs[ nWidthCofs++ ] = pUnitER;
                    }
                }
                if ( pUnitT->TopRef > lev0 ) // the cofactor's top ref level is one of the current two levels
                {
                    assert( pUnitT->TopRef == lev1 );
                    pUnitT->TopRefNew = lev2;
                    if ( pUnitT->Sign != p->nSwaps )
                    {
                        pUnitT->Sign      = p->nSwaps;  // set the current signature
                        p->pWidthCofs[ nWidthCofs++ ] = pUnitT;
                    }
                }
            }
        }
        else
        {
            // add to the temporary plane
            AddToLinkedList( ppListTemp, pUnit );
        }
    }


    // (2) walk through the uppoer level, and tranform all the remaning nodes 
    //     while employing cache for the new lower level
    for ( pLoop = pListTemp; pLoop; )
    {
        pUnit = pLoop;
        pLoop = pLoop->Next;

        pUnitE  = pUnit->pE;
        pUnitER = Unit_Regular(pUnitE);
        pUnitT  = pUnit->pT;
        fComp   = (int)(pUnitER != pUnitE);

        // count the amount of weight to reduce the APL of the children of this node
        if ( p->fMinApl )
            AplWeightHalf = 0.5 * pUnit->Weight;

        // determine what situation is this
        if ( pUnitER->lev == lev1 && pUnitT->lev == lev1 )
        {
            if ( fComp == 0 )
            {
                //                before                        after
                //
                //                 <p1>                          <p1n>              .
                //              0 /    \ 1                    0 /    \ 1            .
                //               /      \                      /      \             .
                //              /        \                    /        \            .
                //           <p2>        <p2>              <p2n>       <p2n>        .
                //        0 /   \ 1    0 /  \ 1          0 /  \ 1    0 /   \ 1      .
                //         /     \      /    \            /    \      /     \       .
                //        /       \    /      \          /      \    /       \      .
                //       F0       F1  F2      F3        F0      F2  F1       F3     .
                //                                 pNew1E   pNew1T  pNew2E   pNew2T
                //
                pNew1E = pUnitE->pE;   // F0
                pNew1T = pUnitT->pE;   // F2

                pNew2E = pUnitE->pT;   // F1
                pNew2T = pUnitT->pT;   // F3
            }
            else
            {
                //                before                        after
                //
                //                 <p1>                          <p1n>           .
                //              0 .    \ 1                    0 /    \ 1         .
                //               .      \                      /      \          .
                //              .        \                    /        \         .
                //           <p2>        <p2>              <p2n>       <p2n>     .
                //        0 /   \ 1    0 /  \ 1          0 .  \ 1    0 .   \ 1   .
                //         /     \      /    \            .    \      .     \    .
                //        /       \    /      \          .      \    .       \   .
                //       F0       F1  F2      F3        F0      F2  F1       F3  .
                //                                 pNew1E   pNew1T  pNew2E   pNew2T
                //
                pNew1E = Unit_Not(pUnitER->pE);  // F0
                pNew1T =          pUnitT->pE;    // F2

                pNew2E = Unit_Not(pUnitER->pT);  // F1
                pNew2T =          pUnitT->pT;    // F3
            }
            // subtract ref counters - on the level P2
            pUnitER->n--;
            pUnitT->n--;

            // mark the change in the APL weights
            if ( p->fMinApl )
            {
                pUnitER->Weight -= AplWeightHalf;
                pUnitT->Weight  -= AplWeightHalf;
                AplWeightAfter  -= pUnit->Weight;
            }
        }
        else if ( pUnitER->lev == lev1 )
        {
            if ( fComp == 0 )
            {
                //                before                        after
                //
                //                 <p1>                          <p1n>          .
                //              0 /    \ 1                    0 /    \ 1        .
                //               /      \                      /      \         .
                //              /        \                    /        \        .
                //           <p2>         \               <p2n>       <p2n>     .
                //        0 /   \ 1        \            0 /  \ 1    0 /   \ 1   .
                //         /     \          \            /    \      /     \    .
                //        /       \          \          /      \    /       \   .
                //       F0       F1         F3        F0      F3  F1       F3  .
                //                                 pNew1E   pNew1T  pNew2E   pNew2T
                //
                pNew1E = pUnitER->pE;      // F0
                pNew1T = pUnitT;          // F3

                pNew2E = pUnitER->pT;     // F1
                pNew2T = pUnitT;          // F3
            }
            else
            {
                //                before                        after
                //
                //                 <p1>                          <p1n>          .
                //              0 .    \ 1                    0 /    \ 1        .
                //               .      \                      /      \         .
                //              .        \                    /        \        .
                //           <p2>         \                <p2n>       <p2n>    .
                //        0 /   \ 1        \            0 .  \ 1    0 .   \ 1   .
                //         /     \          \            .    \      .     \    .
                //        /       \          \          .      \    .       \   .
                //       F0       F1         F3        F0      F3  F1       F3  .
                //                                 pNew1E   pNew1T  pNew2E   pNew2T
                //
                pNew1E = Unit_Not(pUnitER->pE);  // F0
                pNew1T =          pUnitT;        // F3

                pNew2E = Unit_Not(pUnitER->pT);  // F1
                pNew2T =          pUnitT;        // F3
            }
            // subtract ref counter - on the level P2
            pUnitER->n--;
            // subtract ref counter - on other levels
            pUnitT->n--;  ///

            // mark the change in the APL weights
            if ( p->fMinApl )
            {
                pUnitER->Weight -= AplWeightHalf;
                AplWeightAfter  -= AplWeightHalf;
            }
        }
        else if ( pUnitT->lev == lev1 )
        {
            //                before                        after
            //
            //                 <p1>                          <p1n>           .
            //              0 /    \ 1                    0 /    \ 1         .
            //               /      \                      /      \          .
            //              /        \                    /        \         .
            //             /         <p2>              <p2n>       <p2n>     .
            //            /       0 /   \ 1          0 /  \ 1    0 /   \ 1   .
            //           /         /     \            /    \      /     \    .
            //          /         /       \          /      \    /       \   .
            //         F0        F2       F3        F0      F2  F0       F3  .
            //                                 pNew1E   pNew1T  pNew2E   pNew2T
            //
            pNew1E =     pUnitE;    // F0
            pNew1T = pUnitT->pE;    // F2

            pNew2E =     pUnitE;    // F0
            pNew2T = pUnitT->pT;    // F3

            // subtract incoming edge counter - on the level P2
            pUnitT->n--;
            // subtract ref counter - on other levels
            pUnitER->n--;  ///

            // mark the change in the APL weights
            if ( p->fMinApl )
            {
                pUnitT->Weight  -= AplWeightHalf;
                AplWeightAfter  -= AplWeightHalf;
            }
        }
        else 
        {
            assert( 0 ); // should never happen
        }


        // consider all the cases except the last one
        if ( pNew1E == pNew1T )
        {
            pNewPlane20 = pNew1T;
            
            if ( p->fMinWidth )
            {
                // update the cofactors's top ref
                if ( pNew1T->TopRef > lev0 ) // the cofactor's top ref level is one of the current two levels
                {
                    pNew1T->TopRefNew = lev1;
                    if ( pNew1T->Sign  != p->nSwaps )
                    {
                        pNew1T->Sign      = p->nSwaps;  // set the current signature
                        p->pWidthCofs[ nWidthCofs++ ] = pNew1T;
                    }
                }
            }
        }
        else
        {
            // pNew1T can be complemented
            fCompT = Cudd_IsComplement(pNew1T);
            if ( fCompT )
            {
                pNew1E = Unit_Not(pNew1E);
                pNew1T = Unit_Not(pNew1T);
            }

            // check the hash-table 
            fFound = 0;
            for (  HKey = hashKey3(p->Signature, pNew1E, pNew1T, p->nTableSize);
                   p->HTable[HKey].Sign == p->Signature;
                   HKey = (HKey+1) % p->nTableSize )
            if ( p->HTable[HKey].Arg1 == pNew1E && p->HTable[HKey].Arg2 == pNew1T )
            { // the entry is present 
                // assign this entry
                pNewPlane20 = p->HTable[HKey].Arg3;
                assert( pNewPlane20->lev == lev1 );
                fFound = 1;
                p->HashSuccess++;
                break;
            }

            if ( !fFound )
            { // create the new entry
                pNewPlane20 = reoUnitsGetNextUnit( p ); // increments the unit counter
                pNewPlane20->pE  = pNew1E;
                pNewPlane20->pT  = pNew1T;
                pNewPlane20->n   = 0;       // ref will be added later
                pNewPlane20->lev = lev1; 
                if ( p->fMinWidth )
                {
                    pNewPlane20->TopRef = lev1;
                    pNewPlane20->Sign   = 0;
                }
                // set the weight of this node
                if ( p->fMinApl )
                    pNewPlane20->Weight = 0.0;

                // increment ref counters of children
                pNew1ER = Unit_Regular(pNew1E);
                pNew1ER->n++;  //
                pNew1T->n++;   //

                // insert into the data structure
                AddToLinkedList( ppListNew1, pNewPlane20 );

                // add this entry to cache
                assert( p->HTable[HKey].Sign != p->Signature );
                p->HTable[HKey].Sign = p->Signature;
                p->HTable[HKey].Arg1 = pNew1E;
                p->HTable[HKey].Arg2 = pNew1T;
                p->HTable[HKey].Arg3 = pNewPlane20;

                nNodesUnrefAdded++;
                        
                if ( p->fMinWidth )
                {
                    // update the cofactors's top ref
                    if ( pNew1ER->TopRef > lev0 ) // the cofactor's top ref level is one of the current two levels
                    {
                        if ( pNew1ER->Sign != p->nSwaps )
                        {
                            pNew1ER->TopRefNew = lev2;
                            if ( pNew1ER->Sign != p->nSwaps )
                            {
                                pNew1ER->Sign      = p->nSwaps;  // set the current signature
                                p->pWidthCofs[ nWidthCofs++ ] = pNew1ER;
                            }
                        }
                        // otherwise the level is already set correctly
                        else
                        {
                            assert( pNew1ER->TopRefNew == lev1 || pNew1ER->TopRefNew == lev2 );
                        }
                    }
                    // update the cofactors's top ref
                    if ( pNew1T->TopRef > lev0 ) // the cofactor's top ref level is one of the current two levels
                    {
                        if ( pNew1T->Sign != p->nSwaps )
                        {
                            pNew1T->TopRefNew = lev2;
                            if ( pNew1T->Sign  != p->nSwaps )
                            {
                                pNew1T->Sign      = p->nSwaps;  // set the current signature
                                p->pWidthCofs[ nWidthCofs++ ] = pNew1T;
                            }
                        }
                        // otherwise the level is already set correctly
                        else
                        {
                            assert( pNew1T->TopRefNew == lev1 || pNew1T->TopRefNew == lev2 );
                        }
                    }
                }
            }

            if ( p->fMinApl )
            {
                // increment the weight of this node
                pNewPlane20->Weight += AplWeightHalf;
                // mark the change in the APL weight
                AplWeightAfter      += AplWeightHalf;
                // update the total weight of this level
                AplWeightTotalLev1  += AplWeightHalf;
            }

            if ( fCompT )
                pNewPlane20 = Unit_Not(pNewPlane20);
        }

        if ( pNew2E == pNew2T )
        {
            pNewPlane21 = pNew2T;
            
            if ( p->fMinWidth )
            {
                // update the cofactors's top ref
                if ( pNew2T->TopRef > lev0 ) // the cofactor's top ref level is one of the current two levels
                {
                    pNew2T->TopRefNew = lev1;
                    if ( pNew2T->Sign != p->nSwaps )
                    {
                        pNew2T->Sign      = p->nSwaps;  // set the current signature
                        p->pWidthCofs[ nWidthCofs++ ] = pNew2T;
                    }
                }
            }
        }
        else
        {
            assert( !Cudd_IsComplement(pNew2T) );

            // check the hash-table
            fFound = 0;
            for (  HKey = hashKey3(p->Signature, pNew2E, pNew2T, p->nTableSize);
                   p->HTable[HKey].Sign == p->Signature;
                   HKey = (HKey+1) % p->nTableSize )
            if ( p->HTable[HKey].Arg1 == pNew2E && p->HTable[HKey].Arg2 == pNew2T )
            { // the entry is present 
                // assign this entry
                pNewPlane21 = p->HTable[HKey].Arg3;
                assert( pNewPlane21->lev == lev1 );
                fFound = 1;
                p->HashSuccess++;
                break;
            }

            if ( !fFound )
            { // create the new entry
                pNewPlane21 = reoUnitsGetNextUnit( p ); // increments the unit counter
                pNewPlane21->pE  = pNew2E;
                pNewPlane21->pT  = pNew2T;
                pNewPlane21->n   = 0;       // ref will be added later
                pNewPlane21->lev = lev1; 
                if ( p->fMinWidth )
                {
                    pNewPlane21->TopRef = lev1;
                    pNewPlane21->Sign   = 0;
                }
                // set the weight of this node
                if ( p->fMinApl )
                    pNewPlane21->Weight = 0.0;

                // increment ref counters of children
                pNew2ER = Unit_Regular(pNew2E);
                pNew2ER->n++; //
                pNew2T->n++;  //

                // insert into the data structure
//                reoUnitsAddUnitToPlane( &P2new, pNewPlane21 );
                AddToLinkedList( ppListNew1, pNewPlane21 );

                // add this entry to cache
                assert( p->HTable[HKey].Sign != p->Signature );
                p->HTable[HKey].Sign = p->Signature;
                p->HTable[HKey].Arg1 = pNew2E;
                p->HTable[HKey].Arg2 = pNew2T;
                p->HTable[HKey].Arg3 = pNewPlane21;

                nNodesUnrefAdded++;

                        
                if ( p->fMinWidth )
                {
                    // update the cofactors's top ref
                    if ( pNew2ER->TopRef > lev0 ) // the cofactor's top ref level is one of the current two levels
                    {
                        if ( pNew2ER->Sign != p->nSwaps )
                        {
                            pNew2ER->TopRefNew = lev2;
                            if ( pNew2ER->Sign != p->nSwaps )
                            {
                                pNew2ER->Sign      = p->nSwaps;  // set the current signature
                                p->pWidthCofs[ nWidthCofs++ ] = pNew2ER;
                            }
                        }
                        // otherwise the level is already set correctly
                        else
                        {
                            assert( pNew2ER->TopRefNew == lev1 || pNew2ER->TopRefNew == lev2 );
                        }
                    }
                    // update the cofactors's top ref
                    if ( pNew2T->TopRef > lev0 ) // the cofactor's top ref level is one of the current two levels
                    {
                        if ( pNew2T->Sign != p->nSwaps )
                        {
                            pNew2T->TopRefNew = lev2;
                            if ( pNew2T->Sign != p->nSwaps )
                            {
                                pNew2T->Sign      = p->nSwaps;  // set the current signature
                                p->pWidthCofs[ nWidthCofs++ ] = pNew2T;
                            }
                        }
                        // otherwise the level is already set correctly
                        else
                        {
                            assert( pNew2T->TopRefNew == lev1 || pNew2T->TopRefNew == lev2 );
                        }
                    }
                }
            }

            if ( p->fMinApl )
            {
                // increment the weight of this node
                pNewPlane21->Weight += AplWeightHalf;
                // mark the change in the APL weight
                AplWeightAfter      += AplWeightHalf;
                // update the total weight of this level
                AplWeightTotalLev1  += AplWeightHalf;
            }
        }
        // in all cases, the node will be added to the plane-1
        // this should be the same node (pUnit) as was originally there
        // because it is referenced by the above nodes

        assert( !Cudd_IsComplement(pNewPlane21) );
        // should be the case; otherwise reordering is not a local operation

        pUnit->pE  = pNewPlane20;
        pUnit->pT  = pNewPlane21;
        assert( pUnit->lev == lev0 ); 
        // reference counter remains the same; the APL weight remains the same

        // increment ref counters of children
        pNewPlane20R = Unit_Regular(pNewPlane20);
        pNewPlane20R->n++; ///
        pNewPlane21->n++;  ///

        // insert into the data structure
        AddToLinkedList( ppListNew0, pUnit );
        if ( p->fMinApl )
            AplWeightTotalLev0 += pUnit->Weight;
    }

    // (3) walk through the old lower level, find those nodes whose ref counters are not zero, 
    //     and move them to the new uppoer level, free other nodes
    for ( pLoop = pListOld1; pLoop; )
    {
        pUnit = pLoop;
        pLoop = pLoop->Next;
        if ( pUnit->n )
        { 
            assert( !p->fMinApl || pUnit->Weight > 0.0 );
            // the node should be added to the new level
            // no need to check the hash table
            pUnit->lev = lev0;
            AddToLinkedList( ppListNew0, pUnit );
            if ( p->fMinApl )
                AplWeightTotalLev0 += pUnit->Weight;

            nNodesDownMovedUp++;

            if ( p->fMinWidth )
            {
                pUnitER = Unit_Regular(pUnit->pE);
                pUnitT  = pUnit->pT;

                // update the cofactors's top ref
                if ( pUnitER->TopRef > lev0 ) // the cofactor's top ref level is one of the current two levels
                {
                    pUnitER->TopRefNew = lev1;
                    if ( pUnitER->Sign != p->nSwaps )
                    {
                        pUnitER->Sign      = p->nSwaps;  // set the current signature
                        p->pWidthCofs[ nWidthCofs++ ] = pUnitER;
                    }
                }
                if ( pUnitT->TopRef > lev0 ) // the cofactor's top ref level is one of the current two levels
                {
                    pUnitT->TopRefNew = lev1;
                    if ( pUnitT->Sign != p->nSwaps )
                    {
                        pUnitT->Sign      = p->nSwaps;  // set the current signature
                        p->pWidthCofs[ nWidthCofs++ ] = pUnitT;
                    }
                }
            }
        }
        else
        {
            assert( !p->fMinApl || pUnit->Weight == 0.0 );
            // decrement reference counters of children
            pUnitER = Unit_Regular(pUnit->pE);
            pUnitT  = pUnit->pT;
            pUnitER->n--;  ///
            pUnitT->n--;   ///
            // the node should be thrown away
            reoUnitsRecycleUnit( p, pUnit );
            nNodesUnrefRemoved++;
        }
    }

finish:

    // attach the new levels to the planes
    p->pPlanes[lev0].pHead = pListNew0;
    p->pPlanes[lev1].pHead = pListNew1;

    // swap the sift status
    temp                     = p->pPlanes[lev0].fSifted;
    p->pPlanes[lev0].fSifted = p->pPlanes[lev1].fSifted;
    p->pPlanes[lev1].fSifted = temp;

    // swap variables in the variable map
    if ( p->pOrderInt )
    {
        temp               = p->pOrderInt[lev0];
        p->pOrderInt[lev0] = p->pOrderInt[lev1];
        p->pOrderInt[lev1] = temp;
    }

    // adjust the node profile
    p->pPlanes[lev0].statsNodes  -= (nNodesUpMovedDown - nNodesDownMovedUp);
    p->pPlanes[lev1].statsNodes  -= (nNodesDownMovedUp - nNodesUpMovedDown) + nNodesUnrefRemoved - nNodesUnrefAdded;
    p->nNodesCur                 -=  nNodesUnrefRemoved - nNodesUnrefAdded;

    // adjust the node profile on this level
    if ( p->fMinWidth )
    {
        for ( c = 0; c < nWidthCofs; c++ )
        {
            if ( p->pWidthCofs[c]->TopRefNew < p->pWidthCofs[c]->TopRef )
            {
                p->pWidthCofs[c]->TopRef = p->pWidthCofs[c]->TopRefNew;
                nWidthReduction--;
            }
            else if ( p->pWidthCofs[c]->TopRefNew > p->pWidthCofs[c]->TopRef )
            {
                p->pWidthCofs[c]->TopRef = p->pWidthCofs[c]->TopRefNew;
                nWidthReduction++;
            }
        }
        // verify that the profile is okay
        reoProfileWidthVerifyLevel( p->pPlanes + lev0, lev0 );
        reoProfileWidthVerifyLevel( p->pPlanes + lev1, lev1 );

        // compute the total gain in terms of width
        nCostGain = (nNodesDownMovedUp - nNodesUpMovedDown + nNodesUnrefRemoved - nNodesUnrefAdded) + nWidthReduction;
        // adjust the width on this level
        p->pPlanes[lev1].statsWidth -= (int)nCostGain; 
        // set the cost
        p->pPlanes[lev1].statsCost   = p->pPlanes[lev1].statsWidth;
    }
    else if ( p->fMinApl )
    {
        // compute the total gain in terms of APL
        nCostGain = AplWeightPrev - AplWeightAfter;
        // make sure that the ALP is updated correctly
//        assert( p->pPlanes[lev0].statsCost + p->pPlanes[lev1].statsCost - nCostGain ==
//                AplWeightTotalLev0 + AplWeightTotalLev1 );                
        // adjust the profile 
        p->pPlanes[lev0].statsApl  = AplWeightTotalLev0;
        p->pPlanes[lev1].statsApl  = AplWeightTotalLev1;
        // set the cost
        p->pPlanes[lev0].statsCost = p->pPlanes[lev0].statsApl;
        p->pPlanes[lev1].statsCost = p->pPlanes[lev1].statsApl;
    }
    else
    {
        // compute the total gain in terms of the number of nodes
        nCostGain = nNodesUnrefRemoved - nNodesUnrefAdded;
        // adjust the profile (adjusted above)
        // set the cost
        p->pPlanes[lev0].statsCost   = p->pPlanes[lev0].statsNodes;
        p->pPlanes[lev1].statsCost   = p->pPlanes[lev1].statsNodes;
    }

    return nCostGain;
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

