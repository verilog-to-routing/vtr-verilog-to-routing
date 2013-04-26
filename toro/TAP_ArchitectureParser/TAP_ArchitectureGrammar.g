//===========================================================================//
// Purpose: PCCTS grammar for the architecture spec file.
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

//===========================================================================//
#header

<<
#include <cstdio>
using namespace std;

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TC_StringUtils.h"

#include "TGS_Typedefs.h"

#include "TIO_PrintHandler.h"

#include "TAS_ArchitectureSpec.h"

#include "TAP_ArchitectureFile.h"
#include "TAP_ArchitectureScanner_c.h"
>>

//===========================================================================//
#lexclass QUOTED_VALUE

#token CLOSE_QUOTE     "\""                    << mode( START ); >>
#token STRING          "~[\"\n]+"
#token UNCLOSED_STRING "[\n]"

//===========================================================================//
#lexclass START

#token                  "[\ \t]+"           << skip( ); >>
#token CPP_COMMENT      "//~[\n]*[\n]"      << skip( ); newline( ); >>
#token BLOCK_COMMENT    "#~[\n]*[\n]"       << skip( ); newline( ); >>
#token XML_COMMENT      "<\!\-\-~[\n]*[\n]" << skip( ); newline( ); >>
#token NEW_LINE         "[\n\\]"            << skip( ); newline( ); >>
#token END_OF_FILE      "@"
#token OPEN_QUOTE       "\""                << mode( QUOTED_VALUE ); >>
#token EQUAL            "="

#token ARCHITECTURE     "[Aa][Rr][Cc][Hh][Ii][Tt][Ee][Cc][Tt][Uu][Rr][Ee]"

#token CONFIG           "[Cc][Oo][Nn][Ff][Ii][Gg]"
#token IO               "[Ii]{[Nn][Pp][Uu][Tt]}[Oo]{[Uu][Tt][Pp][Uu][Tt]}"
#token PB               "[Pp]{[Hh][Yy][Ss][Ii][Cc][Aa][Ll]}[Bb]{[Ll][Oo][Cc][Kk]}"
#token SB               "[Ss]{[Ww][Ii][Tt][Cc][Hh]}[Bb]{[Oo][Xx]{[Ee][Ss]}}"
#token CB               "[Cc]{[Oo][Nn][Nn][Ee][Cc][Tt][Ii][Oo][Nn]}[Bb]{[Oo][Xx]{[Ee][Ss]}}"
#token SEGMENT          "[Ss][Ee][Gg][Mm][Ee][Nn][Tt]{[Ss]}"
#token MODEL            "[Mm][Oo][Dd][Ee]{[Ll]}{[Ss]}"
#token CELL             "[Cc][Ee][Ll][Ll]{[Ss]}"
#token PIN              "[Pp][Ii][Nn]{[Ss]}"

#token NAME             "[Nn][Aa][Mm][Ee]"
#token TYPE             "[Tt][Yy][Pp][Ee]"
#token CLASS            "[Cc][Ll][Aa][Ss][Ss]"
#token FS               "[Ff][Ss]"

#token INTERCONNECT     "[Ii][Nn][Tt][Ee][Rr][Cc][Oo][Nn][Nn][Ee][Cc][Tt]"

#token FC_IN            "[Ff][Cc][_][Ii][Nn]"
#token FC_OUT           "[Ff][Cc][_][Oo][Uu][Tt]"

#token WIDTH            "[Ww][Ii][Dd][Tt][Hh]"
#token HEIGHT           "[Hh][Ee][Ii][Gg][Hh][Tt]"
#token LENGTH           "[Ll][Ee][Nn][Gg][Tt][Hh]"
#token CAPACITY         "[Cc][Aa][Pp][Aa][Cc][Ii][Tt][Yy]"

#token SIZE             "{[Aa][Rr][Rr][Aa][Yy][_]}[Ss][Ii][Zz][Ee]"
#token RATIO            "{[Aa][Ss][Pp][Ee][Cc][Tt][_]}[Rr][Aa][Tt][Ii][Oo]"
#token ORIGIN           "[Oo][Rr][Ii][Gg][Ii][Nn]"
#token COUNT            "[Cc][Oo][Uu][Nn][Tt]"
#token SIDE             "[Ss][Ii][Dd][Ee]"
#token OFFSET           "[Oo][Ff][Ff][Ss][Ee][Tt]"
#token EQUIVALENCE      "[Ee][Qq][Uu][Ii][Vv][Aa][Ll][Ee][Nn][Cc][Ee]"

#token FULL             "[Ff][Uu][Ll][Ll]"
#token LONGLINE         "[Ll][Oo][Nn][Gg][Ll][Ii][Nn][Ee]"

#token PIN_ASSIGN       "[Pp][Ii][Nn][_][Aa][Ss][Ss][Ii][Gg][Nn]{[Mm][Ee][Nn][Tt]}"
#token GRID_ASSIGN      "[Gg][Rr][Ii][Dd][_][Aa][Ss][Ss][Ii][Gg][Nn]{[Mm][Ee][Nn][Tt]}"

#token MAPPING          "[Mm][Aa][Pp][Pp][Ii][Nn][Gg]"

#token TIMING           "[Tt][Ii][Mm][Ii][Nn][Gg]{[_][Aa][Nn][Aa][Ll][Yy][Ss][Ii][Ss]}"
#token R                "[Rr]"
#token RES              "[Rr][Ee][Ss]"
#token CAP              "[Cc]{[Aa][Pp]}"
#token CAP_IN           "[Cc]{[Aa][Pp]}{[_]}[Ii][Nn]"
#token CAP_OUT          "[Cc]{[Aa][Pp]}{[_]}[Oo][Uu][Tt]"
#token T                "[Tt]"
#token DELAY            "[Dd][Ee][Ll][Aa][Yy]{[_][Ii][Nn]}"

