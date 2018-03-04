/**CFile****************************************************************

  FileName    [covTest.c]

  SystemName  [ABC: Logic synthesis and verification system.]
 
  PackageName [Mapping into network of SOPs/ESOPs.]

  Synopsis    [Testing procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: covTest.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cov.h"

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
Min_Cube_t * Abc_NodeDeriveCoverPro( Min_Man_t * p, Min_Cube_t * pCover0, Min_Cube_t * pCover1 )
{
    Min_Cube_t * pCover;
    Min_Cube_t * pCube0, * pCube1, * pCube;
    if ( pCover0 == NULL || pCover1 == NULL )
        return NULL;
    // clean storage
    Min_ManClean( p, p->nVars );
    // go through the cube pairs
    Min_CoverForEachCube( pCover0, pCube0 )
    Min_CoverForEachCube( pCover1, pCube1 )
    {
        if ( Min_CubesDisjoint( pCube0, pCube1 ) )
            continue;
        pCube = Min_CubesProduct( p, pCube0, pCube1 );
        // add the cube to storage
        Min_SopAddCube( p, pCube );
    }
    Min_SopMinimize( p );
    pCover = Min_CoverCollect( p, p->nVars );
    assert( p->nCubes == Min_CoverCountCubes(pCover) );
    return pCover;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Min_Cube_t * Abc_NodeDeriveCoverSum( Min_Man_t * p, Min_Cube_t * pCover0, Min_Cube_t * pCover1 )
{
    Min_Cube_t * pCover;
    Min_Cube_t * pThis, * pCube;
    if ( pCover0 == NULL || pCover1 == NULL )
        return NULL;
    // clean storage
    Min_ManClean( p, p->nVars );
    // add the cubes to storage
    Min_CoverForEachCube( pCover0, pThis )
    {
        pCube = Min_CubeDup( p, pThis );
        Min_SopAddCube( p, pCube );
    }
    Min_CoverForEachCube( pCover1, pThis )
    {
        pCube = Min_CubeDup( p, pThis );
        Min_SopAddCube( p, pCube );
    }
    Min_SopMinimize( p );
    pCover = Min_CoverCollect( p, p->nVars );
    assert( p->nCubes == Min_CoverCountCubes(pCover) );
    return pCover;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeDeriveSops( Min_Man_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vSupp, Vec_Ptr_t * vNodes )
{
    Min_Cube_t * pCov0[2], * pCov1[2];
    Min_Cube_t * pCoverP, * pCoverN;
    Abc_Obj_t * pObj;
    int i, nCubes, fCompl0, fCompl1;

    // set elementary vars
    Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, i )
    {
        pObj->pCopy = (Abc_Obj_t *)Min_CubeAllocVar( p, i, 0 );
        pObj->pNext = (Abc_Obj_t *)Min_CubeAllocVar( p, i, 1 );
    }

    // get the cover for each node in the array
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        // get the complements
        fCompl0 = Abc_ObjFaninC0(pObj);
        fCompl1 = Abc_ObjFaninC1(pObj);
        // get the covers
        pCov0[0] = (Min_Cube_t *)Abc_ObjFanin0(pObj)->pCopy;
        pCov0[1] = (Min_Cube_t *)Abc_ObjFanin0(pObj)->pNext;
        pCov1[0] = (Min_Cube_t *)Abc_ObjFanin1(pObj)->pCopy;
        pCov1[1] = (Min_Cube_t *)Abc_ObjFanin1(pObj)->pNext;
        // compute the covers
        pCoverP = Abc_NodeDeriveCoverPro( p, pCov0[ fCompl0], pCov1[ fCompl1] );
        pCoverN = Abc_NodeDeriveCoverSum( p, pCov0[!fCompl0], pCov1[!fCompl1] );
        // set the covers
        pObj->pCopy = (Abc_Obj_t *)pCoverP;
        pObj->pNext = (Abc_Obj_t *)pCoverN;
    }

     nCubes = ABC_MIN( Min_CoverCountCubes(pCoverN), Min_CoverCountCubes(pCoverP) );

/*
printf( "\n\n" );
Min_CoverWrite( stdout, pCoverP );
printf( "\n\n" );
Min_CoverWrite( stdout, pCoverN );
*/

//    printf( "\n" );
//    Min_CoverWrite( stdout, pCoverP );

//    Min_CoverExpand( p, pCoverP );
//    Min_SopMinimize( p );
//    pCoverP = Min_CoverCollect( p, p->nVars );

//    printf( "\n" );
//    Min_CoverWrite( stdout, pCoverP );

//    nCubes = Min_CoverCountCubes(pCoverP);

    // clean the copy fields
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->pCopy = pObj->pNext = NULL;
    Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, i )
        pObj->pCopy = pObj->pNext = NULL;

//    Min_CoverWriteFile( pCoverP, Abc_ObjName(pRoot), 0 );
//    printf( "\n" );
//    Min_CoverWrite( stdout, pCoverP );

//    printf( "\n" );
//    Min_CoverWrite( stdout, pCoverP );
//    printf( "\n" );
//    Min_CoverWrite( stdout, pCoverN );
    return nCubes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTestSop( Abc_Ntk_t * pNtk )
{
    Min_Man_t * p;
    Vec_Ptr_t * vSupp, * vNodes;
    Abc_Obj_t * pObj;
    int i, nCubes;
    assert( Abc_NtkIsStrash(pNtk) );

    Abc_NtkCleanCopy(pNtk);
    Abc_NtkCleanNext(pNtk);
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsNode(Abc_ObjFanin0(pObj)) )
        {
            printf( "%-20s :  Trivial.\n", Abc_ObjName(pObj) );
            continue;
        }

        vSupp  = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        vNodes = Abc_NtkDfsNodes( pNtk, &pObj, 1 );

        printf( "%20s :  Cone = %5d.  Supp = %5d.  ", 
            Abc_ObjName(pObj), vNodes->nSize, vSupp->nSize );
