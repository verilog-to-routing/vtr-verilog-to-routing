//============================================================================//
// Purpose: A SimpleToken class for PCCTS generated parsers. 
//          Based on the PCCTS Notes distribution and "simpleToken.h"
//
// U.S. Application Specific Products
// ASIC Software Engineering Services 
// Property of Texas Instruments -- For Unrestricted Use -- Unauthorized
// reproduction and/or sale is strictly prohibited.  This product is protected
// under copyright law as an unpublished work.  Created 1997, (C) Copyright
// 1997 Texas Instruments.  All rights reserved.
// 
// 
//    -------------------------------------------------------------------
//       These  commodities are under  U.S. Government 'distribution
//       license' control.  As such, they are  not to be re-exported
//       without prior approval from the U.S. Department of Commerce.
//    -------------------------------------------------------------------
//
//============================================================================//

#ifndef SIMPLETOKEN_H
#define SIMPLETOKEN_H

#ifdef BUILDING_LIB
typedef enum ANTLRTokenType { };
#endif 

typedef char ANTLRChar;

#include "AToken.h"

//============================================================================//

class SimpleToken : public ANTLRRefCountToken {

   //------------------------------------------------------------------------//
   // Author           : Tom Ferguson
   // Revision history
   //   Original       : 11/25/97
   //   Purpose        : Shamelessly stolen from the PCCTS notes distribution
   //                    Cleaned up as per the coding standard.
   //
   //  10/31/00  kira  : FPINIT.DEV.298 - Changed inheritance from
   //                    ANTLRAbstractToken to ANTLRRefCountToken. The new base
   //                    class tracks the number of references to a token and
   //                    deletes the latter when the former is zero. Plugs
   //                    a major memory leak.
   //------------------------------------------------------------------------//

   public:
      int               _line;

      SimpleToken() { 
         setType( ( ANTLRTokenType )0 );
        _line = 0;
      }
      SimpleToken( ANTLRTokenType t ) { 
         setType( t );
        _line = 0;
      }

      ANTLRTokenType    getType() { return _type; }
      void              setType( ANTLRTokenType t )   { _type = t; }
      virtual int       getLine() { return _line; }
      void              setLine( int line ) { _line = line; }

   protected:

      ANTLRTokenType    _type;
};
//============================================================================//

#endif
