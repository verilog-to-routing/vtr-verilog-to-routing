//===========================================================================//
// Purpose : Declaration and inline definition(s) for a TIO_CustomOutput 
//           class. 
//
//           Inline methods include:
//           - TIO_CustomOutput_c, ~TIO_CustomOutput_c
//           - SetHandler
//           - SetEnabled
//           - IsEnabled
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

#ifndef TIO_CUSTOM_OUTPUT_H
#define TIO_CUSTOM_OUTPUT_H

#include "TIO_Typedefs.h"

//---------------------------------------------------------------------------//
// Define function pointer for an installable custom output handler
//---------------------------------------------------------------------------//
typedef void ( * TIO_PFX_CustomHandler_t )( TIO_PrintMode_t printMode,
                                            const char* pszPrintText,
                                            const char* pszPrintSrc );

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TIO_CustomOutput_c
{
public:

   TIO_CustomOutput_c( TIO_PFX_CustomHandler_t pfxCustomHandler = 0 );
   ~TIO_CustomOutput_c( void );

   bool Write( TIO_PrintMode_t printMode,
               const char* pszPrintText,
               const char* pszPrintSrc ) const;

   void SetHandler( TIO_PFX_CustomHandler_t pfxCustomHandler );
   void SetEnabled( bool isEnabled );
 
   bool IsEnabled( void ) const;

private:

   TIO_PFX_CustomHandler_t pfxCustomHandler_;
                           // Ptr to installed custom output handler function
   bool isEnabled_;        // Used to enable/disable output state
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline TIO_CustomOutput_c::TIO_CustomOutput_c( 
      TIO_PFX_CustomHandler_t pfxCustomHandler )
      :
      pfxCustomHandler_( pfxCustomHandler ),
      isEnabled_( pfxCustomHandler ? true : false )
{
}

//===========================================================================//
inline TIO_CustomOutput_c::~TIO_CustomOutput_c( 
      void )
{
}

//===========================================================================//
inline void TIO_CustomOutput_c::SetHandler( 
      TIO_PFX_CustomHandler_t pfxCustomHandler )
{
   this->pfxCustomHandler_ = pfxCustomHandler;
   this->isEnabled_ = ( pfxCustomHandler ? true : false );
}

//===========================================================================//
inline void TIO_CustomOutput_c::SetEnabled( 
      bool isEnabled )
{
   this->isEnabled_ = isEnabled;
}

//===========================================================================//
inline bool TIO_CustomOutput_c::IsEnabled( 
      void ) const
{
   return( this->isEnabled_ );
}

#endif 
