/**CFile****************************************************************

  FileName    [acecCl.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecCl.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"
#include "misc/vec/vecWec.h"
#include "misc/extra/extra.h"

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
void Acec_ManDerive_rec( Gia_Man_t * pNew, Gia_Man_t * p, int Node, Vec_Int_t * vMirrors )
{
    Gia_Obj_t * pObj;
    int Obj = Node;
    if ( Vec_IntEntry(vMirrors, Node) >= 0 )
        Obj = Abc_Lit2Var( Vec_IntEntry(vMirrors, Node) );
    pObj = Gia_ManObj( p, Obj );
    if ( !~pObj->Value )
    {
        assert( Gia_ObjIsAnd(pObj) );
        Acec_ManDerive_rec( pNew, p, Gia_ObjFaninId0(pObj, Obj), vMirrors );
        Acec_ManDerive_rec( pNew, p, Gia_ObjFaninId1(pObj, Obj), vMirrors );
        if ( Gia_ObjIsXor(pObj) )
            pObj->Value = Gia_ManAppendXorReal( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    }
    // set the original node as well
    if ( Obj != Node )
        Gia_ManObj(p, Node)->Value = Abc_LitNotCond( pObj->Value, Abc_LitIsCompl(Vec_IntEntry(vMirrors, Node)) );
} 
Gia_Man_t * Acec_ManDerive( Gia_Man_t * p, Vec_Int_t * vMirrors )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    assert( p->pMuxes == NULL );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachCo( p, pObj, i )
        Acec_ManDerive_rec( pNew, p, Gia_ObjFaninId0p(p, pObj), vMirrors );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Acec_CollectXorTops( Gia_Man_t * p )
{
    Vec_Int_t * vRootXorSet = Vec_IntAlloc( Gia_ManCoNum(p) );
    Gia_Obj_t * pObj, * pFan0, * pFan1, * pFan00, * pFan01, * pFan10, * pFan11;
    int i, fXor0, fXor1, fFirstXor = 0;
    Gia_ManForEachCoDriver( p, pObj, i )
    {
        if ( !Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
        {
            if ( fFirstXor )
            {
                printf( "XORs do not form a continuous sequence\n" );
                Vec_IntFreeP( &vRootXorSet );
                break;
            }
            continue;
        }
        fFirstXor = 1;
        fXor0 = Gia_ObjRecognizeExor(Gia_Regular(pFan0), &pFan00, &pFan01);
        fXor1 = Gia_ObjRecognizeExor(Gia_Regular(pFan1), &pFan10, &pFan11);
        if ( fXor0 == fXor1 )
        {
            printf( "Both inputs of top level XOR have XOR/non-XOR\n" );
            Vec_IntFreeP( &vRootXorSet );
            break;
        }
        Vec_IntPush( vRootXorSet, Gia_ObjId(p, pObj) );
        Vec_IntPush( vRootXorSet, fXor1 ? Gia_ObjId(p, Gia_Regular(pFan0))  : Gia_ObjId(p, Gia_Regular(pFan1)) );
        Vec_IntPush( vRootXorSet, fXor1 ? Gia_ObjId(p, Gia_Regular(pFan10)) : Gia_ObjId(p, Gia_Regular(pFan00)) );
        Vec_IntPush( vRootXorSet, fXor1 ? Gia_ObjId(p, Gia_Regular(pFan11)) : Gia_ObjId(p, Gia_Regular(pFan01)) );
    }
    for ( i = 0; 4*i < Vec_IntSize(vRootXorSet); i++ )
    {
        printf( "%2d : ", i );
        printf( "%4d <- ", Vec_IntEntry(vRootXorSet, 4*i) );
        printf( "%4d ", Vec_IntEntry(vRootXorSet, 4*i+1) );
        printf( "%4d ", Vec_IntEntry(vRootXorSet, 4*i+2) );
        printf( "%4d ", Vec_IntEntry(vRootXorSet, 4*i+3) );
        printf( "\n" );
    }
    return vRootXorSet;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Acec_DetectLitPolarity( Gia_Man_t * p, int Node, int Leaf )
{
    Gia_Obj_t * pNode;
    int Lit0, Lit1;
    if ( Node < Leaf )
        return -1;
    if ( Node == Leaf )
        return Abc_Var2Lit(Node, 0);
    pNode = Gia_ManObj( p, Node );
    Lit0 = Acec_DetectLitPolarity( p, Gia_ObjFaninId0(pNode, Node), Leaf );
    Lit1 = Acec_DetectLitPolarity( p, Gia_ObjFaninId1(pNode, Node), Leaf );
    Lit0 = Lit0 == -1 ? Lit0 : Abc_LitNotCond( Lit0, Gia_ObjFaninC0(pNode) );
    Lit1 = Lit1 == -1 ? Lit1 : Abc_LitNotCond( Lit1, Gia_ObjFaninC1(pNode) );
    if ( Lit0 == -1 && Lit1 == -1 )
        return -1;
    assert( Lit0 != -1 || Lit1 != -1 );
    if ( Lit0 != -1 && Lit1 != -1 )
    {
        assert( Lit0 == Lit1 );
        printf( "Problem for leaf %d\n", Leaf );
        return Lit0;
    }
    return Lit0 != -1 ? Lit0 : Lit1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Acec_DetectComputeSuppOne_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSupp, Vec_Int_t * vNods )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( pObj->fMark0 )
    {
        Vec_IntPush( vSupp, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Acec_DetectComputeSuppOne_rec( p, Gia_ObjFanin0(pObj), vSupp, vNods );
    Acec_DetectComputeSuppOne_rec( p, Gia_ObjFanin1(pObj), vSupp, vNods );
    Vec_IntPush( vNods, Gia_ObjId(p, pObj) );
}
void Acec_DetectComputeSupports( Gia_Man_t * p, Vec_Int_t * vRootXorSet )
{
    Vec_Int_t * vNods = Vec_IntAlloc( 100 ); 
    Vec_Int_t * vPols = Vec_IntAlloc( 100 ); 
    Vec_Int_t * vSupp = Vec_IntAlloc( 100 );  int i, k, Node, Pol;
    for ( i = 0; 4*i < Vec_IntSize(vRootXorSet); i++ )
    {
        Gia_ManObj( p, Vec_IntEntry(vRootXorSet, 4*i+1) )->fMark0 = 1;
        Gia_ManObj( p, Vec_IntEntry(vRootXorSet, 4*i+2) )->fMark0 = 1;
        Gia_ManObj( p, Vec_IntEntry(vRootXorSet, 4*i+3) )->fMark0 = 1;
    }
    for ( i = 1; 4*i < Vec_IntSize(vRootXorSet); i++ )
    {
        Vec_IntClear( vSupp );
        Gia_ManIncrementTravId( p );

        Gia_ManObj( p, Vec_IntEntry(vRootXorSet, 4*i+1) )->fMark0 = 0;
        Acec_DetectComputeSuppOne_rec( p, Gia_ManObj( p, Vec_IntEntry(vRootXorSet, 4*i+1) ), vSupp, vNods );
        Gia_ManObj( p, Vec_IntEntry(vRootXorSet, 4*i+1) )->fMark0 = 1;

        Vec_IntSort( vSupp, 0 );

        printf( "Out %4d : %4d  \n", i, Vec_IntEntry(vRootXorSet, 4*i+1) );
        Vec_IntPrint( vSupp );

        printf( "Cone:\n" );
        Vec_IntForEachEntry( vNods, Node, k )
            Gia_ObjPrint( p, Gia_ManObj(p, Node) );


        Vec_IntClear( vPols );
        Vec_IntForEachEntry( vSupp, Node, k )
            Vec_IntPush( vPols, Acec_DetectLitPolarity(p, Vec_IntEntry(vRootXorSet, 4*i+1), Node) );

        Vec_IntForEachEntryTwo( vSupp, vPols, Node, Pol, k )
            printf( "%d(%d)  ", Node, Abc_LitIsCompl(Pol) );

        printf( "\n" );

        Vec_IntPrint( vSupp );
    }
    for ( i = 0; 4*i < Vec_IntSize(vRootXorSet); i++ )
    {
        Gia_ManObj( p, Vec_IntEntry(vRootXorSet, 4*i+1) )->fMark0 = 0;
        Gia_ManObj( p, Vec_IntEntry(vRootXorSet, 4*i+2) )->fMark0 = 0;
        Gia_ManObj( p, Vec_IntEntry(vRootXorSet, 4*i+3) )->fMark0 = 0;
    }
    Vec_IntFree( vSupp );
    Vec_IntFree( vPols );
    Vec_IntFree( vNods );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Acec_DetectXorBuildNew( Gia_Man_t * p, Vec_Int_t * vRootXorSet )
{
    Gia_Man_t * pNew;
    int i, k, iOr1, iAnd1, iAnd2, pLits[3]; // carry, in1, in2
    Vec_Int_t * vMirrors = Vec_IntStart( Gia_ManObjNum(p) );
    for ( i = 0; 4*i < Vec_IntSize(vRootXorSet); i++ )
    {
        pLits[0] = Acec_DetectLitPolarity( p, Vec_IntEntry(vRootXorSet, 4*i), Vec_IntEntry(vRootXorSet, 4*i+1) );
        // get polarity of two new ones
        for ( k = 1; k < 3; k++ )
            pLits[k] = Acec_DetectLitPolarity( p, Vec_IntEntry(vRootXorSet, 4*i), Vec_IntEntry(vRootXorSet, 4*i+k+1) );
        // create the gate
        iOr1  = Gia_ManAppendOr( p, pLits[1], pLits[2] );
        iAnd1 = Gia_ManAppendAnd( p, pLits[0], iOr1 );
        iAnd2 = Gia_ManAppendAnd( p, pLits[1], pLits[2] );
        pLits[0] = Gia_ManAppendOr( p, iAnd1, iAnd2 );
        Vec_IntWriteEntry( vMirrors, Vec_IntEntry(vRootXorSet, 4*i+1), pLits[0] );
    }
    // remap the AIG using map
    pNew = Acec_ManDerive( p, vMirrors );
    Vec_IntFree( vMirrors );
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Acec_DetectAdditional( Gia_Man_t * p, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gia_Man_t * pNew;
    Vec_Int_t * vRootXorSet;
//    Vec_Int_t * vXors, * vAdds = Ree_ManComputeCuts( p, &vXors, 0 );

    //Ree_ManPrintAdders( vAdds, 1 );
//    printf( "Detected %d full-adders and %d half-adders.  Found %d XOR-cuts.  ", Ree_ManCountFadds(vAdds), Vec_IntSize(vAdds)/6-Ree_ManCountFadds(vAdds), Vec_IntSize(vXors)/4 );
//    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    clk = Abc_Clock();
    vRootXorSet = Acec_CollectXorTops( p );
    if ( vRootXorSet )
    {
        Acec_DetectComputeSupports( p, vRootXorSet );

        pNew = Acec_DetectXorBuildNew( p, vRootXorSet );
        Vec_IntFree( vRootXorSet );
    }
    else
        pNew = Gia_ManDup( p );

    printf( "Detected %d top XORs.  ", Vec_IntSize(vRootXorSet)/4 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

//    Vec_IntFree( vXors );
//    Vec_IntFree( vAdds );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Acec_RewriteTop( Gia_Man_t * p, Acec_Box_t * pBox )
{
    Vec_Int_t * vRes = Vec_IntAlloc( Gia_ManCoNum(p) + 1 );
    Vec_Int_t * vLevel;
    int i, k, iStart, iLit, Driver, Count = 0;
    // determine how much to shift
    Driver = Gia_ObjFaninId0p( p, Gia_ManCo(p, 0) );
    Vec_WecForEachLevel( pBox->vRootLits, vLevel, iStart )
        if ( Abc_Lit2Var(Vec_IntEntry(vLevel,0)) == Driver )
            break;
    assert( iStart < Gia_ManCoNum(p) );
    //Vec_WecPrintLits( pBox->vRootLits );
    Vec_WecForEachLevelStart( pBox->vRootLits, vLevel, i, iStart )
    {
        int In[3] = {0}, Out[2];
        assert( Vec_IntSize(vLevel) > 0 );
        assert( Vec_IntSize(vLevel) <= 3 );
        if ( Vec_IntSize(vLevel) == 1 )
        {
            Vec_IntPush( vRes, Vec_IntEntry(vLevel, 0) );
            continue;
        }
        Vec_IntForEachEntry( vLevel, iLit, k )
            In[k] = iLit;
        Acec_InsertFadd( p, In, Out );
        Vec_IntPush( vRes, Out[0] );
        if ( i+1 < Vec_WecSize(pBox->vRootLits) )
            Vec_IntPush( Vec_WecEntry(pBox->vRootLits, i+1), Out[1] );
        else
            Vec_IntPush( Vec_WecPushLevel(pBox->vRootLits), Out[1] );
        Count++;
    }
    assert( Vec_IntSize(vRes) >= Gia_ManCoNum(p) );
    Vec_IntShrink( vRes, Gia_ManCoNum(p) );
    printf( "Added %d adders for replace CLAs.  ", Count );
    return vRes;
}
Gia_Man_t * Acec_RewriteReplace( Gia_Man_t * p, Vec_Int_t * vRes )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;  int i;
    assert( Gia_ManCoNum(p) == Vec_IntSize(vRes) );
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
    {
        int iLit = Vec_IntEntry( vRes, i );
        Gia_Obj_t * pRepr = Gia_ManObj( p, Abc_Lit2Var(iLit) );
        pObj->Value = Gia_ManAppendCo( pNew, pRepr->Value );
    }
    // set correct phase
    Gia_ManSetPhase( p );
    Gia_ManSetPhase( pNew );
    Gia_ManForEachCo( pNew, pObj, i )
        if ( Gia_ObjPhase(pObj) != Gia_ObjPhase(Gia_ManCo(p, i)) )
            Gia_ObjFlipFaninC0( pObj );
    // remove dangling nodes
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
Gia_Man_t * Acec_ManDecla( Gia_Man_t * pGia, int fBooth, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gia_Man_t * pNew = NULL;
    Vec_Bit_t * vIgnore = fBooth ? Acec_BoothFindPPG(pGia) : NULL;
    Acec_Box_t * pBox = Acec_DeriveBox( pGia, vIgnore, 0, 0, fVerbose );
    Vec_Int_t * vResult;
    Vec_BitFreeP( &vIgnore );
    if ( pBox == NULL ) // cannot match
    {
        printf( "Cannot find arithmetic boxes.\n" );
        return Gia_ManDup( pGia );
    }
    vResult = Acec_RewriteTop( pGia, pBox );
    Acec_BoxFreeP( &pBox );
    pNew = Acec_RewriteReplace( pGia, vResult );
    Vec_IntFree( vResult );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

