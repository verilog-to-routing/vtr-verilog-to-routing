//===========================================================================//
// Purpose : Declaration and inline definitions for TFP_FabricInterface
//           abstract base class.
//
//           Inline methods include:
//           - TFP_FabricInterface, ~TFP_FabricInterface
//
//===========================================================================//

#ifndef TFP_FABRIC_INTERFACE_H
#define TFP_FABRIC_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 06/15/12 jeffr : Original
//===========================================================================//
class TFP_FabricInterface_c
{
public:

   TFP_FabricInterface_c( void );
   virtual ~TFP_FabricInterface_c( void );

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
inline TFP_FabricInterface_c::TFP_FabricInterface_c( 
      void )
{
}

//===========================================================================//
inline TFP_FabricInterface_c::~TFP_FabricInterface_c( 
      void )
{
}

#endif
