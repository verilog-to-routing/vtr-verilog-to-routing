//===========================================================================//
// Purpose : Function definitions for miscellaneous helpful string functions.
//
//           Functions include:
//           - TNO_ExtractStringStatusMode
//           - TNO_ExtractStringNodeType
//           - TNO_FormatNameIndex
//           - TNO_ParseNameIndex
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

#include "TNO_StringUtils.h"

//===========================================================================//
// Function       : TNO_ExtractStringStatusMode
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_ExtractStringStatusMode(
      TNO_StatusMode_t mode,
      string*          psrMode )
{
   if( psrMode )
   {
      *psrMode = "";

      switch( mode )
      {
      case TNO_STATUS_OPEN:    *psrMode = "open";    break;
      case TNO_STATUS_GROUTED: *psrMode = "grouted"; break;
      case TNO_STATUS_ROUTED:  *psrMode = "routed";  break;
      default:                 *psrMode = "*";       break;
      }
   }
} 

//===========================================================================//
// Function       : TNO_ExtractStringNodeType
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_ExtractStringNodeType(
      TNO_NodeType_t type,
      string*         psrType )
{
   if( psrType )
   {
      *psrType = "";

      switch( type )
      {
      case TNO_NODE_INST_PIN:   *psrType = "instpin";   break;
      case TNO_NODE_SEGMENT:    *psrType = "segment";   break;
      case TNO_NODE_SWITCH_BOX: *psrType = "switchbox"; break;
      default:                  *psrType = "*";         break;
      }
   }
} 

//===========================================================================//
// Function       : TNO_FormatNameIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/12 jeffr : Original
//===========================================================================//
void TNO_FormatNameIndex(
      const char*   pszName,
            size_t  index,
            string* psrNameIndex ) 
{
   if( psrNameIndex )
   {  
      *psrNameIndex = "";

      if( pszName )
      {
         *psrNameIndex += pszName;
      }
      if( index != SIZE_MAX )
      {
         char szIndex[TIO_FORMAT_STRING_LEN_VALUE];
         sprintf( szIndex, "%lu", static_cast< unsigned long >( index ));
         *psrNameIndex += "[";
         *psrNameIndex += szIndex;
         *psrNameIndex += "]";
      }
   }
}

//===========================================================================//
void TNO_FormatNameIndex(
      const string& srName,
            size_t  index,
            string* psrNameIndex )
{
   const char* pszName = ( srName.length( ) ? srName.data( ) : 0 );
   TNO_FormatNameIndex( pszName, index, psrNameIndex );
}

//===========================================================================//
// Function       : TNO_ParseNameIndex
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 02/21/12 jeffr : Original
//===========================================================================//
void TNO_ParseNameIndex(
      const char*   pszNameIndex,
            string* psrName,
            size_t* pindex )
{
   string srName( "" );
   size_t index = SIZE_MAX;

   if( pszNameIndex )
   {
      srName = pszNameIndex;
      size_t left = srName.find( '[' );
      size_t right = srName.find( ']' );
      if(( left != string::npos ) && ( right != string::npos ))
      {
         string srIndex( srName.substr( left + 1, right - left - 1 ));
         index = atoi( srIndex.data( ));
         srName = srName.substr( 0, left );
      }
   }

   if( psrName )
   {
      *psrName = srName;
   }
   if( pindex )
   {
      *pindex = index;
   }
}

//===========================================================================//
void TNO_ParseNameIndex(
      const string& srNameIndex,
            string* psrName,
            size_t* pindex )
{
   const char* pszNameIndex = ( srNameIndex.length( ) ? srNameIndex.data( ) : 0 );
   TNO_ParseNameIndex( pszNameIndex, psrName, pindex );
}
