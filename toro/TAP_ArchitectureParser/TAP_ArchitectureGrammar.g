//===========================================================================//
// Purpose: PCCTS grammar for the architecture spec file.
//
//===========================================================================//

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

#token                   "[\ \t]+"           << skip( ); >>
#token CPP_COMMENT       "//~[\n]*[\n]"      << skip( ); newline( ); >>
#token BLOCK_COMMENT     "#~[\n]*[\n]"       << skip( ); newline( ); >>
#token XML_COMMENT       "<\!\-\-~[\n]*[\n]" << skip( ); newline( ); >>
#token NEW_LINE          "[\n\\]"            << skip( ); newline( ); >>
#token END_OF_FILE       "@"
#token OPEN_QUOTE        "\""                << mode( QUOTED_VALUE ); >>
#token EQUAL             "="

#token ARCHITECTURE      "[Aa][Rr][Cc][Hh][Ii][Tt][Ee][Cc][Tt][Uu][Rr][Ee]"

#token CONFIG            "[Cc][Oo][Nn][Ff][Ii][Gg]"
#token IO                "[Ii]{[Nn][Pp][Uu][Tt]}[Oo]{[Uu][Tt][Pp][Uu][Tt]}"
#token PB                "[Pp]{[Hh][Yy][Ss][Ii][Cc][Aa][Ll]}[Bb]{[Ll][Oo][Cc][Kk]}"
#token SB                "[Ss]{[Ww][Ii][Tt][Cc][Hh]}[Bb]{[Oo][Xx]{[Ee][Ss]}}"
#token CB                "[Cc]{[Oo][Nn][Nn][Ee][Cc][Tt][Ii][Oo][Nn]}[Bb]{[Oo][Xx]{[Ee][Ss]}}"
#token SEGMENT           "[Ss][Ee][Gg][Mm][Ee][Nn][Tt]{[Ss]}"
#token MODEL             "[Mm][Oo][Dd][Ee]{[Ll]}{[Ss]}"
#token CELL              "[Cc][Ee][Ll][Ll]{[Ss]}"
#token PIN               "[Pp][Ii][Nn]{[Ss]}"

#token NAME              "[Nn][Aa][Mm][Ee]"
#token TYPE              "[Tt][Yy][Pp][Ee]"
#token CLASS             "[Cc][Ll][Aa][Ss][Ss]"
#token FS                "[Ff][Ss]"

#token INTERCONNECT      "[Ii][Nn][Tt][Ee][Rr][Cc][Oo][Nn][Nn][Ee][Cc][Tt]"

#token FC_IN             "[Ff][Cc][_][Ii][Nn]"
#token FC_OUT            "[Ff][Cc][_][Oo][Uu][Tt]"

#token WIDTH             "[Ww][Ii][Dd][Tt][Hh]"
#token HEIGHT            "[Hh][Ee][Ii][Gg][Hh][Tt]"
#token LENGTH            "[Ll][Ee][Nn][Gg][Tt][Hh]"
#token CAPACITY          "[Cc][Aa][Pp][Aa][Cc][Ii][Tt][Yy]"

#token SIZE              "{[Aa][Rr][Rr][Aa][Yy][_]}[Ss][Ii][Zz][Ee]"
#token RATIO             "{[Aa][Ss][Pp][Ee][Cc][Tt][_]}[Rr][Aa][Tt][Ii][Oo]"
#token ORIGIN            "[Oo][Rr][Ii][Gg][Ii][Nn]"
#token COUNT             "[Cc][Oo][Uu][Nn][Tt]"
#token SIDE              "[Ss][Ii][Dd][Ee]"
#token OFFSET            "[Oo][Ff][Ff][Ss][Ee][Tt]"
#token EQUIVALENCE       "[Ee][Qq][Uu][Ii][Vv][Aa][Ll][Ee][Nn][Cc][Ee]"

#token FULL              "[Ff][Uu][Ll][Ll]"
#token LONGLINE          "[Ll][Oo][Nn][Gg][Ll][Ii][Nn][Ee]"

#token PIN_ASSIGN        "[Pp][Ii][Nn][_][Aa][Ss][Ss][Ii][Gg][Nn]{[Mm][Ee][Nn][Tt]}"
#token GRID_ASSIGN       "[Gg][Rr][Ii][Dd][_][Aa][Ss][Ss][Ii][Gg][Nn]{[Mm][Ee][Nn][Tt]}"

