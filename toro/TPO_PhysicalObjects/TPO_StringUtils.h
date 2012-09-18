//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
//
//===========================================================================//

#ifndef TPO_STRING_UTILS_H
#define TPO_STRING_UTILS_H

#include <string>
using namespace std;

#include "TPO_Typedefs.h"

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//

void TPO_ExtractStringInstSource( TPO_InstSource_t source, string* psrSource );

void TPO_ExtractStringLatchType( TPO_LatchType_t type, string* psrType );
void TPO_ExtractStringLatchState( TPO_LatchState_t state, string* psrState );

void TPO_ExtractStringStatusMode( TPO_StatusMode_t status, string* psrStatus );

#endif
