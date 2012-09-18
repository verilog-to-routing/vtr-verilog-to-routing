//===========================================================================//
// Purpose : Declaration and inline definitions for TOP_OptionsInterface
//           abstract base class.
//
//           Inline methods include:
//           - TOP_OptionsInterface, ~TOP_OptionsInterface
//
//===========================================================================//

#ifndef TOP_OPTIONS_INTERFACE_H
#define TOP_OPTIONS_INTERFACE_H

#include <string>
using namespace std;

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOP_OptionsInterface_c
{
public:

   TOP_OptionsInterface_c( void );
   virtual ~TOP_OptionsInterface_c( void );

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
// 05/01/12 jeffr : Original
//===========================================================================//
inline TOP_OptionsInterface_c::TOP_OptionsInterface_c( 
      void )
{
}

//===========================================================================//
inline TOP_OptionsInterface_c::~TOP_OptionsInterface_c( 
      void )
{
}

#endif
