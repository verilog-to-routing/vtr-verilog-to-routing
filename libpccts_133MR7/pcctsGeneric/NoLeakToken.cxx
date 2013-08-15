//============================================================================//
// Purpose: The NoLeakToken class for PPCTS. 
//          Obtained from PCCTS Notes distribution.
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
#include <stdio.h>

void NoLeakToken::init() {
   next=0;
   prev=0;
   serial=++counter;
   insertNode();
}

NoLeakToken::NoLeakToken() {
   init();
   setLine(0);
}

NoLeakToken::~NoLeakToken() {
////   dumpNode( "NoLeak Token Destructor" );
   removeNode();
}

NoLeakToken::NoLeakToken( const NoLeakToken& from ) 
   :  SimpleToken( from ) {
   init();
}; 

//  don't need to copy information because both elements already on list

NoLeakToken & NoLeakToken::operator = ( const NoLeakToken& from ) {
   this->SimpleToken::operator = ( from );
   return *this;
}      

void NoLeakToken::insertNode() {
   if ( listHead == 0 ) {
      listHead = this;
      prev = this;
      next = this;
   } 
   else {
      next = listHead;
      prev = listHead->prev;
      next->prev = this;
      prev->next = this;      
      listHead = this;
   }
}

void NoLeakToken::removeNode() {

   //
   // prev==0  and next==0 imply element has already been removed from
   //   list, probably as a result of a clearList() operation
   //

   if ( next != 0 && prev != 0 ) {
      next->prev = prev;
      prev->next = next;

      if ( listHead == this ) {
         listHead = next;

         if ( listHead == this ) {
            listHead = 0;
         }
      }
      prev = 0;
      next = 0;     
   }
}

void NoLeakToken::dumpNode( const char* s ) {

//   ANTLRChar *     p;

//   if (s != 0) {printf("%s ",s);};
//   if (getType() == Eof) {
//     printf("TokenType \"EOF\" Token # %d\n",serial);
//   } else {
//     p=getText();
//     if (p == 0) {
// p="";
//     };
//    printf("TokenType (%s) Text=(%s) Token # %d  Line=%d\n",
//    P::tokenName(getType()),
//    p,
//    serial,     
//    getLine());
//   };
}

void NoLeakToken::dumpList() {

   NoLeakToken* element;

   if ( listHead == 0 ) {
      printf( "\n*** Token list is empty ***\n" );
   } 
   else {
      printf( "\n*** Dump of Tokens on list ***\n\n" );
      element = listHead;

      do {
         element->dumpNode( "DumpList" );
         element = element->next;
      } while ( element != listHead );

      printf( "\n*** End of Dump ***\n\n" );
   }
}

void NoLeakToken::destroyList() {
   while ( listHead != 0 ) {
      delete listHead;           // uses side-effect of destructor
   }
}

void NoLeakToken::clearCounter() {
   counter = 0;
}

void NoLeakToken::clearList() {
   while (listHead != 0) {       // uses side-effect of removeNode
      listHead->removeNode();
   }
}

int NoLeakToken::counter = 0;
NoLeakToken* NoLeakToken::listHead = 0;

