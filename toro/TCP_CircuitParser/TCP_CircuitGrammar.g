//===========================================================================//
// Purpose: PCCTS grammar for the circuit design file.
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
#include "TC_StringUtils.h"

#include "TCD_CircuitDesign.h"

#include "TCP_CircuitFile.h"
#include "TCP_CircuitScanner_c.h"
>>

//===========================================================================//
#lexclass QUOTED_VALUE
 
#token CLOSE_QUOTE     "\""                    << mode( START ); >>
#token STRING          "~[\"\n]+"
#token UNCLOSED_STRING "[\n]"

//===========================================================================//
#lexclass START

#token               "[\ \t]+"           << skip( ); >>
#token CPP_COMMENT   "//~[\n]*[\n]"      << skip( ); newline( ); >>
#token BLOCK_COMMENT "#~[\n]*[\n]"       << skip( ); newline( ); >>
#token XML_COMMENT   "<\!\-\-~[\n]*[\n]" << skip( ); newline( ); >>
#token NEW_LINE      "[\n\\]"            << skip( ); newline( ); >>
#token END_OF_FILE   "@"
#token OPEN_QUOTE    "\""                << mode( QUOTED_VALUE ); >>
#token EQUAL         "="

#token CIRCUIT    "[Cc][Ii][Rr][Cc][Uu][Ii][Tt]"
#token IO         "[Ii]{[Nn][Pp][Uu][Tt]}[Oo]{[Uu][Tt][Pp][Uu][Tt]}"
#token PB         "[Pp]{[Hh][Yy][Ss][Ii][Cc][Aa][Ll]}[Bb]{[Ll][Oo][Cc][Kk]}"
#token SB         "[Ss]{[Ww][Ii][Tt][Cc][Hh]}[Bb]{[Oo][Xx]{[Ee][Ss]}}"
#token BLOCK      "[Bb][Ll][Oo][Cc][Kk]"
#token PORT       "[Pp][Oo][Rr][Tt]"
#token INST       "[Ii][Nn][Ss][Tt]"
#token CELL       "[Cc][Ee][Ll][Ll]"
#token NAME       "[Nn][Aa][Mm][Ee]"
#token MASTER     "[Mm][Aa][Ss][Tt][Ee][Rr]"
#token SOURCE     "[Ss][Oo][Uu][Rr][Cc][Ee]"
#token NET        "[Nn][Ee][Tt]"
#token PIN        "[Pp][Ii][Nn]"
#token CLOCK      "[Cc][Ll][Oo][Cc][Kk]"

#token TYPE       "[Tt][Yy][Pp][Ee]"
#token STATE      "[Ss][Tt][Aa][Tt][Ee]"
#token ROUTABLE   "[Rr][Oo][Uu][Tt][Aa][Bb][Ll][Ee]"
#token STATUS     "[Ss][Tt][Aa][Tt][Uu][Ss]"
#token HIER       "[Hh][Ii][Ee][Rr]{[Aa][Rr][Cc][Hh][Yy]}"
#token PACK       "[Pp][Aa][Cc][Kk]{[Ii][Nn][Gg]}"
#token PLACE      "[Pp][Ll][Aa][Cc][Ee][Mm][Ee][Nn][Tt]"
#token TRACK      "[Tt][Rr][Aa][Cc][Kk]"
#token CHANNEL    "[Cc][Hh][Aa][Nn][Nn][Ee][Ll]"
#token SEGMENT    "[Ss][Ee][Gg][Mm][Ee][Nn][Tt]"
#token SIDES      "[Ss][Ii][Dd][Ee]{[Ss]}"
#token LENGTH     "[Ll][Ee][Nn][Gg][Tt][Hh]"
#token RELATIVE   "[Rr][Ee][Ll][Aa][Tt][Ii][Vv][Ee]"
#token REGION     "[Rr][Ee][Gg][Ii][Oo][Nn]"

