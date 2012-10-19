//===========================================================================//
// Purpose : Method definitions for the TIO_PrintHandler class.
//
//           Public methods include:
//           - NewInstance, DeleteInstance, GetInstance, HasInstance
//           - Info, Warning, Error, Fatal, Trace, Internal, Direct
//           - Write
//           - WriteBanner
//           - WriteCenter
//           - WriteVersion
//           - WriteHelp
//           - SetStdioOutput 
//           - SetCustomOutput 
//           - SetLogFileOutput
//           - SetUserFileOutput
//           - AddInfoAcceptRegExp
//           - AddInfoRejectRegExp
//           - AddWarningAcceptRegExp
//           - AddWarningRejectRegExp
//           - AddErrorAcceptRegExp
//           - AddErrorRejectRegExp
//           - AddTraceAcceptRegExp
//           - AddTraceRejectRegExp
//           - FindUserName
//           - FindHostName
//           - FindWorkingDir
//           - EnableOutput, DisableOutput
//           - IsOutputEnabled
//           - IsWithinMaxWarningCount
//           - IsWithinMaxErrorCount
//           - IsWithinMaxCount
//           - IsValidMaxWarningCount
//           - IsValidMaxErrorCount
//           - IsValidNew
//
//           Protected methods include:
//           - TIO_PrintHandler_c, ~TIO_PrintHandler_c
//
//           Private methods include:
//           - WriteMessage_
//           - FormatMessage_
//           - PrefixMessage_
//           - OutputMessage_
//           - ApplySystemCommand_
//
//===========================================================================//

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
using namespace std;

#if defined( SUN8 ) || defined( SUN10 )
   #include <time.h>
#endif

#include "TC_Typedefs.h"
#include "TC_StringUtils.h"
#include "TC_Name.h"
#include "TCT_NameList.h"

#include "TIO_Typedefs.h"
#include "TIO_StringText.h"
#include "TIO_SkinHandler.h"
#include "TIO_PrintHandler.h"

// Initialize the user message "singleton" class, as needed
TIO_PrintHandler_c* TIO_PrintHandler_c::pinstance_ = 0;

//===========================================================================//
// Method         : TIO_PrintHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_PrintHandler_c::TIO_PrintHandler_c( 
      void )
{
   this->outputs_.isStdioEnabled = false;
   this->outputs_.isCustomEnabled = false;
   this->outputs_.isLogFileEnabled = false;
   this->outputs_.isUserFileEnabled = false;

   this->display_.pinfoAcceptList = 0;
   this->display_.pinfoRejectList = 0;
   this->display_.pwarningAcceptList = 0;
   this->display_.pwarningRejectList = 0;
   this->display_.perrorAcceptList = 0;
   this->display_.perrorRejectList = 0;
   this->display_.ptraceAcceptList = 0;
   this->display_.ptraceRejectList = 0;

   this->counts_.warningCount = 0;
   this->counts_.errorCount = 0;
   this->counts_.internalCount = 0;

   this->counts_.maxWarningCount = UINT_MAX;
   this->counts_.maxErrorCount = 1;

   this->counts_.isMaxWarningEnabled = true;
   this->counts_.isMaxErrorEnabled = true;

   this->formats_.timeStampsEnabled = false;
   this->formats_.srPrefix = "";

   pinstance_ = 0;
}

//===========================================================================//
// Method         : ~TIO_PrintHandler_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_PrintHandler_c::~TIO_PrintHandler_c( 
      void )
{
   TCT_NameList_c< TC_Name_c >* pinfoAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pinfoAcceptList );
   TCT_NameList_c< TC_Name_c >* pinfoRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pinfoRejectList );
   TCT_NameList_c< TC_Name_c >* pwarningAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pwarningAcceptList );
   TCT_NameList_c< TC_Name_c >* pwarningRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pwarningRejectList );
   TCT_NameList_c< TC_Name_c >* perrorAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.perrorAcceptList );
   TCT_NameList_c< TC_Name_c >* perrorRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.perrorRejectList );
   TCT_NameList_c< TC_Name_c >* ptraceAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.ptraceAcceptList );
   TCT_NameList_c< TC_Name_c >* ptraceRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.ptraceRejectList );

   delete pinfoAcceptList;
   delete pinfoRejectList;
   delete pwarningAcceptList;
   delete pwarningRejectList;
   delete perrorAcceptList;
   delete perrorRejectList;
   delete ptraceAcceptList;
   delete ptraceRejectList;
}

//===========================================================================//
// Method         : NewInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::NewInstance( 
      void )
{
   pinstance_ = new TC_NOTHROW TIO_PrintHandler_c;
}

//===========================================================================//
// Method         : DeleteInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::DeleteInstance( 
      void )
{
   if( pinstance_ )
   {
      delete pinstance_;
      pinstance_ = 0;
   }
}

//===========================================================================//
// Method         : GetInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
TIO_PrintHandler_c& TIO_PrintHandler_c::GetInstance(
      bool newInstance )
{
   if( !pinstance_ )
   {
      if( newInstance )
      {
         NewInstance( );

         if( pinstance_ )
         {
            pinstance_->SetStdioOutput( stdout );
         }
      }
   }
   return( *pinstance_ );
}

//===========================================================================//
// Method         : HasInstance
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::HasInstance(
      void )
{
   return( pinstance_ ? true : false );
}

