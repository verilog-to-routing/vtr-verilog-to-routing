//===========================================================================//
// Purpose : Method definitions for a TFV_FabricPlane class.
//
//           Public methods include:
//           - TFV_FabricPlane_c, ~TFV_FabricPlane_c
//           - Print, PrintLaff
//           - Init
//           - Add
//           - Delete
//           - Find
//           - FindNearest
//           - IsClear
//           - IsSolid
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TFV_FabricPlane.h"

//===========================================================================//
// Method         : TFV_FabricPlane_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricPlane_c::TFV_FabricPlane_c( 
      void )
{
}

//===========================================================================//
TFV_FabricPlane_c::TFV_FabricPlane_c( 
      const TGS_Region_c& region )
      :
      TTPT_TilePlane_c< TFV_FabricData_c >( region )
{
}

//===========================================================================//
TFV_FabricPlane_c::TFV_FabricPlane_c( 
      const TFV_FabricPlane_c& fabricPlane )
{
   this->Init( fabricPlane );
}

//===========================================================================//
// Method         : ~TFV_FabricPlane_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricPlane_c::~TFV_FabricPlane_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TFV_FabricPlane_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TFV_FabricPlane_t::Print( pfile, spaceLen );
}

//===========================================================================//
// Method         : PrintLaff
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TFV_FabricPlane_c::PrintLaff( 
      void ) const
{
   TFV_FabricPlane_t::PrintLaff( );
} 

//===========================================================================//
void TFV_FabricPlane_c::PrintLaff( 
      const char* pszFileName ) const
{
   TFV_FabricPlane_t::PrintLaff( pszFileName );
} 

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricPlane_c::Init( 
      const TGS_Region_c& region )
{
   return( TFV_FabricPlane_t::Init( region ));
} 

//===========================================================================//
bool TFV_FabricPlane_c::Init( 
      const TFV_FabricPlane_t& fabricPlane )
{
   return( TFV_FabricPlane_t::Init( fabricPlane ));
} 

//===========================================================================//
// Method         : Add
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricPlane_c::Add(
      const TGS_Region_c& region,
            TFV_AddMode_t addMode )
{
   bool ok = true;

   if( TFV_FabricPlane_t::IsValid( ))
   {
      ok = TFV_FabricPlane_t::Add( region, addMode );
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricPlane_c::Add", 
                                  "Fabric plane's tile plane is not initialized!\n" );
   }
   return( ok );
}

