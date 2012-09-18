/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 *
 *   ..\..\pccts\bin\Antlr.exe -CC -gh TCBP_CircuitBlifGrammar.g
 *
 */

#define ANTLR_VERSION	13333
#include "pcctscfg.h"
#include "pccts_stdio.h"
#include "tokens.h"

#include <stdio.h>

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TIO_PrintHandler.h"

#include "TCD_CircuitDesign.h"

#include "TCBP_CircuitBlifFile.h"
#include "TCBP_CircuitBlifScanner_c.h"
#include "AParser.h"
#include "TCBP_CircuitBlifParser_c.h"
#include "DLexerBase.h"
#include "ATokPtr.h"

/* MR23 In order to remove calls to PURIFY use the antlr -nopurify option */

#ifndef PCCTS_PURIFY
#define PCCTS_PURIFY(r,s) memset((char *) &(r),'\0',(s));
#endif


void
TCBP_CircuitBlifParser_c::start(void)
{
  zzRULE;
  topModel(  &pcircuitDesign_->srName,
             &pcircuitDesign_->instList,
             &pcircuitDesign_->portList  );
  {
    while ( (LA(1)==MODEL) ) {
      cellModel(  &pcircuitDesign_->cellList  );
    }
  }
  zzmatch(END_OF_FILE); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x1);
}

void
TCBP_CircuitBlifParser_c::topModel(string* psrName,TPO_InstList_t* pinstList,TPO_PortList_t* pportList)
{
  zzRULE;
  zzmatch(MODEL); consume();
  stringText(  psrName  );
  {
    for (;;) {
      if ( !((setwd1[LA(1)]&0x2))) break;
      if ( (LA(1)==INPUTS) ) {
        zzmatch(INPUTS); consume();
        pinList(  pportList, TC_TYPE_OUTPUT  );
      }
      else {
        if ( (LA(1)==OUTPUTS) ) {
          zzmatch(OUTPUTS); consume();
          pinList(  pportList, TC_TYPE_INPUT  );
        }
        else {
          if ( (LA(1)==CLOCKS) ) {
            zzmatch(CLOCKS); consume();
            pinList(  pportList, TC_TYPE_CLOCK  );
          }
          else {
            if ( (LA(1)==NAMES) ) {
              zzmatch(NAMES); consume();
              namesElem(  pinstList  );
            }
            else {
              if ( (LA(1)==LATCH) ) {
                zzmatch(LATCH); consume();
                latchElem(  pinstList  );
              }
              else {
                if ( (LA(1)==SUBCKT) ) {
                  zzmatch(SUBCKT); consume();
                  subcktElem(  pinstList  );
                }
                else break; /* MR6 code for exiting loop "for sure" */
              }
            }
          }
        }
      }
    }
  }
  zzmatch(END); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x4);
}

void
TCBP_CircuitBlifParser_c::cellModel(TLO_CellList_t* pcellList)
{
  zzRULE;
  
  string srCellName_;
  TLO_PortList_t portList_;
  TLO_Cell_c cell_;
  zzmatch(MODEL); consume();
  stringText(  &srCellName_  );
  {
    for (;;) {
      if ( !((setwd1[LA(1)]&0x8))) break;
      if ( (LA(1)==INPUTS) ) {
        zzmatch(INPUTS); consume();
        portList(  &portList_, TC_TYPE_INPUT  );
      }
      else {
        if ( (LA(1)==OUTPUTS) ) {
          zzmatch(OUTPUTS); consume();
          portList(  &portList_, TC_TYPE_OUTPUT  );
        }
        else {
          if ( (LA(1)==IGNORED) ) {
            zzmatch(IGNORED); consume();
            {
              while ( (LA(1)==STRING) ) {
                zzmatch(STRING); consume();
              }
            }
          }
          else break; /* MR6 code for exiting loop "for sure" */
        }
      }
    }
  }
  
  cell_.SetName( srCellName_ );
  cell_.SetSource( TLO_CELL_SOURCE_CUSTOM );
  cell_.SetPortList( portList_ );
  pcellList->Add( cell_ );
  zzmatch(END); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x10);
}

void
TCBP_CircuitBlifParser_c::pinList(TPO_PortList_t* pportList,TC_TypeMode_t type)
{
  zzRULE;
  
  string srPinName;
  {
    int zzcnt=1;
    do {
      stringText(  &srPinName  );
      
      TPO_Port_t port( srPinName );
      port.SetInputOutputType( type );
      TPO_Pin_t pin( srPinName, type );
      port.AddPin( pin );
      pportList->Add( port );
    } while ( (setwd1[LA(1)]&0x20) );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x40);
}

