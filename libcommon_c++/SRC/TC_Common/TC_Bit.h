//===========================================================================//
// Purpose : Declaration and inline definitions for a (regexp) TC_Bit class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TC_BIT_H
#define TC_BIT_H

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
class TC_Bit_c
{
public:

   TC_Bit_c( TC_BitMode_t value = TC_BIT_UNDEFINED );
   TC_Bit_c( char value );
   TC_Bit_c( const TC_Bit_c& bit );
   ~TC_Bit_c( void );

   TC_Bit_c& operator=( const TC_Bit_c& value );
   bool operator==( const TC_Bit_c& value ) const;
   bool operator!=( const TC_Bit_c& value ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrValue ) const;

   TC_BitMode_t GetValue( void ) const;

   void SetValue( TC_BitMode_t value );
   void SetValue( char value );

   bool IsValid( void ) const;

private:

   char value_; // States include 'true', 'false', 'don't care', and 'undefined'
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TC_Bit_c::IsValid( 
      void ) const
{
   return( this->value_ != TC_BIT_UNDEFINED ? true : false );
}

#endif 
