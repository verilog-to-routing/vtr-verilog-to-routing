//===========================================================================//
// Purpose: PCCTS grammar for the circuit design file.
//
//===========================================================================//

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

#token                 "[\ \t]+"      << skip( ); >>
#token CPP_COMMENT     "//~[\n]*[\n]" << skip( ); newline( ); >>
#token BLOCK_COMMENT   "#~[\n]*[\n]"  << skip( ); newline( ); >>
#token NEW_LINE        "[\n\\]"       << skip( ); newline( ); >>
#token END_OF_FILE     "@"
#token OPEN_QUOTE      "\""           << mode( QUOTED_VALUE ); >>
#token EQUAL           "="

#token CIRCUIT      "[Cc][Ii][Rr][Cc][Uu][Ii][Tt]"
#token IO           "[Ii]{[Nn][Pp][Uu][Tt]}[Oo]{[Uu][Tt][Pp][Uu][Tt]}"
#token PB           "[Pp]{[Hh][Yy][Ss][Ii][Cc][Aa][Ll]}[Bb]{[Ll][Oo][Cc][Kk]}"
#token SB           "[Ss]{[Ww][Ii][Tt][Cc][Hh]}[Bb]{[Oo][Xx]{[Ee][Ss]}}"
#token BLOCK        "[Bb][Ll][Oo][Cc][Kk]"
#token PORT         "[Pp][Oo][Rr][Tt]"
#token INST         "[Ii][Nn][Ss][Tt]"
#token CELL         "[Cc][Ee][Ll][Ll]"
#token MASTER       "[Mm][Aa][Ss][Tt][Ee][Rr]"
#token NET          "[Nn][Ee][Tt]"
#token PIN          "[Pp][Ii][Nn]"

#token TYPE         "[Tt][Yy][Pp][Ee]"
#token STATE        "[Ss][Tt][Aa][Tt][Ee]"
#token ROUTABLE     "[Rr][Oo][Uu][Tt][Aa][Bb][Ll][Ee]"
#token STATUS       "[Ss][Tt][Aa][Tt][Uu][Ss]"
#token HIER         "[Hh][Ii][Ee][Rr]{[Aa][Rr][Cc][Hh][Yy]}"
#token PACK         "[Pp][Aa][Cc][Kk]{[Ii][Nn][Gg]}"
#token PLACE        "[Pp][Ll][Aa][Cc][Ee][Mm][Ee][Nn][Tt]"
#token CHANNEL      "[Cc][Hh][Aa][Nn][Nn][Ee][Ll]"
#token SEGMENT      "[Ss][Ee][Gg][Mm][Ee][Nn][Tt]"
#token RELATIVE     "[Rr][Ee][Ll][Aa][Tt][Ii][Vv][Ee]"

#token GROUTE       "[Gg]{[Ll][Oo][Bb][Aa][Ll][_]}[Rr][Oo][Uu][Tt][Ee]"
#token ROUTE        "[Rr][Oo][Uu][Tt][Ee]"
#token SWITCHBOX    "[Ss][Ww][Ii][Tt][Cc][Hh][Bb][Oo][Xx]"

#token PLACE_FLOAT  "[Ff][Ll][Oo][Aa][Tt]"
#token PLACE_FIXED  "[Ff][Ii][Xx][Ee][Dd]"
#token PLACE_PLACED "[Pp][Ll][Aa][Cc][Ee][Dd]"
#tokclass PLACE_STATUS_VAL { PLACE_FLOAT PLACE_FIXED PLACE_PLACED }

#token NET_OPEN     "[Oo][Pp][Ee][Nn]"
#token NET_GROUTED  "[Gg]{[Ll][Oo][Bb][Aa][Ll][_]}[Rr][Oo][Uu][Tt][Ee][Dd]"
#token NET_ROUTED   "[Rr][Oo][Uu][Tt][Ee][Dd]"
#tokclass NET_STATUS_VAL { NET_OPEN NET_GROUTED NET_ROUTED }

#token TYPE_FALLING_EDGE "[Ff][Ee]"
#token TYPE_RISING_EDGE  "[Rr][Ee]"
#token TYPE_ACTIVE_HIGH  "[Aa][Hh]"
#token TYPE_ACTIVE_LOW   "[Aa][Ll]"
#token TYPE_ASYNCHRONOUS "[Aa][Ss]"
#tokclass LATCH_TYPE_VAL  { TYPE_FALLING_EDGE TYPE_RISING_EDGE TYPE_ACTIVE_HIGH TYPE_ACTIVE_LOW TYPE_ASYNCHRONOUS }

