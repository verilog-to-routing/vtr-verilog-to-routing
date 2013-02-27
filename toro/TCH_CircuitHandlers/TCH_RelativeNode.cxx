//===========================================================================//
// Purpose : Method definitions for the TCH_RelativeNode class.
//
//           Public methods include:
//           - TCH_RelativeNode_c, ~TCH_RelativeNode_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - SetSideIndex
//           - GetSideIndex
//           - HasSideIndex
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

#include "TCH_RelativeNode.h"

//===========================================================================//
// Method         : TCH_RelativeNode_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeNode_c::TCH_RelativeNode_c( 
      void )
      :
      vpr_type_( 0 )
{
   this->Reset( );
} 

//===========================================================================//
TCH_RelativeNode_c::TCH_RelativeNode_c( 
      const string&            srBlockName,
            t_type_descriptor* vpr_type )
      :
      srBlockName_( srBlockName ),
      vpr_type_( vpr_type )
{
   this->Reset( );
} 

//===========================================================================//
TCH_RelativeNode_c::TCH_RelativeNode_c( 
      const char*              pszName,
            t_type_descriptor* vpr_type )
      :
      srBlockName_( TIO_PSZ_STR( pszName )),
      vpr_type_( vpr_type )
{
   this->Reset( );
} 

//===========================================================================//
TCH_RelativeNode_c::TCH_RelativeNode_c( 
      const TCH_RelativeNode_c& relativeNode )
      :
      srBlockName_( relativeNode.srBlockName_ ),
      vpr_type_( relativeNode.vpr_type_ ),
      vpr_gridPoint_( relativeNode.vpr_gridPoint_ )
{
   this->sideIndex_.left = relativeNode.sideIndex_.left;
   this->sideIndex_.right = relativeNode.sideIndex_.right;
   this->sideIndex_.lower = relativeNode.sideIndex_.lower;
   this->sideIndex_.upper = relativeNode.sideIndex_.upper;
} 

