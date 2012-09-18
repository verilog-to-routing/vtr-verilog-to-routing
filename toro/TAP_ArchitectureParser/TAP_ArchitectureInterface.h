//===========================================================================//
// Purpose : Declaration and inline definitions for TAP_ArchitectureInterface
//           abstract base class.
//
//           Inline methods include:
//           - TAP_ArchitectureInterface, ~TAP_ArchitectureInterface
//
//===========================================================================//

#ifndef TAP_ARCHITECTURE_INTERFACE_H
#define TAP_ARCHITECTURE_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAP_ArchitectureInterface_c
{
public:

   TAP_ArchitectureInterface_c( void );
   virtual ~TAP_ArchitectureInterface_c( void );

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
inline TAP_ArchitectureInterface_c::TAP_ArchitectureInterface_c( 
      void )
{
}

//===========================================================================//
inline TAP_ArchitectureInterface_c::~TAP_ArchitectureInterface_c( 
      void )
{
}

#endif
