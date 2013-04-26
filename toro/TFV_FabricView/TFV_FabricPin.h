//===========================================================================//
// Purpose : Declaration and inline definitions for a TFV_FabricPin class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - SetType, SetSide, SetOffset, SetDelta, SetWidth, SetSlice
//           - SetChannelCount
//           - GetType, GetSide, GetOffset, GetDelta, GetWidth, SetSlice
//           - GetChannelCount
//           - AddConnection
//           - GetConnectionList
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

#ifndef TFV_FABRIC_PIN_H
#define TFV_FABRIC_PIN_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"
#include "TC_SideIndex.h"

#include "TFV_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
class TFV_FabricPin_c
{
public:

   TFV_FabricPin_c( void );
   TFV_FabricPin_c( const string& srName,
                    TC_TypeMode_t type = TC_TYPE_UNDEFINED,
                    TC_SideMode_t side = TC_SIDE_UNDEFINED,
                    int offset = 0,
                    double delta = 0.0,
                    double width = 0.0,
                    unsigned int slice = 0 );
   TFV_FabricPin_c( const char* pszName,
                    TC_TypeMode_t type = TC_TYPE_UNDEFINED,
                    TC_SideMode_t side = TC_SIDE_UNDEFINED,
                    int offset = 0,
                    double delta = 0.0,
                    double width = 0.0,
                    unsigned int slice = 0 );
   TFV_FabricPin_c( const string& srName,
                    unsigned int slice,
                    TC_SideMode_t side );
   TFV_FabricPin_c( const char* pszName,
                    unsigned int slice,
                    TC_SideMode_t side );
   TFV_FabricPin_c( const TFV_FabricPin_c& fabricPin );
   ~TFV_FabricPin_c( void );

   TFV_FabricPin_c& operator=( const TFV_FabricPin_c& fabricPin );
   bool operator<( const TFV_FabricPin_c& fabricPin ) const;
   bool operator==( const TFV_FabricPin_c& fabricPin ) const;
   bool operator!=( const TFV_FabricPin_c& fabricPin ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   void SetType( TC_TypeMode_t type );
   void SetSide( TC_SideMode_t side );
   void SetOffset( int offset );
   void SetDelta( double delta );
   void SetWidth( double width );
   void SetSlice( unsigned int slice );
   void SetChannelCount( unsigned int channelCount );

   TC_TypeMode_t GetType( void ) const;
   TC_SideMode_t GetSide( void ) const;
   int GetOffset( void ) const;
   double GetDelta( void ) const;
   double GetWidth( void ) const;
   unsigned int GetSlice( void ) const;
   unsigned int GetChannelCount( void ) const;

   void AddConnection( const TFV_Connection_t& connection );
   const TFV_ConnectionList_t& GetConnectionList( void ) const;

   bool IsValid( void ) const;

private:

   string        srName_;

   TC_TypeMode_t type_;
   TC_SideMode_t side_;
   int           offset_;
   double        delta_;
   double        width_;
   unsigned int  slice_;

   unsigned int  channelCount_;

   TFV_ConnectionList_t connectionList_;

private:

   enum TFV_DefCapacity_e 
   { 
      TFV_CONNECTION_LIST_DEF_CAPACITY = 16
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
inline void TFV_FabricPin_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TFV_FabricPin_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TFV_FabricPin_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TFV_FabricPin_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline void TFV_FabricPin_c::SetType( 
      TC_TypeMode_t type )
{
   this->type_ = type;
}

//===========================================================================//
inline void TFV_FabricPin_c::SetSide( 
      TC_SideMode_t side )
{
   this->side_ = side;
}

//===========================================================================//
inline void TFV_FabricPin_c::SetOffset( 
      int offset )
{
   this->offset_ = offset;
}

//===========================================================================//
inline void TFV_FabricPin_c::SetDelta( 
      double delta )
{
   this->delta_ = delta;
}

//===========================================================================//
inline void TFV_FabricPin_c::SetWidth( 
      double width )
{
   this->width_ = width;
}

//===========================================================================//
inline void TFV_FabricPin_c::SetSlice( 
      unsigned int slice )
{
   this->slice_ = slice;
}

//===========================================================================//
inline void TFV_FabricPin_c::SetChannelCount( 
      unsigned int channelCount )
{
   this->channelCount_ = channelCount;
}

//===========================================================================//
inline TC_TypeMode_t TFV_FabricPin_c::GetType( 
      void ) const
{
   return( this->type_ );
}

//===========================================================================//
inline TC_SideMode_t TFV_FabricPin_c::GetSide( 
      void ) const
{
   return( this->side_ );
}

//===========================================================================//
inline int TFV_FabricPin_c::GetOffset( 
      void ) const
{
   return( this->offset_ );
}

//===========================================================================//
inline double TFV_FabricPin_c::GetDelta( 
      void ) const
{
   return( this->delta_ );
}

//===========================================================================//
inline double TFV_FabricPin_c::GetWidth( 
      void ) const
{
   return( this->width_ );
}

//===========================================================================//
inline unsigned int TFV_FabricPin_c::GetSlice( 
      void ) const
{
   return( this->slice_ );
}

//===========================================================================//
inline unsigned int TFV_FabricPin_c::GetChannelCount( 
      void ) const
{
   return( this->channelCount_ );
}

//===========================================================================//
inline void TFV_FabricPin_c::AddConnection(
      const TFV_Connection_t& connection )
{
   this->connectionList_.Add( connection );
}

//===========================================================================//
inline const TFV_ConnectionList_t& TFV_FabricPin_c::GetConnectionList( 
      void ) const
{
   return( this->connectionList_ );
}

//===========================================================================//
inline bool TFV_FabricPin_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
