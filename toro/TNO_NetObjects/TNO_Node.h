//===========================================================================//
// Purpose : Declaration and inline definitions for a TNO_Node class.
//
//           Inline methods include:
//           - GetType
//           - GetInstPin, GetSegment, GetSwitchBox
//           - IsInstPin, IsSegment, IsSwitchBox
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#ifndef TNO_NODE_H
#define TNO_NODE_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_NameIndex.h"

#include "TNO_Typedefs.h"
#include "TNO_InstPin.h"
#include "TNO_Segment.h"
#include "TNO_SwitchBox.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
class TNO_Node_c
{
public:

   TNO_Node_c( void );
   TNO_Node_c( const TNO_InstPin_c& instPin );
   TNO_Node_c( const TNO_Segment_c& segment );
   TNO_Node_c( const TNO_SwitchBox_c& switchBox );
   TNO_Node_c( const TNO_Node_c& node );
   ~TNO_Node_c( void );

   TNO_Node_c& operator=( const TNO_Node_c& node );
   bool operator==( const TNO_Node_c& node ) const;
   bool operator!=( const TNO_Node_c& node ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0, 
               bool verbose = true ) const;

   TNO_NodeType_t GetType( void ) const;
   const TNO_InstPin_c& GetInstPin( void ) const;
   const TNO_Segment_c& GetSegment( void ) const;
   const TNO_SwitchBox_c& GetSwitchBox( void ) const;

   void Set( const TNO_InstPin_c& instPin );
   void Set( const TNO_Segment_c& segment );
   void Set( const TNO_SwitchBox_c& switchBox );

   void Clear( void );

   bool IsInstPin( void ) const;
   bool IsSegment( void ) const;
   bool IsSwitchBox( void ) const;

   bool IsValid( void ) const;

private:

   TNO_NodeType_t  type_;      // Defines route node type ;
                               // (ie. INST_PIN, SEGMENT, or SWITCH_BOX)
   TNO_InstPin_c   instPin_;
   TNO_Segment_c   segment_;
   TNO_SwitchBox_c switchBox_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
inline TNO_NodeType_t TNO_Node_c::GetType( 
      void ) const
{
   return( this->type_ );
}

//===========================================================================//
inline const TNO_InstPin_c& TNO_Node_c::GetInstPin( 
      void ) const
{
   return( this->instPin_ );
}

//===========================================================================//
inline const TNO_Segment_c& TNO_Node_c::GetSegment( 
      void ) const
{
   return( this->segment_ );
}

//===========================================================================//
inline const TNO_SwitchBox_c& TNO_Node_c::GetSwitchBox( 
      void ) const
{
   return( this->switchBox_ );
}

//===========================================================================//
inline bool TNO_Node_c::IsInstPin( 
      void ) const
{
   return( this->type_ == TNO_NODE_INST_PIN ? true : false );
}

//===========================================================================//
inline bool TNO_Node_c::IsSegment( 
      void ) const
{
   return( this->type_ == TNO_NODE_SEGMENT ? true : false );
}

//===========================================================================//
inline bool TNO_Node_c::IsSwitchBox( 
      void ) const
{
   return( this->type_ == TNO_NODE_SWITCH_BOX ? true : false );
}

//===========================================================================//
inline bool TNO_Node_c::IsValid( 
      void ) const
{
   return( this->type_ != TNO_NODE_UNDEFINED ? true : false );
}

#endif