//===========================================================================//
// Method         : Info
// Purpose        : Format and print the given info message.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::Info(
      const char* pszText,
      ... )
{
   va_list vaArgs;                      // Make a variable argument list
   va_start( vaArgs, pszText );         // Initialize variable argument list
         
   this->Info( TIO_PRINT_INFO, pszText, vaArgs );

   va_end( vaArgs );                    // Reset variable argument list
}

//===========================================================================//
void TIO_PrintHandler_c::Info(
            TIO_PrintMode_t mode,
      const char*           pszText,
            va_list         vaArgs )
{
   string srText( TIO_PSZ_STR( pszText ));
   if(( srText.length( ) > 1 ) && ( srText[srText.length( )-1] == '\n' ))
   {
      srText = srText.substr( 0, srText.length( )-1 );
   }

   TCT_NameList_c< TC_Name_c >* pinfoAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pinfoAcceptList );
   TCT_NameList_c< TC_Name_c >* pinfoRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pinfoRejectList );

   mode = ( !pinfoAcceptList ? mode : TIO_PRINT_UNDEFINED );
   if( pinfoAcceptList && ( mode == TIO_PRINT_UNDEFINED ))
   {
      if( pinfoAcceptList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_INFO;
      }
   }
   if( pinfoRejectList && ( mode == TIO_PRINT_INFO ))
   {
      if( pinfoRejectList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_UNDEFINED;
      }
   }

   if( mode != TIO_PRINT_UNDEFINED )
   {
      this->WriteMessage_( mode, pszText, vaArgs );
   }
}

//===========================================================================//
// Method         : Warning
// Purpose        : Format and print a (recoverable) warning message. The
//                  max warning count is checked to determine a return value
//                  (ie. do we continue or abort after this warning message).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::Warning(
      const char* pszText,
      ... )
{
   va_list vaArgs;                      // Make a variable argument list
   va_start( vaArgs, pszText );         // Initialize variable argument list
         
   bool isValid = this->Warning( TIO_PRINT_WARNING, pszText, vaArgs );

   va_end( vaArgs );                    // Reset variable argument list

   return( isValid );
}

//===========================================================================//
bool TIO_PrintHandler_c::Warning(
            TIO_PrintMode_t mode,
      const char*           pszText,
            va_list         vaArgs )
{
   bool isValid = true;

   string srText( TIO_PSZ_STR( pszText ));
   if(( srText.length( ) > 1 ) && ( srText[srText.length( )-1] == '\n' ))
   {
      srText = srText.substr( 0, srText.length( )-1 );
   }

   TCT_NameList_c< TC_Name_c >* pwarningAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pwarningAcceptList );
   TCT_NameList_c< TC_Name_c >* pwarningRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pwarningRejectList );
   TCT_NameList_c< TC_Name_c >* perrorAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.perrorAcceptList );
   TCT_NameList_c< TC_Name_c >* perrorRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.perrorRejectList );

   mode = ( !pwarningAcceptList && !perrorAcceptList ? mode : TIO_PRINT_UNDEFINED );
   if( pwarningAcceptList && ( mode == TIO_PRINT_UNDEFINED ))
   {
      if( pwarningAcceptList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_WARNING;
      }
   }
   if( pwarningRejectList && ( mode == TIO_PRINT_WARNING ))
   {
      if( pwarningRejectList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_UNDEFINED;
      }
   }
   if( perrorAcceptList && ( mode == TIO_PRINT_UNDEFINED ))
   {
      if( perrorAcceptList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_ERROR;
      }
   }
   if( perrorRejectList && ( mode == TIO_PRINT_ERROR ))
   {
      if( perrorRejectList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_UNDEFINED;
      }
   }

   if( mode != TIO_PRINT_UNDEFINED )
   {
      this->WriteMessage_( mode, pszText, vaArgs );

      if( mode == TIO_PRINT_ERROR )
      {
         // Update current error count and test against the max error count
         isValid = this->IsValidMaxErrorCount( );
      }
      if( mode == TIO_PRINT_WARNING )
      {
         // Update current warning count and test against the max warning count
         isValid = this->IsValidMaxWarningCount( );
      }
   }
   return( isValid );
}

//===========================================================================//
// Method         : Error
// Purpose        : Format and print a (non-recoverable) error message. The
//                  max error count is checked to determine a return value
//                  (ie. do we continue or abort after this error message).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::Error(
      const char* pszText,
      ... )
{
   va_list vaArgs;                      // Make a variable argument list
   va_start( vaArgs, pszText );         // Initialize variable argument list
         
   bool isValid = this->Error( TIO_PRINT_ERROR, pszText, vaArgs );

   va_end( vaArgs );                    // Reset variable argument list

   return( isValid );
}

