//===========================================================================//
// Purpose : Declaration and inline definitions for TCP_CircuitInterface
//           abstract base class.
//
//           Inline methods include:
//           - TCP_CircuitInterface, ~TCP_CircuitInterface
//
//===========================================================================//

#ifndef TCP_CIRCUIT_INTERFACE_H
#define TCP_CIRCUIT_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
class TCP_CircuitInterface_c
{
public:

   TCP_CircuitInterface_c( void );
   virtual ~TCP_CircuitInterface_c( void );

   virtual bool SyntaxError( unsigned int lineNum, 
                             const string& srFileName,
			     const char* pszMessageText ) = 0;

   virtual bool IsValid( void ) const = 0;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
inline TCP_CircuitInterface_c::TCP_CircuitInterface_c( 
      void )
{
}

//===========================================================================//
inline TCP_CircuitInterface_c::~TCP_CircuitInterface_c( 
      void )
{
}

#endif
