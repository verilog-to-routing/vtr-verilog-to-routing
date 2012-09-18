//===========================================================================//
// Purpose : Declaration and inline definitions for a TP_Process class.  This
//           class is responsible for the most excellent packing, placement 
//           and routing based on the given control and rule options.
//
//===========================================================================//

#ifndef TP_PROCESS_H
#define TP_PROCESS_H

#include "TOS_OptionsStore.h"

#include "TFS_FloorplanStore.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TP_Process_c
{
public:

   TP_Process_c( void );
   ~TP_Process_c( void );

   bool Apply( const TOS_OptionsStore_c& optionsStore,
               TFS_FloorplanStore_c* pfloorplanStore );
};

#endif 
