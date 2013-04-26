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
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
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
