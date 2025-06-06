/**CFile****************************************************************

  FileName    [bacPrsTrans.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Parse tree to netlist transformation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacPrsTrans.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"
#include "bacPrs.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Bac_Trip_t_ Bac_Trip_t;
struct Bac_Trip_t_
{
    Bac_ObjType_t Type;
    char *        pName;
    char *        pCode;
    char *        pSigs[6];
};
/*
static Bac_Trip_t s_Types[100] =
{ 
    { BAC_BOX_CT       , "VERIFIC_PWR",           "1",      {"o"} },
    { BAC_BOX_CF       , "VERIFIC_GND",           "1",      {"o"} },
    { BAC_BOX_CX       , "VERIFIC_X",             "1",      {"o"} },
    { BAC_BOX_CZ       , "VERIFIC_Z",             "1",      {"o"} },
    { BAC_BOX_INV      , "VERIFIC_INV",           "11",     {"i","o"} },
    { BAC_BOX_BUF      , "VERIFIC_BUF",           "11",     {"i","o"} },
    { BAC_BOX_AND      , "VERIFIC_AND",           "111",    {"a0","a1","o"} },
    { BAC_BOX_NAND     , "VERIFIC_NAND",          "111",    {"a0","a1","o"} },
    { BAC_BOX_OR       , "VERIFIC_OR",            "111",    {"a0","a1","o"} },
    { BAC_BOX_NOR      , "VERIFIC_NOR",           "111",    {"a0","a1","o"} },
    { BAC_BOX_XOR      , "VERIFIC_XOR",           "111",    {"a0","a1","o"} },
    { BAC_BOX_XNOR     , "VERIFIC_XNOR",          "111",    {"a0","a1","o"} },
    { BAC_BOX_MUX      , "VERIFIC_MUX",           "1111",   {"c","a1","a0","o"} },                        // changed order
    { (Bac_ObjType_t)-1, "VERIFIC_PULLUP",        "1",      {"o"} },
    { (Bac_ObjType_t)-1, "VERIFIC_PULLDOWN",      "1",      {"o"} },
    { BAC_BOX_TRI      , "VERIFIC_TRI",           "111",    {"i","c","o"} },
    { BAC_BOX_LATCH    , "VERIFIC_DLATCH",        "11111",  {"d","async_val","async_cond","gate","q"} },  // changed order
    { BAC_BOX_LATCHRS  , "VERIFIC_DLATCHRS",      "11111",  {"d","s","r","gate","q"} },                   // changed order
    { BAC_BOX_DFF      , "VERIFIC_DFF",           "11111",  {"d","async_val","async_cond","clk","q"} },   // changed order
    { BAC_BOX_DFFRS    , "VERIFIC_DFFRS",         "11111",  {"d","s","r","clk","q"} },                    // changed order
    { (Bac_ObjType_t)-1, "VERIFIC_NMOS",          "111",    {"c","d","o"} },
    { (Bac_ObjType_t)-1, "VERIFIC_PMOS",          "111",    {"c","d","o"} },
    { (Bac_ObjType_t)-1, "VERIFIC_CMOS",          "1111",   {"d","nc","pc","o"} },
    { (Bac_ObjType_t)-1, "VERIFIC_TRAN",          "111",    {"inout1","inout2","control"} },
    { BAC_BOX_ADD      , "VERIFIC_FADD",          "11111",  {"cin","a","b","o","cout"} },
    { (Bac_ObjType_t)-1, "VERIFIC_RCMOS",         "1111",   {"d","nc","pc","o"} },
    { (Bac_ObjType_t)-1, "VERIFIC_RNMOS",         "111",    {"c","d","o"} },
    { (Bac_ObjType_t)-1, "VERIFIC_RPMOS",         "111",    {"c","d","o"} },
    { (Bac_ObjType_t)-1, "VERIFIC_RTRAN",         "111",    {"inout1","inout2","control"} },
    { (Bac_ObjType_t)-1, "VERIFIC_HDL_ASSERTION", "1",      {"condition"} },
    { BAC_BOX_ADD      , "add_",                  "1aba1",  {"cin","a","b","o","cout"} },
    { BAC_BOX_MUL      , "mult_",                 "ab?",    {"a","b","o"} },            // ? = a * b
    { BAC_BOX_DIV      , "div_",                  "ab?",    {"a","b","o"} },            // ? = 
    { BAC_BOX_MOD      , "mod_",                  "ab?",    {"a","b","o"} },            // ? =
    { BAC_BOX_REM      , "rem_",                  "ab?",    {"a","b","o"} },            // ? =
    { BAC_BOX_SHIL     , "shift_left_",           "1aba",   {"cin","a","amount","o"} },
    { BAC_BOX_SHIR     , "shift_right_",          "1aba",   {"cin","a","amount","o"} },
    { BAC_BOX_ROTL     , "rotate_left_",          "aba",    {"a","amount","o"} },
    { BAC_BOX_ROTR     , "rotate_right_",         "aba",    {"a","amount","o"} },
    { BAC_BOX_RAND     , "reduce_and_",           "ab1",    {"a","o"} },
    { BAC_BOX_ROR      , "reduce_or_",            "ab1",    {"a","o"} },
    { BAC_BOX_RXOR     , "reduce_xor_",           "ab1",    {"a","o"} },
    { BAC_BOX_RNAND    , "reduce_nand_",          "ab1",    {"a","o"} },
    { BAC_BOX_RNOR     , "reduce_nor_",           "ab1",    {"a","o"} },
    { BAC_BOX_RXNOR    , "reduce_xnor_",          "ab1",    {"a","o"} },
    { BAC_BOX_LTHAN    , "LessThan_",             "1ab1",   {"cin","a","b","o"} },
    { BAC_BOX_NMUX     , "Mux_",                  "ab1",    {"sel","data","o"} },       
    { BAC_BOX_SEL      , "Select_",               "aaa",    {"sel","data","o"} },
    { BAC_BOX_DEC      , "Decoder_",              "a?",     {"a","o"} },                // ? = (1 << a)
    { BAC_BOX_EDEC     , "EnabledDecoder_",       "1a?",    {"en","i","o"} },           // ? = (1 << a)
    { BAC_BOX_PSEL     , "PrioSelect_",           "1aaa",   {"cin","sel","data","o"} },
    { BAC_BOX_RAM      , "DualPortRam_",          "1abab",  {"write_enable","write_address","write_data","read_address","read_data"} },
    { BAC_BOX_RAMR     , "ReadPort_",             "1a1b",   {"read_enable", "read_address", "RAM", "read_data" } },
    { BAC_BOX_RAMW     , "WritePort_",            "1ab1",   {"write_enable","write_address","write_data", "RAM"} },
    { BAC_BOX_RAMWC    , "ClockedWritePort_",     "11ab1",  {"clk","write_enable","write_address","write_data", "RAM"} },
    { BAC_BOX_LUT      , "lut",                   "?",      {"i","o"} },
    { BAC_BOX_AND      , "and_",                  "aaa",    {"a","b","o"} },
    { BAC_BOX_OR       , "or_",                   "aaa",    {"a","b","o"} },
    { BAC_BOX_XOR      , "xor_",                  "aaa",    {"a","b","o"} },
    { BAC_BOX_NAND     , "nand_",                 "aaa",    {"a","b","o"} },
    { BAC_BOX_NOR      , "nor_",                  "aaa",    {"a","b","o"} },
    { BAC_BOX_XNOR     , "xnor_",                 "aaa",    {"a","b","o"} },
    { BAC_BOX_BUF      , "buf_",                  "aa",     {"i","o"} },
    { BAC_BOX_INV      , "inv_",                  "aa",     {"i","o"} },
    { BAC_BOX_TRI      , "tri_",                  "a1a",    {"i","c","o"} },
    { BAC_BOX_SUB      , "sub_",                  "aaa",    {"a","b","o"} },
    { BAC_BOX_MIN      , "unary_minus_",          "aa",     {"i","o"} },
    { BAC_BOX_EQU      , "equal_",                "aa1",    {"a","b","o"} },
    { BAC_BOX_NEQU     , "not_equal_",            "aa1",    {"a","b","o"} },
    { BAC_BOX_MUX      , "mux_",                  "1aaa",   {"cond","d1","d0","o"} },                       // changed order
    { BAC_BOX_NMUX     , "wide_mux_",             "ab?",    {"sel","data","o"} },             // ? = b / (1 << a)
    { BAC_BOX_SEL      , "wide_select_",          "ab?",    {"sel","data","o"} },             // ? = b / a
    { BAC_BOX_DFF      , "wide_dff_",             "aaa1a",  {"d","async_val","async_cond","clock","q"} },
    { BAC_BOX_DFFRS    , "wide_dlatch_",          "aaa1a",  {"d","set","reset","clock","q"} },           
    { BAC_BOX_LATCHRS  , "wide_dffrs_",           "aaa1a",  {"d","set","reset","clock","q"} },           
    { BAC_BOX_LATCH    , "wide_dlatchrs_",        "aaa1a",  {"d","async_val","async_cond","clock","q"} },
    { BAC_BOX_PSEL     , "wide_prio_select_",     "ab??",   {"sel","data","carry_in","o"} },  // ? = b / a
    { BAC_BOX_POW      , "pow_",                  "abc",    {"a","b","o"} },                  // ? = 
    { BAC_BOX_PENC     , "PrioEncoder_",          "a?",     {"sel","o"} },
    { BAC_BOX_ABS      , "abs",                   "aa",     {"i","o"} }
};
*/

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Count range size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Psr_ManRangeSizeName( Psr_Ntk_t * p, int Name )
{
    return 1;
}
static inline int Psr_ManRangeSizeRange( Psr_Ntk_t * p, int Range )
{
    char * pStr; 
    int Left, Right;
    if ( Range == 0 ) 
        return 1;
    pStr = Psr_NtkStr( p, Range );
    assert( pStr[0] == '[' );
    Left = Right = atoi( pStr + 1 );
    pStr = strstr( pStr, "=" );
    if ( pStr )
        Right = atoi( pStr + 1 );
    return 1 + (Left > Right ? Left - Right : Right - Left);
}
static inline int Psr_ManRangeSizeConst( Psr_Ntk_t * p, int Const )
{
    return atoi( Psr_NtkStr(p, Const) );
}
static inline int Psr_ManRangeSizeConcat( Psr_Ntk_t * p, int Con )
{
    extern int Psr_ManRangeSizeArray( Psr_Ntk_t * p, Vec_Int_t * vSlices, int Start, int Stop );
    Vec_Int_t * vSigs = Psr_CatSignals(p, Con);
    return Psr_ManRangeSizeArray( p, vSigs, 0, Vec_IntSize(vSigs) );
}
static inline int Psr_ManRangeSizeSignal( Psr_Ntk_t * p, int Sig )
{
    int Value = Abc_Lit2Var2( Sig );
    Psr_ManType_t Type = (Psr_ManType_t)Abc_Lit2Att2( Sig );
    if ( Type == BAC_PRS_NAME )
        return Psr_ManRangeSizeName( p, Value );
    if ( Type == BAC_PRS_SLICE )
        return Psr_ManRangeSizeRange( p, Psr_SliceRange(p, Value) );
    if ( Type == BAC_PRS_CONST )
        return Psr_ManRangeSizeConst( p, Value );
    if ( Type == BAC_PRS_CONCAT )
        return Psr_ManRangeSizeConcat( p, Value );
    assert( 0 );
    return 0;
}
int Psr_ManRangeSizeArray( Psr_Ntk_t * p, Vec_Int_t * vSlices, int Start, int Stop )
{
    int i, Sig, Count = 0;
    assert( Vec_IntSize(vSlices) > 0 );
    Vec_IntForEachEntryStartStop( vSlices, Sig, i, Start, Stop )
        Count += Psr_ManRangeSizeSignal( p, Sig );
    return Count;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

