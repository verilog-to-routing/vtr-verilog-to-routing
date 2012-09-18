//===========================================================================//
// Purpose : Function prototypes for miscellaneous helpful string functions.
//
//===========================================================================//

#ifndef TAS_STRING_UTILS_H
#define TAS_STRING_UTILS_H

#include <string>
using namespace std;

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Function prototypes
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//

void TAS_ExtractStringClassType( TAS_ClassType_t type, string* psrType );

void TAS_ExtractStringPhysicalBlockModelType( TAS_PhysicalBlockModelType_t type, string* psrType );
void TAS_ExtractStringInterconnectMapType( TAS_InterconnectMapType_t type, string* psrType );
void TAS_ExtractStringConnectionBoxType( TAS_ConnectionBoxType_t type, string* psrType );

void TAS_ExtractStringPinAssignPatternType( TAS_PinAssignPatternType_t type, string* psrType );
void TAS_ExtractStringGridAssignDistrMode( TAS_GridAssignDistrMode_t mode, string* psrMode );
void TAS_ExtractStringGridAssignOrientMode( TAS_GridAssignOrientMode_t mode, string* psrMode );

void TAS_ExtractStringTimingDelayType( TAS_TimingDelayType_t type, string* psrType );

void TAS_ExtractStringArraySizeMode( TAS_ArraySizeMode_t mode, string* psrMode );

void TAS_ExtractStringSwitchBoxType( TAS_SwitchBoxType_t type, string* psrType );
void TAS_ExtractStringSwitchBoxModelType( TAS_SwitchBoxModelType_t type, string* psrType );

void TAS_ExtractStringSegmentDirType( TAS_SegmentDirType_t type, string* psrType );

void TAS_ExtractStringChannelUsageMode( TAS_ChannelUsageMode_t mode, string* psrMode );
void TAS_ExtractStringChannelDistrMode( TAS_ChannelDistrMode_t mode, string* psrMode );

#endif
