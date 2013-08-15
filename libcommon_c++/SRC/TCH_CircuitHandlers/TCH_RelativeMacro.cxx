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
//           - ForceRelativeLinks
//           - HasRelativeLink
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
      relativeNodeList_( TCH_RELATIVE_NODE_LIST_DEF_CAPACITY ),
      rotateEnabled_( false )
{
} 

//===========================================================================//
TCH_RelativeMacro_c::TCH_RelativeMacro_c( 
      const TCH_RelativeMacro_c& relativeMacro )
      :
      relativeNodeList_( relativeMacro.relativeNodeList_ ),
      rotateEnabled_( false )
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
      this->rotateEnabled_ = relativeMacro.rotateEnabled_;
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
   return(( this->relativeNodeList_ == relativeMacro.relativeNodeList_ ) &&
          ( this->rotateEnabled_ == relativeMacro.rotateEnabled_ ) ?
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

      // printHandler.Write( pfile, spaceLen + 6, "rotate_enabled=%s\n", 
      //                                          TIO_BOOL_STR( this->rotateEnabled_ ));
   }
   printHandler.Write( pfile, spaceLen, "</macro>\n" );
} 

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
void TCH_RelativeMacro_c::Add(
      const char*          pszFromBlockName,
      const char*          pszToBlockName,
      const TGO_IntDims_t& fromToLink,
            size_t*        pfromNodeIndex,
            size_t*        ptoNodeIndex )
{
   // Add two new nodes to relative macro node list
   size_t fromNodeIndex = this->AddNode_( pszFromBlockName );
   size_t toNodeIndex = this->AddNode_( pszToBlockName );

   // Link new nodes via relative indices based on relative link offsets
   this->LinkNodes_( fromNodeIndex, toNodeIndex, fromToLink );

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
            size_t         fromNodeIndex,
      const char*          pszToBlockName,
      const TGO_IntDims_t& fromToLink,
            size_t*        ptoNodeIndex )
{
   if( fromNodeIndex < this->relativeNodeList_.GetLength( ))
   {
      // Add new node to relative macro node list
      size_t toNodeIndex = this->AddNode_( pszToBlockName );

      // Link new nodes via relative indices based on relative link offsets
      this->LinkNodes_( fromNodeIndex, toNodeIndex, fromToLink );

      if( ptoNodeIndex )
      {
         *ptoNodeIndex = toNodeIndex;
      }
   }
}

