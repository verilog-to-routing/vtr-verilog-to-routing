//===========================================================================//
// Purpose : Declaration and inline definitions for a TNO_Route class.
//
//           Inline methods include:
//           - GetType
//           - GetInstPin, GetSwitchBox, GetChannel
//           - IsInstPin, IsSwitchBox, IsChannel
//           - IsValid
//
//===========================================================================//

#ifndef TNO_ROUTE_H
#define TNO_ROUTE_H

#include <stdio.h>

#include <string>
using namespace std;

#include "TC_NameIndex.h"

#include "TNO_Typedefs.h"
#include "TNO_InstPin.h"
#include "TNO_SwitchBox.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
class TNO_Route_c
{
public:

   TNO_Route_c( void );
   TNO_Route_c( const TNO_InstPin_c& instPin );
   TNO_Route_c( const TNO_SwitchBox_c& switchBox );
   TNO_Route_c( const TNO_Channel_t& channel );
   TNO_Route_c( const TNO_Route_c& route );
   ~TNO_Route_c( void );

   TNO_Route_c& operator=( const TNO_Route_c& route );
   bool operator==( const TNO_Route_c& route ) const;
   bool operator!=( const TNO_Route_c& route ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   TNO_RouteType_t GetType( void ) const;
   const TNO_InstPin_c& GetInstPin( void ) const;
   const TNO_SwitchBox_c& GetSwitchBox( void ) const;
   const TNO_Channel_t& GetChannel( void ) const;

   void Set( const TNO_InstPin_c& instPin );
   void Set( const TNO_SwitchBox_c& switchBox );
   void Set( const TNO_Channel_t& channel );

   void Clear( void );

   bool IsInstPin( void ) const;
   bool IsSwitchBox( void ) const;
   bool IsChannel( void ) const;

   bool IsValid( void ) const;

private:

   TNO_RouteType_t type_;      // Defines route segment type ;
                               // (ie. INST_PIN, SWITCH_BOX, or CHANNEL)
   TNO_InstPin_c   instPin_;
   TNO_SwitchBox_c switchBox_;
   TNO_Channel_t   channel_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
inline TNO_RouteType_t TNO_Route_c::GetType( 
      void ) const
{
   return( this->type_ );
}

//===========================================================================//
inline const TNO_InstPin_c& TNO_Route_c::GetInstPin( 
      void ) const
{
   return( this->instPin_ );
}

//===========================================================================//
inline const TNO_SwitchBox_c& TNO_Route_c::GetSwitchBox( 
      void ) const
{
   return( this->switchBox_ );
}

//===========================================================================//
inline const TNO_Channel_t& TNO_Route_c::GetChannel( 
      void ) const
{
   return( this->channel_ );
}

//===========================================================================//
inline bool TNO_Route_c::IsInstPin( 
      void ) const
{
   return( this->type_ == TNO_ROUTE_INST_PIN ? true : false );
}

//===========================================================================//
inline bool TNO_Route_c::IsSwitchBox( 
      void ) const
{
   return( this->type_ == TNO_ROUTE_SWITCH_BOX ? true : false );
}

//===========================================================================//
inline bool TNO_Route_c::IsChannel( 
      void ) const
{
   return( this->type_ == TNO_ROUTE_CHANNEL ? true : false );
}

//===========================================================================//
inline bool TNO_Route_c::IsValid( 
      void ) const
{
   return( this->type_ != TNO_ROUTE_UNDEFINED ? true : false );
}

#endif
