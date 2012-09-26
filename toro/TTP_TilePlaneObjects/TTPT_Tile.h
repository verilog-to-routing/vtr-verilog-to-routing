//===========================================================================//
// Purpose : Template version for a TTPT_Tile tile object class.
//
//           Inline methods include:
//           - SetMode, SetRegion
//           - GetMode, GetRegion, GetCount
//           - SetStitchLowerLeft, SetStitchLeftLower
//           - SetStitchRightUpper, SetStitchUpperRight
//           - GetStitchLowerLeft, GetStitchLeftLower
//           - GetStitchRightUpper, GetStitchUpperRight
//           - FindOrient, FindArea, FindDistance
//           - IsClear, IsSolid
//           - IsValid
//
//           Public methods include:
//           - TTPT_Tile_c, ~TTPT_Tile_c
//           - operator=
//           - operator==, operator!=
//           - Print
//           - SetData, AddData, ReplaceData
//           - GetData, FindData, DeleteData
//           - HasData
//           - MakeClear, MakeSolid
//           - FindLessThan, FindGreaterThan
//           - IsLessThan, IsGreaterThan
//           - IsEqualData
//           - IsIntersecting
//           - IsWithin
//
//           Private methods include:
//           - NewAllocData_
//           - DeleteAllocData_
//
//===========================================================================//

#ifndef TTPT_TILE_H
#define TTPT_TILE_H

#include <cstdio>
using namespace std;

#include "TC_Typedefs.h"

#include "TIO_PrintHandler.h"

#include "TGS_Typedefs.h"
#include "TGS_Region.h"
#include "TGS_Point.h"
#include "TGS_Line.h"

