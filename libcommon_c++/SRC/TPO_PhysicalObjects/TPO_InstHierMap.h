//===========================================================================//
// Purpose : Declaration and inline definitions for a TPO_InstHierMap class.
//
//           Inline methods include:
//           - GetInstName
//           - GetHierNameList
//           - SetInstName
//           - SetHierNameList
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

#ifndef TPO_INST_HIER_MAP_H
#define TPO_INST_HIER_MAP_H

#include <cstdio>
#include <string>
using namespace std;

#include "TC_Typedefs.h"
#include "TC_Name.h"

#include "TPO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//

class TPO_InstHierMap_c
{
public:

   TPO_InstHierMap_c( void );
   TPO_InstHierMap_c( const string& srInstName,
                      const TPO_NameList_t& hierNameList );
   TPO_InstHierMap_c( const char* pszInstName,
                      const TPO_NameList_t& hierNameList );
   TPO_InstHierMap_c( const TPO_InstHierMap_c& instHierMap );
   ~TPO_InstHierMap_c( void );

   TPO_InstHierMap_c& operator=( const TPO_InstHierMap_c& instHierMap );
   bool operator==( const TPO_InstHierMap_c& instHierMap ) const;
   bool operator!=( const TPO_InstHierMap_c& instHierMap ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   const char* GetInstName( void ) const;
   const TPO_NameList_t& GetHierNameList( void ) const;

   void SetInstName( const string& srInstName );
   void SetInstName( const char* pszInstName );
   void SetHierNameList( const TPO_NameList_t& hierNameList );

   bool IsValid( void ) const;

private:

   string srInstName_; // Defines instance name to be packed (per BLIF)
   TPO_NameList_t hierNameList_;
                       // Defines a hierarchical architecture PB name list

private:

   enum TPO_DefCapacity_e 
   { 
      TPO_INST_HIER_NAME_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline const char* TPO_InstHierMap_c::GetInstName( 
      void ) const 
{
   return( TIO_SR_STR( this->srInstName_ ));
}

//===========================================================================//
inline const TPO_NameList_t& TPO_InstHierMap_c::GetHierNameList( 
      void ) const
{
   return( this->hierNameList_ );
}

//===========================================================================//
inline void TPO_InstHierMap_c::SetInstName( 
      const string& srInstName )
{
   this->srInstName_ = srInstName;
}

//===========================================================================//
inline void TPO_InstHierMap_c::SetInstName( 
      const char* pszInstName )
{
   this->srInstName_ = TIO_PSZ_STR( pszInstName );
}

//===========================================================================//
inline void TPO_InstHierMap_c::SetHierNameList(
      const TPO_NameList_t& hierNameList )
{
   this->hierNameList_ = hierNameList;
}

//===========================================================================//
inline bool TPO_InstHierMap_c::IsValid( 
      void ) const
{
   return( this->srInstName_.length( ) ? true : false );
}

#endif
