//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
//
//===========================================================================//

#ifndef TTP_STRING_UTILS_H
#define TTP_STRING_UTILS_H

#include <string>
using namespace std;

#include "TTP_Typedefs.h"

//===========================================================================//
// Function prototypes
//---------------------------------------------------------------------------//
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TTP_ExtractStringTileMode( TTP_TileMode_t tileMode,
                                string* psrTileMode );

#endif 
