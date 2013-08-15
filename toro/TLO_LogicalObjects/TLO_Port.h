//===========================================================================//
// Purpose : Declaration and inline definitions for a TLO_Port class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - SetType
//           - SetCount, 
//           - SetEquivalent
//           - SetClass
//           - SetCap, SetDelay
//           - SetPower
//           - GetType
//           - GetCount
//           - GetClass
//           - GetCap, GetDelay
//           - GetPower
//           - IsEquivalent
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

#ifndef TLO_PORT_H
#define TLO_PORT_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"

#include "TLO_Power.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
// 07/17/13 jeffr : Added TLO_Power_c member support
//===========================================================================//
class TLO_Port_c
{
public:

   TLO_Port_c( void );
   TLO_Port_c( const string& srName,
               TC_TypeMode_t type = TC_TYPE_UNDEFINED,
               size_t count = 0,
               bool isEquivalent = false );
   TLO_Port_c( const char* pszName,
               TC_TypeMode_t type = TC_TYPE_UNDEFINED,
               size_t count = 0,
               bool isEquivalent = false );
   TLO_Port_c( const TLO_Port_c& port );
   ~TLO_Port_c( void );

   TLO_Port_c& operator=( const TLO_Port_c& port );
   bool operator<( const TLO_Port_c& port ) const;
   bool operator==( const TLO_Port_c& port ) const;
   bool operator!=( const TLO_Port_c& port ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   void SetType( TC_TypeMode_t type );
   void SetCount( size_t count );
   void SetEquivalent( bool isEquivalent );
   void SetClass( const string& srClass );
   void SetClass( const char* pszClass );
   void SetCap( double cap );
   void SetDelay( double delay );
   void SetPower( const TLO_Power_c& power );

   TC_TypeMode_t GetType( void ) const;
   size_t GetCount( void ) const;
   const char* GetClass( void ) const;
   double GetCap( void ) const;
   double GetDelay( void ) const;
   const TLO_Power_c& GetPower( void ) const;

   void Clear( void );
   
   bool IsEquivalent( void ) const;
   bool IsValid( void ) const;

private:

   string        srName_; // Defines port name
   TC_TypeMode_t type_;   // Defines port type
                          // For example: input, output, clock, reset, power

   size_t count_;         // Number of pins associated with this port
   bool   isEquivalent_;  // TRUE => port's pins are logically equivalent

   string srClass_;       // Defines optional port class name

   double cap_;           // Port capacitance (fF)
   double delay_;         // Port delay (ps)

   TLO_Power_c power_;    // Optional port power constants.
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TLO_Port_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TLO_Port_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TLO_Port_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TLO_Port_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline void TLO_Port_c::SetType( 
      TC_TypeMode_t type )
{
   this->type_ = type;
}

//===========================================================================//
inline void TLO_Port_c::SetCount( 
      size_t count )
{
   this->count_ = count;
}

//===========================================================================//
inline void TLO_Port_c::SetEquivalent(
      bool isEquivalent )
{
   this->isEquivalent_ = isEquivalent;
}

//===========================================================================//
inline void TLO_Port_c::SetClass( 
      const string& srClass )
{
   this->srClass_ = srClass;
}

//===========================================================================//
inline void TLO_Port_c::SetClass( 
      const char* pszClass )
{
   this->srClass_ = TIO_PSZ_STR( pszClass );
}

//===========================================================================//
inline void TLO_Port_c::SetCap( 
      double cap )
{
   this->cap_ = cap;
}

//===========================================================================//
inline void TLO_Port_c::SetDelay( 
      double delay )
{
   this->delay_ = delay;
}

//===========================================================================//
inline void TLO_Port_c::SetPower( 
      const TLO_Power_c& power )
{
   this->power_ = power;
}

//===========================================================================//
inline TC_TypeMode_t TLO_Port_c::GetType( 
      void ) const
{
   return( this->type_ );
}

//===========================================================================//
inline size_t TLO_Port_c::GetCount( 
      void ) const
{
   return( this->count_ );
}

//===========================================================================//
inline const char* TLO_Port_c::GetClass( 
      void ) const
{
   return( TIO_SR_STR( this->srClass_ ));
}

//===========================================================================//
inline bool TLO_Port_c::IsEquivalent( 
      void ) const
{
   return( this->isEquivalent_ );
}

//===========================================================================//
inline double TLO_Port_c::GetCap( 
      void ) const
{
   return( this->cap_ );
}

//===========================================================================//
inline double TLO_Port_c::GetDelay( 
      void ) const
{
   return( this->delay_ );
}

//===========================================================================//
inline const TLO_Power_c& TLO_Port_c::GetPower( 
      void ) const
{
   return( this->power_ );
}

//===========================================================================//
inline bool TLO_Port_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
