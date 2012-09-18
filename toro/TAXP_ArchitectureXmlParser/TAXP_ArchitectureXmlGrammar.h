//===========================================================================//
// Purpose : Methods for the 'TAXP_ArchitectureXmlParser_c' class.
//
//           Inline public methods include:
//           - syn
//           - SetInterface
//           - SetScanner
//           - SetFileName
//           - SetArchitectureXmlFile
//           - SetArchitectureSpec
//
//===========================================================================//

//===========================================================================//
// Method         : syn
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAXP_ArchitectureXmlParser_c::syn( 
      ANTLRAbstractToken* /* pToken */, 
      ANTLRChar*          pszGroup,
      SetWordType*        /* pWordType */,
      ANTLRTokenType      tokenType,
      int                 /* k */ )
{
   if( LT( 1 ) && ( LT( 1 )->getType( ) != END_OF_FILE ))
   {
      int lineNum = LT( 1 )->getLine( );

      string srFoundToken = this->token_tbl[LT( 1 )->getType( )];
      string srFoundText  = LT( 1 )->getText( );

      string srMsg;

      if( LT( 1 )->getType( ) == UNCLOSED_STRING )
      {
	 srMsg = "at newline character. Closing quote is missing";
      }
      else
      {
	 srMsg  = "at " + srFoundToken + " token, ";
	 srMsg += "\"" + srFoundText + "\"";

         if( tokenType && ( tokenType != DLGminToken ))
         {
            string srExpectToken = this->token_tbl[tokenType];
	    srMsg += ", expected a " + srExpectToken + " token";
         }

	 if( strlen( pszGroup ) > 0 )
	 {
	    srMsg += " in ";
	    srMsg += pszGroup;
	 }
      }

      this->parchitectureXmlFile_->SyntaxError( lineNum, 
                                                this->srFileName_, 
                                                srMsg.data( ));
      this->consumeUntilToken( END_OF_FILE );
   }
}

//===========================================================================//
// Method         : SetInterface
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAXP_ArchitectureXmlParser_c::SetInterface( 
      TAXP_ArchitectureXmlInterface_c* pinterface )
{
   this->pinterface_ = pinterface;
}

//===========================================================================//
// Method         : SetScanner
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAXP_ArchitectureXmlParser_c::SetScanner( 
      TAXP_ArchitectureXmlScanner_c* pscanner )
{
   this->pscanner_ = pscanner;
}

//===========================================================================//
// Method         : SetFileName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAXP_ArchitectureXmlParser_c::SetFileName(
      const char* pszFileName )
{
   this->srFileName_ = TIO_PSZ_STR( pszFileName );
}

//===========================================================================//
// Method         : SetArchitectureXmlFile
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAXP_ArchitectureXmlParser_c::SetArchitectureXmlFile(
      TAXP_ArchitectureXmlFile_c* parchitectureXmlFile )
{
   this->parchitectureXmlFile_ = parchitectureXmlFile;
}

//===========================================================================//
// Method         : SetArchitectureSpec
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAXP_ArchitectureXmlParser_c::SetArchitectureSpec(
      TAS_ArchitectureSpec_c* parchitectureSpec )
{
   this->parchitectureSpec_ = parchitectureSpec;
}
