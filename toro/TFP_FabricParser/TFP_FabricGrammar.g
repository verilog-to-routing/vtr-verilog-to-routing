//===========================================================================//
// Purpose: PCCTS grammar for the fabric model file.
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

//===========================================================================//
#header

<<
#include <cstdio>
using namespace std;

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TC_SideName.h"

#include "TFM_FabricModel.h"

#include "TFP_FabricFile.h"
#include "TFP_FabricScanner_c.h"
>>

//===========================================================================//
#lexclass QUOTED_VALUE

#token CLOSE_QUOTE     "\""                    << mode( START ); >>
#token STRING          "~[\"\n]+"
#token UNCLOSED_STRING "[\n]"

//===========================================================================//
#lexclass START

#token                 "[\ \t]+"      << skip( ); >>
#token CPP_COMMENT     "//~[\n]*[\n]" << skip( ); newline( ); >>
#token BLOCK_COMMENT   "#~[\n]*[\n]"  << skip( ); newline( ); >>
#token NEW_LINE        "[\n\\]"       << skip( ); newline( ); >>
#token END_OF_FILE     "@"
#token OPEN_QUOTE      "\""           << mode( QUOTED_VALUE ); >>
#token EQUAL           "="

#token FABRIC       "[Ff][Aa][Bb][Rr][Ii][Cc]"
#token CONFIG       "[Cc][Oo][Nn][Ff][Ii][Gg]"
#token IO           "[Ii]{[Nn][Pp][Uu][Tt]}[Oo]{[Uu][Tt][Pp][Uu][Tt]}"
#token PB           "[Pp]{[Hh][Yy][Ss][Ii][Cc][Aa][Ll]}[Bb]{[Ll][Oo][Cc][Kk]}"
#token SB           "[Ss]{[Ww][Ii][Tt][Cc][Hh]}[Bb]{[Oo][Xx]{[Ee][Ss]}}"
#token CB           "[Cc]{[Oo][Nn][Nn][Ee][Cc][Tt][Ii][Oo][Nn]}[Bb]{[Oo][Xx]{[Ee][Ss]}}"
#token CHANNEL      "[Cc][Hh][Aa][Nn][Nn][Ee][Ll]{[Ss]}"
#token SEGMENT      "[Ss][Ee][Gg][Mm][Ee][Nn][Tt]{[Ss]}"

#token SIZE         "{[Ff][Aa][Bb][Rr][Ii][Cc][_]}[Ss][Ii][Zz][Ee]"
#token COUNT        "[Cc][Oo][Uu][Nn][Tt]"
#token WIDTH        "[Ww][Ii][Dd][Tt][Hh]"
#token HEIGHT       "[Hh][Ee][Ii][Gg][Hh][Tt]"

#token SIDE         "[Ss][Ii][Dd][Ee]"
#token OFFSET       "[Oo][Ff][Ff][Ss][Ee][Tt]"
#token INDEX        "[Ii][Nn][Dd][Ee][Xx]"
#token SLICE        "[Ss][Ll][Ii][Cc][Ee]"
#token CAPACITY     "[Cc][Aa][Pp][Aa][Cc][Ii][Tt][Yy]"

#token CELL         "[Cc][Ee][Ll][Ll]"
#token MASTER       "[Mm][Aa][Ss][Tt][Ee][Rr]"
#token PIN          "[Pp][Ii][Nn]"

#token MAPPING      "[Mm][Aa][Pp][Pp][Ii][Nn][Gg]"

#token LINE         "[Ll][Ii][Nn][Ee]"
#token REGION       "[Rr][Ee][Gg][Ii][Oo][Nn]"
#token ORIENT       "[Oo][Rr][Ii][Ee][Nn][Tt]"

#token TIMING       "[Tt][Ii][Mm][Ii][Nn][Gg]{[_][Aa][Nn][Aa][Ll][Yy][Ss][Ii][Ss]}"
#token R            "[Rr]"
#token RES          "[Rr][Ee][Ss]"
#token CAP          "[Cc]{[Aa][Pp]}"
#token CAP_IN       "[Cc]{[Aa][Pp]}{[_]}[Ii][Nn]"
#token CAP_OUT      "[Cc]{[Aa][Pp]}{[_]}[Oo][Uu][Tt]"
#token T            "[Tt]"
#token DELAY        "[Dd][Ee][Ll][Aa][Yy]{[_][Ii][Nn]}"

#token HORIZONTAL   "[Hh]{[Oo][Rr]{[Ii]}[Zz]{[Oo][Nn][Tt][Aa][Ll]}}"
#token VERTICAL     "[Vv]{[Ee][Rr][Tt]{[Ii][Cc][Aa][Ll]}}"
#tokclass ORIENT_VAL { HORIZONTAL VERTICAL }

