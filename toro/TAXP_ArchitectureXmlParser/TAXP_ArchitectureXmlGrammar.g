//===========================================================================//
// Purpose: PCCTS grammar for the architecture spec (XML) file.
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

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TAS_ArchitectureSpec.h"

#include "TAXP_ArchitectureXmlFile.h"
#include "TAXP_ArchitectureXmlScanner_c.h"
>>

//===========================================================================//
#lexclass QUOTED_VALUE

#token CLOSE_QUOTE     "\""                    << mode( START ); >>
#token STRING          "~[\"\n]+"
#token UNCLOSED_STRING "[\n]"

//===========================================================================//
#lexclass START

#token                 "[\ \t]+"           << skip( ); >>
#token CPP_COMMENT     "//~[\n]*[\n]"      << skip( ); newline( ); >>
#token BLOCK_COMMENT   "#~[\n]*[\n]"       << skip( ); newline( ); >>
#token XML_COMMENT     "<\!\-\-~[\n]*[\n]" << skip( ); newline( ); >>
#token NEW_LINE        "[\n\\]"            << skip( ); newline( ); >>
#token END_OF_FILE     "@"
#token OPEN_QUOTE      "\""                << mode( QUOTED_VALUE ); >>
#token EQUAL           "="

#token ARCHITECTURE     "[Aa][Rr][Cc][Hh][Ii][Tt][Ee][Cc][Tt][Uu][Rr][Ee]"

#token LAYOUT           "[Ll][Aa][Yy][Oo][Uu][Tt]"
#token DEVICE           "[Dd][Ee][Vv][Ii][Cc][Ee]"
#token MODELS           "[Mm][Oo][Dd][Ee][Ll][Ss]"
#token SEGMENTLIST      "[Ss][Ee][Gg][Mm][Ee][Nn][Tt][Ll][Ii][Ss][Tt]"
#token SWITCHLIST       "[Ss][Ww][Ii][Tt][Cc][Hh][Ll][Ii][Ss][Tt]"
#token COMPLEXBLOCKLIST "[Cc][Oo][Mm][Pp][Ll][Ee][Xx][Bb][Ll][Oo][Cc][Kk][Ll][Ii][Ss][Tt]"

#token MODEL            "[Mm][Oo][Dd][Ee][Ll]"
#token SEGMENT          "[Ss][Ee][Gg][Mm][Ee][Nn][Tt]"
#token SWITCH           "[Ss][Ww][Ii][Tt][Cc][Hh]"
#token PB_TYPE          "[Pp][Bb][_][Tt][Yy][Pp][Ee]"

#token NAME             "[Nn][Aa][Mm][Ee]"
#token TYPE             "[Tt][Yy][Pp][Ee]"
#token FS               "[Ff][Ss]"

#token NUM_PB           "[Nn][Uu][Mm][_][Pp][Bb]"
#token CLASS            "[Cc][Ll][Aa][Ss][Ss]"
#token BLIF_MODEL       "[Bb][Ll][Ii][Ff][_][Mm][Oo][Dd][Ee][Ll]"

#token AUTO             "[Aa][Uu][Tt][Oo]"
#token WIDTH            "[Ww][Ii][Dd][Tt][Hh]"
#token HEIGHT           "[Hh][Ee][Ii][Gg][Hh][Tt]"
#token CAPACITY         "[Cc][Aa][Pp][Aa][Cc][Ii][Tt][Yy]"
#token LENGTH           "[Ll][Ee][Nn][Gg][Tt][Hh]"

#token SIZING           "[Ss][Ii][Zz][Ii][Nn][Gg]"
#token TIMING           "[Tt][Ii][Mm][Ii][Nn][Gg]"
#token AREA             "[Aa][Rr][Ee][Aa]"
#token SWITCH_BLOCK     "[Ss][Ww][Ii][Tt][Cc][Hh][_][Bb][Ll][Oo][Cc][Kk]"

#token R_MINW_NMOS      "[Rr][_][Mm][Ii][Nn][Ww][_][Nn][Mm][Oo][Ss]"
#token R_MINW_PMOS      "[Rr][_][Mm][Ii][Nn][Ww][_][Pp][Mm][Oo][Ss]"
#token IPIN_MUX_SIZE    "[Ii][Pp][Ii][Nn][_][Mm][Uu][Xx][_][Tt][Rr][Aa][Nn][Ss][_][Ss][Ii][Zz][Ee]"
#token C_IPIN_CBLOCK    "[Cc][_][Ii][Pp][Ii][Nn][_][Cc][Bb][Ll][Oo][Cc][Kk]"
#token T_IPIN_CBLOCK    "[Tt][_][Ii][Pp][Ii][Nn][_][Cc][Bb][Ll][Oo][Cc][Kk]"
#token GRID_LOGIC_AREA  "[Gg][Rr][Ii][Dd][_][Ll][Oo][Gg][Ii][Cc][_][Tt][Ii][Ll][Ee][_][Aa][Rr][Ee][Aa]"
#token CHAN_WIDTH_DISTR "[Cc][Hh][Aa][Nn][_][Ww][Ii][Dd][Tt][Hh][_][Dd][Ii][Ss][Tt][Rr]"

#token DISTR            "[Dd][Ii][Ss][Tt][Rr]"
#token PEAK             "[Pp][Ee][Aa][Kk]"
#token XPEAK            "[Xx][Pp][Ee][Aa][Kk]"
#token DC               "[Dd][Cc]"
#token FREQ             "[Ff][Rr][Ee][Qq]"
#token RMETAL           "[Rr][Mm][Ee][Tt][Aa][Ll]"
#token CMETAL           "[Cc][Mm][Ee][Tt][Aa][Ll]"

#token MUX              "[Mm][Uu][Xx]"
#token WIRE_SWITCH      "[Ww][Ii][Rr][Ee][_][Ss][Ww][Ii][Tt][Cc][Hh]"
#token OPIN_SWITCH      "[Oo][Pp][Ii][Nn][_][Ss][Ww][Ii][Tt][Cc][Hh]"

#token R                "[Rr]"
#token C                "[Cc]"
#token CIN              "[Cc][Ii][Nn]"
#token COUT             "[Cc][Oo][Uu][Tt]"
#token TDEL             "[Tt][Dd][Ee][Ll]"
#token BUF_SIZE         "[Bb][Uu][Ff][_][Ss][Ii][Zz][Ee]"
#token MUX_TRANS_SIZE   "[Mm][Uu][Xx][_][Tt][Rr][Aa][Nn][Ss][_][Ss][Ii][Zz][Ee]"

