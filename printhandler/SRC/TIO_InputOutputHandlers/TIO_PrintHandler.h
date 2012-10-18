//===========================================================================//
// Purpose : Declaration and inline definitions for a TIO_PrintHandler 
//           singleton class. This class handles all user interface messages.
//
//           Inline methods include:
//           - SetTimeStampsEnabled
//           - SetPrefix
//           - ClearPrefix
//           - SetMaxErrorCount
//           - SetMaxWarningCount
//           - GetErrorCount
//           - GetWarningCount
//           - GetInternalCount
//           - GetLogFileName
//           - GetUserFileName
//           - EnableStdioOutput, DisableStdioOutput
//           - EnableCustomOutput, DisableCustomOutput
//           - EnableLogFileOutput, DisableLogFileOutput
//           - EnableUserFileOutput, DisableUserFileOutput
//           - IsStdioOutputEnabled
//           - IsCustomOutputEnabled
//           - IsLogFileOutputEnabled
//           - IsUserFileOutputEnabled
//
//           Protected methods include:
//           - TIO_PrintHandler_c, ~TIO_PrintHandler_c
//
//           Private methods include:
//           - WriteMessage_
//           - FormatMessage_
//           - PrefixMessage_
//           - OutputMessage_
//
//===========================================================================//

#ifndef TIO_PRINT_HANDLER_H
#define TIO_PRINT_HANDLER_H

#include <string>
using namespace std;

