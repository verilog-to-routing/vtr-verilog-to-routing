//===========================================================================//
// Purpose : Methods for the 'TAP_ArchitectureParser_c' class.
//
//           Inline public methods include:
//           - syn
//           - SetInterface
//           - SetScanner
//           - SetFileName
//           - SetArchitectureFile
//           - SetArchitectureSpec
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
// 05/15/12 jeffr : Original
//===========================================================================//
void TAP_ArchitectureParser_c::syn( 
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

      this->parchitectureFile_->SyntaxError( lineNum, 
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
void TAP_ArchitectureParser_c::SetInterface( 
      TAP_ArchitectureInterface_c* pinterface )
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
void TAP_ArchitectureParser_c::SetScanner( 
      TAP_ArchitectureScanner_c* pscanner )
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
void TAP_ArchitectureParser_c::SetFileName(
      const char* pszFileName )
{
   this->srFileName_ = TIO_PSZ_STR( pszFileName );
}

//===========================================================================//
// Method         : SetArchitectureFile
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAP_ArchitectureParser_c::SetArchitectureFile(
      TAP_ArchitectureFile_c* parchitectureFile )
{
   this->parchitectureFile_ = parchitectureFile;
}

//===========================================================================//
// Method         : SetArchitectureSpec
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
void TAP_ArchitectureParser_c::SetArchitectureSpec(
      TAS_ArchitectureSpec_c* parchitectureSpec )
{
   this->parchitectureSpec_ = parchitectureSpec;
}