#token STATE_TRUE        "1"
#token STATE_FALSE       "0"
#token STATE_DONT_CARE   "2"
#token STATE_UNKNOWN     "3"
#tokclass LATCH_STATE_VAL { STATE_TRUE STATE_FALSE STATE_DONT_CARE STATE_UNKNOWN }

#token LEFT         "[Ll]{[Ee][Ff][Tt]}"
#token RIGHT        "[Rr]{[Ii][Gg][Hh][Tt]}"
#token LOWER        "[Ll][Oo][Ww][Ee][Rr]"
#token UPPER        "[Uu][Pp][Pp][Ee][Rr]"
#token BOTTOM       "[Bb]{[Oo][Tt][Tt][Oo][Mm]}"
#token TOP          "[Tt]{[Oo][Pp]}"
#tokclass SIDE_VAL  { LEFT RIGHT LOWER UPPER BOTTOM TOP R T }

#token INPUT        "[Ii][Nn][Pp][Uu][Tt]"
#token OUTPUT       "[Oo][Uu][Tt][Pp][Uu][Tt]"
#token SIGNAL       "[Ss][Ii][Gg][Nn][Aa][Ll]"
#token CLOCK        "[Cc][Ll][Oo][Cc][Kk]"
#token POWER        "[Pp][Oo][Ww][Ee][Rr]"
#token GLOBAL       "[Gg][Ll][Oo][Bb][Aa][Ll]"
#tokclass TYPE_VAL  { INPUT OUTPUT SIGNAL CLOCK POWER GLOBAL }

#token BOOL_TRUE    "([Tt][Rr][Uu][Ee]|[Yy][Ee][Ss]|[Oo][Nn])"
#token BOOL_FALSE   "([Ff][Aa][Ll][Ss][Ee]|[Nn][Oo]|[Oo][Ff][Ff])"
#tokclass BOOL_VAL  { BOOL_TRUE BOOL_FALSE }

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

class TCP_CircuitParser_c
{
<<
public:

   void syn( ANTLRAbstractToken* /* pToken */, 
             ANTLRChar*          pszGroup,
             SetWordType*        /* pWordType */,
             ANTLRTokenType      tokenType,
             int                 /* k */ );
            
   void SetScanner( TCP_CircuitScanner_c* pscanner );
   void SetFileName( const char* pszFileName );
   void SetCircuitFile( TCP_CircuitFile_c* pcircuitFile );
   void SetCircuitDesign( TCD_CircuitDesign_c* pcircuitDesign );
>> 
<<
private:

   TPO_StatusMode_t FindPlaceStatusMode_( ANTLRTokenType tokenType );
   TNO_StatusMode_t FindNetStatusMode_( ANTLRTokenType tokenType );
   TPO_LatchType_t FindLatchType_( ANTLRTokenType tokenType );
   TPO_LatchState_t FindLatchState_( ANTLRTokenType tokenType );
   TC_SideMode_t FindSideMode_( ANTLRTokenType tokenType );
   TC_TypeMode_t FindTypeMode_( ANTLRTokenType tokenType );
   bool FindBool_( ANTLRTokenType tokenType );
>> 
<<
private:

