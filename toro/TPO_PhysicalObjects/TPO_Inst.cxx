//===========================================================================//
// Purpose : Method definitions for the TPO_Inst class.
//
//           Public methods include:
//           - TPO_Inst_c, ~TPO_Inst_c
//           - operator=, operator<
//           - operator==, operator!=
//           - Print
//           - PrintBLIF
//           - SetInputOutputType
//           - SetNamesLogicBitsList
//           - SetLatchClockType, SetLatchInitState
//           - SetSubcktPinMapList
//           - SetPackHierMapList
//           - SetPlaceRegionList, SetPlaceRelativeList
//           - SetPlaceStatus, SetPlaceFabricName
//           - AddPin
//           - FindPin
//           - FindPinCount
//           - Clear
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TPO_StringUtils.h"
#include "TPO_Inst.h"

//===========================================================================//
// Method         : TPO_Inst_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_Inst_c::TPO_Inst_c( 
      void )
{
   this->Clear( );
} 

//===========================================================================//
TPO_Inst_c::TPO_Inst_c( 
      const string& srName )
{
   this->Clear( );

   this->srName_ = srName;
} 

//===========================================================================//
TPO_Inst_c::TPO_Inst_c( 
      const char* pszName )
{
   this->Clear( );

   this->srName_ = TIO_PSZ_STR( pszName );
} 

//===========================================================================//
TPO_Inst_c::TPO_Inst_c( 
      const string&              srName,
      const string&              srCellName,
      const TPO_PinList_t&       pinList,
            TPO_InstSource_t     source,
      const TPO_LogicBitsList_t& logicBitsList )
{
   this->Clear( );

   this->srName_ = srName;
   this->srCellName_ = srCellName;
   this->pinList_ = pinList;
   this->source_ = source;

   this->names_.logicBitsList = logicBitsList;
} 

//===========================================================================//
TPO_Inst_c::TPO_Inst_c( 
      const char*                pszName,
      const char*                pszCellName,
      const TPO_PinList_t&       pinList,
            TPO_InstSource_t     source,
      const TPO_LogicBitsList_t& logicBitsList )
{
   this->Clear( );

   this->srName_ = TIO_PSZ_STR( pszName );
   this->srCellName_ = TIO_PSZ_STR( pszCellName );
   this->pinList_ = pinList;
   this->source_ = source;

   this->names_.logicBitsList = logicBitsList;
} 

//===========================================================================//
TPO_Inst_c::TPO_Inst_c( 
      const string&          srName,
      const string&          srCellName,
      const TPO_PinList_t&   pinList,
            TPO_InstSource_t source,
            TPO_LatchType_t  clockType,
            TPO_LatchState_t initState )
{
   this->Clear( );

   this->srName_ = srName;
   this->srCellName_ = srCellName;
   this->pinList_ = pinList;
   this->source_ = source;

   this->latch_.clockType = clockType;
   this->latch_.initState = initState;
} 

//===========================================================================//
TPO_Inst_c::TPO_Inst_c( 
      const char*            pszName,
      const char*            pszCellName,
      const TPO_PinList_t&   pinList,
            TPO_InstSource_t source,
            TPO_LatchType_t  clockType,
            TPO_LatchState_t initState )
{
   this->Clear( );

   this->srName_ = TIO_PSZ_STR( pszName );
   this->srCellName_ = TIO_PSZ_STR( pszCellName );
   this->pinList_ = pinList;
   this->source_ = source;

   this->latch_.clockType = clockType;
   this->latch_.initState = initState;
} 

//===========================================================================//
TPO_Inst_c::TPO_Inst_c( 
      const string&           srName,
      const string&           srCellName,
            TPO_InstSource_t  source,
      const TPO_PinMapList_t& pinMapList )
{
   this->Clear( );

   this->srName_ = srName;
   this->srCellName_ = srCellName;
   this->source_ = source;

   this->subckt_.pinMapList = pinMapList;
} 

//===========================================================================//
TPO_Inst_c::TPO_Inst_c( 
      const char*             pszName,
      const char*             pszCellName,
            TPO_InstSource_t  source,
      const TPO_PinMapList_t& pinMapList )
{
   this->Clear( );

   this->srName_ = TIO_PSZ_STR( pszName );
   this->srCellName_ = TIO_PSZ_STR( pszCellName );
   this->source_ = source;

   this->subckt_.pinMapList = pinMapList;
} 