#include "TTP_Typedefs.h"
#include "TTP_StringUtils.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > class TTPT_Tile_c
{
public:

   TTPT_Tile_c( void );
   TTPT_Tile_c( TTP_TileMode_t mode,
                const TGS_Region_c& region );
   TTPT_Tile_c( TTP_TileMode_t mode,
                const TGS_Region_c& region,
                const T& data );
   TTPT_Tile_c( TTP_TileMode_t mode,
                const TGS_Region_c& region,
                unsigned int count,
                const T* pdata );
   TTPT_Tile_c( const TTPT_Tile_c< T >& tile );
   ~TTPT_Tile_c( void );

   TTPT_Tile_c< T >& operator=( const TTPT_Tile_c< T >& tile );
   bool operator==( const TTPT_Tile_c< T >& tile ) const;
   bool operator!=( const TTPT_Tile_c< T >& tile ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const; 

   void SetMode( TTP_TileMode_t mode );
   void SetMode( unsigned int mode );
   void SetRegion( const TGS_Region_c& region );
   bool SetData( const T& data );

   bool AddData( const T& data );
   bool AddData( unsigned int count, 
                 const T* pdata );
   bool ReplaceData( unsigned int count,
                     const T* pdata );

   TTP_TileMode_t GetMode( void ) const;
   const TGS_Region_c& GetRegion( void ) const;
   unsigned int GetCount( void ) const;

   TGS_OrientMode_t FindOrient( void ) const;
   double FindArea( void ) const;
   double FindDistance( const TGS_Region_c& region ) const;
   double FindDistance( const TGS_Point_c& point ) const;

   T* GetData( void ) const;
   T* GetData( void );

   T* FindData( unsigned int i ) const;
   T* FindData( unsigned int i );
   T* FindData( const T& data ) const;
   T* FindData( const T& data );

   bool DeleteData( const T& data );

   bool HasData( void ) const;

   void SetStitchLowerLeft( TTPT_Tile_c< T >* ptileLowerLeft );
   void SetStitchLeftLower( TTPT_Tile_c< T >* ptileLeftLower );
   void SetStitchRightUpper( TTPT_Tile_c< T >* ptileRightUpper );
   void SetStitchUpperRight( TTPT_Tile_c< T >* ptileUpperRight );

   TTPT_Tile_c< T >* GetStitchLowerLeft( void ) const;
   TTPT_Tile_c< T >* GetStitchLeftLower( void ) const;
   TTPT_Tile_c< T >* GetStitchRightUpper( void ) const;
   TTPT_Tile_c< T >* GetStitchUpperRight( void ) const;

   TTPT_Tile_c< T >* GetStitchLowerLeft( void );
   TTPT_Tile_c< T >* GetStitchLeftLower( void );
   TTPT_Tile_c< T >* GetStitchRightUpper( void );
   TTPT_Tile_c< T >* GetStitchUpperRight( void );

   void MakeClear( void );
   bool MakeSolid( const T& data );
   bool MakeSolid( unsigned int count,
                   const T* pdata );

   bool IsClear( void ) const;
   bool IsSolid( void ) const;

   TTPT_Tile_c< T >* FindLessThan( TTPT_Tile_c< T >* ptile,
                                   TGS_OrientMode_t orient );
   TTPT_Tile_c< T >* FindGreaterThan( TTPT_Tile_c< T >* ptile,
                                      TGS_OrientMode_t orient );

   bool IsLessThan( const TTPT_Tile_c< T >& tile,
                    TGS_OrientMode_t orient ) const;
   bool IsLessThan( const TGS_Region_c& region,
                    TGS_OrientMode_t orient ) const;
   bool IsLessThan( const TGS_Point_c& point,
                    TGS_OrientMode_t orient ) const;
   bool IsGreaterThan( const TTPT_Tile_c< T >& tile,
                       TGS_OrientMode_t orient ) const;
   bool IsGreaterThan( const TGS_Region_c& region,
                       TGS_OrientMode_t orient ) const;
   bool IsGreaterThan( const TGS_Point_c& point,
                       TGS_OrientMode_t orient ) const;

   bool IsEqualData( const TTPT_Tile_c< T >& tile ) const;
   bool IsEqualData( const T& data ) const;
   bool IsEqualData( unsigned int count,
                     const T* pdata ) const;

   bool IsIntersecting( const TGS_Region_c& region ) const;
   bool IsIntersecting( const TGS_Line_c& line ) const;

   bool IsWithin( const TGS_Region_c& region ) const;
   bool IsWithin( const TGS_Point_c& point ) const;
   bool IsWithin( const TGS_Point_c& point, 
                  TGS_OrientMode_t orient ) const;

   bool IsValid( void ) const;

private:

   bool NewAllocData_( const T& data );
   void DeleteAllocData_( void );

private:

   TGS_Region_c	region_;       // Define region or area asso. with this tile

   unsigned int mode_ : 2;     // Define tile mode (ie. "clear" or "solid")
   unsigned int count_ : 16;   // Define user-defined data count
   T* pdata_;                  // Ptr to user-defined data asso. with this tile

   void* ptileLowerLeft_;      // For corner stitch pointers such that:
   void* ptileLeftLower_;      //
   void* ptileRightUpper_;     //                       right_upper
   void* ptileUpperRight_;     //                       |
                               //               +-------*+
                               //               |        *-upper_right
                               //    lower_left-*        |
                               //               +*-------+
                               //                |
                               //                left_lower
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > inline void TTPT_Tile_c< T >::SetMode( 
      TTP_TileMode_t mode )
{
   this->mode_ = static_cast< unsigned int >( mode );
}

//===========================================================================//
template< class T > inline void TTPT_Tile_c< T >::SetMode( 
      unsigned int mode )
{
   this->mode_ = mode;
}

//===========================================================================//
template< class T > inline void TTPT_Tile_c< T >::SetRegion( 
      const TGS_Region_c& region )
{
   this->region_ = region;
}

//===========================================================================//
template< class T > inline TTP_TileMode_t TTPT_Tile_c< T >::GetMode(
      void ) const
{
   return( static_cast< TTP_TileMode_t >( this->mode_ ));
}

//===========================================================================//
template< class T > inline const TGS_Region_c& TTPT_Tile_c< T >::GetRegion(
      void ) const
{
   return( this->region_ );
}

//===========================================================================//
template< class T > inline unsigned int TTPT_Tile_c< T >::GetCount(
      void ) const
{
   return( this->count_ );
}

//===========================================================================//
template< class T > inline void TTPT_Tile_c< T >::SetStitchLowerLeft( 
      TTPT_Tile_c< T >* ptileLowerLeft )
{
   this->ptileLowerLeft_ = static_cast< void* >( ptileLowerLeft );
}

//===========================================================================//
template< class T > inline void TTPT_Tile_c< T >::SetStitchLeftLower( 
      TTPT_Tile_c< T >* ptileLeftLower )
{
   this->ptileLeftLower_ = static_cast< void* >( ptileLeftLower );
}

//===========================================================================//
template< class T > inline void TTPT_Tile_c< T >::SetStitchRightUpper( 
      TTPT_Tile_c< T >* ptileRightUpper )
{  
   this->ptileRightUpper_ = static_cast< void* >( ptileRightUpper );
}

//===========================================================================//
template< class T > inline void TTPT_Tile_c< T >::SetStitchUpperRight( 
      TTPT_Tile_c< T >* ptileUpperRight )
{
   this->ptileUpperRight_ = static_cast< void* >( ptileUpperRight );
}

//===========================================================================//
template< class T > inline TTPT_Tile_c< T >* TTPT_Tile_c< T >::GetStitchLowerLeft(
      void ) const
{
   return( static_cast< TTPT_Tile_c< T >* >( this->ptileLowerLeft_ ));
}

//===========================================================================//
template< class T > inline TTPT_Tile_c< T >* TTPT_Tile_c< T >::GetStitchLeftLower(
      void ) const
{
   return( static_cast< TTPT_Tile_c< T >* >( this->ptileLeftLower_ ));
}

//===========================================================================//
template< class T > inline TTPT_Tile_c< T >* TTPT_Tile_c< T >::GetStitchRightUpper(
      void ) const
{
   return( static_cast< TTPT_Tile_c< T >* >( this->ptileRightUpper_ ));
}

//===========================================================================//
template< class T > inline TTPT_Tile_c< T >* TTPT_Tile_c< T >::GetStitchUpperRight(
      void ) const
{
   return( static_cast< TTPT_Tile_c< T >* >( this->ptileUpperRight_ ));
}

//===========================================================================//
template< class T > inline TTPT_Tile_c< T >* TTPT_Tile_c< T >::GetStitchLowerLeft(
      void )
{
   return( static_cast< TTPT_Tile_c< T >* >( this->ptileLowerLeft_ ));
}

//===========================================================================//
template< class T > inline TTPT_Tile_c< T >* TTPT_Tile_c< T >::GetStitchLeftLower(
      void )
{
   return( static_cast< TTPT_Tile_c< T >* >( this->ptileLeftLower_ ));
}

//===========================================================================//
template< class T > inline TTPT_Tile_c< T >* TTPT_Tile_c< T >::GetStitchRightUpper(
      void )
{
   return( static_cast< TTPT_Tile_c< T >* >( this->ptileRightUpper_ ));
}

//===========================================================================//
template< class T > inline TTPT_Tile_c< T >* TTPT_Tile_c< T >::GetStitchUpperRight(
      void )
{
   return( static_cast< TTPT_Tile_c< T >* >( this->ptileUpperRight_ ));
}

//===========================================================================//
template< class T > inline TGS_OrientMode_t TTPT_Tile_c< T >::FindOrient( 
      void ) const
{
   return( this->GetRegion( ).FindOrient( ));
}

//===========================================================================//
template< class T > inline double TTPT_Tile_c< T >::FindArea( 
      void ) const
{
   return( this->GetRegion( ).FindArea( ));
}

//===========================================================================//
template< class T > inline double TTPT_Tile_c< T >::FindDistance( 
      const TGS_Region_c& region ) const
{
   return( this->GetRegion( ).FindDistance( region ));
}

//===========================================================================//
template< class T > inline double TTPT_Tile_c< T >::FindDistance( 
      const TGS_Point_c& point ) const
{
   return( this->GetRegion( ).FindDistance( point ));
}

//===========================================================================//
template< class T > inline bool TTPT_Tile_c< T >::IsClear(
      void ) const
{
   return( this->GetMode( ) == TTP_TILE_CLEAR ? true : false );
}

//===========================================================================//
template< class T > inline bool TTPT_Tile_c< T >::IsSolid(
      void ) const
{
   return( this->GetMode( ) == TTP_TILE_SOLID ? true : false );
}

//===========================================================================//
template< class T > inline bool TTPT_Tile_c< T >::IsValid(
      void ) const
{
   return(( this->GetMode( ) != TTP_TILE_UNDEFINED ) &&
          ( this->region_.IsValid( )) ?
          true : false );
}

//===========================================================================//
// Method         : TTPT_Tile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >::TTPT_Tile_c( 
      void )
      :
      mode_( TTP_TILE_UNDEFINED ),
      count_( 0 ),
      pdata_( 0 ),
      ptileLowerLeft_( 0 ),
      ptileLeftLower_( 0 ),
      ptileRightUpper_( 0 ),
      ptileUpperRight_( 0 )
{
}

//===========================================================================//
template< class T > TTPT_Tile_c< T >::TTPT_Tile_c( 
            TTP_TileMode_t mode,
      const TGS_Region_c&  region )
      :
      mode_( TTP_TILE_UNDEFINED ),
      count_( 0 ),
      pdata_( 0 ),
      ptileLowerLeft_( 0 ),
      ptileLeftLower_( 0 ),
      ptileRightUpper_( 0 ),
      ptileUpperRight_( 0 )
{
   this->SetMode( mode );
   this->SetRegion( region );
}

//===========================================================================//
template< class T > TTPT_Tile_c< T >::TTPT_Tile_c( 
            TTP_TileMode_t mode,
      const TGS_Region_c&  region,
      const T&             data )
      :
      mode_( TTP_TILE_UNDEFINED ),
      count_( 0 ),
      pdata_( 0 ),
      ptileLowerLeft_( 0 ),
      ptileLeftLower_( 0 ),
      ptileRightUpper_( 0 ),
      ptileUpperRight_( 0 )
{
   this->SetMode( mode );
   this->SetRegion( region );
   this->SetData( data );
}

//===========================================================================//
template< class T > TTPT_Tile_c< T >::TTPT_Tile_c( 
            TTP_TileMode_t mode,
      const TGS_Region_c&  region,
            unsigned int   count,
      const T*             pdata )
      :
      mode_( TTP_TILE_UNDEFINED ),
      count_( 0 ),
      pdata_( 0 ),
      ptileLowerLeft_( 0 ),
      ptileLeftLower_( 0 ),
      ptileRightUpper_( 0 ),
      ptileUpperRight_( 0 )
{
   this->SetMode( mode );
   this->SetRegion( region );
   this->AddData( count, pdata );
}

//===========================================================================//
template< class T > TTPT_Tile_c< T >::TTPT_Tile_c( 
      const TTPT_Tile_c< T >& tile )
      :
      mode_( TTP_TILE_UNDEFINED ),
      count_( 0 ),
      pdata_( 0 ),
      ptileLowerLeft_( tile.ptileLowerLeft_ ),
      ptileLeftLower_( tile.ptileLeftLower_ ),
      ptileRightUpper_( tile.ptileRightUpper_ ),
      ptileUpperRight_( tile.ptileUpperRight_ )
{
   this->SetMode( tile.mode_ );
   this->SetRegion( tile.region_ );
   this->ReplaceData( tile.count_, tile.pdata_ );
}

//===========================================================================//
// Method         : ~TTPT_Tile_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >::~TTPT_Tile_c( 
      void )
{
   this->DeleteAllocData_( );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >& TTPT_Tile_c< T >::operator=( 
      const TTPT_Tile_c< T >& tile )
{
   if( &tile != this )
   {
      this->SetMode( tile.mode_ );
      this->SetRegion( tile.region_ );

      this->ReplaceData( tile.count_, tile.pdata_ );

      this->ptileLowerLeft_ = tile.ptileLowerLeft_;
      this->ptileLeftLower_ = tile.ptileLeftLower_;
      this->ptileRightUpper_ = tile.ptileRightUpper_;
      this->ptileUpperRight_ = tile.ptileUpperRight_;
   }
   return( *this );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::operator==( 
      const TTPT_Tile_c< T >& tile ) const
{
   bool isEqual = false;

   if(( this->mode_ == tile.mode_ ) &&
      ( this->region_ == tile.region_ ))
   {  
      isEqual = this->IsEqualData( tile.count_, tile.pdata_ );
   }
   return( isEqual );
}

//===========================================================================//
// Method         : operator==
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::operator!=( 
      const TTPT_Tile_c< T >& tile ) const
{
   return(( !this->operator==( tile )) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_Tile_c< T >::Print(
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   TTP_TileMode_t mode = this->GetMode( );
   string srMode;
   TTP_ExtractStringTileMode( mode, &srMode );

   string srRegion;
   this->region_.ExtractString( &srRegion );
  
   string srData;
   for( unsigned int i = 0; i < this->GetCount( ); ++i )
   {
      const T& data = *this->FindData( i );

      string srSubData;
      data.ExtractString( &srSubData );

      srData += "[";
      srData += srSubData;
      srData += "]";
   }

   printHandler.Write( pfile, spaceLen, "%s %s%s%s\n",  
                                        TIO_SR_STR( srMode ),
                                        TIO_SR_STR( srRegion ),
	 	                        srData.length( ) ? " " : "",
                                        TIO_SR_STR( srData ));
}

//===========================================================================//
// Method         : SetData
// Purpose        : Sets this tile's data.  This requires allocating a new
//                  single data.  This may require deleting any existing 
//                  multiple data array.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::SetData( 
      const T& data )
{
   bool ok = true;

   this->DeleteAllocData_( );
   ok = this->NewAllocData_( data );
   
   return( ok );
}

//===========================================================================//
// Method         : AddData
// Purpose        : Adds to this tile's data.  This may require allocating 
//                  either a new single data or multiple data array.  This
//                  may also require deleting any existing multiple data
//                  array.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::AddData( 
      const T& data )
{
   bool ok = true;

   ok = this->NewAllocData_( data );
   
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::AddData( 
            unsigned int count,
      const T*           pdata )
{
   bool ok = true;

   for( unsigned int i = 0; i < count; ++i )
   {
      ok = this->NewAllocData_( *( pdata + i ));
      if( !ok )
         break;
   }   
   return( ok );
}

//===========================================================================//
// Method         : ReplaceData
// Purpose        : Replaces this tile's data.  This may require allocating 
//                  either a new single data or multiple data array.  This
//                  may also require deleting any existing multiple data
//                  array.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::ReplaceData( 
            unsigned int count,
      const T*           pdata )
{
   bool ok = true;

   this->DeleteAllocData_( );
   for( unsigned int i = 0; i < count; ++i )
   {
      const T& data = *( pdata + i );

      ok = this->NewAllocData_( data );
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : GetData
// Purpose        : Gets this tile's data.  This returns either the existing 
//                  single data or the first multiple data array.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > T* TTPT_Tile_c< T >::GetData(
      void ) const
{
   return( const_cast< T* >( this->pdata_ ));
}

//===========================================================================//
template< class T > T* TTPT_Tile_c< T >::GetData(
      void )
{
   return( this->pdata_ );
}

//===========================================================================//
// Method         : FindData
// Purpose        : Finds this tile's ith data.  This returns either the 
//                  existing single data or the ith multiple data array,
//                  based on the given find index value.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > T* TTPT_Tile_c< T >::FindData(
      unsigned int i ) const
{
   T* pdata = 0;

   if( i < this->GetCount( ))
   {
      pdata = this->pdata_ + i;
   }
   return( const_cast< T* >( pdata ));
}

//===========================================================================//
template< class T > T* TTPT_Tile_c< T >::FindData(
      unsigned int i )
{
   T* pdata = 0;

   if( i < this->GetCount( ))
   {
      pdata = this->pdata_ + i;
   }
   return( pdata );
}

//===========================================================================//
// Method         : FindData
// Purpose        : Finds this tile's ith data.  This returns either the 
//                  existing single data or the ith multiple data array,
//                  based on the given find data value.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > T* TTPT_Tile_c< T >::FindData(
      const T& data ) const
{
   T* pdata = 0;

   for( unsigned int i = 0; i < this->GetCount( ); ++i )
   {
      if( data == *( this->pdata_ + i ))
      {
	 pdata = this->pdata_ + i;
         break;
      }
   }
   return( const_cast< T* >( pdata ));
}

//===========================================================================//
template< class T > T* TTPT_Tile_c< T >::FindData(
      const T& data )
{
   T* pdata = 0;

   for( unsigned int i = 0; i < this->GetCount( ); ++i )
   {
      if( data == *( this->pdata_ + i ))
      {
	 pdata = this->pdata_ + i;
         break;
      }
   }
   return( pdata );
}

//===========================================================================//
// Method         : DeleteData
// Purpose        : Deletes this tile's ith data, if possible.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::DeleteData(
      const T& data )
{
   bool ok = true;

   TTPT_Tile_c< T > tile( *this );

   this->DeleteAllocData_( );
   for( unsigned int i = 0; i < tile.GetCount( ); ++i )
   {
      if( data == *( tile.FindData( i )))
         continue;

      ok = this->NewAllocData_( *tile.FindData( i ));
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : HasData
// Purpose        : Return true if this tile has one or more data.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::HasData(
      void ) const
{
   return( this->pdata_ ? true : false );
}

//===========================================================================//
// Method         : MakeClear
// Purpose        : Make this tile into a clear tile.  This requires setting
//                  the tile's "clear" mode and resetting the tile's data.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_Tile_c< T >::MakeClear(
      void )
{
   this->mode_ = TTP_TILE_CLEAR;

   this->DeleteAllocData_( );
}