//===========================================================================//
void TCH_RelativeMacro_c::Add(
            size_t         fromNodeIndex,
            size_t         toNodeIndex,
      const TGO_IntDims_t& fromToLink )
{
   if( fromNodeIndex < this->relativeNodeList_.GetLength( ) &&
       toNodeIndex < this->relativeNodeList_.GetLength( ))
   {
      // Link new nodes via relative indices based on relative link offsets
      this->LinkNodes_( fromNodeIndex, toNodeIndex, fromToLink );
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
TCH_RelativeNode_c* TCH_RelativeMacro_c::Find( 
            size_t         fromNodeIndex,
      const TGO_IntDims_t& fromToLink ) const
{
   TCH_RelativeNode_c* ptoNode = 0;
   TCH_RelativeNode_c* pfromNode = this->relativeNodeList_[fromNodeIndex];
   if( pfromNode )
   {
      ptoNode = this->Find( *pfromNode, fromToLink );
   }
   return( ptoNode );
}

//===========================================================================//
TCH_RelativeNode_c* TCH_RelativeMacro_c::Find( 
      const TCH_RelativeNode_c& fromNode,
      const TGO_IntDims_t&      fromToLink ) const
{
   TCH_RelativeNode_c* ptoNode = 0;

   TGO_Point_c fromRelativePoint = fromNode.GetRelativePoint( );
   fromRelativePoint.Set( fromRelativePoint.x + fromToLink.dx,
			  fromRelativePoint.y + fromToLink.dy );

   for( size_t i = 0; i < this->relativeNodeList_.GetLength( ); ++i )
   {
      const TCH_RelativeNode_c& toNode = *this->relativeNodeList_[i];
      const TGO_Point_c& toRelativePoint = toNode.GetRelativePoint( );
      if( fromRelativePoint == toRelativePoint )
      {
         ptoNode = this->relativeNodeList_[i];
         break;
      }
   }
   return( ptoNode );
}

//===========================================================================//
TCH_RelativeNode_c* TCH_RelativeMacro_c::Find( 
      const TCH_RelativeNode_c& fromNode ) const
{
   TGO_IntDims_t fromToLink( 0, 0 );
   return( this->Find( fromNode, fromToLink ));
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

      // Iterate for all relative links to update origin points per rotate
      const TGO_Point_c& relativePoint = prelativeNode->GetRelativePoint( );
      const TCH_RelativeLinkList_t& relativeLinkList = prelativeNode->GetRelativeLinkList( );
      for( size_t i = 0; i < relativeLinkList.GetLength( ); ++i )
      {
         size_t nodeIndex = relativeLinkList[i]->GetIndex( );
         TCH_RelativeNode_c* pneighborNode = this->relativeNodeList_[nodeIndex];
         const TGO_Point_c& neighborPoint = pneighborNode->GetRelativePoint( );

         TGO_IntDims_t translate( neighborPoint.x - relativePoint.x,
                                  neighborPoint.y - relativePoint.y, 0 );
         TGO_Transform_c transform( origin, rotate, translate );

         TGO_Point_c gridPoint;
         transform.Apply( origin, &gridPoint );
         pneighborNode->SetVPR_GridPoint( gridPoint );
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
// Method         : ForceRelativeLinks
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
void TCH_RelativeMacro_c::ForceRelativeLinks(
      const TCH_RelativeNode_c& fromNode,
      const TCH_RelativeNode_c& toNode,
      const TGO_IntDims_t&      fromToLink )
{
   const TGO_Point_c& fromPoint = fromNode.GetRelativePoint( );
   const TGO_Point_c& toPoint = toNode.GetRelativePoint( );
   this->ForceRelativeLinks( fromPoint, toPoint, fromToLink );
}

//===========================================================================//
void TCH_RelativeMacro_c::ForceRelativeLinks(
      const TGO_Point_c&   fromPoint,
      const TGO_Point_c&   toPoint,
      const TGO_IntDims_t& fromToLink )
{
   TGO_IntDims_t forceLink( fromPoint.x - toPoint.x + fromToLink.dx,
                            fromPoint.y - toPoint.y + fromToLink.dy );

   for( size_t i = 0; i < this->relativeNodeList_.GetLength( ); ++i )
   {
      TCH_RelativeNode_c* prelativeNode = this->relativeNodeList_[i];
      TGO_Point_c relativePoint = prelativeNode->GetRelativePoint( );
      relativePoint.Set( relativePoint.x + forceLink.dx,
                         relativePoint.y + forceLink.dy );
      prelativeNode->SetRelativePoint( relativePoint );
   }
}

//===========================================================================//
// Method         : HasRelativeLink
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/25/13 jeffr : Original
//===========================================================================//
bool TCH_RelativeMacro_c::HasRelativeLink(
            size_t         fromNodeIndex,
      const TGO_IntDims_t& fromToLink ) const
{
   const TCH_RelativeNode_c* ptoNode = this->Find( fromNodeIndex, fromToLink );
   return( ptoNode ? true : false );
}

//===========================================================================//
bool TCH_RelativeMacro_c::HasRelativeLink(
      const TCH_RelativeNode_c& fromNode,
      const TGO_IntDims_t&      fromToLink ) const
{
   const TCH_RelativeNode_c* ptoNode = this->Find( fromNode, fromToLink );
   return( ptoNode ? true : false );
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
// 06/25/13 jeffr : Original
//===========================================================================//
void TCH_RelativeMacro_c::LinkNodes_(
            size_t         fromNodeIndex,
            size_t         toNodeIndex,
      const TGO_IntDims_t& fromToLink ) const
{
   // Find nodes based on given node indices
   TCH_RelativeNode_c* pfromNode = this->relativeNodeList_[fromNodeIndex];
   TCH_RelativeNode_c* ptoNode = this->relativeNodeList_[toNodeIndex];

   // Set/update from/to nodes based on new/existing relative link offsets
   if( !pfromNode->HasRelativePoint( ))
   {
      TGO_Point_c fromRelativePoint( 0, 0 );
      pfromNode->SetRelativePoint( fromRelativePoint );
   }
   const TGO_Point_c& fromRelativePoint = pfromNode->GetRelativePoint( );
   TGO_Point_c toRelativePoint( fromRelativePoint.x + fromToLink.dx,
                                fromRelativePoint.y + fromToLink.dy );
   ptoNode->SetRelativePoint( toRelativePoint );

   for( size_t i = 0; i < this->relativeNodeList_.GetLength( ); ++i )
   {
      TCH_RelativeNode_c* prelativeNode = this->relativeNodeList_[i];
      if( prelativeNode == ptoNode )
         continue;

      pfromNode = prelativeNode;
      fromNodeIndex = i;

      if( !pfromNode->HasRelativeLink( toNodeIndex ))
      {
         pfromNode->AddRelativeLink( toNodeIndex );
      }
      if( !ptoNode->HasRelativeLink( fromNodeIndex ))
      {
         ptoNode->AddRelativeLink( fromNodeIndex );
      }
   }
}
