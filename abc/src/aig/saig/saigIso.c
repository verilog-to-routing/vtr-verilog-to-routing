/**CFile****************************************************************

  FileName    [saigIso.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Sequential cleanup.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigIso.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig/ioa/ioa.h"
#include "saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Find the canonical permutation of the COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManFindIsoPermCos( Aig_Man_t * pAig, Vec_Int_t * vPermCis )
{
    extern int Iso_ObjCompareByData( Aig_Obj_t ** pp1, Aig_Obj_t ** pp2 );
    Vec_Int_t * vPermCos;
    Aig_Obj_t * pObj, * pFanin;
    int i, Entry, Diff;
    assert( Vec_IntSize(vPermCis) == Aig_ManCiNum(pAig) );
    vPermCos = Vec_IntAlloc( Aig_ManCoNum(pAig) );
    if ( Saig_ManPoNum(pAig) == 1 )
        Vec_IntPush( vPermCos, 0 );
    else
    {
        Vec_Ptr_t * vRoots = Vec_PtrAlloc( Saig_ManPoNum(pAig) );
        Saig_ManForEachPo( pAig, pObj, i )
        {
            pFanin = Aig_ObjFanin0(pObj);
            assert( Aig_ObjIsConst1(pFanin) || pFanin->iData > 0 );
            pObj->iData = Abc_Var2Lit( pFanin->iData, Aig_ObjFaninC0(pObj) );
            Vec_PtrPush( vRoots, pObj );
        }
        Vec_PtrSort( vRoots, (int (*)(void))Iso_ObjCompareByData );
        Vec_PtrForEachEntry( Aig_Obj_t *, vRoots, pObj, i )
            Vec_IntPush( vPermCos, Aig_ObjCioId(pObj) );
        Vec_PtrFree( vRoots );
    }
    // add flop outputs
    Diff = Saig_ManPoNum(pAig) - Saig_ManPiNum(pAig);
    Vec_IntForEachEntryStart( vPermCis, Entry, i, Saig_ManPiNum(pAig) )
        Vec_IntPush( vPermCos, Entry + Diff );
    return vPermCos;
}

/**Function*************************************************************

  Synopsis    [Performs canonical duplication of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManDupIsoCanonical_rec( Aig_Man_t * pNew, Aig_Man_t * pAig, Aig_Obj_t * pObj )
{
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    assert( Aig_ObjIsNode(pObj) );
    if ( !Aig_ObjIsNode(Aig_ObjFanin0(pObj)) || !Aig_ObjIsNode(Aig_ObjFanin1(pObj)) )
    {
        Saig_ManDupIsoCanonical_rec( pNew, pAig, Aig_ObjFanin0(pObj) );
        Saig_ManDupIsoCanonical_rec( pNew, pAig, Aig_ObjFanin1(pObj) );
    }
    else
    {
        assert( Aig_ObjFanin0(pObj)->iData != Aig_ObjFanin1(pObj)->iData );
        if ( Aig_ObjFanin0(pObj)->iData < Aig_ObjFanin1(pObj)->iData )
        {
            Saig_ManDupIsoCanonical_rec( pNew, pAig, Aig_ObjFanin0(pObj) );
            Saig_ManDupIsoCanonical_rec( pNew, pAig, Aig_ObjFanin1(pObj) );
        }
        else
        {
            Saig_ManDupIsoCanonical_rec( pNew, pAig, Aig_ObjFanin1(pObj) );
            Saig_ManDupIsoCanonical_rec( pNew, pAig, Aig_ObjFanin0(pObj) );
        }
    }
    pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Performs canonical duplication of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDupIsoCanonical( Aig_Man_t * pAig, int fVerbose )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    Vec_Int_t * vPerm, * vPermCo;
    int i, Entry;
    // derive permutations
    vPerm   = Saig_ManFindIsoPerm( pAig, fVerbose );
    vPermCo = Saig_ManFindIsoPermCos( pAig, vPerm );
    // create the new manager
    pNew = Aig_ManStart( Aig_ManNodeNum(pAig) );
    pNew->pName = Abc_UtilStrsav( pAig->pName );
    Aig_ManIncrementTravId( pAig );
    // create constant
    pObj = Aig_ManConst1(pAig);
    pObj->pData = Aig_ManConst1(pNew);
    Aig_ObjSetTravIdCurrent( pAig, pObj );
    // create PIs
    Vec_IntForEachEntry( vPerm, Entry, i )
    {
        pObj = Aig_ManCi(pAig, Entry);
        pObj->pData = Aig_ObjCreateCi(pNew);
        Aig_ObjSetTravIdCurrent( pAig, pObj );
    }
    // traverse from the POs
    Vec_IntForEachEntry( vPermCo, Entry, i )
    {
        pObj = Aig_ManCo(pAig, Entry);
        Saig_ManDupIsoCanonical_rec( pNew, pAig, Aig_ObjFanin0(pObj) );
    }
    // create POs
    Vec_IntForEachEntry( vPermCo, Entry, i )
    {
        pObj = Aig_ManCo(pAig, Entry);
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    Aig_ManSetRegNum( pNew, Aig_ManRegNum(pAig) );
    Vec_IntFreeP( &vPerm );
    Vec_IntFreeP( &vPermCo );
    return pNew;
}





/**Function*************************************************************

  Synopsis    [Checks structural equivalence of AIG1 and AIG2.]

  Description [Returns 1 if AIG1 and AIG2 are structurally equivalent 
  under this mapping.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Iso_ManCheckMapping( Aig_Man_t * pAig1, Aig_Man_t * pAig2, Vec_Int_t * vMap2to1, int fVerbose )
{
    Aig_Obj_t * pObj, * pFanin0, * pFanin1;
    int i;
    assert( Aig_ManCiNum(pAig1) == Aig_ManCiNum(pAig2) );
    assert( Aig_ManCoNum(pAig1) == Aig_ManCoNum(pAig2) );
    assert( Aig_ManRegNum(pAig1) == Aig_ManRegNum(pAig2) );
    assert( Aig_ManNodeNum(pAig1) == Aig_ManNodeNum(pAig2) );
    Aig_ManCleanData( pAig1 );
    // map const and PI nodes
    Aig_ManConst1(pAig2)->pData = Aig_ManConst1(pAig1);
    Aig_ManForEachCi( pAig2, pObj, i )
        pObj->pData = Aig_ManCi( pAig1, Vec_IntEntry(vMap2to1, i) );
    // try internal nodes
    Aig_ManForEachNode( pAig2, pObj, i )
    {
        pFanin0 = Aig_ObjChild0Copy( pObj );
        pFanin1 = Aig_ObjChild1Copy( pObj );
        pObj->pData = Aig_TableLookupTwo( pAig1, pFanin0, pFanin1 );
        if ( pObj->pData == NULL )
        {
            if ( fVerbose )
                printf( "Structural equivalence failed at node %d.\n", i );
            return 0;
        }
    }
    // make sure the first PO points to the same node
    if ( Aig_ManCoNum(pAig1)-Aig_ManRegNum(pAig1) == 1 && Aig_ObjChild0Copy(Aig_ManCo(pAig2, 0)) != Aig_ObjChild0(Aig_ManCo(pAig1, 0)) )
    {
        if ( fVerbose )
            printf( "Structural equivalence failed at primary output 0.\n" );
        return 0;
    }
    return 1;    
}

//static int s_Counter;

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Iso_ManNegEdgeNum( Aig_Man_t * pAig )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    if ( pAig->nComplEdges > 0 )
        return pAig->nComplEdges;
    Aig_ManForEachObj( pAig, pObj, i )
        if ( Aig_ObjIsNode(pObj) )
        {
            Counter += Aig_ObjFaninC0(pObj);
            Counter += Aig_ObjFaninC1(pObj);
        }
        else if ( Aig_ObjIsCo(pObj) )
            Counter += Aig_ObjFaninC0(pObj);
    return (pAig->nComplEdges = Counter);
}

/**Function*************************************************************

  Synopsis    [Finds mapping of CIs of AIG2 into those of AIG1.]

  Description [Returns the mapping of CIs of the two AIGs, or NULL
  if there is no mapping.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Iso_ManFindMapping( Aig_Man_t * pAig1, Aig_Man_t * pAig2, Vec_Int_t * vPerm1_, Vec_Int_t * vPerm2_, int fVerbose )
{
    Vec_Int_t * vPerm1, * vPerm2, * vInvPerm2;
    int i, Entry;
    if ( Aig_ManCiNum(pAig1) != Aig_ManCiNum(pAig2) )
        return NULL;
    if ( Aig_ManCoNum(pAig1) != Aig_ManCoNum(pAig2) )
        return NULL;
    if ( Aig_ManRegNum(pAig1) != Aig_ManRegNum(pAig2) )
        return NULL;
    if ( Aig_ManNodeNum(pAig1) != Aig_ManNodeNum(pAig2) )
        return NULL;
    if ( Aig_ManLevelNum(pAig1) != Aig_ManLevelNum(pAig2) )
        return NULL;
//    if ( Iso_ManNegEdgeNum(pAig1) != Iso_ManNegEdgeNum(pAig2) )
//        return NULL;
//    s_Counter++;

    if ( fVerbose ) 
        printf( "AIG1:\n" );
    vPerm1 = vPerm1_ ? vPerm1_ : Saig_ManFindIsoPerm( pAig1, fVerbose );
    if ( fVerbose )
        printf( "AIG1:\n" );
    vPerm2 = vPerm2_ ? vPerm2_ : Saig_ManFindIsoPerm( pAig2, fVerbose );
    if ( vPerm1_ )
        assert( Vec_IntSize(vPerm1_) == Aig_ManCiNum(pAig1) );
    if ( vPerm2_ )
        assert( Vec_IntSize(vPerm2_) == Aig_ManCiNum(pAig2) );
    // find canonical permutation
    // vPerm1/vPerm2 give canonical order of CIs of AIG1/AIG2
    vInvPerm2 = Vec_IntInvert( vPerm2, -1 );
    Vec_IntForEachEntry( vInvPerm2, Entry, i )
    {
        assert( Entry >= 0 && Entry < Aig_ManCiNum(pAig1) );
        Vec_IntWriteEntry( vInvPerm2, i, Vec_IntEntry(vPerm1, Entry) );
    }
    if ( vPerm1_ == NULL )
        Vec_IntFree( vPerm1 );
    if ( vPerm2_ == NULL )
        Vec_IntFree( vPerm2 );
    // check if they are indeed equivalent
    if ( !Iso_ManCheckMapping( pAig1, pAig2, vInvPerm2, fVerbose ) )
        Vec_IntFreeP( &vInvPerm2 );
    return vInvPerm2;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Iso_ManFilterPos_old( Aig_Man_t * pAig, int fVerbose )
{
    int fVeryVerbose = 0;
    Vec_Ptr_t * vParts, * vPerms, * vAigs;
    Vec_Int_t * vPos, * vMap;
    Aig_Man_t * pPart, * pTemp;
    int i, k, nPos;

    // derive AIG for each PO
    nPos = Aig_ManCoNum(pAig) - Aig_ManRegNum(pAig);
    vParts = Vec_PtrAlloc( nPos );
    vPerms = Vec_PtrAlloc( nPos );
    for ( i = 0; i < nPos; i++ )
    {
        pPart = Saig_ManDupCones( pAig, &i, 1 );
        vMap  = Saig_ManFindIsoPerm( pPart, fVeryVerbose );
        Vec_PtrPush( vParts, pPart ); 
        Vec_PtrPush( vPerms, vMap );
    }
//    s_Counter = 0;

    // check AIGs for each PO
    vAigs = Vec_PtrAlloc( 1000 );
    vPos  = Vec_IntAlloc( 1000 );
    Vec_PtrForEachEntry( Aig_Man_t *, vParts, pPart, i )
    {
        if ( fVeryVerbose )
        {
            printf( "AIG %4d : ", i );
            Aig_ManPrintStats( pPart );
        }
        Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pTemp, k )
        {
            if ( fVeryVerbose )
                printf( "Comparing AIG %4d and AIG %4d.  ", Vec_IntEntry(vPos,k), i );
            vMap = Iso_ManFindMapping( pTemp, pPart, 
                (Vec_Int_t *)Vec_PtrEntry(vPerms, Vec_IntEntry(vPos,k)), 
                (Vec_Int_t *)Vec_PtrEntry(vPerms, i),
                fVeryVerbose );
            if ( vMap != NULL )
            {
                if ( fVeryVerbose )
                    printf( "Found match\n" );
//                if ( fVerbose )
//                    printf( "Found match for AIG %4d and AIG %4d.\n", Vec_IntEntry(vPos,k), i );
                Vec_IntFree( vMap );
                break;
            }
            if ( fVeryVerbose )
                printf( "No match.\n" );
        }
        if ( k == Vec_PtrSize(vAigs) )
        {
            Vec_PtrPush( vAigs, pPart );
            Vec_IntPush( vPos, i );
        }
    }
    // delete AIGs
    Vec_PtrForEachEntry( Aig_Man_t *, vParts, pPart, i )
        Aig_ManStop( pPart );
    Vec_PtrFree( vParts );
    Vec_PtrForEachEntry( Vec_Int_t *, vPerms, vMap, i )
        Vec_IntFree( vMap );
    Vec_PtrFree( vPerms );
    // derive the resulting AIG
    pPart = Saig_ManDupCones( pAig, Vec_IntArray(vPos), Vec_IntSize(vPos) );
    Vec_PtrFree( vAigs );
    Vec_IntFree( vPos );

//    printf( "The number of all checks %d.  Complex checks %d.\n", nPos*(nPos-1)/2, s_Counter );
    return pPart;
}

/**Function*************************************************************

  Synopsis    [Takes multi-output sequential AIG.]

  Description [Returns candidate equivalence classes of POs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Iso_StoCompareVecStr( Vec_Str_t ** p1, Vec_Str_t ** p2 )
{
    return Vec_StrCompareVec( *p1, *p2 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Iso_ManFilterPos( Aig_Man_t * pAig, Vec_Ptr_t ** pvPosEquivs, int fVerbose )
{
//    int fVeryVerbose = 0;
    Aig_Man_t * pPart, * pTemp;
    Vec_Ptr_t * vBuffers, * vClasses;
    Vec_Int_t * vLevel, * vRemain;
    Vec_Str_t * vStr, * vPrev;
    int i, nPos;
    abctime clk = Abc_Clock();
    abctime clkDup = 0, clkAig = 0, clkIso = 0, clk2;
    *pvPosEquivs = NULL;

    // derive AIG for each PO
    nPos = Aig_ManCoNum(pAig) - Aig_ManRegNum(pAig);
    vBuffers = Vec_PtrAlloc( nPos );
    for ( i = 0; i < nPos; i++ )
    {
        if ( i % 100 == 0 )
            printf( "%6d finished...\r", i );

        clk2 = Abc_Clock();
        pPart = Saig_ManDupCones( pAig, &i, 1 );
        clkDup += Abc_Clock() - clk2;

        clk2 = Abc_Clock();
        pTemp = Saig_ManDupIsoCanonical( pPart, 0 );
        clkIso += Abc_Clock() - clk2;

        clk2 = Abc_Clock();
        vStr  = Ioa_WriteAigerIntoMemoryStr( pTemp );
        clkAig += Abc_Clock() - clk2;

        Vec_PtrPush( vBuffers, vStr );
        Aig_ManStop( pTemp );
        Aig_ManStop( pPart );
        // remember the output number in nCap (attention: hack!)
        vStr->nCap = i;
    }
//    s_Counter = 0;
    if ( fVerbose )
    {
    Abc_PrintTime( 1, "Duplicate time", clkDup );
    Abc_PrintTime( 1, "Isomorph  time", clkIso );
    Abc_PrintTime( 1, "AIGER     time", clkAig );
    }

    // sort the infos
    clk = Abc_Clock();
    Vec_PtrSort( vBuffers, (int (*)(void))Iso_StoCompareVecStr );

    // create classes
    clk = Abc_Clock();
    vClasses = Vec_PtrAlloc( Saig_ManPoNum(pAig) );
    // start the first class
    Vec_PtrPush( vClasses, (vLevel = Vec_IntAlloc(4)) );
    vPrev = (Vec_Str_t *)Vec_PtrEntry( vBuffers, 0 );
    Vec_IntPush( vLevel, vPrev->nCap );
    // consider other classes
    Vec_PtrForEachEntryStart( Vec_Str_t *, vBuffers, vStr, i, 1 )
    {
        if ( Vec_StrCompareVec(vPrev, vStr) )
            Vec_PtrPush( vClasses, Vec_IntAlloc(4) );
        vLevel = (Vec_Int_t *)Vec_PtrEntryLast( vClasses );
        Vec_IntPush( vLevel, vStr->nCap );
        vPrev = vStr;
    }
    Vec_VecFree( (Vec_Vec_t *)vBuffers );

    if ( fVerbose )
    Abc_PrintTime( 1, "Sorting   time", Abc_Clock() - clk );
//    Abc_PrintTime( 1, "Traversal time", time_Trav );

    // report the results
//    Vec_VecPrintInt( (Vec_Vec_t *)vClasses );
//    printf( "Devided %d outputs into %d cand equiv classes.\n", Saig_ManPoNum(pAig), Vec_PtrSize(vClasses) );
/*
    if ( fVerbose )
    {
        Vec_PtrForEachEntry( Vec_Int_t *, vClasses, vLevel, i )
            if ( Vec_IntSize(vLevel) > 1 )
                printf( "%d ", Vec_IntSize(vLevel) );
            else
                nUnique++;
        printf( " Unique = %d\n", nUnique );
    }
*/

    // canonicize order
    Vec_PtrForEachEntry( Vec_Int_t *, vClasses, vLevel, i )
        Vec_IntSort( vLevel, 0 );
     Vec_VecSortByFirstInt( (Vec_Vec_t *)vClasses, 0 );
       
    // collect the first ones
    vRemain = Vec_IntAlloc( 100 );
    Vec_PtrForEachEntry( Vec_Int_t *, vClasses, vLevel, i )
        Vec_IntPush( vRemain, Vec_IntEntry(vLevel, 0) );

    // derive the resulting AIG
    pPart = Saig_ManDupCones( pAig, Vec_IntArray(vRemain), Vec_IntSize(vRemain) );
    Vec_IntFree( vRemain );

