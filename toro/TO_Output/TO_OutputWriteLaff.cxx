//===========================================================================//
// Purpose : Supporting methods for the TO_Output post-processor class.
//           These methods support formatting and writing a LAFF file.
//
//           Private methods include:
//           - WriteLaffFile_
//           - WriteLaffBarInfo_
//           - WriteLaffHeader_
//           - WriteLaffTrailer_
//           - WriteLaffBoundingBox_
//           - WriteLaffInternalGrid_
//           - WriteLaffFabricView_
//           - WriteLaffFabricPlane__
//           - WriteLaffFabricMapTable_
//           - WriteLaffFabricPinList_
//           - WriteLaffFabricConnectionList_
//           - WriteLaffBoundary_
//           - WriteLaffMarker_
//           - WriteLaffLine_
//           - WriteLaffRegion_
//           - WriteLaffMapLayerId_
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

#include <cmath>
using namespace std;

#include "TIO_StringText.h"
#include "TIO_SkinHandler.h"
#include "TIO_PrintHandler.h"

#include "TC_MinGrid.h"

#include "TGS_Region.h"
#include "TGS_Line.h"
#include "TGS_ArrayGrid.h"
#include "TGS_ArrayGridIter.h"

#include "TFV_StringUtils.h"

#include "TO_Typedefs.h"
#include "TO_Output.h"

//===========================================================================//
// Method         : WriteLaffFile_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
bool TO_Output_c::WriteLaffFile_(
      const TFV_FabricView_c&    fabricView,
      const TOS_OutputOptions_c& outputOptions ) const
{
   bool ok = true;

   const char* pszLaffFileName = outputOptions.srLaffFileName.data( );
   int laffMask = outputOptions.laffMask;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Writing %s file '%s'...\n",
                      TIO_SZ_OUTPUT_LAFF_DEF_TYPE,
                      TIO_PSZ_STR( pszLaffFileName ));

   ok = printHandler.SetUserFileOutput( pszLaffFileName );
   if( ok )
   {
      printHandler.DisableOutput( TIO_PRINT_OUTPUT_ALL );
      printHandler.EnableOutput( TIO_PRINT_OUTPUT_USER_FILE );

      this->WriteLaffBarInfo_( );
      this->WriteLaffHeader_( );
      if( laffMask & TOS_OUTPUT_LAFF_BOUNDING_BOX )
      {
         this->WriteLaffBoundingBox_( fabricView );
      }
      if( laffMask & TOS_OUTPUT_LAFF_INTERNAL_GRID )
      {
         this->WriteLaffInternalGrid_( fabricView );
      }
      if( laffMask & TOS_OUTPUT_LAFF_FABRIC_VIEW )
      {
         this->WriteLaffFabricView_( fabricView );
      }
      this->WriteLaffTrailer_( );

      printHandler.EnableOutput( TIO_PRINT_OUTPUT_ALL );
      printHandler.DisableOutput( TIO_PRINT_OUTPUT_USER_FILE );
   }
   else
   {
      printHandler.Error( "Failed to open %s file '%s' in \"%s\" mode.\n",
                          TIO_SZ_OUTPUT_LAFF_DEF_TYPE,
                          TIO_PSZ_STR( pszLaffFileName ),
                          "write" );
   }
   return( ok );
}

