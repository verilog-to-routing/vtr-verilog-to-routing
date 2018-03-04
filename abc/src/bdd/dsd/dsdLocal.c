/**CFile****************************************************************

  FileName    [dsdLocal.c]

  PackageName [DSD: Disjoint-support decomposition package.]

  Synopsis    [Deriving the local function of the DSD node.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 8.0. Started - September 22, 2003.]

  Revision    [$Id: dsdLocal.c,v 1.0 2002/22/09 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dsdInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      STATIC VARIABLES                            ///
////////////////////////////////////////////////////////////////////////

static DdNode * Extra_dsdRemap( DdManager * dd, DdNode * bFunc, st__table * pCache,
    int * pVar2Form, int * pForm2Var, DdNode * pbCube0[], DdNode * pbCube1[] );
static DdNode * Extra_bddNodePointedByCube( DdManager * dd, DdNode * bF, DdNode * bC );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns the local function of the DSD node. ]

  Description [The local function is computed using the global function 
  of the node and the global functions of the formal inputs. The resulting
  local function is mapped using the topmost N variables of the manager.
  The number of variables N is equal to the number of formal inputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Dsd_TreeGetPrimeFunction( DdManager * dd, Dsd_Node_t * pNode ) 
{
    int * pForm2Var;   // the mapping of each formal input into its first var
    int * pVar2Form;   // the mapping of each var into its formal inputs
    int i, iVar, iLev, * pPermute;
    DdNode ** pbCube0, ** pbCube1;
    DdNode * bFunc, * bRes, * bTemp;
    st__table * pCache;
    
    pPermute  = ABC_ALLOC( int, dd->size );
    pVar2Form = ABC_ALLOC( int, dd->size );
    pForm2Var = ABC_ALLOC( int, dd->size );

    pbCube0 = ABC_ALLOC( DdNode *, dd->size );
    pbCube1 = ABC_ALLOC( DdNode *, dd->size );

    // remap the global function in such a way that
    // the support variables of each formal input are adjacent
    iLev = 0;
    for ( i = 0; i < pNode->nDecs; i++ )
    {
        pForm2Var[i] = dd->invperm[i];
        for ( bTemp = pNode->pDecs[i]->S; bTemp != b1; bTemp = cuddT(bTemp) )
        {
            iVar = dd->invperm[iLev];
            pPermute[bTemp->index] = iVar;
            pVar2Form[iVar] = i;
            iLev++;
        }

        // collect the cubes representing each assignment
        pbCube0[i] = Extra_bddGetOneCube( dd, Cudd_Not(pNode->pDecs[i]->G) );
        Cudd_Ref( pbCube0[i] );
        pbCube1[i] = Extra_bddGetOneCube( dd, pNode->pDecs[i]->G );
        Cudd_Ref( pbCube1[i] );
    }

    // remap the function
    bFunc = Cudd_bddPermute( dd, pNode->G, pPermute ); Cudd_Ref( bFunc );
    // remap the cube
    for ( i = 0; i < pNode->nDecs; i++ )
    {
        pbCube0[i] = Cudd_bddPermute( dd, bTemp = pbCube0[i], pPermute ); Cudd_Ref( pbCube0[i] );
        Cudd_RecursiveDeref( dd, bTemp );
        pbCube1[i] = Cudd_bddPermute( dd, bTemp = pbCube1[i], pPermute ); Cudd_Ref( pbCube1[i] );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    // remap the function
    pCache = st__init_table( st__ptrcmp, st__ptrhash);;
    bRes = Extra_dsdRemap( dd, bFunc, pCache, pVar2Form, pForm2Var, pbCube0, pbCube1 );  Cudd_Ref( bRes );
    st__free_table( pCache );

    Cudd_RecursiveDeref( dd, bFunc );
    for ( i = 0; i < pNode->nDecs; i++ )
    {
        Cudd_RecursiveDeref( dd, pbCube0[i] );
        Cudd_RecursiveDeref( dd, pbCube1[i] );
    }
/*
////////////
    // permute the function once again
    // in such a way that i-th var stood for i-th formal input
    for ( i = 0; i < dd->size; i++ )
        pPermute[i] = -1;
    for ( i = 0; i < pNode->nDecs; i++ )
        pPermute[dd->invperm[i]] = i;
    bRes = Cudd_bddPermute( dd, bTemp = bRes, pPermute ); Cudd_Ref( bRes );
    Cudd_RecursiveDeref( dd, bTemp );
////////////
*/
    ABC_FREE(pPermute);
    ABC_FREE(pVar2Form);
    ABC_FREE(pForm2Var);
    ABC_FREE(pbCube0);
    ABC_FREE(pbCube1);

    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_dsdRemap( DdManager * dd, DdNode * bF, st__table * pCache,
    int * pVar2Form, int * pForm2Var, DdNode * pbCube0[], DdNode * pbCube1[] )
{
    DdNode * bFR, * bF0, * bF1;
    DdNode * bRes0, * bRes1, * bRes;
    int iForm;
    
    bFR = Cudd_Regular(bF);
    if ( cuddIsConstant(bFR) )
        return bF;

    // check the hash-table
    if ( bFR->ref != 1 )
    {
        if ( st__lookup( pCache, (char *)bF, (char **)&bRes ) )
            return bRes;
    }

    // get the formal input
    iForm = pVar2Form[bFR->index];

    // get the nodes pointed to by the cube
    bF0 = Extra_bddNodePointedByCube( dd, bF, pbCube0[iForm] );
    bF1 = Extra_bddNodePointedByCube( dd, bF, pbCube1[iForm] );

    // call recursively for these nodes
    bRes0 = Extra_dsdRemap( dd, bF0, pCache, pVar2Form, pForm2Var, pbCube0, pbCube1 ); Cudd_Ref( bRes0 );
    bRes1 = Extra_dsdRemap( dd, bF1, pCache, pVar2Form, pForm2Var, pbCube0, pbCube1 ); Cudd_Ref( bRes1 );

    // derive the result using ITE
    bRes = Cudd_bddIte( dd, dd->vars[ pForm2Var[iForm] ], bRes1, bRes0 ); Cudd_Ref( bRes );
    Cudd_RecursiveDeref( dd, bRes0 );
    Cudd_RecursiveDeref( dd, bRes1 );

    // add to the hash table
    if ( bFR->ref != 1 )
        st__insert( pCache, (char *)bF, (char *)bRes );
    Cudd_Deref( bRes );
    return bRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Extra_bddNodePointedByCube( DdManager * dd, DdNode * bF, DdNode * bC )
{
    DdNode * bFR, * bCR;
    DdNode * bF0, * bF1;
    DdNode * bC0, * bC1;
    int LevelF, LevelC;

    assert( bC != b0 );
    if ( bC == b1 )
        return bF;

//    bRes = cuddCacheLookup2( dd, Extra_bddNodePointedByCube, bF, bC );
//    if ( bRes )
//      return bRes;
    // there is no need for caching because this operation is very fast
    // there will no gain reusing the results of this operations
    // instead, it will flush CUDD cache of other useful entries


    bFR = Cudd_Regular( bF ); 
    bCR = Cudd_Regular( bC ); 
    assert( !cuddIsConstant( bFR ) );

    LevelF = dd->perm[bFR->index];
    LevelC = dd->perm[bCR->index];

    if ( LevelF <= LevelC )
    {
        if ( bFR != bF )
        {
            bF0 = Cudd_Not( cuddE(bFR) );
            bF1 = Cudd_Not( cuddT(bFR) );
        }
        else
        {
            bF0 = cuddE(bFR);
            bF1 = cuddT(bFR);
        }
    }
    else
    {
        bF0 = bF1 = bF;
    }

    if ( LevelC <= LevelF )
    {
        if ( bCR != bC )
        {
            bC0 = Cudd_Not( cuddE(bCR) );
            bC1 = Cudd_Not( cuddT(bCR) );
        }
        else
        {
            bC0 = cuddE(bCR);
            bC1 = cuddT(bCR);
        }
    }
    else
    {
        bC0 = bC1 = bC;
    }

    assert( bC0 == b0 || bC1 == b0 );
    if ( bC0 == b0 )
        return Extra_bddNodePointedByCube( dd, bF1, bC1 );
    return Extra_bddNodePointedByCube( dd, bF0, bC0 );
}

#if 0

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * dsdTreeGetPrimeFunction( DdManager * dd, Dsd_Node_t * pNode, int fRemap ) 
{
    DdNode * bCof0,  * bCof1, * bCube0, * bCube1, * bNewFunc, * bTemp;
    int i;
    int fAllBuffs = 1;
    static int Permute[MAXINPUTS];

    assert( pNode );
    assert( !Dsd_IsComplement( pNode ) );
    assert( pNode->Type == DT_PRIME );

    // transform the function of this block to depend on inputs
    // corresponding to the formal inputs

    // first, substitute those inputs that have some blocks associated with them
    // second, remap the inputs to the top of the manager (then, it is easy to output them)

    // start the function
    bNewFunc = pNode->G;  Cudd_Ref( bNewFunc );
    // go over all primary inputs
    for ( i = 0; i < pNode->nDecs; i++ )
    if ( pNode->pDecs[i]->Type != DT_BUF ) // remap only if it is not the buffer
    {
        bCube0 = Extra_bddFindOneCube( dd, Cudd_Not(pNode->pDecs[i]->G) );  Cudd_Ref( bCube0 );
        bCof0 = Cudd_Cofactor( dd, bNewFunc, bCube0 );                     Cudd_Ref( bCof0 );
        Cudd_RecursiveDeref( dd, bCube0 );

        bCube1 = Extra_bddFindOneCube( dd,          pNode->pDecs[i]->G  );  Cudd_Ref( bCube1 );
        bCof1 = Cudd_Cofactor( dd, bNewFunc, bCube1 );                     Cudd_Ref( bCof1 );
        Cudd_RecursiveDeref( dd, bCube1 );

        Cudd_RecursiveDeref( dd, bNewFunc );

        // use the variable in the i-th level of the manager
//      bNewFunc = Cudd_bddIte( dd, dd->vars[dd->invperm[i]],bCof1,bCof0 );     Cudd_Ref( bNewFunc );
        // use the first variale in the support of the component
        bNewFunc = Cudd_bddIte( dd, dd->vars[pNode->pDecs[i]->S->index],bCof1,bCof0 );     Cudd_Ref( bNewFunc );
        Cudd_RecursiveDeref( dd, bCof0 );
        Cudd_RecursiveDeref( dd, bCof1 );
    }

    if ( fRemap )
    {
        // remap the function to the top of the manager
        // remap the function to the first variables of the manager
        for ( i = 0; i < pNode->nDecs; i++ )
    //      Permute[ pNode->pDecs[i]->S->index ] = dd->invperm[i];
            Permute[ pNode->pDecs[i]->S->index ] = i;

        bNewFunc = Cudd_bddPermute( dd, bTemp = bNewFunc, Permute );   Cudd_Ref( bNewFunc );
        Cudd_RecursiveDeref( dd, bTemp );
    }

    Cudd_Deref( bNewFunc );
    return bNewFunc;
}

#endif

////////////////////////////////////////////////////////////////////////
///                           END OF FILE                            ///
////////////////////////////////////////////////////////////////////////
ABC_NAMESPACE_IMPL_END

