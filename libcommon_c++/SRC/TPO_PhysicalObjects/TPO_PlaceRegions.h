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
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
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