#token X                "[Xx]"
#token Y                "[Yy]"
#token IO               "[Ii][Oo]"
#token INPUT_PORTS      "[Ii][Nn][Pp][Uu][Tt][_][Pp][Oo][Rr][Tt][Ss]"
#token OUTPUT_PORTS     "[Oo][Uu][Tt][Pp][Uu][Tt][_][Pp][Oo][Rr][Tt][Ss]"
#token PORT             "[Pp][Oo][Rr][Tt]"
#token IS_CLOCK         "[Ii][Ss][_][Cc][Ll][Oo][Cc][Kk]"

#token INTERCONNECT     "[Ii][Nn][Tt][Ee][Rr][Cc][Oo][Nn][Nn][Ee][Cc][Tt]"

#token FC_IN            "[Ff][Cc][_][Ii][Nn]"
#token FC_OUT           "[Ff][Cc][_][Oo][Uu][Tt]"
#token FC               "[Ff][Cc]"

#token DEFAULT_IN_TYPE  "[Dd][Ee][Ff][Aa][Uu][Ll][Tt][_][Ii][Nn][_][Tt][Yy][Pp][Ee]"
#token DEFAULT_IN_VAL   "[Dd][Ee][Ff][Aa][Uu][Ll][Tt][_][Ii][Nn][_][Vv][Aa][Ll]"
#token DEFAULT_OUT_TYPE "[Dd][Ee][Ff][Aa][Uu][Ll][Tt][_][Oo][Uu][Tt][_][Tt][Yy][Pp][Ee]"
#token DEFAULT_OUT_VAL  "[Dd][Ee][Ff][Aa][Uu][Ll][Tt][_][Oo][Uu][Tt][_][Vv][Aa][Ll]"

#token PINLOCATIONS     "[Pp][Ii][Nn][Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn][Ss]"
#token GRIDLOCATIONS    "[Gg][Rr][Ii][Dd][Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn][Ss]"
#token PATTERN          "[Pp][Aa][Tt][Tt][Ee][Rr][Nn]"

#token DELAY            "[Dd][Ee][Ll][Aa][Yy]"
#token DELAY_CONSTANT   "[Dd][Ee][Ll][Aa][Yy][_][Cc][Oo][Nn][Ss][Tt][Aa][Nn][Tt]"
#token DELAY_MATRIX     "[Dd][Ee][Ll][Aa][Yy][_][Mm][Aa][Tt][Rr][Ii][Xx]"
#token T_SETUP          "([Tt]|[Cc][Ll][Oo][Cc][Kk])[_][Ss][Ee][Tt][Uu][Pp]"
#token T_HOLD           "([Tt]|[Cc][Ll][Oo][Cc][Kk])[_][Hh][Oo][Ll][Dd]"
#token T_CLOCK_TO_Q     "([Tt]|[Cc][Ll][Oo][Cc][Kk]){[_][Cc][Ll][Oo][Cc][Kk]}[_][Tt][Oo][_][Qq]"
#token C_CONSTANT       "[Cc]{[Aa][Pp]}[_][Cc][Oo][Nn][Ss][Tt][Aa][Nn][Tt]"
#token C_MATRIX         "[Cc]{[Aa][Pp]}[_][Mm][Aa][Tt][Rr][Ii][Xx]"
#token PACK_PATTERN     "[Pp][Aa][Cc][Kk][_][Pp][Aa][Tt][Tt][Ee][Rr][Nn]"

#token PRIORITY         "[Pp][Rr][Ii][Oo][Rr][Ii][Tt][Yy]"
#token MULTIPLE_START   "[Ss][Tt][Aa][Rr][Tt]"
#token MULTIPLE_REPEAT  "[Rr][Ee][Pp][Ee][Aa][Tt]"
#token SINGLE_POS       "[Pp][Oo][Ss]"

#token SB         "[Ss][Bb]"
#token CB         "[Cc][Bb]"

#token INPUT      "[Ii][Nn][Pp][Uu][Tt]"
#token OUTPUT     "[Oo][Uu][Tt][Pp][Uu][Tt]"
#token CLOCK      "[Cc][Ll][Oo][Cc][Kk]"

#token NUM_PINS   "[Nn][Uu][Mm][_][Pp][Ii][Nn][Ss]"
#token PORT_CLASS "[Pp][Oo][Rr][Tt][_][Cc][Ll][Aa][Ss][Ss]"
#token EQUIVALENT "[Ee][Qq][Uu][Ii][Vv][Aa][Ll][Ee][Nn][Tt]"
#token MODE       "[Mm][Oo][Dd][Ee]"

#token COMPLETE   "[Cc][Oo][Mm][Pp][Ll][Ee][Tt][Ee]"
#token DIRECT     "[Dd][Ii][Rr][Ee][Cc][Tt]"

#token LOC        "[Ll][Oo][Cc]"
#token SIDE       "[Ss][Ii][Dd][Ee]"
#token OFFSET     "[Oo][Ff][Ff][Ss][Ee][Tt]"

#token MIN        "[Mm][Ii][Nn]"
#token MAX        "[Mm][Aa][Xx]"
#token IN_PORT    "[Ii][Nn][_][Pp][Oo][Rr][Tt]"
#token OUT_PORT   "[Oo][Uu][Tt][_][Pp][Oo][Rr][Tt]"
#token VALUE      "[Vv][Aa][Ll][Uu][Ee]"

#token BOOL_TRUE  "[Tt][Rr][Uu][Ee]"
#token BOOL_FALSE "[Ff][Aa][Ll][Ss][Ee]"

#token BIT_CHAR   "[01]"
#token NEG_INT    "[\-][0-9]+"
#token POS_INT    "[0-9]+"
#token FLOAT      "{\-}{[0-9]+}.[0-9]+"
#token EXP        "{\-}{[0-9]+.}[0-9]+[Ee][\+\-][0-9]+"
#token STRING     "[a-zA-Z_/\|][a-zA-Z0-9_/\|\(\)\[\]\.\+\-\~]*"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//

class TAXP_ArchitectureXmlParser_c
{
<<
public:

   void syn( ANTLRAbstractToken* /* pToken */, 
             ANTLRChar*          pszGroup,
             SetWordType*        /* pWordType */,
             ANTLRTokenType      tokenType,
             int                 /* k */ );

   void SetInterface( TAXP_ArchitectureXmlInterface_c* pinterface );
   void SetScanner( TAXP_ArchitectureXmlScanner_c* pscanner );
   void SetFileName( const char* pszFileName );
   void SetArchitectureXmlFile( TAXP_ArchitectureXmlFile_c* parchitectureXmlFile );
   void SetArchitectureSpec( TAS_ArchitectureSpec_c* parchitectureSpec );
>> 
<<
private:

