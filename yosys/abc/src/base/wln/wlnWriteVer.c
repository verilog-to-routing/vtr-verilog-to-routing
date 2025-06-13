/**CFile****************************************************************

  FileName    [wlnWriteVer.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Word-level network.]

  Synopsis    [Writing Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 23, 2018.]

  Revision    [$Id: wlnWriteVer.c,v 1.00 2018/09/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wln.h"

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
void Wln_WriteTableOne( FILE * pFile, int nFans, int nOuts, word * pTable, int Id )
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
void Wln_WriteTables( FILE * pFile, Wln_Ntk_t * p )
{
    Vec_Int_t * vNodes;
    word * pTable; 
    int i, iObj;
    if ( p->vTables == NULL || Vec_PtrSize(p->vTables) == 0 )
        return;
    // map tables into their nodes
    vNodes = Vec_IntStart( Vec_PtrSize(p->vTables) );
    Wln_NtkForEachObj( p, iObj )
        if ( Wln_ObjType(p, iObj) == ABC_OPER_TABLE )
            Vec_IntWriteEntry( vNodes, Wln_ObjFanin1(p, iObj), iObj );
    // write tables
    Vec_PtrForEachEntry( word *, p->vTables, pTable, i )
    {
        int iNode  = Vec_IntEntry( vNodes, i );
        int iFanin = Wln_ObjFanin0( p, iNode );
        Wln_WriteTableOne( pFile, Wln_ObjRange(p, iFanin), Wln_ObjRange(p, iNode), pTable, i );
    }
    Vec_IntFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [This was used to add POs to each node except PIs and MUXes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wln_WriteAddPos( Wln_Ntk_t * p )
{
    int iObj;
    Wln_NtkForEachObj( p, iObj )
        if ( !Wln_ObjIsCio(p, iObj) )
            Wln_ObjCreateCo( p, iObj );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Wln_WriteVerIntVec( FILE * pFile, Wln_Ntk_t * p, Vec_Int_t * vVec, int Start )
{
    char * pName;
    int LineLength  = Start;
    int NameCounter = 0;
    int AddedLength, i, iObj;
    Vec_IntForEachEntry( vVec, iObj, i )
    {
        pName = Wln_ObjName( p, iObj );
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
void Wln_WriteVerInt( FILE * pFile, Wln_Ntk_t * p )
{
    int k, j, iObj, iFanin;
    char Range[100];
    fprintf( pFile, "module %s ( ", p->pName );
    fprintf( pFile, "\n   " );
    if ( Wln_NtkCiNum(p) > 0 )
    {
        Wln_WriteVerIntVec( pFile, p, &p->vCis, 3 );
        fprintf( pFile, ",\n   " );
    }
    if ( Wln_NtkCoNum(p) > 0 )
        Wln_WriteVerIntVec( pFile, p, &p->vCos, 3 );
    fprintf( pFile, "  );\n" );
    Wln_NtkForEachObj( p, iObj )
    {
        int End = Wln_ObjRangeEnd(p, iObj);
        int Beg = Wln_ObjRangeBeg(p, iObj);
        int nDigits = Abc_Base10Log(Abc_AbsInt(End)+1) + Abc_Base10Log(Abc_AbsInt(Beg)+1) + (int)(End < 0) + (int)(Beg < 0);
        sprintf( Range, "%s[%d:%d]%*s", (!p->fSmtLib && Wln_ObjIsSigned(p, iObj)) ? "signed ":"       ", End, Beg, 8-nDigits, "" );
        fprintf( pFile, "  " );
        if ( Wln_ObjIsCi(p, iObj) )
            fprintf( pFile, "input  " );
        else if ( Wln_ObjIsCo(p, iObj) )
            fprintf( pFile, "output " );
        else
            fprintf( pFile, "       " );
        if ( Wln_ObjIsCio(p, iObj) )
        {
            fprintf( pFile, "wire %s %s ;\n", Range, Wln_ObjName(p, iObj) );
            if ( Wln_ObjIsCi(p, iObj) )
                continue;
            fprintf( pFile, "  assign                         " );
            fprintf( pFile, "%-16s = %s ;\n", Wln_ObjName(p, iObj), Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            continue;
        }
        if ( Wln_ObjType(p, iObj) == ABC_OPER_SEL_NMUX || Wln_ObjType(p, iObj) == ABC_OPER_SEL_SEL )
            fprintf( pFile, "reg  %s ", Range );
        else
            fprintf( pFile, "wire %s ", Range );
        if ( Wln_ObjType(p, iObj) == ABC_OPER_TABLE )
        {
            // wire [3:0] s4972; table0 s4972_Index(s4971, s4972);
            fprintf( pFile, "%s ;              table%d", Wln_ObjName(p, iObj), Wln_ObjFanin1(p, iObj) );
            fprintf( pFile, " s%d_Index(%s, ", iObj, Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            fprintf( pFile, "%s)",             Wln_ObjName(p, iObj) );
        }
        else if ( Wln_ObjType(p, iObj) == ABC_OPER_LUT )
        {
            // wire [3:0] s4972; LUT lut4972_Index(s4971, s4972);
            fprintf( pFile, "%s ;           LUT", Wln_ObjName(p, iObj) );
            fprintf( pFile, " lut%d (%s, ", iObj, Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            for ( k = 1; k < Wln_ObjFaninNum(p, iObj); k++ )
                fprintf( pFile, "%s, ", Wln_ObjName(p, Wln_ObjFanin(p, iObj, k)) );
            fprintf( pFile, "%s)",             Wln_ObjName(p, iObj) );
        }
        else if ( Wln_ObjIsConst(p, iObj) )
            fprintf( pFile, "%-16s = %s", Wln_ObjName(p, iObj), Wln_ObjConstString(p, iObj) );
        else if ( Wln_ObjType(p, iObj) == ABC_OPER_SHIFT_ROTR || Wln_ObjType(p, iObj) == ABC_OPER_SHIFT_ROTL )
        {
            //  wire [27:0] s4960 = (s57 >> 17) | (s57 << 11);
            int Num0 = Wln_ObjFanin1(p, iObj);
            int Num1 = Wln_ObjRange(p, iObj) - Num0;
            assert( Num0 > 0 && Num0 < Wln_ObjRange(p, iObj) );
            fprintf( pFile, "%-16s = ", Wln_ObjName(p, iObj) );
            if ( Wln_ObjType(p, iObj) == ABC_OPER_SHIFT_ROTR )
                fprintf( pFile, "(%s >> %d) | (%s << %d)", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)), Num0, Wln_ObjName(p, Wln_ObjFanin0(p, iObj)), Num1 );
            else
                fprintf( pFile, "(%s << %d) | (%s >> %d)", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)), Num0, Wln_ObjName(p, Wln_ObjFanin0(p, iObj)), Num1 );
        }
        else if ( Wln_ObjType(p, iObj) == ABC_OPER_SEL_NMUX )
        {
            fprintf( pFile, "%s ;\n", Wln_ObjName(p, iObj) );
            fprintf( pFile, "         " );
            fprintf( pFile, "always @( " );
            Wln_ObjForEachFanin( p, iObj, iFanin, k )
                fprintf( pFile, "%s%s", k ? " or ":"", Wln_ObjName(p, Wln_ObjFanin(p, iObj, k)) );
            fprintf( pFile, " )\n" );
            fprintf( pFile, "           " );
            fprintf( pFile, "begin\n" );
            fprintf( pFile, "             " );
            fprintf( pFile, "case ( %s )\n", Wln_ObjName(p, Wln_ObjFanin(p, iObj, 0)) );
            Wln_ObjForEachFanin( p, iObj, iFanin, k )
            {
                if ( !k ) continue;
                fprintf( pFile, "               " );
                fprintf( pFile, "%d : %s = ", k-1, Wln_ObjName(p, iObj) );
                fprintf( pFile, "%s ;\n", Wln_ObjName(p, Wln_ObjFanin(p, iObj, k)) );
            }
            fprintf( pFile, "             " );
            fprintf( pFile, "endcase\n" );
            fprintf( pFile, "           " );
            fprintf( pFile, "end\n" );
            continue;
        }
        else if ( Wln_ObjType(p, iObj) == ABC_OPER_SEL_SEL )
        {
            fprintf( pFile, "%s ;\n", Wln_ObjName(p, iObj) );
            fprintf( pFile, "         " );
            fprintf( pFile, "always @( " );
            Wln_ObjForEachFanin( p, iObj, iFanin, k )
                fprintf( pFile, "%s%s", k ? " or ":"", Wln_ObjName(p, Wln_ObjFanin(p, iObj, k)) );
            fprintf( pFile, " )\n" );
            fprintf( pFile, "           " );
            fprintf( pFile, "begin\n" );
            fprintf( pFile, "             " );
            fprintf( pFile, "case ( %s )\n", Wln_ObjName(p, Wln_ObjFanin(p, iObj, 0)) );
            Wln_ObjForEachFanin( p, iObj, iFanin, k )
            {
                if ( !k ) continue;
                fprintf( pFile, "               " );
                fprintf( pFile, "%d\'b", Wln_ObjFaninNum(p, iObj)-1 );
                for ( j = Wln_ObjFaninNum(p, iObj)-1; j > 0; j-- )
                    fprintf( pFile, "%d", (int)(j==k) );
                fprintf( pFile, " : %s = ", Wln_ObjName(p, iObj) );
                fprintf( pFile, "%s ;\n", Wln_ObjName(p, Wln_ObjFanin(p, iObj, k)) );
            }
            fprintf( pFile, "               " );
            fprintf( pFile, "default" );
            fprintf( pFile, " : %s = ", Wln_ObjName(p, iObj) );
            fprintf( pFile, "%d\'b", Wln_ObjRange(p, iObj) );
            for ( j = Wln_ObjRange(p, iObj)-1; j >= 0; j-- )
                fprintf( pFile, "%d", 0 );
            fprintf( pFile, " ;\n" );
            fprintf( pFile, "             " );
            fprintf( pFile, "endcase\n" );
            fprintf( pFile, "           " );
            fprintf( pFile, "end\n" );
            continue;
        }
        else if ( Wln_ObjType(p, iObj) == ABC_OPER_SEL_DEC )
        {
            int nRange = Wln_ObjRange(p, Wln_ObjFanin0(p, iObj));
            assert( (1 << nRange) == Wln_ObjRange(p, iObj) );
            fprintf( pFile, "%s ;\n", Wln_ObjName(p, iObj) );
            for ( k = 0; k < Wln_ObjRange(p, iObj); k++ )
            {
                fprintf( pFile, "         " );
                fprintf( pFile, "wire " );
                fprintf( pFile, "%s_", Wln_ObjName(p, iObj) );
                for ( j = 0; j < nRange; j++ )
                    fprintf( pFile, "%d", (k >> (nRange-1-j)) & 1 );
                fprintf( pFile, " = " );
                for ( j = 0; j < nRange; j++ )
                    fprintf( pFile, "%s%s%s[%d]", 
                        j ? " & ":"", ((k >> (nRange-1-j)) & 1) ? " ":"~", 
                        Wln_ObjName(p, Wln_ObjFanin(p, iObj, 0)), nRange-1-j );
                fprintf( pFile, " ;\n" );
            }
            fprintf( pFile, "         " );
            fprintf( pFile, "assign %s = { ", Wln_ObjName(p, iObj) );
            for ( k = Wln_ObjRange(p, iObj)-1; k >= 0; k-- )
            {
                fprintf( pFile, "%s%s_", k < Wln_ObjRange(p, iObj)-1 ? ", ":"", Wln_ObjName(p, iObj) );
                for ( j = 0; j < nRange; j++ )
                    fprintf( pFile, "%d", (k >> (nRange-1-j)) & 1 );
            }
            fprintf( pFile, " } ;\n" );
            continue;
        }
        else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_ADDSUB )
        {
            // out = mode ? a+b+cin : a-b-cin
            fprintf( pFile, "%s ;\n", Wln_ObjName(p, iObj) );
            fprintf( pFile, "         " );
            fprintf( pFile, "assign " );
            fprintf( pFile, "%s = %s ? %s + %s + %s : %s - %s - %s ;\n", 
                        Wln_ObjName(p, iObj), Wln_ObjName(p, Wln_ObjFanin0(p, iObj)),
                        Wln_ObjName(p, Wln_ObjFanin2(p, iObj)), Wln_ObjName(p, Wln_ObjFanin(p, iObj,3)), Wln_ObjName(p, Wln_ObjFanin1(p, iObj)),
                        Wln_ObjName(p, Wln_ObjFanin2(p, iObj)), Wln_ObjName(p, Wln_ObjFanin(p, iObj,3)), Wln_ObjName(p, Wln_ObjFanin1(p, iObj)) 
                   );
            continue;
        }
        else if ( Wln_ObjType(p, iObj) == ABC_OPER_RAMR || Wln_ObjType(p, iObj) == ABC_OPER_RAMW )
        {
            if ( 1 )
            {
                fprintf( pFile, "%s ;\n", Wln_ObjName(p, iObj) );
                fprintf( pFile, "         " );
                fprintf( pFile, "%s (", Wln_ObjType(p, iObj) == ABC_OPER_RAMR ? "ABC_READ" : "ABC_WRITE" );
                Wln_ObjForEachFanin( p, iObj, iFanin, k )
                    fprintf( pFile, " .%s(%s),", k==0 ? "mem_in" : (k==1 ? "addr": "data"), Wln_ObjName(p, iFanin) );
                fprintf( pFile, " .%s(%s) ) ;\n", Wln_ObjType(p, iObj) == ABC_OPER_RAMR ? "data" : "mem_out", Wln_ObjName(p, iObj) );
                continue;
            }
            else
            {
                int nBitsMem  = Wln_ObjRange(p,  Wln_ObjFanin(p, iObj, 0) );
                //int nBitsAddr = Wln_ObjRange(p,  Wln_ObjFanin(p, iObj, 1) );
                int nBitsDat  = Wln_ObjType(p, iObj) == ABC_OPER_RAMR ? Wln_ObjRange(p, iObj) : Wln_ObjRange(p, Wln_ObjFanin(p, iObj, 2));
                int Depth     = nBitsMem / nBitsDat;
                assert( nBitsMem % nBitsDat == 0 );
                fprintf( pFile, "%s ;\n", Wln_ObjName(p, iObj) );
                fprintf( pFile, "         " );
                fprintf( pFile, "%s_%d (", Wln_ObjType(p, iObj) == ABC_OPER_RAMR ? "CPL_MEM_READ" : "CPL_MEM_WRITE", Depth );
                Wln_ObjForEachFanin( p, iObj, iFanin, k )
                    fprintf( pFile, " .%s(%s),", k==0 ? "mem_data_in" : (k==1 ? "addr_in": "data_in"), Wln_ObjName(p, iFanin) );
                fprintf( pFile, " .%s(%s) ) ;\n", "data_out", Wln_ObjName(p, iObj) );
                continue;
            }
        }
        else if ( Wln_ObjType(p, iObj) == ABC_OPER_DFFRSE )
        {
            fprintf( pFile, "%s ;\n", Wln_ObjName(p, iObj) );
            continue;
        }
        else 
        {
            fprintf( pFile, "%-16s = ", Wln_ObjName(p, iObj) );
            if ( Wln_ObjType(p, iObj) == ABC_OPER_BIT_BUF )
                fprintf( pFile, "%s", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_BIT_MUX )
            {
                fprintf( pFile, "%s ? ", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
                fprintf( pFile, "%s : ", Wln_ObjName(p, Wln_ObjFanin1(p, iObj)) );
                fprintf( pFile, "%s",    Wln_ObjName(p, Wln_ObjFanin2(p, iObj)) );
            }
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_MIN )
                fprintf( pFile, "-%s", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_BIT_INV )
                fprintf( pFile, "~%s", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_LOGIC_NOT )
                fprintf( pFile, "!%s", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_RED_AND )
                fprintf( pFile, "&%s", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_RED_OR )
                fprintf( pFile, "|%s", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_RED_XOR )
                fprintf( pFile, "^%s", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_RED_NAND )
                fprintf( pFile, "~&%s", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_RED_NOR )
                fprintf( pFile, "~|%s", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_RED_NXOR )
                fprintf( pFile, "~^%s", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_SLICE )
                fprintf( pFile, "%s [%d:%d]", Wln_ObjName(p, Wln_ObjFanin0(p, iObj)), Wln_ObjRangeEnd(p, iObj), Wln_ObjRangeBeg(p, iObj) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_SIGNEXT )
                fprintf( pFile, "{ {%d{%s[%d]}}, %s }", Wln_ObjRange(p, iObj) - Wln_ObjRange(p, Wln_ObjFanin0(p, iObj)), Wln_ObjName(p, Wln_ObjFanin0(p, iObj)), Wln_ObjRange(p, Wln_ObjFanin0(p, iObj)) - 1, Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_ZEROPAD )
                fprintf( pFile, "{ {%d{1\'b0}}, %s }", Wln_ObjRange(p, iObj) - Wln_ObjRange(p, Wln_ObjFanin0(p, iObj)), Wln_ObjName(p, Wln_ObjFanin0(p, iObj)) );
            else if ( Wln_ObjType(p, iObj) == ABC_OPER_CONCAT )
            {
                fprintf( pFile, "{" );
                Wln_ObjForEachFanin( p, iObj, iFanin, k )
                    fprintf( pFile, " %s%s", Wln_ObjName(p, Wln_ObjFanin(p, iObj, k)), k == Wln_ObjFaninNum(p, iObj)-1 ? "":"," );
                fprintf( pFile, " }" );
            }
            else
            {
                fprintf( pFile, "%s ", Wln_ObjName(p, Wln_ObjFanin(p, iObj, 0)) );
                if ( Wln_ObjType(p, iObj) == ABC_OPER_SHIFT_R )
                    fprintf( pFile, ">>" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_SHIFT_RA )
                    fprintf( pFile, ">>>" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_SHIFT_L )
                    fprintf( pFile, "<<" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_SHIFT_LA )
                    fprintf( pFile, "<<<" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_BIT_AND )
                    fprintf( pFile, "&" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_BIT_OR )
                    fprintf( pFile, "|" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_BIT_XOR )
                    fprintf( pFile, "^" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_BIT_NAND )
                    fprintf( pFile, "~&" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_BIT_NOR )
                    fprintf( pFile, "~|" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_BIT_NXOR )
                    fprintf( pFile, "~^" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_LOGIC_IMPL )
                    fprintf( pFile, "=>" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_LOGIC_AND )
                    fprintf( pFile, "&&" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_LOGIC_OR )
                    fprintf( pFile, "||" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_LOGIC_XOR )
                    fprintf( pFile, "^^" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_COMP_EQU )
                    fprintf( pFile, "==" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_COMP_NOTEQU )
                    fprintf( pFile, "!=" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_COMP_LESS )
                    fprintf( pFile, "<" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_COMP_MORE )
                    fprintf( pFile, ">" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_COMP_LESSEQU )
                    fprintf( pFile, "<=" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_COMP_MOREEQU )
                    fprintf( pFile, ">=" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_ADD )
                    fprintf( pFile, "+" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_SUB )
                    fprintf( pFile, "-" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_MUL )
                    fprintf( pFile, "*" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_DIV )
                    fprintf( pFile, "/" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_REM )
                    fprintf( pFile, "%%" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_MOD )
                    fprintf( pFile, "%%" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_POW )
                    fprintf( pFile, "**" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_SQRT )
                    fprintf( pFile, "@" );
                else if ( Wln_ObjType(p, iObj) == ABC_OPER_ARI_SQUARE )
                    fprintf( pFile, "#" );
                else 
                {
                    //assert( 0 );
                    printf( "Failed to write node \"%s\" with unknown operator type (%d).\n", Wln_ObjName(p, iObj), Wln_ObjType(p, iObj) );
                    fprintf( pFile, "???\n" );
                    continue;
                }
                fprintf( pFile, " %s", Wln_ObjName(p, Wln_ObjFanin(p, iObj, 1)) );
                if ( Wln_ObjFaninNum(p, iObj) == 3 && Wln_ObjType(p, iObj) == ABC_OPER_ARI_ADD ) 
                    fprintf( pFile, " + %s", Wln_ObjName(p, Wln_ObjFanin(p, iObj, 2)) );
            }
        }
        fprintf( pFile, " ;%s\n", (p->fSmtLib && Wln_ObjIsSigned(p, iObj)) ? " // signed SMT-LIB operator" : "" );
    }
    iFanin = 0;
    // write DFFs in the end
    fprintf( pFile, "\n" );
    Wln_NtkForEachFf( p, iObj, j )
    {
        char * pInNames[8] = {"d", "clk", "reset", "set", "enable", "async", "sre", "init"};
        fprintf( pFile, "         " );
        fprintf( pFile, "%s (", "ABC_DFFRSE" );
        Wln_ObjForEachFanin( p, iObj, iFanin, k )
            if ( iFanin ) fprintf( pFile, " .%s(%s),", pInNames[k], Wln_ObjName(p, iFanin) );
        fprintf( pFile, " .%s(%s) ) ;\n", "q", Wln_ObjName(p, iObj) );
    }
    fprintf( pFile, "\n" );
    fprintf( pFile, "endmodule\n\n" );
} 
void Wln_WriteVer( Wln_Ntk_t * p, char * pFileName )
{
    FILE * pFile;
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Wln_WriteVer(): Cannot open the output file \"%s\".\n", pFileName );
        return;
    }
    fprintf( pFile, "// Benchmark \"%s\" from file \"%s\" written by ABC on %s\n", p->pName, p->pSpec ? p->pSpec : "unknown", Extra_TimeStamp() );
    fprintf( pFile, "\n" );
    Wln_WriteTables( pFile, p );
//    if ( fAddCos )
//        Wln_WriteAddPos( p );
    Wln_WriteVerInt( pFile, p );
    fprintf( pFile, "\n" );
    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

