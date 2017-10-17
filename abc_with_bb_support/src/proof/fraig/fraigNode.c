/**CFile****************************************************************

  FileName    [fraigNode.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Implementation of the FRAIG node.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - October 1, 2004]

  Revision    [$Id: fraigNode.c,v 1.3 2005/07/08 01:01:32 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// returns the complemented attribute of the node
#define Fraig_NodeIsSimComplement(p) (Fraig_IsComplement(p)? !(Fraig_Regular(p)->fInv) : (p)->fInv)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the constant 1 node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeCreateConst( Fraig_Man_t * p )
{
    Fraig_Node_t * pNode;

    // create the node
    pNode = (Fraig_Node_t *)Fraig_MemFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Fraig_Node_t) );

    // assign the number and add to the array of nodes
    pNode->Num   = p->vNodes->nSize;
    Fraig_NodeVecPush( p->vNodes,  pNode );
    pNode->NumPi = -1;  // this is not a PI, so its number is -1
    pNode->Level =  0;  // just like a PI, it has 0 level
    pNode->nRefs =  1;  // it is a persistent node, which comes referenced
    pNode->fInv  =  1;  // the simulation info is complemented

    // create the simulation info
    pNode->puSimR = (unsigned *)Fraig_MemFixedEntryFetch( p->mmSims );
    pNode->puSimD = pNode->puSimR + p->nWordsRand;
    memset( pNode->puSimR, 0, sizeof(unsigned) * p->nWordsRand );
    memset( pNode->puSimD, 0, sizeof(unsigned) * p->nWordsDyna );

    // count the number of ones in the simulation vector
    pNode->nOnes = p->nWordsRand * sizeof(unsigned) * 8;

    // insert it into the hash table
    Fraig_HashTableLookupF0( p, pNode );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates a primary input node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeCreatePi( Fraig_Man_t * p )
{
    Fraig_Node_t * pNode, * pNodeRes;
    int i;
    abctime clk;

    // create the node
    pNode = (Fraig_Node_t *)Fraig_MemFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Fraig_Node_t) );
    pNode->puSimR = (unsigned *)Fraig_MemFixedEntryFetch( p->mmSims );
    pNode->puSimD = pNode->puSimR + p->nWordsRand;
    memset( pNode->puSimD, 0, sizeof(unsigned) * p->nWordsDyna );

    // assign the number and add to the array of nodes
    pNode->Num   = p->vNodes->nSize;
    Fraig_NodeVecPush( p->vNodes,  pNode );

    // assign the PI number and add to the array of primary inputs
    pNode->NumPi = p->vInputs->nSize;   
    Fraig_NodeVecPush( p->vInputs, pNode );

    pNode->Level =  0;  // PI has 0 level
    pNode->nRefs =  1;  // it is a persistent node, which comes referenced
    pNode->fInv  =  0;  // the simulation info of the PI is not complemented

    // derive the simulation info for the new node
clk = Abc_Clock();
    // set the random simulation info for the primary input
    pNode->uHashR = 0;
    for ( i = 0; i < p->nWordsRand; i++ )
    {
        // generate the simulation info
        pNode->puSimR[i] = FRAIG_RANDOM_UNSIGNED;
        // for reasons that take very long to explain, it makes sense to have (0000000...) 
        // pattern in the set (this helps if we need to return the counter-examples)
        if ( i == 0 )
            pNode->puSimR[i] <<= 1;
        // compute the hash key
        pNode->uHashR ^= pNode->puSimR[i] * s_FraigPrimes[i];
    }
    // count the number of ones in the simulation vector
    pNode->nOnes = Fraig_BitStringCountOnes( pNode->puSimR, p->nWordsRand );

    // set the systematic simulation info for the primary input
    pNode->uHashD = 0;
    for ( i = 0; i < p->iWordStart; i++ )
    {
        // generate the simulation info
        pNode->puSimD[i] = FRAIG_RANDOM_UNSIGNED;
        // compute the hash key
        pNode->uHashD ^= pNode->puSimD[i] * s_FraigPrimes[i];
    }
p->timeSims += Abc_Clock() - clk;

    // insert it into the hash table
    pNodeRes = Fraig_HashTableLookupF( p, pNode );
    assert( pNodeRes == NULL );
    // add to the runtime of simulation
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description [This procedure should be called to create the constant
  node and the PI nodes first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeCreate( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 )
{
    Fraig_Node_t * pNode;
    abctime clk;

    // create the node
    pNode = (Fraig_Node_t *)Fraig_MemFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Fraig_Node_t) );

    // assign the children
    pNode->p1  = p1;  Fraig_Ref(p1);  Fraig_Regular(p1)->nRefs++;
    pNode->p2  = p2;  Fraig_Ref(p2);  Fraig_Regular(p2)->nRefs++;

    // assign the number and add to the array of nodes
    pNode->Num = p->vNodes->nSize;
    Fraig_NodeVecPush( p->vNodes,  pNode );

    // assign the PI number
    pNode->NumPi = -1;

    // compute the level of this node
    pNode->Level = 1 + Abc_MaxInt(Fraig_Regular(p1)->Level, Fraig_Regular(p2)->Level);
    pNode->fInv  = Fraig_NodeIsSimComplement(p1) & Fraig_NodeIsSimComplement(p2);
    pNode->fFailTfo = Fraig_Regular(p1)->fFailTfo | Fraig_Regular(p2)->fFailTfo;

    // derive the simulation info 
clk = Abc_Clock();
    // allocate memory for the simulation info
    pNode->puSimR = (unsigned *)Fraig_MemFixedEntryFetch( p->mmSims );
    pNode->puSimD = pNode->puSimR + p->nWordsRand;
    // derive random simulation info
    pNode->uHashR = 0;
    Fraig_NodeSimulate( pNode, 0, p->nWordsRand, 1 );
    // derive dynamic simulation info
    pNode->uHashD = 0;
    Fraig_NodeSimulate( pNode, 0, p->iWordStart, 0 );
    // count the number of ones in the random simulation info
    pNode->nOnes = Fraig_BitStringCountOnes( pNode->puSimR, p->nWordsRand );
    if ( pNode->fInv )
        pNode->nOnes = p->nWordsRand * 32 - pNode->nOnes;
    // add to the runtime of simulation
p->timeSims += Abc_Clock() - clk;

#ifdef FRAIG_ENABLE_FANOUTS
    // create the fanout info
    Fraig_NodeAddFaninFanout( Fraig_Regular(p1), pNode );
    Fraig_NodeAddFaninFanout( Fraig_Regular(p2), pNode );
#endif
    return pNode;
}


/**Function*************************************************************

  Synopsis    [Simulates the node.]

  Description [Simulates the random or dynamic simulation info through 
  the node. Uses phases of the children to determine their real simulation
  info. Uses phase of the node to determine the way its simulation info 
  is stored. The resulting info is guaranteed to be 0 for the first pattern.]
  
  SideEffects [This procedure modified the hash value of the simulation info.]

  SeeAlso     []

***********************************************************************/
void Fraig_NodeSimulate( Fraig_Node_t * pNode, int iWordStart, int iWordStop, int fUseRand )
{
    unsigned * pSims, * pSims1, * pSims2;
    unsigned uHash;
    int fCompl, fCompl1, fCompl2, i;

    assert( !Fraig_IsComplement(pNode) );

    // get hold of the simulation information
    pSims  = fUseRand? pNode->puSimR                    : pNode->puSimD;
    pSims1 = fUseRand? Fraig_Regular(pNode->p1)->puSimR : Fraig_Regular(pNode->p1)->puSimD;
    pSims2 = fUseRand? Fraig_Regular(pNode->p2)->puSimR : Fraig_Regular(pNode->p2)->puSimD;

    // get complemented attributes of the children using their random info
    fCompl  = pNode->fInv;
    fCompl1 = Fraig_NodeIsSimComplement(pNode->p1);
    fCompl2 = Fraig_NodeIsSimComplement(pNode->p2);

    // simulate
    uHash = 0;
    if ( fCompl1 && fCompl2 )
    {
        if ( fCompl )
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims[i] = (pSims1[i] | pSims2[i]);
                uHash ^= pSims[i] * s_FraigPrimes[i];
            }
        else
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims[i] = ~(pSims1[i] | pSims2[i]);
                uHash ^= pSims[i] * s_FraigPrimes[i];
            }
    }
    else if ( fCompl1 && !fCompl2 )
    {
        if ( fCompl )
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims[i] = (pSims1[i] | ~pSims2[i]);
                uHash ^= pSims[i] * s_FraigPrimes[i];
            }
        else
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims[i] = (~pSims1[i] & pSims2[i]);
                uHash ^= pSims[i] * s_FraigPrimes[i];
            }
    }
    else if ( !fCompl1 && fCompl2 )
    {
        if ( fCompl )
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims[i] = (~pSims1[i] | pSims2[i]);
                uHash ^= pSims[i] * s_FraigPrimes[i];
            }
        else
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims[i] = (pSims1[i] & ~pSims2[i]);
                uHash ^= pSims[i] * s_FraigPrimes[i];
            }
    }
    else // if ( !fCompl1 && !fCompl2 )
    {
        if ( fCompl )
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims[i] = ~(pSims1[i] & pSims2[i]);
                uHash ^= pSims[i] * s_FraigPrimes[i];
            }
        else
            for ( i = iWordStart; i < iWordStop; i++ )
            {
                pSims[i] = (pSims1[i] & pSims2[i]);
                uHash ^= pSims[i] * s_FraigPrimes[i];
            }
    }

    if ( fUseRand )
        pNode->uHashR ^= uHash;
    else
        pNode->uHashD ^= uHash;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

