/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 *
 *   ..\..\pccts\bin\Antlr.exe -CC -gh TAP_ArchitectureGrammar.g
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

#include "TAP_ArchitectureFile.h"
#include "TAP_ArchitectureScanner_c.h"
#include "AParser.h"
#include "TAP_ArchitectureParser_c.h"
#include "DLexerBase.h"
#include "ATokPtr.h"

/* MR23 In order to remove calls to PURIFY use the antlr -nopurify option */

#ifndef PCCTS_PURIFY
#define PCCTS_PURIFY(r,s) memset((char *) &(r),'\0',(s));
#endif


void
TAP_ArchitectureParser_c::start(void)
{
  zzRULE;
  zzmatch(120); consume();
  zzmatch(ARCHITECTURE); consume();
  stringText(  &this->parchitectureSpec_->srName  );
  zzmatch(121); consume();
  {
    while ( (LA(1)==120) ) {
      zzmatch(120); consume();
      {
        if ( (LA(1)==CONFIG) ) {
          configDef(  &this->parchitectureSpec_->config  );
        }
        else {
          if ( (LA(1)==IO) ) {
            inputOutputList(  &this->parchitectureSpec_->physicalBlockList  );
          }
          else {
            if ( (LA(1)==PB) ) {
              physicalBlockList(  &this->parchitectureSpec_->physicalBlockList  );
            }
            else {
              if ( (LA(1)==MODEL) ) {
                modeList(  &this->parchitectureSpec_->modeList  );
              }
              else {
                if ( (LA(1)==SB) ) {
                  switchBoxList(  &this->parchitectureSpec_->switchBoxList  );
                }
                else {
                  if ( (LA(1)==SEGMENT) ) {
                    segmentList(  &this->parchitectureSpec_->segmentList  );
                  }
                  else {
                    if ( (LA(1)==CELL) ) {
                      cellList(  &this->parchitectureSpec_->cellList  );
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
  }
  zzmatch(122); consume();
  zzmatch(ARCHITECTURE); consume();
  zzmatch(121); consume();
  zzmatch(END_OF_FILE); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x1);
}

void
TAP_ArchitectureParser_c::configDef(TAS_Config_c* pconfig)
{
  zzRULE;
  ANTLRTokenPtr arraySizeModeVal=NULL, modelTypeVal=NULL, dirTypeVal=NULL;
  zzmatch(CONFIG); consume();
  zzmatch(121); consume();
  {
    while ( (LA(1)==120) ) {
      zzmatch(120); consume();
      {
        if ( (LA(1)==SIZE) ) {
          zzmatch(SIZE); consume();
          zzsetmatch(ARRAY_SIZE_MODE_VAL_set, ARRAY_SIZE_MODE_VAL_errset);
          arraySizeModeVal = (ANTLRTokenPtr)LT(1);

          
          pconfig->layout.sizeMode = this->FindArraySizeMode_( arraySizeModeVal->getType( ));
 consume();
          {
            for (;;) {
              if ( !((setwd1[LA(1)]&0x2))) break;
              if ( (LA(1)==RATIO) ) {
                zzmatch(RATIO); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd1[LA(1)]&0x4) ) {
                    }
                    else {FAIL(1,err4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                floatNum(  &pconfig->layout.autoSize.aspectRatio  );
              }
              else {
                if ( (LA(1)==WIDTH) ) {
                  zzmatch(WIDTH); consume();
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd1[LA(1)]&0x8) ) {
                      }
                      else {FAIL(1,err5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  intNum(  &pconfig->layout.manualSize.gridDims.width  );
                }
                else {
                  if ( (LA(1)==HEIGHT) ) {
                    zzmatch(HEIGHT); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd1[LA(1)]&0x10) ) {
                        }
                        else {FAIL(1,err6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    intNum(  &pconfig->layout.manualSize.gridDims.height  );
                  }
                  else break; /* MR6 code for exiting loop "for sure" */
                }
              }
            }
          }
        }
        else {
          if ( (LA(1)==SB) ) {
            zzmatch(SB); consume();
            zzsetmatch(SB_MODEL_TYPE_VAL_set, SB_MODEL_TYPE_VAL_errset);
            modelTypeVal = (ANTLRTokenPtr)LT(1);

            
            pconfig->device.switchBoxes.modelType = this->FindSwitchBoxModelType_( modelTypeVal->getType( ));
 consume();
          }
          else {
            if ( (LA(1)==CB) ) {
              zzmatch(CB); consume();
              {
                for (;;) {
                  if ( !((setwd1[LA(1)]&0x20))) break;
                  if ( (setwd1[LA(1)]&0x40) ) {
                    {
                      if ( (LA(1)==CAP) ) {
                        zzmatch(CAP); consume();
                      }
                      else {
                        if ( (LA(1)==CAP_IN) ) {
                          zzmatch(CAP_IN); consume();
                        }
                        else {FAIL(1,err9,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd1[LA(1)]&0x80) ) {
                        }
                        else {FAIL(1,err10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    floatNum(  &pconfig->device.connectionBoxes.capInput  );
                  }
                  else {
                    if ( (setwd2[LA(1)]&0x1) ) {
                      {
                        if ( (LA(1)==T) ) {
                          zzmatch(T); consume();
                        }
                        else {
                          if ( (LA(1)==DELAY) ) {
                            zzmatch(DELAY); consume();
                          }
                          else {FAIL(1,err11,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd2[LA(1)]&0x2) ) {
                          }
                          else {FAIL(1,err12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      expNum(  &pconfig->device.connectionBoxes.delayInput  );
                    }
                    else break; /* MR6 code for exiting loop "for sure" */
                  }
                }
              }
            }
            else {
              if ( (LA(1)==SEGMENT) ) {
                zzmatch(SEGMENT); consume();
                zzsetmatch(SEGMENT_DIR_TYPE_VAL_set, SEGMENT_DIR_TYPE_VAL_errset);
                dirTypeVal = (ANTLRTokenPtr)LT(1);

                
                pconfig->device.segments.dirType = this->FindSegmentDirType_( dirTypeVal->getType( ));
 consume();
              }
              else {FAIL(1,err15,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
      }
      zzmatch(121); consume();
    }
  }
  zzmatch(122); consume();
  zzmatch(CONFIG); consume();
  zzmatch(121); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x4);
}

void
TAP_ArchitectureParser_c::inputOutputList(TAS_InputOutputList_t* pinputOutputList)
{
  zzRULE;
  
  TAS_InputOutput_t inputOutput;
  
      TC_FloatDims_t dims;
  TGS_Point_c origin;
  zzmatch(IO); consume();
  stringText(  &inputOutput.srName  );
  {
    for (;;) {
      if ( !((setwd2[LA(1)]&0x8))) break;
      if ( (LA(1)==CAPACITY) ) {
        zzmatch(CAPACITY); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd2[LA(1)]&0x10) ) {
            }
            else {FAIL(1,err16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        uintNum(  &inputOutput.capacity  );
      }
      else {
        if ( (LA(1)==FC_IN) ) {
          zzmatch(FC_IN); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd2[LA(1)]&0x20) ) {
              }
              else {FAIL(1,err17,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          fcDef(  &inputOutput.fcIn  );
        }
        else {
          if ( (LA(1)==FC_OUT) ) {
            zzmatch(FC_OUT); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd2[LA(1)]&0x40) ) {
                }
                else {FAIL(1,err18,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            fcDef(  &inputOutput.fcOut  );
          }
          else {
            if ( (LA(1)==SIZE) ) {
              zzmatch(SIZE); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd2[LA(1)]&0x80) ) {
                  }
                  else {FAIL(1,err19,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              floatDims(  &dims  );
            }
            else {
              if ( (LA(1)==ORIGIN) ) {
                zzmatch(ORIGIN); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd3[LA(1)]&0x1) ) {
                    }
                    else {FAIL(1,err20,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                originPoint(  &origin  );
              }
              else break; /* MR6 code for exiting loop "for sure" */
            }
          }
        }
      }
    }
  }
  zzmatch(121); consume();
  {
    while ( (LA(1)==120) ) {
      zzmatch(120); consume();
      {
        if ( (LA(1)==MODEL) ) {
          zzmatch(MODEL); consume();
          modeNameList(  &inputOutput.modeNameList  );
          zzmatch(121); consume();
        }
        else {
          if ( (LA(1)==PIN) ) {
            zzmatch(PIN); consume();
            pinList(  &inputOutput.portList  );
            zzmatch(123); consume();
          }
          else {
            if ( (LA(1)==TIMING) ) {
              zzmatch(TIMING); consume();
              timingDelayLists(  &inputOutput.timingDelayLists  );
              zzmatch(121); consume();
            }
            else {
              if ( (LA(1)==PIN_ASSIGN) ) {
                zzmatch(PIN_ASSIGN); consume();
                pinAssignList(  &inputOutput.pinAssignPattern,
                                   &inputOutput.pinAssignList  );
                zzmatch(123); consume();
              }
              else {
                if ( (LA(1)==GRID_ASSIGN) ) {
                  zzmatch(GRID_ASSIGN); consume();
                  gridAssignList(  &inputOutput.gridAssignList  );
                  zzmatch(121); consume();
                }
                else {FAIL(1,err21,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
        }
      }
    }
  }
  zzmatch(122); consume();
  zzmatch(IO); consume();
  zzmatch(121);
  
  if( inputOutput.IsValid( ))
  {
    inputOutput.SetUsage( TAS_USAGE_INPUT_OUTPUT );
    inputOutput.SetDims( dims );
    inputOutput.SetOrigin( origin );
    
         pinputOutputList->Add( inputOutput );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x2);
}

void
TAP_ArchitectureParser_c::physicalBlockList(TAS_PhysicalBlockList_t* pphysicalBlockList)
{
  zzRULE;
  
  TAS_PhysicalBlock_c physicalBlock;
  
      TC_FloatDims_t dims;
  TGS_Point_c origin;
  zzmatch(PB); consume();
  stringText(  &physicalBlock.srName  );
  {
    for (;;) {
      if ( !((setwd3[LA(1)]&0x4))) break;
      if ( (LA(1)==COUNT) ) {
        zzmatch(COUNT); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd3[LA(1)]&0x8) ) {
            }
            else {FAIL(1,err22,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        uintNum(  &physicalBlock.numPB  );
      }
      else {
        if ( (LA(1)==CELL) ) {
          zzmatch(CELL); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd3[LA(1)]&0x10) ) {
              }
              else {FAIL(1,err23,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          cellModelText(  &physicalBlock.srModelName,
                                    &physicalBlock.modelType  );
        }
        else {
          if ( (LA(1)==FC_IN) ) {
            zzmatch(FC_IN); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd3[LA(1)]&0x20) ) {
                }
                else {FAIL(1,err24,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            fcDef(  &physicalBlock.fcIn  );
          }
          else {
            if ( (LA(1)==FC_OUT) ) {
              zzmatch(FC_OUT); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd3[LA(1)]&0x40) ) {
                  }
                  else {FAIL(1,err25,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              fcDef(  &physicalBlock.fcOut  );
            }
            else {
              if ( (LA(1)==SIZE) ) {
                zzmatch(SIZE); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd3[LA(1)]&0x80) ) {
                    }
                    else {FAIL(1,err26,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                floatDims(  &dims  );
              }
              else {
                if ( (LA(1)==ORIGIN) ) {
                  zzmatch(ORIGIN); consume();
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd4[LA(1)]&0x1) ) {
                      }
                      else {FAIL(1,err27,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  originPoint(  &origin  );
                }
                else break; /* MR6 code for exiting loop "for sure" */
              }
            }
          }
        }
      }
    }
  }
  zzmatch(121); consume();
  {
    while ( (LA(1)==120) ) {
      zzmatch(120); consume();
      {
        if ( (LA(1)==MODEL) ) {
          zzmatch(MODEL); consume();
          modeNameList(  &physicalBlock.modeNameList  );
          zzmatch(121); consume();
        }
        else {
          if ( (LA(1)==PIN) ) {
            zzmatch(PIN); consume();
            pinList(  &physicalBlock.portList  );
            zzmatch(123); consume();
          }
          else {
            if ( (LA(1)==TIMING) ) {
              zzmatch(TIMING); consume();
              timingDelayLists(  &physicalBlock.timingDelayLists  );
              zzmatch(121); consume();
            }
            else {
              if ( (LA(1)==PIN_ASSIGN) ) {
                zzmatch(PIN_ASSIGN); consume();
                pinAssignList(  &physicalBlock.pinAssignPattern,
                                   &physicalBlock.pinAssignList  );
                zzmatch(123); consume();
              }
              else {
                if ( (LA(1)==GRID_ASSIGN) ) {
                  zzmatch(GRID_ASSIGN); consume();
                  gridAssignList(  &physicalBlock.gridAssignList  );
                  zzmatch(121); consume();
                }
                else {FAIL(1,err28,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
        }
      }
    }
  }
  zzmatch(122); consume();
  zzmatch(PB); consume();
  zzmatch(121);
  
  if( physicalBlock.IsValid( ))
  {
    physicalBlock.SetUsage( TAS_USAGE_PHYSICAL_BLOCK );
    
         pphysicalBlockList->Add( physicalBlock );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x2);
}

void
TAP_ArchitectureParser_c::modeList(TAS_ModeList_t* pmodeList)
{
  zzRULE;
  ANTLRTokenPtr mapTypeVal=NULL;
  
  TAS_Mode_c mode;
  
      TAS_PhysicalBlock_c physicalBlock;
  TAS_Interconnect_c interconnect;
  
      string srInputName, srOutputName;
  zzmatch(MODEL); consume();
  stringText(  &mode.srName  );
  zzmatch(121); consume();
  {
    while ( (LA(1)==120) ) {
      zzmatch(120); consume();
      {
        if ( (LA(1)==PB) ) {
          zzmatch(PB); consume();
          stringText(  &physicalBlock.srName  );
          zzmatch(121);
          
          physicalBlock.SetUsage( TAS_USAGE_PHYSICAL_BLOCK );
          mode.physicalBlockList.Add( physicalBlock );
 consume();
        }
        else {
          if ( (LA(1)==INTERCONNECT) ) {
            zzmatch(INTERCONNECT);
            
            interconnect.inputNameList.Clear( );
            interconnect.outputNameList.Clear( );
            interconnect.timingDelayLists.delayList.Clear( );
            interconnect.timingDelayLists.delayMatrixList.Clear( );
            interconnect.timingDelayLists.delayClockSetupList.Clear( );
            interconnect.timingDelayLists.delayClockToQList.Clear( );
 consume();
            stringText(  &interconnect.srName  );
            zzsetmatch(MAP_TYPE_VAL_set, MAP_TYPE_VAL_errset);
            mapTypeVal = (ANTLRTokenPtr)LT(1);

            
            interconnect.mapType = this->FindInterconnectMapType_( mapTypeVal->getType( ));
 consume();
            zzmatch(121); consume();
            {
              while ( (LA(1)==120) ) {
                zzmatch(120); consume();
                {
                  if ( (LA(1)==INPUT) ) {
                    zzmatch(INPUT); consume();
                    stringText(  &srInputName  );
                    zzmatch(121);
                    
                    interconnect.inputNameList.Add( srInputName );
 consume();
                  }
                  else {
                    if ( (LA(1)==OUTPUT) ) {
                      zzmatch(OUTPUT); consume();
                      stringText(  &srOutputName  );
                      zzmatch(121);
                      
                      interconnect.outputNameList.Add( srOutputName );
 consume();
                    }
                    else {
                      if ( (LA(1)==TIMING) ) {
                        zzmatch(TIMING); consume();
                        timingDelayLists(  &interconnect.timingDelayLists  );
                        zzmatch(121); consume();
                      }
                      else {FAIL(1,err31,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                }
              }
            }
            zzmatch(122); consume();
            zzmatch(INTERCONNECT); consume();
            zzmatch(121);
            
            mode.interconnectList.Add( interconnect );
 consume();
          }
          else {FAIL(1,err32,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  zzmatch(122); consume();
  zzmatch(MODEL); consume();
  zzmatch(121);
  
  if( mode.IsValid( ))
  {
    pmodeList->Add( mode );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x4);
}

void
TAP_ArchitectureParser_c::switchBoxList(TAS_SwitchBoxList_t* pswitchBoxList)
{
  zzRULE;
  ANTLRTokenPtr typeVal=NULL, modelTypeVal=NULL;
  
  TAS_SwitchBox_c switchBox;
  
      TC_FloatDims_t dims;
  TGS_Point_c origin;
  TAS_MapTable_t mapTable;
  zzmatch(SB); consume();
  stringText(  &switchBox.srName  );
  {
    for (;;) {
      if ( !((setwd4[LA(1)]&0x8))) break;
      if ( (LA(1)==TYPE) ) {
        zzmatch(TYPE); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd4[LA(1)]&0x10) ) {
            }
            else {FAIL(1,err33,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzsetmatch(SB_TYPE_VAL_set, SB_TYPE_VAL_errset);
        typeVal = (ANTLRTokenPtr)LT(1);

        
        switchBox.type = this->FindSwitchBoxType_( typeVal->getType( ));
 consume();
      }
      else {
        if ( (LA(1)==MODEL) ) {
          zzmatch(MODEL); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd4[LA(1)]&0x20) ) {
              }
              else {FAIL(1,err36,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          zzsetmatch(SB_MODEL_TYPE_VAL_set, SB_MODEL_TYPE_VAL_errset);
          modelTypeVal = (ANTLRTokenPtr)LT(1);

          
          switchBox.model = this->FindSwitchBoxModelType_( modelTypeVal->getType( ));
 consume();
        }
        else {
          if ( (LA(1)==FS) ) {
            zzmatch(FS); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd4[LA(1)]&0x40) ) {
                }
                else {FAIL(1,err37,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            uintNum(  &switchBox.fs  );
          }
          else {
            if ( (LA(1)==SIZE) ) {
              zzmatch(SIZE); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd4[LA(1)]&0x80) ) {
                  }
                  else {FAIL(1,err38,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              floatDims(  &dims  );
            }
            else {
              if ( (LA(1)==ORIGIN) ) {
                zzmatch(ORIGIN); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd5[LA(1)]&0x1) ) {
                    }
                    else {FAIL(1,err39,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                originPoint(  &origin  );
              }
              else break; /* MR6 code for exiting loop "for sure" */
            }
          }
        }
      }
    }
  }
  zzmatch(121); consume();
  {
    while ( (LA(1)==120) ) {
      zzmatch(120); consume();
      {
        if ( (LA(1)==MAPPING) ) {
          zzmatch(MAPPING); consume();
          mapSideTable(  switchBox.fs, &mapTable  );
          zzmatch(123); consume();
        }
        else {
          if ( (LA(1)==TIMING) ) {
            zzmatch(TIMING); consume();
            {
              for (;;) {
                if ( !((setwd5[LA(1)]&0x2))) break;
                if ( (setwd5[LA(1)]&0x4) ) {
                  {
                    if ( (LA(1)==R) ) {
                      zzmatch(R); consume();
                    }
                    else {
                      if ( (LA(1)==RES) ) {
                        zzmatch(RES); consume();
                      }
                      else {FAIL(1,err40,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd5[LA(1)]&0x8) ) {
                      }
                      else {FAIL(1,err41,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  floatNum(  &switchBox.timing.res  );
                }
                else {
                  if ( (setwd5[LA(1)]&0x10) ) {
                    {
                      if ( (LA(1)==CAP) ) {
                        zzmatch(CAP); consume();
                      }
                      else {
                        if ( (LA(1)==CAP_IN) ) {
                          zzmatch(CAP_IN); consume();
                        }
                        else {FAIL(1,err42,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd5[LA(1)]&0x20) ) {
                        }
                        else {FAIL(1,err43,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
                          if ( (setwd5[LA(1)]&0x40) ) {
                          }
                          else {FAIL(1,err44,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      floatNum(  &switchBox.timing.capOutput  );
                    }
                    else {
                      if ( (setwd5[LA(1)]&0x80) ) {
                        {
                          if ( (LA(1)==T) ) {
                            zzmatch(T); consume();
                          }
                          else {
                            if ( (LA(1)==DELAY) ) {
                              zzmatch(DELAY); consume();
                            }
                            else {FAIL(1,err45,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        {
                          if ( (LA(1)==EQUAL) ) {
                            zzmatch(EQUAL); consume();
                          }
                          else {
                            if ( (setwd6[LA(1)]&0x1) ) {
                            }
                            else {FAIL(1,err46,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        floatNum(  &switchBox.timing.delay  );
                      }
                      else break; /* MR6 code for exiting loop "for sure" */
                    }
                  }
                }
              }
            }
            zzmatch(121); consume();
          }
          else {FAIL(1,err47,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  zzmatch(122); consume();
  zzmatch(SB); consume();
  zzmatch(121);
  
  if( switchBox.IsValid( ))
  {
    switchBox.SetDims( dims );
    switchBox.SetOrigin( origin );
    switchBox.SetMapTable( mapTable );
    
         pswitchBoxList->Add( switchBox );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd6, 0x2);
}

void
TAP_ArchitectureParser_c::segmentList(TAS_SegmentList_t* psegmentList)
{
  zzRULE;
  
  TAS_Segment_c segment;
  zzmatch(SEGMENT); consume();
  {
    while ( (LA(1)==LENGTH) ) {
      zzmatch(LENGTH); consume();
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd6[LA(1)]&0x4) ) {
          }
          else {FAIL(1,err48,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      segmentLength(  &segment.length  );
    }
  }
  zzmatch(121); consume();
  {
    while ( (LA(1)==120) ) {
      zzmatch(120); consume();
      zzmatch(TIMING); consume();
      {
        for (;;) {
          if ( !((setwd6[LA(1)]&0x8))) break;
          if ( (setwd6[LA(1)]&0x10) ) {
            {
              if ( (LA(1)==R) ) {
                zzmatch(R); consume();
              }
              else {
                if ( (LA(1)==RES) ) {
                  zzmatch(RES); consume();
                }
                else {FAIL(1,err49,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd6[LA(1)]&0x20) ) {
                }
                else {FAIL(1,err50,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
                  if ( (setwd6[LA(1)]&0x40) ) {
                  }
                  else {FAIL(1,err51,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              floatNum(  &segment.timing.cap  );
            }
            else break; /* MR6 code for exiting loop "for sure" */
          }
        }
      }
      zzmatch(121); consume();
    }
  }
  zzmatch(122); consume();
  zzmatch(SEGMENT); consume();
  zzmatch(121);
  
  if( segment.IsValid( ))
  {
    psegmentList->Add( segment );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd6, 0x80);
}

void
TAP_ArchitectureParser_c::cellList(TAS_CellList_t* pcellList)
{
  zzRULE;
  ANTLRTokenPtr classTypeVal=NULL, typeVal=NULL;
  
  TAS_Cell_c cell;
  TLO_Port_c port;
  
      string srName;
  TC_TypeMode_t type = TC_TYPE_UNDEFINED;
  zzmatch(CELL); consume();
  stringText(  &srName  );
  
  cell.SetName( srName );
  {
    while ( (LA(1)==CLASS) ) {
      zzmatch(CLASS); consume();
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd7[LA(1)]&0x1) ) {
          }
          else {FAIL(1,err52,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      zzsetmatch(CLASS_TYPE_VAL_set, CLASS_TYPE_VAL_errset);
      classTypeVal = (ANTLRTokenPtr)LT(1);

      
      cell.classType = this->FindClassType_( classTypeVal->getType( ));
 consume();
    }
  }
  zzmatch(121); consume();
  {
    while ( (LA(1)==120) ) {
      zzmatch(120); consume();
      {
        zzmatch(PIN); consume();
        stringText(  &srName  );
        
        port.Clear( );
        port.SetName( srName );
        zzsetmatch(TYPE_VAL_set, TYPE_VAL_errset);
        typeVal = (ANTLRTokenPtr)LT(1);

        
        type = this->FindTypeMode_( typeVal->getType( ));
        port.SetType( type );
 consume();
        zzmatch(123);
        
        cell.AddPort( port );
 consume();
      }
    }
  }
  zzmatch(122); consume();
  zzmatch(CELL); consume();
  zzmatch(121);
  
  if( cell.IsValid( ))
  {
    pcellList->Add( cell );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x2);
}

void
TAP_ArchitectureParser_c::fcDef(TAS_ConnectionFc_c* pfc)
{
  zzRULE;
  ANTLRTokenPtr floatVal=NULL, uintVal=NULL, bitVal=NULL;
  
  unsigned int uintValue = 0;
  double floatValue = 0.0;
  {
    if ( (LA(1)==FULL) ) {
      zzmatch(FULL);
      
      pfc->type = TAS_CONNECTION_BOX_FULL;
      pfc->percent = 0.0;
      pfc->absolute = 0;
 consume();
    }
    else {
      if ( (LA(1)==FLOAT) ) {
        zzmatch(FLOAT);
        floatVal = (ANTLRTokenPtr)LT(1);

        
        pfc->type = TAS_CONNECTION_BOX_FRACTION;
        pfc->percent = atof( floatVal->getText( ));
        pfc->absolute = 0;
 consume();
      }
      else {
        if ( (LA(1)==POS_INT) ) {
          zzmatch(POS_INT);
          uintVal = (ANTLRTokenPtr)LT(1);

          
          pfc->type = TAS_CONNECTION_BOX_ABSOLUTE;
          pfc->percent = 0.0;
          pfc->absolute = static_cast< unsigned int >( atol( uintVal->getText( )));
 consume();
        }
        else {
          if ( (LA(1)==BIT_CHAR) ) {
            zzmatch(BIT_CHAR);
            bitVal = (ANTLRTokenPtr)LT(1);

            
            pfc->type = TAS_CONNECTION_BOX_ABSOLUTE;
            pfc->percent = 0.0;
            pfc->absolute = static_cast< unsigned int >( atol( bitVal->getText( )));
 consume();
          }
          else {FAIL(1,err57,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x4);
}

void
TAP_ArchitectureParser_c::modeNameList(TAS_ModeNameList_t* pmodeNameList)
{
  zzRULE;
  
  string srModeName;
  {
    while ( (setwd7[LA(1)]&0x8) ) {
      stringText(  &srModeName  );
      
      pmodeNameList->Add( srModeName );
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd7, 0x10);
}

void
TAP_ArchitectureParser_c::pinList(TLO_PortList_t* pportList)
{
  zzRULE;
  ANTLRTokenPtr typeVal=NULL, boolVal=NULL;
  
  TLO_Port_c port;
  
      string srName;
  TC_TypeMode_t type = TC_TYPE_UNDEFINED;
  unsigned int count = 0;
  bool isEquivalent = false;
  double cap = 0.0;
  double delay = 0.0;
  stringText(  &srName  );
  
  port.SetName( srName );
  zzsetmatch(TYPE_VAL_set, TYPE_VAL_errset);
  typeVal = (ANTLRTokenPtr)LT(1);

  
  type = this->FindTypeMode_( typeVal->getType( ));
  port.SetType( type );
 consume();
  {
    for (;;) {
      if ( !((setwd7[LA(1)]&0x20))) break;
      if ( (LA(1)==COUNT) ) {
        zzmatch(COUNT); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd7[LA(1)]&0x40) ) {
            }
            else {FAIL(1,err58,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        uintNum(  &count  );
        
        port.SetCount( count );
      }
      else {
        if ( (LA(1)==EQUIVALENCE) ) {
          zzmatch(EQUIVALENCE); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd7[LA(1)]&0x80) ) {
              }
              else {FAIL(1,err59,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          zzsetmatch(BOOL_VAL_set, BOOL_VAL_errset);
          boolVal = (ANTLRTokenPtr)LT(1);

          
          isEquivalent = this->FindBool_( boolVal->getType( ));
          port.SetEquivalent( isEquivalent );
 consume();
        }
        else break; /* MR6 code for exiting loop "for sure" */
      }
    }
  }
  {
    while ( (LA(1)==121) ) {
      zzmatch(121); consume();
      zzmatch(120); consume();
      zzmatch(TIMING); consume();
      {
        for (;;) {
          if ( !((setwd8[LA(1)]&0x1))) break;
          if ( (setwd8[LA(1)]&0x2) ) {
            {
              if ( (LA(1)==CAP) ) {
                zzmatch(CAP); consume();
              }
              else {
                if ( (LA(1)==CAP_IN) ) {
                  zzmatch(CAP_IN); consume();
                }
                else {FAIL(1,err62,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd8[LA(1)]&0x4) ) {
                }
                else {FAIL(1,err63,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            floatNum(  &cap  );
            
            port.SetCap( cap );
          }
          else {
            if ( (setwd8[LA(1)]&0x8) ) {
              {
                if ( (LA(1)==T) ) {
                  zzmatch(T); consume();
                }
                else {
                  if ( (LA(1)==DELAY) ) {
                    zzmatch(DELAY); consume();
                  }
                  else {FAIL(1,err64,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd8[LA(1)]&0x10) ) {
                  }
                  else {FAIL(1,err65,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              expNum(  &delay  );
              
              port.SetDelay( delay );
            }
            else break; /* MR6 code for exiting loop "for sure" */
          }
        }
      }
      zzmatch(121); consume();
    }
  }
  
  pportList->Add( port );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd8, 0x20);
}

void
TAP_ArchitectureParser_c::timingDelayLists(TAS_TimingDelayLists_c* ptimingDelayLists)
{
  zzRULE;
  
  TAS_TimingDelay_c timingDelay;
  {
    if ( (LA(1)==MAX_DELAY) ) {
      zzmatch(MAX_DELAY); consume();
      {
        for (;;) {
          if ( !((setwd8[LA(1)]&0x40))) break;
          if ( (setwd8[LA(1)]&0x80) ) {
            {
              if ( (LA(1)==T) ) {
                zzmatch(T); consume();
              }
              else {
                if ( (LA(1)==DELAY) ) {
                  zzmatch(DELAY); consume();
                }
                else {FAIL(1,err66,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd9[LA(1)]&0x1) ) {
                }
                else {FAIL(1,err67,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            expNum(  &timingDelay.delay  );
          }
          else {
            if ( (LA(1)==INPUT) ) {
              zzmatch(INPUT); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd9[LA(1)]&0x2) ) {
                  }
                  else {FAIL(1,err68,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              stringText(  &timingDelay.srInputPortName  );
            }
            else {
              if ( (LA(1)==OUTPUT) ) {
                zzmatch(OUTPUT); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd9[LA(1)]&0x4) ) {
                    }
                    else {FAIL(1,err69,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                stringText(  &timingDelay.srOutputPortName  );
              }
              else break; /* MR6 code for exiting loop "for sure" */
            }
          }
        }
      }
      
      timingDelay.type = TAS_TIMING_DELAY_CONSTANT;
      ptimingDelayLists->delayList.Add( timingDelay );
    }
    else {
      if ( (LA(1)==MAX_DELAY_MATRIX) ) {
        zzmatch(MAX_DELAY_MATRIX); consume();
        {
          for (;;) {
            if ( !((setwd9[LA(1)]&0x8))) break;
            if ( (setwd9[LA(1)]&0x10) ) {
              {
                if ( (LA(1)==T) ) {
                  zzmatch(T); consume();
                }
                else {
                  if ( (LA(1)==DELAY) ) {
                    zzmatch(DELAY); consume();
                  }
                  else {FAIL(1,err70,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd9[LA(1)]&0x20) ) {
                  }
                  else {FAIL(1,err71,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              delayMatrixDef(  &timingDelay.delayMatrix  );
            }
            else {
              if ( (LA(1)==INPUT) ) {
                zzmatch(INPUT); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd9[LA(1)]&0x40) ) {
                    }
                    else {FAIL(1,err72,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                stringText(  &timingDelay.srInputPortName  );
              }
              else {
                if ( (LA(1)==OUTPUT) ) {
                  zzmatch(OUTPUT); consume();
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd9[LA(1)]&0x80) ) {
                      }
                      else {FAIL(1,err73,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  stringText(  &timingDelay.srOutputPortName  );
                }
                else break; /* MR6 code for exiting loop "for sure" */
              }
            }
          }
        }
        
        timingDelay.type = TAS_TIMING_DELAY_MATRIX;
        ptimingDelayLists->delayMatrixList.Add( timingDelay );
      }
      else {
        if ( (LA(1)==CLOCK_SETUP_DELAY) ) {
          zzmatch(CLOCK_SETUP_DELAY); consume();
          {
            for (;;) {
              if ( !((setwd10[LA(1)]&0x1))) break;
              if ( (setwd10[LA(1)]&0x2) ) {
                {
                  if ( (LA(1)==T) ) {
                    zzmatch(T); consume();
                  }
                  else {
                    if ( (LA(1)==DELAY) ) {
                      zzmatch(DELAY); consume();
                    }
                    else {FAIL(1,err74,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd10[LA(1)]&0x4) ) {
                    }
                    else {FAIL(1,err75,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                expNum(  &timingDelay.delay  );
              }
              else {
                if ( (LA(1)==INPUT) ) {
                  zzmatch(INPUT); consume();
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd10[LA(1)]&0x8) ) {
                      }
                      else {FAIL(1,err76,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  stringText(  &timingDelay.srInputPortName  );
                }
                else {
                  if ( (LA(1)==CLOCK) ) {
                    zzmatch(CLOCK); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd10[LA(1)]&0x10) ) {
                        }
                        else {FAIL(1,err77,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    stringText(  &timingDelay.srClockPortName  );
                  }
                  else break; /* MR6 code for exiting loop "for sure" */
                }
              }
            }
          }
          
          timingDelay.type = TAS_TIMING_DELAY_SETUP;
          ptimingDelayLists->delayClockSetupList.Add( timingDelay );
        }
        else {
          if ( (LA(1)==CLOCK_TO_Q_DELAY) ) {
            zzmatch(CLOCK_TO_Q_DELAY); consume();
            {
              for (;;) {
                if ( !((setwd10[LA(1)]&0x20))) break;
                if ( (setwd10[LA(1)]&0x40) ) {
                  {
                    if ( (LA(1)==T) ) {
                      zzmatch(T); consume();
                    }
                    else {
                      if ( (LA(1)==DELAY) ) {
                        zzmatch(DELAY); consume();
                      }
                      else {FAIL(1,err78,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd10[LA(1)]&0x80) ) {
                      }
                      else {FAIL(1,err79,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  expNum(  &timingDelay.delay  );
                }
                else {
                  if ( (LA(1)==OUTPUT) ) {
                    zzmatch(OUTPUT); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd11[LA(1)]&0x1) ) {
                        }
                        else {FAIL(1,err80,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    stringText(  &timingDelay.srOutputPortName  );
                  }
                  else {
                    if ( (LA(1)==CLOCK) ) {
                      zzmatch(CLOCK); consume();
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd11[LA(1)]&0x2) ) {
                          }
                          else {FAIL(1,err81,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      stringText(  &timingDelay.srClockPortName  );
                    }
                    else break; /* MR6 code for exiting loop "for sure" */
                  }
                }
              }
            }
            
            timingDelay.type = TAS_TIMING_DELAY_CLOCK_TO_Q;
            ptimingDelayLists->delayClockToQList.Add( timingDelay );
          }
          else {FAIL(1,err82,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd11, 0x4);
}

void
TAP_ArchitectureParser_c::delayMatrixDef(TAS_DelayMatrix_t* pdelayMatrix)
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
    } while ( (setwd11[LA(1)]&0x8) );
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
  resynch(setwd11, 0x10);
}

void
TAP_ArchitectureParser_c::cellModelText(string* psrString,TAS_PhysicalBlockModelType_t* ptype)
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
  else
  {
    *ptype = TAS_PHYSICAL_BLOCK_MODEL_CUSTOM;
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd11, 0x20);
}

void
TAP_ArchitectureParser_c::pinAssignList(TAS_PinAssignPatternType_t* ppinAssignPattern,TAS_PinAssignList_t* ppinAssignList)
{
  zzRULE;
  ANTLRTokenPtr patternTypeVal=NULL, sideVal=NULL;
  
  TAS_PinAssign_c pinAssign;
  
      string srPortName;
  zzsetmatch(PIN_PATTERN_TYPE_VAL_set, PIN_PATTERN_TYPE_VAL_errset);
  patternTypeVal = (ANTLRTokenPtr)LT(1);

  
  pinAssign.pattern = this->FindPinAssignPatternType_( patternTypeVal->getType( ));
  *ppinAssignPattern = pinAssign.pattern;
 consume();
  {
    for (;;) {
      if ( !((setwd11[LA(1)]&0x40))) break;
      if ( (LA(1)==SIDE) ) {
        zzmatch(SIDE); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd11[LA(1)]&0x80) ) {
            }
            else {FAIL(1,err85,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzsetmatch(SIDE_VAL_set, SIDE_VAL_errset);
        sideVal = (ANTLRTokenPtr)LT(1);

        
        pinAssign.side = this->FindSideMode_( sideVal->getType( ));
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
              if ( (setwd12[LA(1)]&0x1) ) {
              }
              else {FAIL(1,err88,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          uintNum(  &pinAssign.offset  );
        }
        else break; /* MR6 code for exiting loop "for sure" */
      }
    }
  }
  {
    while ( (LA(1)==121) ) {
      zzmatch(121); consume();
      zzmatch(120); consume();
      zzmatch(PIN); consume();
      {
        while ( (setwd12[LA(1)]&0x2) ) {
          stringText(  &srPortName  );
          
          pinAssign.portNameList.Add( srPortName );
        }
      }
      zzmatch(121); consume();
    }
  }
  
  if( pinAssign.IsValid( ))
  {
    ppinAssignList->Add( pinAssign );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd12, 0x4);
}

void
TAP_ArchitectureParser_c::gridAssignList(TAS_GridAssignList_t* pgridAssignList)
{
  zzRULE;
  ANTLRTokenPtr distrModeVal=NULL, orientModeVal=NULL;
  
  TAS_GridAssign_c gridAssign;
  zzsetmatch(GRID_DISTR_MODE_VAL_set, GRID_DISTR_MODE_VAL_errset);
  distrModeVal = (ANTLRTokenPtr)LT(1);

  
  gridAssign.distr = this->FindGridAssignDistrMode_( distrModeVal->getType( ));
 consume();
  {
    for (;;) {
      if ( !((setwd12[LA(1)]&0x8))) break;
      if ( (LA(1)==ORIENT) ) {
        zzmatch(ORIENT); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd12[LA(1)]&0x10) ) {
            }
            else {FAIL(1,err91,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzsetmatch(GRID_ORIENT_MODE_VAL_set, GRID_ORIENT_MODE_VAL_errset);
        orientModeVal = (ANTLRTokenPtr)LT(1);

        
        gridAssign.orient = this->FindGridAssignOrientMode_( orientModeVal->getType( ));
 consume();
      }
      else {
        if ( (LA(1)==PRIORITY) ) {
          zzmatch(PRIORITY); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd12[LA(1)]&0x20) ) {
              }
              else {FAIL(1,err94,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          uintNum(  &gridAssign.priority  );
        }
        else {
          if ( (LA(1)==SINGLE_POS) ) {
            zzmatch(SINGLE_POS); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd12[LA(1)]&0x40) ) {
                }
                else {FAIL(1,err95,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            floatNum(  &gridAssign.singlePercent  );
          }
          else {
            if ( (LA(1)==MULTI_START) ) {
              zzmatch(MULTI_START); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd12[LA(1)]&0x80) ) {
                  }
                  else {FAIL(1,err96,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              uintNum(  &gridAssign.multipleStart  );
            }
            else {
              if ( (LA(1)==MULTI_REPEAT) ) {
                zzmatch(MULTI_REPEAT); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd13[LA(1)]&0x1) ) {
                    }
                    else {FAIL(1,err97,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                uintNum(  &gridAssign.multipleRepeat  );
              }
              else break; /* MR6 code for exiting loop "for sure" */
            }
          }
        }
      }
    }
  }
  
  if( gridAssign.IsValid( ))
  {
    pgridAssignList->Add( gridAssign );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd13, 0x2);
}

void
TAP_ArchitectureParser_c::mapSideTable(unsigned int Fs,TAS_MapTable_t* pmapTable)
{
  zzRULE;
  
  if( !pmapTable->IsValid( ))
  {
    pmapTable->SetCapacity( Fs * Fs );
  }
  TAS_MapList_t mapList( Fs );
  TAS_SideIndex_t sideIndex;
  
      size_t curTokenLine, nextTokenLine;
  {
    int zzcnt=1;
    do {
      mapSideIndex(  &sideIndex  );
      
      mapList.Add( sideIndex );
      
         curTokenLine = LT( 0 )->getLine( );
      nextTokenLine = LT( 1 )->getLine( );
      if( curTokenLine != nextTokenLine )
      {
        pmapTable->Add( mapList );
        mapList.Clear( );
      }
    } while ( (setwd13[LA(1)]&0x4) );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd13, 0x8);
}

void
TAP_ArchitectureParser_c::mapSideIndex(TC_SideIndex_c* psideIndex)
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
  resynch(setwd13, 0x10);
}

void
TAP_ArchitectureParser_c::segmentLength(unsigned int * plength)
{
  zzRULE;
  {
    if ( (LA(1)==FULL) ) {
      zzmatch(FULL);
      
      *plength = UINT_MAX;
 consume();
    }
    else {
      if ( (LA(1)==LONGLINE) ) {
        zzmatch(LONGLINE);
        
        *plength = UINT_MAX;
 consume();
      }
      else {
        if ( (setwd13[LA(1)]&0x20) ) {
          uintNum(  plength  );
        }
        else {FAIL(1,err98,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd13, 0x40);
}

void
TAP_ArchitectureParser_c::floatDims(TC_FloatDims_t* pfloatDims)
{
  zzRULE;
  floatNum(  &pfloatDims->width  );
  floatNum(  &pfloatDims->height  );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd13, 0x80);
}

void
TAP_ArchitectureParser_c::originPoint(TGS_Point_c* poriginPoint)
{
  zzRULE;
  floatNum(  &poriginPoint->x  );
  floatNum(  &poriginPoint->y  );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd14, 0x1);
}

void
TAP_ArchitectureParser_c::stringText(string* psrString)
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
        else {FAIL(1,err99,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
    else {FAIL(1,err100,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd14, 0x2);
}

void
TAP_ArchitectureParser_c::floatNum(double* pdouble)
{
  zzRULE;
  ANTLRTokenPtr qfloatVal=NULL, qposIntVal=NULL, qnegIntVal=NULL, floatVal=NULL, posIntVal=NULL, negIntVal=NULL, bitVal=NULL;
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
            else {FAIL(1,err101,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
          else {
            if ( (LA(1)==BIT_CHAR) ) {
              zzmatch(BIT_CHAR);
              bitVal = (ANTLRTokenPtr)LT(1);

              
              *pdouble = atof( bitVal->getText( ));
 consume();
            }
            else {FAIL(1,err102,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd14, 0x4);
}

void
TAP_ArchitectureParser_c::expNum(double* pdouble)
{
  zzRULE;
  ANTLRTokenPtr qexpVal=NULL, qfloatVal=NULL, qposIntVal=NULL, qnegIntVal=NULL, expVal=NULL, floatVal=NULL, posIntVal=NULL, negIntVal=NULL, bitVal=NULL;
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
              else {FAIL(1,err103,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
            else {
              if ( (LA(1)==BIT_CHAR) ) {
                zzmatch(BIT_CHAR);
                bitVal = (ANTLRTokenPtr)LT(1);

                
                *pdouble = atof( bitVal->getText( ));
 consume();
              }
              else {FAIL(1,err104,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd14, 0x8);
}

void
TAP_ArchitectureParser_c::intNum(int* pint)
{
  zzRULE;
  ANTLRTokenPtr qintVal=NULL, intVal=NULL, bitVal=NULL;
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
      else {
        if ( (LA(1)==BIT_CHAR) ) {
          zzmatch(BIT_CHAR);
          bitVal = (ANTLRTokenPtr)LT(1);

          
          *pint = static_cast< int >( atol( bitVal->getText( )));
 consume();
        }
        else {FAIL(1,err105,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd14, 0x10);
}

void
TAP_ArchitectureParser_c::uintNum(unsigned int* puint)
{
  zzRULE;
  ANTLRTokenPtr quintVal=NULL, uintVal=NULL, bitVal=NULL;
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
      else {
        if ( (LA(1)==BIT_CHAR) ) {
          zzmatch(BIT_CHAR);
          bitVal = (ANTLRTokenPtr)LT(1);

          
          *puint = static_cast< unsigned int >( atol( bitVal->getText( )));
 consume();
        }
        else {FAIL(1,err106,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd14, 0x20);
}

void
TAP_ArchitectureParser_c::boolType(bool* pbool)
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
        else {FAIL(1,err107,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd14, 0x40);
}

#include "TAP_ArchitectureGrammar.h"
