//===========================================================================//
// Purpose : Methods for the 'TCP_CircuitParser_c' class.
//
//           Inline public methods include:
//           - syn
//           - SetScanner
//           - SetFileName
//           - SetCircuitFile
//           - SetCircuitDesign
//
//           Inline private methods include:
//           - FindPlaceStatusMode_
//           - FindNetStatusMode_
//           - FindSideMode_
//           - FindTypeMode_
//           - FindBool_
//
//===========================================================================//

//===========================================================================//
// Method         : syn
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
void TCP_CircuitParser_c::syn( 
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

      this->pcircuitFile_->SyntaxError( lineNum, 
                                          this->srFileName_, 
                                          srMsg.data( ));
      this->consumeUntilToken( END_OF_FILE );
   }
}

//===========================================================================//
// Method         : SetScanner
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
void TCP_CircuitParser_c::SetScanner( 
      TCP_CircuitScanner_c* pscanner )
{
   this->pscanner_ = pscanner;
}

//===========================================================================//
// Method         : SetFileName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
void TCP_CircuitParser_c::SetFileName(
      const char* pszFileName )
{
   this->srFileName_ = TIO_PSZ_STR( pszFileName );
}

//===========================================================================//
// Method         : SetCircuitFile
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
void TCP_CircuitParser_c::SetCircuitFile(
      TCP_CircuitFile_c* pcircuitFile )
{
   this->pcircuitFile_ = pcircuitFile;
}

//===========================================================================//
// Method         : SetCircuitDesign
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
void TCP_CircuitParser_c::SetCircuitDesign(
      TCD_CircuitDesign_c* pcircuitDesign )
{
   this->pcircuitDesign_ = pcircuitDesign;
}

//===========================================================================//
// Method         : FindPlaceStatusMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TPO_StatusMode_t TCP_CircuitParser_c::FindPlaceStatusMode_(
      ANTLRTokenType tokenType )
{
   TPO_StatusMode_t mode = TPO_STATUS_UNDEFINED;
   switch( tokenType )
   {
   case PLACE_FLOAT:  mode = TPO_STATUS_FLOAT;  break;
   case PLACE_FIXED:  mode = TPO_STATUS_FIXED;  break;
   case PLACE_PLACED: mode = TPO_STATUS_PLACED; break;
   }
   return( mode );
}

//===========================================================================//
// Method         : FindNetStatusMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TNO_StatusMode_t TCP_CircuitParser_c::FindNetStatusMode_(
      ANTLRTokenType tokenType )
{
   TNO_StatusMode_t mode = TNO_STATUS_UNDEFINED;
   switch( tokenType )
   {
   case NET_OPEN:    mode = TNO_STATUS_OPEN;    break;
   case NET_GROUTED: mode = TNO_STATUS_GROUTED; break;
   case NET_ROUTED:  mode = TNO_STATUS_ROUTED;  break;
   }
   return( mode );
}

//===========================================================================//
// Method         : FindSideMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TC_SideMode_t TCP_CircuitParser_c::FindSideMode_(
      ANTLRTokenType tokenType )
{
   TC_SideMode_t mode = TC_SIDE_UNDEFINED;
   switch( tokenType )
   {
   case LEFT:   mode = TC_SIDE_LEFT;  break;
   case R:
   case RIGHT:  mode = TC_SIDE_RIGHT; break;
   case BOTTOM:
   case LOWER:  mode = TC_SIDE_LOWER; break;
   case T: 
   case TOP:  
   case UPPER:  mode = TC_SIDE_UPPER; break; 
   }
   return( mode );
}

//===========================================================================//
// Method         : FindTypeMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TC_TypeMode_t TCP_CircuitParser_c::FindTypeMode_(
      ANTLRTokenType tokenType )
{
   TC_TypeMode_t type = TC_TYPE_UNDEFINED;
   switch( tokenType )
   {
   case INPUT:  type = TC_TYPE_INPUT;  break;
   case OUTPUT: type = TC_TYPE_OUTPUT; break;
   case SIGNAL: type = TC_TYPE_SIGNAL; break;
   case CLOCK:  type = TC_TYPE_CLOCK;  break;
   case POWER:  type = TC_TYPE_POWER;  break;
   case GLOBAL: type = TC_TYPE_GLOBAL; break;
   }
   return( type );
}

//===========================================================================//
// Method         : FindBool_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
bool TCP_CircuitParser_c::FindBool_(
      ANTLRTokenType tokenType )
{
   bool bool_ = FALSE;
   switch( tokenType )
   {
   case BOOL_TRUE:  bool_ = TRUE;  break;
   case BOOL_FALSE: bool_ = FALSE; break;
   }
   return( bool_ );
}