//===========================================================================//
TPO_Inst_c::TPO_Inst_c( 
      const TPO_Inst_c& inst )
{
   this->Clear( );

   this->srName_ = inst.srName_;
   this->srCellName_ = inst.srCellName_;
   this->pinList_ = inst.pinList_;
   this->source_ = inst.source_;

   this->inputOutput_.type = inst.inputOutput_.type;

   this->names_.logicBitsList = inst.names_.logicBitsList;

   this->latch_.clockType = inst.latch_.clockType;
   this->latch_.initState = inst.latch_.initState;

   this->subckt_.pinMapList = inst.subckt_.pinMapList;

   this->pack_.hierMapList = inst.pack_.hierMapList;

   this->place_.relativeList = inst.place_.relativeList;
   this->place_.regionList = inst.place_.regionList;

   this->place_.status = inst.place_.status;
   this->place_.srFabricName = inst.place_.srFabricName;
} 

//===========================================================================//
// Method         : ~TPO_Inst_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
TPO_Inst_c::~TPO_Inst_c( 
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
TPO_Inst_c& TPO_Inst_c::operator=( 
      const TPO_Inst_c& inst )
{
   if( &inst != this )
   {
      this->srName_ = inst.srName_;
      this->srCellName_ = inst.srCellName_;
      this->pinList_ = inst.pinList_;
      this->source_ = inst.source_;
      this->inputOutput_.type = inst.inputOutput_.type;
      this->names_.logicBitsList = inst.names_.logicBitsList;
      this->latch_.clockType = inst.latch_.clockType;
      this->latch_.initState = inst.latch_.initState;
      this->subckt_.pinMapList = inst.subckt_.pinMapList;
      this->pack_.hierMapList = inst.pack_.hierMapList;
      this->place_.relativeList = inst.place_.relativeList;
      this->place_.regionList = inst.place_.regionList;
      this->place_.status = inst.place_.status;
      this->place_.srFabricName = inst.place_.srFabricName;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator<
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_Inst_c::operator<( 
      const TPO_Inst_c& inst ) const
{
   return(( TC_CompareStrings( this->srName_, inst.srName_ ) < 0 ) ? 
          true : false );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_Inst_c::operator==( 
      const TPO_Inst_c& inst ) const
{
   return(( this->srName_ == inst.srName_ ) ? true : false );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
bool TPO_Inst_c::operator!=( 
      const TPO_Inst_c& inst ) const
{
   return( !this->operator==( inst ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   printHandler.Write( pfile, 0, "\"%s\" ", TIO_SR_STR( this->srName_ ));
   if( this->srCellName_.length( ))
   {
      printHandler.Write( pfile, 0, "\"%s\" ", TIO_SR_STR( this->srCellName_ ));
   }
   else if( this->source_ != TPO_INST_SOURCE_UNDEFINED )
   {
      string srSource;
      TPO_ExtractStringInstSource( this->source_, &srSource );
      printHandler.Write( pfile, 0, "\".%s\" ", TIO_SR_STR( srSource ));
   }

   if( this->place_.status != TPO_STATUS_UNDEFINED )
   {
      string srStatus;
      TPO_ExtractStringStatusMode( this->place_.status, &srStatus );
      printHandler.Write( pfile, 0, "status = %s ", TIO_SR_STR( srStatus ));
   }
   printHandler.Write( pfile, 0, ">\n" );

   if( this->source_ == TPO_INST_SOURCE_LATCH )
   {
      printHandler.Write( pfile, spaceLen, "<clock " ); 
      if( this->latch_.clockType != TPO_LATCH_TYPE_UNDEFINED )
      {
         string srClockType;
         TPO_ExtractStringLatchType( this->latch_.clockType, &srClockType );
         printHandler.Write( pfile, 0, "type = %s ", TIO_SR_STR( srClockType ));
      }
      if( this->latch_.initState != TPO_LATCH_STATE_UNDEFINED )
      {
         string srInitState;
         TPO_ExtractStringLatchState( this->latch_.initState, &srInitState );
         printHandler.Write( pfile, 0, "state = %s ", TIO_SR_STR( srInitState ));
      }
      printHandler.Write( pfile, 0, "/>\n" );
   }

   if( this->pinList_.IsValid( ))
   {
      for( size_t i = 0; i < this->pinList_.GetLength( ); ++i )
      {
         string srPin;
         this->pinList_[i]->ExtractString( &srPin );   
         printHandler.Write( pfile, spaceLen, "<pin %s />\n", TIO_SR_STR( srPin ));
      }
   }

   if( this->pack_.hierMapList.IsValid( ))
   {
      this->pack_.hierMapList.Print( pfile, spaceLen );
   }

   for( size_t i = 0; i < this->place_.regionList.GetLength( ); ++i )
   {
      string srRegion;
      this->place_.regionList[i]->ExtractString( &srRegion );
      printHandler.Write( pfile, spaceLen, "<region %s />\n", TIO_SR_STR( srRegion ));
   }

   for( size_t i = 0; i < this->place_.relativeList.GetLength( ); ++i )
   {
      string srRelative;
      this->place_.relativeList[i]->ExtractString( &srRelative );
      printHandler.Write( pfile, spaceLen, "<relative %s />\n", TIO_SR_STR( srRelative ));
   }

   if( this->place_.srFabricName.length( ))
   {
      const string& srFabricName = this->place_.srFabricName;
      printHandler.Write( pfile, spaceLen, "<placement \"%s\" />\n", TIO_SR_STR( srFabricName ));
   }
}

//===========================================================================//
// Method         : PrintBLIF
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::PrintBLIF( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srInputPin, srOutputPin, srClockPin;

   switch( this->source_ )
   {
   case TPO_INST_SOURCE_NAMES:

      printHandler.Write( pfile, spaceLen, ".names" );
      for( size_t i = 0; i < this->pinList_.GetLength( ); ++i )
      {
         const TPO_Pin_t& pin = *this->pinList_[i];
         printHandler.Write( pfile, spaceLen, " %s",
                                              TIO_PSZ_STR( pin.GetName( )));

      }
      printHandler.Write( pfile, 0, "\n" );

      for( size_t i = 0; i < this->names_.logicBitsList.GetLength( ); ++i )
      {
         string srLogicBits;
         const TPO_LogicBits_t& logicBits = *this->names_.logicBitsList[i];
         for( size_t j = 0; j < logicBits.GetLength( ); ++j )
         {
            string srLogicBit;
            logicBits[j]->ExtractString( &srLogicBit );

            srLogicBits += srLogicBit;
            srLogicBits += ( j + 2 == logicBits.GetLength( ) ? " " : "" );
         }
         printHandler.Write( pfile, spaceLen, "%s\n",
                                              TIO_SR_STR( srLogicBits ));
      }
      break;

   case TPO_INST_SOURCE_LATCH:

      srInputPin = ( this->pinList_[0] ? this->pinList_[0]->GetName( ) : "" );
      srOutputPin = ( this->pinList_[1] ? this->pinList_[1]->GetName( ) : "" );
      srClockPin = ( this->pinList_[2] ? this->pinList_[2]->GetName( ) : "" );

      printHandler.Write( pfile, spaceLen, ".latch %s %s",
                                           TIO_SR_STR( srInputPin ),
                                           TIO_SR_STR( srOutputPin ));
      if( this->latch_.clockType != TPO_LATCH_TYPE_UNDEFINED )
      {
         string srClockType;
         TPO_ExtractStringLatchType( this->latch_.clockType, &srClockType );
         printHandler.Write( pfile, spaceLen, " %s %s",
                                              TIO_SR_STR( srClockType ),
                                               TIO_SR_STR( srClockPin ));
      }
      if( this->latch_.initState != TPO_LATCH_STATE_UNDEFINED )
      {
         string srInitState;
         TPO_ExtractStringLatchState( this->latch_.initState, &srInitState );
         printHandler.Write( pfile, spaceLen, " %s",
                                              TIO_SR_STR( srInitState ));
      }
      printHandler.Write( pfile, 0, "\n" );
      break;

   case TPO_INST_SOURCE_SUBCKT:

      printHandler.Write( pfile, spaceLen, ".subckt %s",
                                           TIO_SR_STR( this->srCellName_ ));

      for( size_t i = 0; i < this->subckt_.pinMapList.GetLength( ); ++i )
      {
         const TPO_PinMap_c& pinMap = *this->subckt_.pinMapList[i];
         printHandler.Write( pfile, 0, " %s=%s",
                                       TIO_PSZ_STR( pinMap.GetCellPinName( )),
                                       TIO_PSZ_STR( pinMap.GetInstPinName( )));
      }
      printHandler.Write( pfile, 0, "\n" );
      break;

   default:

      printHandler.Write( pfile, spaceLen, "// " );
      this->Print( pfile, spaceLen );
      break;
   }
}

//===========================================================================//
// Method         : SetInputOutputType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::SetInputOutputType( 
      TC_TypeMode_t type )
{
   switch( type )
   {
   case TC_TYPE_INPUT:  this->source_ = TPO_INST_SOURCE_OUTPUT;    break;
   case TC_TYPE_OUTPUT: this->source_ = TPO_INST_SOURCE_INPUT;     break;
   default:             this->source_ = TPO_INST_SOURCE_UNDEFINED; break;
   }
   this->inputOutput_.type = type;
}

//===========================================================================//
// Method         : SetNamesLogicBitsList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::SetNamesLogicBitsList( 
      const TPO_LogicBitsList_t& logicBitsList )
{
   this->source_ = TPO_INST_SOURCE_NAMES;
   this->names_.logicBitsList = logicBitsList;
}

//===========================================================================//
// Method         : SetLatchClockType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::SetLatchClockType( 
      TPO_LatchType_t clockType )
{
   this->source_ = TPO_INST_SOURCE_LATCH;
   this->latch_.clockType = clockType;
}

//===========================================================================//
// Method         : SetLatchInstState
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::SetLatchInitState( 
      TPO_LatchState_t initState )
{
   this->source_ = TPO_INST_SOURCE_LATCH;
   this->latch_.initState = initState;
}

//===========================================================================//
// Method         : SetSubcktPinMapList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::SetSubcktPinMapList( 
      const TPO_PinMapList_t& pinMapList )
{
   this->source_ = TPO_INST_SOURCE_SUBCKT;
   this->subckt_.pinMapList = pinMapList;
}

//===========================================================================//
// Method         : SetPackHierMapList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::SetPackHierMapList(
      const TPO_HierMapList_t& hierMapList )
{
   this->pack_.hierMapList = hierMapList;
}

//===========================================================================//
// Method         : SetPlaceRegionList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::SetPlaceRegionList(
      const TGS_RegionList_t& regionList )
{
   this->place_.regionList = regionList;
}

//===========================================================================//
// Method         : SetPlaceRelativeList
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::SetPlaceRelativeList(
      const TPO_RelativeList_t& relativeList )
{
   this->place_.relativeList = relativeList;
}

//===========================================================================//
// Method         : SetPlaceStatus
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::SetPlaceStatus( 
      TPO_StatusMode_t status )
{
   this->place_.status = status;
}

//===========================================================================//
// Method         : SetPlaceFabricName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::SetPlaceFabricName( 
      const string& srFabricName )
{
   this->place_.srFabricName = srFabricName;
}

