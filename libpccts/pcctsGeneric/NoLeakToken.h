//============================================================================//
// Purpose: The NoLeakToken for PCCTS generated parsers. 
//          Based on the PCCTS Notes distribution and "noleakTok"
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

#ifndef NOLEAKTOKEN_H
#define NOLEAKTOKEN_H

#include "SimpleToken.h"

//============================================================================//

struct NoLeakToken : public SimpleToken {

   //------------------------------------------------------------------------//
   // Author           : Tom Ferguson
   // Revision history
   //   Original       : 11/25/97
   //   Purpose        : Shamelessly stolen from the PCCTS "notes" 
   //                    distribution
   //------------------------------------------------------------------------//

   public:

      static int           counter; 
      static NoLeakToken*  listHead;   
      NoLeakToken*         next;
      NoLeakToken*         prev;
      int                  serial;

      NoLeakToken();
      virtual ~NoLeakToken();
      NoLeakToken( const NoLeakToken & );

      NoLeakToken&         operator = ( const NoLeakToken & );

      static void          clearList();
      static void          clearCounter();
      static void          destroyList();
      virtual void         dumpNode( const char* s = 0);
      static void          dumpList(); 
      virtual ANTLRChar*   getText()=0;
      virtual void         insertNode();  
      virtual void         removeNode();   
      virtual void         setText( ANTLRChar * ) = 0;
      
   private:
      void init();
};
//============================================================================//

#endif

