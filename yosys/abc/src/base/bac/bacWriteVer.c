/**CFile****************************************************************

  FileName    [bacWriteVer.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Verilog writer.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacWriteVer.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"
#include "bacPrs.h"
#include "map/mio/mio.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writing parser state into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Psr_ManWriteVerilogConcat( FILE * pFile, Psr_Ntk_t * p, int Con )
{
    extern void Psr_ManWriteVerilogArray( FILE * pFile, Psr_Ntk_t * p, Vec_Int_t * vSigs, int Start, int Stop, int fOdd );
    Vec_Int_t * vSigs = Psr_CatSignals(p, Con);
    fprintf( pFile, "{" );
    Psr_ManWriteVerilogArray( pFile, p, vSigs, 0, Vec_IntSize(vSigs), 0 );
    fprintf( pFile, "}" );
}
static void Psr_ManWriteVerilogSignal( FILE * pFile, Psr_Ntk_t * p, int Sig )
{
    int Value = Abc_Lit2Var2( Sig );
    Psr_ManType_t Type = (Psr_ManType_t)Abc_Lit2Att2( Sig );
    if ( Type == BAC_PRS_NAME || Type == BAC_PRS_CONST )
        fprintf( pFile, "%s", Psr_NtkStr(p, Value) );
    else if ( Type == BAC_PRS_SLICE )
        fprintf( pFile, "%s%s", Psr_NtkStr(p, Psr_SliceName(p, Value)), Psr_NtkStr(p, Psr_SliceRange(p, Value)) );
    else if ( Type == BAC_PRS_CONCAT )
        Psr_ManWriteVerilogConcat( pFile, p, Value );
    else assert( 0 );
}
void Psr_ManWriteVerilogArray( FILE * pFile, Psr_Ntk_t * p, Vec_Int_t * vSigs, int Start, int Stop, int fOdd )
{
    int i, Sig;
    assert( Vec_IntSize(vSigs) > 0 );
    Vec_IntForEachEntryStartStop( vSigs, Sig, i, Start, Stop )
    {
        if ( fOdd && !(i & 1) )
            continue;
        Psr_ManWriteVerilogSignal( pFile, p, Sig );
        fprintf( pFile, "%s", i == Stop - 1 ? "" : ", " );
    }
}
static void Psr_ManWriteVerilogArray2( FILE * pFile, Psr_Ntk_t * p, Vec_Int_t * vSigs )
{
    int i, FormId, ActSig;
    assert( Vec_IntSize(vSigs) % 2 == 0 );
    Vec_IntForEachEntryDouble( vSigs, FormId, ActSig, i )
    {
        fprintf( pFile, "." );
        fprintf( pFile, "%s", Psr_NtkStr(p, FormId) );
        fprintf( pFile, "(" );
        Psr_ManWriteVerilogSignal( pFile, p, ActSig );
        fprintf( pFile, ")%s", (i == Vec_IntSize(vSigs) - 2) ? "" : ", " );
    }
}
static void Psr_ManWriteVerilogMux( FILE * pFile, Psr_Ntk_t * p, Vec_Int_t * vSigs )
{
    int i, FormId, ActSig;
    char * pStrs[4] = { " = ", " ? ", " : ", ";\n" };
    assert( Vec_IntSize(vSigs) == 8 );
    fprintf( pFile, "  assign " );
    Psr_ManWriteVerilogSignal( pFile, p, Vec_IntEntryLast(vSigs) );
    fprintf( pFile, "%s", pStrs[0] );
    Vec_IntForEachEntryDouble( vSigs, FormId, ActSig, i )
    {
        Psr_ManWriteVerilogSignal( pFile, p, ActSig );
        fprintf( pFile, "%s", pStrs[1+i/2] );
        if ( i == 4 )
            break;
    }
}
static void Psr_ManWriteVerilogBoxes( FILE * pFile, Psr_Ntk_t * p )
{
    Vec_Int_t * vBox; int i;
    Psr_NtkForEachBox( p, vBox, i )
    {
        Bac_ObjType_t NtkId = (Bac_ObjType_t)Psr_BoxNtk(p, i);
        if ( NtkId == BAC_BOX_MUX )
            Psr_ManWriteVerilogMux( pFile, p, vBox );
        else if ( Psr_BoxIsNode(p, i) ) // node   ------- check order of fanins
        {
            fprintf( pFile, "  %s (", Ptr_TypeToName(NtkId) );
            Psr_ManWriteVerilogSignal( pFile, p, Vec_IntEntryLast(vBox) );
            if ( Psr_BoxIONum(p, i) > 1 )
                fprintf( pFile, ", " );                
            Psr_ManWriteVerilogArray( pFile, p, vBox, 0, Vec_IntSize(vBox)-2, 1 );
            fprintf( pFile, ");\n" );
        }
        else // box
        {
            //char * s = Psr_NtkStr(p, Vec_IntEntry(vBox, 0));
            fprintf( pFile, "  %s %s (", Psr_NtkStr(p, NtkId), Psr_BoxName(p, i) ? Psr_NtkStr(p, Psr_BoxName(p, i)) : "" );
            Psr_ManWriteVerilogArray2( pFile, p, vBox );
            fprintf( pFile, ");\n" );
        }
    }
}
static void Psr_ManWriteVerilogIos( FILE * pFile, Psr_Ntk_t * p, int SigType )
{
    int NameId, RangeId, i;
    char * pSigNames[4]   = { "inout", "input", "output", "wire" }; 
    Vec_Int_t * vSigs[4]  = { &p->vInouts,  &p->vInputs,  &p->vOutputs,  &p->vWires };
    Vec_Int_t * vSigsR[4] = { &p->vInoutsR, &p->vInputsR, &p->vOutputsR, &p->vWiresR };
    if ( SigType == 3 )
        fprintf( pFile, "\n" );
    Vec_IntForEachEntryTwo( vSigs[SigType], vSigsR[SigType], NameId, RangeId, i )
        fprintf( pFile, "  %s %s%s;\n", pSigNames[SigType], RangeId ? Psr_NtkStr(p, RangeId) : "", Psr_NtkStr(p, NameId) );
}
static void Psr_ManWriteVerilogIoOrder( FILE * pFile, Psr_Ntk_t * p, Vec_Int_t * vOrder )
{
    int i, NameId;
    Vec_IntForEachEntry( vOrder, NameId, i )
        fprintf( pFile, "%s%s", Psr_NtkStr(p, NameId), i == Vec_IntSize(vOrder) - 1 ? "" : ", " );
}
static void Psr_ManWriteVerilogNtk( FILE * pFile, Psr_Ntk_t * p )
{
    int s;
    // write header
    fprintf( pFile, "module %s (\n    ", Psr_NtkStr(p, p->iModuleName) );
    Psr_ManWriteVerilogIoOrder( pFile, p, &p->vOrder );
    fprintf( pFile, "\n  );\n" );
    // write declarations
    for ( s = 0; s < 4; s++ )
        Psr_ManWriteVerilogIos( pFile, p, s );
    fprintf( pFile, "\n" );
    // write objects
    Psr_ManWriteVerilogBoxes( pFile, p );
    fprintf( pFile, "endmodule\n\n" );
}
void Psr_ManWriteVerilog( char * pFileName, Vec_Ptr_t * vPrs )
{
    Psr_Ntk_t * pNtk = Psr_ManRoot(vPrs); int i;
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open output file \"%s\".\n", pFileName );
        return;
    }
    fprintf( pFile, "// Design \"%s\" written by ABC on %s\n\n", Psr_NtkStr(pNtk, pNtk->iModuleName), Extra_TimeStamp() );
    Vec_PtrForEachEntry( Psr_Ntk_t *, vPrs, pNtk, i )
        Psr_ManWriteVerilogNtk( pFile, pNtk );
    fclose( pFile );
}



/**Function*************************************************************

  Synopsis    [Writing word-level Verilog.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// compute range of a name (different from range of a multi-bit wire)
static inline int Bac_ObjGetRange( Bac_Ntk_t * p, int iObj )
{
    int i, NameId = Bac_ObjName(p, iObj);
    assert( Bac_ObjIsCi(p, iObj) );
//    if ( Bac_NameType(NameId) == BAC_NAME_INDEX )
//        NameId = Bac_ObjName(p, iObj - Abc_Lit2Var2(NameId));
    assert( Bac_NameType(NameId) == BAC_NAME_WORD || Bac_NameType(NameId) == BAC_NAME_INFO );
    for ( i = iObj + 1; i < Bac_NtkObjNum(p); i++ )
        if ( !Bac_ObjIsCi(p, i) || Bac_ObjNameType(p, i) != BAC_NAME_INDEX )
            break;
    return i - iObj;
}

static inline void Bac_ManWriteVar( Bac_Ntk_t * p, int RealName )
{
    Vec_StrPrintStr( p->pDesign->vOut, Bac_NtkStr(p, RealName) );
}
static inline void Bac_ManWriteRange( Bac_Ntk_t * p, int Beg, int End )
{
    Vec_Str_t * vStr = p->pDesign->vOut;
    Vec_StrPrintStr( vStr, "[" );
    if ( End >= 0 )
    {
        Vec_StrPrintNum( vStr, End );
        Vec_StrPrintStr( vStr, ":" );
    }
    Vec_StrPrintNum( vStr, Beg );
    Vec_StrPrintStr( vStr, "]" );
}
static inline void Bac_ManWriteConstBit( Bac_Ntk_t * p, int iObj, int fHead )
{
    Vec_Str_t * vStr = p->pDesign->vOut;
    int Const = Bac_ObjGetConst(p, iObj);
    assert( Const );
    if ( fHead )
        Vec_StrPrintStr( vStr, "1\'b" );
    if ( Const == BAC_BOX_CF )
        Vec_StrPush( vStr, '0' );
    else if ( Const == BAC_BOX_CT )
        Vec_StrPush( vStr, '1' );
    else if ( Const == BAC_BOX_CX )
        Vec_StrPush( vStr, 'x' );
    else if ( Const == BAC_BOX_CZ )
        Vec_StrPush( vStr, 'z' );
    else assert( 0 );
}
static inline int Bac_ManFindRealNameId( Bac_Ntk_t * p, int iObj )
{
    int NameId = Bac_ObjName(p, iObj);
    assert( Bac_ObjIsCi(p, iObj) );
    if ( Bac_NameType(NameId) == BAC_NAME_INDEX )
        NameId = Bac_ObjName(p, iObj - Abc_Lit2Var2(NameId));
    if ( Bac_NameType(NameId) == BAC_NAME_INFO )
        return Bac_NtkInfoName(p, Abc_Lit2Var2(NameId));
    assert( Bac_NameType(NameId) == BAC_NAME_BIN || Bac_NameType(NameId) == BAC_NAME_WORD );
    return Abc_Lit2Var2(NameId);
}
static inline int Bac_ManFindRealIndex( Bac_Ntk_t * p, int iObj )
{
    int iBit = 0, NameId = Bac_ObjName(p, iObj);
    assert( Bac_ObjIsCi(p, iObj) );
    assert( Bac_NameType(NameId) != BAC_NAME_BIN );
    if ( Bac_NameType(NameId) == BAC_NAME_INDEX )
        NameId = Bac_ObjName(p, iObj - (iBit = Abc_Lit2Var2(NameId)));
    if ( Bac_NameType(NameId) == BAC_NAME_INFO )
        return Bac_NtkInfoIndex(p, Abc_Lit2Var2(NameId), iBit);
    assert( Bac_NameType(NameId) == BAC_NAME_WORD );
    return iBit;
}
static inline void Bac_ManWriteSig( Bac_Ntk_t * p, int iObj )
{
    if ( Bac_ObjIsCo(p, iObj) )
        iObj = Bac_ObjFanin(p, iObj);
    assert( Bac_ObjIsCi(p, iObj) );
    if ( Bac_ObjGetConst(p, iObj) )
        Bac_ManWriteConstBit( p, iObj, 1 );
    else
    {
        int NameId = Bac_ObjName(p, iObj);
        if ( Bac_NameType(NameId) == BAC_NAME_BIN )
            Bac_ManWriteVar( p, Abc_Lit2Var2(NameId) );
        else
        {
            Bac_ManWriteVar( p, Bac_ManFindRealNameId(p, iObj) );
            Bac_ManWriteRange( p, Bac_ManFindRealIndex(p, iObj), -1 );
        }
    }
}
static inline void Bac_ManWriteConcat( Bac_Ntk_t * p, int iStart, int nObjs )
{
    Vec_Str_t * vStr = p->pDesign->vOut; 
    assert( nObjs >= 1 );
    if ( nObjs == 1 )
    {
        Bac_ManWriteSig( p, iStart );
        return;
    }
    Vec_StrPrintStr( vStr, "{" );
    if ( Bac_ObjIsBo(p, iStart) ) // box output
    {
        int i;
        for ( i = iStart + nObjs - 1; i >= iStart; i-- )
        {
            if ( Bac_ObjNameType(p, i) == BAC_NAME_INDEX )
                continue;
            if ( Vec_StrEntryLast(vStr) != '{' )
                Vec_StrPrintStr( vStr, ", " );
            Bac_ManWriteVar( p, Bac_ManFindRealNameId(p, i) );
        }
    }
    else if ( Bac_ObjIsBi(p, iStart) ) // box input
    {
        int e, b, k, NameId;
        for ( e = iStart - nObjs + 1; e <= iStart; )
        {
            if ( Vec_StrEntryLast(vStr) != '{' )
                Vec_StrPrintStr( vStr, ", " );
            // write constant
            if ( Bac_ObjGetConst(p, Bac_ObjFanin(p, e)) )
            {
                int fBinary = Bac_ObjIsConstBin(p, Bac_ObjFanin(p, e)-1);                
                for ( b = e + 1; b <= iStart; b++ )
                {
                    if ( !Bac_ObjGetConst(p, Bac_ObjFanin(p, b)) )
                        break;
                    if ( !Bac_ObjIsConstBin(p, Bac_ObjFanin(p, b)-1) )
                        fBinary = 0;
                }
                Vec_StrPrintNum( vStr, b - e );
                if ( fBinary && b - e > 8 ) // write hex if more than 8 bits
                {
                    int Digit = 0, nBits = ((b - e) & 3) ? (b - e) & 3 : 4;
                    Vec_StrPrintStr( vStr, "\'h" );
                    for ( k = e; k < b; k++ )
                    {
                        Digit = 2*Digit + Bac_ObjGetConst(p, Bac_ObjFanin(p, k)) - BAC_BOX_CF;
                        assert( Digit < 16 );
                        if ( --nBits == 0 )
                        {
                            Vec_StrPush( vStr, (char)(Digit < 10 ? '0' + Digit : 'a' + Digit - 10) );
                            nBits = 4;
                            Digit = 0;
                        }
                    }
                    assert( nBits == 4 );
                    assert( Digit == 0 );
                }
                else
                {
                    Vec_StrPrintStr( vStr, "\'b" );
                    for ( k = e; k < b; k++ )
                        Bac_ManWriteConstBit( p, Bac_ObjFanin(p, k), 0 );
                }
                e = b;
                continue;
            }
            // try replication
            for ( b = e + 1; b <= iStart; b++ )
                if ( Bac_ObjFanin(p, b) != Bac_ObjFanin(p, e) )
                    break;
            if ( b > e + 2 ) // more than two
            {
                Vec_StrPrintNum( vStr, b - e );
                Vec_StrPrintStr( vStr, "{" );
                Bac_ManWriteSig( p, e );
                Vec_StrPrintStr( vStr, "}" );
                e = b;
                continue;
            }
            NameId = Bac_ObjName(p, Bac_ObjFanin(p, e));
            if ( Bac_NameType(NameId) == BAC_NAME_BIN )
            {
                Bac_ManWriteVar( p, Abc_Lit2Var2(NameId) );
                e++;
                continue;
            }
            // find end of the slice
            for ( b = e + 1; b <= iStart; b++ )
                if ( Bac_ObjFanin(p, e) - Bac_ObjFanin(p, b) != b - e )
                    break;
            // write signal name
            Bac_ManWriteVar( p, Bac_ManFindRealNameId(p, Bac_ObjFanin(p, e)) );
            if ( b == e + 1 ) // literal
                Bac_ManWriteRange( p, Bac_ManFindRealIndex(p, Bac_ObjFanin(p, e)), -1 );
            else // slice or complete variable
            {
                // consider first variable of the slice
                int f = Bac_ObjFanin( p, b-1 );
                assert( Bac_ObjNameType(p, f) != BAC_NAME_BIN );
                if ( Bac_ObjNameType(p, f) == BAC_NAME_INDEX || Bac_ObjGetRange(p, f) != b - e ) // slice
                    Bac_ManWriteRange( p, Bac_ManFindRealIndex(p, f), Bac_ManFindRealIndex(p, Bac_ObjFanin(p, e)) );
                // else this is complete variable
            }
            e = b;
        }
    }
    else assert( 0 );
    Vec_StrPrintStr( vStr, "}" );
}
static inline void Bac_ManWriteGate( Bac_Ntk_t * p, int iObj )
{
    Vec_Str_t * vStr = p->pDesign->vOut; int iTerm, k;
    char * pGateName = Abc_NamStr(p->pDesign->pMods, Bac_BoxNtkId(p, iObj));
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
    Mio_Gate_t * pGate = Mio_LibraryReadGateByName( pLib, pGateName, NULL );
    Vec_StrPrintStr( vStr, "  " );
    Vec_StrPrintStr( vStr, pGateName );
    Vec_StrPrintStr( vStr, " " );
    Vec_StrPrintStr( vStr, Bac_ObjName(p, iObj) ? Bac_ObjNameStr(p, iObj) : "" );
    Vec_StrPrintStr( vStr, " (" );
    Bac_BoxForEachBi( p, iObj, iTerm, k )
    {
        Vec_StrPrintStr( vStr, k ? ", ." : "." );
        Vec_StrPrintStr( vStr, Mio_GateReadPinName(pGate, k) );
        Vec_StrPrintStr( vStr, "(" );
        Bac_ManWriteSig( p, iTerm );
        Vec_StrPrintStr( vStr, ")" );
    }
    Bac_BoxForEachBo( p, iObj, iTerm, k )
    {
        Vec_StrPrintStr( vStr, Bac_BoxBiNum(p, iObj) ? ", ." : "." );
        Vec_StrPrintStr( vStr, Mio_GateReadOutName(pGate) );
        Vec_StrPrintStr( vStr, "(" );
        Bac_ManWriteSig( p, iTerm );
        Vec_StrPrintStr( vStr, ")" );
    }
    Vec_StrPrintStr( vStr, ");\n" );
}
static inline void Bac_ManWriteAssign( Bac_Ntk_t * p, int iObj )
{
    Vec_Str_t * vStr = p->pDesign->vOut;
    Bac_ObjType_t Type = Bac_ObjType(p, iObj);
    int nInputs = Bac_BoxBiNum(p, iObj);
    int nOutputs = Bac_BoxBoNum(p, iObj);
    assert( nOutputs == 1 );
    Vec_StrPrintStr( vStr, "  assign " );
    Bac_ManWriteSig( p, iObj + 1 );
    Vec_StrPrintStr( vStr, " = " );
    if ( nInputs == 0 )
    {
        if ( Type == BAC_BOX_CF )
            Vec_StrPrintStr( vStr, "1\'b0" );
        else if ( Type == BAC_BOX_CT )
            Vec_StrPrintStr( vStr, "1\'b1" );
        else if ( Type == BAC_BOX_CX )
            Vec_StrPrintStr( vStr, "1\'bx" );
        else if ( Type == BAC_BOX_CZ )
            Vec_StrPrintStr( vStr, "1\'bz" );
        else assert( 0 );       
    }
    else if ( nInputs == 1 )
    {
        if ( Type == BAC_BOX_INV )
            Vec_StrPrintStr( vStr, "~" );
        else assert( Type == BAC_BOX_BUF );
        Bac_ManWriteSig( p, iObj - 1 );
    }
    else if ( nInputs == 2 )
    {
        if ( Type == BAC_BOX_NAND || Type == BAC_BOX_NOR || Type == BAC_BOX_XNOR || Type == BAC_BOX_SHARPL )
            Vec_StrPrintStr( vStr, "~" );
        Bac_ManWriteSig( p, iObj - 1 );
        if ( Type == BAC_BOX_AND || Type == BAC_BOX_SHARPL )
            Vec_StrPrintStr( vStr, " & " );
        else if ( Type == BAC_BOX_SHARP || Type == BAC_BOX_NOR )
            Vec_StrPrintStr( vStr, " & ~" );
        else if ( Type == BAC_BOX_OR )
            Vec_StrPrintStr( vStr, " | " );
        else if ( Type == BAC_BOX_NAND )
            Vec_StrPrintStr( vStr, " | ~" );
        else if ( Type == BAC_BOX_XOR || Type == BAC_BOX_XNOR )
            Vec_StrPrintStr( vStr, " ^ " );
        else assert( 0 );
        Bac_ManWriteSig( p, iObj - 2 );
    }
    Vec_StrPrintStr( vStr, ";\n" );
}
void Bac_ManWriteVerilogBoxes( Bac_Ntk_t * p, int fUseAssign )
{
    Vec_Str_t * vStr = p->pDesign->vOut;
    int iObj, k, i, o, StartPos;
    Bac_NtkForEachBox( p, iObj ) // .subckt/.gate/box (formal/actual binding) 
    {
        // skip constants
        if ( Bac_ObjIsConst(p, iObj) )
            continue;
        // write mapped
        if ( Bac_ObjIsGate(p, iObj) )
        {
            Bac_ManWriteGate( p, iObj );
            continue;
        }
        // write primitives as assign-statements
        if ( !Bac_ObjIsBoxUser(p, iObj) && fUseAssign )
        {
            Bac_ManWriteAssign( p, iObj );
            continue;
        }
        // write header
        StartPos = Vec_StrSize(vStr);
        if ( Bac_ObjIsBoxUser(p, iObj) )
        {
            int Value, Beg, End, Range;
            Bac_Ntk_t * pModel = Bac_BoxNtk( p, iObj );
            Vec_StrPrintStr( vStr, "  " );
            Vec_StrPrintStr( vStr, Bac_NtkName(pModel) );
            Vec_StrPrintStr( vStr, " " );
            Vec_StrPrintStr( vStr, Bac_ObjName(p, iObj) ? Bac_ObjNameStr(p, iObj) : ""  );
            Vec_StrPrintStr( vStr, " (" );
            // write arguments
            i = o = 0;
            assert( Bac_NtkInfoNum(pModel) );
            Vec_IntForEachEntryTriple( &pModel->vInfo, Value, Beg, End, k )
            {
                int NameId = Abc_Lit2Var2( Value );
                int Type = Abc_Lit2Att2( Value );
                Vec_StrPrintStr( vStr, k ? ", " : "" );
                if ( Vec_StrSize(vStr) > StartPos + 70 )
                {
                    StartPos = Vec_StrSize(vStr);
                    Vec_StrPrintStr( vStr, "\n    " );
                }
                Vec_StrPrintStr( vStr, "." );
                Vec_StrPrintStr( vStr, Bac_NtkStr(p, NameId) );
                Vec_StrPrintStr( vStr, "(" );
                Range = Bac_InfoRange( Beg, End );
                assert( Range > 0 );
                if ( Type == 1 )
                    Bac_ManWriteConcat( p, Bac_BoxBi(p, iObj, i), Range ), i += Range;
                else if ( Type == 2 )
                    Bac_ManWriteConcat( p, Bac_BoxBo(p, iObj, o), Range ), o += Range;
                else assert( 0 );
                Vec_StrPrintStr( vStr, ")" );
            }
            assert( i == Bac_BoxBiNum(p, iObj) );
            assert( o == Bac_BoxBoNum(p, iObj) );
        }
        else
        {
            int iTerm, k, Range, iSig = 0;
            Vec_Int_t * vBits = Bac_BoxCollectRanges( p, iObj );
            char * pName = Bac_NtkGenerateName( p, Bac_ObjType(p, iObj), vBits );
            char * pSymbs = Bac_ManPrimSymb( p->pDesign, Bac_ObjType(p, iObj) );
            Vec_StrPrintStr( vStr, "  " );
            Vec_StrPrintStr( vStr, pName );
            Vec_StrPrintStr( vStr, " " );
            Vec_StrPrintStr( vStr, Bac_ObjName(p, iObj) ? Bac_ObjNameStr(p, iObj) : ""  );
            Vec_StrPrintStr( vStr, " (" );
            // write inputs
            Bac_BoxForEachBiMain( p, iObj, iTerm, k )
            {
                Range = Vec_IntEntry( vBits, iSig );
                Vec_StrPrintStr( vStr, iSig ? ", " : "" );
                if ( Vec_StrSize(vStr) > StartPos + 70 )
                {
                    StartPos = Vec_StrSize(vStr);
                    Vec_StrPrintStr( vStr, "\n    " );
                }
                Vec_StrPrintStr( vStr, "." );
                Vec_StrPush( vStr, pSymbs[iSig] );
                Vec_StrPrintStr( vStr, "(" );
                Bac_ManWriteConcat( p, iTerm, Range );
                Vec_StrPrintStr( vStr, ")" );
                iSig++;
            }
            Bac_BoxForEachBoMain( p, iObj, iTerm, k )
            {
                Range = Vec_IntEntry( vBits, iSig );
                Vec_StrPrintStr( vStr, iSig ? ", " : "" );
                if ( Vec_StrSize(vStr) > StartPos + 70 )
                {
                    StartPos = Vec_StrSize(vStr);
                    Vec_StrPrintStr( vStr, "\n    " );
                }
                Vec_StrPrintStr( vStr, "." );
                Vec_StrPush( vStr, pSymbs[iSig] );
                Vec_StrPrintStr( vStr, "(" );
                Bac_ManWriteConcat( p, iTerm, Range );
                Vec_StrPrintStr( vStr, ")" );
                iSig++;
            }
            assert( iSig == Vec_IntSize(vBits) );
        }
        Vec_StrPrintStr( vStr, ");\n" );
    }
}
void Bac_ManWriteVerilogNtk( Bac_Ntk_t * p, int fUseAssign )
{
    char * pKeyword[4] = { "wire ", "input ", "output ", "inout " };
    Vec_Str_t * vStr = p->pDesign->vOut;
    int k, iObj, iTerm, Value, Beg, End, Length, fHaveWires, StartPos;
//    assert( Bac_NtkInfoNum(p) );
    assert( Vec_IntSize(&p->vFanin) == Bac_NtkObjNum(p) );
//    Bac_NtkPrint( p );
    // write header
    Vec_StrPrintStr( vStr, "module " );
    Vec_StrPrintStr( vStr, Bac_NtkName(p) );
    Vec_StrPrintStr( vStr, " (\n    " );
    StartPos = Vec_StrSize(vStr);
    Vec_IntForEachEntryTriple( &p->vInfo, Value, Beg, End, k )
    if ( Abc_Lit2Att2(Value) != 0 )
    {
        Vec_StrPrintStr( vStr, k ? ", " : "" );
        if ( Vec_StrSize(vStr) > StartPos + 70 )
        {
            StartPos = Vec_StrSize(vStr);
            Vec_StrPrintStr( vStr, "\n    " );
        }
        Bac_ManWriteVar( p, Abc_Lit2Var2(Value) );
    }
    Vec_StrPrintStr( vStr, "\n  );\n" );
    // write inputs/outputs
    Vec_IntForEachEntryTriple( &p->vInfo, Value, Beg, End, k )
    if ( Abc_Lit2Att2(Value) != 0 )
    {
        Vec_StrPrintStr( vStr, "  " );
        Vec_StrPrintStr( vStr, pKeyword[Abc_Lit2Att2(Value)] );
        if ( Beg >= 0 )
            Bac_ManWriteRange( p, Beg, End );
        Bac_ManWriteVar( p, Abc_Lit2Var2(Value) );
        Vec_StrPrintStr( vStr, ";\n" );
    }
    Vec_StrPrintStr( vStr, "\n" );
    // write word-level wires
    Bac_NtkForEachBox( p, iObj )
    if ( !Bac_ObjIsConst(p, iObj) )
        Bac_BoxForEachBo( p, iObj, iTerm, k )
        if ( Bac_ObjNameType(p, iTerm) == BAC_NAME_WORD || Bac_ObjNameType(p, iTerm) == BAC_NAME_INFO )
        {
            Vec_StrPrintStr( vStr, "  wire " );
            Bac_ManWriteRange( p, Bac_ManFindRealIndex(p, iTerm), Bac_ManFindRealIndex(p, iTerm + Bac_ObjGetRange(p, iTerm) - 1) );
            Bac_ManWriteVar( p, Bac_ManFindRealNameId(p, iTerm) );
            Vec_StrPrintStr( vStr, ";\n" );
        }
    // check if there are any wires left
    fHaveWires = 0;
    Bac_NtkForEachBox( p, iObj )
    if ( !Bac_ObjIsConst(p, iObj) )
        Bac_BoxForEachBo( p, iObj, iTerm, k )
        if ( Bac_ObjNameType(p, iTerm) == BAC_NAME_BIN )
        { fHaveWires = 1; iObj = Bac_NtkObjNum(p); break; }
    // write bit-level wires
    if ( fHaveWires )
    {
        Length = 7;
        Vec_StrPrintStr( vStr, "\n  wire " );
        Bac_NtkForEachBox( p, iObj )
        if ( !Bac_ObjIsConst(p, iObj) )
            Bac_BoxForEachBo( p, iObj, iTerm, k )
            if ( Bac_ObjNameType(p, iTerm) == BAC_NAME_BIN )
            {
                if ( Length > 72 )
                    Vec_StrPrintStr( vStr, ";\n  wire " ), Length = 7;
                if ( Length > 7 )
                    Vec_StrPrintStr( vStr, ", " );
                Vec_StrPrintStr( vStr, Bac_ObjNameStr(p, iTerm) ); 
                Length += strlen(Bac_ObjNameStr(p, iTerm));
            }
        Vec_StrPrintStr( vStr, ";\n" );
    }
    Vec_StrPrintStr( vStr, "\n" );
    // write objects
    Bac_ManWriteVerilogBoxes( p, fUseAssign );
    Vec_StrPrintStr( vStr, "endmodule\n\n" );
}
void Bac_ManWriteVerilog( char * pFileName, Bac_Man_t * p, int fUseAssign )
{
    Bac_Ntk_t * pNtk; int i;
    // check the library
    if ( p->pMioLib && p->pMioLib != Abc_FrameReadLibGen() )
    {
        printf( "Genlib library used in the mapped design is not longer a current library.\n" );
        return;
    }
    // derive the stream
    p->vOut = Vec_StrAlloc( 10000 );
    p->vOut2 = Vec_StrAlloc( 1000 );
    Vec_StrPrintStr( p->vOut, "// Design \"" );
    Vec_StrPrintStr( p->vOut, Bac_ManName(p) );
    Vec_StrPrintStr( p->vOut, "\" written via CBA package in ABC on " );
    Vec_StrPrintStr( p->vOut, Extra_TimeStamp() );
    Vec_StrPrintStr( p->vOut, "\n\n" );
    Bac_ManAssignInternWordNames( p );
    Bac_ManForEachNtk( p, pNtk, i )
        Bac_ManWriteVerilogNtk( pNtk, fUseAssign );
    // dump into file
    if ( p->vOut && Vec_StrSize(p->vOut) > 0 )
    {
        FILE * pFile = fopen( pFileName, "wb" );
        if ( pFile == NULL )
            printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        else
        {
            fwrite( Vec_StrArray(p->vOut), 1, Vec_StrSize(p->vOut), pFile );
            fclose( pFile );
        }
    }
    Vec_StrFreeP( &p->vOut );
    Vec_StrFreeP( &p->vOut2 );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

