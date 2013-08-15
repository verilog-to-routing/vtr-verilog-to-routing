//============================================================================//
// Purpose: A derived token class to prevent memory leaks. 
//          Application may choose to derive from this token class 
//          but all PCCTS-required base capability is included. 
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


#include "NoLeakToken.h"
#include "GenericToken.h"

#include "string.h"
#include <stdio.h>


void GenericToken::init() {
   pText=0;
}

GenericToken::GenericToken() {
   init();
   setLine(0);
}

GenericToken::~GenericToken() {
////   dumpNode("Token Destructor");
   delete [] pText;
   pText=0;
}

GenericToken::GenericToken(ANTLRTokenType tokenTypeNew) {
   init();
   setType(tokenTypeNew);
   setLine(0);
}

GenericToken::GenericToken( ANTLRTokenType tokenTypeNew,
                            ANTLRChar* textNew,   
                            int lineNew ) {
   init();
   setType( tokenTypeNew );
   setLine( lineNew );
   setText( textNew );
}         

GenericToken::GenericToken( const GenericToken& from ) 
   :  NoLeakToken( from ) {
   init();
   setText( from.pText );
}; 

//  create new copy of text - not just another reference to existing text

GenericToken & GenericToken::operator = ( const GenericToken& from ) {

   this->NoLeakToken::operator = ( from );

   if ( this != &from ) {
      setText( from.pText );
   };
   return *this;
}      

//  create new copy of text - not just another reference to existing text

void GenericToken::setText( ANTLRChar* s ) {

   if ( pText != 0 ) {
      delete [] pText;
      pText=0;
   };
   if ( s != 0 ) {
      pText = new char[ strlen( s ) + 1 ];
//    if ( pText == 0 ) myPanic ( "GenericToken::setText strdup failed" );
//    assert( pText !=0 );
      strcpy( pText, s );
   };
}

ANTLRAbstractToken* GenericToken::makeToken( ANTLRTokenType tokenType,
                                             ANTLRChar* text,
                                             int line ) {
   return new GenericToken( tokenType, text, line );
}

void GenericToken::dumpNode( const char* s ) {
};

