//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_TimingDelay class.
//
//           Inline methods include:
//           - IsValid
//
//===========================================================================//

#ifndef TAS_TIMING_DELAY_H
#define TAS_TIMING_DELAY_H

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
class TAS_TimingDelay_c
{
public:

   TAS_TimingDelay_c( void );
   TAS_TimingDelay_c( const TAS_TimingDelay_c& timingDelay );
   ~TAS_TimingDelay_c( void );

   TAS_TimingDelay_c& operator=( const TAS_TimingDelay_c& timingDelay );
   bool operator==( const TAS_TimingDelay_c& timingDelay ) const;
   bool operator!=( const TAS_TimingDelay_c& timingDelay ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   bool IsValid( void ) const;

public:

   TAS_TimingMode_t mode; // Selects delay type: CONSTANT|MATRIX|etc.
   TAS_TimingType_t type; // Selects delay type: MIN|MAX

   double valueMin;
   double valueMax;
   double valueNom;
   TAS_TimingValueMatrix_t valueMatrix;

   string srInputPortName;
   string srOutputPortName;
   string srClockPortName;
   string srPackPatternName;

private:

   enum TAS_DefCapacity_e 
   { 
      TAS_TIMING_VALUE_MATRIX_DEF_CAPACITY = 0
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline bool TAS_TimingDelay_c::IsValid( 
      void ) const
{
   return( this->mode != TAS_TIMING_MODE_UNDEFINED ? true : false );
}

#endif
