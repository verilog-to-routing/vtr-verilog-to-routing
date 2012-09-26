//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_GridAssign class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TAS_GRID_ASSIGN_H
#define TAS_GRID_ASSIGN_H

#include <cstdio>
#include <string>
using namespace std;

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAS_GridAssign_c
{
public:

   TAS_GridAssign_c( void );
   TAS_GridAssign_c( const TAS_GridAssign_c& gridAssign );
   ~TAS_GridAssign_c( void );

   TAS_GridAssign_c& operator=( const TAS_GridAssign_c& gridAssign );
   bool operator==( const TAS_GridAssign_c& gridAssign ) const;
   bool operator!=( const TAS_GridAssign_c& gridAssign ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   TAS_GridAssignDistrMode_t distr;
                            // Sets assignment distribution: SINGLE|MULTIPLE|FILL
   TAS_GridAssignOrientMode_t orient;
                            // Sets assignment orientation: COLUMN|ROW

   unsigned int priority;   // Defines distribution assignment priority 
                            // (to handle distribution assignment collisions)
   
   double singlePercent;        // Defines SINGLE percent (postion) distribution 

   unsigned int multipleStart;  // Defines MULTIPLE start for multiple distributions
   unsigned int multipleRepeat; // Defines MULTIPLE repeat interval
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TAS_GridAssign_c::IsValid( 
      void ) const
{
   return( this->distr != TAS_GRID_ASSIGN_DISTR_UNDEFINED ? true : false );
}

#endif
