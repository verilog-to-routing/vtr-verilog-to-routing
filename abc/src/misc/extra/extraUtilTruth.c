/**CFile****************************************************************

  FileName    [extraUtilMisc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [Various procedures for truth table manipulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilMisc.c,v 1.0 2003/09/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "extra.h"

ABC_NAMESPACE_IMPL_START


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

static unsigned s_VarMasks[5][2] = {
    { 0x33333333, 0xAAAAAAAA },
    { 0x55555555, 0xCCCCCCCC },
    { 0x0F0F0F0F, 0xF0F0F0F0 },
    { 0x00FF00FF, 0xFF00FF00 },
    { 0x0000FFFF, 0xFFFF0000 }
};

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Derive elementary truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned ** Extra_TruthElementary( int nVars )
{
    unsigned ** pRes;
    int i, k, nWords;
    nWords = Extra_TruthWordNum(nVars);
    pRes = (unsigned **)Extra_ArrayAlloc( nVars, nWords, 4 );
    for ( i = 0; i < nVars; i++ )
    {
        if ( i < 5 )
        {
            for ( k = 0; k < nWords; k++ )
                pRes[i][k] = s_VarMasks[i][1];
        }
        else
        {
            for ( k = 0; k < nWords; k++ )
                if ( k & (1 << (i-5)) )
                    pRes[i][k] = ~(unsigned)0;
                else
                    pRes[i][k] = 0;
        }
    }
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Swaps two adjacent variables in the truth table.]

  Description [Swaps var number Start and var number Start+1 (0-based numbers).
  The input truth table is pIn. The output truth table is pOut.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthSwapAdjacentVars( unsigned * pOut, unsigned * pIn, int nVars, int iVar )
{
    static unsigned PMasks[4][3] = {
        { 0x99999999, 0x22222222, 0x44444444 },
        { 0xC3C3C3C3, 0x0C0C0C0C, 0x30303030 },
        { 0xF00FF00F, 0x00F000F0, 0x0F000F00 },
        { 0xFF0000FF, 0x0000FF00, 0x00FF0000 }
    };
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step, Shift;

    assert( iVar < nVars - 1 );
    if ( iVar < 4 )
    {
        Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & PMasks[iVar][0]) | ((pIn[i] & PMasks[iVar][1]) << Shift) | ((pIn[i] & PMasks[iVar][2]) >> Shift);
    }
    else if ( iVar > 4 )
    {
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 4*Step )
        {
            for ( i = 0; i < Step; i++ )
                pOut[i] = pIn[i];
            for ( i = 0; i < Step; i++ )
                pOut[Step+i] = pIn[2*Step+i];
            for ( i = 0; i < Step; i++ )
                pOut[2*Step+i] = pIn[Step+i];
            for ( i = 0; i < Step; i++ )
                pOut[3*Step+i] = pIn[3*Step+i];
            pIn  += 4*Step;
            pOut += 4*Step;
        }
    }
    else // if ( iVar == 4 )
    {
        for ( i = 0; i < nWords; i += 2 )
        {
            pOut[i]   = (pIn[i]   & 0x0000FFFF) | ((pIn[i+1] & 0x0000FFFF) << 16);
            pOut[i+1] = (pIn[i+1] & 0xFFFF0000) | ((pIn[i]   & 0xFFFF0000) >> 16);
        }
    }
}

/**Function*************************************************************

  Synopsis    [Swaps two adjacent variables in the truth table.]

  Description [Swaps var number Start and var number Start+1 (0-based numbers).
  The input truth table is pIn. The output truth table is pOut.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthSwapAdjacentVars2( unsigned * pIn, unsigned * pOut, int nVars, int Start )
{
    int nWords = (nVars <= 5)? 1 : (1 << (nVars-5));
    int i, k, Step;

    assert( Start < nVars - 1 );
    switch ( Start )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0x99999999) | ((pIn[i] & 0x22222222) << 1) | ((pIn[i] & 0x44444444) >> 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0xC3C3C3C3) | ((pIn[i] & 0x0C0C0C0C) << 2) | ((pIn[i] & 0x30303030) >> 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0xF00FF00F) | ((pIn[i] & 0x00F000F0) << 4) | ((pIn[i] & 0x0F000F00) >> 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & 0xFF0000FF) | ((pIn[i] & 0x0000FF00) << 8) | ((pIn[i] & 0x00FF0000) >> 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i += 2 )
        {
            pOut[i]   = (pIn[i]   & 0x0000FFFF) | ((pIn[i+1] & 0x0000FFFF) << 16);
            pOut[i+1] = (pIn[i+1] & 0xFFFF0000) | ((pIn[i]   & 0xFFFF0000) >> 16);
        }
        return;
    default:
        Step = (1 << (Start - 5));
        for ( k = 0; k < nWords; k += 4*Step )
        {
            for ( i = 0; i < Step; i++ )
                pOut[i] = pIn[i];
            for ( i = 0; i < Step; i++ )
                pOut[Step+i] = pIn[2*Step+i];
            for ( i = 0; i < Step; i++ )
                pOut[2*Step+i] = pIn[Step+i];
            for ( i = 0; i < Step; i++ )
                pOut[3*Step+i] = pIn[3*Step+i];
            pIn  += 4*Step;
            pOut += 4*Step;
        }
        return;
    }
}

/**Function*************************************************************

  Synopsis    [Expands the truth table according to the phase.]

  Description [The input and output truth tables are in pIn/pOut. The current number
  of variables is nVars. The total number of variables in nVarsAll. The last argument
  (Phase) contains shows where the variables should go.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthStretch( unsigned * pOut, unsigned * pIn, int nVars, int nVarsAll, unsigned Phase )
{
    unsigned * pTemp;
    int i, k, Var = nVars - 1, Counter = 0;
    for ( i = nVarsAll - 1; i >= 0; i-- )
        if ( Phase & (1 << i) )
        {
            for ( k = Var; k < i; k++ )
            {
                Extra_TruthSwapAdjacentVars( pOut, pIn, nVarsAll, k );
                pTemp = pIn; pIn = pOut; pOut = pTemp;
                Counter++;
            }
            Var--;
        }
    assert( Var == -1 );
    // swap if it was moved an even number of times
    if ( !(Counter & 1) )
        Extra_TruthCopy( pOut, pIn, nVarsAll );
}

/**Function*************************************************************

  Synopsis    [Shrinks the truth table according to the phase.]

  Description [The input and output truth tables are in pIn/pOut. The current number
  of variables is nVars. The total number of variables in nVarsAll. The last argument
  (Phase) contains shows what variables should remain.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthShrink( unsigned * pOut, unsigned * pIn, int nVars, int nVarsAll, unsigned Phase )
{
    unsigned * pTemp;
    int i, k, Var = 0, Counter = 0;
    for ( i = 0; i < nVarsAll; i++ )
        if ( Phase & (1 << i) )
        {
            for ( k = i-1; k >= Var; k-- )
            {
                Extra_TruthSwapAdjacentVars( pOut, pIn, nVarsAll, k );
                pTemp = pIn; pIn = pOut; pOut = pTemp;
                Counter++;
            }
            Var++;
        }
    assert( Var == nVars );
    // swap if it was moved an even number of times
    if ( !(Counter & 1) )
        Extra_TruthCopy( pOut, pIn, nVarsAll );
}


/**Function*************************************************************

  Synopsis    [Returns 1 if TT depends on the given variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_TruthVarInSupport( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            if ( (pTruth[i] & 0x55555555) != ((pTruth[i] & 0xAAAAAAAA) >> 1) )
                return 1;
        return 0;
    case 1:
        for ( i = 0; i < nWords; i++ )
            if ( (pTruth[i] & 0x33333333) != ((pTruth[i] & 0xCCCCCCCC) >> 2) )
                return 1;
        return 0;
    case 2:
        for ( i = 0; i < nWords; i++ )
            if ( (pTruth[i] & 0x0F0F0F0F) != ((pTruth[i] & 0xF0F0F0F0) >> 4) )
                return 1;
        return 0;
    case 3:
        for ( i = 0; i < nWords; i++ )
            if ( (pTruth[i] & 0x00FF00FF) != ((pTruth[i] & 0xFF00FF00) >> 8) )
                return 1;
        return 0;
    case 4:
        for ( i = 0; i < nWords; i++ )
            if ( (pTruth[i] & 0x0000FFFF) != ((pTruth[i] & 0xFFFF0000) >> 16) )
                return 1;
        return 0;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                if ( pTruth[i] != pTruth[Step+i] )
                    return 1;
            pTruth += 2*Step;
        }
        return 0;
    }
}

/**Function*************************************************************

  Synopsis    [Returns the number of support vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_TruthSupportSize( unsigned * pTruth, int nVars )
{
    int i, Counter = 0;
    for ( i = 0; i < nVars; i++ )
        Counter += Extra_TruthVarInSupport( pTruth, nVars, i );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns support of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_TruthSupport( unsigned * pTruth, int nVars )
{
    int i, Support = 0;
    for ( i = 0; i < nVars; i++ )
        if ( Extra_TruthVarInSupport( pTruth, nVars, i ) )
            Support |= (1 << i);
    return Support;
}



/**Function*************************************************************

  Synopsis    [Computes positive cofactor of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthCofactor1( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0xAAAAAAAA) | ((pTruth[i] & 0xAAAAAAAA) >> 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0xCCCCCCCC) | ((pTruth[i] & 0xCCCCCCCC) >> 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0xF0F0F0F0) | ((pTruth[i] & 0xF0F0F0F0) >> 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0xFF00FF00) | ((pTruth[i] & 0xFF00FF00) >> 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0xFFFF0000) | ((pTruth[i] & 0xFFFF0000) >> 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                pTruth[i] = pTruth[Step+i];
            pTruth += 2*Step;
        }
        return;
    }
}

/**Function*************************************************************

  Synopsis    [Computes negative cofactor of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthCofactor0( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0x55555555) | ((pTruth[i] & 0x55555555) << 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0x33333333) | ((pTruth[i] & 0x33333333) << 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0x0F0F0F0F) | ((pTruth[i] & 0x0F0F0F0F) << 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0x00FF00FF) | ((pTruth[i] & 0x00FF00FF) << 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = (pTruth[i] & 0x0000FFFF) | ((pTruth[i] & 0x0000FFFF) << 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                pTruth[Step+i] = pTruth[i];
            pTruth += 2*Step;
        }
        return;
    }
}


/**Function*************************************************************

  Synopsis    [Existentially quantifies the variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthExist( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] |=  ((pTruth[i] & 0xAAAAAAAA) >> 1) | ((pTruth[i] & 0x55555555) << 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] |=  ((pTruth[i] & 0xCCCCCCCC) >> 2) | ((pTruth[i] & 0x33333333) << 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] |=  ((pTruth[i] & 0xF0F0F0F0) >> 4) | ((pTruth[i] & 0x0F0F0F0F) << 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] |=  ((pTruth[i] & 0xFF00FF00) >> 8) | ((pTruth[i] & 0x00FF00FF) << 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] |=  ((pTruth[i] & 0xFFFF0000) >> 16) | ((pTruth[i] & 0x0000FFFF) << 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                pTruth[i]     |= pTruth[Step+i];
                pTruth[Step+i] = pTruth[i];
            }
            pTruth += 2*Step;
        }
        return;
    }
}

/**Function*************************************************************

  Synopsis    [Existentially quantifies the variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthForall( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] &=  ((pTruth[i] & 0xAAAAAAAA) >> 1) | ((pTruth[i] & 0x55555555) << 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] &=  ((pTruth[i] & 0xCCCCCCCC) >> 2) | ((pTruth[i] & 0x33333333) << 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] &=  ((pTruth[i] & 0xF0F0F0F0) >> 4) | ((pTruth[i] & 0x0F0F0F0F) << 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] &=  ((pTruth[i] & 0xFF00FF00) >> 8) | ((pTruth[i] & 0x00FF00FF) << 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] &=  ((pTruth[i] & 0xFFFF0000) >> 16) | ((pTruth[i] & 0x0000FFFF) << 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                pTruth[i]     &= pTruth[Step+i];
                pTruth[Step+i] = pTruth[i];
            }
            pTruth += 2*Step;
        }
        return;
    }
}


/**Function*************************************************************

  Synopsis    [Computes negative cofactor of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthMux( unsigned * pOut, unsigned * pCof0, unsigned * pCof1, int nVars, int iVar )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pCof0[i] & 0x55555555) | (pCof1[i] & 0xAAAAAAAA);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pCof0[i] & 0x33333333) | (pCof1[i] & 0xCCCCCCCC);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pCof0[i] & 0x0F0F0F0F) | (pCof1[i] & 0xF0F0F0F0);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pCof0[i] & 0x00FF00FF) | (pCof1[i] & 0xFF00FF00);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pCof0[i] & 0x0000FFFF) | (pCof1[i] & 0xFFFF0000);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                pOut[i]      = pCof0[i];
                pOut[Step+i] = pCof1[Step+i];
            }
            pOut += 2*Step;
        }
        return;
    }
}

/**Function*************************************************************

  Synopsis    [Checks symmetry of two variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_TruthVarsSymm( unsigned * pTruth, int nVars, int iVar0, int iVar1 )
{
    static unsigned uTemp0[16], uTemp1[16];
    assert( nVars <= 9 );
    // compute Cof01
    Extra_TruthCopy( uTemp0, pTruth, nVars );
    Extra_TruthCofactor0( uTemp0, nVars, iVar0 );
    Extra_TruthCofactor1( uTemp0, nVars, iVar1 );
    // compute Cof10
    Extra_TruthCopy( uTemp1, pTruth, nVars );
    Extra_TruthCofactor1( uTemp1, nVars, iVar0 );
    Extra_TruthCofactor0( uTemp1, nVars, iVar1 );
    // compare
    return Extra_TruthIsEqual( uTemp0, uTemp1, nVars );
}

/**Function*************************************************************

  Synopsis    [Checks antisymmetry of two variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_TruthVarsAntiSymm( unsigned * pTruth, int nVars, int iVar0, int iVar1 )
{
    static unsigned uTemp0[16], uTemp1[16];
    assert( nVars <= 9 );
    // compute Cof00
    Extra_TruthCopy( uTemp0, pTruth, nVars );
    Extra_TruthCofactor0( uTemp0, nVars, iVar0 );
    Extra_TruthCofactor0( uTemp0, nVars, iVar1 );
    // compute Cof11
    Extra_TruthCopy( uTemp1, pTruth, nVars );
    Extra_TruthCofactor1( uTemp1, nVars, iVar0 );
    Extra_TruthCofactor1( uTemp1, nVars, iVar1 );
    // compare
    return Extra_TruthIsEqual( uTemp0, uTemp1, nVars );
}

/**Function*************************************************************

  Synopsis    [Changes phase of the function w.r.t. one variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthChangePhase( unsigned * pTruth, int nVars, int iVar )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Step;
    unsigned Temp;

    assert( iVar < nVars );
    switch ( iVar )
    {
    case 0:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ((pTruth[i] & 0x55555555) << 1) | ((pTruth[i] & 0xAAAAAAAA) >> 1);
        return;
    case 1:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ((pTruth[i] & 0x33333333) << 2) | ((pTruth[i] & 0xCCCCCCCC) >> 2);
        return;
    case 2:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ((pTruth[i] & 0x0F0F0F0F) << 4) | ((pTruth[i] & 0xF0F0F0F0) >> 4);
        return;
    case 3:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ((pTruth[i] & 0x00FF00FF) << 8) | ((pTruth[i] & 0xFF00FF00) >> 8);
        return;
    case 4:
        for ( i = 0; i < nWords; i++ )
            pTruth[i] = ((pTruth[i] & 0x0000FFFF) << 16) | ((pTruth[i] & 0xFFFF0000) >> 16);
        return;
    default:
        Step = (1 << (iVar - 5));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                Temp = pTruth[i];
                pTruth[i] = pTruth[Step+i];
                pTruth[Step+i] = Temp;
            }
            pTruth += 2*Step;
        }
        return;
    }
}

/**Function*************************************************************

  Synopsis    [Computes minimum overlap in supports of cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_TruthMinCofSuppOverlap( unsigned * pTruth, int nVars, int * pVarMin )
{
    static unsigned uCofactor[16];
    int i, ValueCur, ValueMin, VarMin;
    unsigned uSupp0, uSupp1;
    int nVars0, nVars1;
    assert( nVars <= 9 );
    ValueMin = 32;
    VarMin   = -1;
    for ( i = 0; i < nVars; i++ )
    {
        // get negative cofactor
        Extra_TruthCopy( uCofactor, pTruth, nVars );
        Extra_TruthCofactor0( uCofactor, nVars, i );
        uSupp0 = Extra_TruthSupport( uCofactor, nVars );
        nVars0 = Extra_WordCountOnes( uSupp0 );
//Extra_PrintBinary( stdout, &uSupp0, 8 ); printf( "\n" );
        // get positive cofactor
        Extra_TruthCopy( uCofactor, pTruth, nVars );
        Extra_TruthCofactor1( uCofactor, nVars, i );
        uSupp1 = Extra_TruthSupport( uCofactor, nVars );
        nVars1 = Extra_WordCountOnes( uSupp1 );
//Extra_PrintBinary( stdout, &uSupp1, 8 ); printf( "\n" );
        // get the number of common vars
        ValueCur = Extra_WordCountOnes( uSupp0 & uSupp1 );
        if ( ValueMin > ValueCur && nVars0 <= 5 && nVars1 <= 5 )
        {
            ValueMin = ValueCur;
            VarMin = i;
        }
        if ( ValueMin == 0 )
            break;
    }
    if ( pVarMin )
        *pVarMin = VarMin;
    return ValueMin;
}


/**Function*************************************************************

  Synopsis    [Counts the number of 1's in each cofactor.]

  Description [The resulting numbers are stored in the array of shorts, 
  whose length is 2*nVars. The number of 1's is counted in a different
  space than the original function. For example, if the function depends 
  on k variables, the cofactors are assumed to depend on k-1 variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_TruthCountOnesInCofs( unsigned * pTruth, int nVars, short * pStore )
{
    int nWords = Extra_TruthWordNum( nVars );
    int i, k, Counter;
    memset( pStore, 0, sizeof(short) * 2 * nVars );
    if ( nVars <= 5 )
    {
        if ( nVars > 0 )
        {
            pStore[2*0+0] = Extra_WordCountOnes( pTruth[0] & 0x55555555 );
            pStore[2*0+1] = Extra_WordCountOnes( pTruth[0] & 0xAAAAAAAA );
        }
        if ( nVars > 1 )
        {
            pStore[2*1+0] = Extra_WordCountOnes( pTruth[0] & 0x33333333 );
            pStore[2*1+1] = Extra_WordCountOnes( pTruth[0] & 0xCCCCCCCC );
        }
        if ( nVars > 2 )
        {
            pStore[2*2+0] = Extra_WordCountOnes( pTruth[0] & 0x0F0F0F0F );
            pStore[2*2+1] = Extra_WordCountOnes( pTruth[0] & 0xF0F0F0F0 );
        }
        if ( nVars > 3 )
        {
            pStore[2*3+0] = Extra_WordCountOnes( pTruth[0] & 0x00FF00FF );
            pStore[2*3+1] = Extra_WordCountOnes( pTruth[0] & 0xFF00FF00 );
        }
        if ( nVars > 4 )
        {
            pStore[2*4+0] = Extra_WordCountOnes( pTruth[0] & 0x0000FFFF );
            pStore[2*4+1] = Extra_WordCountOnes( pTruth[0] & 0xFFFF0000 );
        }
        return;
    }
    // nVars >= 6
    // count 1's for all other variables
    for ( k = 0; k < nWords; k++ )
    {
        Counter = Extra_WordCountOnes( pTruth[k] );
        for ( i = 5; i < nVars; i++ )
            if ( k & (1 << (i-5)) )
                pStore[2*i+1] += Counter;
            else
                pStore[2*i+0] += Counter;
    }
    // count 1's for the first five variables
    for ( k = 0; k < nWords/2; k++ )
    {
        pStore[2*0+0] += Extra_WordCountOnes( (pTruth[0] & 0x55555555) | ((pTruth[1] & 0x55555555) <<  1) );
        pStore[2*0+1] += Extra_WordCountOnes( (pTruth[0] & 0xAAAAAAAA) | ((pTruth[1] & 0xAAAAAAAA) >>  1) );
        pStore[2*1+0] += Extra_WordCountOnes( (pTruth[0] & 0x33333333) | ((pTruth[1] & 0x33333333) <<  2) );
        pStore[2*1+1] += Extra_WordCountOnes( (pTruth[0] & 0xCCCCCCCC) | ((pTruth[1] & 0xCCCCCCCC) >>  2) );
        pStore[2*2+0] += Extra_WordCountOnes( (pTruth[0] & 0x0F0F0F0F) | ((pTruth[1] & 0x0F0F0F0F) <<  4) );
        pStore[2*2+1] += Extra_WordCountOnes( (pTruth[0] & 0xF0F0F0F0) | ((pTruth[1] & 0xF0F0F0F0) >>  4) );
        pStore[2*3+0] += Extra_WordCountOnes( (pTruth[0] & 0x00FF00FF) | ((pTruth[1] & 0x00FF00FF) <<  8) );
        pStore[2*3+1] += Extra_WordCountOnes( (pTruth[0] & 0xFF00FF00) | ((pTruth[1] & 0xFF00FF00) >>  8) );
        pStore[2*4+0] += Extra_WordCountOnes( (pTruth[0] & 0x0000FFFF) | ((pTruth[1] & 0x0000FFFF) << 16) );
        pStore[2*4+1] += Extra_WordCountOnes( (pTruth[0] & 0xFFFF0000) | ((pTruth[1] & 0xFFFF0000) >> 16) );
        pTruth += 2;
    }
}


/**Function*************************************************************

  Synopsis    [Canonicize the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Extra_TruthHash( unsigned * pIn, int nWords )
{
    // The 1,024 smallest prime numbers used to compute the hash value
    // http://www.math.utah.edu/~alfeld/math/primelist.html
    static int HashPrimes[1024] = { 2, 3, 5, 
    7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 
    101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 
    193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 
    293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 
    409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 
    521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 
    641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 
    757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 
    881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997, 
    1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069, 1087, 1091, 
    1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 1187, 1193, 
    1201, 1213, 1217, 1223, 1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 1291, 
    1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, 
    1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 
    1499, 1511, 1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601, 
    1607, 1609, 1613, 1619, 1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693, 1697, 1699, 
    1709, 1721, 1723, 1733, 1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811, 
    1823, 1831, 1847, 1861, 1867, 1871, 1873, 1877, 1879, 1889, 1901, 1907, 1913, 1931, 
    1933, 1949, 1951, 1973, 1979, 1987, 1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 
    2039, 2053, 2063, 2069, 2081, 2083, 2087, 2089, 2099, 2111, 2113, 2129, 2131, 2137, 
    2141, 2143, 2153, 2161, 2179, 2203, 2207, 2213, 2221, 2237, 2239, 2243, 2251, 2267, 
    2269, 2273, 2281, 2287, 2293, 2297, 2309, 2311, 2333, 2339, 2341, 2347, 2351, 2357, 
    2371, 2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417, 2423, 2437, 2441, 2447, 2459, 
    2467, 2473, 2477, 2503, 2521, 2531, 2539, 2543, 2549, 2551, 2557, 2579, 2591, 2593, 
    2609, 2617, 2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677, 2683, 2687, 2689, 2693, 
    2699, 2707, 2711, 2713, 2719, 2729, 2731, 2741, 2749, 2753, 2767, 2777, 2789, 2791, 
    2797, 2801, 2803, 2819, 2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 2903, 
    2909, 2917, 2927, 2939, 2953, 2957, 2963, 2969, 2971, 2999, 3001, 3011, 3019, 3023, 
    3037, 3041, 3049, 3061, 3067, 3079, 3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 
    3169, 3181, 3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251, 3253, 3257, 3259, 3271, 
    3299, 3301, 3307, 3313, 3319, 3323, 3329, 3331, 3343, 3347, 3359, 3361, 3371, 3373, 
    3389, 3391, 3407, 3413, 3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 3511, 
    3517, 3527, 3529, 3533, 3539, 3541, 3547, 3557, 3559, 3571, 3581, 3583, 3593, 3607, 
    3613, 3617, 3623, 3631, 3637, 3643, 3659, 3671, 3673, 3677, 3691, 3697, 3701, 3709, 
    3719, 3727, 3733, 3739, 3761, 3767, 3769, 3779, 3793, 3797, 3803, 3821, 3823, 3833, 
    3847, 3851, 3853, 3863, 3877, 3881, 3889, 3907, 3911, 3917, 3919, 3923, 3929, 3931, 
    3943, 3947, 3967, 3989, 4001, 4003, 4007, 4013, 4019, 4021, 4027, 4049, 4051, 4057, 
    4073, 4079, 4091, 4093, 4099, 4111, 4127, 4129, 4133, 4139, 4153, 4157, 4159, 4177, 
    4201, 4211, 4217, 4219, 4229, 4231, 4241, 4243, 4253, 4259, 4261, 4271, 4273, 4283, 
    4289, 4297, 4327, 4337, 4339, 4349, 4357, 4363, 4373, 4391, 4397, 4409, 4421, 4423, 
    4441, 4447, 4451, 4457, 4463, 4481, 4483, 4493, 4507, 4513, 4517, 4519, 4523, 4547, 
    4549, 4561, 4567, 4583, 4591, 4597, 4603, 4621, 4637, 4639, 4643, 4649, 4651, 4657, 
    4663, 4673, 4679, 4691, 4703, 4721, 4723, 4729, 4733, 4751, 4759, 4783, 4787, 4789, 
    4793, 4799, 4801, 4813, 4817, 4831, 4861, 4871, 4877, 4889, 4903, 4909, 4919, 4931, 
    4933, 4937, 4943, 4951, 4957, 4967, 4969, 4973, 4987, 4993, 4999, 5003, 5009, 5011, 
    5021, 5023, 5039, 5051, 5059, 5077, 5081, 5087, 5099, 5101, 5107, 5113, 5119, 5147, 
    5153, 5167, 5171, 5179, 5189, 5197, 5209, 5227, 5231, 5233, 5237, 5261, 5273, 5279, 
    5281, 5297, 5303, 5309, 5323, 5333, 5347, 5351, 5381, 5387, 5393, 5399, 5407, 5413, 
    5417, 5419, 5431, 5437, 5441, 5443, 5449, 5471, 5477, 5479, 5483, 5501, 5503, 5507, 
    5519, 5521, 5527, 5531, 5557, 5563, 5569, 5573, 5581, 5591, 5623, 5639, 5641, 5647, 
    5651, 5653, 5657, 5659, 5669, 5683, 5689, 5693, 5701, 5711, 5717, 5737, 5741, 5743, 
    5749, 5779, 5783, 5791, 5801, 5807, 5813, 5821, 5827, 5839, 5843, 5849, 5851, 5857, 
    5861, 5867, 5869, 5879, 5881, 5897, 5903, 5923, 5927, 5939, 5953, 5981, 5987, 6007, 
    6011, 6029, 6037, 6043, 6047, 6053, 6067, 6073, 6079, 6089, 6091, 6101, 6113, 6121, 
    6131, 6133, 6143, 6151, 6163, 6173, 6197, 6199, 6203, 6211, 6217, 6221, 6229, 6247, 
    6257, 6263, 6269, 6271, 6277, 6287, 6299, 6301, 6311, 6317, 6323, 6329, 6337, 6343, 
    6353, 6359, 6361, 6367, 6373, 6379, 6389, 6397, 6421, 6427, 6449, 6451, 6469, 6473, 
    6481, 6491, 6521, 6529, 6547, 6551, 6553, 6563, 6569, 6571, 6577, 6581, 6599, 6607, 
    6619, 6637, 6653, 6659, 6661, 6673, 6679, 6689, 6691, 6701, 6703, 6709, 6719, 6733, 
    6737, 6761, 6763, 6779, 6781, 6791, 6793, 6803, 6823, 6827, 6829, 6833, 6841, 6857, 
    6863, 6869, 6871, 6883, 6899, 6907, 6911, 6917, 6947, 6949, 6959, 6961, 6967, 6971, 
    6977, 6983, 6991, 6997, 7001, 7013, 7019, 7027, 7039, 7043, 7057, 7069, 7079, 7103, 
    7109, 7121, 7127, 7129, 7151, 7159, 7177, 7187, 7193, 7207, 7211, 7213, 7219, 7229, 
    7237, 7243, 7247, 7253, 7283, 7297, 7307, 7309, 7321, 7331, 7333, 7349, 7351, 7369, 
    7393, 7411, 7417, 7433, 7451, 7457, 7459, 7477, 7481, 7487, 7489, 7499, 7507, 7517, 
    7523, 7529, 7537, 7541, 7547, 7549, 7559, 7561, 7573, 7577, 7583, 7589, 7591, 7603, 
    7607, 7621, 7639, 7643, 7649, 7669, 7673, 7681, 7687, 7691, 7699, 7703, 7717, 7723, 
    7727, 7741, 7753, 7757, 7759, 7789, 7793, 7817, 7823, 7829, 7841, 7853, 7867, 7873, 
    7877, 7879, 7883, 7901, 7907, 7919, 7927, 7933, 7937, 7949, 7951, 7963, 7993, 8009, 
    8011, 8017, 8039, 8053, 8059, 8069, 8081, 8087, 8089, 8093, 8101, 8111, 8117, 8123, 
    8147, 8161 };
    int i;
    unsigned uHashKey;
    assert( nWords <= 1024 );
    uHashKey = 0;
    for ( i = 0; i < nWords; i++ )
        uHashKey ^= HashPrimes[i] * pIn[i];
    return uHashKey;
}

 
/**Function*************************************************************

  Synopsis    [Canonicize the truth table.]

  Description [Returns the phase. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Extra_TruthSemiCanonicize( unsigned * pInOut, unsigned * pAux, int nVars, char * pCanonPerm, short * pStore )
{
    unsigned * pIn = pInOut, * pOut = pAux, * pTemp;
    int nWords = Extra_TruthWordNum( nVars );
    int i, Temp, fChange, Counter, nOnes;//, k, j, w, Limit;
    unsigned uCanonPhase;

    // canonicize output
    uCanonPhase = 0;
    nOnes = Extra_TruthCountOnes(pIn, nVars);
    if ( (nOnes > nWords * 16) || ((nOnes == nWords * 16) && (pIn[0] & 1)) )
    {
        uCanonPhase |= (1 << nVars);
        Extra_TruthNot( pIn, pIn, nVars );
    }

    // collect the minterm counts
    Extra_TruthCountOnesInCofs( pIn, nVars, pStore );

    // canonicize phase
    for ( i = 0; i < nVars; i++ )
    {
        if ( pStore[2*i+0] <= pStore[2*i+1] )
            continue;
        uCanonPhase |= (1 << i);
        Temp = pStore[2*i+0];
        pStore[2*i+0] = pStore[2*i+1];
        pStore[2*i+1] = Temp;
        Extra_TruthChangePhase( pIn, nVars, i );
    }

//    Extra_PrintHexadecimal( stdout, pIn, nVars );
//    printf( "\n" );

    // permute
    Counter = 0;
    do {
        fChange = 0;
        for ( i = 0; i < nVars-1; i++ )
        {
            if ( pStore[2*i] <= pStore[2*(i+1)] )
                continue;
            Counter++;
            fChange = 1;

            Temp = pCanonPerm[i];
            pCanonPerm[i] = pCanonPerm[i+1];
            pCanonPerm[i+1] = Temp;

            Temp = pStore[2*i];
            pStore[2*i] = pStore[2*(i+1)];
            pStore[2*(i+1)] = Temp;

            Temp = pStore[2*i+1];
            pStore[2*i+1] = pStore[2*(i+1)+1];
            pStore[2*(i+1)+1] = Temp;

            Extra_TruthSwapAdjacentVars( pOut, pIn, nVars, i );
            pTemp = pIn; pIn = pOut; pOut = pTemp;
        }
    } while ( fChange );

/*
    Extra_PrintBinary( stdout, &uCanonPhase, nVars+1 ); printf( " : " );
    for ( i = 0; i < nVars; i++ )
        printf( "%d=%d/%d  ", pCanonPerm[i], pStore[2*i], pStore[2*i+1] );
    printf( "  C = %d\n", Counter );
    Extra_PrintHexadecimal( stdout, pIn, nVars );
    printf( "\n" );
*/

