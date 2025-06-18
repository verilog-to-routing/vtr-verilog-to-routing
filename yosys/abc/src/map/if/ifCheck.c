/**CFile****************************************************************

  FileName    [ifCheck.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Sequential mapping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifCheck.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#ifdef WIN32
typedef unsigned __int64 word;
#else
typedef unsigned long long word;
#endif

// elementary truth tables
static word Truths6[6] = {
    0xAAAAAAAAAAAAAAAA,
    0xCCCCCCCCCCCCCCCC,
    0xF0F0F0F0F0F0F0F0,
    0xFF00FF00FF00FF00,
    0xFFFF0000FFFF0000,
    0xFFFFFFFF00000000
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns 1 if the node Leaf is reachable on the path.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManCutReach_rec( If_Obj_t * pPath, If_Obj_t * pLeaf )
{
    if ( pPath == pLeaf )
        return 1;
    if ( pPath->fMark )
        return 0;
    assert( If_ObjIsAnd(pPath) );
    if ( If_ManCutReach_rec( If_ObjFanin0(pPath), pLeaf ) )
        return 1;
    if ( If_ManCutReach_rec( If_ObjFanin1(pPath), pLeaf ) )
        return 1;
    return 0;
}
int If_ManCutReach( If_Man_t * p, If_Cut_t * pCut, If_Obj_t * pPath, If_Obj_t * pLeaf )
{
    If_Obj_t * pTemp;
    int i, RetValue;
    If_CutForEachLeaf( p, pCut, pTemp, i )
        pTemp->fMark = 1;
    RetValue = If_ManCutReach_rec( pPath, pLeaf );
    If_CutForEachLeaf( p, pCut, pTemp, i )
        pTemp->fMark = 0;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Derive truth table for each cofactor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManCutTruthCheck_rec( If_Obj_t * pObj, word * pTruths )
{
    word T0, T1;
    if ( pObj->fMark )
        return pTruths[If_ObjId(pObj)];
    assert( If_ObjIsAnd(pObj) );
    T0 = If_ManCutTruthCheck_rec( If_ObjFanin0(pObj), pTruths );
    T1 = If_ManCutTruthCheck_rec( If_ObjFanin1(pObj), pTruths );
    T0 = If_ObjFaninC0(pObj) ? ~T0 : T0;
    T1 = If_ObjFaninC1(pObj) ? ~T1 : T1;
    return T0 & T1;
}
int If_ManCutTruthCheck( If_Man_t * p, If_Obj_t * pObj, If_Cut_t * pCut, If_Obj_t * pLeaf, int Cof, word * pTruths )
{
    word Truth;
    If_Obj_t * pTemp;
    int i, k = 0;
    assert( Cof == 0 || Cof == 1 );
    If_CutForEachLeaf( p, pCut, pTemp, i )
    {
        assert( pTemp->fMark == 0 );
        pTemp->fMark = 1;
        if ( pLeaf == pTemp )
            pTruths[If_ObjId(pTemp)] = (Cof ? ~((word)0) : 0);
        else
            pTruths[If_ObjId(pTemp)] = Truths6[k++] ;
    }
    assert( k + 1 == If_CutLeaveNum(pCut) );
    // compute truth table
    Truth = If_ManCutTruthCheck_rec( pObj, pTruths );
    If_CutForEachLeaf( p, pCut, pTemp, i )
        pTemp->fMark = 0;
    return Truth == 0 || Truth == ~((word)0);
}


/**Function*************************************************************

  Synopsis    [Checks if cut can be structurally/functionally decomposed.]

  Description [The decomposition is Fn(a,b,c,...) = F2(a, Fn-1(b,c,...)).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManCutCheck( If_Man_t * p, If_Obj_t * pObj, If_Cut_t * pCut )
{
    static int nDecCalls    = 0;
    static int nDecStruct   = 0;
    static int nDecStruct2  = 0;
    static int nDecFunction = 0;
    word * pTruths;
    If_Obj_t * pLeaf, * pPath;
    int i;
    if ( pCut == NULL )
    {
        printf( "DecStr  = %9d (%6.2f %%).\n", nDecStruct,   100.0*nDecStruct/nDecCalls );
        printf( "DecStr2 = %9d (%6.2f %%).\n", nDecStruct2,  100.0*nDecStruct2/nDecCalls );
        printf( "DecFunc = %9d (%6.2f %%).\n", nDecFunction, 100.0*nDecFunction/nDecCalls );
        printf( "Total   = %9d (%6.2f %%).\n", nDecCalls,    100.0*nDecCalls/nDecCalls );
        return;
    }
    assert( If_ObjIsAnd(pObj) );
    assert( pCut->nLeaves <= 7 );
    nDecCalls++;
    // check structural decomposition
    If_CutForEachLeaf( p, pCut, pLeaf, i )
        if ( pLeaf == If_ObjFanin0(pObj) || pLeaf == If_ObjFanin1(pObj) )
            break;
    if ( i < If_CutLeaveNum(pCut) )
    {
        pPath = (pLeaf == If_ObjFanin0(pObj)) ? If_ObjFanin1(pObj) : If_ObjFanin0(pObj);
        if ( !If_ManCutReach( p, pCut, pPath, pLeaf ) )
        {
            nDecStruct++;
//            nDecFunction++;
//            return;
        }
        else
            nDecStruct2++;
    }
    // check functional decomposition
    pTruths = malloc( sizeof(word) * If_ManObjNum(p) );
    If_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        if ( If_ManCutTruthCheck( p, pObj, pCut, pLeaf, 0, pTruths ) )
        {
            nDecFunction++;
            break;
        }
        if ( If_ManCutTruthCheck( p, pObj, pCut, pLeaf, 1, pTruths ) )
        {
            nDecFunction++;
            break;
        }
    }
    free( pTruths );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