//===========================================================================//
// Method         : WriteLaffBarInfo_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffBarInfo_( 
      void ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srIO, srPB, srSB, srCB, srCH, srCV, srSH, srSV, srIG, srIM, srIP, srBB;
   TFV_ExtractStringLayerType( TFV_LAYER_INPUT_OUTPUT, &srIO );
   TFV_ExtractStringLayerType( TFV_LAYER_PHYSICAL_BLOCK, &srPB );
   TFV_ExtractStringLayerType( TFV_LAYER_SWITCH_BOX, &srSB );
   TFV_ExtractStringLayerType( TFV_LAYER_CONNECTION_BOX, &srCB );
   TFV_ExtractStringLayerType( TFV_LAYER_CHANNEL_HORZ, &srCH );
   TFV_ExtractStringLayerType( TFV_LAYER_CHANNEL_VERT, &srCV );
   TFV_ExtractStringLayerType( TFV_LAYER_SEGMENT_HORZ, &srSH );
   TFV_ExtractStringLayerType( TFV_LAYER_SEGMENT_VERT, &srSV );
   TFV_ExtractStringLayerType( TFV_LAYER_INTERNAL_GRID, &srIG );
   TFV_ExtractStringLayerType( TFV_LAYER_INTERNAL_MAP, &srIM );
   TFV_ExtractStringLayerType( TFV_LAYER_INTERNAL_PIN, &srIP );
   TFV_ExtractStringLayerType( TFV_LAYER_BOUNDING_BOX, &srBB );

   int layerIO   = TO_LAFF_LAYER_IO;
   int layerPB   = TO_LAFF_LAYER_PB;
   int layerSB   = TO_LAFF_LAYER_SB;
   int layerCB   = TO_LAFF_LAYER_CB;
   int layerCH   = TO_LAFF_LAYER_CH;
   int layerCV   = TO_LAFF_LAYER_CV;
   int layerSH   = TO_LAFF_LAYER_SH;
   int layerSV   = TO_LAFF_LAYER_SV;
   int layerGrid = TO_LAFF_LAYER_IG;
   int layerMap  = TO_LAFF_LAYER_IM;
   int layerPin  = TO_LAFF_LAYER_IP;
   int layerBox  = TO_LAFF_LAYER_BB;

   int colorBlue   = TO_LAFF_COLOR_BLUE;
   int colorRed    = TO_LAFF_COLOR_RED;
   int colorGreen  = TO_LAFF_COLOR_GREEN;
   int colorPink   = TO_LAFF_COLOR_PINK;
   int colorCyan   = TO_LAFF_COLOR_CYAN;
   int colorYellow = TO_LAFF_COLOR_YELLOW;
   int colorWhite  = TO_LAFF_COLOR_WHITE;

   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   double minGrid = MinGrid.GetGrid( );
   unsigned int precision = MinGrid.GetPrecision( );
   unsigned int magnitude = MinGrid.GetMagnitude( );

   printHandler.Write( "(OBJECT 'BARINFO' (\n" );
   printHandler.Write( "  (ATTR 'UNITS' (\n" );
   printHandler.Write( "    (TEXT '%0.*f MICRONS')))\n", precision, 1.0 / static_cast< double >( magnitude ));
   printHandler.Write( "  (ATTR 'MINGRID' (\n" );
   printHandler.Write( "    (TEXT '%0.*f %0.*f')))\n", precision, minGrid, precision, minGrid );
   printHandler.Write( "  (ATTR 'LAYERS' (\n" );

   printHandler.Write( "    (TEXT '%d 1  7 %d %d %s')\n", colorPink,  layerIO, layerIO, TIO_SR_STR( srIO ));
   printHandler.Write( "    (TEXT '%d 1  7 %d %d %s')\n", colorCyan,  layerPB, layerPB, TIO_SR_STR( srPB ));
   printHandler.Write( "    (TEXT '%d 1  7 %d %d %s')\n", colorGreen, layerSB, layerSB, TIO_SR_STR( srSB ));
   printHandler.Write( "    (TEXT '%d 1  7 %d %d %s')\n", colorWhite, layerCB, layerCB, TIO_SR_STR( srCB ));
   printHandler.Write( "    (TEXT '%d 1  7 %d %d %s')\n", colorBlue,  layerCH, layerCH, TIO_SR_STR( srCH ));
   printHandler.Write( "    (TEXT '%d 1  7 %d %d %s')\n", colorRed,   layerCV, layerCV, TIO_SR_STR( srCV ));
   printHandler.Write( "    (TEXT '%d 1  7 %d %d %s')\n", colorBlue,  layerSH, layerSH, TIO_SR_STR( srSH ));
   printHandler.Write( "    (TEXT '%d 1  7 %d %d %s')\n", colorRed,   layerSV, layerSV, TIO_SR_STR( srSV ));

   printHandler.Write( "    (TEXT '%d 1  1 %d %d %s')\n", colorYellow, layerGrid, layerGrid, TIO_SR_STR( srIG ));
   printHandler.Write( "    (TEXT '%d 1  1 %d %d %s')\n", colorYellow, layerMap,  layerMap,  TIO_SR_STR( srIM ));
   printHandler.Write( "    (TEXT '%d 1  1 %d %d %s')\n", colorWhite,  layerPin,  layerPin,  TIO_SR_STR( srIP ));
   printHandler.Write( "    (TEXT '%d 1  0 %d %d %s')\n", colorWhite,  layerBox,  layerBox,  TIO_SR_STR( srBB ));

   printHandler.Write( "  ))\n" );
   printHandler.Write( "))\n" );
}

