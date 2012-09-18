//===========================================================================//
// Purpose : Method definitions for the TFS_FloorplanStore class.
//
//           Public methods include:
//           - TFS_FloorplanStore_c, ~TFS_FloorplanStore_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - IsValid
//
//===========================================================================//

#include "TFS_FloorplanStore.h"

//===========================================================================//
// Method         : TFS_FloorplanStore_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFS_FloorplanStore_c::TFS_FloorplanStore_c( 
      void )
{
} 

//===========================================================================//
TFS_FloorplanStore_c::TFS_FloorplanStore_c( 
      const TFS_FloorplanStore_c& floorplanStore )
      :
      architectureSpec( floorplanStore.architectureSpec ),
      fabricModel( floorplanStore.fabricModel ),
      circuitDesign( floorplanStore.circuitDesign )
{
} 

//===========================================================================//
// Method         : ~TFS_FloorplanStore_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFS_FloorplanStore_c::~TFS_FloorplanStore_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
TFS_FloorplanStore_c& TFS_FloorplanStore_c::operator=( 
      const TFS_FloorplanStore_c& floorplanStore )
{
   if( &floorplanStore != this )
   {
      this->architectureSpec = floorplanStore.architectureSpec;
      this->fabricModel = floorplanStore.fabricModel;
      this->circuitDesign = floorplanStore.circuitDesign;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFS_FloorplanStore_c::operator==( 
      const TFS_FloorplanStore_c& floorplanStore ) const
{
   return(( this->architectureSpec == floorplanStore.architectureSpec ) &&
          ( this->fabricModel == floorplanStore.fabricModel ) &&
          ( this->circuitDesign == floorplanStore.circuitDesign ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFS_FloorplanStore_c::operator!=( 
      const TFS_FloorplanStore_c& floorplanStore ) const
{
   return( !this->operator==( floorplanStore ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
void TFS_FloorplanStore_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   this->architectureSpec.Print( pfile, spaceLen );
   this->fabricModel.Print( pfile, spaceLen );
   this->circuitDesign.Print( pfile, spaceLen );
}

//===========================================================================//
// Method         : IsValid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
bool TFS_FloorplanStore_c::IsValid(
      void ) const
{
   return(( this->architectureSpec.IsValid( )) &&
          ( this->fabricModel.IsValid( )) &&
          ( this->circuitDesign.IsValid( )) ?
	  true : false );
}
