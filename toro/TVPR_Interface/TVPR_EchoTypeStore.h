//===========================================================================//
// Purpose : Declaration for the TVPR_EchoTypeStore class.
//
//===========================================================================//
#ifndef TVPR_ECHO_TYPE_STORE_H
#define TVPR_ECHO_TYPE_STORE_H

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
class TVPR_EchoTypeStore_c
{
public:

   enum e_echo_files echoType;
   const char*       pszFileType;
};

#endif