#token LEFT         "[Ll]{[Ee][Ff][Tt]}"
#token RIGHT        "[Rr]{[Ii][Gg][Hh][Tt]}"
#token LOWER        "[Ll][Oo][Ww][Ee][Rr]"
#token UPPER        "[Uu][Pp][Pp][Ee][Rr]"
#token BOTTOM       "[Bb]{[Oo][Tt][Tt][Oo][Mm]}"
#token TOP          "[Tt]{[Oo][Pp]}"
#tokclass SIDE_VAL  { LEFT RIGHT LOWER UPPER BOTTOM TOP R T }

#token BIT_CHAR   "[01]"
#token NEG_INT    "[\-][0-9]+"
#token POS_INT    "[0-9]+"
#token FLOAT      "{\-}{[0-9]+}.[0-9]+"
#token EXP        "{\-}{[0-9]+}.[0-9]+[Ee][\+\-][0-9]+"
#token STRING     "[a-zA-Z_/\|][a-zA-Z0-9_/\|\(\)\[\]\.\+\-\~]*"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//

class TFP_FabricParser_c
{
<<
public:

   void syn( ANTLRAbstractToken* /* pToken */, 
             ANTLRChar*          pszGroup,
             SetWordType*        /* pWordType */,
             ANTLRTokenType      tokenType,
             int                 /* k */ );
            
   void SetInterface( TFP_FabricInterface_c* pinterface );
   void SetScanner( TFP_FabricScanner_c* pscanner );
   void SetFileName( const char* pszFileName );
   void SetFabricFile( TFP_FabricFile_c* pfabricFile );
   void SetFabricModel( TFM_FabricModel_c* pfabricModel );
>> 
<<
private:

   TGS_OrientMode_t FindOrientMode_( ANTLRTokenType tokenType );
   TC_SideMode_t FindSideMode_( ANTLRTokenType tokenType );
>> 
<<
private:

   TFP_FabricInterface_c* pinterface_;
   TFP_FabricScanner_c*   pscanner_;
   string                 srFileName_;
   TFP_FabricFile_c*      pfabricFile_;
   TFM_FabricModel_c*     pfabricModel_;
>>

//===========================================================================//
start 
   :
   "<" FABRIC stringText[ &this->pfabricModel_->srName ] ">"
   (  "<"
      ( configDef[ &pfabricModel_->config ]
      |  inputOutputList[ &pfabricModel_->inputOutputList ]
      |  physicalBlockList[ &pfabricModel_->physicalBlockList ]
      |  switchBoxList[ &pfabricModel_->switchBoxList ]
      |  channelList[ &pfabricModel_->channelList ]
      |  segmentList[ &pfabricModel_->segmentList ]
      )
   )*
   "</" FABRIC ">"
   END_OF_FILE
   ;

//===========================================================================//
configDef[ TFM_Config_c* pconfig ]
   :
   CONFIG ">"
   (  "<"
      (  REGION regionDef[ &pconfig->fabricRegion ]
      )*
      ">"
   )
   "</" CONFIG ">"
   ;

//===========================================================================//
inputOutputList[ TFM_InputOutputList_t* pinputOutputList ]
   :
   <<
      TFM_InputOutput_t inputOutput;

