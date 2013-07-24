//===========================================================================//
// Purpose : Method definitions for the TAS_Cell class.
//
//           Public methods include:
//           - TAS_Cell_c, ~TAS_Cell_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
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

#if defined( SUN8 ) || defined( SUN10 )
   #include <math.h>
#endif

#include "TC_MinGrid.h"

#include "TIO_PrintHandler.h"

#include "TAS_StringUtils.h"
#include "TAS_Cell.h"

//===========================================================================//
// Method         : TAS_Cell_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_Cell_c::TAS_Cell_c( 
      void )
      :
      classType( TAS_CLASS_UNDEFINED )
{
}

//===========================================================================//
TAS_Cell_c::TAS_Cell_c( 
      const string&          srName,
            TLO_CellSource_t source )
      :
      TLO_Cell_c( srName, source ),
      classType( TAS_CLASS_UNDEFINED )
{
}

//===========================================================================//
TAS_Cell_c::TAS_Cell_c( 
      const char*            pszName,
            TLO_CellSource_t source )
      :
      TLO_Cell_c( pszName, source ),
      classType( TAS_CLASS_UNDEFINED )
{
   this->dims_.Set( 0.0, 0.0 );
   this->origin_.Set( 0.0, 0.0 );
}

//===========================================================================//
TAS_Cell_c::TAS_Cell_c( 
      const TAS_Cell_c& cell )
      :
      TLO_Cell_c( cell ),
      classType( cell.classType )
{
   this->dims_ = cell.dims_;
   this->origin_ = cell.origin_;
}