//===========================================================================//
// Method         : WriteLaffHeader_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffHeader_(
      void ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
   const char* pszSourceName = skinHandler.GetSourceName( );

   printHandler.Write( "(OBJECT '%s' (\n", TIO_PSZ_STR( pszSourceName ));
   printHandler.Write( "  (DETAIL (\n" );
}

//===========================================================================//
// Method         : WriteLaffTrailer_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffTrailer_(
      void ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( "  ))\n" );
   printHandler.Write( "  (BOUNDARY)\n" );
   printHandler.Write( "  (STRUCT)\n" );
   printHandler.Write( "  (PORTS)\n" );
   printHandler.Write( "  (NAMES)\n" );
   printHandler.Write( "  (SYMBOLIC)\n" );
   printHandler.Write( "  (DIAGNOSTIC)\n" );
   printHandler.Write( "))\n" );
}

//===========================================================================//
// Method         : WriteLaffBoundingBox_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffBoundingBox_(
      const TFV_FabricView_c& fabricView ) const
{
   if( fabricView.IsValid( ))
   {
      const TGS_Region_c& region = fabricView.GetRegion( );
      this->WriteLaffBoundary_( TO_LAFF_LAYER_BB, region );
   }
}

//===========================================================================//
// Method         : WriteLaffInternalGrid_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffInternalGrid_(
      const TFV_FabricView_c& fabricView ) const
{
   if( fabricView.IsValid( ))
   {
      const TGS_Region_c& region = fabricView.GetRegion( );

      TGS_ArrayGrid_c arrayGrid( 1.0, 1.0 );
      TGS_ArrayGridIter_c arrayGridIter( arrayGrid, region );
      double x, y;
      while( arrayGridIter.Next( &x, &y ))
      {
         TGS_Point_c gridPoint( x, y );
         arrayGrid.SnapToGrid( gridPoint, &gridPoint );

         TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
         double minGrid = MinGrid.GetGrid( );

         TGS_Line_c horzLine( gridPoint.x - minGrid, gridPoint.y, 
                              gridPoint.x + minGrid, gridPoint.y );
         TGS_Line_c vertLine( gridPoint.x, gridPoint.y - minGrid, 
                              gridPoint.x, gridPoint.y + minGrid );

         this->WriteLaffLine_( TO_LAFF_LAYER_IG, horzLine );
         this->WriteLaffLine_( TO_LAFF_LAYER_IG, vertLine );
      }
   }
}

//===========================================================================//
// Method         : WriteLaffFabricView_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffFabricView_(
      const TFV_FabricView_c& fabricView ) const
{
   if( fabricView.IsValid( ))
   {
      const TGS_LayerRange_t& layerRange = fabricView.GetLayerRange( );
      for( TGS_Layer_t layer = layerRange.i; layer <= layerRange.j; ++layer )
      {
         const TFV_FabricPlane_c& fabricPlane = *fabricView.FindFabricPlane( layer );
         this->WriteLaffFabricPlane_( fabricView, layer, fabricPlane );
      }
   }
}

