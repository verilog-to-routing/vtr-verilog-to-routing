//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_NameType class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetType
//           - Set
//           - IsValid
//
//===========================================================================//

#ifndef TC_NAME_TYPE_H
#define TC_NAME_TYPE_H

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
class TC_NameType_c
{
public:

   TC_NameType_c( void );
   TC_NameType_c( const string& srName,
   	          TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   TC_NameType_c( const char* pszName,
	          TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   TC_NameType_c( const TC_NameType_c& nameType );
   ~TC_NameType_c( void );

   TC_NameType_c& operator=( const TC_NameType_c& nameType );
   bool operator<( const TC_NameType_c& nameType ) const;
   bool operator==( const TC_NameType_c& nameType ) const;
   bool operator!=( const TC_NameType_c& nameType ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrNameType ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   TC_TypeMode_t GetType( void ) const;

   void Set( const string& srName,
	     TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   void Set( const char* pszName,
	     TC_TypeMode_t type = TC_TYPE_UNDEFINED );

   bool IsValid( void ) const;

private:

   string        srName_; // Defines name string
   TC_TypeMode_t type_;   // Defines type mode
                          // (eg. input, output, signal, clock, reset, power)
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TC_NameType_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TC_NameType_c::SetName( 
      const char* pszName )
{
   this->srName_ = ( pszName ? pszName : "" );
}

//===========================================================================//
inline const char* TC_NameType_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TC_NameType_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline TC_TypeMode_t TC_NameType_c::GetType( 
      void ) const
{
   return( this->type_ );
}

//===========================================================================//
inline bool TC_NameType_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
