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
