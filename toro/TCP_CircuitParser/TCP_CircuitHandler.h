//===========================================================================//
// Purpose : Definition for the 'TCP_CircuitHandler_c' class.
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

#ifndef TCP_CIRCUIT_HANDLER_H
#define TCP_CIRCUIT_HANDLER_H

#include "TCD_CircuitDesign.h"

#include "TCP_CircuitInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
class TCP_CircuitHandler_c : public TCP_CircuitInterface_c
{
public:

   TCP_CircuitHandler_c( TCD_CircuitDesign_c* pcircuitDesign );
   ~TCP_CircuitHandler_c( void );

   bool Init( TCD_CircuitDesign_c* pcircuitDesign );

   bool SyntaxError( unsigned int lineNum, 
                     const string& srFileName,
                     const char* pszMessageText );

   bool IsValid( void ) const;

private:

   TCD_CircuitDesign_c* pcircuitDesign_;
};

#endif
