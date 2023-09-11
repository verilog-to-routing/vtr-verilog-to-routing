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

extern int Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash );

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
Gia_Man_t * Gia_ManFromMiniAig( Mini_Aig_t * p, Vec_Int_t ** pvCopies, int fGiaSimple )
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
    if ( fGiaSimple )
        pGia->fGiaSimple = fGiaSimple;
    else
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
    assert( Vec_IntSize(vCopies) == nNodes );
    if ( pvCopies )
        *pvCopies = vCopies;
    else
        Vec_IntFree( vCopies );
    Gia_ManSetRegNum( pGia, Mini_AigRegNum(p) );
    if ( !fGiaSimple )
    {
        pGia = Gia_ManCleanup( pTemp = pGia );
        if ( pvCopies )
            Gia_ManDupRemapLiterals( *pvCopies, pTemp );
        Gia_ManStop( pTemp );
    }
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
    pGia = Gia_ManFromMiniAig( (Mini_Aig_t *)p, &pAbc->vCopyMiniAig, 0 );
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
void Gia_ManReadMiniAigNames( char * pFileName, Gia_Man_t * pGia )
{
    char * filename3 = Abc_UtilStrsavTwo( pFileName, ".ilo" ); 
    FILE * pFile = fopen( filename3, "rb" );
    if ( pFile )
    {
        char Buffer[5000], * pName; int i, iLines = 0;
        Vec_Ptr_t * vTemp = Vec_PtrAlloc( Gia_ManRegNum(pGia) );
        assert( pGia->vNamesIn == NULL );
        pGia->vNamesIn = Vec_PtrAlloc( Gia_ManCiNum(pGia) );
        assert( pGia->vNamesOut == NULL );
        pGia->vNamesOut = Vec_PtrAlloc( Gia_ManCoNum(pGia) );
        while ( fgets(Buffer, 5000, pFile) )
        {
            if ( Buffer[strlen(Buffer)-1] == '\n' )
                Buffer[strlen(Buffer)-1] = 0;
            if ( iLines < Gia_ManPiNum(pGia) )
                Vec_PtrPush( pGia->vNamesIn, Abc_UtilStrsav(Buffer) );
            else if ( iLines < Gia_ManCiNum(pGia) )
                Vec_PtrPush( vTemp, Abc_UtilStrsav(Buffer) );
            else 
                Vec_PtrPush( pGia->vNamesOut, Abc_UtilStrsav(Buffer) );
            iLines++;
        }
        Vec_PtrForEachEntry( char *, vTemp, pName, i )
        {
            Vec_PtrPush( pGia->vNamesIn,  Abc_UtilStrsav(pName) );
            Vec_PtrPush( pGia->vNamesOut, Abc_UtilStrsavTwo(pName, "_in") );
        }
        Vec_PtrFreeFree( vTemp );
        fclose( pFile );
        printf( "Read ILO names into file \"%s\".\n", filename3 );
    }
    ABC_FREE( filename3 );
}
Gia_Man_t * Gia_ManReadMiniAig( char * pFileName, int fGiaSimple )
{
    Mini_Aig_t * p = Mini_AigLoad( pFileName );
    Gia_Man_t * pTemp, * pGia = Gia_ManFromMiniAig( p, NULL, fGiaSimple );
    ABC_FREE( pGia->pName );
    pGia->pName = Extra_FileNameGeneric( pFileName ); 
    Mini_AigStop( p );
    Gia_ManReadMiniAigNames( pFileName, pGia );
    if ( !Gia_ManIsNormalized(pGia) )
    {
        pGia = Gia_ManDupNormalize( pTemp = pGia, 0 );
        ABC_SWAP( Vec_Ptr_t *, pTemp->vNamesIn,  pGia->vNamesIn  );
        ABC_SWAP( Vec_Ptr_t *, pTemp->vNamesOut, pGia->vNamesOut );
        Gia_ManStop( pTemp );
    }
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

  Synopsis    [Converts MiniLUT into GIA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFromMiniLut2( Mini_Lut_t * p, Vec_Int_t ** pvCopies )
{
    Gia_Man_t * pGia;
    Vec_Int_t * vCopies;
    Vec_Int_t * vCover = Vec_IntAlloc( 1000 );
    Vec_Int_t * vLits = Vec_IntAlloc( 100 );
    int i, k, Fan, iGiaLit, nNodes;
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
    pGia->fGiaSimple = 1;
    for ( i = 2; i < nNodes; i++ )
    {
        if ( Mini_LutNodeIsPi( p, i ) )
            iGiaLit = Gia_ManAppendCi(pGia);
        else if ( Mini_LutNodeIsPo( p, i ) )
            iGiaLit = Gia_ManAppendCo(pGia, Vec_IntEntry(vCopies, Mini_LutNodeFanin(p, i, 0)));
        else if ( Mini_LutNodeIsNode( p, i ) )
        {
            unsigned * puTruth = Mini_LutNodeTruth( p, i );
            Vec_IntClear( vLits );
            Mini_LutForEachFanin( p, i, Fan, k )
                Vec_IntPush( vLits, Vec_IntEntry(vCopies, Fan) );
            iGiaLit = Kit_TruthToGia( pGia, puTruth, Vec_IntSize(vLits), vCover, vLits, 0 );
        }
        else assert( 0 );
        Vec_IntPush( vCopies, iGiaLit );
    }
    Vec_IntFree( vCover );
    Vec_IntFree( vLits );
    assert( Vec_IntSize(vCopies) == nNodes );
    if ( pvCopies )
        *pvCopies = vCopies;
    else
        Vec_IntFree( vCopies );
    Gia_ManSetRegNum( pGia, Mini_LutRegNum(p) );
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
    Vec_Int_t * vInvMap = Vec_IntStart( Gia_ManObjNum(pGia) );
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
    Gia_ManConst0(pGia)->Value = 0;
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
            int LutInv, Fanin = Gia_ObjFanin0(pObj)->Value;
            if ( (LutInv = Vec_IntEntry(vInvMap, Fanin)) == 0 )
            {
                LutInv = Mini_LutCreateNode( p, 1, &Fanin, (unsigned *)pTruth );
                Vec_IntWriteEntry( vInvMap, Fanin, LutInv );
                Count++;
            }
            pObj->Value = Mini_LutCreatePo( p, LutInv );
        }
    }
    Vec_IntFree( vInvMap );
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
void Abc_FrameGiaInputMiniLut2( Abc_Frame_t * pAbc, void * p )
{
    if ( pAbc == NULL )
        printf( "ABC framework is not initialized by calling Abc_Start()\n" );
    Vec_IntFreeP( &pAbc->vCopyMiniLut );
    Gia_ManStopP( &pAbc->pGiaMiniLut );
    pAbc->pGiaMiniLut = Gia_ManFromMiniLut2( (Mini_Lut_t *)p, &pAbc->vCopyMiniLut );
//    Abc_FrameUpdateGia( pAbc, pGia );
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
    //Gia_AigerWrite( pGia, "aig_m_lut.aig", 0, 0, 0 );
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
int * Abc_FrameReadMiniLutSwitching( Abc_Frame_t * pAbc )
{
    Vec_Int_t * vSwitching;
    int i, iObj, * pRes = NULL;
    if ( pAbc->pGiaMiniLut == NULL )
    {
        printf( "GIA derived from MiniLut is not available.\n" );
        return NULL;
    }
    vSwitching = Gia_ManComputeSwitchProbs( pAbc->pGiaMiniLut, 48, 16, 0 );
    pRes = ABC_CALLOC( int, Vec_IntSize(pAbc->vCopyMiniLut) );
    Vec_IntForEachEntry( pAbc->vCopyMiniLut, iObj, i )
        if ( iObj >= 0 )
            pRes[i] = (int)(10000*Vec_FltEntry( (Vec_Flt_t *)vSwitching, Abc_Lit2Var(iObj) ));
    Vec_IntFree( vSwitching );
    return pRes;
}
int * Abc_FrameReadMiniLutSwitchingPo( Abc_Frame_t * pAbc )
{
    Vec_Int_t * vSwitching;
    int i, iObj, * pRes = NULL;
    if ( pAbc->pGiaMiniAig == NULL )
    {
        printf( "GIA derived from MiniAIG is not available.\n" );
        return NULL;
    }
    vSwitching = Gia_ManComputeSwitchProbs( pAbc->pGiaMiniAig, 48, 16, 0 );
    pRes = ABC_CALLOC( int, Gia_ManCoNum(pAbc->pGiaMiniAig) );
    Gia_ManForEachCoDriverId( pAbc->pGiaMiniAig, iObj, i )
         pRes[i] = (int)(10000*Vec_FltEntry( (Vec_Flt_t *)vSwitching, iObj ));
    Vec_IntFree( vSwitching );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Returns equivalences of MiniAig nodes.]

  Description [The resulting array contains as many entries as there are objects
  in the initial MiniAIG. If the i-th entry of the array is equal to -1, it means
  that the i-th MiniAIG object is not equivalent to any other object. Otherwise, 
  the i-th entry contains the literal of the representative of the equivalence 
  class of objects, to which the i-th object belongs. The representative is defined 
  as the first object belonging to the equivalence class in the current topological 
  order. It can be the constant 0 node, a flop output or an internal node. It is 
  the user's responsibility to free the resulting array when it is not needed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManMapEquivAfterScorr( Gia_Man_t * p, Vec_Int_t * vMap )
{
    Vec_Int_t * vRes = Vec_IntStartFull( Vec_IntSize(vMap) );
    Vec_Int_t * vGia2Mini = Vec_IntStartFull( Gia_ManObjNum(p) );
    Gia_Obj_t * pObj, * pRepr;
    int i, iObjLit, iReprLit, fCompl, iReprGia, iReprMini;
    Vec_IntForEachEntry( vMap, iObjLit, i )
    {
        if ( iObjLit == -1 )
            continue;
//        if ( Gia_ObjHasRepr(p, Abc_Lit2Var(iObjLit)) && !Gia_ObjProved(p, Abc_Lit2Var(iObjLit)) )
//            continue;
        iReprGia = Gia_ObjReprSelf( p, Abc_Lit2Var(iObjLit) );
        iReprMini = Vec_IntEntry( vGia2Mini, iReprGia );
        if ( iReprMini == -1 )
        {
            Vec_IntWriteEntry( vGia2Mini, iReprGia, i );
            continue;
        }
        if ( iReprMini == i )
            continue;
        assert( iReprMini < i );
        Vec_IntWriteEntry( vRes, i, iReprMini );
    }
    Vec_IntFree( vGia2Mini );
    Gia_ManSetPhase( p );
    Vec_IntForEachEntry( vRes, iReprMini, i )
    {
        if ( iReprMini == -1 )
            continue;
        iObjLit  = Vec_IntEntry(vMap, i);
        iReprLit = Vec_IntEntry(vMap, iReprMini);
        pObj     = Gia_ManObj( p, Abc_Lit2Var(iObjLit) );
        pRepr    = Gia_ManObj( p, Abc_Lit2Var(iReprLit) );
        fCompl   = Abc_LitIsCompl(iObjLit) ^ Abc_LitIsCompl(iReprLit) ^ pObj->fPhase ^ pRepr->fPhase;
        Vec_IntWriteEntry( vRes, i, Abc_Var2Lit(iReprMini, fCompl) );
    }
    return vRes;
}
int * Abc_FrameReadMiniAigEquivClasses( Abc_Frame_t * pAbc )
{
    Vec_Int_t * vRes;
    int * pRes;
    if ( pAbc->pGiaMiniAig == NULL )
        printf( "GIA derived from MiniAig is not available.\n" );
    if ( pAbc->vCopyMiniAig == NULL )
        printf( "Mapping of MiniAig nodes is not available.\n" );
    if ( pAbc->pGia2 == NULL )
        printf( "Internal GIA with equivalence classes is not available.\n" );
    if ( pAbc->pGia2->pReprs == NULL )
    {
        printf( "Equivalence classes of internal GIA are not available.\n" );
        return NULL;
    }
    else if ( 0 )
    {
        int i;
        for ( i = 1; i < Gia_ManObjNum(pAbc->pGia2); i++ )
            if ( Gia_ObjHasRepr(pAbc->pGia2, i) )
                printf( "Obj %3d : Repr %3d   Proved %d   Failed %d\n", i, Gia_ObjRepr(pAbc->pGia2, i), Gia_ObjProved(pAbc->pGia2, i), Gia_ObjFailed(pAbc->pGia2, i) );
    }
    if ( Gia_ManObjNum(pAbc->pGia2) != Gia_ManObjNum(pAbc->pGiaMiniAig) )
        printf( "Internal GIA with equivalence classes is not directly derived from MiniAig.\n" );
    // derive the set of equivalent node pairs
    vRes = Gia_ManMapEquivAfterScorr( pAbc->pGia2, pAbc->vCopyMiniAig );
    pRes = Vec_IntReleaseArray( vRes );
    Vec_IntFree( vRes );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Verifies equivalences of MiniAig nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_MiniAigReduce( Mini_Aig_t * p, int * pEquivs )
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
        if ( pEquivs[i] != -1 )
            iGiaLit = Abc_LitNotCond( Vec_IntEntry(vCopies, Abc_Lit2Var(pEquivs[i])), Abc_LitIsCompl(pEquivs[i]) );
        Vec_IntPush( vCopies, iGiaLit );
    }
    Gia_ManHashStop( pGia );
    assert( Vec_IntSize(vCopies) == nNodes );
    Vec_IntFree( vCopies );
    Gia_ManSetRegNum( pGia, Mini_AigRegNum(p) );
    pGia = Gia_ManSeqCleanup( pTemp = pGia );
    Gia_ManStop( pTemp );
    return pGia;
}
Gia_Man_t * Gia_MiniAigMiter( Mini_Aig_t * p, int * pEquivs )
{
    Gia_Man_t * pGia, * pTemp;
    Vec_Int_t * vCopies;
    int i, iGiaLit = 0, iGiaLit2, nNodes, iPos = 0, nPos = 0, Temp;
    // get the number of nodes
    nNodes = Mini_AigNodeNum(p);
    // create ABC network
    pGia = Gia_ManStart( 2 * nNodes );
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
        {
            nPos++;
            Vec_IntPush( vCopies, -1 );
            continue;
        }
        else if ( Mini_AigNodeIsAnd( p, i ) )
            iGiaLit = Gia_ManHashAnd(pGia, Gia_ObjFromMiniFanin0Copy(pGia, vCopies, p, i), Gia_ObjFromMiniFanin1Copy(pGia, vCopies, p, i));
        else assert( 0 );
        Vec_IntPush( vCopies, iGiaLit );
    }
    assert( Vec_IntSize(vCopies) == nNodes );
    assert( nPos > Mini_AigRegNum(p) );
    // create miters for each equiv class
    for ( i = 1; i < nNodes; i++ )
    {
        if ( pEquivs[i] == -1 )
            continue;
        iGiaLit = Vec_IntEntry(vCopies, i);
        iGiaLit2 = Abc_LitNotCond( Vec_IntEntry(vCopies, Abc_Lit2Var(pEquivs[i])), Abc_LitIsCompl(pEquivs[i]) );
        Gia_ManAppendCo( pGia, Gia_ManHashXor(pGia, iGiaLit, iGiaLit2) );
    }
    // create flop inputs
    Temp = Gia_ManCoNum(pGia);
    for ( i = 1; i < nNodes; i++ )
    {
        if ( !Mini_AigNodeIsPo( p, i ) )
            continue;
        if ( iPos++ >= nPos - Mini_AigRegNum(p) )
            Gia_ManAppendCo(pGia, Gia_ObjFromMiniFanin0Copy(pGia, vCopies, p, i));
    }
    assert( iPos == nPos );
    assert( Mini_AigRegNum(p) == Gia_ManCoNum(pGia) - Temp );
    Gia_ManSetRegNum( pGia, Mini_AigRegNum(p) );
    Gia_ManHashStop( pGia );
    Vec_IntFree( vCopies );
    pGia = Gia_ManCleanup( pTemp = pGia );
    Gia_ManStop( pTemp );
    return pGia;
}
void Gia_MiniAigVerify( Abc_Frame_t * pAbc, char * pFileName )
{
    int * pEquivs;
    Gia_Man_t * pGia;
    char * pFileMiter   = "mini_aig_miter.aig";
    char * pFileReduced = "mini_aig_reduced.aig";
    Mini_Aig_t * p = Mini_AigLoad( pFileName );
    Abc_FrameGiaInputMiniAig( pAbc, p );
    Cmd_CommandExecute( pAbc, "&ps; &scorr; &ps" );
    pEquivs = Abc_FrameReadMiniAigEquivClasses( pAbc );
    // dump miter for verification
    pGia = Gia_MiniAigMiter( p, pEquivs );
    Gia_AigerWrite( pGia, pFileMiter, 0, 0, 0 );
    printf( "Dumped miter AIG in file \"%s\".\n", pFileMiter );
    Gia_ManStop( pGia );
    // dump reduced AIG
    pGia = Gia_MiniAigReduce( p, pEquivs );
    Gia_AigerWrite( pGia, pFileReduced, 0, 0, 0 );
    printf( "Dumped reduced AIG in file \"%s\".\n", pFileReduced );
    Gia_ManStop( pGia );
    // cleanup
    ABC_FREE( pEquivs );
    Mini_AigStop( p );
}

/**Function*************************************************************

  Synopsis    [Collects supergate for the outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MiniAigSuperGates_rec( Mini_Aig_t * p, int iObj, Vec_Int_t * vRes, Vec_Int_t * vMap )
{
    int iFan0, iFan1;
    if ( Mini_AigNodeIsPi(p, iObj) )
    {
        assert( Vec_IntEntry(vMap, iObj) >= 0 );
        Vec_IntPush( vRes, Vec_IntEntry(vMap, iObj) );
        return;
    }
    iFan0 = Mini_AigNodeFanin0( p, iObj );
    iFan1 = Mini_AigNodeFanin1( p, iObj );
    assert( !Abc_LitIsCompl(iFan0) );
    assert( !Abc_LitIsCompl(iFan1) );
    Gia_MiniAigSuperGates_rec( p, Abc_Lit2Var(iFan0), vRes, vMap );
    Gia_MiniAigSuperGates_rec( p, Abc_Lit2Var(iFan1), vRes, vMap );
}
Vec_Wec_t * Gia_MiniAigSuperGates( Mini_Aig_t * p )
{
    Vec_Wec_t * vRes = Vec_WecStart( Mini_AigPoNum(p) );
    Vec_Int_t * vMap = Vec_IntStartFull( Mini_AigNodeNum(p) );
    int i, Index = 0;
    Mini_AigForEachPi( p, i )
        Vec_IntWriteEntry( vMap, i, Index++ );
    assert( Index == Mini_AigPiNum(p) );
    Index = 0;
    Mini_AigForEachPo( p, i )
    {
        int iFan0 = Mini_AigNodeFanin0( p, i );
        assert( !Abc_LitIsCompl(iFan0) );
        Gia_MiniAigSuperGates_rec( p, Abc_Lit2Var(iFan0), Vec_WecEntry(vRes, Index++), vMap );
    }
    assert( Index == Mini_AigPoNum(p) );
    Vec_IntFree( vMap );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Transform.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_MiniAigSuperPrintDouble( Vec_Int_t * p, int nPis )
{
    int i, Entry;
    printf( "\n" );
    Vec_IntForEachEntry( p, Entry, i )
        printf( "%d(%d) ", Entry%nPis, Entry/nPis );
    printf( "  Total = %d\n", Vec_IntSize(p) );
}
int Gia_MiniAigSuperMerge( Vec_Int_t * p, int nPis )
{
    int i, k = 0, This, Prev = -1, fChange = 0;
    Vec_IntForEachEntry( p, This, i )
    {
        if ( Prev == This )
        {
            Vec_IntWriteEntry( p, k++, (This/nPis+1)*nPis + This%nPis );
            Prev = -1;
            fChange = 1;
        }
        else 
        {
            if ( Prev != -1 ) 
                Vec_IntWriteEntry( p, k++, Prev );
            Prev = This;
        }
    }
    if ( Prev != -1 )
        Vec_IntWriteEntry( p, k++, Prev );
    Vec_IntShrink( p, k );
    return fChange;
}
int Gia_MiniAigSuperPreprocess( Mini_Aig_t * p, Vec_Wec_t * vSuper, int nPis, int fVerbose )
{
    Vec_Int_t * vRes;
    int i, nIters, Multi = 1;
    Vec_WecForEachLevel( vSuper, vRes, i )
    {
        Vec_IntSort( vRes, 0 );
        if ( fVerbose ) 
            printf( "\nOutput %d\n", i );
        if ( fVerbose )
            Gia_MiniAigSuperPrintDouble( vRes, nPis );
        for ( nIters = 1; Gia_MiniAigSuperMerge(vRes, nPis); nIters++ )
        {
            if ( fVerbose )
                Gia_MiniAigSuperPrintDouble( vRes, nPis );
        }
        Multi = Abc_MaxInt( Multi, nIters );
    }
    if ( fVerbose )
        printf( "Multi = %d.\n", Multi );
    return Multi;
}

/**Function*************************************************************

  Synopsis    [Derive AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_MiniAigSuperDeriveGia( Vec_Wec_t * p, int nPis, int Multi )
{
    Gia_Man_t * pNew;
    Vec_Int_t * vTemp, * vLits = Vec_IntAlloc( 100 );
    Vec_Int_t * vDrivers = Vec_IntAlloc(100);
    int i, k, iObj, iLit, nInputs = nPis*Multi;
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "tree" );
    for ( i = 0; i < nInputs; i++ )
        Gia_ManAppendCi( pNew );
    Gia_ManHashAlloc( pNew );
    Vec_WecForEachLevel( p, vTemp, i )
    {
        Vec_IntClear( vLits );
        Vec_IntForEachEntry( vTemp, iObj, k )
        {
            assert( iObj < nInputs );
            Vec_IntPush( vLits, 2+2*((iObj%nPis)*Multi+iObj/nPis) );
        }
        Vec_IntPush( vDrivers, Gia_ManHashAndMulti2(pNew, vLits) );
    }
    Gia_ManHashStop( pNew );
    Vec_IntFree( vLits );
    Vec_IntForEachEntry( vDrivers, iLit, i )
        Gia_ManAppendCo( pNew, iLit );
    Vec_IntFree( vDrivers );
    return pNew;
}
Gia_Man_t * Gia_MiniAigSuperDerive( char * pFileName, int fVerbose )
{
    Mini_Aig_t * p     = Mini_AigLoad( pFileName );
    Vec_Wec_t * vSuper = Gia_MiniAigSuperGates( p );
    int Multi          = Gia_MiniAigSuperPreprocess( p, vSuper, Mini_AigPiNum(p), fVerbose );
    Gia_Man_t * pNew   = Gia_MiniAigSuperDeriveGia( vSuper, Mini_AigPiNum(p), Multi );
    Vec_WecFree( vSuper );
    Mini_AigStop( p );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Process file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_MiniAigProcessFile()
{
    Vec_Int_t * vTriples = Vec_IntAlloc( 100 );
    char * pFileName = "test.txt";
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
        printf( "Cannot open the file.\n" );
    else
    {
        int nLines = 0, nLinesAll = 0;
        char * pToken;
        char Buffer[1000];
        while ( fgets( Buffer, 1000, pFile ) != NULL )
        {
            nLinesAll++;
            if ( Buffer[0] != '#' )
                continue;
            //printf( "%s", Buffer );
            nLines++;
            pToken = strtok( Buffer+3, " \r\n\r+=" );
            while ( pToken )
            {
                Vec_IntPush( vTriples, atoi(pToken) );
                pToken = strtok( NULL, " \r\n\r+=" );
            }
        }
        fclose( pFile );
        printf( "Collected %d (out of %d) lines.\n", nLines, nLinesAll );
        printf( "Entries = %d\n", Vec_IntSize(vTriples) );
    }
    return vTriples;
}
void Gia_MiniAigGenerate_rec( Mini_Aig_t * p, Vec_Int_t * vTriples, int iObj, Vec_Int_t * vDefs, Vec_Int_t * vMap )
{
    int Index, Entry0, Entry1, Entry2, Value;
    if ( Vec_IntEntry(vMap, iObj) >= 0 )
        return;
    Index  = Vec_IntEntry( vDefs, iObj );
    Entry0 = Vec_IntEntry( vTriples, 3*Index+0 );
    Entry1 = Vec_IntEntry( vTriples, 3*Index+1 );
    Entry2 = Vec_IntEntry( vTriples, 3*Index+2 );
    Gia_MiniAigGenerate_rec( p, vTriples, Entry1, vDefs, vMap );
    Gia_MiniAigGenerate_rec( p, vTriples, Entry2, vDefs, vMap );
    assert( Vec_IntEntry(vMap, Entry1) >= 0 );
    assert( Vec_IntEntry(vMap, Entry2) >= 0 );
    Value  = Mini_AigAnd( p, Vec_IntEntry(vMap, Entry1), Vec_IntEntry(vMap, Entry2) );
    Vec_IntWriteEntry( vMap, Entry0, Value );
}
void Gia_MiniAigGenerateFromFile()
{
    Mini_Aig_t * p = Mini_AigStart();
    Vec_Int_t * vTriples = Gia_MiniAigProcessFile();
    Vec_Int_t * vDefs    = Vec_IntStartFull( Vec_IntSize(vTriples) ); 
    Vec_Int_t * vMap     = Vec_IntStartFull( Vec_IntSize(vTriples) ); 
    Vec_Int_t * vMapIn   = Vec_IntStart( Vec_IntSize(vTriples) );
    Vec_Int_t * vMapOut  = Vec_IntStart( Vec_IntSize(vTriples) );
    Vec_Int_t * vPis = Vec_IntAlloc( 100 );
    Vec_Int_t * vPos = Vec_IntAlloc( 100 );
    int i, ObjOut, ObjIn;
    assert( Vec_IntSize(vTriples) % 3 == 0 );
    for ( i = 0; i < Vec_IntSize(vTriples)/3; i++ )
    {
        int Entry0 = Vec_IntEntry(vTriples, 3*i+0);
        int Entry1 = Vec_IntEntry(vTriples, 3*i+1);
        int Entry2 = Vec_IntEntry(vTriples, 3*i+2);
        Vec_IntWriteEntry( vDefs,   Entry0, i );
        Vec_IntAddToEntry( vMapOut, Entry0, 1 );
        Vec_IntAddToEntry( vMapIn,  Entry1, 1 );
        Vec_IntAddToEntry( vMapIn,  Entry2, 1 );
    }
    Vec_IntForEachEntryTwo( vMapOut, vMapIn, ObjOut, ObjIn, i )
        if ( !ObjOut && ObjIn )
            Vec_IntPush( vPis, i );
        else if ( ObjOut && !ObjIn )
            Vec_IntPush( vPos, i );
    Vec_IntForEachEntry( vPis, ObjIn, i )
        Vec_IntWriteEntry( vMap, ObjIn, Mini_AigCreatePi(p) );
    Vec_IntForEachEntry( vPos, ObjOut, i )
        Gia_MiniAigGenerate_rec( p, vTriples, ObjOut, vDefs, vMap );
    Vec_IntForEachEntry( vPos, ObjOut, i )
    {
        assert( Vec_IntEntry(vMap, ObjOut) >= 0 );
        Mini_AigCreatePo( p, Vec_IntEntry(vMap, ObjOut) );
    }
    Vec_IntFree( vTriples );
    Vec_IntFree( vDefs );
    Vec_IntFree( vMap );
    Vec_IntFree( vMapIn );
    Vec_IntFree( vMapOut );
    Vec_IntFree( vPis );
    Vec_IntFree( vPos );
    Mini_AigDump( p, "test.miniaig" );
    Mini_AigStop( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

