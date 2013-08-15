//===========================================================================//
// Purpose : Methods for the 'TOP_OptionsParser_c' class.
//
//           Inline public methods include:
//           - syn
//           - SetScanner
//           - SetFileName
//           - SetOptionsFile
//           - SetOptionsStore
//
//           Inline private methods include:
//           - FindOutputLaffMask_
//           - FindExecuteToolMask_
//           - FindBool_
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
//---------------------------------------------------------------------------//

//===========================================================================//
// Method         : syn
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOP_OptionsParser_c::syn( 
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
         else
         {
            if( this->srActiveCmd_.length( ))
            {
               srMsg += ", check \"";
               srMsg += this->srActiveCmd_;
               srMsg += "\" parameters";
            }
         }

         if( strlen( pszGroup ) > 0 )
         {
            srMsg += " in ";
            srMsg += pszGroup;
         }
      }

      this->poptionsFile_->SyntaxError( lineNum, 
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
// 05/01/12 jeffr : Original
//===========================================================================//
void TOP_OptionsParser_c::SetScanner( 
      TOP_OptionsScanner_c* pscanner )
{
   this->pscanner_ = pscanner;
}

//===========================================================================//
// Method         : SetFileName
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOP_OptionsParser_c::SetFileName(
      const char* pszFileName )
{
   this->srFileName_ = TIO_PSZ_STR( pszFileName );;
}

//===========================================================================//
// Method         : SetOptionsFile
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOP_OptionsParser_c::SetOptionsFile(
      TOP_OptionsFile_c* poptionsFile )
{
   this->poptionsFile_ = poptionsFile;
}

//===========================================================================//
// Method         : SetOptionsStore
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
void TOP_OptionsParser_c::SetOptionsStore(
      TOS_OptionsStore_c* poptionsStore )
{
   this->pinputOptions_ = &poptionsStore->controlSwitches.inputOptions;
   this->poutputOptions_ = &poptionsStore->controlSwitches.outputOptions;
   this->pmessageOptions_ = &poptionsStore->controlSwitches.messageOptions;
   this->pexecuteOptions_ = &poptionsStore->controlSwitches.executeOptions;

   this->pfabricOptions_ = &poptionsStore->rulesSwitches.fabricOptions;
   this->ppackOptions_ = &poptionsStore->rulesSwitches.packOptions;
   this->pplaceOptions_ = &poptionsStore->rulesSwitches.placeOptions;
   this->prouteOptions_ = &poptionsStore->rulesSwitches.routeOptions;
}

//===========================================================================//
// Method         : FindOutputLaffMask_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
int TOP_OptionsParser_c::FindOutputLaffMask_(
      ANTLRTokenType tokenType )
{
   int mask = TOS_OUTPUT_LAFF_UNDEFINED;
   switch( tokenType )
   {
   case NONE:          mask = TOS_OUTPUT_LAFF_NONE;          break;
   case FABRIC_VIEW:   mask = TOS_OUTPUT_LAFF_FABRIC_VIEW;   break;
   case BOUNDING_BOX:  mask = TOS_OUTPUT_LAFF_BOUNDING_BOX;  break;
   case INTERNAL_GRID: mask = TOS_OUTPUT_LAFF_INTERNAL_GRID; break;
   case ANY:
   case ALL:           mask = TOS_OUTPUT_LAFF_ALL;           break;
   }
   return( mask );
}

//===========================================================================//
// Method         : FindExecuteToolMask_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
int TOP_OptionsParser_c::FindExecuteToolMask_(
      ANTLRTokenType tokenType )
{
   int mask = TOS_EXECUTE_TOOL_UNDEFINED;
   switch( tokenType )
   {
   case NONE:  mask = TOS_EXECUTE_TOOL_NONE;  break;
   case PACK:  mask = TOS_EXECUTE_TOOL_PACK;  break;
   case PLACE: mask = TOS_EXECUTE_TOOL_PLACE; break;
   case ROUTE: mask = TOS_EXECUTE_TOOL_ROUTE; break;
   case ANY:
   case ALL:   mask = TOS_EXECUTE_TOOL_ALL;   break;
   }
   return( mask );
}

//===========================================================================//
// Method         : FindBool_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
bool TOP_OptionsParser_c::FindBool_( 
      ANTLRTokenType tokenType )
{
   bool boolType = false;
   switch( tokenType )
   {
   case BOOL_ON: 
   case BOOL_YES: 
   case BOOL_TRUE:  boolType = true;  break;
   case BOOL_OFF: 
   case BOOL_NO: 
   case BOOL_FALSE: boolType = false; break;
   }
   return( boolType );
}
