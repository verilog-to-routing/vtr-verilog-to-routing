//===========================================================================//
// Purpose : Methods for the 'TCBP_CircutBlifHandler_c' class.
//
//           Public methods include:
//           - TCBP_CircuitBlifHandler_c, ~TCBP_CircuitBlifHandler_c
//           - Init
//           - IsValid
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

#include "TIO_PrintHandler.h"

#include "TCBP_CircuitBlifHandler.h"

//===========================================================================//
// Method         : TCBP_CircuitBlifHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TCBP_CircuitBlifHandler_c::TCBP_CircuitBlifHandler_c(
      TCD_CircuitDesign_c* pcircuitDesign )
      :
      pcircuitDesign_( 0 )
{
   if( pcircuitDesign )
   {
      this->Init( pcircuitDesign );
   }
}

//===========================================================================//
// Method         : ~TCBP_CircuitBlifHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TCBP_CircuitBlifHandler_c::~TCBP_CircuitBlifHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TCBP_CircuitBlifHandler_c::Init( 
      TCD_CircuitDesign_c* pcircuitDesign )
{
   this->pcircuitDesign_ = pcircuitDesign;

   return( this->pcircuitDesign_ ? true : false );
}

//===========================================================================//
// Method         : IsValid
// Purpose        : Return true if this circuit handler was able to properly 
//                  initialize the current 'pcircuitDesign_' object.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TCBP_CircuitBlifHandler_c::IsValid( void ) const
{
   bool isValid = false;

   if( this->pcircuitDesign_ )
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      if( printHandler.IsWithinMaxErrorCount( ) &&
          printHandler.IsWithinMaxWarningCount( ))
      {
         isValid = true;
      }
   }
   return( isValid );
}
