//===========================================================================//
// Purpose : Regular expression class based on PCRE
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
#ifndef ACL_REGEXP_H
#define ACL_REGEXP_H

#include "pcre.h"

class RegExp
{
public:

   RegExp( const char* expression, int options = 0 );
  ~RegExp( void );

   bool        Index( const char* subject, size_t* start, size_t* len );
   const char* Match( const char* subject );
   bool        IsValidRE( void ) const;

public:

   const char* pszError;
   int         errorOffset;

private:

   char*  pszMatch_;
   pcre*  ppcreCode_;
   int    pcreOptions_;
   int    pcreRc_;
};

//===========================================================================//
// Function inline defintions
//===========================================================================//

inline bool RegExp::IsValidRE( void ) const
{
   return( ( ppcreCode_ != NULL ) ? true : false );
}

#endif
