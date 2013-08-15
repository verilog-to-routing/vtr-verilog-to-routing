//===========================================================================//
// Purpose : Declaration and inline definitions for a TNO_Node class.
//
//           Inline methods include:
//           - GetType
//           - GetInstPin, GetSegment, GetSwitchBox
//           - IsInstPin, IsSegment, IsSwitchBox
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

#ifndef TNO_NODE_H
#define TNO_NODE_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_NameIndex.h"

#include "TNO_Typedefs.h"
#include "TNO_InstPin.h"
#include "TNO_Segment.h"
#include "TNO_SwitchBox.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
class TNO_Node_c
{
public:

   TNO_Node_c( void );
   TNO_Node_c( const TNO_InstPin_c& instPin );
   TNO_Node_c( const TNO_Segment_c& segment );
   TNO_Node_c( const TNO_SwitchBox_c& switchBox );
   TNO_Node_c( const TNO_Node_c& node );
   ~TNO_Node_c( void );

   TNO_Node_c& operator=( const TNO_Node_c& node );
   bool operator==( const TNO_Node_c& node ) const;
   bool operator!=( const TNO_Node_c& node ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0, 
               bool verbose = true ) const;

   TNO_NodeType_t GetType( void ) const;
   const TNO_InstPin_c& GetInstPin( void ) const;
   const TNO_Segment_c& GetSegment( void ) const;
   const TNO_SwitchBox_c& GetSwitchBox( void ) const;

   void Set( const TNO_InstPin_c& instPin );
   void Set( const TNO_Segment_c& segment );
   void Set( const TNO_SwitchBox_c& switchBox );

   void Clear( void );

   bool IsInstPin( void ) const;
   bool IsSegment( void ) const;
   bool IsSwitchBox( void ) const;

   bool IsValid( void ) const;

private:

   TNO_NodeType_t  type_;      // Defines route node type ;
                               // (ie. INST_PIN, SEGMENT, or SWITCH_BOX)
   TNO_InstPin_c   instPin_;
   TNO_Segment_c   segment_;
   TNO_SwitchBox_c switchBox_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
inline TNO_NodeType_t TNO_Node_c::GetType( 
      void ) const
{
   return( this->type_ );
}

//===========================================================================//
inline const TNO_InstPin_c& TNO_Node_c::GetInstPin( 
      void ) const
{
   return( this->instPin_ );
}

//===========================================================================//
inline const TNO_Segment_c& TNO_Node_c::GetSegment( 
      void ) const
{
   return( this->segment_ );
}

//===========================================================================//
inline const TNO_SwitchBox_c& TNO_Node_c::GetSwitchBox( 
      void ) const
{
   return( this->switchBox_ );
}

//===========================================================================//
inline bool TNO_Node_c::IsInstPin( 
      void ) const
{
   return( this->type_ == TNO_NODE_INST_PIN ? true : false );
}

//===========================================================================//
inline bool TNO_Node_c::IsSegment( 
      void ) const
{
   return( this->type_ == TNO_NODE_SEGMENT ? true : false );
}

//===========================================================================//
inline bool TNO_Node_c::IsSwitchBox( 
      void ) const
{
   return( this->type_ == TNO_NODE_SWITCH_BOX ? true : false );
}

//===========================================================================//
inline bool TNO_Node_c::IsValid( 
      void ) const
{
   return( this->type_ != TNO_NODE_UNDEFINED ? true : false );
}

#endif
