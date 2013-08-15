//===========================================================================//
// Purpose : Method definitions for the TAS_Clock class.
//
//           Public methods include:
//           - TAS_Clock_c, ~TAS_Clock_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TAS_Clock.h"

//===========================================================================//
// Method         : TAS_Clock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TAS_Clock_c::TAS_Clock_c( 
      void )
      :
      autoSize( false ),
      bufferSize( 0.0),
      capWire( 0.0 )
{
}

//===========================================================================//
TAS_Clock_c::TAS_Clock_c( 
      const TAS_Clock_c& clock )
      :
      autoSize( clock.autoSize ),
      bufferSize( clock.bufferSize ),
      capWire( clock.capWire )
{
}

//===========================================================================//
// Method         : ~TAS_Clock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TAS_Clock_c::~TAS_Clock_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TAS_Clock_c& TAS_Clock_c::operator=( 
      const TAS_Clock_c& clock )
{
   if( &clock != this )
   {
      this->autoSize = clock.autoSize;
      this->bufferSize = clock.bufferSize;
      this->capWire = clock.capWire;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TAS_Clock_c::operator==( 
      const TAS_Clock_c& clock ) const
{
   return(( this->autoSize == clock.autoSize ) &&
          ( TCTF_IsEQ( this->bufferSize, clock.bufferSize )) &&
          ( TCTF_IsEQ( this->capWire, clock.capWire )) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TAS_Clock_c::operator!=( 
      const TAS_Clock_c& clock ) const
{
   return( !this->operator==( clock ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TAS_Clock_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<clock" );
   if( this->autoSize )
   {
      printHandler.Write( pfile, 0, " buffer_size=\"auto\"" );
   }
   else if( TCTF_IsGT( this->bufferSize, 0.0 ))
   {
      printHandler.Write( pfile, 0, " buffer_size=\"%0.*f\"", 
                                    precision, this->bufferSize );
   }
   if( TCTF_IsGT( this->capWire, 0.0 ))
   {
      printHandler.Write( pfile, 0, " C_wire=\"%0.*e\"", 
                                    precision + 1, this->capWire );
   }
   printHandler.Write( pfile, 0, "/>\n" );
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TAS_Clock_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_Clock_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( ) + 1;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<clock" );
   if( this->autoSize )
   {
      printHandler.Write( pfile, 0, " buffer_size=\"auto\"" );
   }
   else if( TCTF_IsGT( this->bufferSize, 0.0 ))
   {
      printHandler.Write( pfile, 0, " buffer_size=\"%0.*f\"", 
                                    precision, this->bufferSize );
   }
   if( TCTF_IsGT( this->capWire, 0.0 ))
   {
      printHandler.Write( pfile, 0, " C_wire=\"%0.*e\"", 
                                    precision + 1, this->capWire );
   }
   printHandler.Write( pfile, 0, "/>\n" );
}
