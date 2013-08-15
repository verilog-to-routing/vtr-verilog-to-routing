//===========================================================================//
// Purpose : Declaration and inline definitions for a TCL_CommandLine class.
//           This class handles parsing the given program command-line for 
//           required and optional switches.  An optional usage help message 
//           is displayed if necessary.
//
//           Inline methods include:
//           - GetControlSwitches
//           - GetRulesSwitches
//           - GetArgc
//           - GetArgv
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2012-2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com) //
//                                                                           //
// Permission is hereby granted, free of charge, to any person obtaining a   //
// copy of this software and associated documentation files (the "Software"),//
// to deal in the Software without restriction, including without limitation //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,  //
// and/or sell copies of the Software, and to permit persons to whom the     //
// Software is furnished to do so, subject to the following conditions:      //
//                                                                           //
// The above copyright notice and this permission notice shall be included   //
// in all copies or substantial portions of the Software.                    //
//                                                                           //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   //
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                //
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN // 
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,  //
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR     //
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE //
// USE OR OTHER DEALINGS IN THE SOFTWARE.                                    //
//---------------------------------------------------------------------------//

#ifndef TCL_COMMAND_LINE_H
#define TCL_COMMAND_LINE_H

#include <cstdio>
using namespace std;

#include "TOS_ControlSwitches.h"
#include "TOS_RulesSwitches.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TCL_CommandLine_c
{
public:

   TCL_CommandLine_c( void );
   TCL_CommandLine_c( int argc, 
                      char* argv[],
                      bool* pok = 0 );
   ~TCL_CommandLine_c( void );

   bool Parse( int argc, 
               char* argv[] );

   void ExtractArgString( string* psrArgString ) const;

   const TOS_ControlSwitches_c& GetControlSwitches( void ) const;
   const TOS_RulesSwitches_c& GetRulesSwitches( void ) const;

   int GetArgc( void ) const;
   char** GetArgv( void ) const;

   bool IsValid( void ) const;

private:

   bool ParseBool_( const char* pszBool,
                    bool* pbool );
   bool ParseExecuteToolMode_( const char* pszMode,
                               int* pmode );
   bool ParsePlaceCostMode_( const char* pszMode,
                             TOS_PlaceCostMode_t* pmode );
   bool ParseRouteAbstractMode_( const char* pszMode,
                                 TOS_RouteAbstractMode_t* pmode );
   bool ParseRouteResourceMode_( const char* pszMode,
                                 TOS_RouteResourceMode_t* pmode );
   bool ParseRouteCostMode_( const char* pszMode,
                             TOS_RouteCostMode_t* pmode );

private:

   TOS_ControlSwitches_c controlSwitches_; // Used to hold command line arguments
   TOS_RulesSwitches_c   rulesSwitches_;   // "

   int    argc_;  // Cached command line arguments
   char** pargv_; // "
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
inline const TOS_ControlSwitches_c& TCL_CommandLine_c::GetControlSwitches( 
      void ) const
{
   return( this->controlSwitches_ );
}

//===========================================================================//
inline const TOS_RulesSwitches_c& TCL_CommandLine_c::GetRulesSwitches( 
      void ) const
{
   return( this->rulesSwitches_ );
}

//===========================================================================//
inline int TCL_CommandLine_c::GetArgc( 
      void ) const
{
   return( this->argc_ );
}

//===========================================================================//
inline char** TCL_CommandLine_c::GetArgv( 
      void ) const
{
   return( this->pargv_ );
}

//===========================================================================//
inline bool TCL_CommandLine_c::IsValid( 
      void ) const
{
   return( this->controlSwitches_.inputOptions.optionsFileNameList.IsValid( ));
}

#endif