#include "TIO_Typedefs.h"
#include "TIO_StdioOutput.h"
#include "TIO_CustomOutput.h"
#include "TIO_FileOutput.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TIO_PrintHandler_c
{
public:

   static void NewInstance( void );
   static void DeleteInstance( void );
   static TIO_PrintHandler_c& GetInstance( bool newInstance = true );
   static bool HasInstance( void );

   void Info( const char* pszText, ... );
   void Info( TIO_PrintMode_t mode, 
              const char* pszText, va_list vaArgs );

   bool Warning( const char* pszText, ... );
   bool Warning( TIO_PrintMode_t mode, 
                 const char* pszText, va_list vaArgs );

   bool Error( const char* pszText, ... );
   bool Error( TIO_PrintMode_t mode, 
               const char* pszText, va_list vaArgs );

   bool Fatal( const char* pszText, ... );
   bool Fatal( TIO_PrintMode_t mode, 
               const char* pszText, va_list vaArgs );

   void Trace( const char* pszText, ... );
   void Trace( FILE* pfile, 
               size_t lenSpace, 
               const char* pszText, ... );
   void Trace( TIO_PrintMode_t mode, 
               const char* pszText, va_list vaArgs );

   bool Internal( const char* pszSource, 
                  const char* pszText, ... );
   bool Internal( TIO_PrintMode_t mode, 
                  const char* pszSource, 
                  const char* pszText, va_list vaArgs );

   void Direct( const char* pszText, ... );
   void Direct( TIO_PrintMode_t mode, 
                const char* pszText, va_list vaArgs );

   void Write( const char* pszText, ... );
   void Write( size_t lenSpace, 
               const char* pszText, ... );
   void Write( FILE* pfile, 
               size_t lenSpace, 
               const char* pszText, ... );

   void WriteBanner( FILE* pfile, 
                     size_t lenSpace );
   void WriteBanner( void );
   void WriteCenter( FILE* pfile, 
                     size_t lenSpace, 
                     const char* pszText, 
                     size_t lenText, 
                     const char* pszPrefix = 0, 
                     const char* pszPostfix = 0 );

   void WriteVersion( void );
   void WriteHelp( void );

   void SetStdioOutput( FILE* pstream );
   void SetCustomOutput( TIO_PFX_CustomHandler_t pfxCustomHandler );
   bool SetLogFileOutput( const char* pszLogFileName = 0,
                          TIO_FileOpenMode_t fileOpenMode = TIO_FILE_OPEN_WRITE );
   bool SetUserFileOutput( const char* pszUserFileName = 0,
                           TIO_FileOpenMode_t fileOpenMode = TIO_FILE_OPEN_WRITE );

   void SetTimeStampsEnabled( bool timeStampsEnable );

   void SetPrefix( const char* pszPrefix );
   void SetPrefix( const string& srPrefix );
   void ClearPrefix( void );

   void AddInfoAcceptRegExp( const char* pszRegExp ); 
   void AddInfoRejectRegExp( const char* pszRegExp ); 
   void AddWarningAcceptRegExp( const char* pszRegExp );
   void AddWarningRejectRegExp( const char* pszRegExp );
   void AddErrorAcceptRegExp( const char* pszRegExp );
   void AddErrorRejectRegExp( const char* pszRegExp );
   void AddTraceAcceptRegExp( const char* pszRegExp );
   void AddTraceRejectRegExp( const char* pszRegExp );

   void SetMaxWarningCount( unsigned long maxWarningCount );
   void SetMaxErrorCount( unsigned long maxErrorCount );

   unsigned long GetWarningCount( void ) const;
   unsigned long GetErrorCount( void ) const;
   unsigned long GetInternalCount( void ) const;

   const char* GetLogFileName( void ) const;
   const char* GetUserFileName( void ) const;

   void FindUserName( string* psrUserName ) const;
   void FindHostName( string* psrHostName ) const;
   void FindWorkingDir( string* psrWorkingDir ) const;

   void EnableStdioOutput( void );
   void EnableCustomOutput( void );
   void EnableLogFileOutput( void );
   void EnableUserFileOutput( void );
   void EnableOutput( int outputMask = TIO_PRINT_OUTPUT_ALL );

   void DisableStdioOutput( void );
   void DisableCustomOutput( void );
   void DisableLogFileOutput( void );
   void DisableUserFileOutput( void );
   void DisableOutput( int outputMask = TIO_PRINT_OUTPUT_ALL );

   bool IsStdioOutputEnabled( void ) const;
   bool IsCustomOutputEnabled( void ) const;
   bool IsLogFileOutputEnabled( void ) const;
   bool IsUserFileOutputEnabled( void ) const;
   bool IsOutputEnabled( int outputMask = TIO_PRINT_OUTPUT_ALL );

   bool IsWithinMaxWarningCount( void ) const;
   bool IsWithinMaxErrorCount( void ) const;
   bool IsWithinMaxCount( void ) const;

   bool IsValidMaxWarningCount( void );
   bool IsValidMaxErrorCount( void );

   bool IsValidNew( const void* pvoid, 
                    size_t allocLen,
                    const char* pszSource );

protected:

   TIO_PrintHandler_c( void );
   ~TIO_PrintHandler_c( void );

private:

   bool WriteMessage_( TIO_PrintMode_t mode, 
                       const char* pszText,
                       va_list vaArgs,
                       const char* pszSource = 0 );
   bool FormatMessage_( const char* pszText,
                        va_list vaArgs,
                        char* pszMessage );
   bool PrefixMessage_( TIO_PrintMode_t mode, 
                        const char* pszText,
                        const char* pszSource,
                        char* pszMessage );
   void OutputMessage_( TIO_PrintMode_t mode,
                        const char* pszText,
                        const char* pszSource = 0 );

   void ApplySystemCommand_( const char* pszCommand,
                             size_t lenCommandStdout,
                             string* psrCommandStdout ) const;

private:

   class TIO_PrintHandlerOutputs_c
   {
   public:

      TIO_StdioOutput_c  stdioOutput;     // Used for optional standard output IF
      TIO_CustomOutput_c customOutput;    // Used for optional custom output IF
      TIO_FileOutput_c   logFileOutput;   // Used for optional log file output IF
      TIO_FileOutput_c   userFileOutput;  // Used for optional user file output IF

      bool isStdioEnabled;                // Enable optional standard output IF
      bool isCustomEnabled;               // Enable optional custom output IF
      bool isLogFileEnabled;              // Enable optional log file output IF
      bool isUserFileEnabled;             // Enable optional user file output IF

   } outputs_;

   class TIO_PrintHandlerDisplay_c
   {
   public:

      void* pinfoAcceptList;              // Define INFO accept message filters
      void* pinfoRejectList;              // Define INFO reject message filters
      void* pwarningAcceptList;           // Define WARNING accept message filters
      void* pwarningRejectList;           // Define WARNING reject message filters
      void* perrorAcceptList;             // Define ERROR accept message filters
      void* perrorRejectList;             // Define ERROR reject message filters
      void* ptraceAcceptList;             // Define TRACE accept message filters
      void* ptraceRejectList;             // Define TRACE reject message filters

   } display_;

   class TIO_PrintHandlerCounts_c
   {
   public:

      unsigned long warningCount;         // Track total warning message count
      unsigned long errorCount;           // Track total error message count
      unsigned long internalCount;        // Track total internal message count

      unsigned long maxWarningCount;      // Define max warning message count
      unsigned long maxErrorCount;        // Define max error message count

      bool isMaxWarningEnabled;           // Enable max warning message check
      bool isMaxErrorEnabled;             // Enable max error message check

   } counts_;

   class TIO_PrintHandlerFormat_c
   {
   public:

      bool   timeStampsEnabled;           // Enable print message time stamps
      string srPrefix;                    // Define print message prefix

   } formats_;

   static TIO_PrintHandler_c* pinstance_; // Define ptr for singleton instance

private:

   enum TIO_DefCapacity_e 
   { 
      TIO_DISPLAY_INFO_LIST_DEF_CAPACITY = 1,
      TIO_DISPLAY_WARNING_LIST_DEF_CAPACITY = 1,
      TIO_DISPLAY_ERROR_LIST_DEF_CAPACITY = 1,
      TIO_DISPLAY_TRACE_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline void TIO_PrintHandler_c::SetTimeStampsEnabled(
      bool timeStampsEnabled )
{
   this->formats_.timeStampsEnabled = timeStampsEnabled;
}

//===========================================================================//
inline void TIO_PrintHandler_c::SetPrefix(
      const char* pszPrefix )
{
   this->formats_.srPrefix = TIO_PSZ_STR( pszPrefix );
}

//===========================================================================//
inline void TIO_PrintHandler_c::SetPrefix(
      const string& srPrefix )
{
   this->formats_.srPrefix = srPrefix;
}

//===========================================================================//
inline void TIO_PrintHandler_c::ClearPrefix(
      void )
{
   this->formats_.srPrefix = "";
}

//===========================================================================//
inline void TIO_PrintHandler_c::SetMaxWarningCount( 
      unsigned long maxWarningCount )
{
   this->counts_.maxWarningCount = maxWarningCount;
}

//===========================================================================//
inline void TIO_PrintHandler_c::SetMaxErrorCount( 
      unsigned long maxErrorCount )
{
   this->counts_.maxErrorCount = maxErrorCount;
}

//===========================================================================//
inline unsigned long TIO_PrintHandler_c::GetWarningCount(
      void ) const
{
   return( this->counts_.warningCount );
}

//===========================================================================//
inline unsigned long TIO_PrintHandler_c::GetErrorCount( 
      void ) const
{
   return( this->counts_.errorCount );
}

//===========================================================================//
inline unsigned long TIO_PrintHandler_c::GetInternalCount( 
      void ) const
{
   return( this->counts_.internalCount );
}

//===========================================================================//
inline const char* TIO_PrintHandler_c::GetLogFileName(
      void ) const
{
   return( this->outputs_.logFileOutput.GetName( ));
}

//===========================================================================//
inline const char* TIO_PrintHandler_c::GetUserFileName(
      void ) const
{
   return( this->outputs_.userFileOutput.GetName( ));
}

//===========================================================================//
inline void TIO_PrintHandler_c::EnableStdioOutput( 
      void )
{
   this->outputs_.stdioOutput.SetEnabled( this->outputs_.isStdioEnabled );
}

//===========================================================================//
inline void TIO_PrintHandler_c::EnableCustomOutput(
      void )
{
   this->outputs_.customOutput.SetEnabled( this->outputs_.isCustomEnabled );
}

//===========================================================================//
inline void TIO_PrintHandler_c::EnableLogFileOutput(
      void )
{
   this->outputs_.logFileOutput.SetEnabled( this->outputs_.isLogFileEnabled );
}

//===========================================================================//
inline void TIO_PrintHandler_c::EnableUserFileOutput(
      void )
{
   this->outputs_.userFileOutput.SetEnabled( this->outputs_.isUserFileEnabled );
}

//===========================================================================//
inline void TIO_PrintHandler_c::DisableStdioOutput(
      void )
{
   this->outputs_.stdioOutput.SetEnabled( false );
}

//===========================================================================//
inline void TIO_PrintHandler_c::DisableCustomOutput(
      void )
{
   this->outputs_.customOutput.SetEnabled( false );
}

//===========================================================================//
inline void TIO_PrintHandler_c::DisableLogFileOutput( 
      void )
{
   this->outputs_.logFileOutput.SetEnabled( false );
}

//===========================================================================//
inline void TIO_PrintHandler_c::DisableUserFileOutput( 
      void )
{
   this->outputs_.userFileOutput.SetEnabled( false );
}

//===========================================================================//
inline bool TIO_PrintHandler_c::IsStdioOutputEnabled( 
      void ) const
{
   return( this->outputs_.isStdioEnabled );
}

//===========================================================================//
inline bool TIO_PrintHandler_c::IsCustomOutputEnabled( 
      void ) const
{
   return( this->outputs_.isCustomEnabled );
}

//===========================================================================//
inline bool TIO_PrintHandler_c::IsLogFileOutputEnabled( 
      void ) const
{
   return( this->outputs_.isLogFileEnabled );
}

//===========================================================================//
inline bool TIO_PrintHandler_c::IsUserFileOutputEnabled( 
      void ) const
{
   return( this->outputs_.isUserFileEnabled );
}

#endif