//===========================================================================//
// Method         : WriteLaffFabricPlane_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffFabricPlane_(
      const TFV_FabricView_c&  fabricView,
            TGS_Layer_t        layer,
      const TFV_FabricPlane_c& fabricPlane ) const
{
   if( fabricPlane.IsValid( ))
   {
      TFV_FabricPlaneIter_t fabricPlaneIter( fabricPlane );
      TFV_FabricFigure_t* pfabricFigure = 0;
      while( fabricPlaneIter.Next( &pfabricFigure, TFV_FIGURE_SOLID ))
      {
         const TGS_Region_c& region = pfabricFigure->GetRegion( );
         const TFV_FabricData_c& fabricData = *pfabricFigure->GetData( );

         const char* pszText = 0;
         char szText[TIO_FORMAT_STRING_LEN_VALUE];
         switch( layer )
         {
         case TFV_LAYER_INPUT_OUTPUT:
         case TFV_LAYER_PHYSICAL_BLOCK:
            pszText = fabricData.GetName( );
            break;

         case TFV_LAYER_SWITCH_BOX:
            pszText = fabricData.GetName( );
            break;

         case TFV_LAYER_CONNECTION_BOX:
            break;

         case TFV_LAYER_CHANNEL_HORZ:
         case TFV_LAYER_CHANNEL_VERT:
            pszText = fabricData.GetName( );
            break;

         case TFV_LAYER_SEGMENT_HORZ:
         case TFV_LAYER_SEGMENT_VERT:
            sprintf( szText, "%u", fabricData.GetTrackIndex( ));
            pszText = szText;
            break;

         default:
            break;
         }

         TO_LaffLayerId_t layerId = this->WriteLaffMapLayerId_( layer );
         this->WriteLaffRegion_( layerId, region, pszText );

         const TC_MapTable_c& mapTable = fabricData.GetMapTable( );
         if( mapTable.IsValid( ))
         {
            this->WriteLaffFabricMapTable_( region, fabricData );
         }

         const TFV_FabricPinList_t& pinList = fabricData.GetPinList( );
         if( pinList.IsValid( ))
         {
            this->WriteLaffFabricPinList_( region, pinList );
         }

         const TFV_ConnectionList_t& connectionList = fabricData.GetConnectionList( );
         if( connectionList.IsValid( ))
         {
            this->WriteLaffFabricConnectionList_( fabricView,
                                                  region, connectionList );
         }
      }
   }
}

//===========================================================================//
// Method         : WriteLaffFabricMapTable_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffFabricMapTable_(
      const TGS_Region_c&     region,
      const TFV_FabricData_c& fabricData ) const
{
   const TC_MapTable_c& mapTable = fabricData.GetMapTable( );

   const TC_MapSideList_t& leftSideList = *mapTable.FindMapSideList( TC_SIDE_LEFT );
   const TC_MapSideList_t& rightSideList = *mapTable.FindMapSideList( TC_SIDE_RIGHT );
   const TC_MapSideList_t& lowerSideList = *mapTable.FindMapSideList( TC_SIDE_LOWER );
   const TC_MapSideList_t& upperSideList = *mapTable.FindMapSideList( TC_SIDE_UPPER );

   this->WriteLaffFabricMapTable_( region, fabricData,
                                   TC_SIDE_LEFT, leftSideList );
   this->WriteLaffFabricMapTable_( region, fabricData,
                                   TC_SIDE_RIGHT, rightSideList);
   this->WriteLaffFabricMapTable_( region, fabricData,
                                   TC_SIDE_LOWER, lowerSideList );
   this->WriteLaffFabricMapTable_( region, fabricData,
                                   TC_SIDE_UPPER, upperSideList );
}

