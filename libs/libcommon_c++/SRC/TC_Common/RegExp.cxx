//===========================================================================//
// Purpose : Method definitions for the 'RegExp' class.
//
//           Public methods include:
//           - RegExp, ~RegExp
//           - Index
//           - Match
//           - IsValidRE
//
//---------------------------------------------------------------------------//
//                                            _____ ___     _   ___ ___ ___ 
// U.S. Application Specific Products        |_   _|_ _|   /_\ / __|_ _/ __|
//                                             | |  | |   / _ \\__ \| | (__ 
// ASIC Software Engineering Services          |_| |___| /_/ \_\___/___\___|
//
// Property of Texas Instruments -- For Unrestricted Use -- Unauthorized
// reproduction and/or sale is strictly prohibited.  This product is 
// protected under copyright law as an unpublished work.
//
// Created 2003, (C) Copyright 2003 Texas Instruments.  
//
// All rights reserved.
//
//   ---------------------------------------------------------------------
//      These commodities are under U.S. Government 'distribution
//      license' control.  As such, they are not to be re-exported
//      without prior approval from the U.S. Department of Commerce.
//   ---------------------------------------------------------------------
//
//===========================================================================//

#include "RegExp.h"
#include <string.h>

//===========================================================================//
// Method         : RegExp
// Author         : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 04/22/03 jsykes: Original
//===========================================================================//
RegExp::RegExp( const char* expression, int options )
{
   this->pcreOptions_ = options;
   this->pszMatch_ = NULL;

   ppcreCode_ = pcre_compile( expression, options,
                              &pszError, &errorOffset, 0 );
}

//===========================================================================//
// Method         : ~RegExp
// Author         : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 04/22/03 jsykes: Original
//===========================================================================//
RegExp::~RegExp( void )
{
   if ( ppcreCode_ )
      pcre_free( ppcreCode_ );
}

//===========================================================================//
// Method         : Index
// Author         : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 04/22/03 jsykes: Original
//===========================================================================//
bool RegExp::Index(
   const char* subject, size_t* start, size_t* len )
{
   bool match = false;

   if ( !ppcreCode_ || !subject || !start || !len )
      return( false );

   int subjectLen = static_cast<int>( strlen( subject ) );
   int msize;

   pcre_fullinfo( ppcreCode_, 0, PCRE_INFO_CAPTURECOUNT, &msize);

   msize = 3 * ( msize + 1 );
   int *m = new int[ msize ];

   pcreRc_ = pcre_exec( ppcreCode_, NULL, subject, subjectLen,
                        0, pcreOptions_, m, msize );

   if ( pcreRc_ >= 0 )
   {
      *start = m[0];
      *len   = m[1] - m[0];

      match = true;
   }

   delete[] m;

   return( match );
}

//===========================================================================//
// Method         : Match
// Author         : Jon Sykes
//---------------------------------------------------------------------------//
// Version history
// 04/22/03 jsykes: Original
//===========================================================================//
const char* RegExp::Match( const char* subject )
{
   size_t start, len;
   if ( this->Index( subject, &start, &len ) )
   {
      if ( this->pszMatch_ )
         delete[] this->pszMatch_;

      this->pszMatch_ = new char[ len + 1 ];

      strncpy( this->pszMatch_, &subject[ start ], len );
      this->pszMatch_[ len ] = 0;
      
      return( this->pszMatch_ );
   }
   else
      return( NULL );
}
