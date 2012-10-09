//============================================================================//
// Purpose: A token buffer class designed to work with NoLeakToken class. 
//          Based on the PCCTS Notes distribution.
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

#ifndef GENERICTOKENBUFFER_H
#define GENERICTOKENBUFFER_H

#include "GenericToken.h"

#include "ATokenBuffer.h"

//============================================================================//

struct GenericTokenBuffer : public ANTLRTokenBuffer {

   //------------------------------------------------------------------------//
   // Author           : Tom Ferguson
   // Revision history
   //   Original       : 11/25/97
   //   Purpose        : Shamelessly stolen from PCCTS notes distribution
   //------------------------------------------------------------------------//

   GenericTokenBuffer( ANTLRTokenStream *in, int k=1, int chksz=200);

   virtual                       ~GenericTokenBuffer() {};
   virtual ANTLRAbstractToken*   getANTLRToken();
}; 
//============================================================================//

#endif
