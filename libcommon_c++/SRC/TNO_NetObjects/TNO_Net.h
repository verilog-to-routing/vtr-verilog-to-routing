//===========================================================================//
// Purpose : Declaration and inline definitions for a TNO_Net class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - SetType, SetRoutable, SetStatus, SetIndex
//           - GetType, GetRoutable, GetStatus, GetIndex
//           - AddInstPin, AddGlobalRoute, AddRoute
//           - AddInstPinList, AddGlobalRouteList, AddRouteList
//           - GetInstPinList, GetGlobalRouteList, GetRouteList
//           - HasInstPinList, HasGlobalRouteList, HasRouteList
//           - ClearInstPinList, ClearGlobalRouteList, ClearRouteList
//           - SetInstPinListCapacity
//           - IsRoutable
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
//---------------------------------------------------------------------------//

#ifndef TNO_NET_H
#define TNO_NET_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"
#include "TC_StringUtils.h"
#include "TC_NameType.h"
#include "TC_NameLength.h"

#include "TNO_Typedefs.h"
#include "TNO_InstPin.h"
#include "TNO_Node.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
class TNO_Net_c 
{
public:

   TNO_Net_c( void );
   TNO_Net_c( const string& srName, 
              TC_TypeMode_t type = TC_TYPE_UNDEFINED,
              bool routable = true,
              TNO_StatusMode_t status = TNO_STATUS_UNDEFINED );
   TNO_Net_c( const char* pszName, 
              TC_TypeMode_t type = TC_TYPE_UNDEFINED,
              bool routable = true,
              TNO_StatusMode_t status = TNO_STATUS_UNDEFINED );
   TNO_Net_c( const TNO_Net_c& net );
   ~TNO_Net_c( void );

