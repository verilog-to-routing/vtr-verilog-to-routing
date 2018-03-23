/**CFile****************************************************************

  FileName    [wlcWriteVer.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Writes word-level Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcWriteVer.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"

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
void Wlc_WriteTableOne( FILE * pFile, int nFans, int nOuts, word * pTable, int Id )
{
    int m, nMints = (1<<nFans);
//    Abc_TtPrintHexArrayRev( stdout, pTable, nMints );  printf( "\n" );
    assert( nOuts > 0 && nOuts <= 64 && (64 % nOuts) == 0 );
    fprintf( pFile, "module table%d(ind, val);\n", Id );
    fprintf( pFile, "  input  [%d:0] ind;\n", nFans-1 );
    fprintf( pFile, "  output [%d:0] val;\n", nOuts-1 );
    fprintf( pFile, "  reg    [%d:0] val;\n", nOuts-1 );
    fprintf( pFile, "  always @(ind)\n" );
    fprintf( pFile, "  begin\n" );
    fprintf( pFile, "    case (ind)\n" );
    for ( m = 0; m < nMints; m++ )
    fprintf( pFile, "      %d\'h%x: val = %d\'h%x;\n", nFans, m, nOuts, (unsigned)((pTable[(nOuts * m) >> 6] >> ((nOuts * m) & 63)) & Abc_Tt6Mask(nOuts)) );
    fprintf( pFile, "    endcase\n" );
    fprintf( pFile, "  end\n" );
    fprintf( pFile, "endmodule\n" );
    fprintf( pFile, "\n" );
}
void Wlc_WriteTables( FILE * pFile, Wlc_Ntk_t * p )
{
    Vec_Int_t * vNodes;
    Wlc_Obj_t * pObj, * pFanin;
    word * pTable;
    int i;
    if ( p->vTables == NULL || Vec_PtrSize(p->vTables) == 0 )
        return;
    // map tables into their nodes
    vNodes = Vec_IntStart( Vec_PtrSize(p->vTables) );
    Wlc_NtkForEachObj( p, pObj, i )
        if ( pObj->Type == WLC_OBJ_TABLE )
            Vec_IntWriteEntry( vNodes, Wlc_ObjTableId(pObj), i );
    // write tables
    Vec_PtrForEachEntry( word *, p->vTables, pTable, i )
    {
        pObj = Wlc_NtkObj( p, Vec_IntEntry(vNodes, i) );
        assert( pObj->Type == WLC_OBJ_TABLE );
        pFanin = Wlc_ObjFanin0( p, pObj );
        Wlc_WriteTableOne( pFile, Wlc_ObjRange(pFanin), Wlc_ObjRange(pObj), pTable, i );
    }
    Vec_IntFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [This was used to add POs to each node except PIs and MUXes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_WriteAddPos( Wlc_Ntk_t * p )
{
    Wlc_Obj_t * pObj;
    int i;
    Vec_IntClear( &p->vPos );
    Wlc_NtkForEachObj( p, pObj, i )
        if ( pObj->Type != WLC_OBJ_PI && pObj->Type != WLC_OBJ_MUX )
        {
            pObj->fIsPo = 1;
            Vec_IntPush( &p->vPos, Wlc_ObjId(p, pObj) );
        }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wlc_WriteVerIntVec( FILE * pFile, Wlc_Ntk_t * p, Vec_Int_t * vVec, int Start )
{
    char * pName;
    int LineLength  = Start;
    int NameCounter = 0;
    int AddedLength, i, iObj;
    Vec_IntForEachEntry( vVec, iObj, i )
    {
        pName = Wlc_ObjName( p, iObj );
        // get the line length after this name is written
        AddedLength = strlen(pName) + 2;
        if ( NameCounter && LineLength + AddedLength + 3 > 70 )
        { // write the line extender
            fprintf( pFile, "\n   " );
            // reset the line length
            LineLength  = Start;
            NameCounter = 0;
        }
        fprintf( pFile, " %s%s", pName, (i==Vec_IntSize(vVec)-1)? "" : "," );
        LineLength += AddedLength;
        NameCounter++;
    }
} 
void Wlc_WriteVerInt( FILE * pFile, Wlc_Ntk_t * p, int fNoFlops )
{
    Wlc_Obj_t * pObj;
    int i, k, iFanin;
    char Range[100];
    fprintf( pFile, "module %s ( ", p->pName );
    fprintf( pFile, "\n   " );
    if ( Wlc_NtkPiNum(p) > 0 || (fNoFlops && Wlc_NtkCiNum(p)) )
    {
        Wlc_WriteVerIntVec( pFile, p, fNoFlops ? &p->vCis : &p->vPis, 3 );
        fprintf( pFile, ",\n   " );
    }
    if ( Wlc_NtkPoNum(p) > 0 || (fNoFlops && Wlc_NtkCoNum(p))  )
        Wlc_WriteVerIntVec( pFile, p, fNoFlops ? &p->vCos : &p->vPos, 3 );
    fprintf( pFile, "  );\n" );
    // mark fanins of rotation shifts
    Wlc_NtkForEachObj( p, pObj, i )
        if ( pObj->Type == WLC_OBJ_ROTATE_R || pObj->Type == WLC_OBJ_ROTATE_L )
            Wlc_ObjFanin1(p, pObj)->Mark = 1;
    Wlc_NtkForEachObj( p, pObj, i )
    {
        int nDigits   = Abc_Base10Log(Abc_AbsInt(pObj->End)+1) + Abc_Base10Log(Abc_AbsInt(pObj->Beg)+1) + (int)(pObj->End < 0) + (int)(pObj->Beg < 0);
        if ( pObj->Mark ) 
        {
            pObj->Mark = 0;
            continue;
        }
        sprintf( Range, "%s[%d:%d]%*s", (!p->fSmtLib && Wlc_ObjIsSigned(pObj)) ? "signed ":"       ", pObj->End, pObj->Beg, 8-nDigits, "" );
        fprintf( pFile, "  " );
        if ( pObj->Type == WLC_OBJ_PI || (fNoFlops && pObj->Type == WLC_OBJ_FO) )
            fprintf( pFile, "input  " );
        else if ( pObj->fIsPo || (fNoFlops && pObj->fIsFi) )
            fprintf( pFile, "output " );
        else
            fprintf( pFile, "       " );
        if ( Wlc_ObjIsCi(pObj) || pObj->fIsPo || (fNoFlops && pObj->fIsFi) )
        {
            fprintf( pFile, "wire %s %s ;\n", Range, Wlc_ObjName(p, i) );
            if ( Wlc_ObjIsCi(pObj) )
                continue;
            Range[0] = 0;
        }
        if ( pObj->fIsPo || (fNoFlops && pObj->fIsFi) )
            fprintf( pFile, "  assign                         " );
        else if ( pObj->Type == WLC_OBJ_MUX && Wlc_ObjFaninNum(pObj) > 3 )
            fprintf( pFile, "reg  %s ", Range );
        else
            fprintf( pFile, "wire %s ", Range );
        if ( pObj->Type == WLC_OBJ_TABLE )
        {
            // wire [3:0] s4972; table0 s4972_Index(s4971, s4972);
            fprintf( pFile, "%s ;              table%d", Wlc_ObjName(p, i), Wlc_ObjTableId(pObj) );
            fprintf( pFile, " s%d_Index(%s, ", i, Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            fprintf( pFile, "%s)",             Wlc_ObjName(p, i) );
        }
        else if ( pObj->Type == WLC_OBJ_CONST )
        {
            fprintf( pFile, "%-16s = %d\'%sh", Wlc_ObjName(p, i), Wlc_ObjRange(pObj), Wlc_ObjIsSigned(pObj) ? "s":"" );
            if ( pObj->fXConst )
            {
                for ( k = 0; k < (Wlc_ObjRange(pObj) + 3) / 4; k++ )
                    fprintf( pFile, "x" );
            }
            else
                Abc_TtPrintHexArrayRev( pFile, (word *)Wlc_ObjConstValue(pObj), (Wlc_ObjRange(pObj) + 3) / 4 );
        }
        else if ( pObj->Type == WLC_OBJ_ROTATE_R || pObj->Type == WLC_OBJ_ROTATE_L )
        {
            //  wire [27:0] s4960 = (s57 >> 17) | (s57 << 11);
            Wlc_Obj_t * pShift = Wlc_ObjFanin1(p, pObj);
            int Num0 = *Wlc_ObjConstValue(pShift);
            int Num1 = Wlc_ObjRange(pObj) - Num0;
            assert( pShift->Type == WLC_OBJ_CONST );
            assert( Num0 > 0 && Num0 < Wlc_ObjRange(pObj) );
            fprintf( pFile, "%-16s = ", Wlc_ObjName(p, i) );
            if ( pObj->Type == WLC_OBJ_ROTATE_R )
                fprintf( pFile, "(%s >> %d) | (%s << %d)", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)), Num0, Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)), Num1 );
            else
                fprintf( pFile, "(%s << %d) | (%s >> %d)", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)), Num0, Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)), Num1 );
        }
        else if ( pObj->Type == WLC_OBJ_MUX && Wlc_ObjFaninNum(pObj) > 3 )
        {
            fprintf( pFile, "%s ;\n", Wlc_ObjName(p, i) );
            fprintf( pFile, "         " );
            fprintf( pFile, "always @( " );
            Wlc_ObjForEachFanin( pObj, iFanin, k )
                fprintf( pFile, "%s%s", k ? " or ":"", Wlc_ObjName(p, Wlc_ObjFaninId(pObj, k)) );
            fprintf( pFile, " )\n" );
            fprintf( pFile, "           " );
            fprintf( pFile, "begin\n" );
            fprintf( pFile, "             " );
            fprintf( pFile, "case ( %s )\n", Wlc_ObjName(p, Wlc_ObjFaninId(pObj, 0)) );
            Wlc_ObjForEachFanin( pObj, iFanin, k )
            {
                if ( !k ) continue;
                fprintf( pFile, "               " );
                fprintf( pFile, "%d : %s = ", k-1, Wlc_ObjName(p, i) );
                fprintf( pFile, "%s ;\n", Wlc_ObjName(p, Wlc_ObjFaninId(pObj, k)) );
            }
            fprintf( pFile, "             " );
            fprintf( pFile, "endcase\n" );
            fprintf( pFile, "           " );
            fprintf( pFile, "end\n" );
            continue;
        }
        else
        {
            fprintf( pFile, "%-16s = ", Wlc_ObjName(p, i) );
            if ( pObj->Type == WLC_OBJ_BUF )
                fprintf( pFile, "%s", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_MUX )
            {
                fprintf( pFile, "%s ? ", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
                fprintf( pFile, "%s : ", Wlc_ObjName(p, Wlc_ObjFaninId2(pObj)) );
                fprintf( pFile, "%s",    Wlc_ObjName(p, Wlc_ObjFaninId1(pObj)) );
            }
            else if ( pObj->Type == WLC_OBJ_ARI_MINUS )
                fprintf( pFile, "-%s", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_BIT_NOT )
                fprintf( pFile, "~%s", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_LOGIC_NOT )
                fprintf( pFile, "!%s", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_REDUCT_AND )
                fprintf( pFile, "&%s", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_REDUCT_OR )
                fprintf( pFile, "|%s", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_REDUCT_XOR )
                fprintf( pFile, "^%s", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_REDUCT_NAND )
                fprintf( pFile, "~&%s", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_REDUCT_NOR )
                fprintf( pFile, "~|%s", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_REDUCT_NXOR )
                fprintf( pFile, "~^%s", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_BIT_SELECT )
                fprintf( pFile, "%s [%d:%d]", Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)), Wlc_ObjRangeEnd(pObj), Wlc_ObjRangeBeg(pObj) );
            else if ( pObj->Type == WLC_OBJ_BIT_SIGNEXT )
                fprintf( pFile, "{ {%d{%s[%d]}}, %s }", Wlc_ObjRange(pObj) - Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)), Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)), Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)) - 1, Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_BIT_ZEROPAD )
                fprintf( pFile, "{ {%d{1\'b0}}, %s }", Wlc_ObjRange(pObj) - Wlc_ObjRange(Wlc_ObjFanin0(p, pObj)), Wlc_ObjName(p, Wlc_ObjFaninId0(pObj)) );
            else if ( pObj->Type == WLC_OBJ_BIT_CONCAT )
            {
                fprintf( pFile, "{" );
                Wlc_ObjForEachFanin( pObj, iFanin, k )
                    fprintf( pFile, " %s%s", Wlc_ObjName(p, Wlc_ObjFaninId(pObj, k)), k == Wlc_ObjFaninNum(pObj)-1 ? "":"," );
                fprintf( pFile, " }" );
            }
            else
            {
                fprintf( pFile, "%s ", Wlc_ObjName(p, Wlc_ObjFaninId(pObj, 0)) );
                if ( pObj->Type == WLC_OBJ_SHIFT_R )
                    fprintf( pFile, ">>" );
                else if ( pObj->Type == WLC_OBJ_SHIFT_RA )
                    fprintf( pFile, ">>>" );
                else if ( pObj->Type == WLC_OBJ_SHIFT_L )
                    fprintf( pFile, "<<" );
                else if ( pObj->Type == WLC_OBJ_SHIFT_LA )
                    fprintf( pFile, "<<<" );
                else if ( pObj->Type == WLC_OBJ_BIT_AND )
                    fprintf( pFile, "&" );
                else if ( pObj->Type == WLC_OBJ_BIT_OR )
                    fprintf( pFile, "|" );
                else if ( pObj->Type == WLC_OBJ_BIT_XOR )
                    fprintf( pFile, "^" );
                else if ( pObj->Type == WLC_OBJ_BIT_NAND )
                    fprintf( pFile, "~&" );
                else if ( pObj->Type == WLC_OBJ_BIT_NOR )
                    fprintf( pFile, "~|" );
                else if ( pObj->Type == WLC_OBJ_BIT_NXOR )
                    fprintf( pFile, "~^" );
                else if ( pObj->Type == WLC_OBJ_LOGIC_IMPL )
                    fprintf( pFile, "=>" );
                else if ( pObj->Type == WLC_OBJ_LOGIC_AND )
                    fprintf( pFile, "&&" );
                else if ( pObj->Type == WLC_OBJ_LOGIC_OR )
                    fprintf( pFile, "||" );
                else if ( pObj->Type == WLC_OBJ_LOGIC_XOR )
                    fprintf( pFile, "^^" );
                else if ( pObj->Type == WLC_OBJ_COMP_EQU )
                    fprintf( pFile, "==" );
                else if ( pObj->Type == WLC_OBJ_COMP_NOTEQU )
                    fprintf( pFile, "!=" );
                else if ( pObj->Type == WLC_OBJ_COMP_LESS )
                    fprintf( pFile, "<" );
                else if ( pObj->Type == WLC_OBJ_COMP_MORE )
                    fprintf( pFile, ">" );
                else if ( pObj->Type == WLC_OBJ_COMP_LESSEQU )
                    fprintf( pFile, "<=" );
                else if ( pObj->Type == WLC_OBJ_COMP_MOREEQU )
                    fprintf( pFile, ">=" );
                else if ( pObj->Type == WLC_OBJ_ARI_ADD )
                    fprintf( pFile, "+" );
                else if ( pObj->Type == WLC_OBJ_ARI_SUB )
                    fprintf( pFile, "-" );
                else if ( pObj->Type == WLC_OBJ_ARI_MULTI )
                    fprintf( pFile, "*" );
                else if ( pObj->Type == WLC_OBJ_ARI_DIVIDE )
                    fprintf( pFile, "/" );
                else if ( pObj->Type == WLC_OBJ_ARI_REM )
                    fprintf( pFile, "%%" );
                else if ( pObj->Type == WLC_OBJ_ARI_MODULUS )
                    fprintf( pFile, "%%" );
                else if ( pObj->Type == WLC_OBJ_ARI_POWER )
                    fprintf( pFile, "**" );
                else if ( pObj->Type == WLC_OBJ_ARI_SQRT )
                    fprintf( pFile, "@" );
                else if ( pObj->Type == WLC_OBJ_ARI_SQUARE )
                    fprintf( pFile, "#" );
                else assert( 0 );
                    //fprintf( pFile, "???" );
                fprintf( pFile, " %s", Wlc_ObjName(p, Wlc_ObjFaninId(pObj, 1)) );
            }
        }
        fprintf( pFile, " ;%s\n", (p->fSmtLib && Wlc_ObjIsSigned(pObj)) ? " // signed SMT-LIB operator" : "" );
    }
    iFanin = 0;
    assert( !p->vInits || Wlc_NtkFfNum(p) == Vec_IntSize(p->vInits) );
    if ( !fNoFlops )
    {
        if ( p->vInits )
        Wlc_NtkForEachCi( p, pObj, i )
        {
            int nDigits   = Abc_Base10Log(pObj->End+1) + 1;
            char * pName  = Wlc_ObjName(p, Wlc_ObjId(p, pObj));
            assert( i == Wlc_ObjCiId(pObj) );
            if ( pObj->Type == WLC_OBJ_PI )
                continue;
            sprintf( Range, "       [%d:%d]%*s", Wlc_ObjRange(pObj) - 1, 0, 8-nDigits, "" );
            fprintf( pFile, "         " );
            fprintf( pFile, "wire %s ", Range );
            fprintf( pFile, "%s_init%*s = ", pName, 11 - (int)strlen(pName), "" );
            if ( Vec_IntEntry(p->vInits, i-Wlc_NtkPiNum(p)) > 0 )
                fprintf( pFile, "%s", Wlc_ObjName(p, Wlc_ObjId(p, Wlc_NtkPi(p, Vec_IntEntry(p->vInits, i-Wlc_NtkPiNum(p))))));
            else
            {
                if ( p->pInits[iFanin] == 'x' || p->pInits[iFanin] == 'X' )
                {
                    fprintf( pFile, "%d\'h", Wlc_ObjRange(pObj) );
                    for ( k = 0; k < (Wlc_ObjRange(pObj) + 3) / 4; k++ )
                        fprintf( pFile, "x" );
                }
                else
                {
                    fprintf( pFile, "%d\'b", Wlc_ObjRange(pObj) );
                    for ( k = Wlc_ObjRange(pObj)-1; k >= 0; k-- )
                        fprintf( pFile, "%c", p->pInits[iFanin + k] );
                }
            }
            fprintf( pFile, ";\n" );
            iFanin += Wlc_ObjRange(pObj);
        }
        Wlc_NtkForEachCi( p, pObj, i )
        {
            assert( i == Wlc_ObjCiId(pObj) );
            if ( pObj->Type == WLC_OBJ_PI )
                continue;
            fprintf( pFile, "         " );
            fprintf( pFile, "CPL_FF" );
            if ( Wlc_ObjRange(pObj) > 1 )
                fprintf( pFile, "#%d%*s", Wlc_ObjRange(pObj), 4 - Abc_Base10Log(Wlc_ObjRange(pObj)+1), "" );
            else
                fprintf( pFile, "     " );
            fprintf( pFile, " reg%d (",       i );
            fprintf( pFile, " .q( %s ),",      Wlc_ObjName(p, Wlc_ObjId(p, pObj)) );
            fprintf( pFile, " .qbar()," );
            fprintf( pFile, " .d( %s ),",      Wlc_ObjName(p, Wlc_ObjId(p, Wlc_ObjFo2Fi(p, pObj))) );
            fprintf( pFile, " .clk( %s ),",    "1\'b0" );
            fprintf( pFile, " .arst( %s ),",   "1\'b0" );
            if ( p->vInits )
                fprintf( pFile, " .arstval( %s_init )", Wlc_ObjName(p, Wlc_ObjId(p, pObj)) );
            else
                fprintf( pFile, " .arstval( %s )", "1\'b0" );
            fprintf( pFile, " ) ;\n" );
        }
        assert( !p->vInits || iFanin == (int)strlen(p->pInits) );
    }
    fprintf( pFile, "endmodule\n\n" );
} 
void Wlc_WriteVer( Wlc_Ntk_t * p, char * pFileName, int fAddCos, int fNoFlops )
{
    FILE * pFile;
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Wlc_WriteVer(): Cannot open the output file \"%s\".\n", pFileName );
        return;
    }
    fprintf( pFile, "// Benchmark \"%s\" written by ABC on %s\n", p->pName, Extra_TimeStamp() );
    fprintf( pFile, "\n" );
    Wlc_WriteTables( pFile, p );
    if ( fAddCos )
        Wlc_WriteAddPos( p );
    Wlc_WriteVerInt( pFile, p, fNoFlops );
    fprintf( pFile, "\n" );
    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

