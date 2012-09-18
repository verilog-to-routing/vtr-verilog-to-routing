//===========================================================================//
// Purpose : Declaration for a master input-processing-output class.
//
//===========================================================================//

#ifndef TM_MASTER_H
#define TM_MASTER_H

#include "TCL_CommandLine.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TM_Master_c
{
public:

   TM_Master_c( void );
   TM_Master_c( const TCL_CommandLine_c& commandLine,
                bool* pok = 0 );
   ~TM_Master_c( void );

   bool Apply( const TCL_CommandLine_c& commandLine );

private:

   void NewHandlerSingletons_( void ) const;
   void DeleteHandlerSingletons_( void ) const;
};

#endif
