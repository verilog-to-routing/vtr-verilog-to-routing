//===========================================================================//
// Purpose : Declaration and inline definition(s) for a TIO_CustomOutput 
//           class. 
//
//           Inline methods include:
//           - TIO_CustomOutput_c, ~TIO_CustomOutput_c
//           - SetHandler
//           - SetEnabled
//           - IsEnabled
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
//---------------------------------------------------------------------------//

#ifndef TIO_CUSTOM_OUTPUT_H
#define TIO_CUSTOM_OUTPUT_H

#include "TIO_Typedefs.h"

//---------------------------------------------------------------------------//
// Define function pointer for an installable custom output handler
//---------------------------------------------------------------------------//
typedef void ( * TIO_PFX_CustomHandler_t )( TIO_PrintMode_t printMode,
                                            const char* pszPrintText,
                                            const char* pszPrintSrc );

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TIO_CustomOutput_c
{
public:

   TIO_CustomOutput_c( TIO_PFX_CustomHandler_t pfxCustomHandler = 0 );
   ~TIO_CustomOutput_c( void );

   bool Write( TIO_PrintMode_t printMode,
               const char* pszPrintText,
               const char* pszPrintSrc ) const;

   void SetHandler( TIO_PFX_CustomHandler_t pfxCustomHandler );
   void SetEnabled( bool isEnabled );
 
   bool IsEnabled( void ) const;

private:

   TIO_PFX_CustomHandler_t pfxCustomHandler_;
                           // Ptr to installed custom output handler function
   bool isEnabled_;        // Used to enable/disable output state
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline TIO_CustomOutput_c::TIO_CustomOutput_c( 
      TIO_PFX_CustomHandler_t pfxCustomHandler )
      :
      pfxCustomHandler_( pfxCustomHandler ),
      isEnabled_( pfxCustomHandler ? true : false )
{
}

//===========================================================================//
inline TIO_CustomOutput_c::~TIO_CustomOutput_c( 
      void )
{
}

//===========================================================================//
inline void TIO_CustomOutput_c::SetHandler( 
      TIO_PFX_CustomHandler_t pfxCustomHandler )
{
   this->pfxCustomHandler_ = pfxCustomHandler;
   this->isEnabled_ = ( pfxCustomHandler ? true : false );
}

//===========================================================================//
inline void TIO_CustomOutput_c::SetEnabled( 
      bool isEnabled )
{
   this->isEnabled_ = isEnabled;
}

//===========================================================================//
inline bool TIO_CustomOutput_c::IsEnabled( 
      void ) const
{
   return( this->isEnabled_ );
}

#endif 