#token EST              "[Ee][Ss][Tt]{[Ii][Mm][Aa][Tt][Ee][Dd]}"
#token MINW_NMOS_R      "[Mm][Ii][Nn][_][Ww][Ii][Dd][Tt][Hh][_][Nn][Mm][Oo][Ss][_][Rr][Ee][Ss]"
#token MINW_PMOS_R      "[Mm][Ii][Nn][_][Ww][Ii][Dd][Tt][Hh][_][Pp][Mm][Oo][Ss][_][Rr][Ee][Ss]"
#token MUX_IN_PIN_SIZE  "[Mm][Uu][Xx][_][Tt][Rr][Aa][Nn][Ss][_][Ii][Nn][_][Pp][Ii][Nn][_][Ss][Ii][Zz][Ee]"
#token LOGIC_TILE_AREA  "[Gg][Rr][Ii][Dd][_][Ll][Oo][Gg][Ii][Cc][_][Tt][Ii][Ll][Ee][_][Aa][Rr][Ee][Aa]"

#token WIRE_SWITCH      "[Ww][Ii][Rr][Ee][_][Ss][Ww][Ii][Tt][Cc][Hh]"
#token OPIN_SWITCH      "[Oo][Pp][Ii][Nn][_][Ss][Ww][Ii][Tt][Cc][Hh]"

#token FREQ             "[Ff][Rr][Ee][Qq]"
#token MUX              "[Mm][Uu][Xx]"
#token PATTERN          "[Pp][Aa][Tt][Tt][Ee][Rr][Nn]"

#token INPUT_PORTS      "[Ii][Nn][Pp][Uu][Tt][_][Pp][Oo][Rr][Tt][Ss]"
#token OUTPUT_PORTS     "[Oo][Uu][Tt][Pp][Uu][Tt][_][Pp][Oo][Rr][Tt][Ss]"
#token IS_CLOCK         "[Ii][Ss][_][Cc][Ll][Oo][Cc][Kk]"

#token INPUT            "[Ii][Nn][Pp][Uu][Tt]"
#token OUTPUT           "[Oo][Uu][Tt][Pp][Uu][Tt]"
#token CLOCK            "[Cc][Ll][Oo][Cc][Kk]"

#token PRIORITY         "[Pp][Rr][Ii][Oo][Rr][Ii][Tt][Yy]"
#token SINGLE_POS       "[Pp][Oo][Ss]"
#token MULTIPLE_START   "[Ss][Tt][Aa][Rr][Tt]"
#token MULTIPLE_REPEAT  "[Rr][Ee][Pp][Ee][Aa][Tt]"

#token VALUE            "[Vv][Aa][Ll][Uu][Ee]"
#token MIN_VALUE        "[Mm][Ii][Nn]{[_][Vv][Aa][Ll][Uu][Ee]}"
#token MAX_VALUE        "[Mm][Aa][Xx]{[_][Vv][Aa][Ll][Uu][Ee]}"

#token MATRIX           "[Mm][Aa][Tt][Rr][Ii][Xx]"
#token MIN_MATRIX       "[Mm][Ii][Nn][_][Mm][Aa][Tt][Rr][Ii][Xx]"
#token MAX_MATRIX       "[Mm][Aa][Xx][_][Mm][Aa][Tt][Rr][Ii][Xx]"

#token BOOL_TRUE    "([Tt][Rr][Uu][Ee]|[Yy][Ee][Ss]|[Oo][Nn])"
#token BOOL_FALSE   "([Ff][Aa][Ll][Ss][Ee]|[Nn][Oo]|[Oo][Ff][Ff])"

#token BIT_CHAR     "[01]"
#token NEG_INT      "[\-][0-9]+"
#token POS_INT      "[0-9]+"
#token FLOAT        "{\-}{[0-9]+}.[0-9]+"
#token EXP          "{\-}{[0-9]+.}[0-9]+[Ee][\+\-][0-9]+"
#token STRING       "[a-zA-Z_/\|][a-zA-Z0-9_/\|\(\)\[\]\.\+\-\~]*"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//

class TAP_ArchitectureParser_c
{
<<
public:

   void syn( ANTLRAbstractToken* /* pToken */, 
             ANTLRChar*          pszGroup,
             SetWordType*        /* pWordType */,
             ANTLRTokenType      tokenType,
             int                 /* k */ );

   void SetInterface( TAP_ArchitectureInterface_c* pinterface );
   void SetScanner( TAP_ArchitectureScanner_c* pscanner );
   void SetFileName( const char* pszFileName );
   void SetArchitectureFile( TAP_ArchitectureFile_c* parchitectureFile );
   void SetArchitectureSpec( TAS_ArchitectureSpec_c* parchitectureSpec );
>> 
<<
private:

