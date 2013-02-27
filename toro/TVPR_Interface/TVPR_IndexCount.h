//===========================================================================//
// Purpose : Declaration and inline definitions for a TVPR_IndexCount class.
//
//           Inline methods include:
//           - GetBlockIndex, GetNodeIndex, GetSiblingCount
//           - SetBlockIndex, SetNodeIndex, SetSiblingCount
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

#ifndef TVPR_INDEX_COUNT_H
#define TVPR_INDEX_COUNT_H

#include <cstdio>
#include <climits>
using namespace std;

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
class TVPR_IndexCount_c
{
public:

   TVPR_IndexCount_c( void );
   TVPR_IndexCount_c( int blockIndex,
                      int nodeIndex,
                      size_t siblingCount = SIZE_MAX );
   TVPR_IndexCount_c( const TVPR_IndexCount_c& indexCount );
   ~TVPR_IndexCount_c( void );

   TVPR_IndexCount_c& operator=( const TVPR_IndexCount_c& indexCount );
   bool operator<( const TVPR_IndexCount_c& indexCount ) const;
   bool operator==( const TVPR_IndexCount_c& indexCount ) const;
   bool operator!=( const TVPR_IndexCount_c& indexCount ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrIndexCount ) const;

   int GetBlockIndex( void ) const;
   int GetNodeIndex( void ) const;
   size_t GetSiblingCount( void ) const;

   void SetBlockIndex( int blockIndex );
   void SetNodeIndex( int nodeIndex );
   void SetSiblingCount( size_t siblingCount );

   void Clear( void );

   bool IsValid( void ) const;

private:

   int    blockIndex_;
   int    nodeIndex_;
   size_t siblingCount_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 10/05/12 jeffr : Original
//===========================================================================//
inline int TVPR_IndexCount_c::GetBlockIndex( 
      void ) const
{
   return( this->blockIndex_ );
}

//===========================================================================//
inline int TVPR_IndexCount_c::GetNodeIndex( 
      void ) const
{
   return( this->nodeIndex_ );
}

//===========================================================================//
inline size_t TVPR_IndexCount_c::GetSiblingCount( 
      void ) const
{
   return( this->siblingCount_ );
}

//===========================================================================//
inline void TVPR_IndexCount_c::SetBlockIndex( 
      int blockIndex )
{
   this->blockIndex_ = blockIndex;
} 

//===========================================================================//
inline void TVPR_IndexCount_c::SetNodeIndex( 
      int nodeIndex )
{
   this->nodeIndex_ = nodeIndex;
} 

//===========================================================================//
inline void TVPR_IndexCount_c::SetSiblingCount( 
      size_t siblingCount )
{
   this->siblingCount_ = siblingCount;
} 

//===========================================================================//
inline bool TVPR_IndexCount_c::IsValid( 
      void ) const
{
   return(( this->blockIndex_ != INT_MAX ) &&
          ( this->nodeIndex_ != INT_MAX ) ? 
          true : false );
}

#endif
