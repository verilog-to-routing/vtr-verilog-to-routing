//===========================================================================//
// Purpose : Method definitions for the TOS_OptionsStore class.
//
//           Public methods include:
//           - TOS_OptionsStore_c, ~TOS_OptionsStore_c
//           - Print
//           - Init
//           - Apply
//
//===========================================================================//

#include "TOS_OptionsStore.h"

//===========================================================================//
// Method         : TOS_OptionsStore_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_OptionsStore_c::TOS_OptionsStore_c( 
      void )
{
} 

//===========================================================================//
// Method         : ~TOS_OptionsStore_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_OptionsStore_c::~TOS_OptionsStore_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_OptionsStore_c::Print( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->Print( pfile, spaceLen );
}

//===========================================================================//
void TOS_OptionsStore_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   this->controlSwitches.Print( pfile, spaceLen );
   this->rulesSwitches.Print( pfile, spaceLen );
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_OptionsStore_c::Init( 
      void )
{
   this->controlSwitches.Init( );
   this->rulesSwitches.Init( );
}

//===========================================================================//
// Method         : Apply
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_OptionsStore_c::Apply( 
      void )
{
   this->controlSwitches.Apply( );
   this->rulesSwitches.Apply( );
}
