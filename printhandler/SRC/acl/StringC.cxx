//===========================================================================//
// Purpose : StringC class based on std::string
// Author  : Jon Sykes
// Date    : 04/25/03
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

#include "StringC.h"

//===========================================================================//
unsigned int StringC::hash( void ) const
{
   // Adapted from the TCL hash function
   //

   const char* sz = this->srString_.c_str();
   unsigned int result = 0;
   while( *sz )
   {
      result += (result << 3) + *sz;
      ++sz;
   }

   return( result );
}

//===========================================================================//
StringC StringC::strip( stripType t, char c ) const
{
   StringC srCopy = this->srString_.c_str();

   std::string::size_type pos;
   std::string::size_type begin = 0;

   if ( ( t == both ) || ( t == leading ) )
   {
      pos = srCopy.srString_.find_first_not_of( c );
      if ( pos != std::string::npos )
         srCopy.srString_.erase( begin, pos );
   }

   std::string::size_type end = srCopy.srString_.size() - 1;

   if ( ( t == both ) || ( t == trailing ) )
   {
      pos = srCopy.srString_.find_last_not_of( c );
      if ( pos != std::string::npos )
         srCopy.srString_.erase( pos + 1, end );
   }

   return( srCopy );
}