//===========================================================================//
// Method         : ~TCH_RelativeNode_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
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
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeNode_c& TCH_RelativeNode_c::operator=( 
      const TCH_RelativeNode_c& relativeNode )
{
   if( &relativeNode != this )
   {
      this->srBlockName_ = relativeNode.srBlockName_;
      this->vpr_type_ = relativeNode.vpr_type_;
      this->vpr_gridPoint_ = relativeNode.vpr_gridPoint_;
      this->sideIndex_.left = relativeNode.sideIndex_.left;
      this->sideIndex_.right = relativeNode.sideIndex_.right;
      this->sideIndex_.lower = relativeNode.sideIndex_.lower;
      this->sideIndex_.upper = relativeNode.sideIndex_.upper;
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
bool TCH_RelativeNode_c::operator==( 
      const TCH_RelativeNode_c& relativeNode ) const
{
   return(( this->srBlockName_ == relativeNode.srBlockName_ ) &&
          ( this->vpr_type_ == relativeNode.vpr_type_ ) &&
          ( this->vpr_gridPoint_ == relativeNode.vpr_gridPoint_ ) &&
          ( this->sideIndex_.left == relativeNode.sideIndex_.left ) &&
          ( this->sideIndex_.right == relativeNode.sideIndex_.right ) &&
          ( this->sideIndex_.lower == relativeNode.sideIndex_.lower ) &&
          ( this->sideIndex_.upper == relativeNode.sideIndex_.upper ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
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
// 01/15/13 jeffr : Original
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
   if(( this->sideIndex_.left != TCH_RELATIVE_NODE_UNDEFINED ) ||
      ( this->sideIndex_.right != TCH_RELATIVE_NODE_UNDEFINED ) ||
      ( this->sideIndex_.lower != TCH_RELATIVE_NODE_UNDEFINED ) ||
      ( this->sideIndex_.upper != TCH_RELATIVE_NODE_UNDEFINED ))
   {
      printHandler.Write( pfile, 0, " side_index=(" );

      if( this->sideIndex_.left != TCH_RELATIVE_NODE_UNDEFINED )
         printHandler.Write( pfile, 0, "%u", this->sideIndex_.left );
      else
         printHandler.Write( pfile, 0, "-" );

      printHandler.Write( pfile, 0, "," );

      if( this->sideIndex_.right != TCH_RELATIVE_NODE_UNDEFINED )
         printHandler.Write( pfile, 0, "%u", this->sideIndex_.right );
      else
         printHandler.Write( pfile, 0, "-" );

      printHandler.Write( pfile, 0, "," );

      if( this->sideIndex_.lower != TCH_RELATIVE_NODE_UNDEFINED )
         printHandler.Write( pfile, 0, "%u", this->sideIndex_.lower );
      else
         printHandler.Write( pfile, 0, "-" );

      printHandler.Write( pfile, 0, "," );

      if( this->sideIndex_.upper != TCH_RELATIVE_NODE_UNDEFINED )
         printHandler.Write( pfile, 0, "%u", this->sideIndex_.upper );
      else
         printHandler.Write( pfile, 0, "-" );

      printHandler.Write( pfile, 0, ")" );
   }
   printHandler.Write( pfile, 0, ">\n" );
} 

//===========================================================================//
// Method         : SetSideIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeNode_c::SetSideIndex(
      TC_SideMode_t side,
      size_t        relativeNodeIndex )
{
   switch( side )
   {
   case TC_SIDE_LEFT:  this->sideIndex_.left = relativeNodeIndex;  break;
   case TC_SIDE_RIGHT: this->sideIndex_.right = relativeNodeIndex; break;
   case TC_SIDE_LOWER: this->sideIndex_.lower = relativeNodeIndex; break;
   case TC_SIDE_UPPER: this->sideIndex_.upper = relativeNodeIndex; break;
   default:                                                        break;
   }
} 

//===========================================================================//
// Method         : GetSideIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
size_t TCH_RelativeNode_c::GetSideIndex(
      TC_SideMode_t side ) const
{
   size_t relativeNodeIndex = TCH_RELATIVE_NODE_UNDEFINED;

   switch( side )
   {
   case TC_SIDE_LEFT:  relativeNodeIndex = this->sideIndex_.left;  break;
   case TC_SIDE_RIGHT: relativeNodeIndex = this->sideIndex_.right; break;
   case TC_SIDE_LOWER: relativeNodeIndex = this->sideIndex_.lower; break;
   case TC_SIDE_UPPER: relativeNodeIndex = this->sideIndex_.upper; break;
   default:                                                        break;
   }
   return( relativeNodeIndex );
} 

//===========================================================================//
// Method         : HasSideIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeNode_c::HasSideIndex(
      TC_SideMode_t side ) const
{
   bool hasSideIndex = false; 

   switch( side )
   {
   case TC_SIDE_LEFT: 
      hasSideIndex = ( this->sideIndex_.left != TCH_RELATIVE_NODE_UNDEFINED ? 
                       true : false );
      break;
   case TC_SIDE_RIGHT: 
      hasSideIndex = ( this->sideIndex_.right != TCH_RELATIVE_NODE_UNDEFINED ? 
                       true : false );
      break;
   case TC_SIDE_LOWER: 
      hasSideIndex = ( this->sideIndex_.lower != TCH_RELATIVE_NODE_UNDEFINED ? 
                       true : false );
      break;
   case TC_SIDE_UPPER: 
      hasSideIndex = ( this->sideIndex_.upper != TCH_RELATIVE_NODE_UNDEFINED ? 
                       true : false );
      break;
   default:
      break;
   }
   return( hasSideIndex );
} 

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeNode_c::Reset( 
      void )
{
   this->vpr_gridPoint_.Reset( );

   this->sideIndex_.left = TCH_RELATIVE_NODE_UNDEFINED;
   this->sideIndex_.right = TCH_RELATIVE_NODE_UNDEFINED;
   this->sideIndex_.lower = TCH_RELATIVE_NODE_UNDEFINED;
   this->sideIndex_.upper = TCH_RELATIVE_NODE_UNDEFINED;
} 
