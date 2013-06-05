//===========================================================================//
// Purpose : Enums, typedefs, and defines for TGO_GeometricObject classes.
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

#ifndef TGO_TYPEDEFS_H
#define TGO_TYPEDEFS_H

#include "TCT_Dims.h"
#include "TCT_OrderedVector.h"

//---------------------------------------------------------------------------//
// Define geometric object class orientation typedefs
//---------------------------------------------------------------------------//

enum TGO_OrientMode_e 
{
   TGO_ORIENT_UNDEFINED = 0,
   TGO_ORIENT_HORIZONTAL,
   TGO_ORIENT_VERTICAL,
   TGO_ORIENT_DEFAULT,
   TGO_ORIENT_ALTERNATE
};
typedef enum TGO_OrientMode_e TGO_OrientMode_t;

//---------------------------------------------------------------------------//
// Define geometric object class direction typedefs
//---------------------------------------------------------------------------//

enum TGO_DirMode_e
{
   TGO_DIR_UNDEFINED = 0,
   TGO_DIR_LEFT,
   TGO_DIR_RIGHT,
   TGO_DIR_DOWN,
   TGO_DIR_UP,
   TGO_DIR_PREV,
   TGO_DIR_NEXT,
   TGO_DIR_ANY
};
typedef enum TGO_DirMode_e TGO_DirMode_t;

//---------------------------------------------------------------------------//
// Define geometric object class rotate typedefs
//---------------------------------------------------------------------------//

enum TGO_RotateMode_e
{
   TGO_ROTATE_UNDEFINED = 0,
   TGO_ROTATE_R0,
   TGO_ROTATE_R90,
   TGO_ROTATE_R180,
   TGO_ROTATE_R270,
   TGO_ROTATE_MX,
   TGO_ROTATE_MXR90,
   TGO_ROTATE_MY,
   TGO_ROTATE_MYR90,
   TGO_ROTATE_MAX
};
typedef enum TGO_RotateMode_e TGO_RotateMode_t;

//---------------------------------------------------------------------------//
// Define geometric object list typedefs
//---------------------------------------------------------------------------//

typedef TCT_Dims_c< int > TGO_IntDims_t;

class TGO_Point_c; // Forward declaration for subsequent class
typedef TCT_OrderedVector_c< TGO_Point_c > TGO_PointList_t;

class TGO_Region_c; // Forward declaration for subsequent class
typedef TCT_OrderedVector_c< TGO_Region_c > TGO_RegionList_t;

class TGO_Box_c; // Forward declaration for subsequent class
typedef TCT_OrderedVector_c< TGO_Box_c > TGO_BoxList_t;

#endif
