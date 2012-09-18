/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 *
 *   ..\..\pccts\bin\Antlr.exe -CC -gh TCP_CircuitGrammar.g
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
#include "TC_StringUtils.h"

#include "TCD_CircuitDesign.h"

#include "TCP_CircuitFile.h"
#include "TCP_CircuitScanner_c.h"
#include "AParser.h"
#include "TCP_CircuitParser_c.h"
#include "DLexerBase.h"
#include "ATokPtr.h"

/* MR23 In order to remove calls to PURIFY use the antlr -nopurify option */

#ifndef PCCTS_PURIFY
#define PCCTS_PURIFY(r,s) memset((char *) &(r),'\0',(s));
#endif


void
TCP_CircuitParser_c::start(void)
{
  zzRULE;
  zzmatch(63); consume();
  zzmatch(CIRCUIT); consume();
  stringText(  &pcircuitDesign_->srName  );
  zzmatch(64); consume();
  {
    while ( (LA(1)==63) ) {
      zzmatch(63); consume();
      {
        if ( (LA(1)==PB) ) {
          blockList(  &pcircuitDesign_->blockList  );
        }
        else {
          if ( (setwd1[LA(1)]&0x1) ) {
            portList(  &pcircuitDesign_->portList  );
          }
          else {
            if ( (LA(1)==INST) ) {
              instList(  &pcircuitDesign_->instList  );
            }
            else {
              if ( (LA(1)==NET) ) {
                netList(  &pcircuitDesign_->netList  );
              }
              else {FAIL(1,err1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
      }
    }
  }
  zzmatch(65); consume();
  zzmatch(CIRCUIT); consume();
  zzmatch(64); consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x2);
}

void
TCP_CircuitParser_c::blockList(TPO_InstList_t* pblockList)
{
  zzRULE;
  ANTLRTokenPtr statusVal=NULL;
  
  TPO_Inst_c inst;
  
      string srName;
  string srCellName;
  string srPlaceFabricName;
  TPO_HierMapList_t hierMapList_;
  TPO_RelativeList_t relativeList_;
  TGS_RegionList_t regionList_;
  zzmatch(PB); consume();
  stringText(  &srName  );
  
  inst.SetName( srName );
  {
    for (;;) {
      if ( !((setwd1[LA(1)]&0x4))) break;
      if ( (setwd1[LA(1)]&0x8) ) {
        {
          if ( (LA(1)==PB) ) {
            zzmatch(PB); consume();
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
            if ( (setwd1[LA(1)]&0x10) ) {
            }
            else {FAIL(1,err3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        stringText(  &srCellName  );
        
        inst.SetCellName( srCellName );
      }
      else {
        if ( (LA(1)==STATUS) ) {
          zzmatch(STATUS); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd1[LA(1)]&0x20) ) {
              }
              else {FAIL(1,err4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          zzsetmatch(PLACE_STATUS_VAL_set, PLACE_STATUS_VAL_errset);
          statusVal = (ANTLRTokenPtr)LT(1);

          
          inst.SetPlaceStatus( this->FindPlaceStatusMode_( statusVal->getType( )));
 consume();
        }
        else break; /* MR6 code for exiting loop "for sure" */
      }
    }
  }
  zzmatch(64); consume();
  {
    while ( (LA(1)==63) ) {
      zzmatch(63); consume();
      {
        if ( (LA(1)==PACK) ) {
          zzmatch(PACK); consume();
          hierMapList(  &hierMapList_  );
        }
        else {
          if ( (LA(1)==PLACE) ) {
            zzmatch(PLACE); consume();
            stringText(  &srPlaceFabricName  );
            
            inst.SetPlaceFabricName( srPlaceFabricName );
          }
          else {
            if ( (LA(1)==RELATIVE) ) {
              zzmatch(RELATIVE); consume();
              relativeList(  &relativeList_  );
            }
            else {
              if ( (LA(1)==REGION) ) {
                zzmatch(REGION); consume();
                regionList(  &regionList_  );
              }
              else {FAIL(1,err7,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
      }
      zzmatch(64); consume();
    }
  }
  zzmatch(65); consume();
  zzmatch(PB); consume();
  zzmatch(64);
  
  if( inst.IsValid( ))
  {
    inst.SetPackHierMapList( hierMapList_ );
    inst.SetPlaceRelativeList( relativeList_ );
    inst.SetPlaceRegionList( regionList_ );
    
         pblockList->Add( inst );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd1, 0x40);
}

void
TCP_CircuitParser_c::portList(TPO_PortList_t* pportList)
{
  zzRULE;
  ANTLRTokenPtr statusVal=NULL;
  
  TPO_Inst_c port;
  TPO_Pin_t pin;
  TPO_InstSource_t source;
  
      string srName;
  string srCellName;
  string srPlaceFabricName;
  {
    if ( (LA(1)==PORT) ) {
      zzmatch(PORT); consume();
    }
    else {
      if ( (LA(1)==IO) ) {
        zzmatch(IO); consume();
      }
      else {FAIL(1,err8,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  stringText(  &srName  );
  
  port.SetName( srName );
  {
    for (;;) {
      if ( !((setwd1[LA(1)]&0x80))) break;
      if ( (setwd2[LA(1)]&0x1) ) {
        {
          if ( (LA(1)==CELL) ) {
            zzmatch(CELL); consume();
          }
          else {
            if ( (LA(1)==MASTER) ) {
              zzmatch(MASTER); consume();
            }
            else {FAIL(1,err9,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd2[LA(1)]&0x2) ) {
            }
            else {FAIL(1,err10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        cellSourceText(  &srCellName,
                                                  &source  );
        
        port.SetCellName( srCellName );
        port.SetSource( source );
      }
      else {
        if ( (LA(1)==STATUS) ) {
          zzmatch(STATUS); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd2[LA(1)]&0x4) ) {
              }
              else {FAIL(1,err11,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          zzsetmatch(PLACE_STATUS_VAL_set, PLACE_STATUS_VAL_errset);
          statusVal = (ANTLRTokenPtr)LT(1);

          
          port.SetPlaceStatus( this->FindPlaceStatusMode_( statusVal->getType( )));
 consume();
        }
        else break; /* MR6 code for exiting loop "for sure" */
      }
    }
  }
  zzmatch(64); consume();
  {
    while ( (LA(1)==63) ) {
      zzmatch(63); consume();
      {
        if ( (LA(1)==PIN) ) {
          zzmatch(PIN); consume();
          pinDef(  &pin  );
          zzmatch(64);
          
          port.AddPin( pin);
 consume();
        }
        else {
          if ( (LA(1)==PLACE) ) {
            zzmatch(PLACE); consume();
            stringText(  &srPlaceFabricName  );
            zzmatch(67);
            
            port.SetPlaceFabricName( srPlaceFabricName );
 consume();
          }
          else {FAIL(1,err12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  zzmatch(65); consume();
  {
    if ( (LA(1)==PORT) ) {
      zzmatch(PORT); consume();
    }
    else {
      if ( (LA(1)==IO) ) {
        zzmatch(IO); consume();
      }
      else {FAIL(1,err13,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  zzmatch(64);
  
  if( port.IsValid( ))
  {
    pportList->Add( port );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x8);
}

void
TCP_CircuitParser_c::instList(TPO_InstList_t* pinstList)
{
  zzRULE;
  
  TPO_Inst_c inst;
  TPO_Pin_t pin;
  TPO_InstSource_t source;
  
      string srName;
  string srCellName;
  zzmatch(INST); consume();
  stringText(  &srName  );
  
  inst.SetName( srName );
  {
    while ( (setwd2[LA(1)]&0x10) ) {
      {
        if ( (LA(1)==CELL) ) {
          zzmatch(CELL); consume();
        }
        else {
          if ( (LA(1)==MASTER) ) {
            zzmatch(MASTER); consume();
          }
          else {FAIL(1,err14,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      {
        if ( (LA(1)==EQUAL) ) {
          zzmatch(EQUAL); consume();
        }
        else {
          if ( (setwd2[LA(1)]&0x20) ) {
          }
          else {FAIL(1,err15,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      cellSourceText(  &srCellName,
                                                  &source  );
      
      inst.SetCellName( srCellName );
      inst.SetSource( source );
    }
  }
  zzmatch(64); consume();
  {
    while ( (LA(1)==63) ) {
      zzmatch(63); consume();
      zzmatch(PIN); consume();
      pinDef(  &pin  );
      
      inst.AddPin( pin );
      zzmatch(64); consume();
    }
  }
  zzmatch(65); consume();
  zzmatch(INST); consume();
  zzmatch(64);
  
  if( inst.IsValid( ))
  {
    pinstList->Add( inst );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd2, 0x40);
}

void
TCP_CircuitParser_c::netList(TNO_NetList_c* pnetList)
{
  zzRULE;
  ANTLRTokenPtr typeVal=NULL, boolVal=NULL, statusVal=NULL;
  
  TNO_Net_c net;
  
      string srName;
  TNO_InstPinList_t instPinList_;
  TNO_GlobalRouteList_t globalRouteList_;
  TNO_RouteList_t routeList_;
  zzmatch(NET); consume();
  stringText(  &srName  );
  
  net.SetName( srName );
  {
    for (;;) {
      if ( !((setwd2[LA(1)]&0x80))) break;
      if ( (LA(1)==TYPE) ) {
        zzmatch(TYPE); consume();
        {
          if ( (LA(1)==EQUAL) ) {
            zzmatch(EQUAL); consume();
          }
          else {
            if ( (setwd3[LA(1)]&0x1) ) {
            }
            else {FAIL(1,err16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzsetmatch(TYPE_VAL_set, TYPE_VAL_errset);
        typeVal = (ANTLRTokenPtr)LT(1);

        
        net.SetType( this->FindTypeMode_( typeVal->getType( )));
 consume();
      }
      else {
        if ( (LA(1)==ROUTABLE) ) {
          zzmatch(ROUTABLE); consume();
          {
            if ( (LA(1)==EQUAL) ) {
              zzmatch(EQUAL); consume();
            }
            else {
              if ( (setwd3[LA(1)]&0x2) ) {
              }
              else {FAIL(1,err19,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          zzsetmatch(BOOL_VAL_set, BOOL_VAL_errset);
          boolVal = (ANTLRTokenPtr)LT(1);

          
          net.SetRoutable( this->FindBool_( boolVal->getType( )));
 consume();
        }
        else {
          if ( (LA(1)==STATUS) ) {
            zzmatch(STATUS); consume();
            {
              if ( (LA(1)==EQUAL) ) {
                zzmatch(EQUAL); consume();
              }
              else {
                if ( (setwd3[LA(1)]&0x4) ) {
                }
                else {FAIL(1,err22,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            zzsetmatch(NET_STATUS_VAL_set, NET_STATUS_VAL_errset);
            statusVal = (ANTLRTokenPtr)LT(1);

            
            net.SetStatus( this->FindNetStatusMode_( statusVal->getType( )));
 consume();
          }
          else break; /* MR6 code for exiting loop "for sure" */
        }
      }
    }
  }
  zzmatch(64); consume();
  {
    while ( (LA(1)==63) ) {
      zzmatch(63); consume();
      {
        if ( (LA(1)==PIN) ) {
          zzmatch(PIN); consume();
          instPinList(  &instPinList_  );
          zzmatch(64); consume();
        }
        else {
          if ( (LA(1)==GROUTE) ) {
            zzmatch(GROUTE); consume();
            globalRouteList(  &globalRouteList_  );
            zzmatch(64); consume();
          }
          else {
            if ( (LA(1)==ROUTE) ) {
              zzmatch(ROUTE); consume();
              routeList(  &routeList_  );
              zzmatch(67); consume();
            }
            else {FAIL(1,err25,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
    }
  }
  zzmatch(65); consume();
  zzmatch(NET); consume();
  zzmatch(64);
  
  if( net.IsValid( ))
  {
    net.AddInstPinList( instPinList_ );
    net.AddGlobalRouteList( globalRouteList_ );
    net.AddRouteList( routeList_ );
    
         pnetList->Add( net );
  }
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x8);
}

void
TCP_CircuitParser_c::pinDef(TPO_Pin_t* ppin)
{
  zzRULE;
  ANTLRTokenPtr typeVal=NULL;
  
  string srName;
  TC_TypeMode_t type = TC_TYPE_UNDEFINED;
  
      ppin->Clear( );
  stringText(  &srName  );
  zzsetmatch(TYPE_VAL_set, TYPE_VAL_errset);
  typeVal = (ANTLRTokenPtr)LT(1);

  
 consume();
  
  type = this->FindTypeMode_( typeVal->getType( ));
  ppin->Set( srName, type );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x10);
}

void
TCP_CircuitParser_c::instPinList(TNO_InstPinList_t* pinstPinList)
{
  zzRULE;
  
  TNO_InstPin_c instPin;
  instPinDef(  &instPin  );
  
  if( instPin.IsValid( ))
  {
    pinstPinList->Add( instPin );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x20);
}

void
TCP_CircuitParser_c::instPinDef(TNO_InstPin_c* pinstPin)
{
  zzRULE;
  ANTLRTokenPtr typeVal=NULL;
  
  string srInstName, srPinName;
  
      pinstPin->Clear( );
  stringText(  &srInstName  );
  stringText(  &srPinName  );
  
  pinstPin->Set( srInstName, srPinName );
  zzsetmatch(TYPE_VAL_set, TYPE_VAL_errset);
  typeVal = (ANTLRTokenPtr)LT(1);

  
  pinstPin->SetType( this->FindTypeMode_( typeVal->getType( )));
 consume();
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x40);
}

void
TCP_CircuitParser_c::globalRouteList(TNO_GlobalRouteList_t* pglobalRouteList)
{
  zzRULE;
  
  TNO_GlobalRoute_t globalRoute;
  
      string srName;
  unsigned int length = 0;
  stringText(  &srName  );
  uintNum(  &length  );
  
  globalRoute.Set( srName, length );
  pglobalRouteList->Add( globalRoute );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd3, 0x80);
}

void
TCP_CircuitParser_c::routeList(TNO_RouteList_t* prouteList)
{
  zzRULE;
  
  TNO_Route_c route;
  
      TNO_InstPin_c instPin;
  TNO_SwitchBox_c switchBox;
  TNO_Channel_t channel;
  {
    while ( (LA(1)==63) ) {
      zzmatch(63); consume();
      {
        if ( (LA(1)==PIN) ) {
          zzmatch(PIN); consume();
          instPinDef(  &instPin  );
          
          route.Clear( );
          route.Set( instPin );
        }
        else {
          if ( (LA(1)==SB) ) {
            zzmatch(SB); consume();
            switchBoxDef(  &switchBox  );
            
            route.Clear( );
            route.Set( switchBox );
          }
          else {
            if ( (LA(1)==CHANNEL) ) {
              zzmatch(CHANNEL); consume();
              channelDef(  &channel  );
              
              route.Clear( );
              route.Set( channel );
            }
            else {FAIL(1,err26,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
      zzmatch(64);
      
      if( route.IsValid( ))
      {
        prouteList->Add( route );
      }
 consume();
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x1);
}

void
TCP_CircuitParser_c::switchBoxDef(TNO_SwitchBox_c* pswitchBox)
{
  zzRULE;
  
  string srName;
  TC_SideIndex_c sideIndex_;
  
      pswitchBox->Clear( );
  stringText(  &srName  );
  
  pswitchBox->SetName( srName );
  sideIndex(  &sideIndex_  );
  
  pswitchBox->SetInputPin( sideIndex_ );
  sideIndex(  &sideIndex_  );
  
  pswitchBox->SetOutputPin( sideIndex_ );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x2);
}

void
TCP_CircuitParser_c::channelDef(TNO_Channel_t* pchannel)
{
  zzRULE;
  
  string srName;
  unsigned int index = 0;
  
      pchannel->Clear( );
  stringText(  &srName  );
  uintNum(  &index  );
  
  pchannel->Set( srName, index );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x4);
}

void
TCP_CircuitParser_c::hierMapList(TPO_HierMapList_t* phierMapList)
{
  zzRULE;
  
  TPO_HierMap_c hierMap;
  
      string srName;
  TPO_HierNameList_t hierNameList;
  stringText(  &srName  );
  
  hierMap.SetInstName( srName );
  {
    zzmatch(63); consume();
    zzmatch(HIER); consume();
    {
      while ( (setwd4[LA(1)]&0x8) ) {
        stringText(  &srName  );
        
        hierNameList.Add( srName );
      }
    }
    zzmatch(64);
    
    hierMap.SetHierNameList( hierNameList );
 consume();
  }
  
  if( hierMap.IsValid( ))
  {
    phierMapList->Add( hierMap );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x10);
}

void
TCP_CircuitParser_c::relativeList(TPO_RelativeList_t* prelativeList)
{
  zzRULE;
  ANTLRTokenPtr sideVal=NULL;
  
  TPO_Relative_t relative;
  
      TC_SideMode_t side = TC_SIDE_UNDEFINED;
  string srName;
  zzsetmatch(SIDE_VAL_set, SIDE_VAL_errset);
  sideVal = (ANTLRTokenPtr)LT(1);

  
  side = this->FindSideMode_( sideVal->getType( ));
 consume();
  stringText(  &srName  );
  
  relative.Set( side, srName );
  prelativeList->Add( relative );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x20);
}

void
TCP_CircuitParser_c::sideIndex(TC_SideIndex_c* psideIndex)
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
  resynch(setwd4, 0x40);
}

void
TCP_CircuitParser_c::regionList(TGS_RegionList_t* pregionList)
{
  zzRULE;
  
  TGS_Region_c region;
  regionDef(  &region  );
  
  if( region.IsValid( ))
  {
    pregionList->Add( region );
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd4, 0x80);
}

void
TCP_CircuitParser_c::regionDef(TGS_Region_c* pregion)
{
  zzRULE;
  floatNum(  &pregion->x1  );
  floatNum(  &pregion->y1  );
  floatNum(  &pregion->x2  );
  floatNum(  &pregion->y2  );
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x1);
}

void
TCP_CircuitParser_c::cellSourceText(string* psrString,TPO_InstSource_t* psource)
{
  zzRULE;
  stringText(  psrString  );
  
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
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x2);
}

void
TCP_CircuitParser_c::stringText(string* psrString)
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
        else {FAIL(1,err29,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
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
    else {FAIL(1,err30,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x4);
}

void
TCP_CircuitParser_c::floatNum(double* pdouble)
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
        else {FAIL(1,err31,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x8);
}

void
TCP_CircuitParser_c::expNum(double* pdouble)
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
          else {FAIL(1,err32,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
    }
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x10);
}

void
TCP_CircuitParser_c::uintNum(unsigned int* puint)
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
    else {FAIL(1,err33,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  return;
fail:
  syn(zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk);
  resynch(setwd5, 0x20);
}

#include "TCP_CircuitGrammar.h"