#token GROUTE     "[Gg]{[Ll][Oo][Bb][Aa][Ll][_]}[Rr][Oo][Uu][Tt][Ee]"
#token ROUTE      "[Rr][Oo][Uu][Tt][Ee]"
#token SWITCHBOX  "[Ss][Ww][Ii][Tt][Cc][Hh][Bb][Oo][Xx]"

#token BOOL_TRUE  "([Tt][Rr][Uu][Ee]|[Yy][Ee][Ss]|[Oo][Nn])"
#token BOOL_FALSE "([Ff][Aa][Ll][Ss][Ee]|[Nn][Oo]|[Oo][Ff][Ff])"

#token NEG_INT    "[\-][0-9]+"
#token POS_INT    "[0-9]+"
#token FLOAT      "{\-}{[0-9]+}.[0-9]+"
#token EXP        "{\-}{[0-9]+}.[0-9]+[Ee][\+\-][0-9]+"
#token STRING     "[a-zA-Z_/\|\[\]\.][a-zA-Z0-9_/\|\[\]\(\)\.\+\-\~]*"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//

class TCP_CircuitParser_c
{
<<
public:

   void syn( ANTLRAbstractToken* /* pToken */, 
             ANTLRChar*          pszGroup,
             SetWordType*        /* pWordType */,
             ANTLRTokenType      tokenType,
             int                 /* k */ );
            
   void SetInterface( TCP_CircuitInterface_c* pinterface );
   void SetScanner( TCP_CircuitScanner_c* pscanner );
   void SetFileName( const char* pszFileName );
   void SetCircuitFile( TCP_CircuitFile_c* pcircuitFile );
   void SetCircuitDesign( TCD_CircuitDesign_c* pcircuitDesign );
>> 
<<
private:

   TCP_CircuitInterface_c* pinterface_;
   TCP_CircuitScanner_c*   pscanner_;
   string                  srFileName_;
   TCP_CircuitFile_c*      pcircuitFile_;
   TCD_CircuitDesign_c*    pcircuitDesign_;
>>

//===========================================================================//
start 
   :
   "<" CIRCUIT { NAME { EQUAL } } stringText[ &pcircuitDesign_->srName ] ">"
   (  "<"
      (  blockList[ &pcircuitDesign_->blockList ] 
      |  portList[ &pcircuitDesign_->portList,
                   &pcircuitDesign_->portNameList ]
      |  instList[ &pcircuitDesign_->instList,
                   &pcircuitDesign_->instNameList ]
      |  netList[ &pcircuitDesign_->netList ]
      )
   )*
   "</" CIRCUIT ">"
   ;

//===========================================================================//
blockList[ TPO_InstList_t* pblockList ]
   :
   <<
      TPO_Inst_c inst;

      string srName;
      string srCellName;
      string srPlaceFabricName;
      TPO_StatusMode_t placeStatus = TPO_STATUS_UNDEFINED;

      TPO_HierMapList_t hierMapList_;
      TPO_RelativeList_t relativeList_;
      TGS_RegionList_t regionList_;
   >>
   ( BLOCK | PB )
   { NAME { EQUAL } } stringText[ &srName ]
   <<
      inst.SetName( srName );
   >>
   (  MASTER { EQUAL } stringText[ &srCellName ]
      <<
         inst.SetCellName( srCellName );
      >>
   |  STATUS { EQUAL } placeStatusMode[ &placeStatus ]
      <<
         inst.SetPlaceStatus( placeStatus );
      >>
   )*
   ">"
   (  "<" 
      (  PACK hierMapList[ &hierMapList_ ] "</" PACK ">"
      |  REGION ">" regionList[ &regionList_ ] "</" REGION ">"
      |  RELATIVE ">" relativeList[ &relativeList_ ] "</" RELATIVE ">"
      |  PLACE { NAME { EQUAL } } stringText[ &srPlaceFabricName ]
         <<
            inst.SetPlaceFabricName( srPlaceFabricName );
         >>
         "/>"
      )
   )*
   "</" ( BLOCK | PB ) ">"
   <<
      if( inst.IsValid( ))
      {
         inst.SetPackHierMapList( hierMapList_ );
         inst.SetPlaceRelativeList( relativeList_ );
         inst.SetPlaceRegionList( regionList_ );

         pblockList->Add( inst );
      }
   >>
   ;

//===========================================================================//
portList[ TPO_PortList_t* pportList,
          TPO_NameList_t* pportNameList ]
   :
   <<
      TPO_Inst_c port;
      TPO_Pin_t pin;
      TPO_InstSource_t source;

