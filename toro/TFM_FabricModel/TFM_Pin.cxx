//===========================================================================//
// Purpose : Method definitions for the TFM_Pin class.
//
//           Public methods include:
//           - TFM_Pin_c, ~TFM_Pin_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
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

#include "TC_MinGrid.h"
#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TFM_Pin.h"

//===========================================================================//
// Method         : TFM_Pin_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TFM_Pin_c::TFM_Pin_c( 
      void )
      :
      type( TC_TYPE_UNDEFINED ),
      side( TC_SIDE_UNDEFINED ),
      offset( 0 ),
      delta( 0.0 ),
      width( 0.0 ),
      slice( 0 )
{
}

//===========================================================================//
TFM_Pin_c::TFM_Pin_c( 
      const string&       srName,
            TC_TypeMode_t type_,
            TC_SideMode_t side_,
            int           offset_,
            double        delta_,
            double        width_,
            unsigned int  slice_ )
      :
      TPO_Pin_t( srName ),
      type( type_ ),
      side( side_ ),
      offset( offset_ ),
      delta( delta_ ),
      width( width_ ),
      slice( slice_ )
{
}

//===========================================================================//
TFM_Pin_c::TFM_Pin_c( 
      const char*         pszName,
            TC_TypeMode_t type_,
            TC_SideMode_t side_,
            int           offset_,
            double        delta_,
            double        width_,
            unsigned int  slice_ )
      :
      TPO_Pin_t( pszName ),
      type( type_ ),
      side( side_ ),
      offset( offset_ ),
      delta( delta_ ),
      width( width_ ),
      slice( slice_ )
{
}

//===========================================================================//
TFM_Pin_c::TFM_Pin_c( 
      const TFM_Pin_c& pin )
      :
      TPO_Pin_t( pin ),
      type( pin.type ),
      side( pin.side ),
      offset( pin.offset ),
      delta( pin.delta ),
      width( pin.width ),
      slice( pin.slice ),
      connectionPattern( pin.connectionPattern )
{
}

//===========================================================================//
// Method         : ~TFM_Pin_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TFM_Pin_c::~TFM_Pin_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TFM_Pin_c& TFM_Pin_c::operator=( 
      const TFM_Pin_c& pin )
{
   if( &pin != this )
   {
      TPO_Pin_t::operator=( pin );
      this->type = pin.type;
      this->side = pin.side;
      this->offset = pin.offset;
      this->delta = pin.delta;
      this->width = pin.width;
      this->slice = pin.slice;
      this->connectionPattern = pin.connectionPattern;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TFM_Pin_c::operator<( 
      const TFM_Pin_c& pin ) const
{
   bool isLessThan = false;

   if( this->slice < pin.slice )
   {
      isLessThan = true;
   }
   else if( this->slice == pin.slice )
   {
      if( this->side < pin.side )
      {
         isLessThan = true;
      }
      else if( this->side == pin.side )
      {
         if( this->offset < pin.offset )
         {
            isLessThan = true;
         }
         else if( this->offset == pin.offset )
         {
            if( TCTF_IsLT( this->delta, pin.delta ))
            {
               isLessThan = true;
            }
            else if( TCTF_IsEQ( this->delta, pin.delta ))
            {
               if( TC_CompareStrings( this->GetName( ), pin.GetName( )) < 0 )
               {
                  isLessThan = true;
               }
            }
         }
      }
   }
   return( isLessThan );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TFM_Pin_c::operator==( 
      const TFM_Pin_c& pin ) const
{
   return( TPO_Pin_t::operator==( pin ) ? true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TFM_Pin_c::operator!=( 
      const TFM_Pin_c& pin ) const
{
   return( !this->operator==( pin ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TFM_Pin_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<pin name=\"%s\"",
                                        TIO_PSZ_STR( TPO_Pin_t::GetName( )));
   if( this->type != TC_TYPE_UNDEFINED )
   {
      string srType;
      TC_ExtractStringTypeMode( this->type, &srType );
      printHandler.Write( pfile, 0, " type=\"%s\"", 
                                    TIO_SR_STR( srType ));
   }
   if( this->side != TC_SIDE_UNDEFINED )
   {
      string srSide;
      TC_ExtractStringSideMode( this->side, &srSide );
      printHandler.Write( pfile, 0, " side=\"%s\" offset=\"%d\" delta=\"%0.*f\" width=\"%0.*f\" slice=\"%u\"", 
                                    TIO_SR_STR( srSide ),
                                    this->offset,
                                    precision, this->delta,
                                    precision, this->width,
                                    this->slice );
   }
   if( this->connectionPattern.IsValid( ))
   {
      printHandler.Write( pfile, 0, ">\n" );

      string srConnectionBoxPattern;
      this->connectionPattern.ExtractString( &srConnectionBoxPattern );
      printHandler.Write( pfile, spaceLen + 3, "<cb> %s </cb>\n",
                                               TIO_SR_STR( srConnectionBoxPattern ));

      printHandler.Write( pfile, spaceLen, "</pin>\n" );
   }
   else
   {
      printHandler.Write( pfile, 0, "/>\n" );
   }
}
