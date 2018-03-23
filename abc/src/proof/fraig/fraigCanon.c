/**CFile****************************************************************

  FileName    [fraigCanon.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [AND-node creation and elementary AND-operation.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - October 1, 2004]

  Revision    [$Id: fraigCanon.c,v 1.4 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#include <limits.h>
#include "fraigInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [The internal AND operation for the two FRAIG nodes.]

  Description [This procedure is the core of the FRAIG package, because
  it performs the two-step canonicization of FRAIG nodes. The first step
  involves the lookup in the structural hash table (which hashes two ANDs 
  into a node that has them as fanins, if such a node exists). If the node 
  is not found in the structural hash table, an attempt is made to find a 
  functionally equivalent node in another hash table (which hashes the 
  simulation info into the nodes, which has this simulation info). Some 
  tricks used on the way are described in the comments to the code and
  in the paper "FRAIGs: Functionally reduced AND-INV graphs".]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeAndCanon( Fraig_Man_t * pMan, Fraig_Node_t * p1, Fraig_Node_t * p2 )
{
    Fraig_Node_t * pNodeNew, * pNodeOld, * pNodeRepr;
    int fUseSatCheck;
//    int RetValue;

    // check for trivial cases
    if ( p1 == p2 )
        return p1;
    if ( p1 == Fraig_Not(p2) )
        return Fraig_Not(pMan->pConst1);
    if ( Fraig_NodeIsConst(p1) )
    {
        if ( p1 == pMan->pConst1 )
            return p2;
        return Fraig_Not(pMan->pConst1);
    }
    if ( Fraig_NodeIsConst(p2) )
    {
        if ( p2 == pMan->pConst1 )
            return p1;
        return Fraig_Not(pMan->pConst1);
    }
/*
    // check for less trivial cases
    if ( Fraig_IsComplement(p1) )
    {
        if ( RetValue = Fraig_NodeIsInSupergate( Fraig_Regular(p1), p2 ) )
        {
            if ( RetValue == -1 )
                pMan->nImplies0++;
            else
                pMan->nImplies1++;

            if ( RetValue == -1 )
                return p2;
        }
    }
    else
    {
        if ( RetValue = Fraig_NodeIsInSupergate( p1, p2 ) )
        {
            if ( RetValue == 1 )
                pMan->nSimplifies1++;
            else
                pMan->nSimplifies0++;

            if ( RetValue == 1 )
                return p1;
            return Fraig_Not(pMan->pConst1);
        }
    }
 
    if ( Fraig_IsComplement(p2) )
    {
        if ( RetValue = Fraig_NodeIsInSupergate( Fraig_Regular(p2), p1 ) )
        {
            if ( RetValue == -1 )
                pMan->nImplies0++;
            else
                pMan->nImplies1++;

            if ( RetValue == -1 )
                return p1;
        }
    }
    else
    {
        if ( RetValue = Fraig_NodeIsInSupergate( p2, p1 ) )
        {
            if ( RetValue == 1 )
                pMan->nSimplifies1++;
            else
                pMan->nSimplifies0++;

            if ( RetValue == 1 )
                return p2;
            return Fraig_Not(pMan->pConst1);
        }
    }
*/
    // perform level-one structural hashing
    if ( Fraig_HashTableLookupS( pMan, p1, p2, &pNodeNew ) ) // the node with these children is found
    {
        // if the existent node is part of the cone of unused logic
        // (that is logic feeding the node which is equivalent to the given node)
        // return the canonical representative of this node
        // determine the phase of the given node, with respect to its canonical form
        pNodeRepr = Fraig_Regular(pNodeNew)->pRepr;
        if ( pMan->fFuncRed && pNodeRepr )
            return Fraig_NotCond( pNodeRepr, Fraig_IsComplement(pNodeNew) ^ Fraig_NodeComparePhase(Fraig_Regular(pNodeNew), pNodeRepr) );
        // otherwise, the node is itself a canonical representative, return it
        return pNodeNew;
    }
    // the same node is not found, but the new one is created

    // if one level hashing is requested (without functionality hashing), return
    if ( !pMan->fFuncRed )
        return pNodeNew;

    // check if the new node is unique using the simulation info
    if ( pNodeNew->nOnes == 0 || pNodeNew->nOnes == (unsigned)pMan->nWordsRand * 32 )
    {
        pMan->nSatZeros++;
        if ( !pMan->fDoSparse ) // if we do not do sparse functions, skip
            return pNodeNew;
        // check the sparse function simulation hash table
        pNodeOld = Fraig_HashTableLookupF0( pMan, pNodeNew );
        if ( pNodeOld == NULL ) // the node is unique (it is added to the table)
            return pNodeNew;
    }
    else
    {
        // check the simulation hash table
        pNodeOld = Fraig_HashTableLookupF( pMan, pNodeNew );
        if ( pNodeOld == NULL ) // the node is unique
            return pNodeNew;
    }
    assert( pNodeOld->pRepr == 0 );
    // there is another node which looks the same according to simulation

    // use SAT to resolve the ambiguity
    fUseSatCheck = (pMan->nInspLimit == 0 || Fraig_ManReadInspects(pMan) < pMan->nInspLimit); 
    if ( fUseSatCheck && Fraig_NodeIsEquivalent( pMan, pNodeOld, pNodeNew, pMan->nBTLimit, 1000000 ) )
    {
        // set the node to be equivalent with this node
        // to prevent loops, only set if the old node is not in the TFI of the new node
        // the loop may happen in the following case: suppose 
        // NodeC = AND(NodeA, NodeB) and at the same time NodeA => NodeB
        // in this case, NodeA and NodeC are functionally equivalent
        // however, NodeA is a fanin of node NodeC (this leads to the loop)
        // add the node to the list of equivalent nodes or dereference it
        if ( pMan->fChoicing && !Fraig_CheckTfi( pMan, pNodeOld, pNodeNew ) )
        { 
            // if the old node is not in the TFI of the new node and choicing 
            // is enabled, add the new node to the list of equivalent ones
            pNodeNew->pNextE = pNodeOld->pNextE;
            pNodeOld->pNextE = pNodeNew;
        }
        // set the canonical representative of this node
        pNodeNew->pRepr = pNodeOld;
        // return the equivalent node
        return Fraig_NotCond( pNodeOld, Fraig_NodeComparePhase(pNodeOld, pNodeNew) );
    }

    // now we add another member to this simulation class
    if ( pNodeNew->nOnes == 0 || pNodeNew->nOnes == (unsigned)pMan->nWordsRand * 32 )
    {
        Fraig_Node_t * pNodeTemp;
        assert( pMan->fDoSparse );
        pNodeTemp = Fraig_HashTableLookupF0( pMan, pNodeNew );
//        assert( pNodeTemp == NULL );
//        Fraig_HashTableInsertF0( pMan, pNodeNew );
    }
    else
    {
        pNodeNew->pNextD = pNodeOld->pNextD;
        pNodeOld->pNextD = pNodeNew;
    }
    // return the new node
    assert( pNodeNew->pRepr == 0 );
    return pNodeNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