   TAP_ArchitectureInterface_c* pinterface_;
   TAP_ArchitectureScanner_c*   pscanner_;
   string                       srFileName_;
   TAP_ArchitectureFile_c*      parchitectureFile_;
   TAS_ArchitectureSpec_c*      parchitectureSpec_;
>>

//===========================================================================//
start 
   : 
   "<" ARCHITECTURE { NAME { EQUAL } } stringText[ &this->parchitectureSpec_->srName ] ">"
   (  "<"
      (  configDef[ &this->parchitectureSpec_->config ]
      |  inputOutputList[ &this->parchitectureSpec_->physicalBlockList ]
      |  physicalBlockList[ &this->parchitectureSpec_->physicalBlockList ]
      |  modeList[ &this->parchitectureSpec_->modeList ]
      |  switchBoxList[ &this->parchitectureSpec_->switchBoxList ]
      |  segmentList[ &this->parchitectureSpec_->segmentList ]
      |  cellList[ &this->parchitectureSpec_->cellList ]
      )
   )* 
   "</" ARCHITECTURE ">"
   END_OF_FILE
   ;

//===========================================================================//
configDef[ TAS_Config_c* pconfig ]
   :
   CONFIG ">"
   (  "<"
      (  SIZE MODEL { EQUAL } arraySizeMode[ &pconfig->layout.sizeMode ]
         (  RATIO { EQUAL } floatNum[ &pconfig->layout.autoSize.aspectRatio ]
         |  WIDTH { EQUAL } intNum[ &pconfig->layout.manualSize.gridDims.dx ]
         |  HEIGHT { EQUAL } intNum[ &pconfig->layout.manualSize.gridDims.dy ]
         )*
      |  EST
         (  MINW_NMOS_R { EQUAL } floatNum[ &pconfig->device.areaModel.resMinWidthNMOS ]
         |  MINW_PMOS_R { EQUAL } floatNum[ &pconfig->device.areaModel.resMinWidthPMOS ]
         |  MUX_IN_PIN_SIZE { EQUAL } floatNum[ &pconfig->device.areaModel.sizeInputPinMux ]
         |  LOGIC_TILE_AREA { EQUAL } floatNum[ &pconfig->device.areaModel.areaGridTile ]
         )*
      |  SB 
         (  MODEL { EQUAL } switchBoxModelType[ &pconfig->device.switchBoxes.modelType ]
         |  FS { EQUAL } uintNum[ &pconfig->device.switchBoxes.fs ] 
         )*
      |  CB
         (  ( CAP | CAP_IN ) { EQUAL } expNum[ &pconfig->device.connectionBoxes.capInput ]
         |  ( T | DELAY ) { EQUAL } expNum[ &pconfig->device.connectionBoxes.delayInput ]
         )*
      |  SEGMENT 
         (  TYPE { EQUAL } segmentDirType[ &pconfig->device.segments.dirType ]
         )*
      )
      "/>"
   )*
   "</" CONFIG ">"
   ;

//===========================================================================//
inputOutputList[ TAS_InputOutputList_t* pinputOutputList ]
   : 
   << 
      TAS_InputOutput_t inputOutput;

      TGS_FloatDims_t dims;
      TGS_Point_c origin;
   >>
   IO 
   { NAME { EQUAL } } stringText[ &inputOutput.srName ]
   (  CAPACITY { EQUAL } uintNum[ &inputOutput.capacity ]
   |  FC_IN { EQUAL } fcDef[ &inputOutput.fcIn ]
   |  FC_OUT { EQUAL } fcDef[ &inputOutput.fcOut ]
   |  SIZE { EQUAL } floatDims[ &dims ]
   |  ORIGIN { EQUAL } originPoint[ &origin ]
   )*
   ">"
   (  "<"
      (  MODEL ">" modeNameList[ &inputOutput.modeNameList ] "</" MODEL ">"
      |  PIN pinList[ &inputOutput.portList ]
         ( "/>" | "</" PIN ">" )
      |  PIN_ASSIGN pinAssignList[ &inputOutput.pinAssignPattern,
                                   &inputOutput.pinAssignList ] 
         ( "/>" | "</" PIN_ASSIGN ">" )
      |  GRID_ASSIGN gridAssignList[ &inputOutput.gridAssignList ] "/>"
      |  TIMING timingDelayLists[ &inputOutput.timingDelayLists ] "/>"
      )      
   )*
   "</" IO ">"
   <<
      if( inputOutput.IsValid( ))
      {
         inputOutput.SetUsage( TAS_USAGE_INPUT_OUTPUT );
         inputOutput.SetDims( dims );
         inputOutput.SetOrigin( origin );

         pinputOutputList->Add( inputOutput );
      }
   >>
   ;

//===========================================================================//
physicalBlockList[ TAS_PhysicalBlockList_t* pphysicalBlockList ]
   : 
   <<
      TAS_PhysicalBlock_c physicalBlock;

      TGS_FloatDims_t dims;
      TGS_Point_c origin;
   >>
   PB 
   { NAME { EQUAL } } stringText[ &physicalBlock.srName ]
   (  COUNT { EQUAL } uintNum[ &physicalBlock.numPB ]
   |  CELL { EQUAL } cellModelText[ &physicalBlock.srModelName,
                                    &physicalBlock.modelType ]
   |  CLASS { EQUAL } classType[ &physicalBlock.classType ]
   |  FC_IN { EQUAL } fcDef[ &physicalBlock.fcIn ]
   |  FC_OUT { EQUAL } fcDef[ &physicalBlock.fcOut ]
   |  WIDTH uintNum[ &physicalBlock.width ]
   |  HEIGHT uintNum[ &physicalBlock.height ]
   |  SIZE { EQUAL } floatDims[ &dims ]
   |  ORIGIN { EQUAL } originPoint[ &origin ]
   )*
   ">"
   (  "<"
      (  MODEL ">" modeNameList[ &physicalBlock.modeNameList ] "</" MODEL ">"
      |  PIN pinList[ &physicalBlock.portList ] 
         ( "/>" | "</" PIN ">" )
      |  PIN_ASSIGN pinAssignList[ &physicalBlock.pinAssignPattern,
                                   &physicalBlock.pinAssignList ] 
         ( "/>" | "</" PIN_ASSIGN ">" )
      |  GRID_ASSIGN gridAssignList[ &physicalBlock.gridAssignList ] "/>"
      |  TIMING timingDelayLists[ &physicalBlock.timingDelayLists ] "/>"
      )
   )*
   "</" PB ">"
   <<
      if( physicalBlock.IsValid( ))
      {
         physicalBlock.SetUsage( TAS_USAGE_PHYSICAL_BLOCK );
         physicalBlock.SetDims( dims );
         physicalBlock.SetOrigin( origin );

         pphysicalBlockList->Add( physicalBlock );
      }
   >>
   ;

//===========================================================================//
modeList[ TAS_ModeList_t* pmodeList ]
   :
   <<
      TAS_Mode_c mode;

