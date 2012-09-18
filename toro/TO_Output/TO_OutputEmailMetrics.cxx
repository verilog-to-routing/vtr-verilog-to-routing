//===========================================================================//
// Purpose : Supporting methods for the TO_Output_c post-processor class.
//           These methods support formatting and sending a metrics email.
//
//           Private methods include:
//           - SendMetricsEmail_
//           - BuildMetricsEmailSubject_
//           - BuildMetricsEmailBody_
//           - MailMetricsEmailMessage_
//
//===========================================================================//

#include "TC_StringUtils.h"

#include "TIO_StringText.h"
#include "TIO_SkinHandler.h"
#include "TIO_PrintHandler.h"

#include "TO_Output.h"

//===========================================================================//
// Method         : SendMetricsEmail_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TO_Output_c::SendMetricsEmail_( 
      const TOS_InputOptions_c&  inputOptions,
      const TOS_OutputOptions_c& outputOptions ) const
{
   bool ok = true;

   const string& srEmailAddress = outputOptions.srMetricsEmailAddress;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   printHandler.Info( "Sending %s email to '%s'...\n",
                      TIO_SZ_OUTPUT_METRICS_DEF_TYPE,
		      TIO_SR_STR( srEmailAddress ));

   const char* pszOptionsFileName = inputOptions.optionsFileNameList[0]->GetName( );

   string srMessageSubject;
   this->BuildMetricsEmailSubject_( pszOptionsFileName, 
                                    &srMessageSubject );

   const TCL_CommandLine_c& commandLine = *this->pcommandLine_;
   string srCommandLine;
   commandLine.ExtractArgString( &srCommandLine );

   string srMessageBody;
   this->BuildMetricsEmailBody_( srCommandLine, 
                                 &srMessageBody );

   ok = this->MailMetricsEmailMessage_( srEmailAddress, 
                                        srMessageSubject, 
                                        srMessageBody );
   return( ok );
}

//===========================================================================//
// Method         : BuildMetricsEmailSubject_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TO_Output_c::BuildMetricsEmailSubject_(
      const char*   pszFileName,
            string* psrSubject ) const
{
   if( psrSubject )
   {
      TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
      const char* pszBinaryName = skinHandler.GetBinaryName( );
   
      *psrSubject = "";
      *psrSubject += pszBinaryName;
      *psrSubject += " ";
      *psrSubject += pszFileName;
   }
}

//===========================================================================//
// Method         : BuildMetricsEmailBody_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TO_Output_c::BuildMetricsEmailBody_(
      const string& srCommandLine,
            string* psrBody ) const
{
   if( psrBody )
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

      char szTimeStamp[TIO_FORMAT_STRING_LEN_DATE_TIME];
      size_t lenTimeStamp = sizeof( szTimeStamp );
      TC_FormatStringDateTimeStamp( szTimeStamp, lenTimeStamp, "[", "]" );

      TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );
      const char* pszBinaryName = skinHandler.GetBinaryName( );
   
      string srWorkingDir;
      printHandler.FindWorkingDir( &srWorkingDir );

      string srUserName;
      printHandler.FindUserName( &srUserName );

      string srHostName;
      printHandler.FindHostName( &srHostName );

      const char* pszBuildVersion = TIO_SZ_BUILD_VERSION;

      *psrBody = "";
      *psrBody += szTimeStamp;
      *psrBody += " ";
      *psrBody += pszBinaryName;
      *psrBody += " ";
      *psrBody += srCommandLine;
      *psrBody += ", ";
      *psrBody += srUserName;
      *psrBody += ", ";
      *psrBody += srHostName;
      *psrBody += ", ";
      *psrBody += srWorkingDir;
      *psrBody += ", ";
      *psrBody += pszBuildVersion;

      *psrBody += "\n";
   }
}

//===========================================================================//
// Method         : MailMetricsEmailMessage_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TO_Output_c::MailMetricsEmailMessage_(
      const string& srAddress,
      const string& srSubject,
      const string& srBody ) const
{
   bool ok = true;

   int lenBody = static_cast< int >( srBody.length( ));
   int lenSubject = static_cast< int >( srSubject.length( ));
   int lenAddress = static_cast< int >( srAddress.length( ));

   const char* pszTemplate = "echo \"\" | mail -s \"\" ";
   size_t lenTemplate = strlen( pszTemplate ) + 1;

   if( lenBody + lenSubject + lenAddress + lenTemplate > TIO_FORMAT_STRING_LEN_EMAIL )
   {
      lenBody = static_cast< int >( TIO_FORMAT_STRING_LEN_EMAIL - lenSubject - lenAddress - lenTemplate );
   }

   char szSendCmd[TIO_FORMAT_STRING_LEN_EMAIL + TIO_FORMAT_STRING_LEN_DATA];
   sprintf( szSendCmd, "echo \"%*s\" | mail -s \"%*s\" %*s",
                       lenBody, TIO_SR_STR( srBody ),
                       lenSubject, TIO_SR_STR( srSubject ),
	               lenAddress, TIO_SR_STR( srAddress ));
   ok = ( system( szSendCmd ) >= 0 ? true : false );

   return( ok );
}
