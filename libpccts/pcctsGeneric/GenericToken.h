//============================================================================//
// Purpose: A generic derived token class derived from the NoLeakToken
//          class. 
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

#ifndef GENERICTOKEN_H
#define GENERICTOKEN_H

#include "NoLeakToken.h"
#include "AToken.h"

//============================================================================//

struct GenericToken : public NoLeakToken {

   //------------------------------------------------------------------------//
   // Author           : 
   // Revision history
   //   Original       : 11/27/97
   //   Purpose        : A derived "generic" token usable by all applications 
   //                    which don't want to use tokens which "leak" memory
   //------------------------------------------------------------------------//

   public:

       ANTLRChar* pText;

       GenericToken();
       virtual ~GenericToken();
       GenericToken( ANTLRTokenType t );
       GenericToken( ANTLRTokenType t, ANTLRChar* text, int line );
       GenericToken( const GenericToken & );

       GenericToken&                operator = ( const GenericToken & );
       virtual void                 dumpNode( const char* s = 0 );
       virtual ANTLRChar*           getText() { return pText; }
       virtual ANTLRAbstractToken*  makeToken( ANTLRTokenType ANTLRTokenType, 
                                               ANTLRChar* text, 
                                               int line );
       virtual void                 setText( ANTLRChar* s );

   private:

       void init();
};
//============================================================================//

typedef GenericToken ANTLRToken;
#endif