//===========================================================================//
bool TIO_PrintHandler_c::Error(
            TIO_PrintMode_t mode,
      const char*           pszText,
            va_list         vaArgs )
{
   bool isValid = true;

   string srText( TIO_PSZ_STR( pszText ));
   if(( srText.length( ) > 1 ) && ( srText[srText.length( )-1] == '\n' ))
   {
      srText = srText.substr( 0, srText.length( )-1 );
   }

   TCT_NameList_c< TC_Name_c >* perrorAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.perrorAcceptList );
   TCT_NameList_c< TC_Name_c >* perrorRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.perrorRejectList );
   TCT_NameList_c< TC_Name_c >* pwarningAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pwarningAcceptList );
   TCT_NameList_c< TC_Name_c >* pwarningRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pwarningRejectList );

   mode = ( !perrorAcceptList && !pwarningAcceptList ? mode : TIO_PRINT_UNDEFINED );
   if( perrorAcceptList && ( mode == TIO_PRINT_UNDEFINED ))
   {
      if( perrorAcceptList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_ERROR;
      }
   }
   if( perrorRejectList && ( mode == TIO_PRINT_ERROR ))
   {
      if( perrorRejectList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_UNDEFINED;
      }
   }
   if( pwarningAcceptList && ( mode == TIO_PRINT_UNDEFINED ))
   {
      if( pwarningAcceptList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_WARNING;
      }
   }
   if( pwarningRejectList && ( mode == TIO_PRINT_WARNING ))
   {
      if( pwarningRejectList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_UNDEFINED;
      }
   }

   if( mode != TIO_PRINT_UNDEFINED )
   {
      this->WriteMessage_( mode, pszText, vaArgs );

      if( mode == TIO_PRINT_ERROR )
      {
         // Update current error count and test against the max error count
         isValid = this->IsValidMaxErrorCount( );
      }
      if( mode == TIO_PRINT_WARNING )
      {
         // Update current warning count and test against the max warning count
         isValid = this->IsValidMaxWarningCount( );
      }
   }
   return( isValid );
}

//===========================================================================//
// Method         : Fatal
// Purpose        : Format and print a (program-specified) fatal message.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
// 08/10/12 jeffr : Changed return "void" to "bool" (will always be false)
//===========================================================================//
bool TIO_PrintHandler_c::Fatal(
      const char* pszText,
      ... )
{
   va_list vaArgs;                      // Make a variable argument list
   va_start( vaArgs, pszText );         // Initialize variable argument list
         
   this->Fatal( TIO_PRINT_FATAL, pszText, vaArgs );

   va_end( vaArgs );                    // Reset variable argument list

   return( false );
}

//===========================================================================//
bool TIO_PrintHandler_c::Fatal(
            TIO_PrintMode_t mode,
      const char*           pszText,
            va_list         vaArgs )
{
   this->WriteMessage_( mode, pszText, vaArgs );

   return( false );
}

//===========================================================================//
// Method         : Trace
// Purpose        : Format and print a (user-specified) trace message.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
// 08/10/12 jeffr : Replaced "(int)" cast with original "static_cast< int >"
//===========================================================================//
void TIO_PrintHandler_c::Trace(
      const char* pszText,
      ... )
{
   va_list vaArgs;                      // Make a variable argument list
   va_start( vaArgs, pszText );         // Initialize variable argument list
         
   this->Trace( TIO_PRINT_TRACE, pszText, vaArgs );

   va_end( vaArgs );                    // Reset variable argument list
}

//===========================================================================//
void TIO_PrintHandler_c::Trace( 
            FILE*  pfile,
            size_t lenSpace,
      const char*  pszText, 
      ... )
{
   FILE* pstream = this->outputs_.stdioOutput.GetStream( );
   if( pfile )
   {
      this->outputs_.stdioOutput.SetStream( pfile );
   }

   static char szText[TIO_FORMAT_STRING_LEN_MAX];
   if( lenSpace > 0 )
   {
      sprintf( szText, "%*s%s", 
               static_cast< int >( lenSpace ), lenSpace ? " " : "", pszText );
      pszText = szText;
   }

   va_list vaArgs;                   // Make a variable argument list
   va_start( vaArgs, pszText );      // Initialize variable argument list
         
   this->Trace( TIO_PRINT_TRACE, pszText, vaArgs );

   va_end( vaArgs );                 // Reset variable argument list

   if( pfile )
   {
      this->outputs_.stdioOutput.SetStream( pstream );
   }
}

//===========================================================================//
void TIO_PrintHandler_c::Trace(
            TIO_PrintMode_t mode,
      const char*           pszText,
            va_list         vaArgs )
{
   string srText( TIO_PSZ_STR( pszText ));
   if(( srText.length( ) > 1 ) && ( srText[srText.length( )-1] == '\n' ))
   {
      srText = srText.substr( 0, srText.length( )-1 );
   }

   TCT_NameList_c< TC_Name_c >* ptraceAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.ptraceAcceptList );
   TCT_NameList_c< TC_Name_c >* ptraceRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.ptraceRejectList );

   mode = ( ptraceRejectList ? mode : TIO_PRINT_UNDEFINED );
   if( ptraceAcceptList && ( mode == TIO_PRINT_UNDEFINED ))
   {
      if( ptraceAcceptList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_TRACE;
      }
   }
   if( ptraceRejectList && ( mode == TIO_PRINT_TRACE ))
   {
      if( ptraceRejectList->MatchRegExp( srText ))
      {
         mode = TIO_PRINT_UNDEFINED;
      }
   }

   if( mode != TIO_PRINT_UNDEFINED )
   {
      this->WriteMessage_( mode, pszText, vaArgs );
   }
}

//===========================================================================//
// Method         : Internal
// Purpose        : Format and print an (programming error) internal 
//                  message.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::Internal( 
      const char*  pszSource, 
      const char*  pszText, 
      ... )
{
   va_list vaArgs;                      // Make a variable argument list
   va_start( vaArgs, pszText );         // Initialize variable argument list
         
   this->Internal( TIO_PRINT_INTERNAL, pszSource, pszText, vaArgs );

   // Update current internal count
   ++this->counts_.internalCount;

   va_end( vaArgs );                    // Reset variable argument list

   return( false );
}