      TAS_PhysicalBlock_c physicalBlock;
      TAS_Interconnect_c interconnect;

      string srInputName, srOutputName;
   >>
   MODEL
   { NAME { EQUAL } } stringText[ &mode.srName ] 
   ">"
   (  "<"
      (  PB
         { NAME { EQUAL } } stringText[ &physicalBlock.srName ]
         "/>"
         <<
            physicalBlock.SetUsage( TAS_USAGE_PHYSICAL_BLOCK );
            mode.physicalBlockList.Add( physicalBlock );
         >>
      |  INTERCONNECT 
         << 
            interconnect.inputNameList.Clear( );
            interconnect.outputNameList.Clear( );

            interconnect.timingDelayLists.delayList.Clear( );
            interconnect.timingDelayLists.delayMatrixList.Clear( );
            interconnect.timingDelayLists.tSetupList.Clear( );
            interconnect.timingDelayLists.tHoldList.Clear( );
            interconnect.timingDelayLists.clockToQList.Clear( );
            interconnect.timingDelayLists.capList.Clear( );
            interconnect.timingDelayLists.capMatrixList.Clear( );
            interconnect.timingDelayLists.packPatternList.Clear( );
         >>
         { NAME { EQUAL } } stringText[ &interconnect.srName ]
         TYPE { EQUAL } interconnectMapType[ &interconnect.mapType ]
         ">"
         (  "<"
            (  INPUT NAME { EQUAL } stringText[ &srInputName ]
               <<
                  interconnect.inputNameList.Add( srInputName );
               >>
            |  OUTPUT NAME { EQUAL } stringText[ &srOutputName ]
               <<
                  interconnect.outputNameList.Add( srOutputName );
               >>
            |  TIMING timingDelayLists[ &interconnect.timingDelayLists ]
            )
            "/>"
         )*
         "</" INTERCONNECT ">" 
         <<
            mode.interconnectList.Add( interconnect );
         >>
      )
   )*
   "</" MODEL ">"
   <<
      if( mode.IsValid( ))
      {
         pmodeList->Add( mode );
      }
   >>
   ;

//===========================================================================//
switchBoxList[ TAS_SwitchBoxList_t* pswitchBoxList ]
   : 
   << 
      TAS_SwitchBox_c switchBox;

