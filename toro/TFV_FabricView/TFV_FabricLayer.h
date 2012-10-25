//===========================================================================//
// Purpose : Declaration and inline definitions for a TFV_FabricLayer class.
//
//           Inline methods include:
//           - GetName, GetLayer, GetOrient
//           - GetRegion, GetPlane
//           - Clear
//           - IsWithin
//           - IsIntersecting
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

#ifndef TFV_FABRIC_LAYER_H
#define TFV_FABRIC_LAYER_H

#include <cstdio>
#include <string>
using namespace std;

#include "TGS_Typedefs.h"

#include "TFV_Typedefs.h"
#include "TFV_FabricPlane.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
class TFV_FabricLayer_c 
{
public:

   TFV_FabricLayer_c( void );
   TFV_FabricLayer_c( const string& srName,
                      TGS_Layer_t layer,
                      TGS_OrientMode_t orient,
                      const TGS_Region_c& region );
   TFV_FabricLayer_c( const char* pszName,
                      TGS_Layer_t layer,
                      TGS_OrientMode_t orient,
                      const TGS_Region_c& region );
   TFV_FabricLayer_c( const TFV_FabricLayer_c& fabricLayer );
   ~TFV_FabricLayer_c( void );

   TFV_FabricLayer_c& operator=( const TFV_FabricLayer_c& fabricLayer );
   bool operator==( const TFV_FabricLayer_c& fabricLayer ) const;
   bool operator!=( const TFV_FabricLayer_c& fabricLayer ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   const char* GetName( void ) const;
   TGS_Layer_t GetLayer( void ) const;
   TGS_OrientMode_t GetOrient( void ) const;
   const TGS_Region_c& GetRegion( void ) const;
   const TFV_FabricPlane_c* GetPlane( void ) const;

   bool Init( const string& srName,
              TGS_Layer_t layer,
              TGS_OrientMode_t orient,
              const TGS_Region_c& region );
   bool Init( const char* pszName,
              TGS_Layer_t layer,
              TGS_OrientMode_t orient,
              const TGS_Region_c& region );

   void Clear( void );

   bool Add( const TGS_Region_c& region,             
             const TFV_FabricData_c& fabricData,
             TFV_AddMode_t addMode = TFV_ADD_MERGE );
   bool Add( const TFV_FabricFigure_t& fabricFigure,             
             TFV_AddMode_t addMode = TFV_ADD_MERGE );

   bool Delete( const TGS_Region_c& region,
                TFV_DeleteMode_t deleteMode = TFV_DELETE_INTERSECT );
   bool Delete( const TGS_Region_c& region,
                const TFV_FabricData_c& fabricData,
                TFV_DeleteMode_t deleteMode = TFV_DELETE_INTERSECT );
   bool Delete( const TFV_FabricFigure_t& fabricFigure,
                TFV_DeleteMode_t deleteMode = TFV_DELETE_INTERSECT );

   bool Find( const TGS_Point_c& point,
              TFV_FabricFigure_t** ppfabricFigure = 0 ) const;
   bool Find( const TGS_Region_c& region,
              TFV_FabricFigure_t** ppfabricFigure = 0 ) const;
   bool Find( const TGS_Region_c& region,
              TFV_FindMode_t findMode,
              TFV_FigureMode_t figureMode,
              TFV_FabricFigure_t** ppfabricFigure = 0 ) const;
   bool Find( const TGS_Region_c& region,
              TFV_FabricData_c** ppfabricData = 0 ) const;
   bool Find( const TGS_Region_c& region,
              TFV_FindMode_t findMode,
              TFV_FigureMode_t figureMode,
              TFV_FabricData_c** ppfabricData = 0 ) const;
   bool Find( const TGS_Region_c& region,
              TGS_RegionList_t* pregionList ) const;
   bool Find( const TGS_Region_c& region,
              const TFV_FabricData_c& fabricData,
              TGS_RegionList_t* pregionList ) const;

   bool FindConnected( const TGS_Region_c& region,
                       TGS_RegionList_t* pregionList ) const;

   bool FindNearest( const TGS_Region_c& region,
                     TGS_Region_c* pfoundRegion,
                     double maxDistance = TC_FLT_MAX ) const;
   bool FindNearest( const TGS_Region_c& region,
                     TC_SideMode_t sideMode,
                     TGS_Region_c* pfoundRegion ) const;

   bool IsClear( const TGS_Point_c& point ) const;
   bool IsClearAny( const TGS_Region_c& region ) const;
   bool IsClearAny( const TGS_RegionList_t& regionList ) const;
   bool IsClearAll( const TGS_Region_c& region ) const;
   bool IsClearAll( const TGS_RegionList_t& regionList ) const;

   bool IsSolid( const TGS_Point_c& point,
                 TFV_FabricFigure_t** ppfabricFigure = 0 ) const;
   bool IsSolidAny( const TGS_Region_c& region,
                    TFV_FabricFigure_t** ppfabricFigure = 0 ) const;
   bool IsSolidAny( const TGS_RegionList_t& regionList,
                    TFV_FabricFigure_t** ppfabricFigure = 0 ) const;
   bool IsSolidAll( const TGS_Region_c& region,
                    TFV_FabricFigure_t** ppfabricFigure = 0 ) const;
   bool IsSolidAll( const TGS_RegionList_t& regionList,
                    TFV_FabricFigure_t** ppfabricFigure = 0 ) const;

   bool IsWithin( const TGS_Point_c& point ) const;
   bool IsWithin( const TGS_Region_c& region ) const;

   bool IsIntersecting( const TGS_Region_c& region ) const;

   bool IsValid( void ) const;

private:

   string            srName_;   // Define name asso. with this layer
   TGS_Layer_t       layer_;    // Define number asso. with this layer
   TGS_OrientMode_t  orient_;   // Define orientation asso. with layer
   TGS_Region_c      region_;   // Define region asso. with this layer

   TFV_FabricPlane_c plane_;    // Define tile plane's asso. with this layer
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
inline const char* TFV_FabricLayer_c::GetName( 
      void ) const
{
   return( this->srName_.length( ) ? this->srName_.data( ) : 0 );
}

//===========================================================================//
inline TGS_Layer_t TFV_FabricLayer_c::GetLayer( 
      void ) const
{
   return( this->layer_ );
}

//===========================================================================//
inline TGS_OrientMode_t TFV_FabricLayer_c::GetOrient( 
      void ) const
{
   return( this->orient_ );
}

//===========================================================================//
inline const TGS_Region_c& TFV_FabricLayer_c::GetRegion( 
      void ) const
{
   return( this->region_ );
}

//===========================================================================//
inline const TFV_FabricPlane_c* TFV_FabricLayer_c::GetPlane( 
      void ) const
{
   return( &this->plane_ );
}

//===========================================================================//
inline void TFV_FabricLayer_c::Clear(
      void )
{
   this->plane_.Clear( );
}

//===========================================================================//
inline bool TFV_FabricLayer_c::IsWithin( 
      const TGS_Point_c& point ) const
{
   return( this->region_.IsWithin( point ));
}

//===========================================================================//
inline bool TFV_FabricLayer_c::IsWithin( 
      const TGS_Region_c& region ) const
{
   return( this->region_.IsWithin( region ));
}

//===========================================================================//
inline bool TFV_FabricLayer_c::IsIntersecting( 
      const TGS_Region_c& region ) const
{
   return( this->region_.IsIntersecting( region ));
}

//===========================================================================//
inline bool TFV_FabricLayer_c::IsValid( 
      void ) const
{
   return(( this->srName_.length( )) &&
          ( this->layer_ != TGS_LAYER_UNDEFINED ) &&
          ( this->orient_ != TGS_ORIENT_UNDEFINED ) &&
          ( this->region_.IsValid( )) &&
          ( this->plane_.IsValid( )) ?
          true : false );
}

#endif
