//===========================================================================//
// Purpose : Declaration and inline definitions for TCBP_CircuitBlifInterface
//           abstract base class.
//
//           Inline methods include:
//           - TCBP_CircuitBlifInterface, ~TCBP_CircuitBlifInterface
//
//===========================================================================//

#ifndef TCBP_CIRCUIT_BLIF_INTERFACE_H
#define TCBP_CIRCUIT_BLIF_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TCBP_CircuitBlifInterface_c
{
public:

   TCBP_CircuitBlifInterface_c( void );
   virtual ~TCBP_CircuitBlifInterface_c( void );

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
// 05/15/12 jeffr : Original
//===========================================================================//
inline TCBP_CircuitBlifInterface_c::TCBP_CircuitBlifInterface_c( 
      void )
{
}

//===========================================================================//
inline TCBP_CircuitBlifInterface_c::~TCBP_CircuitBlifInterface_c( 
      void )
{
}

#endif