      TGS_FloatDims_t dims;
      TGS_Point_c origin;
      TAS_MapTable_t mapTable;
   >>
   SB 
   { NAME { EQUAL } } stringText[ &switchBox.srName ]
   (  TYPE { EQUAL } switchBoxType[ &switchBox.type ]
   |  MODEL { EQUAL } switchBoxModelType[ &switchBox.model ]
   |  FS { EQUAL } uintNum[ &switchBox.fs ]
   |  SIZE { EQUAL } floatDims[ &dims ]
   |  ORIGIN { EQUAL } originPoint[ &origin ]
   )*
   ">"
   (  "<" 
      (  MAPPING mapSideTable[ switchBox.fs, &mapTable ]
      |  TIMING
         (  ( R | RES ) { EQUAL } floatNum[ &switchBox.timing.res ]
         |  ( CAP | CAP_IN ) { EQUAL } expNum[ &switchBox.timing.capInput ]
         |  CAP_OUT { EQUAL } expNum[ &switchBox.timing.capOutput ]
         |  ( T | DELAY ) { EQUAL } expNum[ &switchBox.timing.delay ]
         )* 
      )
      "/>"
   )*
   "</" SB ">"
   <<
      if( switchBox.IsValid( ))
      {
         switchBox.SetDims( dims );
         switchBox.SetOrigin( origin );
         switchBox.SetMapTable( mapTable );

         pswitchBoxList->Add( switchBox );
      }
   >>
   ;

//===========================================================================//
segmentList[ TAS_SegmentList_t* psegmentList ]
   : 
   << 
      TAS_Segment_c segment;
   >>
   SEGMENT 
   (  LENGTH { EQUAL } segmentLength[ &segment.length ]
   |  TYPE { EQUAL } segmentDirType[ &segment.dirType ]
   |  FREQ { EQUAL } floatNum[ &segment.trackFreq ]
   )*
   ">"
   (  "<" 
      (  TIMING
         (  ( R | RES ) { EQUAL } floatNum[ &segment.timing.res ]
         |  CAP { EQUAL } expNum[ &segment.timing.cap ]
         )*
         "/>"
      | sbList[ &segment.sbPattern ]
      | cbList[ &segment.cbPattern ]
      | MUX NAME { EQUAL } stringText[ &segment.srMuxSwitchName ] "/>"
      | WIRE_SWITCH NAME { EQUAL } stringText[ &segment.srWireSwitchName ] "/>"
      | OPIN_SWITCH NAME { EQUAL } stringText[ &segment.srOutputSwitchName ] "/>"
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
cellList[ TAS_CellList_t* pcellList ]
   : 
   << 
      TAS_Cell_c cell;
      TLO_Port_c port;

      string srName;
      TC_TypeMode_t type = TC_TYPE_UNDEFINED;
   >>
   CELL 
   { NAME { EQUAL } } stringText[ &srName ] 
   <<
      cell.SetName( srName );
   >>
   (  CLASS { EQUAL } classType[ &cell.classType ]
   )*
   ">"
   (  "<" 
      (  PIN
         { NAME { EQUAL } } stringText[ &srName ]
         <<
            port.Clear( );
            port.SetName( srName );
         >>
         TYPE { EQUAL } typeMode[ &type ]
         << 
            port.SetType( type );
         >>
         "/>"
         <<
            cell.AddPort( port );
         >>
      )
   )*
   "</" CELL ">"
   <<
      if( cell.IsValid( ))
      {
         pcellList->Add( cell );
      }
   >>
   ;

//===========================================================================//
fcDef[ TAS_ConnectionFc_c* pfc ]
   :
   <<
      unsigned int uintValue = 0;
      double floatValue = 0.0;
   >>
   (  FULL
      <<
         pfc->type = TAS_CONNECTION_BOX_FULL;
         pfc->percent = 0.0;
         pfc->absolute = 0;
      >>
   |  OPEN_QUOTE
      (  qfloatVal:STRING
         <<
            pfc->type = TAS_CONNECTION_BOX_FRACTION;
            pfc->percent = atof( qfloatVal->getText( ));
         >>
      |  quintVal:POS_INT
         <<
            pfc->type = TAS_CONNECTION_BOX_ABSOLUTE;
            pfc->absolute = static_cast< unsigned int >( atol( quintVal->getText( )));
         >>
      |  qbitVal:BIT_CHAR
         <<
            pfc->type = TAS_CONNECTION_BOX_ABSOLUTE;
            pfc->absolute = static_cast< unsigned int >( atol( qbitVal->getText( )));
         >>
      )
      CLOSE_QUOTE
   |  floatVal:FLOAT
      <<
         pfc->type = TAS_CONNECTION_BOX_FRACTION;
         pfc->percent = atof( floatVal->getText( ));
      >>
   |  uintVal:POS_INT
      <<
         pfc->type = TAS_CONNECTION_BOX_ABSOLUTE;
         pfc->absolute = static_cast< unsigned int >( atol( uintVal->getText( )));
      >>
   |  bitVal:BIT_CHAR
      <<
         pfc->type = TAS_CONNECTION_BOX_ABSOLUTE;
         pfc->absolute = static_cast< unsigned int >( atol( bitVal->getText( )));
      >>
   )
   { CLOSE_QUOTE }
   ;

//===========================================================================//
modeNameList[ TAS_ModeNameList_t* pmodeNameList ]

   :
   <<
       string srModeName;
   >>
   ( stringText[ &srModeName ]
      <<
          pmodeNameList->Add( srModeName );
      >>
   )*
   ;

//===========================================================================//
pinList[ TLO_PortList_t* pportList ]
   :
   <<
      TLO_Port_c port;

      string srName;
      TC_TypeMode_t type = TC_TYPE_UNDEFINED;
      unsigned int count = 0;
      bool equivalent = false;
      string srClass;
      double cap = 0.0;
      double delay = 0.0;
   >>
   { NAME { EQUAL } } stringText[ &srName ]
   <<
      port.SetName( srName );
   >>
   (  TYPE { EQUAL } typeMode[ &type ]
      << 
         port.SetType( type );
      >>
   |  COUNT { EQUAL } uintNum[ &count ]
      << 
         port.SetCount( count );
      >>
   |  EQUIVALENCE { EQUAL } boolType[ &equivalent ]
      << 
         port.SetEquivalent( equivalent );
      >>
   |  CLASS { EQUAL } stringText[ &srClass ]
      << 
         port.SetClass( srClass );
      >>
   )*
   {  ">" 
      "<" TIMING
      (  ( CAP | CAP_IN ) { EQUAL } expNum[ &cap ]
         << 
            port.SetCap( cap );
         >>
      |  ( T | DELAY ) { EQUAL } expNum[ &delay ]
         << 
            port.SetDelay( delay );
         >>
      )*
      "/>"
   }
   <<
      pportList->Add( port );
   >>
   ;

//===========================================================================//
timingDelayLists[ TAS_TimingDelayLists_c* ptimingDelayLists ]
   :
   <<
      TAS_TimingDelay_c timingDelay;
   >>
   MODEL { EQUAL } timingMode[ &timingDelay.mode ]
   (  INPUT { EQUAL } stringText[ &timingDelay.srInputPortName ]
   |  OUTPUT { EQUAL } stringText[ &timingDelay.srOutputPortName ]
   |  CLOCK { EQUAL } stringText[ &timingDelay.srClockPortName ]
   |  NAME { EQUAL } stringText[ &timingDelay.srPackPatternName ]
   |  VALUE { EQUAL } expNum[ &timingDelay.valueNom ]
   |  MIN_VALUE { EQUAL } expNum[ &timingDelay.valueMin ] 
      << timingDelay.type = TAS_TIMING_TYPE_MIN_VALUE; >>
   |  MAX_VALUE { EQUAL } expNum[ &timingDelay.valueMax ] 
      << timingDelay.type = TAS_TIMING_TYPE_MAX_VALUE; >>
   |  MATRIX { EQUAL } timingValueMatrixDef[ &timingDelay.valueMatrix ]
   |  MIN_MATRIX { EQUAL } timingValueMatrixDef[ &timingDelay.valueMatrix ]
      << timingDelay.type = TAS_TIMING_TYPE_MIN_MATRIX; >>
   |  MAX_MATRIX { EQUAL } timingValueMatrixDef[ &timingDelay.valueMatrix ]
      << timingDelay.type = TAS_TIMING_TYPE_MAX_MATRIX; >>
   )*
   <<
      ptimingDelayLists->delayList.Add( timingDelay );
   >>
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

         curTokenLine = ( LT( 0 ) ? LT( 0 )->getLine( ) : 0 );
         nextTokenLine = ( LT( 1 ) ? LT( 1 )->getLine( ) : 0 );
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
         size_t width = SIZE_MAX;
         size_t height = valueTable.GetLength( );
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
cellModelText[ string* psrString, 
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
      else
      {
         *ptype = TAS_PHYSICAL_BLOCK_MODEL_CUSTOM;
      }
   >>
   ;

//===========================================================================//
pinAssignList[ TAS_PinAssignPatternType_t* ppinAssignPattern,
               TAS_PinAssignList_t* ppinAssignList ]
   :
   <<
      TAS_PinAssign_c pinAssign;

      string srPortName;
   >>
   (  PATTERN { EQUAL } pinAssignPatternType[ &pinAssign.pattern ]
      <<
         *ppinAssignPattern = pinAssign.pattern;
      >>
   |  SIDE { EQUAL } sideMode[ &pinAssign.side ]
   |  OFFSET { EQUAL } uintNum[ &pinAssign.offset ]
   )*
   {  ">" 
      "<" PIN ">"
      (  stringText[ &srPortName ] 
         <<
            pinAssign.portNameList.Add( srPortName );
         >>
      )*
      "</" PIN ">"
   }
   <<
      if( pinAssign.IsValid( ))
      {
         ppinAssignList->Add( pinAssign );
      }
   >>
   ;

//===========================================================================//
gridAssignList[ TAS_GridAssignList_t* pgridAssignList ]
   :
   <<
      TAS_GridAssign_c gridAssign;
   >>
   (  MODEL { EQUAL } gridAssignDistrMode[ &gridAssign.distr ]
   |  ORIENT { EQUAL } gridAssignOrientMode[ &gridAssign.orient ]
   |  PRIORITY { EQUAL } uintNum[ &gridAssign.priority ]
   |  SINGLE_POS { EQUAL } floatNum[ &gridAssign.singlePercent ] 
   |  MULTI_START { EQUAL } uintNum[ &gridAssign.multipleStart ]
   |  MULTI_REPEAT { EQUAL } uintNum[ &gridAssign.multipleRepeat ]
   )*
   <<
      if( gridAssign.IsValid( ))
      {
         pgridAssignList->Add( gridAssign );
      }
   >>
   ;

//===========================================================================//
mapSideTable[ unsigned int Fs,
              TAS_MapTable_t* pmapTable ]
   :
   <<
      if( !pmapTable->IsValid( ))
      {
         pmapTable->SetCapacity( Fs * Fs );
      }
      TAS_MapList_t mapList( Fs );
      TAS_SideIndex_t sideIndex;

      size_t curTokenLine, nextTokenLine;
   >>
   (  mapSideIndex[ &sideIndex ]
      <<
         mapList.Add( sideIndex );

         curTokenLine = LT( 0 )->getLine( );
         nextTokenLine = LT( 1 )->getLine( );
         if( curTokenLine != nextTokenLine )
         {
            pmapTable->Add( mapList );
            mapList.Clear( );
         }
      >>
   )+
   ;

//===========================================================================//
mapSideIndex[ TC_SideIndex_c* psideIndex ]
   :
   <<
      TC_SideMode_t side = TC_SIDE_UNDEFINED;
      unsigned int index = 0;
   >>
   sideMode[ &side ]
   uintNum[ &index ]
   << 
      psideIndex->Set( side, index );
   >>
   ;

//===========================================================================//
segmentLength[ unsigned int *plength ]
   :
   (  FULL
      <<
         *plength = UINT_MAX;
      >>
   |  LONGLINE
      <<
         *plength = UINT_MAX;
      >>
   |  uintNum[ plength ]
   )
   ;

//===========================================================================//
sbList[ TAS_BitPattern_t* psbPattern ]
   :
   <<
      string srPattern;
   >>
   SB TYPE { EQUAL } stringText[ &srPattern ] ">"
   (  bitStringVal:BIT_CHAR
      <<
         string srBit = bitStringVal->getText( );
         if( srBit.length( ))
         {
            TC_Bit_c bit( srBit[ 0 ] );
            psbPattern->Add( bit );
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
   CB TYPE { EQUAL } stringText[ &srPattern ] ">"
   (  bitStringVal:BIT_CHAR
      <<
         string srBit = bitStringVal->getText( );
         if( srBit.length( ))
         {
            TC_Bit_c bit( srBit[ 0 ] );
            pcbPattern->Add( bit );
         }
      >>
   )*
   "</" CB ">"
   ;

//===========================================================================//
floatDims[ TGS_FloatDims_t* pfloatDims ]
   : 
   floatNum[ &pfloatDims->dx ]
   floatNum[ &pfloatDims->dy ]
   ;

//===========================================================================//
originPoint[ TGS_Point_c* poriginPoint ]
   : 
   floatNum[ &poriginPoint->x ]
   floatNum[ &poriginPoint->y ]
   ;

//===========================================================================//
typeMode[ TC_TypeMode_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      if( TC_stricmp( srType.data( ), "input" ) == 0 )
      {
         *ptype = TC_TYPE_INPUT;
      }
      else if( TC_stricmp( srType.data( ), "output" ) == 0 )
      {
         *ptype = TC_TYPE_OUTPUT;
      }
      else if( TC_stricmp( srType.data( ), "signal" ) == 0 )
      {
         *ptype = TC_TYPE_SIGNAL;
      }
      else if( TC_stricmp( srType.data( ), "clock" ) == 0 )
      {
         *ptype = TC_TYPE_CLOCK;
      }
      else if( TC_stricmp( srType.data( ), "power" ) == 0 )
      {
         *ptype = TC_TYPE_POWER;
      }
      else if( TC_stricmp( srType.data( ), "global" ) == 0 )
      {
         *ptype = TC_TYPE_GLOBAL;
      }
      else
      {
         *ptype = TC_TYPE_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid type, expected \"input\", \"output\", \"signal\", \"clock\", \"power\", or \"global\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
sideMode[ TC_SideMode_t* pside ]
   :
   <<
      string srSide;
   >>
   stringText[ &srSide ]
   <<
      if( TC_stricmp( srSide.data( ), "left" ) == 0 )
      {
         *pside = TC_SIDE_LEFT;
      }
      else if( TC_stricmp( srSide.data( ), "right" ) == 0 )
      {
         *pside = TC_SIDE_RIGHT;
      }
      else if(( TC_stricmp( srSide.data( ), "lower" ) == 0 ) ||
              ( TC_stricmp( srSide.data( ), "bottom" ) == 0 ))
      {
         *pside = TC_SIDE_LOWER;
      }
      else if(( TC_stricmp( srSide.data( ), "upper" ) == 0 ) ||
              ( TC_stricmp( srSide.data( ), "top" ) == 0 ))
      {
         *pside = TC_SIDE_UPPER;
      }
      else
      {
         *pside = TC_SIDE_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid side, expected \"left\", \"right\", \"lower\", or \"upper\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
classType[ TAS_ClassType_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      if( TC_stricmp( srType.data( ), "lut" ) == 0 )
      {
         *ptype = TAS_CLASS_LUT;
      }
      else if(( TC_stricmp( srType.data( ), "flipflop" ) == 0 ) ||
              ( TC_stricmp( srType.data( ), "ff" ) == 0 ))
      {
         *ptype = TAS_CLASS_FLIPFLOP;
      }
      else if(( TC_stricmp( srType.data( ), "memory" ) == 0 ) ||
              ( TC_stricmp( srType.data( ), "ram" ) == 0 ))
      {
         *ptype = TAS_CLASS_MEMORY;
      }
      else if(( TC_stricmp( srType.data( ), "subckt" ) == 0 ) ||
              ( TC_stricmp( srType.data( ), "blif" ) == 0 ))
      {
         *ptype = TAS_CLASS_SUBCKT;
      }
      else
      {
         *ptype = TAS_CLASS_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid class type, expected \"lut\", \"flipflop\", \"memory\", or \"subckt\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
arraySizeMode[ TAS_ArraySizeMode_t* pmode ]
   :
   <<
      string srMode;
   >>
   stringText[ &srMode ]
   <<
      if( TC_stricmp( srMode.data( ), "auto" ) == 0 )
      {
         *pmode = TAS_ARRAY_SIZE_AUTO;
      }
      else if(( TC_stricmp( srMode.data( ), "manual" ) == 0 ) ||
              ( TC_stricmp( srMode.data( ), "fixed" ) == 0 ))
      {
         *pmode = TAS_ARRAY_SIZE_MANUAL;
      }
      else
      {
         *pmode = TAS_ARRAY_SIZE_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid array size mode, expected \"auto\" or \"manual\"" );
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
      if( TC_stricmp( srType.data( ), "buffer" ) == 0 )
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
                                         ": Invalid switchbox type, expected \"buffer\" or \"mux\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
switchBoxModelType[ TAS_SwitchBoxModelType_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      if( TC_stricmp( srType.data( ), "wilton" ) == 0 )
      {
         *ptype = TAS_SWITCH_BOX_MODEL_WILTON;
      }
      else if(( TC_stricmp( srType.data( ), "subset" ) == 0 ) ||
              ( TC_stricmp( srType.data( ), "disjoint" ) == 0 ))
      {
         *ptype = TAS_SWITCH_BOX_MODEL_SUBSET;
      }
      else if( TC_stricmp( srType.data( ), "universal" ) == 0 )
      {
         *ptype = TAS_SWITCH_BOX_MODEL_UNIVERSAL;
      }
      else if( TC_stricmp( srType.data( ), "custom" ) == 0 )
      {
         *ptype = TAS_SWITCH_BOX_MODEL_CUSTOM;
      }
      else
      {
         *ptype = TAS_SWITCH_BOX_MODEL_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid switchbox model type, expected \"wilton\", \"subset\", \"universal\", or \"custom\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
segmentDirType[ TAS_SegmentDirType_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      if(( TC_stricmp( srType.data( ), "unidir" ) == 0 ) ||
         ( TC_stricmp( srType.data( ), "uni" ) == 0 ))
      {
         *ptype = TAS_SEGMENT_DIR_UNIDIRECTIONAL;
      }
      else if(( TC_stricmp( srType.data( ), "bidir" ) == 0 ) ||
              ( TC_stricmp( srType.data( ), "bi" ) == 0 ))
      {
         *ptype = TAS_SEGMENT_DIR_BIDIRECTIONAL;
      }
      else
      {
         *ptype = TAS_SEGMENT_DIR_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid segment type, expected \"unidir\" or \"bidir\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
pinAssignPatternType[ TAS_PinAssignPatternType_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      if( TC_stricmp( srType.data( ), "spread" ) == 0 )
      {
         *ptype = TAS_PIN_ASSIGN_PATTERN_SPREAD;
      }
      else if( TC_stricmp( srType.data( ), "custom" ) == 0 )
      {
         *ptype = TAS_PIN_ASSIGN_PATTERN_CUSTOM;
      }
      else
      {
         *ptype = TAS_PIN_ASSIGN_PATTERN_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid pin assign pattern type, expected \"spread\" or \"custom\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
gridAssignDistrMode[ TAS_GridAssignDistrMode_t* pmode ]
   :
   <<
      string srMode;
   >>
   stringText[ &srMode ]
   <<
      if(( TC_stricmp( srMode.data( ), "single" ) == 0 ) ||
         ( TC_stricmp( srMode.data( ), "rel" ) == 0 ))
      {
         *pmode = TAS_GRID_ASSIGN_DISTR_SINGLE;
      }
      else if(( TC_stricmp( srMode.data( ), "multiple" ) == 0 ) ||
              ( TC_stricmp( srMode.data( ), "multi" ) == 0 ) ||
              ( TC_stricmp( srMode.data( ), "col" ) == 0 ))
      {
         *pmode = TAS_GRID_ASSIGN_DISTR_MULTIPLE;
      }
      else if( TC_stricmp( srMode.data( ), "fill" ) == 0 )
      {
         *pmode = TAS_GRID_ASSIGN_DISTR_FILL;
      }
      else if( TC_stricmp( srMode.data( ), "perimeter" ) == 0 )
      {
         *pmode = TAS_GRID_ASSIGN_DISTR_PERIMETER;
      }
      else
      {
         *pmode = TAS_GRID_ASSIGN_DISTR_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid grid assign distribution mode, expected \"single\", \"multiple\", \"fill\", or \"perimeter\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
gridAssignOrientMode[ TAS_GridAssignOrientMode_t* pmode ]
   :
   <<
      string srMode;
   >>
   stringText[ &srMode ]
   <<
      if(( TC_stricmp( srMode.data( ), "column" ) == 0 ) ||
         ( TC_stricmp( srMode.data( ), "col" ) == 0 ))
      {
         *pmode = TAS_GRID_ASSIGN_ORIENT_COLUMN;
      }
      else if( TC_stricmp( srMode.data( ), "row" ) == 0 )
      {
         *pmode = TAS_GRID_ASSIGN_ORIENT_ROW;
      }
      else
      {
         *pmode = TAS_GRID_ASSIGN_ORIENT_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid grid assign orientation mode, expected \"column\" or \"row\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
interconnectMapType[ TAS_InterconnectMapType_t* ptype ]
   :
   <<
      string srType;
   >>
   stringText[ &srType ]
   <<
      if( TC_stricmp( srType.data( ), "complete" ) == 0 )
      {
         *ptype = TAS_INTERCONNECT_MAP_COMPLETE;
      }
      else if( TC_stricmp( srType.data( ), "direct" ) == 0 )
      {
         *ptype = TAS_INTERCONNECT_MAP_DIRECT;
      }
      else if( TC_stricmp( srType.data( ), "mux" ) == 0 )
      {
         *ptype = TAS_INTERCONNECT_MAP_MUX;
      }
      else
      {
         *ptype = TAS_INTERCONNECT_MAP_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid grid assign orientation type, expected \"complete\", \"direct\", or \"mux\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
timingMode[ TAS_TimingMode_t* pmode ]
   :
   <<
      string srMode;
   >>
   stringText[ &srMode ]
   <<
      if(( TC_stricmp( srMode.data( ), "delay_constant" ) == 0 ) ||
         ( TC_stricmp( srMode.data( ), "delay" ) == 0 ))
      {
         *pmode = TAS_TIMING_MODE_DELAY_CONSTANT;
      }
      else if( TC_stricmp( srMode.data( ), "delay_matrix" ) == 0 )
      {
         *pmode = TAS_TIMING_MODE_DELAY_MATRIX;
      }
      else if(( TC_stricmp( srMode.data( ), "t_setup" ) == 0 ) ||
              ( TC_stricmp( srMode.data( ), "clock_setup" ) == 0 ))
      {
         *pmode = TAS_TIMING_MODE_T_SETUP;
      }
      else if(( TC_stricmp( srMode.data( ), "t_clock_to_q" ) == 0 ) ||
              ( TC_stricmp( srMode.data( ), "clock_to_q" ) == 0 ))
      {
         *pmode = TAS_TIMING_MODE_CLOCK_TO_Q;
      }
      else if(( TC_stricmp( srMode.data( ), "cap_constant" ) == 0 ) ||
              ( TC_stricmp( srMode.data( ), "cap" ) == 0 ))
      {
         *pmode = TAS_TIMING_MODE_CAP_CONSTANT;
      }
      else if( TC_stricmp( srMode.data( ), "cap_matrix" ) == 0 )
      {
         *pmode = TAS_TIMING_MODE_CAP_MATRIX;
      }
      else if( TC_stricmp( srMode.data( ), "pack_pattern" ) == 0 )
      {
         *pmode = TAS_TIMING_MODE_PACK_PATTERN;
      }
      else
      {
         *pmode = TAS_TIMING_MODE_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid timing mode, expected \"delay_constant\", \"delay_matrix\", \"t_setup\", \"t_hold\", \"clock_to_q\", \"cap_constant\", \"cap_matrix\", or \"pack_pattern\"" );
         this->consumeUntilToken( END_OF_FILE );
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
   |  bitVal:BIT_CHAR
      <<
         *pdouble = atof( bitVal->getText( ));
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
   |  bitVal:BIT_CHAR
      <<
         *pdouble = atof( bitVal->getText( ));
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
   |  bitVal:BIT_CHAR
      <<
         *pint = static_cast< int >( atol( bitVal->getText( )));
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
   |  bitVal:BIT_CHAR
      <<
         *puint = static_cast< unsigned int >( atol( bitVal->getText( )));
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
#include "TAP_ArchitectureGrammar.h"
>>
