//===========================================================================//
// Purpose: PCCTS grammar for the circuit design (BLIF) file.
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

#include "TIO_PrintHandler.h"

#include "TCD_CircuitDesign.h"

#include "TCBP_CircuitBlifFile.h"
#include "TCBP_CircuitBlifScanner_c.h"
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

#token MODEL        "[.][Mm][Oo][Dd][Ee][Ll]"
#token INPUTS       "[.][Ii][Nn][Pp][Uu][Tt]{[Ss]}"
#token OUTPUTS      "[.][Oo][Uu][Tt][Pp][Uu][Tt]{[Ss]}"
#token CLOCKS       "[.][Cc][Ll][Oo][Cc][Kk]{[Ss]}"
#token NAMES        "[.][Nn][Aa][Mm][Ee]{[Ss]}"
#token LATCH        "[.][Ll][Aa][Tt][Cc][Hh]"
#token SUBCKT       "[.][Ss][Uu][Bb][Cc][Kk][Tt]"
#token END          "[.][Ee][Nn][Dd]"
#token IGNORED      "[.][A-Za-z]*"

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

#token BIT_STRING   "[01\-]+"
#token BIT_PAIR     "[01][\ \t]+[01]"
#token STRING       "[a-zA-Z_/\|\[\]][a-zA-Z0-9_/\|\[\]\(\)\.\+\~\^]*"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//

class TCBP_CircuitBlifParser_c
{
<<
public:

   void syn( ANTLRAbstractToken* /* pToken */, 
             ANTLRChar*          pszGroup,
             SetWordType*        /* pWordType */,
             ANTLRTokenType      tokenType,
             int                 /* k */ );

   void SetScanner( TCBP_CircuitBlifScanner_c* pscanner );
   void SetFileName( const char* pszFileName );
   void SetCircuitBlifFile( TCBP_CircuitBlifFile_c* pcircuitBlifFile );
   void SetCircuitDesign( TCD_CircuitDesign_c* pcircuitDesign );
>> 
<<
private:

   TPO_LatchType_t FindLatchType_( ANTLRTokenType tokenType );
   TPO_LatchState_t FindLatchState_( ANTLRTokenType tokenType );
>> 
<<
private:

   TCBP_CircuitBlifScanner_c* pscanner_;
   string                     srFileName_;
   TCBP_CircuitBlifFile_c*    pcircuitBlifFile_;
   TCD_CircuitDesign_c*       pcircuitDesign_;

