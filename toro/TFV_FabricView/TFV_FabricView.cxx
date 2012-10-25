//===========================================================================//
// Purpose : Method definitions for a TFV_FabricView class.
//
//           Public methods include:
//           - TFV_FabricView_c, ~TFV_FabricView_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - Init
//           - Clear
//           - Add
//           - Delete
//           - Find
//           - FindConnected
//           - FindNearest
//           - FindFabricLayer
//           - FindFabricPlane
//           - FindFabricBox
//           - ApplyIntersect
//           - IsClear, IsClearAny, IsClearAll
//           - IsSolid, IsSolidAny, IsSolidAll
//           - IsWithin
//           - IsIntersecting
//
//           Private methods include:
//           - Allocate_, Deallocate_
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

#include "TIO_PrintHandler.h"

#include "TFV_Typedefs.h"
#include "TFV_StringUtils.h"
#include "TFV_FabricView.h"

//===========================================================================//
// Method         : TFV_FabricView_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricView_c::TFV_FabricView_c( 
      void )
      :
      pfabricLayerList_( 0 )
{
}

//===========================================================================//
TFV_FabricView_c::TFV_FabricView_c( 
      const string&       srName,
      const TGS_Region_c& region )
      :
      pfabricLayerList_( 0 )
{
   this->Init( srName, region );
}

//===========================================================================//
TFV_FabricView_c::TFV_FabricView_c( 
      const char*         pszName,
      const TGS_Region_c& region )
      :
      pfabricLayerList_( 0 )
{
   this->Init( pszName, region );
}

//===========================================================================//
TFV_FabricView_c::TFV_FabricView_c( 
      const TGS_Region_c& region )
      :
      pfabricLayerList_( 0 )
{
   this->Init( region );
}

//===========================================================================//
TFV_FabricView_c::TFV_FabricView_c( 
      const TFV_FabricView_c& fabricView )
      :
      pfabricLayerList_( 0 )
{
   this->Init( fabricView );
}

