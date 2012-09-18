//===========================================================================//
// Purpose : Method definitions for a TFV_FabricLayer class.
//
//           Public methods include:
//           - TFV_FabricLayer_c, ~TFV_FabricLayer_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - Init
//           - Add
//           - Delete
//           - Find
//           - FindConnected
//           - FindNearest
//           - IsClear, IsClearAny, IsClearAll
//           - IsSolid, IsSolidAny, IsSolidAll
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TGS_StringUtils.h"

#include "TFV_FabricLayer.h"

//===========================================================================//
// Method         : TFV_FabricLayer_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricLayer_c::TFV_FabricLayer_c( 
      void )
      :
      srName_( "" ),
      layer_( TGS_LAYER_UNDEFINED ),
      orient_( TGS_ORIENT_UNDEFINED )
{
}

//===========================================================================//
TFV_FabricLayer_c::TFV_FabricLayer_c( 
      const string&          srName,
            TGS_Layer_t      layer,
            TGS_OrientMode_t orient,
      const TGS_Region_c&    region )
      :
      srName_( "" ),
      layer_( TGS_LAYER_UNDEFINED ),
      orient_( TGS_ORIENT_UNDEFINED )
{
   this->Init( srName, layer, orient, region );
}

//===========================================================================//
TFV_FabricLayer_c::TFV_FabricLayer_c( 
      const char*            pszName,
            TGS_Layer_t      layer,
            TGS_OrientMode_t orient,
      const TGS_Region_c&    region )
      :
      srName_( "" ),
      layer_( TGS_LAYER_UNDEFINED ),
      orient_( TGS_ORIENT_UNDEFINED )
{
   this->Init( pszName, layer, orient, region );
}

//===========================================================================//
TFV_FabricLayer_c::TFV_FabricLayer_c( 
      const TFV_FabricLayer_c& fabricLayer )
      :
      srName_( fabricLayer.srName_ ),
      layer_( fabricLayer.layer_ ),
      orient_( fabricLayer.orient_ ),
      region_( fabricLayer.region_ ),
      plane_( fabricLayer.plane_ )
{
}

