//===========================================================================//
// Purpose : Definition for the 'TOP_OptionsHandler_c' class.
//
//===========================================================================//

#ifndef TOP_OPTIONS_HANDLER_H
#define TOP_OPTIONS_HANDLER_H

#include "TOS_OptionsStore.h"

#include "TOP_OptionsInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOP_OptionsHandler_c : public TOP_OptionsInterface_c
{
public:

   TOP_OptionsHandler_c( TOS_OptionsStore_c* poptionsStore );
   ~TOP_OptionsHandler_c( void );

   bool Init( TOS_OptionsStore_c* poptionsStore );

   bool IsValid( void ) const;

private:

   TOS_OptionsStore_c* poptionsStore_;
};

#endif
