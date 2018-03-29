/**CFile****************************************************************

  FileName    [covBuild.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Mapping into network of SOPs/ESOPs.]

  Synopsis    [Network construction procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: covBuild.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Derives the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCovDeriveCube( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, Min_Cube_t * pCube, Vec_Int_t * vSupp, int fCompl )
{
    Vec_Int_t * vLits;
    Abc_Obj_t * pNodeNew, * pFanin;
    int i, iFanin, Lit;
    // create empty cube
    if ( pCube->nLits == 0 )
    {
        if ( fCompl )
            return Abc_NtkCreateNodeConst0(pNtkNew);
        return Abc_NtkCreateNodeConst1(pNtkNew);
    }
    // get the literals of this cube
    vLits = Vec_IntAlloc( 10 );
    Min_CubeGetLits( pCube, vLits );
    assert( pCube->nLits == (unsigned)vLits->nSize );
    // create special case when there is only one literal
    if ( pCube->nLits == 1 )
    {
        iFanin = Vec_IntEntry(vLits,0);
        pFanin = Abc_NtkObj( pObj->pNtk, Vec_IntEntry(vSupp, iFanin) );
        Lit = Min_CubeGetVar(pCube, iFanin);
        assert( Lit == 1 || Lit == 2 );
        Vec_IntFree( vLits );
        if ( (Lit == 1) ^ fCompl )// negative
            return Abc_NtkCreateNodeInv( pNtkNew, pFanin->pCopy );
        return pFanin->pCopy;
    }
    assert( pCube->nLits > 1 );
    // create the AND cube
    pNodeNew = Abc_NtkCreateNode( pNtkNew );
    for ( i = 0; i < vLits->nSize; i++ )
    {
        iFanin = Vec_IntEntry(vLits,i);
        pFanin = Abc_NtkObj( pObj->pNtk, Vec_IntEntry(vSupp, iFanin) );
        Lit = Min_CubeGetVar(pCube, iFanin);
        assert( Lit == 1 || Lit == 2 );
        Vec_IntWriteEntry( vLits, i, Lit==1 );
        Abc_ObjAddFanin( pNodeNew, pFanin->pCopy );
    }
    pNodeNew->pData = Abc_SopCreateAnd( (Mem_Flex_t *)pNtkNew->pManFunc, vLits->nSize, vLits->pArray );
    if ( fCompl )
        Abc_SopComplement( (char *)pNodeNew->pData );
    Vec_IntFree( vLits );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Derives the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCovDeriveNode_rec( Cov_Man_t * p, Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, int Level )
{
    Min_Cube_t * pCover = NULL, * pCube;
    Abc_Obj_t * pFaninNew, * pNodeNew, * pFanin;
    Vec_Int_t * vSupp;
    int Entry, nCubes, i;

    if ( Abc_ObjIsCi(pObj) )
        return pObj->pCopy;
    assert( Abc_ObjIsNode(pObj) );
    // skip if already computed
    if ( pObj->pCopy )
        return pObj->pCopy;

    // get the support and the cover
    vSupp  = Abc_ObjGetSupp( pObj ); 
    pCover = Abc_ObjGetCover2( pObj );
    assert( vSupp );
/*
    if ( pCover &&  pCover->nVars - Min_CoverSuppVarNum(p->pManMin, pCover) > 0 )
    {
        printf( "%d\n ", pCover->nVars - Min_CoverSuppVarNum(p->pManMin, pCover) );
        Min_CoverWrite( stdout, pCover );
    }
*/
/*
    // print the support of this node
    printf( "{ " );
    Vec_IntForEachEntry( vSupp, Entry, i )
        printf( "%d ", Entry );
    printf( "}  cubes = %d\n", Min_CoverCountCubes( pCover ) );
*/
    // process the fanins
    Vec_IntForEachEntry( vSupp, Entry, i )
    {
        pFanin = Abc_NtkObj(pObj->pNtk, Entry);
        Abc_NtkCovDeriveNode_rec( p, pNtkNew, pFanin, Level+1 );
    }

    // for each cube, construct the node
    nCubes = Min_CoverCountCubes( pCover );
    if ( nCubes == 0 )
        pNodeNew = Abc_NtkCreateNodeConst0(pNtkNew);
    else if ( nCubes == 1 )
        pNodeNew = Abc_NtkCovDeriveCube( pNtkNew, pObj, pCover, vSupp, 0 );
    else
    {
        pNodeNew = Abc_NtkCreateNode( pNtkNew );
        Min_CoverForEachCube( pCover, pCube )
        {
            pFaninNew = Abc_NtkCovDeriveCube( pNtkNew, pObj, pCube, vSupp, 0 );
            Abc_ObjAddFanin( pNodeNew, pFaninNew );
        }
        pNodeNew->pData = Abc_SopCreateXorSpecial( (Mem_Flex_t *)pNtkNew->pManFunc, nCubes );
    }
