//===========================================================================//
// Purpose : Method definitions for the TFH_FabricGridHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - GetLength
//           - GetBlockGridList
//           - Clear
//           - Add
//           - IsMember
//           - IsValid
//
//           Protected methods include:
//           - TFH_FabricGridHandler_c, ~TFH_FabricGridHandler_c
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
#include "TFH_FabricGridHandler.h"

// Initialize the block grid handler "singleton" class, as needed
TFH_FabricGridHandler_c* TFH_FabricGridHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TFH_FabricGridHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricGridHandler_c::TFH_FabricGridHandler_c( 
      void )
      :
      blockGridList_( TFH_BLOCK_GRID_LIST_DEF_CAPACITY )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TFH_FabricGridHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricGridHandler_c::~TFH_FabricGridHandler_c( 
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
void TFH_FabricGridHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TFH_FabricGridHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricGridHandler_c::DeleteInstance( 
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
TFH_FabricGridHandler_c& TFH_FabricGridHandler_c::GetInstance(
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
bool TFH_FabricGridHandler_c::HasInstance(
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
size_t TFH_FabricGridHandler_c::GetLength(
      void ) const
{
   return( this->blockGridList_.GetLength( )); 
}

//===========================================================================//
// Method         : GetBlockGridList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const TFH_BlockGridList_t& TFH_FabricGridHandler_c::GetBlockGridList(
      void ) const
{
   return( this->blockGridList_ );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricGridHandler_c::Clear( 
      void )
{
   this->blockGridList_.Clear( );
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricGridHandler_c::Add( 
      const string&         srBlockName,
            TFH_BlockType_t blockType,
      const TGO_Point_c&    gridPoint )
{
   TFH_BlockGrid_c blockGrid( srBlockName, blockType, gridPoint );
   this->Add( blockGrid );
}

//===========================================================================//
void TFH_FabricGridHandler_c::Add( 
      const char*           pszBlockName,
            TFH_BlockType_t blockType,
      const TGO_Point_c&    gridPoint )
{
   TFH_BlockGrid_c blockGrid( pszBlockName, blockType, gridPoint );
   this->Add( blockGrid );
}

//===========================================================================//
void TFH_FabricGridHandler_c::Add( 
      const TFH_BlockGrid_c& blockGrid )
{
   this->blockGridList_.Add( blockGrid );
}

//===========================================================================//
// Method         : IsMember
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricGridHandler_c::IsMember( 
      const string&         srBlockName,
            TFH_BlockType_t blockType,
      const TGO_Point_c&    gridPoint ) const
{
   TFH_BlockGrid_c blockGrid( srBlockName, blockType, gridPoint );
   return( this->IsMember( blockGrid ));
}

//===========================================================================//
bool TFH_FabricGridHandler_c::IsMember( 
      const char*           pszBlockName,
            TFH_BlockType_t blockType,
      const TGO_Point_c&    gridPoint ) const
{
   TFH_BlockGrid_c blockGrid( pszBlockName, blockType, gridPoint );
   return( this->IsMember( blockGrid ));
}

//===========================================================================//
bool TFH_FabricGridHandler_c::IsMember( 
      const TFH_BlockGrid_c& blockGrid ) const
{
   return( this->blockGridList_.IsMember( blockGrid ));
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricGridHandler_c::IsValid(
      void ) const
{
   return( this->blockGridList_.IsValid( ) ? true : false );
}