/*
    // process symmetric variable groups
    uSymms = 0;
    for ( i = 0; i < nVars-1; i++ )
    {
        if ( pStore[2*i] != pStore[2*(i+1)] ) // i and i+1 cannot be symmetric
            continue;
        if ( pStore[2*i] != pStore[2*i+1] )
            continue;
        if ( Extra_TruthVarsSymm( pIn, nVars, i, i+1 ) )
            continue;
        if ( Extra_TruthVarsAntiSymm( pIn, nVars, i, i+1 ) )
            Extra_TruthChangePhase( pIn, nVars, i+1 );
    }
*/

/*
    // process symmetric variable groups
    uSymms = 0;
    for ( i = 0; i < nVars-1; i++ )
    {
        if ( pStore[2*i] != pStore[2*(i+1)] ) // i and i+1 cannot be symmetric
            continue;
        // i and i+1 can be symmetric
        // find the end of this group
        for ( k = i+1; k < nVars; k++ )
            if ( pStore[2*i] != pStore[2*k] ) 
                break;
        Limit = k;
        assert( i < Limit-1 );
        // go through the variables in this group
        for ( j = i + 1; j < Limit; j++ )
        {
            // check symmetry
            if ( Extra_TruthVarsSymm( pIn, nVars, i, j ) )
            {
                uSymms |= (1 << j);
                continue;
            }
            // they are phase-unknown
            if ( pStore[2*i] == pStore[2*i+1] ) 
            {
                if ( Extra_TruthVarsAntiSymm( pIn, nVars, i, j ) )
                {
                    Extra_TruthChangePhase( pIn, nVars, j );
                    uCanonPhase ^= (1 << j);
                    uSymms |= (1 << j);
                    continue;
                }
            }

            // they are not symmetric - move j as far as it goes in the group
            for ( k = j; k < Limit-1; k++ )
            {
                Counter++;

                Temp = pCanonPerm[k];
                pCanonPerm[k] = pCanonPerm[k+1];
                pCanonPerm[k+1] = Temp;

                assert( pStore[2*k] == pStore[2*(k+1)] );
                Extra_TruthSwapAdjacentVars( pOut, pIn, nVars, k );
                pTemp = pIn; pIn = pOut; pOut = pTemp;
            }
            Limit--;
            j--;
        }
        i = Limit - 1;
    }
*/

    // swap if it was moved an even number of times
    if ( Counter & 1 )
        Extra_TruthCopy( pOut, pIn, nVars );
    return uCanonPhase;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

