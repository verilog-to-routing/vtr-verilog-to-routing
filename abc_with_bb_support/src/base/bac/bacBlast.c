/**CFile****************************************************************

  FileName    [bacBlast.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Bit-blasting of the netlist.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacBlast.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"
#include "base/abc/abc.h"
#include "map/mio/mio.h"
#include "bool/dec/dec.h"
#include "base/main/mainInt.h"

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
void Bac_ManPrepareGates( Bac_Man_t * p )
{
    Dec_Graph_t ** ppGraphs; int i;
    if ( p->pMioLib == NULL )
        return;
    ppGraphs = ABC_CALLOC( Dec_Graph_t *, Abc_NamObjNumMax(p->pMods) );
    for ( i = 1; i < Abc_NamObjNumMax(p->pMods); i++ )
    {
        char * pGateName = Abc_NamStr( p->pMods, i );
        Mio_Gate_t * pGate = Mio_LibraryReadGateByName( (Mio_Library_t *)p->pMioLib, pGateName, NULL );
        if ( pGate != NULL )
            ppGraphs[i] = Dec_Factor( Mio_GateReadSop(pGate) );
    }
    assert( p->ppGraphs == NULL );
    p->ppGraphs = (void **)ppGraphs;
}
void Bac_ManUndoGates( Bac_Man_t * p )
{
    int i;
    if ( p->pMioLib == NULL )
        return;
    for ( i = 1; i < Abc_NamObjNumMax(p->pMods); i++ )
        if ( p->ppGraphs[i] )
            Dec_GraphFree( (Dec_Graph_t *)p->ppGraphs[i] );
    ABC_FREE( p->ppGraphs );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bac_ManAddBarbuf( Gia_Man_t * pNew, int iRes, Bac_Man_t * p, int iLNtk, int iLObj, int iRNtk, int iRObj, Vec_Int_t * vMap )
{
    int iBufLit, iIdLit;
    if ( iRes == 0 || iRes == 1 )
        return iRes;
    assert( iRes > 0 );
    if ( vMap && Abc_Lit2Var(iRes) < Vec_IntSize(vMap) && (iIdLit = Vec_IntEntry(vMap, Abc_Lit2Var(iRes))) >= 0 && 
        Vec_IntEntry(&p->vBuf2LeafNtk, Abc_Lit2Var(iIdLit)) == iLNtk && Vec_IntEntry(&p->vBuf2RootNtk, Abc_Lit2Var(iIdLit)) == iRNtk )
        return Abc_LitNotCond( Vec_IntEntry(pNew->vBarBufs, Abc_Lit2Var(iIdLit)), Abc_LitIsCompl(iRes) ^ Abc_LitIsCompl(iIdLit) );
    assert( Bac_ManNtkIsOk(p, iLNtk) && Bac_ManNtkIsOk(p, iRNtk) );
    Vec_IntPush( &p->vBuf2LeafNtk, iLNtk );
    Vec_IntPush( &p->vBuf2LeafObj, iLObj );
    Vec_IntPush( &p->vBuf2RootNtk, iRNtk );
    Vec_IntPush( &p->vBuf2RootObj, iRObj );
    iBufLit = Gia_ManAppendBuf( pNew, iRes );
    if ( vMap )
    {
        Vec_IntSetEntryFull( vMap, Abc_Lit2Var(iRes), Abc_Var2Lit(Vec_IntSize(pNew->vBarBufs), Abc_LitIsCompl(iRes)) );
        Vec_IntPush( pNew->vBarBufs, iBufLit );
    }
    return iBufLit;
}
int Bac_ManExtract_rec( Gia_Man_t * pNew, Bac_Ntk_t * p, int i, int fBuffers, Vec_Int_t * vMap )
{
    int iRes = Bac_ObjCopy( p, i );
    if ( iRes >= 0 )
        return iRes;
    if ( Bac_ObjIsCo(p, i) )
        iRes = Bac_ManExtract_rec( pNew, p, Bac_ObjFanin(p, i), fBuffers, vMap );
    else if ( Bac_ObjIsPi(p, i) )
    {
        Bac_Ntk_t * pHost = Bac_NtkHostNtk( p );
        int iObj = Bac_BoxBi( pHost, Bac_NtkHostObj(p), Bac_ObjIndex(p, i) );
        iRes = Bac_ManExtract_rec( pNew, pHost, iObj, fBuffers, vMap );
        if ( fBuffers )
            iRes = Bac_ManAddBarbuf( pNew, iRes, p->pDesign, Bac_NtkId(p), i, Bac_NtkId(pHost), iObj, vMap );
    }
    else if ( Bac_ObjIsBo(p, i) )
    {
        int iBox = Bac_BoxBoBox(p, i);
        if ( Bac_ObjIsBoxUser(p, iBox) ) // user box
        {
            Bac_Ntk_t * pBox = Bac_BoxBoNtk( p, i );
            int iObj = Bac_NtkPo( pBox, Bac_ObjIndex(p, i) );
            iRes = Bac_ManExtract_rec( pNew, pBox, iObj, fBuffers, vMap );
            if ( fBuffers )
                iRes = Bac_ManAddBarbuf( pNew, iRes, p->pDesign, Bac_NtkId(p), i, Bac_NtkId(pBox), iObj, vMap );
        }
        else // primitive
        {
            int iFanin, nLits, pLits[16];
            assert( Bac_ObjIsBoxPrim(p, iBox) );
            Bac_BoxForEachFanin( p, iBox, iFanin, nLits )
                pLits[nLits] = Bac_ManExtract_rec( pNew, p, iFanin, fBuffers, vMap );
            assert( nLits <= 16 );
            if ( p->pDesign->ppGraphs ) // mapped gate
            {
                extern int Gia_ManFactorGraph( Gia_Man_t * p, Dec_Graph_t * pFForm, Vec_Int_t * vLeaves );
                Dec_Graph_t * pGraph = (Dec_Graph_t *)p->pDesign->ppGraphs[Bac_BoxNtkId(p, iBox)];
                Vec_Int_t Leaves = { nLits, nLits, pLits };
                assert( pGraph != NULL );
                return Gia_ManFactorGraph( pNew, pGraph, &Leaves );
            }
            else
            {
                Bac_ObjType_t Type = Bac_ObjType(p, iBox);
                if ( nLits == 0 )
                {
                    if ( Type == BAC_BOX_CF )
                        iRes = 0;
                    else if ( Type == BAC_BOX_CT )
                        iRes = 1;
                    else assert( 0 );
                }
                else if ( nLits == 1 )
                {
                    if ( Type == BAC_BOX_BUF )
                        iRes = pLits[0];
                    else if ( Type == BAC_BOX_INV )
                        iRes = Abc_LitNot( pLits[0] );
                    else assert( 0 );
                }
                else if ( nLits == 2 )
                {
                    if ( Type == BAC_BOX_AND )
                        iRes = Gia_ManHashAnd( pNew, pLits[0], pLits[1] );
                    else if ( Type == BAC_BOX_NAND )
                        iRes = Abc_LitNot( Gia_ManHashAnd( pNew, pLits[0], pLits[1] ) );
                    else if ( Type == BAC_BOX_OR )
                        iRes = Gia_ManHashOr( pNew, pLits[0], pLits[1] );
                    else if ( Type == BAC_BOX_NOR )
                        iRes = Abc_LitNot( Gia_ManHashOr( pNew, pLits[0], pLits[1] ) );
                    else if ( Type == BAC_BOX_XOR )
                        iRes = Gia_ManHashXor( pNew, pLits[0], pLits[1] );
                    else if ( Type == BAC_BOX_XNOR )
                        iRes = Abc_LitNot( Gia_ManHashXor( pNew, pLits[0], pLits[1] ) );
                    else if ( Type == BAC_BOX_SHARP )
                        iRes = Gia_ManHashAnd( pNew, pLits[0], Abc_LitNot(pLits[1]) );
                    else if ( Type == BAC_BOX_SHARPL )
                        iRes = Gia_ManHashAnd( pNew, Abc_LitNot(pLits[0]), pLits[1] );
                    else assert( 0 );
                }
                else if ( nLits == 3 )
                {
                    if ( Type == BAC_BOX_MUX )
                        iRes = Gia_ManHashMux( pNew, pLits[0], pLits[1], pLits[2] );
                    else if ( Type == BAC_BOX_MAJ )
                        iRes = Gia_ManHashMaj( pNew, pLits[0], pLits[1], pLits[2] );
                    else if ( Type == BAC_BOX_ADD )
                    {
                        int iRes0 = Gia_ManHashAnd( pNew, pLits[1], pLits[2] );
                        int iRes1 = Gia_ManHashOr( pNew, pLits[1], pLits[2] );
                        assert( Bac_BoxBoNum(p, iBox) == 2 );
                        if ( Bac_BoxBo(p, iBox, 0) == i ) // sum
                            iRes = Gia_ManHashXor( pNew, pLits[0], Gia_ManHashAnd(pNew, Abc_LitNot(iRes0), iRes1) );
                        else if ( Bac_BoxBo(p, iBox, 1) == i ) // cout
                            iRes = Gia_ManHashOr( pNew, iRes0, Gia_ManHashAnd(pNew, pLits[0], iRes1) );
                        else assert( 0 );
                    }
                    else assert( 0 );
                }
                else assert( 0 );
            }
        }
    }
    else assert( 0 );
    Bac_ObjSetCopy( p, i, iRes );
    return iRes;
}
Gia_Man_t * Bac_ManExtract( Bac_Man_t * p, int fBuffers, int fVerbose )
{
    Bac_Ntk_t * pNtk, * pRoot = Bac_ManRoot(p);
    Gia_Man_t * pNew, * pTemp; 
    Vec_Int_t * vMap = NULL;
    int i, iObj;

    Vec_IntClear( &p->vBuf2LeafNtk );
    Vec_IntClear( &p->vBuf2LeafObj );
    Vec_IntClear( &p->vBuf2RootNtk );
    Vec_IntClear( &p->vBuf2RootObj );

    Bac_ManForEachNtk( p, pNtk, i )
    {
        Bac_NtkDeriveIndex( pNtk );
        Bac_NtkStartCopies( pNtk );
    }

    // start the manager
    pNew = Gia_ManStart( Bac_ManNodeNum(p) );
    pNew->pName = Abc_UtilStrsav(p->pName);
    pNew->pSpec = Abc_UtilStrsav(p->pSpec);

    // primary inputs
    Bac_NtkForEachPi( pRoot, iObj, i )
        Bac_ObjSetCopy( pRoot, iObj, Gia_ManAppendCi(pNew) );

    // internal nodes
    Gia_ManHashAlloc( pNew );
    pNew->vBarBufs = Vec_IntAlloc( 10000 );
    vMap = Vec_IntStartFull( 10000 );
    Bac_ManPrepareGates( p );
    Bac_NtkForEachPo( pRoot, iObj, i )
        Bac_ManExtract_rec( pNew, pRoot, iObj, fBuffers, vMap );
    Bac_ManUndoGates( p );
    Vec_IntFreeP( &vMap );
    Gia_ManHashStop( pNew );

    // primary outputs
    Bac_NtkForEachPo( pRoot, iObj, i )
        Gia_ManAppendCo( pNew, Bac_ObjCopy(pRoot, iObj) );
    assert( Vec_IntSize(&p->vBuf2LeafNtk) == pNew->nBufs );

    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    //Gia_ManPrintStats( pNew, NULL );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Mark each GIA node with the network it belongs to.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bac_ManMarkNodesGia( Bac_Man_t * p, Gia_Man_t * pGia )
{
    Gia_Obj_t * pObj; int i, Count = 0;
    assert( Vec_IntSize(&p->vBuf2LeafNtk) == Gia_ManBufNum(pGia) );
    Gia_ManConst0(pGia)->Value = 1;
    Gia_ManForEachPi( pGia, pObj, i )
        pObj->Value = 1;
    Gia_ManForEachAnd( pGia, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) )
            pObj->Value = Vec_IntEntry( &p->vBuf2LeafNtk, Count++ );
        else
        {
            pObj->Value = Gia_ObjFanin0(pObj)->Value;
            assert( pObj->Value == Gia_ObjFanin1(pObj)->Value );
        }
    }
    assert( Count == Gia_ManBufNum(pGia) );
    Gia_ManForEachPo( pGia, pObj, i )
    {
        assert( Gia_ObjFanin0(pObj)->Value == 1 );
        pObj->Value = 1;
    }
}
void Bac_ManRemapBarbufs( Bac_Man_t * pNew, Bac_Man_t * p )
{
    Bac_Ntk_t * pNtk;  int Entry, i;
    //assert( Vec_IntSize(&p->vBuf2RootNtk) );
    assert( !Vec_IntSize(&pNew->vBuf2RootNtk) );
    Vec_IntAppend( &pNew->vBuf2RootNtk, &p->vBuf2RootNtk );
    Vec_IntAppend( &pNew->vBuf2RootObj, &p->vBuf2RootObj );
    Vec_IntAppend( &pNew->vBuf2LeafNtk, &p->vBuf2LeafNtk );
    Vec_IntAppend( &pNew->vBuf2LeafObj, &p->vBuf2LeafObj );
    Vec_IntForEachEntry( &p->vBuf2LeafObj, Entry, i )
    {
        pNtk = Bac_ManNtk( p, Vec_IntEntry(&p->vBuf2LeafNtk, i) );
        Vec_IntWriteEntry( &pNew->vBuf2LeafObj, i, Bac_ObjCopy(pNtk, Entry) );
    }
    Vec_IntForEachEntry( &p->vBuf2RootObj, Entry, i )
    {
        pNtk = Bac_ManNtk( p, Vec_IntEntry(&p->vBuf2RootNtk, i) );
        Vec_IntWriteEntry( &pNew->vBuf2RootObj, i, Bac_ObjCopy(pNtk, Entry) );
    }
}
void Bac_NtkCreateAndConnectBuffer( Gia_Man_t * pGia, Gia_Obj_t * pObj, Bac_Ntk_t * p, int iTerm )
{
    int iObj;
    if ( pGia && Gia_ObjFaninId0p(pGia, pObj) > 0 )
    {
        iObj = Bac_ObjAlloc( p, BAC_OBJ_BI, Gia_ObjFanin0(pObj)->Value );
        Bac_ObjAlloc( p, Gia_ObjFaninC0(pObj) ? BAC_BOX_INV : BAC_BOX_BUF, -1 );
    }
    else
    {
        Bac_ObjAlloc( p, pGia && Gia_ObjFaninC0(pObj) ? BAC_BOX_CT : BAC_BOX_CF, -1 );
    }
    iObj = Bac_ObjAlloc( p, BAC_OBJ_BO, -1 );
    Bac_ObjSetFanin( p, iTerm, iObj );
}
void Bac_NtkInsertGia( Bac_Man_t * p, Gia_Man_t * pGia )
{
    Bac_Ntk_t * pNtk, * pRoot = Bac_ManRoot( p );
    int i, j, k, iBox, iTerm, Count = 0;
    Gia_Obj_t * pObj;

    Gia_ManConst0(pGia)->Value = ~0;
    Gia_ManForEachPi( pGia, pObj, i )
        pObj->Value = Bac_NtkPi( pRoot, i );
    Gia_ManForEachAnd( pGia, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) )
        {
            pNtk = Bac_ManNtk( p, Vec_IntEntry(&p->vBuf2RootNtk, Count) );
            iTerm = Vec_IntEntry( &p->vBuf2RootObj, Count );
            assert( Bac_ObjIsCo(pNtk, iTerm) );
            if ( Bac_ObjFanin(pNtk, iTerm) == -1 ) // not a feedthrough
                Bac_NtkCreateAndConnectBuffer( pGia, pObj, pNtk, iTerm );
            // prepare leaf
            pObj->Value = Vec_IntEntry( &p->vBuf2LeafObj, Count++ );
        }
        else
        {
            int iLit0 = Gia_ObjFanin0(pObj)->Value;
            int iLit1 = Gia_ObjFanin1(pObj)->Value;
            Bac_ObjType_t Type;
            pNtk = Bac_ManNtk( p, pObj->Value );
            if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
                Type = BAC_BOX_NOR;
            else if ( Gia_ObjFaninC1(pObj) )
                Type = BAC_BOX_SHARP;
            else if ( Gia_ObjFaninC0(pObj) )
            {
                Type = BAC_BOX_SHARP;
                ABC_SWAP( int, iLit0, iLit1 );
            }
            else 
                Type = BAC_BOX_AND;
            // create box
            iTerm = Bac_ObjAlloc( pNtk, BAC_OBJ_BI, iLit1 );
            iTerm = Bac_ObjAlloc( pNtk, BAC_OBJ_BI, iLit0 );
            Bac_ObjAlloc( pNtk, Type, -1 );
            pObj->Value = Bac_ObjAlloc( pNtk, BAC_OBJ_BO, -1 );
        }
    }
    assert( Count == Gia_ManBufNum(pGia) );

    // create constant 0 drivers for COs without barbufs
    Bac_ManForEachNtk( p, pNtk, i )
    {
        Bac_NtkForEachBox( pNtk, iBox )
            Bac_BoxForEachBi( pNtk, iBox, iTerm, j )
                if ( Bac_ObjFanin(pNtk, iTerm) == -1 )
                    Bac_NtkCreateAndConnectBuffer( NULL, NULL, pNtk, iTerm );
        Bac_NtkForEachPo( pNtk, iTerm, k )
            if ( pNtk != pRoot && Bac_ObjFanin(pNtk, iTerm) == -1 )
                Bac_NtkCreateAndConnectBuffer( NULL, NULL, pNtk, iTerm );
    }
    // create node and connect POs
    Gia_ManForEachPo( pGia, pObj, i )
        if ( Bac_ObjFanin(pRoot, Bac_NtkPo(pRoot, i)) == -1 ) // not a feedthrough
            Bac_NtkCreateAndConnectBuffer( pGia, pObj, pRoot, Bac_NtkPo(pRoot, i) );
}
Bac_Man_t * Bac_ManInsertGia( Bac_Man_t * p, Gia_Man_t * pGia )
{
    Bac_Man_t * pNew = Bac_ManDupUserBoxes( p );
    Bac_ManMarkNodesGia( p, pGia );
    Bac_ManRemapBarbufs( pNew, p );
    Bac_NtkInsertGia( pNew, pGia );
    Bac_ManMoveNames( pNew, p );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bac_Man_t * Bac_ManBlastTest( Bac_Man_t * p )
{
    Gia_Man_t * pGia = Bac_ManExtract( p, 1, 0 );
    Bac_Man_t * pNew = Bac_ManInsertGia( p, pGia );
    Gia_ManStop( pGia );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Mark each GIA node with the network it belongs to.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_NodeIsSeriousGate( Abc_Obj_t * p )
{
    return Abc_ObjIsNode(p) && Abc_ObjFaninNum(p) > 0 && !Abc_ObjIsBarBuf(p);
}
void Bac_ManMarkNodesAbc( Bac_Man_t * p, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj, * pFanin; int i, k, Count = 0;
    assert( Vec_IntSize(&p->vBuf2LeafNtk) == pNtk->nBarBufs2 );
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->iTemp = 1;
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( Abc_ObjIsBarBuf(pObj) )
            pObj->iTemp = Vec_IntEntry( &p->vBuf2LeafNtk, Count++ );
        else if ( Abc_NodeIsSeriousGate(pObj) )
        {
            pObj->iTemp = Abc_ObjFanin0(pObj)->iTemp;
            Abc_ObjForEachFanin( pObj, pFanin, k )
                assert( pObj->iTemp == pFanin->iTemp );
        }
    }
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        if ( !Abc_NodeIsSeriousGate(Abc_ObjFanin0(pObj)) )
            continue;
        assert( Abc_ObjFanin0(pObj)->iTemp == 1 );
        pObj->iTemp = Abc_ObjFanin0(pObj)->iTemp;
    }
    assert( Count == pNtk->nBarBufs2 );
}
void Bac_NtkCreateOrConnectFanin( Abc_Obj_t * pFanin, Bac_Ntk_t * p, int iTerm )
{
    int iObj;
    if ( pFanin && Abc_NodeIsSeriousGate(pFanin) )//&& Bac_ObjName(p, pFanin->iTemp) == -1 ) // gate without name
    {
        iObj = pFanin->iTemp;
    }
    else if ( pFanin && (Abc_ObjIsPi(pFanin) || Abc_ObjIsBarBuf(pFanin) || Abc_NodeIsSeriousGate(pFanin)) ) // PI/BO or gate with name
    {
        iObj = Bac_ObjAlloc( p, BAC_OBJ_BI, pFanin->iTemp );
        Bac_ObjAlloc( p, BAC_BOX_GATE, p->pDesign->ElemGates[2] ); // buffer
        iObj = Bac_ObjAlloc( p, BAC_OBJ_BO, -1 );
    }
    else
    {
        assert( !pFanin || Abc_NodeIsConst0(pFanin) || Abc_NodeIsConst1(pFanin) );
        Bac_ObjAlloc( p, BAC_BOX_GATE, p->pDesign->ElemGates[(pFanin && Abc_NodeIsConst1(pFanin))] ); // const 0/1
        iObj = Bac_ObjAlloc( p, BAC_OBJ_BO, -1 );
    }
    Bac_ObjSetFanin( p, iTerm, iObj );
}
void Bac_NtkPrepareLibrary( Bac_Man_t * p, Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;
    Mio_Gate_t * pGate0 = Mio_LibraryReadConst0( pLib );
    Mio_Gate_t * pGate1 = Mio_LibraryReadConst1( pLib );
    Mio_Gate_t * pGate2 = Mio_LibraryReadBuf( pLib );
    if ( !pGate0 || !pGate1 || !pGate2 )
    {
        printf( "The library does not have one of the elementary gates.\n" );
        return;
    }
    p->ElemGates[0] = Abc_NamStrFindOrAdd( p->pMods, Mio_GateReadName(pGate0), NULL );
    p->ElemGates[1] = Abc_NamStrFindOrAdd( p->pMods, Mio_GateReadName(pGate1), NULL );
    p->ElemGates[2] = Abc_NamStrFindOrAdd( p->pMods, Mio_GateReadName(pGate2), NULL );
    Mio_LibraryForEachGate( pLib, pGate )
        if ( pGate != pGate0 && pGate != pGate1 && pGate != pGate2 )
            Abc_NamStrFindOrAdd( p->pMods, Mio_GateReadName(pGate), NULL );
    assert( Abc_NamObjNumMax(p->pMods) > 1 );
}
int Bac_NtkBuildLibrary( Bac_Man_t * p )
{
    int RetValue = 1;
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
    if ( pLib == NULL )
        printf( "The standard cell library is not available.\n" ), RetValue = 0;
    else
        Bac_NtkPrepareLibrary( p, pLib );
    p->pMioLib = pLib;
    return RetValue;
}
void Bac_NtkInsertNtk( Bac_Man_t * p, Abc_Ntk_t * pNtk )
{
    Bac_Ntk_t * pCbaNtk, * pRoot = Bac_ManRoot( p );
    int i, j, k, iBox, iTerm, Count = 0;
    Abc_Obj_t * pObj;
    assert( Abc_NtkHasMapping(pNtk) );
    Bac_NtkPrepareLibrary( p, (Mio_Library_t *)pNtk->pManFunc );
    p->pMioLib = pNtk->pManFunc;

    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->iTemp = Bac_NtkPi( pRoot, i );
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( Abc_ObjIsBarBuf(pObj) )
        {
            pCbaNtk = Bac_ManNtk( p, Vec_IntEntry(&p->vBuf2RootNtk, Count) );
            iTerm = Vec_IntEntry( &p->vBuf2RootObj, Count );
            assert( Bac_ObjIsCo(pCbaNtk, iTerm) );
            if ( Bac_ObjFanin(pCbaNtk, iTerm) == -1 ) // not a feedthrough
                Bac_NtkCreateOrConnectFanin( Abc_ObjFanin0(pObj), pCbaNtk, iTerm );
            // prepare leaf
            pObj->iTemp = Vec_IntEntry( &p->vBuf2LeafObj, Count++ );
        }
        else if ( Abc_NodeIsSeriousGate(pObj) )
        {
            pCbaNtk = Bac_ManNtk( p, pObj->iTemp );
            for ( k = Abc_ObjFaninNum(pObj)-1; k >= 0; k-- )
                iTerm = Bac_ObjAlloc( pCbaNtk, BAC_OBJ_BI, Abc_ObjFanin(pObj, k)->iTemp );
            Bac_ObjAlloc( pCbaNtk, BAC_BOX_GATE, Abc_NamStrFind(p->pMods, Mio_GateReadName((Mio_Gate_t *)pObj->pData)) );
            pObj->iTemp = Bac_ObjAlloc( pCbaNtk, BAC_OBJ_BO, -1 );
        }
    }
    assert( Count == pNtk->nBarBufs2 );

    // create constant 0 drivers for COs without barbufs
    Bac_ManForEachNtk( p, pCbaNtk, i )
    {
        Bac_NtkForEachBox( pCbaNtk, iBox )
            Bac_BoxForEachBi( pCbaNtk, iBox, iTerm, j )
                if ( Bac_ObjFanin(pCbaNtk, iTerm) == -1 )
                    Bac_NtkCreateOrConnectFanin( NULL, pCbaNtk, iTerm );
        Bac_NtkForEachPo( pCbaNtk, iTerm, k )
            if ( pCbaNtk != pRoot && Bac_ObjFanin(pCbaNtk, iTerm) == -1 )
                Bac_NtkCreateOrConnectFanin( NULL, pCbaNtk, iTerm );
    }
    // create node and connect POs
    Abc_NtkForEachPo( pNtk, pObj, i )
        if ( Bac_ObjFanin(pRoot, Bac_NtkPo(pRoot, i)) == -1 ) // not a feedthrough
            Bac_NtkCreateOrConnectFanin( Abc_ObjFanin0(pObj), pRoot, Bac_NtkPo(pRoot, i) );
}
void * Bac_ManInsertAbc( Bac_Man_t * p, void * pAbc )
{
    Abc_Ntk_t * pNtk = (Abc_Ntk_t *)pAbc;
    Bac_Man_t * pNew = Bac_ManDupUserBoxes( p );
    Bac_ManMarkNodesAbc( p, pNtk );
    Bac_ManRemapBarbufs( pNew, p );
    Bac_NtkInsertNtk( pNew, pNtk );
    Bac_ManMoveNames( pNew, p );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

