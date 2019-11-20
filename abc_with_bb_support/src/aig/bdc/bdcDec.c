/**CFile****************************************************************

  FileName    [bdcDec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth-table-based bi-decomposition engine.]

  Synopsis    [Decomposition procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 30, 2007.]

  Revision    [$Id: bdcDec.c,v 1.00 2007/01/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bdcInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Bdc_Type_t Bdc_DecomposeStep( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Isf_t * pIsfL, Bdc_Isf_t * pIsfR );
static int        Bdc_DecomposeUpdateRight( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Isf_t * pIsfL, Bdc_Isf_t * pIsfR, unsigned * puTruth, Bdc_Type_t Type );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs one step of bi-decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bdc_Fun_t * Bdc_ManDecompose_rec( Bdc_Man_t * p, Bdc_Isf_t * pIsf )
{
    Bdc_Fun_t * pFunc;
    Bdc_Isf_t IsfL, * pIsfL = &IsfL;
    Bdc_Isf_t IsfB, * pIsfR = &IsfB;
    // check computed results
    if ( pFunc = Bdc_TableLookup( p, pIsf ) )
        return pFunc;
    // decide on the decomposition type
    pFunc = Bdc_FunNew( p );
    if ( pFunc == NULL )
        return NULL;
    pFunc->Type = Bdc_DecomposeStep( p, pIsf, pIsfL, pIsfR );
    // decompose the left branch
    pFunc->pFan0 = Bdc_ManDecompose_rec( p, pIsfL );
    if ( pFunc->pFan0 == NULL )
        return NULL;
    // decompose the right branch
    if ( Bdc_DecomposeUpdateRight( p, pIsf, pIsfL, pIsfR, pFunc->pFan0->puFunc, pFunc->Type ) )
    {
        p->nNodes--;
        return pFunc->pFan0;
    }
    pFunc->pFan1 = Bdc_ManDecompose_rec( p, pIsfL );
    if ( pFunc->pFan1 == NULL )
        return NULL;
    // compute the function of node
    pFunc->puFunc = (unsigned *)Vec_IntFetch(p->vMemory, p->nWords); 
    if ( pFunc->Type == BDC_TYPE_AND )
        Kit_TruthAnd( pFunc->puFunc, pFunc->pFan0->puFunc, pFunc->pFan1->puFunc, p->nVars );
    else if ( pFunc->Type == BDC_TYPE_OR )
        Kit_TruthOr( pFunc->puFunc, pFunc->pFan0->puFunc, pFunc->pFan1->puFunc, p->nVars );
    else 
        assert( 0 );
    // verify correctness
    assert( Bdc_TableCheckContainment(p, pIsf, pFunc->puFunc) );
    // convert from OR to AND
    if ( pFunc->Type == BDC_TYPE_OR )
    {
        pFunc->Type = BDC_TYPE_AND;
        pFunc->pFan0 = Bdc_Not(pFunc->pFan0);
        pFunc->pFan1 = Bdc_Not(pFunc->pFan1);
        Kit_TruthNot( pFunc->puFunc, pFunc->puFunc, p->nVars );
        pFunc = Bdc_Not(pFunc);
    }
    Bdc_TableAdd( p, Bdc_Regular(pFunc) );
    return pFunc;
}

/**Function*************************************************************

  Synopsis    [Updates the ISF of the right after the left was decompoosed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bdc_DecomposeUpdateRight( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Isf_t * pIsfL, Bdc_Isf_t * pIsfR, unsigned * puTruth, Bdc_Type_t Type )
{
	if ( Type == BDC_TYPE_OR ) 
	{
//		Right.Q = bdd_appex( Q, CompSpecLeftF, bddop_diff, setRightRes );
//		Right.R = bdd_exist( R, setRightRes );

//		if ( pR->Q )  Cudd_RecursiveDeref( dd, pR->Q );
//		if ( pR->R )  Cudd_RecursiveDeref( dd, pR->R );
//		pR->Q = Cudd_bddAndAbstract( dd, pF->Q, Cudd_Not(CompSpecF), pL->V );	Cudd_Ref( pR->Q );
//		pR->R = Cudd_bddExistAbstract( dd, pF->R, pL->V );                      Cudd_Ref( pR->R );

//		assert( pR->R != b0 );
//		return (int)( pR->Q == b0 );

        Kit_TruthSharp( pIsfR->puOn, pIsf->puOn, puTruth, p->nVars );
        Kit_TruthExistSet( pIsfR->puOn, pIsfR->puOn, p->nVars, pIsfL->uSupp ); 
        Kit_TruthExistSet( pIsfR->puOff, pIsf->puOff, p->nVars, pIsfL->uSupp ); 
        assert( !Kit_TruthIsConst0(pIsfR->puOff, p->nVars) );
        return Kit_TruthIsConst0(pIsfR->puOn, p->nVars);
	}
	else if ( Type == BDC_TYPE_AND )
	{
//		Right.R = bdd_appex( R, CompSpecLeftF, bddop_and, setRightRes );
//		Right.Q = bdd_exist( Q, setRightRes );

//		if ( pR->Q )  Cudd_RecursiveDeref( dd, pR->Q );
//		if ( pR->R )  Cudd_RecursiveDeref( dd, pR->R );
//		pR->R = Cudd_bddAndAbstract( dd, pF->R, CompSpecF, pL->V );	            Cudd_Ref( pR->R );
//		pR->Q = Cudd_bddExistAbstract( dd, pF->Q, pL->V );                      Cudd_Ref( pR->Q );

//		assert( pR->Q != b0 );
//		return (int)( pR->R == b0 );

        Kit_TruthSharp( pIsfR->puOn, pIsf->puOn, puTruth, p->nVars );
        Kit_TruthExistSet( pIsfR->puOn, pIsfR->puOn, p->nVars, pIsfL->uSupp ); 
        Kit_TruthExistSet( pIsfR->puOff, pIsf->puOff, p->nVars, pIsfL->uSupp ); 
        assert( !Kit_TruthIsConst0(pIsfR->puOff, p->nVars) );
        return Kit_TruthIsConst0(pIsfR->puOn, p->nVars);
	}
	return 0;
}

/**Function*************************************************************

  Synopsis    [Checks existence of OR-bidecomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Bdc_DecomposeGetCost( Bdc_Man_t * p, int nLeftVars, int nRightVars )
{
	assert( nLeftVars > 0 );
	assert( nRightVars > 0 );
	// compute the decomposition coefficient
	if ( nLeftVars >= nRightVars )
		return BDC_SCALE * (p->nVars * nRightVars + nLeftVars);
	else // if ( nLeftVars < nRightVars )
		return BDC_SCALE * (p->nVars * nLeftVars + nRightVars);
}

/**Function*************************************************************

  Synopsis    [Checks existence of weak OR-bidecomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bdc_DecomposeFindInitialVarSet( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Isf_t * pIsfL, Bdc_Isf_t * pIsfR )
{
    char pVars[16];
	int v, nVars, Beg, End;

	assert( pIsfL->uSupp == 0 );
	assert( pIsfR->uSupp == 0 );

	// fill in the variables
    nVars = 0;
	for ( v = 0; v < p->nVars; v++ )
        if ( pIsf->uSupp & (1 << v) )
            pVars[nVars++] = v;

    // try variable pairs
    for ( Beg = 0; Beg < nVars; Beg++ )
    {
        Kit_TruthExistNew( p->puTemp1, pIsf->puOff, p->nVars, pVars[Beg] ); 
        for ( End = nVars - 1; End > Beg; End-- )
        {
            Kit_TruthExistNew( p->puTemp2, pIsf->puOff, p->nVars, pVars[End] ); 
            if ( Kit_TruthIsDisjoint3(pIsf->puOn, p->puTemp1, p->puTemp2, p->nVars) )
            {
		        pIsfL->uSupp = (1 << Beg);
		        pIsfR->uSupp = (1 << End);
		        pIsfL->Var = Beg;
		        pIsfR->Var = End;
                return 1;
            }
        }
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Checks existence of weak OR-bidecomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bdc_DecomposeWeakOr( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Isf_t * pIsfL, Bdc_Isf_t * pIsfR )
{
	int v, VarCost, VarBest, Cost, VarCostBest = 0;

    for ( v = 0; v < p->nVars; v++ )
	{
        Kit_TruthExistNew( p->puTemp1, pIsf->puOff, p->nVars, v );
//		if ( (Q & !bdd_exist( R, VarSetXa )) != bddfalse )
//		Exist = Cudd_bddExistAbstract( dd, pF->R, Var );   Cudd_Ref( Exist );
//		if ( Cudd_bddIteConstant( dd, pF->Q, Cudd_Not(Exist), b0 ) != b0 )
        if ( !Kit_TruthIsImply( pIsf->puOn, p->puTemp1, p->nVars ) )
		{
			// measure the cost of this variable
//			VarCost = bdd_satcountset( bdd_forall( Q, VarSetXa ), VarCube );

//			Univ = Cudd_bddUnivAbstract( dd, pF->Q, Var );   Cudd_Ref( Univ );
//			VarCost = Kit_TruthCountOnes( Univ, p->nVars );
//			Cudd_RecursiveDeref( dd, Univ );

            Kit_TruthForallNew( p->puTemp2, pIsf->puOn, p->nVars, v );
            VarCost = Kit_TruthCountOnes( p->puTemp2, p->nVars );
			if ( VarCost == 0 )
				VarCost = 1;
			if ( VarCostBest < VarCost )
			{
				VarCostBest = VarCost;
				VarBest = v;
			}
		}
	}

	// derive the components for weak-bi-decomposition if the variable is found
	if ( VarCostBest )
	{
//		funQLeftRes = Q & bdd_exist( R, setRightORweak );

//		Temp = Cudd_bddExistAbstract( dd, pF->R, VarBest );     Cudd_Ref( Temp );
//		pL->Q = Cudd_bddAnd( dd, pF->Q, Temp );			        Cudd_Ref( pL->Q );
//		Cudd_RecursiveDeref( dd, Temp );

        Kit_TruthExistNew( p->puTemp1, pIsf->puOff, p->nVars, VarBest );
        Kit_TruthAnd( pIsfL->puOn, pIsf->puOn, p->puTemp1, p->nVars );

//		pL->R = pF->R;		                                    Cudd_Ref( pL->R );
//		pL->V = VarBest;                                        Cudd_Ref( pL->V );
        Kit_TruthCopy( pIsfL->puOff, pIsf->puOff, p->nVars );
        pIsfL->Var = VarBest;

//		assert( pL->Q != b0 );
//		assert( pL->R != b0 );
//		assert( Cudd_bddIteConstant( dd, pL->Q, pL->R, b0 ) == b0 );

		// express cost in percents of the covered boolean space
		Cost = VarCostBest * BDC_SCALE / (1<<p->nVars);
		if ( Cost == 0 )
			Cost = 1;
        return Cost;
	}
    return 0;
}

/**Function*************************************************************

  Synopsis    [Checks existence of OR-bidecomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bdc_DecomposeOr( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Isf_t * pIsfL, Bdc_Isf_t * pIsfR )
{
    unsigned uSuppRem;
	int v, nLeftVars = 1, nRightVars = 1; 
    // clean the var sets
    Bdc_IsfClean( pIsfL );
    Bdc_IsfClean( pIsfR );
    // find initial variable sets
    if ( !Bdc_DecomposeFindInitialVarSet( p, pIsf, pIsfL, pIsfR ) )
        return Bdc_DecomposeWeakOr( p, pIsf, pIsfL, pIsfR );
    // prequantify the variables in the offset
    Kit_TruthExistNew( p->puTemp1, pIsf->puOff, p->nVars, pIsfL->Var ); 
    Kit_TruthExistNew( p->puTemp2, pIsf->puOff, p->nVars, pIsfR->Var );
    // go through the remaining variables
    uSuppRem = pIsf->uSupp & ~pIsfL->uSupp & ~pIsfR->uSupp;
    assert( Kit_WordCountOnes(uSuppRem) > 0 );
    for ( v = 0; v < p->nVars; v++ )
    {
        if ( (uSuppRem & (1 << v)) == 0 )
            continue;
        // prequantify this variable
        Kit_TruthExistNew( p->puTemp3, p->puTemp1, p->nVars, v );
        Kit_TruthExistNew( p->puTemp4, p->puTemp2, p->nVars, v );
		if ( nLeftVars < nRightVars )
		{
//			if ( (Q & bdd_exist( pF->R, pL->V & VarNew ) & bdd_exist( pF->R, pR->V )) == bddfalse )
//			if ( VerifyORCondition( dd, pF->Q, pF->R, pL->V, pR->V, VarNew ) )
			if ( Kit_TruthIsDisjoint3(pIsf->puOn, p->puTemp3, p->puTemp2, p->nVars) )
			{
//				pL->V &= VarNew;
                pIsfL->uSupp |= (1 << v);
				nLeftVars++;
			}
//			else if ( (Q & bdd_exist( pF->R, pR->V & VarNew ) & bdd_exist( pF->R, pL->V )) == bddfalse )
			else if ( Kit_TruthIsDisjoint3(pIsf->puOn, p->puTemp4, p->puTemp1, p->nVars) )
			{
//				pR->V &= VarNew;
                pIsfR->uSupp |= (1 << v);
				nRightVars++;
			}
		}
		else
		{
//			if ( (Q & bdd_exist( pF->R, pR->V & VarNew ) & bdd_exist( pF->R, pL->V )) == bddfalse )
			if ( Kit_TruthIsDisjoint3(pIsf->puOn, p->puTemp4, p->puTemp1, p->nVars) )
			{
//				pR->V &= VarNew;
                pIsfR->uSupp |= (1 << v);
				nRightVars++;
			}
//			else if ( (Q & bdd_exist( pF->R, pL->V & VarNew ) & bdd_exist( pF->R, pR->V )) == bddfalse )
			else if ( Kit_TruthIsDisjoint3(pIsf->puOn, p->puTemp3, p->puTemp2, p->nVars) )
			{
//				pL->V &= VarNew;
                pIsfL->uSupp |= (1 << v);
				nLeftVars++;
			}
		}
    }

	// derive the functions Q and R for the left branch
//	pL->Q = bdd_appex( pF->Q, bdd_exist( pF->R, pL->V ), bddop_and, pR->V );
//	pL->R = bdd_exist( pF->R, pR->V );

//	Temp = Cudd_bddExistAbstract( dd, pF->R, pL->V );      Cudd_Ref( Temp );
//	pL->Q = Cudd_bddAndAbstract( dd, pF->Q, Temp, pR->V ); Cudd_Ref( pL->Q );
//	Cudd_RecursiveDeref( dd, Temp );
//	pL->R = Cudd_bddExistAbstract( dd, pF->R, pR->V );     Cudd_Ref( pL->R );

    Kit_TruthAnd( pIsfL->puOn, pIsf->puOn, p->puTemp1, p->nVars );
    Kit_TruthExistSet( pIsfL->puOn, pIsfL->puOn, p->nVars, pIsfR->uSupp );
    Kit_TruthCopy( pIsfL->puOff, p->puTemp2, p->nVars );

	// derive the functions Q and R for the right branch
//	Temp = Cudd_bddExistAbstract( dd, pF->R, pR->V );      Cudd_Ref( Temp );
//	pR->Q = Cudd_bddAndAbstract( dd, pF->Q, Temp, pL->V ); Cudd_Ref( pR->Q );
//	Cudd_RecursiveDeref( dd, Temp );
//	pR->R = Cudd_bddExistAbstract( dd, pF->R, pL->V );     Cudd_Ref( pR->R );

/*
    Kit_TruthAnd( pIsfR->puOn, pIsf->puOn, p->puTemp2, p->nVars );
    Kit_TruthExistSet( pIsfR->puOn, pIsfR->puOn, p->nVars, pIsfL->uSupp );
    Kit_TruthCopy( pIsfR->puOff, p->puTemp1, p->nVars );
*/

