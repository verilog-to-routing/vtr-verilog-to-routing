//===========================================================================//
// Purpose : Method definitions for the TAS_Segment class.
//
//           Public methods include:
//           - TAS_Segment_c, ~TAS_Segment_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
//
//===========================================================================//

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TAS_StringUtils.h"

#include "TAS_Segment.h"

//===========================================================================//
// Method         : TAS_Segment_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_Segment_c::TAS_Segment_c( 
      void )
      :
      length( 0 ),
      dirType( TAS_SEGMENT_DIR_UNDEFINED ),
      trackFreq( 0.0 ),
      trackPercent( 0.0 )
{
   this->timing.res = 0.0;
   this->timing.cap = 0.0;
}

//===========================================================================//
TAS_Segment_c::TAS_Segment_c( 
      const TAS_Segment_c& segment )
      :
      length( segment.length ),
      dirType( segment.dirType ),
      trackFreq( segment.trackFreq ),
      trackPercent( segment.trackPercent ),
      sbPattern( segment.sbPattern ),
      cbPattern( segment.cbPattern ),
      srMuxSwitchName( segment.srMuxSwitchName ),
      srWireSwitchName( segment.srWireSwitchName ),
      srOutputSwitchName( segment.srOutputSwitchName )
{
   this->timing.res = segment.timing.res;
   this->timing.cap = segment.timing.cap;
}

//===========================================================================//
// Method         : ~TAS_Segment_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_Segment_c::~TAS_Segment_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_Segment_c& TAS_Segment_c::operator=( 
      const TAS_Segment_c& segment )
{
   if( &segment != this )
   {
      this->length = segment.length;
      this->dirType = segment.dirType;
      this->trackFreq = segment.trackFreq;
      this->trackPercent = segment.trackPercent;
      this->sbPattern = segment.sbPattern;
      this->cbPattern = segment.cbPattern;
      this->srMuxSwitchName = segment.srMuxSwitchName;
      this->srWireSwitchName = segment.srWireSwitchName;
      this->srOutputSwitchName = segment.srOutputSwitchName;
      this->timing.res = segment.timing.res;
      this->timing.cap = segment.timing.cap;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_Segment_c::operator==( 
      const TAS_Segment_c& segment ) const
{
   return(( this->length == segment.length ) &&
          ( this->dirType == segment.dirType ) &&
          ( TCTF_IsEQ( this->trackFreq, segment.trackFreq )) &&
          ( TCTF_IsEQ( this->trackPercent, segment.trackPercent )) &&
          ( this->sbPattern == segment.sbPattern ) &&
          ( this->cbPattern == segment.cbPattern ) &&
          ( this->srMuxSwitchName == segment.srMuxSwitchName ) &&
          ( this->srWireSwitchName == segment.srWireSwitchName ) &&
          ( this->srOutputSwitchName == segment.srOutputSwitchName ) &&
          ( TCTF_IsEQ( this->timing.res, segment.timing.res )) &&
          ( TCTF_IsEQ( this->timing.cap, segment.timing.cap )) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_Segment_c::operator!=( 
      const TAS_Segment_c& segment ) const
{
   return( !this->operator==( segment ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_Segment_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<segment " );

   if( this->length != UINT_MAX )
   {
      printHandler.Write( pfile, 0, "length = %u ", this->length );
   }
   else
   {
      printHandler.Write( pfile, 0, "length = longline " );
   }
   printHandler.Write( pfile, 0, ">\n" );

   if( TCTF_IsGT( this->timing.res, 0.0 ) ||
       TCTF_IsGT( this->timing.cap, 0.0 ))
   {
      printHandler.Write( pfile, spaceLen, "<timing\n" );
      spaceLen += 3;

      if( TCTF_IsGT( this->timing.res, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "res = %0.*f\n", precision, this->timing.res );
      }
      if( TCTF_IsGT( this->timing.cap, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "cap = %0.*f\n", precision, this->timing.cap );
      }

      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "/>\n" );
   }
   printHandler.Write( pfile, spaceLen, "</segment>\n" );
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_Segment_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_Segment_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<segment " );
   if( this->length != UINT_MAX )
   {
      printHandler.Write( pfile, 0, "length=\"%u\" ",
	   	          this->length );
   }
   else
   {
      printHandler.Write( pfile, 0, "length=\"longline\" " );
   }

   string srDirType;
   TAS_ExtractStringSegmentDirType( this->dirType, &srDirType );
   printHandler.Write( pfile, 0, "type=\"%s\" freq=\"%0.*f\" Rmetal=\"%0.*f\" Cmetal=\"%0.*e\">\n",
		       TIO_SR_STR( srDirType ),
		       precision, this->trackFreq,
		       precision, this->timing.res,
		       precision + 1, this->timing.cap );
   spaceLen += 3;

   if( this->sbPattern.IsValid( ))
   {
      string srSwitchBoxPattern;
      this->sbPattern.ExtractString( &srSwitchBoxPattern );
      printHandler.Write( pfile, spaceLen, "<sb type=\"pattern\">%s</sb>\n",
 			                   TIO_SR_STR( srSwitchBoxPattern ));
   }
   if( this->cbPattern.IsValid( ))
   {
      string srConnectionBoxPattern;
      this->cbPattern.ExtractString( &srConnectionBoxPattern );
      printHandler.Write( pfile, spaceLen, "<cb type=\"pattern\">%s</cb>\n",
 			                   TIO_SR_STR( srConnectionBoxPattern ));
   }
   if( this->srMuxSwitchName.length( ))
   {
      printHandler.Write( pfile, spaceLen, "<mux name=\"%s\"/>\n", 
	   	                           TIO_SR_STR( this->srMuxSwitchName ));
   }
   if( this->srWireSwitchName.length( ))
   {
      printHandler.Write( pfile, spaceLen, "<wire_switch name=\"%s\"/>\n", 
	   	                           TIO_SR_STR( this->srWireSwitchName ));
   }
   if( this->srOutputSwitchName.length( ))
   {
      printHandler.Write( pfile, spaceLen, "<opin_switch name=\"%s\"/>\n", 
	   	                           TIO_SR_STR( this->srOutputSwitchName ));
   }
   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</segment>\n" );
}
