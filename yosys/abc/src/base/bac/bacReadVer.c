/**CFile****************************************************************

  FileName    [bacReadVer.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [BLIF writer.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: bacReadVer.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bac.h"
#include "bacPrs.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// Verilog keywords
typedef enum { 
    PRS_VER_NONE = 0,  // 0:  unused
    PRS_VER_INPUT,     // 1:  input
    PRS_VER_OUTPUT,    // 2:  output
    PRS_VER_INOUT,     // 3:  inout
    PRS_VER_WIRE,      // 4:  wire
    PRS_VER_MODULE,    // 5:  module
    PRS_VER_ASSIGN,    // 6:  assign
    PRS_VER_REG,       // 7:  reg
    PRS_VER_ALWAYS,    // 8:  always
    PRS_VER_DEFPARAM,  // 9:  always
    PRS_VER_BEGIN,     // 10: begin
    PRS_VER_END,       // 11: end
    PRS_VER_ENDMODULE, // 12: endmodule
    PRS_VER_UNKNOWN    // 13: unknown
} Bac_VerType_t;

static const char * s_VerTypes[PRS_VER_UNKNOWN+1] = {
    NULL,              // 0:  unused
    "input",           // 1:  input
    "output",          // 2:  output
    "inout",           // 3:  inout
    "wire",            // 4:  wire
    "module",          // 5:  module
    "assign",          // 6:  assign
    "reg",             // 7:  reg
    "always",          // 8:  always
    "defparam",        // 9:  defparam
    "begin",           // 10: begin
    "end",             // 11: end
    "endmodule",       // 12: endmodule
    NULL               // 13: unknown 
};

static inline void Psr_NtkAddVerilogDirectives( Psr_Man_t * p )
{
    int i;
    for ( i = 1; s_VerTypes[i]; i++ )
        Abc_NamStrFindOrAdd( p->pStrs, (char *)s_VerTypes[i],   NULL );
    assert( Abc_NamObjNumMax(p->pStrs) == i );
}


// character recognition 
static inline int Psr_CharIsSpace( char c )   { return (c == ' ' || c == '\t' || c == '\r' || c == '\n');                           }
static inline int Psr_CharIsDigit( char c )   { return (c >= '0' && c <= '9');                                                      }
static inline int Psr_CharIsDigitB( char c )  { return (c == '0' || c == '1'  || c == 'x' || c == 'z');                             }
static inline int Psr_CharIsDigitH( char c )  { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');  }
static inline int Psr_CharIsChar( char c )    { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');                            }
static inline int Psr_CharIsSymb1( char c )   { return Psr_CharIsChar(c) || c == '_';                                               }
static inline int Psr_CharIsSymb2( char c )   { return Psr_CharIsSymb1(c) || Psr_CharIsDigit(c) || c == '$';                        }

static inline int Psr_ManIsChar( Psr_Man_t * p, char c )    { return p->pCur[0] == c;                        }
static inline int Psr_ManIsChar1( Psr_Man_t * p, char c )   { return p->pCur[1] == c;                        }
static inline int Psr_ManIsDigit( Psr_Man_t * p )           { return Psr_CharIsDigit(*p->pCur);              }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

// collect predefined modules names
static const char * s_VerilogModules[100] = 
{
    "const0", // BAC_BOX_CF,  
    "const1", // BAC_BOX_CT,  
    "constX", // BAC_BOX_CX,  
    "constZ", // BAC_BOX_CZ,  
    "buf",    // BAC_BOX_BUF,  
    "not",    // BAC_BOX_INV,  
    "and",    // BAC_BOX_AND,  
    "nand",   // BAC_BOX_NAND, 
    "or",     // BAC_BOX_OR,   
    "nor",    // BAC_BOX_NOR,  
    "xor",    // BAC_BOX_XOR,  
    "xnor",   // BAC_BOX_XNOR, 
    "sharp",  // BAC_BOX_SHARP,
    "mux",    // BAC_BOX_MUX,  
    "maj",    // BAC_BOX_MAJ,  
    NULL
};
static const char * s_KnownModules[100] = 
{
    "VERIFIC_",
    "add_",                  
    "mult_",                 
    "div_",                  
    "mod_",                  
    "rem_",                  
    "shift_left_",           
    "shift_right_",          
    "rotate_left_",          
    "rotate_right_",         
    "reduce_and_",           
    "reduce_or_",            
    "reduce_xor_",           
    "reduce_nand_",          
    "reduce_nor_",           
    "reduce_xnor_",          
    "LessThan_",             
    "Mux_",                  
    "Select_",               
    "Decoder_",              
    "EnabledDecoder_",       
    "PrioSelect_",           
    "DualPortRam_",          
    "ReadPort_",             
    "WritePort_",            
    "ClockedWritePort_",     
    "lut",                   
    "and_",                  
    "or_",                   
    "xor_",                  
    "nand_",                 
    "nor_",                  
    "xnor_",                 
    "buf_",                  
    "inv_",                  
    "tri_",                  
    "sub_",                  
    "unary_minus_",          
    "equal_",                
    "not_equal_",            
    "mux_",                  
    "wide_mux_",             
    "wide_select_",          
    "wide_dff_",             
    "wide_dlatch_",          
    "wide_dffrs_",           
    "wide_dlatchrs_",        
    "wide_prio_select_",     
    "pow_",                  
    "PrioEncoder_",          
    "abs",                   
    NULL
};

// check if it is a Verilog predefined module
static inline int Psr_ManIsVerilogModule( Psr_Man_t * p, char * pName )
{
    int i;
    for ( i = 0; s_VerilogModules[i]; i++ )
        if ( !strcmp(pName, s_VerilogModules[i]) )
            return BAC_BOX_CF + i;
    return 0;
}
// check if it is a known module
static inline int Psr_ManIsKnownModule( Psr_Man_t * p, char * pName )
{
    int i;
    for ( i = 0; s_KnownModules[i]; i++ )
        if ( !strncmp(pName, s_KnownModules[i], strlen(s_KnownModules[i])) )
            return i;
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

// skips Verilog comments (returns 1 if some comments were skipped)
static inline int Psr_ManUtilSkipComments( Psr_Man_t * p )
{
    if ( !Psr_ManIsChar(p, '/') )
        return 0;
    if ( Psr_ManIsChar1(p, '/') )
    {
        for ( p->pCur += 2; p->pCur < p->pLimit; p->pCur++ )
            if ( Psr_ManIsChar(p, '\n') )
                { p->pCur++; return 1; }
    }
    else if ( Psr_ManIsChar1(p, '*') )
    {
        for ( p->pCur += 2; p->pCur < p->pLimit; p->pCur++ )
            if ( Psr_ManIsChar(p, '*') && Psr_ManIsChar1(p, '/') )
                { p->pCur++; p->pCur++; return 1; }
    }
    return 0;
}
static inline int Psr_ManUtilSkipName( Psr_Man_t * p )
{
    if ( !Psr_ManIsChar(p, '\\') )
        return 0;
    for ( p->pCur++; p->pCur < p->pLimit; p->pCur++ )
        if ( Psr_ManIsChar(p, ' ') )
            { p->pCur++; return 1; }
    return 0;
}

// skip any number of spaces and comments
static inline int Psr_ManUtilSkipSpaces( Psr_Man_t * p )
{
    while ( p->pCur < p->pLimit )
    {
        while ( Psr_CharIsSpace(*p->pCur) ) 
            p->pCur++;
        if ( !*p->pCur )
            return Psr_ManErrorSet(p, "Unexpectedly reached end-of-file.", 1);
        if ( !Psr_ManUtilSkipComments(p) )
            return 0;
    }
    return Psr_ManErrorSet(p, "Unexpectedly reached end-of-file.", 1);
}
// skip everything including comments until the given char
static inline int Psr_ManUtilSkipUntil( Psr_Man_t * p, char c )
{
    while ( p->pCur < p->pLimit )
    {
        if ( Psr_ManIsChar(p, c) )
            return 1;
        if ( Psr_ManUtilSkipComments(p) )
            continue;
        if ( Psr_ManUtilSkipName(p) )
            continue;
        p->pCur++;
    }
    return 0;
}
// skip everything including comments until the given word
static inline int Psr_ManUtilSkipUntilWord( Psr_Man_t * p, char * pWord )
{
    char * pPlace = strstr( p->pCur, pWord );
    if ( pPlace == NULL )  return 1;
    p->pCur = pPlace + strlen(pWord);
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Psr_ManReadName( Psr_Man_t * p )
{
    char * pStart = p->pCur;
    if ( Psr_ManIsChar(p, '\\') ) // escaped name
    {
        pStart = ++p->pCur;
        while ( !Psr_ManIsChar(p, ' ') ) 
            p->pCur++;
    }    
    else if ( Psr_CharIsSymb1(*p->pCur) ) // simple name
    {
        p->pCur++;
        while ( Psr_CharIsSymb2(*p->pCur) ) 
            p->pCur++;
    }
    else 
        return 0;
    return Abc_NamStrFindOrAddLim( p->pStrs, pStart, p->pCur, NULL );
}
static inline int Psr_ManReadNameList( Psr_Man_t * p, Vec_Int_t * vTemp, char LastSymb )
{
    Vec_IntClear( vTemp );
    while ( 1 )
    {
        int Item = Psr_ManReadName(p);
        if ( Item == 0 )                    return Psr_ManErrorSet(p, "Cannot read name in the list.", 0);
        if ( Psr_ManUtilSkipSpaces(p) )     return Psr_ManErrorSet(p, "Error number 1.", 0);
        if ( Item == PRS_VER_WIRE  )
            continue;
        Vec_IntPush( vTemp, Item );
        if ( Psr_ManIsChar(p, LastSymb) )   break;
        if ( !Psr_ManIsChar(p, ',') )       return Psr_ManErrorSet(p, "Expecting comma in the list.", 0);
        p->pCur++;
        if ( Psr_ManUtilSkipSpaces(p) )     return Psr_ManErrorSet(p, "Error number 2.", 0);
    }
    return 1;
}
static inline int Psr_ManReadConstant( Psr_Man_t * p )
{
    char * pStart = p->pCur;
    assert( Psr_ManIsDigit(p) );
    while ( Psr_ManIsDigit(p) ) 
        p->pCur++;
    if ( !Psr_ManIsChar(p, '\'') )          return Psr_ManErrorSet(p, "Cannot read constant.", 0);
    p->pCur++;
    if ( Psr_ManIsChar(p, 'b') )
    {
        p->pCur++;
        while ( Psr_CharIsDigitB(*p->pCur) ) 
        {
            if ( *p->pCur == '0' )
                p->pNtk->fHasC0s = 1;
            else if ( *p->pCur == '1' )
                p->pNtk->fHasC1s = 1;
            else if ( *p->pCur == 'x' )
                p->pNtk->fHasCXs = 1;
            else if ( *p->pCur == 'z' )
                p->pNtk->fHasCZs = 1;
            p->pCur++;
        }
    }
    else if ( Psr_ManIsChar(p, 'h') )
    {
        p->pCur++;
        p->pNtk->fHasC0s = 1;
        while ( Psr_CharIsDigitH(*p->pCur) ) 
        {
            if ( *p->pCur != '0' )
                p->pNtk->fHasC1s = 1;
            p->pCur++;
        }
    }
    else if ( Psr_ManIsChar(p, 'd') )
    {
        p->pCur++;
        p->pNtk->fHasC0s = 1;
        while ( Psr_ManIsDigit(p) ) 
        {
            if ( *p->pCur != '0' )
                p->pNtk->fHasC1s = 1;
            p->pCur++;
        }
    }
    else                                    return Psr_ManErrorSet(p, "Cannot read radix of constant.", 0);
    return Abc_NamStrFindOrAddLim( p->pStrs, pStart, p->pCur, NULL );
}
static inline int Psr_ManReadRange( Psr_Man_t * p )
{
    assert( Psr_ManIsChar(p, '[') );
    Vec_StrClear( &p->vCover );
    Vec_StrPush( &p->vCover, *p->pCur++ );
    if ( Psr_ManUtilSkipSpaces(p) )         return Psr_ManErrorSet(p, "Error number 3.", 0);
    if ( !Psr_ManIsDigit(p) )               return Psr_ManErrorSet(p, "Cannot read digit in range specification.", 0);
    while ( Psr_ManIsDigit(p) )
        Vec_StrPush( &p->vCover, *p->pCur++ );
    if ( Psr_ManUtilSkipSpaces(p) )         return Psr_ManErrorSet(p, "Error number 4.", 0);
    if ( Psr_ManIsChar(p, ':') )
    {
        Vec_StrPush( &p->vCover, *p->pCur++ );
        if ( Psr_ManUtilSkipSpaces(p) )     return Psr_ManErrorSet(p, "Error number 5.", 0);
        if ( !Psr_ManIsDigit(p) )           return Psr_ManErrorSet(p, "Cannot read digit in range specification.", 0);
        while ( Psr_ManIsDigit(p) )
            Vec_StrPush( &p->vCover, *p->pCur++ );
        if ( Psr_ManUtilSkipSpaces(p) )     return Psr_ManErrorSet(p, "Error number 6.", 0);
    }
    if ( !Psr_ManIsChar(p, ']') )           return Psr_ManErrorSet(p, "Cannot read closing brace in range specification.", 0);
    Vec_StrPush( &p->vCover, *p->pCur++ );
    Vec_StrPush( &p->vCover, '\0' );
    return Abc_NamStrFindOrAdd( p->pStrs, Vec_StrArray(&p->vCover), NULL );
}
static inline int Psr_ManReadConcat( Psr_Man_t * p, Vec_Int_t * vTemp2 )
{
    extern int Psr_ManReadSignalList( Psr_Man_t * p, Vec_Int_t * vTemp, char LastSymb, int fAddForm );
    assert( Psr_ManIsChar(p, '{') );
    p->pCur++;
    if ( !Psr_ManReadSignalList( p, vTemp2, '}', 0 ) ) return Psr_ManErrorSet(p, "Error number 7.", 0);
    // check final
    assert( Psr_ManIsChar(p, '}') );
    p->pCur++;
    // return special case
    assert( Vec_IntSize(vTemp2) > 0 );
    if ( Vec_IntSize(vTemp2) == 1 )
        return Vec_IntEntry(vTemp2, 0);
    return Abc_Var2Lit2( Psr_NtkAddConcat(p->pNtk, vTemp2), BAC_PRS_CONCAT );
}
static inline int Psr_ManReadSignal( Psr_Man_t * p )
{
    int Item;
    if ( Psr_ManUtilSkipSpaces(p) )         return Psr_ManErrorSet(p, "Error number 8.", 0);
    if ( Psr_ManIsDigit(p) )
    {
        Item = Psr_ManReadConstant(p);
        if ( Item == 0 )                    return Psr_ManErrorSet(p, "Error number 9.", 0);
        if ( Psr_ManUtilSkipSpaces(p) )     return Psr_ManErrorSet(p, "Error number 10.", 0);
        return Abc_Var2Lit2( Item, BAC_PRS_CONST );
    }
    if ( Psr_ManIsChar(p, '{') )
    {
        if ( p->fUsingTemp2 )               return Psr_ManErrorSet(p, "Cannot read nested concatenations.", 0);
        p->fUsingTemp2 = 1;
        Item = Psr_ManReadConcat(p, &p->vTemp2);
        p->fUsingTemp2 = 0;
        if ( Item == 0 )                    return Psr_ManErrorSet(p, "Error number 11.", 0);
        if ( Psr_ManUtilSkipSpaces(p) )     return Psr_ManErrorSet(p, "Error number 12.", 0);
        return Item;
    }
    else
    {
        Item = Psr_ManReadName( p );
        if ( Item == 0 )                    return Psr_ManErrorSet(p, "Error number 13.", 0);    // was        return 1;                
        if ( Psr_ManUtilSkipSpaces(p) )     return Psr_ManErrorSet(p, "Error number 14.", 0);
        if ( Psr_ManIsChar(p, '[') )
        {
            int Range = Psr_ManReadRange(p);
            if ( Range == 0 )               return Psr_ManErrorSet(p, "Error number 15.", 0);
            if ( Psr_ManUtilSkipSpaces(p) ) return Psr_ManErrorSet(p, "Error number 16.", 0);
            return Abc_Var2Lit2( Psr_NtkAddSlice(p->pNtk, Item, Range), BAC_PRS_SLICE );
        }
        return Abc_Var2Lit2( Item, BAC_PRS_NAME );
    }
}
int Psr_ManReadSignalList( Psr_Man_t * p, Vec_Int_t * vTemp, char LastSymb, int fAddForm )
{
    Vec_IntClear( vTemp );
    while ( 1 )
    {
        int Item = Psr_ManReadSignal(p);
        if ( Item == 0 )                    return Psr_ManErrorSet(p, "Cannot read signal in the list.", 0);
        if ( fAddForm )
            Vec_IntPush( vTemp, 0 );
        Vec_IntPush( vTemp, Item );
        if ( Psr_ManIsChar(p, LastSymb) )   break;
        if ( !Psr_ManIsChar(p, ',') )       return Psr_ManErrorSet(p, "Expecting comma in the list.", 0);
        p->pCur++;
    }
    return 1;
}
static inline int Psr_ManReadSignalList2( Psr_Man_t * p, Vec_Int_t * vTemp )
{
    int FormId, ActItem;
    Vec_IntClear( vTemp );
    assert( Psr_ManIsChar(p, '.') );
    while ( Psr_ManIsChar(p, '.') )
    {
        p->pCur++;
        FormId = Psr_ManReadName( p );
        if ( FormId == 0 )                  return Psr_ManErrorSet(p, "Cannot read formal name of the instance.", 0);
        if ( !Psr_ManIsChar(p, '(') )       return Psr_ManErrorSet(p, "Cannot read \"(\" in the instance.", 0);
        p->pCur++;
        if ( Psr_ManUtilSkipSpaces(p) )     return Psr_ManErrorSet(p, "Error number 17.", 0);
        ActItem = Psr_ManReadSignal( p );
        if ( ActItem == 0 )                 return Psr_ManErrorSet(p, "Cannot read actual name of the instance.", 0);
        if ( !Psr_ManIsChar(p, ')') )       return Psr_ManErrorSet(p, "Cannot read \")\" in the instance.", 0);
        p->pCur++;
        Vec_IntPushTwo( vTemp, FormId, ActItem );
        if ( Psr_ManUtilSkipSpaces(p) )     return Psr_ManErrorSet(p, "Error number 18.", 0);
        if ( Psr_ManIsChar(p, ')') )        break;
        if ( !Psr_ManIsChar(p, ',') )       return Psr_ManErrorSet(p, "Expecting comma in the instance.", 0);
        p->pCur++;
        if ( Psr_ManUtilSkipSpaces(p) )     return Psr_ManErrorSet(p, "Error number 19.", 0);
    }
    assert( Vec_IntSize(vTemp) > 0 );
    assert( Vec_IntSize(vTemp) % 2 == 0 );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Psr_ManReadDeclaration( Psr_Man_t * p, int Type )
{
    int i, NameId, RangeId = 0;
    Vec_Int_t * vNames[4]  = { &p->pNtk->vInputs,  &p->pNtk->vOutputs,  &p->pNtk->vInouts,  &p->pNtk->vWires };
    Vec_Int_t * vNamesR[4] = { &p->pNtk->vInputsR, &p->pNtk->vOutputsR, &p->pNtk->vInoutsR, &p->pNtk->vWiresR };
    assert( Type >= PRS_VER_INPUT && Type <= PRS_VER_WIRE );
    if ( Psr_ManUtilSkipSpaces(p) )                                   return Psr_ManErrorSet(p, "Error number 20.", 0);
    if ( Psr_ManIsChar(p, '[') && !(RangeId = Psr_ManReadRange(p)) )  return Psr_ManErrorSet(p, "Error number 21.", 0);
    if ( !Psr_ManReadNameList( p, &p->vTemp, ';' ) )                  return Psr_ManErrorSet(p, "Error number 22.", 0);
    Vec_IntForEachEntry( &p->vTemp, NameId, i )
    {
        Vec_IntPush( vNames[Type - PRS_VER_INPUT], NameId );
        Vec_IntPush( vNamesR[Type - PRS_VER_INPUT], RangeId );
        if ( Type < PRS_VER_WIRE )
            Vec_IntPush( &p->pNtk->vOrder, Abc_Var2Lit2(NameId, Type) );
    }
    return 1;
}
static inline int Psr_ManReadAssign( Psr_Man_t * p )
{
    int OutItem, InItem, fCompl = 0, fCompl2 = 0, Oper = 0;
    // read output name
    OutItem = Psr_ManReadSignal( p );
    if ( OutItem == 0 )                     return Psr_ManErrorSet(p, "Cannot read output in assign-statement.", 0);
    if ( !Psr_ManIsChar(p, '=') )           return Psr_ManErrorSet(p, "Expecting \"=\" in assign-statement.", 0);
    p->pCur++;
    if ( Psr_ManUtilSkipSpaces(p) )         return Psr_ManErrorSet(p, "Error number 23.", 0);
    if ( Psr_ManIsChar(p, '~') ) 
    { 
        fCompl = 1; 
        p->pCur++; 
    }
    // read first name
    InItem = Psr_ManReadSignal( p );
    if ( InItem == 0 )                      return Psr_ManErrorSet(p, "Cannot read first input name in the assign-statement.", 0);
    Vec_IntClear( &p->vTemp );
    Vec_IntPush( &p->vTemp, 0 );
    Vec_IntPush( &p->vTemp, InItem );
    // check unary operator
    if ( Psr_ManIsChar(p, ';') )
    {
        Vec_IntPush( &p->vTemp, 0 );
        Vec_IntPush( &p->vTemp, OutItem );
        Oper = fCompl ? BAC_BOX_INV : BAC_BOX_BUF;
        Psr_NtkAddBox( p->pNtk, Oper, 0, &p->vTemp );
        return 1;
    }
    if ( Psr_ManIsChar(p, '&') ) 
        Oper = BAC_BOX_AND;
    else if ( Psr_ManIsChar(p, '|') ) 
        Oper = BAC_BOX_OR;
    else if ( Psr_ManIsChar(p, '^') ) 
        Oper = BAC_BOX_XOR;
    else if ( Psr_ManIsChar(p, '?') ) 
        Oper = BAC_BOX_MUX;
    else                                    return Psr_ManErrorSet(p, "Unrecognized operator in the assign-statement.", 0);
    p->pCur++; 
    if ( Psr_ManUtilSkipSpaces(p) )         return Psr_ManErrorSet(p, "Error number 24.", 0);
    if ( Psr_ManIsChar(p, '~') ) 
    { 
        fCompl2 = 1; 
        p->pCur++; 
    }
    // read second name
    InItem = Psr_ManReadSignal( p );
    if ( InItem == 0 )                      return Psr_ManErrorSet(p, "Cannot read second input name in the assign-statement.", 0);
    Vec_IntPush( &p->vTemp, 0 );
    Vec_IntPush( &p->vTemp, InItem );
    // read third argument
    if ( Oper == BAC_BOX_MUX )
    {
        assert( fCompl == 0 ); 
        if ( !Psr_ManIsChar(p, ':') )       return Psr_ManErrorSet(p, "Expected colon in the MUX assignment.", 0);
        p->pCur++; 
        // read third name
        InItem = Psr_ManReadSignal( p );
        if ( InItem == 0 )                  return Psr_ManErrorSet(p, "Cannot read third input name in the assign-statement.", 0);
        Vec_IntPush( &p->vTemp, 0 );
        Vec_IntPush( &p->vTemp, InItem );
        if ( !Psr_ManIsChar(p, ';') )       return Psr_ManErrorSet(p, "Expected semicolon at the end of the assign-statement.", 0);
    }
    else
    {
        // figure out operator
        if ( Oper == BAC_BOX_AND )
        {
            if ( fCompl && !fCompl2 )
                Oper = BAC_BOX_SHARPL;
            else if ( !fCompl && fCompl2 )
                Oper = BAC_BOX_SHARP;
            else if ( fCompl && fCompl2 )
                Oper = BAC_BOX_NOR;
        }
        else if ( Oper == BAC_BOX_OR )
        {
            if ( fCompl && fCompl2 )
                Oper = BAC_BOX_NAND;
            else assert( !fCompl && !fCompl2 );
        }
        else if ( Oper == BAC_BOX_XOR )
        {
            if ( fCompl && !fCompl2 )
                Oper = BAC_BOX_XNOR;
            else assert( !fCompl && !fCompl2 );
        }
    }
    // write binary operator
    Vec_IntPush( &p->vTemp, 0 );
    Vec_IntPush( &p->vTemp, OutItem );
    Psr_NtkAddBox( p->pNtk, Oper, 0, &p->vTemp );
    return 1;
}
static inline int Psr_ManReadInstance( Psr_Man_t * p, int Func )
{
    int InstId, Status;
/*
    static Counter = 0;
    if ( ++Counter == 7 )
    {
        int s=0;
    }
*/
    if ( Psr_ManUtilSkipSpaces(p) )         return Psr_ManErrorSet(p, "Error number 25.", 0);
    if ( (InstId = Psr_ManReadName(p)) )
        if (Psr_ManUtilSkipSpaces(p))       return Psr_ManErrorSet(p, "Error number 26.", 0);
    if ( !Psr_ManIsChar(p, '(') )           return Psr_ManErrorSet(p, "Expecting \"(\" in module instantiation.", 0);
    p->pCur++;
    if ( Psr_ManUtilSkipSpaces(p) )         return Psr_ManErrorSet(p, "Error number 27.", 0);
    if ( Psr_ManIsChar(p, '.') ) // box
        Status = Psr_ManReadSignalList2(p, &p->vTemp);
    else  // node
    {
        //char * s = Abc_NamStr(p->pStrs, Func);
        // translate elementary gate
        int iFuncNew = Psr_ManIsVerilogModule(p, Abc_NamStr(p->pStrs, Func));
        if ( iFuncNew == 0 )                return Psr_ManErrorSet(p, "Cannot find elementary gate.", 0);
        Func = iFuncNew;
        Status = Psr_ManReadSignalList( p, &p->vTemp, ')', 1 );
    }
    if ( Status == 0 )                      return Psr_ManErrorSet(p, "Error number 28.", 0);
    assert( Psr_ManIsChar(p, ')') );
    p->pCur++;
    if ( Psr_ManUtilSkipSpaces(p) )         return Psr_ManErrorSet(p, "Error number 29.", 0);
    if ( !Psr_ManIsChar(p, ';') )           return Psr_ManErrorSet(p, "Expecting semicolon in the instance.", 0);
    // add box 
    Psr_NtkAddBox( p->pNtk, Func, InstId, &p->vTemp );
    return 1;
}
static inline int Psr_ManReadArguments( Psr_Man_t * p )
{
    int iRange = 0, iType = -1;
    Vec_Int_t * vSigs[3]  = { &p->pNtk->vInputs,  &p->pNtk->vOutputs,  &p->pNtk->vInouts  };
    Vec_Int_t * vSigsR[3] = { &p->pNtk->vInputsR, &p->pNtk->vOutputsR, &p->pNtk->vInoutsR };
    assert( Psr_ManIsChar(p, '(') );
    p->pCur++;
    if ( Psr_ManUtilSkipSpaces(p) )             return Psr_ManErrorSet(p, "Error number 30.", 0);
    while ( 1 )
    {
        int iName = Psr_ManReadName( p );
        if ( iName == 0 )                       return Psr_ManErrorSet(p, "Error number 31.", 0);
        if ( Psr_ManUtilSkipSpaces(p) )         return Psr_ManErrorSet(p, "Error number 32.", 0);
        if ( iName >= PRS_VER_INPUT && iName <= PRS_VER_INOUT ) // declaration
        {
            iType = iName;
            if ( Psr_ManIsChar(p, '[') )
            {
                iRange = Psr_ManReadRange(p);
                if ( iRange == 0 )              return Psr_ManErrorSet(p, "Error number 33.", 0);
                if ( Psr_ManUtilSkipSpaces(p) ) return Psr_ManErrorSet(p, "Error number 34.", 0);
            }
            iName = Psr_ManReadName( p );
            if ( iName == 0 )                   return Psr_ManErrorSet(p, "Error number 35.", 0);
        }
        if ( iType > 0 )
        {
            Vec_IntPush( vSigs[iType - PRS_VER_INPUT], iName );
            Vec_IntPush( vSigsR[iType - PRS_VER_INPUT], iRange );
            Vec_IntPush( &p->pNtk->vOrder, Abc_Var2Lit2(iName, iType) );
        }
        if ( Psr_ManIsChar(p, ')') )
            break;
        if ( !Psr_ManIsChar(p, ',') )           return Psr_ManErrorSet(p, "Expecting comma in the instance.", 0);
        p->pCur++;
        if ( Psr_ManUtilSkipSpaces(p) )         return Psr_ManErrorSet(p, "Error number 36.", 0);
    }
    // check final
    assert( Psr_ManIsChar(p, ')') );
    return 1;
}
// this procedure can return:
// 0 = reached end-of-file; 1 = successfully parsed; 2 = recognized as primitive; 3 = failed and skipped; 4 = error (failed and could not skip)
static inline int Psr_ManReadModule( Psr_Man_t * p )
{
    int iToken, Status;
    if ( p->pNtk != NULL )                  return Psr_ManErrorSet(p, "Parsing previous module is unfinished.", 4);
    if ( Psr_ManUtilSkipSpaces(p) )
    { 
        Psr_ManErrorClear( p );       
        return 0; 
    }
    // read keyword
    iToken = Psr_ManReadName( p );
    if ( iToken != PRS_VER_MODULE )         return Psr_ManErrorSet(p, "Cannot read \"module\" keyword.", 4);
    if ( Psr_ManUtilSkipSpaces(p) )         return 4;
    // read module name
    iToken = Psr_ManReadName( p );
    if ( iToken == 0 )                      return Psr_ManErrorSet(p, "Cannot read module name.", 4);
    if ( Psr_ManIsKnownModule(p, Abc_NamStr(p->pStrs, iToken)) )
    {
        if ( Psr_ManUtilSkipUntilWord( p, "endmodule" ) ) return Psr_ManErrorSet(p, "Cannot find \"endmodule\" keyword.", 4);
        //printf( "Warning! Skipped known module \"%s\".\n", Abc_NamStr(p->pStrs, iToken) );
        Vec_IntPush( &p->vKnown, iToken );
        return 2;
    }
    Psr_ManInitializeNtk( p, iToken, 1 );
    // skip arguments
    if ( Psr_ManUtilSkipSpaces(p) )         return 4;
    if ( !Psr_ManIsChar(p, '(') )           return Psr_ManErrorSet(p, "Cannot find \"(\" in the argument declaration.", 4);
    if ( !Psr_ManReadArguments(p) )         return 4;
    assert( *p->pCur == ')' );
    p->pCur++;
    if ( Psr_ManUtilSkipSpaces(p) )         return 4;
    // read declarations and instances
    while ( Psr_ManIsChar(p, ';') )
    {
        p->pCur++;
        if ( Psr_ManUtilSkipSpaces(p) )     return 4;
        iToken = Psr_ManReadName( p );
        if ( iToken == PRS_VER_ENDMODULE )
        {
            Vec_IntPush( &p->vSucceeded, p->pNtk->iModuleName );
            Psr_ManFinalizeNtk( p );
            return 1;
        }
        if ( iToken >= PRS_VER_INPUT && iToken <= PRS_VER_WIRE ) // declaration
            Status = Psr_ManReadDeclaration( p, iToken );
        else if ( iToken == PRS_VER_REG || iToken == PRS_VER_DEFPARAM ) // unsupported keywords
            Status = Psr_ManUtilSkipUntil( p, ';' );
        else // read instance
        {
            if ( iToken == PRS_VER_ASSIGN )
                Status = Psr_ManReadAssign( p );
            else
                Status = Psr_ManReadInstance( p, iToken );
            if ( Status == 0 )
            {
                if ( Psr_ManUtilSkipUntilWord( p, "endmodule" ) ) return Psr_ManErrorSet(p, "Cannot find \"endmodule\" keyword.", 4);
                //printf( "Warning! Failed to parse \"%s\". Adding module \"%s\" as blackbox.\n", 
                //    Abc_NamStr(p->pStrs, iToken), Abc_NamStr(p->pStrs, p->pNtk->iModuleName) );
                Vec_IntPush( &p->vFailed, p->pNtk->iModuleName );
                // cleanup
                Vec_IntErase( &p->pNtk->vWires );
                Vec_IntErase( &p->pNtk->vWiresR );
                Vec_IntErase( &p->pNtk->vSlices );
                Vec_IntErase( &p->pNtk->vConcats );
                Vec_IntErase( &p->pNtk->vBoxes );
                Vec_IntErase( &p->pNtk->vObjs );
                p->fUsingTemp2 = 0;
                // add
                Psr_ManFinalizeNtk( p );
                Psr_ManErrorClear( p );
                return 3;
            }
        }
        if ( !Status )                      return 4;
        if ( Psr_ManUtilSkipSpaces(p) )     return 4;
    }
    return Psr_ManErrorSet(p, "Cannot find \";\" in the module definition.", 4);
}
static inline int Psr_ManReadDesign( Psr_Man_t * p )
{
    while ( 1 )
    {
        int RetValue = Psr_ManReadModule( p );
        if ( RetValue == 0 ) // end of file
            break;
        if ( RetValue == 1 ) // successfully parsed
            continue;
        if ( RetValue == 2 ) // recognized as primitive
            continue;
        if ( RetValue == 3 ) // failed and skipped
            continue;
        if ( RetValue == 4 ) // error
            return 0;
        assert( 0 );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Psr_ManPrintModules( Psr_Man_t * p )
{
    char * pName; int i; 
    printf( "Succeeded parsing %d models:\n", Vec_IntSize(&p->vSucceeded) );
    Psr_ManForEachNameVec( &p->vSucceeded, p, pName, i )
        printf( " %s", pName );
    printf( "\n" );
    printf( "Skipped %d known models:\n", Vec_IntSize(&p->vKnown) );
    Psr_ManForEachNameVec( &p->vKnown, p, pName, i )
        printf( " %s", pName );
    printf( "\n" );
    printf( "Skipped %d failed models:\n", Vec_IntSize(&p->vFailed) );
    Psr_ManForEachNameVec( &p->vFailed, p, pName, i )
        printf( " %s", pName );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Psr_ManReadVerilog( char * pFileName )
{
    Vec_Ptr_t * vPrs = NULL;
    Psr_Man_t * p = Psr_ManAlloc( pFileName );
    if ( p == NULL )
        return NULL;
    Psr_NtkAddVerilogDirectives( p );
    Psr_ManReadDesign( p );   
    //Psr_ManPrintModules( p );
    if ( Psr_ManErrorPrint(p) )
        ABC_SWAP( Vec_Ptr_t *, vPrs, p->vNtks );
    Psr_ManFree( p );
    return vPrs;
}

void Psr_ManReadVerilogTest( char * pFileName )
{
    abctime clk = Abc_Clock();
    extern void Psr_ManWriteVerilog( char * pFileName, Vec_Ptr_t * p );
    Vec_Ptr_t * vPrs = Psr_ManReadVerilog( "c/hie/dump/1/netlist_1.v" );
//    Vec_Ptr_t * vPrs = Psr_ManReadVerilog( "aga/me/me_wide.v" );
//    Vec_Ptr_t * vPrs = Psr_ManReadVerilog( "aga/ray/ray_wide.v" );
    if ( !vPrs ) return;
    printf( "Finished reading %d networks. ", Vec_PtrSize(vPrs) );
    printf( "NameIDs = %d. ", Abc_NamObjNumMax(Psr_ManNameMan(vPrs)) );
    printf( "Memory = %.2f MB. ", 1.0*Psr_ManMemory(vPrs)/(1<<20) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Psr_ManWriteVerilog( "c/hie/dump/1/netlist_1_out_new.v", vPrs );
//    Psr_ManWriteVerilog( "aga/me/me_wide_out.v", vPrs );
//    Psr_ManWriteVerilog( "aga/ray/ray_wide_out.v", vPrs );
//    Abc_NamPrint( p->pStrs );
    Psr_ManVecFree( vPrs );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

