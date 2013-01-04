//===========================================================================//
// Purpose : Enums, typedefs, and defines for TVPR_Interface classes.
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

#ifndef TVPR_TYPEDEFS_H
#define TVPR_TYPEDEFS_H

#include "TCT_SortedVector.h"

//---------------------------------------------------------------------------//
// Define VPR interface data constants and typedefs
//---------------------------------------------------------------------------//

class TVPR_IndexCount_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedVector_c< TVPR_IndexCount_c > TVPR_IndexCountList_t;

#endif
