/**CFile****************************************************************

  FileName    [ioReadBblif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to read AIG in the binary format.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioReadBblif.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"
#include "bool/dec/dec.h"
#include "misc/bbl/bblif.h"

ABC_NAMESPACE_IMPL_START


// For description of Binary BLIF format, refer to "abc/src/aig/bbl/bblif.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Fnction*************************************************************

  Synopsis    [Constructs ABC network from the manager.]

  Description [The ABC network is started, as well as the array vCopy,
  which will map the new ID of each object in the BBLIF manager into
  the ponter ot the corresponding object in the ABC. For each internal
  node, determined by Bbl_ObjIsLut(), the SOP representation is created
  by retrieving the SOP representation of the BBLIF object. Finally,
  the objects are connected using fanin/fanout creation, and the dummy
  names are assigned because ABC requires each CI/CO to have a name.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Bbl_ManToAbc( Bbl_Man_t * p )
{
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObjNew = NULL;
    Bbl_Obj_t * pObj, * pFanin;
    Vec_Ptr_t * vCopy;
    // start the network
    pNtk = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    pNtk->pName = Extra_UtilStrsav( Bbl_ManName(p) );
    // create objects
    vCopy = Vec_PtrStart( 1000 );
    Bbl_ManForEachObj( p, pObj )
    {
        if ( Bbl_ObjIsInput(pObj) )
            pObjNew = Abc_NtkCreatePi( pNtk );
        else if ( Bbl_ObjIsOutput(pObj) )
            pObjNew = Abc_NtkCreatePo( pNtk );
        else if ( Bbl_ObjIsLut(pObj) )
            pObjNew = Abc_NtkCreateNode( pNtk );
        else assert( 0 );
        if ( Bbl_ObjIsLut(pObj) )
            pObjNew->pData = Abc_SopRegister( (Mem_Flex_t *)pNtk->pManFunc, Bbl_ObjSop(p, pObj) );
        Vec_PtrSetEntry( vCopy, Bbl_ObjId(pObj), pObjNew );
    }
    // connect objects
    Bbl_ManForEachObj( p, pObj )
        Bbl_ObjForEachFanin( pObj, pFanin )
            Abc_ObjAddFanin( (Abc_Obj_t *)Vec_PtrEntry(vCopy, Bbl_ObjId(pObj)), (Abc_Obj_t *)Vec_PtrEntry(vCopy, Bbl_ObjId(pFanin)) );
    // finalize
    Vec_PtrFree( vCopy );
    Abc_NtkAddDummyPiNames( pNtk );
    Abc_NtkAddDummyPoNames( pNtk );
    if ( !Abc_NtkCheck( pNtk ) )
        printf( "Bbl_ManToAbc(): Network check has failed.\n" );
    return pNtk;
}

/**Fnction*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManDfs_rec( Bbl_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    extern void Bbl_ObjMark( Bbl_Obj_t * p );
    extern int  Bbl_ObjIsMarked( Bbl_Obj_t * p );
    Bbl_Obj_t * pFanin;
    if ( Bbl_ObjIsMarked(pObj) || Bbl_ObjIsInput(pObj) )
        return;
    Bbl_ObjForEachFanin( pObj, pFanin )
        Bbl_ManDfs_rec( pFanin, vNodes );
    assert( !Bbl_ObjIsMarked(pObj) ); // checks if acyclic
    Bbl_ObjMark( pObj );
    Vec_PtrPush( vNodes, pObj );
}

/**Fnction*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Bbl_ManDfs( Bbl_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Bbl_Obj_t * pObj;
    vNodes = Vec_PtrAlloc( 1000 );
    Bbl_ManForEachObj( p, pObj )
        if ( Bbl_ObjIsLut(pObj) )
            Bbl_ManDfs_rec( pObj, vNodes );
    return vNodes;
}

/**Fnction*************************************************************

  Synopsis    [Constructs AIG in ABC from the manager.]

  Description [The ABC network is started, as well as the array vCopy,
  which will map the new ID of each object in the BBLIF manager into
  the ponter ot the corresponding AIG object in the ABC. For each internal
  node in a topological oder the AIG representation is created
  by factoring the SOP representation of the BBLIF object. Finally,
  the CO objects are created, and the dummy names are assigned because 
  ABC requires each CI/CO to have a name.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Bbl_ManToAig( Bbl_Man_t * p )
{
    extern int Bbl_ManFncSize( Bbl_Man_t * p );
    extern int Bbl_ObjFncHandle( Bbl_Obj_t * p );
    extern Abc_Obj_t * Dec_GraphToAig( Abc_Ntk_t * pNtk, Dec_Graph_t * pFForm, Vec_Ptr_t * vFaninAigs );
    int fVerbose = 0;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObjNew = NULL;
    Bbl_Obj_t * pObj, * pFanin;
    Vec_Ptr_t * vCopy, * vNodes, * vFaninAigs;
    Dec_Graph_t ** pFForms;
    int i;
    abctime clk;
clk = Abc_Clock();
    // map SOP handles into factored forms
    pFForms = ABC_CALLOC( Dec_Graph_t *, Bbl_ManFncSize(p) );
    Bbl_ManForEachObj( p, pObj )
        if ( pFForms[Bbl_ObjFncHandle(pObj)] == NULL )
            pFForms[Bbl_ObjFncHandle(pObj)] = Dec_Factor( Bbl_ObjSop(p, pObj) );
if ( fVerbose )
ABC_PRT( "Fct", Abc_Clock() - clk );
    // start the network
    pNtk = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pNtk->pName = Extra_UtilStrsav( Bbl_ManName(p) );
    vCopy = Vec_PtrStart( 1000 );
    // create CIs
    Bbl_ManForEachObj( p, pObj )
    {
        if ( !Bbl_ObjIsInput(pObj) )
            continue;
        Vec_PtrSetEntry( vCopy, Bbl_ObjId(pObj), Abc_NtkCreatePi(pNtk) );
    }
clk = Abc_Clock();
    // create internal nodes
    vNodes = Bbl_ManDfs( p );
    vFaninAigs = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Bbl_Obj_t *, vNodes, pObj, i )
    {
        // collect fanin AIGs
        Vec_PtrClear( vFaninAigs );
        Bbl_ObjForEachFanin( pObj, pFanin )
            Vec_PtrPush( vFaninAigs, Vec_PtrEntry( vCopy, Bbl_ObjId(pFanin) ) );
        // create the new node
        pObjNew = Dec_GraphToAig( pNtk, pFForms[Bbl_ObjFncHandle(pObj)], vFaninAigs );
        Vec_PtrSetEntry( vCopy, Bbl_ObjId(pObj), pObjNew );
    }
    Vec_PtrFree( vFaninAigs );
    Vec_PtrFree( vNodes );
if ( fVerbose )
ABC_PRT( "AIG", Abc_Clock() - clk );
    // create COs
    Bbl_ManForEachObj( p, pObj )
    {
        if ( !Bbl_ObjIsOutput(pObj) )
            continue;
        pObjNew = (Abc_Obj_t *)Vec_PtrEntry( vCopy, Bbl_ObjId(Bbl_ObjFaninFirst(pObj)) );
        Abc_ObjAddFanin( Abc_NtkCreatePo(pNtk), pObjNew );
    }
    Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );
    // clear factored forms
    for ( i = Bbl_ManFncSize(p) - 1; i >= 0; i-- )
        if ( pFForms[i] )
            Dec_GraphFree( pFForms[i] );
    ABC_FREE( pFForms );
    // finalize
clk = Abc_Clock();
    Vec_PtrFree( vCopy );
    Abc_NtkAddDummyPiNames( pNtk );
    Abc_NtkAddDummyPoNames( pNtk );
if ( fVerbose )
ABC_PRT( "Nam", Abc_Clock() - clk );
//    if ( !Abc_NtkCheck( pNtk ) )
//        printf( "Bbl_ManToAig(): Network check has failed.\n" );
    return pNtk;
}

/**Fnction*************************************************************

  Synopsis    [Verifies equivalence for two combinational networks.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManVerify( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 )
{
    extern void Abc_NtkCecFraig( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int nSeconds, int fVerbose );
    Abc_Ntk_t * pAig1, * pAig2;
    pAig1 = Abc_NtkStrash( pNtk1, 0, 1, 0 );
    pAig2 = Abc_NtkStrash( pNtk2, 0, 1, 0 );
    Abc_NtkShortNames( pAig1 );
    Abc_NtkShortNames( pAig2 );
    Abc_NtkCecFraig( pAig1, pAig2, 0, 0 );
    Abc_NtkDelete( pAig1 );
    Abc_NtkDelete( pAig2 );
}

/**Fnction*************************************************************

  Synopsis    [Performs testing of the new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bbl_ManTest( Abc_Ntk_t * pNtk )
{
    extern Bbl_Man_t * Bbl_ManFromAbc( Abc_Ntk_t * pNtk );

    Abc_Ntk_t * pNtkNew;
    Bbl_Man_t * p, * pNew;
    char * pFileName = "test.bblif";
    abctime clk, clk1, clk2, clk3, clk4, clk5;
clk = Abc_Clock();
    p = Bbl_ManFromAbc( pNtk );
    Bbl_ManPrintStats( p );
clk1 = Abc_Clock() - clk;
//Bbl_ManDumpBlif( p, "test_bbl.blif" );

    // write into file and back
clk = Abc_Clock();
    Bbl_ManDumpBinaryBlif( p, pFileName );
clk2 = Abc_Clock() - clk;

    // read from file
clk = Abc_Clock();
    pNew = Bbl_ManReadBinaryBlif( pFileName );
    Bbl_ManStop( p ); p = pNew;
clk3 = Abc_Clock() - clk;

    // generate ABC network
clk = Abc_Clock();
    pNtkNew = Bbl_ManToAig( p );
//    pNtkNew = Bbl_ManToAbc( p );
    Bbl_ManStop( p );
clk4 = Abc_Clock() - clk;

    // equivalence check
clk = Abc_Clock();
//    Bbl_ManVerify( pNtk, pNtkNew );
    Abc_NtkDelete( pNtkNew );
clk5 = Abc_Clock() - clk;

printf( "Runtime stats:\n" );
ABC_PRT( "ABC to Man", clk1 );
ABC_PRT( "Writing   ", clk2 );
ABC_PRT( "Reading   ", clk3 );
ABC_PRT( "Man to ABC", clk4 );
ABC_PRT( "Verify    ", clk5 );
}

/**Function*************************************************************

  Synopsis    [Reads the AIG in the binary format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadBblif( char * pFileName, int fCheck )
{
    Bbl_Man_t * p;
    Abc_Ntk_t * pNtkNew;
    // read the file
    p = Bbl_ManReadBinaryBlif( pFileName );
    pNtkNew = Bbl_ManToAig( p );
    Bbl_ManStop( p );
    // check the result
    if ( fCheck && !Abc_NtkCheckRead( pNtkNew ) )
    {
        printf( "Io_ReadBaf: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