   size_t circuitSubcktCount_;
>>

//===========================================================================//
start 
   : 
   topModel[ &pcircuitDesign_->srName,
             &pcircuitDesign_->portList,
             &pcircuitDesign_->portNameList,
             &pcircuitDesign_->instList,
             &pcircuitDesign_->instNameList ]
   (  cellModel[ &pcircuitDesign_->cellList ] )*
   END_OF_FILE
   ;

//===========================================================================//
topModel[ string* psrName,
          TPO_PortList_t* pportList,
          TPO_NameList_t* pportNameList,
          TPO_InstList_t* pinstList,
          TPO_NameList_t* pinstNameList ]
   : 
   MODEL stringText[ psrName ]
   (  INPUTS pinList[ pportList, pportNameList, TC_TYPE_OUTPUT ]
   |  OUTPUTS pinList[ pportList, pportNameList, TC_TYPE_INPUT ]
   |  CLOCKS pinList[ pportList, pportNameList, TC_TYPE_CLOCK ]
   |  NAMES namesElem[ pinstList, pinstNameList ]
   |  LATCH latchElem[ pinstList, pinstNameList ]
   |  SUBCKT subcktElem[ pinstList, pinstNameList ]
   )*
   END
   ;

//===========================================================================//
cellModel[ TLO_CellList_t* pcellList ]
   : 
   <<
      string srCellName_;
      TLO_PortList_t portList_;
      TLO_Cell_c cell_;
   >>
   MODEL stringText[ &srCellName_ ]
   (  INPUTS portList[ &portList_, TC_TYPE_INPUT ]
   |  OUTPUTS portList[ &portList_, TC_TYPE_OUTPUT ]
   |  IGNORED ( STRING )*
   )*
   <<
      cell_.SetName( srCellName_ );
      cell_.SetSource( TLO_CELL_SOURCE_CUSTOM );
      cell_.SetPortList( portList_ );
      pcellList->Add( cell_ );
   >>
   END
   ;

//===========================================================================//
pinList[ TPO_PortList_t* pportList, 
         TPO_NameList_t* pportNameList,
         TC_TypeMode_t type ]
   : 
   <<
      string srPinName;
   >>
   (  stringText[ &srPinName ]
      <<
         TPO_Port_t port( srPinName );
         port.SetInputOutputType( type );
         TPO_Pin_t pin( srPinName, type );
         port.AddPin( pin );
         pportList->Add( port );
         pportNameList->Add( port.GetName( ));
      >>
   )+
   ;

//===========================================================================//
portList[ TLO_PortList_t* pportList, 
          TC_TypeMode_t type ]
   : 
   <<
      string srPortName;
   >>
   (  stringText[ &srPortName ]
      <<
         TLO_Port_c port( srPortName, type );
         pportList->Add( port );
      >>
   )+
   ;

//===========================================================================//
namesElem[ TPO_InstList_t* pinstList,
           TPO_NameList_t* pinstNameList ]
   : 
   <<
      string srPinName;
      string srPeekName;

      TPO_Pin_t outputPin;
      TPO_Pin_t inputPin;
      TPO_PinList_t pinList;

      TPO_LogicBitsList_t logicBitsList;

      TPO_Inst_c inst;
   >>
   stringText[ &srPinName ]
   (
      stringText[ &srPeekName ]
      <<
         inputPin.Set( srPinName, TC_TYPE_INPUT );
         pinList.Add( inputPin );

         srPinName = srPeekName;
      >>
   )*
   <<
      outputPin.Set( srPinName, TC_TYPE_OUTPUT );
      pinList.Add( outputPin );
   >>
   logicBitsTable[ &logicBitsList ]
   <<
      inst.SetName( outputPin.GetName( ));
      inst.SetPinList( pinList );
      inst.SetNamesLogicBitsList( logicBitsList );
      pinstList->Add( inst );
      pinstNameList->Add( inst.GetName( ));
   >>
   ;

//===========================================================================//
latchElem[ TPO_InstList_t* pinstList,
           TPO_NameList_t* pinstNameList ]
   : 
   <<
      string srInputName;
      string srOutputName;
      string srClockName;

      TPO_PinList_t pinList;

      TPO_LatchType_t clockType = TPO_LATCH_TYPE_UNDEFINED;
      TPO_LatchState_t initState = TPO_LATCH_STATE_UNDEFINED;

      TPO_Inst_c inst;
   >>
   stringText[ &srInputName ]
   <<
      TPO_Pin_t inputPin( srInputName, TC_TYPE_INPUT );
      pinList.Add( inputPin );
   >>
   stringText[ &srOutputName ]
   <<
      TPO_Pin_t outputPin( srOutputName, TC_TYPE_OUTPUT );
      pinList.Add( outputPin );
   >>
   latchType[ &clockType ]
   stringText[ &srClockName ]
   <<
      TPO_Pin_t clockPin( srClockName, TC_TYPE_CLOCK );
      pinList.Add( clockPin );
   >>
   {  latchState[ &initState ]
   }
   <<
      inst.SetName( srOutputName );
      inst.SetPinList( pinList );
      inst.SetLatchClockType( clockType );
      inst.SetLatchInitState( initState );
      pinstList->Add( inst );
      pinstNameList->Add( inst.GetName( ));
   >>
   ;

//===========================================================================//
subcktElem[ TPO_InstList_t* pinstList,
            TPO_NameList_t* pinstNameList ]
   : 
   <<
      string srInstName;
      string srCellName;