#token MAPPING           "[Mm][Aa][Pp][Pp][Ii][Nn][Gg]"

#token TIMING            "[Tt][Ii][Mm][Ii][Nn][Gg]{[_][Aa][Nn][Aa][Ll][Yy][Ss][Ii][Ss]}"
#token R                 "[Rr]"
#token RES               "[Rr][Ee][Ss]"
#token CAP               "[Cc]{[Aa][Pp]}"
#token CAP_IN            "[Cc]{[Aa][Pp]}{[_]}[Ii][Nn]"
#token CAP_OUT           "[Cc]{[Aa][Pp]}{[_]}[Oo][Uu][Tt]"
#token T                 "[Tt]"
#token DELAY             "[Dd][Ee][Ll][Aa][Yy]{[_][Ii][Nn]}"

#token EST               "[Ee][Ss][Tt]{[Ii][Mm][Aa][Tt][Ee][Dd]}"
#token MINW_NMOS_R       "[Mm][Ii][Nn][_][Ww][Ii][Dd][Tt][Hh][_][Nn][Mm][Oo][Ss][_][Rr][Ee][Ss]"
#token MINW_PMOS_R       "[Mm][Ii][Nn][_][Ww][Ii][Dd][Tt][Hh][_][Pp][Mm][Oo][Ss][_][Rr][Ee][Ss]"
#token MUX_IN_PIN_SIZE   "[Mm][Uu][Xx][_][Tt][Rr][Aa][Nn][Ss][_][Ii][Nn][_][Pp][Ii][Nn][_][Ss][Ii][Zz][Ee]"
#token LOGIC_TILE_AREA   "[Gg][Rr][Ii][Dd][_][Ll][Oo][Gg][Ii][Cc][_][Tt][Ii][Ll][Ee][_][Aa][Rr][Ee][Aa]"

#token FREQ              "[Ff][Rr][Ee][Qq]"

#token WIRE_SWITCH       "[Ww][Ii][Rr][Ee][_][Ss][Ww][Ii][Tt][Cc][Hh]"
#token OPIN_SWITCH       "[Oo][Pp][Ii][Nn][_][Ss][Ww][Ii][Tt][Cc][Hh]"

#token PATTERN           "[Pp][Aa][Tt][Tt][Ee][Rr][Nn]"

#token INPUT_PORTS       "[Ii][Nn][Pp][Uu][Tt][_][Pp][Oo][Rr][Tt][Ss]"
#token OUTPUT_PORTS      "[Oo][Uu][Tt][Pp][Uu][Tt][_][Pp][Oo][Rr][Tt][Ss]"
#token IS_CLOCK          "[Ii][Ss][_][Cc][Ll][Oo][Cc][Kk]"

#token INPUT             "[Ii][Nn][Pp][Uu][Tt]"
#token OUTPUT            "[Oo][Uu][Tt][Pp][Uu][Tt]"
#token CLOCK             "[Cc][Ll][Oo][Cc][Kk]"

#token MAX_DELAY         "[Mm][Aa][Xx][_][Dd][Ee][Ll][Aa][Yy]"
#token MAX_DELAY_MATRIX  "{[Mm][Aa][Xx][_]}[Dd][Ee][Ll][Aa][Yy][_][Mm][Aa][Tt][Rr][Ii][Xx]"
#token CLOCK_SETUP_DELAY "{[Cc][Ll][Oo][Cc][Kk][_]}[Ss][Ee][Tt][Uu][Pp][_][Dd][Ee][Ll][Aa][Yy]"
#token CLOCK_TO_Q_DELAY  "[Cc][Ll][Oo][Cc][Kk]{[_][Tt][Oo][_][Qq]}[_][Dd][Ee][Ll][Aa][Yy]"

#token PRIORITY          "[Pp][Rr][Ii][Oo][Rr][Ii][Tt][Yy]"
#token SINGLE_POS        "[Pp][Oo][Ss]"
#token MULTIPLE_START    "[Ss][Tt][Aa][Rr][Tt]"
#token MULTIPLE_REPEAT   "[Rr][Ee][Pp][Ee][Aa][Tt]"

#token AUTO         "[Aa][Uu][Tt][Oo]"
#token MANUAL       "[Mm][Aa][Nn][Uu][Aa][Ll]"
#token FIXED        "[Ff][Ii][Xx][Ee][Dd]"
#tokclass ARRAY_SIZE_MODE_VAL { AUTO MANUAL FIXED }