//===========================================================================//
void TO_Output_c::WriteLaffFabricMapTable_(
      const TGS_Region_c&     region,
      const TFV_FabricData_c& fabricData,
            TC_SideMode_t     side,
      const TC_MapSideList_t& mapSideList ) const
{
   for( size_t i = 0; i < mapSideList.GetLength( ); ++i )
   {
      const TC_SideList_t& sideList = *mapSideList[i];
      if( !sideList.IsValid( ))
         continue;

      unsigned int index = static_cast< unsigned int >( i );
      this->WriteLaffFabricMapTable_( region, fabricData,
                                      side, index, sideList );
   }
}

//===========================================================================//
void TO_Output_c::WriteLaffFabricMapTable_(
      const TGS_Region_c&     region,
      const TFV_FabricData_c& fabricData,
            TC_SideMode_t     side,
            unsigned int      index,
      const TC_SideList_t&    sideList ) const
{
   double track1 = fabricData.CalcTrack( region, side, index );

   TGS_Point_c point1;
   switch( side )
   {
   case TC_SIDE_LEFT:  point1.Set( region.x1, track1 ); break;
   case TC_SIDE_RIGHT: point1.Set( region.x2, track1 ); break;
   case TC_SIDE_LOWER: point1.Set( track1, region.y1 ); break;
   case TC_SIDE_UPPER: point1.Set( track1, region.y2 ); break;
   default:                                             break;
   }

   for( size_t i = 0; i < sideList.GetLength( ); ++i )
   {
      const TC_SideIndex_c& sideIndex = *sideList[i];
      side = sideIndex.GetSide( );
      index = static_cast< unsigned int >( sideIndex.GetIndex( ));
      double track2 = fabricData.CalcTrack( region, side, index );

      TGS_Point_c point2;
      switch( side )
      {
      case TC_SIDE_LEFT:  point2.Set( region.x1, track2 ); break;
      case TC_SIDE_RIGHT: point2.Set( region.x2, track2 ); break;
      case TC_SIDE_LOWER: point2.Set( track2, region.y1 ); break;
      case TC_SIDE_UPPER: point2.Set( track2, region.y2 ); break;
      default:                                             break;
      }

      if( point1.IsValid( ) && point2.IsValid( ))
      {
         TGS_Line_c mapLine( point1, point2 );
         this->WriteLaffLine_( TO_LAFF_LAYER_IM, mapLine );
      }
   }
}

//===========================================================================//
// Method         : WriteLaffFabricPinList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffFabricPinList_(
      const TGS_Region_c&        region,
      const TFV_FabricPinList_t& pinList ) const
{
   for( size_t i = 0; i < pinList.GetLength( ); ++i )
   {
      const TFV_FabricPin_c& pin = *pinList[i];
      const char* pszText = pin.GetName( );
      double width = pin.GetWidth( );

      TFV_FabricData_c fabricData;
      TGS_Point_c point = fabricData.CalcPoint( region, pin );

      this->WriteLaffRegion_( TO_LAFF_LAYER_IP, point, width / 2.0, pszText );
   }
}

//===========================================================================//
// Method         : WriteLaffFabricConnectionList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffFabricConnectionList_(
      const TFV_FabricView_c&     fabricView,
      const TGS_Region_c&         region,
      const TFV_ConnectionList_t& connectionList ) const
{
   // Find channel asso. with the given connection box region
   TGS_OrientMode_t orient = region.FindOrient( );
   TGS_Layer_t layer = ( orient == TGS_ORIENT_HORIZONTAL ?
                         TFV_LAYER_CHANNEL_VERT : TFV_LAYER_CHANNEL_HORZ );

   TFV_FabricFigure_t* pfabricFigure = 0;
   if( fabricView.IsSolidAny( layer, region, &pfabricFigure ))
   {
      const TGS_Region_c& channelRegion = pfabricFigure->GetRegion( );
      const TFV_FabricData_c& channelData = *pfabricFigure->GetData( );

      for( size_t i = 0; i < connectionList.GetLength( ); ++i )
      {
         const TFV_Connection_t connection = *connectionList[i];
         double track = channelData.CalcTrack( channelRegion, connection );

         TGS_Point_c point = region.FindCenter( TGS_SNAP_MIN_GRID );
         switch( connection.GetSide( ))
         {
         case TC_SIDE_LEFT:  
         case TC_SIDE_RIGHT: point.x = track; break;
         case TC_SIDE_LOWER: 
         case TC_SIDE_UPPER: point.y = track; break;
         default:                             break;
         }

         // Display a 'X' marker on the connection box layer
         double width = region.FindWidth( );
         this->WriteLaffMarker_( TO_LAFF_LAYER_CB, point, width );
      }
   }
}

