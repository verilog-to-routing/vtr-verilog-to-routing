//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_RegExp class.
//
//           Inline methods include:
//           - IsValid
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

#ifndef TC_REGEXP_H
#define TC_REGEXP_H

#include "pcre.h"

class TC_RegExp
{
public:

   TC_RegExp( const char* pszExpression, 
              int pcreOptions = 0 );
   ~TC_RegExp( void );

   bool Index( const char* pszSubject, 
               size_t* pstart, 
               size_t* plen ) const;
   const char* Match( const char* pszSubject );

   bool IsValid( void ) const;

public:

   const char* pszError;
   int         errorOffset;

private:

   char* pszMatch_;
   pcre* ppcreCode_;
   int   pcreOptions_;
};

//===========================================================================//
// Purpose:        : Class inline definition(s)
// Author          : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 04/22/03 jsykes : Original
// 07/10/12 jeffr  : Ported from acl to TC_* common library
//===========================================================================//
inline bool TC_RegExp::IsValid(
      void ) const
{
   return( this->ppcreCode_ ? true : false );
}

#endif
