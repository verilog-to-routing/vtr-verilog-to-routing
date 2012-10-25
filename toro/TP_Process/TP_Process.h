//===========================================================================//
// Purpose : Declaration and inline definitions for a TP_Process class.  This
//           class is responsible for the most excellent packing, placement 
//           and routing based on the given control and rule options.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TP_PROCESS_H
#define TP_PROCESS_H

#include "TOS_OptionsStore.h"

#include "TFS_FloorplanStore.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TP_Process_c
{
public:

   TP_Process_c( void );
   ~TP_Process_c( void );

   bool Apply( const TOS_OptionsStore_c& optionsStore,
               TFS_FloorplanStore_c* pfloorplanStore );
};

#endif 
