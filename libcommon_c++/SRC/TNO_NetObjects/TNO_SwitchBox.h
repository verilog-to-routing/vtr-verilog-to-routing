//===========================================================================//
// Purpose : Declaration and inline definitions for a TNO_SwitchBox class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetInput, GetOutput
//           - SetInput, SetOutput
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

#ifndef TNO_SWITCH_BOX_H
#define TNO_SWITCH_BOX_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_SideIndex.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
class TNO_SwitchBox_c
{
public:

   TNO_SwitchBox_c( void );
   TNO_SwitchBox_c( const string& srName );
   TNO_SwitchBox_c( const char* pszName );
   TNO_SwitchBox_c( const string& srName,
                    const TC_SideIndex_c& input,
                    const TC_SideIndex_c& output );
   TNO_SwitchBox_c( const char* pszName,
                    const TC_SideIndex_c& input,
                    const TC_SideIndex_c& output );
   TNO_SwitchBox_c( const string& srName,
                    TC_SideMode_t inputSide,
                    size_t inputIndex,
                    TC_SideMode_t outputSide,
                    size_t outputIndex );
   TNO_SwitchBox_c( const char* pszName,
                    TC_SideMode_t inputSide,
                    size_t inputIndex,
                    TC_SideMode_t outputSide,
                    size_t outputIndex );
   TNO_SwitchBox_c( const TNO_SwitchBox_c& switchBox );
   ~TNO_SwitchBox_c( void );

   TNO_SwitchBox_c& operator=( const TNO_SwitchBox_c& switchBox );
   bool operator<( const TNO_SwitchBox_c& switchBox ) const;
   bool operator==( const TNO_SwitchBox_c& switchBox ) const;
   bool operator!=( const TNO_SwitchBox_c& switchBox ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrSwitchBox ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   const TC_SideIndex_c& GetInput( void ) const;
   const TC_SideIndex_c& GetOutput( void ) const;

   void SetInput( const TC_SideIndex_c& input );
   void SetOutput( const TC_SideIndex_c& output );

   void Clear( void );

   bool IsValid( void ) const;

private:

   string srName_;
   TC_SideIndex_c input_;
   TC_SideIndex_c output_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
inline void TNO_SwitchBox_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TNO_SwitchBox_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TNO_SwitchBox_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TNO_SwitchBox_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const TC_SideIndex_c& TNO_SwitchBox_c::GetInput( 
      void ) const
{
   return( this->input_ );
}

//===========================================================================//
inline const TC_SideIndex_c& TNO_SwitchBox_c::GetOutput( 
      void ) const
{
   return( this->output_ );
}

//===========================================================================//
inline void TNO_SwitchBox_c::SetInput( 
      const TC_SideIndex_c& input )
{
   this->input_ = input;
}

//===========================================================================//
inline void TNO_SwitchBox_c::SetOutput( 
      const TC_SideIndex_c& output )
{
   this->output_ = output;
}

//===========================================================================//
inline bool TNO_SwitchBox_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