//===========================================================================//
bool TFV_FabricPlane_c::Add(
      const TGS_Region_c&     region,
      const TFV_FabricData_c& fabricData,
            TFV_AddMode_t     addMode )
{
   bool ok = true;

   double minGrid = this->GetMinGrid( );

   if( TFV_FabricPlane_t::IsValid( ))
   {
      if(( addMode == TFV_ADD_NEW ) || ( addMode == TFV_ADD_MERGE ))
      {
         TGS_Region_c addRegion( region );

         TFV_FabricFigure_t* pfabricFigure = 0;
         while( this->IsSolid( TFV_IS_SOLID_ANY, addRegion, &pfabricFigure ))
         {
            if( *pfabricFigure->GetData( ) == fabricData )
            {
               const TGS_Region_c& fabricRegion( pfabricFigure->GetRegion( ));
               if( fabricRegion.IsWithin( addRegion ))
               {
                  addRegion.Reset( );
                  break;
               }

               TGS_RegionList_t addRegionList( 4 );
               region.FindDifference( addRegion, fabricRegion, 
                                      TGS_ORIENT_VERTICAL, 
                                      &addRegionList, minGrid );

               addRegion = *addRegionList[ 0 ];
               for( size_t i = 1; i < addRegionList.GetLength( ); ++i )
               {
                  const TGS_Region_c& addRegion_ = *addRegionList[ i ];
                  this->Add( addRegion_, fabricData, addMode );
               }
               continue;
	    }

            TGS_Region_c deleteRegion( pfabricFigure->GetRegion( ));
            deleteRegion.ApplyIntersect( addRegion );

            TFV_FabricPlane_t::Delete( deleteRegion, TFV_DELETE_INTERSECT );
         }

         if( addRegion.IsValid( ))
         {
            ok = TFV_FabricPlane_t::Add( addRegion, fabricData, addMode );
         }
      }
      else if( addMode == TFV_ADD_OVERLAP )
      {
         ok = TFV_FabricPlane_t::Add( region, fabricData, addMode );
      }
      else if( addMode == TFV_ADD_REGION )
      {
         ok = TFV_FabricPlane_t::Add( region, fabricData, addMode );
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricPlane_c::Add", 
                                  "Fabric plane's tile plane is not initialized!\n" );
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
bool TFV_FabricPlane_c::Delete(
      const TGS_Region_c&    region,
            TFV_DeleteMode_t deleteMode )
{
   bool ok = true;

   if( TFV_FabricPlane_t::IsValid( ))
   {
      TFV_FabricFigure_t* pfabricFigure = 0;
      pfabricFigure = TFV_FabricPlane_t::Find( region.x1, region.y1 );
      if( pfabricFigure )
      {
         ok = TFV_FabricPlane_t::Delete( region, deleteMode );
      }
      else
      {
         string srRegion;
         region.ExtractString( &srRegion );

         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         ok = printHandler.Internal( "TFV_FabricPlane_c::Delete", 
                                     "Fabric plane's tile not found for region (%s)!\n",
                                     TIO_SR_STR( srRegion ));
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricPlane_c::Delete", 
                                  "Fabric plane's tile plane is not initialized!\n" );
   }
   return( ok );
}

//===========================================================================//
bool TFV_FabricPlane_c::Delete(
      const TGS_Region_c&     region,
      const TFV_FabricData_c& fabricData,
            TFV_DeleteMode_t  deleteMode )
{
   bool ok = true;

   if( TFV_FabricPlane_t::IsValid( ))
   {
      TFV_FabricFigure_t* pfabricFigure = 0;
      pfabricFigure = TFV_FabricPlane_t::Find( region.x1, region.y1 );
      if( pfabricFigure )
      {
         ok = TFV_FabricPlane_t::Delete( region, fabricData, deleteMode );
      }
      else
      {
         string srRegion;
         region.ExtractString( &srRegion );

         TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
         ok = printHandler.Internal( "TFV_FabricPlane_c::Delete", 
                                     "Fabric plane's tile not found for region (%s)!\n",
                                     TIO_SR_STR( srRegion ));
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricPlane_c::Delete", 
                                  "Fabric plane's tile plane is not initialized!\n" );
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
bool TFV_FabricPlane_c::Find(
      const TGS_Point_c&         point,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool ok = true;

   if( TFV_FabricPlane_t::IsValid( ))
   {
      TFV_FabricFigure_t* pfabricFigure = 0;
      pfabricFigure = TFV_FabricPlane_t::Find( point );

      if( ppfabricFigure )
      {
         *ppfabricFigure = pfabricFigure;
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricPlane_c::Find", 
                                  "Fabric plane's tile plane is not initialized!\n" );
   }
   return( ok ) ;
}

//===========================================================================//
bool TFV_FabricPlane_c::Find(
      const TGS_Region_c&        region,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   return( this->Find( region, TFV_FIND_EXACT, TFV_FIGURE_ANY, ppfabricFigure ));
}

//===========================================================================//
bool TFV_FabricPlane_c::Find(
      const TGS_Region_c&        region,
            TFV_FindMode_t       findMode,
            TFV_FigureMode_t     figureMode,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool ok = true;

   if( TFV_FabricPlane_t::IsValid( ))
   {
      TFV_FabricFigure_t* pfabricFigure = 0;
      pfabricFigure = TFV_FabricPlane_t::Find( region, findMode, figureMode );

      if( ppfabricFigure )
      {
         *ppfabricFigure = pfabricFigure;
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricPlane_c::Find", 
                                  "Fabric plane's tile plane is not initialized!\n" );
   }
   return( ok ) ;
}

//===========================================================================//
bool TFV_FabricPlane_c::Find(
      const TGS_Region_c&    region,
            TFV_FindMode_t   findMode,
            TFV_FigureMode_t figureMode,
            TGS_Region_c*    pfabricRegion ) const
{
   bool ok = true;

   TFV_FabricFigure_t* pfabricFigure = 0;
   ok = this->Find( region, findMode, figureMode, &pfabricFigure );
   if( ok && pfabricFigure )
   {
      if( pfabricRegion )
      {
	 *pfabricRegion = pfabricFigure->GetRegion( );
      }
   }
   return( ok );
}

//===========================================================================//
bool TFV_FabricPlane_c::Find(
      const TGS_Region_c&      region,
            TFV_FabricData_c** ppfabricData ) const
{
   return( this->Find( region, TFV_FIND_EXACT, TFV_FIGURE_ANY, ppfabricData ));
}

//===========================================================================//
bool TFV_FabricPlane_c::Find(
      const TGS_Region_c&      region,
            TFV_FindMode_t     findMode,
            TFV_FigureMode_t   figureMode,
            TFV_FabricData_c** ppfabricData ) const
{
   bool ok = true;
   TFV_FabricFigure_t* pfabricFigure = 0;
   ok = this->Find( region, findMode, figureMode, &pfabricFigure );
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
// Method         : FindNearest
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricPlane_c::FindNearest(
      const TGS_Region_c&        region,
            TFV_FabricFigure_t** ppfabricFigure,
            double               maxDistance ) const
{
   bool ok = true;

   if( TFV_FabricPlane_t::IsValid( ))
   {
      TFV_FabricFigure_t* pfabricFigure = 0;
      pfabricFigure = TCTF_IsLT( maxDistance, TC_FLT_MAX ) ? 
                      TFV_FabricPlane_t::FindNearest( region, maxDistance ) :
                      TFV_FabricPlane_t::FindNearest( region );

      if( ppfabricFigure )
      {
         *ppfabricFigure = pfabricFigure;
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricPlane_c::FindNearest", 
                                  "Fabric plane's tile plane is not initialized!\n" );
   }
   return( ok ) ;
}

//===========================================================================//
bool TFV_FabricPlane_c::FindNearest(
      const TGS_Region_c&        region,
            TC_SideMode_t        sideMode,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool ok = true;

   if( TFV_FabricPlane_t::IsValid( ))
   {
      TFV_FabricFigure_t* pfabricFigure = 0;
      pfabricFigure = TFV_FabricPlane_t::FindNearest( region, sideMode );
      if( ppfabricFigure )
      {
         *ppfabricFigure = pfabricFigure;
      }
   }
   else
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      ok = printHandler.Internal( "TFV_FabricPlane_c::FindNearest", 
                                  "Fabric plane's tile plane is not initialized!\n" );
   }
   return( ok ) ;
}

//===========================================================================//
// Method         : IsClear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricPlane_c::IsClear(
      const TGS_Point_c& point ) const
{
   bool isClear = false;

   if( TFV_FabricPlane_t::IsValid( ) &&
       point.IsValid( ))
   {
      isClear = TFV_FabricPlane_t::IsClear( point );
   }
   return( isClear );
}

//===========================================================================//
bool TFV_FabricPlane_c::IsClear(
            TFV_IsClearMode_t isClearMode,
      const TGS_Region_c&     region ) const
{
   bool isClear = false;

   if( TFV_FabricPlane_t::IsValid( ) &&
       region.IsValid( ))
   {
      isClear = TFV_FabricPlane_t::IsClear( isClearMode, region );
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
bool TFV_FabricPlane_c::IsSolid(
      const TGS_Point_c&         point,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool isSolid = false;

   if( TFV_FabricPlane_t::IsValid( ) &&
       point.IsValid( ))
   {
      isSolid = TFV_FabricPlane_t::IsSolid( point );
      if( isSolid )
      {
         if( ppfabricFigure )
         {
            *ppfabricFigure = TFV_FabricPlane_t::Find( point );
         }
      }
   }
   return( isSolid );
}

//===========================================================================//
bool TFV_FabricPlane_c::IsSolid(
            TFV_IsSolidMode_t    isSolidMode,
      const TGS_Region_c&        region,
            TFV_FabricFigure_t** ppfabricFigure ) const
{
   bool isSolid = false;

  if( TFV_FabricPlane_t::IsValid( ) &&
       region.IsValid( ))
   {
      isSolid = TFV_FabricPlane_t::IsSolid( isSolidMode, region, ppfabricFigure );
   }
   return( isSolid );
}