   TNO_Net_c& operator=( const TNO_Net_c& net );
   bool operator<( const TNO_Net_c& net ) const;
   bool operator==( const TNO_Net_c& net ) const;
   bool operator!=( const TNO_Net_c& net ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetType( TC_TypeMode_t type );
   void SetRoutable( bool routable );
   void SetStatus( TNO_StatusMode_t status );
   void SetIndex( unsigned int index );

   TC_TypeMode_t GetType( void ) const;
   bool GetRoutable( void ) const;
   TNO_StatusMode_t GetStatus( void ) const;
   unsigned int GetIndex( void ) const;

   void AddInstPin( const TNO_InstPin_c& instPin );
   void AddInstPin( const string& srInstName, 
                    const string& srPinName,
                    TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   void AddInstPin( const char* pszInstName, 
                    const char* pszPinName,
                    TC_TypeMode_t type = TC_TYPE_UNDEFINED );
   void AddGlobalRoute( const TNO_GlobalRoute_t& globalRoute );
   void AddRoute( const TNO_Route_t& route );

   void AddInstPinList( const TNO_InstPinList_t& instPinList );
   void AddGlobalRouteList( const TNO_GlobalRouteList_t& globalRouteList );
   void AddRouteList( const TNO_RouteList_t& routeList );

   const TNO_InstPinList_t& GetInstPinList( void ) const;
   const TNO_GlobalRouteList_t& GetGlobalRouteList( void ) const;
   const TNO_RouteList_t& GetRouteList( void ) const;

   bool HasInstPinList( void ) const;
   bool HasGlobalRouteList( void ) const;
   bool HasRouteList( void ) const;

   void ClearInstPinList( void );
   void ClearGlobalRouteList( void );
   void ClearRouteList( void );

   size_t FindInstPinCount( TC_TypeMode_t type = TC_TYPE_UNDEFINED ) const;

   void SetInstPinListCapacity( size_t capacity );

   bool IsRoutable( void ) const;

   bool IsValid( void ) const;

private:

   string srName_;              // Defines net name

   unsigned int type_     : 4;  // Defines type (eg. SIGNAL|CLOCK|POWER)
                                // (see the TC_TypeMode_t enumeration)
   unsigned int routable_ : 1;  // True => net is routable
                                // False => otherwise
   unsigned int status_   : 3;  // Defines status (eg. OPEN|GROUTED|ROUTED)
                                // (see the TNO_StatusMode_t enumeration)
   unsigned int index_    : 16; // Defines index (optional)

   TNO_InstPinList_t instPinList_;    
                                // List of instance/pins asso. with this net
   TNO_GlobalRouteList_t globalRouteList_; 
                                // List of global route channels (optional)
                                // (assumes net status == GROUTED)
   TNO_RouteList_t routeList_;  // List of detailed route nodes (optional)
                                // (assumes net status == ROUTED)
private:

   enum TNO_DefCapacity_e 
   { 
      TNO_INST_PIN_LIST_DEF_CAPACITY = 2,
      TNO_GLOBAL_ROUTE_LIST_DEF_CAPACITY = 16,
      TNO_ROUTE_LIST_DEF_CAPACITY = 16
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
inline void TNO_Net_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TNO_Net_c::SetName(
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TNO_Net_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TNO_Net_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline void TNO_Net_c::SetType(
      TC_TypeMode_t type )
{
   this->type_ = static_cast< unsigned int >( type );
}

//===========================================================================//
inline void TNO_Net_c::SetRoutable(
      bool routable )
{
   this->routable_ = static_cast< unsigned int >( routable );
}

//===========================================================================//
inline void TNO_Net_c::SetStatus(
      TNO_StatusMode_t status )
{
   this->status_ = static_cast< unsigned int >( status );
}

//===========================================================================//
inline void TNO_Net_c::SetIndex(
      unsigned int index )
{
   this->index_ = static_cast< unsigned int >( index );
}

//===========================================================================//
inline TC_TypeMode_t TNO_Net_c::GetType( 
      void ) const
{
   return( static_cast< TC_TypeMode_t >( this->type_ ));
}

//===========================================================================//
inline bool TNO_Net_c::GetRoutable(
      void ) const
{
   return( static_cast< bool >( this->routable_ ));
}

//===========================================================================//
inline TNO_StatusMode_t TNO_Net_c::GetStatus( 
      void ) const
{
   return( static_cast< TNO_StatusMode_t >( this->status_ ));
}

//===========================================================================//
inline unsigned int TNO_Net_c::GetIndex(
      void ) const
{
   return( static_cast< unsigned int >( this->index_ ));
}

//===========================================================================//
inline void TNO_Net_c::AddInstPinList( 
      const TNO_InstPinList_t& instPinList )
{
   this->instPinList_.Add( instPinList );
}

//===========================================================================//
inline void TNO_Net_c::AddGlobalRouteList( 
      const TNO_GlobalRouteList_t& globalRouteList )
{
   this->globalRouteList_.Add( globalRouteList );
}

//===========================================================================//
inline void TNO_Net_c::AddRouteList( 
      const TNO_RouteList_t& routeList )
{
   this->routeList_.Add( routeList );
}

//===========================================================================//
inline const TNO_InstPinList_t& TNO_Net_c::GetInstPinList( 
      void ) const
{
   return( this->instPinList_ );
}

//===========================================================================//
inline const TNO_GlobalRouteList_t& TNO_Net_c::GetGlobalRouteList( 
      void ) const
{
   return( this->globalRouteList_ );
}

//===========================================================================//
inline const TNO_RouteList_t& TNO_Net_c::GetRouteList( 
      void ) const
{
   return( this->routeList_ );
}

//===========================================================================//
inline bool TNO_Net_c::HasInstPinList(
      void ) const
{
   return( this->instPinList_.GetLength( ) > 0 ? true : false );
}

//===========================================================================//
inline bool TNO_Net_c::HasGlobalRouteList(
      void ) const
{
   return( this->globalRouteList_.GetLength( ) > 0 ? true : false );
}

//===========================================================================//
inline bool TNO_Net_c::HasRouteList(
      void ) const
{
   return( this->routeList_.GetLength( ) > 0 ? true : false );
}

//===========================================================================//
inline void TNO_Net_c::ClearInstPinList( 
      void )
{
   this->instPinList_.Clear( );
}

//===========================================================================//
inline void TNO_Net_c::ClearGlobalRouteList( 
      void )
{
   this->globalRouteList_.Clear( );
}

//===========================================================================//
inline void TNO_Net_c::ClearRouteList( 
      void )
{
   this->routeList_.Clear( );
}

//===========================================================================//
inline void TNO_Net_c::SetInstPinListCapacity(
      size_t capacity )
{
   this->instPinList_.SetCapacity( capacity );
}

//===========================================================================//
inline bool TNO_Net_c::IsRoutable( 
      void ) const
{
   return( this->GetRoutable( ) ? true : false );
}

//===========================================================================//
inline bool TNO_Net_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif 
