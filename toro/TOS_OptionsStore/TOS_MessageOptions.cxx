//===========================================================================//
// Purpose : Method definitions for the TOS_MessageOptions class.
//
//           Public methods include:
//           - TOS_MessageOptions_c, ~TOS_MessageOptions_c
//           - Print
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

#include "TC_MinGrid.h"

#include "TIO_PrintHandler.h"

#include "TOS_StringUtils.h"
#include "TOS_MessageOptions.h"

//===========================================================================//
// Method         : TOS_MessageOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_MessageOptions_c::TOS_MessageOptions_c( 
      void )
      :
      minGridPrecision( 0.001 ),
      timeStampsEnable( false )
{
   this->info.acceptList.SetCapacity( TOS_DISPLAY_INFO_LIST_DEF_CAPACITY );
   this->info.rejectList.SetCapacity( TOS_DISPLAY_INFO_LIST_DEF_CAPACITY );

   this->warning.acceptList.SetCapacity( TOS_DISPLAY_WARNING_LIST_DEF_CAPACITY );
   this->warning.rejectList.SetCapacity( TOS_DISPLAY_WARNING_LIST_DEF_CAPACITY );

   this->error.acceptList.SetCapacity( TOS_DISPLAY_ERROR_LIST_DEF_CAPACITY );
   this->error.rejectList.SetCapacity( TOS_DISPLAY_ERROR_LIST_DEF_CAPACITY );

   this->trace.acceptList.SetCapacity( TOS_DISPLAY_TRACE_LIST_DEF_CAPACITY );
   this->trace.rejectList.SetCapacity( TOS_DISPLAY_TRACE_LIST_DEF_CAPACITY );

   this->trace.read.options = false;
   this->trace.read.xml = false;
   this->trace.read.blif = false;
   this->trace.read.architecture = false;
   this->trace.read.fabric = false;
   this->trace.read.circuit = false;

   this->trace.vpr.showSetup = false;
   this->trace.vpr.echoFile = false;
   this->trace.vpr.echoFileNameList.SetCapacity( TOS_DISPLAY_TRACE_LIST_DEF_CAPACITY );
}

//===========================================================================//
TOS_MessageOptions_c::TOS_MessageOptions_c( 
            double                 minGridPrecision_,
            bool                   timeStampsEnable_,
      const TOS_DisplayNameList_t& infoAcceptList,
      const TOS_DisplayNameList_t& infoRejectList,
      const TOS_DisplayNameList_t& warningAcceptList,
      const TOS_DisplayNameList_t& warningRejectList,
      const TOS_DisplayNameList_t& errorAcceptList,
      const TOS_DisplayNameList_t& errorRejectList,
      const TOS_DisplayNameList_t& traceAcceptList,
      const TOS_DisplayNameList_t& traceRejectList )
      :
      minGridPrecision( minGridPrecision_ ),
      timeStampsEnable( timeStampsEnable_ )
{
   this->info.acceptList = infoAcceptList;
   this->info.rejectList = infoRejectList;

   this->warning.acceptList = warningAcceptList;
   this->warning.rejectList = warningRejectList;

   this->error.acceptList = errorAcceptList;
   this->error.rejectList = errorRejectList;

   this->trace.acceptList = traceAcceptList;
   this->trace.rejectList = traceRejectList;

   this->trace.read.options = false;
   this->trace.read.xml = false;
   this->trace.read.blif = false;
   this->trace.read.architecture = false;
   this->trace.read.fabric = false;
   this->trace.read.circuit = false;

   this->trace.vpr.showSetup = false;
   this->trace.vpr.echoFile = false;
   this->trace.vpr.echoFileNameList.SetCapacity( TOS_DISPLAY_TRACE_LIST_DEF_CAPACITY );
}

