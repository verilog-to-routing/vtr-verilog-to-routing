//===========================================================================//
// Purpose : Enums, typedefs, and defines for TGS_GeometricShape classes.
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
//---------------------------------------------------------------------------//

#ifndef TGS_TYPEDEFS_H
#define TGS_TYPEDEFS_H

#include <climits>
using namespace std;

#include "TCT_Dims.h"
#include "TCT_OrderedVector.h"

//---------------------------------------------------------------------------//
// Define geometric shape class orientation typedefs
//---------------------------------------------------------------------------//

enum TGS_OrientMode_e 
{
   TGS_ORIENT_UNDEFINED = 0,
   TGS_ORIENT_HORIZONTAL,
   TGS_ORIENT_VERTICAL,
   TGS_ORIENT_DEFAULT,
   TGS_ORIENT_ALTERNATE
};
typedef enum TGS_OrientMode_e TGS_OrientMode_t;

//---------------------------------------------------------------------------//
// Define geometric shape class side typedefs
//---------------------------------------------------------------------------//

enum TGS_CornerMode_e
{
   TGS_CORNER_UNDEFINED = 0,
   TGS_CORNER_LOWER_LEFT,
   TGS_CORNER_LOWER_RIGHT,
   TGS_CORNER_UPPER_LEFT,
   TGS_CORNER_UPPER_RIGHT,

   TGS_CORNER_LL = TGS_CORNER_LOWER_LEFT,
   TGS_CORNER_LR = TGS_CORNER_LOWER_RIGHT,
   TGS_CORNER_UL = TGS_CORNER_UPPER_LEFT,
   TGS_CORNER_UR = TGS_CORNER_UPPER_RIGHT,

   TGS_CORNER_CENTER
};
typedef enum TGS_CornerMode_e TGS_CornerMode_t;

//---------------------------------------------------------------------------//
// Define geometric shape class direction typedefs
//---------------------------------------------------------------------------//

enum TGS_DirMode_e
{
   TGS_DIR_UNDEFINED = 0,
   TGS_DIR_LEFT,
   TGS_DIR_RIGHT,
   TGS_DIR_DOWN,
   TGS_DIR_UP,
   TGS_DIR_PREV,
   TGS_DIR_NEXT,
   TGS_DIR_ANY
};
typedef enum TGS_DirMode_e TGS_DirMode_t;

//---------------------------------------------------------------------------//
// Define geometric grid class snap typedefs
//---------------------------------------------------------------------------//

enum TGS_SnapMode_e
{
   TGS_SNAP_UNDEFINED = 0,
   TGS_SNAP_MIN_GRID
};
typedef enum TGS_SnapMode_e TGS_SnapMode_t;

//---------------------------------------------------------------------------//
// Define geometric array grid snap typedefs
//---------------------------------------------------------------------------//

enum TGS_ArraySnapMode_e 
{ 
   TGS_ARRAY_SNAP_UNDEFINED = 0,
   TGS_ARRAY_SNAP_WITHIN,
   TGS_ARRAY_SNAP_NEAREST
};
typedef enum TGS_ArraySnapMode_e TGS_ArraySnapMode_t;
 
typedef TCT_Dims_c< int > TGS_IntDims_t;
typedef TCT_Dims_c< double > TGS_FloatDims_t;

//---------------------------------------------------------------------------//
// Define geometric shape class scale typedefs
//---------------------------------------------------------------------------//

class TGS_Point_c; // Forward declaration for subsequent class typedefs
typedef TGS_Point_c TGS_Scale_t;

//---------------------------------------------------------------------------//
// Define geometric shape class layer typedefs
//---------------------------------------------------------------------------//

typedef int TGS_Layer_t;

#define TGS_LAYER_UNDEFINED INT_MIN

#include "TCT_Range.h"

typedef TCT_Range_c< TGS_Layer_t > TGS_LayerRange_t;

//---------------------------------------------------------------------------//
// Define geometric shape list typedefs
//---------------------------------------------------------------------------//

class TGS_Point_c; // Forward declaration for subsequent class
typedef TCT_OrderedVector_c< TGS_Point_c > TGS_PointList_t;

class TGS_Region_c; // Forward declaration for subsequent class
typedef TCT_OrderedVector_c< TGS_Region_c > TGS_RegionList_t;

class TGS_Rect_c; // Forward declaration for subsequent class
typedef TCT_OrderedVector_c< TGS_Rect_c > TGS_RectList_t;

class TGS_Line_c; // Forward declaration for subsequent class
typedef TCT_OrderedVector_c< TGS_Line_c > TGS_LineList_t;

class TGS_Box_c; // Forward declaration for subsequent class
typedef TCT_OrderedVector_c< TGS_Box_c > TGS_BoxList_t;

#endif
