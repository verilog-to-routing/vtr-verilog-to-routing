//===========================================================================//
// Purpose : Declaration and inline definitions for the TRP_RelativeBlock 
//           class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - SetBlockName
//           - SetVPR_Index, SetVPR_Type
//           - SetRelativeMacroIndex, SetRelativeNodeIndex
//           - GetBlockName
//           - GetVPR_Index, GetVPR_Type
//           - GetRelativeMacroIndex, GetRelativeNodeIndex
//           - HasRelativeMacroIndex, HasRelativeNodeIndex
//           - IsValid
//
//===========================================================================//

//---------------------------------------------------------------------------//
// Copyright (C) 2013 Jeff Rudolph, Texas Instruments (jrudolph@ti.com)      //
//                                                                           //
// This program is free software; you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by the     //
// Free Software Foundation; version 3 of the License, or any later version. //
//                                                                           //
// This program is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even an implied warranty of MERCHANTABILITY //
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License   //
// for more details.                                                         //
//                                                                           //
// You should have received a copy of the GNU General Public License along   //
// with this program; if not, see <http://www.gnu.org/licenses>.             //
//---------------------------------------------------------------------------//

#ifndef TRP_RELATIVE_BLOCK_H
#define TRP_RELATIVE_BLOCK_H

#include <cstdio>
using namespace std;

#include "TRP_Typedefs.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TRP_RelativeBlock_c
{
public:

   TRP_RelativeBlock_c( void );
   TRP_RelativeBlock_c( const string& srName,
                        int vpr_index = -1,
                        const t_type_descriptor* vpr_type = 0,
                        size_t relativeMacroIndex = TRP_RELATIVE_MACRO_UNDEFINED,
                        size_t relativeNodeIndex = TRP_RELATIVE_NODE_UNDEFINED );
   TRP_RelativeBlock_c( const char* pszName,
                        int vpr_index = -1,
                        const t_type_descriptor* vpr_type = 0,
                        size_t relativeMacroIndex = TRP_RELATIVE_MACRO_UNDEFINED,
                        size_t relativeNodeIndex = TRP_RELATIVE_NODE_UNDEFINED );
   TRP_RelativeBlock_c( const TRP_RelativeBlock_c& relativeBlock );
   ~TRP_RelativeBlock_c( void );

   TRP_RelativeBlock_c& operator=( const TRP_RelativeBlock_c& relativeBlock );
   bool operator<( const TRP_RelativeBlock_c& relativeBlock ) const;
   bool operator==( const TRP_RelativeBlock_c& relativeBlock ) const;
   bool operator!=( const TRP_RelativeBlock_c& relativeBlock ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   void SetBlockName( const string& srName );
   void SetBlockName( const char* pszName );
   void SetVPR_Index( int vpr_index );
   void SetVPR_Type( const t_type_descriptor* vpr_type );
   void SetRelativeMacroIndex( size_t relativeMacroIndex );
   void SetRelativeNodeIndex( size_t relativeNodeIndex );

   const char* GetBlockName( void ) const;
   int GetVPR_Index( void ) const;
   const t_type_descriptor* GetVPR_Type( void ) const;
   size_t GetRelativeMacroIndex( void ) const;
   size_t GetRelativeNodeIndex( void ) const;

   bool HasRelativeMacroIndex( void ) const;
   bool HasRelativeNodeIndex( void ) const;

   void Reset( void );

   bool IsValid( void ) const;

private:

   string             srName_;    // Name asso. with this relative placement block
   int                vpr_index_; // VPR index asso. with this relative placement block
   t_type_descriptor* vpr_type_;  // VPR type asso. with this relative placement block

   size_t relativeMacroIndex_;    // Index to relative macro in relative macro list
   size_t relativeNodeIndex_;     // Index to relative node in a relative node list
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline void TRP_RelativeBlock_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TRP_RelativeBlock_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TRP_RelativeBlock_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TRP_RelativeBlock_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline void TRP_RelativeBlock_c::SetBlockName( 
      const string& srBlockName )
{
   this->srName_ = srBlockName;
}

//===========================================================================//
inline void TRP_RelativeBlock_c::SetBlockName( 
      const char* pszBlockName )
{
   this->srName_ = TIO_PSZ_STR( pszBlockName );
}

//===========================================================================//
inline void TRP_RelativeBlock_c::SetVPR_Index(
      int vpr_index )
{
   this->vpr_index_ = vpr_index;
}

//===========================================================================//
inline void TRP_RelativeBlock_c::SetVPR_Type( 
      const t_type_descriptor* vpr_type )
{
   this->vpr_type_ = const_cast< t_type_descriptor* >( vpr_type );
} 

//===========================================================================//
inline void TRP_RelativeBlock_c::SetRelativeMacroIndex( 
      size_t relativeMacroIndex )
{
   this->relativeMacroIndex_ = relativeMacroIndex;
} 

//===========================================================================//
inline void TRP_RelativeBlock_c::SetRelativeNodeIndex( 
      size_t relativeNodeIndex )
{
   this->relativeNodeIndex_ = relativeNodeIndex;
} 

//===========================================================================//
inline const char* TRP_RelativeBlock_c::GetBlockName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline int TRP_RelativeBlock_c::GetVPR_Index( 
      void ) const
{
   return( this->vpr_index_ );
}

//===========================================================================//
inline const t_type_descriptor* TRP_RelativeBlock_c::GetVPR_Type( 
      void ) const
{
   return( this->vpr_type_ );
} 

//===========================================================================//
inline size_t TRP_RelativeBlock_c::GetRelativeMacroIndex( 
      void ) const
{
   return( this->relativeMacroIndex_ );
}

//===========================================================================//
inline size_t TRP_RelativeBlock_c::GetRelativeNodeIndex( 
      void ) const
{
   return( this->relativeNodeIndex_ );
}

//===========================================================================//
inline bool TRP_RelativeBlock_c::HasRelativeMacroIndex( 
      void ) const
{
   return( this->relativeMacroIndex_ != TRP_RELATIVE_MACRO_UNDEFINED ? true : false );
}

//===========================================================================//
inline bool TRP_RelativeBlock_c::HasRelativeNodeIndex( 
      void ) const
{
   return( this->relativeNodeIndex_ != TRP_RELATIVE_NODE_UNDEFINED ? true : false );
}

//===========================================================================//
inline bool TRP_RelativeBlock_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