//        if ( vSupp->nSize <= 128 )
        {
            p = Min_ManAlloc( vSupp->nSize );
            nCubes = Abc_NodeDeriveSops( p, pObj, vSupp, vNodes );
            printf( "Cubes = %5d.  ", nCubes );
            Min_ManFree( p );
        }
        printf( "\n" );


        Vec_PtrFree( vNodes );
        Vec_PtrFree( vSupp );
    }
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Min_Cube_t * Abc_NodeDeriveCover( Min_Man_t * p, Min_Cube_t * pCov0, Min_Cube_t * pCov1, int fComp0, int fComp1 )
{
    Min_Cube_t * pCover0, * pCover1, * pCover;
    Min_Cube_t * pCube0, * pCube1, * pCube;

    // complement the first if needed
    if ( !fComp0 )
        pCover0 = pCov0;
    else if ( pCov0 && pCov0->nLits == 0 ) // topmost one is the tautology cube
        pCover0 = pCov0->pNext;
    else
        pCover0 = p->pOne0, p->pOne0->pNext = pCov0;

    // complement the second if needed
    if ( !fComp1 )
        pCover1 = pCov1;
    else if ( pCov1 && pCov1->nLits == 0 ) // topmost one is the tautology cube
        pCover1 = pCov1->pNext;
    else
        pCover1 = p->pOne1, p->pOne1->pNext = pCov1;

    if ( pCover0 == NULL || pCover1 == NULL )
        return NULL;

    // clean storage
    Min_ManClean( p, p->nVars );
    // go through the cube pairs
    Min_CoverForEachCube( pCover0, pCube0 )
    Min_CoverForEachCube( pCover1, pCube1 )
    {
        if ( Min_CubesDisjoint( pCube0, pCube1 ) )
            continue;
        pCube = Min_CubesProduct( p, pCube0, pCube1 );
        // add the cube to storage
        Min_EsopAddCube( p, pCube );
    }
 
    if ( p->nCubes > 10 )
    {
//        printf( "(%d,", p->nCubes );
        Min_EsopMinimize( p );
//        printf( "%d) ", p->nCubes );
    }

    pCover = Min_CoverCollect( p, p->nVars );
    assert( p->nCubes == Min_CoverCountCubes(pCover) );

//    if ( p->nCubes > 1000 )
//        printf( "%d ", p->nCubes );
    return pCover;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeDeriveEsops( Min_Man_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vSupp, Vec_Ptr_t * vNodes )
{
    Min_Cube_t * pCover, * pCube;
    Abc_Obj_t * pObj;
    int i;

    // set elementary vars
    Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Min_CubeAllocVar( p, i, 0 );

    // get the cover for each node in the array
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        pCover = Abc_NodeDeriveCover( p,  
            (Min_Cube_t *)Abc_ObjFanin0(pObj)->pCopy,  
            (Min_Cube_t *)Abc_ObjFanin1(pObj)->pCopy,
            Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj) );
        pObj->pCopy = (Abc_Obj_t *)pCover;
        if ( p->nCubes > 3000 )
            return -1;
    }

    // add complement if needed
    if ( Abc_ObjFaninC0(pRoot) )
    {
        if ( pCover && pCover->nLits == 0 ) // topmost one is the tautology cube
        {
            pCube = pCover;
            pCover = pCover->pNext;
            Min_CubeRecycle( p, pCube );
            p->nCubes--;
        }
        else
        {
            pCube = Min_CubeAlloc( p );
            pCube->pNext = pCover;
            p->nCubes++;
        }
    }
/*
    Min_CoverExpand( p, pCover );
    Min_EsopMinimize( p );
    pCover = Min_CoverCollect( p, p->nVars );
*/
    // clean the copy fields
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->pCopy = NULL;
    Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, i )
        pObj->pCopy = NULL;

//    Min_CoverWriteFile( pCover, Abc_ObjName(pRoot), 1 );
//    Min_CoverWrite( stdout, pCover );
    return p->nCubes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkTestEsop( Abc_Ntk_t * pNtk )
{
    Min_Man_t * p;
    Vec_Ptr_t * vSupp, * vNodes;
    Abc_Obj_t * pObj;
    int i, nCubes;
    assert( Abc_NtkIsStrash(pNtk) );

    Abc_NtkCleanCopy(pNtk);
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        if ( !Abc_ObjIsNode(Abc_ObjFanin0(pObj)) )
        {
            printf( "%-20s :  Trivial.\n", Abc_ObjName(pObj) );
            continue;
        }

        vSupp  = Abc_NtkNodeSupport( pNtk, &pObj, 1 );
        vNodes = Abc_NtkDfsNodes( pNtk, &pObj, 1 );

        printf( "%20s :  Cone = %5d.  Supp = %5d.  ", 
            Abc_ObjName(pObj), vNodes->nSize, vSupp->nSize );
//        if ( vSupp->nSize <= 128 )
        {
            p = Min_ManAlloc( vSupp->nSize );
            nCubes = Abc_NodeDeriveEsops( p, pObj, vSupp, vNodes );
            printf( "Cubes = %5d.  ", nCubes );
            Min_ManFree( p );
        }
        printf( "\n" );


        Vec_PtrFree( vNodes );
        Vec_PtrFree( vSupp );
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