      TC_SideName_c channel;
   >>
   IO 
   stringText[ &inputOutput.srName ]
   (  ( CELL | MASTER ) { EQUAL } stringText[ &inputOutput.srMasterName ]
   )*
   ">"
   (  "<" 
      (  REGION regionDef[ &inputOutput.region ]
      |  PIN pinList[ &inputOutput.pinList ]
      |  SLICE
         (  COUNT { EQUAL } uintNum[ &inputOutput.slice.count ]
         |  CAPACITY { EQUAL } uintNum[ &inputOutput.slice.capacity ]
         )* 
      |  TIMING
         (  ( CAP | CAP_IN ) { EQUAL } floatNum[ &inputOutput.timing.capInput ]
         |  ( T | DELAY ) { EQUAL } expNum[ &inputOutput.timing.delay ]
         )* 
      )
      ( { "/" } | "</" PIN ) ">"
   )*
   "</" IO ">"
   <<
      if( inputOutput.IsValid( ))
      {
         inputOutput.blockType = TFM_BLOCK_INPUT_OUTPUT;
         pinputOutputList->Add( inputOutput );
      }
   >>
   ;

//===========================================================================//
physicalBlockList[ TFM_PhysicalBlockList_t* pphysicalBlockList ]
   :
   <<
      TFM_PhysicalBlock_t physicalBlock;
   >>
   PB 
   stringText[ &physicalBlock.srName ]
   (  ( CELL | MASTER ) { EQUAL } stringText[ &physicalBlock.srMasterName ]
   )*
   ">"
   (  "<" 
      (  REGION regionDef[ &physicalBlock.region ]
      |  PIN pinList[ &physicalBlock.pinList ]
      |  SLICE
         (  COUNT { EQUAL } uintNum[ &physicalBlock.slice.count ]
         |  CAPACITY { EQUAL } uintNum[ &physicalBlock.slice.capacity ]
         )* 
      |  TIMING
         (  ( CAP | CAP_IN ) { EQUAL } floatNum[ &physicalBlock.timing.capInput ]
         |  ( T | DELAY ) { EQUAL } expNum[ &physicalBlock.timing.delay ]
         )* 
      )
      ( { "/" } | "</" PIN ) ">"
   )*
   "</" PB ">"
   <<
      if( physicalBlock.IsValid( ))
      {
         physicalBlock.blockType = TFM_BLOCK_PHYSICAL_BLOCK;
         pphysicalBlockList->Add( physicalBlock );
      }
   >>
   ;

//===========================================================================//
switchBoxList[ TFM_SwitchBoxList_t* pswitchBoxList ]
   :
   <<
      TFM_SwitchBox_t switchBox;

      TC_SideName_c channel;
   >>
   SB 
   stringText[ &switchBox.srName ]
   (  ( CELL | MASTER ) { EQUAL } stringText[ &switchBox.srMasterName ]
   )*
   ">"
   (  "<" 
      (  REGION regionDef[ &switchBox.region ]
      |  MAPPING sideMapTable[ &switchBox.mapTable ]
      |  TIMING
         (  ( R | RES ) { EQUAL } floatNum[ &switchBox.timing.res ]
         |  ( CAP | CAP_IN ) { EQUAL } floatNum[ &switchBox.timing.capInput ]
         |  CAP_OUT { EQUAL } floatNum[ &switchBox.timing.capOutput ]
         |  ( T | DELAY ) { EQUAL } expNum[ &switchBox.timing.delay ]
         )* 
      )
      ">"
   )*
   "</" SB ">"
   <<
      if( switchBox.IsValid( ))
      {
         switchBox.blockType = TFM_BLOCK_SWITCH_BOX;
         pswitchBoxList->Add( switchBox );
      }
   >>
   ;

//===========================================================================//
channelList[ TFM_ChannelList_t* pchannelList ]
   :
   <<
      TFM_Channel_c channel;
   >>
   CHANNEL 
   stringText[ &channel.srName ]
   (  ORIENT { EQUAL } orientVal:ORIENT_VAL
      << 
         channel.orient = this->FindOrientMode_( orientVal->getType( ));
      >>
   |  COUNT { EQUAL } uintNum[ &channel.count ]
   )*
   ">"
   (  "<" REGION regionDef[ &channel.region ] ">"
   )*
   "</" CHANNEL ">"
   <<
      if( channel.IsValid( ))
      {
         pchannelList->Add( channel );
      }
   >>
   ;

//===========================================================================//
segmentList[ TFM_SegmentList_t* psegmentList ]
   :
   <<
      TFM_Segment_c segment;
   >>
   SEGMENT 
   stringText[ &segment.srName ]
   (  INDEX { EQUAL } uintNum[ &segment.index ]
   |  WIDTH { EQUAL } floatNum[ &segment.path.width ]
   )*
   ">"
   (  "<" 
      (  LINE lineDef[ &segment.path.line ]
      |  TIMING
         (  ( R | RES ) { EQUAL } floatNum[ &segment.timing.res ]
         |  CAP { EQUAL } floatNum[ &segment.timing.cap ]
         )* 
      )
      ">"
   )*
   "</" SEGMENT ">"
   <<
      if( segment.IsValid( ))
      {
         psegmentList->Add( segment );
      }
   >>
   ;

//===========================================================================//
pinList[ TFM_PinList_t* ppinList ]
   :
   <<
      TFM_Pin_c pin;