   TAXP_ArchitectureXmlInterface_c* pinterface_;
   TAXP_ArchitectureXmlScanner_c*   pscanner_;
   string                           srFileName_;
   TAXP_ArchitectureXmlFile_c*      parchitectureXmlFile_;
   TAS_ArchitectureSpec_c*          parchitectureSpec_;
>>

//===========================================================================//
start 
   : 
   "<" ARCHITECTURE ">"
   (  "<"
      (  layoutDef[ &this->parchitectureSpec_->config ]
      |  deviceDef[ &this->parchitectureSpec_->config ]
      |  complexBlockList[ &this->parchitectureSpec_->physicalBlockList ]
      |  cellList[ &this->parchitectureSpec_->cellList ]
      |  switchBoxList[ &this->parchitectureSpec_->switchBoxList ]
      |  segmentList[ &this->parchitectureSpec_->segmentList ]
      )
   )* 
   "</" ARCHITECTURE ">"
   END_OF_FILE
   ;

//===========================================================================//
layoutDef[ TAS_Config_c* pconfig ]
   :
   LAYOUT
   {  AUTO EQUAL floatNum[ &pconfig->layout.autoSize.aspectRatio ]
      <<
         pconfig->layout.sizeMode = TAS_ARRAY_SIZE_AUTO;
      >>
   |  WIDTH EQUAL intNum[ &pconfig->layout.manualSize.gridDims.dx ]
      HEIGHT EQUAL intNum[ &pconfig->layout.manualSize.gridDims.dy ]
      <<
         pconfig->layout.sizeMode = TAS_ARRAY_SIZE_MANUAL;
      >>
   }
   "/>"
   ;

//===========================================================================//
deviceDef[ TAS_Config_c* pconfig ]
   :
   DEVICE ">"
   (  "<"
      (  SIZING 
         (  R_MINW_NMOS EQUAL floatNum[ &pconfig->device.areaModel.resMinWidthNMOS ]
         |  R_MINW_PMOS EQUAL floatNum[ &pconfig->device.areaModel.resMinWidthPMOS ]
         |  IPIN_MUX_SIZE EQUAL floatNum[ &pconfig->device.areaModel.sizeInputPinMux ]
         )*
         "/>"
      |  AREA
         (  GRID_LOGIC_AREA EQUAL floatNum[ &pconfig->device.areaModel.areaGridTile ]
         )*
         "/>"
      |  CHAN_WIDTH_DISTR ">"
         (  channelWidth[ &pconfig->device.channelWidth.io,
                          &pconfig->device.channelWidth.x,
                          &pconfig->device.channelWidth.y ]
         )*
         "</" CHAN_WIDTH_DISTR ">"
      |  SWITCH_BLOCK
         (  TYPE EQUAL switchBoxModelType[ &pconfig->device.switchBoxes.modelType ]
         |  FS EQUAL uintNum[ &pconfig->device.switchBoxes.fs ]
         )*
         "/>"
      |  TIMING
         (  C_IPIN_CBLOCK EQUAL expNum[ &pconfig->device.connectionBoxes.capInput ]
         |  T_IPIN_CBLOCK EQUAL expNum[ &pconfig->device.connectionBoxes.delayInput ]
         )*
         "/>"
      )
   )*
   "</" DEVICE ">"
   ;

//===========================================================================//
cellList[ TAS_CellList_t* pcellList ]
   : 
   MODELS ">"
   (  cellDef[ pcellList ]
   )*
   "</" MODELS ">"
   ;

//===========================================================================//
cellDef[ TAS_CellList_t* pcellList ]
   : 
   << 
       TAS_Cell_c cell;