//===========================================================================//
// Method         : MakeSolid
// Purpose        : Make this tile into a solid tile.  This requires setting
//                  the tile's "solid" mode and setting the tile's data.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::MakeSolid( 
      const T& data )
{ 
   bool ok = true;

   this->mode_ = TTP_TILE_SOLID;

   this->DeleteAllocData_( );
   ok = this->NewAllocData_( data );

   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::MakeSolid( 
            unsigned int count,
      const T*           pdata )
{ 
   bool ok = true;

   this->mode_ = TTP_TILE_SOLID;

   this->DeleteAllocData_( );
   for( unsigned int i = 0; i < count; ++i )
   {
      this->NewAllocData_( *( pdata + i ));
      if( !ok )
         break;
   }   
   return( ok );
}

//===========================================================================//
// Method         : FindLessThan
// Purpose        : Returns the "left of or lower than" tile based on this 
//                  tile's region and the given tile's region with respect to 
//                  the given orientation.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_Tile_c< T >::FindLessThan( 
      TTPT_Tile_c< T >* ptile,
      TGS_OrientMode_t  orient )
{
   TTPT_Tile_c< T >* plessThan = 0;

   if( ptile )
   {
      plessThan = ( this->IsLessThan( *ptile, orient ) ? this : ptile );
   }
   return( plessThan );
}

