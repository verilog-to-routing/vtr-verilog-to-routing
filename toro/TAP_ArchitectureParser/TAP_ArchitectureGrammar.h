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
//           Inline private methods include:
//           - FindArraySizeMode_
//           - FindSwitchBoxType_
//           - FindSwitchBoxModelType_
//           - FindSegmentDirType_
//           - FindPinAssignPatternType_
//           - FindGridAssignDistrMode_
//           - FindGridAssignOrientMode_
//           - FindInterconnectMapType_
//           - FindClassType_
//           - FindSideMode_
//           - FindTypeMode_
//           - FindBool_
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

//===========================================================================//
// Method         : FindArraySizeMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TAS_ArraySizeMode_t TAP_ArchitectureParser_c::FindArraySizeMode_(
      ANTLRTokenType tokenType )
{
   TAS_ArraySizeMode_t mode = TAS_ARRAY_SIZE_UNDEFINED;
   switch( tokenType )
   {
   case AUTO:   mode = TAS_ARRAY_SIZE_AUTO;   break;
   case MANUAL:
   case FIXED:  mode = TAS_ARRAY_SIZE_MANUAL; break;
   }
   return( mode );
}

//===========================================================================//
// Method         : FindSwitchBoxType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TAS_SwitchBoxType_t TAP_ArchitectureParser_c::FindSwitchBoxType_(
      ANTLRTokenType tokenType )
{
   TAS_SwitchBoxType_t type = TAS_SWITCH_BOX_UNDEFINED;
   switch( tokenType )
   {
   case BUFFER: type = TAS_SWITCH_BOX_BUFFERED; break;
   case MUX:    type = TAS_SWITCH_BOX_MUX;      break;
   }
   return( type );
}

//===========================================================================//
// Method         : FindSwitchBoxModelType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TAS_SwitchBoxModelType_t TAP_ArchitectureParser_c::FindSwitchBoxModelType_(
      ANTLRTokenType tokenType )
{
   TAS_SwitchBoxModelType_t modelType = TAS_SWITCH_BOX_MODEL_UNDEFINED;
   switch( tokenType )
   {
   case WILTON:    modelType = TAS_SWITCH_BOX_MODEL_WILTON;    break;
   case DISJOINT:
   case SUBSET:    modelType = TAS_SWITCH_BOX_MODEL_SUBSET;    break;
   case UNIVERSAL: modelType = TAS_SWITCH_BOX_MODEL_UNIVERSAL; break;
   case CUSTOM:    modelType = TAS_SWITCH_BOX_MODEL_CUSTOM;    break;
   }
   return( modelType );
}

//===========================================================================//
// Method         : FindSegmentDirType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TAS_SegmentDirType_t TAP_ArchitectureParser_c::FindSegmentDirType_(
      ANTLRTokenType tokenType )
{
   TAS_SegmentDirType_t type = TAS_SEGMENT_DIR_UNDEFINED;
   switch( tokenType )
   {
   case UNIDIR: type = TAS_SEGMENT_DIR_UNIDIRECTIONAL; break;
   case BIDIR:  type = TAS_SEGMENT_DIR_BIDIRECTIONAL;  break;
   }
   return( type );
}

//===========================================================================//
// Method         : FindPinAssignPatternType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TAS_PinAssignPatternType_t TAP_ArchitectureParser_c::FindPinAssignPatternType_(
      ANTLRTokenType tokenType )
{
   TAS_PinAssignPatternType_t type = TAS_PIN_ASSIGN_PATTERN_UNDEFINED;
   switch( tokenType )
   {
   case SPREAD: type = TAS_PIN_ASSIGN_PATTERN_SPREAD; break;
   case CUSTOM: type = TAS_PIN_ASSIGN_PATTERN_CUSTOM; break;
   }
   return( type );
}

//===========================================================================//
// Method         : FindGridAssignDistrMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TAS_GridAssignDistrMode_t TAP_ArchitectureParser_c::FindGridAssignDistrMode_(
      ANTLRTokenType tokenType )
{
   TAS_GridAssignDistrMode_t mode = TAS_GRID_ASSIGN_DISTR_UNDEFINED;
   switch( tokenType )
   {
   case REL:
   case SINGLE:    mode = TAS_GRID_ASSIGN_DISTR_SINGLE;    break;
   case COL:
   case MULTIPLE:  mode = TAS_GRID_ASSIGN_DISTR_MULTIPLE;  break;
   case FILL:      mode = TAS_GRID_ASSIGN_DISTR_FILL;      break;
   case PERIMETER: mode = TAS_GRID_ASSIGN_DISTR_PERIMETER; break;
   }
   return( mode );
}

//===========================================================================//
// Method         : FindGridAssignOrientMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TAS_GridAssignOrientMode_t TAP_ArchitectureParser_c::FindGridAssignOrientMode_(
      ANTLRTokenType tokenType )
{
   TAS_GridAssignOrientMode_t mode = TAS_GRID_ASSIGN_ORIENT_UNDEFINED;
   switch( tokenType )
   {
   case COL: mode = TAS_GRID_ASSIGN_ORIENT_COLUMN; break;
   case ROW: mode = TAS_GRID_ASSIGN_ORIENT_ROW;    break;
   }
   return( mode );
}

//===========================================================================//
// Method         : FindInterconnectMapType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TAS_InterconnectMapType_t TAP_ArchitectureParser_c::FindInterconnectMapType_(
      ANTLRTokenType tokenType )
{
   TAS_InterconnectMapType_t type = TAS_INTERCONNECT_MAP_UNDEFINED;
   switch( tokenType )
   {
   case COMPLETE: type = TAS_INTERCONNECT_MAP_COMPLETE; break;
   case DIRECT:   type = TAS_INTERCONNECT_MAP_DIRECT;   break;
   case MUX:      type = TAS_INTERCONNECT_MAP_MUX;      break;
   }
   return( type );
}

//===========================================================================//
// Method         : FindClassType_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TAS_ClassType_t TAP_ArchitectureParser_c::FindClassType_(
      ANTLRTokenType tokenType )
{
   TAS_ClassType_t type = TAS_CLASS_UNDEFINED;
   switch( tokenType )
   {
   case LUT:      type = TAS_CLASS_LUT;      break;
   case FLIPFLOP: type = TAS_CLASS_FLIPFLOP; break;
   case MEMORY:   type = TAS_CLASS_MEMORY;   break;
   case SUBCKT:   type = TAS_CLASS_SUBCKT;   break;
   }
   return( type );
}

//===========================================================================//
// Method         : FindSideMode_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
TC_SideMode_t TAP_ArchitectureParser_c::FindSideMode_(
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
TC_TypeMode_t TAP_ArchitectureParser_c::FindTypeMode_(
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
bool TAP_ArchitectureParser_c::FindBool_(
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
