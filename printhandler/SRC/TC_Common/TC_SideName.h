//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_SideName class.
//
//           Inline methods include:
//           - GetSide, GetName
//           - Set
//           - IsValid
//
//===========================================================================//

#ifndef TC_SIDE_NAME_H
#define TC_SIDE_NAME_H

#include <stdio.h>

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
class TC_SideName_c
{
public:

   TC_SideName_c( void );
   TC_SideName_c( TC_SideMode_t side,
 	          const string& srName );
   TC_SideName_c( TC_SideMode_t side,
 	          const char* pszName );
   TC_SideName_c( const TC_SideName_c& sideName );
   ~TC_SideName_c( void );

   TC_SideName_c& operator=( const TC_SideName_c& sideName );
   bool operator==( const TC_SideName_c& sideName ) const;
   bool operator!=( const TC_SideName_c& sideName ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrSideName ) const;

   TC_SideMode_t GetSide( void ) const;
   const char* GetName( void ) const;

   void Set( TC_SideMode_t side,
             const string& srName );
   void Set( TC_SideMode_t side,
             const char* pszName );

   bool IsValid( void ) const;

private:

   TC_SideMode_t side_;
   string        srName_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline TC_SideMode_t TC_SideName_c::GetSide( 
      void ) const
{
   return( this->side_ );
}

//===========================================================================//
inline const char* TC_SideName_c::GetName( 
      void ) const
{
   return( this->srName_.data( ));
}

//===========================================================================//
inline bool TC_SideName_c::IsValid( 
      void ) const
{
   return( this->side_ != TC_SIDE_UNDEFINED ? true : false );
}

#endif