//===========================================================================//
// Method         : WriteLaffBoundary_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffBoundary_(
            TO_LaffLayerId_t layerId,
      const TGS_Region_c&    region ) const
{
   if( region.IsValid( ))
   {
      TGS_Line_c leftLine( region.x1, region.y1, region.x1, region.y2 );
      TGS_Line_c rightLine( region.x2, region.y1, region.x2, region.y2 );
      TGS_Line_c lowerLine( region.x1, region.y1, region.x2, region.y1 );
      TGS_Line_c upperLine( region.x1, region.y2, region.x2, region.y2 );
   
      this->WriteLaffLine_( layerId, leftLine );
      this->WriteLaffLine_( layerId, rightLine );
      this->WriteLaffLine_( layerId, lowerLine );
      this->WriteLaffLine_( layerId, upperLine );
   }
}

//===========================================================================//
// Method         : WriteLaffMarker_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffMarker_(
            TO_LaffLayerId_t layerId,
      const TGS_Point_c&     point,
            double           length ) const
{
   TGS_Line_c foreSlash( point.x - length, point.y - length,
                         point.x + length, point.y + length );
   TGS_Line_c backSlash( point.x - length, point.y + length,
                         point.x + length, point.y - length );

   this->WriteLaffLine_( layerId, foreSlash );
   this->WriteLaffLine_( layerId, backSlash );
}

//===========================================================================//
// Method         : WriteLaffLine_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffLine_(
            TO_LaffLayerId_t layerId,
      const TGS_Line_c&      line,
      const char*            pszText ) const
{
   if( line.IsValid( ))
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      int magnitude = static_cast< int >( MinGrid.GetMagnitude( ));
      int units = static_cast< int >( MinGrid.GetUnits( ));

      long int x1 = magnitude / units * MinGrid.FloatToLongInt( line.x1 );
      long int y1 = magnitude / units * MinGrid.FloatToLongInt( line.y1 ); 
      long int x2 = magnitude / units * MinGrid.FloatToLongInt( line.x2 );
      long int y2 = magnitude / units * MinGrid.FloatToLongInt( line.y2 ); 
      int width = 0;

      if( pszText )
      {
         unsigned int height = ( units >= 100 ? 16 : 4 );
         long int x = (( line.FindOrient( ) == TGS_ORIENT_HORIZONTAL ) ?
                       (( x2 - x1 ) / 2 + x1 ) : x1 );
         long int y = (( line.FindOrient( ) == TGS_ORIENT_VERTICAL ) ?
                       (( y2 - y1 ) / 2 + y1 ) : y1 );

         printHandler.Write( "    (CLF %d %d %ld %ld %ld %ld (\n", layerId, width, x1, y1, x2, y2 );
         if( strlen( pszText ) <= 256 )
         {
            printHandler.Write( "      (SNAM %d %u 1 %ld %ld '%s')))\n", layerId, height, x, y, pszText );
         }
         else
         {
            printHandler.Write( "      (SNAM %d %u 1 %ld %ld '%0.255s...')))\n", layerId, height, x, y, pszText );
         }
      }
      else
      {
         printHandler.Write( "    (CLF %d %d %ld %ld %ld %ld)\n", layerId, width, x1, y1, x2, y2 );
      }
   }
}

//===========================================================================//
void TO_Output_c::WriteLaffLine_(
            TO_LaffLayerId_t layerId,
      const TGS_Line_c&      line,
      const string&          srText ) const
{
   this->WriteLaffLine_( layerId, line, srText.data( ));
}

