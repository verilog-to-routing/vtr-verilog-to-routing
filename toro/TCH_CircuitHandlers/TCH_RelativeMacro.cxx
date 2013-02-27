//===========================================================================//
// Purpose : Method definitions for the TCH_RelativeMacro class.
//
//           Public methods include:
//           - TCH_RelativeMacro_c, ~TCH_RelativeMacro_c
//           - operator=
//           - operator==, operator!=
//           - operator[]
//           - Print
//           - Add
//           - Find
//           - Set
//           - Clear
//           - Reset
//
//           Private methods include:
//           - AddNode_
//           - LinkNodes_
//
//===========================================================================//

#include "TC_StringUtils.h"

#include "TGO_Transform.h"

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

#include "TCH_RelativeMacro.h"

//===========================================================================//
// Method         : TCH_RelativeMacro_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeMacro_c::TCH_RelativeMacro_c( 
      void )
      :
      relativeNodeList_( TCH_RELATIVE_NODE_LIST_DEF_CAPACITY )
{
} 

//===========================================================================//
TCH_RelativeMacro_c::TCH_RelativeMacro_c( 
      const TCH_RelativeMacro_c& relativeMacro )
      :
      relativeNodeList_( relativeMacro.relativeNodeList_ )
{
} 

//===========================================================================//
// Method         : ~TCH_RelativeMacro_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeMacro_c::~TCH_RelativeMacro_c( 
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
TCH_RelativeMacro_c& TCH_RelativeMacro_c::operator=( 
      const TCH_RelativeMacro_c& relativeMacro )
{
   if( &relativeMacro != this )
   {
      this->relativeNodeList_ = relativeMacro.relativeNodeList_;
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
bool TCH_RelativeMacro_c::operator==( 
      const TCH_RelativeMacro_c& relativeMacro ) const
{
   return(( this->relativeNodeList_ == relativeMacro.relativeNodeList_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeMacro_c::operator!=( 
      const TCH_RelativeMacro_c& relativeMacro ) const
{
   return( !this->operator==( relativeMacro ) ? true : false );
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeNode_c* TCH_RelativeMacro_c::operator[]( 
      size_t relativeNodeIndex ) const
{
   return( this->relativeNodeList_[relativeNodeIndex] );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeMacro_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<macro>\n" );
   for( size_t i = 0; i < this->relativeNodeList_.GetLength( ); ++i )
   {
      TCH_RelativeNode_c* prelativeNode = this->relativeNodeList_[i];
      prelativeNode->Print( pfile, spaceLen + 3 );
   }
   printHandler.Write( pfile, spaceLen, "</macro>\n" );
} 

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeMacro_c::Add(
      const char*         pszFromBlockName,
      const char*         pszToBlockName,
            TC_SideMode_t side,
            size_t*       pfromNodeIndex,
            size_t*       ptoNodeIndex )
{
   // Add two new nodes to relative macro node list
   size_t fromNodeIndex = this->AddNode_( pszFromBlockName );
   size_t toNodeIndex = this->AddNode_( pszToBlockName );

   // Link new nodes via neighbor indices based on relative side and anti-side
   this->LinkNodes_( fromNodeIndex, toNodeIndex, side );

   if( pfromNodeIndex )
   {
      *pfromNodeIndex = fromNodeIndex;
   }
   if( ptoNodeIndex )
   {
      *ptoNodeIndex = toNodeIndex;
   }
}

//===========================================================================//
void TCH_RelativeMacro_c::Add(
            size_t        fromNodeIndex,
      const char*         pszToBlockName,
            TC_SideMode_t side,
            size_t*       ptoNodeIndex )
{
   if( fromNodeIndex < this->relativeNodeList_.GetLength( ))
   {
      // Add new node to relative macro node list
      size_t toNodeIndex = this->AddNode_( pszToBlockName );

      // Link new nodes via neighbor indices based on relative side and anti-side
      this->LinkNodes_( fromNodeIndex, toNodeIndex, side );

      if( ptoNodeIndex )
      {
         *ptoNodeIndex = toNodeIndex;
      }
   }
}

//===========================================================================//
void TCH_RelativeMacro_c::Add(
      size_t        fromNodeIndex,
      size_t        toNodeIndex,
      TC_SideMode_t side )
{
   if( fromNodeIndex < this->relativeNodeList_.GetLength( ) &&
       toNodeIndex < this->relativeNodeList_.GetLength( ))
   {
      // Link new nodes via neighbor indices based on relative side and anti-side
      this->LinkNodes_( fromNodeIndex, toNodeIndex, side );
   }
}

//===========================================================================//
void TCH_RelativeMacro_c::Add(
      const char*   pszFromBlockName,
            size_t* pfromNodeIndex )
{
   // Add single (empty) new node to relative macro node list
   size_t fromNodeIndex = this->AddNode_( pszFromBlockName );

   if( pfromNodeIndex )
   {
      *pfromNodeIndex = fromNodeIndex;
   }
}      

//===========================================================================//
// Method         : Find
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TCH_RelativeNode_c* TCH_RelativeMacro_c::Find( 
      size_t relativeNodeIndex ) const
{
   return( this->operator[]( relativeNodeIndex ));
}

//===========================================================================//
// Method         : Set
// Description    : Sets placement coordinates for all nodes in a relative
//                  macro based on the given origin point and rotate mode.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeMacro_c::Set(
            size_t           relativeNodeIndex,
      const TGO_Point_c&     origin,
            TGO_RotateMode_t rotate )
{
   // Start by setting given relative node based on the given origin point
   TCH_RelativeNode_c* prelativeNode = this->Find( relativeNodeIndex );
   if( prelativeNode )
   {
      prelativeNode->SetVPR_GridPoint( origin );

      // Next, use recursion to set all neighbors based on cumulative translate 
      if( prelativeNode->HasSideIndex( TC_SIDE_LEFT ))
      {
         size_t neighborNodeIndex = prelativeNode->GetSideIndex( TC_SIDE_LEFT );
         TGO_IntDims_t translate( -1, 0, 0 );
         this->Set( neighborNodeIndex, origin, rotate, translate );
      }
      if( prelativeNode->HasSideIndex( TC_SIDE_RIGHT ))
      {
         size_t neighborNodeIndex = prelativeNode->GetSideIndex( TC_SIDE_RIGHT );
         TGO_IntDims_t translate( 1, 0, 0 );
         this->Set( neighborNodeIndex, origin, rotate, translate );
      }
      if( prelativeNode->HasSideIndex( TC_SIDE_LOWER ))
      {
         size_t neighborNodeIndex = prelativeNode->GetSideIndex( TC_SIDE_LOWER );
         TGO_IntDims_t translate( 0, -1, 0 );
         this->Set( neighborNodeIndex, origin, rotate, translate );
      }
      if( prelativeNode->HasSideIndex( TC_SIDE_UPPER ))
      {
         size_t neighborNodeIndex = prelativeNode->GetSideIndex( TC_SIDE_UPPER );
         TGO_IntDims_t translate( 0, 1, 0 );
         this->Set( neighborNodeIndex, origin, rotate, translate );
      }
   }
}

//===========================================================================//
void TCH_RelativeMacro_c::Set(
            size_t           relativeNodeIndex,
      const TGO_Point_c&     origin,
            TGO_RotateMode_t rotate,
      const TGO_IntDims_t&   translate )
{
   // Start by setting given relative node based on the given origin point
   TCH_RelativeNode_c* prelativeNode = Find( relativeNodeIndex );
   if( prelativeNode &&
       !prelativeNode->GetVPR_GridPoint( ).IsValid( ))
   {
      // Next, define transform, then apply recursively to all relative neighbors
      TGO_Transform_c transform( origin, rotate, translate );

      TGO_Point_c gridPoint;
      transform.Apply( origin, &gridPoint );
      prelativeNode->SetVPR_GridPoint( gridPoint );

      if( prelativeNode->HasSideIndex( TC_SIDE_LEFT ))
      {
         size_t neighborNodeIndex = prelativeNode->GetSideIndex( TC_SIDE_LEFT );
         TGO_IntDims_t translate_( translate.dx - 1, translate.dy, translate.dz );
         this->Set( neighborNodeIndex, origin, rotate, translate_ );
      }
      if( prelativeNode->HasSideIndex( TC_SIDE_RIGHT ))
      {
         size_t neighborNodeIndex = prelativeNode->GetSideIndex( TC_SIDE_RIGHT );
         TGO_IntDims_t translate_( translate.dx + 1, translate.dy, translate.dz );
         this->Set( neighborNodeIndex, origin, rotate, translate_ );
      }
      if( prelativeNode->HasSideIndex( TC_SIDE_LOWER ))
      {
         size_t neighborNodeIndex = prelativeNode->GetSideIndex( TC_SIDE_LOWER );
         TGO_IntDims_t translate_( translate.dx, translate.dy - 1, translate.dz );
         this->Set( neighborNodeIndex, origin, rotate, translate_ );
      }
      if( prelativeNode->HasSideIndex( TC_SIDE_UPPER ))
      {
         size_t neighborNodeIndex = prelativeNode->GetSideIndex( TC_SIDE_UPPER );
         TGO_IntDims_t translate_( translate.dx, translate.dy + 1, translate.dz );
         this->Set( neighborNodeIndex, origin, rotate, translate_ );
      }
   }
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeMacro_c::Clear( 
      void ) 
{
   this->relativeNodeList_.Clear( );

   // Danger, Danger Will Robinson!
   // Beware of any dangling references left in from relativeBlockList_
}

//===========================================================================//
// Method         : Reset
// Description    : Resets all placement coordinates and neighbor node
//                  indices for all relative placement nodes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeMacro_c::Reset(
      void )
{
   for( size_t i = 0; i < this->relativeNodeList_.GetLength( ); ++i )
   {
      TCH_RelativeNode_c* prelativeNode = this->relativeNodeList_[i];
      prelativeNode->Reset( );
   }
}

//===========================================================================//
// Method         : AddNode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
size_t TCH_RelativeMacro_c::AddNode_(
      const char* pszBlockName )
{
   // Add new node to existing relative macro node list
   TCH_RelativeNode_c node( pszBlockName );
   this->relativeNodeList_.Add( node );

   // Find and return new node index (assuming most recently added node)
   size_t nodeIndex = this->relativeNodeList_.GetLength( ) - 1;

   return( nodeIndex );
}

//===========================================================================//
// Method         : LinkNodes_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TCH_RelativeMacro_c::LinkNodes_(
      size_t        fromNodeIndex,
      size_t        toNodeIndex,
      TC_SideMode_t side ) const
{
   // Find nodes based on given node indices
   TCH_RelativeNode_c* pfromNode = this->relativeNodeList_[fromNodeIndex];
   TCH_RelativeNode_c* ptoNode = this->relativeNodeList_[toNodeIndex];

   // Link nodes via neighbor indices based on relative side and anti-side
   switch( side )
   {
   case TC_SIDE_LEFT:
      pfromNode->SetSideIndex( TC_SIDE_LEFT, toNodeIndex );
      ptoNode->SetSideIndex( TC_SIDE_RIGHT, fromNodeIndex );
      break;
   case TC_SIDE_RIGHT:
      pfromNode->SetSideIndex( TC_SIDE_RIGHT, toNodeIndex );
      ptoNode->SetSideIndex( TC_SIDE_LEFT, fromNodeIndex );
      break;
   case TC_SIDE_LOWER:
      pfromNode->SetSideIndex( TC_SIDE_LOWER, toNodeIndex );
      ptoNode->SetSideIndex( TC_SIDE_UPPER, fromNodeIndex );
      break;
   case TC_SIDE_UPPER:
      pfromNode->SetSideIndex( TC_SIDE_UPPER, toNodeIndex );
      ptoNode->SetSideIndex( TC_SIDE_LOWER, fromNodeIndex );
      break;
   default:
      break;
   }
}
