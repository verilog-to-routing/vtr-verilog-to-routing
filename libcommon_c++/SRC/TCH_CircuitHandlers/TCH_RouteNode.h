//===========================================================================//
// Purpose : Declaration and inline definitions for the TCH_RouteNode class.
//
//           Inline methods include:
//           - GetType, GetName, GetInstName, GetPortName, GetPinName
//           - GetVPR_Type
//           - GetVPR_Pin, GetVPR_Track, GetVPR_Channel
//           - GetVPR_GridPoint
//           - GetVPR_NodeIndex
//           - SetVPR_Type
//           - SetVPR_Pin, SetVPR_Track, SetVPR_Channel
//           - SetVPR_GridPoint
//           - SetVPR_NodeIndex
//           - SetLegal
//           - IsLegal
//           - IsValid
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

#ifndef TCH_ROUTE_NODE_H
#define TCH_ROUTE_NODE_H

#include <cstdio>
using namespace std;

#include "TNO_Node.h"

#include "TGO_Region.h"
#include "TGO_Point.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
class TCH_RouteNode_c
      :
      private TNO_Node_c
{
public:

   TCH_RouteNode_c( void );
   TCH_RouteNode_c( const TNO_Node_c& node );
   TCH_RouteNode_c( const TCH_RouteNode_c& routeNode );
   ~TCH_RouteNode_c( void );

   TCH_RouteNode_c& operator=( const TCH_RouteNode_c& routeNode );
   bool operator==( const TCH_RouteNode_c& routeNode ) const;
   bool operator!=( const TCH_RouteNode_c& routeNode ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   TNO_NodeType_t GetType( void ) const;
   const char* GetName( void ) const;
   const char* GetInstName( void ) const;
   const char* GetPortName( void ) const;
   const char* GetPinName( void ) const;

   t_rr_type GetVPR_Type( void ) const;
   int GetVPR_Pin( void ) const;
   int GetVPR_Track( void ) const;
   const TGO_Region_c& GetVPR_Channel( void ) const;
   const TGO_Point_c& GetVPR_GridPoint( void ) const;
   int GetVPR_NodeIndex( void ) const;

   void SetVPR_Type( t_rr_type vpr_type );
   void SetVPR_Pin( int vpr_pin );
   void SetVPR_Track( int vpr_track );
   void SetVPR_Channel( const TGO_Region_c& vpr_channel );
   void SetVPR_GridPoint( const TGO_Point_c& vpr_gridPoint );
   void SetVPR_NodeIndex( int vpr_rrNodeIndex );

   void SetLegal( bool isLegal );

   bool IsLegal( void ) const;
   bool IsValid( void ) const;

private:

   class TCH_VPR_c
   {
   public:

      t_rr_type type;  // Defines VPR t_rr_type 
                       // (eg. SOURCE, SINK, IPIN, OPIN, CHANX, CHANY)
      int pin;         // DefineS VPR pin index asso. with an "instPin" node
      int track;       // Defines VPR track index asso. with a "segment" node
      TGO_Region_c channel;
                       // Defines VPR channel region asso. with a "segment" node
      TGO_Point_c gridPoint; 
                       // Defines VPR grid location asso. with an "instPin" node

      int rrNodeIndex; // Defines VPR "rr_graph" node asso. with this node
   } vpr_;

   bool isLegal_; // TRUE => route node has been validated w.r.t a VPR "rr_graph")
                  // (insures segment path is valid within the VPR "rr_graph")
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
inline TNO_NodeType_t TCH_RouteNode_c::GetType( 
      void ) const
{
   return( TNO_Node_c::GetType( ));
}

//===========================================================================//
inline const char* TCH_RouteNode_c::GetName( 
      void ) const
{
   return( TNO_Node_c::GetInstPin( ).GetName( ));
}

//===========================================================================//
inline const char* TCH_RouteNode_c::GetInstName( 
      void ) const
{
   return( TNO_Node_c::GetInstPin( ).GetInstName( ));
}

//===========================================================================//
inline const char* TCH_RouteNode_c::GetPortName( 
      void ) const
{
   return( TNO_Node_c::GetInstPin( ).GetPortName( ));
}

//===========================================================================//
inline const char* TCH_RouteNode_c::GetPinName( 
      void ) const
{
   return( TNO_Node_c::GetInstPin( ).GetPinName( ));
}

//===========================================================================//
inline t_rr_type TCH_RouteNode_c::GetVPR_Type(
      void ) const
{
   return( this->vpr_.type );
}

//===========================================================================//
inline int TCH_RouteNode_c::GetVPR_Pin(
      void ) const
{
   return( this->vpr_.pin );
}

//===========================================================================//
inline int TCH_RouteNode_c::GetVPR_Track(
      void ) const
{
   return( this->vpr_.track );
}

//===========================================================================//
inline const TGO_Region_c& TCH_RouteNode_c::GetVPR_Channel(
      void ) const
{
   return( this->vpr_.channel );
}

//===========================================================================//
inline const TGO_Point_c& TCH_RouteNode_c::GetVPR_GridPoint(
      void ) const
{
   return( this->vpr_.gridPoint );
}

//===========================================================================//
inline int TCH_RouteNode_c::GetVPR_NodeIndex(
      void ) const
{
   return( this->vpr_.rrNodeIndex );
}

//===========================================================================//
inline void TCH_RouteNode_c::SetVPR_Type(
      t_rr_type vpr_type )
{
   this->vpr_.type = vpr_type;
}

//===========================================================================//
inline void TCH_RouteNode_c::SetVPR_Pin(
      int vpr_pin )
{
   this->vpr_.pin = vpr_pin;
}

//===========================================================================//
inline void TCH_RouteNode_c::SetVPR_Track(
      int vpr_track )
{
   this->vpr_.track = vpr_track;
}

//===========================================================================//
inline void TCH_RouteNode_c::SetVPR_Channel(
      const TGO_Region_c& vpr_channel )
{
   this->vpr_.channel = vpr_channel;
}

//===========================================================================//
inline void TCH_RouteNode_c::SetVPR_GridPoint(
      const TGO_Point_c& vpr_gridPoint )
{
   this->vpr_.gridPoint = vpr_gridPoint;
}

//===========================================================================//
inline void TCH_RouteNode_c::SetVPR_NodeIndex(
      int vpr_rrNodeIndex )
{
   this->vpr_.rrNodeIndex = vpr_rrNodeIndex;
}

//===========================================================================//
inline void TCH_RouteNode_c::SetLegal( 
      bool isLegal )
{
   this->isLegal_ = isLegal;
} 

//===========================================================================//
inline bool TCH_RouteNode_c::IsLegal( 
      void ) const
{
   return( this->isLegal_ ? true : false );
}

//===========================================================================//
inline bool TCH_RouteNode_c::IsValid( 
      void ) const
{
   return( TNO_Node_c::IsValid( ));
}

#endif
