//===========================================================================//
// Purpose : Method definitions for the TCH_RelativeMove class.
//
//           Public methods include:
//           - TCH_RelativeMove_c, ~TCH_RelativeMove_c
//           - operator=
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

#include "TC_StringUtils.h"

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

#include "TIO_PrintHandler.h"

#include "TCH_RelativeMove.h"

//===========================================================================//
// Method         : TCH_RelativeMove_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeMove_c::TCH_RelativeMove_c( 
      void )
      :
      fromMacroIndex_( TCH_RELATIVE_MACRO_UNDEFINED ),
      fromEmpty_( false ),
      toMacroIndex_( TCH_RELATIVE_MACRO_UNDEFINED ),
      toEmpty_( false )
{
} 

//===========================================================================//
TCH_RelativeMove_c::TCH_RelativeMove_c( 
            size_t       fromMacroIndex, 
      const TGO_Point_c& fromPoint, 
            bool         fromEmpty,
            size_t       toMacroIndex, 
      const TGO_Point_c& toPoint,
            bool         toEmpty )
      :
      fromMacroIndex_( fromMacroIndex ),
      fromPoint_( fromPoint ),
      fromEmpty_( fromEmpty ),
      toMacroIndex_( toMacroIndex ),
      toPoint_( toPoint ),
      toEmpty_( toEmpty )
{
} 

//===========================================================================//
TCH_RelativeMove_c::TCH_RelativeMove_c( 
            size_t       fromMacroIndex, 
      const TGO_Point_c& fromPoint, 
            size_t       toMacroIndex, 
      const TGO_Point_c& toPoint )
      :
      fromMacroIndex_( fromMacroIndex ),
      fromPoint_( fromPoint ),
      fromEmpty_( false ),
      toMacroIndex_( toMacroIndex ),
      toPoint_( toPoint ),
      toEmpty_( false )
{
} 

//===========================================================================//
TCH_RelativeMove_c::TCH_RelativeMove_c( 
            size_t       fromMacroIndex, 
      const TGO_Point_c& fromPoint, 
      const TGO_Point_c& toPoint )
      :
      fromMacroIndex_( fromMacroIndex ),
      fromPoint_( fromPoint ),
      fromEmpty_( false ),
      toMacroIndex_( TCH_RELATIVE_MACRO_UNDEFINED ),
      toPoint_( toPoint ),
      toEmpty_( false )
{
} 

//===========================================================================//
TCH_RelativeMove_c::TCH_RelativeMove_c( 
      const TGO_Point_c& fromPoint, 
            size_t       toMacroIndex, 
      const TGO_Point_c& toPoint )
      :
      fromMacroIndex_( TCH_RELATIVE_MACRO_UNDEFINED ),
      fromPoint_( fromPoint ),
      fromEmpty_( false ),
      toMacroIndex_( toMacroIndex ),
      toPoint_( toPoint ),
      toEmpty_( false )
{
} 

//===========================================================================//
TCH_RelativeMove_c::TCH_RelativeMove_c( 
      const TGO_Point_c& fromPoint, 
      const TGO_Point_c& toPoint )
      :
      fromMacroIndex_( TCH_RELATIVE_MACRO_UNDEFINED ),
      fromPoint_( fromPoint ),
      fromEmpty_( false ),
      toMacroIndex_( TCH_RELATIVE_MACRO_UNDEFINED ),
      toPoint_( toPoint ),
      toEmpty_( false )
{
} 

//===========================================================================//
TCH_RelativeMove_c::TCH_RelativeMove_c( 
      const TCH_RelativeMove_c& relativeMove )
      :
      fromMacroIndex_( relativeMove.fromMacroIndex_ ),
      fromPoint_( relativeMove.fromPoint_ ),
      fromEmpty_( relativeMove.fromEmpty_ ),
      toMacroIndex_( relativeMove.toMacroIndex_ ),
      toPoint_( relativeMove.toPoint_ ),
      toEmpty_( relativeMove.toEmpty_ )
{
} 

//===========================================================================//
// Method         : ~TCH_RelativeMove_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeMove_c::~TCH_RelativeMove_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeMove_c& TCH_RelativeMove_c::operator=( 
      const TCH_RelativeMove_c& relativeMove )
{
   if( &relativeMove != this )
   {
      this->fromMacroIndex_ = relativeMove.fromMacroIndex_;
      this->fromPoint_ = relativeMove.fromPoint_;
      this->fromEmpty_ = relativeMove.fromEmpty_;
      this->toMacroIndex_ = relativeMove.toMacroIndex_;
      this->toPoint_ = relativeMove.toPoint_;
      this->toEmpty_ = relativeMove.toEmpty_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeMove_c::operator==( 
      const TCH_RelativeMove_c& relativeMove ) const
{
   return(( this->fromMacroIndex_ == relativeMove.fromMacroIndex_ ) &&
          ( this->fromPoint_ == relativeMove.fromPoint_ ) &&
          ( this->fromEmpty_ == relativeMove.fromEmpty_ ) &&
          ( this->toMacroIndex_ == relativeMove.toMacroIndex_ ) &&
          ( this->toPoint_ == relativeMove.toPoint_ ) &&
          ( this->toEmpty_ == relativeMove.toEmpty_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeMove_c::operator!=( 
      const TCH_RelativeMove_c& relativeMove ) const
{
   return( !this->operator==( relativeMove ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeMove_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<move" );
   if( this->fromMacroIndex_ != TCH_RELATIVE_MACRO_UNDEFINED )
   {
      printHandler.Write( pfile, 0, " from_macro_index=%u", this->fromMacroIndex_ );
   }
   if( this->fromPoint_.IsValid( ))
   {
      string srFromPoint;
      this->fromPoint_.ExtractString( &srFromPoint );
      printHandler.Write( pfile, 0, " from_point=(%s)", TIO_SR_STR( srFromPoint ));
   }
   printHandler.Write( pfile, 0, " from_empty=%s", TIO_BOOL_VAL( this->fromEmpty_ ));

   if( this->toMacroIndex_ != TCH_RELATIVE_NODE_UNDEFINED )
   {
      printHandler.Write( pfile, 0, " to_node_index=%u", this->toMacroIndex_ );
   }
   if( this->toPoint_.IsValid( ))
   {
      string srToPoint;
      this->toPoint_.ExtractString( &srToPoint );
      printHandler.Write( pfile, 0, " to_point=(%s)", TIO_SR_STR( srToPoint ));
   }
   printHandler.Write( pfile, 0, " to_empty=%s", TIO_BOOL_VAL( this->toEmpty_ ));
   printHandler.Write( pfile, 0, ">\n" );
} 