//===========================================================================//
bool TIO_PrintHandler_c::Internal( 
            TIO_PrintMode_t mode,
      const char*           pszSource, 
      const char*           pszText,
            va_list         vaArgs )
{
   this->WriteMessage_( mode, pszText, vaArgs, pszSource );

   return( false );
}

//===========================================================================//
// Method         : Direct
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::Direct(
      const char* pszText,
      ... )
{
   va_list vaArgs;                      // Make a variable argument list
   va_start( vaArgs, pszText );         // Initialize variable argument list
         
   this->Direct( TIO_PRINT_DIRECT, pszText, vaArgs );

   va_end( vaArgs );                    // Reset variable argument list
}

//===========================================================================//
void TIO_PrintHandler_c::Direct(
            TIO_PrintMode_t mode,
      const char*           pszText,
            va_list         vaArgs )
{
   this->WriteMessage_( mode, pszText, vaArgs );
}

//===========================================================================//
// Method         : Write
// Purpose        : Format and print a (user-specified) message.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
// 08/10/12 jeffr : Replaced "(int)" cast with original "static_cast< int >"
//===========================================================================//
void TIO_PrintHandler_c::Write(
      const char* pszText, 
      ... )
{
   va_list vaArgs;                      // Make a variable argument list
   va_start( vaArgs, pszText );         // Initialize variable argument list
         
   this->WriteMessage_( TIO_PRINT_DIRECT, pszText, vaArgs );

   va_end( vaArgs );                    // Reset variable argument list
}

//===========================================================================//
void TIO_PrintHandler_c::Write( 
            size_t lenSpace,
      const char*  pszText, 
      ... )
{
   static char szText[TIO_FORMAT_STRING_LEN_MAX];
   if( lenSpace > 0 )
   {
      sprintf( szText, "%*s%s", 
               static_cast< int >( lenSpace ), lenSpace ? " " : "", pszText );
      pszText = szText;
   }

   va_list vaArgs;                   // Make a variable argument list
   va_start( vaArgs, pszText );      // Initialize variable argument list
         
   this->WriteMessage_( TIO_PRINT_DIRECT, pszText, vaArgs );

   va_end( vaArgs );                 // Reset variable argument list
}

//===========================================================================//
void TIO_PrintHandler_c::Write( 
            FILE*  pfile,
            size_t lenSpace,
      const char*  pszText, 
      ... )
{
   FILE* pstream = this->outputs_.stdioOutput.GetStream( );
   if( pfile )
   {
      this->outputs_.stdioOutput.SetStream( pfile );
   }

   static char szText[TIO_FORMAT_STRING_LEN_MAX];
   if( lenSpace > 0 )
   {
      sprintf( szText, "%*s%s", 
               static_cast< int >( lenSpace ), lenSpace ? " " : "", pszText );
      pszText = szText;
   }

   va_list vaArgs;                   // Make a variable argument list
   va_start( vaArgs, pszText );      // Initialize variable argument list
         
   this->WriteMessage_( TIO_PRINT_DIRECT, pszText, vaArgs );

   va_end( vaArgs );                 // Reset variable argument list

   if( pfile )
   {
      this->outputs_.stdioOutput.SetStream( pstream );
   }
}

//===========================================================================//
// Method         : WriteBanner
// Purpose        : Format and print a (program) banner message.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::WriteBanner( 
      FILE*  pfile,
      size_t lenSpace )
{
   static char szSep[TIO_FORMAT_STRING_LEN_MAX];
   static size_t lenSep = TIO_FORMAT_BANNER_LEN_DEF;

   TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
   const char* pszProgramTitle = skinHandler.GetProgramTitle( );
   const char* pszCopyright = skinHandler.GetCopyright( );

   TC_FormatStringFilled( '_', szSep, lenSep );

   this->WriteCenter( pfile, lenSpace, szSep, lenSep );

   this->WriteCenter( pfile, lenSpace, pszProgramTitle, lenSep );
   this->WriteCenter( pfile, lenSpace, TIO_SZ_BUILD_VERSION, lenSep );
   this->WriteCenter( pfile, lenSpace, pszCopyright, lenSep );

   this->WriteCenter( pfile, lenSpace, szSep, lenSep );
}

//===========================================================================//
void TIO_PrintHandler_c::WriteBanner( 
      void )
{
   FILE* pfile = 0;
   size_t spaceLen = 0;

   this->WriteBanner( pfile, spaceLen );
}

