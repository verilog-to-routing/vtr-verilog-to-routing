/**CFile****************************************************************

  FileName    [abcOrder.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Exploring static BDD variable orders.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcOrder.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Abc_NtkChangeCiOrder( Abc_Ntk_t * pNtk, Vec_Ptr_t * vSupp, int fReverse );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Changes the order of primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFindCiOrder( Abc_Ntk_t * pNtk, int fReverse, int fVerbose )
{
    Vec_Ptr_t * vSupp;
    vSupp = Abc_NtkSupport( pNtk );
    Abc_NtkChangeCiOrder( pNtk, vSupp, fReverse );
    Vec_PtrFree( vSupp );
}

/**Function*************************************************************

  Synopsis    [Implements the given variable order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkImplementCiOrder( Abc_Ntk_t * pNtk, char * pFileName, int fReverse, int fVerbose )
{
    char Buffer[1000];
    FILE * pFile;
    Vec_Ptr_t * vSupp;
    Abc_Obj_t * pObj;
    pFile = fopen( pFileName, "r" );
    vSupp = Vec_PtrAlloc( Abc_NtkCiNum(pNtk) );
    while ( fscanf( pFile, "%s", Buffer ) == 1 )
    {
        pObj = Abc_NtkFindCi( pNtk, Buffer );
        if ( pObj == NULL || !Abc_ObjIsCi(pObj) )
        {
            printf( "Name \"%s\" is not a PI name. Cannot use this order.\n", Buffer );
            Vec_PtrFree( vSupp );
            fclose( pFile );
            return;
        }
        Vec_PtrPush( vSupp, pObj );
    }
    fclose( pFile );
    if ( Vec_PtrSize(vSupp) != Abc_NtkCiNum(pNtk) )
    {
        printf( "The number of names in the order (%d) is not the same as the number of PIs (%d).\n", Vec_PtrSize(vSupp), Abc_NtkCiNum(pNtk) );
        Vec_PtrFree( vSupp );
        return;
    }
    Abc_NtkChangeCiOrder( pNtk, vSupp, fReverse );
    Vec_PtrFree( vSupp );
}
 
/**Function*************************************************************

  Synopsis    [Changes the order of primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkChangeCiOrder( Abc_Ntk_t * pNtk, Vec_Ptr_t * vSupp, int fReverse )
{
    Abc_Obj_t * pObj;
    int i;
    assert( Vec_PtrSize(vSupp) == Abc_NtkCiNum(pNtk) );
    // order CIs using the array
    if ( fReverse )
        Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, i )
            Vec_PtrWriteEntry( pNtk->vCis, Vec_PtrSize(vSupp)-1-i, pObj );
    else
        Vec_PtrForEachEntry( Abc_Obj_t *, vSupp, pObj, i )
            Vec_PtrWriteEntry( pNtk->vCis, i, pObj );
    // order PIs accordingly
    Vec_PtrClear( pNtk->vPis );
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjIsPi(pObj) )
            Vec_PtrPush( pNtk->vPis, pObj );
//    Abc_NtkForEachCi( pNtk, pObj, i )
//        printf( "%s ", Abc_ObjName(pObj) );
//    printf( "\n" );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

