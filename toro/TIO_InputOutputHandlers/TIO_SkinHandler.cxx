//===========================================================================//
// Purpose : Method definitions for the TIO_SkinHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - Set
//
//           Protected methods include:
//           - TIO_SkinHandler_c, ~TIO_SkinHandler_c
// 
//===========================================================================//

#if defined( SUN8 ) || defined( SUN10 )
   #include <time.h>
#endif

#include "TC_Typedefs.h"

#include "TIO_StringText.h"

#include "TIO_SkinHandler.h"

// Initialize the user execute "singleton" class, as needed
TIO_SkinHandler_c* TIO_SkinHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TIO_SkinHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_SkinHandler_c::TIO_SkinHandler_c( 
      void )
      :
      skinMode_( TIO_SkinHandler_c::TIO_SKIN_UNDEFINED )
{
   pinstance_ = 0;
}

//===========================================================================//
// Method         : TIO_SkinHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_SkinHandler_c::~TIO_SkinHandler_c( 
      void )
{
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_SkinHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TIO_SkinHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_SkinHandler_c::DeleteInstance( 
      void )
{
   if( pinstance_ )
   {
      delete pinstance_;
      pinstance_ = 0;
   }
}

//===========================================================================//
// Method         : GetInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_SkinHandler_c& TIO_SkinHandler_c::GetInstance(
      void )
{
   if( !pinstance_ )
   {
      NewInstance( );
   }
   return( *pinstance_ );
}

//===========================================================================//
// Method         : HasInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_SkinHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_SkinHandler_c::Set(
      int skinMode )
{
   this->skinMode_ = skinMode;

   switch( this->skinMode_ )
   {
   case TIO_SkinHandler_c::TIO_SKIN_VPR:

      this->srProgramName_ = TIO_SZ_VPR_PROGRAM_NAME;
      this->srProgramTitle_ = TIO_SZ_VPR_PROGRAM_TITLE;
      this->srBinaryName_ = TIO_SZ_VPR_BINARY_NAME;
      this->srSourceName_ = TIO_SZ_VPR_SOURCE_NAME;
      this->srCopyright_ = TIO_SZ_VPR_COPYRIGHT;
      break;

   case TIO_SkinHandler_c::TIO_SKIN_TORO:

      this->srProgramName_ = TIO_SZ_TORO_PROGRAM_NAME;
      this->srProgramTitle_ = TIO_SZ_TORO_PROGRAM_TITLE;
      this->srBinaryName_ = TIO_SZ_TORO_BINARY_NAME;
      this->srSourceName_ = TIO_SZ_TORO_SOURCE_NAME;
      this->srCopyright_ = TIO_SZ_TORO_COPYRIGHT;
      break;
   }
}
