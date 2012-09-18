/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 *
 *   ..\..\pccts\bin\Antlr.exe -CC -gh TAXP_ArchitectureXmlGrammar.g
 *
 */

#define ANTLR_VERSION	13333
#include "pcctscfg.h"
#include "pccts_stdio.h"
#include "tokens.h"

#include <stdio.h>

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TAS_ArchitectureSpec.h"

#include "TAXP_ArchitectureXmlFile.h"
#include "TAXP_ArchitectureXmlScanner_c.h"
#include "AParser.h"
#include "TAXP_ArchitectureXmlParser_c.h"
#include "DLexerBase.h"
#include "ATokPtr.h"

/* MR23 In order to remove calls to PURIFY use the antlr -nopurify option */

#ifndef PCCTS_PURIFY
#define PCCTS_PURIFY(r,s) memset((char *) &(r),'\0',(s));
#endif


void
TAXP_ArchitectureXmlParser_c::start(void)
{
  zzRULE;
  zzmatch(108); consume();
  zzmatch(ARCHITECTURE); consume();
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      zzmatch(108); consume();
      {
        if ( (LA(1)==LAYOUT) ) {
          layoutDef(  &this->parchitectureSpec_->config  );
        }
        else {
          if ( (LA(1)==DEVICE) ) {
            deviceDef(  &this->parchitectureSpec_->config  );
          }
          else {
            if ( (LA(1)==COMPLEXBLOCKLIST) ) {
              complexBlockList(  &this->parchitectureSpec_->physicalBlockList  );
            }
            else {
              if ( (LA(1)==MODELS) ) {
                cellList(  &this->parchitectureSpec_->cellList  );
              }
              else {
                if ( (LA(1)==SWITCHLIST) ) {
                  switchBoxList(  &this->parchitectureSpec_->switchBoxList  );
                }
                else {
                  if ( (LA(1)==SEGMENTLIST) ) {
                    segmentList(  &this->parchitectureSpec_->segmentList  );
                  }
                  else {FAIL(1,err1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
            }
          }
        }
      }
    }
  }
  zzmatch(110); consume();
  zzmatch(ARCHITECTURE); consume();
  zzmatch(109); consume();
  zzmatch(END_OF_FILE); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x1);
}

void
TAXP_ArchitectureXmlParser_c::layoutDef(TAS_Config_c* pconfig)
{
  zzRULE;
  zzmatch(LAYOUT); consume();
  {
    if ( (LA(1)==AUTO) ) {
      zzmatch(AUTO); consume();
      zzmatch(EQUAL); consume();
      floatNum(  &pconfig->layout.autoSize.aspectRatio  );
    }
    else {
      if ( (LA(1)==WIDTH) ) {
        zzmatch(WIDTH); consume();
        zzmatch(EQUAL); consume();
        intNum(  &pconfig->layout.manualSize.gridDims.width  );
        zzmatch(HEIGHT); consume();
        zzmatch(EQUAL); consume();
        intNum(  &pconfig->layout.manualSize.gridDims.height  );
      }
      else {
        if ( (LA(1)==111) ) {
        }
        else {FAIL(1,err2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  zzmatch(111); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x2);
}

void
TAXP_ArchitectureXmlParser_c::deviceDef(TAS_Config_c* pconfig)
{
  zzRULE;
  zzmatch(DEVICE); consume();
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      zzmatch(108); consume();
      {
        if ( (LA(1)==SIZING) ) {
          zzmatch(SIZING); consume();
          {
            for (;;) {
              if ( !((setwd1[LA(1)]&0x4))) break;
              if ( (LA(1)==R_MINW_NMOS) ) {
                zzmatch(R_MINW_NMOS); consume();
                zzmatch(EQUAL); consume();
                floatNum(  &pconfig->device.areaModel.resMinWidthNMOS  );
              }
              else {
                if ( (LA(1)==R_MINW_PMOS) ) {
                  zzmatch(R_MINW_PMOS); consume();
                  zzmatch(EQUAL); consume();
                  floatNum(  &pconfig->device.areaModel.resMinWidthPMOS  );
                }
                else {
                  if ( (LA(1)==IPIN_MUX_SIZE) ) {
                    zzmatch(IPIN_MUX_SIZE); consume();
                    zzmatch(EQUAL); consume();
                    floatNum(  &pconfig->device.areaModel.sizeInputPinMux  );
                  }
                  else break; /* MR6 code for exiting loop "for sure" */
                }
              }
            }
          }
          zzmatch(111); consume();
        }
        else {
          if ( (LA(1)==AREA) ) {
            zzmatch(AREA); consume();
            {
              while ( (LA(1)==GRID_LOGIC_AREA) ) {
                zzmatch(GRID_LOGIC_AREA); consume();
                zzmatch(EQUAL); consume();
                floatNum(  &pconfig->device.areaModel.areaGridTile  );
              }
            }
            zzmatch(111); consume();
          }
          else {
            if ( (LA(1)==CHAN_WIDTH_DISTR) ) {
              zzmatch(CHAN_WIDTH_DISTR); consume();
              zzmatch(109); consume();
              {
                while ( (LA(1)==108) ) {
                  channelWidth(  &pconfig->device.channelWidth.io,
                          &pconfig->device.channelWidth.x,
                          &pconfig->device.channelWidth.y  );
                }
              }
              zzmatch(110); consume();
              zzmatch(CHAN_WIDTH_DISTR); consume();
              zzmatch(109); consume();
            }
            else {
              if ( (LA(1)==SWITCH_BLOCK) ) {
                zzmatch(SWITCH_BLOCK); consume();
                {
                  for (;;) {
                    if ( !((setwd1[LA(1)]&0x8))) break;
                    if ( (LA(1)==TYPE) ) {
                      zzmatch(TYPE); consume();
                      zzmatch(EQUAL); consume();
                      switchBoxModelType(  &pconfig->device.switchBoxes.modelType  );
                    }
                    else {
                      if ( (LA(1)==FS) ) {
                        zzmatch(FS); consume();
                        zzmatch(EQUAL); consume();
                        uintNum(  &pconfig->device.switchBoxes.fs  );
                      }
                      else break; /* MR6 code for exiting loop "for sure" */
                    }
                  }
                }
                zzmatch(111); consume();
              }
              else {
                if ( (LA(1)==TIMING) ) {
                  zzmatch(TIMING); consume();
                  {
                    for (;;) {
                      if ( !((setwd1[LA(1)]&0x10))) break;
                      if ( (LA(1)==C_IPIN_CBLOCK) ) {
                        zzmatch(C_IPIN_CBLOCK); consume();
                        zzmatch(EQUAL); consume();
                        floatNum(  &pconfig->device.connectionBoxes.capInput  );
                      }
                      else {
                        if ( (LA(1)==T_IPIN_CBLOCK) ) {
                          zzmatch(T_IPIN_CBLOCK); consume();
                          zzmatch(EQUAL); consume();
                          expNum(  &pconfig->device.connectionBoxes.delayInput  );
                        }
                        else break; /* MR6 code for exiting loop "for sure" */
                      }
                    }
                  }
                  zzmatch(111); consume();
                }
                else {FAIL(1,err3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
        }
      }
    }
  }
  zzmatch(110); consume();
  zzmatch(DEVICE); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x20);
}

void
TAXP_ArchitectureXmlParser_c::cellList(TAS_CellList_t* pcellList)
{
  zzRULE;
  zzmatch(MODELS); consume();
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      cellDef(  pcellList  );
    }
  }
  zzmatch(110); consume();
  zzmatch(MODELS); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x40);
}

void
TAXP_ArchitectureXmlParser_c::cellDef(TAS_CellList_t* pcellList)
{
  zzRULE;
  
  TAS_Cell_c cell;
  
       string srName;
  TLO_PortList_t portList;
  zzmatch(108); consume();
  zzmatch(MODEL); consume();
  zzmatch(NAME); consume();
  zzmatch(EQUAL); consume();
  stringText(  &srName  );
  zzmatch(109);
  
  cell.SetName( srName );
  cell.SetSource( TLO_CELL_SOURCE_CUSTOM );
 consume();
  {
    while ( (LA(1)==108) ) {
      zzmatch(108); consume();
      {
        if ( (LA(1)==INPUT_PORTS) ) {
          inputPortList(  &portList  );
        }
        else {
          if ( (LA(1)==OUTPUT_PORTS) ) {
            outputPortList(  &portList  );
          }
          else {FAIL(1,err4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  
  if( portList.IsValid( ))
  {
    cell.SetPortList( portList );
  }
  zzmatch(110); consume();
  zzmatch(MODEL); consume();
  zzmatch(109);
  
  if( cell.IsValid( ))
  {
    pcellList->Add( cell );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x80);
}

void
TAXP_ArchitectureXmlParser_c::switchBoxList(TAS_SwitchBoxList_t* pswitchBoxList)
{
  zzRULE;
  zzmatch(SWITCHLIST); consume();
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      switchBoxDef(  pswitchBoxList  );
    }
  }
  zzmatch(110); consume();
  zzmatch(SWITCHLIST); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x1);
}

void
TAXP_ArchitectureXmlParser_c::switchBoxDef(TAS_SwitchBoxList_t* pswitchBoxList)
{
  zzRULE;
  
  TAS_SwitchBox_c switchBox;
  zzmatch(108); consume();
  zzmatch(SWITCH); consume();
  {
    for (;;) {
      if ( !((setwd2[LA(1)]&0x2))) break;
      if ( (LA(1)==NAME) ) {
        zzmatch(NAME); consume();
        zzmatch(EQUAL); consume();
        stringText(  &switchBox.srName  );
      }
      else {
        if ( (LA(1)==TYPE) ) {
          zzmatch(TYPE); consume();
          zzmatch(EQUAL); consume();
          switchBoxType(  &switchBox.type  );
        }
        else {
          if ( (LA(1)==R) ) {
            zzmatch(R); consume();
            zzmatch(EQUAL); consume();
            floatNum(  &switchBox.timing.res  );
          }
          else {
            if ( (LA(1)==CIN) ) {
              zzmatch(CIN); consume();
              zzmatch(EQUAL); consume();
              floatNum(  &switchBox.timing.capInput  );
            }
            else {
              if ( (LA(1)==COUT) ) {
                zzmatch(COUT); consume();
                zzmatch(EQUAL); consume();
                floatNum(  &switchBox.timing.capOutput  );
              }
              else {
                if ( (LA(1)==TDEL) ) {
                  zzmatch(TDEL); consume();
                  zzmatch(EQUAL); consume();
                  floatNum(  &switchBox.timing.delay  );
                }
                else {
                  if ( (LA(1)==BUF_SIZE) ) {
                    zzmatch(BUF_SIZE); consume();
                    zzmatch(EQUAL); consume();
                    floatNum(  &switchBox.area.buffer  );
                  }
                  else {
                    if ( (LA(1)==MUX_TRANS_SIZE) ) {
                      zzmatch(MUX_TRANS_SIZE); consume();
                      zzmatch(EQUAL); consume();
                      floatNum(  &switchBox.area.muxTransistor  );
                    }
                    else break; /* MR6 code for exiting loop "for sure" */
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  zzmatch(111);
  
  if( switchBox.IsValid( ))
  {
    pswitchBoxList->Add( switchBox );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x4);
}

void
TAXP_ArchitectureXmlParser_c::segmentList(TAS_SegmentList_t* psegmentList)
{
  zzRULE;
  zzmatch(SEGMENTLIST); consume();
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      segmentDef(  psegmentList  );
    }
  }
  zzmatch(110); consume();
  zzmatch(SEGMENTLIST); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x8);
}

void
TAXP_ArchitectureXmlParser_c::segmentDef(TAS_SegmentList_t* psegmentList)
{
  zzRULE;
  
  TAS_Segment_c segment;
  zzmatch(108); consume();
  zzmatch(SEGMENT); consume();
  {
    for (;;) {
      if ( !((setwd2[LA(1)]&0x10))) break;
      if ( (LA(1)==LENGTH) ) {
        zzmatch(LENGTH); consume();
        zzmatch(EQUAL); consume();
        segmentLength(  &segment.length  );
      }
      else {
        if ( (LA(1)==TYPE) ) {
          zzmatch(TYPE); consume();
          zzmatch(EQUAL); consume();
          segmentDirType(  &segment.dirType  );
        }
        else {
          if ( (LA(1)==FREQ) ) {
            zzmatch(FREQ); consume();
            zzmatch(EQUAL); consume();
            floatNum(  &segment.trackFreq  );
          }
          else {
            if ( (LA(1)==RMETAL) ) {
              zzmatch(RMETAL); consume();
              zzmatch(EQUAL); consume();
              floatNum(  &segment.timing.res  );
            }
            else {
              if ( (LA(1)==CMETAL) ) {
                zzmatch(CMETAL); consume();
                zzmatch(EQUAL); consume();
                floatNum(  &segment.timing.cap  );
              }
              else break; /* MR6 code for exiting loop "for sure" */
            }
          }
        }
      }
    }
  }
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      zzmatch(108); consume();
      {
        if ( (LA(1)==SB) ) {
          sbList(  &segment.sbPattern  );
        }
        else {
          if ( (LA(1)==CB) ) {
            cbList(  &segment.cbPattern  );
          }
          else {
            if ( (LA(1)==MUX) ) {
              zzmatch(MUX); consume();
              zzmatch(NAME); consume();
              zzmatch(EQUAL); consume();
              stringText(  &segment.srMuxSwitchName  );
              zzmatch(111); consume();
            }
            else {
              if ( (LA(1)==WIRE_SWITCH) ) {
                zzmatch(WIRE_SWITCH); consume();
                zzmatch(NAME); consume();
                zzmatch(EQUAL); consume();
                stringText(  &segment.srWireSwitchName  );
                zzmatch(111); consume();
              }
              else {
                if ( (LA(1)==OPIN_SWITCH) ) {
                  zzmatch(OPIN_SWITCH); consume();
                  zzmatch(NAME); consume();
                  zzmatch(EQUAL); consume();
                  stringText(  &segment.srOutputSwitchName  );
                  zzmatch(111); consume();
                }
                else {FAIL(1,err5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
        }
      }
    }
  }
  zzmatch(110); consume();
  zzmatch(SEGMENT); consume();
  zzmatch(109);
  
  if( segment.IsValid( ))
  {
    psegmentList->Add( segment );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x20);
}

void
TAXP_ArchitectureXmlParser_c::complexBlockList(TAS_PhysicalBlockList_t* pphysicalBlockList)
{
  zzRULE;
  zzmatch(COMPLEXBLOCKLIST); consume();
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      zzmatch(108); consume();
      pbtypeList(  pphysicalBlockList  );
    }
  }
  zzmatch(110); consume();
  zzmatch(COMPLEXBLOCKLIST); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x40);
}

void
TAXP_ArchitectureXmlParser_c::pbtypeList(TAS_PhysicalBlockList_t* pphysicalBlockList)
{
  zzRULE;
  zzmatch(PB_TYPE); consume();
  pbtypeDef(  pphysicalBlockList  );
  zzmatch(110); consume();
  zzmatch(PB_TYPE); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x80);
}

void
TAXP_ArchitectureXmlParser_c::pbtypeDef(TAS_PhysicalBlockList_t* pphysicalBlockList)
{
  zzRULE;
  
  TAS_PhysicalBlock_c physicalBlock;
  
      unsigned int ignored;
  zzmatch(NAME); consume();
  zzmatch(EQUAL); consume();
  stringText(  &physicalBlock.srName  );
  {
    for (;;) {
      if ( !((setwd3[LA(1)]&0x1))) break;
      if ( (LA(1)==HEIGHT) ) {
        zzmatch(HEIGHT); consume();
        zzmatch(EQUAL); consume();
        uintNum(  &physicalBlock.height  );
      }
      else {
        if ( (LA(1)==CAPACITY) ) {
          zzmatch(CAPACITY); consume();
          zzmatch(EQUAL); consume();
          uintNum(  &physicalBlock.capacity  );
        }
        else {
          if ( (LA(1)==AREA) ) {
            zzmatch(AREA); consume();
            zzmatch(EQUAL); consume();
            uintNum(  &ignored  );
          }
          else {
            if ( (LA(1)==NUM_PB) ) {
              zzmatch(NUM_PB); consume();
              zzmatch(EQUAL); consume();
              uintNum(  &physicalBlock.numPB  );
            }
            else {
              if ( (LA(1)==CLASS) ) {
                zzmatch(CLASS); consume();
                zzmatch(EQUAL); consume();
                classText(  &physicalBlock.classType  );
              }
              else {
                if ( (LA(1)==BLIF_MODEL) ) {
                  zzmatch(BLIF_MODEL); consume();
                  zzmatch(EQUAL); consume();
                  blifModelText(  &physicalBlock.srModelName, 
                                      &physicalBlock.modelType  );
                }
                else break; /* MR6 code for exiting loop "for sure" */
              }
            }
          }
        }
      }
    }
  }
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      zzmatch(108); consume();
      {
        if ( (setwd3[LA(1)]&0x2) ) {
          fcDef(  &physicalBlock.fcIn,
                &physicalBlock.fcOut  );
        }
        else {
          if ( (LA(1)==MODE) ) {
            modeDef(  &physicalBlock.modeList  );
          }
          else {
            if ( (LA(1)==PB_TYPE) ) {
              pbtypeList(  &physicalBlock.physicalBlockList  );
            }
            else {
              if ( (LA(1)==INTERCONNECT) ) {
                interconnectList(  &physicalBlock.interconnectList  );
              }
              else {
                if ( (setwd3[LA(1)]&0x4) ) {
                  portDef(  &physicalBlock.portList  );
                }
                else {
                  if ( (setwd3[LA(1)]&0x8) ) {
                    timingDelayLists(  &physicalBlock.timingDelayLists  );
                  }
                  else {
                    if ( (LA(1)==PINLOCATIONS) ) {
                      pinAssignList(  &physicalBlock.pinAssignPattern,
                        &physicalBlock.pinAssignList  );
                    }
                    else {
                      if ( (LA(1)==GRIDLOCATIONS) ) {
                        gridAssignList(  &physicalBlock.gridAssignList  );
                      }
                      else {FAIL(1,err6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  
  if( physicalBlock.IsValid( ))
  {
    pphysicalBlockList->Add( physicalBlock );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x10);
}

void
TAXP_ArchitectureXmlParser_c::sbList(TAS_BitPattern_t* psbPattern)
{
  zzRULE;
  ANTLRTokenPtr bitStringVal=NULL;
  
  string srPattern;
  zzmatch(SB); consume();
  zzmatch(TYPE); consume();
  zzmatch(EQUAL); consume();
  stringText(  &srPattern  );
  zzmatch(109); consume();
  {
    while ( (LA(1)==BIT_CHAR) ) {
      zzmatch(BIT_CHAR);
      bitStringVal = (ANTLRTokenPtr)LT(1);

      
      if( TC_stricmp( srPattern.data( ), "pattern" ) == 0 )
      {
        string srBit = bitStringVal->getText( );
        if( srBit.length( ))
        {
          TC_Bit_c bit( srBit[ 0 ] );
          psbPattern->Add( bit );
        }
      }
 consume();
    }
  }
  zzmatch(110); consume();
  zzmatch(SB); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x20);
}

void
TAXP_ArchitectureXmlParser_c::cbList(TAS_BitPattern_t* pcbPattern)
{
  zzRULE;
  ANTLRTokenPtr bitStringVal=NULL;
  
  string srPattern;
  zzmatch(CB); consume();
  zzmatch(TYPE); consume();
  zzmatch(EQUAL); consume();
  stringText(  &srPattern  );
  zzmatch(109); consume();
  {
    while ( (LA(1)==BIT_CHAR) ) {
      zzmatch(BIT_CHAR);
      bitStringVal = (ANTLRTokenPtr)LT(1);

      
      if( TC_stricmp( srPattern.data( ), "pattern" ) == 0 )
      {
        string srBit = bitStringVal->getText( );
        if( srBit.length( ))
        {
          TC_Bit_c bit( srBit[ 0 ] );
          pcbPattern->Add( bit );
        }
      }
 consume();
    }
  }
  zzmatch(110); consume();
  zzmatch(CB); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x40);
}

void
TAXP_ArchitectureXmlParser_c::inputPortList(TLO_PortList_t* pportList)
{
  zzRULE;
  
  string srName;
  string srIsClock;
  TC_TypeMode_t type;
  zzmatch(INPUT_PORTS); consume();
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      zzmatch(108); consume();
      zzmatch(PORT); consume();
      zzmatch(NAME); consume();
      zzmatch(EQUAL); consume();
      stringText(  &srName  );
      
      type = TC_TYPE_INPUT;
      {
        if ( (LA(1)==IS_CLOCK) ) {
          zzmatch(IS_CLOCK); consume();
          zzmatch(EQUAL); consume();
          stringText(  &srIsClock  );
          
          if(( srIsClock.length( ) == 1 ) && ( srIsClock[ 0 ] == '1' ))
          {
            type = TC_TYPE_CLOCK;
          }
        }
        else {
          if ( (LA(1)==111) ) {
          }
          else {FAIL(1,err7,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      zzmatch(111);
      
      TLO_Port_c port( srName, type );
      pportList->Add( port );
 consume();
    }
  }
  zzmatch(110); consume();
  zzmatch(INPUT_PORTS); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x80);
}

void
TAXP_ArchitectureXmlParser_c::outputPortList(TLO_PortList_t* pportList)
{
  zzRULE;
  
  string srName;
  zzmatch(OUTPUT_PORTS); consume();
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      zzmatch(108); consume();
      zzmatch(PORT); consume();
      zzmatch(NAME); consume();
      zzmatch(EQUAL); consume();
      stringText(  &srName  );
      zzmatch(111);
      
      TLO_Port_c port( srName, TC_TYPE_OUTPUT );
      pportList->Add( port );
 consume();
    }
  }
  zzmatch(110); consume();
  zzmatch(OUTPUT_PORTS); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x1);
}

void
TAXP_ArchitectureXmlParser_c::fcDef(TAS_ConnectionFc_c* pfcIn,TAS_ConnectionFc_c* pfcOut)
{
  zzRULE;
  
  double floatValue = 0.0;
  unsigned int uintValue = 0;
  {
    if ( (LA(1)==FC_IN) ) {
      zzmatch(FC_IN); consume();
      zzmatch(TYPE); consume();
      zzmatch(EQUAL); consume();
      connectionBoxInType(  &pfcIn->type  );
      zzmatch(109); consume();
      {
        if ( (setwd4[LA(1)]&0x2) ) {
          floatNum(  &floatValue  );
        }
        else {
          if ( (LA(1)==110) ) {
          }
          else {FAIL(1,err8,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      zzmatch(110); consume();
      zzmatch(FC_IN); consume();
      zzmatch(109);
      
      uintValue = static_cast< unsigned int >( floatValue + TC_FLT_EPSILON );
      pfcIn->percent = ( pfcIn->type == TAS_CONNECTION_BOX_FRACTION ? floatValue : 0.0 );
      pfcIn->absolute = ( pfcIn->type == TAS_CONNECTION_BOX_ABSOLUTE ? uintValue : 0 );
 consume();
    }
    else {
      if ( (LA(1)==FC_OUT) ) {
        zzmatch(FC_OUT); consume();
        zzmatch(TYPE); consume();
        zzmatch(EQUAL); consume();
        connectionBoxOutType(  &pfcOut->type  );
        zzmatch(109); consume();
        {
          if ( (setwd4[LA(1)]&0x4) ) {
            floatNum(  &floatValue  );
          }
          else {
            if ( (LA(1)==110) ) {
            }
            else {FAIL(1,err9,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzmatch(110); consume();
        zzmatch(FC_OUT); consume();
        zzmatch(109);
        
        uintValue = static_cast< unsigned int >( floatValue + TC_FLT_EPSILON );
        pfcOut->percent = ( pfcOut->type == TAS_CONNECTION_BOX_FRACTION ? floatValue : 0.0 );
        pfcOut->absolute = ( pfcOut->type == TAS_CONNECTION_BOX_ABSOLUTE ? uintValue : 0 );
 consume();
      }
      else {FAIL(1,err10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x8);
}

void
TAXP_ArchitectureXmlParser_c::modeDef(TAS_ModeList_t* pmodeList)
{
  zzRULE;
  
  TAS_Mode_c mode;
  zzmatch(MODE); consume();
  zzmatch(NAME); consume();
  zzmatch(EQUAL); consume();
  stringText(  &mode.srName  );
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      zzmatch(108); consume();
      {
        for (;;) {
          if ( !((setwd4[LA(1)]&0x10))) break;
          if ( (LA(1)==PB_TYPE) ) {
            pbtypeList(  &mode.physicalBlockList  );
          }
          else {
            if ( (LA(1)==INTERCONNECT) ) {
              interconnectList(  &mode.interconnectList  );
            }
            else break; /* MR6 code for exiting loop "for sure" */
          }
        }
      }
    }
  }
  zzmatch(110); consume();
  zzmatch(MODE); consume();
  zzmatch(109);
  
  if( mode.IsValid( ))
  {
    pmodeList->Add( mode );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x20);
}

void
TAXP_ArchitectureXmlParser_c::interconnectList(TAS_InterconnectList_t* pinterconnectList)
{
  zzRULE;
  zzmatch(INTERCONNECT); consume();
  zzmatch(109); consume();
  {
    while ( (LA(1)==108) ) {
      interconnectDef(  pinterconnectList  );
    }
  }
  zzmatch(110); consume();
  zzmatch(INTERCONNECT); consume();
  zzmatch(109); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x40);
}

void
TAXP_ArchitectureXmlParser_c::interconnectDef(TAS_InterconnectList_t* pinterconnectList)
{
  zzRULE;
  
  TAS_Interconnect_c interconnect;
  zzmatch(108); consume();
  {
    if ( (LA(1)==COMPLETE) ) {
      zzmatch(COMPLETE); consume();
      interconnectElem(  &interconnect, TAS_INTERCONNECT_MAP_COMPLETE  );
      {
        if ( (LA(1)==111) ) {
          zzmatch(111); consume();
        }
        else {
          if ( (LA(1)==110) ) {
            zzmatch(110); consume();
            zzmatch(COMPLETE); consume();
            zzmatch(109); consume();
          }
          else {FAIL(1,err11,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
    else {
      if ( (LA(1)==DIRECT) ) {
        zzmatch(DIRECT); consume();
        interconnectElem(  &interconnect, TAS_INTERCONNECT_MAP_DIRECT  );
        {
          if ( (LA(1)==111) ) {
            zzmatch(111); consume();
          }
          else {
            if ( (LA(1)==110) ) {
              zzmatch(110); consume();
              zzmatch(DIRECT); consume();
              zzmatch(109); consume();
            }
            else {FAIL(1,err12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
      else {
        if ( (LA(1)==MUX) ) {
          zzmatch(MUX); consume();
          interconnectElem(  &interconnect, TAS_INTERCONNECT_MAP_MUX  );
          {
            if ( (LA(1)==111) ) {
              zzmatch(111); consume();
            }
            else {
              if ( (LA(1)==110) ) {
                zzmatch(110); consume();
                zzmatch(MUX); consume();
                zzmatch(109); consume();
              }
              else {FAIL(1,err13,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
        else {FAIL(1,err14,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  
  if( interconnect.IsValid( ))
  {
    pinterconnectList->Add( interconnect );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x80);
}

void
TAXP_ArchitectureXmlParser_c::interconnectElem(TAS_Interconnect_c* pinterconnect,TAS_InterconnectMapType_t mapType)
{
  zzRULE;
  
  pinterconnect->mapType = mapType;
  string srInputName, srOutputName;
  {
    int zzcnt=1;
    do {
      if ( (LA(1)==NAME) ) {
        zzmatch(NAME); consume();
        zzmatch(EQUAL); consume();
        stringText(  &pinterconnect->srName  );
      }
      else {
        if ( (LA(1)==INPUT) ) {
          zzmatch(INPUT); consume();
          zzmatch(EQUAL); consume();
          stringText(  &srInputName  );
          
          pinterconnect->inputNameList.Add( srInputName );
        }
        else {
          if ( (LA(1)==OUTPUT) ) {
            zzmatch(OUTPUT); consume();
            zzmatch(EQUAL); consume();
            stringText(  &srOutputName  );
            
            pinterconnect->outputNameList.Add( srOutputName );
          }
          else {
            if ( (LA(1)==109) ) {
              zzmatch(109); consume();
              {
                int zzcnt=1;
                do {
                  zzmatch(108); consume();
                  timingDelayLists(  &pinterconnect->timingDelayLists  );
                } while ( (LA(1)==108) );
              }
            }
            /* MR10 ()+ */ else {
              if ( zzcnt > 1 ) break;
              else {FAIL(1,err15,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
      }
      zzcnt++;
    } while ( 1 );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x1);
}

void
TAXP_ArchitectureXmlParser_c::portDef(TLO_PortList_t* pportList)
{
  zzRULE;
  {
    if ( (LA(1)==INPUT) ) {
      zzmatch(INPUT); consume();
      portTypeDef(  pportList, TC_TYPE_INPUT  );
    }
    else {
      if ( (LA(1)==OUTPUT) ) {
        zzmatch(OUTPUT); consume();
        portTypeDef(  pportList, TC_TYPE_OUTPUT  );
      }
      else {
        if ( (LA(1)==CLOCK) ) {
          zzmatch(CLOCK); consume();
          portTypeDef(  pportList, TC_TYPE_CLOCK  );
        }
        else {FAIL(1,err16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  zzmatch(111); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x2);
}

void
TAXP_ArchitectureXmlParser_c::portTypeDef(TLO_PortList_t* pportList,TC_TypeMode_t type)
{
  zzRULE;
  
  TLO_Port_c port;
  
      string srName;
  unsigned int count = 1;
  bool isEquivalent = false;
  string srClass;
  zzmatch(NAME); consume();
  zzmatch(EQUAL); consume();
  stringText(  &srName  );
  {
    for (;;) {
      if ( !((setwd5[LA(1)]&0x4))) break;
      if ( (LA(1)==NUM_PINS) ) {
        zzmatch(NUM_PINS); consume();
        zzmatch(EQUAL); consume();
        uintNum(  &count  );
      }
      else {
        if ( (LA(1)==EQUIVALENT) ) {
          zzmatch(EQUIVALENT); consume();
          zzmatch(EQUAL); consume();
          boolType(  &isEquivalent  );
        }
        else {
          if ( (LA(1)==PORT_CLASS) ) {
            zzmatch(PORT_CLASS); consume();
            zzmatch(EQUAL); consume();
            stringText(  &srClass  );
          }
          else break; /* MR6 code for exiting loop "for sure" */
        }
      }
    }
  }
  
  port.SetName( srName );
  port.SetType( type );
  port.SetCount( count );
  port.SetEquivalent( isEquivalent );
  port.SetClass( srClass );
  
      pportList->Add( port );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x8);
}

void
TAXP_ArchitectureXmlParser_c::timingDelayLists(TAS_TimingDelayLists_c* ptimingDelayLists)
{
  zzRULE;
  
  TAS_TimingDelay_c timingDelay;
  
      string srMax;
  {
    if ( (LA(1)==DELAY_CONSTANT) ) {
      zzmatch(DELAY_CONSTANT); consume();
      {
        for (;;) {
          if ( !((setwd5[LA(1)]&0x10))) break;
          if ( (LA(1)==MAX) ) {
            zzmatch(MAX); consume();
            zzmatch(EQUAL); consume();
            floatNum(  &timingDelay.delay  );
          }
          else {
            if ( (LA(1)==IN_PORT) ) {
              zzmatch(IN_PORT); consume();
              zzmatch(EQUAL); consume();
              stringText(  &timingDelay.srInputPortName  );
            }
            else {
              if ( (LA(1)==OUT_PORT) ) {
                zzmatch(OUT_PORT); consume();
                zzmatch(EQUAL); consume();
                stringText(  &timingDelay.srOutputPortName  );
              }
              else break; /* MR6 code for exiting loop "for sure" */
            }
          }
        }
      }
      zzmatch(111);
      
      timingDelay.type = TAS_TIMING_DELAY_CONSTANT;
      ptimingDelayLists->delayList.Add( timingDelay );
 consume();
    }
    else {
      if ( (LA(1)==DELAY_MATRIX) ) {
        zzmatch(DELAY_MATRIX); consume();
        {
          for (;;) {
            if ( !((setwd5[LA(1)]&0x20))) break;
            if ( (LA(1)==TYPE) ) {
              zzmatch(TYPE); consume();
              zzmatch(EQUAL); consume();
              stringText(  &srMax  );
            }
            else {
              if ( (LA(1)==IN_PORT) ) {
                zzmatch(IN_PORT); consume();
                zzmatch(EQUAL); consume();
                stringText(  &timingDelay.srInputPortName  );
              }
              else {
                if ( (LA(1)==OUT_PORT) ) {
                  zzmatch(OUT_PORT); consume();
                  zzmatch(EQUAL); consume();
                  stringText(  &timingDelay.srOutputPortName  );
                }
                else break; /* MR6 code for exiting loop "for sure" */
              }
            }
          }
        }
        zzmatch(109); consume();
        delayMatrixDef(  &timingDelay.delayMatrix  );
        zzmatch(110); consume();
        zzmatch(DELAY_MATRIX); consume();
        zzmatch(109);
        
        timingDelay.type = TAS_TIMING_DELAY_MATRIX;
        ptimingDelayLists->delayMatrixList.Add( timingDelay );
 consume();
      }
      else {
        if ( (LA(1)==T_SETUP) ) {
          zzmatch(T_SETUP); consume();
          {
            for (;;) {
              if ( !((setwd5[LA(1)]&0x40))) break;
              if ( (LA(1)==VALUE) ) {
                zzmatch(VALUE); consume();
                zzmatch(EQUAL); consume();
                floatNum(  &timingDelay.delay  );
              }
              else {
                if ( (LA(1)==PORT) ) {
                  zzmatch(PORT); consume();
                  zzmatch(EQUAL); consume();
                  stringText(  &timingDelay.srOutputPortName  );
                }
                else {
                  if ( (LA(1)==CLOCK) ) {
                    zzmatch(CLOCK); consume();
                    zzmatch(EQUAL); consume();
                    stringText(  &timingDelay.srClockPortName  );
                  }
                  else break; /* MR6 code for exiting loop "for sure" */
                }
              }
            }
          }
          zzmatch(111);
          
          timingDelay.type = TAS_TIMING_DELAY_SETUP;
          ptimingDelayLists->delayClockSetupList.Add( timingDelay );
 consume();
        }
        else {
          if ( (LA(1)==T_CLOCK) ) {
            zzmatch(T_CLOCK); consume();
            {
              for (;;) {
                if ( !((setwd5[LA(1)]&0x80))) break;
                if ( (LA(1)==MAX) ) {
                  zzmatch(MAX); consume();
                  zzmatch(EQUAL); consume();
                  floatNum(  &timingDelay.delay  );
                }
                else {
                  if ( (LA(1)==PORT) ) {
                    zzmatch(PORT); consume();
                    zzmatch(EQUAL); consume();
                    stringText(  &timingDelay.srOutputPortName  );
                  }
                  else {
                    if ( (LA(1)==CLOCK) ) {
                      zzmatch(CLOCK); consume();
                      zzmatch(EQUAL); consume();
                      stringText(  &timingDelay.srClockPortName  );
                    }
                    else break; /* MR6 code for exiting loop "for sure" */
                  }
                }
              }
            }
            zzmatch(111);
            
            timingDelay.type = TAS_TIMING_DELAY_CLOCK_TO_Q;
            ptimingDelayLists->delayClockToQList.Add( timingDelay );
 consume();
          }
          else {FAIL(1,err17,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd6, 0x1);
}

void
TAXP_ArchitectureXmlParser_c::delayMatrixDef(TAS_DelayMatrix_t* pdelayMatrix)
{
  zzRULE;
  
  TAS_DelayTable_t delayTable;
  TAS_DelayList_t delayList;
  
      double value;
  size_t curTokenLine, nextTokenLine;
  {
    int zzcnt=1;
    do {
      expNum(  &value  );
      
      delayList.Add( value );
      
         curTokenLine = LT( 0 )->getLine( );
      nextTokenLine = LT( 1 )->getLine( );
      if( curTokenLine != nextTokenLine )
      {
        delayTable.Add( delayList );
        delayList.Clear( );
      }
    } while ( (setwd6[LA(1)]&0x2) );
  }
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd6, 0x4);
}

void
TAXP_ArchitectureXmlParser_c::pinAssignList(TAS_PinAssignPatternType_t* ppinAssignPattern,TAS_PinAssignList_t* ppinAssignList)
{
  zzRULE;
  
  TAS_PinAssign_c pinAssign;
  zzmatch(PINLOCATIONS); consume();
  zzmatch(PATTERN); consume();
  zzmatch(EQUAL); consume();
  pinAssignPatternType(  ppinAssignPattern  );
  {
    if ( (LA(1)==109) ) {
      zzmatch(109); consume();
      {
        while ( (LA(1)==108) ) {
          pinAssignDef(  *ppinAssignPattern, ppinAssignList  );
        }
      }
    }
    else {
      if ( (setwd6[LA(1)]&0x8) ) {
      }
      else {FAIL(1,err18,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  {
    if ( (LA(1)==111) ) {
      zzmatch(111); consume();
    }
    else {
      if ( (LA(1)==110) ) {
        zzmatch(110); consume();
        zzmatch(PINLOCATIONS); consume();
        zzmatch(109); consume();
      }
      else {FAIL(1,err19,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd6, 0x10);
}

void
TAXP_ArchitectureXmlParser_c::pinAssignDef(TAS_PinAssignPatternType_t pinAssignPattern,TAS_PinAssignList_t* ppinAssignList)
{
  zzRULE;
  
  TAS_PinAssign_c pinAssign;
  
      string srPortName;
  zzmatch(108); consume();
  zzmatch(LOC); consume();
  {
    for (;;) {
      if ( !((setwd6[LA(1)]&0x20))) break;
      if ( (LA(1)==SIDE) ) {
        zzmatch(SIDE); consume();
        zzmatch(EQUAL); consume();
        sideMode(  &pinAssign.side  );
      }
      else {
        if ( (LA(1)==OFFSET) ) {
          zzmatch(OFFSET); consume();
          zzmatch(EQUAL); consume();
          uintNum(  &pinAssign.offset  );
        }
        else break; /* MR6 code for exiting loop "for sure" */
      }
    }
  }
  zzmatch(109); consume();
  {
    while ( (setwd6[LA(1)]&0x40) ) {
      stringText(  &srPortName  );
      
      pinAssign.portNameList.Add( srPortName );
    }
  }
  zzmatch(110); consume();
  zzmatch(LOC); consume();
  zzmatch(109);
  
  pinAssign.pattern = pinAssignPattern;
  if( pinAssign.IsValid( ))
  {
    ppinAssignList->Add( pinAssign );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd6, 0x80);
}

void
TAXP_ArchitectureXmlParser_c::gridAssignList(TAS_GridAssignList_t* pgridAssignList)
{
  zzRULE;
  zzmatch(GRIDLOCATIONS); consume();
  {
    if ( (LA(1)==109) ) {
      zzmatch(109); consume();
      {
        while ( (LA(1)==108) ) {
          gridAssignDef(  pgridAssignList  );
        }
      }
    }
    else {
      if ( (setwd7[LA(1)]&0x1) ) {
      }
      else {FAIL(1,err20,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  {
    if ( (LA(1)==111) ) {
      zzmatch(111); consume();
    }
    else {
      if ( (LA(1)==110) ) {
        zzmatch(110); consume();
        zzmatch(GRIDLOCATIONS); consume();
        zzmatch(109); consume();
      }
      else {FAIL(1,err21,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x2);
}

void
TAXP_ArchitectureXmlParser_c::gridAssignDef(TAS_GridAssignList_t* pgridAssignList)
{
  zzRULE;
  
  TAS_GridAssign_c gridAssign;
  zzmatch(108); consume();
  zzmatch(LOC); consume();
  zzmatch(TYPE); consume();
  zzmatch(EQUAL); consume();
  gridAssignDistrMode(  &gridAssign.distr  );
  {
    for (;;) {
      if ( !((setwd7[LA(1)]&0x4))) break;
      if ( (LA(1)==PRIORITY) ) {
        zzmatch(PRIORITY); consume();
        zzmatch(EQUAL); consume();
        uintNum(  &gridAssign.priority  );
      }
      else {
        if ( (LA(1)==MULTIPLE_START) ) {
          zzmatch(MULTIPLE_START); consume();
          zzmatch(EQUAL); consume();
          uintNum(  &gridAssign.multipleStart  );
        }
        else {
          if ( (LA(1)==MULTIPLE_REPEAT) ) {
            zzmatch(MULTIPLE_REPEAT); consume();
            zzmatch(EQUAL); consume();
            uintNum(  &gridAssign.multipleRepeat  );
          }
          else {
            if ( (LA(1)==SINGLE_POS) ) {
              zzmatch(SINGLE_POS); consume();
              zzmatch(EQUAL); consume();
              floatNum(  &gridAssign.singlePercent  );
            }
            else break; /* MR6 code for exiting loop "for sure" */
          }
        }
      }
    }
  }
  zzmatch(111);
  
  if( gridAssign.IsValid( ))
  {
    pgridAssignList->Add( gridAssign );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x8);
}

void
TAXP_ArchitectureXmlParser_c::segmentLength(unsigned int* plength)
{
  zzRULE;
  
  string srLength;
  stringText(  &srLength  );
  
  if( TC_stricmp( srLength.data( ), "longline" ) == 0 )
  *plength = UINT_MAX;
  else
  *plength = static_cast< unsigned int >( atol( srLength.data( )));
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x10);
}

void
TAXP_ArchitectureXmlParser_c::segmentDirType(TAS_SegmentDirType_t* pdirType)
{
  zzRULE;
  
  string srDirType;
  stringText(  &srDirType  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x20);
}

void
TAXP_ArchitectureXmlParser_c::switchBoxType(TAS_SwitchBoxType_t* ptype)
{
  zzRULE;
  
  string srType;
  stringText(  &srType  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x40);
}

void
TAXP_ArchitectureXmlParser_c::switchBoxModelType(TAS_SwitchBoxModelType_t* pmodelType)
{
  zzRULE;
  
  string srModelType;
  stringText(  &srModelType  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x80);
}

void
TAXP_ArchitectureXmlParser_c::channelWidth(TAS_ChannelWidth_c* pchannelWidthIO,TAS_ChannelWidth_c* pchannelWidthX,TAS_ChannelWidth_c* pchannelWidthY)
{
  zzRULE;
  zzmatch(108); consume();
  {
    if ( (LA(1)==IO) ) {
      zzmatch(IO);
      
      pchannelWidthIO->usageMode = TAS_CHANNEL_USAGE_IO;
 consume();
      zzmatch(WIDTH); consume();
      zzmatch(EQUAL); consume();
      floatNum(  &pchannelWidthIO->width  );
    }
    else {
      if ( (LA(1)==X) ) {
        zzmatch(X);
        
        pchannelWidthX->usageMode = TAS_CHANNEL_USAGE_X;
 consume();
        zzmatch(DISTR); consume();
        zzmatch(EQUAL); consume();
        channelDistrMode(  &pchannelWidthX->distrMode  );
        {
          for (;;) {
            if ( !((setwd8[LA(1)]&0x1))) break;
            if ( (LA(1)==PEAK) ) {
              zzmatch(PEAK); consume();
              zzmatch(EQUAL); consume();
              floatNum(  &pchannelWidthX->peak  );
            }
            else {
              if ( (LA(1)==XPEAK) ) {
                zzmatch(XPEAK); consume();
                zzmatch(EQUAL); consume();
                floatNum(  &pchannelWidthX->xpeak  );
              }
              else {
                if ( (LA(1)==DC) ) {
                  zzmatch(DC); consume();
                  zzmatch(EQUAL); consume();
                  floatNum(  &pchannelWidthX->dc  );
                }
                else {
                  if ( (LA(1)==WIDTH) ) {
                    zzmatch(WIDTH); consume();
                    zzmatch(EQUAL); consume();
                    floatNum(  &pchannelWidthX->width  );
                  }
                  else break; /* MR6 code for exiting loop "for sure" */
                }
              }
            }
          }
        }
      }
      else {
        if ( (LA(1)==Y) ) {
          zzmatch(Y);
          
          pchannelWidthY->usageMode = TAS_CHANNEL_USAGE_Y;
 consume();
          zzmatch(DISTR); consume();
          zzmatch(EQUAL); consume();
          channelDistrMode(  &pchannelWidthY->distrMode  );
          {
            for (;;) {
              if ( !((setwd8[LA(1)]&0x2))) break;
              if ( (LA(1)==PEAK) ) {
                zzmatch(PEAK); consume();
                zzmatch(EQUAL); consume();
                floatNum(  &pchannelWidthY->peak  );
              }
              else {
                if ( (LA(1)==XPEAK) ) {
                  zzmatch(XPEAK); consume();
                  zzmatch(EQUAL); consume();
                  floatNum(  &pchannelWidthY->xpeak  );
                }
                else {
                  if ( (LA(1)==DC) ) {
                    zzmatch(DC); consume();
                    zzmatch(EQUAL); consume();
                    floatNum(  &pchannelWidthY->dc  );
                  }
                  else {
                    if ( (LA(1)==WIDTH) ) {
                      zzmatch(WIDTH); consume();
                      zzmatch(EQUAL); consume();
                      floatNum(  &pchannelWidthY->width  );
                    }
                    else break; /* MR6 code for exiting loop "for sure" */
                  }
                }
              }
            }
          }
        }
        else {FAIL(1,err22,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  zzmatch(111); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x4);
}

void
TAXP_ArchitectureXmlParser_c::channelDistrMode(TAS_ChannelDistrMode_t* pmode)
{
  zzRULE;
  
  string srMode;
  stringText(  &srMode  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x8);
}

void
TAXP_ArchitectureXmlParser_c::connectionBoxInType(TAS_ConnectionBoxType_t* ptype)
{
  zzRULE;
  
  string srType;
  stringText(  &srType  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x10);
}

void
TAXP_ArchitectureXmlParser_c::connectionBoxOutType(TAS_ConnectionBoxType_t* ptype)
{
  zzRULE;
  
  string srType;
  stringText(  &srType  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x20);
}

void
TAXP_ArchitectureXmlParser_c::pinAssignPatternType(TAS_PinAssignPatternType_t* ppatternType)
{
  zzRULE;
  
  string srPatternType;
  stringText(  &srPatternType  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x40);
}

void
TAXP_ArchitectureXmlParser_c::gridAssignDistrMode(TAS_GridAssignDistrMode_t* pdistrMode)
{
  zzRULE;
  
  string srDistrMode;
  stringText(  &srDistrMode  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x80);
}

void
TAXP_ArchitectureXmlParser_c::sideMode(TC_SideMode_t* pmode)
{
  zzRULE;
  
  string srMode;
  stringText(  &srMode  );
  
  if( TC_stricmp( srMode.data( ), "left" ) == 0 )
  {
    *pmode = TC_SIDE_LEFT;
  }
  else if( TC_stricmp( srMode.data( ), "right" ) == 0 )
  {
    *pmode = TC_SIDE_RIGHT;
  }
  else if( TC_stricmp( srMode.data( ), "bottom" ) == 0 )
  {
    *pmode = TC_SIDE_LOWER;
  }
  else if( TC_stricmp( srMode.data( ), "top" ) == 0 )
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd9, 0x1);
}

void
TAXP_ArchitectureXmlParser_c::classText(TAS_ClassType_t* ptype)
{
  zzRULE;
  
  string srType;
  stringText(  &srType  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd9, 0x2);
}

void
TAXP_ArchitectureXmlParser_c::blifModelText(string* psrString,TAS_PhysicalBlockModelType_t* ptype)
{
  zzRULE;
  stringText(  psrString  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd9, 0x4);
}

void
TAXP_ArchitectureXmlParser_c::stringText(string* psrString)
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
        else {FAIL(1,err23,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
    else {FAIL(1,err24,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd9, 0x8);
}

void
TAXP_ArchitectureXmlParser_c::floatNum(double* pdouble)
{
  zzRULE;
  ANTLRTokenPtr qfloatVal=NULL, qposIntVal=NULL, qnegIntVal=NULL, floatVal=NULL, posIntVal=NULL, negIntVal=NULL;
  {
    if ( (LA(1)==OPEN_QUOTE) ) {
      zzmatch(OPEN_QUOTE); consume();
      {
        if ( (LA(1)==STRING) ) {
          zzmatch(STRING);
          qfloatVal = (ANTLRTokenPtr)LT(1);

          
          *pdouble = atof( qfloatVal->getText( ));
 consume();
        }
        else {
          if ( (LA(1)==POS_INT) ) {
            zzmatch(POS_INT);
            qposIntVal = (ANTLRTokenPtr)LT(1);

            
            *pdouble = atof( qposIntVal->getText( ));
 consume();
          }
          else {
            if ( (LA(1)==NEG_INT) ) {
              zzmatch(NEG_INT);
              qnegIntVal = (ANTLRTokenPtr)LT(1);

              
              *pdouble = atof( qnegIntVal->getText( ));
 consume();
            }
            else {FAIL(1,err25,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
      zzmatch(CLOSE_QUOTE); consume();
    }
    else {
      if ( (LA(1)==FLOAT) ) {
        zzmatch(FLOAT);
        floatVal = (ANTLRTokenPtr)LT(1);

        
        *pdouble = atof( floatVal->getText( ));
 consume();
      }
      else {
        if ( (LA(1)==POS_INT) ) {
          zzmatch(POS_INT);
          posIntVal = (ANTLRTokenPtr)LT(1);

          
          *pdouble = atof( posIntVal->getText( ));
 consume();
        }
        else {
          if ( (LA(1)==NEG_INT) ) {
            zzmatch(NEG_INT);
            negIntVal = (ANTLRTokenPtr)LT(1);

            
            *pdouble = atof( negIntVal->getText( ));
 consume();
          }
          else {FAIL(1,err26,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd9, 0x10);
}

void
TAXP_ArchitectureXmlParser_c::expNum(double* pdouble)
{
  zzRULE;
  ANTLRTokenPtr qexpVal=NULL, qfloatVal=NULL, qposIntVal=NULL, qnegIntVal=NULL, expVal=NULL, floatVal=NULL, posIntVal=NULL, negIntVal=NULL;
  {
    if ( (LA(1)==OPEN_QUOTE) ) {
      zzmatch(OPEN_QUOTE); consume();
      {
        if ( (LA(1)==EXP) ) {
          zzmatch(EXP);
          qexpVal = (ANTLRTokenPtr)LT(1);

          
          *pdouble = atof( qexpVal->getText( ));
 consume();
        }
        else {
          if ( (LA(1)==STRING) ) {
            zzmatch(STRING);
            qfloatVal = (ANTLRTokenPtr)LT(1);

            
            *pdouble = atof( qfloatVal->getText( ));
 consume();
          }
          else {
            if ( (LA(1)==POS_INT) ) {
              zzmatch(POS_INT);
              qposIntVal = (ANTLRTokenPtr)LT(1);

              
              *pdouble = atof( qposIntVal->getText( ));
 consume();
            }
            else {
              if ( (LA(1)==NEG_INT) ) {
                zzmatch(NEG_INT);
                qnegIntVal = (ANTLRTokenPtr)LT(1);

                
                *pdouble = atof( qnegIntVal->getText( ));
 consume();
              }
              else {FAIL(1,err27,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
      }
      zzmatch(CLOSE_QUOTE); consume();
    }
    else {
      if ( (LA(1)==EXP) ) {
        zzmatch(EXP);
        expVal = (ANTLRTokenPtr)LT(1);

        
        *pdouble = atof( expVal->getText( ));
 consume();
      }
      else {
        if ( (LA(1)==FLOAT) ) {
          zzmatch(FLOAT);
          floatVal = (ANTLRTokenPtr)LT(1);

          
          *pdouble = atof( floatVal->getText( ));
 consume();
        }
        else {
          if ( (LA(1)==POS_INT) ) {
            zzmatch(POS_INT);
            posIntVal = (ANTLRTokenPtr)LT(1);

            
            *pdouble = atof( posIntVal->getText( ));
 consume();
          }
          else {
            if ( (LA(1)==NEG_INT) ) {
              zzmatch(NEG_INT);
              negIntVal = (ANTLRTokenPtr)LT(1);

              
              *pdouble = atof( negIntVal->getText( ));
 consume();
            }
            else {FAIL(1,err28,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd9, 0x20);
}

void
TAXP_ArchitectureXmlParser_c::intNum(int* pint)
{
  zzRULE;
  ANTLRTokenPtr qintVal=NULL, intVal=NULL;
  {
    if ( (LA(1)==OPEN_QUOTE) ) {
      zzmatch(OPEN_QUOTE); consume();
      zzmatch(STRING);
      qintVal = (ANTLRTokenPtr)LT(1);

      
      *pint = static_cast< int >( atol( qintVal->getText( )));
 consume();
      zzmatch(CLOSE_QUOTE); consume();
    }
    else {
      if ( (LA(1)==POS_INT) ) {
        zzmatch(POS_INT);
        intVal = (ANTLRTokenPtr)LT(1);

        
        *pint = static_cast< int >( atol( intVal->getText( )));
 consume();
      }
      else {FAIL(1,err29,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd9, 0x40);
}

void
TAXP_ArchitectureXmlParser_c::uintNum(unsigned int* puint)
{
  zzRULE;
  ANTLRTokenPtr quintVal=NULL, uintVal=NULL;
  {
    if ( (LA(1)==OPEN_QUOTE) ) {
      zzmatch(OPEN_QUOTE); consume();
      zzmatch(STRING);
      quintVal = (ANTLRTokenPtr)LT(1);

      
      *puint = static_cast< unsigned int >( atol( quintVal->getText( )));
 consume();
      zzmatch(CLOSE_QUOTE); consume();
    }
    else {
      if ( (LA(1)==POS_INT) ) {
        zzmatch(POS_INT);
        uintVal = (ANTLRTokenPtr)LT(1);

        
        *puint = static_cast< unsigned int >( atol( uintVal->getText( )));
 consume();
      }
      else {FAIL(1,err30,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd9, 0x80);
}

void
TAXP_ArchitectureXmlParser_c::boolType(bool* pbool)
{
  zzRULE;
  ANTLRTokenPtr qboolVal=NULL;
  {
    
    const char* pszBool;
    if ( (LA(1)==OPEN_QUOTE) ) {
      zzmatch(OPEN_QUOTE); consume();
      zzmatch(STRING);
      qboolVal = (ANTLRTokenPtr)LT(1);

      
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
 consume();
      zzmatch(CLOSE_QUOTE); consume();
    }
    else {
      if ( (LA(1)==BOOL_TRUE) ) {
        zzmatch(BOOL_TRUE);
        
        *pbool = true;
 consume();
      }
      else {
        if ( (LA(1)==BOOL_FALSE) ) {
          zzmatch(BOOL_FALSE);
          
          *pbool = false;
 consume();
        }
        else {FAIL(1,err31,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd10, 0x1);
}

#include "TAXP_ArchitectureXmlGrammar.h"
