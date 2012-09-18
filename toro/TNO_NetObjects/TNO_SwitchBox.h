//===========================================================================//
// Purpose : Declaration and inline definitions for a TNO_SwitchBox class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetInputPin, GetOutputPin
//           - SetInputPin, SetOutputPin
//           - IsValid
//
//===========================================================================//

#ifndef TNO_SWITCH_BOX_H
#define TNO_SWITCH_BOX_H

#include <stdio.h>

#include <string>
using namespace std;

#include "TC_SideIndex.h"

#include "TNO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
class TNO_SwitchBox_c
{
public:

   TNO_SwitchBox_c( void );
   TNO_SwitchBox_c( const string& srName );
   TNO_SwitchBox_c( const char* pszName );
   TNO_SwitchBox_c( const TNO_SwitchBox_c& switchBox );
   ~TNO_SwitchBox_c( void );

   TNO_SwitchBox_c& operator=( const TNO_SwitchBox_c& switchBox );
   bool operator<( const TNO_SwitchBox_c& switchBox ) const;
   bool operator==( const TNO_SwitchBox_c& switchBox ) const;
   bool operator!=( const TNO_SwitchBox_c& switchBox ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrSwitchBox ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   const TNO_SwitchBoxPin_t& GetInputPin( void ) const;
   const TNO_SwitchBoxPin_t& GetOutputPin( void ) const;

   void SetInputPin( const TNO_SwitchBoxPin_t& inputPin );
   void SetOutputPin( const TNO_SwitchBoxPin_t& outputPin );

   void Clear( void );

   bool IsValid( void ) const;

private:

   string srName_;
   TNO_SwitchBoxPin_t inputPin_;
   TNO_SwitchBoxPin_t outputPin_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
inline void TNO_SwitchBox_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TNO_SwitchBox_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TNO_SwitchBox_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TNO_SwitchBox_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const TNO_SwitchBoxPin_t& TNO_SwitchBox_c::GetInputPin( 
      void ) const
{
   return( this->inputPin_ );
}

//===========================================================================//
inline const TNO_SwitchBoxPin_t& TNO_SwitchBox_c::GetOutputPin( 
      void ) const
{
   return( this->outputPin_ );
}

//===========================================================================//
inline void TNO_SwitchBox_c::SetInputPin( 
      const TNO_SwitchBoxPin_t& inputPin )
{
   this->inputPin_ = inputPin;
}

//===========================================================================//
inline void TNO_SwitchBox_c::SetOutputPin( 
      const TNO_SwitchBoxPin_t& outputPin )
{
   this->outputPin_ = outputPin;
}

//===========================================================================//
inline bool TNO_SwitchBox_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
