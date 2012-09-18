//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
//
//===========================================================================//

#ifndef TFV_STRING_UTILS_H
#define TFV_STRING_UTILS_H

#include <string>
using namespace std;

#include "TFV_Typedefs.h"

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//

void TFV_ExtractStringLayerType( TFV_LayerType_t type, string* psrType );
void TFV_ExtractStringDataType( TFV_DataType_t type, string* psrType );

#endif