#token SPREAD       "[Ss][Pp][Rr][Ee][Aa][Dd]"
#token CUSTOM       "[Cc][Uu][Ss][Tt][Oo][Mm]"
#tokclass PIN_PATTERN_TYPE_VAL { SPREAD CUSTOM }

#token SINGLE       "[Ss][Ii][Nn][Gg][Ll][Ee]"
#token MULTIPLE     "[Mm][Uu][Ll][Tt][Ii]{[Pp][Ll][Ee]}"
#token COL          "[Cc][Oo][Ll]{[Uu][Mm][Nn]}"
#token REL          "[Rr][Ee][Ll]"
#token FILL         "[Ff][Ii][Ll][Ll]"
#token PERIMETER    "[Pp][Ee][Rr][Ii][Mm][Ee][Tt][Ee][Rr]"
#tokclass GRID_DISTR_MODE_VAL { SINGLE MULTIPLE REL COL FILL PERIMETER }

#token ROW          "[Rr][Oo][Ww]"
#tokclass GRID_ORIENT_MODE_VAL { COL ROW }

#token MUX          "[Mm][Uu][Xx]"
#token BUFFER       "[Bb][Uu][Ff][Ff][Ee][Rr]{[Ee][Dd]}"
#tokclass SB_TYPE_VAL { BUFFER MUX }

#token WILTON       "[Ww][Ii][Ll][Tt][Oo][Nn]"
#token SUBSET       "[Ss][Uu][Bb][Ss][Ee][Tt]"
#token DISJOINT     "[Dd][Ii][Ss][Jj][Oo][Ii][Nn][Tt]"
#token UNIVERSAL    "[Uu][Nn][Ii][Vv][Ee][Rr][Ss][Aa][Ll]"
#tokclass SB_MODEL_TYPE_VAL { WILTON SUBSET DISJOINT UNIVERSAL CUSTOM }

#token UNIDIR       "[Uu][Nn][Ii]{{[_]}[Dd][Ii][Rr]}"
#token BIDIR        "[Bb][Ii]{{[_]}[Dd][Ii][Rr]}"
#tokclass SEGMENT_DIR_TYPE_VAL { UNIDIR BIDIR }

#token LUT          "[Ll][Uu][Tt]"
#token FLIPFLOP     "[Ff]{[Ll][Ii][Pp]}[Ff]{[Ll][Oo][Pp]}"
#token MEMORY       "([Mm][Ee][Mm][Oo][Rr][Yy]|[Rr][Aa][Mm])"
#token SUBCKT       "([Ss][Uu][Bb][Cc][Kk][Tt]|[Bb][Ll][Ii][Ff])"
#tokclass CLASS_TYPE_VAL { LUT FLIPFLOP MEMORY SUBCKT }

#token COMPLETE     "[Cc][Oo][Mm][Pp][Ll][Ee][Tt][Ee]"
#token DIRECT       "[Dd][Ii][Rr][Ee][Cc][Tt]"
#tokclass MAP_TYPE_VAL { COMPLETE DIRECT MUX }

#token LEFT         "[Ll]{[Ee][Ff][Tt]}"
#token RIGHT        "[Rr]{[Ii][Gg][Hh][Tt]}"
#token LOWER        "[Ll][Oo][Ww][Ee][Rr]"
#token UPPER        "[Uu][Pp][Pp][Ee][Rr]"
#token BOTTOM       "[Bb]{[Oo][Tt][Tt][Oo][Mm]}"
#token TOP          "[Tt]{[Oo][Pp]}"
#tokclass SIDE_VAL  { LEFT RIGHT LOWER UPPER BOTTOM TOP R T }

#token SIGNAL       "[Ss][Ii][Gg][Nn][Aa][Ll]"
#token POWER        "[Pp][Oo][Ww][Ee][Rr]"
#token GLOBAL       "[Gg][Ll][Oo][Bb][Aa][Ll]"
#tokclass TYPE_VAL  { INPUT OUTPUT SIGNAL CLOCK POWER GLOBAL }

