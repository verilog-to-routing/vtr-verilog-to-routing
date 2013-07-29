//===========================================================================//
// Purpose : Declaration and inline definitions for a TPO_PlaceRegions class.
//
//           Inline methods include:
//           - GetRegionList, GetNameList
//           - SetRegionList, SetNameList
//           - AddRegion, AddName
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

#ifndef TPO_PLACE_REGIONS_H
#define TPO_PLACE_REGIONS_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Name.h"

#include "TGO_Typedefs.h"
#include "TGO_Region.h"

#include "TPO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//
class TPO_PlaceRegions_c
{
public:

   TPO_PlaceRegions_c( void );
   TPO_PlaceRegions_c( const TGO_RegionList_t& regionList,
                       const TPO_NameList_t& nameList );
   TPO_PlaceRegions_c( const TPO_PlaceRegions_c& placeRegions );
   ~TPO_PlaceRegions_c( void );

   TPO_PlaceRegions_c& operator=( const TPO_PlaceRegions_c& placeRegions );
   bool operator==( const TPO_PlaceRegions_c& placeRegions ) const;
   bool operator!=( const TPO_PlaceRegions_c& placeRegions ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   const TGO_RegionList_t& GetRegionList( void ) const;
   const TPO_NameList_t& GetNameList( void ) const;

   void SetRegionList( const TGO_RegionList_t& regionList );
   void SetNameList( const TPO_NameList_t& nameList );

   void AddRegion( const TGO_Region_c& region );
   void AddName( const TC_Name_c& name );
   void AddName( const string& srName );
   void AddName( const char* pszName );

   bool IsValid( void ) const;

private:

   TGO_RegionList_t regionList_;
   TPO_NameList_t nameList_;

private:

   enum TPO_DefCapacity_e 
   { 
      TPO_REGION_LIST_DEF_CAPACITY = 1,
      TPO_NAME_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//
inline const TGO_RegionList_t& TPO_PlaceRegions_c::GetRegionList( 
      void ) const
{
   return( this->regionList_ );
}

//===========================================================================//
inline const TPO_NameList_t& TPO_PlaceRegions_c::GetNameList( 
      void ) const
{
   return( this->nameList_ );
}

//===========================================================================//
inline void TPO_PlaceRegions_c::SetRegionList(
      const TGO_RegionList_t& regionList )
{
   this->regionList_ = regionList;
}

//===========================================================================//
inline void TPO_PlaceRegions_c::SetNameList(
      const TPO_NameList_t& nameList )
{
   this->nameList_ = nameList;
}

//===========================================================================//
inline void TPO_PlaceRegions_c::AddRegion(
      const TGO_Region_c& region )
{
   this->regionList_.Add( region );
}

//===========================================================================//
inline void TPO_PlaceRegions_c::AddName(
      const TC_Name_c& name )
{
   this->nameList_.Add( name );
}

//===========================================================================//
inline void TPO_PlaceRegions_c::AddName(
      const string& srName )
{
   TC_Name_c name( srName );
   this->AddName( name );
}

//===========================================================================//
inline void TPO_PlaceRegions_c::AddName(
      const char* pszName )
{
   TC_Name_c name( pszName );
   this->AddName( name );
}

//===========================================================================//
inline bool TPO_PlaceRegions_c::IsValid( 
      void ) const
{
   return( this->regionList_.IsValid( ) &&
           this->nameList_.IsValid( ) ? 
           true : false );
}

#endif
