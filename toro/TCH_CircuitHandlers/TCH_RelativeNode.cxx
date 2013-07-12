//===========================================================================//
// Purpose : Method definitions for the TCH_RelativeNode class.
//
//           Public methods include:
//           - TCH_RelativeNode_c, ~TCH_RelativeNode_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - Reset
//           - AddRelativeLink
//           - HasRelativePoint
//           - HasRelativeLink
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

#include "TCH_RelativeNode.h"

//===========================================================================//
// Method         : TCH_RelativeNode_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
TCH_RelativeNode_c::TCH_RelativeNode_c( 
      void )
      :
      vpr_type_( 0 ),
      relativeLinkList_( TCH_RELATIVE_LINK_LIST_DEF_CAPACITY )
{
   this->Reset( );
} 

//===========================================================================//
TCH_RelativeNode_c::TCH_RelativeNode_c( 
      const string&            srBlockName,
            t_type_descriptor* vpr_type )
      :
      srBlockName_( srBlockName ),
      vpr_type_( vpr_type ),
      relativeLinkList_( TCH_RELATIVE_LINK_LIST_DEF_CAPACITY )
{
   this->Reset( );
} 

//===========================================================================//
TCH_RelativeNode_c::TCH_RelativeNode_c( 
      const char*              pszName,
            t_type_descriptor* vpr_type )
      :
      srBlockName_( TIO_PSZ_STR( pszName )),
      vpr_type_( vpr_type ),
      relativeLinkList_( TCH_RELATIVE_LINK_LIST_DEF_CAPACITY )
{
   this->Reset( );
} 

//===========================================================================//
TCH_RelativeNode_c::TCH_RelativeNode_c( 
      const TCH_RelativeNode_c& relativeNode )
      :
      srBlockName_( relativeNode.srBlockName_ ),
      vpr_type_( relativeNode.vpr_type_ ),
      vpr_gridPoint_( relativeNode.vpr_gridPoint_ ),
      relativePoint_( relativeNode.relativePoint_ ),
      relativeLinkList_( relativeNode.relativeLinkList_ )
{
} 

//===========================================================================//
// Method         : ~TCH_RelativeNode_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
TCH_RelativeNode_c::~TCH_RelativeNode_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
TCH_RelativeNode_c& TCH_RelativeNode_c::operator=( 
      const TCH_RelativeNode_c& relativeNode )
{
   if( &relativeNode != this )
   {
      this->srBlockName_ = relativeNode.srBlockName_;
      this->vpr_type_ = relativeNode.vpr_type_;
      this->vpr_gridPoint_ = relativeNode.vpr_gridPoint_;
      this->relativePoint_ = relativeNode.relativePoint_;
      this->relativeLinkList_ = relativeNode.relativeLinkList_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeNode_c::operator==( 
      const TCH_RelativeNode_c& relativeNode ) const
{
   return(( this->srBlockName_ == relativeNode.srBlockName_ ) &&
          ( this->vpr_type_ == relativeNode.vpr_type_ ) &&
          ( this->vpr_gridPoint_ == relativeNode.vpr_gridPoint_ ) &&
          ( this->relativePoint_ == relativeNode.relativePoint_ ) &&
          ( this->relativeLinkList_ == relativeNode.relativeLinkList_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeNode_c::operator!=( 
      const TCH_RelativeNode_c& relativeNode ) const
{
   return( !this->operator==( relativeNode ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
void TCH_RelativeNode_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<node" );
   printHandler.Write( pfile, 0, " block=\"%s\"", TIO_SR_STR( this->srBlockName_ ));
   if( this->vpr_type_ )
   {
      printHandler.Write( pfile, 0, " vpr_type=%d", this->vpr_type_ );
   }
   if( this->vpr_gridPoint_.IsValid( ))
   {
      string srGridPoint;
      this->vpr_gridPoint_.ExtractString( &srGridPoint );
      printHandler.Write( pfile, 0, " vpr_grid_point=(%s)", TIO_SR_STR( srGridPoint ));
   }
   if( this->relativePoint_.IsValid( ))
   {
      string srRelativePoint;
      this->relativePoint_.ExtractString( &srRelativePoint );
      printHandler.Write( pfile, 0, " relative_point=(%s)", TIO_SR_STR( srRelativePoint ));
   }
   if( this->relativeLinkList_.IsValid( ))
   {
      string srRelativeLinkList;
      this->relativeLinkList_.ExtractString( &srRelativeLinkList );
      printHandler.Write( pfile, 0, " relative_link_list=[%s]", TIO_SR_STR( srRelativeLinkList ));
   }
   printHandler.Write( pfile, 0, ">\n" );
} 

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
void TCH_RelativeNode_c::Reset( 
      void )
{
   this->vpr_gridPoint_.Reset( );
   this->relativePoint_.Reset( );
   this->relativeLinkList_.Clear( );
} 


//===========================================================================//
// Method         : AddRelativeLink
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
void TCH_RelativeNode_c::AddRelativeLink(
      const TC_Index_c& relativeLinkIndex )
{
   this->relativeLinkList_.Add( relativeLinkIndex );
}

//===========================================================================//
void TCH_RelativeNode_c::AddRelativeLink(
      int linkIndex )
{
   TC_Index_c relativeLinkIndex( linkIndex );
   this->AddRelativeLink( relativeLinkIndex );
}

//===========================================================================//
void TCH_RelativeNode_c::AddRelativeLink(
      size_t linkIndex )
{
   TC_Index_c relativeLinkIndex( static_cast< int >( linkIndex ));
   this->AddRelativeLink( relativeLinkIndex );
}

//===========================================================================//
// Method         : HasRelativePoint
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeNode_c::HasRelativePoint(
      void ) const
{
   return( this->relativePoint_.IsValid( ));
}

//===========================================================================//
// Method         : HasRelativeLink
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeNode_c::HasRelativeLink(
      const TC_Index_c& relativeLinkIndex ) const
{
   return( this->relativeLinkList_.IsMember( relativeLinkIndex ));
}

//===========================================================================//
bool TCH_RelativeNode_c::HasRelativeLink(
      int linkIndex ) const
{
   TC_Index_c relativeLinkIndex( linkIndex );
   return( this->HasRelativeLink( relativeLinkIndex ));
}

//===========================================================================//
bool TCH_RelativeNode_c::HasRelativeLink(
      size_t linkIndex ) const
{
   TC_Index_c relativeLinkIndex( static_cast< int >( linkIndex ));
   return( this->HasRelativeLink( relativeLinkIndex ));
}
