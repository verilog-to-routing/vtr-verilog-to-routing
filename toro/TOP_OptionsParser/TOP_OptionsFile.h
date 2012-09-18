//===========================================================================//
// Purpose : Declaration and inline definitions for a TOP_OptionsFile class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TOP_OPTIONS_FILE_H
#define TOP_OPTIONS_FILE_H

#include <stdio.h>

#include "TOS_OptionsStore.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOP_OptionsFile_c
{
public:

   TOP_OptionsFile_c( FILE* pfile, 
                      const char* pszFileParserName,
                      TOS_OptionsStore_c* poptionsStore );
   ~TOP_OptionsFile_c( void );

   bool SyntaxError( unsigned int lineNum, 
                     const string& srFileName,
                     const char* pszMessageText );

   bool IsValid( void ) const;

private:

   bool ok_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline bool TOP_OptionsFile_c::IsValid( void ) const
{
   return( this->ok_ );
}

#endif
