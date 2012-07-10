//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_SideIndex class.
//
//           Inline methods include:
//           - GetSide, GetIndex
//           - Set
//           - IsValid
//
//===========================================================================//

#ifndef TC_SIDE_INDEX_H
#define TC_SIDE_INDEX_H

#include <stdio.h>
#include <limits.h>

#include <string>
using namespace std;

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TC_SideIndex_c
{
public:

   TC_SideIndex_c( void );
   TC_SideIndex_c( TC_SideMode_t side,
 	           size_t index = SIZE_MAX );
   TC_SideIndex_c( const TC_SideIndex_c& sideIndex );
   ~TC_SideIndex_c( void );

   TC_SideIndex_c& operator=( const TC_SideIndex_c& sideIndex );
   bool operator==( const TC_SideIndex_c& sideIndex ) const;
   bool operator!=( const TC_SideIndex_c& sideIndex ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrSideIndex ) const;

   TC_SideMode_t GetSide( void ) const;
   size_t GetIndex( void ) const;

   void Set( TC_SideMode_t side,
             size_t index = SIZE_MAX );

   bool IsValid( void ) const;

private:

   TC_SideMode_t side_;
   size_t        index_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline TC_SideMode_t TC_SideIndex_c::GetSide( 
      void ) const
{
   return( this->side_ );
}

//===========================================================================//
inline size_t TC_SideIndex_c::GetIndex( 
      void ) const
{
   return( this->index_ );
}

//===========================================================================//
inline bool TC_SideIndex_c::IsValid( 
      void ) const
{
   return( this->side_ != TC_SIDE_UNDEFINED ? true : false );
}

#endif
