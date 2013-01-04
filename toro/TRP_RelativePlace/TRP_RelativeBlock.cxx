//===========================================================================//
// Purpose : Method definitions for the TRP_RelativeBlock class.
//
//           Public methods include:
//           - TRP_RelativeBlock_c, ~TRP_RelativeBlock_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - Reset
//
//===========================================================================//

#include "TC_StringUtils.h"

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
//                                                                           //
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
//---------------------------------------------------------------------------//

#include "TIO_PrintHandler.h"

#include "TRP_RelativeBlock.h"

//===========================================================================//
// Method         : TRP_RelativeBlock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TRP_RelativeBlock_c::TRP_RelativeBlock_c( 
      void )
      :
      vpr_index_( -1 ),
      vpr_type_( 0 )
{
   this->Reset( );
} 

//===========================================================================//
TRP_RelativeBlock_c::TRP_RelativeBlock_c( 
      const string&            srName,
            int                vpr_index,
      const t_type_descriptor* vpr_type,
            size_t             relativeMacroIndex,
            size_t             relativeNodeIndex )
      :
      srName_( srName ),
      vpr_index_( vpr_index ),
      vpr_type_( const_cast< t_type_descriptor* >( vpr_type )),
      relativeMacroIndex_( relativeMacroIndex ),
      relativeNodeIndex_( relativeNodeIndex )
{
} 

//===========================================================================//
TRP_RelativeBlock_c::TRP_RelativeBlock_c( 
      const char*              pszName,
            int                vpr_index,
      const t_type_descriptor* vpr_type,
            size_t             relativeMacroIndex,
            size_t             relativeNodeIndex )
      :
      srName_( TIO_PSZ_STR( pszName )),
      vpr_index_( vpr_index ),
      vpr_type_( const_cast< t_type_descriptor* >( vpr_type )),
      relativeMacroIndex_( relativeMacroIndex ),
      relativeNodeIndex_( relativeNodeIndex )
{
} 

//===========================================================================//
TRP_RelativeBlock_c::TRP_RelativeBlock_c( 
      const TRP_RelativeBlock_c& relativeBlock )
      :
      srName_( relativeBlock.srName_ ),
      vpr_index_( relativeBlock.vpr_index_ ),
      vpr_type_( relativeBlock.vpr_type_ ),
      relativeMacroIndex_( relativeBlock.relativeMacroIndex_ ),
      relativeNodeIndex_( relativeBlock.relativeNodeIndex_ )
{
} 

//===========================================================================//
// Method         : ~TRP_RelativeBlock_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TRP_RelativeBlock_c::~TRP_RelativeBlock_c( 
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
TRP_RelativeBlock_c& TRP_RelativeBlock_c::operator=( 
      const TRP_RelativeBlock_c& relativeBlock )
{
   if( &relativeBlock != this )
   {
      this->srName_ = relativeBlock.srName_;
      this->vpr_index_ = relativeBlock.vpr_index_;
      this->vpr_type_ = relativeBlock.vpr_type_;
      this->relativeMacroIndex_ = relativeBlock.relativeMacroIndex_;
      this->relativeNodeIndex_ = relativeBlock.relativeNodeIndex_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativeBlock_c::operator<( 
      const TRP_RelativeBlock_c& relativeBlock ) const
{
   return(( TC_CompareStrings( this->srName_, relativeBlock.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativeBlock_c::operator==( 
      const TRP_RelativeBlock_c& relativeBlock ) const
{
   return(( this->srName_ == relativeBlock.srName_ ) &&
          ( this->vpr_index_ == relativeBlock.vpr_index_ ) &&
          ( this->vpr_type_ == relativeBlock.vpr_type_ ) &&
          ( this->relativeMacroIndex_ == relativeBlock.relativeMacroIndex_ ) &&
          ( this->relativeNodeIndex_ == relativeBlock.relativeNodeIndex_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TRP_RelativeBlock_c::operator!=( 
      const TRP_RelativeBlock_c& relativeBlock ) const
{
   return( !this->operator==( relativeBlock ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativeBlock_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<block" );
   printHandler.Write( pfile, 0, " name=\"%s\"", TIO_SR_STR( this->srName_ ));
   if( this->vpr_index_ != -1 )
   {
      printHandler.Write( pfile, 0, " vpr_index=%d", this->vpr_index_ );
   }
   if( this->vpr_type_ )
   {
      printHandler.Write( pfile, 0, " vpr_type=[0x%8x]", this->vpr_type_ );
   }
   if( this->relativeMacroIndex_ != TRP_RELATIVE_MACRO_UNDEFINED )
   {
      printHandler.Write( pfile, 0, " macro_index=%u", this->relativeMacroIndex_ );
   }
   if( this->relativeNodeIndex_ != TRP_RELATIVE_NODE_UNDEFINED )
   {
      printHandler.Write( pfile, 0, " node_index=%u", this->relativeNodeIndex_ );
   }
   printHandler.Write( pfile, 0, ">\n" );
} 

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TRP_RelativeBlock_c::Reset( 
      void )
{
   this->relativeMacroIndex_ = TRP_RELATIVE_MACRO_UNDEFINED;
   this->relativeNodeIndex_ = TRP_RELATIVE_NODE_UNDEFINED;
} 
