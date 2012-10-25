//===========================================================================//
// Purpose : Definition for the 'TOP_OptionsHandler_c' class.
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

#ifndef TOP_OPTIONS_HANDLER_H
#define TOP_OPTIONS_HANDLER_H

#include "TOS_OptionsStore.h"

#include "TOP_OptionsInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOP_OptionsHandler_c : public TOP_OptionsInterface_c
{
public:

   TOP_OptionsHandler_c( TOS_OptionsStore_c* poptionsStore );
   ~TOP_OptionsHandler_c( void );

   bool Init( TOS_OptionsStore_c* poptionsStore );

   bool IsValid( void ) const;

private:

   TOS_OptionsStore_c* poptionsStore_;
};

#endif
