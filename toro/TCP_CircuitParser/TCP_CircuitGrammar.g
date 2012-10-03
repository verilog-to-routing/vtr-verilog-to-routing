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
#token PORT         "[Pp][Oo][Rr][Tt]"
#token INST         "[Ii][Nn][Ss][Tt]"
#token CELL         "[Cc][Ee][Ll][Ll]"
#token MASTER       "[Mm][Aa][Ss][Tt][Ee][Rr]"
#token NET          "[Nn][Ee][Tt]"
#token PIN          "[Pp][Ii][Nn]"

#token TYPE         "[Tt][Yy][Pp][Ee]"
#token ROUTABLE     "[Rr][Oo][Uu][Tt][Aa][Bb][Ll][Ee]"
#token STATUS       "[Ss][Tt][Aa][Tt][Uu][Ss]"
#token PACK         "[Pp][Aa][Cc][Kk]{[Ii][Nn][Gg]}"
#token PLACE        "[Pp][Ll][Aa][Cc][Ee][Mm][Ee][Nn][Tt]"
#token CHANNEL      "[Cc][Hh][Aa][Nn][Nn][Ee][Ll]"
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
   PB
   stringText[ &srName ]
   <<
      inst.SetName( srName );
   >>
   (  ( PB | MASTER ) { EQUAL } stringText[ &srCellName ]
      <<
         inst.SetCellName( srCellName );
      >>
   |  STATUS { EQUAL } statusVal:PLACE_STATUS_VAL
      <<
         inst.SetPlaceStatus( this->FindPlaceStatusMode_( statusVal->getType( )));
      >>
   )*
   ">"
   (  "<" 
      (  PACK hierMapList[ &hierMapList_ ]
      |  PLACE stringText[ &srPlaceFabricName ]
         <<
            inst.SetPlaceFabricName( srPlaceFabricName );
         >>
      |  RELATIVE relativeList[ &relativeList_ ]
      |  REGION regionList[ &regionList_ ]
      )
      ">"
   )*
   "</" PB ">"
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
   (  ( CELL | MASTER ) { EQUAL } cellSourceText[ &srCellName,
                                                  &source ]
      <<
         port.SetCellName( srCellName );
         port.SetSource( source );
      >>
   |  STATUS { EQUAL } statusVal:PLACE_STATUS_VAL
      <<
         port.SetPlaceStatus( this->FindPlaceStatusMode_( statusVal->getType( )));
      >>
   )*
   ">"
   (  "<" 
      (  PIN pinDef[ &pin ] ">"
         <<
            port.AddPin( pin);
         >>
      |  PLACE stringText[ &srPlaceFabricName ] "/>"
         <<
            port.SetPlaceFabricName( srPlaceFabricName );
         >>
      )
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
   (  ( CELL | MASTER ) { EQUAL } cellSourceText[ &srCellName,
                                                  &source ]
      <<
         inst.SetCellName( srCellName );
         inst.SetSource( source );
     >>
   )*
   ">"
   (  "<"
      PIN pinDef[ &pin ]
      <<
         inst.AddPin( pin );
      >>
      ">"
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
      (  PIN instPinList[ &instPinList_ ] ">"
      |  GROUTE globalRouteList[ &globalRouteList_ ] ">"
      |  ROUTE routeList[ &routeList_ ] "/>"
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
      string srInstName, srPinName;

      pinstPin->Clear( );
   >>
   stringText[ &srInstName ]
   stringText[ &srPinName ]
   <<
      pinstPin->Set( srInstName, srPinName );
   >>
   typeVal:TYPE_VAL
   << 
      pinstPin->SetType( this->FindTypeMode_( typeVal->getType( )));
   >>
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
      TNO_Channel_t channel;
      TNO_SwitchBox_c switchBox;
   >>
   (  "<"
      (  PIN instPinDef[ &instPin ]
         <<
            node.Clear( );
            node.Set( instPin );
         >>
      |  CHANNEL channelDef[ &channel ]
         <<
            node.Clear( );
            node.Set( channel );
         >>
      |  SB switchBoxDef[ &switchBox ]
         <<
            node.Clear( );
            node.Set( switchBox );
         >>
      )
      ">"
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
   <<
      pswitchBox->SetName( srName );
   >>
   sideIndex[ &sideIndex_ ]
   <<
      pswitchBox->SetInput( sideIndex_ );
   >>
   sideIndex[ &sideIndex_ ]
   <<
      pswitchBox->SetOutput( sideIndex_ );
   >>
   ;

//===========================================================================//
channelDef[ TNO_Channel_t* pchannel ]
   :
   <<
      string srName;
      unsigned int index = 0;

      pchannel->Clear( );
   >>
   stringText[ &srName ]
   uintNum[ &index ]
   << 
      pchannel->Set( srName, index );
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
   (  "<" HIER 
      (
         stringText[ &srName ]
         << 
            hierNameList.Add( srName );
         >>
      )*
      ">"
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
#include "TCP_CircuitGrammar.h"
>>
