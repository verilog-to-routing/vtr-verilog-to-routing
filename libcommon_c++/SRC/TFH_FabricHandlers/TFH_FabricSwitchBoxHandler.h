//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TFH_FabricSwitchBoxHandler singleton class. This class supports
//           switch box overrides for the VPR place and route tool.
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

#ifndef TFH_FABRIC_SWITCH_BOX_HANDLER_H
#define TFH_FABRIC_SWITCH_BOX_HANDLER_H

#include <cstdio>
using namespace std;

#include "TFH_Typedefs.h"
#include "TFH_SwitchBox.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TFH_FabricSwitchBoxHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TFH_FabricSwitchBoxHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   size_t GetLength( void ) const;
   const TFH_SwitchBoxList_t& GetSwitchBoxList( void ) const;

   void Clear( void );

   void Add( const TGO_Point_c& vpr_gridPoint,
             const string& srSwitchBoxName,
             const TC_MapTable_c& mapTable );
   void Add( const TGO_Point_c& vpr_gridPoint,
             const char* pszSwitchBoxName,
             const TC_MapTable_c& mapTable );
   void Add( const TFH_SwitchBox_c& switchBox );

   bool IsMember( const TGO_Point_c& vpr_gridPoint ) const;
   bool IsMember( const TFH_SwitchBox_c& switchBox ) const;

   bool IsValid( void ) const;

protected:

   TFH_FabricSwitchBoxHandler_c( void );
   ~TFH_FabricSwitchBoxHandler_c( void );

private:

   TFH_SwitchBoxList_t switchBoxList_;

   static TFH_FabricSwitchBoxHandler_c* pinstance_; // Define ptr for singleton instance

private:

   enum TFH_DefCapacity_e 
   { 
      TFH_SWITCH_BOX_LIST_DEF_CAPACITY = 64
   };
};

#endif