//===========================================================================//
// Method         : ~TFV_FabricView_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricView_c::~TFV_FabricView_c( 
      void )
{
   this->Clear( );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricView_c& TFV_FabricView_c::operator=( 
      const TFV_FabricView_c& fabricView )
{
   if( &fabricView != this )
   {
      this->Init( fabricView );
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::operator==( 
      const TFV_FabricView_c& fabricView ) const
{
   return(( this->srName_ == fabricView.srName_ ) &&
          ( this->region_ == fabricView.region_ ) &&
          ( this->layerRange_ == fabricView.layerRange_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::operator!=( 
      const TFV_FabricView_c& fabricView ) const
{
   return(( !this->operator==( fabricView )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TFV_FabricView_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srRegion;
   this->region_.ExtractString( &srRegion );

   printHandler.Write( pfile, spaceLen, "\"%s\" (%s) [%d-%d]",
                                        TIO_SR_STR( this->srName_ ),
                                        TIO_SR_STR( srRegion ),
                                        this->layerRange_.i, this->layerRange_.j );
   if(( this->pfabricLayerList_ ) &&
      ( this->pfabricLayerList_->IsValid( )))
   {
      this->pfabricLayerList_->Print( pfile, spaceLen + 3 );
   }
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::Init(
      const string&       srName,
      const TGS_Region_c& region )
{
   this->Clear( );

   this->srName_ = srName;
   this->region_ = region;
   this->layerRange_.Set( TFV_LAYER_MIN, TFV_LAYER_MAX );
   this->pfabricLayerList_ = this->Allocate_( this->region_, this->layerRange_ );

   return( this->pfabricLayerList_ ? true : false );
}

//===========================================================================//
bool TFV_FabricView_c::Init(
      const char*         pszName,
      const TGS_Region_c& region )
{
   this->Clear( );

   this->srName_ = TIO_PSZ_STR( pszName );
   this->region_ = region;
   this->layerRange_.Set( TFV_LAYER_MIN, TFV_LAYER_MAX );
   this->pfabricLayerList_ = this->Allocate_( this->region_, this->layerRange_ );

   return( this->pfabricLayerList_ ? true : false );
}

//===========================================================================//
bool TFV_FabricView_c::Init(
      const TGS_Region_c& region )
{
   this->Clear( );

   this->region_ = region;
   this->layerRange_.Set( TFV_LAYER_MIN, TFV_LAYER_MAX );
   this->pfabricLayerList_ = this->Allocate_( this->region_, this->layerRange_ );

   return( this->pfabricLayerList_ ? true : false );
}

//===========================================================================//
bool TFV_FabricView_c::Init(
      const TFV_FabricView_c& fabricView )
{
   bool ok = true;

   this->Clear( );

   if( fabricView.IsValid( ))
   {
      this->Init( fabricView.GetName( ), fabricView.GetRegion( ));
      ok = this->Add( fabricView );
   }
   return( ok );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TFV_FabricView_c::Clear( 
      void )
{
   this->srName_ = "";

   this->region_.Reset( );
   this->layerRange_.Reset( );

   this->Deallocate_( this->pfabricLayerList_ );
   this->pfabricLayerList_ = 0;
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::Add( 
            TGS_Layer_t   layer,
      const TGS_Region_c& region,
            TFV_AddMode_t addMode )
{
   TFV_FabricData_c fabricData;
   TFV_FabricFigure_t fabricFigure( TFV_FIGURE_SOLID, region, fabricData );

   return( this->Add( layer, fabricFigure, addMode ));
}

//===========================================================================//
bool TFV_FabricView_c::Add( 
      const TGS_Rect_c&   rect,
            TFV_AddMode_t addMode )
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->Add( layer, region, addMode ));
}

//===========================================================================//
bool TFV_FabricView_c::Add( 
            TGS_Layer_t       layer,
      const TGS_Region_c&     region,
      const TFV_FabricData_c& fabricData,
            TFV_AddMode_t     addMode )
{
   TFV_FabricFigure_t fabricFigure( TFV_FIGURE_SOLID, region, fabricData );

   return( this->Add( layer, fabricFigure, addMode ));
}

//===========================================================================//
bool TFV_FabricView_c::Add( 
      const TGS_Rect_c&       rect,
      const TFV_FabricData_c& fabricData,
            TFV_AddMode_t     addMode )
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->Add( layer, region, fabricData, addMode ));
}

//===========================================================================//
bool TFV_FabricView_c::Add( 
            TGS_Layer_t         layer,
      const TFV_FabricFigure_t& fabricFigure,
            TFV_AddMode_t       addMode )
{
   bool ok = true;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         ok = pfabricLayer->Add( fabricFigure, addMode );
      }
      else
      {
         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         ok = printHandler.Internal( "TFV_FabricView_c::Add", 
                                     "Fabric view's tile plane not found for layer (%d)!\n",
                                     layer );
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricView_c::Add", 
                                  "Fabric view's tile plane list is not initialized!\n" );
   }
   return( ok );
}

//===========================================================================//
bool TFV_FabricView_c::Add(  
      const TFV_FabricView_c& fabricView )
{
   bool ok = true;

   const TGS_LayerRange_t& thisLayerRange = this->GetLayerRange( );
   const TGS_Region_c& thisRegion = this->GetRegion( );

   const TGS_LayerRange_t& fabricViewLayerRange = fabricView.GetLayerRange( );
   for( TGS_Layer_t layer = fabricViewLayerRange.i; layer <= fabricViewLayerRange.j; ++layer )
   {
      if( layer < thisLayerRange.i )
         continue;

      if( layer > thisLayerRange.j )
         break;

      const TFV_FabricPlane_c* pfabricPlane = fabricView.FindFabricPlane( layer );
      if( !pfabricPlane )
         continue;

      TFV_FabricPlaneIter_t fabricPlaneIter( *pfabricPlane );
      TFV_FabricFigure_t* pfabricFigure = 0;
      while( fabricPlaneIter.Next( &pfabricFigure, TFV_FIGURE_SOLID ))
      {
         TGS_Region_c region = pfabricFigure->GetRegion( );
         const TFV_FabricData_c& fabricData = *pfabricFigure->GetData( );

         if( thisRegion.IsWithin( region ))
         {
            ok = this->Add( layer, region, fabricData );
         }
         else if( thisRegion.IsIntersecting( region ))
         {
            region.ApplyIntersect( thisRegion );
            ok = this->Add( layer, region, fabricData );
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : Delete
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::Delete( 
            TGS_Layer_t      layer,
      const TGS_Region_c&    region,
            TFV_DeleteMode_t deleteMode )
{
   TFV_FabricFigure_t fabricFigure( TFV_FIGURE_UNDEFINED, region );

   return( this->Delete( layer, fabricFigure, deleteMode ));
}

//===========================================================================//
bool TFV_FabricView_c::Delete( 
      const TGS_Rect_c&      rect,
            TFV_DeleteMode_t deleteMode )
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->Delete( layer, region, deleteMode ));
}

//===========================================================================//
bool TFV_FabricView_c::Delete( 
            TGS_Layer_t       layer,
      const TGS_Region_c&     region,
      const TFV_FabricData_c& fabricData,
            TFV_DeleteMode_t  deleteMode )
{
   TFV_FabricFigure_t fabricFigure( TFV_FIGURE_UNDEFINED, region, fabricData );

   return( this->Delete( layer, fabricFigure, deleteMode ));
}

//===========================================================================//
bool TFV_FabricView_c::Delete( 
      const TGS_Rect_c&       rect,
      const TFV_FabricData_c& fabricData,
            TFV_DeleteMode_t  deleteMode )
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->Delete( layer, region, fabricData, deleteMode ));
}

//===========================================================================//
bool TFV_FabricView_c::Delete( 
            TGS_Layer_t         layer,
      const TFV_FabricFigure_t& fabricFigure,
            TFV_DeleteMode_t    deleteMode )
{
   bool ok = true;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         ok = pfabricLayer->Delete( fabricFigure, deleteMode );
      }
      else
      {
         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         ok = printHandler.Internal( "TFV_FabricView_c::Delete", 
                                     "Fabric view's tile plane not found for layer (%d)!\n",
                                     layer );
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricView_c::Delete", 
                                  "Fabric view's tile plane list is not initialized!\n" );
   }
   return( ok );
}

//===========================================================================//
// Method         : Find
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::Find( 
            TGS_Layer_t          layer,
      const TGS_Region_c&        region,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   TFV_FindMode_t findMode = TFV_FIND_EXACT;
   TFV_FigureMode_t figureMode = TFV_FIGURE_ANY;

   return( this->Find( layer, region, findMode, figureMode, ppfabricFigure ));
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
      const TGS_Rect_c&          rect,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->Find( layer, region, ppfabricFigure ));
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
            TGS_Layer_t          layer,
      const TGS_Point_c&         point,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool ok = true;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         ok = pfabricLayer->Find( point, ppfabricFigure );
      }
      else
      {
         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         ok = printHandler.Internal( "TFV_FabricView_c::Find", 
                                     "Fabric view's tile plane not found for layer (%d)!\n",
                                     layer );
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricView_c::Find", 
                                  "Fabric view's tile plane list is not initialized!\n" );
   }
   return( ok );
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
      const TGS_Point_c&         point,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   TGS_Layer_t layer = point.z;

   return( this->Find( layer, point, ppfabricFigure ));
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
            TGS_Layer_t          layer,
      const TGS_Region_c&        region,
            TFV_FindMode_t       findMode,
            TFV_FigureMode_t     figureMode,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool ok = true;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         ok = pfabricLayer->Find( region, findMode, figureMode, ppfabricFigure );
      }
      else
      {
         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         ok = printHandler.Internal( "TFV_FabricView_c::Find", 
                                     "Fabric view's tile plane not found for layer (%d)!\n",
                                     layer );
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricView_c::Find", 
                                  "Fabric view's tile plane list is not initialized!\n" );
   }
   return( ok );
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
      const TGS_Rect_c&          rect,
            TFV_FindMode_t       findMode,
            TFV_FigureMode_t     figureMode,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->Find( layer, region, findMode, figureMode, ppfabricFigure ));
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
            TGS_Layer_t        layer,
      const TGS_Region_c&      region,
            TFV_FabricData_c** ppfabricData ) const
{
   TFV_FindMode_t findMode = TFV_FIND_EXACT;
   TFV_FigureMode_t figureMode = TFV_FIGURE_ANY;

   return( this->Find( layer, region, findMode, figureMode, ppfabricData ));
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
      const TGS_Rect_c&        rect,
            TFV_FabricData_c** ppfabricData ) const
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->Find( layer, region, ppfabricData ));
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
            TGS_Layer_t        layer,
      const TGS_Point_c&       point,
            TFV_FabricData_c** ppfabricData ) const
{
   TFV_FindMode_t findMode = TFV_FIND_EXACT;
   TFV_FigureMode_t figureMode = TFV_FIGURE_ANY;

   return( this->Find( layer, point, findMode, figureMode, ppfabricData ));
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
            TGS_Layer_t        layer,
      const TGS_Region_c&      region,
            TFV_FindMode_t     findMode,
            TFV_FigureMode_t   figureMode,
            TFV_FabricData_c** ppfabricData ) const
{
   bool ok = true;

   TFV_FabricFigure_t* pfabricFigure = 0;
   ok = this->Find( layer, region, findMode, figureMode, &pfabricFigure );
   if( ok && pfabricFigure )
   {
      if( ppfabricData )
      {
         *ppfabricData = pfabricFigure->GetData( );
      }
   }
   return( ok );
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
      const TGS_Rect_c&        rect,
            TFV_FindMode_t     findMode,
            TFV_FigureMode_t   figureMode,
            TFV_FabricData_c** ppfabricData ) const
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->Find( layer, region, findMode, figureMode, ppfabricData ));
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
            TGS_Layer_t        layer,
      const TGS_Point_c&       point,
            TFV_FindMode_t     findMode,
            TFV_FigureMode_t   figureMode,
            TFV_FabricData_c** ppfabricData ) const
{
   TGS_Region_c region( point, point );

   return( this->Find( layer, region, findMode, figureMode, ppfabricData ));
}

