//===========================================================================//
// Purpose : Declaration and inline definitions for the TFH_GridBlock class.
//
//           Inline methods include:
//           - SetGridPoint, SetTypeIndex
//           - SetBlockType, SetBlockName, SetMasterName
//           - GetGridPoint, GetTypeIndex
//           - GetBlockType, GetBlockName, GetMasterName
//           - IsValid
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

#ifndef TFH_GRID_BLOCK_H
#define TFH_GRID_BLOCK_H

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
class TFH_GridBlock_c
{
public:

   TFH_GridBlock_c( void );
   TFH_GridBlock_c( const TGO_Point_c& vpr_gridPoint );
   TFH_GridBlock_c( const TGO_Point_c& vpr_gridPoint,
                    int vpr_typeIndex,
                    TFH_BlockType_t blockType,
                    const string& srBlockName,
                    const string& srMasterName );
   TFH_GridBlock_c( const TGO_Point_c& vpr_gridPoint,
                    int vpr_typeIndex,
                    TFH_BlockType_t blockType,
                    const char* pszBlockName,
                    const char* pszMasterName );
   TFH_GridBlock_c( int x, int y );
   TFH_GridBlock_c( const TFH_GridBlock_c& gridBlock );
   ~TFH_GridBlock_c( void );

   TFH_GridBlock_c& operator=( const TFH_GridBlock_c& gridBlock );
   bool operator<( const TFH_GridBlock_c& gridBlock ) const;
   bool operator==( const TFH_GridBlock_c& gridBlock ) const;
   bool operator!=( const TFH_GridBlock_c& gridBlock ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetGridPoint( const TGO_Point_c& vpr_gridPoint );
   void SetTypeIndex( int vpr_typeIndex );
   void SetBlockType( TFH_BlockType_t blockType );
   void SetBlockName( const string& srBlockName );
   void SetBlockName( const char* pszBlockName );
   void SetMasterName( const string& srMasterName );
   void SetMasterName( const char* pszMasterName );

   const TGO_Point_c& GetGridPoint( void ) const;
   int GetTypeIndex( void ) const;
   TFH_BlockType_t GetBlockType( void ) const;
   const char* GetBlockName( void ) const;
   const char* GetMasterName( void ) const;

   bool IsValid( void ) const;

private:

   TGO_Point_c vpr_gridPoint_;   // VPR's grid point array coordinate
   int vpr_typeIndex_;           // VPR's type_desciptors list index

   TFH_BlockType_t blockType_;   // Block type asso. with this block grid
   string srBlockName_;          // Block name asso. with this block grid
   string srMasterName_;         // Master name asso. with this block grid
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline void TFH_GridBlock_c::SetGridPoint( 
      const TGO_Point_c& vpr_gridPoint )
{
   this->vpr_gridPoint_ = vpr_gridPoint;
} 

//===========================================================================//
inline void TFH_GridBlock_c::SetTypeIndex( 
      int vpr_typeIndex )
{
   this->vpr_typeIndex_ = vpr_typeIndex;
} 

//===========================================================================//
inline void TFH_GridBlock_c::SetBlockType( 
      TFH_BlockType_t blockType )
{
   this->blockType_ = blockType;
} 

//===========================================================================//
inline void TFH_GridBlock_c::SetBlockName( 
      const string& srBlockName )
{
   this->srBlockName_ = srBlockName;
}

//===========================================================================//
inline void TFH_GridBlock_c::SetBlockName( 
      const char* pszBlockName )
{
   this->srBlockName_ = TIO_PSZ_STR( pszBlockName );
}

//===========================================================================//
inline void TFH_GridBlock_c::SetMasterName( 
      const string& srMasterName )
{
   this->srMasterName_ = srMasterName;
}

//===========================================================================//
inline void TFH_GridBlock_c::SetMasterName( 
      const char* pszMasterName )
{
   this->srMasterName_ = TIO_PSZ_STR( pszMasterName );
}

//===========================================================================//
inline const TGO_Point_c& TFH_GridBlock_c::GetGridPoint( 
      void ) const
{
   return( this->vpr_gridPoint_ );
}

//===========================================================================//
inline int TFH_GridBlock_c::GetTypeIndex( 
      void ) const
{
   return( this->vpr_typeIndex_ );
}

//===========================================================================//
inline TFH_BlockType_t TFH_GridBlock_c::GetBlockType( 
      void ) const
{
   return( this->blockType_ );
} 

//===========================================================================//
inline const char* TFH_GridBlock_c::GetBlockName( 
      void ) const
{
   return( TIO_SR_STR( this->srBlockName_ ));
}

//===========================================================================//
inline const char* TFH_GridBlock_c::GetMasterName( 
      void ) const
{
   return( TIO_SR_STR( this->srMasterName_ ));
}

//===========================================================================//
inline bool TFH_GridBlock_c::IsValid( 
      void ) const
{
   return( this->vpr_gridPoint_.IsValid( ) ? true : false );
}

#endif
