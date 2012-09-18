/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 *
 *   ..\..\pccts\bin\Antlr.exe -CC -gh TOP_OptionsGrammar.g
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

#include "TOS_OptionsStore.h"

#include "TOP_OptionsFile.h"
#include "TOP_OptionsScanner_c.h"
#include "AParser.h"
#include "TOP_OptionsParser_c.h"
#include "DLexerBase.h"
#include "ATokPtr.h"

/* MR23 In order to remove calls to PURIFY use the antlr -nopurify option */

#ifndef PCCTS_PURIFY
#define PCCTS_PURIFY(r,s) memset((char *) &(r),'\0',(s));
#endif


void
TOP_OptionsParser_c::start(void)
{
  zzRULE;
  
  this->srActiveCmd_ = "";
  {
    for (;;) {
      if ( !((setwd1[LA(1)]&0x1))) break;
      if ( (setwd1[LA(1)]&0x2) ) {
        inputOptions();
      }
      else {
        if ( (setwd1[LA(1)]&0x4) ) {
          outputOptions();
        }
        else {
          if ( (setwd1[LA(1)]&0x8) ) {
            messageOptions();
          }
          else {
            if ( (setwd1[LA(1)]&0x10) ) {
              executeOptions();
            }
            else {
              if ( (setwd1[LA(1)]&0x20) ) {
                packOptions();
              }
              else {
                if ( (setwd1[LA(1)]&0x40) ) {
                  placeOptions();
                }
                else {
                  if ( (setwd1[LA(1)]&0x80) ) {
                    routeOptions();
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
  zzmatch(END_OF_FILE); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x1);
}

void
TOP_OptionsParser_c::inputOptions(void)
{
  zzRULE;
  
  this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
  if ( (LA(1)==INPUT_FILE_XML) ) {
    zzmatch(INPUT_FILE_XML); consume();
    {
      if ( (LA(1)==EQUAL) ) {
        zzmatch(EQUAL); consume();
      }
      else {
        if ( (setwd2[LA(1)]&0x2) ) {
        }
        else {FAIL(1,err1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    stringText(  &pinputOptions_->srXmlFileName  );
    
    pinputOptions_->xmlFileEnable = true;
  }
  else {
    if ( (LA(1)==INPUT_FILE_BLIF) ) {
      zzmatch(INPUT_FILE_BLIF); consume();
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd2[LA(1)]&0x4) ) {
          }
          else {FAIL(1,err2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      stringText(  &pinputOptions_->srBlifFileName  );
      
      pinputOptions_->blifFileEnable = true;
    }
    else {
      if ( (LA(1)==INPUT_FILE_ARCH) ) {
        zzmatch(INPUT_FILE_ARCH); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd2[LA(1)]&0x8) ) {
            }
            else {FAIL(1,err3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        stringText(  &pinputOptions_->srArchitectureFileName  );
        
        pinputOptions_->architectureFileEnable = true;
      }
      else {
        if ( (LA(1)==INPUT_FILE_FABRIC) ) {
          zzmatch(INPUT_FILE_FABRIC); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd2[LA(1)]&0x10) ) {
              }
              else {FAIL(1,err4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          stringText(  &pinputOptions_->srFabricFileName  );
          
          pinputOptions_->fabricFileEnable = true;
        }
        else {
          if ( (LA(1)==INPUT_FILE_CIRCUIT) ) {
            zzmatch(INPUT_FILE_CIRCUIT); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd2[LA(1)]&0x20) ) {
                }
                else {FAIL(1,err5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            stringText(  &pinputOptions_->srCircuitFileName  );
            
            pinputOptions_->circuitFileEnable = true;
          }
          else {
            if ( (LA(1)==INPUT_ENABLE_XML) ) {
              zzmatch(INPUT_ENABLE_XML); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd2[LA(1)]&0x40) ) {
                  }
                  else {FAIL(1,err6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              boolType(  &pinputOptions_->xmlFileEnable  );
            }
            else {
              if ( (LA(1)==INPUT_ENABLE_BLIF) ) {
                zzmatch(INPUT_ENABLE_BLIF); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd2[LA(1)]&0x80) ) {
                    }
                    else {FAIL(1,err7,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                boolType(  &pinputOptions_->blifFileEnable  );
              }
              else {
                if ( (LA(1)==INPUT_ENABLE_ARCH) ) {
                  zzmatch(INPUT_ENABLE_ARCH); consume();
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd3[LA(1)]&0x1) ) {
                      }
                      else {FAIL(1,err8,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  boolType(  &pinputOptions_->architectureFileEnable  );
                }
                else {
                  if ( (LA(1)==INPUT_ENABLE_FABRIC) ) {
                    zzmatch(INPUT_ENABLE_FABRIC); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd3[LA(1)]&0x2) ) {
                        }
                        else {FAIL(1,err9,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    boolType(  &pinputOptions_->fabricFileEnable  );
                  }
                  else {
                    if ( (LA(1)==INPUT_ENABLE_CIRCUIT) ) {
                      zzmatch(INPUT_ENABLE_CIRCUIT); consume();
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd3[LA(1)]&0x4) ) {
                          }
                          else {FAIL(1,err10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      boolType(  &pinputOptions_->circuitFileEnable  );
                    }
                    else {
                      if ( (LA(1)==INPUT_DATA_PREPACKED) ) {
                        zzmatch(INPUT_DATA_PREPACKED); consume();
                        {
                          if ( (LA(1)==EQUAL) ) {
                            zzmatch(EQUAL); consume();
                          }
                          else {
                            if ( (setwd3[LA(1)]&0x8) ) {
                            }
                            else {FAIL(1,err11,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        inputDataMode(  &pinputOptions_->prePackedDataMode  );
                      }
                      else {
                        if ( (LA(1)==INPUT_DATA_PREPLACED) ) {
                          zzmatch(INPUT_DATA_PREPLACED); consume();
                          {
                            if ( (LA(1)==EQUAL) ) {
                              zzmatch(EQUAL); consume();
                            }
                            else {
                              if ( (setwd3[LA(1)]&0x10) ) {
                              }
                              else {FAIL(1,err12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                            }
                          }
                          inputDataMode(  &pinputOptions_->prePlacedDataMode  );
                        }
                        else {
                          if ( (LA(1)==INPUT_DATA_PREROUTED) ) {
                            zzmatch(INPUT_DATA_PREROUTED); consume();
                            {
                              if ( (LA(1)==EQUAL) ) {
                                zzmatch(EQUAL); consume();
                              }
                              else {
                                if ( (setwd3[LA(1)]&0x20) ) {
                                }
                                else {FAIL(1,err13,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                              }
                            }
                            inputDataMode(  &pinputOptions_->preRoutedDataMode  );
                            
                            this->srActiveCmd_ = "";
                          }
                          else {FAIL(1,err14,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x40);
}

void
TOP_OptionsParser_c::outputOptions(void)
{
  zzRULE;
  
  this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
  if ( (LA(1)==OUTPUT_FILE_LOG) ) {
    zzmatch(OUTPUT_FILE_LOG); consume();
    {
      if ( (LA(1)==EQUAL) ) {
        zzmatch(EQUAL); consume();
      }
      else {
        if ( (setwd3[LA(1)]&0x80) ) {
        }
        else {FAIL(1,err15,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    stringText(  &poutputOptions_->srLogFileName  );
  }
  else {
    if ( (LA(1)==OUTPUT_FILE_OPTIONS) ) {
      zzmatch(OUTPUT_FILE_OPTIONS); consume();
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd4[LA(1)]&0x1) ) {
          }
          else {FAIL(1,err16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      stringText(  &poutputOptions_->srOptionsFileName  );
    }
    else {
      if ( (LA(1)==OUTPUT_FILE_XML) ) {
        zzmatch(OUTPUT_FILE_XML); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd4[LA(1)]&0x2) ) {
            }
            else {FAIL(1,err17,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        stringText(  &poutputOptions_->srXmlFileName  );
      }
      else {
        if ( (LA(1)==OUTPUT_FILE_BLIF) ) {
          zzmatch(OUTPUT_FILE_BLIF); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd4[LA(1)]&0x4) ) {
              }
              else {FAIL(1,err18,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          stringText(  &poutputOptions_->srBlifFileName  );
        }
        else {
          if ( (LA(1)==OUTPUT_FILE_ARCH) ) {
            zzmatch(OUTPUT_FILE_ARCH); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd4[LA(1)]&0x8) ) {
                }
                else {FAIL(1,err19,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            stringText(  &poutputOptions_->srArchitectureFileName  );
          }
          else {
            if ( (LA(1)==OUTPUT_FILE_FABRIC) ) {
              zzmatch(OUTPUT_FILE_FABRIC); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd4[LA(1)]&0x10) ) {
                  }
                  else {FAIL(1,err20,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              stringText(  &poutputOptions_->srFabricFileName  );
            }
            else {
              if ( (LA(1)==OUTPUT_FILE_CIRCUIT) ) {
                zzmatch(OUTPUT_FILE_CIRCUIT); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd4[LA(1)]&0x20) ) {
                    }
                    else {FAIL(1,err21,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                stringText(  &poutputOptions_->srCircuitFileName  );
              }
              else {
                if ( (LA(1)==OUTPUT_FILE_RC_DELAYS) ) {
                  zzmatch(OUTPUT_FILE_RC_DELAYS); consume();
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd4[LA(1)]&0x40) ) {
                      }
                      else {FAIL(1,err22,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  stringText(  &poutputOptions_->srRcDelaysFileName  );
                }
                else {
                  if ( (LA(1)==OUTPUT_FILE_LAFF) ) {
                    zzmatch(OUTPUT_FILE_LAFF); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd4[LA(1)]&0x80) ) {
                        }
                        else {FAIL(1,err23,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    stringText(  &poutputOptions_->srLaffFileName  );
                  }
                  else {
                    if ( (LA(1)==OUTPUT_FILE_METRICS) ) {
                      zzmatch(OUTPUT_FILE_METRICS); consume();
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd5[LA(1)]&0x1) ) {
                          }
                          else {FAIL(1,err24,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      stringText(  &poutputOptions_->srMetricsFileName  );
                    }
                    else {
                      if ( (LA(1)==OUTPUT_EMAIL_METRICS) ) {
                        zzmatch(OUTPUT_EMAIL_METRICS); consume();
                        {
                          if ( (LA(1)==EQUAL) ) {
                            zzmatch(EQUAL); consume();
                          }
                          else {
                            if ( (setwd5[LA(1)]&0x2) ) {
                            }
                            else {FAIL(1,err25,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        stringText(  &poutputOptions_->srMetricsEmailAddress  );
                      }
                      else {
                        if ( (LA(1)==OUTPUT_ENABLE_LOG) ) {
                          zzmatch(OUTPUT_ENABLE_LOG); consume();
                          {
                            if ( (LA(1)==EQUAL) ) {
                              zzmatch(EQUAL); consume();
                            }
                            else {
                              if ( (setwd5[LA(1)]&0x4) ) {
                              }
                              else {FAIL(1,err26,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                            }
                          }
                          boolType(  &poutputOptions_->logFileEnable  );
                        }
                        else {
                          if ( (LA(1)==OUTPUT_ENABLE_OPTIONS) ) {
                            zzmatch(OUTPUT_ENABLE_OPTIONS); consume();
                            {
                              if ( (LA(1)==EQUAL) ) {
                                zzmatch(EQUAL); consume();
                              }
                              else {
                                if ( (setwd5[LA(1)]&0x8) ) {
                                }
                                else {FAIL(1,err27,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                              }
                            }
                            boolType(  &poutputOptions_->optionsFileEnable  );
                          }
                          else {
                            if ( (LA(1)==OUTPUT_ENABLE_XML) ) {
                              zzmatch(OUTPUT_ENABLE_XML); consume();
                              {
                                if ( (LA(1)==EQUAL) ) {
                                  zzmatch(EQUAL); consume();
                                }
                                else {
                                  if ( (setwd5[LA(1)]&0x10) ) {
                                  }
                                  else {FAIL(1,err28,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                }
                              }
                              boolType(  &poutputOptions_->xmlFileEnable  );
                            }
                            else {
                              if ( (LA(1)==OUTPUT_ENABLE_BLIF) ) {
                                zzmatch(OUTPUT_ENABLE_BLIF); consume();
                                {
                                  if ( (LA(1)==EQUAL) ) {
                                    zzmatch(EQUAL); consume();
                                  }
                                  else {
                                    if ( (setwd5[LA(1)]&0x20) ) {
                                    }
                                    else {FAIL(1,err29,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                  }
                                }
                                boolType(  &poutputOptions_->blifFileEnable  );
                              }
                              else {
                                if ( (LA(1)==OUTPUT_ENABLE_ARCH) ) {
                                  zzmatch(OUTPUT_ENABLE_ARCH); consume();
                                  {
                                    if ( (LA(1)==EQUAL) ) {
                                      zzmatch(EQUAL); consume();
                                    }
                                    else {
                                      if ( (setwd5[LA(1)]&0x40) ) {
                                      }
                                      else {FAIL(1,err30,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                    }
                                  }
                                  boolType(  &poutputOptions_->architectureFileEnable  );
                                }
                                else {
                                  if ( (LA(1)==OUTPUT_ENABLE_FABRIC) ) {
                                    zzmatch(OUTPUT_ENABLE_FABRIC); consume();
                                    {
                                      if ( (LA(1)==EQUAL) ) {
                                        zzmatch(EQUAL); consume();
                                      }
                                      else {
                                        if ( (setwd5[LA(1)]&0x80) ) {
                                        }
                                        else {FAIL(1,err31,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                      }
                                    }
                                    boolType(  &poutputOptions_->fabricFileEnable  );
                                  }
                                  else {
                                    if ( (LA(1)==OUTPUT_ENABLE_CIRCUIT) ) {
                                      zzmatch(OUTPUT_ENABLE_CIRCUIT); consume();
                                      {
                                        if ( (LA(1)==EQUAL) ) {
                                          zzmatch(EQUAL); consume();
                                        }
                                        else {
                                          if ( (setwd6[LA(1)]&0x1) ) {
                                          }
                                          else {FAIL(1,err32,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                        }
                                      }
                                      boolType(  &poutputOptions_->circuitFileEnable  );
                                    }
                                    else {
                                      if ( (LA(1)==OUTPUT_ENABLE_RC_DELAYS) ) {
                                        zzmatch(OUTPUT_ENABLE_RC_DELAYS); consume();
                                        {
                                          if ( (LA(1)==EQUAL) ) {
                                            zzmatch(EQUAL); consume();
                                          }
                                          else {
                                            if ( (setwd6[LA(1)]&0x2) ) {
                                            }
                                            else {FAIL(1,err33,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                          }
                                        }
                                        boolType(  &poutputOptions_->rcDelaysFileEnable  );
                                      }
                                      else {
                                        if ( (LA(1)==OUTPUT_ENABLE_LAFF) ) {
                                          zzmatch(OUTPUT_ENABLE_LAFF); consume();
                                          {
                                            if ( (LA(1)==EQUAL) ) {
                                              zzmatch(EQUAL); consume();
                                            }
                                            else {
                                              if ( (setwd6[LA(1)]&0x4) ) {
                                              }
                                              else {FAIL(1,err34,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                            }
                                          }
                                          boolType(  &poutputOptions_->laffFileEnable  );
                                        }
                                        else {
                                          if ( (LA(1)==OUTPUT_ENABLE_METRICS) ) {
                                            zzmatch(OUTPUT_ENABLE_METRICS); consume();
                                            {
                                              if ( (LA(1)==EQUAL) ) {
                                                zzmatch(EQUAL); consume();
                                              }
                                              else {
                                                if ( (setwd6[LA(1)]&0x8) ) {
                                                }
                                                else {FAIL(1,err35,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                              }
                                            }
                                            boolType(  &poutputOptions_->metricsFileEnable  );
                                          }
                                          else {
                                            if ( (LA(1)==OUTPUT_LAFF_MODE) ) {
                                              zzmatch(OUTPUT_LAFF_MODE); consume();
                                              {
                                                if ( (LA(1)==EQUAL) ) {
                                                  zzmatch(EQUAL); consume();
                                                }
                                                else {
                                                  if ( (setwd6[LA(1)]&0x10) ) {
                                                  }
                                                  else {FAIL(1,err36,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                                }
                                              }
                                              outputLaffMask(  &poutputOptions_->laffMask  );
                                            }
                                            else {
                                              if ( (LA(1)==OUTPUT_RC_DELAYS_MODE) ) {
                                                zzmatch(OUTPUT_RC_DELAYS_MODE); consume();
                                                {
                                                  if ( (LA(1)==EQUAL) ) {
                                                    zzmatch(EQUAL); consume();
                                                  }
                                                  else {
                                                    if ( (setwd6[LA(1)]&0x20) ) {
                                                    }
                                                    else {FAIL(1,err37,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                                  }
                                                }
                                                rcDelaysExtractMode(  &poutputOptions_->rcDelaysExtractMode  );
                                              }
                                              else {
                                                if ( (LA(1)==OUTPUT_RC_DELAYS_SORT) ) {
                                                  zzmatch(OUTPUT_RC_DELAYS_SORT); consume();
                                                  {
                                                    if ( (LA(1)==EQUAL) ) {
                                                      zzmatch(EQUAL); consume();
                                                    }
                                                    else {
                                                      if ( (setwd6[LA(1)]&0x40) ) {
                                                      }
                                                      else {FAIL(1,err38,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                                    }
                                                  }
                                                  rcDelaysSortMode(  &poutputOptions_->rcDelaysSortMode  );
                                                }
                                                else {
                                                  if ( (LA(1)==OUTPUT_RC_DELAYS_NETS) ) {
                                                    zzmatch(OUTPUT_RC_DELAYS_NETS); consume();
                                                    {
                                                      if ( (LA(1)==EQUAL) ) {
                                                        zzmatch(EQUAL); consume();
                                                      }
                                                      else {
                                                        if ( (setwd6[LA(1)]&0x80) ) {
                                                        }
                                                        else {FAIL(1,err39,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                                      }
                                                    }
                                                    rcDelaysNameList(  &poutputOptions_->rcDelaysNetNameList  );
                                                  }
                                                  else {
                                                    if ( (LA(1)==OUTPUT_RC_DELAYS_BLOCKS) ) {
                                                      zzmatch(OUTPUT_RC_DELAYS_BLOCKS); consume();
                                                      {
                                                        if ( (LA(1)==EQUAL) ) {
                                                          zzmatch(EQUAL); consume();
                                                        }
                                                        else {
                                                          if ( (setwd7[LA(1)]&0x1) ) {
                                                          }
                                                          else {FAIL(1,err40,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                                        }
                                                      }
                                                      rcDelaysNameList(  &poutputOptions_->rcDelaysBlockNameList  );
                                                    }
                                                    else {
                                                      if ( (LA(1)==OUTPUT_RC_DELAYS_LENGTH) ) {
                                                        zzmatch(OUTPUT_RC_DELAYS_LENGTH); consume();
                                                        {
                                                          if ( (LA(1)==EQUAL) ) {
                                                            zzmatch(EQUAL); consume();
                                                          }
                                                          else {
                                                            if ( (setwd7[LA(1)]&0x2) ) {
                                                            }
                                                            else {FAIL(1,err41,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                                          }
                                                        }
                                                        floatNum(  &poutputOptions_->rcDelaysMaxWireLength  );
                                                        
                                                        this->srActiveCmd_ = "";
                                                      }
                                                      else {FAIL(1,err42,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
            }
          }
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
TOP_OptionsParser_c::messageOptions(void)
{
  zzRULE;
  
  string srEchoName;
  string srFileName;
  
      this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
  if ( (LA(1)==FORMAT_MIN_GRID) ) {
    zzmatch(FORMAT_MIN_GRID); consume();
    {
      if ( (LA(1)==EQUAL) ) {
        zzmatch(EQUAL); consume();
      }
      else {
        if ( (setwd7[LA(1)]&0x8) ) {
        }
        else {FAIL(1,err43,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    floatNum(  &pmessageOptions_->minGridPrecision  );
  }
  else {
    if ( (LA(1)==FORMAT_TIME_STAMPS) ) {
      zzmatch(FORMAT_TIME_STAMPS); consume();
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd7[LA(1)]&0x10) ) {
          }
          else {FAIL(1,err44,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      boolType(  &pmessageOptions_->timeStampsEnable  );
    }
    else {
      if ( (LA(1)==DISPLAY_INFO_ACCEPT) ) {
        zzmatch(DISPLAY_INFO_ACCEPT); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd7[LA(1)]&0x20) ) {
            }
            else {FAIL(1,err45,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        displayNameList(  &pmessageOptions_->info.acceptList  );
      }
      else {
        if ( (LA(1)==DISPLAY_INFO_REJECT) ) {
          zzmatch(DISPLAY_INFO_REJECT); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd7[LA(1)]&0x40) ) {
              }
              else {FAIL(1,err46,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          displayNameList(  &pmessageOptions_->info.rejectList  );
        }
        else {
          if ( (LA(1)==DISPLAY_WARNING_ACCEPT) ) {
            zzmatch(DISPLAY_WARNING_ACCEPT); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd7[LA(1)]&0x80) ) {
                }
                else {FAIL(1,err47,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            displayNameList(  &pmessageOptions_->warning.acceptList  );
          }
          else {
            if ( (LA(1)==DISPLAY_WARNING_REJECT) ) {
              zzmatch(DISPLAY_WARNING_REJECT); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd8[LA(1)]&0x1) ) {
                  }
                  else {FAIL(1,err48,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              displayNameList(  &pmessageOptions_->warning.rejectList  );
            }
            else {
              if ( (LA(1)==DISPLAY_ERROR_ACCEPT) ) {
                zzmatch(DISPLAY_ERROR_ACCEPT); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd8[LA(1)]&0x2) ) {
                    }
                    else {FAIL(1,err49,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                displayNameList(  &pmessageOptions_->error.acceptList  );
              }
              else {
                if ( (LA(1)==DISPLAY_ERROR_REJECT) ) {
                  zzmatch(DISPLAY_ERROR_REJECT); consume();
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd8[LA(1)]&0x4) ) {
                      }
                      else {FAIL(1,err50,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  displayNameList(  &pmessageOptions_->error.rejectList  );
                }
                else {
                  if ( (LA(1)==DISPLAY_TRACE_ACCEPT) ) {
                    zzmatch(DISPLAY_TRACE_ACCEPT); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd8[LA(1)]&0x8) ) {
                        }
                        else {FAIL(1,err51,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    displayNameList(  &pmessageOptions_->trace.acceptList  );
                  }
                  else {
                    if ( (LA(1)==DISPLAY_TRACE_REJECT) ) {
                      zzmatch(DISPLAY_TRACE_REJECT); consume();
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd8[LA(1)]&0x10) ) {
                          }
                          else {FAIL(1,err52,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      displayNameList(  &pmessageOptions_->trace.rejectList  );
                    }
                    else {
                      if ( (LA(1)==TRACE_READ_OPTIONS) ) {
                        zzmatch(TRACE_READ_OPTIONS); consume();
                        {
                          if ( (LA(1)==EQUAL) ) {
                            zzmatch(EQUAL); consume();
                          }
                          else {
                            if ( (setwd8[LA(1)]&0x20) ) {
                            }
                            else {FAIL(1,err53,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        boolType(  &pmessageOptions_->trace.read.options  );
                      }
                      else {
                        if ( (LA(1)==TRACE_READ_XML) ) {
                          zzmatch(TRACE_READ_XML); consume();
                          {
                            if ( (LA(1)==EQUAL) ) {
                              zzmatch(EQUAL); consume();
                            }
                            else {
                              if ( (setwd8[LA(1)]&0x40) ) {
                              }
                              else {FAIL(1,err54,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                            }
                          }
                          boolType(  &pmessageOptions_->trace.read.xml  );
                        }
                        else {
                          if ( (LA(1)==TRACE_READ_BLIF) ) {
                            zzmatch(TRACE_READ_BLIF); consume();
                            {
                              if ( (LA(1)==EQUAL) ) {
                                zzmatch(EQUAL); consume();
                              }
                              else {
                                if ( (setwd8[LA(1)]&0x80) ) {
                                }
                                else {FAIL(1,err55,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                              }
                            }
                            boolType(  &pmessageOptions_->trace.read.blif  );
                          }
                          else {
                            if ( (LA(1)==TRACE_READ_ARCH) ) {
                              zzmatch(TRACE_READ_ARCH); consume();
                              {
                                if ( (LA(1)==EQUAL) ) {
                                  zzmatch(EQUAL); consume();
                                }
                                else {
                                  if ( (setwd9[LA(1)]&0x1) ) {
                                  }
                                  else {FAIL(1,err56,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                }
                              }
                              boolType(  &pmessageOptions_->trace.read.architecture  );
                            }
                            else {
                              if ( (LA(1)==TRACE_READ_FABRIC) ) {
                                zzmatch(TRACE_READ_FABRIC); consume();
                                {
                                  if ( (LA(1)==EQUAL) ) {
                                    zzmatch(EQUAL); consume();
                                  }
                                  else {
                                    if ( (setwd9[LA(1)]&0x2) ) {
                                    }
                                    else {FAIL(1,err57,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                  }
                                }
                                boolType(  &pmessageOptions_->trace.read.fabric  );
                              }
                              else {
                                if ( (LA(1)==TRACE_READ_CIRCUIT) ) {
                                  zzmatch(TRACE_READ_CIRCUIT); consume();
                                  {
                                    if ( (LA(1)==EQUAL) ) {
                                      zzmatch(EQUAL); consume();
                                    }
                                    else {
                                      if ( (setwd9[LA(1)]&0x4) ) {
                                      }
                                      else {FAIL(1,err58,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                    }
                                  }
                                  boolType(  &pmessageOptions_->trace.read.circuit  );
                                }
                                else {
                                  if ( (LA(1)==TRACE_VPR_SHOW_SETUP) ) {
                                    zzmatch(TRACE_VPR_SHOW_SETUP); consume();
                                    {
                                      if ( (LA(1)==EQUAL) ) {
                                        zzmatch(EQUAL); consume();
                                      }
                                      else {
                                        if ( (setwd9[LA(1)]&0x8) ) {
                                        }
                                        else {FAIL(1,err59,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                      }
                                    }
                                    boolType(  &pmessageOptions_->trace.vpr.showSetup  );
                                  }
                                  else {
                                    if ( (LA(1)==TRACE_VPR_ECHO_FILE) ) {
                                      zzmatch(TRACE_VPR_ECHO_FILE); consume();
                                      {
                                        if ( (LA(1)==EQUAL) ) {
                                          zzmatch(EQUAL); consume();
                                        }
                                        else {
                                          if ( (setwd9[LA(1)]&0x10) ) {
                                          }
                                          else {FAIL(1,err60,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                        }
                                      }
                                      {
                                        if ( (setwd9[LA(1)]&0x20) ) {
                                          boolType(  &pmessageOptions_->trace.vpr.echoFile  );
                                        }
                                        else {
                                          if ( (setwd9[LA(1)]&0x40) ) {
                                            stringText(  &srEchoName  );
                                            {
                                              if ( (setwd9[LA(1)]&0x80) ) {
                                                stringText(  &srFileName  );
                                              }
                                              else {
                                                if ( (setwd10[LA(1)]&0x1) ) {
                                                }
                                                else {FAIL(1,err61,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                              }
                                            }
                                            
                                            TC_NameFile_c echoFileName( srEchoName, srFileName );
                                            pmessageOptions_->trace.vpr.echoFileNameList.Add( echoFileName );
                                          }
                                          else {FAIL(1,err62,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                        }
                                      }
                                      
                                      this->srActiveCmd_ = "";
                                    }
                                    else {FAIL(1,err63,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
              }
            }
          }
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd10, 0x2);
}

void
TOP_OptionsParser_c::executeOptions(void)
{
  zzRULE;
  
  this->srActiveCmd_ = "";
  if ( (LA(1)==HALT_MAX_WARNINGS) ) {
    zzmatch(HALT_MAX_WARNINGS); consume();
    {
      if ( (LA(1)==EQUAL) ) {
        zzmatch(EQUAL); consume();
      }
      else {
        if ( (setwd10[LA(1)]&0x4) ) {
        }
        else {FAIL(1,err64,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    ulongNum(  &pexecuteOptions_->maxWarningCount   );
  }
  else {
    if ( (LA(1)==HALT_MAX_ERRORS) ) {
      zzmatch(HALT_MAX_ERRORS); consume();
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd10[LA(1)]&0x8) ) {
          }
          else {FAIL(1,err65,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      ulongNum(  &pexecuteOptions_->maxErrorCount  );
    }
    else {
      if ( (LA(1)==EXECUTE_MODE) ) {
        zzmatch(EXECUTE_MODE); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd10[LA(1)]&0x10) ) {
            }
            else {FAIL(1,err66,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        executeToolMask(  &pexecuteOptions_->toolMask  );
        
        this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
      }
      else {FAIL(1,err67,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd10, 0x20);
}

void
TOP_OptionsParser_c::packOptions(void)
{
  zzRULE;
  
  this->srActiveCmd_ = "";
  if ( (LA(1)==PACK_ALGORITHM) ) {
    zzmatch(PACK_ALGORITHM); consume();
    {
      if ( (LA(1)==EQUAL) ) {
        zzmatch(EQUAL); consume();
      }
      else {
        if ( (LA(1)==AAPACK) ) {
        }
        else {FAIL(1,err68,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    packAlgorithmMode(  &ppackOptions_->algorithmMode  );
  }
  else {
    if ( (LA(1)==PACK_CLUSTER_NETS) ) {
      zzmatch(PACK_CLUSTER_NETS); consume();
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd10[LA(1)]&0x40) ) {
          }
          else {FAIL(1,err69,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      packClusterNetsMode(  &ppackOptions_->clusterNetsMode  );
    }
    else {
      if ( (LA(1)==PACK_AREA_WEIGHT) ) {
        zzmatch(PACK_AREA_WEIGHT); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd10[LA(1)]&0x80) ) {
            }
            else {FAIL(1,err70,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        floatNum(  &ppackOptions_->areaWeight  );
      }
      else {
        if ( (LA(1)==PACK_NETS_WEIGHT) ) {
          zzmatch(PACK_NETS_WEIGHT); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd11[LA(1)]&0x1) ) {
              }
              else {FAIL(1,err71,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          floatNum(  &ppackOptions_->netsWeight  );
        }
        else {
          if ( (LA(1)==PACK_AFFINITY_MODE) ) {
            zzmatch(PACK_AFFINITY_MODE); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd11[LA(1)]&0x2) ) {
                }
                else {FAIL(1,err72,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            packAffinityMode(  &ppackOptions_->affinityMode  );
          }
          else {
            if ( (LA(1)==PACK_BLOCK_SIZE) ) {
              zzmatch(PACK_BLOCK_SIZE); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (LA(1)==POS_INT) ) {
                  }
                  else {FAIL(1,err73,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              uintNum(  &ppackOptions_->blockSize  );
            }
            else {
              if ( (LA(1)==PACK_LUT_SIZE) ) {
                zzmatch(PACK_LUT_SIZE); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (LA(1)==POS_INT) ) {
                    }
                    else {FAIL(1,err74,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                uintNum(  &ppackOptions_->lutSize  );
              }
              else {
                if ( (LA(1)==PACK_COST_MODE) ) {
                  zzmatch(PACK_COST_MODE); consume();
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd11[LA(1)]&0x4) ) {
                      }
                      else {FAIL(1,err75,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  packCostMode(  &ppackOptions_->costMode  );
                  
                  this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
                }
                else {FAIL(1,err76,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd11, 0x8);
}

void
TOP_OptionsParser_c::placeOptions(void)
{
  zzRULE;
  
  this->srActiveCmd_ = "";
  if ( (LA(1)==PLACE_ALGORITHM) ) {
    zzmatch(PLACE_ALGORITHM); consume();
    {
      if ( (LA(1)==EQUAL) ) {
        zzmatch(EQUAL); consume();
      }
      else {
        if ( (LA(1)==ANNEALING) ) {
        }
        else {FAIL(1,err77,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    placeAlgorithmMode(  &pplaceOptions_->algorithmMode  );
  }
  else {
    if ( (LA(1)==PLACE_RANDOM_SEED) ) {
      zzmatch(PLACE_RANDOM_SEED); consume();
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (LA(1)==POS_INT) ) {
          }
          else {FAIL(1,err78,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      uintNum(  &pplaceOptions_->randomSeed  );
    }
    else {
      if ( (LA(1)==PLACE_TEMP_INIT) ) {
        zzmatch(PLACE_TEMP_INIT); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd11[LA(1)]&0x10) ) {
            }
            else {FAIL(1,err79,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        floatNum(  &pplaceOptions_->initTemp  );
      }
      else {
        if ( (LA(1)==PLACE_TEMP_INIT_FACTOR) ) {
          zzmatch(PLACE_TEMP_INIT_FACTOR); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd11[LA(1)]&0x20) ) {
              }
              else {FAIL(1,err80,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          floatNum(  &pplaceOptions_->initTempFactor  );
        }
        else {
          if ( (LA(1)==PLACE_TEMP_INIT_EPSILON) ) {
            zzmatch(PLACE_TEMP_INIT_EPSILON); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd11[LA(1)]&0x40) ) {
                }
                else {FAIL(1,err81,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            floatNum(  &pplaceOptions_->initTempEpsilon  );
          }
          else {
            if ( (LA(1)==PLACE_TEMP_EXIT) ) {
              zzmatch(PLACE_TEMP_EXIT); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd11[LA(1)]&0x80) ) {
                  }
                  else {FAIL(1,err82,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              floatNum(  &pplaceOptions_->exitTemp  );
            }
            else {
              if ( (LA(1)==PLACE_TEMP_EXIT_FACTOR) ) {
                zzmatch(PLACE_TEMP_EXIT_FACTOR); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd12[LA(1)]&0x1) ) {
                    }
                    else {FAIL(1,err83,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                floatNum(  &pplaceOptions_->exitTempFactor  );
              }
              else {
                if ( (LA(1)==PLACE_TEMP_EXIT_EPSILON) ) {
                  zzmatch(PLACE_TEMP_EXIT_EPSILON); consume();
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd12[LA(1)]&0x2) ) {
                      }
                      else {FAIL(1,err84,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  floatNum(  &pplaceOptions_->exitTempEpsilon  );
                }
                else {
                  if ( (LA(1)==PLACE_TEMP_REDUCE) ) {
                    zzmatch(PLACE_TEMP_REDUCE); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd12[LA(1)]&0x4) ) {
                        }
                        else {FAIL(1,err85,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    floatNum(  &pplaceOptions_->reduceTemp  );
                  }
                  else {
                    if ( (LA(1)==PLACE_TEMP_REDUCE_FACTOR) ) {
                      zzmatch(PLACE_TEMP_REDUCE_FACTOR); consume();
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd12[LA(1)]&0x8) ) {
                          }
                          else {FAIL(1,err86,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      floatNum(  &pplaceOptions_->reduceTempFactor  );
                    }
                    else {
                      if ( (LA(1)==PLACE_TEMP_INNER_NUM) ) {
                        zzmatch(PLACE_TEMP_INNER_NUM); consume();
                        {
                          if ( (LA(1)==EQUAL) ) {
                            zzmatch(EQUAL); consume();
                          }
                          else {
                            if ( (setwd12[LA(1)]&0x10) ) {
                            }
                            else {FAIL(1,err87,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        floatNum(  &pplaceOptions_->innerNum  );
                      }
                      else {
                        if ( (LA(1)==PLACE_SEARCH_LIMIT) ) {
                          zzmatch(PLACE_SEARCH_LIMIT); consume();
                          {
                            if ( (LA(1)==EQUAL) ) {
                              zzmatch(EQUAL); consume();
                            }
                            else {
                              if ( (setwd12[LA(1)]&0x20) ) {
                              }
                              else {FAIL(1,err88,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                            }
                          }
                          floatNum(  &pplaceOptions_->searchLimit  );
                        }
                        else {
                          if ( (LA(1)==PLACE_COST_MODE) ) {
                            zzmatch(PLACE_COST_MODE); consume();
                            {
                              if ( (LA(1)==EQUAL) ) {
                                zzmatch(EQUAL); consume();
                              }
                              else {
                                if ( (setwd12[LA(1)]&0x40) ) {
                                }
                                else {FAIL(1,err89,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                              }
                            }
                            placeCostMode(  &pplaceOptions_->costMode  );
                          }
                          else {
                            if ( (LA(1)==PLACE_TIMING_COST_FACTOR) ) {
                              zzmatch(PLACE_TIMING_COST_FACTOR); consume();
                              {
                                if ( (LA(1)==EQUAL) ) {
                                  zzmatch(EQUAL); consume();
                                }
                                else {
                                  if ( (setwd12[LA(1)]&0x80) ) {
                                  }
                                  else {FAIL(1,err90,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                }
                              }
                              floatNum(  &pplaceOptions_->timingCostFactor  );
                            }
                            else {
                              if ( (LA(1)==PLACE_TIMING_UPDATE_INT) ) {
                                zzmatch(PLACE_TIMING_UPDATE_INT); consume();
                                {
                                  if ( (LA(1)==EQUAL) ) {
                                    zzmatch(EQUAL); consume();
                                  }
                                  else {
                                    if ( (LA(1)==POS_INT) ) {
                                    }
                                    else {FAIL(1,err91,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                  }
                                }
                                uintNum(  &pplaceOptions_->timingUpdateInt  );
                              }
                              else {
                                if ( (LA(1)==PLACE_TIMING_UPDATE_COUNT) ) {
                                  zzmatch(PLACE_TIMING_UPDATE_COUNT); consume();
                                  {
                                    if ( (LA(1)==EQUAL) ) {
                                      zzmatch(EQUAL); consume();
                                    }
                                    else {
                                      if ( (LA(1)==POS_INT) ) {
                                      }
                                      else {FAIL(1,err92,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                    }
                                  }
                                  uintNum(  &pplaceOptions_->timingUpdateCount  );
                                }
                                else {
                                  if ( (LA(1)==PLACE_SLACK_INIT_WEIGHT) ) {
                                    zzmatch(PLACE_SLACK_INIT_WEIGHT); consume();
                                    {
                                      if ( (LA(1)==EQUAL) ) {
                                        zzmatch(EQUAL); consume();
                                      }
                                      else {
                                        if ( (setwd13[LA(1)]&0x1) ) {
                                        }
                                        else {FAIL(1,err93,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                      }
                                    }
                                    floatNum(  &pplaceOptions_->slackInitWeight  );
                                  }
                                  else {
                                    if ( (LA(1)==PLACE_SLACK_FINAL_WEIGHT) ) {
                                      zzmatch(PLACE_SLACK_FINAL_WEIGHT); consume();
                                      {
                                        if ( (LA(1)==EQUAL) ) {
                                          zzmatch(EQUAL); consume();
                                        }
                                        else {
                                          if ( (setwd13[LA(1)]&0x2) ) {
                                          }
                                          else {FAIL(1,err94,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                        }
                                      }
                                      floatNum(  &pplaceOptions_->slackFinalWeight  );
                                      
                                      this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
                                    }
                                    else {FAIL(1,err95,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
              }
            }
          }
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd13, 0x4);
}

void
TOP_OptionsParser_c::routeOptions(void)
{
  zzRULE;
  
  this->srActiveCmd_ = "";
  if ( (LA(1)==ROUTE_ALGORITHM) ) {
    zzmatch(ROUTE_ALGORITHM); consume();
    {
      if ( (LA(1)==EQUAL) ) {
        zzmatch(EQUAL); consume();
      }
      else {
        if ( (LA(1)==PATHFINDER) ) {
        }
        else {FAIL(1,err96,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    routeAlgorithmMode(  &prouteOptions_->algorithmMode  );
  }
  else {
    if ( (LA(1)==ROUTE_TYPE) ) {
      zzmatch(ROUTE_TYPE); consume();
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd13[LA(1)]&0x8) ) {
          }
          else {FAIL(1,err97,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      routeAbstractMode(  &prouteOptions_->abstractMode  );
    }
    else {
      if ( (LA(1)==ROUTE_WINDOW_SIZE) ) {
        zzmatch(ROUTE_WINDOW_SIZE); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (LA(1)==POS_INT) ) {
            }
            else {FAIL(1,err98,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        uintNum(  &prouteOptions_->windowSize  );
      }
      else {
        if ( (LA(1)==ROUTE_CHANNEL_WIDTH) ) {
          zzmatch(ROUTE_CHANNEL_WIDTH); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (LA(1)==POS_INT) ) {
              }
              else {FAIL(1,err99,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          uintNum(  &prouteOptions_->channelWidth  );
        }
        else {
          if ( (LA(1)==ROUTE_MAX_ITERATIONS) ) {
            zzmatch(ROUTE_MAX_ITERATIONS); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (LA(1)==POS_INT) ) {
                }
                else {FAIL(1,err100,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            uintNum(  &prouteOptions_->maxIterations  );
          }
          else {
            if ( (LA(1)==ROUTE_CONG_HIST_FACTOR) ) {
              zzmatch(ROUTE_CONG_HIST_FACTOR); consume();
              {
                if ( (LA(1)==EQUAL) ) {
                  zzmatch(EQUAL); consume();
                }
                else {
                  if ( (setwd13[LA(1)]&0x10) ) {
                  }
                  else {FAIL(1,err101,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              floatNum(  &prouteOptions_->histCongestionFactor  );
            }
            else {
              if ( (LA(1)==ROUTE_CONG_INIT_FACTOR) ) {
                zzmatch(ROUTE_CONG_INIT_FACTOR); consume();
                {
                  if ( (LA(1)==EQUAL) ) {
                    zzmatch(EQUAL); consume();
                  }
                  else {
                    if ( (setwd13[LA(1)]&0x20) ) {
                    }
                    else {FAIL(1,err102,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                floatNum(  &prouteOptions_->initCongestionFactor  );
              }
              else {
                if ( (LA(1)==ROUTE_CONG_PRESENT_FACTOR) ) {
                  zzmatch(ROUTE_CONG_PRESENT_FACTOR); consume();
                  {
                    if ( (LA(1)==EQUAL) ) {
                      zzmatch(EQUAL); consume();
                    }
                    else {
                      if ( (setwd13[LA(1)]&0x40) ) {
                      }
                      else {FAIL(1,err103,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                  floatNum(  &prouteOptions_->presentCongestionFactor  );
                }
                else {
                  if ( (LA(1)==ROUTE_BEND_COST_FACTOR) ) {
                    zzmatch(ROUTE_BEND_COST_FACTOR); consume();
                    {
                      if ( (LA(1)==EQUAL) ) {
                        zzmatch(EQUAL); consume();
                      }
                      else {
                        if ( (setwd13[LA(1)]&0x80) ) {
                        }
                        else {FAIL(1,err104,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                      }
                    }
                    floatNum(  &prouteOptions_->bendCostFactor  );
                  }
                  else {
                    if ( (LA(1)==ROUTE_RESOURCE_MODE) ) {
                      zzmatch(ROUTE_RESOURCE_MODE); consume();
                      {
                        if ( (LA(1)==EQUAL) ) {
                          zzmatch(EQUAL); consume();
                        }
                        else {
                          if ( (setwd14[LA(1)]&0x1) ) {
                          }
                          else {FAIL(1,err105,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                      }
                      routeResourceMode(  &prouteOptions_->resourceMode  );
                    }
                    else {
                      if ( (LA(1)==ROUTE_COST_MODE) ) {
                        zzmatch(ROUTE_COST_MODE); consume();
                        {
                          if ( (LA(1)==EQUAL) ) {
                            zzmatch(EQUAL); consume();
                          }
                          else {
                            if ( (setwd14[LA(1)]&0x2) ) {
                            }
                            else {FAIL(1,err106,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                          }
                        }
                        routeCostMode(  &prouteOptions_->costMode  );
                      }
                      else {
                        if ( (LA(1)==ROUTE_TIMING_ASTAR_FACTOR) ) {
                          zzmatch(ROUTE_TIMING_ASTAR_FACTOR); consume();
                          {
                            if ( (LA(1)==EQUAL) ) {
                              zzmatch(EQUAL); consume();
                            }
                            else {
                              if ( (setwd14[LA(1)]&0x4) ) {
                              }
                              else {FAIL(1,err107,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                            }
                          }
                          floatNum(  &prouteOptions_->timingAStarFactor  );
                        }
                        else {
                          if ( (LA(1)==ROUTE_TIMING_MAX_CRIT) ) {
                            zzmatch(ROUTE_TIMING_MAX_CRIT); consume();
                            {
                              if ( (LA(1)==EQUAL) ) {
                                zzmatch(EQUAL); consume();
                              }
                              else {
                                if ( (setwd14[LA(1)]&0x8) ) {
                                }
                                else {FAIL(1,err108,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                              }
                            }
                            floatNum(  &prouteOptions_->timingMaxCriticality  );
                          }
                          else {
                            if ( (LA(1)==ROUTE_TIMING_SLACK_CRIT) ) {
                              zzmatch(ROUTE_TIMING_SLACK_CRIT); consume();
                              {
                                if ( (LA(1)==EQUAL) ) {
                                  zzmatch(EQUAL); consume();
                                }
                                else {
                                  if ( (setwd14[LA(1)]&0x10) ) {
                                  }
                                  else {FAIL(1,err109,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                }
                              }
                              floatNum(  &prouteOptions_->slackCriticality  );
                              
                              this->srActiveCmd_ = ( LT( 1 ) ? LT( 1 )->getText( ) : "" );
                            }
                            else {FAIL(1,err110,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd14, 0x20);
}

void
TOP_OptionsParser_c::inputDataMode(TOS_InputDataMode_t* pmode)
{
  zzRULE;
  ANTLRTokenPtr modeListVal=NULL, modeVal=NULL;
  
  *pmode = TOS_INPUT_DATA_UNDEFINED;
  if ( (LA(1)==OPEN_BRACE) ) {
    zzmatch(OPEN_BRACE); consume();
    {
      int zzcnt=1;
      do {
        zzsetmatch(INPUT_DATA_VAL_set, INPUT_DATA_VAL_errset);
        modeListVal = (ANTLRTokenPtr)LT(1);

        
        *pmode = this->FindInputDataMode_( modeListVal->getType( ));
 consume();
      } while ( (setwd14[LA(1)]&0x40) );
    }
    zzmatch(CLOSE_BRACE); consume();
  }
  else {
    if ( (setwd14[LA(1)]&0x80) ) {
      zzsetmatch(INPUT_DATA_VAL_set, INPUT_DATA_VAL_errset);
      modeVal = (ANTLRTokenPtr)LT(1);

      
      *pmode = this->FindInputDataMode_( modeVal->getType( ));
 consume();
    }
    else {FAIL(1,err113,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd15, 0x1);
}

void
TOP_OptionsParser_c::outputLaffMask(int* pmask)
{
  zzRULE;
  ANTLRTokenPtr maskListVal=NULL, maskVal=NULL;
  
  *pmask = TOS_OUTPUT_LAFF_NONE;
  if ( (LA(1)==OPEN_BRACE) ) {
    zzmatch(OPEN_BRACE); consume();
    {
      int zzcnt=1;
      do {
        zzsetmatch(OUTPUT_LAFF_VAL_set, OUTPUT_LAFF_VAL_errset);
        maskListVal = (ANTLRTokenPtr)LT(1);

        
        *pmask |= this->FindOutputLaffMask_( maskListVal->getType( ));
 consume();
      } while ( (setwd15[LA(1)]&0x2) );
    }
    zzmatch(CLOSE_BRACE); consume();
  }
  else {
    if ( (setwd15[LA(1)]&0x4) ) {
      zzsetmatch(OUTPUT_LAFF_VAL_set, OUTPUT_LAFF_VAL_errset);
      maskVal = (ANTLRTokenPtr)LT(1);

      
      *pmask = this->FindOutputLaffMask_( maskVal->getType( ));
 consume();
    }
    else {FAIL(1,err116,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd15, 0x8);
}

void
TOP_OptionsParser_c::rcDelaysExtractMode(TOS_RcDelaysExtractMode_t* pmode)
{
  zzRULE;
  if ( (LA(1)==ELMORE) ) {
    zzmatch(ELMORE);
    
    *pmode = TOS_RC_DELAYS_EXTRACT_ELMORE;
 consume();
  }
  else {
    if ( (LA(1)==D2M) ) {
      zzmatch(D2M);
      
      *pmode = TOS_RC_DELAYS_EXTRACT_D2M;
 consume();
    }
    else {FAIL(1,err117,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd15, 0x10);
}

void
TOP_OptionsParser_c::rcDelaysSortMode(TOS_RcDelaysSortMode_t* pmode)
{
  zzRULE;
  if ( (LA(1)==BY_NETS) ) {
    zzmatch(BY_NETS);
    
    *pmode = TOS_RC_DELAYS_SORT_BY_NETS;
 consume();
  }
  else {
    if ( (LA(1)==BY_DELAYS) ) {
      zzmatch(BY_DELAYS);
      
      *pmode = TOS_RC_DELAYS_SORT_BY_DELAYS;
 consume();
    }
    else {FAIL(1,err118,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd15, 0x20);
}

void
TOP_OptionsParser_c::rcDelaysNameList(TOS_RcDelaysNameList_t* pnameList)
{
  zzRULE;
  
  string srString;
  if ( (LA(1)==OPEN_BRACE) ) {
    zzmatch(OPEN_BRACE); consume();
    {
      while ( (setwd15[LA(1)]&0x40) ) {
        stringText(  &srString  );
        
        pnameList->Add( srString );
      }
    }
    zzmatch(CLOSE_BRACE); consume();
  }
  else {
    if ( (setwd15[LA(1)]&0x80) ) {
      
      string srString_;
      stringText(  &srString_  );
      
      pnameList->Add( srString_ );
    }
    else {FAIL(1,err119,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd16, 0x1);
}

void
TOP_OptionsParser_c::displayNameList(TOS_DisplayNameList_t* pnameList)
{
  zzRULE;
  
  string srString;
  if ( (LA(1)==OPEN_BRACE) ) {
    zzmatch(OPEN_BRACE); consume();
    {
      while ( (setwd16[LA(1)]&0x2) ) {
        stringText(  &srString  );
        
        TC_Name_c name( srString );
        pnameList->Add( name );
      }
    }
    zzmatch(CLOSE_BRACE); consume();
  }
  else {
    if ( (setwd16[LA(1)]&0x4) ) {
      
      string srString_;
      stringText(  &srString_  );
      
      TC_Name_c name( srString_ );
      pnameList->Add( name );
    }
    else {FAIL(1,err120,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd16, 0x8);
}

void
TOP_OptionsParser_c::executeToolMask(int* pmask)
{
  zzRULE;
  ANTLRTokenPtr maskListVal=NULL, maskVal=NULL;
  
  *pmask = TOS_EXECUTE_TOOL_NONE;
  if ( (LA(1)==OPEN_BRACE) ) {
    zzmatch(OPEN_BRACE); consume();
    {
      int zzcnt=1;
      do {
        zzsetmatch(EXECUTE_TOOL_VAL_set, EXECUTE_TOOL_VAL_errset);
        maskListVal = (ANTLRTokenPtr)LT(1);

        
        *pmask |= this->FindExecuteToolMask_( maskListVal->getType( ));
 consume();
      } while ( (setwd16[LA(1)]&0x10) );
    }
    zzmatch(CLOSE_BRACE); consume();
  }
  else {
    if ( (setwd16[LA(1)]&0x20) ) {
      zzsetmatch(EXECUTE_TOOL_VAL_set, EXECUTE_TOOL_VAL_errset);
      maskVal = (ANTLRTokenPtr)LT(1);

      
      *pmask = this->FindExecuteToolMask_( maskVal->getType( ));
 consume();
    }
    else {FAIL(1,err123,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd16, 0x40);
}

void
TOP_OptionsParser_c::packAlgorithmMode(TOS_PackAlgorithmMode_t* pmode)
{
  zzRULE;
  zzmatch(AAPACK);
  
  *pmode = TOS_PACK_ALGORITHM_AAPACK;
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd16, 0x80);
}

void
TOP_OptionsParser_c::packClusterNetsMode(TOS_PackClusterNetsMode_t* pmode)
{
  zzRULE;
  if ( (LA(1)==MIN_CONNECTIONS) ) {
    zzmatch(MIN_CONNECTIONS);
    
    *pmode = TOS_PACK_CLUSTER_NETS_MIN_CONNECTIONS;
 consume();
  }
  else {
    if ( (LA(1)==MAX_CONNECTIONS) ) {
      zzmatch(MAX_CONNECTIONS);
      
      *pmode = TOS_PACK_CLUSTER_NETS_MAX_CONNECTIONS;
 consume();
    }
    else {FAIL(1,err124,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd17, 0x1);
}

void
TOP_OptionsParser_c::packAffinityMode(TOS_PackAffinityMode_t* pmode)
{
  zzRULE;
  if ( (LA(1)==NONE) ) {
    zzmatch(NONE);
    
    *pmode = TOS_PACK_AFFINITY_NONE;
 consume();
  }
  else {
    if ( (LA(1)==ANY) ) {
      zzmatch(ANY);
      
      *pmode = TOS_PACK_AFFINITY_ANY;
 consume();
    }
    else {FAIL(1,err125,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd17, 0x2);
}

void
TOP_OptionsParser_c::packCostMode(TOS_PackCostMode_t* pmode)
{
  zzRULE;
  if ( (LA(1)==ROUTABILITY_DRIVEN) ) {
    zzmatch(ROUTABILITY_DRIVEN);
    
    *pmode = TOS_PACK_COST_ROUTABILITY_DRIVEN;
 consume();
  }
  else {
    if ( (LA(1)==TIMING_DRIVEN) ) {
      zzmatch(TIMING_DRIVEN);
      
      *pmode = TOS_PACK_COST_TIMING_DRIVEN;
 consume();
    }
    else {FAIL(1,err126,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd17, 0x4);
}

void
TOP_OptionsParser_c::placeAlgorithmMode(TOS_PlaceAlgorithmMode_t* pmode)
{
  zzRULE;
  zzmatch(ANNEALING);
  
  *pmode = TOS_PLACE_ALGORITHM_ANNEALING;
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd17, 0x8);
}

void
TOP_OptionsParser_c::placeCostMode(TOS_PlaceCostMode_t* pmode)
{
  zzRULE;
  if ( (LA(1)==ROUTABILITY_DRIVEN) ) {
    zzmatch(ROUTABILITY_DRIVEN);
    
    *pmode = TOS_PLACE_COST_ROUTABILITY_DRIVEN;
 consume();
  }
  else {
    if ( (LA(1)==TIMING_DRIVEN) ) {
      zzmatch(TIMING_DRIVEN);
      
      *pmode = TOS_PLACE_COST_PATH_TIMING_DRIVEN;
 consume();
    }
    else {
      if ( (LA(1)==PATH_TIMING_DRIVEN) ) {
        zzmatch(PATH_TIMING_DRIVEN);
        
        *pmode = TOS_PLACE_COST_PATH_TIMING_DRIVEN;
 consume();
      }
      else {
        if ( (LA(1)==NET_TIMING_DRIVEN) ) {
          zzmatch(NET_TIMING_DRIVEN);
          
          *pmode = TOS_PLACE_COST_NET_TIMING_DRIVEN;
 consume();
        }
        else {FAIL(1,err127,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd17, 0x10);
}

void
TOP_OptionsParser_c::routeAlgorithmMode(TOS_RouteAlgorithmMode_t* pmode)
{
  zzRULE;
  zzmatch(PATHFINDER);
  
  *pmode = TOS_ROUTE_ALGORITHM_PATHFINDER;
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd17, 0x20);
}

void
TOP_OptionsParser_c::routeAbstractMode(TOS_RouteAbstractMode_t* pmode)
{
  zzRULE;
  if ( (LA(1)==GLOBAL) ) {
    zzmatch(GLOBAL);
    
    *pmode = TOS_ROUTE_ABSTRACT_GLOBAL;
 consume();
  }
  else {
    if ( (LA(1)==DETAILED) ) {
      zzmatch(DETAILED);
      
      *pmode = TOS_ROUTE_ABSTRACT_DETAILED;
 consume();
    }
    else {FAIL(1,err128,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd17, 0x40);
}

void
TOP_OptionsParser_c::routeResourceMode(TOS_RouteResourceMode_t* pmode)
{
  zzRULE;
  if ( (LA(1)==DEMAND_ONLY) ) {
    zzmatch(DEMAND_ONLY);
    
    *pmode = TOS_ROUTE_RESOURCE_DEMAND_ONLY;
 consume();
  }
  else {
    if ( (LA(1)==DELAY_NORMALIZED) ) {
      zzmatch(DELAY_NORMALIZED);
      
      *pmode = TOS_ROUTE_RESOURCE_DELAY_NORMALIZED;
 consume();
    }
    else {FAIL(1,err129,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd17, 0x80);
}

void
TOP_OptionsParser_c::routeCostMode(TOS_RouteCostMode_t* pmode)
{
  zzRULE;
  if ( (LA(1)==BREADTH_FIRST) ) {
    zzmatch(BREADTH_FIRST);
    
    *pmode = TOS_ROUTE_COST_BREADTH_FIRST;
 consume();
  }
  else {
    if ( (LA(1)==TIMING_DRIVEN) ) {
      zzmatch(TIMING_DRIVEN);
      
      *pmode = TOS_ROUTE_COST_TIMING_DRIVEN;
 consume();
    }
    else {FAIL(1,err130,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd18, 0x1);
}

void
TOP_OptionsParser_c::name(TC_Name_c& srName)
{
  zzRULE;
  ANTLRTokenPtr qnameVal=NULL, nameVal=NULL;
  
  srName = "";
  if ( (LA(1)==OPEN_QUOTE) ) {
    zzmatch(OPEN_QUOTE); consume();
    {
      if ( (LA(1)==STRING) ) {
        zzmatch(STRING);
        qnameVal = (ANTLRTokenPtr)LT(1);

        
        srName = qnameVal->getText( );
 consume();
      }
      else {
        if ( (LA(1)==CLOSE_QUOTE) ) {
        }
        else {FAIL(1,err131,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    zzmatch(CLOSE_QUOTE); consume();
  }
  else {
    if ( (LA(1)==STRING) ) {
      zzmatch(STRING);
      nameVal = (ANTLRTokenPtr)LT(1);

      
      srName = nameVal->getText( );
 consume();
    }
    else {FAIL(1,err132,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd18, 0x2);
}

void
TOP_OptionsParser_c::stringText(string* psrString)
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
        else {FAIL(1,err133,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
    else {FAIL(1,err134,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd18, 0x4);
}

void
TOP_OptionsParser_c::floatNum(double* pdouble)
{
  zzRULE;
  ANTLRTokenPtr fVal=NULL, posIntVal=NULL, negIntVal=NULL;
  if ( (LA(1)==FLOAT) ) {
    zzmatch(FLOAT);
    fVal = (ANTLRTokenPtr)LT(1);

    
    *pdouble = atof( fVal->getText( ));
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
      else {FAIL(1,err135,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd18, 0x8);
}

void
TOP_OptionsParser_c::intNum(int* pint)
{
  zzRULE;
  ANTLRTokenPtr posIntVal=NULL, negIntVal=NULL;
  if ( (LA(1)==POS_INT) ) {
    zzmatch(POS_INT);
    posIntVal = (ANTLRTokenPtr)LT(1);

    
    *pint = atoi( posIntVal->getText( ));
 consume();
  }
  else {
    if ( (LA(1)==NEG_INT) ) {
      zzmatch(NEG_INT);
      negIntVal = (ANTLRTokenPtr)LT(1);

      
      *pint = atoi( negIntVal->getText( ));
 consume();
    }
    else {FAIL(1,err136,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd18, 0x10);
}

void
TOP_OptionsParser_c::uintNum(unsigned int* puint)
{
  zzRULE;
  ANTLRTokenPtr uintVal=NULL;
  zzmatch(POS_INT);
  uintVal = (ANTLRTokenPtr)LT(1);

  
  *puint = static_cast< unsigned int >( atol( uintVal->getText( )));
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd18, 0x20);
}

void
TOP_OptionsParser_c::ulongNum(unsigned long* plong)
{
  zzRULE;
  ANTLRTokenPtr ulongVal=NULL;
  if ( (LA(1)==POS_INT) ) {
    zzmatch(POS_INT);
    ulongVal = (ANTLRTokenPtr)LT(1);

    
    *plong = atol( ulongVal->getText( ));
 consume();
  }
  else {
    if ( (LA(1)==NEG_INT) ) {
      zzmatch(NEG_INT);
      
      *plong = ULONG_MAX;
 consume();
    }
    else {FAIL(1,err137,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd18, 0x40);
}

void
TOP_OptionsParser_c::boolType(bool* pbool)
{
  zzRULE;
  ANTLRTokenPtr boolVal=NULL;
  zzsetmatch(BOOL_set, BOOL_errset);
  boolVal = (ANTLRTokenPtr)LT(1);

  
  *pbool = this->FindBool_( boolVal->getType( ));
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd18, 0x80);
}

#include "TOP_OptionsGrammar.h"