//===========================================================================//
bool TFV_FabricView_c::Find( 
      TGS_Layer_t         layer,
      TFV_FabricLayer_c** ppfabricLayer ) const
{
   bool ok = true;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( ppfabricLayer )
      {
         *ppfabricLayer = pfabricLayer;
      }
      ok = ( pfabricLayer ? true : false );
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricView_c::Find", 
                                  "Fabric view's tile plane list is not initialized!\n" );
   }
   return( ok );
}

//===========================================================================//
// Method         : FindConnected
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::FindConnected( 
            TGS_Layer_t     layer,
      const TGS_Region_c&   region,
            TGS_RectList_t* prectList ) const
{
   TGS_Rect_c rect( layer, region );
   return( this->FindConnected( rect, prectList ));
}

//===========================================================================//
bool TFV_FabricView_c::FindConnected( 
      const TGS_Rect_c&     rect,
            TGS_RectList_t* prectList ) const
{
   bool foundConnected = false;

   const TGS_Region_c& thisRegion = rect.region;

   TGS_Layer_t thisLayer = rect.layer;
   TGS_Layer_t prevLayer = rect.layer - 1;
   TGS_Layer_t nextLayer = rect.layer + 1;

   TGS_RectList_t rectList;

   const TFV_FabricLayer_c* pfabricLayer = this->FindFabricLayer( thisLayer );
   if( pfabricLayer )
   {
      TGS_RegionList_t regionList;
      if( pfabricLayer->FindConnected( thisRegion, &regionList ))
      {
         foundConnected = true;

         for( size_t i = 0; i < regionList.GetLength( ); ++i )
         {
            const TGS_Region_c& region = *regionList[ i ];

            TGS_Rect_c rect_( thisLayer, region );
            rectList.Add( rect_ );
         }
      }
   }

   const TFV_FabricLayer_c* pprevLayer = this->FindFabricLayer( prevLayer );
   if( pprevLayer )
   {
      TGS_RegionList_t regionList;
      if( pprevLayer->Find( thisRegion, &regionList ))
      {
         foundConnected = true;

         for( size_t i = 0; i < regionList.GetLength( ); ++i )
         {
            const TGS_Region_c& region = *regionList[ i ];

            TGS_Rect_c rect_( prevLayer, region );
            rectList.Add( rect_ );
         }
      }
   }

   const TFV_FabricLayer_c* pnextLayer = this->FindFabricLayer( nextLayer );
   if( pnextLayer )
   {
      TGS_RegionList_t regionList;
      if( pnextLayer->Find( thisRegion, &regionList ))
      {
         foundConnected = true;

         for( size_t i = 0; i < regionList.GetLength( ); ++i )
         {
            const TGS_Region_c& region = *regionList[ i ];

            TGS_Rect_c rect_( nextLayer, region );
            rectList.Add( rect_ );
         }
      }
   }

   if( prectList )
   {
      *prectList = rectList;
   }
   return( foundConnected );
}

