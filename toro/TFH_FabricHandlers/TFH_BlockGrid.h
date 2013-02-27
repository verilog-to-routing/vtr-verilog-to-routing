//===========================================================================//
// Purpose : Declaration and inline definitions for the TFH_BlockGrid class.
//
//           Inline methods include:
//           - SetBlockName
//           - SetBlockType
//           - SetGridPoint
//           - GetBlockName
//           - GetBlockType
//           - GetGridPoint
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

#ifndef TFH_BLOCK_GRID_H
#define TFH_BLOCK_GRID_H

#include <cstdio>
#include <string>
using namespace std;

#include "TGO_Point.h"

#include "TFH_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TFH_BlockGrid_c
{
public:

   TFH_BlockGrid_c( void );
   TFH_BlockGrid_c( const string& srBlockName,
                    TFH_BlockType_t blockType,
                    const TGO_Point_c& gridPoint );
   TFH_BlockGrid_c( const char* pszBlockName,
                    TFH_BlockType_t blockType,
                    const TGO_Point_c& gridPoint );
   TFH_BlockGrid_c( const TFH_BlockGrid_c& blockGrid );
   ~TFH_BlockGrid_c( void );

   TFH_BlockGrid_c& operator=( const TFH_BlockGrid_c& blockGrid );
   bool operator<( const TFH_BlockGrid_c& blockGrid ) const;
   bool operator==( const TFH_BlockGrid_c& blockGrid ) const;
   bool operator!=( const TFH_BlockGrid_c& blockGrid ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetBlockName( const string& srBlockName );
   void SetBlockName( const char* pszBlockName );
   void SetBlockType( TFH_BlockType_t blockType );
   void SetGridPoint( const TGO_Point_c& gridPoint );

   const char* GetBlockName( void ) const;
   TFH_BlockType_t GetBlockType( void ) const;
   const TGO_Point_c& GetGridPoint( void ) const;

   bool IsValid( void ) const;

private:

   string srBlockName_;          // Block name asso. with this block grid
   TFH_BlockType_t blockType_;   // Block type asso. with this block grid
   TGO_Point_c gridPoint_;       // VPR's grid point array coordinate
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline void TFH_BlockGrid_c::SetBlockName( 
      const string& srBlockName )
{
   this->srBlockName_ = srBlockName;
}

//===========================================================================//
inline void TFH_BlockGrid_c::SetBlockName( 
      const char* pszBlockName )
{
   this->srBlockName_ = TIO_PSZ_STR( pszBlockName );
}

//===========================================================================//
inline void TFH_BlockGrid_c::SetBlockType( 
      TFH_BlockType_t blockType )
{
   this->blockType_ = blockType;
} 

//===========================================================================//
inline void TFH_BlockGrid_c::SetGridPoint( 
      const TGO_Point_c& gridPoint )
{
   this->gridPoint_ = gridPoint;
} 

//===========================================================================//
inline const char* TFH_BlockGrid_c::GetBlockName( 
      void ) const
{
   return( TIO_SR_STR( this->srBlockName_ ));
}

//===========================================================================//
inline TFH_BlockType_t TFH_BlockGrid_c::GetBlockType( 
      void ) const
{
   return( this->blockType_ );
} 

//===========================================================================//
inline const TGO_Point_c& TFH_BlockGrid_c::GetGridPoint( 
      void ) const
{
   return( this->gridPoint_ );
}

//===========================================================================//
inline bool TFH_BlockGrid_c::IsValid( 
      void ) const
{
   return( this->gridPoint_.IsValid( ) ? true : false );
}

#endif
