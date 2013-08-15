//============================================================================//
// Purpose: A token buffer class designed to work with the no leak tokens. 
//          Based on the PCCTS Notes distribution classes.
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

#include "GenericTokenBuffer.h"
#include "DLexerBase.h"

//============================================================================//

GenericTokenBuffer::GenericTokenBuffer( ANTLRTokenStream* in, int generick, 
                                        int chksz) 
   :  ANTLRTokenBuffer( in, generick, chksz ) {

   //------------------------------------------------------------------------//
   // Author           : Tom Ferguson
   // Revision history
   //   Original       : 11/25/97
   //   Purpose        : Shamelessly stolen from PCCTS notes distribution
   //------------------------------------------------------------------------//

}                                   // GenericTokenBuffer::GenericTokenBuffer
//============================================================================//

//============================================================================//

ANTLRAbstractToken* GenericTokenBuffer::getANTLRToken() {

   //------------------------------------------------------------------------//
   // Author           : Tom Ferguson
   // Revision history
   //   Original       : 11/25/97
   //   Purpose        : Shamelessly stolen from PCCTS notes distribution
   //------------------------------------------------------------------------//

   ANTLRToken* myToken;

   myToken = ( ANTLRToken * )ANTLRTokenBuffer::getANTLRToken();

   return myToken;
}                                      // GenericTokenBuffer::getANTLRToken
//============================================================================//
