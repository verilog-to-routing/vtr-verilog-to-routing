/**CFile****************************************************************

  FileName    [cbaTypes.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 21, 2015.]

  Revision    [$Id: cbaTypes.h,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__base__cba__cba__types_h
#define ABC__base__cba__cba__types_h

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
    CBA_OBJ_NONE = 0,  // 00:  unused
    CBA_OBJ_PI,        // 01:  input
    CBA_OBJ_PO,        // 02:  output
    CBA_OBJ_BOX,       // 03:  box

    CBA_BOX_CF,        // 04: 
    CBA_BOX_CT,        // 05:   
    CBA_BOX_CX,        // 06:   
    CBA_BOX_CZ,        // 07:  

    CBA_BOX_BUF,       // 08:    
    CBA_BOX_INV,       // 09:    
    CBA_BOX_AND,       // 10:    
    CBA_BOX_NAND,      // 11:   
    CBA_BOX_OR,        // 12:     
    CBA_BOX_NOR,       // 13:    
    CBA_BOX_XOR,       // 14:    
    CBA_BOX_XNOR,      // 15:   
    CBA_BOX_SHARP,     // 16:  
    CBA_BOX_SHARPL,    // 17:  
    CBA_BOX_MUX,       // 18:    
    CBA_BOX_MAJ,       // 19:    

    CBA_BOX_ABC,       // 20:
    CBA_BOX_BA,        // 21:
    CBA_BOX_BO,        // 22:
    CBA_BOX_BX,        // 23:
    CBA_BOX_BN,        // 24:
    CBA_BOX_BAO,       // 25:
    CBA_BOX_BOA,       // 26:

    CBA_BOX_RAND,      // 27:
    CBA_BOX_RNAND,     // 28:
    CBA_BOX_ROR,       // 29:
    CBA_BOX_RNOR,      // 30:
    CBA_BOX_RXOR,      // 31:
    CBA_BOX_RXNOR,     // 32:

    CBA_BOX_LNOT,      // 33
    CBA_BOX_LAND,      // 34:
    CBA_BOX_LNAND,     // 35:
    CBA_BOX_LOR,       // 36:
    CBA_BOX_LNOR,      // 37:
    CBA_BOX_LXOR,      // 38:
    CBA_BOX_LXNOR,     // 39:

    CBA_BOX_NMUX,      // 40:  
    CBA_BOX_SEL,       // 41:
    CBA_BOX_PSEL,      // 42:
    CBA_BOX_ENC,       // 43:
    CBA_BOX_PENC,      // 44:
    CBA_BOX_DEC,       // 45:
    CBA_BOX_EDEC,      // 46:

    CBA_BOX_ADD,       // 47:
    CBA_BOX_SUB,       // 48:
    CBA_BOX_MUL,       // 49:
    CBA_BOX_SMUL,      // 50:
    CBA_BOX_DIV,       // 51:
    CBA_BOX_MOD,       // 52:
    CBA_BOX_REM,       // 53:
    CBA_BOX_POW,       // 54:
    CBA_BOX_MIN,       // 55:
    CBA_BOX_SQRT,      // 56:
    CBA_BOX_ABS,       // 57:

    CBA_BOX_SLTHAN,    // 58:
    CBA_BOX_LTHAN,     // 59:
    CBA_BOX_LETHAN,    // 60:
    CBA_BOX_METHAN,    // 61:
    CBA_BOX_MTHAN,     // 62:
    CBA_BOX_EQU,       // 63:
    CBA_BOX_NEQU,      // 64:

    CBA_BOX_SHIL,      // 65:
    CBA_BOX_SHIR,      // 66:
    CBA_BOX_SHILA,     // 67:
    CBA_BOX_SHIRA,     // 68:
    CBA_BOX_ROTL,      // 69:
    CBA_BOX_ROTR,      // 70:

    CBA_BOX_NODE,      // 71:  
    CBA_BOX_LUT,       // 72: 
    CBA_BOX_GATE,      // 73:  
    CBA_BOX_TABLE,     // 74:  

    CBA_BOX_TRI,       // 75:
    CBA_BOX_RAM,       // 76:
    CBA_BOX_RAMR,      // 77:
    CBA_BOX_RAMW,      // 78:
    CBA_BOX_RAMWC,     // 79:
    CBA_BOX_RAML,      // 80:
    CBA_BOX_RAMS,      // 81:
    CBA_BOX_RAMBOX,    // 82:

    CBA_BOX_LATCH,     // 83:
    CBA_BOX_LATCHRS,   // 84:
    CBA_BOX_DFF,       // 85:
    CBA_BOX_DFFCPL,    // 86:
    CBA_BOX_DFFRS,     // 87:

    CBA_BOX_SLICE,     // 88:
    CBA_BOX_CONCAT,    // 89: 

    CBA_BOX_LAST       // 90
} Cba_ObjType_t; 


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

