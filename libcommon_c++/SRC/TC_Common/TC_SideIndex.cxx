//===========================================================================//
// Purpose : Method definitions for the TC_SideIndex class.
//
//           Public methods include:
//           - TC_SideIndex_c, ~TC_SideIndex_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - Set
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TC_StringUtils.h"
#include "TC_SideIndex.h"

//===========================================================================//
// Method         : TC_SideIndex_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_SideIndex_c::TC_SideIndex_c( 
      void )
      :
      side_( TC_SIDE_UNDEFINED ),
      index_( UINT_MAX )
{
} 

//===========================================================================//
TC_SideIndex_c::TC_SideIndex_c( 
      TC_SideMode_t side,
      size_t        index )
      :
      side_( side ),
      index_( index )
{
} 

//===========================================================================//
TC_SideIndex_c::TC_SideIndex_c( 
      const TC_SideIndex_c& sideIndex )
      :
      side_( sideIndex.side_ ),
      index_( sideIndex.index_ )
{
} 

//===========================================================================//
// Method         : ~TC_SideIndex_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_SideIndex_c::~TC_SideIndex_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TC_SideIndex_c& TC_SideIndex_c::operator=( 
      const TC_SideIndex_c& sideIndex )
{
   if ( &sideIndex != this )
   {
      this->side_ = sideIndex.side_;
      this->index_ = sideIndex.index_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_SideIndex_c::operator==( 
      const TC_SideIndex_c& sideIndex ) const
{
   return(( this->side_ == sideIndex.side_ ) &&
          ( this->index_ == sideIndex.index_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TC_SideIndex_c::operator!=( 
      const TC_SideIndex_c& sideIndex ) const
{
   return( !this->operator==( sideIndex ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideIndex_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   string srSideIndex;
   this->ExtractString( &srSideIndex );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Write( pfile, spaceLen, "%s\n", TIO_SR_STR( srSideIndex ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideIndex_c::ExtractString( 
      string* psrSideIndex ) const
{
   if ( psrSideIndex )
   {
      if ( this->IsValid( ))
      {
	 string srSide;
         TC_ExtractStringSideMode( this->side_, &srSide );

         *psrSideIndex = srSide;

	 if ( this->index_ != SIZE_MAX )
	 {
   	    char szIndex[ TIO_FORMAT_STRING_LEN_VALUE ];
            sprintf( szIndex, "%lu", this->index_ );

            *psrSideIndex += " ";
            *psrSideIndex += szIndex;
         }
      }
      else
      {
         *psrSideIndex = "?";
      }
   }
} 

//===========================================================================//
// Method         : Set
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TC_SideIndex_c::Set( 
      TC_SideMode_t side,
      size_t        index )
{
   this->side_ = side;
   this->index_ = index;
} 