//===========================================================================//
void TPO_Inst_c::SetPlaceFabricName( 
      const char* pszFabricName )
{
   this->place_.srFabricName = TIO_PSZ_STR( pszFabricName );
}

//===========================================================================//
// Method         : AddPin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::AddPin(
      const TPO_Pin_t& pin )
{
   this->pinList_.Add( pin );
}

//===========================================================================//
// Method         : FindPin
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
const TPO_Pin_t* TPO_Inst_c::FindPin(
      TC_TypeMode_t type ) const
{
   const TPO_Pin_t* ppin = 0;

   for( size_t i = 0; i < this->pinList_.GetLength( ); ++i )
   {
      const TPO_Pin_t& pin = *this->pinList_[i];
      if( pin.GetType( ) == type )
      {
         ppin = this->pinList_[i];
         break;
      }
   }
   return( ppin );
}

//===========================================================================//
// Method         : FindPinCount
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
size_t TPO_Inst_c::FindPinCount(
            TC_TypeMode_t   type,
      const TLO_CellList_t& cellList ) const
{
   size_t pinCount = 0;

   if( cellList.IsMember( this->srCellName_ ))
   {
      const TLO_Cell_c& cell = *cellList.Find( this->srCellName_ );
      pinCount = cell.FindPortCount( type );
   }
   return( pinCount );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TPO_Inst_c::Clear( 
      void )
{
   this->srName_ = "";
   this->srCellName_ = "";

   this->pinList_.Clear( );
   this->pinList_.SetCapacity( TPO_PIN_LIST_DEF_CAPACITY );

   this->source_ = TPO_INST_SOURCE_UNDEFINED;

   this->inputOutput_.type = TC_TYPE_UNDEFINED;

   this->names_.logicBitsList.Clear( );
   this->names_.logicBitsList.SetCapacity( TPO_LOGIC_BITS_LIST_DEF_CAPACITY );

   this->latch_.clockType = TPO_LATCH_TYPE_UNDEFINED;
   this->latch_.initState = TPO_LATCH_STATE_UNDEFINED;

   this->subckt_.pinMapList.Clear( );
   this->subckt_.pinMapList.SetCapacity( TPO_PIN_MAP_LIST_DEF_CAPACITY );

   this->pack_.hierMapList.Clear( );
   this->pack_.hierMapList.SetCapacity( TPO_HIER_MAP_LIST_DEF_CAPACITY );

   this->place_.regionList.Clear( );
   this->place_.regionList.SetCapacity( TPO_REGION_LIST_DEF_CAPACITY );
   this->place_.relativeList.Clear( );
   this->place_.relativeList.SetCapacity( TPO_RELATIVE_LIST_DEF_CAPACITY );

   this->place_.status = TPO_STATUS_UNDEFINED;
   this->place_.srFabricName = "";
}
