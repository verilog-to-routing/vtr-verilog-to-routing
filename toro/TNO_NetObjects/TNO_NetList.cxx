//===========================================================================//
// Purpose : Method definitions for the TNO_NetList class.
//
//           Public methods include:
//           - TNO_NetList_c, ~TNO_NetList_c
//           - operator=
//           - operator==, operator!=
//           - operator[]
//           - Print
//           - ExpandNameList
//           - IsRoutable
//
//           Private methods include:
//           - ExpandBusRegExp_
//           - ApplyRegExp_
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

#include "TIO_Typedefs.h"
#include "TIO_PrintHandler.h"

#include "TNO_NetList.h"

//===========================================================================//
// Method         : TNO_NetList_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_NetList_c::TNO_NetList_c( 
      size_t allocLen,
      size_t reallocLen )
      :
      TCT_SortedNameDynamicVector_c< TNO_Net_c >( allocLen, reallocLen )
{
}

//===========================================================================//
TNO_NetList_c::TNO_NetList_c( 
      const TNO_NetList_c& netList )
      :
      TCT_SortedNameDynamicVector_c< TNO_Net_c >( netList )
{
}

//===========================================================================//
// Method         : ~TNO_NetList_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_NetList_c::~TNO_NetList_c( 
      void )
{
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_NetList_c& TNO_NetList_c::operator=( 
      const TNO_NetList_c& netList )
{
   if( &netList != this )
   {
      TCT_SortedNameDynamicVector_c< TNO_Net_c >::operator=( netList );
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_NetList_c::operator==( 
      const TNO_NetList_c& netList ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::operator==( netList ));
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_NetList_c::operator!=( 
      const TNO_NetList_c& netList ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::operator!=( netList ));
}

//===========================================================================//
// Method         : operator[]
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
TNO_Net_c* TNO_NetList_c::operator[]( 
      size_t netIndex )
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::operator[]( netIndex ));
}

//===========================================================================//
TNO_Net_c* TNO_NetList_c::operator[]( 
      size_t netIndex ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::operator[]( netIndex ));
}

//===========================================================================//
TNO_Net_c* TNO_NetList_c::operator[]( 
      const TNO_Net_c& net )
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::operator[]( net ));
}

//===========================================================================//
TNO_Net_c* TNO_NetList_c::operator[]( 
      const TNO_Net_c& net ) const
{
   return( TCT_SortedNameDynamicVector_c< TNO_Net_c >::operator[]( net ));
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
void TNO_NetList_c::Print( 
      FILE*  pfile,
      size_t spaceLen ) const
{
   for( size_t i = 0; i < this->GetLength( ); ++i )
   {
      TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
      printHandler.Write( pfile, spaceLen, "<" );

      const TNO_Net_c& net = *this->operator[]( i );
      net.Print( pfile, spaceLen + 3 );
   }
}

//===========================================================================//
// Method         : ExpandNameList
// Purpose        : Applies regular expression pattern matching on the given 
//                  net name, returning a list of matching net names, if any.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_NetList_c::ExpandNameList( 
      const TNO_NameList_t& netNameList,
            TNO_NameList_t* pnetNameList,
            TC_TypeMode_t   netType,
            bool            isExpandBusEnabled,
            bool            isShowWarningEnabled,
            bool            isShowErrorEnabled,
      const char*           pszShowRegExpType ) const
{
   bool ok = true;

   if( pnetNameList )
   {
      TNO_NameList_t netNameList_( netNameList );
      pnetNameList->Clear( );

      // Replace any potential bus ranges "[n:m]" with "[n]" to "[m]"
      if( isExpandBusEnabled )
      {
         this->ExpandBusRegExp_( &netNameList_ );
      }

      // Ready to apply regular expression matching to net name list
      if( !pszShowRegExpType )
      {
         pszShowRegExpType = "net";
      }

      ok = this->ApplyRegExp_( netNameList_,
                               &netNameList_,
                               isShowWarningEnabled,
                               isShowErrorEnabled,
                               pszShowRegExpType );
      if( ok )
      {
         // Add all expanded net names to a sortable net list
         TNO_NetList_c netList_( netNameList_.GetLength( ));

         for( size_t i = 0; i < netNameList_.GetLength( ); ++i )
         {
            const TC_Name_c& netName_ = *netNameList_[i];
            const char* pszNetName_ = netName_.GetName( );

            TNO_Net_c* pnet = this->Find( pszNetName_ );
            if( !pnet )
               continue;

            if( !pnet->IsRoutable( ))
               continue;

            if(( netType != TC_TYPE_UNDEFINED ) && ( netType != pnet->GetType( )))
               continue;
 
            TNO_Net_c net( pszNetName_ );
            netList_.Add( net );
         }

         // Force sort to detect and remove duplicate nets
         netList_.Find( "" );

         // Load and return net name list base on non-duplicate net list
         for( size_t i = 0; i < netList_.GetLength( ); ++i )
         {
            const TNO_Net_c& net = *netList_[i];

            const char* pszNetName = net.GetName( ); 
            TC_Name_c netName_( pszNetName ); 
            pnetNameList->Add( netName_ );
         }
      }
   }
   return( ok );
}

//===========================================================================//
bool TNO_NetList_c::ExpandNameList(
      const TNO_NameList_t& netNameList,
            TNO_NameList_t* pnetNameList,
            bool            isExpandBusEnabled,
            bool            isShowWarningEnabled,
            bool            isShowErrorEnabled,
      const char*           pszShowRegExpType ) const
{
   TC_TypeMode_t netType = TC_TYPE_UNDEFINED;
   return( this->ExpandNameList( netNameList, 
                                 pnetNameList,
                                 netType,
                                 isExpandBusEnabled,
                                 isShowWarningEnabled,
                                 isShowErrorEnabled,
                                 pszShowRegExpType ));
}