//===========================================================================//
// Method         : FindNearest
// Purpose        : Find and return the nearest fabric view rectangle based 
//                  on the given reference rectangle|region|point, and asso.
//                  desired matching data constraints (ie. net index, type,
//                  orientation, and width).  The nearest rectangle is 
//                  searched based on the given search side w.r.t. to a given 
//                  reference rectangle.  The nearest rectangle may be 
//                  neighboring on same layer or may be orthogonal on a 
//                  different layer.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::FindNearest(
            TGS_Layer_t   layer,
      const TGS_Region_c& region,
            TGS_Region_c* pfoundRegion,
            double        maxDistance ) const
{
   TGS_Rect_c foundRect;

   TGS_Rect_c rect( layer, region );
   this->FindNearest( rect, &foundRect, maxDistance );
   if( pfoundRegion )
   {
      *pfoundRegion = foundRect.region;
   }
   return( foundRect.IsValid( ) ? true : false );
}

//===========================================================================//
bool TFV_FabricView_c::FindNearest(
            TGS_Layer_t   layer,
      const TGS_Region_c& region,
            TGS_Rect_c*   pfoundRect,
            double        maxDistance ) const
{
   TGS_Rect_c rect( layer, region );
   return( this->FindNearest( rect, pfoundRect, maxDistance ));
}

