//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
//
//===========================================================================//

#ifndef TGS_STRING_UTILS_H
#define TGS_STRING_UTILS_H

#include <string>
using namespace std;

#include "TGS_Typedefs.h"

//===========================================================================//
// Function prototypes
//---------------------------------------------------------------------------//
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TGS_ExtractStringOrientMode( TGS_OrientMode_t orientMode,
                                  string* psrOrientMode );

#endif 
