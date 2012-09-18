//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_MessageOptions 
//           class.
//
//===========================================================================//

#ifndef TOS_MESSAGE_OPTIONS_H
#define TOS_MESSAGE_OPTIONS_H

#include <stdio.h>

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOS_MessageOptions_c
{
public:

   TOS_MessageOptions_c( void );
   TOS_MessageOptions_c( double minGridPrecision,
                         bool timeStampsEnable,
                         const TOS_DisplayNameList_t& infoAcceptList,
                         const TOS_DisplayNameList_t& infoRejectList,
                         const TOS_DisplayNameList_t& warningAcceptList,
                         const TOS_DisplayNameList_t& warningRejectList,
                         const TOS_DisplayNameList_t& errorAcceptList,
                         const TOS_DisplayNameList_t& errorRejectList,
                         const TOS_DisplayNameList_t& traceAcceptList,
                         const TOS_DisplayNameList_t& traceRejectList );
   ~TOS_MessageOptions_c( void );

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

public:

   double minGridPrecision; // Define min (manufacturing) grid precision (for messages)
   bool   timeStampsEnable; // Enable prefix time stamp on messages

   class TOS_MessageInfo_c
   {
   public:

      TOS_DisplayNameList_t acceptList;  // Display info enable/disable list
      TOS_DisplayNameList_t rejectList; // "
   } info;

   class TOS_MessageWarning_c
   {
   public:

      TOS_DisplayNameList_t acceptList; // Display warning enable/disable list
      TOS_DisplayNameList_t rejectList; // "
   } warning;

   class TOS_MessageError_c
   {
   public:

      TOS_DisplayNameList_t acceptList; // Display error enable/disable list
      TOS_DisplayNameList_t rejectList; // "
   } error;

   class TOS_MessageTrace_c
   {
   public:

      TOS_DisplayNameList_t acceptList; // Display trace enable/disable list
      TOS_DisplayNameList_t rejectList; // "

      class TOS_MessageTraceRead_c
      {
      public:

         bool options;        // Define various trace read messages
         bool xml;            // "
         bool blif;           // "
         bool architecture;   // "
         bool fabric;         // "
         bool circuit;        // "
      } read;

      class TOS_MessageTraceVPR_c
      {
      public:

         bool showSetup;      // Define various trace VPR messages
         bool echoFile;       // "
         TOS_EchoFileNameList_t echoFileNameList;
      } vpr;
   } trace;

private:

   enum TOS_DefCapacity_e 
   { 
      TOS_DISPLAY_INFO_LIST_DEF_CAPACITY = 1,
      TOS_DISPLAY_WARNING_LIST_DEF_CAPACITY = 1,
      TOS_DISPLAY_ERROR_LIST_DEF_CAPACITY = 1,
      TOS_DISPLAY_TRACE_LIST_DEF_CAPACITY = 1
   };
};

#endif 
