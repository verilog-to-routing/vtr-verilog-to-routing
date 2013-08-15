//===========================================================================//
// Purpose : Declaration and inline definitions for a TPO_HierInstMap class.
//
//           Inline methods include:
//           - GetName
//           - GetHierName
//           - GetInstName
//           - GetInstIndex
//           - SetHierName
//           - SetInstName
//           - SetInstIndex
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

#ifndef TPO_HIER_INST_MAP_H
#define TPO_HIER_INST_MAP_H

#include <cstdio>
#include <string>
using namespace std;

#include "TPO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//

class TPO_HierInstMap_c
{
public:

   TPO_HierInstMap_c( void );
   TPO_HierInstMap_c( const string& srHierName,
                      const string& srInstName,
                      size_t instIndex );
   TPO_HierInstMap_c( const char* pszHierName,
                      const char* pszInstName,
                      size_t instIndex );
   TPO_HierInstMap_c( const string& srHierName );
   TPO_HierInstMap_c( const char* pszHierName );
   TPO_HierInstMap_c( const TPO_HierInstMap_c& hierInstMap );
   ~TPO_HierInstMap_c( void );

   TPO_HierInstMap_c& operator=( const TPO_HierInstMap_c& hierInstMap );
   bool operator<( const TPO_HierInstMap_c& hierInstMap ) const;
   bool operator==( const TPO_HierInstMap_c& hierInstMap ) const;
   bool operator!=( const TPO_HierInstMap_c& hierInstMap ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   const char* GetName( void ) const;

   const char* GetHierName( void ) const;
   const char* GetInstName( void ) const;
   size_t GetInstIndex( void ) const;

   void SetHierName( const string& srHierName );
   void SetHierName( const char* pszHierName );
   void SetInstName( const string& srInstName );
   void SetInstName( const char* pszInstName );
   void SetInstIndex( size_t instIndex );

   bool IsValid( void ) const;

private:

   string srHierName_; // Defines hierarchical (ie. packed) instance name
   string srInstName_; // Defines asso. (ie. top-level) instance name
   size_t instIndex_;  // Defines asso. (ie. top-level) instance index
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 07/26/13 jeffr : Original
//===========================================================================//
inline const char* TPO_HierInstMap_c::GetName( 
      void ) const 
{
   return( TIO_SR_STR( this->srHierName_ ));
}

//===========================================================================//
inline const char* TPO_HierInstMap_c::GetHierName( 
      void ) const 
{
   return( TIO_SR_STR( this->srHierName_ ));
}

//===========================================================================//
inline const char* TPO_HierInstMap_c::GetInstName( 
      void ) const 
{
   return( TIO_SR_STR( this->srInstName_ ));
}

//===========================================================================//
inline size_t TPO_HierInstMap_c::GetInstIndex( 
      void ) const
{
   return( this->instIndex_ );
}

//===========================================================================//
inline void TPO_HierInstMap_c::SetHierName( 
      const string& srHierName )
{
   this->srHierName_ = srHierName;
}

//===========================================================================//
inline void TPO_HierInstMap_c::SetHierName( 
      const char* pszHierName )
{
   this->srHierName_ = TIO_PSZ_STR( pszHierName );
}

//===========================================================================//
inline void TPO_HierInstMap_c::SetInstName( 
      const string& srInstName )
{
   this->srInstName_ = srInstName;
}

//===========================================================================//
inline void TPO_HierInstMap_c::SetInstName( 
      const char* pszInstName )
{
   this->srInstName_ = TIO_PSZ_STR( pszInstName );
}

//===========================================================================//
inline void TPO_HierInstMap_c::SetInstIndex(
      size_t instIndex )
{
   this->instIndex_ = instIndex;
}

//===========================================================================//
inline bool TPO_HierInstMap_c::IsValid( 
      void ) const
{
   return( this->srHierName_.length( ) ? true : false );
}

#endif
