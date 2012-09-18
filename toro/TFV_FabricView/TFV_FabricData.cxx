//===========================================================================//
// Purpose : Method definitions for the TFV_FabricData class.
//
//           Public methods include:
//           - TFV_FabricData_c, ~TFV_FabricData_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - ExtractString
//           - CalcTrack
//           - CalcPoint
//
//===========================================================================//

#include "TIO_PrintHandler.h"

#include "TC_Typedefs.h"
#include "TCT_Generic.h"

#include "TFV_StringUtils.h"
#include "TFV_FabricData.h"

//===========================================================================//
// Method         : TFV_FabricData_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricData_c::TFV_FabricData_c( 
      void )
      :
      dataType_( TFV_DATA_UNDEFINED ),
      pinList_( TFV_PIN_LIST_DEF_CAPACITY ),
      connectionList_( TFV_CONNECTION_LIST_DEF_CAPACITY )
{
   this->track_.index = 0;
   this->track_.horzCount = 0;
   this->track_.vertCount = 0;

   this->slice_.count = 0;
   this->slice_.capacity = 0;

   this->timing_.res = 0.0;
   this->timing_.capInput = 0.0;
   this->timing_.capOutput = 0.0;
   this->timing_.delay = 0.0;
}

//===========================================================================//
TFV_FabricData_c::TFV_FabricData_c( 
      TFV_DataType_t dataType )
      :
      dataType_( dataType ),
      pinList_( TFV_PIN_LIST_DEF_CAPACITY ),
      connectionList_( TFV_CONNECTION_LIST_DEF_CAPACITY )
{
   this->track_.index = 0;
   this->track_.horzCount = 0;
   this->track_.vertCount = 0;

   this->slice_.count = 0;
   this->slice_.capacity = 0;

   this->timing_.res = 0.0;
   this->timing_.capInput = 0.0;
   this->timing_.capOutput = 0.0;
   this->timing_.delay = 0.0;
}

//===========================================================================//
TFV_FabricData_c::TFV_FabricData_c( 
      const TFV_FabricData_c& fabricData )
      :
      dataType_( fabricData.dataType_ ),
      srName_( fabricData.srName_ ),
      srMasterName_( fabricData.srMasterName_ ),
      pinList_( fabricData.pinList_ ),
      connectionList_( fabricData.connectionList_ ),
      mapTable_( fabricData.mapTable_ )
{
   this->track_.index = fabricData.track_.index;
   this->track_.horzCount = fabricData.track_.horzCount;
   this->track_.vertCount = fabricData.track_.vertCount;

   this->slice_.count = fabricData.slice_.count;
   this->slice_.capacity = fabricData.slice_.capacity;

   this->timing_.res = fabricData.timing_.res;
   this->timing_.capInput = fabricData.timing_.capInput;
   this->timing_.capOutput = fabricData.timing_.capOutput;
   this->timing_.delay = fabricData.timing_.delay;
}

