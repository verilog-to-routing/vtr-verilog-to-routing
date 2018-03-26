/**CFile****************************************************************

  FileName    [lpkAbcUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    [Procedures working on decomposed functions.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkAbcUtil.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "lpkInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lpk_Fun_t * Lpk_FunAlloc( int nVars )
{
    Lpk_Fun_t * p;
    p = (Lpk_Fun_t *)ABC_ALLOC( char, sizeof(Lpk_Fun_t) + sizeof(unsigned) * Kit_TruthWordNum(nVars) * 3 );
    memset( p, 0, sizeof(Lpk_Fun_t) );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes the function]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_FunFree( Lpk_Fun_t * p )
{
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Creates the starting function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lpk_Fun_t * Lpk_FunCreate( Abc_Ntk_t * pNtk, Vec_Ptr_t * vLeaves, unsigned * pTruth, int nLutK, int AreaLim, int DelayLim )
{
    Lpk_Fun_t * p;
    Abc_Obj_t * pNode;
    int i;
    p = Lpk_FunAlloc( Vec_PtrSize(vLeaves) );
    p->Id = Vec_PtrSize(vLeaves);
    p->vNodes = vLeaves;
    p->nVars = Vec_PtrSize(vLeaves);
    p->nLutK = nLutK;
    p->nAreaLim = AreaLim;
    p->nDelayLim = DelayLim;
    p->uSupp = Kit_TruthSupport( pTruth, p->nVars );
    Kit_TruthCopy( Lpk_FunTruth(p,0), pTruth, p->nVars );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pNode, i )
    {
        p->pFanins[i] = i;
        p->pDelays[i] = pNode->Level;
    }
    Vec_PtrPush( p->vNodes, p );
    return p;
}

/**Function*************************************************************

  Synopsis    [Creates the new function with the given truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lpk_Fun_t * Lpk_FunDup( Lpk_Fun_t * p, unsigned * pTruth )
{
    Lpk_Fun_t * pNew;
    pNew = Lpk_FunAlloc( p->nVars );
    pNew->Id = Vec_PtrSize(p->vNodes);
    pNew->vNodes = p->vNodes;
    pNew->nVars = p->nVars;
    pNew->nLutK = p->nLutK;
    pNew->nAreaLim = p->nAreaLim;
    pNew->nDelayLim = p->nDelayLim;
    pNew->uSupp = Kit_TruthSupport( pTruth, p->nVars );
    Kit_TruthCopy( Lpk_FunTruth(pNew,0), pTruth, p->nVars );
    memcpy( pNew->pFanins, p->pFanins, 16 );
    memcpy( pNew->pDelays, p->pDelays, 16 );
    Vec_PtrPush( p->vNodes, pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Minimizes support of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_FunSuppMinimize( Lpk_Fun_t * p )
{
    int i, k, nVarsNew;
    // compress the truth table
    if ( p->uSupp == Kit_BitMask(p->nVars) )
        return 0;
    // invalidate support info
    p->fSupports = 0;
//Extra_PrintBinary( stdout, &p->uSupp, p->nVars ); printf( "\n" );
    // minimize support
    nVarsNew = Kit_WordCountOnes(p->uSupp);
    Kit_TruthShrink( Lpk_FunTruth(p, 1), Lpk_FunTruth(p, 0), nVarsNew, p->nVars, p->uSupp, 1 );
    k = 0;
    Lpk_SuppForEachVar( p->uSupp, i )
    {
        p->pFanins[k] = p->pFanins[i];
        p->pDelays[k] = p->pDelays[i];
/*
        if ( p->fSupports )
        {
            p->puSupps[2*k+0] = p->puSupps[2*i+0];
            p->puSupps[2*k+1] = p->puSupps[2*i+1];
        }
*/
        k++;
    }
    assert( k == nVarsNew );
    p->nVars = k;
    p->uSupp = Kit_BitMask(p->nVars);
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes cofactors w.r.t. each variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_FunComputeCofSupps( Lpk_Fun_t * p )
{
    unsigned * pTruth  = Lpk_FunTruth( p, 0 );
    unsigned * pTruth0 = Lpk_FunTruth( p, 1 );
    unsigned * pTruth1 = Lpk_FunTruth( p, 2 );
    int Var;
    assert( p->fSupports == 0 );
//    Lpk_SuppForEachVar( p->uSupp, Var )
    for ( Var = 0; Var < (int)p->nVars; Var++ )
    {
        Kit_TruthCofactor0New( pTruth0, pTruth, p->nVars, Var );
        Kit_TruthCofactor1New( pTruth1, pTruth, p->nVars, Var );
        p->puSupps[2*Var+0] = Kit_TruthSupport( pTruth0, p->nVars );
        p->puSupps[2*Var+1] = Kit_TruthSupport( pTruth1, p->nVars );
    }
    p->fSupports = 1;
}

/**Function*************************************************************

  Synopsis    [Get the delay of the bound set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_SuppDelay( unsigned uSupp, char * pDelays )
{
    int Delay, Var;
    Delay = 0;
    Lpk_SuppForEachVar( uSupp, Var )
        Delay = Abc_MaxInt( Delay, pDelays[Var] );
    return Delay + 1;
}

/**Function*************************************************************

  Synopsis    [Converts support into variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_SuppToVars( unsigned uBoundSet, char * pVars )
{
    int i, nVars = 0;
    Lpk_SuppForEachVar( uBoundSet, i )
        pVars[nVars++] = i;
    return nVars;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

