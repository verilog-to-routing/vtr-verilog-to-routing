//===========================================================================//
// Purpose : Definition for the 'TAP_ArchitectureHandler_c' class.
//
//===========================================================================//

#ifndef TAP_ARCHITECTURE_HANDLER_H
#define TAP_ARCHITECTURE_HANDLER_H

#include "TAS_ArchitectureSpec.h"

#include "TAP_ArchitectureInterface.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAP_ArchitectureHandler_c : public TAP_ArchitectureInterface_c
{
public:

   TAP_ArchitectureHandler_c( TAS_ArchitectureSpec_c* parchitectureSpec );
   ~TAP_ArchitectureHandler_c( void );

   bool Init( TAS_ArchitectureSpec_c* parchitectureSpec );

   bool SyntaxError( unsigned int lineNum, 
                     const string& srFileName,
                     const char* pszMessageText );

   bool IsValid( void ) const;

private:

   TAS_ArchitectureSpec_c* parchitectureSpec_;
};

#endif