   TCP_CircuitScanner_c*   pscanner_;
   string                  srFileName_;
   TCP_CircuitFile_c*      pcircuitFile_;
   TCD_CircuitDesign_c*    pcircuitDesign_;
>>

//===========================================================================//
start 
   :
   "<" CIRCUIT stringText[ &pcircuitDesign_->srName ] ">"
   (  "<"
      (  blockList[ &pcircuitDesign_->blockList ] 
      |  portList[ &pcircuitDesign_->portList ]
      |  instList[ &pcircuitDesign_->instList ] 
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
      TPO_HierMapList_t hierMapList_;
      TPO_RelativeList_t relativeList_;
      TGS_RegionList_t regionList_;
   >>
   ( BLOCK | PB )
   stringText[ &srName ]
   <<
      inst.SetName( srName );
   >>
   stringText[ &srCellName ]
   <<
      inst.SetCellName( srCellName );
   >>
   (  STATUS { EQUAL } statusVal:PLACE_STATUS_VAL
      <<
         inst.SetPlaceStatus( this->FindPlaceStatusMode_( statusVal->getType( )));
      >>
   )*
   ">"
   (  "<" 
      (  PACK hierMapList[ &hierMapList_ ] "</" PACK ">"
      |  PLACE stringText[ &srPlaceFabricName ]
         <<
            inst.SetPlaceFabricName( srPlaceFabricName );
         >>
         "/>"
      |  RELATIVE relativeList[ &relativeList_ ] "/>"
      |  REGION regionList[ &regionList_ ] "/>"
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
portList[ TPO_PortList_t* pportList ]
   :
   <<
      TPO_Inst_c port;
      TPO_Pin_t pin;
      TPO_InstSource_t source;

      string srName;
      string srCellName;
      string srPlaceFabricName;
   >>
   ( PORT | IO )
   stringText[ &srName ]
   <<
      port.SetName( srName );
   >>
   cellSourceText[ &srCellName, &source ]
   <<
      port.SetCellName( srCellName );
      port.SetSource( source );
   >>
   (  STATUS { EQUAL } statusVal:PLACE_STATUS_VAL
      <<
         port.SetPlaceStatus( this->FindPlaceStatusMode_( statusVal->getType( )));
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
      }
   >>
   ;

//===========================================================================//
instList[ TPO_InstList_t* pinstList ]
   :
   <<
      TPO_Inst_c inst;
      TPO_Pin_t pin;
      TPO_InstSource_t source;

      string srName;
      string srCellName;
   >>
   INST
   stringText[ &srName ]
   <<
      inst.SetName( srName );
   >>
   cellSourceText[ &srCellName, &source ]
   <<
      inst.SetCellName( srCellName );
      inst.SetSource( source );
   >>
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
      }
   >>
   ;

//===========================================================================//
netList[ TNO_NetList_c* pnetList ]
   :
   <<
      TNO_Net_c net;

      string srName;
      TNO_InstPinList_t instPinList_;
      TNO_GlobalRouteList_t globalRouteList_;
      TNO_RouteList_t routeList_;
   >>
   NET 
   stringText[ &srName ]
   <<
      net.SetName( srName );
   >>
   (  TYPE { EQUAL } typeVal:TYPE_VAL
      << 
         net.SetType( this->FindTypeMode_( typeVal->getType( )));
      >>
   |  ROUTABLE { EQUAL } boolVal:BOOL_VAL
      << 
         net.SetRoutable( this->FindBool_( boolVal->getType( )));
      >>
   |  STATUS { EQUAL } statusVal:NET_STATUS_VAL
      <<
         net.SetStatus( this->FindNetStatusMode_( statusVal->getType( )));
      >>
   )*
   ">"
   (  "<"
      (  PIN instPinList[ &instPinList_ ] "/>"
      |  GROUTE globalRouteList[ &globalRouteList_ ] "/>"
      |  ROUTE routeList[ &routeList_ ] "</" ROUTE ">"
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
   (  TYPE { EQUAL } typeVal:LATCH_TYPE_VAL
      << 
         clockType = this->FindLatchType_( typeVal->getType( ));
         pinst->SetLatchClockType( clockType );
      >>
   |  STATE { EQUAL } stateVal:LATCH_STATE_VAL
      << 
         initState = this->FindLatchState_( stateVal->getType( ));
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
   stringText[ &srName ]
   typeVal:TYPE_VAL
   <<
   >>
   << 
      type = this->FindTypeMode_( typeVal->getType( ));
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

      pinstPin->Clear( );
   >>
   stringText[ &srInstName ]
   stringText[ &srPortName ]
   {  stringText[ &srPinName ]  }
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
   {  typeVal:TYPE_VAL 
      << 
         pinstPin->SetType( this->FindTypeMode_( typeVal->getType( )));
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
   stringText[ &srName ]
   uintNum[ &length ]
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
      (  PIN instPinDef[ &instPin ]
         <<
            node.Clear( );
            node.Set( instPin );
         >>
      |  SEGMENT segmentDef[ &segment ]
         <<
            node.Clear( );
            node.Set( segment );
         >>
      |  SB switchBoxDef[ &switchBox ]
         <<
            node.Clear( );
            node.Set( switchBox );
         >>
      )
      "/>"
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
   stringText[ &srName ]
   sideIndex[ &sideIndex_ ]
   sideIndex[ &sideIndex_ ]
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
   stringText[ &srName ]
   regionDef[ &channel ]
   uintNum[ &track ]
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
      TPO_HierNameList_t hierNameList;
   >>
   stringText[ &srName ]
   << 
      hierMap.SetInstName( srName );
   >>
   (  ">" 
      "<" HIER 
      (
         stringText[ &srName ]
         << 
            hierNameList.Add( srName );
         >>
      )*
      "/>"
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
   sideVal:SIDE_VAL
   << 
      side = this->FindSideMode_( sideVal->getType( ));
   >>
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
   |  latchStateVal:LATCH_STATE_VAL
      <<
         *pdouble = atof( latchStateVal->getText( ));
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
   |  latchStateVal:LATCH_STATE_VAL
      <<
         *pdouble = atof( latchStateVal->getText( ));
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
   |  latchStateVal:LATCH_STATE_VAL
      <<
         *puint = static_cast< unsigned int >( atol( latchStateVal->getText( )));
      >>
   ;

//===========================================================================//

}

<<
#include "TCP_CircuitGrammar.h"
>>