//===========================================================================//
bool TFV_FabricView_c::FindNearest(
      const TGS_Rect_c& rect,
            TGS_Rect_c* pfoundRect,
            double      maxDistance ) const
{
   TGS_Rect_c foundRect;

   TGS_Layer_t layer = rect.layer;
   const TFV_FabricLayer_c* pfabricLayer = this->FindFabricLayer( layer );
   if( pfabricLayer )
   {
      TGS_Region_c foundRegion;

      const TGS_Region_c& region = rect.region;
      pfabricLayer->FindNearest( region, &foundRegion, maxDistance );
      if( foundRegion.IsValid( ))
      {
         foundRect.Set( layer, foundRegion );
      }
   }
   if( pfoundRect )
   {
      *pfoundRect = foundRect;
   }
   return( foundRect.IsValid( ) ? true : false );
}

//===========================================================================//
bool TFV_FabricView_c::FindNearest(
            TGS_Layer_t   layer,
      const TGS_Point_c&  point,
            TGS_Region_c* pfoundRegion,
            double        maxDistance ) const
{
   TGS_Rect_c foundRect;

   TGS_Rect_c rect( layer, point, point );
   this->FindNearest( rect, &foundRect, maxDistance );
   if( pfoundRegion )
   {
      *pfoundRegion = foundRect.region;
   }
   return( foundRect.IsValid( ) ? true : false );
}

//===========================================================================//
bool TFV_FabricView_c::FindNearest(
            TGS_Layer_t  layer,
      const TGS_Point_c& point,
            TGS_Rect_c*  pfoundRect,
            double       maxDistance ) const
{
   TGS_Rect_c rect( layer, point, point );
   return( this->FindNearest( rect, pfoundRect, maxDistance ));
}

//===========================================================================//
bool TFV_FabricView_c::FindNearest(
            TGS_Layer_t   layer,
      const TGS_Region_c& region,
            TC_SideMode_t sideMode,
            TGS_Region_c* pfoundRegion ) const
{
   TGS_Rect_c foundRect;

   TGS_Rect_c rect( layer, region );
   this->FindNearest( rect, sideMode, &foundRect );
   if( pfoundRegion )
   {
      *pfoundRegion = foundRect.region;
   }
   return( foundRect.IsValid( ) ? true : false );
}

