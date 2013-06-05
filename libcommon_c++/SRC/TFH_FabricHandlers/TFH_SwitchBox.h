//===========================================================================//
// Purpose : Declaration and inline definitions for the TFH_SwitchBox class.
//
//           Inline methods include:
//           - SetGridPoint, SetSwitchBoxName, SetMapTable
//           - GetGridPoint, GetSwitchBoxName, GetMapTable
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

#ifndef TFH_SWITCH_BOX_H
#define TFH_SWITCH_BOX_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_MapTable.h"

#include "TGO_Point.h"

#include "TFH_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/15/13 jeffr : Original
//===========================================================================//
class TFH_SwitchBox_c
{
public:

   TFH_SwitchBox_c( void );
   TFH_SwitchBox_c( int x, int y );
   TFH_SwitchBox_c( const TGO_Point_c& vpr_gridPoint );
   TFH_SwitchBox_c( const TGO_Point_c& vpr_gridPoint,
                    const string& srSwitchBoxName,
                    const TC_MapTable_c& mapTable );
   TFH_SwitchBox_c( const TGO_Point_c& vpr_gridPoint,
                    const char* pszSwitchBoxName,
                    const TC_MapTable_c& mapTable );
   TFH_SwitchBox_c( const TFH_SwitchBox_c& switchBox );
   ~TFH_SwitchBox_c( void );

   TFH_SwitchBox_c& operator=( const TFH_SwitchBox_c& switchBox );
   bool operator<( const TFH_SwitchBox_c& switchBox ) const;
   bool operator==( const TFH_SwitchBox_c& switchBox ) const;
   bool operator!=( const TFH_SwitchBox_c& switchBox ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetGridPoint( const TGO_Point_c& vpr_gridPoint );
   void SetSwitchBoxName( const string& srSwitchBoxName );
   void SetSwitchBoxName( const char* pszSwitchBoxName );
   void SetMapTable( const TC_MapTable_c& mapTable );

   const TGO_Point_c& GetGridPoint( void ) const;
   const char* GetSwitchBoxName( void ) const;
   const TC_MapTable_c& GetMapTable( void ) const;

   bool IsValid( void ) const;

private:

   TGO_Point_c vpr_gridPoint_;   // VPR's grid point array coordinate

   string srSwitchBoxName_;      // Specifies switch box name
   TC_MapTable_c mapTable_;      // Specifies switch box side-name map table
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 04/15/13 jeffr : Original
//===========================================================================//
inline void TFH_SwitchBox_c::SetGridPoint( 
      const TGO_Point_c& vpr_gridPoint )
{
   this->vpr_gridPoint_ = vpr_gridPoint;
} 

//===========================================================================//
inline void TFH_SwitchBox_c::SetSwitchBoxName( 
      const string& srSwitchBoxName )
{
   this->srSwitchBoxName_ = srSwitchBoxName;
}

//===========================================================================//
inline void TFH_SwitchBox_c::SetSwitchBoxName( 
      const char* pszSwitchBoxName )
{
   this->srSwitchBoxName_ = TIO_PSZ_STR( pszSwitchBoxName );
}

//===========================================================================//
inline void TFH_SwitchBox_c::SetMapTable( 
      const TC_MapTable_c& mapTable )
{
   this->mapTable_ = mapTable;
} 

//===========================================================================//
inline const TGO_Point_c& TFH_SwitchBox_c::GetGridPoint( 
      void ) const
{
   return( this->vpr_gridPoint_ );
}

//===========================================================================//
inline const char* TFH_SwitchBox_c::GetSwitchBoxName( 
      void ) const
{
   return( TIO_SR_STR( this->srSwitchBoxName_ ));
}

//===========================================================================//
inline const TC_MapTable_c& TFH_SwitchBox_c::GetMapTable( 
      void ) const
{
   return( this->mapTable_ );
}

//===========================================================================//
inline bool TFH_SwitchBox_c::IsValid( 
      void ) const
{
   return( this->vpr_gridPoint_.IsValid( ) ? true : false );
}

#endif
