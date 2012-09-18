//===========================================================================//
// Purpose : Declaration and inline definitions for a TPO_HierMap class.
//
//           Inline methods include:
//           - GetInstName
//           - GetHierNameList
//           - SetInstName
//           - SetHierNameList
//           - IsValid
//
//===========================================================================//

#ifndef TPO_HIER_MAP_H
#define TPO_HIER_MAP_H

#include <stdio.h>

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

class TPO_HierMap_c
{
public:

   TPO_HierMap_c( void );
   TPO_HierMap_c( const string& srInsName,
                  const TPO_HierNameList_t& hierNameList );
   TPO_HierMap_c( const char* pszInsName,
                  const TPO_HierNameList_t& hierNameList );
   TPO_HierMap_c( const TPO_HierMap_c& hierMap );
   ~TPO_HierMap_c( void );

   TPO_HierMap_c& operator=( const TPO_HierMap_c& hierMap );
   bool operator==( const TPO_HierMap_c& hierMap ) const;
   bool operator!=( const TPO_HierMap_c& hierMap ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   const char* GetInstName( void ) const;
   const TPO_HierNameList_t& GetHierNameList( void ) const;

   void SetInstName( const string& srInstName );
   void SetInstName( const char* pszInstName );
   void SetHierNameList( const TPO_HierNameList_t& hierNameList );

   bool IsValid( void ) const;

private:

   string srInstName_; // Defines instance name to be packed (per BLIF)
   TPO_HierNameList_t hierNameList_;
                       // Defines a hierarchical architecture PB name list

private:

   enum TPO_DefCapacity_e 
   { 
      TPO_HIER_NAME_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline const char* TPO_HierMap_c::GetInstName( 
      void ) const 
{
   return( TIO_SR_STR( this->srInstName_ ));
}

//===========================================================================//
inline const TPO_HierNameList_t& TPO_HierMap_c::GetHierNameList( 
      void ) const
{
   return( this->hierNameList_ );
}

//===========================================================================//
inline void TPO_HierMap_c::SetInstName( 
      const string& srInstName )
{
   this->srInstName_ = srInstName;
}

//===========================================================================//
inline void TPO_HierMap_c::SetInstName( 
      const char* pszInstName )
{
   this->srInstName_ = TIO_PSZ_STR( pszInstName );
}

//===========================================================================//
inline void TPO_HierMap_c::SetHierNameList(
      const TPO_HierNameList_t& hierNameList )
{
   this->hierNameList_ = hierNameList;
}

//===========================================================================//
inline bool TPO_HierMap_c::IsValid( 
      void ) const
{
   return( this->srInstName_.length( ) ? true : false );
}

#endif