//===========================================================================//
// Method         : WriteCenter
// Purpose        : Format and print a (user-specified) center message.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
// 08/10/12 jeffr : Replaced "(int)" cast with original "static_cast< int >"
//===========================================================================//
void TIO_PrintHandler_c::WriteCenter( 
            FILE*  pfile,
            size_t lenSpace,
      const char*  pszText, 
            size_t lenText,
      const char*  pszPrefix,
      const char*  pszPostfix )
{
   FILE* pstream = this->outputs_.stdioOutput.GetStream( );
   if( pfile )
   {
      this->outputs_.stdioOutput.SetStream( pfile );
   }

   static char szText[TIO_FORMAT_STRING_LEN_MAX];
   if( lenSpace > 0 )
   {
      sprintf( szText, "%*s%s", 
               static_cast< int >( lenSpace ), lenSpace ? " " : "", pszText );
      pszText = szText;
   }

   static char szCenteredText[TIO_FORMAT_STRING_LEN_MAX];
   TC_FormatStringCentered( pszText, lenText,
                            szCenteredText, TIO_FORMAT_STRING_LEN_MAX );
   if( pszPrefix )
   {
      TC_AddStringPrefix( szCenteredText, pszPrefix );
   }
   if( pszPostfix )
   {
      TC_AddStringPostfix( szCenteredText, pszPostfix );
   }
   pszText = szCenteredText;

   va_list vaArgs;                   // Make a variable argument list

   #if defined( SUN8 ) || defined( SUN10 ) || defined( WIN32 ) || defined( _WIN32 )
      vaArgs = 0;
   #endif

   this->WriteMessage_( TIO_PRINT_RETURN, pszText, vaArgs );

   va_end( vaArgs );                 // Reset variable argument list

   if( pfile )
   {
      this->outputs_.stdioOutput.SetStream( pstream );
   }
}

//===========================================================================//
// Method         : WriteVersion
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::WriteVersion( 
      void )
{
   TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
   const char* pszProgramName = skinHandler.GetProgramName( );

   this->Write( "%s\n" \
                "%s, built %s\n", 
                TIO_PSZ_STR( pszProgramName ), 
                TIO_SZ_BUILD_VERSION, 
                TIO_SZ_BUILD_TIME_STAMP );
}

//===========================================================================//
// Method         : WriteHelp
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::WriteHelp( 
      void )
{
   TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
   const char* pszBinaryName = skinHandler.GetBinaryName( );

   this->Write( TIO_SZ_PROGRAM_HELP, pszBinaryName );
}

//===========================================================================//
// Method         : SetStdioOutput
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::SetStdioOutput( 
      FILE* pstream )
{
   this->outputs_.stdioOutput.SetStream( pstream );

   bool isEnabled = this->outputs_.stdioOutput.IsEnabled( );
   this->outputs_.isStdioEnabled = isEnabled;
}

//===========================================================================//
// Method         : SetCustomOutput
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::SetCustomOutput( 
      TIO_PFX_CustomHandler_t pfxCustomHandler )
{
   this->outputs_.customOutput.SetHandler( pfxCustomHandler );

   bool isEnabled = this->outputs_.customOutput.IsEnabled( );
   this->outputs_.isCustomEnabled = isEnabled;
}

//===========================================================================//
// Method         : SetLogFileOutput
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::SetLogFileOutput( 
      const char*              pszLogFileName,
            TIO_FileOpenMode_t fileOpenMode )
{
   if( pszLogFileName )
   {
      this->outputs_.logFileOutput.Open( pszLogFileName, fileOpenMode );
   }
   else
   {
      this->outputs_.logFileOutput.Close( );
   }

   bool isEnabled = this->outputs_.logFileOutput.IsEnabled( );
   this->outputs_.isLogFileEnabled = isEnabled;

   return( this->outputs_.logFileOutput.IsValid( ));
}

//===========================================================================//
// Method         : SetUserFileOutput
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::SetUserFileOutput( 
      const char*              pszUserFileName,
            TIO_FileOpenMode_t fileOpenMode )
{
   if( pszUserFileName )
   {
      this->outputs_.userFileOutput.Open( pszUserFileName, fileOpenMode );
   }
   else
   {
      this->outputs_.userFileOutput.Close( );
   }

   bool isEnabled = this->outputs_.userFileOutput.IsEnabled( );
   this->outputs_.isUserFileEnabled = isEnabled;

   return( this->outputs_.userFileOutput.IsValid( ));
}

//===========================================================================//
// Method         : AddInfoAcceptRegExp
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::AddInfoAcceptRegExp( 
      const char* pszRegExp )
{
   if( !this->display_.pinfoAcceptList )
   {
      this->display_.pinfoAcceptList = new TC_NOTHROW TCT_NameList_c< TC_Name_c >( TIO_DISPLAY_INFO_LIST_DEF_CAPACITY );
   }
   TCT_NameList_c< TC_Name_c >* pinfoAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pinfoAcceptList );
   pinfoAcceptList->Add( pszRegExp );
}

//===========================================================================//
// Method         : AddInfoRejectRegExp
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::AddInfoRejectRegExp( 
      const char* pszRegExp )
{
   if( !this->display_.pinfoRejectList )
   {
      this->display_.pinfoRejectList = new TC_NOTHROW TCT_NameList_c< TC_Name_c >( TIO_DISPLAY_INFO_LIST_DEF_CAPACITY );
   }
   TCT_NameList_c< TC_Name_c >* pinfoRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pinfoRejectList );
   pinfoRejectList->Add( pszRegExp );
}

//===========================================================================//
// Method         : AddWarningAcceptRegExp
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::AddWarningAcceptRegExp( 
      const char* pszRegExp )
{
   if( !this->display_.pwarningAcceptList )
   {
      this->display_.pwarningAcceptList = new TC_NOTHROW TCT_NameList_c< TC_Name_c >( TIO_DISPLAY_WARNING_LIST_DEF_CAPACITY );
   }
   TCT_NameList_c< TC_Name_c >* pwarningAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pwarningAcceptList );
   pwarningAcceptList->Add( pszRegExp );
}

