/**CFile****************************************************************

  FileName    [fraigTrans.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Adds the additive and distributive choices to the AIG.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fraigTrans.c,v 1.1 2005/02/28 05:34:34 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Adds choice nodes based on associativity.]

  Description [Make nLimit big AND gates and add all decompositions
               to the Fraig.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManAddChoices( Fraig_Man_t * pMan, int fVerbose, int nLimit )
{
//    ProgressBar * pProgress;
    char Buffer[100];
    abctime clkTotal = Abc_Clock();
    int i, nNodesBefore, nNodesAfter, nInputs, nMaxNodes;
    int /*nMaxLevel,*/ nDistributive;
    Fraig_Node_t *pNode, *pRepr;
    Fraig_Node_t *pX, *pA, *pB, *pC, /* *pD,*/ *pN, /* *pQ, *pR,*/ *pT;
    int fShortCut = 0;

    nDistributive = 0;

//    Fraig_ManSetApprox( pMan, 1 );

    // NO functional reduction
    if (fShortCut) Fraig_ManSetFuncRed( pMan, 0 );

    // First we mark critical functions i.e. compute those
    // nodes which lie on the critical path. Note that this
    // doesn't update the required times on any choice nodes
    // which are not the representatives
/*
    nMaxLevel = Fraig_GetMaxLevel( pMan );
    for ( i = 0; i < pMan->nOutputs; i++ )
    {
        Fraig_SetNodeRequired( pMan, pMan->pOutputs[i], nMaxLevel );
    }
*/
    nNodesBefore = Fraig_ManReadNodeNum( pMan );
    nInputs      = Fraig_ManReadInputNum( pMan );
    nMaxNodes    = nInputs + nLimit * ( nNodesBefore - nInputs );

    printf ("Limit = %d, Before = %d\n", nMaxNodes, nNodesBefore );

    if (0) 
    {
        char buffer[128];
        sprintf (buffer, "test" );
//        Fraig_MappingShow( pMan, buffer );
    }

