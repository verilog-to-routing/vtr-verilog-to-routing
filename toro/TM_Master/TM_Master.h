//===========================================================================//
// Purpose : Declaration for a master input-processing-output class.
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

#ifndef TM_MASTER_H
#define TM_MASTER_H

#include "TCL_CommandLine.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TM_Master_c
{
public:

   TM_Master_c( void );
   TM_Master_c( const TCL_CommandLine_c& commandLine,
                bool* pok = 0 );
   ~TM_Master_c( void );

   bool Apply( const TCL_CommandLine_c& commandLine );

private:

   void NewHandlerSingletons_( void ) const;
   void DeleteHandlerSingletons_( void ) const;
};

#endif