//===========================================================================//
bool TNO_NetList_c::ExpandNameList(
      const string&         srNetName,
            TNO_NameList_t* pnetNameList,
            bool            isExpandBusEnabled,
            bool            isShowWarningEnabled,
            bool            isShowErrorEnabled,
      const char*           pszShowRegExpType ) const
{
   TC_Name_c netName( srNetName );
   TNO_NameList_t netNameList( 1 );
   netNameList.Add( netName );

   return( this->ExpandNameList( netNameList, 
                                 pnetNameList,
                                 isExpandBusEnabled,
                                 isShowWarningEnabled,
                                 isShowErrorEnabled,
                                 pszShowRegExpType ));
}

//===========================================================================//
bool TNO_NetList_c::ExpandNameList(
      const char*           pszNetName,
            TNO_NameList_t* pnetNameList,
            bool            isExpandBusEnabled,
            bool            isShowWarningEnabled,
            bool            isShowErrorEnabled,
      const char*           pszShowRegExpType ) const
{
   TC_Name_c netName( pszNetName );
   TNO_NameList_t netNameList( 1 );
   netNameList.Add( netName );

   return( this->ExpandNameList( netNameList, 
                                 pnetNameList,
                                 isExpandBusEnabled,
                                 isShowWarningEnabled,
                                 isShowErrorEnabled,
                                 pszShowRegExpType ));
}

//===========================================================================//
// Method         : IsRoutable
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_NetList_c::IsRoutable( 
      size_t netIndex ) const
{
   bool isRoutable = false;

   TNO_Net_c* pnet = this->operator[]( netIndex );
   if( pnet )
   {
      isRoutable = pnet->IsRoutable( );
   } 
   return( isRoutable );
}

//===========================================================================//
bool TNO_NetList_c::IsRoutable( 
      const TNO_Net_c& net ) const
{
   size_t netIndex = this->FindIndex( net );

   return( this->IsRoutable( netIndex ));
}

//===========================================================================//
bool TNO_NetList_c::IsRoutable( 
      const string& srNetName ) const
{
   size_t netIndex = this->FindIndex( srNetName );

   return( this->IsRoutable( netIndex ));
}

//===========================================================================//
bool TNO_NetList_c::IsRoutable( 
      const char* pszNetName ) const
{
   size_t netIndex = this->FindIndex( pszNetName );

   return( this->IsRoutable( netIndex ));
}

//===========================================================================//
// Method         : ExpandBusRegExp_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_NetList_c::ExpandBusRegExp_(
      TNO_NameList_t* pnameList ) const
{
   bool foundBusRegExp = false;

   TNO_NameList_t nameList( pnameList->GetLength( ));

   for( size_t i = 0; i < pnameList->GetLength( ); ++i )
   {
      const TC_Name_c* pname = ( *pnameList )[i];
      const string& srName = pname->GetString( );

      size_t delimiter = srName.find( ':' );
      size_t left = srName.find( '[' );
      size_t right = srName.find( ']' );
      if(( left == string::npos ) &&
         ( right == string::npos ))
      {
         left = srName.find( '(' );
         right = srName.find( ')' );
      }

      if(( delimiter != string::npos ) && 
         ( left != string::npos ) &&
         ( right != string::npos ) &&
         ( left < delimiter - 1 ) &&
         ( delimiter + 1 < right ))
      {
         bool isBusRegExp = true;

         for( size_t pos = left + 1; pos <= right - 1; ++pos )
         {
            if( !isdigit( srName[pos] ) && ( pos != delimiter ))
            {
               isBusRegExp = false;
               break;
            }
         }

         if( isBusRegExp )
         {
            foundBusRegExp = true;

            string srn( srName.substr( left + 1, delimiter - left - 1 ));
            string srm( srName.substr( delimiter + 1, right - delimiter - 1 ));

            int n = atoi( srn.data( ));
            int m = atoi( srm.data( ));

            while( foundBusRegExp )
            {
               char szn[TIO_FORMAT_STRING_LEN_VALUE];
               sprintf( szn, "%d", n );

               size_t len = srName.length( );

               string srName_( "" );
               srName_ += srName.substr( 0, left + 1 );
               srName_ += szn;
               srName_ += srName.substr( right, len - right );

               TC_Name_c name( srName_ );
               nameList.Add( name );

               if( n == m )
                  break;
               else if( n < m )
                  ++n;
               else if( n > m )
                  --n;
            }
         }
      }
      else
      {
         TC_Name_c name( srName );
         nameList.Add( name );
      }
   }

   if( foundBusRegExp )
   {
      pnameList->Clear( );
      pnameList->SetCapacity( nameList.GetLength( ));
      pnameList->Add( nameList );
   }
   return( foundBusRegExp );
}

//===========================================================================//
// Method         : ApplyRegExp_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
bool TNO_NetList_c::ApplyRegExp_( 
      const TNO_NameList_t& nameList,
            TNO_NameList_t* pnameList,
            bool            isShowWarningEnabled,
            bool            isShowErrorEnabled,
      const char*           pszShowRegExpType ) const
{
   bool ok = false;
   ok = TCT_SortedNameDynamicVector_c< TNO_Net_c >::ApplyRegExp( nameList,
                                                                 pnameList,
                                                                 isShowWarningEnabled,
                                                                 isShowErrorEnabled,
                                                                 pszShowRegExpType );
   return( ok );
}
