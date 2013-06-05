//===========================================================================//
// Purpose : Method definitions for the TCH_VPR_GraphToRoute class.
//
//           Public methods include:
//           - TCH_VPR_GraphToRoute_c, ~TCH_VPR_GraphToRoute_c
//           - operator=, operator<
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

#include "TCH_VPR_GraphToRoute.h"

//===========================================================================//
// Method         : TCH_VPR_GraphToRoute_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_VPR_GraphToRoute_c::TCH_VPR_GraphToRoute_c( 
      void )
      :
      routeNodeIndex_( TCH_ROUTE_NODE_INDEX_INVALID )
{
   this->vpr_.rrNodeIndex = -1;
} 

//===========================================================================//
TCH_VPR_GraphToRoute_c::TCH_VPR_GraphToRoute_c( 
      int    vpr_rrNodeIndex,
      size_t routeNodeIndex )
      :
      routeNodeIndex_( routeNodeIndex )
{
   this->vpr_.rrNodeIndex = vpr_rrNodeIndex;
} 

//===========================================================================//
TCH_VPR_GraphToRoute_c::TCH_VPR_GraphToRoute_c( 
      const TCH_VPR_GraphToRoute_c& vpr_graphToRoute )
      :
      routeNodeIndex_( vpr_graphToRoute.routeNodeIndex_ )
{
   this->vpr_.rrNodeIndex = vpr_graphToRoute.vpr_.rrNodeIndex;
} 

//===========================================================================//
// Method         : ~TCH_VPR_GraphToRoute_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
TCH_VPR_GraphToRoute_c::~TCH_VPR_GraphToRoute_c( 
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
TCH_VPR_GraphToRoute_c& TCH_VPR_GraphToRoute_c::operator=( 
      const TCH_VPR_GraphToRoute_c& vpr_graphToRoute )
{
   if( &vpr_graphToRoute != this )
   {
      this->vpr_.rrNodeIndex = vpr_graphToRoute.vpr_.rrNodeIndex;
      this->routeNodeIndex_ = vpr_graphToRoute.routeNodeIndex_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_VPR_GraphToRoute_c::operator<( 
      const TCH_VPR_GraphToRoute_c& vpr_graphToRoute ) const
{
   return( this->vpr_.rrNodeIndex < vpr_graphToRoute.vpr_.rrNodeIndex ? 
           true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_VPR_GraphToRoute_c::operator==( 
      const TCH_VPR_GraphToRoute_c& vpr_graphToRoute ) const
{
   return(( this->vpr_.rrNodeIndex == vpr_graphToRoute.vpr_.rrNodeIndex ) &&
          ( this->routeNodeIndex_ == vpr_graphToRoute.routeNodeIndex_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
bool TCH_VPR_GraphToRoute_c::operator!=( 
      const TCH_VPR_GraphToRoute_c& vpr_graphToRoute ) const
{
   return( !this->operator==( vpr_graphToRoute ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
void TCH_VPR_GraphToRoute_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<vpr_graphToRoute" );
   printHandler.Write( pfile, 0, " <vpr" );
   printHandler.Write( pfile, 0, " rrNodeIndex=%d", this->vpr_.rrNodeIndex );
   printHandler.Write( pfile, 0, ">" );
   printHandler.Write( pfile, 0, " routeNodeIndex=%d", this->routeNodeIndex_ );
   printHandler.Write( pfile, 0, ">\n" );
} 