//===========================================================================//
// Method         : WriteLaffRegion_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
void TO_Output_c::WriteLaffRegion_(
            TO_LaffLayerId_t layerId,
      const TGS_Region_c&    region,
      const char*            pszText ) const
{
   if( region.IsValid( ) && region.HasArea( ))
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      int magnitude = static_cast< int >( MinGrid.GetMagnitude( ));
      int units = static_cast< int >( MinGrid.GetUnits( ));

      long int x1 = magnitude / units * MinGrid.FloatToLongInt( region.x1 );
      long int y1 = magnitude / units * MinGrid.FloatToLongInt( region.y1 );
      long int x2 = magnitude / units * MinGrid.FloatToLongInt( region.x2 );
      long int y2 = magnitude / units * MinGrid.FloatToLongInt( region.y2 );

      if( pszText )
      {
         unsigned int height = ( units >= 100 ? 16 : 4 );

         printHandler.Write( "    (RECT %d %ld %ld %ld %ld (\n", layerId, x1, y1, x2, y2 );
         if( strlen( pszText ) <= 256 )
         {
            printHandler.Write( "      (SNAM %d %u 1 %ld %ld '%s')))\n", layerId, height, x1, y1, pszText );
         }
         else
         {
            printHandler.Write( "      (SNAM %d %u 1 %ld %ld '%0.255s...')))\n", layerId, height, x1, y1, pszText );
         }
      }
      else
      {
         printHandler.Write( "    (RECT %d %ld %ld %ld %ld)\n", layerId, x1, y1, x2, y2 );
      }
   }
}

//===========================================================================//
void TO_Output_c::WriteLaffRegion_(
            TO_LaffLayerId_t layerId,
      const TGS_Region_c&    region,
      const string&          srText ) const
{
   this->WriteLaffRegion_( layerId, region, srText.data( ));
}

//===========================================================================//
void TO_Output_c::WriteLaffRegion_(
            TO_LaffLayerId_t layerId,
      const TGS_Point_c&     point,
            double           scale,
      const char*            pszText ) const
{
   TGS_Region_c region( point, point, scale, TGS_SNAP_MIN_GRID );
   this->WriteLaffRegion_( layerId, region, pszText );
}

//===========================================================================//
void TO_Output_c::WriteLaffRegion_(
            TO_LaffLayerId_t layerId,
      const TGS_Point_c&     point,
            double           scale,
      const string&          srText ) const
{
   this->WriteLaffRegion_( layerId, point, scale, srText.data( ));
}

//===========================================================================//
// Method         : WriteLaffMapLayerId_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/30/12 jeffr : Original
//===========================================================================//
TO_LaffLayerId_t TO_Output_c::WriteLaffMapLayerId_(
      TGS_Layer_t layer ) const
{
   TO_LaffLayerId_t layerId = TO_LAFF_LAYER_UNDEFINED;

   switch( layer )
   {
   case TFV_LAYER_INPUT_OUTPUT:   layerId = TO_LAFF_LAYER_IO; break;
   case TFV_LAYER_PHYSICAL_BLOCK: layerId = TO_LAFF_LAYER_PB; break;
   case TFV_LAYER_SWITCH_BOX:     layerId = TO_LAFF_LAYER_SB; break;
   case TFV_LAYER_CONNECTION_BOX: layerId = TO_LAFF_LAYER_CB; break;
   case TFV_LAYER_CHANNEL_HORZ:   layerId = TO_LAFF_LAYER_CH; break;
   case TFV_LAYER_CHANNEL_VERT:   layerId = TO_LAFF_LAYER_CV; break;
   case TFV_LAYER_SEGMENT_HORZ:   layerId = TO_LAFF_LAYER_SH; break;
   case TFV_LAYER_SEGMENT_VERT:   layerId = TO_LAFF_LAYER_SV; break;
   default:                       layerId = TO_LAFF_LAYER_UNDEFINED; break;
   }
   return( layerId );
}
