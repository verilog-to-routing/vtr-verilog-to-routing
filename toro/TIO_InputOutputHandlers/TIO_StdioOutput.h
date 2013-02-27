//===========================================================================//
// Purpose : Declaration and inline definition(s) for a TIO_StdioOutput
//           class.
//
//           Inline methods include:
//           - TIO_StdioOutput_c, ~TIO_StdioOutput_c
//           - Write
//           - Flush
//           - SetStream
//           - SetEnabled
//           - GetStream
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

#ifndef TIO_STDIO_OUTPUT_H
#define TIO_STDIO_OUTPUT_H

#include <cstdio>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TIO_StdioOutput_c
{
public:

   TIO_StdioOutput_c( FILE* pstream = 0 );
   ~TIO_StdioOutput_c( void );
 
   void Write( const char* pszString ) const;
   void Flush( void ) const;

   void SetStream( FILE* pstream );
   void SetEnabled( bool isEnabled );

   FILE* GetStream( void ) const;
 
   bool IsEnabled( void ) const;

private:

   FILE* pstream_;                   // Define this output's stream handle
   bool  isEnabled_;                 // Used to enable/disable output state
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline TIO_StdioOutput_c::TIO_StdioOutput_c( 
      FILE* pstream )
      :
      pstream_( pstream ),
      isEnabled_( pstream ? true : false )
{
}

//===========================================================================//
inline TIO_StdioOutput_c::~TIO_StdioOutput_c( 
      void )
{
}

//===========================================================================//
inline void TIO_StdioOutput_c::Write( 
      const char* pszString ) const
{
   fputs( pszString, this->pstream_ );
}

//===========================================================================//
inline void TIO_StdioOutput_c::Flush( 
      void ) const
{
   fflush( this->pstream_ );
}

//===========================================================================//
inline void TIO_StdioOutput_c::SetStream( 
      FILE* pstream )
{
   this->pstream_ = pstream;
   this->isEnabled_ = pstream ? true : false;
}

//===========================================================================//
inline void TIO_StdioOutput_c::SetEnabled( 
      bool isEnabled )
{
   this->isEnabled_ = isEnabled;
}

//===========================================================================//
inline FILE* TIO_StdioOutput_c::GetStream( 
      void ) const
{
   return( this->pstream_ );
}

//===========================================================================//
inline bool TIO_StdioOutput_c::IsEnabled( 
      void ) const
{
   return( this->isEnabled_ );
}

#endif
