/**CFile****************************************************************

  FileName    [giaMini.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Reader/writer for MiniAIG.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaMini.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "opt/dau/dau.h"
#include "base/main/mainInt.h"
#include "misc/util/utilTruth.h"
#include "aig/miniaig/miniaig.h"
#include "aig/miniaig/minilut.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Converts MiniAIG into GIA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ObjFromMiniFanin0Copy( Gia_Man_t * pGia, Vec_Int_t * vCopies, Mini_Aig_t * p, int Id )
{
    int Lit = Mini_AigNodeFanin0( p, Id );
    return Abc_LitNotCond( Vec_IntEntry(vCopies, Abc_Lit2Var(Lit)), Abc_LitIsCompl(Lit) );
}
int Gia_ObjFromMiniFanin1Copy( Gia_Man_t * pGia, Vec_Int_t * vCopies, Mini_Aig_t * p, int Id )
{
    int Lit = Mini_AigNodeFanin1( p, Id );
    return Abc_LitNotCond( Vec_IntEntry(vCopies, Abc_Lit2Var(Lit)), Abc_LitIsCompl(Lit) );
}
Gia_Man_t * Gia_ManFromMiniAig( Mini_Aig_t * p, Vec_Int_t ** pvCopies )
{
    Gia_Man_t * pGia, * pTemp;
    Vec_Int_t * vCopies;
    int i, iGiaLit = 0, nNodes;
    // get the number of nodes
    nNodes = Mini_AigNodeNum(p);
    // create ABC network
    pGia = Gia_ManStart( nNodes );
    pGia->pName = Abc_UtilStrsav( "MiniAig" );
    // create mapping from MiniAIG objects into ABC objects
    vCopies = Vec_IntAlloc( nNodes );
    Vec_IntPush( vCopies, 0 );
    // iterate through the objects
    Gia_ManHashAlloc( pGia );
    for ( i = 1; i < nNodes; i++ )
    {
        if ( Mini_AigNodeIsPi( p, i ) )
            iGiaLit = Gia_ManAppendCi(pGia);
        else if ( Mini_AigNodeIsPo( p, i ) )
            iGiaLit = Gia_ManAppendCo(pGia, Gia_ObjFromMiniFanin0Copy(pGia, vCopies, p, i));
        else if ( Mini_AigNodeIsAnd( p, i ) )
            iGiaLit = Gia_ManHashAnd(pGia, Gia_ObjFromMiniFanin0Copy(pGia, vCopies, p, i), Gia_ObjFromMiniFanin1Copy(pGia, vCopies, p, i));
        else assert( 0 );
        Vec_IntPush( vCopies, iGiaLit );
    }
    Gia_ManHashStop( pGia );
    assert( Vec_IntSize(vCopies) == nNodes );
    if ( pvCopies )
        *pvCopies = vCopies;
    else
        Vec_IntFree( vCopies );
    Gia_ManSetRegNum( pGia, Mini_AigRegNum(p) );
    pGia = Gia_ManCleanup( pTemp = pGia );
    if ( pvCopies )
        Gia_ManDupRemapLiterals( *pvCopies, pTemp );
    Gia_ManStop( pTemp );
    return pGia;
}

/**Function*************************************************************

  Synopsis    [Converts GIA into MiniAIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mini_Aig_t * Gia_ManToMiniAig( Gia_Man_t * pGia )
{
    Mini_Aig_t * p;
    Gia_Obj_t * pObj;
    int i;
    // create the manager
    p = Mini_AigStart();
    Gia_ManConst0(pGia)->Value = Mini_AigLitConst0();
    // create primary inputs
    Gia_ManForEachCi( pGia, pObj, i )
        pObj->Value = Mini_AigCreatePi(p);
    // create internal nodes
    Gia_ManForEachAnd( pGia, pObj, i )
        pObj->Value = Mini_AigAnd( p, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    // create primary outputs
    Gia_ManForEachCo( pGia, pObj, i )
        pObj->Value = Mini_AigCreatePo( p, Gia_ObjFanin0Copy(pObj) );
    // set registers
    Mini_AigSetRegNum( p, Gia_ManRegNum(pGia) );
    return p;
}

/**Function*************************************************************

  Synopsis    [Procedures to input/output MiniAIG into/from internal GIA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameGiaInputMiniAig( Abc_Frame_t * pAbc, void * p )
{
    Gia_Man_t * pGia;
    if ( pAbc == NULL )
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
    Gia_ManStopP( &pAbc->pGiaMiniAig );
    Vec_IntFreeP( &pAbc->vCopyMiniAig );
    pGia = Gia_ManFromMiniAig( (Mini_Aig_t *)p, &pAbc->vCopyMiniAig );
    Abc_FrameUpdateGia( pAbc, pGia );
    pAbc->pGiaMiniAig = Gia_ManDup( pGia );
//    Gia_ManDelete( pGia );
}
void * Abc_FrameGiaOutputMiniAig( Abc_Frame_t * pAbc )
{
    Gia_Man_t * pGia;
    if ( pAbc == NULL )
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
    pGia = Abc_FrameReadGia( pAbc );
    if ( pGia == NULL )
        printf( "Current network in ABC framework is not defined.\n" );
    return Gia_ManToMiniAig( pGia );
}

/**Function*************************************************************

  Synopsis    [Procedures to read/write GIA to/from MiniAIG file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManReadMiniAig( char * pFileName )
{
    Mini_Aig_t * p = Mini_AigLoad( pFileName );
    Gia_Man_t * pGia = Gia_ManFromMiniAig( p, NULL );
    ABC_FREE( pGia->pName );
    pGia->pName = Extra_FileNameGeneric( pFileName ); 
    Mini_AigStop( p );
    return pGia;
}
void Gia_ManWriteMiniAig( Gia_Man_t * pGia, char * pFileName )
{
    Mini_Aig_t * p = Gia_ManToMiniAig( pGia );
    Mini_AigDump( p, pFileName );
    //Mini_AigDumpVerilog( "test_miniaig.v", "top", p );
    Mini_AigStop( p );
}




/**Function*************************************************************

  Synopsis    [Converts MiniLUT into GIA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFromMiniLut( Mini_Lut_t * p, Vec_Int_t ** pvCopies )
{
    Gia_Man_t * pGia, * pTemp;
    Vec_Int_t * vCopies;
    Vec_Int_t * vCover = Vec_IntAlloc( 1000 );
    Vec_Int_t * vLits = Vec_IntAlloc( 100 );
    int i, k, Fan, iGiaLit, nNodes;
    int LutSize = Abc_MaxInt( 2, Mini_LutSize(p) );
    // get the number of nodes
    nNodes = Mini_LutNodeNum(p);
    // create ABC network
    pGia = Gia_ManStart( 3 * nNodes );
    pGia->pName = Abc_UtilStrsav( "MiniLut" );
    // create mapping from MiniLUT objects into ABC objects
    vCopies = Vec_IntAlloc( nNodes );
    Vec_IntPush( vCopies, 0 );
    Vec_IntPush( vCopies, 1 );
    // iterate through the objects
    Gia_ManHashAlloc( pGia );
    for ( i = 2; i < nNodes; i++ )
    {
        if ( Mini_LutNodeIsPi( p, i ) )
            iGiaLit = Gia_ManAppendCi(pGia);
        else if ( Mini_LutNodeIsPo( p, i ) )
            iGiaLit = Gia_ManAppendCo(pGia, Vec_IntEntry(vCopies, Mini_LutNodeFanin(p, i, 0)));
        else if ( Mini_LutNodeIsNode( p, i ) )
        {
            unsigned * puTruth = Mini_LutNodeTruth( p, i );
            word Truth = ((word)*puTruth << 32) | (word)*puTruth; 
            word * pTruth = LutSize < 6 ? &Truth : (word *)puTruth;
            Vec_IntClear( vLits );
            Mini_LutForEachFanin( p, i, Fan, k )
                Vec_IntPush( vLits, Vec_IntEntry(vCopies, Fan) );
            iGiaLit = Dsm_ManTruthToGia( pGia, pTruth, vLits, vCover );
        }
        else assert( 0 );
        Vec_IntPush( vCopies, iGiaLit );
    }
    Vec_IntFree( vCover );
    Vec_IntFree( vLits );
    Gia_ManHashStop( pGia );
    assert( Vec_IntSize(vCopies) == nNodes );
    if ( pvCopies )
        *pvCopies = vCopies;
    else
        Vec_IntFree( vCopies );
    Gia_ManSetRegNum( pGia, Mini_LutRegNum(p) );
    pGia = Gia_ManCleanup( pTemp = pGia );
    if ( pvCopies )
        Gia_ManDupRemapLiterals( *pvCopies, pTemp );
    Gia_ManStop( pTemp );
    return pGia;
}

/**Function*************************************************************

  Synopsis    [Marks LUTs that should be complemented.]

  Description [These are LUTs whose all PO fanouts require them
  in negative polarity.  Other fanouts may require them in 
  positive polarity.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Bit_t * Gia_ManFindComplLuts( Gia_Man_t * pGia )
{
    Gia_Obj_t * pObj;  int i;
    // mark objects pointed by COs in negative polarity
    Vec_Bit_t * vMarks = Vec_BitStart( Gia_ManObjNum(pGia) );
    Gia_ManForEachCo( pGia, pObj, i )
        if ( Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) && Gia_ObjFaninC0(pObj) )
            Vec_BitWriteEntry( vMarks, Gia_ObjFaninId0p(pGia, pObj), 1 );
    // unmark objects pointed by COs in positive polarity
    Gia_ManForEachCo( pGia, pObj, i )
        if ( Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) && !Gia_ObjFaninC0(pObj) )
            Vec_BitWriteEntry( vMarks, Gia_ObjFaninId0p(pGia, pObj), 0 );
    return vMarks;
}

/**Function*************************************************************

  Synopsis    [Converts GIA into MiniLUT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mini_Lut_t * Gia_ManToMiniLut( Gia_Man_t * pGia )
{
    Mini_Lut_t * p;
    Vec_Bit_t * vMarks;
    Gia_Obj_t * pObj, * pFanin;
    Vec_Int_t * vLeaves = Vec_IntAlloc( 16 );
    int i, k, iFanin, LutSize, nWords, Count = 0, pVars[16];
    word * pTruth;
    assert( Gia_ManHasMapping(pGia) );
    LutSize = Gia_ManLutSizeMax( pGia );
    LutSize = Abc_MaxInt( LutSize, 2 );
    nWords  = Abc_Truth6WordNum( LutSize );
    assert( LutSize >= 2 );
    // create the manager
    p = Mini_LutStart( LutSize );
    // create primary inputs
    Gia_ManFillValue( pGia );
    Gia_ManForEachCi( pGia, pObj, i )
        pObj->Value = Mini_LutCreatePi(p);
    // create internal nodes
    vMarks = Gia_ManFindComplLuts( pGia );
    Gia_ObjComputeTruthTableStart( pGia, LutSize );
    Gia_ManForEachLut( pGia, i )
    {
        Vec_IntClear( vLeaves );
        Gia_LutForEachFanin( pGia, i, iFanin, k )
            Vec_IntPush( vLeaves, iFanin );
        if ( Vec_IntSize(vLeaves) > 6 )
        {
            int Extra = Vec_IntSize(vLeaves) - 7;
            for ( k = Extra; k >= 0; k-- )
                Vec_IntPush( vLeaves, Vec_IntEntry(vLeaves, k) );
            for ( k = Extra; k >= 0; k-- )
                Vec_IntDrop( vLeaves, k );
            assert( Vec_IntSize(vLeaves) == Gia_ObjLutSize(pGia, i) );
        }
        Gia_ManForEachObjVec( vLeaves, pGia, pFanin, k )
            pVars[k] = pFanin->Value;
        pObj = Gia_ManObj( pGia, i );
        pTruth = Gia_ObjComputeTruthTableCut( pGia, pObj, vLeaves );
        if ( Vec_BitEntry(vMarks, i) )
            Abc_TtNot( pTruth, nWords );
        Vec_IntForEachEntry( vLeaves, iFanin, k )
            if ( Vec_BitEntry(vMarks, iFanin) )
                Abc_TtFlip( pTruth, nWords, k );
        pObj->Value = Mini_LutCreateNode( p, Gia_ObjLutSize(pGia, i), pVars, (unsigned *)pTruth );
    }
    Vec_IntFree( vLeaves );
    // create inverter truth table
    Vec_WrdClear( pGia->vTtMemory );
    for ( i = 0; i < nWords; i++ )
        Vec_WrdPush( pGia->vTtMemory, ABC_CONST(0x5555555555555555) );
    pTruth = Vec_WrdArray( pGia->vTtMemory );
    // create primary outputs
    Gia_ManForEachCo( pGia, pObj, i )
    {
        if ( Gia_ObjFanin0(pObj) == Gia_ManConst0(pGia) )
            pObj->Value = Mini_LutCreatePo( p, Gia_ObjFaninC0(pObj) );
        else if ( Gia_ObjFaninC0(pObj) == Vec_BitEntry(vMarks, Gia_ObjFaninId0p(pGia, pObj)) )
            pObj->Value = Mini_LutCreatePo( p, Gia_ObjFanin0(pObj)->Value );
        else // add inverter LUT
        {
            int Fanin = Gia_ObjFanin0(pObj)->Value;
            int LutInv = Mini_LutCreateNode( p, 1, &Fanin, (unsigned *)pTruth );
            pObj->Value = Mini_LutCreatePo( p, LutInv );
            Count++;
        }
    }
    Vec_BitFree( vMarks );
    Gia_ObjComputeTruthTableStop( pGia );
    // set registers
    Mini_LutSetRegNum( p, Gia_ManRegNum(pGia) );
    //Mini_LutPrintStats( p );
    //printf( "Added %d inverters.\n", Count );
    return p;
}
char * Gia_ManToMiniLutAttr( Gia_Man_t * pGia, void * pMiniLut )
{ 
    Mini_Lut_t * p = (Mini_Lut_t *)pMiniLut; int i;
    char * pAttrs = ABC_CALLOC( char, Mini_LutNodeNum(p) );
    Gia_ManForEachLut( pGia, i )
        if ( Gia_ObjLutIsMux(pGia, i) )
            pAttrs[Gia_ManObj(pGia, i)->Value] = 1;
    return pAttrs;
}

/**Function*************************************************************

  Synopsis    [Procedures to input/output MiniAIG into/from internal GIA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FrameGiaInputMiniLut( Abc_Frame_t * pAbc, void * p )
{
    Gia_Man_t * pGia;
    if ( pAbc == NULL )
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
    pGia = Gia_ManFromMiniLut( (Mini_Lut_t *)p, NULL );
    Abc_FrameUpdateGia( pAbc, pGia );
//    Gia_ManDelete( pGia );
}
void * Abc_FrameGiaOutputMiniLut( Abc_Frame_t * pAbc )
{
    Mini_Lut_t * pRes = NULL;
    Gia_Man_t * pGia;
    if ( pAbc == NULL )
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
    Gia_ManStopP( &pAbc->pGiaMiniLut );
    Vec_IntFreeP( &pAbc->vCopyMiniLut );
    pGia = Abc_FrameReadGia( pAbc );
    if ( pGia == NULL )
        printf( "Current network in ABC framework is not defined.\n" );
    pRes = Gia_ManToMiniLut( pGia );
    pAbc->pGiaMiniLut = Gia_ManFromMiniLut( pRes, &pAbc->vCopyMiniLut );
    return pRes;
}
char * Abc_FrameGiaOutputMiniLutAttr( Abc_Frame_t * pAbc, void * pMiniLut )
{
    Gia_Man_t * pGia;
    if ( pAbc == NULL )
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
    pGia = Abc_FrameReadGia( pAbc );
    if ( pGia == NULL )
        printf( "Current network in ABC framework is not defined.\n" );
    return Gia_ManToMiniLutAttr( pGia, pMiniLut );
}

/**Function*************************************************************

  Synopsis    [Procedures to read/write GIA to/from MiniAIG file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManReadMiniLut( char * pFileName )
{
    Mini_Lut_t * p = Mini_LutLoad( pFileName );
    Gia_Man_t * pGia = Gia_ManFromMiniLut( p, NULL );
    ABC_FREE( pGia->pName );
    pGia->pName = Extra_FileNameGeneric( pFileName ); 
    Mini_LutStop( p );
    return pGia;
}
void Gia_ManWriteMiniLut( Gia_Man_t * pGia, char * pFileName )
{
    Mini_Lut_t * p = Gia_ManToMiniLut( pGia );
    Mini_LutDump( p, pFileName );
    Mini_LutStop( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Gia_ManMapMiniLut2MiniAig( Gia_Man_t * p, Gia_Man_t * p1, Gia_Man_t * p2, Vec_Int_t * vMap1, Vec_Int_t * vMap2 )
{
    int * pRes = ABC_FALLOC( int, Vec_IntSize(vMap2) );
    Vec_Int_t * vMap = Vec_IntStartFull( Gia_ManObjNum(p) );
    int i, Entry, iRepr, fCompl, iLit;
    Gia_Obj_t * pObj;
    Gia_ManSetPhase( p1 );
    Gia_ManSetPhase( p2 );
    Vec_IntForEachEntry( vMap1, Entry, i )
    {
        if ( Entry == -1 )
            continue;
        pObj = Gia_ManObj( p1, Abc_Lit2Var(Entry) );
        if ( ~pObj->Value == 0 )
            continue;
        fCompl = Abc_LitIsCompl(Entry) ^ pObj->fPhase;
        iRepr = Gia_ObjReprSelf(p, Abc_Lit2Var(pObj->Value));
        Vec_IntWriteEntry( vMap, iRepr, Abc_Var2Lit(i, fCompl) );
    }
    Vec_IntForEachEntry( vMap2, Entry, i )
    {
        if ( Entry == -1 )
            continue;
        pObj = Gia_ManObj( p2, Abc_Lit2Var(Entry) );
        if ( ~pObj->Value == 0 )
            continue;
        fCompl = Abc_LitIsCompl(Entry) ^ pObj->fPhase;
        iRepr = Gia_ObjReprSelf(p, Abc_Lit2Var(pObj->Value));
        if ( (iLit = Vec_IntEntry(vMap, iRepr)) == -1 )
            continue;
        pRes[i] = Abc_LitNotCond( iLit, fCompl );
    }
    Vec_IntFill( vMap, Gia_ManCoNum(p1), -1 );
    Vec_IntForEachEntry( vMap1, Entry, i )
    {
        if ( Entry == -1 )
            continue;
        pObj = Gia_ManObj( p1, Abc_Lit2Var(Entry) );
        if ( !Gia_ObjIsCo(pObj) )
            continue;
        Vec_IntWriteEntry( vMap, Gia_ObjCioId(pObj), i );
    }
    Vec_IntForEachEntry( vMap2, Entry, i )
    {
        if ( Entry == -1 )
            continue;
        pObj = Gia_ManObj( p2, Abc_Lit2Var(Entry) );
        if ( !Gia_ObjIsCo(pObj) )
            continue;
        assert( pRes[i] == -1 );
        pRes[i] = Abc_Var2Lit( Vec_IntEntry(vMap, Gia_ObjCioId(pObj)), 0 );
        assert( pRes[i] != -1 );
    }
    Vec_IntFree( vMap );
    return pRes;
}
void Gia_ManNameMapVerify( Gia_Man_t * p, Gia_Man_t * p1, Gia_Man_t * p2, Vec_Int_t * vMap1, Vec_Int_t * vMap2, int * pMap )
{
    int iLut, iObj1, iObj2, nSize = Vec_IntSize(vMap2);
    Gia_Obj_t * pObjAig, * pObjLut;
    Gia_ManSetPhase( p1 );
    Gia_ManSetPhase( p2 );
    for ( iLut = 0; iLut < nSize; iLut++ )
        if ( pMap[iLut] >= 0 )
        {
            int iObj   = Abc_Lit2Var( pMap[iLut] );
            int fCompl = Abc_LitIsCompl( pMap[iLut] );
            int iLitAig = Vec_IntEntry( vMap1, iObj );
            int iLitLut = Vec_IntEntry( vMap2, iLut );
            pObjAig = Gia_ManObj( p1, Abc_Lit2Var(iLitAig) );
            if ( Gia_ObjIsCo(pObjAig) )
                continue;
            if ( ~pObjAig->Value == 0 )
                continue;
            pObjLut = Gia_ManObj( p2, Abc_Lit2Var(iLitLut) );
            if ( ~pObjLut->Value == 0 )
                continue;
            iObj1 = Gia_ObjReprSelf(p, Abc_Lit2Var(pObjAig->Value));
            iObj2 = Gia_ObjReprSelf(p, Abc_Lit2Var(pObjLut->Value));
            if ( iObj1 != iObj2 )
                printf( "Found functional mismatch for LutId %d and AigId %d.\n", iLut, iObj );
            if ( (pObjLut->fPhase ^ Abc_LitIsCompl(iLitLut)) != (pObjAig->fPhase ^ Abc_LitIsCompl(iLitAig) ^ fCompl) )
                printf( "Found phase mismatch for LutId %d and AigId %d.\n", iLut, iObj );
        }
}
int * Abc_FrameReadMiniLutNameMapping( Abc_Frame_t * pAbc )
{
    int fVerbose = 0;
    int nConfs = 1000;
    Gia_Man_t * pGia, * pTemp;
    int * pRes = NULL;
    if ( pAbc->pGiaMiniAig == NULL )
        printf( "GIA derived from MiniAig is not available.\n" );
    if ( pAbc->pGiaMiniLut == NULL )
        printf( "GIA derived from MiniLut is not available.\n" );
    if ( pAbc->pGiaMiniAig == NULL || pAbc->pGiaMiniLut == NULL )
        return NULL;
    pGia = Gia_ManDup2( pAbc->pGiaMiniAig, pAbc->pGiaMiniLut );
    //Gia_AigerWrite( pGia, "aig_m_lut.aig", 0, 0 );
    // compute equivalences in this AIG
    pTemp = Gia_ManComputeGiaEquivs( pGia, nConfs, fVerbose );
    Gia_ManStop( pTemp );
    //if ( fVerbose )
    //    Abc_PrintTime( 1, "Equivalence computation time", Abc_Clock() - clk );
    //if ( fVerbose )
    //    Gia_ManPrintStats( pGia, NULL );
    //Vec_IntPrint( pAbc->vCopyMiniAig );
    //Vec_IntPrint( pAbc->vCopyMiniLut );
    pRes = Gia_ManMapMiniLut2MiniAig( pGia, pAbc->pGiaMiniAig, pAbc->pGiaMiniLut, pAbc->vCopyMiniAig, pAbc->vCopyMiniLut );
    //Gia_ManNameMapVerify( pGia, pAbc->pGiaMiniAig, pAbc->pGiaMiniLut, pAbc->vCopyMiniAig, pAbc->vCopyMiniLut, pRes );
    Gia_ManStop( pGia );
    return pRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

