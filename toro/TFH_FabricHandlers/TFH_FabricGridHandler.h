//===========================================================================//
// Purpose : Declaration and inline definitions for the universal 
//           TFH_FabricGridHandler singleton class. This class supports
//           block grid overrides for the VPR placement tool
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

#ifndef TFH_FABRIC_GRID_HANDLER_H
#define TFH_FABRIC_GRID_HANDLER_H

#include <cstdio>
using namespace std;

#include "TFH_Typedefs.h"
#include "TFH_BlockGrid.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TFH_FabricGridHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TFH_FabricGridHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   size_t GetLength( void ) const;
   const TFH_BlockGridList_t& GetBlockGridList( void ) const;

   void Clear( void );

   void Add( const string& srBlockName,
             TFH_BlockType_t blockType,
	     const TGO_Point_c& gridPoint );
   void Add( const char* pszBlockName,
             TFH_BlockType_t blockType,
             const TGO_Point_c& gridPoint );
   void Add( const TFH_BlockGrid_c& blockGrid );

   bool IsMember( const string& srBlockName,
                  TFH_BlockType_t blockType,
                  const TGO_Point_c& gridPoint ) const;
   bool IsMember( const char* pszBlockName,
                  TFH_BlockType_t blockType,
                  const TGO_Point_c& gridPoint ) const;
   bool IsMember( const TFH_BlockGrid_c& blockGrid ) const;

   bool IsValid( void ) const;

protected:

   TFH_FabricGridHandler_c( void );
   ~TFH_FabricGridHandler_c( void );

private:

   TFH_BlockGridList_t blockGridList_;

   static TFH_FabricGridHandler_c* pinstance_; // Define ptr for singleton instance

private:

   enum TFH_DefCapacity_e 
   { 
      TFH_BLOCK_GRID_LIST_DEF_CAPACITY = 1
   };
};

#endif
