//===========================================================================//
// Purpose : Declaration and inline definitions for a TFM_Channel class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TFM_CHANNEL_H
#define TFM_CHANNEL_H

#include <cstdio>
#include <string>
using namespace std;

#include "TGS_Typedefs.h"
#include "TGS_Region.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
class TFM_Channel_c
{
public:

   TFM_Channel_c( void );
   TFM_Channel_c( const string& srName,
                  const TGS_Region_c& region,
                  TGS_OrientMode_t orient,
                  unsigned int count );
   TFM_Channel_c( const char* pszName,
                  const TGS_Region_c& region,
                  TGS_OrientMode_t orient,
                  unsigned int count );
   TFM_Channel_c( const TFM_Channel_c& channel );
   ~TFM_Channel_c( void );

   TFM_Channel_c& operator=( const TFM_Channel_c& channel );
   bool operator==( const TFM_Channel_c& channel ) const;
   bool operator!=( const TFM_Channel_c& channel ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   bool IsValid( void ) const;

public:

   string srName;       // Unique identifier

   TGS_Region_c region; // Specifies routing channel region within a floorplan
   TGS_OrientMode_t orient;
                        // Defines channel orientation (ie. horizontal | vertical)
   unsigned int count;  // Defines #of routing segments available within channel
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/20/12 jeffr : Original
//===========================================================================//
inline void TFM_Channel_c::SetName( 
      const string& srName_ )
{
   this->srName = srName_;
}

//===========================================================================//
inline void TFM_Channel_c::SetName( 
      const char* pszName )
{
   this->srName = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TFM_Channel_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName ));
}

//===========================================================================//
inline const char* TFM_Channel_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName ));
}

//===========================================================================//
inline bool TFM_Channel_c::IsValid( 
      void ) const
{
   return( this->srName.length( ) ? true : false );
}

#endif