//===========================================================================//
// Method         : ~TFV_FabricLayer_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricLayer_c::~TFV_FabricLayer_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricLayer_c& TFV_FabricLayer_c::operator=( 
      const TFV_FabricLayer_c& fabricLayer )
{
   if( &fabricLayer != this )
   {
      this->srName_ = fabricLayer.srName_;
      this->layer_ = fabricLayer.layer_;
      this->orient_ = fabricLayer.orient_;
      this->region_ = fabricLayer.region_;
      this->plane_ = fabricLayer.plane_;
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
bool TFV_FabricLayer_c::operator==( 
      const TFV_FabricLayer_c& fabricLayer ) const
{
   return(( this->srName_ == fabricLayer.srName_ ) &&
          ( this->layer_ == fabricLayer.layer_ ) &&
          ( this->orient_ == fabricLayer.orient_ ) &&
          ( this->region_ == fabricLayer.region_ ) &&
          ( this->plane_ == fabricLayer.plane_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::operator!=( 
      const TFV_FabricLayer_c& fabricLayer ) const
{
   return(( !this->operator==( fabricLayer )) ? 
          true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TFV_FabricLayer_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   const char* pszName = this->srName_.data( );
   TGS_Layer_t layer = this->layer_;

   string srOrient;
   TGS_ExtractStringOrientMode( this->orient_, &srOrient );

   string srLayerRegion;
   this->region_.ExtractString( &srLayerRegion );

   printHandler.Write( pfile, spaceLen, "\"%s\" [%d] %s (%s)",
                                        TIO_PSZ_STR( pszName ),
                                        layer,
                                        TIO_SR_STR( srOrient ),
                                        TIO_SR_STR( srLayerRegion ));
   if( this->plane_.IsValid( ))
   {
      this->plane_.Print( pfile, spaceLen + 3 );
   }
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::Init(
      const string&          srName,
            TGS_Layer_t      layer,
            TGS_OrientMode_t orient,
      const TGS_Region_c&    region )
{
   this->srName_ = srName;
   this->layer_ = layer;
   this->orient_ = orient;
   this->region_ = region;
   return( this->plane_.Init( region ));
}

//===========================================================================//
bool TFV_FabricLayer_c::Init(
      const char*            pszName,
            TGS_Layer_t      layer,
            TGS_OrientMode_t orient,
      const TGS_Region_c&    region )
{
   string srName( TIO_PSZ_STR( pszName ));
   return( this->Init( srName, layer, orient, region ));
}

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::Add( 
      const TGS_Region_c&     region,
      const TFV_FabricData_c& fabricData,
            TFV_AddMode_t     addMode )
{
   return( this->plane_.Add( region, fabricData, addMode ));
}

//===========================================================================//
bool TFV_FabricLayer_c::Add( 
      const TFV_FabricFigure_t& fabricFigure,
            TFV_AddMode_t       addMode )
{
   const TGS_Region_c& region = fabricFigure.GetRegion( );
   const TFV_FabricData_c& fabricData = *fabricFigure.GetData( );

   return( this->Add( region, fabricData, addMode ));
}

//===========================================================================//
// Method         : Delete
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::Delete( 
      const TGS_Region_c&    region,
            TFV_DeleteMode_t deleteMode )
{
   return( this->plane_.Delete( region, deleteMode ));
}

//===========================================================================//
bool TFV_FabricLayer_c::Delete( 
      const TGS_Region_c&     region,
      const TFV_FabricData_c& fabricData,
            TFV_DeleteMode_t  deleteMode )
{
   return( this->plane_.Delete( region, fabricData, deleteMode ));
}

//===========================================================================//
bool TFV_FabricLayer_c::Delete( 
      const TFV_FabricFigure_t& fabricFigure,
            TFV_DeleteMode_t    deleteMode )
{
   bool ok = true;

   const TGS_Region_c& region = fabricFigure.GetRegion( );
   if( fabricFigure.HasData( ))
   {
      const TFV_FabricData_c& fabricData = *fabricFigure.GetData( );
      ok = this->Delete( region, fabricData, deleteMode );
   }
   else
   {
      ok = this->Delete( region, deleteMode );
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
bool TFV_FabricLayer_c::Find( 
      const TGS_Point_c&         point,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   return( this->plane_.Find( point, ppfabricFigure ));
}

//===========================================================================//
bool TFV_FabricLayer_c::Find( 
      const TGS_Region_c&        region,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   return( this->plane_.Find( region, ppfabricFigure ));
}

//===========================================================================//
bool TFV_FabricLayer_c::Find( 
      const TGS_Region_c&        region,
            TFV_FindMode_t       findMode,
	    TFV_FigureMode_t     figureMode,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   return( this->plane_.Find( region, findMode, figureMode, ppfabricFigure ));
}

//===========================================================================//
bool TFV_FabricLayer_c::Find( 
      const TGS_Region_c&      region,
            TFV_FabricData_c** ppfabricData ) const
{
   return( this->plane_.Find( region, ppfabricData ));
}

//===========================================================================//
bool TFV_FabricLayer_c::Find( 
      const TGS_Region_c&      region,
            TFV_FindMode_t     findMode,
	    TFV_FigureMode_t   figureMode,
            TFV_FabricData_c** ppfabricData ) const
{
   return( this->plane_.Find( region, findMode, figureMode, ppfabricData ));
}

//===========================================================================//
bool TFV_FabricLayer_c::Find( 
      const TGS_Region_c&     region,
            TGS_RegionList_t* pregionList ) const
{
   if( pregionList )
   {
      pregionList->Clear( );

      const TFV_FabricPlane_t& fabricPlane = this->plane_;
      if( fabricPlane.IsSolid( TFV_IS_SOLID_ANY, region ))
      {
         TFV_FabricPlaneIter_t fabricPlaneIter( fabricPlane, region );
         TFV_FabricFigure_t* pfabricFigure = 0;
         while( fabricPlaneIter.Next( &pfabricFigure, TFV_FIGURE_SOLID ))
         {
            const TGS_Region_c& fabricRegion = pfabricFigure->GetRegion( );
            pregionList->Add( fabricRegion );
         }
      }
   }
   return( pregionList && pregionList->IsValid( ) ? true : false );
}

//===========================================================================//
bool TFV_FabricLayer_c::Find( 
      const TGS_Region_c&     region,
      const TFV_FabricData_c& fabricData,
            TGS_RegionList_t* pregionList ) const
{
   if( pregionList )
   {
      pregionList->Clear( );

      const TFV_FabricPlane_t& fabricPlane = this->plane_;
      if( fabricPlane.IsSolid( TFV_IS_SOLID_ANY, region ))
      {
         TFV_FabricPlaneIter_t fabricPlaneIter( fabricPlane, region );
         TFV_FabricFigure_t* pfabricFigure = 0;
         while( fabricPlaneIter.Next( &pfabricFigure, TFV_FIGURE_SOLID ))
         {
   	    if( fabricData == *pfabricFigure->GetData( ))
            {
               const TGS_Region_c& fabricRegion = pfabricFigure->GetRegion( );
               pregionList->Add( fabricRegion );
            }
         }
      }
   }
   return( pregionList && pregionList->IsValid( ) ? true : false );
}

//===========================================================================//
// Method         : FindConnected
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::FindConnected( 
      const TGS_Region_c&     region,
            TGS_RegionList_t* pregionList ) const
{
   double minGrid = this->plane_.GetMinGrid( );

   TGS_Region_c searchRegion( region );
   searchRegion.ApplyScale( minGrid );

   const TFV_FabricPlane_t& fabricPlane = this->plane_;
   if( fabricPlane.IsSolid( TFV_IS_SOLID_ANY, region ))
   {
      TFV_FabricPlaneIter_t fabricPlaneIter( fabricPlane, searchRegion );
      TFV_FabricFigure_t* pfabricFigure = 0;
      while( fabricPlaneIter.Next( &pfabricFigure, TFV_FIGURE_SOLID ))
      {
         const TGS_Region_c& foundRegion = pfabricFigure->GetRegion( );
         if( pregionList )
         {
            pregionList->Add( foundRegion );
         }
      }
   }
   return( pregionList && pregionList->IsValid( ) ? true : false );
}

//===========================================================================//
// Method         : FindNearest
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::FindNearest(
      const TGS_Region_c& region,
            TGS_Region_c* pfoundRegion,
	    double        maxDistance ) const
{
   TGS_Region_c foundRegion;

   const TGS_Region_c& planeRegion = this->plane_.GetRegion( );

   TGS_Region_c findRegion( region );
   findRegion.ApplyIntersect( planeRegion );

   TFV_FabricFigure_t* pfabricFigure = 0;
   if( findRegion.IsValid( ) &&
       this->plane_.FindNearest( findRegion, &pfabricFigure, maxDistance ) &&
       pfabricFigure )
   {
      foundRegion = pfabricFigure->GetRegion( );
   }

   if( pfoundRegion )
   {
      *pfoundRegion = foundRegion;
   }     
   return( foundRegion.IsValid( ) ? true : false );
}

//===========================================================================//
bool TFV_FabricLayer_c::FindNearest(
      const TGS_Region_c& region,
            TC_SideMode_t sideMode,
            TGS_Region_c* pfoundRegion ) const
{
   TGS_Region_c foundRegion;

   const TGS_Region_c& planeRegion = this->plane_.GetRegion( );

   TGS_Region_c findRegion( region );
   findRegion.ApplyIntersect( planeRegion );

   TFV_FabricFigure_t* pfabricFigure = 0;
   if( findRegion.IsValid( ) &&
       this->plane_.FindNearest( findRegion, sideMode, &pfabricFigure ) &&
       pfabricFigure )
   {
      foundRegion = pfabricFigure->GetRegion( );
   }

   if( pfoundRegion )
   {
      *pfoundRegion = foundRegion;
   }     
   return( foundRegion.IsValid( ) ? true : false );
}

//===========================================================================//
// Method         : IsClear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::IsClear(
      const TGS_Point_c& point ) const
{
   bool isClear = this->plane_.IsClear( point );
   return( isClear );
}

//===========================================================================//
// Method         : IsClearAny
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::IsClearAny(
      const TGS_Region_c& region ) const
{
   bool isClear = this->plane_.IsClear( TFV_IS_CLEAR_ANY, region );
   return( isClear );
}

//===========================================================================//
bool TFV_FabricLayer_c::IsClearAny(
      const TGS_RegionList_t& regionList ) const
{
   bool isClear = false;

   for( size_t i = 0; i < regionList.GetLength( ); ++i )
   {
      const TGS_Region_c& region = *regionList[ i ];

      isClear = this->IsClearAny( region ); 
      if( isClear )
         break;
   }
   return( isClear );
}

//===========================================================================//
// Method         : IsClearAll
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::IsClearAll(
      const TGS_Region_c& region ) const
{
   bool isClear = this->plane_.IsClear( TFV_IS_CLEAR_ALL, region );
   return( isClear );
}

//===========================================================================//
bool TFV_FabricLayer_c::IsClearAll(
      const TGS_RegionList_t& regionList ) const
{
   bool isClear = false;

   for( size_t i = 0; i < regionList.GetLength( ); ++i )
   {
      const TGS_Region_c& region = *regionList[ i ];

      isClear = this->IsClearAll( region );
      if( !isClear )
         break;
   }
   return( isClear );
}

//===========================================================================//
// Method         : IsSolid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::IsSolid(
      const TGS_Point_c&         point,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool isSolid = this->plane_.IsSolid( point, ppfabricFigure );
   return( isSolid );
}

//===========================================================================//
// Method         : IsSolidAny
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::IsSolidAny(
      const TGS_Region_c&        region,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool isSolid = this->plane_.IsSolid( TFV_IS_SOLID_ANY, region, ppfabricFigure );
   return( isSolid );
}

//===========================================================================//
bool TFV_FabricLayer_c::IsSolidAny(
      const TGS_RegionList_t&    regionList,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool isSolid = false;

   for( size_t i = 0; i < regionList.GetLength( ); ++i )
   {
      const TGS_Region_c& region = *regionList[ i ];

      isSolid = this->IsSolidAny( region, ppfabricFigure );
      if( isSolid )
         break;
   }
   return( isSolid );
}

//===========================================================================//
// Method         : IsSolidAll
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricLayer_c::IsSolidAll(
      const TGS_Region_c&        region,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool isSolid = this->plane_.IsSolid( TFV_IS_SOLID_ALL, region, ppfabricFigure );
   return( isSolid );
}

//===========================================================================//
bool TFV_FabricLayer_c::IsSolidAll(
      const TGS_RegionList_t&    regionList,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool isSolid = false;

   for( size_t i = 0; i < regionList.GetLength( ); ++i )
   {
      const TGS_Region_c& region = *regionList[ i ];

      isSolid = this->IsSolidAll( region, ppfabricFigure );
      if( !isSolid )
         break;
   }
   return( isSolid );
}
