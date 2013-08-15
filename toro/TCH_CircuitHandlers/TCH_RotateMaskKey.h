//===========================================================================//
// Purpose : Declaration and inline definitions for the TCH_RotateMaskKey 
//           class.
//
//           Inline methods include:
//           - SetOriginPoint, SetNodeIndex
//           - GetOriginPoint, GetNodeIndex
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

#ifndef TCH_ROTATE_MASK_KEY_H
#define TCH_ROTATE_MASK_KEY_H

#include <cstdio>
using namespace std;

#include "TGO_Point.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TCH_RotateMaskKey_c
{
public:

   TCH_RotateMaskKey_c( void );
   TCH_RotateMaskKey_c( const TGO_Point_c& originPoint, 
                        size_t nodeIndex ); 
   TCH_RotateMaskKey_c( int x, int y, int z,
                        size_t nodeIndex ); 
   TCH_RotateMaskKey_c( int x, int y,
                        size_t nodeIndex ); 
   TCH_RotateMaskKey_c( const TCH_RotateMaskKey_c& rotateMaskKey );
   ~TCH_RotateMaskKey_c( void );

   TCH_RotateMaskKey_c& operator=( const TCH_RotateMaskKey_c& rotateMaskKey );
   bool operator<( const TCH_RotateMaskKey_c& rotateMaskKey ) const;
   bool operator==( const TCH_RotateMaskKey_c& rotateMaskKey ) const;
   bool operator!=( const TCH_RotateMaskKey_c& rotateMaskKey ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetOriginPoint( const TGO_Point_c& originPoint );
   void SetNodeIndex( size_t nodeIndex );

   const TGO_Point_c& GetOriginPoint( void ) const;
   size_t GetNodeIndex( void ) const;

   bool IsValid( void ) const;

private:

   TGO_Point_c originPoint_;
   size_t      nodeIndex_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline void TCH_RotateMaskKey_c::SetOriginPoint( 
      const TGO_Point_c& originPoint )
{
   this->originPoint_ = originPoint;
} 

//===========================================================================//
inline void TCH_RotateMaskKey_c::SetNodeIndex( 
      size_t nodeIndex )
{
   this->nodeIndex_ = nodeIndex;
} 

//===========================================================================//
inline const TGO_Point_c& TCH_RotateMaskKey_c::GetOriginPoint( 
      void ) const
{
   return( this->originPoint_ );
}

//===========================================================================//
inline size_t TCH_RotateMaskKey_c::GetNodeIndex( 
      void ) const
{
   return( this->nodeIndex_ );
}

//===========================================================================//
inline bool TCH_RotateMaskKey_c::IsValid( 
      void ) const
{
   return( this->originPoint_.IsValid( ) && this->nodeIndex_ != SIZE_MAX ? true : false );
}

#endif
