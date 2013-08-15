//===========================================================================//
// Purpose : Declaration and inline definitions for a TC_SideName class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetSide, GetDx, GetDy, GetName
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

#ifndef TC_SIDE_NAME_H
#define TC_SIDE_NAME_H

#include <cstdio>
#include <climits>
#include <string>
using namespace std;

#include "TIO_Typedefs.h"

#include "TC_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TC_SideName_c
{
public:

   TC_SideName_c( void );
   TC_SideName_c( TC_SideMode_t side,
                  int dx, int dy,
                  const string& srName );
   TC_SideName_c( TC_SideMode_t side,
                  int dx, int dy,
                  const char* pszName );
   TC_SideName_c( const TC_SideName_c& sideName );
   ~TC_SideName_c( void );

   TC_SideName_c& operator=( const TC_SideName_c& sideName );
   bool operator<( const TC_SideName_c& sideName ) const;
   bool operator==( const TC_SideName_c& sideName ) const;
   bool operator!=( const TC_SideName_c& sideName ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void ExtractString( string* psrSideName ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   TC_SideMode_t GetSide( void ) const;
   int GetDx( void ) const;
   int GetDy( void ) const;

   void Set( TC_SideMode_t side,
             int dx, int dy,
             const string& srName );
   void Set( TC_SideMode_t side,
             int dx, int dy,
             const char* pszName );
   void Clear( void );

   bool IsValid( void ) const;

private:

   TC_SideMode_t side_;
   int           dx_;
   int           dy_;
   string        srName_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TC_SideName_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TC_SideName_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TC_SideName_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TC_SideName_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline TC_SideMode_t TC_SideName_c::GetSide( 
      void ) const
{
   return( this->side_ );
}

//===========================================================================//
inline int TC_SideName_c::GetDx( 
      void ) const
{
   return( this->dx_ );
}

//===========================================================================//
inline int TC_SideName_c::GetDy( 
      void ) const
{
   return( this->dy_ );
}

//===========================================================================//
inline bool TC_SideName_c::IsValid( 
      void ) const
{
   return(( this->side_ != TC_SIDE_UNDEFINED ) || 
          ( this->dx_ != INT_MAX && this->dy_ != INT_MAX ) ? 
          true : false );
}

#endif
