//===========================================================================//
// Purpose : Methods for the 'TCP_CircuitParser_c' class.
//
//           Inline public methods include:
//           - syn
//           - SetInterface
//           - SetScanner
//           - SetFileName
//           - SetCircuitFile
//           - SetCircuitDesign
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

      this->pcircuitFile_->SyntaxError( lineNum, 
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
void TCP_CircuitParser_c::SetInterface( 
      TCP_CircuitInterface_c* pinterface )
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