//===========================================================================//
// Method         : AddWarningRejectRegExp
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::AddWarningRejectRegExp( 
      const char* pszRegExp )
{
   if( !this->display_.pwarningRejectList )
   {
      this->display_.pwarningRejectList = new TC_NOTHROW TCT_NameList_c< TC_Name_c >( TIO_DISPLAY_WARNING_LIST_DEF_CAPACITY );
   }
   TCT_NameList_c< TC_Name_c >* pwarningRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.pwarningRejectList );
   pwarningRejectList->Add( pszRegExp );
}

//===========================================================================//
// Method         : AddErrorAcceptRegExp
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::AddErrorAcceptRegExp( 
      const char* pszRegExp )
{
   if( !this->display_.perrorAcceptList )
   {
      this->display_.perrorAcceptList = new TC_NOTHROW TCT_NameList_c< TC_Name_c >( TIO_DISPLAY_ERROR_LIST_DEF_CAPACITY );
   }
   TCT_NameList_c< TC_Name_c >* perrorAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.perrorAcceptList );
   perrorAcceptList->Add( pszRegExp );
}

//===========================================================================//
// Method         : AddErrorRejectRegExp
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::AddErrorRejectRegExp( 
      const char* pszRegExp )
{
   if( !this->display_.perrorRejectList )
   {
      this->display_.perrorRejectList = new TC_NOTHROW TCT_NameList_c< TC_Name_c >( TIO_DISPLAY_ERROR_LIST_DEF_CAPACITY );
   }
   TCT_NameList_c< TC_Name_c >* perrorRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.perrorRejectList );
   perrorRejectList->Add( pszRegExp );
}

//===========================================================================//
// Method         : AddTraceAcceptRegExp
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::AddTraceAcceptRegExp( 
      const char* pszRegExp )
{
   if( !this->display_.ptraceAcceptList )
   {
      this->display_.ptraceAcceptList = new TC_NOTHROW TCT_NameList_c< TC_Name_c >( TIO_DISPLAY_TRACE_LIST_DEF_CAPACITY );
   }
   TCT_NameList_c< TC_Name_c >* ptraceAcceptList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.ptraceAcceptList );
   ptraceAcceptList->Add( pszRegExp );
}

//===========================================================================//
// Method         : AddTraceRejectRegExp
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::AddTraceRejectRegExp( 
      const char* pszRegExp )
{
   if( !this->display_.ptraceRejectList )
   {
      this->display_.ptraceRejectList = new TC_NOTHROW TCT_NameList_c< TC_Name_c >( TIO_DISPLAY_TRACE_LIST_DEF_CAPACITY );
   }
   TCT_NameList_c< TC_Name_c >* ptraceRejectList = static_cast< TCT_NameList_c< TC_Name_c >* >( this->display_.ptraceRejectList );
   ptraceRejectList->Add( pszRegExp );
}

//===========================================================================//
// Method         : FindUserName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::FindUserName(
      string* psrUserName ) const
{
   const char* pszUserNameCmd = "";
   #if defined( SUN8 ) || defined( SUN10 ) || defined( LINUX_I686 ) || defined( LINUX_X86_64 )
      pszUserNameCmd = "whoami"; 
   #elif defined( WIN32 ) || defined( _WIN32 )
      pszUserNameCmd = "echo %USERNAME%";
   #endif
   this->ApplySystemCommand_( pszUserNameCmd,
                              TIO_COMMAND_USERNAME_LEN, 
                              psrUserName );
}

//===========================================================================//
// Method         : FindHostName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::FindHostName(
      string* psrHostName ) const
{
   const char* pszHostNameCmd = "";
   #if defined( SUN8 ) || defined( SUN10 ) || defined( LINUX_I686 ) || defined( LINUX_X86_64 )
      pszHostNameCmd = "hostname", 
   #elif defined( WIN32 ) || defined( _WIN32 )
      pszHostNameCmd = "hostname", 
   #endif
   this->ApplySystemCommand_( pszHostNameCmd,
                              TIO_COMMAND_HOSTNAME_LEN, 
                              psrHostName );
}

//===========================================================================//
// Method         : FindWorkingDir
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::FindWorkingDir(
      string* psrWorkingDir ) const
{
   this->ApplySystemCommand_( "pwd", 
                              TIO_COMMAND_WORKINGDIR_LEN, 
                              psrWorkingDir );
}

//===========================================================================//
// Method         : EnableOutput
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::EnableOutput( 
      int outputMask )
{
   if( outputMask & TIO_PRINT_OUTPUT_STDIO )
   {
      this->EnableStdioOutput( );
   }
   if( outputMask & TIO_PRINT_OUTPUT_CUSTOM )
   {
      this->EnableCustomOutput( );
   }
   if( outputMask & TIO_PRINT_OUTPUT_LOG_FILE )
   {
      this->EnableLogFileOutput( );
   }
   if( outputMask & TIO_PRINT_OUTPUT_USER_FILE )
   {
      this->EnableUserFileOutput( );
   }
}

//===========================================================================//
// Method         : DisableOutput
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::DisableOutput( 
      int outputMask )
{
   if( outputMask & TIO_PRINT_OUTPUT_STDIO )
   {
      this->DisableStdioOutput( );
   }
   if( outputMask & TIO_PRINT_OUTPUT_CUSTOM )
   {
      this->DisableCustomOutput( );
   }
   if( outputMask & TIO_PRINT_OUTPUT_LOG_FILE )
   {
      this->DisableLogFileOutput( );
   }
   if( outputMask & TIO_PRINT_OUTPUT_USER_FILE )
   {
      this->DisableUserFileOutput( );
   }
}

