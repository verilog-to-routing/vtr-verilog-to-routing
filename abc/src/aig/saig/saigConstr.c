/**CFile****************************************************************

  FileName    [saigConstr.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Structural constraint detection.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigConstr.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"
#include "bool/kit/kit.h"
#include "aig/ioa/ioa.h"

ABC_NAMESPACE_IMPL_START

/*
    Property holds iff it is const 0.
    Constraint holds iff it is const 0.

    The following structure is used for folding constraints:
    - the output of OR gate is 0 as long as all constraints hold
    - as soon as one constraint fail, the property output becomes 0 forever
      because the flop becomes 1 and it stays 1 forever


       property output

             |
          |-----|
          | and |
          |-----|
           |   |
           |  / \
           | /inv\
           | -----
       ____|   |_________________________
       |             |                  |
      / \       -----------             |   
     /   \      |   or    |             |
    /     \     -----------             |
   / logic \     |   |   |              |
  /  cone   \    |   |   |              |
 /___________\   |   |   |              |
                 |   | ------           |   
                 |   | |flop| (init=0)  |
                 |   | ------           |
                 |   |   |              |
                 |   |   |______________| 
                 |   |
                 c1  c2 
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_DetectConstrCollectSuper_rec( Aig_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    // if the new node is complemented or a PI, another gate begins
    if ( Aig_IsComplement(pObj) || !Aig_ObjIsNode(pObj) )//|| (Aig_ObjRefs(pObj) > 1) )
    {
        Vec_PtrPushUnique( vSuper, Aig_Not(pObj) );
        return;
    }
    // go through the branches
    Saig_DetectConstrCollectSuper_rec( Aig_ObjChild0(pObj), vSuper );
    Saig_DetectConstrCollectSuper_rec( Aig_ObjChild1(pObj), vSuper );
}

/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_DetectConstrCollectSuper( Aig_Obj_t * pObj )
{
    Vec_Ptr_t * vSuper;
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsAnd(pObj) );
    vSuper = Vec_PtrAlloc( 4 );
    Saig_DetectConstrCollectSuper_rec( Aig_ObjChild0(pObj), vSuper );
    Saig_DetectConstrCollectSuper_rec( Aig_ObjChild1(pObj), vSuper );
    return vSuper;
}

/**Function*************************************************************

  Synopsis    [Returns NULL if not contained, or array with unique entries.]

  Description [Returns NULL if vSuper2 is not contained in vSuper. Otherwise
  returns the array of entries in vSuper that are not found in vSuper2.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManDetectConstrCheckCont( Vec_Ptr_t * vSuper, Vec_Ptr_t * vSuper2 )
{
    Vec_Ptr_t * vUnique;
    Aig_Obj_t * pObj, * pObj2;
    int i;
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper2, pObj2, i )
        if ( Vec_PtrFind( vSuper, pObj2 ) == -1 )
            return 0;
    vUnique = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pObj, i )
        if ( Vec_PtrFind( vSuper2, pObj ) == -1 )
            Vec_PtrPush( vUnique, pObj );
    return vUnique;
}

/**Function*************************************************************

  Synopsis    [Detects constraints using structural methods.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManDetectConstr( Aig_Man_t * p, int iOut, Vec_Ptr_t ** pvOuts, Vec_Ptr_t ** pvCons )
{
    Vec_Ptr_t * vSuper, * vSuper2 = NULL, * vUnique;
    Aig_Obj_t * pObj, * pObj2, * pFlop;
    int i, nFlops, RetValue;
    assert( iOut >= 0 && iOut < Saig_ManPoNum(p) );
    *pvOuts = NULL;
    *pvCons = NULL;
    pObj = Aig_ObjChild0( Aig_ManCo(p, iOut) );
    if ( pObj == Aig_ManConst0(p) )
    {
        vUnique = Vec_PtrStart( 1 );
        Vec_PtrWriteEntry( vUnique, 0, Aig_ManConst1(p) );
        *pvOuts = vUnique;
        *pvCons = Vec_PtrAlloc( 0 );
        return -1;
    }
    if ( Aig_IsComplement(pObj) || !Aig_ObjIsNode(pObj) )
    {
        printf( "The output is not an AND.\n" );
        return 0;
    }
    vSuper = Saig_DetectConstrCollectSuper( pObj );
    assert( Vec_PtrSize(vSuper) >= 2 );
    nFlops = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pObj, i )
        nFlops += Saig_ObjIsLo( p, Aig_Regular(pObj) );
    if ( nFlops == 0 )
    {
        printf( "There is no flop outputs.\n" );
        Vec_PtrFree( vSuper );
        return 0;
    }
    // try flops 
    vUnique = NULL;
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pObj, i )
    {
        pFlop = Aig_Regular( pObj );
        if ( !Saig_ObjIsLo(p, pFlop) )
            continue;
        pFlop = Saig_ObjLoToLi( p, pFlop );
        pObj2 = Aig_ObjChild0( pFlop );
        if ( !Aig_IsComplement(pObj2) || !Aig_ObjIsNode(Aig_Regular(pObj2)) )
            continue;
        vSuper2 = Saig_DetectConstrCollectSuper( Aig_Regular(pObj2) );
        // every node in vSuper2 should appear in vSuper
        vUnique = Saig_ManDetectConstrCheckCont( vSuper, vSuper2 );
        if ( vUnique != NULL )
        {
///           assert( !Aig_IsComplement(pObj) );
 //           assert( Vec_PtrFind( vSuper2, pObj ) >= 0 );
            if ( Aig_IsComplement(pObj) )
            {
                printf( "Special flop input is complemented.\n" );
                Vec_PtrFreeP( &vUnique );
                Vec_PtrFree( vSuper2 );
                break;
            }
            if ( Vec_PtrFind( vSuper2, pObj ) == -1 )
            {
                printf( "Cannot find special flop about the inputs of OR gate.\n" );
                Vec_PtrFreeP( &vUnique );
                Vec_PtrFree( vSuper2 );
                break;
            }
            // remove the flop output
            Vec_PtrRemove( vSuper2, pObj );
            break;
        }
        Vec_PtrFree( vSuper2 );
    }
    Vec_PtrFree( vSuper );
    if ( vUnique == NULL )
    {
        printf( "There is no structural constraints.\n" );
        return 0;
    }
    // vUnique contains unique entries
    // vSuper2 contains the supergate
    printf( "Output %d : Structural analysis found %d original properties and %d constraints.\n", 
        iOut, Vec_PtrSize(vUnique), Vec_PtrSize(vSuper2) );
    // remember the number of constraints
    RetValue = Vec_PtrSize(vSuper2);
    // make the AND of properties 
//    Vec_PtrFree( vUnique );
//    Vec_PtrFree( vSuper2 );
    *pvOuts = vUnique;
    *pvCons = vSuper2;
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Procedure used for sorting nodes by ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManDupCompare( Aig_Obj_t ** pp1, Aig_Obj_t ** pp2 )
{
    int Diff = Aig_ObjToLit(*pp1) - Aig_ObjToLit(*pp2);
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG while unfolding constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDupUnfoldConstrs( Aig_Man_t * pAig )
{
    Vec_Ptr_t * vOutsAll, * vConsAll;
    Vec_Ptr_t * vOuts, * vCons, * vCons0;
    Aig_Man_t * pAigNew;
    Aig_Obj_t * pMiter, * pObj;
    int i, k, RetValue;
    // detect constraints for each output
    vOutsAll = Vec_PtrAlloc( Saig_ManPoNum(pAig) );
    vConsAll = Vec_PtrAlloc( Saig_ManPoNum(pAig) );
    Saig_ManForEachPo( pAig, pObj, i )
    {
        RetValue = Saig_ManDetectConstr( pAig, i, &vOuts, &vCons );
        if ( RetValue == 0 )
        {
            Vec_PtrFreeP( &vOuts );
            Vec_PtrFreeP( &vCons );
            Vec_VecFree( (Vec_Vec_t *)vOutsAll );
            Vec_VecFree( (Vec_Vec_t *)vConsAll );
            return Aig_ManDupDfs( pAig );
        }
        Vec_PtrSort( vOuts, (int (*)(const void *, const void *))Saig_ManDupCompare );
        Vec_PtrSort( vCons, (int (*)(const void *, const void *))Saig_ManDupCompare );
        Vec_PtrPush( vOutsAll, vOuts );
        Vec_PtrPush( vConsAll, vCons );
    }
    // check if constraints are compatible
    vCons0 = (Vec_Ptr_t *)Vec_PtrEntry( vConsAll, 0 );
    Vec_PtrForEachEntry( Vec_Ptr_t *, vConsAll, vCons, i )
        if ( Vec_PtrSize(vCons) )
            vCons0 = vCons;
    Vec_PtrForEachEntry( Vec_Ptr_t *, vConsAll, vCons, i )
    {
        // Constant 0 outputs are always compatible (vOuts stores the negation)
        vOuts = (Vec_Ptr_t *)Vec_PtrEntry( vOutsAll, i );
        if ( Vec_PtrSize(vOuts) == 1 && (Aig_Obj_t *)Vec_PtrEntry( vOuts, 0 ) == Aig_ManConst1(pAig) )
            continue;
        if ( !Vec_PtrEqual(vCons0, vCons) )
            break;
    }
    if ( i < Vec_PtrSize(vConsAll) )
    {
        printf( "Collected constraints are not compatible.\n" );
        Vec_VecFree( (Vec_Vec_t *)vOutsAll );
        Vec_VecFree( (Vec_Vec_t *)vConsAll );
        return Aig_ManDupDfs( pAig );
    }

    // start the new manager
    pAigNew = Aig_ManStart( Aig_ManNodeNum(pAig) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    // create variables for PIs
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // transform each output
    Vec_PtrForEachEntry( Vec_Ptr_t *, vOutsAll, vOuts, i )
    {
        // AND the outputs
        pMiter = Aig_ManConst1( pAigNew );
        Vec_PtrForEachEntry( Aig_Obj_t *, vOuts, pObj, k )
            pMiter = Aig_And( pAigNew, pMiter, Aig_Not(Aig_ObjRealCopy(pObj)) );
        Aig_ObjCreateCo( pAigNew, pMiter );
    }
    // add constraints
    pAigNew->nConstrs = Vec_PtrSize(vCons0);
    Vec_PtrForEachEntry( Aig_Obj_t *, vCons0, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjRealCopy(pObj) );
    // transfer to register outputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );
//    Vec_PtrFreeP( &vOuts );
//    Vec_PtrFreeP( &vCons );
    Vec_VecFree( (Vec_Vec_t *)vOutsAll );
    Vec_VecFree( (Vec_Vec_t *)vConsAll );

    Aig_ManSetRegNum( pAigNew, Aig_ManRegNum(pAig) );
    Aig_ManCleanup( pAigNew );
    Aig_ManSeqCleanup( pAigNew );
    return pAigNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG while folding in the constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDupFoldConstrs( Aig_Man_t * pAig, Vec_Int_t * vConstrs )
{
    Aig_Man_t * pAigNew;
    Aig_Obj_t * pMiter, * pFlopOut, * pFlopIn, * pObj;
    int Entry, i;
    assert( Saig_ManRegNum(pAig) > 0 );
    // start the new manager
    pAigNew = Aig_ManStart( Aig_ManNodeNum(pAig) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    // create variables for PIs
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );

    // OR the constraint outputs
    pMiter = Aig_ManConst0( pAigNew );
    Vec_IntForEachEntry( vConstrs, Entry, i )
    {
        assert( Entry > 0 && Entry < Saig_ManPoNum(pAig) );
        pObj = Aig_ManCo( pAig, Entry );
        pMiter = Aig_Or( pAigNew, pMiter, Aig_ObjChild0Copy(pObj) );
    }
    // create additional flop
    pFlopOut = Aig_ObjCreateCi( pAigNew );
    pFlopIn  = Aig_Or( pAigNew, pMiter, pFlopOut );

    // create primary output
    Saig_ManForEachPo( pAig, pObj, i )
    {
        pMiter = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_Not(pFlopIn) );
        Aig_ObjCreateCo( pAigNew, pMiter );
    }

    // transfer to register outputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );
    // create additional flop 
    Aig_ObjCreateCo( pAigNew, pFlopIn );

    Aig_ManSetRegNum( pAigNew, Aig_ManRegNum(pAig)+1 );
    Aig_ManCleanup( pAigNew );
    Aig_ManSeqCleanup( pAigNew );
    return pAigNew;
}


/**Function*************************************************************

  Synopsis    [Tests the above two procedures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManFoldConstrTest( Aig_Man_t * pAig )
{
    Aig_Man_t * pAig1, * pAig2;
    Vec_Int_t * vConstrs;
    // unfold constraints
    pAig1 = Saig_ManDupUnfoldConstrs( pAig );
    // create the constraint list
    vConstrs = Vec_IntStartNatural( Saig_ManPoNum(pAig1) );
    Vec_IntRemove( vConstrs, 0 );
    // fold constraints back
    pAig2 = Saig_ManDupFoldConstrs( pAig1, vConstrs );
    Vec_IntFree( vConstrs );
    // compare the two AIGs
    Ioa_WriteAiger( pAig2, "test.aig", 0, 0 );
    Aig_ManStop( pAig1 );
    Aig_ManStop( pAig2 );
}

/**Function*************************************************************

  Synopsis    [Experiment with the above procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManDetectConstrTest( Aig_Man_t * p )
{
    Vec_Ptr_t * vOuts, * vCons;
    int RetValue = Saig_ManDetectConstr( p, 0, &vOuts, &vCons );
    Vec_PtrFreeP( &vOuts );
    Vec_PtrFreeP( &vCons );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

