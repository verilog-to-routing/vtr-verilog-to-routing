/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 *
 *   ..\..\pccts\bin\Antlr.exe -CC -gh TFP_FabricGrammar.g
 *
 */

#define ANTLR_VERSION	13333
#include "pcctscfg.h"
#include "pccts_stdio.h"
#include "tokens.h"

#include <stdio.h>

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TC_SideName.h"

#include "TFM_FabricModel.h"

#include "TFP_FabricFile.h"
#include "TFP_FabricScanner_c.h"
#include "AParser.h"
#include "TFP_FabricParser_c.h"
#include "DLexerBase.h"
#include "ATokPtr.h"

/* MR23 In order to remove calls to PURIFY use the antlr -nopurify option */

#ifndef PCCTS_PURIFY
#define PCCTS_PURIFY(r,s) memset((char *) &(r),'\0',(s));
#endif


void
TFP_FabricParser_c::start(void)
{
  zzRULE;
  zzmatch(58); consume();
  zzmatch(FABRIC); consume();
  stringText(  &this->pfabricModel_->srName  );
  zzmatch(59); consume();
  {
    while ( (LA(1)==58) ) {
      zzmatch(58); consume();
      {
        if ( (LA(1)==CONFIG) ) {
          configDef(  &pfabricModel_->config  );
        }
        else {
          if ( (LA(1)==IO) ) {
            inputOutputList(  &pfabricModel_->inputOutputList  );
          }
          else {
            if ( (LA(1)==PB) ) {
              physicalBlockList(  &pfabricModel_->physicalBlockList  );
            }
            else {
              if ( (LA(1)==SB) ) {
                switchBoxList(  &pfabricModel_->switchBoxList  );
              }
              else {
                if ( (LA(1)==CHANNEL) ) {
                  channelList(  &pfabricModel_->channelList  );
                }
                else {
                  if ( (LA(1)==SEGMENT) ) {
                    segmentList(  &pfabricModel_->segmentList  );
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
  zzmatch(60); consume();
  zzmatch(FABRIC); consume();
  zzmatch(59); consume();
  zzmatch(END_OF_FILE); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x1);
}

void
TFP_FabricParser_c::configDef(TFM_Config_c* pconfig)
{
  zzRULE;
  zzmatch(CONFIG); consume();
  zzmatch(59); consume();
  {
    zzmatch(58); consume();
    {
      while ( (LA(1)==REGION) ) {
        zzmatch(REGION); consume();
        regionDef(  &pconfig->fabricRegion  );
      }
    }
    zzmatch(59); consume();
  }
  zzmatch(60); consume();
  zzmatch(CONFIG); consume();
  zzmatch(59); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x2);
}

void
TFP_FabricParser_c::inputOutputList(TFM_InputOutputList_t* pinputOutputList)
{
  zzRULE;
  
  TFM_InputOutput_t inputOutput;
  
      TC_SideName_c channel;
  zzmatch(IO); consume();
  stringText(  &inputOutput.srName  );
  {
    while ( (setwd1[LA(1)]&0x4) ) {
      {
        if ( (LA(1)==CELL) ) {
          zzmatch(CELL); consume();
        }
        else {
          if ( (LA(1)==MASTER) ) {
            zzmatch(MASTER); consume();
          }
          else {FAIL(1,err2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd1[LA(1)]&0x8) ) {
          }
          else {FAIL(1,err3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      stringText(  &inputOutput.srMasterName  );
    }
  }
  zzmatch(59); consume();
  {
    while ( (LA(1)==58) ) {
      zzmatch(58); consume();
      {
        if ( (LA(1)==REGION) ) {
          zzmatch(REGION); consume();
          regionDef(  &inputOutput.region  );
          zzmatch(59); consume();
        }
        else {
          if ( (LA(1)==PIN) ) {
            zzmatch(PIN); consume();
            pinList(  &inputOutput.pinList  );
            {
              if ( (LA(1)==61) ) {
                zzmatch(61); consume();
              }
              else {
                if ( (LA(1)==60) ) {
                  zzmatch(60); consume();
                  zzmatch(PIN); consume();
                  zzmatch(59); consume();
                }
                else {FAIL(1,err4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
          else {
            if ( (LA(1)==SLICE) ) {
              zzmatch(SLICE); consume();
              {
                for (;;) {
                  if ( !((setwd1[LA(1)]&0x10))) break;
                  if ( (LA(1)==COUNT) ) {
                    zzmatch(COUNT); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd1[LA(1)]&0x20) ) {
                        }
                        else {FAIL(1,err5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    uintNum(  &inputOutput.slice.count  );
                  }
                  else {
                    if ( (LA(1)==CAPACITY) ) {
                      zzmatch(CAPACITY); consume();
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd1[LA(1)]&0x40) ) {
                          }
                          else {FAIL(1,err6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      uintNum(  &inputOutput.slice.capacity  );
                    }
                    else break; /* MR6 code for exiting loop "for sure" */
                  }
                }
              }
              zzmatch(59); consume();
            }
            else {
              if ( (LA(1)==TIMING) ) {
                zzmatch(TIMING); consume();
                {
                  for (;;) {
                    if ( !((setwd1[LA(1)]&0x80))) break;
                    if ( (setwd2[LA(1)]&0x1) ) {
                      {
                        if ( (LA(1)==CAP) ) {
                          zzmatch(CAP); consume();
                        }
                        else {
                          if ( (LA(1)==CAP_IN) ) {
                            zzmatch(CAP_IN); consume();
                          }
                          else {FAIL(1,err7,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd2[LA(1)]&0x2) ) {
                          }
                          else {FAIL(1,err8,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      floatNum(  &inputOutput.timing.capInput  );
                    }
                    else {
                      if ( (setwd2[LA(1)]&0x4) ) {
                        {
                          if ( (LA(1)==T) ) {
                            zzmatch(T); consume();
                          }
                          else {
                            if ( (LA(1)==DELAY) ) {
                              zzmatch(DELAY); consume();
                            }
                            else {FAIL(1,err9,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        {
                          if ( (LA(1)==EQUAL) ) {
                            zzmatch(EQUAL); consume();
                          }
                          else {
                            if ( (setwd2[LA(1)]&0x8) ) {
                            }
                            else {FAIL(1,err10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        expNum(  &inputOutput.timing.delay  );
                      }
                      else break; /* MR6 code for exiting loop "for sure" */
                    }
                  }
                }
                zzmatch(59); consume();
              }
              else {FAIL(1,err11,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
      }
    }
  }
  zzmatch(60); consume();
  zzmatch(IO); consume();
  zzmatch(59);
  
  if( inputOutput.IsValid( ))
  {
    inputOutput.blockType = TFM_BLOCK_INPUT_OUTPUT;
    pinputOutputList->Add( inputOutput );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x10);
}

void
TFP_FabricParser_c::physicalBlockList(TFM_PhysicalBlockList_t* pphysicalBlockList)
{
  zzRULE;
  
  TFM_PhysicalBlock_t physicalBlock;
  zzmatch(PB); consume();
  stringText(  &physicalBlock.srName  );
  {
    while ( (setwd2[LA(1)]&0x20) ) {
      {
        if ( (LA(1)==CELL) ) {
          zzmatch(CELL); consume();
        }
        else {
          if ( (LA(1)==MASTER) ) {
            zzmatch(MASTER); consume();
          }
          else {FAIL(1,err12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd2[LA(1)]&0x40) ) {
          }
          else {FAIL(1,err13,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      stringText(  &physicalBlock.srMasterName  );
    }
  }
  zzmatch(59); consume();
  {
    while ( (LA(1)==58) ) {
      zzmatch(58); consume();
      {
        if ( (LA(1)==REGION) ) {
          zzmatch(REGION); consume();
          regionDef(  &physicalBlock.region  );
          zzmatch(59); consume();
        }
        else {
          if ( (LA(1)==PIN) ) {
            zzmatch(PIN); consume();
            pinList(  &physicalBlock.pinList  );
            {
              if ( (LA(1)==61) ) {
                zzmatch(61); consume();
              }
              else {
                if ( (LA(1)==60) ) {
                  zzmatch(60); consume();
                  zzmatch(PIN); consume();
                  zzmatch(59); consume();
                }
                else {FAIL(1,err14,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
          else {
            if ( (LA(1)==SLICE) ) {
              zzmatch(SLICE); consume();
              {
                for (;;) {
                  if ( !((setwd2[LA(1)]&0x80))) break;
                  if ( (LA(1)==COUNT) ) {
                    zzmatch(COUNT); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd3[LA(1)]&0x1) ) {
                        }
                        else {FAIL(1,err15,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    uintNum(  &physicalBlock.slice.count  );
                  }
                  else {
                    if ( (LA(1)==CAPACITY) ) {
                      zzmatch(CAPACITY); consume();
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd3[LA(1)]&0x2) ) {
                          }
                          else {FAIL(1,err16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      uintNum(  &physicalBlock.slice.capacity  );
                    }
                    else break; /* MR6 code for exiting loop "for sure" */
                  }
                }
              }
              zzmatch(59); consume();
            }
            else {
              if ( (LA(1)==TIMING) ) {
                zzmatch(TIMING); consume();
                {
                  for (;;) {
                    if ( !((setwd3[LA(1)]&0x4))) break;
                    if ( (setwd3[LA(1)]&0x8) ) {
                      {
                        if ( (LA(1)==CAP) ) {
                          zzmatch(CAP); consume();
                        }
                        else {
                          if ( (LA(1)==CAP_IN) ) {
                            zzmatch(CAP_IN); consume();
                          }
                          else {FAIL(1,err17,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd3[LA(1)]&0x10) ) {
                          }
                          else {FAIL(1,err18,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      floatNum(  &physicalBlock.timing.capInput  );
                    }
                    else {
                      if ( (setwd3[LA(1)]&0x20) ) {
                        {
                          if ( (LA(1)==T) ) {
                            zzmatch(T); consume();
                          }
                          else {
                            if ( (LA(1)==DELAY) ) {
                              zzmatch(DELAY); consume();
                            }
                            else {FAIL(1,err19,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        {
                          if ( (LA(1)==EQUAL) ) {
                            zzmatch(EQUAL); consume();
                          }
                          else {
                            if ( (setwd3[LA(1)]&0x40) ) {
                            }
                            else {FAIL(1,err20,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        expNum(  &physicalBlock.timing.delay  );
                      }
                      else break; /* MR6 code for exiting loop "for sure" */
                    }
                  }
                }
                zzmatch(59); consume();
              }
              else {FAIL(1,err21,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
      }
    }
  }
  zzmatch(60); consume();
  zzmatch(PB); consume();
  zzmatch(59);
  
  if( physicalBlock.IsValid( ))
  {
    physicalBlock.blockType = TFM_BLOCK_PHYSICAL_BLOCK;
    pphysicalBlockList->Add( physicalBlock );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x80);
}

void
TFP_FabricParser_c::switchBoxList(TFM_SwitchBoxList_t* pswitchBoxList)
{
  zzRULE;
  
  TFM_SwitchBox_t switchBox;
  
      TC_SideName_c channel;
  zzmatch(SB); consume();
  stringText(  &switchBox.srName  );
  {
    while ( (setwd4[LA(1)]&0x1) ) {
      {
        if ( (LA(1)==CELL) ) {
          zzmatch(CELL); consume();
        }
        else {
          if ( (LA(1)==MASTER) ) {
            zzmatch(MASTER); consume();
          }
          else {FAIL(1,err22,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd4[LA(1)]&0x2) ) {
          }
          else {FAIL(1,err23,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      stringText(  &switchBox.srMasterName  );
    }
  }
  zzmatch(59); consume();
  {
    while ( (LA(1)==58) ) {
      zzmatch(58); consume();
      {
        if ( (LA(1)==REGION) ) {
          zzmatch(REGION); consume();
          regionDef(  &switchBox.region  );
        }
        else {
          if ( (LA(1)==MAPPING) ) {
            zzmatch(MAPPING); consume();
            sideMapTable(  &switchBox.mapTable  );
          }
          else {
            if ( (LA(1)==TIMING) ) {
              zzmatch(TIMING); consume();
              {
                for (;;) {
                  if ( !((setwd4[LA(1)]&0x4))) break;
                  if ( (setwd4[LA(1)]&0x8) ) {
                    {
                      if ( (LA(1)==R) ) {
                        zzmatch(R); consume();
                      }
                      else {
                        if ( (LA(1)==RES) ) {
                          zzmatch(RES); consume();
                        }
                        else {FAIL(1,err24,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd4[LA(1)]&0x10) ) {
                        }
                        else {FAIL(1,err25,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    floatNum(  &switchBox.timing.res  );
                  }
                  else {
                    if ( (setwd4[LA(1)]&0x20) ) {
                      {
                        if ( (LA(1)==CAP) ) {
                          zzmatch(CAP); consume();
                        }
                        else {
                          if ( (LA(1)==CAP_IN) ) {
                            zzmatch(CAP_IN); consume();
                          }
                          else {FAIL(1,err26,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd4[LA(1)]&0x40) ) {
                          }
                          else {FAIL(1,err27,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      floatNum(  &switchBox.timing.capInput  );
                    }
                    else {
                      if ( (LA(1)==CAP_OUT) ) {
                        zzmatch(CAP_OUT); consume();
                        {
                          if ( (LA(1)==EQUAL) ) {
                            zzmatch(EQUAL); consume();
                          }
                          else {
                            if ( (setwd4[LA(1)]&0x80) ) {
                            }
                            else {FAIL(1,err28,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        floatNum(  &switchBox.timing.capOutput  );
                      }
                      else {
                        if ( (setwd5[LA(1)]&0x1) ) {
                          {
                            if ( (LA(1)==T) ) {
                              zzmatch(T); consume();
                            }
                            else {
                              if ( (LA(1)==DELAY) ) {
                                zzmatch(DELAY); consume();
                              }
                              else {FAIL(1,err29,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                            }
                          }
                          {
                            if ( (LA(1)==EQUAL) ) {
                              zzmatch(EQUAL); consume();
                            }
                            else {
                              if ( (setwd5[LA(1)]&0x2) ) {
                              }
                              else {FAIL(1,err30,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                            }
                          }
                          expNum(  &switchBox.timing.delay  );
                        }
                        else break; /* MR6 code for exiting loop "for sure" */
                      }
                    }
                  }
                }
              }
            }
            else {FAIL(1,err31,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
      zzmatch(59); consume();
    }
  }
  zzmatch(60); consume();
  zzmatch(SB); consume();
  zzmatch(59);
  
  if( switchBox.IsValid( ))
  {
    switchBox.blockType = TFM_BLOCK_SWITCH_BOX;
    pswitchBoxList->Add( switchBox );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x4);
}

void
TFP_FabricParser_c::channelList(TFM_ChannelList_t* pchannelList)
{
  zzRULE;
  ANTLRTokenPtr orientVal=NULL;
  
  TFM_Channel_c channel;
  zzmatch(CHANNEL); consume();
  stringText(  &channel.srName  );
  {
    for (;;) {
      if ( !((setwd5[LA(1)]&0x8))) break;
      if ( (LA(1)==ORIENT) ) {
        zzmatch(ORIENT); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd5[LA(1)]&0x10) ) {
            }
            else {FAIL(1,err32,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzsetmatch(ORIENT_VAL_set, ORIENT_VAL_errset);
        orientVal = (ANTLRTokenPtr)LT(1);

        
        channel.orient = this->FindOrientMode_( orientVal->getType( ));
 consume();
      }
      else {
        if ( (LA(1)==COUNT) ) {
          zzmatch(COUNT); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd5[LA(1)]&0x20) ) {
              }
              else {FAIL(1,err35,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          uintNum(  &channel.count  );
        }
        else break; /* MR6 code for exiting loop "for sure" */
      }
    }
  }
  zzmatch(59); consume();
  {
    while ( (LA(1)==58) ) {
      zzmatch(58); consume();
      zzmatch(REGION); consume();
      regionDef(  &channel.region  );
      zzmatch(59); consume();
    }
  }
  zzmatch(60); consume();
  zzmatch(CHANNEL); consume();
  zzmatch(59);
  
  if( channel.IsValid( ))
  {
    pchannelList->Add( channel );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x40);
}

void
TFP_FabricParser_c::segmentList(TFM_SegmentList_t* psegmentList)
{
  zzRULE;
  
  TFM_Segment_c segment;
  zzmatch(SEGMENT); consume();
  stringText(  &segment.srName  );
  {
    for (;;) {
      if ( !((setwd5[LA(1)]&0x80))) break;
      if ( (LA(1)==INDEX) ) {
        zzmatch(INDEX); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd6[LA(1)]&0x1) ) {
            }
            else {FAIL(1,err36,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        uintNum(  &segment.index  );
      }
      else {
        if ( (LA(1)==WIDTH) ) {
          zzmatch(WIDTH); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd6[LA(1)]&0x2) ) {
              }
              else {FAIL(1,err37,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          floatNum(  &segment.path.width  );
        }
        else break; /* MR6 code for exiting loop "for sure" */
      }
    }
  }
  zzmatch(59); consume();
  {
    while ( (LA(1)==58) ) {
      zzmatch(58); consume();
      {
        if ( (LA(1)==LINE) ) {
          zzmatch(LINE); consume();
          lineDef(  &segment.path.line  );
        }
        else {
          if ( (LA(1)==TIMING) ) {
            zzmatch(TIMING); consume();
            {
              for (;;) {
                if ( !((setwd6[LA(1)]&0x4))) break;
                if ( (setwd6[LA(1)]&0x8) ) {
                  {
                    if ( (LA(1)==R) ) {
                      zzmatch(R); consume();
                    }
                    else {
                      if ( (LA(1)==RES) ) {
                        zzmatch(RES); consume();
                      }
                      else {FAIL(1,err38,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd6[LA(1)]&0x10) ) {
                      }
                      else {FAIL(1,err39,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  floatNum(  &segment.timing.res  );
                }
                else {
                  if ( (LA(1)==CAP) ) {
                    zzmatch(CAP); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd6[LA(1)]&0x20) ) {
                        }
                        else {FAIL(1,err40,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    floatNum(  &segment.timing.cap  );
                  }
                  else break; /* MR6 code for exiting loop "for sure" */
                }
              }
            }
          }
          else {FAIL(1,err41,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      zzmatch(59); consume();
    }
  }
  zzmatch(60); consume();
  zzmatch(SEGMENT); consume();
  zzmatch(59);
  
  if( segment.IsValid( ))
  {
    psegmentList->Add( segment );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd6, 0x40);
}

void
TFP_FabricParser_c::pinList(TFM_PinList_t* ppinList)
{
  zzRULE;
  ANTLRTokenPtr sideVal=NULL;
  
  TFM_Pin_c pin;
  
      string srName;
  stringText(  &srName  );
  
  pin.SetName( srName );
  {
    for (;;) {
      if ( !((setwd6[LA(1)]&0x80))) break;
      if ( (LA(1)==SIDE) ) {
        zzmatch(SIDE); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd7[LA(1)]&0x1) ) {
            }
            else {FAIL(1,err42,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzsetmatch(SIDE_VAL_set, SIDE_VAL_errset);
        sideVal = (ANTLRTokenPtr)LT(1);

        
        pin.side = this->FindSideMode_( sideVal->getType( ));
 consume();
      }
      else {
        if ( (LA(1)==OFFSET) ) {
          zzmatch(OFFSET); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd7[LA(1)]&0x2) ) {
              }
              else {FAIL(1,err45,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          floatNum(  &pin.offset  );
        }
        else {
          if ( (LA(1)==WIDTH) ) {
            zzmatch(WIDTH); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd7[LA(1)]&0x4) ) {
                }
                else {FAIL(1,err46,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            floatNum(  &pin.width  );
          }
          else {
            if ( (LA(1)==SLICE) ) {
              zzmatch(SLICE); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd7[LA(1)]&0x8) ) {
                  }
                  else {FAIL(1,err47,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              uintNum(  &pin.slice  );
            }
            else break; /* MR6 code for exiting loop "for sure" */
          }
        }
      }
    }
  }
  {
    while ( (LA(1)==59) ) {
      zzmatch(59); consume();
      connectionPattern(  &pin.connectionPattern  );
    }
  }
  
  if( pin.IsValid( ))
  {
    ppinList->Add( pin );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x10);
}

void
TFP_FabricParser_c::connectionPattern(TFM_BitPattern_t* pconnectionPattern)
{
  zzRULE;
  ANTLRTokenPtr bitStringVal=NULL;
  
  string srPattern;
  zzmatch(58); consume();
  zzmatch(CB); consume();
  {
    while ( (LA(1)==BIT_CHAR) ) {
      zzmatch(BIT_CHAR);
      bitStringVal = (ANTLRTokenPtr)LT(1);

      
      string srBit = bitStringVal->getText( );
      if( srBit.length( ))
      {
        TC_Bit_c bit( srBit[ 0 ] );
        pconnectionPattern->Add( bit );
      }
 consume();
    }
  }
  zzmatch(59); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x20);
}

void
TFP_FabricParser_c::sideMapTable(TC_MapTable_c* pmapTable)
{
  zzRULE;
  
  TC_SideIndex_c sideIndex;
  TC_SideMode_t side = TC_SIDE_UNDEFINED;
  size_t index;
  
      TC_SideList_t sideList;
  
      size_t curTokenLine, nextTokenLine;
  {
    int zzcnt=1;
    do {
      sideMapIndex(  &sideIndex  );
      
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
    } while ( (setwd7[LA(1)]&0x40) );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x80);
}

void
TFP_FabricParser_c::sideMapIndex(TC_SideIndex_c* psideIndex)
{
  zzRULE;
  ANTLRTokenPtr sideVal=NULL;
  
  TC_SideMode_t side = TC_SIDE_UNDEFINED;
  unsigned int index = 0;
  zzsetmatch(SIDE_VAL_set, SIDE_VAL_errset);
  sideVal = (ANTLRTokenPtr)LT(1);

  
  side = this->FindSideMode_( sideVal->getType( ));
 consume();
  uintNum(  &index  );
  
  psideIndex->Set( side, index );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x1);
}

void
TFP_FabricParser_c::regionDef(TGS_Region_c* pregion)
{
  zzRULE;
  floatNum(  &pregion->x1  );
  floatNum(  &pregion->y1  );
  floatNum(  &pregion->x2  );
  floatNum(  &pregion->y2  );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x2);
}

void
TFP_FabricParser_c::lineDef(TGS_Line_c* pline)
{
  zzRULE;
  floatNum(  &pline->x1  );
  floatNum(  &pline->y1  );
  floatNum(  &pline->x2  );
  floatNum(  &pline->y2  );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x4);
}

void
TFP_FabricParser_c::stringText(string* psrString)
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
        else {FAIL(1,err48,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
    else {FAIL(1,err49,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x8);
}

void
TFP_FabricParser_c::floatNum(double* pdouble)
{
  zzRULE;
  ANTLRTokenPtr floatVal=NULL, posIntVal=NULL, negIntVal=NULL, bitVal=NULL;
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
      else {
        if ( (LA(1)==BIT_CHAR) ) {
          zzmatch(BIT_CHAR);
          bitVal = (ANTLRTokenPtr)LT(1);

          
          *pdouble = atof( bitVal->getText( ));
 consume();
        }
        else {FAIL(1,err50,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x10);
}

void
TFP_FabricParser_c::expNum(double* pdouble)
{
  zzRULE;
  ANTLRTokenPtr expVal=NULL, floatVal=NULL, posIntVal=NULL, negIntVal=NULL, bitVal=NULL;
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
        else {
          if ( (LA(1)==BIT_CHAR) ) {
            zzmatch(BIT_CHAR);
            bitVal = (ANTLRTokenPtr)LT(1);

            
            *pdouble = atof( bitVal->getText( ));
 consume();
          }
          else {FAIL(1,err51,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x20);
}

void
TFP_FabricParser_c::uintNum(unsigned int* puint)
{
  zzRULE;
  ANTLRTokenPtr uintVal=NULL, bitVal=NULL;
  if ( (LA(1)==POS_INT) ) {
    zzmatch(POS_INT);
    uintVal = (ANTLRTokenPtr)LT(1);

    
    *puint = static_cast< unsigned int >( atol( uintVal->getText( )));
 consume();
  }
  else {
    if ( (LA(1)==BIT_CHAR) ) {
      zzmatch(BIT_CHAR);
      bitVal = (ANTLRTokenPtr)LT(1);

      
      *puint = static_cast< unsigned int >( atol( bitVal->getText( )));
 consume();
    }
    else {FAIL(1,err52,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x40);
}

#include "TFP_FabricGrammar.h"
