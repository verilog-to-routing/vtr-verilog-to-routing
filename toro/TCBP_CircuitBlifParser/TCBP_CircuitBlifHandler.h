//===========================================================================//
// Purpose : Definition for the 'TCBP_CircuitBlifHandler_c' class.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
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

#ifndef TCBP_CIRCUIT_BLIF_HANDLER_H
#define TCBP_CIRCUIT_BLIF_HANDLER_H

#include "TCD_CircuitDesign.h"

#include "TCBP_CircuitBlifInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TCBP_CircuitBlifHandler_c : public TCBP_CircuitBlifInterface_c
{
public:

   TCBP_CircuitBlifHandler_c( TCD_CircuitDesign_c* pcircuitDesign );
   ~TCBP_CircuitBlifHandler_c( void );

   bool Init( TCD_CircuitDesign_c* pcircuitDesign );

   bool IsValid( void ) const;

private:

   TCD_CircuitDesign_c* pcircuitDesign_;
};

#endif