//===========================================================================//
bool TFV_FabricView_c::FindNearest(
            TGS_Layer_t   layer,
      const TGS_Region_c& region,
            TC_SideMode_t sideMode,
            TGS_Rect_c*   pfoundRect ) const
{
   TGS_Rect_c rect( layer, region );
   return( this->FindNearest( rect, sideMode, pfoundRect ));
}

//===========================================================================//
bool TFV_FabricView_c::FindNearest(
      const TGS_Rect_c&   rect,
            TC_SideMode_t sideMode,
            TGS_Rect_c*   pfoundRect ) const
{
   TGS_Rect_c foundRect;

   TGS_Layer_t layer = rect.layer;
   const TFV_FabricLayer_c* pfabricLayer = this->FindFabricLayer( layer );
   if( pfabricLayer )
   {
      TGS_Region_c foundRegion;

      const TGS_Region_c& region = rect.region;
      pfabricLayer->FindNearest( region, sideMode, &foundRegion );
      if( foundRegion.IsValid( ))
      {
         foundRect.Set( layer, foundRegion );
      }
   }
   if( pfoundRect )
   {
      *pfoundRect = foundRect;
   }
   return( foundRect.IsValid( ) ? true : false );
}

//===========================================================================//
// Method         : FindFabricLayer
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricLayer_c* TFV_FabricView_c::FindFabricLayer(
      TGS_Layer_t layer ) const
{
   TFV_FabricLayer_c* pfabricLayer = 0;
   bool ok = this->Find( layer, &pfabricLayer );

   return( ok ? pfabricLayer : 0 );
}

//===========================================================================//
// Method         : FindFabricPlane
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
const TFV_FabricPlane_c* TFV_FabricView_c::FindFabricPlane(
      TGS_Layer_t layer ) const
{
   const TFV_FabricLayer_c* pfabricLayer = this->FindFabricLayer( layer );
   const TFV_FabricPlane_c* pfabricPlane = pfabricLayer ? 
                                           pfabricLayer->GetPlane( ) : 0;
   return( pfabricPlane );
}

//===========================================================================//
// Method         : FindFabricBox
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
const TGS_Box_c& TFV_FabricView_c::FindFabricBox(
      void ) const
{
   return( this->fabricBox_ );
}

//===========================================================================//
// Method         : ApplyIntersect
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::ApplyIntersect(  
      const TGS_Box_c& box )
{
   bool ok = true;

   const TGS_Box_c& thisBox = this->FindFabricBox( );
   if( thisBox.IsIntersecting( box ))
   {
      TGS_Region_c boxRegion( box.lowerLeft.x, box.lowerLeft.y,
                              box.upperRight.x, box.upperRight.y );

      TFV_FabricView_c thisView( *this );
      this->Clear( );
      this->Init( thisView.GetRegion( ));

      const TGS_LayerRange_t& thisViewLayerRange = thisView.GetLayerRange( );
      for( TGS_Layer_t layer = thisViewLayerRange.i; layer <= thisViewLayerRange.j; ++layer )
      {
         if( layer < box.lowerLeft.z )
            continue;

         if( layer > box.upperRight.z )
            break;

         const TFV_FabricPlane_c* pfabricPlane = thisView.FindFabricPlane( layer );
         if( !pfabricPlane )
            continue;

         const TGS_Region_c& fabricRegion = pfabricPlane->GetRegion( );
         TFV_FabricPlaneIter_t fabricPlaneIter( *pfabricPlane, fabricRegion );

         TFV_FabricFigure_t* pfabricFigure = 0;
         while( fabricPlaneIter.Next( &pfabricFigure, TFV_FIGURE_SOLID ))
         {
            TGS_Region_c region = pfabricFigure->GetRegion( );
            const TFV_FabricData_c& fabricData = *pfabricFigure->GetData( );

            if( boxRegion.IsWithin( region ))
            {
               ok = this->Add( layer, region, fabricData );
            }
            else if( boxRegion.IsIntersecting( region ))
            {
               region.ApplyIntersect( boxRegion );
               ok = this->Add( layer, region, fabricData );
            }
         }
      }
   }
   return( ok );
}

