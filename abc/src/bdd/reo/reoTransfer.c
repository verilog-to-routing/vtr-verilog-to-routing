/**CFile****************************************************************

  FileName    [reoTransfer.c]

  PackageName [REO: A specialized DD reordering engine.]

  Synopsis    [Transfering a DD from the CUDD manager into REO"s internal data structures and back.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 15, 2002.]

  Revision    [$Id: reoTransfer.c,v 1.0 2002/15/10 03:00:00 alanmi Exp $]

***********************************************************************/

#include "reo.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transfers the DD into the internal reordering data structure.]

  Description [It is important that the hash table is lossless.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
reo_unit * reoTransferNodesToUnits_rec( reo_man * p, DdNode * F )
{
    DdManager * dd = p->dd;
    reo_unit * pUnit;
    int HKey = -1; // Suppress "might be used uninitialized"
        int fComp;
    
    fComp = Cudd_IsComplement(F);
    F = Cudd_Regular(F);

    // check the hash-table
    if ( F->ref != 1 )
    {
        // search cache - use linear probing
        for ( HKey = hashKey2(p->Signature,F,p->nTableSize); p->HTable[HKey].Sign == p->Signature; HKey = (HKey+1) % p->nTableSize )
            if ( p->HTable[HKey].Arg1 == (reo_unit *)F )
            {
                pUnit = p->HTable[HKey].Arg2;  
                assert( pUnit );
                // increment the edge counter
                pUnit->n++;
                return Unit_NotCond( pUnit, fComp );
            }
    }
    // the entry in not found in the cache
    
    // create a new entry
    pUnit         = reoUnitsGetNextUnit( p );
    pUnit->n      = 1;
    if ( cuddIsConstant(F) )
    {
        pUnit->lev    = REO_CONST_LEVEL;
        pUnit->pE     = (reo_unit*)(ABC_PTRUINT_T)(cuddV(F));
        pUnit->pT     = NULL;
        // check if the diagram that is being reordering has complement edges
        if ( F != dd->one )
            p->fThisIsAdd = 1;
        // insert the unit into the corresponding plane
        reoUnitsAddUnitToPlane( &(p->pPlanes[p->nSupp]), pUnit ); // increments the unit counter
    }
    else
    {
        pUnit->lev    = p->pMapToPlanes[F->index];
        pUnit->pE     = reoTransferNodesToUnits_rec( p, cuddE(F) );
        pUnit->pT     = reoTransferNodesToUnits_rec( p, cuddT(F) );
        // insert the unit into the corresponding plane
        reoUnitsAddUnitToPlane( &(p->pPlanes[pUnit->lev]), pUnit ); // increments the unit counter
    }

    // add to the hash table
    if ( F->ref != 1 )
    {
        // the next free entry is already found - it is pointed to by HKey
        // while we traversed the diagram, the hash entry to which HKey points,
        // might have been used. Make sure that its signature is different.
        for ( ; p->HTable[HKey].Sign == p->Signature; HKey = (HKey+1) % p->nTableSize );
        p->HTable[HKey].Sign = p->Signature;
        p->HTable[HKey].Arg1 = (reo_unit *)F;
        p->HTable[HKey].Arg2 = pUnit;
    }

    // increment the counter of nodes
    p->nNodesCur++;
    return Unit_NotCond( pUnit, fComp );
}

/**Function*************************************************************

  Synopsis    [Creates the DD from the internal reordering data structure.]

  Description [It is important that the hash table is lossless.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * reoTransferUnitsToNodes_rec( reo_man * p, reo_unit * pUnit )
{
    DdManager * dd = p->dd;
    DdNode * bRes, * E, * T;
    int HKey = -1; // Suppress "might be used uninitialized"
        int fComp;

    fComp = Cudd_IsComplement(pUnit);
    pUnit = Unit_Regular(pUnit);

    // check the hash-table
    if ( pUnit->n != 1 )
    {
        for ( HKey = hashKey2(p->Signature,pUnit,p->nTableSize); p->HTable[HKey].Sign == p->Signature; HKey = (HKey+1) % p->nTableSize )
            if ( p->HTable[HKey].Arg1 == pUnit )
            {
                bRes = (DdNode*) p->HTable[HKey].Arg2;  
                assert( bRes );
                return Cudd_NotCond( bRes, fComp );
            }
    }

    // treat the case of constants
    if ( Unit_IsConstant(pUnit) )
    {
        bRes = cuddUniqueConst( dd, ((double)((int)(ABC_PTRUINT_T)(pUnit->pE))) );
        cuddRef( bRes );
    }
    else
    {
        // split and recur on children of this node
        E = reoTransferUnitsToNodes_rec( p, pUnit->pE );
        if ( E == NULL )
            return NULL;
        cuddRef(E);

        T = reoTransferUnitsToNodes_rec( p, pUnit->pT );
        if ( T == NULL )
        {
            Cudd_RecursiveDeref(dd, E);
            return NULL;
        }
        cuddRef(T);
        
        // consider the case when Res0 and Res1 are the same node
        assert( E != T );
        assert( !Cudd_IsComplement(T) );

        bRes = cuddUniqueInter( dd, p->pMapToDdVarsFinal[pUnit->lev], T, E );
        if ( bRes == NULL ) 
        {
            Cudd_RecursiveDeref(dd,E);
            Cudd_RecursiveDeref(dd,T);
            return NULL;
        }
        cuddRef( bRes );
        cuddDeref( E );
        cuddDeref( T );
    }

    // do not keep the result if the ref count is only 1, since it will not be visited again
    if ( pUnit->n != 1 )
    {
         // while we traversed the diagram, the hash entry to which HKey points,
         // might have been used. Make sure that its signature is different.
         for ( ; p->HTable[HKey].Sign == p->Signature; HKey = (HKey+1) % p->nTableSize );
         p->HTable[HKey].Sign = p->Signature;
         p->HTable[HKey].Arg1 = pUnit;
         p->HTable[HKey].Arg2 = (reo_unit *)bRes;  

         // add the DD to the referenced DD list in order to be able to store it in cache
         p->pRefNodes[p->nRefNodes++] = bRes;  Cudd_Ref( bRes ); 
         // no need to do this, because the garbage collection will not take bRes away
         // it is held by the diagram in the making
    }
    // increment the counter of nodes
    p->nNodesCur++;
    cuddDeref( bRes );
    return Cudd_NotCond( bRes, fComp );
}

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