//===========================================================================//
// Method         : ~TAS_Cell_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_Cell_c::~TAS_Cell_c( 
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
TAS_Cell_c& TAS_Cell_c::operator=( 
      const TAS_Cell_c& cell )
{
   if( &cell != this )
   {
      TLO_Cell_c::operator=( cell );
      this->classType = cell.classType;
      this->dims_ = cell.dims_;
      this->origin_ = cell.origin_;
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
bool TAS_Cell_c::operator==( 
      const TAS_Cell_c& cell ) const
{
   return( TLO_Cell_c::operator==( cell ) ? true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_Cell_c::operator!=( 
      const TAS_Cell_c& cell ) const
{
   return( !this->operator==( cell ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_Cell_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<cell name=\"%s\"",
                                        TIO_PSZ_STR( TLO_Cell_c::GetName( )));
   if( TLO_Cell_c::GetMasterName( ) && *TLO_Cell_c::GetMasterName( ))
   {
      printHandler.Write( pfile, 0, " master=\"%s\"",
                                    TIO_PSZ_STR( TLO_Cell_c::GetMasterName( )));
   }

   if( this->classType != TAS_CLASS_UNDEFINED )
   {
      string srClassType;
      TAS_ExtractStringClassType( this->classType, &srClassType );
      printHandler.Write( pfile, 0, " class=\"%s\"", 
                                    TIO_SR_STR( srClassType ));
   }

   if( this->dims_.IsValid( ) || this->origin_.IsValid( ))
   {
      printHandler.Write( pfile, 0, "\n      " ); 

      if( this->dims_.IsValid( ))
      {
         printHandler.Write( pfile, 0, "<size> %0.*f %0.*f </size>", 
                                       precision, this->dims_.dx, 
                                       precision, this->dims_.dy );
      }
      if( this->origin_.IsValid( ))
      {
         printHandler.Write( pfile, 0, "<origin> %0.*f %0.*f </origin>", 
                                       precision, this->origin_.x, 
                                       precision, this->origin_.y );
      }
   }
   printHandler.Write( pfile, 0, ">\n" );

   if( TLO_Cell_c::GetPortList( ).IsValid( ))
   {
      TLO_Cell_c::GetPortList( ).Print( pfile, spaceLen + 3 );
   }

   printHandler.Write( pfile, spaceLen, "</cell>\n" );
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_Cell_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_Cell_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<model name=\"%s\">\n",
                                        TIO_PSZ_STR( TLO_Cell_c::GetName( )));
   spaceLen += 3;

   if( TLO_Cell_c::HasPortType( TC_TYPE_INPUT ) ||
       TLO_Cell_c::HasPortType( TC_TYPE_CLOCK ))
   {
      printHandler.Write( pfile, spaceLen, "<input_ports>\n" );
      spaceLen += 3;

      const TLO_PortList_t& portList = TLO_Cell_c::GetPortList( );
      for( size_t i = 0; i < portList.GetLength( ); ++i )
      {
         const TLO_Port_c& port = *portList[i];
         if(( port.GetType( ) != TC_TYPE_INPUT ) && ( port.GetType( ) != TC_TYPE_CLOCK ))
            continue;

         printHandler.Write( pfile, spaceLen, "<port name=\"%s\" is_clock=\"%s\"",
                                              TIO_PSZ_STR( port.GetName( )),
                                              port.GetType( ) == TC_TYPE_CLOCK ? "1" : "0" );

         const TLO_Power_c& portPower = port.GetPower( );
         if( portPower.IsValid( ))
         {
            TLO_PowerType_t wireType = TLO_POWER_TYPE_UNDEFINED;
            double wireCap = 0.0;
            double wireRelativeLength = 0.0;
            double wireAbsoluteLength = 0.0;
            portPower.GetWire( &wireType, &wireCap, &wireRelativeLength, &wireAbsoluteLength );

            TLO_PowerType_t bufferType = TLO_POWER_TYPE_UNDEFINED;
            double bufferAbsoluteSize = 0.0;
            portPower.GetBuffer( &bufferType, &bufferAbsoluteSize );

            printHandler.Write( pfile, 0, ">\n" );
            printHandler.Write( pfile, spaceLen + 3, "<power" );
            if( wireType == TLO_POWER_TYPE_CAP )
            {
               printHandler.Write( pfile, 0, " wire_capacitance=\"%0.*e\"",
                                             precision + 1, wireCap );
            }
            else if( wireType == TLO_POWER_TYPE_RELATIVE_LENGTH )
            {
               printHandler.Write( pfile, 0, " wire_relative_length=\"%0.*f\"",
                                             precision, wireRelativeLength );
            }
            else if( wireType == TLO_POWER_TYPE_ABSOLUTE_LENGTH )
            {
               if( TCTF_IsGT( wireAbsoluteLength, 0.0 ))
               {
                  printHandler.Write( pfile, 0, " wire_length=\"%0.*f\"",
                                                precision, wireAbsoluteLength );
               }
               else
               {
                  printHandler.Write( pfile, 0, " wire_length=\"auto\"" );
               }
            }
            if( bufferType == TLO_POWER_TYPE_ABSOLUTE_SIZE )
            {
               if( TCTF_IsGT( bufferAbsoluteSize, 0.0 ))
               {
                  printHandler.Write( pfile, 0, " buffer_size=\"%0.*f\"",
                                                precision, bufferAbsoluteSize );
               }
               else
               {
                  printHandler.Write( pfile, 0, " buffer_size=\"auto\"" );
               }
            }
            printHandler.Write( pfile, 0, "/>\n" );
            printHandler.Write( pfile, spaceLen, "</port>\n" );
         }
         else
         {
            printHandler.Write( pfile, 0, "/>\n" );
         }
      }
      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "</input_ports>\n" );
   }

   if( TLO_Cell_c::HasPortType( TC_TYPE_OUTPUT ))
   {
      printHandler.Write( pfile, spaceLen, "<output_ports>\n" );
      spaceLen += 3;

      const TLO_PortList_t& portList = TLO_Cell_c::GetPortList( );
      for( size_t i = 0; i < portList.GetLength( ); ++i )
      {
         const TLO_Port_c& port = *portList[i];
         if( port.GetType( ) != TC_TYPE_OUTPUT )
            continue;

         printHandler.Write( pfile, spaceLen, "<port name=\"%s\"/>\n",
                                              TIO_PSZ_STR( port.GetName( )));
      }
      spaceLen -= 3;
      printHandler.Write( pfile, spaceLen, "</output_ports>\n" );
   }
   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</model>\n" );
}