//===========================================================================//
// Method         : IsOutputEnabled
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::IsOutputEnabled( 
      int outputMask )
{
   bool isOutputEnabled = false;

   if( outputMask & TIO_PRINT_OUTPUT_STDIO )
   {
      isOutputEnabled = ( isOutputEnabled ? 
                          true : this->IsStdioOutputEnabled( ));
   }
   if( outputMask & TIO_PRINT_OUTPUT_CUSTOM )
   {
      isOutputEnabled = ( isOutputEnabled ? 
                          true : this->IsCustomOutputEnabled( ));
   }
   if( outputMask & TIO_PRINT_OUTPUT_LOG_FILE )
   {
      isOutputEnabled = ( isOutputEnabled ? 
                          true : this->IsLogFileOutputEnabled( ));
   }
   if( outputMask & TIO_PRINT_OUTPUT_USER_FILE )
   {
      isOutputEnabled = ( isOutputEnabled ? 
                          true : this->IsUserFileOutputEnabled( ));
   }
   return( isOutputEnabled );
}

//===========================================================================//
// Method         : IsWithinMaxWarningCount
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::IsWithinMaxWarningCount(
      void ) const
{
   bool isWithin = true;

   if( this->counts_.isMaxWarningEnabled &&
       this->counts_.maxWarningCount <= this->counts_.warningCount )
   {
      isWithin = false;
   }
   return( isWithin );
}

//===========================================================================//
// Method         : IsWithinMaxErrorCount
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::IsWithinMaxErrorCount(
      void ) const
{
   bool isWithin = true;

   if( this->counts_.isMaxErrorEnabled &&
       this->counts_.maxErrorCount <= this->counts_.errorCount )
   {
      isWithin = false;
   }
   return( isWithin );
}

//===========================================================================//
// Method         : IsWithinMaxCount
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::IsWithinMaxCount(
      void ) const
{
   bool isWithin = true;

   if( !this->IsWithinMaxErrorCount( ) ||
       !this->IsWithinMaxWarningCount( ))
   {
      isWithin = false;
   }
   return( isWithin );
}

//===========================================================================//
// Method         : IsValidMaxWarningCount
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::IsValidMaxWarningCount(
      void )
{
   bool isValid = true;

   ++this->counts_.warningCount;
   if( !this->IsWithinMaxWarningCount( ))
   {
      this->counts_.isMaxErrorEnabled = false;
      this->Fatal( "Exceeded maximum warning count (%u).\n", 
                   this->counts_.maxWarningCount );
      this->counts_.isMaxErrorEnabled = true;

      isValid = false;
   }
   return( isValid );
}

//===========================================================================//
// Method         : IsValidMaxErrorCount
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::IsValidMaxErrorCount(
      void )
{
   bool isValid = true;

   ++this->counts_.errorCount;
   if( !this->IsWithinMaxErrorCount( ))
   {
      this->counts_.isMaxErrorEnabled = false;
      this->Fatal( "Exceeded maximum error count (%u).\n", 
                   this->counts_.maxErrorCount );
      this->counts_.isMaxErrorEnabled = true;

      isValid = false;
   }
   return( isValid );
}

//===========================================================================//
// Method         : IsValidNew
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::IsValidNew( 
      const void*  pvoid, 
            size_t allocLen,
      const char*  pszSource )
{
   if( !pvoid && pszSource )
   {
      this->Internal( pszSource, 
                      "Failed to allocate %u bytes memory!\n",
                      allocLen );
   }
   return( pvoid ? true : false );
}

//===========================================================================//
// Method         : WriteMessage_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::WriteMessage_( 
            TIO_PrintMode_t mode,       // Defines print severity level mode
      const char*           pszText,    // Ptr to message text string 
            va_list         vaArgs,     // Define optional variable arguments
      const char*           pszSource ) // Ptr to optional source string
{
   static char szMessage[TIO_FORMAT_STRING_LEN_MAX];

   // Extract and format message based on optional variable argument list
   bool isValid = this->FormatMessage_( pszText, vaArgs, szMessage );

   // Write message to zero or more enabled output streams
   if( isValid )
   {
      this->OutputMessage_( mode, szMessage, pszSource );
   }
   return( isValid );
}

//===========================================================================//
// Method         : FormatMessage_
// Purpose        : Format and return a message string based on the given 
//                  message text string and optional variable argument list.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::FormatMessage_( 
      const char*   pszText,     // Ptr to message text string 
            va_list vaArgs,      // Define optional variable arguments
            char*   pszMessage ) // Ptr to returned message string
{
   if( pszMessage )
   {
      // Extract and format based on variable argument list
      vsprintf( pszMessage, pszText, vaArgs );
   }
   return( pszMessage && *pszMessage ? true : false );
}

