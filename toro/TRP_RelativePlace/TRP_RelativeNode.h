//===========================================================================//
// Purpose : Declaration and inline definitions for the TRP_RelativeNode 
//           class.
//
//           Inline methods include:
//           - SetBlockName
//           - SetVPR_Type
//           - SetVPR_GridPoint
//           - GetBlockName
//           - GetVPR_Type
//           - GetVPR_GridPoint
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

#ifndef TRP_RELATIVE_NODE_H
#define TRP_RELATIVE_NODE_H

#include <cstdio>
using namespace std;

#include "TC_Typedefs.h"

#include "TGO_Point.h"

#include "TRP_Typedefs.h"

#include "vpr_api.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
class TRP_RelativeNode_c
{
public:

   TRP_RelativeNode_c( void );
   TRP_RelativeNode_c( const string& srBlockName,
                       t_type_descriptor* vpr_type = 0 );
   TRP_RelativeNode_c( const char* pszBlockName,
                       t_type_descriptor* vpr_type = 0 );
   TRP_RelativeNode_c( const TRP_RelativeNode_c& relativeNode );
   ~TRP_RelativeNode_c( void );

   TRP_RelativeNode_c& operator=( const TRP_RelativeNode_c& relativeNode );
   bool operator==( const TRP_RelativeNode_c& relativeNode ) const;
   bool operator!=( const TRP_RelativeNode_c& relativeNode ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetBlockName( const string& srBlockName );
   void SetBlockName( const char* pszBlockName );
   void SetVPR_Type( const t_type_descriptor* vpr_type );
   void SetVPR_Type( t_type_descriptor* vpr_type );
   void SetVPR_GridPoint( const TGO_Point_c& vpr_gridPoint );
   void SetSideIndex( TC_SideMode_t side,
                      size_t relativeNodeIndex );

   const char* GetBlockName( void ) const;
   t_type_descriptor* GetVPR_Type( void ) const;
   const TGO_Point_c& GetVPR_GridPoint( void ) const;
   size_t GetSideIndex( TC_SideMode_t side ) const;

   bool HasSideIndex( TC_SideMode_t side ) const;

   void Reset( void );

   bool IsValid( void ) const;

private:

   string srBlockName_;          // Block asso. with this relative placement node
   t_type_descriptor* vpr_type_; // VPR type asso. with this relative placement node

   TGO_Point_c vpr_gridPoint_;   // VPR grid array coordinates

   class TRP_RelativeNodeSides_c // Neighbor placement node indices
   {
   public:

      size_t left;
      size_t right;
      size_t lower;
      size_t upper;

   } sideIndex_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 01/15/13 jeffr : Original
//===========================================================================//
inline void TRP_RelativeNode_c::SetBlockName( 
      const string& srBlockName )
{
   this->srBlockName_ = srBlockName;
}

//===========================================================================//
inline void TRP_RelativeNode_c::SetBlockName( 
      const char* pszBlockName )
{
   this->srBlockName_ = TIO_PSZ_STR( pszBlockName );
}

//===========================================================================//
inline void TRP_RelativeNode_c::SetVPR_Type( 
      const t_type_descriptor* vpr_type )
{
   this->vpr_type_ = const_cast< t_type_descriptor* >( vpr_type );
} 

//===========================================================================//
inline void TRP_RelativeNode_c::SetVPR_Type( 
      t_type_descriptor* vpr_type )
{
   this->vpr_type_ = vpr_type;
} 

//===========================================================================//
inline void TRP_RelativeNode_c::SetVPR_GridPoint( 
      const TGO_Point_c& vpr_gridPoint )
{
   this->vpr_gridPoint_ = vpr_gridPoint;
} 

//===========================================================================//
inline const char* TRP_RelativeNode_c::GetBlockName( 
      void ) const
{
   return( TIO_SR_STR( this->srBlockName_ ));
}

//===========================================================================//
inline t_type_descriptor* TRP_RelativeNode_c::GetVPR_Type( 
      void ) const
{
   return( this->vpr_type_ );
} 

//===========================================================================//
inline const TGO_Point_c& TRP_RelativeNode_c::GetVPR_GridPoint( 
      void ) const
{
   return( this->vpr_gridPoint_ );
}

//===========================================================================//
inline bool TRP_RelativeNode_c::IsValid( 
      void ) const
{
   return( this->srBlockName_.length( ) ? true : false );
}

#endif