//	assert( pL->Q != b0 );
//	assert( pL->R != b0 );
//	assert( Cudd_bddIteConstant( dd, pL->Q, pL->R, b0 ) == b0 );
    assert( !Kit_TruthIsConst0(pIsfL->puOn, p->nVars) );
    assert( !Kit_TruthIsConst0(pIsfL->puOff, p->nVars) );
    assert( Kit_TruthIsDisjoint(pIsfL->puOn, pIsfL->puOff, p->nVars) );

	return Bdc_DecomposeGetCost( p, nLeftVars, nRightVars );		
}

/**Function*************************************************************

  Synopsis    [Performs one step of bi-decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bdc_Type_t Bdc_DecomposeStep( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Isf_t * pIsfL, Bdc_Isf_t * pIsfR )
{
    int CostOr, CostAnd, CostOrL, CostOrR, CostAndL, CostAndR;

    Bdc_IsfClean( p->pIsfOL );
    Bdc_IsfClean( p->pIsfOR );
    Bdc_IsfClean( p->pIsfAL );
    Bdc_IsfClean( p->pIsfAR );

    // perform OR decomposition
    CostOr = Bdc_DecomposeOr( p, pIsf, p->pIsfOL, p->pIsfOR );

    // perform AND decomposition
    Bdc_IsfNot( pIsf );
    CostAnd = Bdc_DecomposeOr( p, pIsf, p->pIsfAL, p->pIsfAR );
    Bdc_IsfNot( pIsf );
    Bdc_IsfNot( p->pIsfAL );
    Bdc_IsfNot( p->pIsfAR );

    // check the hash table
    Bdc_SuppMinimize( p, p->pIsfOL );
    CostOrL = (Bdc_TableLookup(p, p->pIsfOL) != NULL);
    Bdc_SuppMinimize( p, p->pIsfOR );
    CostOrR = (Bdc_TableLookup(p, p->pIsfOR) != NULL);
    Bdc_SuppMinimize( p, p->pIsfAL );
    CostAndL = (Bdc_TableLookup(p, p->pIsfAL) != NULL);
    Bdc_SuppMinimize( p, p->pIsfAR );
    CostAndR = (Bdc_TableLookup(p, p->pIsfAR) != NULL);

    // check if there is any reuse for the components
    if ( CostOrL + CostOrR < CostAndL + CostAndR )
    {
        Bdc_IsfCopy( pIsfL, p->pIsfOL );
        Bdc_IsfCopy( pIsfR, p->pIsfOR );
        return BDC_TYPE_OR;
    }
    if ( CostOrL + CostOrR > CostAndL + CostAndR )
    {
        Bdc_IsfCopy( pIsfL, p->pIsfAL );
        Bdc_IsfCopy( pIsfR, p->pIsfAR );
        return BDC_TYPE_AND; 
    }

    // compare the two-component costs
    if ( CostOr < CostAnd )
    {
        Bdc_IsfCopy( pIsfL, p->pIsfOL );
        Bdc_IsfCopy( pIsfR, p->pIsfOR );
        return BDC_TYPE_OR;
    }
    return BDC_TYPE_AND;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


