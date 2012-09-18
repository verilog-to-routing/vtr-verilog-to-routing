//===========================================================================//
// Purpose : Methods for the 'TFP_FabricParser_c' class.
//
//           Inline public methods include:
//           - syn
//           - SetInterface
//           - SetScanner
//           - SetFileName
//           - SetFabricFile
//           - SetFabricModel
//
//           Inline private methods include:
//           - FindOrientMode_
//           - FindSideMode_
//
//===========================================================================//

//===========================================================================//
// Method         : syn
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
void TFP_FabricParser_c::syn( 
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

      this->pfabricFile_->SyntaxError( lineNum, 
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
// 06/15/12 jeffr : Original
//===========================================================================//
void TFP_FabricParser_c::SetInterface( 
      TFP_FabricInterface_c* pinterface )
{
   this->pinterface_ = pinterface;
}

//===========================================================================//
// Method         : SetScanner
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
void TFP_FabricParser_c::SetScanner( 
      TFP_FabricScanner_c* pscanner )
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
void TFP_FabricParser_c::SetFileName(
      const char* pszFileName )
{
   this->srFileName_ = TIO_PSZ_STR( pszFileName );
}

//===========================================================================//
// Method         : SetFabricFile
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
void TFP_FabricParser_c::SetFabricFile(
      TFP_FabricFile_c* pfabricFile )
{
   this->pfabricFile_ = pfabricFile;
}

//===========================================================================//
// Method         : SetFabricModel
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
void TFP_FabricParser_c::SetFabricModel(
      TFM_FabricModel_c* pfabricModel )
{
   this->pfabricModel_ = pfabricModel;
}

//===========================================================================//
// Method         : FindOrientMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TGS_OrientMode_t TFP_FabricParser_c::FindOrientMode_(
      ANTLRTokenType tokenType )
{
   TGS_OrientMode_t mode = TGS_ORIENT_UNDEFINED;
   switch( tokenType )
   {
   case HORIZONTAL: mode = TGS_ORIENT_HORIZONTAL; break;
   case VERTICAL:   mode = TGS_ORIENT_VERTICAL;   break;
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
TC_SideMode_t TFP_FabricParser_c::FindSideMode_(
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
