//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_SwitchBox class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - SetDims, SetOrigin
//           - SetMapTable
//           - GetMapTable
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

#ifndef TAS_SWITCH_BOX_H
#define TAS_SWITCH_BOX_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_SideIndex.h"

#include "TGS_Point.h"

#include "TAS_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAS_SwitchBox_c
{
public:

   TAS_SwitchBox_c( void );
   TAS_SwitchBox_c( const string& srName );
   TAS_SwitchBox_c( const char* pszName );
   TAS_SwitchBox_c( const TAS_SwitchBox_c& switchBox );
   ~TAS_SwitchBox_c( void );

   TAS_SwitchBox_c& operator=( const TAS_SwitchBox_c& switchBox );
   bool operator==( const TAS_SwitchBox_c& switchBox ) const;
   bool operator!=( const TAS_SwitchBox_c& switchBox ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   void SetDims( const TGS_FloatDims_t& dims );
   void SetOrigin( const TGS_Point_c& origin );

   void SetMapTable( const TAS_MapTable_t& mapTable );

   const TAS_MapTable_t& GetMapTable( void ) const;

   bool IsValid( void ) const;

public:

   string srName;            // Switch box name

   TAS_SwitchBoxModelType_t model;
                             // Selects Wilton, Subset, or Universal model type
   TAS_SwitchBoxType_t type; // Selects tri-state buffer vs. multiplexor type 

   unsigned int fs;          // Defines 'Fs', the switch box flexibility
                             // (ie. #of wires that connect to switch box)

   class TAS_SwitchBoxTiming_c
   {
   public:

      double res;            // Switch resistance (ohms)
      double capInput;       // Switch input capacitance (farads)
      double capOutput;      // Switch output capacitance (farads)
      double delay;          // Switch intrinsic delay (seconds)
   } timing;

   class TAS_SwitchBoxArea_c
   {
   public:

      double buffer;         // Defines estimated buffer area
                             // (based on min-width transistor units)
                             // Applies to UNIDECTIONAL segments only
      double muxTransistor;  // Defines estimated mux transistor area
                             // (based on min-width transistor units)
                             // Applies to UNIDECTIONAL segments only
   } area;

   class TAS_SwitchBoxPower_c
   {
   public:

      bool   autoSize;       // Switch power buffer type
      double bufferSize;     // Switch power buffer size
   } power;

private:

   TGS_FloatDims_t dims_;    // Defines width/height dimensions (dx,dy)
   TGS_Point_c     origin_;  // Defines reference origin (x,y)

   TAS_MapTable_t mapTable_; // Defines switch box side map table
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TAS_SwitchBox_c::SetName( 
      const string& srName_ )
{
   this->srName = srName_;
}

//===========================================================================//
inline void TAS_SwitchBox_c::SetName( 
      const char* pszName )
{
   this->srName = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TAS_SwitchBox_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName ));
}

//===========================================================================//
inline const char* TAS_SwitchBox_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName ));
}

//===========================================================================//
inline void TAS_SwitchBox_c::SetDims(
      const TGS_FloatDims_t& dims )
{
   this->dims_ = dims;
}

//===========================================================================//
inline void TAS_SwitchBox_c::SetOrigin(
      const TGS_Point_c& origin )
{
   this->origin_ = origin;
}

//===========================================================================//
inline void TAS_SwitchBox_c::SetMapTable(
      const TAS_MapTable_t& mapTable )
{
   this->mapTable_ = mapTable;
}

//===========================================================================//
inline const TAS_MapTable_t& TAS_SwitchBox_c::GetMapTable(
      void ) const      
{
   return( this->mapTable_ );
}

//===========================================================================//
inline bool TAS_SwitchBox_c::IsValid( 
      void ) const
{
   return( this->srName.length( ) ? true : false );
}

#endif
