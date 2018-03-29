/**CFile****************************************************************

  FileName    [acbTypes.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 21, 2015.]

  Revision    [$Id: acbTypes.h,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__acb__acb__types_h
#define ABC__base__acb__acb__types_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// network objects
typedef enum { 
    ABC_OPER_NONE = 0,     // 00  unused
    ABC_OPER_PI,           // 01  input
    ABC_OPER_PO,           // 02  output
    ABC_OPER_CI,           // 03  combinational input
    ABC_OPER_CO,           // 04  combinational output
    ABC_OPER_FON,          // 05  output placeholder
    ABC_OPER_BOX,          // 06  box

    ABC_OPER_CONST_F,      // 07
    ABC_OPER_CONST_T,      // 08
    ABC_OPER_CONST_X,      // 09
    ABC_OPER_CONST_Z,      // 10

    ABC_OPER_BIT_BUF,      // 11
    ABC_OPER_BIT_INV,      // 12
    ABC_OPER_BIT_AND,      // 13
    ABC_OPER_BIT_NAND,     // 14
    ABC_OPER_BIT_OR,       // 15
    ABC_OPER_BIT_NOR,      // 16
    ABC_OPER_BIT_XOR,      // 17
    ABC_OPER_BIT_NXOR,     // 18
    ABC_OPER_BIT_SHARP,    // 19
    ABC_OPER_BIT_SHARPL,   // 20
    ABC_OPER_BIT_MUX,      // 21
    ABC_OPER_BIT_MAJ,      // 22

    ABC_OPER_ABC,          // 23
    ABC_OPER_BA,           // 24
    ABC_OPER_BO,           // 25
    ABC_OPER_BX,           // 26
    ABC_OPER_BN,           // 27
    ABC_OPER_BAO,          // 28
    ABC_OPER_BOA,          // 29

    ABC_OPER_RED_AND,      // 30
    ABC_OPER_RED_NAND,     // 31
    ABC_OPER_RED_OR,       // 32
    ABC_OPER_RED_NOR,      // 33
    ABC_OPER_RED_XOR,      // 34
    ABC_OPER_RED_NXOR,     // 35

    ABC_OPER_LOGIC_NOT,    // 36
    ABC_OPER_LOGIC_AND,    // 37
    ABC_OPER_LOGIC_NAND,   // 38
    ABC_OPER_LOGIC_OR,     // 39
    ABC_OPER_LOGIC_NOR,    // 40
    ABC_OPER_LOGIC_XOR,    // 41
    ABC_OPER_LOGIC_XNOR,   // 42

    ABC_OPER_SEL_NMUX,     // 43
    ABC_OPER_SEL_SEL,      // 44
    ABC_OPER_SEL_PSEL,     // 45
    ABC_OPER_SEL_ENC,      // 46
    ABC_OPER_SEL_PENC,     // 47
    ABC_OPER_SEL_DEC,      // 48
    ABC_OPER_SEL_EDEC,     // 49

    ABC_OPER_ARI_ADD,      // 50
    ABC_OPER_ARI_SUB,      // 51
    ABC_OPER_ARI_MUL,      // 52
    ABC_OPER_ARI_SMUL,     // 53
    ABC_OPER_ARI_DIV,      // 54
    ABC_OPER_ARI_MOD,      // 55
    ABC_OPER_ARI_REM,      // 56
    ABC_OPER_ARI_POW,      // 57
    ABC_OPER_ARI_MIN,      // 58
    ABC_OPER_ARI_SQRT,     // 59
    ABC_OPER_ARI_ABS,      // 60

    ABC_OPER_COMP_SLESS,   // 61
    ABC_OPER_COMP_LESS,    // 62
    ABC_OPER_COMP_LESSEQU, // 63
    ABC_OPER_COMP_MOREEQU, // 64
    ABC_OPER_COMP_MORE,    // 65
    ABC_OPER_COMP_EQU,     // 66
    ABC_OPER_COMP_NOTEQU,  // 67

    ABC_OPER_SHIFT_L,      // 68
    ABC_OPER_SHIFT_R,      // 69
    ABC_OPER_SHIFT_LA,     // 70
    ABC_OPER_SHIFT_RA,     // 71
    ABC_OPER_SHIFT_ROTL,   // 72
    ABC_OPER_SHIFT_ROTR,   // 73

    ABC_OPER_NODE,         // 74
    ABC_OPER_LUT,          // 75
    ABC_OPER_GATE,         // 76
    ABC_OPER_TABLE,        // 77

    ABC_OPER_TRI,          // 78
    ABC_OPER_RAM,          // 79
    ABC_OPER_RAMR,         // 80
    ABC_OPER_RAMW,         // 81
    ABC_OPER_RAMWC,        // 82
    ABC_OPER_RAML,         // 83
    ABC_OPER_RAMS,         // 84
    ABC_OPER_RAMBOX,       // 85

    ABC_OPER_LATCH,        // 86
    ABC_OPER_LATCHRS,      // 87
    ABC_OPER_DFF,          // 88
    ABC_OPER_DFFCPL,       // 89
    ABC_OPER_DFFRS,        // 90

    ABC_OPER_SLICE,        // 91
    ABC_OPER_CONCAT,       // 92
    ABC_OPER_ZEROPAD,      // 93
    ABC_OPER_SIGNEXT,      // 94

    ABC_OPER_LOGIC_IMPL,   // 95
    ABC_OPER_ARI_SQUARE,   // 96
    ABC_OPER_CONST,        // 97

    ABC_OPER_LAST          // 98
} Acb_ObjType_t; 


// printing operator types
static inline char * Abc_OperName( int Type )
{
    if ( Type == ABC_OPER_NONE         )   return NULL;
    if ( Type == ABC_OPER_PI           )   return "pi";     
    if ( Type == ABC_OPER_PO           )   return "po";     
    if ( Type == ABC_OPER_CI           )   return "ci";     
    if ( Type == ABC_OPER_CO           )   return "co";     
    if ( Type == ABC_OPER_FON          )   return "fon";     
    if ( Type == ABC_OPER_BOX          )   return "box";    
    
    if ( Type == ABC_OPER_BIT_BUF      )   return "buf";    
    if ( Type == ABC_OPER_BIT_INV      )   return "~";      
    if ( Type == ABC_OPER_BIT_MUX      )   return "mux";    
    if ( Type == ABC_OPER_BIT_MAJ      )   return "maj";    
    if ( Type == ABC_OPER_BIT_AND      )   return "&";      
    if ( Type == ABC_OPER_BIT_OR       )   return "|";      
    if ( Type == ABC_OPER_BIT_XOR      )   return "^";      
    if ( Type == ABC_OPER_BIT_NAND     )   return "~&";     
    if ( Type == ABC_OPER_BIT_NOR      )   return "~|";     
    if ( Type == ABC_OPER_BIT_NXOR     )   return "~^";     

    if ( Type == ABC_OPER_RED_AND      )   return "&";      
    if ( Type == ABC_OPER_RED_OR       )   return "|";      
    if ( Type == ABC_OPER_RED_XOR      )   return "^";      
    if ( Type == ABC_OPER_RED_NAND     )   return "~&";     
    if ( Type == ABC_OPER_RED_NOR      )   return "~|";     
    if ( Type == ABC_OPER_RED_NXOR     )   return "~^";     

    if ( Type == ABC_OPER_LOGIC_NOT    )   return "!";      
    if ( Type == ABC_OPER_LOGIC_IMPL   )   return "=>";     
    if ( Type == ABC_OPER_LOGIC_AND    )   return "&&";     
    if ( Type == ABC_OPER_LOGIC_OR     )   return "||";     
    if ( Type == ABC_OPER_LOGIC_XOR    )   return "^^";   

    if ( Type == ABC_OPER_ARI_ADD      )   return "+";      
    if ( Type == ABC_OPER_ARI_SUB      )   return "-";      
    if ( Type == ABC_OPER_ARI_MUL      )   return "*";      
    if ( Type == ABC_OPER_ARI_DIV      )   return "/";      
    if ( Type == ABC_OPER_ARI_REM      )   return "%";      
    if ( Type == ABC_OPER_ARI_MOD      )   return "mod";    
    if ( Type == ABC_OPER_ARI_POW      )   return "**";     
    if ( Type == ABC_OPER_ARI_MIN      )   return "-";      
    if ( Type == ABC_OPER_ARI_SQRT     )   return "sqrt";   
    if ( Type == ABC_OPER_ARI_SQUARE   )   return "squar";  
    
    if ( Type == ABC_OPER_COMP_EQU     )   return "==";     
    if ( Type == ABC_OPER_COMP_NOTEQU  )   return "!=";     
    if ( Type == ABC_OPER_COMP_LESS    )   return "<";      
    if ( Type == ABC_OPER_COMP_MORE    )   return ">";      
    if ( Type == ABC_OPER_COMP_LESSEQU )   return "<=";     
    if ( Type == ABC_OPER_COMP_MOREEQU )   return ">=";     

    if ( Type == ABC_OPER_SHIFT_L      )   return "<<";     
    if ( Type == ABC_OPER_SHIFT_R      )   return ">>";     
    if ( Type == ABC_OPER_SHIFT_LA     )   return "<<<";    
    if ( Type == ABC_OPER_SHIFT_RA     )   return ">>>";    
    if ( Type == ABC_OPER_SHIFT_ROTL   )   return "rotL";   
    if ( Type == ABC_OPER_SHIFT_ROTR   )   return "rotR";   

    if ( Type == ABC_OPER_SLICE        )   return "[:]";    
    if ( Type == ABC_OPER_CONCAT       )   return "{}";     
    if ( Type == ABC_OPER_ZEROPAD      )   return "zPad";   
    if ( Type == ABC_OPER_SIGNEXT      )   return "sExt";   

    if ( Type == ABC_OPER_TABLE        )   return "table";  
    if ( Type == ABC_OPER_LAST         )   return NULL;     
    assert( 0 );
    return NULL;
}

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                          ITERATORS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_END


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