//===========================================================================//
bool TFV_FabricView_c::ApplyIntersect(  
      const TGS_Rect_c& rect )
{
   TGS_Box_c rectBox( rect.region.x1, rect.region.y1, rect.layer,
                      rect.region.x2, rect.region.y2, rect.layer );

   return( this->ApplyIntersect( rectBox ));
}

//===========================================================================//
bool TFV_FabricView_c::ApplyIntersect(  
      const TGS_Region_c& region )
{
   TGS_Box_c rectBox( region.x1, region.y1, this->layerRange_.i, 
                      region.x2, region.y2, this->layerRange_.j );

   return( this->ApplyIntersect( rectBox ));
}

//===========================================================================//
// Method         : IsClear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::IsClear( 
            TGS_Layer_t  layer,
      const TGS_Point_c& point ) const
{
   bool isClear = false;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         isClear = pfabricLayer->IsClear( point );
      }
   }
   return( isClear );
}

//===========================================================================//
// Method         : IsClearAny
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::IsClearAny( 
            TGS_Layer_t   layer,
      const TGS_Region_c& region ) const
{
   bool isClearAny = false;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         isClearAny = pfabricLayer->IsClearAny( region );
      }
   }
   return( isClearAny );
}

//===========================================================================//
bool TFV_FabricView_c::IsClearAny( 
      const TGS_Rect_c& rect ) const
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->IsClearAny( layer, region ));
}

//===========================================================================//
// Method         : IsClearAll
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::IsClearAll( 
            TGS_Layer_t   layer,
      const TGS_Region_c& region ) const
{
   bool isClearAll = false;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         isClearAll = pfabricLayer->IsClearAll( region );
      }
   }
   return( isClearAll );
}

//===========================================================================//
bool TFV_FabricView_c::IsClearAll( 
      const TGS_Rect_c& rect ) const
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->IsClearAll( layer, region ));
}

//===========================================================================//
// Method         : IsSolid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::IsSolid( 
            TGS_Layer_t          layer,
      const TGS_Point_c&         point,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool isSolid = false;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         isSolid = pfabricLayer->IsSolid( point, ppfabricFigure );
      }
   }
   return( isSolid );
}

//===========================================================================//
// Method         : IsSolidAny
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::IsSolidAny( 
            TGS_Layer_t          layer,
      const TGS_Region_c&        region,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool isSolidAny = false;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         isSolidAny = pfabricLayer->IsSolidAny( region, ppfabricFigure );
      }
   }
   return( isSolidAny );
}

//===========================================================================//
bool TFV_FabricView_c::IsSolidAny( 
      const TGS_Rect_c&          rect,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->IsSolidAny( layer, region, ppfabricFigure ));
}

//===========================================================================//
// Method         : IsSolidAll
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::IsSolidAll( 
            TGS_Layer_t          layer,
      const TGS_Region_c&        region,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool isSolidAll = false;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         isSolidAll = pfabricLayer->IsSolidAll( region, ppfabricFigure );
      }
   }
   return( isSolidAll );
}

//===========================================================================//
bool TFV_FabricView_c::IsSolidAll( 
      const TGS_Rect_c&          rect,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->IsSolidAll( layer, region, ppfabricFigure ));
}

//===========================================================================//
// Method         : IsWithin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::IsWithin( 
      TGS_Layer_t layer ) const
{
   bool isWithin = false;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if( pfabricLayer )
      {
         isWithin = true;
      }
   }
   return( isWithin );
}

//===========================================================================//
bool TFV_FabricView_c::IsWithin( 
      const TGS_Point_c& point ) const
{
   TGS_Layer_t layer = point.z;

   return( this->IsWithin( layer, point ));
}

//===========================================================================//
bool TFV_FabricView_c::IsWithin( 
            TGS_Layer_t  layer,
      const TGS_Point_c& point ) const
{
   bool isWithin = false;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if(( pfabricLayer ) &&
         ( pfabricLayer->IsWithin( point )))
      {
         isWithin = true;
      }
   }
   return( isWithin );
}

