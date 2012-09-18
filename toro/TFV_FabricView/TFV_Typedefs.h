//===========================================================================//
// Purpose : Enums, typedefs, and defines for TFV_FabricView classes.
//
//===========================================================================//

#ifndef TFV_TYPEDEFS_H
#define TFV_TYPEDEFS_H

#include "TTPT_Tile.h"
#include "TTPT_TilePlane.h"
#include "TTPT_TilePlaneIter.h"

#include "TCT_Range.h"
#include "TCT_RangeOrderedVector.h"

#include "TCT_SortedVector.h"

//---------------------------------------------------------------------------//
// Define fabric view constants and typedefs
//---------------------------------------------------------------------------//

class TFV_FabricLayer_c; // Forward declaration for subsequent class typedefs

typedef TCT_RangeOrderedVector_c< TFV_FabricLayer_c > TFV_FabricLayerList_t;

typedef TCT_Range_c< TGS_Layer_t > TFV_FabricRange_t;

//---------------------------------------------------------------------------//
// Define fabric layer constants and typedefs
//---------------------------------------------------------------------------//

enum TFV_LayerType_e
{
   TFV_LAYER_UNDEFINED = 0,
   TFV_LAYER_INPUT_OUTPUT,
   TFV_LAYER_PHYSICAL_BLOCK,
   TFV_LAYER_SWITCH_BOX,
   TFV_LAYER_CONNECTION_BOX,
   TFV_LAYER_CHANNEL_HORZ,
   TFV_LAYER_CHANNEL_VERT,
   TFV_LAYER_SEGMENT_HORZ,
   TFV_LAYER_SEGMENT_VERT,
   TFV_LAYER_INTERNAL_GRID,
   TFV_LAYER_INTERNAL_MAP,
   TFV_LAYER_INTERNAL_PIN,
   TFV_LAYER_BOUNDING_BOX
};
typedef enum TFV_LayerType_e TFV_LayerType_t;

#define TFV_LAYER_MIN ( TFV_LAYER_INPUT_OUTPUT )
#define TFV_LAYER_MAX ( TFV_LAYER_SEGMENT_VERT )

//---------------------------------------------------------------------------//
// Define fabric plane constants and typedefs
//---------------------------------------------------------------------------//

class TFV_FabricData_c; // Forward declaration for subsequent class typedefs

typedef TTPT_TilePlane_c< TFV_FabricData_c > TFV_FabricPlane_t;
typedef TTPT_TilePlaneIter_c< TFV_FabricData_c > TFV_FabricPlaneIter_t;

enum TFV_DataType_e
{
   TFV_DATA_UNDEFINED = 0,
   TFV_DATA_INPUT_OUTPUT,
   TFV_DATA_PHYSICAL_BLOCK,
   TFV_DATA_SWITCH_BOX,
   TFV_DATA_CONNECTION_BOX,
   TFV_DATA_CHANNEL_HORZ,
   TFV_DATA_CHANNEL_VERT,
   TFV_DATA_SEGMENT_HORZ,
   TFV_DATA_SEGMENT_VERT
};
typedef enum TFV_DataType_e TFV_DataType_t;

//---------------------------------------------------------------------------//
// Define fabric figure constants and typedefs
//---------------------------------------------------------------------------//

class TFV_FabricData_c; // Forward declaration for subsequent class typedefs

typedef TTPT_Tile_c< TFV_FabricData_c > TFV_FabricFigure_t;

//---------------------------------------------------------------------------//
// Define fabric data constants and typedefs
//---------------------------------------------------------------------------//

// Define fabric model default sizes
// (used when extracting a fabric model from VPR's internal data structures)

#define TFV_MODEL_INPUT_OUTPUT_DEF_WIDTH   0.25
#define TFV_MODEL_PHYSICAL_BLOCK_DEF_WIDTH 0.25
#define TFV_MODEL_CONNECTION_BOX_DEF_WIDTH 0.004
#define TFV_MODEL_PIN_DEF_WIDTH            0.002
#define TFV_MODEL_PIN_DEF_SPACING          0.004
#define TFV_MODEL_SEGMENT_DEF_WIDTH        0.005
#define TFV_MODEL_SEGMENT_DEF_SPACING      0.005

//---------------------------------------------------------------------------//
// Define fabric pin constants and typedefs
//---------------------------------------------------------------------------//

class TFV_FabricPin_c; // Forward declaration for subsequent class typedefs

typedef TCT_SortedVector_c< TFV_FabricPin_c > TFV_FabricPinList_t;

class TC_SideIndex_c; // Forward declaration for subsequent class typedefs

typedef TC_SideIndex_c TFV_Connection_t;
typedef TCT_SortedVector_c< TFV_Connection_t > TFV_ConnectionList_t;

//---------------------------------------------------------------------------//
// Define fabric view interface constants and typedefs
//---------------------------------------------------------------------------//

typedef TTP_AddMode_t TFV_AddMode_t;

#define TFV_ADD_NEW          TTP_ADD_NEW
#define TFV_ADD_MERGE        TTP_ADD_MERGE
#define TFV_ADD_OVERLAP      TTP_ADD_OVERLAP
#define TFV_ADD_REGION       TTP_ADD_REGION

typedef TTP_DeleteMode_t TFV_DeleteMode_t;

#define TFV_DELETE_EXACT     TTP_DELETE_EXACT
#define TFV_DELETE_WITHIN    TTP_DELETE_WITHIN
#define TFV_DELETE_INTERSECT TTP_DELETE_INTERSECT

typedef TTP_FindMode_t TFV_FindMode_t;

#define TFV_FIND_EXACT       TTP_FIND_EXACT
#define TFV_FIND_WITHIN      TTP_FIND_WITHIN
#define TFV_FIND_INTERSECT   TTP_FIND_INTERSECT

#define TFV_IsClearMode_t TTP_IsClearMode_t

#define TFV_IS_CLEAR_ANY     TTP_IS_CLEAR_ANY
#define TFV_IS_CLEAR_ALL     TTP_IS_CLEAR_ALL

#define TFV_IsSolidMode_t TTP_IsSolidMode_t

#define TFV_IS_SOLID_MATCH   TTP_IS_SOLID_MATCH
#define TFV_IS_SOLID_ANY     TTP_IS_SOLID_ANY
#define TFV_IS_SOLID_ALL     TTP_IS_SOLID_ALL

#define TFV_FigureMode_t TTP_TileMode_t

#define TFV_FIGURE_UNDEFINED TTP_TILE_UNDEFINED
#define TFV_FIGURE_CLEAR     TTP_TILE_CLEAR
#define TFV_FIGURE_SOLID     TTP_TILE_SOLID
#define TFV_FIGURE_ANY       TTP_TILE_ANY

#endif