void
TCBP_CircuitBlifParser_c::portList(TLO_PortList_t* pportList,TC_TypeMode_t type)
{
  zzRULE;
  
  string srPortName;
  {
    int zzcnt=1;
    do {
      stringText(  &srPortName  );
      
      TLO_Port_c port( srPortName, type );
      pportList->Add( port );
    } while ( (setwd1[LA(1)]&0x80) );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x1);
}

void
TCBP_CircuitBlifParser_c::namesElem(TPO_InstList_t* pinstList)
{
  zzRULE;
  
  string srPinName;
  string srPeekName;
  
      TPO_Pin_t outputPin;
  TPO_Pin_t inputPin;
  TPO_PinList_t pinList;
  
      TPO_LogicBitsList_t logicBitsList;
  
      TPO_Inst_c inst;
  stringText(  &srPinName  );
  {
    while ( (setwd2[LA(1)]&0x2) ) {
      stringText(  &srPeekName  );
      
      inputPin.Set( srPinName, TC_TYPE_INPUT );
      pinList.Add( inputPin );
      
         srPinName = srPeekName;
    }
  }
  
  outputPin.Set( srPinName, TC_TYPE_OUTPUT );
  pinList.Add( outputPin );
  logicBitsTable(  &logicBitsList  );
  
  inst.SetName( outputPin.GetName( ));
  inst.SetPinList( pinList );
  inst.SetNamesLogicBitsList( logicBitsList );
  pinstList->Add( inst );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x4);
}

