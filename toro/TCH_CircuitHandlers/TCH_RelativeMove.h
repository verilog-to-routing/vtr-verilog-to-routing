//===========================================================================//
// Purpose : Declaration and inline definitions for the TCH_RelativeMove 
//           class.
//
//           Inline methods include:
//           - SetFromMacroIndex, SetFromPoint, SetFromEmpty
//           - SetToMacroIndex, SetToPoint, SetToEmpty
//           - GetFromMacroIndex, GetFromPoint, GetFromEmpty
//           - GetToMacroIndex, GetToPoint, GetToEmpty
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
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

#ifndef TCH_RELATIVE_MOVE_H
#define TCH_RELATIVE_MOVE_H

#include <cstdio>
using namespace std;

#include "TGO_Point.h"

#include "TCH_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TCH_RelativeMove_c
{
public:

   TCH_RelativeMove_c( void );
   TCH_RelativeMove_c( size_t fromMacroIndex, 
                       const TGO_Point_c& fromPoint, 
                       bool fromEmpty,
                       size_t toMacroIndex, 
                       const TGO_Point_c& toPoint,
                       bool toEmpty );
   TCH_RelativeMove_c( size_t fromMacroIndex, 
                       const TGO_Point_c& fromPoint, 
                       size_t toMacroIndex, 
                       const TGO_Point_c& toPoint );
   TCH_RelativeMove_c( size_t fromMacroIndex, 
                       const TGO_Point_c& fromPoint, 
                       const TGO_Point_c& toPoint );
   TCH_RelativeMove_c( const TGO_Point_c& fromPoint, 
                       size_t toMacoIndex, 
                       const TGO_Point_c& toPoint );
   TCH_RelativeMove_c( const TGO_Point_c& fromPoint, 
                       const TGO_Point_c& toPoint );
   TCH_RelativeMove_c( const TCH_RelativeMove_c& relativeMove );
   ~TCH_RelativeMove_c( void );

   TCH_RelativeMove_c& operator=( const TCH_RelativeMove_c& relativeMove );
   bool operator==( const TCH_RelativeMove_c& relativeMove ) const;
   bool operator!=( const TCH_RelativeMove_c& relativeMove ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetFromMacroIndex( size_t fromMacroIndex );
   void SetFromPoint( const TGO_Point_c& fromPoint );
   void SetFromEmpty( bool fromEmpty );

   void SetToMacroIndex( size_t toMacroIndex );
   void SetToPoint( const TGO_Point_c& toPoint );
   void SetToEmpty( bool toEmpty );

   size_t GetFromMacroIndex( void ) const;
   const TGO_Point_c& GetFromPoint( void ) const;
   bool GetFromEmpty( void ) const;

   size_t GetToMacroIndex( void ) const;
   const TGO_Point_c& GetToPoint( void ) const;
   bool GetToEmpty( void ) const;

   bool IsValid( void ) const;

private:

   size_t      fromMacroIndex_;
   TGO_Point_c fromPoint_;
   bool        fromEmpty_;

   size_t      toMacroIndex_;
   TGO_Point_c toPoint_;
   bool        toEmpty_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline void TCH_RelativeMove_c::SetFromMacroIndex( 
      size_t fromMacroIndex )
{
   this->fromMacroIndex_ = fromMacroIndex;
} 

//===========================================================================//
inline void TCH_RelativeMove_c::SetFromPoint( 
      const TGO_Point_c& fromPoint )
{
   this->fromPoint_ = fromPoint;
} 

//===========================================================================//
inline void TCH_RelativeMove_c::SetFromEmpty( 
      bool fromEmpty )
{
   this->fromEmpty_ = fromEmpty;
} 

//===========================================================================//
inline void TCH_RelativeMove_c::SetToMacroIndex( 
      size_t toMacroIndex )
{
   this->toMacroIndex_ = toMacroIndex;
} 

//===========================================================================//
inline void TCH_RelativeMove_c::SetToPoint( 
      const TGO_Point_c& toPoint )
{
   this->toPoint_ = toPoint;
} 

//===========================================================================//
inline void TCH_RelativeMove_c::SetToEmpty( 
      bool toEmpty )
{
   this->toEmpty_ = toEmpty;
} 

//===========================================================================//
inline size_t TCH_RelativeMove_c::GetFromMacroIndex( 
      void ) const
{
   return( this->fromMacroIndex_ );
}

//===========================================================================//
inline const TGO_Point_c& TCH_RelativeMove_c::GetFromPoint( 
      void ) const
{
   return( this->fromPoint_ );
}

//===========================================================================//
inline bool TCH_RelativeMove_c::GetFromEmpty(
      void ) const
{
   return( this->fromEmpty_ );
}

//===========================================================================//
inline size_t TCH_RelativeMove_c::GetToMacroIndex( 
      void ) const
{
   return( this->toMacroIndex_ );
}

//===========================================================================//
inline const TGO_Point_c& TCH_RelativeMove_c::GetToPoint( 
      void ) const
{
   return( this->toPoint_ );
}

//===========================================================================//
inline bool TCH_RelativeMove_c::GetToEmpty(
      void ) const
{
   return( this->toEmpty_ );
}

//===========================================================================//
inline bool TCH_RelativeMove_c::IsValid( 
      void ) const
{
   return( this->fromPoint_.IsValid( ) && this->toPoint_.IsValid( ) ? 
           true : false );
}

#endif
