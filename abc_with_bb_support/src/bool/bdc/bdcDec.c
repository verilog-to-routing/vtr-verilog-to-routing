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

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Minimizes the support of the ISF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bdc_SuppMinimize2( Bdc_Man_t * p, Bdc_Isf_t * pIsf )
{
    int v;
    abctime clk = 0; // Suppress "might be used uninitialized"
    if ( p->pPars->fVerbose )
        clk = Abc_Clock();
    // compute support
    pIsf->uSupp = Kit_TruthSupport( pIsf->puOn, p->nVars ) | 
        Kit_TruthSupport( pIsf->puOff, p->nVars );
    // go through the support variables
    for ( v = 0; v < p->nVars; v++ )
    {
        if ( (pIsf->uSupp & (1 << v)) == 0 )
            continue;
        Kit_TruthExistNew( p->puTemp1, pIsf->puOn, p->nVars, v );
        Kit_TruthExistNew( p->puTemp2, pIsf->puOff, p->nVars, v );
        if ( !Kit_TruthIsDisjoint( p->puTemp1, p->puTemp2, p->nVars ) )
            continue;
//        if ( !Kit_TruthVarIsVacuous( pIsf->puOn, pIsf->puOff, p->nVars, v ) )
//            continue;
        // remove the variable
        Kit_TruthCopy( pIsf->puOn, p->puTemp1, p->nVars );
        Kit_TruthCopy( pIsf->puOff, p->puTemp2, p->nVars );
//        Kit_TruthExist( pIsf->puOn, p->nVars, v );
//        Kit_TruthExist( pIsf->puOff, p->nVars, v );
        pIsf->uSupp &= ~(1 << v);
    }
    if ( p->pPars->fVerbose )
        p->timeSupps += Abc_Clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Minimizes the support of the ISF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bdc_SuppMinimize( Bdc_Man_t * p, Bdc_Isf_t * pIsf )
{
    int v;
    abctime clk = 0; // Suppress "might be used uninitialized"
    if ( p->pPars->fVerbose )
        clk = Abc_Clock();
    // go through the support variables
    pIsf->uSupp = 0;
    for ( v = 0; v < p->nVars; v++ )
    {
        if ( !Kit_TruthVarInSupport( pIsf->puOn, p->nVars, v ) && 
             !Kit_TruthVarInSupport( pIsf->puOff, p->nVars, v ) )
              continue;
        if ( Kit_TruthVarIsVacuous( pIsf->puOn, pIsf->puOff, p->nVars, v ) )
        {
            Kit_TruthExist( pIsf->puOn, p->nVars, v );
            Kit_TruthExist( pIsf->puOff, p->nVars, v );
            continue;
        }
        pIsf->uSupp |= (1 << v);
    }
    if ( p->pPars->fVerbose )
        p->timeSupps += Abc_Clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Updates the ISF of the right after the left was decompoosed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bdc_DecomposeUpdateRight( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Isf_t * pIsfL, Bdc_Isf_t * pIsfR, Bdc_Fun_t * pFunc0, Bdc_Type_t Type )
{
    unsigned * puTruth = p->puTemp1;
    // get the truth table of the left branch
    if ( Bdc_IsComplement(pFunc0) )
        Kit_TruthNot( puTruth, Bdc_Regular(pFunc0)->puFunc, p->nVars );
    else
        Kit_TruthCopy( puTruth, pFunc0->puFunc, p->nVars );
    // split into parts
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
        Kit_TruthExistSet( pIsfR->puOn, pIsfR->puOn, p->nVars, pIsfL->uUniq ); 
        Kit_TruthExistSet( pIsfR->puOff, pIsf->puOff, p->nVars, pIsfL->uUniq ); 
//        assert( Kit_TruthIsDisjoint(pIsfR->puOn, pIsfR->puOff, p->nVars) );
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

        Kit_TruthAnd( pIsfR->puOff, pIsf->puOff, puTruth, p->nVars );
        Kit_TruthExistSet( pIsfR->puOff, pIsfR->puOff, p->nVars, pIsfL->uUniq ); 
        Kit_TruthExistSet( pIsfR->puOn, pIsf->puOn, p->nVars, pIsfL->uUniq ); 
//        assert( Kit_TruthIsDisjoint(pIsfR->puOn, pIsfR->puOff, p->nVars) );
        assert( !Kit_TruthIsConst0(pIsfR->puOn, p->nVars) );
        return Kit_TruthIsConst0(pIsfR->puOff, p->nVars);
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
		        pIsfL->uUniq = (1 << pVars[Beg]);
		        pIsfR->uUniq = (1 << pVars[End]);
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
	int v, VarCost;
  int VarBest = -1; // Suppress "might be used uninitialized"
  int Cost, VarCostBest = 0;

    for ( v = 0; v < p->nVars; v++ )
	{
        if ( (pIsf->uSupp & (1 << v)) == 0 )
            continue;
//		if ( (Q & !bdd_exist( R, VarSetXa )) != bddfalse )
//		Exist = Cudd_bddExistAbstract( dd, pF->R, Var );   Cudd_Ref( Exist );
//		if ( Cudd_bddIteConstant( dd, pF->Q, Cudd_Not(Exist), b0 ) != b0 )

        Kit_TruthExistNew( p->puTemp1, pIsf->puOff, p->nVars, v );
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
        pIsfL->uUniq = (1 << VarBest);
        pIsfR->uUniq = 0;

//		assert( pL->Q != b0 );
//		assert( pL->R != b0 );
//		assert( Cudd_bddIteConstant( dd, pL->Q, pL->R, b0 ) == b0 );
//        assert( Kit_TruthIsDisjoint(pIsfL->puOn, pIsfL->puOff, p->nVars) );

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
    unsigned uSupportRem;
	int v, nLeftVars = 1, nRightVars = 1; 
    // clean the var sets
    Bdc_IsfStart( p, pIsfL );
    Bdc_IsfStart( p, pIsfR );
    // check that the support is correct
    assert( Kit_TruthSupport(pIsf->puOn, p->nVars) == Kit_TruthSupport(pIsf->puOff, p->nVars) );
    assert( pIsf->uSupp == Kit_TruthSupport(pIsf->puOn, p->nVars) );
    // find initial variable sets
    if ( !Bdc_DecomposeFindInitialVarSet( p, pIsf, pIsfL, pIsfR ) )
        return Bdc_DecomposeWeakOr( p, pIsf, pIsfL, pIsfR );
    // prequantify the variables in the offset
    Kit_TruthExistSet( p->puTemp1, pIsf->puOff, p->nVars, pIsfL->uUniq ); 
    Kit_TruthExistSet( p->puTemp2, pIsf->puOff, p->nVars, pIsfR->uUniq );
    // go through the remaining variables
    uSupportRem = pIsf->uSupp & ~pIsfL->uUniq & ~pIsfR->uUniq;
    for ( v = 0; v < p->nVars; v++ )
    {
        if ( (uSupportRem & (1 << v)) == 0 )
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
                pIsfL->uUniq |= (1 << v);
				nLeftVars++;
                Kit_TruthCopy( p->puTemp1, p->puTemp3, p->nVars );
			}
//			else if ( (Q & bdd_exist( pF->R, pR->V & VarNew ) & bdd_exist( pF->R, pL->V )) == bddfalse )
			else if ( Kit_TruthIsDisjoint3(pIsf->puOn, p->puTemp4, p->puTemp1, p->nVars) )
			{
//				pR->V &= VarNew;
                pIsfR->uUniq |= (1 << v);
				nRightVars++;
                Kit_TruthCopy( p->puTemp2, p->puTemp4, p->nVars );
			}
		}
		else
		{
//			if ( (Q & bdd_exist( pF->R, pR->V & VarNew ) & bdd_exist( pF->R, pL->V )) == bddfalse )
			if ( Kit_TruthIsDisjoint3(pIsf->puOn, p->puTemp4, p->puTemp1, p->nVars) )
			{
//				pR->V &= VarNew;
                pIsfR->uUniq |= (1 << v);
				nRightVars++;
                Kit_TruthCopy( p->puTemp2, p->puTemp4, p->nVars );
			}
//			else if ( (Q & bdd_exist( pF->R, pL->V & VarNew ) & bdd_exist( pF->R, pR->V )) == bddfalse )
			else if ( Kit_TruthIsDisjoint3(pIsf->puOn, p->puTemp3, p->puTemp2, p->nVars) )
			{
//				pL->V &= VarNew;
                pIsfL->uUniq |= (1 << v);
				nLeftVars++;
                Kit_TruthCopy( p->puTemp1, p->puTemp3, p->nVars );
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
    Kit_TruthExistSet( pIsfL->puOn, pIsfL->puOn, p->nVars, pIsfR->uUniq );
    Kit_TruthCopy( pIsfL->puOff, p->puTemp2, p->nVars );

//	assert( pL->Q != b0 );
//	assert( pL->R != b0 );
//	assert( Cudd_bddIteConstant( dd, pL->Q, pL->R, b0 ) == b0 );
    assert( !Kit_TruthIsConst0(pIsfL->puOn, p->nVars) );
    assert( !Kit_TruthIsConst0(pIsfL->puOff, p->nVars) );
//    assert( Kit_TruthIsDisjoint(pIsfL->puOn, pIsfL->puOff, p->nVars) );

	// derive the functions Q and R for the right branch
//	Temp = Cudd_bddExistAbstract( dd, pF->R, pR->V );      Cudd_Ref( Temp );
//	pR->Q = Cudd_bddAndAbstract( dd, pF->Q, Temp, pL->V ); Cudd_Ref( pR->Q );
//	Cudd_RecursiveDeref( dd, Temp );
//	pR->R = Cudd_bddExistAbstract( dd, pF->R, pL->V );     Cudd_Ref( pR->R );

    Kit_TruthAnd( pIsfR->puOn, pIsf->puOn, p->puTemp2, p->nVars );
    Kit_TruthExistSet( pIsfR->puOn, pIsfR->puOn, p->nVars, pIsfL->uUniq );
    Kit_TruthCopy( pIsfR->puOff, p->puTemp1, p->nVars );

    assert( !Kit_TruthIsConst0(pIsfR->puOn, p->nVars) );
    assert( !Kit_TruthIsConst0(pIsfR->puOff, p->nVars) );
//    assert( Kit_TruthIsDisjoint(pIsfR->puOn, pIsfR->puOff, p->nVars) );

    assert( pIsfL->uUniq );
    assert( pIsfR->uUniq );
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
    int WeightOr, WeightAnd, WeightOrL, WeightOrR, WeightAndL, WeightAndR;

    Bdc_IsfClean( p->pIsfOL );
    Bdc_IsfClean( p->pIsfOR );
    Bdc_IsfClean( p->pIsfAL );
    Bdc_IsfClean( p->pIsfAR );

    // perform OR decomposition
    WeightOr = Bdc_DecomposeOr( p, pIsf, p->pIsfOL, p->pIsfOR );

    // perform AND decomposition
    Bdc_IsfNot( pIsf );
    WeightAnd = Bdc_DecomposeOr( p, pIsf, p->pIsfAL, p->pIsfAR );
    Bdc_IsfNot( pIsf );
    Bdc_IsfNot( p->pIsfAL );
    Bdc_IsfNot( p->pIsfAR );

    // check the case when decomposition does not exist
    if ( WeightOr == 0 && WeightAnd == 0 )
    {
        Bdc_IsfCopy( pIsfL, p->pIsfOL );
        Bdc_IsfCopy( pIsfR, p->pIsfOR );
        return BDC_TYPE_MUX;
    }
    // check the hash table
    assert( WeightOr || WeightAnd );
    WeightOrL = WeightOrR = 0;
    if ( WeightOr )
    {
        if ( p->pIsfOL->uUniq )
        {
            Bdc_SuppMinimize( p, p->pIsfOL );
            WeightOrL = (Bdc_TableLookup(p, p->pIsfOL) != NULL);
        }
        if ( p->pIsfOR->uUniq )
        {
            Bdc_SuppMinimize( p, p->pIsfOR );
            WeightOrR = (Bdc_TableLookup(p, p->pIsfOR) != NULL);
        }
    }
    WeightAndL = WeightAndR = 0;
    if ( WeightAnd )
    {
        if ( p->pIsfAL->uUniq )
        {
            Bdc_SuppMinimize( p, p->pIsfAL );
            WeightAndL = (Bdc_TableLookup(p, p->pIsfAL) != NULL);
        }
        if ( p->pIsfAR->uUniq )
        {
            Bdc_SuppMinimize( p, p->pIsfAR );
            WeightAndR = (Bdc_TableLookup(p, p->pIsfAR) != NULL);
        }
    }

    // check if there is any reuse for the components
    if ( WeightOrL + WeightOrR > WeightAndL + WeightAndR )
    {
        p->numReuse++;
        p->numOrs++;
        Bdc_IsfCopy( pIsfL, p->pIsfOL );
        Bdc_IsfCopy( pIsfR, p->pIsfOR );
        return BDC_TYPE_OR;
    }
    if ( WeightOrL + WeightOrR < WeightAndL + WeightAndR )
    {
        p->numReuse++;
        p->numAnds++;
        Bdc_IsfCopy( pIsfL, p->pIsfAL );
        Bdc_IsfCopy( pIsfR, p->pIsfAR );
        return BDC_TYPE_AND; 
    }

    // compare the two-component costs
    if ( WeightOr > WeightAnd )
    {
        if ( WeightOr < BDC_SCALE )
            p->numWeaks++;
        p->numOrs++;
        Bdc_IsfCopy( pIsfL, p->pIsfOL );
        Bdc_IsfCopy( pIsfR, p->pIsfOR );
        return BDC_TYPE_OR;
    }
    if ( WeightAnd < BDC_SCALE )
        p->numWeaks++;
    p->numAnds++;
    Bdc_IsfCopy( pIsfL, p->pIsfAL );
    Bdc_IsfCopy( pIsfR, p->pIsfAR );
    return BDC_TYPE_AND;
}

/**Function*************************************************************

  Synopsis    [Find variable that leads to minimum sum of support sizes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bdc_DecomposeStepMux( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Isf_t * pIsfL, Bdc_Isf_t * pIsfR )
{
    int Var, VarMin, nSuppMin, nSuppCur;
    unsigned uSupp0, uSupp1;
    abctime clk = 0; // Suppress "might be used uninitialized"
    if ( p->pPars->fVerbose )
        clk = Abc_Clock();
    VarMin = -1;
    nSuppMin = 1000;
	for ( Var = 0; Var < p->nVars; Var++ )
    {
        if ( (pIsf->uSupp & (1 << Var)) == 0 )
            continue;
        Kit_TruthCofactor0New( pIsfL->puOn,  pIsf->puOn,  p->nVars, Var );
        Kit_TruthCofactor0New( pIsfL->puOff, pIsf->puOff, p->nVars, Var );
        Kit_TruthCofactor1New( pIsfR->puOn,  pIsf->puOn,  p->nVars, Var );
        Kit_TruthCofactor1New( pIsfR->puOff, pIsf->puOff, p->nVars, Var );
        uSupp0 = Kit_TruthSupport( pIsfL->puOn, p->nVars ) & Kit_TruthSupport( pIsfL->puOff, p->nVars );
        uSupp1 = Kit_TruthSupport( pIsfR->puOn, p->nVars ) & Kit_TruthSupport( pIsfR->puOff, p->nVars );
        nSuppCur = Kit_WordCountOnes(uSupp0) + Kit_WordCountOnes(uSupp1);  
        if ( nSuppMin > nSuppCur )
        {
            nSuppMin = nSuppCur;
            VarMin = Var;
            break;
        }
    }
    if ( VarMin >= 0 )
    {
        Kit_TruthCofactor0New( pIsfL->puOn,  pIsf->puOn,  p->nVars, VarMin );
        Kit_TruthCofactor0New( pIsfL->puOff, pIsf->puOff, p->nVars, VarMin );
        Kit_TruthCofactor1New( pIsfR->puOn,  pIsf->puOn,  p->nVars, VarMin );
        Kit_TruthCofactor1New( pIsfR->puOff, pIsf->puOff, p->nVars, VarMin );
        Bdc_SuppMinimize( p, pIsfL );
        Bdc_SuppMinimize( p, pIsfR );
    }
    if ( p->pPars->fVerbose )
        p->timeMuxes += Abc_Clock() - clk;
    return VarMin;
}

/**Function*************************************************************

  Synopsis    [Creates gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bdc_ManNodeVerify( Bdc_Man_t * p, Bdc_Isf_t * pIsf, Bdc_Fun_t * pFunc )
{
    unsigned * puTruth = p->puTemp1;
    if ( Bdc_IsComplement(pFunc) )
        Kit_TruthNot( puTruth, Bdc_Regular(pFunc)->puFunc, p->nVars );
    else
        Kit_TruthCopy( puTruth, pFunc->puFunc, p->nVars );
    return Bdc_TableCheckContainment( p, pIsf, puTruth );
}

/**Function*************************************************************

  Synopsis    [Creates gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bdc_Fun_t * Bdc_ManCreateGate( Bdc_Man_t * p, Bdc_Fun_t * pFunc0, Bdc_Fun_t * pFunc1, Bdc_Type_t Type )
{
    Bdc_Fun_t * pFunc;
    pFunc = Bdc_FunNew( p );
    if ( pFunc == NULL )
        return NULL;
    pFunc->Type = Type;
    pFunc->pFan0 = pFunc0;
    pFunc->pFan1 = pFunc1;
    pFunc->puFunc = (unsigned *)Vec_IntFetch(p->vMemory, p->nWords); 
    // get the truth table of the left branch
    if ( Bdc_IsComplement(pFunc0) )
        Kit_TruthNot( p->puTemp1, Bdc_Regular(pFunc0)->puFunc, p->nVars );
    else
        Kit_TruthCopy( p->puTemp1, pFunc0->puFunc, p->nVars );
    // get the truth table of the right branch
    if ( Bdc_IsComplement(pFunc1) )
        Kit_TruthNot( p->puTemp2, Bdc_Regular(pFunc1)->puFunc, p->nVars );
    else
        Kit_TruthCopy( p->puTemp2, pFunc1->puFunc, p->nVars );
    // compute the function of node
    if ( pFunc->Type == BDC_TYPE_AND )
    {
        Kit_TruthAnd( pFunc->puFunc, p->puTemp1, p->puTemp2, p->nVars );
    }
    else if ( pFunc->Type == BDC_TYPE_OR )
    {
        Kit_TruthOr( pFunc->puFunc, p->puTemp1, p->puTemp2, p->nVars );
        // transform to AND gate
        pFunc->Type = BDC_TYPE_AND;
        pFunc->pFan0 = Bdc_Not(pFunc->pFan0);
        pFunc->pFan1 = Bdc_Not(pFunc->pFan1);
        Kit_TruthNot( pFunc->puFunc, pFunc->puFunc, p->nVars );
        pFunc = Bdc_Not(pFunc);
    }
    else 
        assert( 0 );
    // add to table
    Bdc_Regular(pFunc)->uSupp = Kit_TruthSupport( Bdc_Regular(pFunc)->puFunc, p->nVars );
    Bdc_TableAdd( p, Bdc_Regular(pFunc) );
    return pFunc;
}

/**Function*************************************************************

  Synopsis    [Performs one step of bi-decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bdc_Fun_t * Bdc_ManDecompose_rec( Bdc_Man_t * p, Bdc_Isf_t * pIsf )
{
//    int static Counter = 0;
//    int LocalCounter = Counter++;
    Bdc_Type_t Type;
    Bdc_Fun_t * pFunc, * pFunc0, * pFunc1;
    Bdc_Isf_t IsfL, * pIsfL = &IsfL;
    Bdc_Isf_t IsfB, * pIsfR = &IsfB;
    int iVar;
    abctime clk = 0; // Suppress "might be used uninitialized"
/*
printf( "Init function (%d):\n", LocalCounter );
Extra_PrintBinary( stdout, pIsf->puOn, 1<<4 );printf("\n");
Extra_PrintBinary( stdout, pIsf->puOff, 1<<4 );printf("\n");
*/
    // check computed results
    assert( Kit_TruthIsDisjoint(pIsf->puOn, pIsf->puOff, p->nVars) );
    if ( p->pPars->fVerbose )
        clk = Abc_Clock();
    pFunc = Bdc_TableLookup( p, pIsf );
    if ( p->pPars->fVerbose )
        p->timeCache += Abc_Clock() - clk;
    if ( pFunc )
        return pFunc;
    // decide on the decomposition type
    if ( p->pPars->fVerbose )
        clk = Abc_Clock();
    Type = Bdc_DecomposeStep( p, pIsf, pIsfL, pIsfR );
    if ( p->pPars->fVerbose )
        p->timeCheck += Abc_Clock() - clk;
    if ( Type == BDC_TYPE_MUX )
    {
        if ( p->pPars->fVerbose )
            clk = Abc_Clock();
        iVar = Bdc_DecomposeStepMux( p, pIsf, pIsfL, pIsfR );
        if ( p->pPars->fVerbose )
            p->timeMuxes += Abc_Clock() - clk;
        p->numMuxes++;
        pFunc0 = Bdc_ManDecompose_rec( p, pIsfL );
        pFunc1 = Bdc_ManDecompose_rec( p, pIsfR );
        if ( pFunc0 == NULL || pFunc1 == NULL )
            return NULL;
        pFunc  = Bdc_FunWithId( p, iVar + 1 );
        pFunc0 = Bdc_ManCreateGate( p, Bdc_Not(pFunc), pFunc0, BDC_TYPE_AND );
        pFunc1 = Bdc_ManCreateGate( p, pFunc,  pFunc1, BDC_TYPE_AND );
        if ( pFunc0 == NULL || pFunc1 == NULL )
            return NULL;
        pFunc = Bdc_ManCreateGate( p, pFunc0, pFunc1, BDC_TYPE_OR );
    }
    else
    {
        pFunc0 = Bdc_ManDecompose_rec( p, pIsfL );
        if ( pFunc0 == NULL )
            return NULL;
        // decompose the right branch
        if ( Bdc_DecomposeUpdateRight( p, pIsf, pIsfL, pIsfR, pFunc0, Type ) )
        {
            p->nNodesNew--;
            return pFunc0;
        }
        Bdc_SuppMinimize( p, pIsfR );
        pFunc1 = Bdc_ManDecompose_rec( p, pIsfR );
        if ( pFunc1 == NULL )
            return NULL;
        // create new gate
        pFunc = Bdc_ManCreateGate( p, pFunc0, pFunc1, Type );
    }
    return pFunc;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

