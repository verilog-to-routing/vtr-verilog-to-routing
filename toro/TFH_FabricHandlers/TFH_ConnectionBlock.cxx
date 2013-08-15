//===========================================================================//
// Purpose : Method definitions for the TFH_ConnectionBlock class.
//
//           Public methods include:
//           - TFH_ConnectionBlock_c, ~TFH_ConnectionBlock_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TFH_StringUtils.h"
#include "TFH_ConnectionBlock.h"

//===========================================================================//
// Method         : TFH_ConnectionBlock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
TFH_ConnectionBlock_c::TFH_ConnectionBlock_c( 
      void )
      : 
      vpr_pinIndex_( -1 ),
      pinSide_( TC_SIDE_UNDEFINED )
{
} 

//===========================================================================//
TFH_ConnectionBlock_c::TFH_ConnectionBlock_c( 
      const TGO_Point_c&  vpr_blockOrigin,
            int           vpr_pinIndex )
      :
      vpr_blockOrigin_( vpr_blockOrigin ),
      vpr_pinIndex_( vpr_pinIndex ),
      pinSide_( TC_SIDE_UNDEFINED )
{
} 

//===========================================================================//
TFH_ConnectionBlock_c::TFH_ConnectionBlock_c( 
      const TGO_Point_c&      vpr_blockOrigin,
            int               vpr_pinIndex,
      const string&           srPinName,
            TC_SideMode_t     pinSide,
      const TFH_BitPattern_t& connectionPattern )
      :
      vpr_blockOrigin_( vpr_blockOrigin ),
      vpr_pinIndex_( vpr_pinIndex ),
      srPinName_( srPinName ),
      pinSide_( pinSide ),
      connectionPattern_( connectionPattern )
{
} 

//===========================================================================//
TFH_ConnectionBlock_c::TFH_ConnectionBlock_c( 
      const TGO_Point_c&      vpr_blockOrigin,
            int               vpr_pinIndex,
      const char*             pszPinName,
            TC_SideMode_t     pinSide,
      const TFH_BitPattern_t& connectionPattern )
      :
      vpr_blockOrigin_( vpr_blockOrigin ),
      vpr_pinIndex_( vpr_pinIndex ),
      srPinName_( TIO_PSZ_STR( pszPinName )),
      pinSide_( pinSide ),
      connectionPattern_( connectionPattern )
{
} 

//===========================================================================//
TFH_ConnectionBlock_c::TFH_ConnectionBlock_c( 
      const TFH_ConnectionBlock_c& connectionBlock )
      :
      vpr_blockOrigin_( connectionBlock.vpr_blockOrigin_ ),
      vpr_pinIndex_( connectionBlock.vpr_pinIndex_ ),
      srPinName_( connectionBlock.srPinName_ ),
      pinSide_( connectionBlock.pinSide_ ),
      connectionPattern_( connectionBlock.connectionPattern_ )
{
} 

//===========================================================================//
// Method         : ~TFH_ConnectionBlock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
TFH_ConnectionBlock_c::~TFH_ConnectionBlock_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
TFH_ConnectionBlock_c& TFH_ConnectionBlock_c::operator=( 
      const TFH_ConnectionBlock_c& connectionBlock )
{
   if( &connectionBlock != this )
   {
      this->vpr_blockOrigin_ = connectionBlock.vpr_blockOrigin_;
      this->vpr_pinIndex_ = connectionBlock.vpr_pinIndex_;
      this->srPinName_ = connectionBlock.srPinName_;
      this->pinSide_ = connectionBlock.pinSide_;
      this->connectionPattern_ = connectionBlock.connectionPattern_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_ConnectionBlock_c::operator<( 
      const TFH_ConnectionBlock_c& connectionBlock ) const
{
   bool isLessThan = false;
   if( this->vpr_blockOrigin_.operator<( connectionBlock.vpr_blockOrigin_ ))
   {
      isLessThan = true;
   }
   else if( this->vpr_blockOrigin_ == connectionBlock.vpr_blockOrigin_ )
   {
      if( this->vpr_pinIndex_ < connectionBlock.vpr_pinIndex_ )
      {
         isLessThan = true;
      }
   }
   return( isLessThan );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_ConnectionBlock_c::operator==( 
      const TFH_ConnectionBlock_c& connectionBlock ) const
{
   return(( this->vpr_blockOrigin_ == connectionBlock.vpr_blockOrigin_ ) &&
          ( this->vpr_pinIndex_ == connectionBlock.vpr_pinIndex_ ) &&
          ( this->srPinName_ == connectionBlock.srPinName_ ) &&
          ( this->pinSide_ == connectionBlock.pinSide_ ) &&
          ( this->connectionPattern_ == connectionBlock.connectionPattern_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
bool TFH_ConnectionBlock_c::operator!=( 
      const TFH_ConnectionBlock_c& connectionBlock ) const
{
   return( !this->operator==( connectionBlock ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/24/13 jeffr : Original
//===========================================================================//
void TFH_ConnectionBlock_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<connection_block" ); 

   if( this->vpr_blockOrigin_.IsValid( ))
   {
      string srBlockOrigin;
      this->vpr_blockOrigin_.ExtractString( &srBlockOrigin );
      printHandler.Write( pfile, 0, " block_origin=(%s)", TIO_SR_STR( srBlockOrigin ));
   }
   if( this->vpr_pinIndex_ != -1 )
   {
      printHandler.Write( pfile, 0, " pin_index=\"%d\"", this->vpr_pinIndex_ );
   }
   if( this->srPinName_.length( ))
   {
      printHandler.Write( pfile, 0, " pin_name=\"%s\"", TIO_SR_STR( this->srPinName_ ));
   }
   if( this->pinSide_ != TC_SIDE_UNDEFINED )
   {
      string srPinSide;
      TC_ExtractStringSideMode( this->pinSide_, &srPinSide );
      printHandler.Write( pfile, 0, " pin_side=\"%s\"", TIO_SR_STR( srPinSide ));
   }
   printHandler.Write( pfile, 0, ">\n" );

   if( this->connectionPattern_.IsValid( ))
   {
      spaceLen += 3;
      string srConnectionBoxPattern;
      this->connectionPattern_.ExtractString( &srConnectionBoxPattern );
      printHandler.Write( pfile, spaceLen + 3, "<cb> %s </cb>\n",
                                               TIO_SR_STR( srConnectionBoxPattern ));
   }
} 