      string srName;
      string srCellName;
      string srPlaceFabricName;
      TPO_StatusMode_t placeStatus = TPO_STATUS_UNDEFINED;
   >>
   ( PORT | IO )
   { NAME { EQUAL } } stringText[ &srName ]
   <<
      port.SetName( srName );
   >>
   (  MASTER { EQUAL } stringText[ &srCellName ]
      <<
         port.SetCellName( srCellName );
      >>
   |  SOURCE { EQUAL } cellSourceText[ &srCellName, &source ]
      <<
         port.SetCellName( srCellName );
         port.SetSource( source );
      >>
   |  STATUS { EQUAL } placeStatusMode[ &placeStatus ]
      <<
         port.SetPlaceStatus( placeStatus );
      >>
   )*
   ">"
   (  "<" 
      (  PIN pinDef[ &pin ]
         <<
            port.AddPin( pin);
            port.SetInputOutputType( pin.GetType( ));
         >>
      |  PLACE stringText[ &srPlaceFabricName ]
         <<
            port.SetPlaceFabricName( srPlaceFabricName );
         >>
      )
      "/>"
   )*
   "</" ( PORT | IO ) ">"
   <<
      if( port.IsValid( ))
      {
         pportList->Add( port );
         pportNameList->Add( port.GetName( ));
      }
   >>
   ;

//===========================================================================//
instList[ TPO_InstList_t* pinstList,
          TPO_NameList_t* pinstNameList ]
   :
   <<
      TPO_Inst_c inst;
      TPO_Pin_t pin;
      TPO_InstSource_t source;

      string srName;
      string srCellName;
   >>
   INST
   { NAME { EQUAL } } stringText[ &srName ]
   <<
      inst.SetName( srName );
   >>
   (  MASTER { EQUAL } stringText[ &srCellName ]
      <<
         inst.SetCellName( srCellName );
      >>
   |  SOURCE { EQUAL } cellSourceText[ &srCellName, &source ]
      <<
         inst.SetCellName( srCellName );
         inst.SetSource( source );
      >>
   )*
   ">"
   (  "<"
      (  CLOCK latchDef[ &inst ]
      |  PIN pinDef[ &pin ]
         <<
            inst.AddPin( pin );
         >>
      )
      "/>"
   )*
   "</" INST ">"
   <<
      if( inst.IsValid( ))
      {
         pinstList->Add( inst );
         pinstNameList->Add( inst.GetName( ));
      }
   >>
   ;

//===========================================================================//
netList[ TNO_NetList_c* pnetList ]
   :
   <<
      TNO_Net_c net;

      string srName;
      TC_TypeMode_t type = TC_TYPE_UNDEFINED;
      TNO_StatusMode_t netStatus = TNO_STATUS_UNDEFINED;
      bool routable = false;