//===========================================================================//
bool TFV_FabricView_c::IsWithin( 
            TGS_Layer_t   layer,
      const TGS_Region_c& region ) const
{
   bool isWithin = false;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if(( pfabricLayer ) &&
         ( pfabricLayer->IsWithin( region )))
      {
         isWithin = true;
      }
   }
   return( isWithin );
}

//===========================================================================//
bool TFV_FabricView_c::IsWithin( 
      const TGS_Rect_c& rect ) const
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->IsWithin( layer, region ));
}

//===========================================================================//
bool TFV_FabricView_c::IsWithin( 
      const TGS_Box_c& box ) const
{
   bool isWithin = false;

   for( TGS_Layer_t layer = box.lowerLeft.z; layer <= box.upperRight.z; ++layer )
   {
      TGS_Region_c region( box.lowerLeft, box.upperRight );
      isWithin = this->IsWithin( layer, region );
      if( !isWithin )
         break;
   }
   return( isWithin );
}

//===========================================================================//
// Method         : IsIntersecting
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricView_c::IsIntersecting( 
            TGS_Layer_t   layer,
      const TGS_Region_c& region ) const
{
   bool isIntersecting = false;

   if( this->pfabricLayerList_ )
   {
      TFV_FabricLayer_c* pfabricLayer = this->pfabricLayerList_->operator[]( layer );
      if(( pfabricLayer ) &&
         ( pfabricLayer->IsIntersecting( region )))
      {
         isIntersecting = true;
      }
   }
   return( isIntersecting );
}

//===========================================================================//
bool TFV_FabricView_c::IsIntersecting( 
      const TGS_Rect_c& rect ) const
{
   TGS_Layer_t layer = rect.layer;
   const TGS_Region_c& region = rect.region;

   return( this->IsIntersecting( layer, region ));
}

//===========================================================================//
// Method         : Allocate_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricLayerList_t* TFV_FabricView_c::Allocate_( 
      const TGS_Region_c&     region,
      const TGS_LayerRange_t& layerRange )
{
  TFV_FabricLayerList_t* pfabricLayerList = 0;

   if( region.IsValid( ) && layerRange.IsValid( ))
   {
      pfabricLayerList = new TC_NOTHROW TFV_FabricLayerList_t( layerRange );

      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      if( printHandler.IsValidNew( pfabricLayerList,
                                   sizeof( TFV_FabricLayerList_t ),
                                   "TFV_FabricView_c::Allocate_" ))
      {
         for( TGS_Layer_t layer = layerRange.i; layer <= layerRange.j; ++layer )
         {
            string srName;
            TFV_ExtractStringLayerType( static_cast< TFV_LayerType_t >( layer ), &srName );
   
            TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED;
            switch( layer )
            {
            case TFV_LAYER_CHANNEL_HORZ:
            case TFV_LAYER_SEGMENT_HORZ: orient = TGS_ORIENT_HORIZONTAL; break;
            case TFV_LAYER_CHANNEL_VERT:
            case TFV_LAYER_SEGMENT_VERT: orient = TGS_ORIENT_VERTICAL;   break;
            default:                     orient = TGS_ORIENT_UNDEFINED;  break;
            }

            TFV_FabricLayer_c fabricLayer;
            pfabricLayerList->Insert( layer, fabricLayer );

            TFV_FabricLayer_c* pfabricLayer = pfabricLayerList->operator[]( layer );
            pfabricLayer->Init( srName, layer, orient, region );
         }
      }
      this->fabricBox_.Set( region.x1, region.y1, layerRange.i,
                            region.x2, region.y2, layerRange.j );
   }
   return( pfabricLayerList );
}

//===========================================================================//
// Method         : Deallocate_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TFV_FabricView_c::Deallocate_( 
      TFV_FabricLayerList_t* pfabricLayerList )
{
   if( pfabricLayerList )
   {
      for( size_t i = 0; i < pfabricLayerList->GetLength( ); ++i )
      {
         TFV_FabricLayer_c* pfabricLayer = pfabricLayerList->operator[]( i );
         if( pfabricLayer )
         {
            pfabricLayer->Clear( );
         }
      }
   }
   delete pfabricLayerList;
}
