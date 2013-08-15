//===========================================================================//
// Purpose : Declaration and inline definitions for the TCH_RelativeMacro 
//           class.
//
//           Inline methods include:
//           - GetRelativeNodeList
//           - GetLength
//           - SetRotateEnabled
//           - IsRotateEnabled
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

#ifndef TCH_RELATIVE_MACRO_H
#define TCH_RELATIVE_MACRO_H

#include <cstdio>
using namespace std;

#include "TCH_Typedefs.h"
#include "TCH_RelativeNode.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TCH_RelativeMacro_c
{
public:

   TCH_RelativeMacro_c( void );
   TCH_RelativeMacro_c( const TCH_RelativeMacro_c& relativeMacro );
   ~TCH_RelativeMacro_c( void );

   TCH_RelativeMacro_c& operator=( const TCH_RelativeMacro_c& relativeMacro );
   bool operator==( const TCH_RelativeMacro_c& relativeMacro ) const;
   bool operator!=( const TCH_RelativeMacro_c& relativeMacro ) const;
   TCH_RelativeNode_c* operator[]( size_t relativeNodeIndex ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   const TCH_RelativeNodeList_t& GetRelativeNodeList( void ) const;
   size_t GetLength( void ) const;

   void Add( const char* pszFromBlockName,
             const char* pszToBlockName,
             const TGO_IntDims_t& fromToLink,
             size_t* pfromNodeIndex = 0,
             size_t* ptoNodeIndex = 0 );
   void Add( size_t fromNodeIndex,
             const char* pszToBlockName,
             const TGO_IntDims_t& fromToLink,
             size_t* ptoNodeIndex = 0 );
   void Add( size_t fromNodeIndex,
             size_t toNodeIndex,
             const TGO_IntDims_t& fromToLink );
   void Add( const char* pszFromBlockName = 0,
             size_t* pfromNodeIndex = 0 );

   TCH_RelativeNode_c* Find( size_t relativeNodeIndex ) const;
   TCH_RelativeNode_c* Find( size_t fromNodeIndex,
                             const TGO_IntDims_t& fromToLink ) const;
   TCH_RelativeNode_c* Find( const TCH_RelativeNode_c& fromNode,
                             const TGO_IntDims_t& fromToLink ) const;
   TCH_RelativeNode_c* Find( const TCH_RelativeNode_c& fromNode ) const;

   void Set( size_t relativeNodeIndex,
             const TGO_Point_c& origin,
             TGO_RotateMode_t rotate = TGO_ROTATE_UNDEFINED );
   void Clear( void );
   void Reset( void );

   void ForceRelativeLinks( const TCH_RelativeNode_c& fromNode,
			    const TCH_RelativeNode_c& toNode,
                            const TGO_IntDims_t& fromToLink );
   void ForceRelativeLinks( const TGO_Point_c& fromPoint,
			    const TGO_Point_c& toPoint,
                            const TGO_IntDims_t& fromToLink );

   bool HasRelativeLink( size_t fromNodeIndex,
                         const TGO_IntDims_t& fromToLink ) const;
   bool HasRelativeLink( const TCH_RelativeNode_c& fromNode,
                         const TGO_IntDims_t& fromToLink ) const;

   void SetRotateEnabled( bool rotateEnabled );
   bool IsRotateEnabled( void ) const;

   bool IsValid( void ) const;

private:

   size_t AddNode_( const char* pszBlockName );
   void LinkNodes_( size_t fromNodeIndex,
                    size_t toNodeIndex,
		    const TGO_IntDims_t& fromToLink ) const;
private:

   TCH_RelativeNodeList_t relativeNodeList_;
   bool rotateEnabled_;

private:

   enum TCH_DefCapacity_e 
   { 
      TCH_RELATIVE_NODE_LIST_DEF_CAPACITY = 8
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline const TCH_RelativeNodeList_t& TCH_RelativeMacro_c::GetRelativeNodeList( 
      void ) const
{
   return( this->relativeNodeList_ );
}

//===========================================================================//
inline size_t TCH_RelativeMacro_c::GetLength( 
      void ) const
{
   return( this->relativeNodeList_.GetLength( ) );
}

//===========================================================================//
inline void TCH_RelativeMacro_c::SetRotateEnabled( 
      bool rotateEnabled )
{
   this->rotateEnabled_ = rotateEnabled;
}

//===========================================================================//
inline bool TCH_RelativeMacro_c::IsRotateEnabled( 
      void ) const
{
   return( this->rotateEnabled_ );
}

//===========================================================================//
inline bool TCH_RelativeMacro_c::IsValid( 
      void ) const
{
   return( this->relativeNodeList_.IsValid( ) ? true : false );
}

#endif