       string srName;
       TLO_PortList_t portList;
   >>
   "<" MODEL NAME EQUAL stringText[ &srName ] ">"
   <<
      cell.SetName( srName );
      cell.SetSource( TLO_CELL_SOURCE_CUSTOM );
   >>
   (  "<"
      (  inputPortList[ &portList ]
      |  outputPortList[ &portList ]
      )
   )*
   <<
      if( portList.IsValid( ))
      {
         cell.SetPortList( portList );
      }
   >>
   "</" MODEL ">"
   <<
      if( cell.IsValid( ))
      {
         pcellList->Add( cell );
      }
   >>
   ;

//===========================================================================//
switchBoxList[ TAS_SwitchBoxList_t* pswitchBoxList ]
   : 
   SWITCHLIST ">"
   (  switchBoxDef[ pswitchBoxList ]
   )*
   "</" SWITCHLIST ">"
   ;

//===========================================================================//
switchBoxDef[ TAS_SwitchBoxList_t* pswitchBoxList ]
   : 
   << 
      TAS_SwitchBox_c switchBox;
   >>
   "<" SWITCH 
   (  NAME EQUAL stringText[ &switchBox.srName ]
   |  TYPE EQUAL switchBoxType[ &switchBox.type ]
   |  R EQUAL floatNum[ &switchBox.timing.res ]
   |  CIN EQUAL expNum[ &switchBox.timing.capInput ]
   |  COUT EQUAL expNum[ &switchBox.timing.capOutput ]
   |  TDEL EQUAL expNum[ &switchBox.timing.delay ]
   |  BUF_SIZE EQUAL floatNum[ &switchBox.area.buffer ]
   |  MUX_TRANS_SIZE EQUAL floatNum[ &switchBox.area.muxTransistor ]
   )*
   "/>"
   <<
      if( switchBox.IsValid( ))
      {
         pswitchBoxList->Add( switchBox );
      }
   >>
   ;

//===========================================================================//
segmentList[ TAS_SegmentList_t* psegmentList ]
   : 
   SEGMENTLIST ">"
   (  segmentDef[ psegmentList ]
   )*
   "</" SEGMENTLIST ">"
   ;

//===========================================================================//
segmentDef[ TAS_SegmentList_t* psegmentList ]
   : 
   << 
      TAS_Segment_c segment;
   >>
   "<" SEGMENT 
   (  LENGTH EQUAL segmentLength[ &segment.length ]
   |  TYPE EQUAL segmentDirType[ &segment.dirType ]
   |  FREQ EQUAL floatNum[ &segment.trackFreq ]
   |  RMETAL EQUAL floatNum[ &segment.timing.res ]
   |  CMETAL EQUAL floatNum[ &segment.timing.cap ]
   )*
   ">"
   (  "<"
      ( sbList[ &segment.sbPattern ]
      | cbList[ &segment.cbPattern ]
      | MUX NAME EQUAL stringText[ &segment.srMuxSwitchName ] "/>"
      | WIRE_SWITCH NAME EQUAL stringText[ &segment.srWireSwitchName ] "/>"
      | OPIN_SWITCH NAME EQUAL stringText[ &segment.srOutputSwitchName ] "/>"
      )
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
complexBlockList[ TAS_PhysicalBlockList_t* pphysicalBlockList ]
   : 
   COMPLEXBLOCKLIST ">"
   (  "<" pbtypeList[ pphysicalBlockList ]
   )*
   "</" COMPLEXBLOCKLIST ">"
   ;

//===========================================================================//
pbtypeList[ TAS_PhysicalBlockList_t* pphysicalBlockList ]
   : 
   PB_TYPE
   pbtypeDef[ pphysicalBlockList ]
   "</" PB_TYPE ">"
   ;

//===========================================================================//
pbtypeDef[ TAS_PhysicalBlockList_t* pphysicalBlockList ]
   : 
   <<
      TAS_PhysicalBlock_c physicalBlock;

      unsigned int ignored;
   >>
   NAME EQUAL stringText[ &physicalBlock.srName ]
   (  HEIGHT EQUAL uintNum[ &physicalBlock.height ]
   |  CAPACITY EQUAL uintNum[ &physicalBlock.capacity ]
   |  AREA EQUAL uintNum[ &ignored ]
   |  NUM_PB EQUAL uintNum[ &physicalBlock.numPB ]
   |  CLASS EQUAL classText[ &physicalBlock.classType ]
   |  BLIF_MODEL EQUAL blifModelText[ &physicalBlock.srModelName, 
                                      &physicalBlock.modelType ]
   )*
   ">"
   (  "<"
      (  fcDef[ &physicalBlock.fcIn,
                &physicalBlock.fcOut ]
      |  modeDef[ &physicalBlock.modeList ]
      |  pbtypeList[ &physicalBlock.physicalBlockList ]
      |  interconnectList[ &physicalBlock.interconnectList ] 
      |  portDef[ &physicalBlock.portList ]
      |  pinAssignList[ &physicalBlock.pinAssignPattern,
                        &physicalBlock.pinAssignList ] 
      |  gridAssignList[ &physicalBlock.gridAssignList ]
      |  timingDelayLists[ &physicalBlock.timingDelayLists ]  
      )
   )*
   <<
      if( physicalBlock.IsValid( ))
      {
         pphysicalBlockList->Add( physicalBlock );
      }
   >>
   ;

//===========================================================================//
sbList[ TAS_BitPattern_t* psbPattern ]
   :
   <<
      string srPattern;
   >>
   SB TYPE EQUAL stringText[ &srPattern ] ">"
   (  bitStringVal:BIT_CHAR
      <<
         if( TC_stricmp( srPattern.data( ), "pattern" ) == 0 )
         {
            string srBit = bitStringVal->getText( );
            if( srBit.length( ))
            {
               TC_Bit_c bit( srBit[ 0 ] );
               psbPattern->Add( bit );
            }
         }
      >>
   )*
   "</" SB ">"
   ;

//===========================================================================//
cbList[ TAS_BitPattern_t* pcbPattern ]
   :
   <<
      string srPattern;
   >>
   CB TYPE EQUAL stringText[ &srPattern ] ">"
   (  bitStringVal:BIT_CHAR
      <<
         if( TC_stricmp( srPattern.data( ), "pattern" ) == 0 )
         {
            string srBit = bitStringVal->getText( );
            if( srBit.length( ))
            {
               TC_Bit_c bit( srBit[ 0 ] );
               pcbPattern->Add( bit );
            }
         }
      >>
   )*
   "</" CB ">"
   ;

//===========================================================================//
inputPortList[ TLO_PortList_t* pportList ]
   :
   <<
      string srName;
      string srIsClock;
      TC_TypeMode_t type;
   >>
   INPUT_PORTS ">"
   (  "<" PORT NAME EQUAL stringText[ &srName ]
      <<
         type = TC_TYPE_INPUT;
      >>
      {  IS_CLOCK EQUAL stringText[ &srIsClock ]
         <<
            if(( srIsClock.length( ) == 1 ) && ( srIsClock[ 0 ] == '1' ))
            {
               type = TC_TYPE_CLOCK;
            }
         >>
      }
      "/>"
      <<
         TLO_Port_c port( srName, type );
         pportList->Add( port );
      >>
   )*
   "</" INPUT_PORTS ">"
   ;

//===========================================================================//
outputPortList[ TLO_PortList_t* pportList ]
   :
   <<
      string srName;
   >>
   OUTPUT_PORTS ">"
   (  "<" PORT NAME EQUAL stringText[ &srName ] 
      "/>"
      <<
         TLO_Port_c port( srName, TC_TYPE_OUTPUT );
         pportList->Add( port );
      >>
   )*
   "</" OUTPUT_PORTS ">"
   ;

//===========================================================================//
fcDef[ TAS_ConnectionFc_c* pfcIn,
       TAS_ConnectionFc_c* pfcOut ]
   :
   <<
      double floatInValue = 0.0;
      double floatOutValue = 0.0;
      unsigned int uintInValue = 0;
      unsigned int uintOutValue = 0;
   >>
   (  FC_IN TYPE EQUAL connectionBoxInType[ &pfcIn->type ] ">"
      { floatNum[ &floatInValue ] } 
      "</" FC_IN ">"
      <<
         uintInValue = static_cast< unsigned int >( floatInValue + TC_FLT_EPSILON );
         pfcIn->percent = ( pfcIn->type == TAS_CONNECTION_BOX_FRACTION ? floatInValue : 0.0 );
         pfcIn->absolute = ( pfcIn->type == TAS_CONNECTION_BOX_ABSOLUTE ? uintInValue : 0 );
      >>
   |  FC_OUT TYPE EQUAL connectionBoxOutType[ &pfcOut->type ] ">"
      { floatNum[ &floatOutValue ] } 
      "</" FC_OUT ">"
      <<
         uintOutValue = static_cast< unsigned int >( floatOutValue + TC_FLT_EPSILON );
         pfcOut->percent = ( pfcOut->type == TAS_CONNECTION_BOX_FRACTION ? floatOutValue : 0.0 );
         pfcOut->absolute = ( pfcOut->type == TAS_CONNECTION_BOX_ABSOLUTE ? uintOutValue : 0 );
      >>
   |  FC
      DEFAULT_IN_TYPE EQUAL connectionBoxInType[ &pfcIn->type ]
      { DEFAULT_IN_VAL EQUAL floatNum[ &floatInValue ] }
      <<
         uintInValue = static_cast< unsigned int >( floatInValue + TC_FLT_EPSILON );
         pfcIn->percent = ( pfcIn->type == TAS_CONNECTION_BOX_FRACTION ? floatInValue : 0.0 );
         pfcIn->absolute = ( pfcIn->type == TAS_CONNECTION_BOX_ABSOLUTE ? uintInValue : 0 );
      >>
      DEFAULT_OUT_TYPE EQUAL connectionBoxInType[ &pfcOut->type ]
      { DEFAULT_OUT_VAL EQUAL floatNum[ &floatOutValue ] }
      <<
         uintOutValue = static_cast< unsigned int >( floatOutValue + TC_FLT_EPSILON );
         pfcOut->percent = ( pfcOut->type == TAS_CONNECTION_BOX_FRACTION ? floatOutValue : 0.0 );
         pfcOut->absolute = ( pfcOut->type == TAS_CONNECTION_BOX_ABSOLUTE ? uintOutValue : 0 );
      >>
      "/>"
   )
   ;

//===========================================================================//
modeDef[ TAS_ModeList_t* pmodeList ]
   :
   << 
      TAS_Mode_c mode;
   >>
   MODE NAME EQUAL stringText[ &mode.srName ] ">"
   (  "<"
      (  pbtypeList[ &mode.physicalBlockList ]
      |  interconnectList[ &mode.interconnectList ]
      )*
   )*
   "</" MODE ">"
   <<
      if( mode.IsValid( ))
      {
         pmodeList->Add( mode );
      }
   >>
   ;

//===========================================================================//
interconnectList[ TAS_InterconnectList_t* pinterconnectList ]
   :
   INTERCONNECT ">"
   (  interconnectDef[ pinterconnectList ]
   )*
   "</" INTERCONNECT ">"
   ;


//===========================================================================//
interconnectDef[ TAS_InterconnectList_t* pinterconnectList ]
   :
   << 
      TAS_Interconnect_c interconnect;
   >>
   "<"
   (  COMPLETE interconnectElem[ &interconnect, TAS_INTERCONNECT_MAP_COMPLETE ]
      ( "/>" | "</" COMPLETE ">" )
   |  DIRECT interconnectElem[ &interconnect, TAS_INTERCONNECT_MAP_DIRECT ]
      ( "/>" | "</" DIRECT ">" )
   |  MUX interconnectElem[ &interconnect, TAS_INTERCONNECT_MAP_MUX ]
      ( "/>" | "</" MUX ">" )
   )
   <<
      if( interconnect.IsValid( ))
      {
         pinterconnectList->Add( interconnect );
      }
   >>
   ;

//===========================================================================//
interconnectElem[ TAS_Interconnect_c* pinterconnect,
                  TAS_InterconnectMapType_t mapType ]
   :
   << 
      pinterconnect->mapType = mapType;
      string srInputName, srOutputName;
   >>
   (  NAME EQUAL stringText[ &pinterconnect->srName ]
   |  INPUT EQUAL stringText[ &srInputName ]
      <<
         pinterconnect->inputNameList.Add( srInputName );
      >>
   |  OUTPUT EQUAL stringText[ &srOutputName ]
      <<
         pinterconnect->outputNameList.Add( srOutputName );
      >>
   )+
   {  ">"
      (  "<"
         timingDelayLists[ &pinterconnect->timingDelayLists ]  
      )*
   }
   ;

//===========================================================================//
portDef[ TLO_PortList_t* pportList ]
   :
   (  INPUT portTypeDef[ pportList, TC_TYPE_INPUT ]
   |  OUTPUT portTypeDef[ pportList, TC_TYPE_OUTPUT ]
   |  CLOCK portTypeDef[ pportList, TC_TYPE_CLOCK ]
   )
   "/>"
   ;

//===========================================================================//
portTypeDef[ TLO_PortList_t* pportList, TC_TypeMode_t type ]
   :
   <<
      TLO_Port_c port;

      string srName;
      unsigned int count = 1;
      bool isEquivalent = false;
      string srClass;
   >>
   NAME EQUAL stringText[ &srName ] 
   (  NUM_PINS EQUAL uintNum[ &count ] 
   |  EQUIVALENT EQUAL boolType[ &isEquivalent ] 
   |  PORT_CLASS EQUAL stringText[ &srClass ] 
   )*
   <<
      port.SetName( srName );
      port.SetType( type );
      port.SetCount( count );
      port.SetEquivalent( isEquivalent );
      port.SetClass( srClass );

      pportList->Add( port );
   >>
   ;

//===========================================================================//
pinAssignList[ TAS_PinAssignPatternType_t* ppinAssignPattern,
               TAS_PinAssignList_t* ppinAssignList ] 
   :
   <<
      TAS_PinAssign_c pinAssign;
   >>
   PINLOCATIONS PATTERN EQUAL pinAssignPatternType[ ppinAssignPattern ] 
   {  ">"
      (  pinAssignDef[ *ppinAssignPattern, ppinAssignList ]
      )*
   }
   ( "/>" | "</" PINLOCATIONS ">" )
   ;

//===========================================================================//
pinAssignDef[ TAS_PinAssignPatternType_t pinAssignPattern,
              TAS_PinAssignList_t* ppinAssignList ] 
   :
   <<
      TAS_PinAssign_c pinAssign;

      string srPortName;
   >>
   "<" LOC 
   (  SIDE EQUAL sideMode[ &pinAssign.side ] 
   |  OFFSET EQUAL uintNum[ &pinAssign.offset ]
   )*
   ">"
   (  stringText[ &srPortName ] 
      <<
         pinAssign.portNameList.Add( srPortName );
      >>
   )*
   "</" LOC ">"
   <<
      pinAssign.pattern = pinAssignPattern;
      if( pinAssign.IsValid( ))
      {
         ppinAssignList->Add( pinAssign );
      }
   >>
   ;

//===========================================================================//
gridAssignList[ TAS_GridAssignList_t* pgridAssignList ]
   :
   GRIDLOCATIONS
   {  ">"
      (  gridAssignDef[ pgridAssignList ]
      )*
   }
   ( "/>" | "</" GRIDLOCATIONS ">" )
   ;

//===========================================================================//
gridAssignDef[ TAS_GridAssignList_t* pgridAssignList ]
   :
   <<
      TAS_GridAssign_c gridAssign;
   >>
   "<" LOC TYPE EQUAL gridAssignDistrMode[ &gridAssign.distr ]
   (  PRIORITY EQUAL uintNum[ &gridAssign.priority ]
   |  MULTIPLE_START EQUAL uintNum[ &gridAssign.multipleStart ]
   |  MULTIPLE_REPEAT EQUAL uintNum[ &gridAssign.multipleRepeat ]
   |  SINGLE_POS EQUAL floatNum[ &gridAssign.singlePercent ]
   )*
   "/>"
   <<
      if( gridAssign.IsValid( ))
      {
         pgridAssignList->Add( gridAssign );
      }
   >>
   ;

//===========================================================================//
timingDelayLists[ TAS_TimingDelayLists_c* ptimingDelayLists ]  
   :
   <<
      TAS_TimingDelay_c timingDelay;
   >>
   (  DELAY_CONSTANT 
      (  MIN EQUAL expNum[ &timingDelay.valueMin ] 
         << timingDelay.type = TAS_TIMING_TYPE_MIN_VALUE; >>
      |  MAX EQUAL expNum[ &timingDelay.valueMax ] 
         << timingDelay.type = TAS_TIMING_TYPE_MAX_VALUE; >>
      |  IN_PORT EQUAL stringText[ &timingDelay.srInputPortName ]
      |  OUT_PORT EQUAL stringText[ &timingDelay.srOutputPortName ]
      )*
      "/>"
      <<
         timingDelay.mode = TAS_TIMING_MODE_DELAY_CONSTANT;
         ptimingDelayLists->delayList.Add( timingDelay );
      >>
   |  DELAY_MATRIX
      (  TYPE EQUAL timingType[ &timingDelay.type ]
      |  IN_PORT EQUAL stringText[ &timingDelay.srInputPortName ]
      |  OUT_PORT EQUAL stringText[ &timingDelay.srOutputPortName ]
      )*
      ">"
      timingValueMatrixDef[ &timingDelay.valueMatrix ]
      "</" DELAY_MATRIX ">"
      <<
         timingDelay.mode = TAS_TIMING_MODE_DELAY_MATRIX;
         ptimingDelayLists->delayMatrixList.Add( timingDelay );
      >>
   |  T_SETUP 
      (  VALUE EQUAL expNum[ &timingDelay.valueNom ] 
      |  PORT EQUAL stringText[ &timingDelay.srInputPortName ]
      |  CLOCK EQUAL stringText[ &timingDelay.srClockPortName ]
      )*
      "/>"
      <<
         timingDelay.mode = TAS_TIMING_MODE_T_SETUP;
         ptimingDelayLists->tSetupList.Add( timingDelay );
      >>
   |  T_HOLD 
      (  VALUE EQUAL expNum[ &timingDelay.valueNom ] 
      |  PORT EQUAL stringText[ &timingDelay.srInputPortName ]
      |  CLOCK EQUAL stringText[ &timingDelay.srClockPortName ]
      )*
      "/>"
      <<
         timingDelay.mode = TAS_TIMING_MODE_T_HOLD;
         ptimingDelayLists->tHoldList.Add( timingDelay );
      >>
   |  T_CLOCK_TO_Q
      (  MIN EQUAL expNum[ &timingDelay.valueMin ] 
         << timingDelay.type = TAS_TIMING_TYPE_MIN_VALUE; >>
      |  MAX EQUAL expNum[ &timingDelay.valueMax ] 
         << timingDelay.type = TAS_TIMING_TYPE_MAX_VALUE; >>
      |  PORT EQUAL stringText[ &timingDelay.srOutputPortName ]
      |  CLOCK EQUAL stringText[ &timingDelay.srClockPortName ]
      )*
      "/>"
      <<
         timingDelay.mode = TAS_TIMING_MODE_CLOCK_TO_Q;
         ptimingDelayLists->clockToQList.Add( timingDelay );
      >>
   |  C_CONSTANT 
      (  C EQUAL floatNum[ &timingDelay.valueNom ] 
      |  IN_PORT EQUAL stringText[ &timingDelay.srInputPortName ]
      |  OUT_PORT EQUAL stringText[ &timingDelay.srOutputPortName ]
      )*
      "/>"
      <<
         timingDelay.mode = TAS_TIMING_MODE_CAP_CONSTANT;
         ptimingDelayLists->capList.Add( timingDelay );
      >>
   |  C_MATRIX
      (  IN_PORT EQUAL stringText[ &timingDelay.srInputPortName ]
      |  OUT_PORT EQUAL stringText[ &timingDelay.srOutputPortName ]
      )*
      ">"
      timingValueMatrixDef[ &timingDelay.valueMatrix ]
      "</" C_MATRIX ">"
      <<
         timingDelay.mode = TAS_TIMING_MODE_CAP_MATRIX;
         ptimingDelayLists->capMatrixList.Add( timingDelay );
      >>
   |  PACK_PATTERN
      (  NAME EQUAL stringText[ &timingDelay.srPackPatternName ]
      |  IN_PORT EQUAL stringText[ &timingDelay.srInputPortName ]
      |  OUT_PORT EQUAL stringText[ &timingDelay.srOutputPortName ]
      )*
      "/>"
      <<
         timingDelay.mode = TAS_TIMING_MODE_PACK_PATTERN;
         ptimingDelayLists->packPatternList.Add( timingDelay );
      >>
   )
   ;

//===========================================================================//
timingValueMatrixDef[ TAS_TimingValueMatrix_t* pvalueMatrix ]
   :
   <<
      TAS_TimingValueTable_t valueTable;
      TAS_TimingValueList_t valueList;

      double value;
      size_t curTokenLine, nextTokenLine;
   >>
   (  expNum[ &value ]
      <<
         valueList.Add( value );

         curTokenLine = LT( 0 )->getLine( );
         nextTokenLine = LT( 1 )->getLine( );
         if( curTokenLine != nextTokenLine )
         {
            valueTable.Add( valueList );
            valueList.Clear( );
         }
      >>
   )+
   <<
      if( valueTable.IsValid( ) && valueTable[ 0 ]->IsValid( ))
      {
         size_t height = valueTable.GetLength( );
         size_t width = SIZE_MAX;
         for( size_t i = 0; i < valueTable.GetLength( ); ++i )
         {
            width = TCT_Min( width, valueTable[ i ]->GetLength( ));
         }

         value = 0.0;
         pvalueMatrix->SetCapacity( width, height, value );
   
         for( size_t j = 0; j < height; ++j )
         {
            const TAS_TimingValueList_t& valueList_ = *valueTable[ j ];
            for( size_t i = 0; i < width; ++i )
            {
               ( *pvalueMatrix )[i][j] = *valueList_[ i ];
            }
         }
      }
   >>
   ;

//===========================================================================//
segmentLength[ unsigned int* plength ]
   :
   <<
      string srLength;
   >>
   stringText[ &srLength ]
   <<
      if( TC_stricmp( srLength.data( ), "longline" ) == 0 )
         *plength = UINT_MAX;
      else
         *plength = static_cast< unsigned int >( atol( srLength.data( )));
   >>
   ;

//===========================================================================//
timingType[ TAS_TimingType_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      if( TC_stricmp( srType.data( ), "min" ) == 0 )
      {
         *ptype = TAS_TIMING_TYPE_MIN_MATRIX;
      }
      else if( TC_stricmp( srType.data( ), "max" ) == 0 )
      {
         *ptype = TAS_TIMING_TYPE_MAX_MATRIX;
      }
      else
      {
         *ptype = TAS_TIMING_TYPE_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid type, expected \"min\" or \"max\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
segmentDirType[ TAS_SegmentDirType_t* pdirType ]
   :
   <<
      string srDirType;
   >>
   stringText[ &srDirType ]
   <<
      if( TC_stricmp( srDirType.data( ), "unidir" ) == 0 )
      {
         *pdirType = TAS_SEGMENT_DIR_UNIDIRECTIONAL;
      }
      else if( TC_stricmp( srDirType.data( ), "bidir" ) == 0 )
      {
         *pdirType = TAS_SEGMENT_DIR_BIDIRECTIONAL;
      }
      else
      {
         *pdirType = TAS_SEGMENT_DIR_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid type, expected \"unidir\" or \"bidir\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
switchBoxType[ TAS_SwitchBoxType_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      if( TC_stricmp( srType.data( ), "buffered" ) == 0 )
      {
         *ptype = TAS_SWITCH_BOX_BUFFERED;
      }
      else if( TC_stricmp( srType.data( ), "mux" ) == 0 )
      {
         *ptype = TAS_SWITCH_BOX_MUX;
      }
      else
      {
         *ptype = TAS_SWITCH_BOX_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid type, expected \"buffered\" or \"mux\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
switchBoxModelType[ TAS_SwitchBoxModelType_t* pmodelType ]
   :
   <<
      string srModelType;
   >>
   stringText[ &srModelType ]
   <<
      if( TC_stricmp( srModelType.data( ), "wilton" ) == 0 )
      {
         *pmodelType = TAS_SWITCH_BOX_MODEL_WILTON;
      }
      else if( TC_stricmp( srModelType.data( ), "subset" ) == 0 )
      {
         *pmodelType = TAS_SWITCH_BOX_MODEL_WILTON;
      }
      else if ( TC_stricmp( srModelType.data( ), "universal" ) == 0 )
      {
         *pmodelType = TAS_SWITCH_BOX_MODEL_WILTON;
      }
      else
      {
         *pmodelType = TAS_SWITCH_BOX_MODEL_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid type, expected \"wilton\", \"subset\", or \"universal\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
channelWidth[ TAS_ChannelWidth_c* pchannelWidthIO,
              TAS_ChannelWidth_c* pchannelWidthX,
              TAS_ChannelWidth_c* pchannelWidthY ]
   :
   "<"
   (  IO 
      <<
         pchannelWidthIO->usageMode = TAS_CHANNEL_USAGE_IO;
      >>
      WIDTH EQUAL floatNum[ &pchannelWidthIO->width ]
   |  X 
      <<
         pchannelWidthX->usageMode = TAS_CHANNEL_USAGE_X;
      >>
      DISTR EQUAL channelDistrMode[ &pchannelWidthX->distrMode ]
      (  PEAK EQUAL floatNum[ &pchannelWidthX->peak ]
      |  XPEAK EQUAL floatNum[ &pchannelWidthX->xpeak ]
      |  DC EQUAL floatNum[ &pchannelWidthX->dc ]
      |  WIDTH EQUAL floatNum[ &pchannelWidthX->width ]
      )*
   |  Y 
      <<
         pchannelWidthY->usageMode = TAS_CHANNEL_USAGE_Y;
      >>
      DISTR EQUAL channelDistrMode[ &pchannelWidthY->distrMode ]
      (  PEAK EQUAL floatNum[ &pchannelWidthY->peak ]
      |  XPEAK EQUAL floatNum[ &pchannelWidthY->xpeak ]
      |  DC EQUAL floatNum[ &pchannelWidthY->dc ]
      |  WIDTH EQUAL floatNum[ &pchannelWidthY->width ]
      )*
   )
   "/>"
   ;

//===========================================================================//
channelDistrMode[ TAS_ChannelDistrMode_t* pmode ]
   :
   <<
      string srMode;
   >>
   stringText[ &srMode ]
   <<
      if( TC_stricmp( srMode.data( ), "uniform" ) == 0 )
      {
         *pmode = TAS_CHANNEL_DISTR_UNIFORM;
      }
      else if( TC_stricmp( srMode.data( ), "gaussian" ) == 0 )
      {
         *pmode = TAS_CHANNEL_DISTR_GAUSSIAN;
      }
      else if( TC_stricmp( srMode.data( ), "pulse" ) == 0 )
      {
         *pmode = TAS_CHANNEL_DISTR_PULSE;
      }
      else if( TC_stricmp( srMode.data( ), "delta" ) == 0 )
      {
         *pmode = TAS_CHANNEL_DISTR_DELTA;
      }
      else
      {
         *pmode = TAS_CHANNEL_DISTR_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid distr, expected \"uniform\", \"gaussian\", \"pulse\", or \"delta\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
connectionBoxInType[ TAS_ConnectionBoxType_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      if( TC_stricmp( srType.data( ), "frac" ) == 0 )
      {
         *ptype = TAS_CONNECTION_BOX_FRACTION;
      }
      else if( TC_stricmp( srType.data( ), "abs" ) == 0 )
      {
         *ptype = TAS_CONNECTION_BOX_ABSOLUTE;
      }
      else
      {
         *ptype = TAS_CONNECTION_BOX_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid type, expected \"frac\" or \"abs\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
connectionBoxOutType[ TAS_ConnectionBoxType_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      if( TC_stricmp( srType.data( ), "frac" ) == 0 )
      {
         *ptype = TAS_CONNECTION_BOX_FRACTION;
      }
      else if( TC_stricmp( srType.data( ), "abs" ) == 0 )
      {
         *ptype = TAS_CONNECTION_BOX_ABSOLUTE;
      }
      else if( TC_stricmp( srType.data( ), "full" ) == 0 )
      {
         *ptype = TAS_CONNECTION_BOX_FULL;
      }
      else
      {
         *ptype = TAS_CONNECTION_BOX_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid type, expected \"frac\", \"abs\", or \"full\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
pinAssignPatternType[ TAS_PinAssignPatternType_t* ppatternType ]
   :
   <<
      string srPatternType;
   >>
   stringText[ &srPatternType ]
   <<
      if( TC_stricmp( srPatternType.data( ), "spread" ) == 0 )
      {
         *ppatternType = TAS_PIN_ASSIGN_PATTERN_SPREAD;
      }
      else if( TC_stricmp( srPatternType.data( ), "custom" ) == 0 )
      {
         *ppatternType = TAS_PIN_ASSIGN_PATTERN_CUSTOM;
      }
      else
      {
         *ppatternType = TAS_PIN_ASSIGN_PATTERN_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid pattern, expected \"spread\" or \"custom\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
gridAssignDistrMode[ TAS_GridAssignDistrMode_t* pdistrMode ]
   :
   <<
      string srDistrMode;
   >>
   stringText[ &srDistrMode ]
   <<
      if( TC_stricmp( srDistrMode.data( ), "col" ) == 0 )
      {
         *pdistrMode = TAS_GRID_ASSIGN_DISTR_MULTIPLE;
      }
      else if( TC_stricmp( srDistrMode.data( ), "rel" ) == 0 )
      {
         *pdistrMode = TAS_GRID_ASSIGN_DISTR_SINGLE;
      }
      else if( TC_stricmp( srDistrMode.data( ), "fill" ) == 0 )
      {
         *pdistrMode = TAS_GRID_ASSIGN_DISTR_FILL;
      }
      else if( TC_stricmp( srDistrMode.data( ), "perimeter" ) == 0 )
      {
         *pdistrMode = TAS_GRID_ASSIGN_DISTR_PERIMETER;
      }
      else
      {
         *pdistrMode = TAS_GRID_ASSIGN_DISTR_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid mode, expected \"col\", \"rel\", \"fill\", or \"perimeter\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
sideMode[ TC_SideMode_t* pmode ]
   :
   <<
      string srMode;
   >>
   stringText[ &srMode ]
   <<
      if( TC_stricmp( srMode.data( ), "left" ) == 0 )
      {
         *pmode = TC_SIDE_LEFT;
      }
      else if( TC_stricmp( srMode.data( ), "right" ) == 0 )
      {
         *pmode = TC_SIDE_RIGHT;
      }
      else if(( TC_stricmp( srMode.data( ), "bottom" ) == 0 ) ||
              ( TC_stricmp( srMode.data( ), "lower" ) == 0 ))
      {
         *pmode = TC_SIDE_LOWER;
      }
      else if(( TC_stricmp( srMode.data( ), "top" ) == 0 ) ||
              ( TC_stricmp( srMode.data( ), "upper" ) == 0 ))
      {
         *pmode = TC_SIDE_UPPER;
      }
      else
      {
         *pmode = TC_SIDE_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid side, expected \"left\", \"right\", \"bottom\", or \"top\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
classText[ TAS_ClassType_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      *ptype = TAS_CLASS_UNDEFINED;
      if( TC_stricmp( srType.data( ), "lut" ) == 0 )
      {
         *ptype = TAS_CLASS_LUT;
      }
      else if( TC_stricmp( srType.data( ), "flipflop" ) == 0 )
      {
         *ptype = TAS_CLASS_FLIPFLOP;
      }
      else if( TC_stricmp( srType.data( ), "memory" ) == 0 )
      {
         *ptype = TAS_CLASS_MEMORY;
      }
      else
      {
         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid class, expected \"lut\", \"flipflop\", or \"memory\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
blifModelText[ string* psrString, 
               TAS_PhysicalBlockModelType_t* ptype ]
   :
   stringText[ psrString ]
   <<
      *ptype = TAS_PHYSICAL_BLOCK_MODEL_UNDEFINED;
      if(( TC_stricmp( psrString->data( ), ".names" ) == 0 ) ||
         ( TC_stricmp( psrString->data( ), ".latch" ) == 0 ) ||
         ( TC_stricmp( psrString->data( ), ".input" ) == 0 ) ||
         ( TC_stricmp( psrString->data( ), ".output" ) == 0 ))
      {
         *ptype = TAS_PHYSICAL_BLOCK_MODEL_STANDARD;
      }
      else if( TC_strnicmp( psrString->data( ), ".subckt", 7 ) == 0 )
      {
         *ptype = TAS_PHYSICAL_BLOCK_MODEL_CUSTOM;
      }

      if(( TC_stricmp( psrString->data( ), ".names" ) != 0 ) &&
         ( TC_stricmp( psrString->data( ), ".latch" ) != 0 ) &&
         ( TC_stricmp( psrString->data( ), ".input" ) != 0 ) &&
         ( TC_stricmp( psrString->data( ), ".output" ) != 0 ) &&
         ( TC_strnicmp( psrString->data( ), ".subckt", 7 ) != 0 ))
      {
         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid blif_model, expected \".names\", \".latch\", \".input\", \".output\", or \".subckt\"" );
         this->consumeUntilToken( END_OF_FILE );
      }

      if( TC_strnicmp( psrString->data( ), ".subckt", 7 ) == 0 )
      {
         *psrString = psrString->substr( 8 );
      }
   >>
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
      OPEN_QUOTE
      (  qfloatVal:STRING
         <<
            *pdouble = atof( qfloatVal->getText( ));
         >>
      |  qposIntVal:POS_INT
         <<
            *pdouble = atof( qposIntVal->getText( ));
         >>
      |  qnegIntVal:NEG_INT
         <<
            *pdouble = atof( qnegIntVal->getText( ));
         >>
      )
      CLOSE_QUOTE
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
   ;

//===========================================================================//
expNum[ double* pdouble ]
   : 
      OPEN_QUOTE
      (  qexpVal:EXP
         <<
            *pdouble = atof( qexpVal->getText( ));
         >>
      |  qfloatVal:STRING
         <<
            *pdouble = atof( qfloatVal->getText( ));
         >>
      |  qposIntVal:POS_INT
         <<
            *pdouble = atof( qposIntVal->getText( ));
         >>
      |  qnegIntVal:NEG_INT
         <<
            *pdouble = atof( qnegIntVal->getText( ));
         >>
      )
      CLOSE_QUOTE
   |  expVal:EXP
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
   ;

//===========================================================================//
intNum[ int* pint ]
   :
      OPEN_QUOTE
      qintVal:STRING
      <<
         *pint = static_cast< int >( atol( qintVal->getText( )));
      >>
      CLOSE_QUOTE
   |  intVal:POS_INT
      <<
         *pint = static_cast< int >( atol( intVal->getText( )));
      >>
   ;

//===========================================================================//
uintNum[ unsigned int* puint ]
   :
      OPEN_QUOTE
      quintVal:STRING
      <<
         *puint = static_cast< unsigned int >( atol( quintVal->getText( )));
      >>
      CLOSE_QUOTE
   |  uintVal:POS_INT
      <<
         *puint = static_cast< unsigned int >( atol( uintVal->getText( )));
      >>
   ;

//===========================================================================//
boolType[ bool* pbool ]
   :
   (  <<
         const char* pszBool;
      >>
      OPEN_QUOTE
      qboolVal:STRING
      <<
         pszBool = qboolVal->getText( );
         if( TC_stricmp( pszBool, "true" ) == 0 )
         {
            *pbool = true;
         }
         else if( TC_stricmp( pszBool, "false" ) == 0 )
         {
            *pbool = false;
         }
         else
         {
            this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                            this->srFileName_,
                                            ": Invalid boolean, expected \"true\" or \"false\"" );
            this->consumeUntilToken( END_OF_FILE );
         }
      >>
      CLOSE_QUOTE
   |  BOOL_TRUE
      <<
         *pbool = true;
      >>
   |  BOOL_FALSE
      <<
         *pbool = false;
      >>
   )
   ;

//===========================================================================//

}

<<
#include "TAXP_ArchitectureXmlGrammar.h"
>>
