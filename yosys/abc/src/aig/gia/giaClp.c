/**CFile****************************************************************

  FileName    [giaClp.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Collapsing AIG.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#include "bdd/dsd/dsd.h"
#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

extern int Abc_ConvertZddToSop( DdManager * dd, DdNode * zCover, char * pSop, int nFanins, Vec_Str_t * vCube, int fPhase );
extern int Abc_CountZddCubes( DdManager * dd, DdNode * zCover );
extern int Abc_NtkDeriveFlatGiaSop( Gia_Man_t * pGia, int * gFanins, char * pSop );
extern int Gia_ManFactorNode( Gia_Man_t * p, char * pSop, Vec_Int_t * vLeaves );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManRebuildIsop( DdManager * dd, DdNode * bLocal, Gia_Man_t * pNew, Vec_Int_t * vFanins, Vec_Str_t * vSop, Vec_Str_t * vCube )
{
    char * pSop;
    DdNode * bCover, * zCover, * zCover0, * zCover1;
    int nFanins = Vec_IntSize(vFanins);
    int fPhase, nCubes, nCubes0, nCubes1;

    // get the ZDD of the negative polarity
    bCover = Cudd_zddIsop( dd, Cudd_Not(bLocal), Cudd_Not(bLocal), &zCover0 );
    Cudd_Ref( zCover0 );
    Cudd_Ref( bCover );
    Cudd_RecursiveDeref( dd, bCover );
    nCubes0 = Abc_CountZddCubes( dd, zCover0 );

    // get the ZDD of the positive polarity
    bCover = Cudd_zddIsop( dd, bLocal, bLocal, &zCover1 );
    Cudd_Ref( zCover1 );
    Cudd_Ref( bCover );
    Cudd_RecursiveDeref( dd, bCover );
    nCubes1 = Abc_CountZddCubes( dd, zCover1 );

    // compare the number of cubes
    if ( nCubes1 <= nCubes0 )
    { // use positive polarity
        nCubes = nCubes1;
        zCover = zCover1;
        Cudd_RecursiveDerefZdd( dd, zCover0 );
        fPhase = 1;
    }
    else
    { // use negative polarity
        nCubes = nCubes0;
        zCover = zCover0;
        Cudd_RecursiveDerefZdd( dd, zCover1 );
        fPhase = 0;
    }
    if ( nCubes > 1000 )
    {
        Cudd_RecursiveDerefZdd( dd, zCover );
        return -1;
    }

    // allocate memory for the cover
    Vec_StrGrow( vSop, (nFanins + 3) * nCubes + 1 );
    pSop = Vec_StrArray( vSop );
    pSop[(nFanins + 3) * nCubes] = 0;
    // create the SOP
    Vec_StrFill( vCube, nFanins, '-' );
    Vec_StrPush( vCube, '\0' );
    Abc_ConvertZddToSop( dd, zCover, pSop, nFanins, vCube, fPhase );
    Cudd_RecursiveDerefZdd( dd, zCover );

    // perform factoring
//    return Abc_NtkDeriveFlatGiaSop( pNew, Vec_IntArray(vFanins), pSop );
    return Gia_ManFactorNode( pNew, pSop, vFanins );
}
int Gia_ManRebuildNode( Dsd_Manager_t * pManDsd, Dsd_Node_t * pNodeDsd, Gia_Man_t * pNew, DdManager * ddNew, Vec_Int_t * vFanins, Vec_Str_t * vSop, Vec_Str_t * vCube )
{
    DdManager * ddDsd = Dsd_ManagerReadDd( pManDsd );
    DdNode * bLocal, * bTemp;
    Dsd_Node_t * pFaninDsd;
    Dsd_Type_t Type;
    int i, nDecs, iLit = -1;

    // add the fanins
    Type  = Dsd_NodeReadType( pNodeDsd );
    nDecs = Dsd_NodeReadDecsNum( pNodeDsd );
    assert( nDecs > 1 );
    Vec_IntClear( vFanins );
    for ( i = 0; i < nDecs; i++ )
    {
        pFaninDsd = Dsd_NodeReadDec( pNodeDsd, i );
        iLit      = Dsd_NodeReadMark( Dsd_Regular(pFaninDsd) );
        iLit      = Abc_LitNotCond( iLit, Dsd_IsComplement(pFaninDsd) );
        assert( Type == DSD_NODE_OR || !Dsd_IsComplement(pFaninDsd) );
        Vec_IntPush( vFanins, iLit );
    }

    // create the local function depending on the type of the node
    switch ( Type )
    {
        case DSD_NODE_CONST1:
        {
            iLit = 1;
            break;
        }
        case DSD_NODE_OR:
        {
            iLit = 0;
            for ( i = 0; i < nDecs; i++ )
                iLit = Gia_ManHashOr( pNew, iLit, Vec_IntEntry(vFanins, i) );
            break;
        }
        case DSD_NODE_EXOR:
        {
            iLit = 0;
            for ( i = 0; i < nDecs; i++ )
                iLit = Gia_ManHashXor( pNew, iLit, Vec_IntEntry(vFanins, i) );
            break;
        }
        case DSD_NODE_PRIME:
        {
            bLocal = Dsd_TreeGetPrimeFunction( ddDsd, pNodeDsd );                Cudd_Ref( bLocal );
            bLocal = Extra_TransferLevelByLevel( ddDsd, ddNew, bTemp = bLocal ); Cudd_Ref( bLocal );
            Cudd_RecursiveDeref( ddDsd, bTemp );
            // bLocal is now in the new BDD manager
            iLit = Gia_ManRebuildIsop( ddNew, bLocal, pNew, vFanins, vSop, vCube );
            Cudd_RecursiveDeref( ddNew, bLocal );
            break;
        }
        default:
        {
            assert( 0 );
            break;
        }
    }
    Dsd_NodeSetMark( pNodeDsd, iLit );
    return iLit;
}
Gia_Man_t * Gia_ManRebuild( Gia_Man_t * p, Dsd_Manager_t * pManDsd, DdManager * ddNew )
{
    Gia_Man_t * pNew;
    Dsd_Node_t ** ppNodesDsd;
    Dsd_Node_t * pNodeDsd;
    int i, nNodesDsd, iLit = -1;
    Vec_Str_t * vSop, * vCube;
    Vec_Int_t * vFanins;

    vFanins = Vec_IntAlloc( 1000 );
    vSop    = Vec_StrAlloc( 10000 );
    vCube   = Vec_StrAlloc( 1000 );

    pNew = Gia_ManStart( 2*Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );

    // save the CI nodes in the DSD nodes
    Dsd_NodeSetMark( Dsd_ManagerReadConst1(pManDsd), 1 );
    for ( i = 0; i < Gia_ManCiNum(p); i++ )
    {
        pNodeDsd = Dsd_ManagerReadInput( pManDsd, i );
        Dsd_NodeSetMark( pNodeDsd, Gia_ManAppendCi( pNew ) );
    }

    // collect DSD nodes in DFS order (leaves and const1 are not collected)
    ppNodesDsd = Dsd_TreeCollectNodesDfs( pManDsd, &nNodesDsd );
    for ( i = 0; i < nNodesDsd; i++ )
    {
        iLit = Gia_ManRebuildNode( pManDsd, ppNodesDsd[i], pNew, ddNew, vFanins, vSop, vCube );
        if ( iLit == -1 )
            break;
    }
    ABC_FREE( ppNodesDsd );
    Vec_IntFree( vFanins );
    Vec_StrFree( vSop );
    Vec_StrFree( vCube );
    if ( iLit == -1 )
    {
        Gia_ManStop( pNew );
        return Gia_ManDup(p);
    }

    // set the pointers to the CO drivers
    for ( i = 0; i < Gia_ManCoNum(p); i++ )
    {
        pNodeDsd = Dsd_ManagerReadRoot( pManDsd, i );
        iLit = Dsd_NodeReadMark( Dsd_Regular(pNodeDsd) );
        iLit = Abc_LitNotCond( iLit, Dsd_IsComplement(pNodeDsd) );
        Gia_ManAppendCo( pNew, iLit );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCollapseDeref( DdManager * dd, Vec_Ptr_t * vFuncs )
{
    DdNode * bFunc; int i;
    Vec_PtrForEachEntry( DdNode *, vFuncs, bFunc, i )
        if ( bFunc )
            Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrFree( vFuncs );
}
void Gia_ObjCollapseDeref( Gia_Man_t * p, DdManager * dd, Vec_Ptr_t * vFuncs, int Id )
{
    if ( Gia_ObjRefDecId(p, Id) )
        return;
    Cudd_RecursiveDeref( dd, (DdNode *)Vec_PtrEntry(vFuncs, Id) );
    Vec_PtrWriteEntry( vFuncs, Id, NULL );
}
Vec_Ptr_t * Gia_ManCollapse( Gia_Man_t * p, DdManager * dd, int nBddLimit, int fVerbose )
{
    Vec_Ptr_t * vFuncs;
    DdNode * bFunc0, * bFunc1, * bFunc;
    Gia_Obj_t * pObj;
    int i, Id;
    Gia_ManCreateRefs( p );
    // assign constant node
    vFuncs = Vec_PtrStart( Gia_ManObjNum(p) );
    if ( Gia_ObjRefNumId(p, 0) > 0 )
        Vec_PtrWriteEntry( vFuncs, 0, Cudd_ReadLogicZero(dd) ), Cudd_Ref(Cudd_ReadLogicZero(dd));
    // assign elementary variables
    Gia_ManForEachCiId( p, Id, i )
        if ( Gia_ObjRefNumId(p, Id) > 0 )
            Vec_PtrWriteEntry( vFuncs, Id, Cudd_bddIthVar(dd,i) ), Cudd_Ref(Cudd_bddIthVar(dd,i));
    // create BDD for AND nodes
    Gia_ManForEachAnd( p, pObj, i )
    {
        bFunc0 = Cudd_NotCond( (DdNode *)Vec_PtrEntry(vFuncs, Gia_ObjFaninId0(pObj, i)), Gia_ObjFaninC0(pObj) );
        bFunc1 = Cudd_NotCond( (DdNode *)Vec_PtrEntry(vFuncs, Gia_ObjFaninId1(pObj, i)), Gia_ObjFaninC1(pObj) );
        bFunc  = Cudd_bddAndLimit( dd, bFunc0, bFunc1, nBddLimit );  
        if ( bFunc == NULL )
        {
            Gia_ManCollapseDeref( dd, vFuncs );
            return NULL;
        }        
        Cudd_Ref( bFunc );
        Vec_PtrWriteEntry( vFuncs, i, bFunc );
        Gia_ObjCollapseDeref( p, dd, vFuncs, Gia_ObjFaninId0(pObj, i) );
        Gia_ObjCollapseDeref( p, dd, vFuncs, Gia_ObjFaninId1(pObj, i) );
    }
    // create BDD for outputs
    Gia_ManForEachCoId( p, Id, i )
    {
        pObj = Gia_ManCo( p, i );
        bFunc0 = Cudd_NotCond( (DdNode *)Vec_PtrEntry(vFuncs, Gia_ObjFaninId0(pObj, Id)), Gia_ObjFaninC0(pObj) );
        Vec_PtrWriteEntry( vFuncs, Id, bFunc0 ); Cudd_Ref( bFunc0 );
        Gia_ObjCollapseDeref( p, dd, vFuncs, Gia_ObjFaninId0(pObj, Id) );
    }
    assert( Vec_PtrSize(vFuncs) == Vec_PtrCountZero(vFuncs) + Gia_ManCoNum(p) );
    // compact
    Gia_ManForEachCoId( p, Id, i )
        Vec_PtrWriteEntry( vFuncs, i, Vec_PtrEntry(vFuncs, Id) );
    Vec_PtrShrink( vFuncs, Gia_ManCoNum(p) );
    return vFuncs;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManCollapseTest( Gia_Man_t * p, int fVerbose )
{
    Gia_Man_t * pNew;
    DdManager * dd, * ddNew;
    Dsd_Manager_t * pManDsd;
    Vec_Ptr_t * vFuncs;
    // derive global BDDs
    dd = Cudd_Init( Gia_ManCiNum(p), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    vFuncs = Gia_ManCollapse( p, dd, 10000, 0 );
    Cudd_AutodynDisable( dd );
    if ( vFuncs == NULL ) 
    {
        Extra_StopManager( dd );
        return Gia_ManDup(p);
    }
    // start ISOP manager
    ddNew = Cudd_Init( Gia_ManCiNum(p), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_zddVarsFromBddVars( ddNew, 2 );
//    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    if ( fVerbose )
    printf( "Ins = %d. Outs = %d.  Shared BDD nodes = %d.  Peak live nodes = %d. Peak nodes = %d.\n", 
        Gia_ManCiNum(p), Gia_ManCoNum(p), 
        Cudd_SharingSize( (DdNode **)Vec_PtrArray(vFuncs), Vec_PtrSize(vFuncs) ),
        Cudd_ReadPeakLiveNodeCount(dd), (int)Cudd_ReadNodeCount(dd) );
    // perform decomposition
    pManDsd = Dsd_ManagerStart( dd, Gia_ManCiNum(p), 0 );
    Dsd_Decompose( pManDsd, (DdNode **)Vec_PtrArray(vFuncs), Vec_PtrSize(vFuncs) );
    if ( fVerbose )
    {
        Vec_Ptr_t * vNamesCi = Gia_GetFakeNames( Gia_ManCiNum(p), 0 );
        Vec_Ptr_t * vNamesCo = Gia_GetFakeNames( Gia_ManCoNum(p), 1 );
        char ** ppNamesCi = (char **)Vec_PtrArray( vNamesCi );
        char ** ppNamesCo = (char **)Vec_PtrArray( vNamesCo );
        Dsd_TreePrint( stdout, pManDsd, ppNamesCi, ppNamesCo, 0, -1, 0 );
        Vec_PtrFreeFree( vNamesCi );
        Vec_PtrFreeFree( vNamesCo );
    }

    pNew = Gia_ManRebuild( p, pManDsd, ddNew );
    Dsd_ManagerStop( pManDsd );
    // return manager
    Gia_ManCollapseDeref( dd, vFuncs );
    Extra_StopManager( dd );
    Extra_StopManager( ddNew );
    return pNew;
}
void Gia_ManCollapseTestTest( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    pNew = Gia_ManCollapseTest( p, 0 );
    Gia_ManPrintStats( p, NULL );
    Gia_ManPrintStats( pNew, NULL );
    Gia_ManStop( pNew );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintDsdOne( Dsd_Manager_t * pManDsd, int Output, int OffSet )
{
    Vec_Str_t * vStr = Vec_StrAlloc( 100 );
    Dsd_TreePrint4( vStr, pManDsd, Output );
    for ( int i = 0; i < OffSet; i++ )
        printf( " " );
    printf( "Supp %2d  nDsd %2d  %s\n", Dsd_TreeSuppSize(pManDsd, Output), Dsd_TreeNonDsdMax(pManDsd, Output), Vec_StrArray(vStr) );
    Vec_StrFree( vStr );
    fflush( stdout );      
}
void Gia_ManPrintDsd( Dsd_Manager_t * pManDsd, int Output, int nOutputs, int OffSet )
{
    if ( Output == -1 )
    {
        for ( int i = 0; i < nOutputs; i++ )
            Gia_ManPrintDsdOne( pManDsd, i, OffSet );
    }
    else 
    {
        assert( Output >= 0 && Output < nOutputs );
        Gia_ManPrintDsdOne( pManDsd, Output, OffSet );
    }  
}

void Gia_ManCheckDsd( Gia_Man_t * p, int OffSet, int fVerbose )
{
    DdManager * dd;
    Dsd_Manager_t * pManDsd;
    Vec_Ptr_t * vFuncs;
    dd = Cudd_Init( Gia_ManCiNum(p), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    vFuncs = Gia_ManCollapse( p, dd, 10000, 0 );
    Cudd_AutodynDisable( dd );
    if ( vFuncs == NULL ) 
    {
        Extra_StopManager( dd );
        return;
    }
    pManDsd = Dsd_ManagerStart( dd, Gia_ManCiNum(p), 0 );
    if ( pManDsd == NULL )
    {
        Gia_ManCollapseDeref( dd, vFuncs );
        Cudd_Quit( dd );
        return;
    }
    Dsd_Decompose( pManDsd, (DdNode **)Vec_PtrArray(vFuncs), Vec_PtrSize(vFuncs) );
    
    if ( fVerbose )
    {
        Vec_Ptr_t * vNamesCi = Gia_GetFakeNames( Gia_ManCiNum(p), 0 );
        Vec_Ptr_t * vNamesCo = Gia_GetFakeNames( Gia_ManCoNum(p), 1 );
        char ** ppNamesCi = (char **)Vec_PtrArray( vNamesCi );
        char ** ppNamesCo = (char **)Vec_PtrArray( vNamesCo );
        Dsd_TreePrint( stdout, pManDsd, ppNamesCi, ppNamesCo, 0, -1, OffSet );
        Vec_PtrFreeFree( vNamesCi );
        Vec_PtrFreeFree( vNamesCo );
    }
    else
        Gia_ManPrintDsd( pManDsd, 0, Vec_PtrSize(vFuncs), 0 );

    Dsd_ManagerStop( pManDsd );
    Gia_ManCollapseDeref( dd, vFuncs );
    Extra_StopManager( dd );
}

Vec_Ptr_t * Gia_ManRecurDsdCof( DdManager * dd, Vec_Ptr_t * vFuncs, int iVar )
{
    Vec_Ptr_t * vNew = Vec_PtrAlloc( 2 * Vec_PtrSize(vFuncs) ); DdNode * bFunc; int i;
    Vec_PtrForEachEntry( DdNode *, vFuncs, bFunc, i ) {
        DdNode * bCof0 = Cudd_Cofactor( dd, bFunc, Cudd_Not(Cudd_bddIthVar(dd, iVar)) ); Cudd_Ref( bCof0 );
        DdNode * bCof1 = Cudd_Cofactor( dd, bFunc, Cudd_bddIthVar(dd, iVar) );           Cudd_Ref( bCof1 );
        Vec_PtrPush( vNew, bCof0 );
        Vec_PtrPush( vNew, bCof1 );        
    }
    return vNew;
}
void Gia_ManRecurDsd( Gia_Man_t * p, int fVerbose )
{
    DdManager * dd;
    Dsd_Manager_t * pManDsd;
    Vec_Ptr_t * vFuncs, * vAux;
    dd = Cudd_Init( Gia_ManCiNum(p), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    vFuncs = Gia_ManCollapse( p, dd, 10000, 0 );
    Cudd_AutodynDisable( dd );
    if ( vFuncs == NULL ) 
    {
        Extra_StopManager( dd );
        return;
    }
    pManDsd = Dsd_ManagerStart( dd, Gia_ManCiNum(p), 0 );
    if ( pManDsd == NULL )
    {
        Gia_ManCollapseDeref( dd, vFuncs );
        Cudd_Quit( dd );
        return;
    }
    Dsd_Decompose( pManDsd, (DdNode **)Vec_PtrArray(vFuncs), Vec_PtrSize(vFuncs) );

    printf( "Function:\n" );
    Gia_ManPrintDsd( pManDsd, 0, Vec_PtrSize(vFuncs), 0 );

    for ( int i = 0; i < 5 && Dsd_TreeNonDsdMax(pManDsd, -1) > 0; i++ ) 
    {
        int v, iBestV = -1, DsdMin = ABC_INFINITY, SuppMin = ABC_INFINITY;
        for ( v = 0; v < Gia_ManCiNum(p); v++ ) 
        {
            Vec_Ptr_t * vTemp = Gia_ManRecurDsdCof( dd, vFuncs, v );
            Dsd_Decompose( pManDsd, (DdNode **)Vec_PtrArray(vTemp), Vec_PtrSize(vTemp) );
            int DsdCur  = Dsd_TreeNonDsdMax( pManDsd, -1 );
            int SuppCur = Dsd_TreeSuppSize( pManDsd, -1 );
            if ( DsdMin > DsdCur || (DsdMin == DsdCur && SuppMin > SuppCur) )
                DsdMin = DsdCur, SuppMin = SuppCur, iBestV = v;
            Gia_ManCollapseDeref( dd, vTemp );
        }
        assert( iBestV >= 0 );
        vFuncs = Gia_ManRecurDsdCof( dd, vAux = vFuncs, iBestV );
        Gia_ManCollapseDeref( dd, vAux );        
        printf( "Cofactoring variable %c:\n", (int)(iBestV >= 26 ? 'A' - 26 : 'a') + iBestV );
        Dsd_Decompose( pManDsd, (DdNode **)Vec_PtrArray(vFuncs), Vec_PtrSize(vFuncs) );
        Gia_ManPrintDsd( pManDsd, -1, Vec_PtrSize(vFuncs), (i+1)*2 );
    }

    Dsd_ManagerStop( pManDsd );
    Gia_ManCollapseDeref( dd, vFuncs );
    Extra_StopManager( dd );    
}

#else

Gia_Man_t * Gia_ManCollapseTest( Gia_Man_t * p, int fVerbose )
{
    return NULL;
}

void Gia_ManCheckDsd( Gia_Man_t * p, int OffSet, int fVerbose )
{
}

void Gia_ManRecurDsd( Gia_Man_t * p, int fVerbose )
{
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

