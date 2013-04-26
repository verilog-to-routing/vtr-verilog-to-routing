//===========================================================================//
// Purpose : Method definitions for the TFH_FabricBlockHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - GetLength
//           - GetGridBlockList
//           - Clear
//           - Add
//           - IsMember
//           - IsValid
//
//           Protected methods include:
//           - TFH_FabricBlockHandler_c, ~TFH_FabricBlockHandler_c
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

#include "TFH_Typedefs.h"
#include "TFH_FabricBlockHandler.h"

// Initialize the grid block handler "singleton" class, as needed
TFH_FabricBlockHandler_c* TFH_FabricBlockHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TFH_FabricBlockHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricBlockHandler_c::TFH_FabricBlockHandler_c( 
      void )
      :
      gridBlockList_( TFH_GRID_BLOCK_LIST_DEF_CAPACITY )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TFH_FabricBlockHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricBlockHandler_c::~TFH_FabricBlockHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricBlockHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TFH_FabricBlockHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricBlockHandler_c::DeleteInstance( 
      void )
{
   if( pinstance_ )
   {
      delete pinstance_;
      pinstance_ = 0;
   }
}

//===========================================================================//
// Method         : GetInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricBlockHandler_c& TFH_FabricBlockHandler_c::GetInstance(
      bool newInstance )
{
   if( !pinstance_ )
   {
      if( newInstance )
      {
         NewInstance( );
      }
   }
   return( *pinstance_ );
}

//===========================================================================//
// Method         : HasInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricBlockHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : GetLength
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
size_t TFH_FabricBlockHandler_c::GetLength(
      void ) const
{
   return( this->gridBlockList_.GetLength( )); 
}

//===========================================================================//
// Method         : GetGridBlockList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const TFH_GridBlockList_t& TFH_FabricBlockHandler_c::GetGridBlockList(
      void ) const
{
   return( this->gridBlockList_ );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricBlockHandler_c::Clear( 
      void )
{
   this->gridBlockList_.Clear( );
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricBlockHandler_c::Add( 
      const TGO_Point_c&    vpr_gridPoint,
            int             vpr_typeIndex,
            TFH_BlockType_t blockType,
      const string&         srBlockName,
      const string&         srMasterName )
{
   TFH_GridBlock_c gridBlock( vpr_gridPoint, vpr_typeIndex,
                              blockType, srBlockName, srMasterName );
   this->Add( gridBlock );
}

//===========================================================================//
void TFH_FabricBlockHandler_c::Add( 
      const TGO_Point_c&    vpr_gridPoint,
            int             vpr_typeIndex,
            TFH_BlockType_t blockType,
      const char*           pszBlockName,
      const char*           pszMasterName )
{
   TFH_GridBlock_c gridBlock( vpr_gridPoint, vpr_typeIndex,
                              blockType, pszBlockName, pszMasterName );
   this->Add( gridBlock );
}

//===========================================================================//
void TFH_FabricBlockHandler_c::Add( 
      const TFH_GridBlock_c& gridBlock )
{
   this->gridBlockList_.Add( gridBlock );
}

//===========================================================================//
// Method         : IsMember
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricBlockHandler_c::IsMember( 
      const TGO_Point_c& vpr_gridPoint ) const
{
   TFH_GridBlock_c gridBlock( vpr_gridPoint );
   return( this->IsMember( gridBlock ));
}

//===========================================================================//
bool TFH_FabricBlockHandler_c::IsMember( 
      const TFH_GridBlock_c& gridBlock ) const
{
   return( this->gridBlockList_.IsMember( gridBlock ));
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricBlockHandler_c::IsValid(
      void ) const
{
   return( this->gridBlockList_.IsValid( ) ? true : false );
}
