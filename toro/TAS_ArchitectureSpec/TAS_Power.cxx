//===========================================================================//
// Purpose : Method definitions for the TAS_Power class.
//
//           Public methods include:
//           - TAS_Power_c, ~TAS_Power_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
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

#include "TC_MinGrid.h"
#include "TCT_Generic.h"

#include "TIO_PrintHandler.h"

#include "TAS_StringUtils.h"

#include "TAS_Power.h"

//===========================================================================//
// Method         : TAS_Power_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TAS_Power_c::TAS_Power_c( 
      void )
      :
      estimateMethod( TAS_POWER_METHOD_UNDEFINED ),
      portList( TAS_POWER_PORT_LIST_DEF_CAPACITY )

{
   this->staticPower.absolute = 0.0;
   this->dynamicPower.absolute = 0.0;
   this->dynamicPower.capInternal = 0.0;
}

//===========================================================================//
TAS_Power_c::TAS_Power_c( 
      const TAS_Power_c& power )
      :
      estimateMethod( power.estimateMethod ),
      portList( power.portList )
{
   this->staticPower.absolute = power.staticPower.absolute;
   this->dynamicPower.absolute = power.dynamicPower.absolute;
   this->dynamicPower.capInternal = power.dynamicPower.capInternal;
}

//===========================================================================//
// Method         : ~TAS_Power_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TAS_Power_c::~TAS_Power_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
TAS_Power_c& TAS_Power_c::operator=( 
      const TAS_Power_c& power )
{
   if( &power != this )
   {
      this->estimateMethod = power.estimateMethod;
      this->staticPower.absolute = power.staticPower.absolute;
      this->dynamicPower.absolute = power.dynamicPower.absolute;
      this->dynamicPower.capInternal = power.dynamicPower.capInternal;
      this->portList = power.portList;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TAS_Power_c::operator==( 
      const TAS_Power_c& power ) const
{
   return(( this->estimateMethod == power.estimateMethod ) &&
          ( TCTF_IsEQ( this->staticPower.absolute, power.staticPower.absolute )) &&
          ( TCTF_IsEQ( this->dynamicPower.absolute, power.dynamicPower.absolute )) &&
          ( TCTF_IsEQ( this->dynamicPower.capInternal, power.dynamicPower.capInternal )) &&
          ( this->portList == power.portList ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
bool TAS_Power_c::operator!=( 
      const TAS_Power_c& power ) const
{
   return( !this->operator==( power ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TAS_Power_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<power" );
   spaceLen += 3;

   string srEstimateMethod;
   TAS_ExtractStringPowerMethodMode( this->estimateMethod, &srEstimateMethod );
   printHandler.Write( pfile, 0, " method=\"%s\"", 
                                 TIO_SR_STR( srEstimateMethod ));

   if( TCTF_IsGT( this->staticPower.absolute, 0.0 ) ||
       TCTF_IsGT( this->dynamicPower.absolute, 0.0 ) ||
       this->portList.IsValid( ))
   {
      printHandler.Write( pfile, 0, ">\n" );

      if( TCTF_IsGT( this->staticPower.absolute, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "<static_power power_per_instance=\"%0.*f\"/>\n", 
                                              precision, this->staticPower.absolute );
      }
      if( TCTF_IsGT( this->dynamicPower.absolute, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "<dynamic_power power_per_instance=\"%0.*f\"/>\n", 
                                              precision, this->dynamicPower.absolute );
      }
      if( TCTF_IsGT( this->dynamicPower.capInternal, 0.0 ))
      {
         printHandler.Write( pfile, spaceLen, "<dynamic_power C_internal=\"%0.*e\"/>\n", 
                                              precision + 1, this->dynamicPower.capInternal );
      }
      for( size_t i = 0; i < this->portList.GetLength( ); ++i )
      {
         const TLO_Port_c& port = *this->portList[i];
         const char* pszName = port.GetName( );
         const TLO_Power_c& power = port.GetPower( );
   
         printHandler.Write( pfile, spaceLen, "<port name=\"%s\" ", TIO_PSZ_STR( pszName ));
         power.Print( pfile, 0 );
         printHandler.Write( pfile, 0, "/>\n" );
      }
      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "</power>\n" );
   }
   else
   {
      printHandler.Write( pfile, 0, "/>\n" );
   }
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/17/13 jeffr : Original
//===========================================================================//
void TAS_Power_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_Power_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( ) + 1;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<power" );
   spaceLen += 3;

   string srEstimateMethod;
   TAS_ExtractStringPowerMethodMode( this->estimateMethod, &srEstimateMethod );
   printHandler.Write( pfile, 0, " method=\"%s\"", 
                                 TIO_SR_STR( srEstimateMethod ));

   if(( this->estimateMethod == TAS_POWER_METHOD_SPECIFY_SIZES ) ||
      ( this->estimateMethod == TAS_POWER_METHOD_PIN_TOGGLE ) ||
      ( this->estimateMethod == TAS_POWER_METHOD_ABSOLUTE ) ||
      ( this->estimateMethod == TAS_POWER_METHOD_CAP_INTERNAL ))
   {
      printHandler.Write( pfile, 0, ">\n" );

      if(( this->estimateMethod == TAS_POWER_METHOD_SPECIFY_SIZES ) ||
         ( this->estimateMethod == TAS_POWER_METHOD_PIN_TOGGLE ) ||
         ( this->estimateMethod == TAS_POWER_METHOD_ABSOLUTE ) ||
         ( this->estimateMethod == TAS_POWER_METHOD_CAP_INTERNAL ))
      {
         printHandler.Write( pfile, spaceLen, "<static_power power_per_instance=\"%0.*f\"/>\n", 
                                              precision, this->staticPower.absolute );
      }
      if(( this->estimateMethod == TAS_POWER_METHOD_SPECIFY_SIZES ) ||
         ( this->estimateMethod == TAS_POWER_METHOD_ABSOLUTE ))
      {
         printHandler.Write( pfile, spaceLen, "<dynamic_power power_per_instance=\"%0.*f\"/>\n", 
                                              precision, this->dynamicPower.absolute );
      }
      if( this->estimateMethod == TAS_POWER_METHOD_CAP_INTERNAL )
      {
         printHandler.Write( pfile, spaceLen, "<dynamic_power C_internal=\"%0.*e\"/>\n", 
                                              precision + 1, this->dynamicPower.capInternal );
      }
      if(( this->estimateMethod == TAS_POWER_METHOD_SPECIFY_SIZES ) ||
         ( this->estimateMethod == TAS_POWER_METHOD_PIN_TOGGLE ))
      {
         for( size_t i = 0; i < this->portList.GetLength( ); ++i )
         {
            const TLO_Port_c& port = *this->portList[i];
            const char* pszName = port.GetName( );
            const TLO_Power_c& power = port.GetPower( );
   
            printHandler.Write( pfile, spaceLen, "<port name=\"%s\" ", TIO_PSZ_STR( pszName ));
            power.Print( pfile, 0 );
            printHandler.Write( pfile, 0, "/>\n" );
         }
      }

      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "</power>\n" );
   }
   else
   {
      printHandler.Write( pfile, 0, "/>\n" );
   }
}