//===========================================================================//
// Method         : ~TOS_MessageOptions_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TOS_MessageOptions_c::~TOS_MessageOptions_c( 
      void )
{
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOS_MessageOptions_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   unsigned int precision = MinGrid.GetPrecision( );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, spaceLen, "FORMAT_MIN_GRID            = %0.*f\n", precision, this->minGridPrecision );
   printHandler.Write( pfile, spaceLen, "FORMAT_TIME_STAMPS         = %s\n", TIO_BOOL_STR( this->timeStampsEnable ));

   if( this->info.acceptList.GetLength( ))
   {
      for( size_t i = 0; i < this->info.acceptList.GetLength( ); ++i )
      {
 	 const TC_Name_c& infoAccept = *this->info.acceptList[i];
         printHandler.Write( pfile, spaceLen, "DISPLAY_INFO_ACCEPT        = \"%s\"\n", TIO_PSZ_STR( infoAccept.GetName( )));
      }
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// DISPLAY_INFO_ACCEPT     = \"\"\n" );
   }
   if( this->info.rejectList.GetLength( ))
   {
      for( size_t i = 0; i < this->info.rejectList.GetLength( ); ++i )
      {
         const TC_Name_c& infoReject = *this->info.rejectList[i];
         printHandler.Write( pfile, spaceLen, "DISPLAY_INFO_REJECT        = \"%s\"\n", TIO_PSZ_STR( infoReject.GetName( )));
      }
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// DISPLAY_INFO_REJECT     = \"\"\n" );
   }

   if( this->warning.acceptList.GetLength( ))
   {
      for( size_t i = 0; i < this->warning.acceptList.GetLength( ); ++i )
      {
         const TC_Name_c& warningAccept = *this->warning.acceptList[i];
         printHandler.Write( pfile, spaceLen, "DISPLAY_WARNING_ACCEPT     = \"%s\"\n", TIO_PSZ_STR( warningAccept.GetName( )));
      }
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// DISPLAY_WARNING_ACCEPT  = \"\"\n" );
   }
   if( this->warning.rejectList.GetLength( ))
   {
      for( size_t i = 0; i < this->warning.rejectList.GetLength( ); ++i )
      {
         const TC_Name_c& warningReject = *this->warning.rejectList[i];
         printHandler.Write( pfile, spaceLen, "DISPLAY_WARNING_REJECT     = \"%s\"\n", TIO_PSZ_STR( warningReject.GetName( )));
      }
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// DISPLAY_WARNING_REJECT  = \"\"\n" );
   }

   if( this->error.acceptList.GetLength( ))
   {
      for( size_t i = 0; i < this->error.acceptList.GetLength( ); ++i )
      {
         const TC_Name_c& errorAccept = *this->error.acceptList[i];
         printHandler.Write( pfile, spaceLen, "DISPLAY_ERROR_ACCEPT       = \"%s\"\n", TIO_PSZ_STR( errorAccept.GetName( )));
      }
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// DISPLAY_ERROR_ACCEPT    = \"\"\n" );
   }
   if( this->error.rejectList.GetLength( ))
   {
      for( size_t i = 0; i < this->error.rejectList.GetLength( ); ++i )
      {
         const TC_Name_c& errorReject = *this->error.rejectList[i];
         printHandler.Write( pfile, spaceLen, "DISPLAY_ERROR_REJECT       = \"%s\"\n", TIO_PSZ_STR( errorReject.GetName( )));
      }
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// DISPLAY_ERROR_REJECT    = \"\"\n" );
   }

   if( this->trace.acceptList.GetLength( ))
   {
      for( size_t i = 0; i < this->trace.acceptList.GetLength( ); ++i )
      {
         const TC_Name_c& traceAccept = *this->trace.acceptList[i];
         printHandler.Write( pfile, spaceLen, "DISPLAY_TRACE_ACCEPT       = \"%s\"\n", TIO_PSZ_STR( traceAccept.GetName( )));
      }
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// DISPLAY_TRACE_ACCEPT    = \"\"\n" );
   }
   if( this->trace.rejectList.GetLength( ))
   {
      for( size_t i = 0; i < this->trace.rejectList.GetLength( ); ++i )
      {
	 const TC_Name_c& traceReject = *this->trace.rejectList[i];
         printHandler.Write( pfile, spaceLen, "DISPLAY_TRACE_REJECT       = \"%s\"\n", TIO_PSZ_STR( traceReject.GetName( )));
      }
   }
   else
   {
      printHandler.Write( pfile, spaceLen, "// DISPLAY_TRACE_REJECT    = \"\"\n" );
   }

   printHandler.Write( pfile, spaceLen, "TRACE_READ_OPTIONS         = %s\n", TIO_BOOL_STR( this->trace.read.options ));
   printHandler.Write( pfile, spaceLen, "TRACE_READ_XML             = %s\n", TIO_BOOL_STR( this->trace.read.xml ));
   printHandler.Write( pfile, spaceLen, "TRACE_READ_BLIF            = %s\n", TIO_BOOL_STR( this->trace.read.blif ));
   printHandler.Write( pfile, spaceLen, "TRACE_READ_ARCHITECTURE    = %s\n", TIO_BOOL_STR( this->trace.read.architecture ));
   printHandler.Write( pfile, spaceLen, "TRACE_READ_FABRIC          = %s\n", TIO_BOOL_STR( this->trace.read.fabric ));
   printHandler.Write( pfile, spaceLen, "TRACE_READ_CIRCUIT         = %s\n", TIO_BOOL_STR( this->trace.read.circuit ));

   printHandler.Write( pfile, spaceLen, "TRACE_VPR_SHOW_SETUP       = %s\n", TIO_BOOL_STR( this->trace.vpr.showSetup ));
   printHandler.Write( pfile, spaceLen, "TRACE_VPR_ECHO_FILE        = %s\n", TIO_BOOL_STR( this->trace.vpr.echoFile ));
   if( this->trace.vpr.echoFileNameList.GetLength( ))
   {
      for( size_t i = 0; i < this->trace.vpr.echoFileNameList.GetLength( ); ++i )
      {
	 const TC_NameFile_c& echoFileName = *this->trace.vpr.echoFileNameList[i];
	 if( echoFileName.GetFileName( ) && *echoFileName.GetFileName( ))
	 {
            printHandler.Write( pfile, spaceLen, "TRACE_VPR_ECHO_FILE        = \"%s\" \"%s\"\n", 
                                                                               TIO_PSZ_STR( echoFileName.GetName( )),
                                                                               TIO_PSZ_STR( echoFileName.GetFileName( )));
	 }
	 else
	 {
            printHandler.Write( pfile, spaceLen, "TRACE_VPR_ECHO_FILE        = \"%s\"\n", 
                                                                               TIO_PSZ_STR( echoFileName.GetName( )));
	 }
      }
   }
}