      TNO_InstPinList_t instPinList_;
      TNO_GlobalRouteList_t globalRouteList_;
      TNO_RouteList_t routeList_;
   >>
   NET 
   { NAME { EQUAL } } stringText[ &srName ]
   <<
      net.SetName( srName );
   >>
   (  TYPE { EQUAL } typeMode[ &type ]
      << 
         net.SetType( type );
      >>
   |  STATUS { EQUAL } netStatusMode[ &netStatus ]
      <<
         net.SetStatus( netStatus );
      >>
   |  ROUTABLE { EQUAL } boolType[ &routable ]
      << 
         net.SetRoutable( routable );
      >>
   )*
   ">"
   (  "<"
      (  PIN instPinList[ &instPinList_ ] "/>"
      |  GROUTE globalRouteList[ &globalRouteList_ ] "/>"
      |  ROUTE ">" routeList[ &routeList_ ] "</" ROUTE ">"
      )
   )*
   "</" NET ">"
   <<
      if( net.IsValid( ))
      {
         net.AddInstPinList( instPinList_ );
         net.AddGlobalRouteList( globalRouteList_ );
         net.AddRouteList( routeList_ );

         pnetList->Add( net );
      }
   >>
   ;

//===========================================================================//
latchDef[ TPO_Inst_c* pinst ]
   :
   <<
      TPO_LatchType_t clockType = TPO_LATCH_TYPE_UNDEFINED;
      TPO_LatchState_t initState = TPO_LATCH_STATE_UNDEFINED;
   >>
   (  TYPE { EQUAL } latchType[ &clockType ]
      << 
         pinst->SetLatchClockType( clockType );
      >>
   |  STATE { EQUAL } latchState[ &initState ]
      << 
         pinst->SetLatchInitState( initState );
      >>
   )*
   ;

//===========================================================================//
pinDef[ TPO_Pin_t* ppin ]
   :
   <<
      string srName;
      TC_TypeMode_t type = TC_TYPE_UNDEFINED;

      ppin->Clear( );
   >>
   NAME { EQUAL } stringText[ &srName ]
   { TYPE { EQUAL } typeMode[ &type ] }
   << 
      ppin->Set( srName, type );
   >>
   ;

//===========================================================================//
instPinList[ TNO_InstPinList_t* pinstPinList ]
   :
   <<
      TNO_InstPin_c instPin;
   >>
   instPinDef[ &instPin ]
   << 
      if( instPin.IsValid( ))
      {
         pinstPinList->Add( instPin );
      }
   >>
   ;

//===========================================================================//
instPinDef[ TNO_InstPin_c* pinstPin ]
   :
   <<
      string srInstName, srPortName, srPinName;
      TC_TypeMode_t type = TC_TYPE_UNDEFINED;

      pinstPin->Clear( );
   >>
   (  INST { EQUAL } stringText[ &srInstName ]
   |  PORT { EQUAL } stringText[ &srPortName ]
   |  PIN { EQUAL } stringText[ &srPinName ]  
   )*
   <<
      if( srPinName.length( ))
      {
         pinstPin->Set( srInstName, srPortName, srPinName );
      }
      else
      {
         pinstPin->Set( srInstName, srPortName );
      }
   >>
   {  TYPE { EQUAL } typeMode[ &type ]
      << 
         pinstPin->SetType( type );
      >>
   }
   ;

//===========================================================================//
globalRouteList[ TNO_GlobalRouteList_t* pglobalRouteList ]
   :
   <<
      TNO_GlobalRoute_t globalRoute;

      string srName;
      unsigned int length = 0;
   >>
   NAME { EQUAL } stringText[ &srName ]
   LENGTH { EQUAL } uintNum[ &length ]
   << 
      globalRoute.Set( srName, length );
      pglobalRouteList->Add( globalRoute );
   >>
   ;

//===========================================================================//
routeList[ TNO_RouteList_t* prouteList ]
   :
   <<
      TNO_Route_t route;
      TNO_Node_c node;

      TNO_InstPin_c instPin;
      TNO_Segment_c segment;
      TNO_SwitchBox_c switchBox;
   >>
   (  "<"
      (  PIN instPinDef[ &instPin ] "/>"
         <<
            node.Clear( );
            node.Set( instPin );
         >>
      |  SEGMENT segmentDef[ &segment ] "</" SEGMENT ">"
         <<
            node.Clear( );
            node.Set( segment );
         >>
      |  SB switchBoxDef[ &switchBox ] "</" SB ">"
         <<
            node.Clear( );
            node.Set( switchBox );
         >>
      )
      << 
         if( node.IsValid( ))
         {
            route.Add( node );
         }
      >>
   )*
   << 
      if( route.IsValid( ))
      {
         prouteList->Add( route );
      }
   >>
   ;

//===========================================================================//
switchBoxDef[ TNO_SwitchBox_c* pswitchBox ]
   :
   <<
      string srName;
      TC_SideIndex_c sideIndex_;