//===========================================================================//
// Method         : IsGreaterThan
// Purpose        : Returns the "right of or upper than" tile based on this 
//                  tile's region and the given tile's region with respect to 
//                  the given orientation.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_Tile_c< T >::FindGreaterThan( 
      TTPT_Tile_c< T >* ptile,
      TGS_OrientMode_t  orient )
{
   TTPT_Tile_c< T >* pgreaterThan = 0;

   if( ptile )
   {
      pgreaterThan = ( this->IsGreaterThan( *ptile, orient ) ? this : ptile );
   }
   return( pgreaterThan );
}

//===========================================================================//
// Method         : IsLessThan
// Purpose        : Return true IFF this tile is "left of or lower than" 
//                  compared to the given tile, region, or point.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsLessThan(
      const TTPT_Tile_c< T >& tile,
            TGS_OrientMode_t  orient ) const
{
   const TGS_Region_c& region = tile.GetRegion( );

   return( this->IsLessThan( region, orient ));
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsLessThan(
      const TGS_Region_c&    region,
            TGS_OrientMode_t orient ) const
{
   bool isLessThan = false;

   switch( orient )
   {
   case TGS_ORIENT_HORIZONTAL:
      isLessThan = ( TCTF_IsLT( this->region_.x2, region.x1 ) ? true : false );
      break;

   case TGS_ORIENT_VERTICAL:
      isLessThan = ( TCTF_IsLT( this->region_.y2, region.y1 ) ? true : false );
      break;

   default:
      break;
   }
   return( isLessThan );
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsLessThan(
      const TGS_Point_c&     point,
            TGS_OrientMode_t orient ) const
{
   bool isLessThan = false;

   switch( orient )
   {
   case TGS_ORIENT_HORIZONTAL:
      isLessThan = ( TCTF_IsLT( this->region_.x2, point.x ) ? true : false );
      break;

   case TGS_ORIENT_VERTICAL:
      isLessThan = ( TCTF_IsLT( this->region_.y2, point.y ) ? true : false );
      break;

   default:
      break;
   }
   return( isLessThan );
}

//===========================================================================//
// Method         : IsGreaterThan
// Purpose        : Return true IFF this tile is "right of or upper than" 
//                  compared to the given tile, region, or point.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsGreaterThan(
      const TTPT_Tile_c< T >& tile,
            TGS_OrientMode_t  orient ) const
{
   const TGS_Region_c& region = tile.GetRegion( );

   return( this->IsGreaterThan( region, orient ));
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsGreaterThan(
      const TGS_Region_c&    region,
            TGS_OrientMode_t orient ) const
{
   bool isGreaterThan = false;

   switch( orient )
   {
   case TGS_ORIENT_HORIZONTAL:
      isGreaterThan = ( TCTF_IsGT( this->region_.x1, region.x2 ) ? true : false );
      break;

   case TGS_ORIENT_VERTICAL:
      isGreaterThan = ( TCTF_IsGT( this->region_.y1, region.y2 ) ? true : false );
      break;

   default:
      break;
   }
   return( isGreaterThan );
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsGreaterThan(
      const TGS_Point_c&     point,
            TGS_OrientMode_t orient ) const
{
   bool isGreaterThan = false;

   switch( orient )
   {
   case TGS_ORIENT_HORIZONTAL:
      isGreaterThan = ( TCTF_IsGT( this->region_.x1, point.x ) ? true : false );
      break;

   case TGS_ORIENT_VERTICAL:
      isGreaterThan = ( TCTF_IsGT( this->region_.y1, point.y ) ? true : false );
      break;

   default:
      break;
   }
   return( isGreaterThan );
}

//===========================================================================//
// Method         : IsEqualData
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsEqualData(
      const TTPT_Tile_c< T >& tile ) const
{
   return( tile.IsEqualData( this->count_, this->pdata_ ));
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsEqualData(
      const T& data ) const
{
   return( this->IsEqualData( 1, &data ));
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsEqualData(
            unsigned int count,
      const T*           pdata ) const
{
   bool isEqualData = false;

   if( this->GetCount( ) == count )
   {
      isEqualData = true;

      for( unsigned int i = 0; i < this->GetCount( ); ++i )
      {
         if( *( this->pdata_ + i ) != *( pdata + i ))
         {
            isEqualData = false;
            break;
         }
      }
   }
   return( isEqualData );
}

//===========================================================================//
// Method         : IsIntersecting
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsIntersecting(
      const TGS_Region_c& region ) const
{
   return( this->region_.IsIntersecting( region ));
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsIntersecting(
      const TGS_Line_c& line ) const
{
   TGS_Region_c region( line.x1, line.y1, line.x2, line.y2 );
   return( this->region_.IsIntersecting( region ));
}

//===========================================================================//
// Method         : IsWithin
// Purpose        : Return true according to membership of the given region 
//                  or point.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsWithin( 
      const TGS_Region_c& region ) const
{
   return( this->region_.IsWithin( region ));
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsWithin( 
      const TGS_Point_c& point ) const
{
   return( this->region_.IsWithin( point ));
}

//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::IsWithin( 
      const TGS_Point_c&     point,
            TGS_OrientMode_t orient ) const
{
   bool isWithin = false;

   switch( orient )
   {
   case TGS_ORIENT_HORIZONTAL:
      isWithin = ( TCTF_IsLE( this->region_.x1, point.x ) &&
                   TCTF_IsGE( this->region_.x2, point.x ) ?
                   true : false );
      break;

   case TGS_ORIENT_VERTICAL:
      isWithin = ( TCTF_IsLE( this->region_.y1, point.y ) &&
                   TCTF_IsGE( this->region_.y2, point.y ) ?
                   true : false );
      break;

   default:
      break;
   }
   return( isWithin );
}

//===========================================================================//
// Method         : NewAllocData
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_Tile_c< T >::NewAllocData_(
      const T& data )
{
   bool isValidNew = false;

   if( this->count_ == 0 )
   {
      ++this->count_;

      T* pdata = new TC_NOTHROW T;

      TIO_PrintHandler_c& messageHandler = TIO_PrintHandler_c::GetInstance( );
      if( messageHandler.IsValidNew( pdata,
                                     sizeof( T ),
                                     "TTPT_Tile_c< T >::NewAllocData_" ))
      {  
         *pdata = data;
         this->pdata_ = pdata;

         isValidNew = true;
      }      
   }
   else if( this->count_ == 1 )
   {
      ++this->count_;

      T* pdata = new TC_NOTHROW T[static_cast< unsigned long >( this->count_ )];

      TIO_PrintHandler_c& messageHandler = TIO_PrintHandler_c::GetInstance( );
      if( messageHandler.IsValidNew( pdata,
                                     sizeof( T ) * this->count_,
                                     "TTPT_Tile_c< T >::NewAllocData_" ))
      {
         *( pdata + 0 ) = *this->pdata_;
         *( pdata + 1 ) = data;

         delete this->pdata_;
         this->pdata_ = pdata;

         isValidNew = true;
      }      
   }
   else if( this->count_ > 1 )
   {
      ++this->count_;

      T* pdata = new TC_NOTHROW T[static_cast< unsigned long >( this->count_ )];

      TIO_PrintHandler_c& messageHandler = TIO_PrintHandler_c::GetInstance( );
      if( messageHandler.IsValidNew( pdata,
                                     sizeof( T ) * this->count_,
                                     "TTPT_Tile_c< T >::NewAllocData_" ))
      {
         unsigned int i = 0;
         while( i < this->GetCount( ) - 1 )
	 {
            *( pdata + i ) = *( this->pdata_ + i );
            ++i;
         }
         *( pdata + i ) = data;

         delete[] this->pdata_;
         this->pdata_ = pdata;

         isValidNew = true;
      }      
   }
   return( isValidNew );
}

//===========================================================================//
// Method         : DeleteAllocData_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_Tile_c< T >::DeleteAllocData_(
      void )
{
   if( this->count_ == 1 )
   {
      delete this->pdata_;
   }
   else if( this->count_ > 1 )
   {
      delete[] this->pdata_;
   }
   this->pdata_ = 0;
   this->count_ = 0;
}

#endif