//    pProgress = Extra_ProgressBarStart( stdout, nMaxNodes );
Fraig_ManCheckConsistency( pMan );

    for ( i = nInputs+1; (i < Fraig_ManReadNodeNum( pMan )) 
            && (nMaxNodes > Fraig_ManReadNodeNum( pMan )); ++i )
    {
//        if ( i == nNodesBefore )
//            break;

        pNode = Fraig_ManReadIthNode( pMan, i );
        assert ( pNode );

        pRepr = pNode->pRepr ? pNode->pRepr : pNode;
        //printf ("Slack: %d\n", Fraig_NodeReadSlack( pRepr ));
        
        // All the new associative choices we add will have huge slack
        // since we do not redo timing, and timing doesnt handle choices
        // well anyway. However every newly added node is a choice of an
        // existing critical node, so they are considered critical.
//        if ( (Fraig_NodeReadSlack( pRepr ) > 3) && (i < nNodesBefore)  )     
//            continue;

//        if ( pNode->pRepr )
//            continue;

        // Try ((ab)c), x = ab -> (a(bc)) and (b(ac))
        pX = Fraig_NodeReadOne(pNode);
        pC = Fraig_NodeReadTwo(pNode);
        if (Fraig_NodeIsAnd(pX) && !Fraig_IsComplement(pX))
        {
            pA = Fraig_NodeReadOne(Fraig_Regular(pX));
            pB = Fraig_NodeReadTwo(Fraig_Regular(pX));

//            pA = Fraig_NodeGetRepr( pA );
//            pB = Fraig_NodeGetRepr( pB );
//            pC = Fraig_NodeGetRepr( pC );

            if (fShortCut) 
            {
                pT = Fraig_NodeAnd(pMan, pB, pC);
                if ( !pT->pRepr )
                {
                    pN = Fraig_NodeAnd(pMan, pA, pT);
//                    Fraig_NodeAddChoice( pMan, pNode, pN );
                }
            }
            else
                pN = Fraig_NodeAnd(pMan, pA, Fraig_NodeAnd(pMan, pB, pC));
            // assert ( Fraig_NodesEqual(pN, pNode) );


            if (fShortCut) 
            {
                pT = Fraig_NodeAnd(pMan, pA, pC);
                if ( !pT->pRepr )
                {
                    pN = Fraig_NodeAnd(pMan, pB, pT);
//                    Fraig_NodeAddChoice( pMan, pNode, pN );
                }
            }
            else
                pN = Fraig_NodeAnd(pMan, pB, Fraig_NodeAnd(pMan, pA, pC));
            // assert ( Fraig_NodesEqual(pN, pNode) );
        }


        // Try (a(bc)), x = bc -> ((ab)c) and ((ac)b)
        pA = Fraig_NodeReadOne(pNode);
        pX = Fraig_NodeReadTwo(pNode);
        if (Fraig_NodeIsAnd(pX) && !Fraig_IsComplement(pX))
        {
            pB = Fraig_NodeReadOne(Fraig_Regular(pX));
            pC = Fraig_NodeReadTwo(Fraig_Regular(pX));

//            pA = Fraig_NodeGetRepr( pA );
//            pB = Fraig_NodeGetRepr( pB );
//            pC = Fraig_NodeGetRepr( pC );

            if (fShortCut) 
            {
                pT = Fraig_NodeAnd(pMan, pA, pB);
                if ( !pT->pRepr )
                {
                    pN = Fraig_NodeAnd(pMan, pC, pT);
//                    Fraig_NodeAddChoice( pMan, pNode, pN );
                }
            }
            else
                pN = Fraig_NodeAnd(pMan, Fraig_NodeAnd(pMan, pA, pB), pC);
            // assert ( Fraig_NodesEqual(pN, pNode) );

            if (fShortCut) 
            {
                pT = Fraig_NodeAnd(pMan, pA, pC);
                if ( !pT->pRepr )
                {
                    pN = Fraig_NodeAnd(pMan, pB, pT);
//                    Fraig_NodeAddChoice( pMan, pNode, pN );
                }
            }
            else
                pN = Fraig_NodeAnd(pMan, Fraig_NodeAnd(pMan, pA, pC), pB);
            // assert ( Fraig_NodesEqual(pN, pNode) );
        }


/*
        // Try distributive transform
        pQ = Fraig_NodeReadOne(pNode);
        pR = Fraig_NodeReadTwo(pNode);
        if ( (Fraig_IsComplement(pQ) && Fraig_NodeIsAnd(pQ))
             && (Fraig_IsComplement(pR) && Fraig_NodeIsAnd(pR)) )
        {
            pA = Fraig_NodeReadOne(Fraig_Regular(pQ));
            pB = Fraig_NodeReadTwo(Fraig_Regular(pQ));
            pC = Fraig_NodeReadOne(Fraig_Regular(pR));
            pD = Fraig_NodeReadTwo(Fraig_Regular(pR));

            // Now detect the !(xy + xz) pattern, store 
            // x in pA, y in pB and z in pC and set pD = 0 to indicate
            // pattern was found
            assert (pD != 0);
            if (pA == pC) { pC = pD;                   pD = 0; } 
            if (pA == pD) {                            pD = 0; } 
            if (pB == pC) { pB = pA; pA = pC; pC = pD; pD = 0; }
            if (pB == pD) { pB = pA; pA = pD;          pD = 0; }
            if (pD == 0)
            {
                nDistributive++;
                pN = Fraig_Not(Fraig_NodeAnd(pMan, pA, 
                        Fraig_NodeOr(pMan, pB, pC)));
                if (fShortCut) Fraig_NodeAddChoice( pMan, pNode, pN );
                // assert ( Fraig_NodesEqual(pN, pNode) );
            }
        }
*/            
        if ( i % 1000 == 0 )
        {
            sprintf( Buffer, "Adding choice %6d...", i - nNodesBefore );
//            Extra_ProgressBarUpdate( pProgress, i, Buffer );
        }
    }

//    Extra_ProgressBarStop( pProgress );

Fraig_ManCheckConsistency( pMan );

    nNodesAfter = Fraig_ManReadNodeNum( pMan );
    printf ( "Nodes before = %6d. Nodes with associative choices = %6d. Increase = %4.2f %%.\n", 
            nNodesBefore, nNodesAfter, ((float)(nNodesAfter - nNodesBefore)) * 100.0/(nNodesBefore - nInputs) );
    printf ( "Distributive = %d\n", nDistributive );

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