//    return (Vec_Vec_t *)vClasses;
//    Vec_VecFree( (Vec_Vec_t *)vClasses );
    *pvPosEquivs = vClasses;
    return pPart;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Iso_ManTest( Aig_Man_t * pAig, int fVerbose )
{
    Vec_Int_t * vPerm;
    abctime clk = Abc_Clock();
    vPerm = Saig_ManFindIsoPerm( pAig, fVerbose );
    Vec_IntFree( vPerm );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManIsoReduce( Aig_Man_t * pAig, Vec_Ptr_t ** pvPosEquivs, int fVerbose )
{ 
    Aig_Man_t * pPart;
    abctime clk = Abc_Clock();
    pPart = Iso_ManFilterPos( pAig, pvPosEquivs, fVerbose );
    printf( "Reduced %d outputs to %d outputs.  ", Saig_ManPoNum(pAig), Saig_ManPoNum(pPart) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( fVerbose && *pvPosEquivs && Saig_ManPoNum(pAig) != Vec_PtrSize(*pvPosEquivs) )
    {
        printf( "Nontrivial classes:\n" );
        Vec_VecPrintInt( (Vec_Vec_t *)*pvPosEquivs, 1 );
    }
//    Aig_ManStopP( &pPart );
    return pPart;
}

ABC_NAMESPACE_IMPL_END

#include "base/abc/abc.h"

ABC_NAMESPACE_IMPL_START

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Iso_ManTest888( Aig_Man_t * pAig1, int fVerbose )
{
    extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
    extern Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan );
    Abc_Ntk_t * pNtk;
    Aig_Man_t * pAig2;
    Vec_Int_t * vMap;
    
    pNtk = Abc_NtkFromAigPhase( pAig1 );
    Abc_NtkPermute( pNtk, 1, 0, 1, NULL );
    pAig2 = Abc_NtkToDar( pNtk, 0, 1 );
    Abc_NtkDelete( pNtk );

    vMap = Iso_ManFindMapping( pAig1, pAig2, NULL, NULL, fVerbose );
    Aig_ManStop( pAig2 );

    if ( vMap != NULL )
    {
        printf( "Mapping of AIGs is found.\n" );
        if ( fVerbose )
            Vec_IntPrint( vMap );
    }
    else
        printf( "Mapping of AIGs is NOT found.\n" );
    Vec_IntFreeP( &vMap );
    return NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