void
TCBP_CircuitBlifParser_c::latchElem(TPO_InstList_t* pinstList)
{
  zzRULE;
  
  string srInputName;
  string srOutputName;
  string srClockName;
  
      TPO_PinList_t pinList;
  
      TPO_LatchType_t clockType = TPO_LATCH_TYPE_UNDEFINED;
  TPO_LatchState_t initState = TPO_LATCH_STATE_UNDEFINED;
  
      TPO_Inst_c inst;
  stringText(  &srInputName  );
  
  TPO_Pin_t inputPin( srInputName, TC_TYPE_INPUT );
  pinList.Add( inputPin );
  stringText(  &srOutputName  );
  
  TPO_Pin_t outputPin( srOutputName, TC_TYPE_OUTPUT );
  pinList.Add( outputPin );
  latchType(  &clockType  );
  stringText(  &srClockName  );
  
  TPO_Pin_t clockPin( srClockName, TC_TYPE_CLOCK );
  pinList.Add( clockPin );
  {
    if ( (setwd2[LA(1)]&0x8) ) {
      latchState(  &initState  );
    }
    else {
      if ( (setwd2[LA(1)]&0x10) ) {
      }
      else {FAIL(1,err1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  
  inst.SetName( srOutputName );
  inst.SetPinList( pinList );
  inst.SetLatchClockType( clockType );
  inst.SetLatchInitState( initState );
  pinstList->Add( inst );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x20);
}

void
TCBP_CircuitBlifParser_c::subcktElem(TPO_InstList_t* pinstList)
{
  zzRULE;
  
  string srInstName;
  string srCellName;
  
      string srCellPinName;
  string srInstPinName;
  
      TPO_PinList_t pinList;
  TPO_PinMapList_t pinMapList;
  
      char szInstIndex[ TIO_FORMAT_STRING_LEN_VALUE ];
  TPO_Inst_c inst;
  stringText(  &srCellName  );
  {
    int zzcnt=1;
    do {
      stringText(  &srCellPinName  );
      zzmatch(EQUAL); consume();
      stringText(  &srInstPinName  );
      
      TPO_Pin_t pin( srInstPinName );
      pinList.Add( pin );
      
         TPO_PinMap_c pinMap( srInstPinName, srCellPinName );
      pinMapList.Add( pinMap );
    } while ( (setwd2[LA(1)]&0x40) );
  }
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x80);
}

void
TCBP_CircuitBlifParser_c::logicBitsTable(TPO_LogicBitsList_t* plogicBitsList)
{
  zzRULE;
  {
    
    TPO_LogicBits_t logicBits;
    while ( (setwd3[LA(1)]&0x1) ) {
      logicBitsElem(  &logicBits  );
      
      if( logicBits.IsValid( ))
      {
        plogicBitsList->Add( logicBits );
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x2);
}

void
TCBP_CircuitBlifParser_c::logicBitsElem(TPO_LogicBits_t* plogicBits)
{
  zzRULE;
  ANTLRTokenPtr outputTrueVal=NULL, outputFalseVal=NULL, inputOutputStringVal=NULL, inputBitStringVal=NULL, outputBitTrueVal=NULL, outputBitFalseVal=NULL;
  
  string srInputBits, srOutputBit, srInputOutputBits;
  TC_Bit_c inputBit, outputBit;
  
      plogicBits->Clear( );
  {
    if ( (LA(1)==STATE_TRUE) ) {
      zzmatch(STATE_TRUE);
      outputTrueVal = (ANTLRTokenPtr)LT(1);

      
      srInputBits = "";
      srOutputBit = outputTrueVal->getText( );
 consume();
    }
    else {
      if ( (LA(1)==STATE_FALSE) ) {
        zzmatch(STATE_FALSE);
        outputFalseVal = (ANTLRTokenPtr)LT(1);

        
        srInputBits = "";
        srOutputBit = outputFalseVal->getText( );
 consume();
      }
      else {
        if ( (LA(1)==BIT_PAIR) ) {
          zzmatch(BIT_PAIR);
          inputOutputStringVal = (ANTLRTokenPtr)LT(1);

          
          srInputOutputBits = inputOutputStringVal->getText( );
          srInputBits = srInputOutputBits.at( 0 );
          srOutputBit = srInputOutputBits.at( srInputOutputBits.length( )-1 );
 consume();
        }
        else {
          if ( (LA(1)==BIT_STRING) ) {
            zzmatch(BIT_STRING);
            inputBitStringVal = (ANTLRTokenPtr)LT(1);

            
            srInputBits = inputBitStringVal->getText( );
 consume();
            {
              if ( (LA(1)==STATE_TRUE) ) {
                zzmatch(STATE_TRUE);
                outputBitTrueVal = (ANTLRTokenPtr)LT(1);

                
                srOutputBit = outputBitTrueVal->getText( );
 consume();
              }
              else {
                if ( (LA(1)==STATE_FALSE) ) {
                  zzmatch(STATE_FALSE);
                  outputBitFalseVal = (ANTLRTokenPtr)LT(1);

                  
                  srOutputBit = outputBitFalseVal->getText( );
 consume();
                }
                else {FAIL(1,err2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
          else {FAIL(1,err3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  
  for( size_t i = 0; i < srInputBits.length( ); ++i )
  {
    inputBit.SetValue( srInputBits[ i ] );
    plogicBits->Add( inputBit );
  }
  outputBit.SetValue( srOutputBit[ 0 ] );
  plogicBits->Add( outputBit );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x4);
}

void
TCBP_CircuitBlifParser_c::latchType(TPO_LatchType_t* ptype)
{
  zzRULE;
  ANTLRTokenPtr typeVal=NULL;
  zzsetmatch(LATCH_TYPE_VAL_set, LATCH_TYPE_VAL_errset);
  typeVal = (ANTLRTokenPtr)LT(1);

  
  *ptype = this->FindLatchType_( typeVal->getType( ));
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x8);
}

void
TCBP_CircuitBlifParser_c::latchState(TPO_LatchState_t* pstate)
{
  zzRULE;
  ANTLRTokenPtr stateVal=NULL;
  zzsetmatch(LATCH_STATE_VAL_set, LATCH_STATE_VAL_errset);
  stateVal = (ANTLRTokenPtr)LT(1);

  
  *pstate = this->FindLatchState_( stateVal->getType( ));
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x10);
}

void
TCBP_CircuitBlifParser_c::stringText(string* psrString)
{
  zzRULE;
  ANTLRTokenPtr qstringVal=NULL, stringVal=NULL;
  
  *psrString = "";
  if ( (LA(1)==OPEN_QUOTE) ) {
    zzmatch(OPEN_QUOTE); consume();
    {
      if ( (LA(1)==STRING) ) {
        zzmatch(STRING);
        qstringVal = (ANTLRTokenPtr)LT(1);

        
        *psrString = qstringVal->getText( );
 consume();
      }
      else {
        if ( (LA(1)==CLOSE_QUOTE) ) {
        }
        else {FAIL(1,err8,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    zzmatch(CLOSE_QUOTE); consume();
  }
  else {
    if ( (LA(1)==STRING) ) {
      zzmatch(STRING);
      stringVal = (ANTLRTokenPtr)LT(1);

      
      *psrString = stringVal->getText( );
 consume();
    }
    else {FAIL(1,err9,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x20);
}

#include "TCBP_CircuitBlifGrammar.h"
