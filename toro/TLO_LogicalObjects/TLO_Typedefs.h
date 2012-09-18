//===========================================================================//
// Purpose : Enums, typedefs, and defines for TLO_LogicalObjects classes.
//
//===========================================================================//

#ifndef TLO_TYPEDEFS_H
#define TLO_TYPEDEFS_H

#include "TCT_OrderedVector.h"
#include "TCT_SortedNameDynamicVector.h"

//---------------------------------------------------------------------------//
// Define cell constants and typedefs
//---------------------------------------------------------------------------//

class TLO_Cell_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TLO_Cell_c > TLO_CellList_t;

enum TLO_CellSource_e
{
   TLO_CELL_SOURCE_UNDEFINED = 0,
   TLO_CELL_SOURCE_AUTO_DEFINED,
   TLO_CELL_SOURCE_USER_DEFINED,

   TLO_CELL_SOURCE_STANDARD = TLO_CELL_SOURCE_AUTO_DEFINED,
   TLO_CELL_SOURCE_CUSTOM = TLO_CELL_SOURCE_USER_DEFINED
};
typedef enum TLO_CellSource_e TLO_CellSource_t;

//---------------------------------------------------------------------------//
// Define port constants and typedefs
//---------------------------------------------------------------------------//

class TLO_Port_c; // Forward declaration for subsequent class typedefs
typedef TCT_SortedNameDynamicVector_c< TLO_Port_c > TLO_PortList_t;

//---------------------------------------------------------------------------//
// Define pin constants and typedefs
//---------------------------------------------------------------------------//

class TC_NameType_c; // Forward declaration for subsequent class typedefs
typedef TC_NameType_c TLO_Pin_t;
typedef TCT_SortedNameDynamicVector_c< TLO_Pin_t > TLO_PinList_t;

#endif