      string srCellPinName;
      string srInstPinName;
    
      TPO_PinList_t pinList;
      TPO_PinMapList_t pinMapList;

      char szInstIndex[ TIO_FORMAT_STRING_LEN_VALUE ];
      TPO_Inst_c inst;
   >>
   stringText[ &srCellName ]
   ( 
      stringText[ &srCellPinName ]
      EQUAL
      stringText[ &srInstPinName ]
      <<
         TPO_Pin_t pin( srInstPinName );
         pinList.Add( pin );

         TPO_PinMap_c pinMap( srInstPinName, srCellPinName );
         pinMapList.Add( pinMap );
      >>
   )+
   <<
      sprintf( szInstIndex, "%u", this->circuitSubcktCount_++ );

      srInstName = srCellName;
      srInstName += "(";
      srInstName += szInstIndex;
      srInstName += ")";

      inst.SetName( srInstName );
      inst.SetPinList( pinList );
      inst.SetCellName( srCellName );
      inst.SetSubcktPinMapList( pinMapList );
      pinstList->Add( inst );
      pinstNameList->Add( inst.GetName( ));
   >>
   ;

//===========================================================================//
logicBitsTable[ TPO_LogicBitsList_t* plogicBitsList ]
   : 
   ( 
      <<
         TPO_LogicBits_t logicBits;
      >>
      logicBitsElem[ &logicBits ]
      <<
         if( logicBits.IsValid( ))
         {
            plogicBitsList->Add( logicBits );
         }
      >>
   )*
   ;

//===========================================================================//
logicBitsElem[ TPO_LogicBits_t* plogicBits ]
   : 
   <<
      string srInputBits, srOutputBit, srInputOutputBits;
      TC_Bit_c inputBit, outputBit;

      plogicBits->Clear( );
   >>
   (  outputTrueVal:STATE_TRUE 
      <<
         srInputBits = "";
         srOutputBit = outputTrueVal->getText( );
      >>
   |  outputFalseVal:STATE_FALSE 
      <<
         srInputBits = "";
         srOutputBit = outputFalseVal->getText( );
      >>
   |  inputOutputStringVal:BIT_PAIR
      <<
         srInputOutputBits = inputOutputStringVal->getText( );
         srInputBits = srInputOutputBits.at( 0 );
         srOutputBit = srInputOutputBits.at( srInputOutputBits.length( )-1 );
      >>
   |  inputBitStringVal:BIT_STRING 
      <<
         srInputBits = inputBitStringVal->getText( );
      >>
      (  outputBitTrueVal:STATE_TRUE 
         <<
            srOutputBit = outputBitTrueVal->getText( );
         >>
      |  outputBitFalseVal:STATE_FALSE 
         <<
            srOutputBit = outputBitFalseVal->getText( );
         >>
      ) 
   )
   <<
      for( size_t i = 0; i < srInputBits.length( ); ++i )
      {
         inputBit.SetValue( srInputBits[ i ] );
         plogicBits->Add( inputBit );
      }
      outputBit.SetValue( srOutputBit[ 0 ] );
      plogicBits->Add( outputBit );
   >>
   ;

//===========================================================================//
latchType[ TPO_LatchType_t* ptype ]
   : 
   typeVal:LATCH_TYPE_VAL
   << 
      *ptype = this->FindLatchType_( typeVal->getType( ));
   >>
   ;

//===========================================================================//
latchState[ TPO_LatchState_t* pstate ]
   : 
   stateVal:LATCH_STATE_VAL
   << 
      *pstate = this->FindLatchState_( stateVal->getType( ));
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
      } CLOSE_QUOTE
   |  stringVal:STRING
      <<
         *psrString = stringVal->getText( );
      >>
   ;

//===========================================================================//

}

<<
#include "TCBP_CircuitBlifGrammar.h"
>>
