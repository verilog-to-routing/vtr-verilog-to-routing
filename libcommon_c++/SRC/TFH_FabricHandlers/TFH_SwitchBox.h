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
