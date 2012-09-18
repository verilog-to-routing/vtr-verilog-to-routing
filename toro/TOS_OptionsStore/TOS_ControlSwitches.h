//===========================================================================//
// Purpose : Declaration and inline definitions for a TOS_ControlSwitches 
//           class.
//
//           Inline methods include:
//           - GetInputOptions
//           - GetOutputOptions
//           - GetMessageOptions
//           - GetExecuteOptions
//
//===========================================================================//

#ifndef TOS_CONTROL_SWITCHES_H
#define TOS_CONTROL_SWITCHES_H

#include <stdio.h>

#include "TOS_InputOptions.h"
#include "TOS_OutputOptions.h"
#include "TOS_MessageOptions.h"
#include "TOS_ExecuteOptions.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TOS_ControlSwitches_c
{
public:

   TOS_ControlSwitches_c( void );
   ~TOS_ControlSwitches_c( void );

   void Print( void ) const;
   void Print( FILE* pfile, size_t spaceLen ) const;

   void Init( void );
   void Apply( void );

   const TOS_InputOptions_c& GetInputOptions( void ) const;
   const TOS_OutputOptions_c& GetOutputOptions( void ) const;
   const TOS_MessageOptions_c& GetMessageOptions( void ) const;
   const TOS_ExecuteOptions_c& GetExecuteOptions( void ) const;

public:

   TOS_InputOptions_c   inputOptions;
   TOS_OutputOptions_c  outputOptions;
   TOS_MessageOptions_c messageOptions;
   TOS_ExecuteOptions_c executeOptions;

   string srDefaultBaseName; // Used to build default input/output file names
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline const TOS_InputOptions_c& TOS_ControlSwitches_c::GetInputOptions(
      void ) const
{
   return( this->inputOptions );
}

//===========================================================================//
inline const TOS_OutputOptions_c& TOS_ControlSwitches_c::GetOutputOptions(
      void ) const
{
   return( this->outputOptions );
}

//===========================================================================//
inline const TOS_MessageOptions_c& TOS_ControlSwitches_c::GetMessageOptions(
      void ) const
{
   return( this->messageOptions );
}

//===========================================================================//
inline const TOS_ExecuteOptions_c& TOS_ControlSwitches_c::GetExecuteOptions(
      void ) const
{
   return( this->executeOptions );
}

#endif
