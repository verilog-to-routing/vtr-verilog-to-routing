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
