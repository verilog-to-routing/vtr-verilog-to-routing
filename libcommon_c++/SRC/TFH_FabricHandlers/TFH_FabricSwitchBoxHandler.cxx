//===========================================================================//
// Purpose : Method definitions for the TFH_FabricSwitchBoxHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - GetLength
//           - GetSwitchBoxList
//           - Clear
//           - Add
//           - IsMember
//           - IsValid
//
//           Protected methods include:
//           - TFH_FabricSwitchBoxHandler_c, ~TFH_FabricSwitchBoxHandler_c
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
#include "TFH_FabricSwitchBoxHandler.h"

// Initialize the switch box handler "singleton" class, as needed
TFH_FabricSwitchBoxHandler_c* TFH_FabricSwitchBoxHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TFH_FabricSwitchBoxHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricSwitchBoxHandler_c::TFH_FabricSwitchBoxHandler_c( 
      void )
      :
      switchBoxList_( TFH_SWITCH_BOX_LIST_DEF_CAPACITY )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TFH_FabricSwitchBoxHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
TFH_FabricSwitchBoxHandler_c::~TFH_FabricSwitchBoxHandler_c( 
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
void TFH_FabricSwitchBoxHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TFH_FabricSwitchBoxHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricSwitchBoxHandler_c::DeleteInstance( 
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
TFH_FabricSwitchBoxHandler_c& TFH_FabricSwitchBoxHandler_c::GetInstance(
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
bool TFH_FabricSwitchBoxHandler_c::HasInstance(
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
size_t TFH_FabricSwitchBoxHandler_c::GetLength(
      void ) const
{
   return( this->switchBoxList_.GetLength( )); 
}

//===========================================================================//
// Method         : GetSwitchBoxList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
const TFH_SwitchBoxList_t& TFH_FabricSwitchBoxHandler_c::GetSwitchBoxList(
      void ) const
{
   return( this->switchBoxList_ );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricSwitchBoxHandler_c::Clear( 
      void )
{
   this->switchBoxList_.Clear( );
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
void TFH_FabricSwitchBoxHandler_c::Add( 
      const TGO_Point_c&   vpr_gridPoint,
      const string&        srSwitchBoxName,
      const TC_MapTable_c& mapTable )
{
   TFH_SwitchBox_c switchBox( vpr_gridPoint, srSwitchBoxName, mapTable );

   this->Add( switchBox );
}

//===========================================================================//
void TFH_FabricSwitchBoxHandler_c::Add( 
      const TGO_Point_c&   vpr_gridPoint,
      const char*          pszSwitchBoxName,
      const TC_MapTable_c& mapTable )
{
   TFH_SwitchBox_c switchBox( vpr_gridPoint, pszSwitchBoxName, mapTable );

   this->Add( switchBox );
}

//===========================================================================//
void TFH_FabricSwitchBoxHandler_c::Add( 
      const TFH_SwitchBox_c& switchBox )
{
   this->switchBoxList_.Add( switchBox );
}

//===========================================================================//
// Method         : IsMember
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricSwitchBoxHandler_c::IsMember( 
      const TGO_Point_c& vpr_gridPoint ) const
{
   TFH_SwitchBox_c switchBox( vpr_gridPoint );
   return( this->IsMember( switchBox ));
}

//===========================================================================//
bool TFH_FabricSwitchBoxHandler_c::IsMember( 
      const TFH_SwitchBox_c& switchBox ) const
{
   return( this->switchBoxList_.IsMember( switchBox ));
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
bool TFH_FabricSwitchBoxHandler_c::IsValid(
      void ) const
{
   return( this->switchBoxList_.IsValid( ) ? true : false );
}