//===========================================================================//
// Method         : ~TFV_FabricData_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TFV_FabricData_c::~TFV_FabricData_c( 
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
TFV_FabricData_c& TFV_FabricData_c::operator=( 
      const TFV_FabricData_c& fabricData )
{
   if( &fabricData != this )
   {
      this->dataType_ = fabricData.dataType_;
      this->srName_ = fabricData.srName_;
      this->srMasterName_ = fabricData.srMasterName_;
      this->track_.index = fabricData.track_.index;
      this->track_.horzCount = fabricData.track_.horzCount;
      this->track_.vertCount = fabricData.track_.vertCount;
      this->slice_.count = fabricData.slice_.count;
      this->slice_.capacity = fabricData.slice_.capacity;
      this->timing_.res = fabricData.timing_.res;
      this->timing_.capInput = fabricData.timing_.capInput;
      this->timing_.capOutput = fabricData.timing_.capOutput;
      this->timing_.delay = fabricData.timing_.delay;
      this->pinList_ = fabricData.pinList_;
      this->connectionList_ = fabricData.connectionList_;
      this->mapTable_ = fabricData.mapTable_;
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
bool TFV_FabricData_c::operator==( 
      const TFV_FabricData_c& fabricData ) const
{
   return(( this->dataType_ == fabricData.dataType_ ) &&
          ( this->srName_ == fabricData.srName_ ) &&
          ( this->srMasterName_ == fabricData.srMasterName_ ) &&
          ( this->track_.index == fabricData.track_.index ) &&
          ( this->track_.horzCount == fabricData.track_.horzCount ) &&
          ( this->track_.vertCount == fabricData.track_.vertCount ) &&
          ( this->slice_.count == fabricData.slice_.count ) &&
          ( this->slice_.capacity == fabricData.slice_.capacity ) &&
 	  ( TCTF_IsEQ( this->timing_.res, fabricData.timing_.res )) &&
          ( TCTF_IsEQ( this->timing_.capInput, fabricData.timing_.capInput )) &&
          ( TCTF_IsEQ( this->timing_.capOutput, fabricData.timing_.capOutput )) &&
          ( TCTF_IsEQ( this->timing_.delay, fabricData.timing_.delay )) &&
          ( this->pinList_ == fabricData.pinList_ ) &&
          ( this->connectionList_ == fabricData.connectionList_ ) &&
          ( this->mapTable_ == fabricData.mapTable_ ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
bool TFV_FabricData_c::operator!=( 
      const TFV_FabricData_c& fabricData ) const
{
   return( !this->operator==( fabricData ) ? 
           true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TFV_FabricData_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srFabricData;
   this->ExtractString( &srFabricData );

   printHandler.Write( pfile, spaceLen, "[fabric] %s", TIO_SR_STR( srFabricData ));
}

//===========================================================================//
// Method         : ExtractString
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
void TFV_FabricData_c::ExtractString( 
      string* psrData ) const
{
   if( psrData )
   {
      TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
      unsigned int precision = MinGrid.GetPrecision( );

      string srDataType;
      TFV_ExtractStringDataType( this->dataType_, &srDataType );

      *psrData = srDataType;

      char szTrack[TIO_FORMAT_STRING_LEN_DATA];
      char szSlice[TIO_FORMAT_STRING_LEN_DATA];
      char szTiming[TIO_FORMAT_STRING_LEN_DATA];

      switch( this->dataType_ )
      {
      case TFV_DATA_PHYSICAL_BLOCK:
      case TFV_DATA_INPUT_OUTPUT:

         sprintf( szSlice, "%u %u",
                           this->slice_.count, 
	   	           this->slice_.capacity );
         sprintf( szTiming, "%0.*f %0.*e",
                            precision, this->timing_.capInput,
                            precision + 1, this->timing_.delay );
         *psrData += " ";
         *psrData += this->srName_;
         *psrData += " ";
         *psrData += "(";
         *psrData += this->srMasterName_;
         *psrData += ")";
         *psrData += " ";
         *psrData += szSlice;
         *psrData += " ";
         *psrData += szTiming;
	 break;

      case TFV_DATA_SWITCH_BOX:

         sprintf( szTiming, "[%0.*f %0.*f %0.*f %0.*e]",
                            precision, this->timing_.res,
                            precision, this->timing_.capInput,
                            precision, this->timing_.capOutput,
                            precision + 1, this->timing_.delay );
         *psrData += " ";
         *psrData += this->srName_;
         *psrData += " ";
         *psrData += szTiming;
	 break;

      case TFV_DATA_CONNECTION_BOX:
	 break;

      case TFV_DATA_CHANNEL_HORZ:
      case TFV_DATA_CHANNEL_VERT:

         sprintf( szTrack, "%u %u",
                           this->track_.horzCount, 
	   	           this->track_.vertCount ); 
         *psrData += " ";
         *psrData += this->srName_;
         *psrData += " ";
         *psrData += szTrack;
	 break;

      case TFV_DATA_SEGMENT_HORZ:
      case TFV_DATA_SEGMENT_VERT:

         sprintf( szTrack, "%u",
                           this->track_.index );
         sprintf( szTiming, "[%0.*f %0.*e]",
                            precision, this->timing_.res,
		            precision, this->timing_.capInput );
         *psrData += " ";
         *psrData += szTrack;
         *psrData += " ";
         *psrData += szTiming;
	 break;

      default:
	 break;
      }
   }
}

//===========================================================================//
// Method         : CalcTrack
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
double TFV_FabricData_c::CalcTrack(
      const TGS_Region_c&   region,
      const TC_SideIndex_c& sideIndex ) const
{
   TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED;
   switch( sideIndex.GetSide( ))
   {
   case TC_SIDE_LEFT:
   case TC_SIDE_RIGHT: orient = TGS_ORIENT_VERTICAL;   break;
   case TC_SIDE_LOWER:
   case TC_SIDE_UPPER: orient = TGS_ORIENT_HORIZONTAL; break;
   default:                                            break;
   }
   unsigned int index = static_cast< unsigned int >( sideIndex.GetIndex( ));

   return( this->CalcTrack( region, orient, index ));
}

//===========================================================================//
double TFV_FabricData_c::CalcTrack(
      const TGS_Region_c& region,
            TC_SideMode_t side,
            unsigned int  index ) const
{
   TGS_OrientMode_t orient = TGS_ORIENT_UNDEFINED;
   switch( side )
   {
   case TC_SIDE_LEFT:
   case TC_SIDE_RIGHT: orient = TGS_ORIENT_HORIZONTAL; break;
   case TC_SIDE_LOWER:
   case TC_SIDE_UPPER: orient = TGS_ORIENT_VERTICAL;   break;
   default:                                            break;
   }
   return( this->CalcTrack( region, orient, index ));
}

//===========================================================================//
double TFV_FabricData_c::CalcTrack(
      const TGS_Region_c&    region,
            TGS_OrientMode_t orient,
            unsigned int     index ) const
{
   TGS_Region_c region_( region );
   TGS_Point_c center_ = region_.FindCenter( TGS_SNAP_MIN_GRID );

   double width = TFV_MODEL_SEGMENT_DEF_WIDTH;
   double spacing = TFV_MODEL_SEGMENT_DEF_SPACING;

   if( orient == TGS_ORIENT_HORIZONTAL )
   {
      unsigned int count = this->track_.horzCount;
      if( count )
      {
         double y = center_.y - count / 2 * ( width + spacing );
         double y1 = y + spacing / 2.0 + index * ( width + spacing );
         double y2 = y1 + width;
         region_.Set( region_.x1, y1, region_.x2, y2 );
      }
   }
   if( orient == TGS_ORIENT_VERTICAL )
   {
      unsigned int count = this->track_.vertCount;
      if( count )
      {
         double x = center_.x - count / 2 * ( width + spacing );
         double x1 = x + spacing / 2.0 + index * ( width + spacing );
         double x2 = x1 + width;
         region_.Set( x1, region_.y1, x2, region_.y2 );
      }
   }
   center_ = region_.FindCenter( TGS_SNAP_MIN_GRID );

   double track = static_cast< double >( index );
   if( orient == TGS_ORIENT_HORIZONTAL )
   {
      track = center_.y;
   }
   if( orient == TGS_ORIENT_VERTICAL )
   {
      track = center_.x;
   }
   return( track );
}

//===========================================================================//
// Method         : CalcPoint
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 08/15/12 jeffr : Original
//===========================================================================//
TGS_Point_c TFV_FabricData_c::CalcPoint(
      const TGS_Region_c&    region,
      const TFV_FabricPin_c& pin ) const
{
   return( this->CalcPoint( region,
                            pin.GetSide( ),
                            pin.GetOffset( ),
                            pin.GetWidth( )));
}

//===========================================================================//
TGS_Point_c TFV_FabricData_c::CalcPoint(
      const TGS_Region_c& region,
            TC_SideMode_t side,
            double        offset,
            double        width ) const
{
   double x = 0.0;
   double y = 0.0;

   switch( side )
   {
   case TC_SIDE_LEFT:
      x = region.x1 + width / 2.0;
      y = region.y1 + width / 2.0 + offset;
      break;
   case TC_SIDE_RIGHT:
      x = region.x2 - width / 2.0;
      y = region.y1 + width / 2.0 + offset;
      break;
   case TC_SIDE_LOWER:
      x = region.x1 + width / 2.0 + offset;
      y = region.y1 + width / 2.0;
      break;
   case TC_SIDE_UPPER:
      x = region.x1 + width / 2.0 + offset;
      y = region.y2 - width / 2.0;
      break;
   default:
      break;
   }

   TGS_Point_c point( x, y, TGS_SNAP_MIN_GRID );
   return( point );
}
