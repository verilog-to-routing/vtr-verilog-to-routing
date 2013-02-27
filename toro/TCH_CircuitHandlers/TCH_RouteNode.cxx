//===========================================================================//
// Purpose : Method definitions for the TCH_RouteNode class.
//
//           Public methods include:
//           - TCH_RouteNode_c, ~TCH_RouteNode_c
//           - operator=
//           - operator==, operator!=
//           - Print
//
//===========================================================================//

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

#include "TCH_StringUtils.h"
#include "TCH_RouteNode.h"

//===========================================================================//
// Method         : TCH_RouteNode_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_RouteNode_c::TCH_RouteNode_c( 
      void )
      :
      isLegal_( false )
{
   this->vpr_.type = static_cast< t_rr_type >( -1 );
   this->vpr_.pin = -1;
   this->vpr_.track = -1;
   this->vpr_.rrNodeIndex = -1;
} 

//===========================================================================//
TCH_RouteNode_c::TCH_RouteNode_c( 
      const TNO_Node_c& node )
      :
      TNO_Node_c( node ),
      isLegal_( false )
{
   this->vpr_.type = static_cast< t_rr_type >( -1 );
   this->vpr_.pin = -1;
   this->vpr_.track = -1;
   this->vpr_.rrNodeIndex = -1;
} 

//===========================================================================//
TCH_RouteNode_c::TCH_RouteNode_c( 
      const TCH_RouteNode_c& routeNode )
      :
      TNO_Node_c( routeNode ),
      isLegal_( routeNode.isLegal_ )
{
   this->vpr_.type = routeNode.vpr_.type;
   this->vpr_.pin = routeNode.vpr_.pin;
   this->vpr_.track = routeNode.vpr_.track;
   this->vpr_.channel = routeNode.vpr_.channel;
   this->vpr_.gridPoint = routeNode.vpr_.gridPoint;
   this->vpr_.rrNodeIndex = routeNode.vpr_.rrNodeIndex;
} 

//===========================================================================//
// Method         : ~TCH_RouteNode_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_RouteNode_c::~TCH_RouteNode_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_RouteNode_c& TCH_RouteNode_c::operator=( 
      const TCH_RouteNode_c& routeNode )
{
   if( &routeNode != this )
   {
      TNO_Node_c::operator=( routeNode );
      this->vpr_.type = routeNode.vpr_.type;
      this->vpr_.pin = routeNode.vpr_.pin;
      this->vpr_.track = routeNode.vpr_.track;
      this->vpr_.channel = routeNode.vpr_.channel;
      this->vpr_.gridPoint = routeNode.vpr_.gridPoint;
      this->vpr_.rrNodeIndex = routeNode.vpr_.rrNodeIndex;
      this->isLegal_ = routeNode.isLegal_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_RouteNode_c::operator==( 
      const TCH_RouteNode_c& routeNode ) const
{
   return(( TNO_Node_c::operator==( routeNode )) &&
          ( this->vpr_.type == routeNode.vpr_.type ) &&
          ( this->vpr_.pin == routeNode.vpr_.pin ) &&
          ( this->vpr_.track == routeNode.vpr_.track ) &&
          ( this->vpr_.channel == routeNode.vpr_.channel ) &&
          ( this->vpr_.gridPoint == routeNode.vpr_.gridPoint ) &&
          ( this->vpr_.rrNodeIndex == routeNode.vpr_.rrNodeIndex ) &&
          ( this->isLegal_ == routeNode.isLegal_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_RouteNode_c::operator!=( 
      const TCH_RouteNode_c& routeNode ) const
{
   return( !this->operator==( routeNode ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_RouteNode_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srVPR_Type;
   TCH_ExtractStringVPR_Type( this->vpr_.type, &srVPR_Type );

   string srChannel;
   this->vpr_.channel.ExtractString( &srChannel );

   string srGridPoint;
   this->vpr_.gridPoint.ExtractString( &srGridPoint );

   printHandler.Write( pfile, spaceLen, "<routeNode" );
   printHandler.Write( pfile, 0, " isLegal=%s", TIO_BOOL_VAL( this->isLegal_ ));
   printHandler.Write( pfile, 0, " <vpr" );
   printHandler.Write( pfile, 0, " type=%s", TIO_SR_STR( srVPR_Type ));
   printHandler.Write( pfile, 0, " pin=%d", this->vpr_.pin );
   printHandler.Write( pfile, 0, " track=%d", this->vpr_.track );
   printHandler.Write( pfile, 0, " channel=(%s)", TIO_SR_STR( srChannel ));
   printHandler.Write( pfile, 0, " gridPoint=(%s)", TIO_SR_STR( srGridPoint ));
   printHandler.Write( pfile, 0, " rrNodeIndex=%d", this->vpr_.rrNodeIndex );
   printHandler.Write( pfile, 0, ">" );
   printHandler.Write( pfile, 0, ">\n" );

   TNO_Node_c::Print( pfile, spaceLen, false );
} 
