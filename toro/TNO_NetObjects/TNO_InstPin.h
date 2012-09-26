//===========================================================================//
// Purpose : Declaration and inline definitions for a TNO_InstPin class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetInstName, GetPinName
//           - GetType
//           - SetType
//           - Clear
//           - IsValid
//
//===========================================================================//

#ifndef TNO_INST_PIN_H
#define TNO_INST_PIN_H

#include <cstdio>
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
class TNO_InstPin_c
{
public:

   TNO_InstPin_c( void );
   TNO_InstPin_c( const string& srInstName,
                  const string& srPinName,
	          TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   TNO_InstPin_c( const char* pszInstName,
                  const char* pszPinName,
                  TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   TNO_InstPin_c( const TNO_InstPin_c& instPin );
   ~TNO_InstPin_c( void );

   TNO_InstPin_c& operator=( const TNO_InstPin_c& instPin );
   bool operator<( const TNO_InstPin_c& instPin ) const;
   bool operator==( const TNO_InstPin_c& instPin ) const;
   bool operator!=( const TNO_InstPin_c& instPin ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrInstPin ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   const char* GetInstName( void ) const;
   const char* GetPinName( void ) const;
   TC_TypeMode_t GetType( void ) const;

   void SetType( TC_TypeMode_t type );

   void Set( const string& srInstName,
             const string& srPinName,
	     TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   void Set( const char* pszInstName,
             const char* pszPinName,
	     TC_TypeMode_t type = TC_TYPE_UNDEFINED );

   void Clear( void );

   bool IsValid( void ) const;

private:

   string srName_;        // Defines instance|pin name string
   string srInstName_;    // Defines instance name string
   string srPinName_;     // Defines pin name string
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
inline void TNO_InstPin_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TNO_InstPin_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TNO_InstPin_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TNO_InstPin_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TNO_InstPin_c::GetInstName( 
      void ) const
{
   return( TIO_SR_STR( this->srInstName_ ));
}

//===========================================================================//
inline const char* TNO_InstPin_c::GetPinName( 
      void ) const
{
   return( TIO_SR_STR( this->srPinName_ ));
}

//===========================================================================//
inline TC_TypeMode_t TNO_InstPin_c::GetType( 
      void ) const
{
   return( this->type_ );
}

//===========================================================================//
inline void TNO_InstPin_c::SetType(
      TC_TypeMode_t type )
{
   this->type_ = type;
}

//===========================================================================//
inline bool TNO_InstPin_c::IsValid( 
      void ) const
{
   return( this->srInstName_.length( ) &&
           this->srPinName_.length( ) ? 
           true : false );
}

#endif