/*
    printf( "Created node %d(%d) at level %d: ", pNodeNew->Id, pObj->Id, Level );
    Vec_IntForEachEntry( vSupp, Entry, i )
    {
        pFanin = Abc_NtkObj(pObj->pNtk, Entry);
        printf( "%d(%d) ", pFanin->pCopy->Id, pFanin->Id );
    }
    printf( "\n" );
    Min_CoverWrite( stdout, pCover );
*/
    pObj->pCopy = pNodeNew;
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Derives the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCovDerive( Cov_Man_t * p, Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    // perform strashing
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    // reconstruct the network
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        Abc_NtkCovDeriveNode_rec( p, pNtkNew, Abc_ObjFanin0(pObj), 0 );
//        printf( "*** CO %s : %d -> %d \n", Abc_ObjName(pObj), pObj->pCopy->Id, Abc_ObjFanin0(pObj)->pCopy->Id );
    }
    // add the COs
    Abc_NtkFinalize( pNtk, pNtkNew );
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 1 );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkCovDerive: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}




/**Function*************************************************************

  Synopsis    [Derives the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCovDeriveInv( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, int fCompl )
{
    assert( pObj->pCopy );
    if ( !fCompl )
        return pObj->pCopy;
    if ( pObj->pCopy->pCopy == NULL )
        pObj->pCopy->pCopy = Abc_NtkCreateNodeInv( pNtkNew, pObj->pCopy );
    return pObj->pCopy->pCopy;
 }

/**Function*************************************************************

  Synopsis    [Derives the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCovDeriveCubeInv( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, Min_Cube_t * pCube, Vec_Int_t * vSupp )
{
    Vec_Int_t * vLits;
    Abc_Obj_t * pNodeNew, * pFanin;
    int i, iFanin, Lit;
    // create empty cube
    if ( pCube->nLits == 0 )
        return Abc_NtkCreateNodeConst1(pNtkNew);
    // get the literals of this cube
    vLits = Vec_IntAlloc( 10 );
    Min_CubeGetLits( pCube, vLits );
    assert( pCube->nLits == (unsigned)vLits->nSize );
    // create special case when there is only one literal
    if ( pCube->nLits == 1 )
    {
        iFanin = Vec_IntEntry(vLits,0);
        pFanin = Abc_NtkObj( pObj->pNtk, Vec_IntEntry(vSupp, iFanin) );
        Lit = Min_CubeGetVar(pCube, iFanin);
        assert( Lit == 1 || Lit == 2 );
        Vec_IntFree( vLits );
//        if ( Lit == 1 )// negative
//            return Abc_NtkCreateNodeInv( pNtkNew, pFanin->pCopy );
//        return pFanin->pCopy;
        return Abc_NtkCovDeriveInv( pNtkNew, pFanin, Lit==1 );
    }
    assert( pCube->nLits > 1 );
    // create the AND cube
    pNodeNew = Abc_NtkCreateNode( pNtkNew );
    for ( i = 0; i < vLits->nSize; i++ )
    {
        iFanin = Vec_IntEntry(vLits,i);
        pFanin = Abc_NtkObj( pObj->pNtk, Vec_IntEntry(vSupp, iFanin) );
        Lit = Min_CubeGetVar(pCube, iFanin);
        assert( Lit == 1 || Lit == 2 );
        Vec_IntWriteEntry( vLits, i, Lit==1 );
//        Abc_ObjAddFanin( pNodeNew, pFanin->pCopy );
        Abc_ObjAddFanin( pNodeNew, Abc_NtkCovDeriveInv( pNtkNew, pFanin, Lit==1 ) );
    }
//    pNodeNew->pData = Abc_SopCreateAnd( (Mem_Flex_t *)pNtkNew->pManFunc, vLits->nSize, vLits->pArray );
    pNodeNew->pData = Abc_SopCreateAnd( (Mem_Flex_t *)pNtkNew->pManFunc, vLits->nSize, NULL );
    Vec_IntFree( vLits );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Derives the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCovDeriveNodeInv_rec( Cov_Man_t * p, Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj, int fCompl )
{
    Min_Cube_t * pCover, * pCube;
    Abc_Obj_t * pFaninNew, * pNodeNew, * pFanin;
    Vec_Int_t * vSupp;
    int Entry, nCubes, i;

    // skip if already computed
    if ( pObj->pCopy )
        return Abc_NtkCovDeriveInv( pNtkNew, pObj, fCompl );
    assert( Abc_ObjIsNode(pObj) );

    // get the support and the cover
    vSupp  = Abc_ObjGetSupp( pObj ); 
    pCover = Abc_ObjGetCover2( pObj );
    assert( vSupp );

    // process the fanins
    Vec_IntForEachEntry( vSupp, Entry, i )
    {
        pFanin = Abc_NtkObj(pObj->pNtk, Entry);
        Abc_NtkCovDeriveNodeInv_rec( p, pNtkNew, pFanin, 0 );
    }

    // for each cube, construct the node
    nCubes = Min_CoverCountCubes( pCover );
    if ( nCubes == 0 )
        pNodeNew = Abc_NtkCreateNodeConst0(pNtkNew);
    else if ( nCubes == 1 )
        pNodeNew = Abc_NtkCovDeriveCubeInv( pNtkNew, pObj, pCover, vSupp );
    else
    {
        pNodeNew = Abc_NtkCreateNode( pNtkNew );
        Min_CoverForEachCube( pCover, pCube )
        {
            pFaninNew = Abc_NtkCovDeriveCubeInv( pNtkNew, pObj, pCube, vSupp );
            Abc_ObjAddFanin( pNodeNew, pFaninNew );
        }
        pNodeNew->pData = Abc_SopCreateXorSpecial( (Mem_Flex_t *)pNtkNew->pManFunc, nCubes );
    }

    pObj->pCopy = pNodeNew;
    return Abc_NtkCovDeriveInv( pNtkNew, pObj, fCompl );
}

/**Function*************************************************************

  Synopsis    [Derives the decomposed network.]

  Description [The resulting network contains only pure AND/OR/EXOR gates
  and inverters. This procedure is usedful to generate Verilog.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCovDeriveClean( Cov_Man_t * p, Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pNodeNew;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    // perform strashing
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    // reconstruct the network
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pNodeNew = Abc_NtkCovDeriveNodeInv_rec( p, pNtkNew, Abc_ObjFanin0(pObj), Abc_ObjFaninC0(pObj) );
        Abc_ObjAddFanin( pObj->pCopy, pNodeNew );
    }
    // add the COs
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkCovDeriveInv: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}



/**Function*************************************************************

  Synopsis    [Derives the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkCovDerive_rec( Cov_Man_t * p, Abc_Ntk_t * pNtkNew, Abc_Obj_t * pObj )
{
    int fVerbose = 0;
    Min_Cube_t * pCover, * pCovers[3];
    Abc_Obj_t * pNodeNew, * pFanin;
    Vec_Int_t * vSupp;
    Vec_Str_t * vCover;
    int i, Entry, nCubes, Type = 0;
    // skip if already computed
    if ( pObj->pCopy )
        return pObj->pCopy;
    assert( Abc_ObjIsNode(pObj) );
    // get the support and the cover
    vSupp  = Abc_ObjGetSupp( pObj ); 
    assert( vSupp );
    // choose the cover to implement
    pCovers[0] = Abc_ObjGetCover( pObj, 0 );
    pCovers[1] = Abc_ObjGetCover( pObj, 1 );
    pCovers[2] = Abc_ObjGetCover2( pObj );
    // use positive polarity
    if ( pCovers[0] 
        && (!pCovers[1] || Min_CoverCountCubes(pCovers[0]) <= Min_CoverCountCubes(pCovers[1]))
        && (!pCovers[2] || Min_CoverCountCubes(pCovers[0]) <= Min_CoverCountCubes(pCovers[2]))   )
    {
        pCover = pCovers[0];
        Type = '1';
    }
    else 
    // use negative polarity
    if ( pCovers[1] 
        && (!pCovers[0] || Min_CoverCountCubes(pCovers[1]) <= Min_CoverCountCubes(pCovers[0]))
        && (!pCovers[2] || Min_CoverCountCubes(pCovers[1]) <= Min_CoverCountCubes(pCovers[2]))   )
    {
        pCover = pCovers[1];
        Type = '0';
    }
    else 
    // use XOR polarity
    if ( pCovers[2] 
        && (!pCovers[0] || Min_CoverCountCubes(pCovers[2]) < Min_CoverCountCubes(pCovers[0]))
        && (!pCovers[1] || Min_CoverCountCubes(pCovers[2]) < Min_CoverCountCubes(pCovers[1]))   )
    {
        pCover = pCovers[2];
        Type = 'x';
    }
    else
        assert( 0 );
    // print the support of this node
    if ( fVerbose )
    {
        printf( "{ " );
        Vec_IntForEachEntry( vSupp, Entry, i )
            printf( "%d ", Entry );
        printf( "}  cubes = %d\n", Min_CoverCountCubes( pCover ) );
    }
    // process the fanins
    Vec_IntForEachEntry( vSupp, Entry, i )
    {
        pFanin = Abc_NtkObj(pObj->pNtk, Entry);
        Abc_NtkCovDerive_rec( p, pNtkNew, pFanin );
    }
    // for each cube, construct the node
    nCubes = Min_CoverCountCubes( pCover );
    if ( nCubes == 0 )
        pNodeNew = Abc_NtkCreateNodeConst0(pNtkNew);
    else if ( nCubes == 1 )
        pNodeNew = Abc_NtkCovDeriveCube( pNtkNew, pObj, pCover, vSupp, Type == '0' );
    else
    {
        // create the node
        pNodeNew = Abc_NtkCreateNode( pNtkNew );
        Vec_IntForEachEntry( vSupp, Entry, i )
        {
            pFanin = Abc_NtkObj(pObj->pNtk, Entry);
            Abc_ObjAddFanin( pNodeNew, pFanin->pCopy );
        }
        // derive the function
        vCover = Vec_StrAlloc( 100 );
        Min_CoverCreate( vCover, pCover, (char)Type );
        pNodeNew->pData = Abc_SopRegister((Mem_Flex_t *)pNtkNew->pManFunc, Vec_StrArray(vCover) );
        Vec_StrFree( vCover );
    }

/*
    printf( "Created node %d(%d) at level %d: ", pNodeNew->Id, pObj->Id, Level );
    Vec_IntForEachEntry( vSupp, Entry, i )
    {
        pFanin = Abc_NtkObj(pObj->pNtk, Entry);
        printf( "%d(%d) ", pFanin->pCopy->Id, pFanin->Id );
    }
    printf( "\n" );
    Min_CoverWrite( stdout, pCover );
*/
    return pObj->pCopy = pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Derives the decomposed network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCovDeriveRegular( Cov_Man_t * p, Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pNodeNew;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    // perform strashing
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    // reconstruct the network
    if ( Abc_ObjFanoutNum(Abc_AigConst1(pNtk)) > 0 )
        Abc_AigConst1(pNtk)->pCopy = Abc_NtkCreateNodeConst1(pNtkNew);
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        pNodeNew = Abc_NtkCovDerive_rec( p, pNtkNew, Abc_ObjFanin0(pObj) );
        if ( Abc_ObjFaninC0(pObj) )
        {
            if ( pNodeNew->pData && Abc_ObjFanoutNum(Abc_ObjFanin0(pObj)) == 1 )
                Abc_SopComplement( (char *)pNodeNew->pData );
            else
                pNodeNew = Abc_NtkCreateNodeInv( pNtkNew, pNodeNew );
        }
        Abc_ObjAddFanin( pObj->pCopy, pNodeNew );
    }
    // add the COs
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkCovDerive: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

