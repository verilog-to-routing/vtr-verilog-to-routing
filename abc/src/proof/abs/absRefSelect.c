/**CFile****************************************************************

  FileName    [absRefSelect.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [Post-processes the set of selected refinement objects.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absRefSelect.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "abs.h"
#include "absRef.h"

ABC_NAMESPACE_IMPL_START 

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rnm_ManPrintSelected( Rnm_Man_t * p, Vec_Int_t * vNewPPis )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    Gia_ManForEachObjVec( p->vMap, p->pGia, pObj, i )
        if ( Gia_ObjIsPi(p->pGia, pObj) ) 
            printf( "-" );
        else if ( Vec_IntFind(vNewPPis, Gia_ObjId(p->pGia, pObj)) >= 0 )// this is PPI
            printf( "1" ), Counter++;
        else
            printf( "0" );
    printf( " %3d\n", Counter );
}

/**Function*************************************************************

  Synopsis    [Perform structural analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ga2_StructAnalize( Gia_Man_t * p, Vec_Int_t * vFront, Vec_Int_t * vInter, Vec_Int_t * vNewPPis )
{
    Vec_Int_t * vFanins;
    Gia_Obj_t * pObj, * pFanin;
    int i, k;
    // clean labels
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = pObj->fMark1 = 0;
    // label frontier
    Gia_ManForEachObjVec( vFront, p, pObj, i )
        pObj->fMark0 = 1, pObj->fMark1 = 0;
    // label objects
    Gia_ManForEachObjVec( vInter, p, pObj, i )
        pObj->fMark1 = 0, pObj->fMark1 = 1;
    // label selected
    Gia_ManForEachObjVec( vNewPPis, p, pObj, i )
        pObj->fMark1 = 1, pObj->fMark1 = 1;
    // explore selected
    Gia_ManForEachObjVec( vNewPPis, p, pObj, i )
    {
        printf( "Selected PPI %3d : ", i+1 );
        printf( "%6d ",  Gia_ObjId(p, pObj) );
        printf( "\n" );
        vFanins = Ga2_ObjLeaves( p, pObj );
        Gia_ManForEachObjVec( vFanins, p, pFanin, k )
        {
            printf( "    " );
            printf( "%6d ", Gia_ObjId(p, pFanin) );
            if ( pFanin->fMark0 && pFanin->fMark1 )
                printf( "selected PPI" );
            else if ( pFanin->fMark0 && !pFanin->fMark1 )
                printf( "frontier (original PI or PPI)" );
            else if ( !pFanin->fMark0 &&  pFanin->fMark1 )
                printf( "abstracted node" );
            else if ( !pFanin->fMark0 && !pFanin->fMark1 )
                printf( "free variable" );
            printf( "\n" );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Postprocessing the set of PPIs using structural analysis.]

  Description [The following sets are used:
  The set of all PI+PPI is in p->vMap.
  The set of all abstracted objects is in p->vObjs;
  The set of important PPIs is in vOldPPis.
  The new set of selected PPIs is in vNewPPis.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Rnm_ManFilterSelected( Rnm_Man_t * p, Vec_Int_t * vOldPPis )
{
    int fVerbose = 0;
    Vec_Int_t * vNewPPis, * vFanins;
    Gia_Obj_t * pObj, * pFanin;
    int i, k, RetValue, Counters[3] = {0};

    // (0) make sure fanin counters are 0 at the beginning
//    Gia_ManForEachObj( p->pGia, pObj, i )
//        assert( Rnm_ObjCount(p, pObj) == 0 );

    // (1) increment PPI fanin counters
    Vec_IntClear( p->vFanins );
    Gia_ManForEachObjVec( vOldPPis, p->pGia, pObj, i )
    {
        vFanins = Ga2_ObjLeaves( p->pGia, pObj );
        Gia_ManForEachObjVec( vFanins, p->pGia, pFanin, k )
            if ( Rnm_ObjAddToCount(p, pFanin) == 0 ) // fanin counter is 0 -- save it
                Vec_IntPush( p->vFanins, Gia_ObjId(p->pGia, pFanin) );
    }

    // (3) select objects with reconvergence, which create potential constraints
    // - flop objects
    // - objects whose fanin belongs to the justified area
    // - objects whose fanins overlap
    // (these do not guantee reconvergence, but may potentially have it)
    // (other objects cannot have reconvergence, even if they are added)
    vNewPPis = Vec_IntAlloc( 100 );
    Gia_ManForEachObjVec( vOldPPis, p->pGia, pObj, i )
    {
        if ( Gia_ObjIsRo(p->pGia, pObj) )
        {
            if ( fVerbose )
                Counters[0]++;
            Vec_IntPush( vNewPPis, Gia_ObjId(p->pGia, pObj) );
            continue;
        }
        vFanins = Ga2_ObjLeaves( p->pGia, pObj );
        Gia_ManForEachObjVec( vFanins, p->pGia, pFanin, k )
        {
            if ( Rnm_ObjIsJust(p, pFanin) || Rnm_ObjCount(p, pFanin) > 1 )
            {
                if ( fVerbose )
                    Counters[1] += Rnm_ObjIsJust(p, pFanin);
                if ( fVerbose )
                    Counters[2] += (Rnm_ObjCount(p, pFanin) > 1);
                Vec_IntPush( vNewPPis, Gia_ObjId(p->pGia, pObj) );
                break;
            }
        }
    }
    RetValue = Vec_IntUniqify( vNewPPis );
    assert( RetValue == 0 );

    // (4) clear fanin counters
    // this is important for counters to be correctly set in the future iterations -- see step (0)
    Gia_ManForEachObjVec( p->vFanins, p->pGia, pObj, i )
        Rnm_ObjSetCount( p, pObj, 0 );

    // visualize
    if ( fVerbose )
        printf( "*** Refinement %3d : PI+PPI =%4d. Old =%4d. New =%4d.   FF =%4d. Just =%4d. Shared =%4d.\n", 
            p->nRefId, Vec_IntSize(p->vMap), Vec_IntSize(vOldPPis), Vec_IntSize(vNewPPis), Counters[0], Counters[1], Counters[2] );

//    Rnm_ManPrintSelected( p, vNewPPis );
//    Ga2_StructAnalize( p->pGia, p->vMap, p->vObjs, vNewPPis );
    return vNewPPis;
}


/**Function*************************************************************

  Synopsis    [Improved postprocessing the set of PPIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Rnm_ManFilterSelectedNew( Rnm_Man_t * p, Vec_Int_t * vOldPPis )
{
    static int Counter = 0;
    int fVerbose = 0;
    Vec_Int_t * vNewPPis, * vFanins, * vFanins2;
    Gia_Obj_t * pObj, * pFanin, * pFanin2;
    int i, k, k2, RetValue, Counters[3] = {0};

    // return full set of PPIs once in a while
    if ( ++Counter % 9 == 0 )
        return Vec_IntDup( vOldPPis );
    return Rnm_ManFilterSelected( p, vOldPPis );

    // (0) make sure fanin counters are 0 at the beginning
//    Gia_ManForEachObj( p->pGia, pObj, i )
//        assert( Rnm_ObjCount(p, pObj) == 0 );

    // (1) increment two levels of PPI fanin counters
    Vec_IntClear( p->vFanins );
    Gia_ManForEachObjVec( vOldPPis, p->pGia, pObj, i )
    {
        // go through the fanins
        vFanins = Ga2_ObjLeaves( p->pGia, pObj );
        Gia_ManForEachObjVec( vFanins, p->pGia, pFanin, k )
        {
            Rnm_ObjAddToCount(p, pFanin);
            if ( Rnm_ObjIsJust(p, pFanin) )   // included in the abstraction
                Rnm_ObjAddToCount(p, pFanin); // count it second time!
            Vec_IntPush( p->vFanins, Gia_ObjId(p->pGia, pFanin) );

            // go through the fanins of the fanins
            vFanins2 = Ga2_ObjLeaves( p->pGia, pFanin );
            Gia_ManForEachObjVec( vFanins2, p->pGia, pFanin2, k2 )
            {
                Rnm_ObjAddToCount(p, pFanin2);
                if ( Rnm_ObjIsJust(p, pFanin2) )   // included in the abstraction
                    Rnm_ObjAddToCount(p, pFanin2); // count it second time!
                Vec_IntPush( p->vFanins, Gia_ObjId(p->pGia, pFanin2) );
            }
        }
    }

    // (3) select objects with reconvergence, which create potential constraints
    // - flop objects - yes
    // - objects whose fanin (or fanins' fanin) belongs to the justified area - yes
    // - objects whose fanins (or fanins' fanin) overlap - yes
    // (these do not guantee reconvergence, but may potentially have it)
    // (other objects cannot have reconvergence, even if they are added)
    vNewPPis = Vec_IntAlloc( 100 );
    Gia_ManForEachObjVec( vOldPPis, p->pGia, pObj, i )
    {
        if ( Gia_ObjIsRo(p->pGia, pObj) )
        {
            if ( fVerbose )
                Counters[0]++;
            Vec_IntPush( vNewPPis, Gia_ObjId(p->pGia, pObj) );
            continue;
        } 
        // go through the first fanins
        vFanins = Ga2_ObjLeaves( p->pGia, pObj );
        Gia_ManForEachObjVec( vFanins, p->pGia, pFanin, k )
        {
            if ( Rnm_ObjCount(p, pFanin) > 1 )
                Vec_IntPush( vNewPPis, Gia_ObjId(p->pGia, pObj) );
            continue;

            // go through the fanins of the fanins
            vFanins2 = Ga2_ObjLeaves( p->pGia, pFanin );
            Gia_ManForEachObjVec( vFanins2, p->pGia, pFanin2, k2 )
            {
                if ( Rnm_ObjCount(p, pFanin2) > 1 )
                {
//                    Vec_IntPush( vNewPPis, Gia_ObjId(p->pGia, pFanin) );
                    Vec_IntPush( vNewPPis, Gia_ObjId(p->pGia, pObj) );
                }
            }
        }
    }
    RetValue = Vec_IntUniqify( vNewPPis );
//    assert( RetValue == 0 ); // we will have duplicated entries here!

    // (4) clear fanin counters
    // this is important for counters to be correctly set in the future iterations -- see step (0)
    Gia_ManForEachObjVec( p->vFanins, p->pGia, pObj, i )
        Rnm_ObjSetCount( p, pObj, 0 );

    // visualize
    if ( fVerbose )
        printf( "*** Refinement %3d : PI+PPI =%4d. Old =%4d. New =%4d.   FF =%4d. Just =%4d. Shared =%4d.\n", 
            p->nRefId, Vec_IntSize(p->vMap), Vec_IntSize(vOldPPis), Vec_IntSize(vNewPPis), Counters[0], Counters[1], Counters[2] );

//    Rnm_ManPrintSelected( p, vNewPPis );
//    Ga2_StructAnalize( p->pGia, p->vMap, p->vObjs, vNewPPis );
    return vNewPPis;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