      pswitchBox->Clear( );
   >>
   NAME { EQUAL } stringText[ &srName ]
   ">"
   "<" SIDES ">" sideIndex[ &sideIndex_ ] sideIndex[ &sideIndex_ ] "</" SIDES ">"
   <<
      pswitchBox->SetName( srName );
      pswitchBox->SetInput( sideIndex_ );
      pswitchBox->SetOutput( sideIndex_ );
   >>
   ;

//===========================================================================//
segmentDef[ TNO_Segment_c* psegment ]
   :
   <<
      string srName;
      TGS_Region_c channel;
      unsigned int track = 0;

      psegment->Clear( );
   >>
   (  NAME { EQUAL } stringText[ &srName ]
   |  TRACK { EQUAL } uintNum[ &track ]
   )*
   ">"
   "<" CHANNEL ">" regionDef[ &channel ] "</" CHANNEL ">"
   << 
      psegment->SetName( srName );
      psegment->SetChannel( channel );
      psegment->SetTrack( track );
   >>
   ;

//===========================================================================//
hierMapList[ TPO_HierMapList_t* phierMapList ]
   :
   <<
      TPO_HierMap_c hierMap;

      string srName;
      TPO_NameList_t hierNameList;
   >>
   { NAME { EQUAL } } stringText[ &srName ]
   << 
      hierMap.SetInstName( srName );
   >>
   (  ">" 
      "<" HIER ">"
      (
         stringText[ &srName ]
         << 
            hierNameList.Add( srName );
         >>
      )*
      "</" HIER ">"
      << 
         hierMap.SetHierNameList( hierNameList );
      >>
   )
   << 
      if( hierMap.IsValid( ))
      {
         phierMapList->Add( hierMap );
      }
   >>
   ;

//===========================================================================//
relativeList[ TPO_RelativeList_t* prelativeList ]
   :
   <<
      TPO_Relative_t relative;

