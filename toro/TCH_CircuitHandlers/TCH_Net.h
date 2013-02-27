//===========================================================================//
// Purpose : Declaration and inline definitions for the TCH_Net class.
//
//           Inline methods include:
//           - SetName
//           - SetVPR_NetIndex
//           - SetLegal
//           - GetName
//           - GetVPR_NetIndex
//           - GetRoutePathList
//           - AddRoutePath
//           - IsMemberRoutePath
//           - IsLegal
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TCH_NET_H
#define TCH_NET_H

#include <cstdio>
using namespace std;

#include "TCH_Typedefs.h"
#include "TCH_RoutePath.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
class TCH_Net_c
{
public:

   TCH_Net_c( void );
   TCH_Net_c( const string& srName,
              int vpr_netIndex = -1 );
   TCH_Net_c( const char* pszName,
              int vpr_netIndex = -1 );
   TCH_Net_c( const TCH_Net_c& net );
   ~TCH_Net_c( void );

   TCH_Net_c& operator=( const TCH_Net_c& net );
   bool operator==( const TCH_Net_c& net ) const;
   bool operator!=( const TCH_Net_c& net ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   void SetVPR_NetIndex( int vpr_netIndex );
   void SetLegal( bool isLegal );

   const char* GetName( void ) const;
   int GetVPR_NetIndex( void ) const;

   const TCH_RoutePathList_t* GetRoutePathList( void ) const;
   void AddRoutePath( const TCH_RoutePath_c& routePath );
   bool IsMemberRoutePath( const TCH_RoutePath_c& routePath ) const;

   void Reset( void );
   void ResetRoutePathList( void );
   void ResetRoutePathNodeList( void );

   bool IsLegal( void ) const;
   bool IsValid( void ) const;

private:

   string srName_;  // Defines a Toro/VPR net name

   class TCH_VPR_c
   {
   public:

      int netIndex; // Defines index into VPR's global net list structure
   } vpr_;

   TCH_RoutePathList_t routePathList_;
                    // Sorted list of user-defined route paths 
                    // (ie. source->segment->sink paths)
   bool isLegal_;   // TRUE => net has been validated w.r.t VPR's net list
                    // (insures that net actually exists prior to VPR routing)
private:

   enum TCH_DefCapacity_e 
   { 
      TCH_ROUTE_PATH_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/13 jeffr : Original
//===========================================================================//
inline void TCH_Net_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TCH_Net_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline void TCH_Net_c::SetVPR_NetIndex(
      int vpr_netIndex )
{
   this->vpr_.netIndex = vpr_netIndex;
}

//===========================================================================//
inline void TCH_Net_c::SetLegal( 
      bool isLegal )
{
   this->isLegal_ = isLegal;
} 

//===========================================================================//
inline const char* TCH_Net_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline int TCH_Net_c::GetVPR_NetIndex( 
      void ) const
{
   return( this->vpr_.netIndex );
}

//===========================================================================//
inline const TCH_RoutePathList_t* TCH_Net_c::GetRoutePathList( 
      void ) const
{
   return( &this->routePathList_ );
} 

//===========================================================================//
inline void TCH_Net_c::AddRoutePath( 
      const TCH_RoutePath_c& routePath )
{
   this->routePathList_.Add( routePath );
} 

//===========================================================================//
inline bool TCH_Net_c::IsMemberRoutePath( 
      const TCH_RoutePath_c& routePath ) const
{
   return( this->routePathList_.IsMember( routePath ));
}

//===========================================================================//
inline bool TCH_Net_c::IsLegal( 
      void ) const
{
   return( this->isLegal_ ? true : false );
}

//===========================================================================//
inline bool TCH_Net_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
