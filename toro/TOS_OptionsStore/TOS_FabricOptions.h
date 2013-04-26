//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_FabricOptions class.
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

#ifndef TOS_FABRIC_OPTIONS_H
#define TOS_FABRIC_OPTIONS_H

#include <cstdio>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/12/13 jeffr : Original
//===========================================================================//
class TOS_FabricOptions_c
{
public:

   TOS_FabricOptions_c( void );
   TOS_FabricOptions_c( bool blocks_override_,
                        bool switchBoxes_override_,
                        bool connectionBlocks_override_,
                        bool channels_override_ );
   ~TOS_FabricOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   class TOS_Blocks_c
   {
   public:

      bool override;       // Enables applying override block constraints, if any

   } blocks;

   class TOS_SwitchBoxes_c
   {
   public:

      bool override;       // Enables applying override switch box constraints, if any

   } switchBoxes;

   class TOS_ConnectionBlocks_c
   {
   public:

      bool override;       // Enables applying override connection blocks constraints, if any

   } connectionBlocks;

   class TOS_Channels_c
   {
   public:

      bool override;       // Enables applying override channel constraints, if any

   } channels;
};

#endif 
