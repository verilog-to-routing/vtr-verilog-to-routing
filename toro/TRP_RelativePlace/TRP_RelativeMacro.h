//===========================================================================//
// Purpose : Declaration and inline definitions for the TRP_RelativeMacro 
//           class.
//
//           Inline methods include:
//           - GetRelativeNodeList
//           - GetLength
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

#ifndef TRP_RELATIVE_MACRO_H
#define TRP_RELATIVE_MACRO_H

#include <cstdio>
using namespace std;

#include "TRP_Typedefs.h"
#include "TRP_RelativeNode.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TRP_RelativeMacro_c
{
public:

   TRP_RelativeMacro_c( void );
   TRP_RelativeMacro_c( const TRP_RelativeMacro_c& relativeMacro );
   ~TRP_RelativeMacro_c( void );

   TRP_RelativeMacro_c& operator=( const TRP_RelativeMacro_c& relativeMacro );
   bool operator==( const TRP_RelativeMacro_c& relativeMacro ) const;
   bool operator!=( const TRP_RelativeMacro_c& relativeMacro ) const;
   TRP_RelativeNode_c* operator[]( size_t relativeNodeIndex ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   const TRP_RelativeNodeList_t& GetRelativeNodeList( void ) const;
   size_t GetLength( void ) const;

   void Add( const char* pszFromBlockName,
             const char* pszToBlockName,
             TC_SideMode_t side,
             size_t* pfromNodeIndex = 0,
             size_t* ptoNodeIndex = 0 );
   void Add( size_t fromNodeIndex,
             const char* pszToBlockName,
             TC_SideMode_t side,
             size_t* ptoNodeIndex = 0 );
   void Add( size_t fromNodeIndex,
             size_t toNodeIndex,
             TC_SideMode_t side );
   void Add( const char* pszFromBlockName = 0,
             size_t* pfromNodeIndex = 0 );

   TRP_RelativeNode_c* Find( size_t relativeNodeIndex ) const;

   void Set( size_t relativeNodeIndex,
             const TGO_Point_c& origin,
             TGO_RotateMode_t rotate = TGO_ROTATE_UNDEFINED );
   void Set( size_t relativeNodeIndex,
             const TGO_Point_c& origin,
             TGO_RotateMode_t rotate,
             const TGO_IntDims_t& translate );
   void Clear( void );
   void Reset( void );

   bool IsValid( void ) const;

private:

   size_t AddNode_( const char* pszBlockName );
   void LinkNodes_( size_t fromNodeIndex,
                    size_t toNodeIndex,
                    TC_SideMode_t side ) const;
private:

   TRP_RelativeNodeList_t relativeNodeList_;

private:

   enum TRP_DefCapacity_e 
   { 
      TRP_RELATIVE_NODE_LIST_DEF_CAPACITY = 8
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline const TRP_RelativeNodeList_t& TRP_RelativeMacro_c::GetRelativeNodeList( 
      void ) const
{
   return( this->relativeNodeList_ );
}

//===========================================================================//
inline size_t TRP_RelativeMacro_c::GetLength( 
      void ) const
{
   return( this->relativeNodeList_.GetLength( ) );
}

//===========================================================================//
inline bool TRP_RelativeMacro_c::IsValid( 
      void ) const
{
   return( this->relativeNodeList_.IsValid( ) ? true : false );
}

#endif
