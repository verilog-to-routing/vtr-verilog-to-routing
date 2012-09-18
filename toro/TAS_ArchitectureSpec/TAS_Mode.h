//===========================================================================//
// Purpose : Declaration and inline definitions for a TAS_Mode class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - SetPhysicalBlockParent
//           - GetPhysicalBlockParent
//           - IsValid
//
//===========================================================================//

#ifndef TAS_MODE_H
#define TAS_MODE_H

#include <stdio.h>

#include <string>
using namespace std;

#include "TAS_Typedefs.h"
#include "TAS_PhysicalBlock.h"
#include "TAS_Interconnect.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TAS_Mode_c
{
public:

   TAS_Mode_c( void );
   TAS_Mode_c( const string& srName );
   TAS_Mode_c( const char* pszName );
   TAS_Mode_c( const TAS_Mode_c& mode );
   ~TAS_Mode_c( void );

   TAS_Mode_c& operator=( const TAS_Mode_c& mode );
   bool operator==( const TAS_Mode_c& mode ) const;
   bool operator!=( const TAS_Mode_c& mode ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0,
               size_t hierLevel = 0,
               TAS_ModeList_t* pmodeList = 0 ) const;

   void PrintXML( void ) const;
   void PrintXML( FILE* pfile, size_t spaceLen ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   void SetPhysicalBlockParent( const TAS_PhysicalBlock_c* pphysicalBlock );

   const TAS_PhysicalBlock_c* GetPhysicalBlockParent( void ) const;

   bool IsValid( void ) const;

public:

   string srName;

   TAS_PhysicalBlockList_t physicalBlockList;
   TAS_InterconnectList_t interconnectList;

   class TAS_ModeSorted_c
   {
   public:

      TAS_PhysicalBlockSortedList_t physicalBlockList;
   } sorted;

private:

   TAS_PhysicalBlock_c* pphysicalBlockParent_;

private:

   enum TAS_DefCapacity_e 
   { 
      TAS_PHYSICAL_BLOCK_LIST_DEF_CAPACITY = 64,
      TAS_INTERCONNECT_LIST_DEF_CAPACITY = 64
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TAS_Mode_c::SetName( 
      const string& srName_ )
{
   this->srName = srName_;
}

//===========================================================================//
inline void TAS_Mode_c::SetName( 
      const char* pszName )
{
   this->srName = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TAS_Mode_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName ));
}

//===========================================================================//
inline const char* TAS_Mode_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName ));
}

//===========================================================================//
inline void TAS_Mode_c::SetPhysicalBlockParent( 
      const TAS_PhysicalBlock_c* pphysicalBlock )
{
   this->pphysicalBlockParent_ = const_cast< TAS_PhysicalBlock_c* >( pphysicalBlock );
}

//===========================================================================//
inline const TAS_PhysicalBlock_c* TAS_Mode_c::GetPhysicalBlockParent( 
      void ) const
{
   return( this->pphysicalBlockParent_ );
}

//===========================================================================//
inline bool TAS_Mode_c::IsValid( 
      void ) const
{
   return( this->srName.length( ) ? true : false );
}

#endif
