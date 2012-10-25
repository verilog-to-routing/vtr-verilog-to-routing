//===========================================================================//
// Purpose : Method definitions for the TAS_Interconnect class.
//
//           Public methods include:
//           - TAS_Interconnect_c, ~TAS_Interconnect_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - PrintXML
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

#include "TAS_StringUtils.h"
#include "TAS_Interconnect.h"

//===========================================================================//
// Method         : TAS_Interconnect_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_Interconnect_c::TAS_Interconnect_c( 
      void )
      :
      mapType( TAS_INTERCONNECT_MAP_UNDEFINED ),
      inputNameList( TAS_INPUT_NAME_LIST_DEF_CAPACITY ),
      outputNameList( TAS_OUTPUT_NAME_LIST_DEF_CAPACITY )
{
}

//===========================================================================//
TAS_Interconnect_c::TAS_Interconnect_c( 
      const string& srName_ )
      :
      srName( srName_ ),
      mapType( TAS_INTERCONNECT_MAP_UNDEFINED ),
      inputNameList( TAS_INPUT_NAME_LIST_DEF_CAPACITY ),
      outputNameList( TAS_OUTPUT_NAME_LIST_DEF_CAPACITY )
{
}

//===========================================================================//
TAS_Interconnect_c::TAS_Interconnect_c( 
      const char* pszName )
      :
      srName( TIO_PSZ_STR( pszName )),
      mapType( TAS_INTERCONNECT_MAP_UNDEFINED ),
      inputNameList( TAS_INPUT_NAME_LIST_DEF_CAPACITY ),
      outputNameList( TAS_OUTPUT_NAME_LIST_DEF_CAPACITY )
{
}

//===========================================================================//
TAS_Interconnect_c::TAS_Interconnect_c( 
      const TAS_Interconnect_c& interconnect )
      :
      srName( interconnect.srName ),
      mapType( interconnect.mapType ),
      inputNameList( interconnect.inputNameList ),
      outputNameList( interconnect.outputNameList ),
      timingDelayLists( interconnect.timingDelayLists )
{
}

//===========================================================================//
// Method         : ~TAS_OutputName_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TAS_Interconnect_c::~TAS_Interconnect_c( 
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
TAS_Interconnect_c& TAS_Interconnect_c::operator=( 
      const TAS_Interconnect_c& interconnect )
{
   if( &interconnect != this )
   {
      this->srName = interconnect.srName;
      this->mapType = interconnect.mapType;
      this->inputNameList = interconnect.inputNameList;
      this->outputNameList = interconnect.outputNameList;
      this->timingDelayLists = interconnect.timingDelayLists;
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
bool TAS_Interconnect_c::operator==( 
      const TAS_Interconnect_c& interconnect ) const
{
   return(( this->srName == interconnect.srName ) &&
          ( this->mapType == interconnect.mapType ) &&
          ( this->inputNameList == interconnect.inputNameList ) &&
          ( this->outputNameList == interconnect.outputNameList ) &&
          ( this->timingDelayLists == interconnect.timingDelayLists ) ?
          true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TAS_Interconnect_c::operator!=( 
      const TAS_Interconnect_c& interconnect ) const
{
   return( !this->operator==( interconnect ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_Interconnect_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "<interconnect \"%s\" ", TIO_SR_STR( this->srName ));

   if( this->mapType != TAS_INTERCONNECT_MAP_UNDEFINED )
   {
      string srMapType;
      TAS_ExtractStringInterconnectMapType( this->mapType, &srMapType );
      printHandler.Write( pfile, 0, "%s ", TIO_SR_STR( srMapType ));
   }
   printHandler.Write( pfile, 0, ">\n" );

   spaceLen += 3;

   for( size_t i = 0; i < this->inputNameList.GetLength( ); ++i )
   {
      const TC_Name_c& inputName = *this->inputNameList[i];
      printHandler.Write( pfile, spaceLen, "<input \"%s\" />\n", TIO_PSZ_STR( inputName.GetName( )));
   }
   for( size_t i = 0; i < this->outputNameList.GetLength( ); ++i )
   {
      const TC_Name_c& outputName = *this->outputNameList[i];
      printHandler.Write( pfile, spaceLen, "<output \"%s\" />\n", TIO_PSZ_STR( outputName.GetName( )));
   }

   this->timingDelayLists.Print( pfile, spaceLen );

   spaceLen -= 3;
   printHandler.Write( pfile, spaceLen, "</interconnect>\n" );
}

//===========================================================================//
// Method         : PrintXML
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAS_Interconnect_c::PrintXML( 
      void ) const
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->PrintXML( pfile, spaceLen );
}

//===========================================================================//
void TAS_Interconnect_c::PrintXML( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   size_t maxLen = SIZE_MAX;
   bool quotedString = false;

   string srMapType, srInputNameList, srOutputNameList;
   TAS_ExtractStringInterconnectMapType( this->mapType, &srMapType );
   this->inputNameList.ExtractString( &srInputNameList, maxLen, quotedString );
   this->outputNameList.ExtractString( &srOutputNameList, maxLen, quotedString );

   if( !this->timingDelayLists.IsValid( ))
   {
      printHandler.Write( pfile, spaceLen, "<%s name=\"%s\" input=\"%s\" output=\"%s\"/>\n", 
                                           TIO_SR_STR( srMapType ),
                                           TIO_SR_STR( this->srName ),
                                           TIO_SR_STR( srInputNameList ),
		                           TIO_SR_STR( srOutputNameList ));
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "<%s name=\"%s\" input=\"%s\" output=\"%s\">\n", 
                                           TIO_SR_STR( srMapType ),
                                           TIO_SR_STR( this->srName ),
                                           TIO_SR_STR( srInputNameList ),
		                           TIO_SR_STR( srOutputNameList ));

      this->timingDelayLists.PrintXML( pfile, spaceLen + 3 );

      printHandler.Write( pfile, spaceLen, "</%s>\n",
                                           TIO_SR_STR( srMapType ));
   }
}