      TC_SideMode_t side = TC_SIDE_UNDEFINED;
      string srName;
   >>
   sideMode[ &side ]
   stringText[ &srName ]
   << 
      relative.Set( side, srName );
      prelativeList->Add( relative );
   >>
   ;

//===========================================================================//
sideIndex[ TC_SideIndex_c* psideIndex ]
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
regionList[ TGS_RegionList_t* pregionList ]
   :
   <<
      TGS_Region_c region;
   >>
   regionDef[ &region ]
   <<
      if( region.IsValid( ))
      {
         pregionList->Add( region );
      }
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
latchType[ TPO_LatchType_t* platchType ]
   :
   <<
      string srLatchType;
   >>
   stringText[ &srLatchType ]
   <<
      if( TC_stricmp( srLatchType.data( ), "fe" ) == 0 )
      {
         *platchType = TPO_LATCH_TYPE_FALLING_EDGE;
      }
      else if( TC_stricmp( srLatchType.data( ), "re" ) == 0 )
      {
         *platchType = TPO_LATCH_TYPE_RISING_EDGE;
      }
      else if( TC_stricmp( srLatchType.data( ), "ah" ) == 0 )
      {
         *platchType = TPO_LATCH_TYPE_ACTIVE_HIGH;
      }
      else if( TC_stricmp( srLatchType.data( ), "al" ) == 0 )
      {
         *platchType = TPO_LATCH_TYPE_ACTIVE_LOW;
      }
      else if( TC_stricmp( srLatchType.data( ), "as" ) == 0 )
      {
         *platchType = TPO_LATCH_TYPE_ASYNCHRONOUS;
      }
      else
      {
         *platchType = TPO_LATCH_TYPE_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid latch type, expected \"fe\", \"re\", \"ah\", \"al\", or \"as\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
latchState[ TPO_LatchState_t* platchState ]
   :
   <<
      string srLatchState;
   >>
   stringText[ &srLatchState ]
   <<
      if( TC_stricmp( srLatchState.data( ), "0" ) == 0 )
      {
         *platchState = TPO_LATCH_STATE_FALSE;
      }
      else if( TC_stricmp( srLatchState.data( ), "1" ) == 0 )
      {
         *platchState = TPO_LATCH_STATE_TRUE;
      }
      else if( TC_stricmp( srLatchState.data( ), "2" ) == 0 )
      {
         *platchState = TPO_LATCH_STATE_DONT_CARE;
      }
      else if( TC_stricmp( srLatchState.data( ), "3" ) == 0 )
      {
         *platchState = TPO_LATCH_STATE_UNKNOWN;
      }
      else
      {
         *platchState = TPO_LATCH_STATE_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid latch state, expected \"0\", \"1\", \"2\", or \"3\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
placeStatusMode[ TPO_StatusMode_t* pstatusMode ]
   :
   <<
      string srStatusMode;
   >>
   stringText[ &srStatusMode ]
   <<
      if( TC_stricmp( srStatusMode.data( ), "float" ) == 0 )
      {
         *pstatusMode = TPO_STATUS_FLOAT;
      }
      else if( TC_stricmp( srStatusMode.data( ), "fixed" ) == 0 )
      {
         *pstatusMode = TPO_STATUS_FIXED;
      }
      else if( TC_stricmp( srStatusMode.data( ), "placed" ) == 0 )
      {
         *pstatusMode = TPO_STATUS_PLACED;
      }
      else
      {
         *pstatusMode = TPO_STATUS_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid place status, expected \"float\", \"fixed\", or \"placed\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
netStatusMode[ TNO_StatusMode_t* pstatusMode ]
   :
   <<
      string srStatusMode;
   >>
   stringText[ &srStatusMode ]
   <<
      if( TC_stricmp( srStatusMode.data( ), "open" ) == 0 )
      {
         *pstatusMode = TNO_STATUS_OPEN;
      }
      else if(( TC_stricmp( srStatusMode.data( ), "global_routed" ) == 0 ) ||
              ( TC_stricmp( srStatusMode.data( ), "grouted" ) == 0 ))
      {
         *pstatusMode = TNO_STATUS_GROUTED;
      }
      else if( TC_stricmp( srStatusMode.data( ), "routed" ) == 0 )
      {
         *pstatusMode = TNO_STATUS_ROUTED;
      }
      else
      {
         *pstatusMode = TNO_STATUS_UNDEFINED;

         this->pinterface_->SyntaxError( LT( 0 )->getLine( ),
                                         this->srFileName_,
                                         ": Invalid net status, expected \"open\", \"global_routed\", or \"routed\"" );
         this->consumeUntilToken( END_OF_FILE );
      }
   >>
   ;

//===========================================================================//
cellSourceText[ string* psrString, 
                TPO_InstSource_t* psource ]
   :
   stringText[ psrString ]
   <<
      *psource = TPO_INST_SOURCE_UNDEFINED;
      if( TC_stricmp( psrString->data( ), ".names" ) == 0 )
      {
         *psource = TPO_INST_SOURCE_NAMES;
      }
      else if( TC_stricmp( psrString->data( ), ".latch" ) == 0 )
      {
         *psource = TPO_INST_SOURCE_LATCH;
      }
      else if( TC_stricmp( psrString->data( ), ".input" ) == 0 )
      {
         *psource = TPO_INST_SOURCE_INPUT;
      }
      else if( TC_stricmp( psrString->data( ), ".output" ) == 0 )
      {
         *psource = TPO_INST_SOURCE_OUTPUT;
      }
      else
      {
         *psource = TPO_INST_SOURCE_SUBCKT;
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

}

//===========================================================================//
<<
#include "TCP_CircuitGrammar.h"
>>
