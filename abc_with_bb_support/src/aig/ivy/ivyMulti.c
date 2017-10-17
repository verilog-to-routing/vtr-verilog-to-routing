/**CFile****************************************************************

  FileName    [ivyMulti.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Constructing multi-input AND/EXOR gates.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyMulti.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define IVY_EVAL_LIMIT    128

typedef struct Ivy_Eva_t_ Ivy_Eva_t;
struct Ivy_Eva_t_
{
    Ivy_Obj_t * pArg;     // the argument node
    unsigned    Mask;     // the mask of covered nodes
    int         Weight;   // the number of covered nodes
};

static void Ivy_MultiPrint( Ivy_Man_t * p, Ivy_Eva_t * pEvals, int nLeaves, int nEvals );
static int Ivy_MultiCover( Ivy_Man_t * p, Ivy_Eva_t * pEvals, int nLeaves, int nEvals, int nLimit, Vec_Ptr_t * vSols );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Constructs a balanced tree while taking sharing into account.]

  Description [Returns 1 if the implementation exists.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_MultiPlus( Ivy_Man_t * p, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vCone, Ivy_Type_t Type, int nLimit, Vec_Ptr_t * vSols )
{
    static Ivy_Eva_t pEvals[IVY_EVAL_LIMIT];
    Ivy_Eva_t * pEval, * pFan0, * pFan1;
    Ivy_Obj_t * pObj = NULL; // Suppress "might be used uninitialized"
    Ivy_Obj_t * pTemp;
    int nEvals, nEvalsOld, i, k, x, nLeaves;
    unsigned uMaskAll;

    // consider special cases
    nLeaves = Vec_PtrSize(vLeaves);
    assert( nLeaves > 2 );
    if ( nLeaves > 32 || nLeaves + Vec_PtrSize(vCone) > IVY_EVAL_LIMIT )
        return 0;
//    if ( nLeaves == 1 )
//        return Vec_PtrEntry( vLeaves, 0 );
//    if ( nLeaves == 2 )
//        return Ivy_Oper( Vec_PtrEntry(vLeaves, 0), Vec_PtrEntry(vLeaves, 1), Type );

    // set the leaf entries
    uMaskAll = ((1 << nLeaves) - 1);
    nEvals = 0;
    Vec_PtrForEachEntry( Ivy_Obj_t *, vLeaves, pObj, i )
    {
        pEval = pEvals + nEvals;
        pEval->pArg   = pObj;
        pEval->Mask   = (1 << nEvals);
        pEval->Weight = 1;
        // mark the leaf
        Ivy_Regular(pObj)->TravId = nEvals;
        nEvals++;
    }

    // propagate masks through the cone
    Vec_PtrForEachEntry( Ivy_Obj_t *, vCone, pObj, i )
    {
        pObj->TravId = nEvals + i;
        if ( Ivy_ObjIsBuf(pObj) )
            pEvals[pObj->TravId].Mask = pEvals[Ivy_ObjFanin0(pObj)->TravId].Mask;
        else
            pEvals[pObj->TravId].Mask = pEvals[Ivy_ObjFanin0(pObj)->TravId].Mask | pEvals[Ivy_ObjFanin1(pObj)->TravId].Mask;
    }

    // set the internal entries
    Vec_PtrForEachEntry( Ivy_Obj_t *, vCone, pObj, i )
    {
        if ( i == Vec_PtrSize(vCone) - 1 )
            break;
        // skip buffers
        if ( Ivy_ObjIsBuf(pObj) )
            continue;
        // skip nodes without external fanout
        if ( Ivy_ObjRefs(pObj) == 0 )
            continue;
        assert( !Ivy_IsComplement(pObj) );
        pEval = pEvals + nEvals;
        pEval->pArg   = pObj;
        pEval->Mask   = pEvals[pObj->TravId].Mask;
        pEval->Weight = Extra_WordCountOnes(pEval->Mask);
        // mark the node
        pObj->TravId = nEvals;
        nEvals++;
    }

    // find the available nodes
    nEvalsOld = nEvals;
    for ( i = 1; i < nEvals; i++ )
    for ( k = 0; k < i; k++ )
    {
        pFan0 = pEvals + i;
        pFan1 = pEvals + k;
        pTemp = Ivy_TableLookup(p, Ivy_ObjCreateGhost(p, pFan0->pArg, pFan1->pArg, Type, IVY_INIT_NONE));
        // skip nodes in the cone
        if ( pTemp == NULL || pTemp->fMarkB )
            continue;
        // skip the leaves
        for ( x = 0; x < nLeaves; x++ )
            if ( pTemp == Ivy_Regular((Ivy_Obj_t *)vLeaves->pArray[x]) )
                break;
        if ( x < nLeaves )
            continue;
        pEval = pEvals + nEvals;
        pEval->pArg   = pTemp;
        pEval->Mask   = pFan0->Mask | pFan1->Mask;
        pEval->Weight = (pFan0->Mask & pFan1->Mask) ? Extra_WordCountOnes(pEval->Mask) : pFan0->Weight + pFan1->Weight;
        // save the argument
        pObj->TravId = nEvals;
        nEvals++;
        // quit if the number of entries exceeded the limit
        if ( nEvals == IVY_EVAL_LIMIT )
            goto Outside;
        // quit if we found an acceptable implementation
        if ( pEval->Mask == uMaskAll )
            goto Outside;
    }
Outside:

//    Ivy_MultiPrint( pEvals, nLeaves, nEvals );
    if ( !Ivy_MultiCover( p, pEvals, nLeaves, nEvals, nLimit, vSols ) )
        return 0;
    assert( Vec_PtrSize( vSols ) > 0 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes how many uncovered ones this one covers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_MultiPrint( Ivy_Man_t * p, Ivy_Eva_t * pEvals, int nLeaves, int nEvals )
{
    Ivy_Eva_t * pEval;
    int i, k;
    for ( i = nLeaves; i < nEvals; i++ )
    {
        pEval = pEvals + i;
        printf( "%2d  (id = %5d)  : |", i-nLeaves, Ivy_ObjId(pEval->pArg) );
        for ( k = 0; k < nLeaves; k++ )
        {
            if ( pEval->Mask & (1 << k) )
                printf( "+" );
            else
                printf( " " );
        }
        printf( "|  Lev = %d.\n", Ivy_ObjLevel(pEval->pArg) );
    }
}

/**Function*************************************************************

  Synopsis    [Computes how many uncovered ones this one covers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ivy_MultiWeight( unsigned uMask, int nMaskOnes, unsigned uFound )
{
    assert( uMask & ~uFound );
    if ( (uMask & uFound) == 0 )
        return nMaskOnes;
    return Extra_WordCountOnes( uMask & ~uFound );
}

/**Function*************************************************************

  Synopsis    [Finds the cover.]

  Description [Returns 1 if the cover is found.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_MultiCover( Ivy_Man_t * p, Ivy_Eva_t * pEvals, int nLeaves, int nEvals, int nLimit, Vec_Ptr_t * vSols )
{
    int fVerbose = 0;
    Ivy_Eva_t * pEval;
    Ivy_Eva_t * pEvalBest = NULL; // Suppress "might be used uninitialized"
    unsigned uMaskAll, uFound, uTemp;
    int i, k, BestK;
    int WeightBest = -1; // Suppress "might be used uninitialized"
    int WeightCur;
    int LevelBest = -1; // Suppress "might be used uninitialized"
    int LevelCur;
    uMaskAll = (nLeaves == 32)? (~(unsigned)0) : ((1 << nLeaves) - 1);
    uFound = 0;
    // solve the covering problem
    if ( fVerbose )
    printf( "Solution:  " );
    Vec_PtrClear( vSols );
    for ( i = 0; i < nLimit; i++ )
    {
        BestK = -1;
        for ( k = nEvals - 1; k >= 0; k-- )
        {
            pEval = pEvals + k;
            if ( (pEval->Mask & ~uFound) == 0 )
                continue;
            if ( BestK == -1 )
            {
                BestK      = k;
                pEvalBest  = pEval;
                WeightBest = Ivy_MultiWeight( pEvalBest->Mask, pEvalBest->Weight, uFound );
                LevelBest  = Ivy_ObjLevel( Ivy_Regular(pEvalBest->pArg) );
                continue;
            }
            // compare BestK and the new one (k)
            WeightCur = Ivy_MultiWeight( pEval->Mask, pEval->Weight, uFound );
            LevelCur = Ivy_ObjLevel( Ivy_Regular(pEval->pArg) );
            if ( WeightBest < WeightCur || 
                (WeightBest == WeightCur && LevelBest > LevelCur) )
            {
                BestK      = k;
                pEvalBest  = pEval;
                WeightBest = WeightCur;
                LevelBest  = LevelCur;
            }
        }
        assert( BestK != -1 );
        // if the cost is only 1, take the leaf
        if ( WeightBest == 1 && BestK >= nLeaves )
        {
            uTemp = (pEvalBest->Mask & ~uFound);
            for ( k = 0; k < nLeaves; k++ )
                if ( uTemp & (1 << k) )
                    break;
            assert( k < nLeaves );
            BestK     = k;
            pEvalBest = pEvals + BestK;
        }
        if ( fVerbose )
        {
            if ( BestK < nLeaves )
                printf( "L(%d) ", BestK );
            else
                printf( "%d ", BestK - nLeaves );
        }
        // update the found set
        Vec_PtrPush( vSols, pEvalBest->pArg );
        uFound |= pEvalBest->Mask;
        if ( uFound == uMaskAll )
            break;
    }
    if ( uFound == uMaskAll )
    {
        if ( fVerbose )
            printf( "  Found \n\n" );
        return 1;
    }
    else
    {
        if ( fVerbose )
            printf( "  Not found \n\n" );
        return 0;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