      string srName;
   >>
   stringText[ &srName ]
   <<
      pin.SetName( srName );
   >>
   (  SIDE { EQUAL } sideVal:SIDE_VAL
      << 
         pin.side = this->FindSideMode_( sideVal->getType( ));
      >>
   |  OFFSET { EQUAL } floatNum[ &pin.offset ]
   |  WIDTH { EQUAL } floatNum[ &pin.width ]
   |  SLICE { EQUAL } uintNum[ &pin.slice ]
   )*
   ">"
   (  connectionPattern[ &pin.connectionPattern ]
   )*
   <<
      if( pin.IsValid( ))
      {
         ppinList->Add( pin );
      }
   >>
   ;

//===========================================================================//
connectionPattern[ TFM_BitPattern_t* pconnectionPattern ]
   :
   <<
      string srPattern;
   >>
   "<" CB
   (  bitStringVal:BIT_CHAR
      <<
         string srBit = bitStringVal->getText( );
         if( srBit.length( ))
         {
            TC_Bit_c bit( srBit[ 0 ] );
            pconnectionPattern->Add( bit );
         }
      >>
   )*
   ">"
   ;

//===========================================================================//
sideMapTable[ TC_MapTable_c* pmapTable ]
   :
   <<
      TC_SideIndex_c sideIndex;
      TC_SideMode_t side = TC_SIDE_UNDEFINED;
      size_t index;

      TC_SideList_t sideList;

      size_t curTokenLine, nextTokenLine;
   >>
   (  sideMapIndex[ &sideIndex ]
      <<
         if( side == TC_SIDE_UNDEFINED )
         {
            side = sideIndex.GetSide( );
            index = sideIndex.GetIndex( );
         }
         else
         {
            sideList.Add( sideIndex );
         }

         curTokenLine = LT( 0 )->getLine( );
         nextTokenLine = LT( 1 )->getLine( );
         if( curTokenLine != nextTokenLine )
         {
            pmapTable->Add( side, index, sideList );
            side = TC_SIDE_UNDEFINED;
            sideList.Clear( );
         }
      >>
   )+
   ;

//===========================================================================//
sideMapIndex[ TC_SideIndex_c* psideIndex ]
   :
   <<
      TC_SideMode_t side = TC_SIDE_UNDEFINED;
      unsigned int index = 0;
   >>
   sideVal:SIDE_VAL
   << 
      side = this->FindSideMode_( sideVal->getType( ));
   >>
   uintNum[ &index ]
   << 
      psideIndex->Set( side, index );
   >>
   ;

//===========================================================================//
regionDef[ TGS_Region_c* pregion ]
   :
   floatNum[ &pregion->x1 ]
   floatNum[ &pregion->y1 ]
   floatNum[ &pregion->x2 ]
   floatNum[ &pregion->y2 ]
   ;

//===========================================================================//
lineDef[ TGS_Line_c* pline ]
   :
   floatNum[ &pline->x1 ]
   floatNum[ &pline->y1 ]
   floatNum[ &pline->x2 ]
   floatNum[ &pline->y2 ]
   ;

//===========================================================================//
stringText[ string* psrString ]
   : 
   <<
      *psrString = "";
   >>
      OPEN_QUOTE
      { qstringVal:STRING 
      <<
         *psrString = qstringVal->getText( );
      >>
      } 
      CLOSE_QUOTE
   |  stringVal:STRING
      <<
         *psrString = stringVal->getText( );
      >>
   ;

//===========================================================================//
floatNum[ double* pdouble ]
   : 
      floatVal:FLOAT
      <<
         *pdouble = atof( floatVal->getText( ));
      >>
   |  posIntVal:POS_INT
      <<
         *pdouble = atof( posIntVal->getText( ));
      >>
   |  negIntVal:NEG_INT
      <<
         *pdouble = atof( negIntVal->getText( ));
      >>
   |  bitVal:BIT_CHAR
      <<
         *pdouble = atof( bitVal->getText( ));
      >>
   ;

//===========================================================================//
expNum[ double* pdouble ]
   : 
      expVal:EXP
      <<
         *pdouble = atof( expVal->getText( ));
      >>
   |  floatVal:FLOAT
      <<
         *pdouble = atof( floatVal->getText( ));
      >>
   |  posIntVal:POS_INT
      <<
         *pdouble = atof( posIntVal->getText( ));
      >>
   |  negIntVal:NEG_INT
      <<
         *pdouble = atof( negIntVal->getText( ));
      >>
   |  bitVal:BIT_CHAR
      <<
         *pdouble = atof( bitVal->getText( ));
      >>
   ;

//===========================================================================//
uintNum[ unsigned int* puint ]
   :
      uintVal:POS_INT
      <<
         *puint = static_cast< unsigned int >( atol( uintVal->getText( )));
      >>
   |  bitVal:BIT_CHAR
      <<
         *puint = static_cast< unsigned int >( atol( bitVal->getText( )));
      >>
   ;

//===========================================================================//

}

<<
#include "TFP_FabricGrammar.h"
>>
