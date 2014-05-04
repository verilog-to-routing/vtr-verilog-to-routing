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
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
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