//===========================================================================//
// Method         : PrefixMessage_
// Purpose        : Prefix and return a message string based on the given 
//                  message text string and print severity level mode.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TIO_PrintHandler_c::PrefixMessage_(
            TIO_PrintMode_t mode,        // Defines print severity level mode
      const char*           pszText,     // Ptr to message text string 
      const char*           pszSource,   // Ptr to optional source string
            char*           pszMessage ) // Ptr to returned message string
{
   if( pszMessage )
   {
      const char* pszPrefix = TIO_SR_STR( this->formats_.srPrefix );

      pszText = TIO_PSZ_STR( pszText );
      pszSource = TIO_PSZ_STR( pszSource );

      unsigned long warningCount = this->counts_.warningCount + 1;
      unsigned long errorCount = this->counts_.errorCount + 1;
      unsigned long internalCount = this->counts_.internalCount + 1;

      switch( mode )
      {
      case TIO_PRINT_INFO:

         if( this->formats_.timeStampsEnabled )
         {
            static char szTimeStamp[TIO_FORMAT_STRING_LEN_DATE_TIME];
            size_t lenTimeStamp = sizeof( szTimeStamp );
            TC_FormatStringDateTimeStamp( szTimeStamp, lenTimeStamp, "[", "] " );

            sprintf( pszMessage, "%s%s%s", szTimeStamp, pszPrefix, pszText );
         }
         else
         {
            sprintf( pszMessage, "%s%s", pszPrefix, pszText );
         }
         break;

      case TIO_PRINT_WARNING:

         sprintf( pszMessage, "%sWARNING(%lu): %s", pszPrefix, warningCount, pszText );
         break;

      case TIO_PRINT_ERROR:

         sprintf( pszMessage, "%sERROR(%lu): %s", pszPrefix, errorCount, pszText );
         break;

      case TIO_PRINT_FATAL:

         sprintf( pszMessage, "%sFATAL: %s", pszPrefix, pszText );
         break;

      case TIO_PRINT_TRACE:

         if( this->formats_.timeStampsEnabled )
         {
            static char szTimeStamp[TIO_FORMAT_STRING_LEN_DATE_TIME];
            size_t lenTimeStamp = sizeof( szTimeStamp );
            TC_FormatStringDateTimeStamp( szTimeStamp, lenTimeStamp, "[", "] " );

            sprintf( pszMessage, "%sTRACE: %s%s", pszPrefix, szTimeStamp, pszText );
         }
         else
         {
            sprintf( pszMessage, "%sTRACE: %s", pszPrefix, pszText );
         }
         break;

      case TIO_PRINT_INTERNAL:

	 sprintf( pszMessage, "INTERNAL(%lu) @ %s\n"
                              "            %s", internalCount, pszSource, pszText );
         break;

      case TIO_PRINT_DIRECT:

         sprintf( pszMessage, "%s", pszText );
         break;

      case TIO_PRINT_RETURN:

         sprintf( pszMessage, "%s\n", pszText );
         break;

      default:

         sprintf( pszMessage, "%s", pszText );
         break;
      }
   }
   return( pszMessage && *pszMessage ? true : false );
}

//===========================================================================//
// Method         : OutputMessage_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::OutputMessage_( 
            TIO_PrintMode_t mode,       // Defines print severity level mode
      const char*           pszText,    // Ptr to message text string 
      const char*           pszSource ) // Ptr to optional source string
{
   static char szMessage[TIO_FORMAT_STRING_LEN_MAX];
   
   // Format and print message based on enabled output options
   if( this->outputs_.stdioOutput.IsEnabled( ))
   {
      this->PrefixMessage_( mode, pszText, pszSource, szMessage );
      this->outputs_.stdioOutput.Write( szMessage );
      this->outputs_.stdioOutput.Flush( );
   }

   if( this->outputs_.customOutput.IsEnabled( ))
   {
      this->outputs_.customOutput.Write( mode, pszText, pszSource );
   }

   if( this->outputs_.logFileOutput.IsEnabled( ))
   {
      this->PrefixMessage_( mode, pszText, pszSource, szMessage );
      this->outputs_.logFileOutput.Write( szMessage );
      this->outputs_.logFileOutput.Flush( );
   }

   if( this->outputs_.userFileOutput.IsEnabled( ))
   {
      this->PrefixMessage_( mode, pszText, pszSource, szMessage );
      this->outputs_.userFileOutput.Write( szMessage );
   }
} 

//===========================================================================//
// Method         : ApplySystemCommand_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TIO_PrintHandler_c::ApplySystemCommand_(
      const char*   pszCommand,
            size_t  lenCommandStdout,
            string* psrCommandStdout ) const
{
   if( psrCommandStdout )
   {
      *psrCommandStdout = "";

      #if defined( SUN8 ) || defined( SUN10 ) || defined( LINUX_I686 ) || defined( LINUX_X86_64 ) || defined( WIN32 ) || defined( _WIN32 )

         TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
         const char* pszBinaryName = skinHandler.GetBinaryName( );

         string srCommandStdout;
         srCommandStdout += "./.";
         srCommandStdout += pszBinaryName;
         srCommandStdout += ".";
         srCommandStdout += pszCommand;

         string srCommand;
         srCommand += pszCommand;
         srCommand += " > ";
         srCommand += srCommandStdout;

         if( system( srCommand.data( )) >= 0 )
         {
            TIO_FileHandler_c fileIO( srCommandStdout, TIO_FILE_OPEN_READ );
            if( fileIO.IsValid( ))
            {
               char* pszCommandStdout = new TC_NOTHROW char[lenCommandStdout];
               fileIO.Read( pszCommandStdout, lenCommandStdout-1 );
   
               pszCommandStdout[strlen( pszCommandStdout )-1] = 0;
               *psrCommandStdout = pszCommandStdout;
   
               fileIO.Close( );
               delete[] pszCommandStdout;
            }
         }
      #endif
   }
}
