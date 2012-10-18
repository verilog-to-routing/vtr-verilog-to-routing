//===========================================================================//
// Purpose : Method definitions for the TAS_TimingDelay class.
//
//           Public methods include:
//           - TAS_TimingDelay_c, ~TAS_TimingDelay_c
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

#include "TAS_TimingDelay.h"

//===========================================================================//
// Method         : TAS_TimingDelay_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_TimingDelay_c::TAS_TimingDelay_c( 
      void )
      :
      type( TAS_TIMING_DELAY_UNDEFINED ),
      delay( 0.0 ),
      delayMatrix( TAS_DELAY_MATRIX_DEF_CAPACITY, 
                   TAS_DELAY_MATRIX_DEF_CAPACITY )
{
}

//===========================================================================//
TAS_TimingDelay_c::TAS_TimingDelay_c( 
      const TAS_TimingDelay_c& timingDelay )
      :
      type( timingDelay.type ),
      delay( timingDelay.delay ),
      delayMatrix( timingDelay.delayMatrix ),
      srInputPortName( timingDelay.srInputPortName ),
      srOutputPortName( timingDelay.srOutputPortName ),
      srClockPortName( timingDelay.srClockPortName )
{
}

//===========================================================================//
// Method         : ~TAS_TimingDelay_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_TimingDelay_c::~TAS_TimingDelay_c( 
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
TAS_TimingDelay_c& TAS_TimingDelay_c::operator=( 
      const TAS_TimingDelay_c& timingDelay )
{
   if( &timingDelay != this )
   {
      this->type = timingDelay.type;
      this->delay = timingDelay.delay;
      this->delayMatrix = timingDelay.delayMatrix;
      this->srInputPortName = timingDelay.srInputPortName;
      this->srOutputPortName = timingDelay.srOutputPortName;
      this->srClockPortName = timingDelay.srClockPortName;
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
bool TAS_TimingDelay_c::operator==( 
      const TAS_TimingDelay_c& timingDelay ) const
{
   return(( this->type == timingDelay.type ) &&
          ( TCTF_IsEQ( this->delay, timingDelay.delay )) &&
          ( this->delayMatrix == timingDelay.delayMatrix ) &&
          ( this->srInputPortName == timingDelay.srInputPortName ) &&
          ( this->srOutputPortName == timingDelay.srOutputPortName ) &&
          ( this->srClockPortName == timingDelay.srClockPortName ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_TimingDelay_c::operator!=( 
      const TAS_TimingDelay_c& timingDelay ) const
{
   return( !this->operator==( timingDelay ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_TimingDelay_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int expPrecision = MinGrid.GetPrecision( ) + 1;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<timing " );

   string srType;
   TAS_ExtractStringTimingDelayType( this->type, &srType );
   printHandler.Write( pfile, 0, "%s ", TIO_SR_STR( srType ));

   if( this->type == TAS_TIMING_DELAY_CONSTANT )
   {
      printHandler.Write( pfile, 0, "input = \"%s\" ", TIO_SR_STR( this->srInputPortName ));
      printHandler.Write( pfile, 0, "output = \"%s\" ", TIO_SR_STR( this->srOutputPortName ));
      printHandler.Write( pfile, 0, "delay = %0.*e ", expPrecision, this->delay );
      printHandler.Write( pfile, 0, "/>\n" );
   }
   else if( this->type == TAS_TIMING_DELAY_SETUP )
   {
      printHandler.Write( pfile, 0, "input = \"%s\" ", TIO_SR_STR( this->srInputPortName ));
      printHandler.Write( pfile, 0, "clock = \"%s\" ", TIO_SR_STR( this->srClockPortName ));
      printHandler.Write( pfile, 0, "delay = %0.*e ", expPrecision, this->delay );
      printHandler.Write( pfile, 0, "/>\n" );
   }
   else if( this->type == TAS_TIMING_DELAY_CLOCK_TO_Q )
   {
      printHandler.Write( pfile, 0, "output = \"%s\" ", TIO_SR_STR( this->srOutputPortName ));
      printHandler.Write( pfile, 0, "clock = \"%s\" ", TIO_SR_STR( this->srClockPortName ));
      printHandler.Write( pfile, 0, "delay = %0.*e ", expPrecision, this->delay );
      printHandler.Write( pfile, 0, "/>\n" );
   }
   else if( this->type == TAS_TIMING_DELAY_MATRIX )
   {
      printHandler.Write( pfile, 0, "input = \"%s\" ", TIO_SR_STR( this->srInputPortName ));
      printHandler.Write( pfile, 0, "output = \"%s\" ", TIO_SR_STR( this->srOutputPortName ));
      printHandler.Write( pfile, 0, "\n" );
      spaceLen += 3;

      string srDelayMatrix;
      this->delayMatrix.ExtractString( TC_DATA_EXP, &srDelayMatrix, 
                                       4, SIZE_MAX, spaceLen + 8, 0 );
      printHandler.Write( pfile, spaceLen, "delay = %s", TIO_SR_STR( srDelayMatrix ));

      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "/>\n" );
   }
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_TimingDelay_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_TimingDelay_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int expPrecision = MinGrid.GetPrecision( ) + 1;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   switch( this->type )
   {
   case TAS_TIMING_DELAY_CONSTANT:
      printHandler.Write( pfile, spaceLen, "<delay_constant max=\"%0.*e\" in_port=\"%s\" out_port=\"%s\"/>\n",
                                           expPrecision, this->delay,
                                           TIO_SR_STR( this->srInputPortName ),
                                           TIO_SR_STR( this->srOutputPortName ));
      break;

   case TAS_TIMING_DELAY_MATRIX:
      printHandler.Write( pfile, spaceLen, "<delay_matrix type=\"max\" in_port=\"%s\" out_port=\"%s\">\n",
                                           TIO_SR_STR( this->srInputPortName ),
                                           TIO_SR_STR( this->srOutputPortName ));

      for( size_t j = 0; j < this->delayMatrix.GetHeight( ); ++j )
      {
         printHandler.Write( pfile, spaceLen + 3, "" );
         for( size_t i = 0; i < this->delayMatrix.GetWidth( ); ++i )
         {
            printHandler.Write( pfile, 0, "%0.*e%s",
                                          expPrecision, this->delayMatrix[i][j],
                                          i + 1 == this->delayMatrix.GetWidth( ) ? "" : " " );
         }
         printHandler.Write( pfile, 0, "\n" );
      }
      printHandler.Write( pfile, spaceLen, "</delay_matrix>\n" );
      break;

   case TAS_TIMING_DELAY_SETUP:
      printHandler.Write( pfile, spaceLen, "<T_setup value=\"%0.*e\" port=\"%s\" clock=\"%s\"/>\n",
                                           expPrecision, this->delay,
                                           TIO_SR_STR( this->srOutputPortName ),
                                           TIO_SR_STR( this->srClockPortName ));
      break;

   case TAS_TIMING_DELAY_CLOCK_TO_Q:
      printHandler.Write( pfile, spaceLen, "<T_clock_to_Q max=\"%0.*e\" port=\"%s\" clock=\"%s\"/>\n",
                                           expPrecision, this->delay,
                                           TIO_SR_STR( this->srOutputPortName ),
                                           TIO_SR_STR( this->srClockPortName ));
      break;

   case TAS_TIMING_DELAY_UNDEFINED:
      break;
   }
}