#token BOOL_TRUE    "([Tt][Rr][Uu][Ee]|[Yy][Ee][Ss]|[Oo][Nn])"
#token BOOL_FALSE   "([Ff][Aa][Ll][Ss][Ee]|[Nn][Oo]|[Oo][Ff][Ff])"
#tokclass BOOL_VAL  { BOOL_TRUE BOOL_FALSE }

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

   TAS_ClassType_t FindClassType_( ANTLRTokenType tokenType );
   TAS_ArraySizeMode_t FindArraySizeMode_( ANTLRTokenType tokenType );
   TAS_SwitchBoxType_t FindSwitchBoxType_( ANTLRTokenType tokenType );
   TAS_SwitchBoxModelType_t FindSwitchBoxModelType_( ANTLRTokenType tokenType );
   TAS_SegmentDirType_t FindSegmentDirType_( ANTLRTokenType tokenType );
   TAS_PinAssignPatternType_t FindPinAssignPatternType_( ANTLRTokenType tokenType );
   TAS_GridAssignDistrMode_t FindGridAssignDistrMode_( ANTLRTokenType tokenType );
   TAS_GridAssignOrientMode_t FindGridAssignOrientMode_( ANTLRTokenType tokenType );
   TAS_InterconnectMapType_t FindInterconnectMapType_( ANTLRTokenType tokenType );
   TC_SideMode_t FindSideMode_( ANTLRTokenType tokenType );
   TC_TypeMode_t FindTypeMode_( ANTLRTokenType tokenType );
   bool FindBool_( ANTLRTokenType tokenType );

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
   "<" ARCHITECTURE stringText[ &this->parchitectureSpec_->srName ] ">"
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
      (  SIZE arraySizeModeVal:ARRAY_SIZE_MODE_VAL
         << 
            pconfig->layout.sizeMode = this->FindArraySizeMode_( arraySizeModeVal->getType( ));
         >>
         (  RATIO { EQUAL } floatNum[ &pconfig->layout.autoSize.aspectRatio ]
         |  WIDTH { EQUAL } intNum[ &pconfig->layout.manualSize.gridDims.width ]
         |  HEIGHT { EQUAL } intNum[ &pconfig->layout.manualSize.gridDims.height ]
         )*
      |  EST
         (  MINW_NMOS_R { EQUAL } floatNum[ &pconfig->device.areaModel.resMinWidthNMOS ]
         |  MINW_PMOS_R { EQUAL } floatNum[ &pconfig->device.areaModel.resMinWidthPMOS ]
         |  MUX_IN_PIN_SIZE { EQUAL } floatNum[ &pconfig->device.areaModel.sizeInputPinMux ]
         |  LOGIC_TILE_AREA { EQUAL } floatNum[ &pconfig->device.areaModel.areaGridTile ]
         )*
      |  SB modelTypeVal:SB_MODEL_TYPE_VAL
         << 
            pconfig->device.switchBoxes.modelType = this->FindSwitchBoxModelType_( modelTypeVal->getType( ));
         >>
         { FS { EQUAL } uintNum[ &pconfig->device.switchBoxes.fs ] }
      |  CB
         (  ( CAP | CAP_IN ) { EQUAL } expNum[ &pconfig->device.connectionBoxes.capInput ]
         |  ( T | DELAY ) { EQUAL } expNum[ &pconfig->device.connectionBoxes.delayInput ]
         )*
      |  SEGMENT dirTypeVal:SEGMENT_DIR_TYPE_VAL
         << 
            pconfig->device.segments.dirType = this->FindSegmentDirType_( dirTypeVal->getType( ));
         >>
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
   stringText[ &inputOutput.srName ]
   (  CAPACITY { EQUAL } uintNum[ &inputOutput.capacity ]
   |  FC_IN { EQUAL } fcDef[ &inputOutput.fcIn ]
   |  FC_OUT { EQUAL } fcDef[ &inputOutput.fcOut ]
   |  SIZE { EQUAL } floatDims[ &dims ]
   |  ORIGIN { EQUAL } originPoint[ &origin ]
   )*
   "/>"

   (  "<"
      (  MODEL modeNameList[ &inputOutput.modeNameList ] "/>"
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
   stringText[ &physicalBlock.srName ]
   (  COUNT { EQUAL } uintNum[ &physicalBlock.numPB ]
   |  CELL { EQUAL } cellModelText[ &physicalBlock.srModelName,
                                    &physicalBlock.modelType ]
   |  CLASS { EQUAL } classTypeVal:CLASS_TYPE_VAL
      << 
         physicalBlock.classType = this->FindClassType_( classTypeVal->getType( ));
      >>
   |  FC_IN { EQUAL } fcDef[ &physicalBlock.fcIn ]
   |  FC_OUT { EQUAL } fcDef[ &physicalBlock.fcOut ]
   |  SIZE { EQUAL } floatDims[ &dims ]
   |  ORIGIN { EQUAL } originPoint[ &origin ]
   )*
   "/>"

   (  "<"
      (  MODEL modeNameList[ &physicalBlock.modeNameList ] "/>"
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
   stringText[ &mode.srName ] 
   ">"
   (  "<"
      (  PB
         stringText[ &physicalBlock.srName ]
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
            interconnect.timingDelayLists.delayClockSetupList.Clear( );
            interconnect.timingDelayLists.delayClockToQList.Clear( );
         >>
         stringText[ &interconnect.srName ]
         mapTypeVal:MAP_TYPE_VAL
         << 
            interconnect.mapType = this->FindInterconnectMapType_( mapTypeVal->getType( ));
         >>
         ">"
         (  "<"
            (  INPUT stringText[ &srInputName ]
               <<
                  interconnect.inputNameList.Add( srInputName );
               >>
            |  OUTPUT stringText[ &srOutputName ]
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
   stringText[ &switchBox.srName ]
   (  TYPE { EQUAL } typeVal:SB_TYPE_VAL
      << 
         switchBox.type = this->FindSwitchBoxType_( typeVal->getType( ));
      >>
   |  MODEL { EQUAL } modelTypeVal:SB_MODEL_TYPE_VAL
      << 
         switchBox.model = this->FindSwitchBoxModelType_( modelTypeVal->getType( ));
      >>
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
   |  TYPE { EQUAL } dirTypeVal:SEGMENT_DIR_TYPE_VAL
      <<
         segment.dirType = this->FindSegmentDirType_( dirTypeVal->getType( ));
      >>
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
   stringText[ &srName ] 
   <<
      cell.SetName( srName );
   >>
   (  CLASS { EQUAL } classTypeVal:CLASS_TYPE_VAL
      << 
         cell.classType = this->FindClassType_( classTypeVal->getType( ));
      >>
   )*
   ">"
   (  "<" 
      (  PIN
         stringText[ &srName ]
         <<
            port.Clear( );
            port.SetName( srName );
         >>
         typeVal:TYPE_VAL
         << 
            type = this->FindTypeMode_( typeVal->getType( ));
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
   |  floatVal:FLOAT
      <<
         pfc->type = TAS_CONNECTION_BOX_FRACTION;
         pfc->percent = atof( floatVal->getText( ));
         pfc->absolute = 0;
      >>
   |  uintVal:POS_INT
      <<
         pfc->type = TAS_CONNECTION_BOX_ABSOLUTE;
         pfc->percent = 0.0;
         pfc->absolute = static_cast< unsigned int >( atol( uintVal->getText( )));
      >>
   |  bitVal:BIT_CHAR
      <<
         pfc->type = TAS_CONNECTION_BOX_ABSOLUTE;
         pfc->percent = 0.0;
         pfc->absolute = static_cast< unsigned int >( atol( bitVal->getText( )));
      >>
   )
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
      bool isEquivalent = false;
      string srClass;
      double cap = 0.0;
      double delay = 0.0;
   >>
   stringText[ &srName ]
   <<
      port.SetName( srName );
   >>
   typeVal:TYPE_VAL
   << 
      type = this->FindTypeMode_( typeVal->getType( ));
      port.SetType( type );
   >>
   (  COUNT { EQUAL } uintNum[ &count ]
      << 
         port.SetCount( count );
      >>
   |  EQUIVALENCE { EQUAL } boolVal:BOOL_VAL
      << 
         isEquivalent = this->FindBool_( boolVal->getType( ));
         port.SetEquivalent( isEquivalent );
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
   (  MAX_DELAY 
      (  ( T | DELAY ) { EQUAL } expNum[ &timingDelay.delay ]
      |  INPUT { EQUAL } stringText[ &timingDelay.srInputPortName ]
      |  OUTPUT { EQUAL } stringText[ &timingDelay.srOutputPortName ]
      )*
      << 
         timingDelay.type = TAS_TIMING_DELAY_CONSTANT;
         ptimingDelayLists->delayList.Add( timingDelay );
      >>
   |  MAX_DELAY_MATRIX
      (  ( T | DELAY ) { EQUAL } delayMatrixDef[ &timingDelay.delayMatrix ]
      |  INPUT { EQUAL } stringText[ &timingDelay.srInputPortName ]
      |  OUTPUT { EQUAL } stringText[ &timingDelay.srOutputPortName ]
      )*
      << 
         timingDelay.type = TAS_TIMING_DELAY_MATRIX;
         ptimingDelayLists->delayMatrixList.Add( timingDelay );
      >>
   |  CLOCK_SETUP_DELAY
      (  ( T | DELAY ) { EQUAL } expNum[ &timingDelay.delay ]
      |  INPUT { EQUAL } stringText[ &timingDelay.srInputPortName ]
      |  CLOCK { EQUAL } stringText[ &timingDelay.srClockPortName ]
      )*
      << 
         timingDelay.type = TAS_TIMING_DELAY_SETUP;
         ptimingDelayLists->delayClockSetupList.Add( timingDelay );
      >>
   |  CLOCK_TO_Q_DELAY 
      (  ( T | DELAY ) { EQUAL } expNum[ &timingDelay.delay ]
      |  OUTPUT { EQUAL } stringText[ &timingDelay.srOutputPortName ]
      |  CLOCK { EQUAL } stringText[ &timingDelay.srClockPortName ]
      )*
      << 
         timingDelay.type = TAS_TIMING_DELAY_CLOCK_TO_Q;
         ptimingDelayLists->delayClockToQList.Add( timingDelay );
      >>
   )
   ;

//===========================================================================//
delayMatrixDef[ TAS_DelayMatrix_t* pdelayMatrix ]
   :
   <<
      TAS_DelayTable_t delayTable;
      TAS_DelayList_t delayList;

      double value;
      size_t curTokenLine, nextTokenLine;
   >>
   (  expNum[ &value ]
      <<
         delayList.Add( value );

         curTokenLine = ( LT( 0 ) ? LT( 0 )->getLine( ) : 0 );
         nextTokenLine = ( LT( 1 ) ? LT( 1 )->getLine( ) : 0 );
         if( curTokenLine != nextTokenLine )
         {
            delayTable.Add( delayList );
            delayList.Clear( );
         }
      >>
   )+
   <<
      if( delayTable.IsValid( ) && delayTable[ 0 ]->IsValid( ))
      {
         size_t height = delayTable.GetLength( );
         size_t width = SIZE_MAX;
         for( size_t i = 0; i < delayTable.GetLength( ); ++i )
         {
            width = TCT_Min( width, delayTable[ i ]->GetLength( ));
         }

         value = 0.0;
         pdelayMatrix->SetCapacity( width, height, value );
   
         for( size_t j = 0; j < height; ++j )
         {
            const TAS_DelayList_t& delayList_ = *delayTable[ j ];
            for( size_t i = 0; i < width; ++i )
            {
               ( *pdelayMatrix )[i][j] = *delayList_[ i ];
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
   patternTypeVal:PIN_PATTERN_TYPE_VAL
   <<
      pinAssign.pattern = this->FindPinAssignPatternType_( patternTypeVal->getType( ));
      *ppinAssignPattern = pinAssign.pattern;
   >>
   (  SIDE { EQUAL } sideVal:SIDE_VAL
      << 
         pinAssign.side = this->FindSideMode_( sideVal->getType( ));
      >>
   |  OFFSET { EQUAL } uintNum[ &pinAssign.offset ]
   )*
   {  ">" 
      "<" PIN 
      (  stringText[ &srPortName ] 
         <<
            pinAssign.portNameList.Add( srPortName );
         >>
      )*
      "/>"
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
   distrModeVal:GRID_DISTR_MODE_VAL
   <<
      gridAssign.distr = this->FindGridAssignDistrMode_( distrModeVal->getType( ));
   >>
   (  ORIENT { EQUAL } orientModeVal:GRID_ORIENT_MODE_VAL
      <<
         gridAssign.orient = this->FindGridAssignOrientMode_( orientModeVal->getType( ));
      >>
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
   SB TYPE { EQUAL } PATTERN ">"
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
   CB TYPE { EQUAL } PATTERN ">"
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
   floatNum[ &pfloatDims->width ]
   floatNum[ &pfloatDims->height ]
   ;

//===========================================================================//
originPoint[ TGS_Point_c* poriginPoint ]
   : 
   floatNum[ &poriginPoint->x ]
   floatNum[ &poriginPoint->y ]
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
   (  OPEN_QUOTE
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
   )
   ;

//===========================================================================//
expNum[ double* pdouble ]
   : 
   (  OPEN_QUOTE
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
   )
   ;

//===========================================================================//
intNum[ int* pint ]
   :
   (  OPEN_QUOTE
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
   )
   ;

//===========================================================================//
uintNum[ unsigned int* puint ]
   :
   (  OPEN_QUOTE
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
   )
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
