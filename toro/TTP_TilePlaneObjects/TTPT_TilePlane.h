//===========================================================================//
// Purpose   : Template version for a TTPT_TilePlane tile plane object class.
//
// Reference : "Corner Stitching: A Data-Structuring Technique for VLSI 
//             Layout Tools", John K. Ousterhout, IEEE Transactions on 
//             Computer-Aided Design, Vol. CAD-3, No. 1, January 1984
//
//           Inline methods include:
//           - GetRegion, GetMinGrid
//           - IsWithin
//           - IsValid
//
//           Public methods include:
//           - TTPT_TilePlane_c, ~TTPT_TilePlane_c
//           - operator=
//           - operator==, operator!=
//           - Print, PrintLaff
//           - Init
//           - Reset
//           - Add
//           - Delete
//           - Clear
//           - Find
//           - FindNearest
//           - FindConnected
//           - FindAdjacents
//           - FindData
//           - FindCount
//           - FindRegion
//           - Split
//           - Merge
//           - MergeAdjacents
//           - JoinAdjacents
//           - HasAdjacents
//           - HasNeighbor
//           - IsClear, IsSolid, IsSolidNot
//           - IsIntersecting
//           - IsAdjacent
//           - IsMergable
//           - IsSplitable
//           - IsLegal
//
//           Private methods include:
//           - AddPerRegion_
//           - AddPerNew_
//           - AddPerMerge_
//           - AddPerMergeNew_
//           - AddPerOverlap_
//           - AddPerUpperSideSplit_
//           - AddPerLowerSideSplit_
//           - AddPerCenterSideSplits_
//           - AddPerSolidTileMerges_
//           - AddPerSolidTileCorners_
//           - AddPerSolidTileCorner_
//           - DeletePerExact_
//           - DeletePerWithin_
//           - DeletePerIntersect_
//           - DeletePerIntersectMin_
//           - DeletePerRightSide_
//           - DeletePerLeftSide_
//           - DeletePerLowerSide_
//           - DeletePerUpperSide_
//           - FindPerPoint_
//           - FindPerExact
//           - FindPerWithin_
//           - FindPerIntersect_
//           - FindNearestBySides_
//           - FindNearestBySide_
//           - FindNearestByRegion_
//           - FindCountRefresh_
//           - SplitRegionCoords_
//           - MergeRegionCoords_
//           - MergeAdjacentsByExtent_
//           - MergeAdjacentsToLeft_
//           - MergeAdjacentsToRight_
//           - MergeAdjacentsToLower_
//           - MergeAdjacentsToUpper_
//           - JoinAdjacentsPerExact_
//           - JoinAdjacentsPerIntersect_
//           - JoinAdjacentsBySide_
//           - CornerRegionCoords_
//           - CornerTileRemerge_
//           - StitchToSplit_
//           - StitchToMerge_
//           - StitchFromSide_
//           - EstAspectRatio_
//           - HasAspectRatio_
//           - IsClearMatch_, IsClearAny_, IsClearAll_
//           - IsSolidMatch_, IsSolidAny_, IsSolidAll_, IsSolidMax_
//           - IsSolidNotMatch_, IsSolidNotAny_, IsSolidNotAll_
//           - Allocate_, Deallocate_
//           - ShowInternalMessage_
//
//===========================================================================//

#ifndef TTPT_TILE_PLANE_H
#define TTPT_TILE_PLANE_H

#include <cstdio>
using namespace std;

#include "TC_Typedefs.h"
#include "TC_MinGrid.h"
#include "TCT_Generic.h"
#include "TCT_OrderedVector.h"

#include "TIO_PrintHandler.h"
#include "TIO_SkinHandler.h"

#include "TGS_Typedefs.h"
#include "TGS_Region.h"
#include "TGS_Line.h"
#include "TGS_Point.h"

#include "TTP_Typedefs.h"
#include "TTPT_Tile.h"
#include "TTPT_TilePlaneIter.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > class TTPT_TilePlane_c
{
public:

   TTPT_TilePlane_c( void );
   TTPT_TilePlane_c( const TGS_Region_c& region );
   TTPT_TilePlane_c( const TTPT_TilePlane_c< T >& tilePlane );
   ~TTPT_TilePlane_c( void );

   TTPT_TilePlane_c< T >& operator=( const TTPT_TilePlane_c< T >& tilePlane );
   bool operator==( const TTPT_TilePlane_c< T >& tilePlane ) const;
   bool operator!=( const TTPT_TilePlane_c< T >& tilePlane ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const; 
   void PrintLaff( void ) const;
   void PrintLaff( const char* pszFileName ) const;

   bool Init( const TGS_Region_c& region );
   bool Init( const TTPT_TilePlane_c< T >& tilePlane );

   void Reset( void );

   const TGS_Region_c& GetRegion( void ) const;
   double GetMinGrid( void ) const;

   bool Add( const TTPT_Tile_c< T >& tile,
             TTP_AddMode_t addMode = TTP_ADD_OVERLAP );
   bool Add( const TTPT_TilePlane_c< T >& tilePlane,
             TTP_AddMode_t addMode = TTP_ADD_OVERLAP );
   bool Add( const TTPT_Tile_c< T >& tile,
             TGS_OrientMode_t addOrient,
             TTP_AddMode_t addMode = TTP_ADD_OVERLAP );
   bool Add( const TGS_Region_c& region,
             TTP_AddMode_t addMode = TTP_ADD_OVERLAP );
   bool Add( const TGS_Region_c& region,
             TGS_OrientMode_t addOrient,
             TTP_AddMode_t addMode = TTP_ADD_OVERLAP );
   bool Add( const TGS_Region_c& region,
             const T& data,
             TTP_AddMode_t addMode = TTP_ADD_OVERLAP );
   bool Add( const TGS_Region_c& region,
             const T& data,
             TGS_OrientMode_t addOrient,
             TTP_AddMode_t addMode = TTP_ADD_OVERLAP );

   bool Delete( TTPT_Tile_c< T >* ptile );
   bool Delete( const TGS_Region_c& region,
                TTP_DeleteMode_t deleteMode = TTP_DELETE_INTERSECT );
   bool Delete( const TGS_Region_c& region,
                const T& data,
                TTP_DeleteMode_t deleteMode = TTP_DELETE_INTERSECT );

   bool Clear( void );

   TTPT_Tile_c< T >* Find( double x, double y,
                           TTPT_Tile_c< T >* ptile = 0 ) const;
   TTPT_Tile_c< T >* Find( const TGS_Point_c& point,
                           TTPT_Tile_c< T >* ptile = 0 ) const;
   TTPT_Tile_c< T >* Find( const TGS_Region_c& region,
                           TTP_FindMode_t findMode = TTP_FIND_EXACT,
                           TTP_TileMode_t tileMode = TTP_TILE_ANY ) const;

   TTPT_Tile_c< T >* FindNearest( const TGS_Region_c& refRegion,
                                  const TGS_OrientMode_t* porientMode = 0 ) const;
   TTPT_Tile_c< T >* FindNearest( const TGS_Region_c& refRegion,
				  double searchDistance,
                                  const TGS_OrientMode_t* porientMode = 0 ) const;
   TTPT_Tile_c< T >* FindNearest( const TGS_Region_c& refRegion,
				  TC_SideMode_t searchSide ) const;

   bool FindConnected( const TGS_Region_c& region,
                       TTPT_TilePlane_c< T >* ptilePlane ) const;

   bool FindAdjacents( const TGS_Region_c& region,
                       TTPT_TilePlane_c< T >* ptilePlane ) const;
   bool FindAdjacents( const TGS_Region_c& searchRegion,
                       const TGS_Region_c& region,
                       TTPT_TilePlane_c< T >* ptilePlane ) const;

   bool FindData( double x, double y,
                  TCT_OrderedVector_c< T >* pdataList ) const;
   bool FindData( const TGS_Point_c& point,
                  TCT_OrderedVector_c< T >* pdataList ) const;
   bool FindData( const TGS_Region_c& region,
                  TCT_OrderedVector_c< T >* pdataList ) const;

   size_t FindCount( TTP_TileMode_t tileMode ) const;
   size_t FindCount( const TGS_Region_c& region,
                     TTP_TileMode_t tileMode ) const;

   void FindRegion( TTP_TileMode_t tileMode,
                    TGS_Region_c* pregion ) const;

   bool Split( TTPT_Tile_c< T >* ptile,
               const TGS_Line_c& line,
               TGS_DirMode_t dir = TGS_DIR_UNDEFINED );
   bool Split( TTPT_Tile_c< T >* ptile,
               TC_SideMode_t side = TC_SIDE_UNDEFINED );

   bool Merge( TTPT_Tile_c< T >* ptileA,
               TTPT_Tile_c< T >* ptileB );
   bool Merge( TTPT_Tile_c< T >* ptile,
               TC_SideMode_t side = TC_SIDE_UNDEFINED );

   bool MergeAdjacents( const TGS_Point_c& point,
                        TGS_Region_c* pregion,
                        TGS_OrientMode_t orientMode = TGS_ORIENT_UNDEFINED,
                        TTP_MergeMode_t mergeMode = TTP_MERGE_EXACT,
                        TTP_TileMode_t tileMode = TTP_TILE_SOLID ) const;
   bool MergeAdjacents( const TGS_Region_c& region,
                        TGS_Region_c* pregion,
                        TGS_OrientMode_t orientMode = TGS_ORIENT_UNDEFINED,
                        TTP_MergeMode_t mergeMode = TTP_MERGE_EXACT,
                        TTP_TileMode_t tileMode = TTP_TILE_SOLID ) const;
   bool MergeAdjacents( const TTPT_Tile_c< T >& tile,
                        TGS_Region_c* pregion,
                        TGS_OrientMode_t orientMode = TGS_ORIENT_UNDEFINED,
                        TTP_MergeMode_t mergeMode = TTP_MERGE_EXACT ) const;

   bool JoinAdjacents( const TGS_Point_c& point,
                       TGS_RegionList_t* pregionList ) const;
   bool JoinAdjacents( const TGS_Region_c& region,
                       TGS_RegionList_t* pregionList,
                       TTP_JoinMode_t joinMode = TTP_JOIN_EXACT ) const;
   bool JoinAdjacents( const TTPT_Tile_c< T >& tile,
                       TGS_RegionList_t* pregionList ) const;

   bool HasAdjacents( const TGS_Region_c& region ) const;

   bool HasNeighbor( const TGS_Region_c& region,
                     double minDistance ) const;
   bool HasNeighbor( const TTPT_Tile_c< T >& tile,
                     double minDistance ) const;

   bool IsClear( void ) const;
   bool IsClear( double x, double y ) const;
   bool IsClear( const TGS_Point_c& point ) const;
   bool IsClear( const TGS_Region_c& region ) const;
   bool IsClear( TTP_IsClearMode_t isClearMode,
                 const TGS_Region_c& region,
                 TTPT_Tile_c< T >** ppclearTile = 0 ) const;

   bool IsSolid( double x, double y ) const;
   bool IsSolid( const TGS_Point_c& point ) const;
   bool IsSolid( const TGS_Point_c& point,
                 const T& data ) const;
   bool IsSolid( const TGS_Point_c& point,
                 unsigned int count,
                 const T* pdata ) const;
   bool IsSolid( const TGS_Region_c& region ) const;
   bool IsSolid( const TGS_Region_c& region,
                 const T& data ) const;
   bool IsSolid( const TGS_Region_c& region,
                 unsigned int count,
                 const T* pdata ) const;
   bool IsSolid( TTP_IsSolidMode_t isSolidMode,
                 TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolid( TTP_IsSolidMode_t isSolidMode,
                 const TGS_Region_c& region,
                 TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolid( TTP_IsSolidMode_t isSolidMode,
                 const TGS_Region_c& region,
                 const T& data,
                 TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolid( TTP_IsSolidMode_t isSolidMode,
                 const TGS_Region_c& region,
                 unsigned int count,
                 const T* pdata,
                 TTPT_Tile_c< T >** ppsolidTile = 0 ) const;

   bool IsSolidNot( const TGS_Point_c& point,
                    const T& data ) const;
   bool IsSolidNot( const TGS_Point_c& point,
                    unsigned int count,
                    const T* pdata ) const;
   bool IsSolidNot( const TGS_Region_c& region,
                    const T& data ) const;
   bool IsSolidNot( const TGS_Region_c& region,
                    unsigned int count,
                    const T* pdata ) const;
   bool IsSolidNot( TTP_IsSolidMode_t isSolidMode,
                    const TGS_Region_c& region,
                    const T& data,
                    TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolidNot( TTP_IsSolidMode_t isSolidMode,
                    const TGS_Region_c& region,
                    unsigned int count,
                    const T* pdata,
                    TTPT_Tile_c< T >** ppsolidTile = 0 ) const;

   bool IsIntersecting( const TGS_Region_c& region,
                        TGS_Region_c* pintersectRegion = 0,
                        TTP_TileMode_t tileMode = TTP_TILE_ANY ) const;

   bool IsAdjacent( const TGS_Region_c& regionA,
                    const TGS_Region_c& regionB,
                    TC_SideMode_t* pside = 0 ) const;
   bool IsAdjacent( const TTPT_Tile_c< T >& tileA,
                    const TTPT_Tile_c< T >& tileB,
                    TC_SideMode_t* pside = 0 ) const;

   bool IsMergable( const TTPT_Tile_c< T >& tileA,
                    const TTPT_Tile_c< T >& tileB,
                    TGS_OrientMode_t* porientMode = 0 ) const;

   bool IsSplitable( const TTPT_Tile_c< T >& tile,
                     const TGS_Line_c& intersectLine,
                     TGS_DirMode_t dirMode,
                     TGS_OrientMode_t* porientMode = 0 ) const;

   bool IsWithin( const TGS_Point_c& point ) const;
   bool IsWithin( const TGS_Region_c& region ) const;
   bool IsWithin( const TTPT_Tile_c< T >& tile ) const;

   bool IsLegal( void ) const;
   bool IsValid( void ) const;

private:

   bool AddPerRegion_( const TTPT_Tile_c< T >& addTile,
                       TGS_OrientMode_t addOrient );
   bool AddPerNew_( const TTPT_Tile_c< T >& addTile,
                    TGS_OrientMode_t addOrient );
   bool AddPerMerge_( const TTPT_Tile_c< T >& addTile,
                      TGS_OrientMode_t addOrient,
                      TTP_AddMode_t addMode );
   bool AddPerMergeNew_( const TTPT_Tile_c< T >& addTile,
                         TGS_OrientMode_t addOrient,
                         TTP_AddMode_t addMode );
   bool AddPerOverlap_( const TTPT_Tile_c< T >& addTile,
                        TGS_OrientMode_t addOrient,
                        TTP_AddMode_t addMode );
   bool AddPerOverlap_( const TTPT_Tile_c< T >& addTile,
                        const TTPT_Tile_c< T >* poverlapTile,
                        TGS_OrientMode_t addOrient,
                        TTP_AddMode_t addMode );

   bool AddPerUpperSideSplit_( const TGS_Region_c& addRegion );
   bool AddPerLowerSideSplit_( const TGS_Region_c& addRegion );
   bool AddPerCenterSideSplits_( const TGS_Region_c& addRegion );
   bool AddPerSolidTileMerges_( const TGS_Region_c& addRegion );
   bool AddPerSolidTileCorners_( const TGS_Region_c& addRegion,
                                 TGS_OrientMode_t addOrient );
   bool AddPerSolidTileCorner_( const TGS_Region_c& addRegion,
                                const TGS_Point_c& cornerPoint,
                                TGS_OrientMode_t addOrient );

   bool DeletePerExact_( const TGS_Region_c& deleteRegion );
   bool DeletePerExact_( const TGS_Region_c& deleteRegion,
                         const T& deleteData );
   bool DeletePerWithin_( const TGS_Region_c& deleteRegion );
   bool DeletePerWithin_( const TGS_Region_c& deleteRegion,
                          const T& deleteData );
   bool DeletePerIntersect_( const TGS_Region_c& deleteRegion );
   bool DeletePerIntersect_( const TGS_Region_c& deleteRegion,
                             const T& deleteData );
   bool DeletePerIntersectMin_( const TGS_Region_c& deleteRegion );
   bool DeletePerIntersectMin_( const TGS_Region_c& deleteRegion,
                                const T& deleteData );

   bool DeletePerRightSide_( const TTPT_Tile_c< T >& deleteTile );
   bool DeletePerLeftSide_( const TTPT_Tile_c< T >& deleteTile );
   bool DeletePerLowerSide_( const TTPT_Tile_c< T >& deleteTile );
   bool DeletePerUpperSide_( const TTPT_Tile_c< T >& deleteTile );

   TTPT_Tile_c< T >* FindPerPoint_( const TGS_Point_c& point,
                                    TTPT_Tile_c< T >* ptile = 0 ) const;
   TTPT_Tile_c< T >* FindPerExact_( const TGS_Region_c& region,
                                    TTP_TileMode_t tileMode = TTP_TILE_ANY ) const;
   TTPT_Tile_c< T >* FindPerWithin_( const TGS_Region_c& region,
                                     TTP_TileMode_t tileMode = TTP_TILE_ANY ) const;
   TTPT_Tile_c< T >* FindPerIntersect_( const TGS_Region_c& region,
                                        TTP_TileMode_t tileMode = TTP_TILE_ANY ) const;

   TTPT_Tile_c< T >* FindNearestBySides_( const TGS_Region_c& refRegion,
                                          const TTPT_Tile_c< T >& searchTile,
                                          const TGS_OrientMode_t* porientMode = 0 ) const;
   TTPT_Tile_c< T >* FindNearestBySide_( const TGS_Region_c& refRegion,
                                         const TTPT_Tile_c< T >& searchTile,
                                         TC_SideMode_t searchSide ) const;
   TTPT_Tile_c< T >* FindNearestBySide_( const TGS_Region_c& refRegion,
                                         TC_SideMode_t searchSide,
                                         double searchDistance = TC_FLT_MAX ) const;
   TTPT_Tile_c< T >* FindNearestByRegion_( const TGS_Region_c& refRegion,
                                           const TGS_Region_c& searchRegion,
                                           const TGS_OrientMode_t* porientMode = 0 ) const;
   TTPT_Tile_c< T >* FindNearestByRegion_( const TGS_Region_c& refRegion,
                                           double searchDistance,
                                           const TGS_OrientMode_t* porientMode = 0 ) const;

   void FindCountRefresh_( void ) const;
   void FindCountRefresh_( const TGS_Region_c& region ) const;

   void SplitRegionCoords_( TTPT_Tile_c< T >* ptileA,
                            TTPT_Tile_c< T >* ptileB,
                            const TGS_Line_c& line,
                            TGS_DirMode_t dirMode,
                            TGS_OrientMode_t orientMode );

   void MergeRegionCoords_( TTPT_Tile_c< T >* ptileA,
                            TTPT_Tile_c< T >* ptileB,
                            TGS_OrientMode_t orientMode,
                            TGS_CornerMode_t cornerMode );

   bool MergeAdjacentsByExtent_( TGS_OrientMode_t orientMode,
                                 unsigned int tileCount,
                                 const T* ptileData,
                                 TTP_TileMode_t tileMode,
                                 TTP_MergeMode_t mergeMode,
                                 TGS_Region_c* pmergedRegion ) const;
   bool MergeAdjacentsToLeft_( unsigned int tileCount,
                               const T* ptileData,
                               TTP_TileMode_t tileMode,
                               TTP_MergeMode_t mergeMode,
                               TGS_Region_c* pmergedRegion ) const;
   bool MergeAdjacentsToRight_( unsigned int tileCount,
                                const T* ptileData,
                                TTP_TileMode_t tileMode,
                                TTP_MergeMode_t mergeMode,
                                TGS_Region_c* pmergedRegion ) const;
   bool MergeAdjacentsToLower_( unsigned int tileCount,
                                const T* ptileData,
                                TTP_TileMode_t tileMode,
                                TTP_MergeMode_t mergeMode,
                                TGS_Region_c* pmergedRegion ) const;
   bool MergeAdjacentsToUpper_( unsigned int tileCount,
                                const T* ptileData,
                                TTP_TileMode_t tileMode,
                                TTP_MergeMode_t mergeMode,
                                TGS_Region_c* pmergedRegion ) const;

   bool JoinAdjacentsPerExact_( const TGS_Region_c& region,
                                TGS_RegionList_t* pregionList ) const;
   bool JoinAdjacentsPerIntersect_( const TGS_Region_c& region,
                                    TGS_RegionList_t* pregionList ) const;
   bool JoinAdjacentsBySide_( TC_SideMode_t side,
                              const TGS_Region_c& region,
                              unsigned int count,
                              const T* pdata,
                              TTP_TileMode_t tileMode,
                              TGS_RegionList_t* pregionList ) const;

   bool CornerRegionCoords_( const TGS_Region_c& regionA,
                             const TGS_Region_c& regionB,
                             TGS_Region_c* pregionA,
                             TGS_Region_c* pregionB,
                             TGS_OrientMode_t addOrient );
   bool CornerTileRemerge_( TTPT_Tile_c< T >* ptileA,
                            TTPT_Tile_c< T >* ptileB,
                            const TGS_Region_c& regionA,
                            const TGS_Region_c& regionB,
                            TGS_OrientMode_t addOrient );

   void StitchToSplit_( TTPT_Tile_c< T >* ptileA,
                        TTPT_Tile_c< T >* ptileB,
                        TGS_OrientMode_t orientMode );
   void StitchToMerge_( TTPT_Tile_c< T >* ptileA,
                        TTPT_Tile_c< T >* ptileB,
                        TGS_OrientMode_t orientMode,
                        TGS_CornerMode_t cornerMode );
   void StitchFromSide_( TTPT_Tile_c< T >* ptile,
                         TC_SideMode_t side,
			 TTPT_Tile_c< T >* ptileP = 0 );

   void LoadTileList_( TTPT_Tile_c< T >* ptile,
                       TCT_OrderedVector_c< TTPT_Tile_c< T >* >* ptileList );

   void EstAspectRatio_( const TGS_Region_c& newRegion,
                         const TGS_Region_c& curRegion,
                         double* pminAspectRatio,
                         double* pmaxAspectRatio ) const;
   bool HasAspectRatio_( const TGS_Region_c& newRegion,
                         const TGS_Region_c& curRegion ) const;

   bool IsClearMatch_( const TGS_Point_c& point ) const;
   bool IsClearMatch_( const TGS_Region_c& region,
                       TTPT_Tile_c< T >** ppclearTile ) const;
   bool IsClearAny_( const TGS_Region_c& region,
                     TTPT_Tile_c< T >** ppclearTile ) const;
   bool IsClearAll_( const TGS_Region_c& region,
                     TTPT_Tile_c< T >** ppclearTile ) const;

   bool IsSolidMatch_( const TGS_Point_c& point ) const;
   bool IsSolidMatch_( const TGS_Point_c& point,
                       unsigned int count,
                       const T* pdata ) const;
   bool IsSolidMatch_( const TGS_Region_c& region,
                       TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolidMatch_( const TGS_Region_c& region,
                       unsigned int count,
                       const T* pdata,
                       TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolidAny_( const TGS_Region_c& region,
                     TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolidAny_( const TGS_Region_c& region,
                     unsigned int count,
                     const T* pdata,
                     TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolidAll_( const TGS_Region_c& region,
                     TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolidAll_( const TGS_Region_c& region,
                     unsigned int count,
                     const T* pdata,
                     TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolidMax_( const TGS_Region_c& region,
                     TTPT_Tile_c< T >** ppsolidTile = 0 ) const;
   bool IsSolidMax_( const TGS_Region_c& region,
                     unsigned int count,
                     const T* pdata,
                     TTPT_Tile_c< T >** ppsolidTile = 0 ) const;

   bool IsSolidNotMatch_( const TGS_Point_c& point,
                          unsigned int count,
                          const T* pdata ) const;
   bool IsSolidNotMatch_( const TGS_Region_c& region,
                          unsigned int count,
                          const T* pdata,
                          TTPT_Tile_c< T >** ppsolidNotTile ) const;
   bool IsSolidNotAny_( const TGS_Region_c& region,
                        unsigned int count,
                        const T* pdata,
                        TTPT_Tile_c< T >** ppsolidNotTile ) const;
   bool IsSolidNotAny_( const TGS_Region_c& region ) const;
   bool IsSolidNotAll_( const TGS_Region_c& region,
                        unsigned int count,
                        const T* pdata,
                        TTPT_Tile_c< T >** ppsolidNotTile ) const;

   TTPT_Tile_c< T >* Allocate_( const TTPT_Tile_c< T >& tile );
   void Deallocate_( TTPT_Tile_c< T >* ptile );

   bool ShowInternalMessage_( TTP_MessageType_t messageType,
                              const char* pszSourceMethod,
                              const TGS_Region_c* ptileRegion = 0, 
                              const TGS_Region_c* pnextRegion = 0 ) const;

private:

   TGS_Region_c region_;       // Define tile plane bounding region

   TTPT_Tile_c< T >* ptileLL_; // Ptr to lower-left most tile in plane
   TTPT_Tile_c< T >* ptileMRC_;// Ptr to most-recently referenced clear tile
                               // (most-recent reference is cached for performance)
   TTPT_Tile_c< T >* ptileMRS_;// Ptr to most-recently referenced solid tile
                               // (most-recent reference is cached for performance)

   TTPT_TilePlaneIter_c< T > tilePlaneIter_;
                               // Local cached tile plane iterator
                               // (improves performance by avoiding mallocs)

   double minGrid_;            // Define min grid value for tile region coords

   class TTP_TilePlaneCount_c
   {
   public:
      size_t clear;            // Define total number of clear tiles in plane
      size_t solid;            // Define total number of solid tiles in plane
      bool   isValid;          // TRUE => clear & solid tile counts are valid
   } count_;
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > inline const TGS_Region_c& TTPT_TilePlane_c< T >::GetRegion(
      void ) const
{
   return( this->region_ );
}

//===========================================================================//
template< class T > inline double TTPT_TilePlane_c< T >::GetMinGrid(
      void ) const
{
   return( this->minGrid_ );
}

//===========================================================================//
template< class T > inline bool TTPT_TilePlane_c< T >::IsWithin(
      const TGS_Point_c& point ) const
{
   return( this->region_.IsWithin( point ));
}

//===========================================================================//
template< class T > inline bool TTPT_TilePlane_c< T >::IsWithin(
      const TGS_Region_c& region ) const
{
   return( this->region_.IsWithin( region ));
}

//===========================================================================//
template< class T > inline bool TTPT_TilePlane_c< T >::IsWithin(
      const TTPT_Tile_c< T >& tile ) const
{
   return( this->region_.IsWithin( tile.GetRegion( )));
}

//===========================================================================//
template< class T > inline bool TTPT_TilePlane_c< T >::IsValid(
      void ) const
{
   return(( this->region_.IsValid( )) && 
	  ( this->ptileLL_ && this->ptileMRC_ && this->ptileMRS_ ) ?
	  true : false );
}

//===========================================================================//
// Method         : TTPT_TilePlane_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_TilePlane_c< T >::TTPT_TilePlane_c( 
      void )
      :
      ptileLL_( 0 ),
      ptileMRC_( 0 ),
      ptileMRS_( 0 )
{
   this->minGrid_ = TC_MinGrid_c::GetInstance( ).GetGrid( );

   this->count_.clear = 0;
   this->count_.solid = 0;
   this->count_.isValid = false;

   TGS_Region_c region( static_cast< double >( INT_MIN ),
                        static_cast< double >( INT_MIN ),
                        static_cast< double >( INT_MAX ),
                        static_cast< double >( INT_MAX ));
   this->Init( region );
}

//===========================================================================//
template< class T > TTPT_TilePlane_c< T >::TTPT_TilePlane_c( 
      const TGS_Region_c& region )
      :
      ptileLL_( 0 ),
      ptileMRC_( 0 ),
      ptileMRS_( 0 )
{
   this->minGrid_ = TC_MinGrid_c::GetInstance( ).GetGrid( );

   this->count_.clear = 0;
   this->count_.solid = 0;
   this->count_.isValid = false;

   this->Init( region );
}

//===========================================================================//
template< class T > TTPT_TilePlane_c< T >::TTPT_TilePlane_c( 
      const TTPT_TilePlane_c< T >& tilePlane )
      :
      ptileLL_( 0 ),
      ptileMRC_( 0 ),
      ptileMRS_( 0 )
{
   this->minGrid_ = TC_MinGrid_c::GetInstance( ).GetGrid( );

   this->count_.clear = 0;
   this->count_.solid = 0;
   this->count_.isValid = false;

   this->Init( tilePlane );
}

//===========================================================================//
// Method         : ~TTPT_TilePlane_c
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_TilePlane_c< T >::~TTPT_TilePlane_c( 
      void )
{
   this->Reset( );
}

//===========================================================================//
// Method         : operator=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_TilePlane_c< T >& TTPT_TilePlane_c< T >::operator=( 
      const TTPT_TilePlane_c< T >& tilePlane )
{
   if( &tilePlane != this )
   {
      this->Init( tilePlane );
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
template< class T > bool TTPT_TilePlane_c< T >::operator==( 
      const TTPT_TilePlane_c< T >& tilePlane ) const
{
   bool isEqual = false;

   if( this->region_ == tilePlane.region_ )
   {
      isEqual = true;

      if( isEqual )
      {
	 // Test to see if every tile in this plane matches given tile plane
         TTPT_TilePlaneIter_c< T > thisIter( *this );
         TTPT_Tile_c< T >* ptile = 0;
         while( thisIter.Next( &ptile, TTP_TILE_SOLID ))
         {  
            const TGS_Region_c& region = ptile->GetRegion( );
            const TTPT_Tile_c< T >* pfoundTile = tilePlane.Find( region.x1,
                                                                 region.y1 );

            isEqual = ( pfoundTile && *pfoundTile == *ptile ? true : false );
	    if( !isEqual )
               break;
         }
      }
      if( isEqual )
      {
	 // Test to see if every tile in given tile plane matches this plane
         TTPT_TilePlaneIter_c< T > tilePlaneIter( tilePlane );
         TTPT_Tile_c< T >* ptile = 0;
         while( tilePlaneIter.Next( &ptile, TTP_TILE_SOLID ))
         {  
            const TGS_Region_c& region = ptile->GetRegion( );
            const TTPT_Tile_c< T >* pfoundTile = this->Find( region.x1,
	   			     	 	             region.y1 );

            isEqual = ( pfoundTile && *pfoundTile == *ptile ? true : false );
	    if( !isEqual )
               break;
         }
      }
   }
   return( isEqual );
}

//===========================================================================//
// Method         : operator!=
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::operator!=( 
      const TTPT_TilePlane_c< T >& tilePlane ) const
{
   return( !this->operator==( tilePlane ) ? true : false );
}

//===========================================================================//
// Method         : Print
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::Print(
      FILE*  pfile,
      size_t spaceLen ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   if( this->region_.IsValid( ) &&
       TCTF_IsGT( this->region_.x1, static_cast< double >( INT_MIN )) &&
       TCTF_IsGT( this->region_.y1, static_cast< double >( INT_MIN )) &&
       TCTF_IsLT( this->region_.x2, static_cast< double >( INT_MAX )) &&
       TCTF_IsLT( this->region_.y2, static_cast< double >( INT_MAX )))
   {
      string srRegion;
      this->region_.ExtractString( &srRegion );
  
      printHandler.Write( pfile, spaceLen, "[%s]\n", TIO_SR_STR( srRegion ));
      spaceLen += 3;
   }

   TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
   ptilePlane->tilePlaneIter_.Init( *this );
   TTPT_Tile_c< T >* ptile = 0;
   while( ptilePlane->tilePlaneIter_.Next( &ptile, TTP_TILE_SOLID ))
   {
      ptile->Print( pfile, spaceLen );
   }
}

//===========================================================================//
// Method         : PrintLaff
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::PrintLaff(
      void ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   TIO_SkinHandler_c& skinHandler = TIO_SkinHandler_c::GetInstance( );

   unsigned int laffColorRed    = 1;
   // unsigned int laffColorGreen  = 2;
   // unsigned int laffColorBlue   = 3;
   // unsigned int laffColorCyan   = 4;
   unsigned int laffColorYellow = 5;
   // unsigned int laffColorPink   = 6;
   unsigned int laffColorWhite  = 7;

   unsigned int laffLayerBoundary  = 1;
   unsigned int laffLayerTileClear = 2;
   unsigned int laffLayerTileSolid = 3;

   TC_MinGrid_c& MinGrid = TC_MinGrid_c::GetInstance( );
   double minGrid = MinGrid.GetGrid( );
   unsigned int precision = MinGrid.GetPrecision( );
   unsigned int magnitude = MinGrid.GetMagnitude( );
   unsigned int units = MinGrid.GetUnits( );

   printHandler.Write( "(OBJECT 'BARINFO' (\n" );
   printHandler.Write( "  (ATTR 'UNITS' (\n" );
   printHandler.Write( "    (TEXT '%0.*f MICRONS')))\n", precision, 1.0 / static_cast< double >( magnitude ));
   printHandler.Write( "  (ATTR 'MINGRID' (\n" );
   printHandler.Write( "    (TEXT '%0.*f %0.*f')))\n", precision, minGrid, precision, minGrid );
   printHandler.Write( "  (ATTR 'LAYERS' (\n" );
   printHandler.Write( "    (TEXT '%u 1  0 %u %u BOUNDARY')\n",   laffColorYellow, laffLayerBoundary,  laffLayerBoundary );
   printHandler.Write( "    (TEXT '%u 1  7 %u %u TILE_CLEAR')\n", laffColorWhite,  laffLayerTileClear, laffLayerTileClear );
   printHandler.Write( "    (TEXT '%u 1  8 %u %u TILE_SOLID')\n", laffColorRed,    laffLayerTileSolid, laffLayerTileSolid );
   printHandler.Write( "  ))\n" );
   printHandler.Write( "))\n" );
   printHandler.Write( "(OBJECT '%s' (\n", skinHandler.GetSourceName( ));
   printHandler.Write( "  (DETAIL (\n" );

   printHandler.Write( "    ; Tile Plane Boundary\n" );

   if( this->region_.IsValid( ))
   {
      const TGS_Region_c& region = this->region_;

      unsigned int layer = laffLayerBoundary;
      long int x1 = MinGrid.FloatToLongInt( region.x1 ) * magnitude / units;
      long int y1 = MinGrid.FloatToLongInt( region.y1 ) * magnitude / units;
      long int x2 = MinGrid.FloatToLongInt( region.x2 ) * magnitude / units;
      long int y2 = MinGrid.FloatToLongInt( region.y2 ) * magnitude / units;

      printHandler.Write( "    (RECT %u %ld %ld %ld %ld)\n", layer, x1, y1, x2, y2 );
   }

   printHandler.Write( "    ; Tile Plane\n" );

   TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
   ptilePlane->tilePlaneIter_.Init( *this );
   TTPT_Tile_c< T >* ptile = 0;
   while( ptilePlane->tilePlaneIter_.Next( &ptile ))
   {
      const TGS_Region_c& region = ptile->GetRegion( );

      unsigned int layer = ( ptile->IsClear( ) ? laffLayerTileClear : laffLayerTileSolid );
      long int x1 = MinGrid.FloatToLongInt( region.x1 ) * magnitude / units;
      long int y1 = MinGrid.FloatToLongInt( region.y1 ) * magnitude / units;
      long int x2 = MinGrid.FloatToLongInt( region.x2 ) * magnitude / units;
      long int y2 = MinGrid.FloatToLongInt( region.y2 ) * magnitude / units;

      if( ptile->IsClear( ))
      {
         printHandler.Write( "    (RECT %u %ld %ld %ld %ld)\n", layer, x1, y1, x2, y2 );
      }
      else if( ptile->IsSolid( ) && !ptile->GetCount( ))
      {
         printHandler.Write( "    (RECT %u %ld %ld %ld %ld)\n", layer, x1, y1, x2, y2 );
      }
      else if( ptile->IsSolid( ) && ptile->GetCount( ))
      {
         unsigned int height = ( units >= 100 ? 16 : 4 );

         for( unsigned int i = 0; i < ptile->GetCount( ); ++i )
	 {
   	    const T& data = *ptile->FindData( i );

            string srData;
   	    data.ExtractString( &srData );

            printHandler.Write( "    (RECT %u %ld %ld %ld %ld (\n", layer, x1, y1, x2, y2 );
            printHandler.Write( "      (SNAM %u %u 1 %ld %ld '%s')))\n", layer, height, x1, y1 + i * units, TIO_SR_STR( srData ));
         }
      }
   }

   printHandler.Write( "  ))\n" );
   printHandler.Write( "  (BOUNDARY)\n" );
   printHandler.Write( "  (STRUCT)\n" );
   printHandler.Write( "  (PORTS)\n" );
   printHandler.Write( "  (NAMES)\n" );
   printHandler.Write( "  (SYMBOLIC)\n" );
   printHandler.Write( "  (DIAGNOSTIC)\n" );
   printHandler.Write( "))\n" );
}

//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::PrintLaff(
      const char* pszFileName ) const
{
   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   string srLogFileName = printHandler.GetLogFileName( );
  
   printHandler.DisableStdioOutput( );
   printHandler.SetLogFileOutput( pszFileName );

   this->PrintLaff( );

   printHandler.SetLogFileOutput( );
   printHandler.EnableStdioOutput( );

   const char* pszLogFileName = srLogFileName.data( );
   if( pszLogFileName && *pszLogFileName )
   {
      printHandler.SetLogFileOutput( pszLogFileName, 
                                     TIO_FILE_OPEN_APPEND );
   }
}

//===========================================================================//
// Method         : Init
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Init( 
      const TGS_Region_c& region )
{
   TGS_Region_c thisRegion = region;

   this->Reset( );

   this->region_ = thisRegion;

   TTPT_Tile_c< T > tile( TTP_TILE_CLEAR, region );
   TTPT_Tile_c< T >* ptile = this->Allocate_( tile );

   this->ptileLL_ = this->ptileMRC_ = this->ptileMRS_ = ptile;

   return( ptile ? true : false );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Init( 
      const TTPT_TilePlane_c< T >& tilePlane )
{
   bool ok = true;

   if( tilePlane.IsValid( ))
   {
      ok = this->Init( tilePlane.GetRegion( ));

      TTPT_TilePlaneIter_c< T > tilePlaneIter( tilePlane );
      TTPT_Tile_c< T >* ptile = 0;
      while( ok && tilePlaneIter.Next( &ptile, TTP_TILE_SOLID ))
      {
         ok = this->Add( *ptile );
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : Reset
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::Reset(
      void )
{
   if( this->ptileLL_ )
   {
      size_t tilePlaneCount = this->IsClear( ) ? 1 : 
                              this->FindCount( TTP_TILE_CLEAR ) +
                              this->FindCount( TTP_TILE_SOLID );
      if( tilePlaneCount == 1 ) 
      {
 	 // Handle simplest case of single tile (for fastest delete code)
         delete this->ptileLL_;
      }
      else if( tilePlaneCount <= 16 ) 
      {
 	 // Handle next case of small tile count (for fast delete code)
         TCT_OrderedVector_c< TTPT_Tile_c< T >* > tileList( tilePlaneCount );

         // Use recursion to enable "fast" deletion on "small" tile planes
         this->LoadTileList_( this->ptileLL_, &tileList );

         for( size_t i = 0; i < tileList.GetLength( ); ++i )
         {
            TTPT_Tile_c< T >* ptile_ = *tileList[i];
            delete ptile_;
         }
      }
      else
      {
	 // Handle worst case where we iterate to delete allocated memory
         this->tilePlaneIter_.Init( *this, this->region_ );
         TTPT_Tile_c< T >* ptile = 0;  
         while( this->tilePlaneIter_.Next( &ptile ))
         {
            // Update "most recent" that reference this tile prior to deleting tile
            if( this->ptileMRC_ == ptile )
            {
               this->ptileMRC_ = 0;
            }
            if( this->ptileMRS_ == ptile )
            {
               this->ptileMRS_ = 0;
            }

            // Update any tiles that reference this tile prior to deleting tile
            TTPT_Tile_c< T >* ptileLeftLower = ptile->GetStitchLeftLower( );
            while( ptileLeftLower && ( ptileLeftLower->GetStitchRightUpper( ) == ptile ))
            {
               ptileLeftLower->SetStitchRightUpper( 0 );
               ptileLeftLower = ptileLeftLower->GetStitchUpperRight( );
            }

            // Update any tiles that reference this tile prior to deleting tile
            TTPT_Tile_c< T >* ptileUpperRight = ptile->GetStitchUpperRight( );
            while( ptileUpperRight && ( ptileUpperRight->GetStitchLowerLeft( ) == ptile ))
            {
               ptileUpperRight->SetStitchLowerLeft( 0 );
               ptileUpperRight = ptileUpperRight->GetStitchLeftLower( );
            }

            delete ptile;
         }
      }
   }
   this->region_.Reset( );

   this->ptileLL_ = this->ptileMRC_ = this->ptileMRS_ = 0;

   this->count_.clear = 0;
   this->count_.solid = 0;
   this->count_.isValid = false;
}

//===========================================================================//
// Method         : Add
// Purpose        : Adds or inserts a solid tile into the current tile plane.
//                  This method handles adding a new solid tile if and only 
//                  if there are no solid tiles in the desired area.  This 
//                  method also handles adding a merged solid tile when the 
//                  new solid tile intersects with existing solid tiles.
//                  This method also handles adding an overlapping solid
//                  tile when the new solid tile overlaps with existing
//                  solid tiles
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Add(
      const TTPT_Tile_c< T >& tile,
            TTP_AddMode_t     addMode )
{
   return( this->Add( tile, TGS_ORIENT_VERTICAL, addMode ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Add( 
      const TTPT_TilePlane_c< T >& tilePlane,
            TTP_AddMode_t          addMode )
{
   bool ok = true;

   if( tilePlane.IsValid( ))
   {
      TTPT_TilePlaneIter_c< T > tilePlaneIter( tilePlane );
      TTPT_Tile_c< T >* ptile = 0;
      while( ok && tilePlaneIter.Next( &ptile, TTP_TILE_SOLID ))
      {
         ok = this->Add( *ptile, addMode );
      }
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Add(
      const TTPT_Tile_c< T >& tile,
            TGS_OrientMode_t  addOrient,
            TTP_AddMode_t     addMode )
{
   bool ok = true;

   const TGS_Region_c& tileRegion = tile.GetRegion( );
   if( tileRegion.IsLegal( ))
   {
      switch( addMode )
      {
      case TTP_ADD_REGION:

         ok = this->AddPerRegion_( tile, addOrient );
         break;

      case TTP_ADD_NEW:

         ok = this->AddPerNew_( tile, addOrient );
         break;

      case TTP_ADD_MERGE:

         ok = this->AddPerMerge_( tile, addOrient, addMode );
         break;

      case TTP_ADD_OVERLAP:
      case TTP_ADD_DIFFERENCE:

         ok = this->AddPerOverlap_( tile, addOrient, addMode );
         break;

      default:
         break;
      }
      this->count_.isValid = false;
   }
   else
   {
      ok = this->ShowInternalMessage_( TTP_MESSAGE_TILE_REGION_ILLEGAL,
                                       "TTPT_TilePlane_c::Add", &tileRegion );
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Add(
      const TGS_Region_c& region,
            TTP_AddMode_t addMode )
{
   TTPT_Tile_c< T > tile( TTP_TILE_SOLID, region );

   return( this->Add( tile, addMode ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Add(
      const TGS_Region_c&    region,
            TGS_OrientMode_t addOrient,
            TTP_AddMode_t    addMode )
{
   TTPT_Tile_c< T > tile( TTP_TILE_SOLID, region );

   return( this->Add( tile, addOrient, addMode ));
}


//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Add(
      const TGS_Region_c& region,
      const T&            data,
            TTP_AddMode_t addMode )
{
   TTPT_Tile_c< T > tile( TTP_TILE_SOLID, region, data );

   return( this->Add( tile, addMode ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Add(
      const TGS_Region_c&    region,
      const T&               data,
            TGS_OrientMode_t addOrient,
            TTP_AddMode_t    addMode )
{
   TTPT_Tile_c< T > tile( TTP_TILE_SOLID, region, data );

   return( this->Add( tile, addOrient, addMode ));
}

//===========================================================================//
// Method         : Delete
// Purpose        : Deletes a solid tile from the current tile plane.  The 
//                  solid "dead" tile is replaced by a clear tile.  The clear 
//                  "dead" tile is subsequently split and/or merged with 
//                  neighboring clear tiles, as possible.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Delete(
      TTPT_Tile_c< T >* ptile )
{
   bool ok = true;

   // Change the given solid tile into a clear "dead" tile
   ptile->MakeClear( );
   
   // Make copy of clear "dead" tile for iteration over right/left sides
   TTPT_Tile_c< T > deadTile( *ptile );
   
   if( ok )
   {
      ok = this->DeletePerRightSide_( deadTile );
   }
   if( ok )
   {
      ok = this->DeletePerLeftSide_( deadTile );
   }
   if( ok )
   {
      ok = this->DeletePerLeftSide_( deadTile );
   }
   if( ok )
   {
      ok = this->DeletePerLowerSide_( deadTile );
   }
   if( ok )
   {
      ok = this->DeletePerUpperSide_( deadTile );
   }
   this->count_.isValid = false;

   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Delete(
      const TGS_Region_c&    region,
            TTP_DeleteMode_t deleteMode )
{
   bool ok = true;

   if( region.IsLegal( ))
   {
      switch( deleteMode )
      {
      case TTP_DELETE_EXACT:

	 ok = this->DeletePerExact_( region );
         break;

      case TTP_DELETE_WITHIN:

         ok = this->DeletePerWithin_( region );
         break;
   
      case TTP_DELETE_INTERSECT:
   
	ok = this->DeletePerIntersect_( region );
         break;
   
      case TTP_DELETE_INTERSECT_MIN:
   
	 ok = this->DeletePerIntersectMin_( region );
         break;

      default:
         break;
      }
   }
   else
   {
      ok = this->ShowInternalMessage_( TTP_MESSAGE_TILE_REGION_ILLEGAL,
                                       "TTPT_TilePlane_c::Delete", &region );
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Delete(
      const TGS_Region_c&    region,
      const T&               data,
            TTP_DeleteMode_t deleteMode )
{
   bool ok = true;

   if( region.IsLegal( ))
   {
      switch( deleteMode )
      {
      case TTP_DELETE_EXACT:

 	 ok = this->DeletePerExact_( region, data );
         break;

      case TTP_DELETE_WITHIN:

	 ok = this->DeletePerWithin_( region, data );
         break;

      case TTP_DELETE_INTERSECT:

	 ok = this->DeletePerIntersect_( region, data );
         break;
   
      case TTP_DELETE_INTERSECT_MIN:
   
         ok = this->DeletePerIntersectMin_( region, data );
         break;

      default:
         break;
      }
   }
   else
   {
      ok = this->ShowInternalMessage_( TTP_MESSAGE_TILE_REGION_ILLEGAL,
                                       "TTPT_TilePlane_c::Delete", &region );
   }
   return( ok );
}

//===========================================================================//
// Method         : Clear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Clear(
      void )
{
   return( this->Init( this->region_ ));
}

//===========================================================================//
// Method         : Find
// Purpose        : Find and return a pointer to a tile based on the given 
//                  point (x,y) location.  The returned tile pointer may 
//                  identify be either a "clear" or a "solid" tile.  A null 
//                  pointer is returned if the given point is not valid (ie. 
//                  is not within the current tile plane's area).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::Find( 
      double            x, 
      double            y,
      TTPT_Tile_c< T >* ptile ) const
{
   TGS_Point_c point( x, y );

   return( this->Find( point, ptile ));
}

//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::Find( 
      const TGS_Point_c&      point,
            TTPT_Tile_c< T >* ptile ) const
{
   // Search until tile found that contains the given point
   ptile = this->FindPerPoint_( point, ptile );

   return( ptile );
}

//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::Find( 
      const TGS_Region_c&  region,
            TTP_FindMode_t findMode,
            TTP_TileMode_t tileMode ) const
{
   TTPT_Tile_c< T >* ptile = 0;

   switch( findMode )
   {
   case TTP_FIND_EXACT:

      ptile = this->FindPerExact_( region, tileMode );
      break;

   case TTP_FIND_WITHIN:

      ptile = this->FindPerWithin_( region, tileMode );
      break;

   case TTP_FIND_INTERSECT:

      ptile = this->FindPerIntersect_( region, tileMode );
      break;

   default:
      break;
   }
   return( ptile );
}

//===========================================================================//
// Method         : FindNearest
// Purpose        : Find and return a pointer to the nearest "solid" tile
//                  found, if any, based on the given reference region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history 
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindNearest(
      const TGS_Region_c&     refRegion,
      const TGS_OrientMode_t* porientMode ) const
{
   // Start by finding tile based on given initial search reference region
   TTPT_Tile_c< T >* ptile = this->Find( refRegion, TTP_FIND_INTERSECT );
   if( ptile && ptile->IsSolid( ))
   {
      // Nothing to do here, found "solid" tile based on given search region
      ;
   }
   else if( ptile && ptile->IsClear( ))
   {
      // Need to iterate sides to determine nearest potential "solid" tile
      TTPT_Tile_c< T >* psolidTile = this->FindNearestBySides_( refRegion, 
                                                                *ptile,
                                                                porientMode );
      if( psolidTile )
      {
         // Need to iterate region based on nearest potential "solid" tile
         double searchDistance = psolidTile->FindDistance( refRegion );
	 searchDistance += this->minGrid_;

         ptile = this->FindNearestByRegion_( refRegion, 
                                             searchDistance,
                                             porientMode );
      }
      else
      {
         // Need to reset tile pointer after search for "solid" tile, if any
         ptile = 0;
      }
   }
   return( ptile );
}

//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindNearest(
      const TGS_Region_c&     refRegion,
            double            searchDistance,
      const TGS_OrientMode_t* porientMode ) const
{
   // Start by finding tile based on given initial search reference region
   TTPT_Tile_c< T >* ptile = this->Find( refRegion, TTP_FIND_INTERSECT );
   if( ptile && ptile->IsSolid( ))
   {
      // Nothing to do here, found "solid" tile based on given search region
      ;
   }
   else if( ptile && ptile->IsClear( ))
   {
      ptile = this->FindNearestByRegion_( refRegion, 
                                          searchDistance,
                                          porientMode );
   }
   return( ptile );
}

//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindNearest(
      const TGS_Region_c& refRegion,
            TC_SideMode_t searchSide ) const
{
   // Start by finding tile based on given initial search reference region
   TGS_Region_c sideRegion;
   refRegion.FindRegion( searchSide, &sideRegion );

   TTPT_Tile_c< T >* ptile = this->Find( sideRegion, TTP_FIND_INTERSECT );
   if( ptile && ptile->IsSolid( ))
   {
      // Nothing to do here, found "solid" tile based on given search region
      ;
   }
   else if( ptile && ptile->IsClear( ))
   {
      // Need to iterate sides to determine nearest potential "solid" tile
      ptile = this->FindNearestBySide_( sideRegion, *ptile, searchSide );
   }
   return( ptile );
}

//===========================================================================//
// Method         : FindConnected
// Purpose        : Find and return all connected (ie. merge/join adjacents)
//                  regions based on the given region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::FindConnected(
      const TGS_Region_c&          region,
            TTPT_TilePlane_c< T >* ptilePlane ) const
{
   bool foundConnected = false;

   if( ptilePlane )
   {
      ptilePlane->Init( this->region_ );
      ptilePlane->Add( region );

      TGS_Region_c findRegion( region );
      TGS_Region_c mergedRegion;
      if( this->MergeAdjacents( findRegion, &mergedRegion, TTP_MERGE_REGION ))
      {
         ptilePlane->Add( mergedRegion, TTP_ADD_MERGE );

         findRegion = mergedRegion;

         foundConnected = true;
      }

      TGS_RegionList_t joinedRegionList;
      if( this->JoinAdjacents( findRegion, &joinedRegionList, TTP_JOIN_INTERSECT ))
      {
         for( size_t i = 0; i < joinedRegionList.GetLength( ); ++i )
         {
            ptilePlane->Add( *joinedRegionList[i], TTP_ADD_MERGE );
         }
         foundConnected = true;
      }
   }
   return( foundConnected );
}

//===========================================================================//
// Method         : FindAdjacents
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::FindAdjacents(
      const TGS_Region_c&          region,
            TTPT_TilePlane_c< T >* ptilePlane ) const
{
   TGS_Region_c searchRegion( this->region_ );

   return( this->FindAdjacents( searchRegion, region, ptilePlane ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::FindAdjacents(
      const TGS_Region_c&          searchRegion,
      const TGS_Region_c&          region,
            TTPT_TilePlane_c< T >* ptilePlane ) const
{
   bool foundAdjacents = false;

   if( ptilePlane )
   {
      ptilePlane->Init( this->region_ );

      TGS_Region_c intersectRegion( region );
      intersectRegion.ApplyIntersect( this->region_ );
      ptilePlane->Add( intersectRegion );

      // Iterate until no more adjacent regions found relative to tile plane
      bool addedAdjacents = true;
      while( addedAdjacents )
      {
         addedAdjacents = false;

         // Iterate for each tile region in tile plane
         TTPT_TilePlane_c< T > tilePlane( *ptilePlane );
         TTPT_TilePlaneIter_c< T > tilePlaneIter( tilePlane, searchRegion );
         TTPT_Tile_c< T >* ptile = 0;
         while( tilePlaneIter.Next( &ptile, TTP_TILE_SOLID ))
         {
	    TGS_Region_c iterRegion = ptile->GetRegion( );

            // Ignore any iterate regions that fall outside search region
            iterRegion.ApplyIntersect( searchRegion );
            if( !iterRegion.IsValid( ))
               continue;

            iterRegion.ApplyScale( this->minGrid_ );
            TTPT_TilePlaneIter_c< T > thisPlaneIter( *this, iterRegion );
            while( thisPlaneIter.Next( &ptile, TTP_TILE_SOLID ))
            {
               TGS_Region_c tileRegion = ptile->GetRegion( );

               // Ignore any tile regions that fall outside search region
               if( !tileRegion.IsIntersecting( searchRegion ))
                  continue;

               tileRegion.ApplyIntersect( searchRegion );
               if( !tileRegion.IsValid( ))
                  continue;

               if( ptilePlane->IsClear( TTP_IS_CLEAR_ANY, tileRegion ))
               {
                  ptilePlane->Add( tileRegion, TTP_ADD_MERGE );
                  addedAdjacents = true;
                  foundAdjacents = true;
               }
            }
         }
      }
   }
   return( foundAdjacents );
}

//===========================================================================//
// Method         : FindData
// Purpose        : Find and return a list of zero or more "solid" tile data,
//                  if any, based on the given reference point or region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history 
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::FindData(
      double                    x,
      double                    y,
      TCT_OrderedVector_c< T >* pdataList ) const
{
   TGS_Point_c point( x, y );

   return( this->FindData( point, pdataList ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::FindData(
      const TGS_Point_c&              point,
            TCT_OrderedVector_c< T >* pdataList ) const
{
   if( pdataList )
   {
      pdataList->Clear( );

      const TTPT_Tile_c< T >* ptile = this->Find( point );
      if( ptile && ptile->IsSolid( ))
      {
         for( unsigned int i = 0; i < ptile->GetCount( ); ++i )
         {
            const T* pdata = ptile->FindData( i );
            if( pdata && !pdataList->IsMember( *pdata ))
            {
               pdataList->Add( *pdata );
            }
         }
      }
   }
   return( pdataList && pdataList->IsValid( ) ? true : false );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::FindData(
      const TGS_Region_c&             region,
            TCT_OrderedVector_c< T >* pdataList ) const
{
   if( pdataList )
   {
      pdataList->Clear( );

      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this, region );
      TTPT_Tile_c< T >* ptile = 0;
      while( ptilePlane->tilePlaneIter_.Next( &ptile, TTP_TILE_SOLID ))
      {
         for( unsigned int i = 0; i < ptile->GetCount( ); ++i )
         {
            const T* pdata = ptile->FindData( i );
            if( pdata && !pdataList->IsMember( *pdata ))
            {
               pdataList->Add( *pdata );
            }
         }
      }
   }
   return( pdataList && pdataList->IsValid( ) ? true : false );
}

//===========================================================================//
// Method         : FindCount
// Purpose        : Return total number of clear or solid tiles defined
//                  within this tile plane.  The current count is cached and
//                  may be valid based on whether any tiles have been added 
//                  or deleted since the last count
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > size_t TTPT_TilePlane_c< T >::FindCount(
      TTP_TileMode_t tileMode ) const
{
   size_t count = 0;

   this->FindCountRefresh_( );

   switch( tileMode )
   {
   case TTP_TILE_CLEAR: count = this->count_.clear; break;
   case TTP_TILE_SOLID: count = this->count_.solid; break;
   default:                                         break;
   }
   return( count );
}

//===========================================================================//
template< class T > size_t TTPT_TilePlane_c< T >::FindCount(
      const TGS_Region_c&  region,
            TTP_TileMode_t tileMode ) const
{
   size_t count = 0;

   this->FindCountRefresh_( region );

   switch( tileMode )
   {
   case TTP_TILE_CLEAR: count = this->count_.clear; break;
   case TTP_TILE_SOLID: count = this->count_.solid; break;
   default:                                         break;
   }
   return( count );
}

//===========================================================================//
// Method         : FindRegion
// Purpose        : Return bounding region for all clear or solid tiles found
//                  within this tile plane.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::FindRegion(
      TTP_TileMode_t tileMode,
      TGS_Region_c*  pregion ) const
{
   if( pregion )
   {
      pregion->Reset( );

      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this );
      TTPT_Tile_c< T >* ptile = 0;
      while( ptilePlane->tilePlaneIter_.Next( &ptile, tileMode ))
      {
         const TGS_Region_c& tileRegion = ptile->GetRegion( );
         pregion->ApplyUnion( tileRegion );
      }
   }
}

//===========================================================================//
// Method         : Split
// Purpose        : Splits the given tile into two adjacent tiles based on
//                  the given split line.  This includes updating stitch
//                  pointers for neighboring tiles and for the new split
//                  tile.  This split requires the split line to be
//                  intersecting with the given tile.
//                  
//                  The following example illustrates splitting a tile (T)
//                  into two horizontally adjacent tiles (T) and (T'):
//                  
//                                     rt        ...............rt...
//                                      |        .               |  .
//                        +-------------*+       . +-------------*+ .
//                        |              *-tr    . |              *-tr
//                        |     (T)      |       . |     (T')     | .
//                        |              |     bl'-*            rt| .
//                        |              |       . +*------------|+ .
//                        |              |       . +|------------*+ .
//                        |              |       . |lb            *-tr'
//                        |              |       . |     (T)      | .
//                     bl-*              |      bl-*              | .
//                        +*-------------+       . +*-------------+ .
//                         |                        |
//                         lb                       lb
//                  
//                  The following example illustrates splitting a tile (T)
//                  into two vertically adjacent tiles (T) and (T'):
//                  
//                                     rt          .....rt'.....rt...
//                                      |                |       |  .
//                        +-------------*+         +-----*++-----*+ .
//                        |              *-tr      |      *-tr    *-tr
//                        |     (T)      |         | (T)  || (T') | .
//                        |              |         |      ||      | .
//                        |              |         |      ||      | .
//                        |              |         |      ||      | .
//                        |              |         |      ||      | .
//                        |              |         |      ||      | .
//                     bl-*              |      bl-*    bl-*      | .
//                        +*-------------+         +*-----++*-----+ .
//                         |                        |       |       .
//                         lb                      .lb......lb'......
//                  
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Split( 
            TTPT_Tile_c< T >* ptile,
      const TGS_Line_c&       line,
            TGS_DirMode_t     dirMode )
{
   bool ok = true;

   TGS_OrientMode_t orientMode = TGS_ORIENT_UNDEFINED;
   if( this->IsSplitable( *ptile, line, dirMode, &orientMode ))
   {
      // Copy tile (T) to (T'), then split into adjacent tiles
      // (ie. split tile into two horizontally or vertically adjacent tiles)
      TTPT_Tile_c< T >* ptileP = this->Allocate_( *ptile );
      if( ptileP )
      {
         // Based on split intersect orientation, update tile stitch pointers
         if( orientMode == TGS_ORIENT_HORIZONTAL )
         {
            // Update tile (T) y2 and (T') y1 coords and 'right_upper' & 'upper_right' stitches
            this->SplitRegionCoords_( ptile, ptileP, line, dirMode, orientMode );
            this->StitchToSplit_( ptile, ptileP, orientMode );

            TGS_CornerMode_t cornerMode = TGS_CORNER_UPPER_RIGHT;
            if( TCTF_IsLT( ptile->FindArea( ), ptileP->FindArea( )))
            {
       	       TTPT_Tile_c< T > tile( *ptile );
               TTPT_Tile_c< T > tileP( *ptileP );
      
               *ptile = tileP;
               ptile->SetStitchLeftLower( ptileP );
      
               *ptileP = tile;
               ptileP->SetStitchRightUpper( ptile );
       
               cornerMode = TGS_CORNER_LOWER_LEFT;
            }

            if( cornerMode == TGS_CORNER_LOWER_LEFT )
            {
               // Iterate left/right/lower sides to update neighbor tile stitches
               this->StitchFromSide_( ptileP, TC_SIDE_LEFT );
               this->StitchFromSide_( ptileP, TC_SIDE_RIGHT );
               this->StitchFromSide_( ptileP, TC_SIDE_LOWER );
            }
            if( cornerMode == TGS_CORNER_UPPER_RIGHT )
            {
               // Iterate left/right/upper sides to update neighbor tile stitches
               this->StitchFromSide_( ptileP, TC_SIDE_LEFT );
               this->StitchFromSide_( ptileP, TC_SIDE_RIGHT );
               this->StitchFromSide_( ptileP, TC_SIDE_UPPER );
   	    }
         }
         else // if( orientMode == TGS_ORIENT_VERTICAL )
         {
            // Update tile (T) x2 and (T') x1 coords and 'left_lower' & 'lower_left' stitches
            this->SplitRegionCoords_( ptile, ptileP, line, dirMode, orientMode );
            this->StitchToSplit_( ptile, ptileP, orientMode );

            TGS_CornerMode_t cornerMode = TGS_CORNER_UPPER_RIGHT;
            if( TCTF_IsLT( ptile->FindArea( ), ptileP->FindArea( )))
            {
 	       TTPT_Tile_c< T > tile( *ptile );
               TTPT_Tile_c< T > tileP( *ptileP );

               *ptile = tileP;
               ptile->SetStitchLowerLeft( ptileP );

               *ptileP = tile;
               ptileP->SetStitchUpperRight( ptile );
 
               cornerMode = TGS_CORNER_LOWER_LEFT;
            }

            if( cornerMode == TGS_CORNER_LOWER_LEFT )
            {
               // Iterate lower/upper/left sides to update neighbor tile stitches
               this->StitchFromSide_( ptileP, TC_SIDE_LOWER );
               this->StitchFromSide_( ptileP, TC_SIDE_UPPER );
               this->StitchFromSide_( ptileP, TC_SIDE_LEFT );
            }
            if( cornerMode == TGS_CORNER_UPPER_RIGHT )
            {
               // Iterate lower/upper/right sides to update neighbor tile stitches
               this->StitchFromSide_( ptileP, TC_SIDE_LOWER );
               this->StitchFromSide_( ptileP, TC_SIDE_UPPER );
               this->StitchFromSide_( ptileP, TC_SIDE_RIGHT );
   	    }
         }
         ok = true;
      }
      else
      {
         ok = false;
      }
   }
   else
   {
      ok = false;
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Split(
      TTPT_Tile_c< T >* ptile,
      TC_SideMode_t     side )
{
   bool ok = true;

   const TGS_Region_c& tileRegion = ptile->GetRegion( );

   TTPT_Tile_c< T >* pleftTile = ptile->GetStitchLowerLeft( );
   TTPT_Tile_c< T >* prightTile = ptile->GetStitchUpperRight( );

   switch( side )
   {
   case TC_SIDE_LEFT:

      // Split neighboring left tile based on vertical span formed with this tile
      if( pleftTile )
      {
         TGS_Region_c leftRegion = pleftTile->GetRegion( );
         if( TCTF_IsLT( leftRegion.y1, tileRegion.y1 ))
         {
            TGS_Line_c lowerIntersect( leftRegion.x1 - this->minGrid_, tileRegion.y1, 
                                       leftRegion.x2 + this->minGrid_, tileRegion.y1 );
            pleftTile = ptile->GetStitchLowerLeft( );
            if( pleftTile->IsClear( ))
	    {
               ok = this->Split( pleftTile, lowerIntersect, TGS_DIR_DOWN );
            }
         }
         if( TCTF_IsGT( leftRegion.y2, tileRegion.y2 ))
         {
            TGS_Line_c upperIntersect( leftRegion.x1 - this->minGrid_, tileRegion.y2, 
                                       leftRegion.x2 + this->minGrid_, tileRegion.y2 );
            pleftTile = ptile->GetStitchLowerLeft( );
            if( pleftTile->IsClear( ))
	    {
               ok = this->Split( pleftTile, upperIntersect, TGS_DIR_UP );
            }
	 }
	 else
	 {
	    pleftTile = this->Find( leftRegion.x2, tileRegion.y2, ptile );
	    if( pleftTile->IsClear( ))
	    {
	       leftRegion = pleftTile->GetRegion( );
	       if(( TCTF_IsGT( leftRegion.y2, tileRegion.y2 )) &&
	          ( TCTF_IsLE( leftRegion.y1, tileRegion.y2 )))
	       {
	             TGS_Line_c upperIntersect( leftRegion.x1 - this->minGrid_, tileRegion.y2, 
	                                        leftRegion.x2 + this->minGrid_, tileRegion.y2 );
	             ok = this->Split( pleftTile, upperIntersect, TGS_DIR_UP );
	       }
	    }
	 }
      }
      break;

   case TC_SIDE_RIGHT:

      // Split neighboring right tile based on vertical span formed with this tile
      if(( prightTile ) && 
         ( prightTile->GetMode( ) == ptile->GetMode( )))
      {
         TGS_Region_c rightRegion = prightTile->GetRegion( );
         if( TCTF_IsLT( rightRegion.y1, tileRegion.y1 ))
         {
            TGS_Line_c lowerIntersect( rightRegion.x1 - this->minGrid_, tileRegion.y1, 
                                       rightRegion.x2 + this->minGrid_, tileRegion.y1 );
            prightTile = ptile->GetStitchUpperRight( );
            if( prightTile->IsClear( ))
	    {
               ok = this->Split( prightTile, lowerIntersect, TGS_DIR_DOWN );
            }
         }
         if( TCTF_IsGT( rightRegion.y2, tileRegion.y2 ))
         {
            TGS_Line_c upperIntersect( rightRegion.x1 - this->minGrid_, tileRegion.y2, 
                                       rightRegion.x2 + this->minGrid_, tileRegion.y2 );
            prightTile = ptile->GetStitchUpperRight( );
            if( prightTile->IsClear( ))
	    {
               ok = this->Split( prightTile, upperIntersect, TGS_DIR_UP );
            }
	 }
	 else
	 {
	    prightTile = this->Find( rightRegion.x1, tileRegion.y2, ptile );
	    if( prightTile->IsClear( ))
	    {
	       rightRegion = prightTile->GetRegion( );
	       if(( TCTF_IsGT( rightRegion.y2, tileRegion.y2 )) &&
	          ( TCTF_IsLE( rightRegion.y1, tileRegion.y2 )))
	       {
	             TGS_Line_c upperIntersect( rightRegion.x1 - this->minGrid_, tileRegion.y2, 
	                                        rightRegion.x2 + this->minGrid_, tileRegion.y2 );
	             ok = this->Split( prightTile, upperIntersect, TGS_DIR_UP );
	       }
	    }
	 }
      }
      break;

   case TC_SIDE_LOWER:
   case TC_SIDE_UPPER:

      // No action taken since split by neighboring lower/upper tile is nonsense
      break;

   default:
      break;
   }
   return( ok );
}

//===========================================================================//
// Method         : Merge
// Purpose        : Merges the given two tiles into a single larger tile.
//                  This includes updating stitch pointers for neighboring
//                  tiles on three sides.  This merge requires both tiles be
//                  adjacent and have the same width/height.
//                  
//                  The following example illustrates merging two
//                  horizontally adjacent tiles (T) and (T') into a single
//                  tile T:
//                  
//                                     rt        ...............rt...      
//                                      |        .               |  .
//                        +-------------*+       . +-------------*+ .
//                        |              *-tr    . |              *-tr
//                        |     (T')     |       . |     (T)      | .
//                     bl-*            rt|       . |              | .
//                        +*------------|+       . |              | .
//                        +|------------*+       . |              | .
//                        |lb            *-tr    . |              | .
//                        |     (T)      |       . |              | .
//                     bl-*              |      bl-*              | .
//                        +*-------------+       . +*-------------+ .
//                         |                        |
//                         lb                       lb
//                  
//                  The following example illustrates merging two
//                  vertically adjacent tiles T and T' into a single tile T:
//                  
//                             rt      rt          .............rt...
//                              |       |                        |  .
//                        +-----*++-----*+         +-------------*+ .
//                        |      *-tr    *-tr      |              *-tr
//                        | (T)  || (T') |         |     (T)      | .
//                        |      ||      |         |              | .
//                        |      ||      |         |              | .
//                        |      ||      |         |              | .
//                        |      ||      |         |              | .
//                        |      ||      |         |              | .
//                     bl-*    bl-*      |      bl-*              | .
//                        +*-----++*-----+         +*-------------+ .
//                         |       |                |               .
//                         lb      lb              .lb...............
//
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Merge( 
      TTPT_Tile_c< T >* ptileA,
      TTPT_Tile_c< T >* ptileB )
{
   bool ok = true;

   TGS_OrientMode_t orientMode = TGS_ORIENT_UNDEFINED;
   if( this->IsMergable( *ptileA, *ptileB, &orientMode ))
   {
      // Found two horizontally or vertically adjacent tiles...
      // now decide less than/greater than tiles (T) and (T')
      TGS_OrientMode_t altOrient = ( orientMode == TGS_ORIENT_HORIZONTAL ?
                                     TGS_ORIENT_VERTICAL : TGS_ORIENT_HORIZONTAL );
      TTPT_Tile_c< T >* ptile = ptileA->FindLessThan( ptileB, altOrient );
      TTPT_Tile_c< T >* ptileP = ptileA->FindGreaterThan( ptileB, altOrient );
      TGS_CornerMode_t cornerMode = TGS_CORNER_UPPER_RIGHT;

      if( TCTF_IsLT( ptile->FindArea( ), ptileP->FindArea( )))
      {
         ptile = ptileA->FindGreaterThan( ptileB, altOrient );
         ptileP = ptileA->FindLessThan( ptileB, altOrient );
         cornerMode = TGS_CORNER_LOWER_LEFT;
      }

      if( orientMode == TGS_ORIENT_HORIZONTAL )
      {
         if( cornerMode == TGS_CORNER_LOWER_LEFT )
         {
            // Update tile (T) per tile (T') y1 coord and 'left_lower' & 'lower_left' stitches
   	    this->MergeRegionCoords_( ptile, ptileP, orientMode, cornerMode );
            this->StitchToMerge_( ptile, ptileP, orientMode, cornerMode );
   
            // Iterate left, right, and lower sides to update neighbor tile stitches
            this->StitchFromSide_( ptile, TC_SIDE_LEFT, ptileP );
            this->StitchFromSide_( ptile, TC_SIDE_RIGHT, ptileP );
            this->StitchFromSide_( ptile, TC_SIDE_LOWER );
         }
         if( cornerMode == TGS_CORNER_UPPER_RIGHT )
         {
            // Update tile (T) per tile (T') y2 coord and 'right_upper' & 'upper_right' stitches
   	    this->MergeRegionCoords_( ptile, ptileP, orientMode, cornerMode );
            this->StitchToMerge_( ptile, ptileP, orientMode, cornerMode );
   
            // Iterate left, right, and upper sides to update neighbor tile stitches
            this->StitchFromSide_( ptile, TC_SIDE_LEFT, ptileP );
            this->StitchFromSide_( ptile, TC_SIDE_RIGHT, ptileP );
            this->StitchFromSide_( ptile, TC_SIDE_UPPER );
         }
      }
      else // if( orientMode == TGS_ORIENT_VERTICAL )
      {
         if( cornerMode == TGS_CORNER_LOWER_LEFT )
         {
            // Update tile (T) per tile (T') x1 coord and 'left_lower' & 'lower_left' stitches
   	    this->MergeRegionCoords_( ptile, ptileP, orientMode, cornerMode );
            this->StitchToMerge_( ptile, ptileP, orientMode, cornerMode );
   
            // Iterate lower, upper, and left sides to update neighbor tile stitches
            this->StitchFromSide_( ptile, TC_SIDE_LOWER, ptileP );
            this->StitchFromSide_( ptile, TC_SIDE_UPPER, ptileP );
            this->StitchFromSide_( ptile, TC_SIDE_LEFT );
         }
         if( cornerMode == TGS_CORNER_UPPER_RIGHT )
         {
            // Update tile (T) per tile (T') x2 coord and 'right_upper' & 'upper_right' stitches
      	    this->MergeRegionCoords_( ptile, ptileP, orientMode, cornerMode );
            this->StitchToMerge_( ptile, ptileP, orientMode, cornerMode );
   
            // Iterate lower, upper, and right sides to update neighbor tile stitches
            this->StitchFromSide_( ptile, TC_SIDE_LOWER, ptileP );
            this->StitchFromSide_( ptile, TC_SIDE_UPPER, ptileP );
            this->StitchFromSide_( ptile, TC_SIDE_RIGHT );
         }
      }

      // Deallocate the merged tile (T') after merge complete
      this->Deallocate_( ptileP );

      ok = true;
   }
   else
   {
      ok = false;
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::Merge(
      TTPT_Tile_c< T >* ptile,
      TC_SideMode_t     side )
{
   bool ok = true;

   TTPT_Tile_c< T >* pleftTile = ptile->GetStitchLowerLeft( );
   TTPT_Tile_c< T >* prightTile = ptile->GetStitchUpperRight( );
   TTPT_Tile_c< T >* plowerTile = ptile->GetStitchLeftLower( );
   TTPT_Tile_c< T >* pupperTile = ptile->GetStitchRightUpper( );

   TGS_OrientMode_t orientMode = TGS_ORIENT_UNDEFINED;      

   switch( side )
   {
   case TC_SIDE_LEFT:

      // Merge neighboring left tile with this reference tile, if possible
      if(( pleftTile && this->IsMergable( *pleftTile, *ptile, &orientMode )) &&
         ( orientMode == TGS_ORIENT_VERTICAL ))
      {
         ok = this->Merge( pleftTile, ptile );
      }
      break;

   case TC_SIDE_RIGHT:

      // Merge neighboring right tile with this reference tile, if possible
      if(( prightTile && this->IsMergable( *prightTile, *ptile, &orientMode )) &&
         ( orientMode == TGS_ORIENT_VERTICAL ))
      {
         ok = this->Merge( prightTile, ptile );
      }
      break;

   case TC_SIDE_LOWER:

      // Merge neighboring lower tile with this reference tile, if possible
      if(( plowerTile && this->IsMergable( *plowerTile, *ptile, &orientMode )) &&
         ( orientMode == TGS_ORIENT_HORIZONTAL ))
      {
         ok = this->Merge( plowerTile, ptile );
      }
      break;

   case TC_SIDE_UPPER:

      // Merge neighboring upper tile with this reference tile, if possible
      if(( pupperTile && this->IsMergable( *pupperTile, *ptile, &orientMode )) &&
         ( orientMode == TGS_ORIENT_HORIZONTAL ))
      {
         ok = this->Merge( pupperTile, ptile );
      }
      break;

   default:
      break;
   }
   return( ok );
}

//===========================================================================//
// Method         : MergeAdjacents
// Purpose        : Returns a merged region based on the given tile and any 
//                  neighboring tiles.  A merged region is the largest
//                  possible region that can be constructed that completely
//                  includes the given tile's region.
//                  
//                  For example, given the following two tiles T and T'
//                  shown below on the right the tile T will be merged with
//                  T' to return the region shown on the left.
//                  
//                     +-----++------+         +-------------+
//                     | (T) || (T') |         |             |
//                     |     ||      |   ==>   |             |
//                     +-----+|      |         +-------------+
//                            |      |                .      .
//                            +------+                ........
//                  
//                  For example, given the following three tiles T, T', and
//                  T" shown below on the right the tile T will be merged
//                  with T' to return the region shown on the left.
//
//                     +---------+              ....+----+
//                     |  (T')   |              .   |    |
//                     +---------+              ....|    |
//                         +-----+                  |    |
//                         | (T) |        ==>       |    |
//                         +-----+                  |    |
//                         +----------+             |    |.....
//                         |   (T")   |             |    |    .
//                         +----------+             +----+.....
//
//                  Merging the tiles T and T' shown below on the right
//                  will be merged into the larger tile on the left based
//                  on the fully overlapping common edge between T and T'.
//                  
//                     +-----+            +-----+
//                     | (T) |            |     |
//                     +-----+      ==>   |     |
//                     +--------+         |     |...
//                     |  (T')  |         |     |  .
//                     +--------+         +-----+...
//                  
//                  Merging the tiles T and T' shown below on the right
//                  will not be merged since the two tiles do not have a
//                  fully overlapping common edge between T and T'.
//
//                     +-----+            +-----+
//                     | (T) |            |     |
//                     +-----+      ==>   +-----+
//                     +--------+         +--------+
//                     |  (T')  |         |        |
//                     +--------+         +--------+
//                  
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::MergeAdjacents(
      const TGS_Point_c&     point,
            TGS_Region_c*    pregion,
            TGS_OrientMode_t orientMode,
            TTP_MergeMode_t  mergeMode,
            TTP_TileMode_t   tileMode ) const
{
   bool mergedAdjacents = false;

   const TTPT_Tile_c< T >* ptile = this->Find( point );
   if( ptile )
   {
      mergedAdjacents = this->MergeAdjacents( *ptile, pregion, 
                                              mergeMode, orientMode, tileMode );
   }
   return( mergedAdjacents );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::MergeAdjacents(
      const TGS_Region_c&    region,
            TGS_Region_c*    pregion,
            TGS_OrientMode_t orientMode,
            TTP_MergeMode_t  mergeMode,
            TTP_TileMode_t   tileMode ) const
{
   bool mergedAdjacents = false;

   const TTPT_Tile_c< T >* ptile = 0;

   ptile = this->Find( region, TTP_FIND_EXACT, tileMode );
   if( !ptile )
   {
      ptile = this->Find( region, TTP_FIND_WITHIN, tileMode );
   }
   if( !ptile )
   {
      TTPT_TilePlaneIter_c< T > thisIter( *this, region );
      TTPT_Tile_c< T >* pmergeTile = 0;
      while( thisIter.Next( &pmergeTile, tileMode ))
      {
	 TGS_Region_c mergeRegion;
         if( this->MergeAdjacents( *pmergeTile, &mergeRegion, 
                                   mergeMode, orientMode ) &&
             mergeRegion.IsWithin( region ))
         {
	    ptile = pmergeTile;
	    break;
         }
      }
   }

   if( ptile )
   {
      mergedAdjacents = this->MergeAdjacents( *ptile, pregion, 
                                              mergeMode, orientMode ); 
   }
   return( mergedAdjacents );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::MergeAdjacents(
      const TTPT_Tile_c< T >& tile,
            TGS_Region_c*     pregion,
            TGS_OrientMode_t  orientMode,
            TTP_MergeMode_t   mergeMode ) const
{
   bool mergedAdjacents = false;

   unsigned int tileCount = tile.GetCount( );
   const T* ptileData = tile.GetData( );
   TTP_TileMode_t tileMode = tile.GetMode( );
   const TGS_Region_c& tileRegion = tile.GetRegion( );

   TGS_Region_c mergedRegion( tileRegion );

   TGS_OrientMode_t defOrient = ( orientMode != TGS_ORIENT_VERTICAL ?
                                  TGS_ORIENT_HORIZONTAL : TGS_ORIENT_VERTICAL );
   TGS_OrientMode_t altOrient = ( orientMode == TGS_ORIENT_VERTICAL ?
                                  TGS_ORIENT_HORIZONTAL : TGS_ORIENT_VERTICAL );
   if( !mergedAdjacents )
   {
      TGS_Region_c mergedHorzMaxRegion( tileRegion );
      this->MergeAdjacentsByExtent_( defOrient, tileCount, ptileData, tileMode,
                                     mergeMode, &mergedHorzMaxRegion );
      if( TCTF_IsGT( mergedHorzMaxRegion.FindArea( ), mergedRegion.FindArea( )))
      {
         mergedRegion = mergedHorzMaxRegion;
      }
      else if( TCTF_IsEQ( mergedHorzMaxRegion.FindArea( ), mergedRegion.FindArea( )) &&
               TCTF_IsEQ( mergedHorzMaxRegion.FindArea( ), 0.0 ) &&
               TCTF_IsGT( mergedHorzMaxRegion.FindMax( ), mergedRegion.FindMax( )))
      {
         mergedRegion = mergedHorzMaxRegion;
      }

      if( tileRegion != mergedRegion )
      {
         mergedAdjacents = true;
      }
   }
   if( mergedAdjacents )
   {
      TGS_Region_c mergedVertMaxRegion( tileRegion );
      this->MergeAdjacentsByExtent_( altOrient, tileCount, ptileData, tileMode,
                                     mergeMode, &mergedVertMaxRegion );
      if( TCTF_IsGT( mergedVertMaxRegion.FindArea( ), mergedRegion.FindArea( )))
      {
         mergedRegion = mergedVertMaxRegion;
      }
      else if( TCTF_IsEQ( mergedVertMaxRegion.FindArea( ), mergedRegion.FindArea( )) &&
               TCTF_IsEQ( mergedVertMaxRegion.FindArea( ), 0.0 ) &&
               TCTF_IsGT( mergedVertMaxRegion.FindMax( ), mergedRegion.FindMax( )))
      {
         mergedRegion = mergedVertMaxRegion;
      }
   }

   if( pregion )
   {
      *pregion = mergedRegion;
   }
   return( mergedAdjacents );
}

//===========================================================================//
// Method         : JoinAdjacents
// Purpose        : Returns a list of joined regions based on the given tile
//                  and any neighboring tiles.  A joined region is the common
//                  region that can be constructed that joins or connects the
//                  given tile's region.
//                  
//                  For example, given the following two tiles T and T' shown
//                  below on the right the tile T will be joined with T' to
//                  return the region shown on the left based on their common
//                  overlapping edge.
//                  
//                     +-----+             ...+--+
//                     | (T) |             .  |  |
//                     +-----+       ==>   ...|  |
//                        +------+            |  |....
//                        | (T') |            |  |   .
//                        +------+            +--+....
//                  
//                  For example, given the following three tiles T, T', and
//                  T" shown below on the right the tile T will be joined 
//                  with T' and with T" to return the two regions shown on 
//                  the left based on their common overlapping edges.
//                  
//                        +------+   +----+             +------+   +----+
//                        | (T') |   |(T")|             |      |   |    |
//                        +------+   +----+     ==>     |      |   |    |
//                     +--------------------+         ..|      |...|    |...
//                     |          (T)       |         . |      |   |    |  .
//                     |                    |         . |      |   |    |  .
//                     +--------------------+         ..+------+...+----+...
//                  
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::JoinAdjacents(
      const TGS_Point_c&      point,
            TGS_RegionList_t* pregionList ) const
{
   bool joinedAdjacents = false;

   const TTPT_Tile_c< T >* ptile = this->Find( point );
   if( ptile )
   {
      joinedAdjacents = this->JoinAdjacents( *ptile, pregionList ); 
   }
   return( joinedAdjacents );
}

//===========================================================================//
// Method         : JoinAdjacents
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::JoinAdjacents(
      const TGS_Region_c&     region,
            TGS_RegionList_t* pregionList,
            TTP_JoinMode_t    joinMode ) const
{
   bool joinedAdjacents = false;

   if( joinMode == TTP_JOIN_EXACT )
   {
      joinedAdjacents = this->JoinAdjacentsPerExact_( region, pregionList );
   }
   else if( joinMode == TTP_JOIN_INTERSECT )
   {
      joinedAdjacents = this->JoinAdjacentsPerIntersect_( region, pregionList );
   }
   return( joinedAdjacents );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::JoinAdjacents(
      const TTPT_Tile_c< T >& tile,
            TGS_RegionList_t* pregionList ) const
{
   bool joinAdjacents = false;

   TGS_Region_c joinRegion;

   TGS_Region_c region = tile.GetRegion( );
   unsigned int count = tile.GetCount( );
   const T* pdata = tile.GetData( );
   TTP_TileMode_t tileMode = tile.GetMode( );

   for( TTPT_Tile_c< T >* pleftTile = tile.GetStitchLowerLeft( );
        pleftTile;
        pleftTile = pleftTile->GetStitchRightUpper( ))
   {
      if( TCTF_IsGT( pleftTile->GetRegion( ).y1, region.y2 ))
	 break;

      if(( pleftTile->GetMode( ) == tileMode ) &&
         ( pleftTile->IsEqualData( count, pdata )))
      {
         joinRegion.ApplyAdjacent( pleftTile->GetRegion( ), region, this->minGrid_ );
         if( !joinRegion.IsValid( ))
            continue;

         joinAdjacents = this->JoinAdjacentsBySide_( TC_SIDE_LEFT, joinRegion,
                                                     count, pdata, tileMode, 
                                                     pregionList );
         if( !joinAdjacents )
         {
            if( !pregionList->IsMember( joinRegion ))
            {
               pregionList->Add( joinRegion );
            }
            joinAdjacents = true;
         }
      }
   }

   for( TTPT_Tile_c< T >* prightTile = tile.GetStitchUpperRight( );
        prightTile;
        prightTile = prightTile->GetStitchLeftLower( ))
   {
      if( TCTF_IsLT( prightTile->GetRegion( ).y2, region.y1 ))
	 break;

      if(( prightTile->GetMode( ) == tileMode ) &&
         ( prightTile->IsEqualData( count, pdata )))
      {
         joinRegion.ApplyAdjacent( prightTile->GetRegion( ), region, this->minGrid_ );
         if( !joinRegion.IsValid( ))
            continue;

         joinAdjacents = this->JoinAdjacentsBySide_( TC_SIDE_RIGHT, joinRegion,
                                                     count, pdata, tileMode, 
                                                     pregionList );
         if( !joinAdjacents )
         {
            if( !pregionList->IsMember( joinRegion ))
            {
               pregionList->Add( joinRegion );
            }
            joinAdjacents = true;
         }
      }
   }

   for( TTPT_Tile_c< T >* plowerTile = tile.GetStitchLeftLower( );
        plowerTile;
        plowerTile = plowerTile->GetStitchUpperRight( ))
   {
      if( TCTF_IsGT( plowerTile->GetRegion( ).x1, region.x2 ))
	 break;

      if(( plowerTile->GetMode( ) == tileMode ) &&
         ( plowerTile->IsEqualData( count, pdata )))
      {
         joinRegion.ApplyAdjacent( plowerTile->GetRegion( ), region, this->minGrid_ );
         if( !joinRegion.IsValid( ))
            continue;

         joinAdjacents = this->JoinAdjacentsBySide_( TC_SIDE_LOWER, joinRegion,
                                                     count, pdata, tileMode, 
                                                     pregionList );
         if( !joinAdjacents )
         {
            if( !pregionList->IsMember( joinRegion ))
            {
               pregionList->Add( joinRegion );
            }
            joinAdjacents = true;
         }
      }
   }

   for( TTPT_Tile_c< T >* pupperTile = tile.GetStitchRightUpper( );
        pupperTile;
        pupperTile = pupperTile->GetStitchLowerLeft( ))
   {
      if( TCTF_IsLT( pupperTile->GetRegion( ).x2, region.x1 ))
	 break;

      if(( pupperTile->GetMode( ) == tileMode ) &&
         ( pupperTile->IsEqualData( count, pdata )))
      {
         joinRegion.ApplyAdjacent( pupperTile->GetRegion( ), region, this->minGrid_ );
         if( !joinRegion.IsValid( ))
            continue;

         joinAdjacents = this->JoinAdjacentsBySide_( TC_SIDE_UPPER, joinRegion,
                                                     count, pdata, tileMode, 
                                                     pregionList );
         if( !joinAdjacents )
         {
            if( !pregionList->IsMember( joinRegion ))
            {
               pregionList->Add( joinRegion );
            }
            joinAdjacents = true;
         }
      }
   }
   return( joinAdjacents );
}

//===========================================================================//
// Method         : HasAdjacents
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::HasAdjacents(
      const TGS_Region_c& region ) const
{
   bool hasAdjacents = true;

   const TTPT_Tile_c< T >* ptile = this->Find( region );
   if( ptile )
   {
      TTP_TileMode_t tileMode = ptile->GetMode( );

      hasAdjacents = false;
      
      TGS_Point_c leftPoint( region.x1 - this->minGrid_, region.y1 );
      while( !hasAdjacents && TCTF_IsLE( leftPoint.y, region.y2 ))
      {
         TTPT_Tile_c< T >* pleftTile = this->Find( leftPoint );
         if( !pleftTile )
            break;

         hasAdjacents = ( pleftTile->GetMode( ) == tileMode ? true : false );
         leftPoint.y = pleftTile->GetRegion( ).y2 + this->minGrid_;
      }

      TGS_Point_c rightPoint( region.x2 + this->minGrid_, region.y2 );
      while( !hasAdjacents && TCTF_IsGE( rightPoint.y, region.y1 ))
      {
         TTPT_Tile_c< T >* prightTile = this->Find( rightPoint );
         if( !prightTile )
            break;

         hasAdjacents = ( prightTile->GetMode( ) == tileMode ? true : false );
         rightPoint.y = prightTile->GetRegion( ).y1 - this->minGrid_;
      }

      TGS_Point_c lowerPoint( region.x1, region.y1 - this->minGrid_ );
      while( !hasAdjacents && TCTF_IsLE( lowerPoint.x, region.x2 ))
      {
         TTPT_Tile_c< T >* plowerTile = this->Find( lowerPoint );
         if( !plowerTile )
            break;

         hasAdjacents = ( plowerTile->GetMode( ) == tileMode ? true : false );
         lowerPoint.x = plowerTile->GetRegion( ).x2 + this->minGrid_;
      }

      TGS_Point_c upperPoint( region.x2, region.y2 + this->minGrid_ );
      while( !hasAdjacents && TCTF_IsGE( upperPoint.x, region.x1 ))
      {
         TTPT_Tile_c< T >* pupperTile = this->Find( upperPoint );
         if( !pupperTile )
            break;

         hasAdjacents = ( pupperTile->GetMode( ) == tileMode ? true : false );
         upperPoint.x = pupperTile->GetRegion( ).x1 - this->minGrid_;
      }
   }  
   return( hasAdjacents );
}

//===========================================================================//
// Method         : HasNeighbor
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::HasNeighbor(
      const TGS_Region_c& region,
            double        minDistance ) const
{
   bool hasNeighbor = false;

   TTPT_Tile_c< T >* ptile = this->Find( region, TTP_FIND_EXACT );
   if( ptile && ptile->IsSolid( ))
   {
      hasNeighbor = this->HasNeighbor( *ptile, minDistance );
   }
   else
   {
      hasNeighbor = true;
   }
   return( hasNeighbor );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::HasNeighbor(
      const TTPT_Tile_c< T >& tile,
            double            minDistance ) const
{
   bool hasNeighbor = false;

   const TGS_Region_c& tileRegion = tile.GetRegion( );

   if( !hasNeighbor )
   {
      const TTPT_Tile_c< T >* plowerTile = tile.GetStitchLeftLower( );
      if( plowerTile && plowerTile->IsSolid( ))
      {
         hasNeighbor = true;
      }
      else if( plowerTile && plowerTile->IsClear( ))
      {
         const TGS_Region_c& lowerRegion = plowerTile->GetRegion( );
         if( TCTF_IsGT( lowerRegion.x1, tileRegion.x1 - minDistance ) ||
             TCTF_IsLT( lowerRegion.x2, tileRegion.x2 + minDistance ) ||
             TCTF_IsGT( lowerRegion.y1, tileRegion.y1 - minDistance ))
         {
            TGS_Region_c searchRegion( tileRegion.x1 - minDistance,
                                       tileRegion.y1 - minDistance,
                                       tileRegion.x2 + minDistance,
                                       lowerRegion.y1 - this->minGrid_ );
            hasNeighbor = this->IsSolid( TTP_IS_SOLID_ANY, searchRegion );
         }
      }
   }

   if( !hasNeighbor )
   {
      const TTPT_Tile_c< T >* pupperTile = tile.GetStitchRightUpper( );
      if( pupperTile && pupperTile->IsSolid( ))
      {
         hasNeighbor = true;
      }
      else if( pupperTile && pupperTile->IsClear( ))
      {
         const TGS_Region_c& upperRegion = pupperTile->GetRegion( );
         if( TCTF_IsGT( upperRegion.x1, tileRegion.x1 - minDistance ) ||
             TCTF_IsLT( upperRegion.x2, tileRegion.x2 + minDistance ) ||
             TCTF_IsLT( upperRegion.y2, tileRegion.y2 + minDistance ))
         {
            TGS_Region_c searchRegion( tileRegion.x1 - minDistance,
                                       upperRegion.y2 + this->minGrid_,
                                       tileRegion.x2 + minDistance,
            		               tileRegion.y2 + minDistance );
            hasNeighbor = this->IsSolid( TTP_IS_SOLID_ANY, searchRegion );
         }
      }
   }

   if( !hasNeighbor )
   {
      for( TTPT_Tile_c< T >* pleftTile = tile.GetStitchLowerLeft( );
           pleftTile;
           pleftTile = pleftTile->GetStitchRightUpper( ))
      {
         const TGS_Region_c& leftRegion = pleftTile->GetRegion( );
         if( TCTF_IsGT( leftRegion.y1, tileRegion.y2 ))
   	    break;

         if( pleftTile->IsSolid( ))
         {
            hasNeighbor = true;
         }
         else if( pleftTile->IsClear( ))
         {
            if( TCTF_IsGT( leftRegion.x1, tileRegion.x1 - minDistance ))
            {
               hasNeighbor = true;
            }
         }
         if( hasNeighbor )
            break;
      }
   }

   if( !hasNeighbor )
   {
      for( TTPT_Tile_c< T >* prightTile = tile.GetStitchUpperRight( );
           prightTile;
           prightTile = prightTile->GetStitchLeftLower( ))
      {
         const TGS_Region_c& rightRegion = prightTile->GetRegion( );
         if( TCTF_IsLT( rightRegion.y2, tileRegion.y1 ))
   	    break;

         if( prightTile->IsSolid( ))
         {
            hasNeighbor = true;
         }
         else if( prightTile->IsClear( ))
         {
            if( TCTF_IsLT( rightRegion.x2, tileRegion.x2 + minDistance ))
            {
               hasNeighbor = true;
            }
         }
         if( hasNeighbor )
            break;
      }
   }
   return( hasNeighbor );
}

//===========================================================================//
// Method         : IsClear
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsClear(
      void ) const
{
   bool isClear = false;

   if( this->ptileLL_ &&
       this->ptileLL_->IsClear( ) &&
       !this->ptileLL_->GetStitchLowerLeft( ) &&
       !this->ptileLL_->GetStitchLeftLower( ) &&
       !this->ptileLL_->GetStitchRightUpper( ) &&
       !this->ptileLL_->GetStitchUpperRight( ))
   {
      isClear = true;
   }
   return( isClear );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsClear(
      double x,
      double y ) const
{
   TGS_Point_c point( x, y );

   return( this->IsClear( point ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsClear(
      const TGS_Point_c& point ) const
{
   return( this->IsClearMatch_( point ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsClear(
      const TGS_Region_c& region ) const
{
   return( this->IsClearMatch_( region ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsClear(
            TTP_IsClearMode_t  isClearMode,
      const TGS_Region_c&      region,
            TTPT_Tile_c< T >** ppclearTile ) const
{
   bool isClear = false;

   switch( isClearMode )
   {
   case TTP_IS_CLEAR_MATCH:

      isClear = this->IsClearMatch_( region, ppclearTile );
      break;

   case TTP_IS_CLEAR_ANY:

      isClear = this->IsClearAny_( region, ppclearTile );
      break;

   case TTP_IS_CLEAR_ALL:

      isClear = this->IsClearAll_( region, ppclearTile );
      break;

   default:
      break;
   }
   return( isClear );
}

//===========================================================================//
// Method         : IsSolid
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
      double x,
      double y ) const
{
   TGS_Point_c point( x, y );

   return( this->IsSolid( point ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
      const TGS_Point_c& point ) const
{
   return( this->IsSolidMatch_( point ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
      const TGS_Point_c& point,
      const T&           data ) const
{
   return( this->IsSolidMatch_( point, 1, &data ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
      const TGS_Point_c& point,
            unsigned int count,
      const T*           pdata ) const
{
   return( this->IsSolidMatch_( point, count, pdata ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
      const TGS_Region_c& region ) const
{
   return( this->IsSolidMatch_( region ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
      const TGS_Region_c& region,
      const T&            data ) const
{
   return( this->IsSolidMatch_( region, 1, &data ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
      const TGS_Region_c& region,
            unsigned int  count,
      const T*            pdata ) const
{
   return( this->IsSolidMatch_( region, count, pdata ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
      TTP_IsSolidMode_t  isSolidMode,
      TTPT_Tile_c< T >** ppsolidTile ) const
{
   const TGS_Region_c& region = this->GetRegion( );

   return( this->IsSolid( isSolidMode, region, ppsolidTile ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
            TTP_IsSolidMode_t  isSolidMode,
      const TGS_Region_c&      region,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   bool isSolid = false;

   switch( isSolidMode )
   {
   case TTP_IS_SOLID_MATCH:

      isSolid = this->IsSolidMatch_( region, ppsolidTile );
      break;

   case TTP_IS_SOLID_ANY:

      isSolid = this->IsSolidAny_( region, ppsolidTile );
      break;

   case TTP_IS_SOLID_ALL:

      isSolid = this->IsSolidAll_( region, ppsolidTile );
      break;

   case TTP_IS_SOLID_MAX:

      isSolid = this->IsSolidMax_( region, ppsolidTile );
      break;

   default:
      break;
   }
   return( isSolid );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
            TTP_IsSolidMode_t  isSolidMode,
      const TGS_Region_c&      region,
      const T&                 data,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   bool isSolid = false;

   switch( isSolidMode )
   {
   case TTP_IS_SOLID_MATCH:

      isSolid = this->IsSolidMatch_( region, 1, &data, ppsolidTile );
      break;

   case TTP_IS_SOLID_ANY:

      isSolid = this->IsSolidAny_( region, 1, &data, ppsolidTile );
      break;

   case TTP_IS_SOLID_ALL:

      isSolid = this->IsSolidAll_( region, 1, &data, ppsolidTile );
      break;

   default:
      break;
   }
   return( isSolid );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolid(
            TTP_IsSolidMode_t  isSolidMode,
      const TGS_Region_c&      region,
            unsigned int       count,
      const T*                 pdata,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   bool isSolid = false;

   switch( isSolidMode )
   {
   case TTP_IS_SOLID_MATCH:

      isSolid = this->IsSolidMatch_( region, count, pdata, ppsolidTile );
      break;

   case TTP_IS_SOLID_ANY:

      isSolid = this->IsSolidAny_( region, count, pdata, ppsolidTile );
      break;

   case TTP_IS_SOLID_ALL:

      isSolid = this->IsSolidAll_( region, count, pdata, ppsolidTile );
      break;

   default:
      break;
   }
   return( isSolid );
}

//===========================================================================//
// Method         : IsSolidNot
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNot(
      const TGS_Point_c& point,
      const T&           data ) const
{
   return( this->IsSolidNotMatch_( point, 1, &data ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNot(
      const TGS_Point_c& point,
            unsigned int count,
      const T*           pdata ) const
{
   return( this->IsSolidNotMatch_( point, count, pdata ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNot(
      const TGS_Region_c& region,
      const T&            data ) const
{
   return( this->IsSolidNotMatch_( region, 1, &data ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNot(
      const TGS_Region_c& region,
            unsigned int  count,
      const T*            pdata ) const
{
   return( this->IsSolidNotMatch_( region, count, pdata ));
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNot(
            TTP_IsSolidMode_t  isSolidNotMode,
      const TGS_Region_c&      region,
      const T&                 data,
            TTPT_Tile_c< T >** ppSolidNotTile ) const
{
   bool isSolidNot = false;

   switch( isSolidNotMode )
   {
   case TTP_IS_SOLID_MATCH:

      isSolidNot = this->IsSolidNotMatch_( region, 1, &data, ppSolidNotTile );
      break;

   case TTP_IS_SOLID_ANY:

      isSolidNot = this->IsSolidNotAny_( region, 1, &data, ppSolidNotTile );
      break;

   case TTP_IS_SOLID_ALL:

      isSolidNot = this->IsSolidNotAll_( region, 1, &data, ppSolidNotTile );
      break;

   default:
      break;
   }
   return( isSolidNot );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNot(
            TTP_IsSolidMode_t  isSolidNotMode,
      const TGS_Region_c&      region,
            unsigned int       count,
      const T*                 pdata,
            TTPT_Tile_c< T >** ppSolidNotTile ) const
{
   bool isSolidNot = false;

   switch( isSolidNotMode )
   {
   case TTP_IS_SOLID_MATCH:

      isSolidNot = this->IsSolidNotMatch_( region, count, pdata, ppSolidNotTile );
      break;

   case TTP_IS_SOLID_ANY:

      isSolidNot = this->IsSolidNotAny_( region, count, pdata, ppSolidNotTile );
      break;

   case TTP_IS_SOLID_ALL:

      isSolidNot = this->IsSolidNotAll_( region, count, pdata, ppSolidNotTile );
      break;

   default:
      break;
   }
   return( isSolidNot );
}

//===========================================================================//
// Method         : IsIntersecting
// Purpose        : Returns true if the given tile region is intersecting.  
//                  Optionally returns the first intersecting tile found.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsIntersecting(
      const TGS_Region_c&  region,
            TGS_Region_c*  pintersectRegion,
            TTP_TileMode_t tileMode ) const
{
   bool isIntersecting = false;

   TTPT_Tile_c< T >* ptile = this->FindPerIntersect_( region, tileMode );
   if( ptile )
   {
      isIntersecting = true;

      if( pintersectRegion )
      {
         *pintersectRegion = ptile->GetRegion( );
      }
   }
   return( isIntersecting );
} 

//===========================================================================//
// Method         : IsAdjacent
// Purpose        : Returns true if the given tile regions are adjacent.  
//                  Optionally returns the first tile's adjacent side with 
//                  respect to the second tile.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsAdjacent(
      const TGS_Region_c&  regionA,
      const TGS_Region_c&  regionB,
            TC_SideMode_t* pside ) const
{
   TC_SideMode_t side = TC_SIDE_UNDEFINED;

   if( TCTF_IsEQ( regionA.x1, regionB.x2 + this->minGrid_ ))
   {
      side = TC_SIDE_LEFT;
   }
   else if( TCTF_IsEQ( regionA.x2, regionB.x1 - this->minGrid_ ))
   {
      side = TC_SIDE_RIGHT;
   }

   if( TCTF_IsEQ( regionA.y1, regionB.y2 + this->minGrid_ ))
   {
      side = TC_SIDE_LOWER;
   }
   else if( TCTF_IsEQ( regionA.y2, regionB.y1 - this->minGrid_ ))
   {
      side = TC_SIDE_UPPER;
   }

   if( pside )
   {
      *pside = side;
   }
   return( side != TC_SIDE_UNDEFINED ? true : false );
}
   
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsAdjacent(
      const TTPT_Tile_c< T >& tileA,
      const TTPT_Tile_c< T >& tileB,
            TC_SideMode_t*   pside ) const
{
   const TGS_Region_c& regionA = tileA.GetRegion( );
   const TGS_Region_c& regionB = tileB.GetRegion( );

   return( this->IsAdjacent( regionA, regionB, pside ));
}

//===========================================================================//
// Method         : IsMergable
// Purpose        : Returns true if the given two tiles can be merged into a 
//                  single tile.  Optionally returns the merge orientation of 
//                  the first tile with respect to the second tile.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsMergable(
      const TTPT_Tile_c< T >&  tileA,
      const TTPT_Tile_c< T >&  tileB,
            TGS_OrientMode_t*  porientMode ) const
{
   TGS_OrientMode_t orientMode = TGS_ORIENT_UNDEFINED;

   TC_SideMode_t side = TC_SIDE_UNDEFINED;
   if( this->IsAdjacent( tileA, tileB, &side ))
   {
      const TGS_Region_c& regionA = tileA.GetRegion( );
      const TGS_Region_c& regionB = tileB.GetRegion( );

      switch( side )
      {
      case TC_SIDE_LEFT:
      case TC_SIDE_RIGHT:

         if( TCTF_IsEQ( regionA.y1, regionB.y1 ) &&
             TCTF_IsEQ( regionA.y2, regionB.y2 ))
         {
            if( tileA.IsClear( ) && tileB.IsClear( ))
            {
               orientMode = TGS_ORIENT_VERTICAL;
            }
            else if( tileA.IsSolid( ) && tileB.IsSolid( ) &&
                     tileA.IsEqualData( tileB ))
            {
               orientMode = TGS_ORIENT_VERTICAL;
            }
         }
         break;

      case TC_SIDE_LOWER:
      case TC_SIDE_UPPER:

         if( TCTF_IsEQ( regionA.x1, regionB.x1 ) &&
             TCTF_IsEQ( regionA.x2, regionB.x2 ))
         {
            if( tileA.IsClear( ) && tileB.IsClear( ))
            {
               orientMode = TGS_ORIENT_HORIZONTAL;
            }
            else if( tileA.IsSolid( ) && tileB.IsSolid( ) &&
                     tileA.IsEqualData( tileB ))
            {
               orientMode = TGS_ORIENT_HORIZONTAL;
            }
         }
         break;

      default:
         break;
      }
   }

   if( porientMode )
   {
      *porientMode = orientMode;
   }
   return( orientMode != TGS_ORIENT_UNDEFINED ? true : false );
}

//===========================================================================//
// Method         : IsSplitable
// Purpose        : Returns true if the given single tile can be split into 
//                  two adjacent tiles based on the given intersect line.  
//                  Optionally returns the split orientation if the first 
//                  tile with respect to the second tile.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSplitable(
      const TTPT_Tile_c< T >&  tile,
      const TGS_Line_c&        intersectLine,  
            TGS_DirMode_t      dirMode,
            TGS_OrientMode_t*  porientMode ) const
{
   TTP_TileMode_t tileMode = tile.GetMode( );
   const TGS_Region_c& tileRegion = tile.GetRegion( );

   TGS_OrientMode_t orientMode = intersectLine.FindOrient( );
   if( orientMode == TGS_ORIENT_UNDEFINED )
   {
      orientMode = tileRegion.FindOrient( TGS_ORIENT_ALTERNATE );
   }

   if( orientMode == TGS_ORIENT_HORIZONTAL )
   {
      if( TCTF_IsLT( tileRegion.GetDy( ), this->minGrid_ ))
      {
         orientMode = TGS_ORIENT_UNDEFINED;
      }
   }
   if( orientMode == TGS_ORIENT_VERTICAL )
   {
      if( TCTF_IsLT( tileRegion.GetDx( ), this->minGrid_ ))
      {
         orientMode = TGS_ORIENT_UNDEFINED;
      }
   }

   TGS_Region_c intersectRegion( tileRegion );
   if( orientMode == TGS_ORIENT_HORIZONTAL )
   {
      intersectRegion.y1 += this->minGrid_;
      intersectRegion.y2 -= this->minGrid_;
   }
   if( orientMode == TGS_ORIENT_VERTICAL )
   {
      intersectRegion.x1 += this->minGrid_;
      intersectRegion.x2 -= this->minGrid_;
   }

   if(( orientMode == TGS_ORIENT_HORIZONTAL ) ||
      ( orientMode == TGS_ORIENT_VERTICAL ))
   {
      switch( dirMode )
      {
      case TGS_DIR_LEFT:

         if( TCTF_IsEQ( tileRegion.x2, intersectLine.x2 ))
         {
            intersectRegion.x2 += this->minGrid_;
         }         
         break;

      case TGS_DIR_RIGHT:

         if( TCTF_IsEQ( tileRegion.x1, intersectLine.x1 ))
         {
            intersectRegion.x1 -= this->minGrid_; 
         }         
         break;

      case TGS_DIR_DOWN:

         if( TCTF_IsEQ( tileRegion.y2, intersectLine.y2 ))
         {
            intersectRegion.y2 += this->minGrid_;
         }         
         break;

      case TGS_DIR_UP:

         if( TCTF_IsEQ( tileRegion.y1, intersectLine.y1 ))
         {
            intersectRegion.y1 -= this->minGrid_; 
         }         
         break;

      default:
         break;
      }

      TTPT_Tile_c< T > intersectTile( tileMode, intersectRegion );
      if(( !TCTF_IsLE( intersectRegion.x1, intersectRegion.x2 )) ||
         ( !TCTF_IsLE( intersectRegion.y1, intersectRegion.y2 )) ||
         ( !intersectTile.IsIntersecting( intersectLine )))
      {
         orientMode = TGS_ORIENT_UNDEFINED;
      }
   }

   if( porientMode )
   {
      *porientMode = orientMode;
   }
   return( orientMode != TGS_ORIENT_UNDEFINED ? true : false );
}

//===========================================================================//
// Method         : IsLegal
// Purpose        : Returns true if the current tile plane is legal according 
//                  to the tile plane's strip property (ie. does not have any 
//                  horizontally adjacent clear tiles).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsLegal(
      void ) const
{
   bool isLegal = true;

   TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
   ptilePlane->tilePlaneIter_.Init( *this );
   TTPT_Tile_c< T >* ptile = 0;
   while( ptilePlane->tilePlaneIter_.Next( &ptile ))
   {
      const TGS_Region_c& tileRegion = ptile->GetRegion( );

      if(( TCTF_IsGT( tileRegion.x1, tileRegion.x2 )) ||
         ( TCTF_IsGT( tileRegion.y1, tileRegion.y2 )))
      {
         isLegal = this->ShowInternalMessage_( TTP_MESSAGE_IS_LEGAL_INVALID_REGION, 
                                               "TTPT_TilePlane_c::IsLegal", 
				               &tileRegion );
      }

      if(( ptile->IsClear( )) &&
	 ( ptile->GetStitchUpperRight( )) &&
         ( ptile->GetStitchUpperRight( )->IsClear( )))
      {
         const TGS_Region_c& nextRegion = ptile->GetStitchUpperRight( )->GetRegion( );

         isLegal = this->ShowInternalMessage_( TTP_MESSAGE_IS_LEGAL_INVALID_REGION, 
                                               "TTPT_TilePlane_c::IsLegal", 
				               &tileRegion, &nextRegion );
      }
   }
   return( isLegal );
}

//===========================================================================//
// Method         : AddPerRegion_
// Purpose        : Adds or inserts a new solid tile into the current tile 
//                  plane.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerRegion_(
      const TTPT_Tile_c< T >& addTile,
            TGS_OrientMode_t  addOrient )
{
   bool ok = true;

   const TGS_Region_c& addRegion = addTile.GetRegion( );
   unsigned int addCount = addTile.GetCount( );
   const T* paddData = addTile.GetData( );

   if( this->IsWithin( addRegion ))
   {
      // Find clear tile containing new tile's upper side, split into two tiles
      if( ok )
      {
         ok = AddPerUpperSideSplit_( addRegion );
      }

      // Find clear tile containing new tile's lower side, split into two tiles
      if( ok )
      {
         ok = AddPerLowerSideSplit_( addRegion );
      }

      // Iterate new tile's region, splitting and merging left/center/right tiles
      if( ok )
      {
         ok = AddPerCenterSideSplits_( addRegion );
      }

      // After iteration, attempt to merge new tile with neighboring tiles
      // (ie. merge with left/right/upper solid tiles, if possible)
      if( ok )
      {
	 // Find solid tile based on upper-right corner of region 
         TTPT_Tile_c< T >* psolidTile = this->Find( addRegion.x2, addRegion.y2 );
         psolidTile->MakeSolid( addCount, paddData );

         ok = AddPerSolidTileMerges_( addRegion );
      }

      // After merging with neighboring tiles, attempt to re-merge aligned tiles
      // (ie. re-merge with lower/upper vertically aligned solid tiles, if any)
      if( ok )
      {
	 // Find solid tile based on upper-right corner of region 
         TTPT_Tile_c< T >* psolidTile = this->Find( addRegion.x2, addRegion.y2 );

         TGS_Region_c solidRegion = psolidTile->GetRegion( );
         addOrient = ( addOrient != TGS_ORIENT_UNDEFINED ?
                       addOrient : TGS_ORIENT_VERTICAL );

         ok = AddPerSolidTileCorners_( solidRegion, addOrient );
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : AddPerNew_
// Purpose        : Adds or inserts a new solid tile into the current tile 
//                  plane.  This method adds the new solid tile if and only 
//                  if there are no solid tiles in the desired area.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerNew_(
      const TTPT_Tile_c< T >& addTile,
            TGS_OrientMode_t  addOrient )
{
   bool ok = true;

   const TGS_Region_c& addRegion = addTile.GetRegion( );

   TTPT_Tile_c< T >* pintersectTile = 0;
   if( this->IsWithin( addRegion ) && 
       !this->IsSolid( TTP_IS_SOLID_ANY, addRegion, &pintersectTile ))
   {
      ok = this->AddPerRegion_( addTile, addOrient );
   }
   else if( !this->IsWithin( addRegion ))
   {
      // Report message: new tile region not within this tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_ADD_PER_NEW_INVALID, 
                                       "TTPT_TilePlane_c::AddPerNew_", 
                                       &addRegion, &tilePlaneRegion );
   }
   else if( this->IsSolid( TTP_IS_SOLID_ANY, addRegion, &pintersectTile ))
   {
      // Report message: new tile region intersects existing solid tile
      const TGS_Region_c& existsRegion = pintersectTile->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_ADD_PER_NEW_EXISTS, 
                                       "TTPT_TilePlane_c::AddPerNew_", 
                                       &addRegion, &existsRegion );
   }
   return( ok );
}

//===========================================================================//
// Method         : AddPerMerge_
// Purpose        : Adds or inserts a new solid tile into the current tile 
//                  plane.  If the new solid tile overlaps with one or more 
//                  existing solid tiles, then the new solid tile is merged 
//                  with the existing solid tiles.
//
//                  For example, merging the two (A) (ie. "same") data tiles
//                  shown below on the left results in the set of three (A)
//                  (ie. "same") data tiles shown below on the right.
//                  
//                        +-------------+          +-------------+
//                        |             |          |             |
//                        |             |          |     (A)     |
//                        |     (A)     |          |             |
//                        |             |          +-------------+
//                        |       +-----|----+     +------------------+
//                        |       |     |    |     |       (A)        |
//                        +-------|-----+    |     +------------------+
//                                |   (A)    |             +----------+
//                                |          |             |   (A)    |
//                                +----------+             +----------+
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerMerge_( 
      const TTPT_Tile_c< T >& addTile,
            TGS_OrientMode_t  addOrient,
            TTP_AddMode_t     addMode )
{
   bool ok = true;

   const TGS_Region_c& addRegion = addTile.GetRegion( );
   unsigned int addCount = addTile.GetCount( );
   const T* paddData = addTile.GetData( );

   TTPT_Tile_c< T >* pintersectTile = 0;
   bool isWithin = this->IsWithin( addRegion );
   bool isSolidAll = isWithin ?
                     this->IsSolid( TTP_IS_SOLID_ALL, addRegion, addCount, paddData ) : 
                     false;
   bool isSolidNotAny = isWithin ?
                        this->IsSolidNot( TTP_IS_SOLID_ANY, addRegion, addCount, paddData, &pintersectTile ) : 
                        false;
   if( isWithin && !isSolidAll && !isSolidNotAny )
   {
      // Merge existing tile with new tile and add adjacent solid regions
      ok = this->AddPerMergeNew_( addTile, addOrient, addMode );
   }
   else if( !isWithin )
   {
      // Report message: new tile region not within this tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_ADD_PER_MERGE_INVALID, 
                                       "TTPT_TilePlane_c::AddPerMerge_", 
                                       &addRegion, &tilePlaneRegion );
   }
   else if( isSolidNotAny )
   {
      // Report message: new tile region intersects existing solid tile
      const TGS_Region_c& existsRegion = pintersectTile->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_ADD_PER_MERGE_EXISTS, 
                                       "TTPT_TilePlane_c::AddPerMerge_", 
                                       &addRegion, &existsRegion );
   }
   else if( isSolidAll )
   {
      // Nothing to do since solid tile(s) already exists per new region
   }
   return( ok );
}

//===========================================================================//
// Method         : AddPerMergeNew_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerMergeNew_( 
      const TTPT_Tile_c< T >& addTile,
            TGS_OrientMode_t  addOrient,
            TTP_AddMode_t     addMode )
{
   bool ok = true;

   const TGS_Region_c& addRegion = addTile.GetRegion( );
   unsigned int addCount = addTile.GetCount( );
   const T* paddData = addTile.GetData( );

   bool addPerDifference = false;

   TTPT_Tile_c< T >* psolidTile = 0;
   if( this->IsSolid( TTP_IS_SOLID_ANY, addRegion, addCount, paddData, &psolidTile ) &&
       this->IsSolid( TTP_IS_SOLID_MAX, addRegion, addCount, paddData, &psolidTile ))
   {
      const TGS_Region_c& solidRegion = psolidTile->GetRegion( );
      if( TCTF_IsLE( addRegion.FindArea( ), solidRegion.FindArea( )))
      {
         addPerDifference = true;
      }
      if( addRegion.IsCrossed( solidRegion, this->minGrid_ ))
      {
         addPerDifference = this->HasAspectRatio_( addRegion, solidRegion );
      }

      if( addRegion.IsCorner( solidRegion ))
      {
         addPerDifference = true;
      }
      if( addMode == TTP_ADD_DIFFERENCE )
      {
         addPerDifference = true;
      }
   }

   if( addPerDifference )
   {
      // Decide (or override) orient based on user-defined tile orient mode
      addOrient = ( addRegion.IsWide( ) ? 
                    TGS_ORIENT_VERTICAL : TGS_ORIENT_HORIZONTAL );

      // Ready to iterate over region, adding any non-intersecting solid regions
      const TGS_Region_c& solidRegion = psolidTile->GetRegion( );
      TGS_RegionList_t subRegionList( 4 );
      addRegion.FindDifference( solidRegion, addOrient, 
                                &subRegionList, this->minGrid_ );
      for( size_t i = 0; i < subRegionList.GetLength( ); ++i )
      {
         TTPT_Tile_c< T > subTile( TTP_TILE_SOLID, *subRegionList[i], 
                                   addCount, paddData );
         ok = this->AddPerMerge_( subTile, addOrient, addMode );
         if( !ok )
            break;
      }
   }
   else
   {
      // First, iterate over region, deleting any intersecting solid regions
      TTPT_Tile_c< T >* pwithinTile = 0;
      while( this->IsSolid( TTP_IS_SOLID_ANY, addRegion, addCount, paddData, &psolidTile ))
      {
         if( psolidTile->IsWithin( addRegion ))
         {
            pwithinTile = psolidTile;
            break;
         }
   
         TTPT_Tile_c< T > solidTile( *psolidTile );
         const TGS_Region_c& solidRegion = solidTile.GetRegion( );
         unsigned int solidCount = solidTile.GetCount( );
         const T* psolidData = solidTile.GetData( );
   
         ok = this->Delete( psolidTile );
         if( !ok )
            break;
   
         // Decide (or override) orient based on user-defined tile orient mode
         addOrient = ( addRegion.IsWide( ) ? 
                       TGS_ORIENT_VERTICAL : TGS_ORIENT_HORIZONTAL );

         TGS_RegionList_t subRegionList( 4 );
         solidRegion.FindDifference( addRegion, addOrient, 
                                     &subRegionList, this->minGrid_ );
         for( size_t i = 0; i < subRegionList.GetLength( ); ++i )
         {
            TTPT_Tile_c< T > subTile( TTP_TILE_SOLID, *subRegionList[i], 
                                      solidCount, psolidData );
            ok = this->AddPerNew_( subTile, addOrient );
            if( !ok )
               break;
         }
         if( !ok )
            break;
      }
      
      // Second, add a new solid tile adjacent to any existing solid regions
      if( ok && !pwithinTile )
      {
         ok = this->AddPerNew_( addTile, addOrient );
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : AddPerOverlap_
// Purpose        : Adds or inserts a new solid tile into the current tile 
//                  existing solid tiles with non-compatible data, then the 
//                  new and existing tile intersects are defined based on 
//                  the union of the non-compatible data.
//                  
//                  For example, overlapping the two (A) and (B) (ie. 
//                  "different") data tiles shown below on the left results 
//                  in the set of (A) and (B) (ie. "same" and "different") 
//                  data tiles shown below on the right, including the 
//                  combined (A,B) data tile.
//
//                        +-------------+          +-------------+
//                        |             |          |             |
//                        |             |          |     (A)     |
//                        |     (A)     |          |             |
//                        |             |          +-------------+
//                        |       +-----|----+     +------++-----++---+
//                        |       |     |    |     | (A)  ||(A,B)||(B)|
//                        +-------|-----+    |     +------++-----++---+
//                                |   (B)    |             +----------+
//                                |          |             |   (B)    |
//                                +----------+             +----------+
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerOverlap_( 
      const TTPT_Tile_c< T >& addTile,
            TGS_OrientMode_t  addOrient,
            TTP_AddMode_t     addMode )
{
   bool ok = true;

   const TGS_Region_c& addRegion = addTile.GetRegion( );
   unsigned int addCount = addTile.GetCount( );
   const T* paddData = addTile.GetData( );
   
   TTPT_Tile_c< T >* pintersectTile = 0;
   bool isWithin = this->IsWithin( addRegion );
   bool isSolidNotAny = isWithin ?
                        this->IsSolidNot( TTP_IS_SOLID_ANY,
				          addRegion, addCount, paddData, 
                                          &pintersectTile ) : false;
   if( isWithin && isSolidNotAny )
   {
      // Add "different" data tile by using add-by-overlap insertion method
      if( !pintersectTile->IsWithin( addRegion ) ||
          !pintersectTile->IsEqualData( addCount, paddData ))
      {
         const TTPT_Tile_c< T >* poverlapTile = pintersectTile;
         ok = this->AddPerOverlap_( addTile, poverlapTile, addOrient, addMode );
      }
   }   
   else if( isWithin && !isSolidNotAny )
   {
      // Defer adding "same" data tile by using add-by-merge insertion method
      ok = this->AddPerMergeNew_( addTile, addOrient, addMode );
   }   
   else if( !isWithin )
   {
      // Report message: new tile region not within this tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_ADD_PER_OVERLAP_INVALID, 
                                       "TTPT_TilePlane_c::AddPerOverlap_", 
                                       &addRegion, &tilePlaneRegion );
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerOverlap_( 
      const TTPT_Tile_c< T >& addTile,
      const TTPT_Tile_c< T >* poverlapTile,
            TGS_OrientMode_t  addOrient,
            TTP_AddMode_t     addMode )
{
   bool ok = true;

   TTPT_Tile_c< T > addTile_( addTile );
   const TGS_Region_c& addRegion = addTile_.GetRegion( );
   unsigned int addCount = addTile_.GetCount( );
   const T* paddData = addTile_.GetData( );

   TTPT_Tile_c< T > overlapTile_( *poverlapTile );
   TGS_Region_c overlapRegion = overlapTile_.GetRegion( );
   unsigned int overlapCount = overlapTile_.GetCount( );
   const T* poverlapData = overlapTile_.GetData( );

   // Start by deleting the existing overlap tile
   ok = this->Delete( overlapRegion );

   // Build an intersect tile based on add and overlap tile regions and data   
   TGS_Region_c intersectRegion;
   intersectRegion.ApplyIntersect( addRegion, overlapRegion );

   TTPT_Tile_c< T > intersectTile( TTP_TILE_SOLID, intersectRegion );
   if( ok )
   {
      ok = intersectTile.AddData( addCount, paddData );
   }
   if( ok )
   {
      for( unsigned int i = 0; i < overlapCount; ++i )
      {
         const T* poverlapData_i = ( poverlapData + i );

         bool isMemberOverlapData_i = false;
         for( unsigned int j = 0; j < addCount; ++j )
         {
            const T* paddData_j = ( paddData + j );
            if( *poverlapData_i == *paddData_j )
	    {
	       isMemberOverlapData_i = true;
               break;
            }
	 }
         if( !isMemberOverlapData_i )
         {
            ok = intersectTile.AddData( 1, poverlapData_i );
         }
      }
   }

   // First, add the intersect tile with both add and overlap tile data 
   if( ok )
   {
      ok = this->AddPerNew_( intersectTile, addOrient );
   }
   
   // Decide difference from overlap to intersect tiles and add difference
   if( ok )
   {
      // Decide (or override) orient based on user-defined tile orient mode
      addOrient = ( addRegion.IsWide( ) ? 
                    TGS_ORIENT_VERTICAL : TGS_ORIENT_HORIZONTAL );

      TGS_RegionList_t overlapSubRegionList( 4 );
      overlapRegion.FindDifference( intersectRegion, addOrient,
                                    &overlapSubRegionList, this->minGrid_ );
      for( size_t i = 0; i < overlapSubRegionList.GetLength( ); ++i )
      {
         TTPT_Tile_c< T > overlapSubTile( TTP_TILE_SOLID, *overlapSubRegionList[i], 
                                          overlapCount, poverlapData );
         ok = this->AddPerNew_( overlapSubTile, addOrient );
         if( !ok )
            break;
      }
   }

   // Decide difference from add to intersect tiles and add difference
   // Note: difference tiles are added recursively to handle multiple overlaps
   if( ok )
   {
      TGS_RegionList_t addSubRegionList( 4 );
      addRegion.FindDifference( intersectRegion, addOrient,
                                &addSubRegionList, this->minGrid_ );
      for( size_t i = 0; i < addSubRegionList.GetLength( ); ++i )
      {
         const TGS_Region_c& addSubRegion = *addSubRegionList[i];

         TTPT_Tile_c< T > addSubTile( TTP_TILE_SOLID, addSubRegion, 
                                      addCount, paddData );
         ok = this->AddPerOverlap_( addSubTile, addOrient, addMode );
         if( !ok )
            break;
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : AddPerUpperSideSplit_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerUpperSideSplit_(
      const TGS_Region_c& addRegion )
{
   bool ok = true;

   // Find the clear tile containing the upper side of the given new tile region 
   // (Clear tiles cover entire upper side due to tile plane's strip property)
   TTPT_Tile_c< T >* pupperTile = this->Find( addRegion.x2, addRegion.y2 );

   // Split clear tile horizontally into two tiles per given tile's upper side
   const TGS_Region_c& upperRegion = pupperTile->GetRegion( );
   TGS_Line_c upperIntersect( upperRegion.x1 - this->minGrid_, addRegion.y2, 
                              upperRegion.x2 + this->minGrid_, addRegion.y2 );

   if( this->IsSplitable( *pupperTile, upperIntersect, TGS_DIR_UP ))
   {
      ok = this->Split( pupperTile, upperIntersect, TGS_DIR_UP );
   }
   return( ok );
}

//===========================================================================//
// Method         : AddPerLowerSideSplit_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerLowerSideSplit_(
      const TGS_Region_c& addRegion )
{
   bool ok = true;

   // Find clear tile containing lower side of the given new tile region 
   // (Clear tiles cover entire lower side due to tile plane's strip property)
   TTPT_Tile_c< T >* plowerTile = this->Find( addRegion.x1, addRegion.y1 );

   // Split clear tile horizontally into two tiles per given tile's lower side
   const TGS_Region_c& lowerRegion = plowerTile->GetRegion( );
   TGS_Line_c lowerIntersect( lowerRegion.x1 - this->minGrid_, addRegion.y1, 
                              lowerRegion.x2 + this->minGrid_, addRegion.y1 );

   if( this->IsSplitable( *plowerTile, lowerIntersect, TGS_DIR_DOWN ))
   {
      ok = this->Split( plowerTile, lowerIntersect, TGS_DIR_DOWN );
   }
   return( ok );
}

//===========================================================================//
// Method         : AddPerCenterSideSplits_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerCenterSideSplits_(
      const TGS_Region_c& addRegion )
{
   bool ok = true;

   // Iterate new tile's region, splitting and merging left/center/right tiles
   // (iteration direction is assumed from new tile's upper to lower region)
   TGS_Region_c iterRegion( addRegion );
   while( ok && TCTF_IsGE( iterRegion.y2, iterRegion.y1 ))
   {
      // Split clear tile into left clear, center solid, and right clear tiles
      // (Clear tile has at least solid tile due to tile plane's strip property)
      TTPT_Tile_c< T >* pleftCenterTile = this->Find( iterRegion.x2, iterRegion.y2 );
      const TGS_Region_c& leftCenterRegion = pleftCenterTile->GetRegion( );
      if( TCTF_IsLT( leftCenterRegion.x1, iterRegion.x1 ))
      {
         TGS_Line_c leftIntersect( iterRegion.x1, iterRegion.y1 - this->minGrid_,
                                   iterRegion.x1, iterRegion.y2 + this->minGrid_ );
         ok = this->Split( pleftCenterTile, leftIntersect, TGS_DIR_LEFT );
         if( !ok )
            break;

         // And merge left clear tile with upper clear tile, if possible           
         TGS_Point_c leftPoint( iterRegion.x1 - this->minGrid_, iterRegion.y2 );
         TTPT_Tile_c< T >* pleftTile = this->Find( leftPoint );
         ok = this->Merge( pleftTile, TC_SIDE_UPPER );
         if( !ok )
            break;
      }

      TTPT_Tile_c< T >* prightCenterTile = this->Find( iterRegion.x2, iterRegion.y2 );
      const TGS_Region_c& rightCenterRegion = prightCenterTile->GetRegion( );
      if( TCTF_IsGT( rightCenterRegion.x2, iterRegion.x2 ))
      {
         TGS_Line_c rightIntersect( iterRegion.x2, iterRegion.y1 - this->minGrid_,
                                    iterRegion.x2, iterRegion.y2 + this->minGrid_ );
         ok = this->Split( prightCenterTile, rightIntersect, TGS_DIR_RIGHT );
         if( !ok )
            break;
        
         // And merge right clear tile with upper clear tile, if possible           
         TGS_Point_c rightPoint( iterRegion.x2 + this->minGrid_, iterRegion.y2 );
         TTPT_Tile_c< T >* prightTile = this->Find( rightPoint );
         ok = this->Merge( prightTile, TC_SIDE_UPPER );
         if( !ok )
            break;
      }

      // And merge right clear tile with upper clear tile, if possible
      TTPT_Tile_c< T >* pcenterTile = this->Find( iterRegion.x2, iterRegion.y2 );
      if( TCTF_IsLT( iterRegion.y2, addRegion.y2 ))
      {
         ok = this->Merge( pcenterTile, TC_SIDE_UPPER );
         if( !ok )
            break;

         pcenterTile = this->Find( iterRegion.x2, iterRegion.y2 );
      }

      const TGS_Region_c& centerRegion = pcenterTile->GetRegion( );
      iterRegion.y2 = centerRegion.y1 - this->minGrid_;
   }     
   return( ok );
}

//===========================================================================//
// Method         : AddPerSolidTileMerges_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerSolidTileMerges_(
      const TGS_Region_c& addRegion )
{
   bool ok = true;

   TGS_Point_c upperLeft( addRegion.x1, addRegion.y2 );

   while( ok )
   {
      const TTPT_Tile_c< T >& solidTile = *this->Find( upperLeft );
      TGS_Region_c solidRegion = solidTile.GetRegion( );
      
      // Attempt to merge new solid tile with neighbor solid tiles, if possible
      if( ok )
      {
         TTPT_Tile_c< T >* psolidTile = this->Find( upperLeft );
         ok = this->Merge( psolidTile, TC_SIDE_LEFT );
      }
            
      if( ok )
      {
         TTPT_Tile_c< T >* psolidTile = this->Find( upperLeft );
         ok = this->Merge( psolidTile, TC_SIDE_RIGHT );
      }
            
      if( ok )
      {
         TTPT_Tile_c< T >* psolidTile = this->Find( upperLeft );
         ok = this->Merge( psolidTile, TC_SIDE_LOWER );
      }

      if( ok )
      {
         TTPT_Tile_c< T >* psolidTile = this->Find( upperLeft );
         ok = this->Merge( psolidTile, TC_SIDE_UPPER );
      }

      const TTPT_Tile_c< T >& mergedTile = *this->Find( upperLeft );
      const TGS_Region_c& mergedRegion = mergedTile.GetRegion( );
      if( mergedRegion == solidRegion )
	break;
   }
   return( ok );
}

//===========================================================================//
// Method         : AddPerSolidTileCorners_
// Purpose        : Re-merges the given solid tiles with vertically aligned, 
//                  but partially mergable neighboring solid tiles in each 
//                  of four possible corners.  These merges are required to 
//                  maintain the tile plane's strip property whereby all 
//                  clear tiles are separated by solid tiles only.  The
//                  following examples illustrate corner merges whereby the 
//                  common vertically aligned neighboring solid tiles are 
//                  re-merged:
//
//                     +----------+        +-----++---+     
//                     |          |        |     ||   |
//                     |          |        |     ||   |
//                     +----------+   ==>  |     |+---+
//                     +-----+             |     |
//                     |     |             |     |
//                     +-----+             +-----+
//                  
//                          +---+               +---+
//                          |   |               |   |
//                          +---+               |   |
//                     +--------+     ==>  +---+|   |
//                     |        |          |   ||   |
//                     |        |          |   ||   |
//                     +--------+          +---++---+
//                  
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerSolidTileCorners_(
      const TGS_Region_c&    addRegion,
            TGS_OrientMode_t addOrient )
{
   bool ok = true;

   // Build a set of 4 corner points based on given add tile region
   TGS_Point_c lowerLeft;
   TGS_Point_c lowerRight;
   TGS_Point_c upperLeft;
   TGS_Point_c upperRight;

   if( addOrient == TGS_ORIENT_VERTICAL )
   {
      lowerLeft.Set( addRegion.x1, addRegion.y1 - this->minGrid_ );
      lowerRight.Set( addRegion.x2, addRegion.y1 - this->minGrid_ );
      upperLeft.Set( addRegion.x1, addRegion.y2 + this->minGrid_ );
      upperRight.Set( addRegion.x2, addRegion.y2 + this->minGrid_ );
   }
   if( addOrient == TGS_ORIENT_HORIZONTAL )
   {
      lowerLeft.Set( addRegion.x1 - this->minGrid_, addRegion.y1 );
      lowerRight.Set( addRegion.x2 + this->minGrid_, addRegion.y1 );
      upperLeft.Set( addRegion.x1 - this->minGrid_, addRegion.y2 );
      upperRight.Set( addRegion.x2 + this->minGrid_, addRegion.y2 );
   }

   // For each of 4 corner points, test for vertically aligned solid tiles
   if( ok )
   {
      ok = this->AddPerSolidTileCorner_( addRegion, lowerLeft, addOrient );
   }
   if( ok )
   {
      ok = this->AddPerSolidTileCorner_( addRegion, lowerRight, addOrient );
   }
   if( ok )
   {
      ok = this->AddPerSolidTileCorner_( addRegion, upperLeft, addOrient );
   }
   if( ok )
   {
      ok = this->AddPerSolidTileCorner_( addRegion, upperRight, addOrient );
   }
   return( ok );
}

//===========================================================================//
// Method         : AddPerSolidTileCorner_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::AddPerSolidTileCorner_(
      const TGS_Region_c&    addRegion,
      const TGS_Point_c&     cornerPoint,
            TGS_OrientMode_t addOrient )
{
   bool ok = true;

   TTPT_Tile_c< T >* psolidTile = this->Find( addRegion.x1, addRegion.y2 );
   if( psolidTile )
   {
      if( this->IsSolid( cornerPoint, psolidTile->GetCount( ), psolidTile->GetData( )))
      {
         TTPT_Tile_c< T >* pcornerTile = this->Find( cornerPoint );
   
         TGS_Region_c solidRegion, cornerRegion;
         if( this->CornerRegionCoords_( psolidTile->GetRegion( ), 
                                        pcornerTile->GetRegion( ),
                                        &solidRegion, &cornerRegion, addOrient ))
         {
            ok = this->CornerTileRemerge_( psolidTile, pcornerTile, 
                                           solidRegion, cornerRegion, addOrient );
         }
      }
   }
   return( ok );
}

//===========================================================================//
// Method         : DeletePerExact_
// Purpose        : Deletes or removes existing solid tiles from the current 
//                  tile plane based on matching exactly the given region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerExact_( 
      const TGS_Region_c& deleteRegion )
{
   bool ok = true;

   if( this->IsWithin( deleteRegion ))
   {
      TTPT_Tile_c< T >* pdeleteTile = this->Find( deleteRegion.x1, deleteRegion.y1 );
      if( pdeleteTile &&
          pdeleteTile->IsSolid( ) &&
          pdeleteTile->GetRegion( ) == deleteRegion )
      {
         // Delete solid region (no iteration needed single is only region)
         ok = this->Delete( pdeleteTile );
      }
      else
      {
         // Iterate over region, deleting a solid region matching given region
         TTPT_TilePlaneIter_c< T > thisIter( *this, deleteRegion );
         TTPT_Tile_c< T >* psolidTile = 0;
         while( ok && thisIter.Next( &psolidTile, TTP_TILE_SOLID ))
         {
            if( psolidTile->GetRegion( ) != deleteRegion )
               continue;

            ok = this->Delete( psolidTile );
            if( !ok )
               break;

            thisIter.Init( *this, deleteRegion );
         }
      }
   }
   else if( !this->IsWithin( deleteRegion ))
   {
      // Report message: delete tile region not within tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_DELETE_PER_EXACT_INVALID, 
                                       "TTPT_TilePlane_c::DeletePerExact_", 
                                       &deleteRegion, &tilePlaneRegion );
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerExact_( 
      const TGS_Region_c&    deleteRegion,
      const T&               deleteData )
{
   bool ok = true;

   if( this->IsWithin( deleteRegion ))
   {
      // Iterate over region, deleting a solid region matching given region
      TTPT_TilePlaneIter_c< T > thisIter( *this, deleteRegion );
      TTPT_Tile_c< T >* psolidTile = 0;
      while( ok && thisIter.Next( &psolidTile, TTP_TILE_SOLID ))
      {
         if( psolidTile->GetRegion( ) != deleteRegion )
            continue;

	 if( !psolidTile->FindData( deleteData ))
            continue;

         TTPT_Tile_c< T > replaceTile( *psolidTile );
         ok = replaceTile.DeleteData( deleteData );
         if( !ok )
            break;

         ok = this->Delete( psolidTile );
         if( !ok )
            break;

	 if( replaceTile.IsValid( ) && replaceTile.HasData( ))
         {
            TGS_OrientMode_t addOrient = TGS_ORIENT_UNDEFINED;
	    ok = this->AddPerNew_( replaceTile, addOrient );
            if( !ok )
               break;
         }
         thisIter.Init( *this, deleteRegion );
      }
   }
   else if( !this->IsWithin( deleteRegion ))
   {
      // Report message: delete tile region not within tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_DELETE_PER_EXACT_INVALID, 
                                       "TTPT_TilePlane_c::DeletePerExact_", 
                                       &deleteRegion, &tilePlaneRegion );
   }
   return( ok );
}

//===========================================================================//
// Method         : DeletePerWithin_
// Purpose        : Deletes or removes existing solid tiles from the current 
//                  tile plane based on membership within the given region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerWithin_( 
      const TGS_Region_c& deleteRegion )
{
   bool ok = true;

   if( this->IsWithin( deleteRegion ))
   {
      // Iterate over region, deleting any solid regions within given region
      TTPT_TilePlaneIter_c< T > thisIter( *this, deleteRegion );
      TTPT_Tile_c< T >* psolidTile = 0;
      while( ok && thisIter.Next( &psolidTile, TTP_TILE_SOLID ))
      {
         if( !psolidTile->IsWithin( deleteRegion ))
            continue;

         ok = this->Delete( psolidTile );
         if( !ok )
            break;

         thisIter.Init( *this, deleteRegion );
      }
   }
   else if( !this->IsWithin( deleteRegion ))
   {
      // Report message: delete tile region not within tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_DELETE_PER_WITHIN_INVALID, 
                                       "TTPT_TilePlane_c::DeletePerWithin_", 
                                       &deleteRegion, &tilePlaneRegion );
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerWithin_( 
      const TGS_Region_c&    deleteRegion,
      const T&               deleteData )
{
   bool ok = true;

   if( this->IsWithin( deleteRegion ))
   {
      // Iterate over region, deleting any solid regions within given region
      TTPT_TilePlaneIter_c< T > thisIter( *this, deleteRegion );
      TTPT_Tile_c< T >* psolidTile = 0;
      while( ok && thisIter.Next( &psolidTile, TTP_TILE_SOLID ))
      {
         if( !psolidTile->IsWithin( deleteRegion ))
            continue;

	 if( !psolidTile->FindData( deleteData ))
            continue;

         TTPT_Tile_c< T > replaceTile( *psolidTile );
         ok = replaceTile.DeleteData( deleteData );
         if( !ok )
            break;

         ok = this->Delete( psolidTile );
         if( !ok )
            break;

	 if( replaceTile.IsValid( ) && replaceTile.HasData( ))
         {
            TGS_OrientMode_t addOrient = TGS_ORIENT_UNDEFINED;
	    ok = this->AddPerNew_( replaceTile, addOrient );
            if( !ok )
               break;
         }
         thisIter.Init( *this, deleteRegion );
      }
   }
   else if( !this->IsWithin( deleteRegion ))
   {
      // Report message: delete tile region not within tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_DELETE_PER_WITHIN_INVALID, 
                                       "TTPT_TilePlane_c::DeletePerWithin_", 
                                       &deleteRegion, &tilePlaneRegion );
   }
   return( ok );
}

//===========================================================================//
// Method         : DeletePerIntersect_
// Purpose        : Deletes or removes existing solid tiles from the current 
//                  tile plane based on membership intersecting with given 
//                  region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerIntersect_( 
      const TGS_Region_c& deleteRegion )
{
   bool ok = true;

   if( this->IsWithin( deleteRegion ))
   {
      // Iterate over region, deleting any solid regions intersecting region
      TTPT_Tile_c< T >* psolidTile = 0;
      while( this->IsSolid( TTP_IS_SOLID_ANY, deleteRegion, &psolidTile ))
      {
         TTPT_Tile_c< T > solidTile( *psolidTile );
         const TGS_Region_c& solidRegion = solidTile.GetRegion( );
         unsigned int solidCount = solidTile.GetCount( );
         const T* psolidData = solidTile.GetData( );

         if( solidRegion.IsWithin( deleteRegion ))
         {
            if( solidRegion == deleteRegion )
            {
               this->Delete( psolidTile );
            }
            else if( TCTF_IsGT( deleteRegion.x1, solidRegion.x1 ))
            {
               TGS_Line_c leftEdge( deleteRegion.x1 - this->minGrid_, solidRegion.y1,
                                    deleteRegion.x1 - this->minGrid_, solidRegion.y2 );
               this->Split( psolidTile, leftEdge, TGS_DIR_RIGHT );
         
               ok = this->DeletePerIntersect_( deleteRegion );
            }
            else if( TCTF_IsLT( deleteRegion.x2, solidRegion.x2 ))
            {
               TGS_Line_c rightEdge( deleteRegion.x2 + this->minGrid_, solidRegion.y1,
                                     deleteRegion.x2 + this->minGrid_, solidRegion.y2 );
               this->Split( psolidTile, rightEdge, TGS_DIR_LEFT );
         
               ok = this->DeletePerIntersect_( deleteRegion );
            }
            else if( TCTF_IsGT( deleteRegion.y1, solidRegion.y1 ))
            {
               TGS_Line_c lowerEdge( solidRegion.x1, deleteRegion.y1 - this->minGrid_,
                                     solidRegion.x2, deleteRegion.y1 - this->minGrid_ );
               this->Split( psolidTile, lowerEdge, TGS_DIR_UP );
         
               ok = this->DeletePerIntersect_( deleteRegion );
            }
            else if( TCTF_IsLT( deleteRegion.y2, solidRegion.y2 ))
            {
               TGS_Line_c upperEdge( solidRegion.x1, deleteRegion.y2 + this->minGrid_,
                                     solidRegion.x2, deleteRegion.y2 + this->minGrid_ );
               this->Split( psolidTile, upperEdge, TGS_DIR_DOWN );
         
               ok = this->DeletePerIntersect_( deleteRegion );
            }
         }
         else
         {
            ok = this->Delete( psolidTile );
            if( !ok )
               break;

            TGS_RegionList_t subRegionList( 4 );
            solidRegion.FindDifference( deleteRegion, TGS_ORIENT_VERTICAL,
                                        &subRegionList, this->minGrid_ );
            for( size_t i = 0; i < subRegionList.GetLength( ); ++i )
            {
	       TTPT_Tile_c< T > subTile( TTP_TILE_SOLID, *subRegionList[i], 
                                         solidCount, psolidData );
               TGS_OrientMode_t addOrient = TGS_ORIENT_UNDEFINED;
               ok = this->AddPerNew_( subTile, addOrient );
               if( !ok )
                  break;
            }
            if( !ok )
               break;
         }
      }
   }
   else if( !this->IsWithin( deleteRegion ))
   {
      // Report message: delete tile region not within tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_DELETE_PER_INTERSECT_INVALID, 
                                       "TTPT_TilePlane_c::DeletePerIntersect_", 
                                       &deleteRegion, &tilePlaneRegion );
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerIntersect_( 
      const TGS_Region_c&    deleteRegion,
      const T&               deleteData )
{
   bool ok = true;

   if( this->IsWithin( deleteRegion ))
   {
      // Iterate over region, deleting any solid regions intersecting region
      TTPT_Tile_c< T >* psolidTile = 0;
      while( this->IsSolid( TTP_IS_SOLID_ANY, deleteRegion, deleteData, &psolidTile ))
      {
         TTPT_Tile_c< T > solidTile( *psolidTile );
         const TGS_Region_c& solidRegion = solidTile.GetRegion( );
         unsigned int solidCount = solidTile.GetCount( );
         const T* psolidData = solidTile.GetData( );

	 if( !psolidTile->FindData( deleteData ))
            continue;

         TTPT_Tile_c< T > replaceTile( *psolidTile );
         ok = replaceTile.DeleteData( deleteData );
         if( !ok )
            break;

         ok = this->Delete( psolidTile );
         if( !ok )
            break;

         TGS_RegionList_t subRegionList( 4 );
         solidRegion.FindDifference( deleteRegion, TGS_ORIENT_VERTICAL,
                                     &subRegionList, this->minGrid_ );
         for( size_t i = 0; i < subRegionList.GetLength( ); ++i )
         {
	    TTPT_Tile_c< T > subTile( TTP_TILE_SOLID, *subRegionList[i], 
                                      solidCount, psolidData );
            TGS_OrientMode_t addOrient = TGS_ORIENT_UNDEFINED;
            ok = this->AddPerNew_( subTile, addOrient );
            if( !ok )
               break;
         }
         if( !ok )
            break;

	 if( replaceTile.IsValid( ) && replaceTile.HasData( ))
         {
            TGS_OrientMode_t addOrient = TGS_ORIENT_UNDEFINED;
	    ok = this->AddPerNew_( replaceTile, addOrient );
            if( !ok )
               break;
         }
      }
   }
   else if( !this->IsWithin( deleteRegion ))
   {
      // Report message: delete tile region not within tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_DELETE_PER_INTERSECT_INVALID, 
                                       "TTPT_TilePlane_c::DeletePerIntersect_", 
                                       &deleteRegion, &tilePlaneRegion );
   }
   return( ok );
}

//===========================================================================//
// Method         : DeletePerIntersectMin_
// Purpose        : Deletes or removes existing solid tiles from the current 
//                  tile plane based on membership intersecting with given 
//                  region.  In addition, this method reduces the given 
//                  region by the current min grid before deleting.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerIntersectMin_( 
      const TGS_Region_c& deleteRegion )
{
   bool ok = true;

   if( this->IsWithin( deleteRegion ))
   {
      TGS_Region_c deleteMinRegion( deleteRegion, -1.0 * this->minGrid_ );

      // Iterate over region, deleting any solid regions intersecting region
      TTPT_Tile_c< T >* psolidTile = 0;
      while( this->IsSolid( TTP_IS_SOLID_ANY, deleteMinRegion, &psolidTile ))
      {
         TTPT_Tile_c< T > solidTile( *psolidTile );
         const TGS_Region_c& solidRegion = solidTile.GetRegion( );
         unsigned int solidCount = solidTile.GetCount( );
         const T* psolidData = solidTile.GetData( );

         ok = this->Delete( psolidTile );
         if( !ok )
            break;

         TGS_RegionList_t subRegionList( 4 );
         solidRegion.FindDifference( deleteMinRegion, TGS_ORIENT_VERTICAL,
                                     &subRegionList, this->minGrid_, this->minGrid_ );
         for( size_t i = 0; i < subRegionList.GetLength( ); ++i )
         {
	    TTPT_Tile_c< T > subTile( TTP_TILE_SOLID, *subRegionList[i], 
                                      solidCount, psolidData );
            TGS_OrientMode_t addOrient = TGS_ORIENT_UNDEFINED;
            ok = this->AddPerNew_( subTile, addOrient );
            if( !ok )
               break;
         }
         if( !ok )
            break;
      }
   }
   else if( !this->IsWithin( deleteRegion ))
   {
      // Report message: delete tile region not within tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_DELETE_PER_INTERSECT_INVALID, 
                                       "TTPT_TilePlane_c::DeletePerIntersectMin_", 
                                       &deleteRegion, &tilePlaneRegion );
   }
   return( ok );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerIntersectMin_( 
      const TGS_Region_c& deleteRegion,
      const T&            deleteData )
{
   bool ok = true;

   if( this->IsWithin( deleteRegion ))
   {
      TGS_Region_c deleteMinRegion( deleteRegion, -1.0 * this->minGrid_ );

      // Iterate over region, deleting any solid regions intersecting region
      TTPT_Tile_c< T >* psolidTile = 0;
      while( this->IsSolid( TTP_IS_SOLID_ANY, deleteMinRegion, deleteData, &psolidTile ))
      {
         TTPT_Tile_c< T > solidTile( *psolidTile );
         const TGS_Region_c& solidRegion = solidTile.GetRegion( );
         unsigned int solidCount = solidTile.GetCount( );
         const T* psolidData = solidTile.GetData( );

	 if( !psolidTile->FindData( deleteData ))
            continue;

         TTPT_Tile_c< T > replaceTile( *psolidTile );
         ok = replaceTile.DeleteData( deleteData );
         if( !ok )
            break;

         ok = this->Delete( psolidTile );
         if( !ok )
            break;

         TGS_RegionList_t subRegionList( 4 );
         solidRegion.FindDifference( deleteMinRegion, TGS_ORIENT_VERTICAL,
                                     &subRegionList, this->minGrid_, this->minGrid_ );
         for( size_t i = 0; i < subRegionList.GetLength( ); ++i )
         {
	    TTPT_Tile_c< T > subTile( TTP_TILE_SOLID, *subRegionList[i], 
                                      solidCount, psolidData );
            TGS_OrientMode_t addOrient = TGS_ORIENT_UNDEFINED;
            ok = this->AddPerNew_( subTile, addOrient );
            if( !ok )
               break;
         }
         if( !ok )
            break;

	 if( replaceTile.IsValid( ) && replaceTile.HasData( ))
         {
            TGS_OrientMode_t addOrient = TGS_ORIENT_UNDEFINED;
	    ok = this->AddPerNew_( replaceTile, addOrient );
            if( !ok )
               break;
         }
      }
   }
   else if( !this->IsWithin( deleteRegion ))
   {
      // Report message: delete tile region not within tile plane's region
      const TGS_Region_c& tilePlaneRegion = this->GetRegion( );
      ok = this->ShowInternalMessage_( TTP_MESSAGE_DELETE_PER_INTERSECT_INVALID, 
                                       "TTPT_TilePlane_c::DeletePerIntersectMin_", 
                                       &deleteRegion, &tilePlaneRegion );
   }
   return( ok );
}

//===========================================================================//
// Method         : DeletePerRightSide_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerRightSide_(
      const TTPT_Tile_c< T >& deleteTile )
{
   bool ok = true;

   // Iterate along right side of a "dead" delete tile from upper to lower...
   // splitting and merging neighbor clear tiles, where possible
   TTPT_TilePlaneIter_c< T > rightTileIter( *this, deleteTile, TC_SIDE_RIGHT );
   TTPT_Tile_c< T >* prightTile = 0;
   while( ok && rightTileIter.Next( &prightTile ))
   {
      TGS_Region_c rightRegion = prightTile->GetRegion( );
      const TGS_Region_c& deleteRegion = deleteTile.GetRegion( );
      TGS_Point_c clearPoint( deleteRegion.x1, 
			      TCT_Max( deleteRegion.y1, rightRegion.y1 ));

      if( prightTile->IsClear( ))
      {
         // Split "dead" left tile per adjacent neighbor clear right tile
         ok = this->Split( prightTile, TC_SIDE_LEFT );
         if( !ok )
            break;

         // Merge "dead" left tile (possibly split tile) with neighbor tile
         ok = this->Merge( prightTile, TC_SIDE_LEFT );
         if( !ok )
            break;

         // Test and continue merge IFF merged "dead" tile with right tile
         TTPT_Tile_c< T >* pclearTile = this->Find( clearPoint );
         const TGS_Region_c& clearRegion = pclearTile->GetRegion( );
         if( TCTF_IsNEQ( clearRegion.x2, deleteRegion.x2 ))
	 {
            // And merge neighbor lower tile with merged tile, if possible
            pclearTile = this->Find( clearPoint );
            ok = this->Merge( pclearTile, TC_SIDE_LOWER );
            if( !ok )
               break;

            // And merge neighbor upper tile with merged tile too, if possible
            pclearTile = this->Find( clearPoint );
            ok = this->Merge( pclearTile, TC_SIDE_UPPER );
            if( !ok )
               break;
         }
      }
   }

   // Iterate along right side of a "dead" delete tile again...
   // deleting (ie. splitting and merging per neighbor's left side clear tiles
   TGS_Region_c deleteRegion = deleteTile.GetRegion( );
   while( ok && TCTF_IsLE( deleteRegion.y1, deleteRegion.y2 ))
   {
      prightTile = this->Find( deleteRegion.x2 + this->minGrid_, 
                               deleteRegion.y1 );
      if( !prightTile )
	 break;

      TGS_Region_c rightRegion = prightTile->GetRegion( );
      if( prightTile->IsClear( ) &&
          TCTF_IsGT( rightRegion.x1, deleteRegion.x2 ))
      {
         // Split and merge neighbor right tile with "dead" tile too
         ok = this->DeletePerLeftSide_( *prightTile );
         if( !ok )
            break;
      }
      deleteRegion.y1 = rightRegion.y2 + this->minGrid_;
   }
   return( ok );
}

//===========================================================================//
// Method         : DeletePerLeftSide_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerLeftSide_(
      const TTPT_Tile_c< T >& deleteTile )
{
   bool ok = true;

   // Iterate along left side of "dead" delete tile from lower to upper...
   // splitting and merging neighbor clear tiles, where possible
   TGS_Region_c iterRegion( deleteTile.GetRegion( ));
   while( ok && TCTF_IsLE( iterRegion.y1, iterRegion.y2 ))
   {
      TGS_Point_c leftPoint( iterRegion.x1 - this->minGrid_, iterRegion.y1 );
      TTPT_Tile_c< T >* pleftTile = this->Find( leftPoint );
      if( !pleftTile )
	break;

      TGS_Region_c leftRegion = pleftTile->GetRegion( );
      iterRegion.y1 = leftRegion.y2 + this->minGrid_;

      if( !pleftTile->IsClear( ))
         continue;

      // Split "dead" right tiles and/or neighbor clear left tile based ...
      // on adjacent neighbor clear left tile
      const TGS_Region_c& deleteRegion = deleteTile.GetRegion( );
      TGS_Point_c rightPoint( deleteRegion.x1, 
			      TCT_Max( deleteRegion.y1, leftRegion.y1 ));

      // Find "dead" right tile w.r.t. current neighbor left tile's region
      TTPT_Tile_c< T >* prightTile = this->Find( rightPoint );
      TGS_Region_c rightRegion = prightTile->GetRegion( );
      while( ok && TCTF_IsGE( leftRegion.y2, rightRegion.y2 ))
      {
         // Split neighbor left tile based on "dead" right tile
         ok = this->Split( prightTile, TC_SIDE_LEFT );
         if( !ok )
            break;

         // Merge neighbor left tile with "dead" right tile, if possible
         ok = this->Merge( prightTile, TC_SIDE_LEFT );
         if( !ok )
            break;

         // Test and continue merge IFF merged "dead" tile with left tile
         TTPT_Tile_c< T >* pclearTile = this->Find( rightPoint );
         const TGS_Region_c& clearRegion = pclearTile->GetRegion( );
         if( TCTF_IsNEQ( clearRegion.x1, iterRegion.x1 ))
	 {
            // And merge neighbor lower tile with merged tile, if possible
            pclearTile = this->Find( rightPoint );
            ok = this->Merge( pclearTile, TC_SIDE_LOWER );
            if( !ok )
               break;

            // And merge neighbor upper tile with merged tile too, if possible
            pclearTile = this->Find( rightPoint );
            ok = this->Merge( pclearTile, TC_SIDE_UPPER );
            if( !ok )
               break;
         }

         // Continue split and merge on right tiles based on current left tile
         rightPoint.y = rightRegion.y2 + this->minGrid_;
         prightTile = this->Find( rightPoint );
         if( !prightTile )
	    break;

         rightRegion = prightTile->GetRegion( );
      }
      if( !ok )
         break;

      // Split "dead" right tile based on adjacent neighbor clear left tile
      TGS_Region_c leftSubRegion( leftRegion );
      while( ok && TCTF_IsLE( leftSubRegion.y1, leftSubRegion.y2 ))
      {
         pleftTile = this->Find( leftSubRegion.x2, leftSubRegion.y1 ); 
         ok = this->Split( pleftTile, TC_SIDE_RIGHT );
         if( !ok )
            break;

         // Merge "dead" right tile with neighbor left tile, if possible
         ok = this->Merge( pleftTile, TC_SIDE_RIGHT );
         if( !ok )
            break;

         // And merge "dead" lower tile with merged tile, if possible
         pleftTile = this->Find( leftSubRegion.x2, leftSubRegion.y1 );
         ok = this->Merge( pleftTile, TC_SIDE_LOWER );
         if( !ok )
            break;

         // And merge neighbor upper tile with merged tile, if possible
         pleftTile = this->Find( leftSubRegion.x2, leftSubRegion.y1 );
         ok = this->Merge( pleftTile, TC_SIDE_UPPER );
         if( !ok )
            break;

         pleftTile = this->Find( leftSubRegion.x2, leftSubRegion.y1 );

         leftRegion = pleftTile->GetRegion( );
         leftSubRegion.y1 = leftRegion.y2 + this->minGrid_;
      }
      if( !ok )
         break;
   }
   return( ok );
}

//===========================================================================//
// Method         : DeletePerLowerSide_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerLowerSide_(
      const TTPT_Tile_c< T >& deleteTile )
{
   bool ok = true;

   // Try to merge lower side of "dead" delete tile with neighboring tile
   const TGS_Region_c& deleteRegion = deleteTile.GetRegion( );
   TGS_Point_c clearPoint( deleteRegion.x1, deleteRegion.y1 );

   TTPT_Tile_c< T >* pclearTile = this->Find( clearPoint );
   ok = this->Merge( pclearTile, TC_SIDE_LOWER );

   return( ok );
}

//===========================================================================//
// Method         : DeletePerUpperSide_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::DeletePerUpperSide_(
      const TTPT_Tile_c< T >& deleteTile )
{
   bool ok = true;

   // Try to merge upper side of "dead" delete tile with neighboring tile
   const TGS_Region_c& deleteRegion = deleteTile.GetRegion( );
   TGS_Point_c clearPoint( deleteRegion.x1, deleteRegion.y1 );

   TTPT_Tile_c< T >* pclearTile = this->Find( clearPoint );
   ok = this->Merge( pclearTile, TC_SIDE_UPPER );

   return( ok );
}

//===========================================================================//
// Method         : FindPerPoint_
// Purpose        : Find and return a pointer to a tile based on the given 
//                  point (x,y) location.  The returned tile pointer may 
//                  identify be either a "clear" or a "solid" tile.  A null 
//                  pointer is returned if the given point is not valid (ie. 
//                  is not within the current tile plane's area).
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindPerPoint_( 
      const TGS_Point_c&      point,
            TTPT_Tile_c< T >* ptile ) const
{
   TTPT_Tile_c< T >* ptile_ = ptile;

   // Search from "most-recent" tile, if possible, else from "lower-left" tile
   TTPT_Tile_c< T >* ptileMR = 0;
   if( this->ptileMRC_ && this->ptileMRS_ )
   {
      const TGS_Region_c& regionMRC = this->ptileMRC_->GetRegion( );
      const TGS_Region_c& regionMRS = this->ptileMRS_->GetRegion( );

      double distMRC = TCT_Min( regionMRC.FindDistance( point, TGS_CORNER_LL ),
                                regionMRC.FindDistance( point, TGS_CORNER_UR ));
      double distMRS = TCT_Min( regionMRS.FindDistance( point, TGS_CORNER_LL ),
                                regionMRS.FindDistance( point, TGS_CORNER_UR ));

      ptileMR = ( TCTF_IsLT( distMRC, distMRS ) ? this->ptileMRC_ : this->ptileMRS_ );
   }
   else if( this->ptileMRC_ )
   {
      ptileMR = this->ptileMRC_;
   }
   else if( this->ptileMRS_ )
   {
      ptileMR = this->ptileMRS_;
   }

   if( ptile && ptileMR )
   {
      const TGS_Region_c& region = ptile->GetRegion( );
      const TGS_Region_c& regionMR = ptileMR->GetRegion( );

      double dist = TCT_Min( region.FindDistance( point, TGS_CORNER_LL ),
                             region.FindDistance( point, TGS_CORNER_UR ));
      double distMR = TCT_Min( regionMR.FindDistance( point, TGS_CORNER_LL ),
                               regionMR.FindDistance( point, TGS_CORNER_UR ));

      ptile = ( TCTF_IsLT( dist, distMR ) ? ptile : ptileMR );
   }
   if( !ptile )
   {
      ptile = ( ptileMR ? ptileMR : this->ptileLL_ );
   }

   // Validate point is located somewhere within this tile plane
   if( ptile && !this->IsWithin( point ))
   {
      ptile = 0;
   }
   
   if( ptile && !ptile->IsWithin( point ))
   {
      TTPT_Tile_c< T >* ptileLowerLeft = ( ptile ? ptile->GetStitchLowerLeft( ) : 0 );
      TTPT_Tile_c< T >* ptileUpperRight = ( ptile ? ptile->GetStitchUpperRight( ) : 0 );
      TTPT_Tile_c< T >* ptileLeftLower = ( ptile ? ptile->GetStitchLeftLower( ) : 0 );
      TTPT_Tile_c< T >* ptileRightUpper = ( ptile ? ptile->GetStitchRightUpper( ) : 0 );
      if( ptileLowerLeft && ptileLowerLeft->IsWithin( point ))
      {
         ptile = ptileLowerLeft;
      }
      else if( ptileUpperRight && ptileUpperRight->IsWithin( point ))
      {
         ptile = ptileUpperRight;
      }
      else if( ptileLeftLower && ptileLeftLower->IsWithin( point ))
      {
         ptile = ptileLeftLower;
      }
      else if( ptileRightUpper && ptileRightUpper->IsWithin( point ))
      {
         ptile = ptileRightUpper;
      }
      else if( ptile &&
	     ( ptileLowerLeft || ptileRightUpper ) &&
             ( ptile->IsGreaterThan( point, TGS_ORIENT_HORIZONTAL ) ||
               ptile->IsLessThan( point, TGS_ORIENT_VERTICAL )))
      {
         double distLowerLeft = ( ptileLowerLeft ? ptileLowerLeft->FindDistance( point ) : TC_FLT_MAX );
         double distRightUpper = ( ptileRightUpper ? ptileRightUpper->FindDistance( point ) : TC_FLT_MAX );
         ptile = ( TCTF_IsLE( distLowerLeft, distRightUpper ) ? ptileLowerLeft : ptileRightUpper );
      }
      else if( ptile &&
	     ( ptileUpperRight || ptileLeftLower ) &&
             ( ptile->IsLessThan( point, TGS_ORIENT_HORIZONTAL ) ||
               ptile->IsGreaterThan( point, TGS_ORIENT_VERTICAL )))
      {
         double distUpperRight = ( ptileUpperRight ? ptileUpperRight->FindDistance( point ) : TC_FLT_MAX );
         double distLeftLower = ( ptileLeftLower ? ptileLeftLower->FindDistance( point ) : TC_FLT_MAX );
         ptile = ( TCTF_IsLE( distUpperRight, distLeftLower ) ? ptileUpperRight : ptileLeftLower );
      }
   }

   // Iterate until tile found that contains the given point
   while( ptile && !ptile->IsWithin( point ))
   {
      // Iterate up/down until tile found whose vertical range has given point
      size_t iterVerticalCt = 0;
      while( ptile && !ptile->IsWithin( point, TGS_ORIENT_VERTICAL ))
      {
         if( ptile->IsLessThan( point, TGS_ORIENT_VERTICAL ))
         {
            // Iterate upward by following right-upper stitch
            ptile = ptile->GetStitchRightUpper( );
         }
         else if( ptile->IsGreaterThan( point, TGS_ORIENT_VERTICAL ))
         {
            // Iterate downwards by following left-lower stitch
            ptile = ptile->GetStitchLeftLower( );
         }

         // Stop vertical search effort after 'n' iterations 
         // (ie. go on to attempt search horizontally for 'n' iterations)
         if( ++iterVerticalCt > 10 )
            break;
      }

      // Test and update tile if up/down may be closer to given point
      if( ptile && !ptile_ &&
          ptile->IsGreaterThan( point, TGS_ORIENT_HORIZONTAL ) &&
          ptile->GetStitchRightUpper( ))
      {
         TTPT_Tile_c< T >* ptileUpper = ptile->GetStitchRightUpper( );
         const TGS_Region_c& regionUpper = ptileUpper->GetRegion( );
         const TGS_Region_c& region = ptile->GetRegion( );
         if( TCTF_IsLT( fabs( regionUpper.x1 - point.x ), fabs( region.x1 - point.x )))
         {
            ptile = ptile_ = ptileUpper;
         }
      }
      else if( ptile && !ptile_ &&
               ptile->IsLessThan( point, TGS_ORIENT_HORIZONTAL ) &&
               ptile->GetStitchLeftLower( ))
      {
         TTPT_Tile_c< T >* ptileLower = ptile->GetStitchLeftLower( );
         const TGS_Region_c& regionLower = ptileLower->GetRegion( );
         const TGS_Region_c& region = ptile->GetRegion( );
         if( TCTF_IsLT( fabs( regionLower.x2 - point.x ), fabs( region.x2 - point.x )))
         {
            ptile = ptile_ = ptileLower;
         }
      }

      // Iterate left/right until tile found whose horizontal range has given point
      size_t iterHorizontalCt = 0;
      while( ptile && !ptile->IsWithin( point, TGS_ORIENT_HORIZONTAL ))
      {
         if( ptile->IsLessThan( point, TGS_ORIENT_HORIZONTAL ))
         {
            // Iterate to the right by following upper-right stitch
            ptile = ptile->GetStitchUpperRight( );   
         }
         else if( ptile->IsGreaterThan( point, TGS_ORIENT_HORIZONTAL ))
         {
            // Iterate to the left by following lower-left stitch
            ptile = ptile->GetStitchLowerLeft( ); 
         }

         // Stop horizontal search effort after 'n' iterations 
         // (ie. go on to attempt search vertically for 'n' iterations)
         if( ++iterHorizontalCt > 10 )
            break;
      }
   }

   // Update "most-recent" tile pointer based on search result
   if( ptile )
   {
      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      if( ptile->IsClear( ))
      {
         if( !ptilePlane->ptileMRC_ )
         {
            ptilePlane->ptileMRC_ = const_cast< TTPT_Tile_c< T >* >( ptile );
         }
         else
         {
            const TGS_Region_c& tileRegionMRC = ptilePlane->ptileMRC_->GetRegion( );
            const TGS_Region_c& tileRegion = ptile->GetRegion( );
            if( !tileRegion.IsAdjacent( tileRegionMRC, this->minGrid_ ) ||
                TCTF_IsNEQ( tileRegion.x1, this->region_.x1 ) ||
                TCTF_IsNEQ( tileRegion.x2, this->region_.x2 ))
            {
               ptilePlane->ptileMRC_ = const_cast< TTPT_Tile_c< T >* >( ptile );
            }
         }
      }
      else if( ptile->IsSolid( ))
      {
         ptilePlane->ptileMRS_ = const_cast< TTPT_Tile_c< T >* >( ptile );
      }
   }
   return( ptile );
}

//===========================================================================//
// Method         : FindPerExact_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindPerExact_(
      const TGS_Region_c&  region,
            TTP_TileMode_t tileMode ) const
{
   TTPT_Tile_c< T >* ptile = 0;

   if( this->IsWithin( region ))
   {
      TGS_Point_c point( region.x1, region.y1 );
      ptile = this->FindPerPoint_( point );

      ptile = ( ptile && ptile->GetMode( ) & tileMode ? ptile : 0 );
      ptile = ( ptile && ptile->GetRegion( ) == region ? ptile : 0 );
   }
   return( ptile );
}

//===========================================================================//
// Method         : FindPerWithin_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindPerWithin_(
      const TGS_Region_c&  region,
            TTP_TileMode_t tileMode ) const
{
   TTPT_Tile_c< T >* ptile = 0;

   if( this->IsWithin( region ))
   {
      TGS_Point_c point( region.x1, region.y1 );
      ptile = this->FindPerPoint_( point );

      ptile = ( ptile && ptile->GetMode( ) & tileMode ? ptile : 0 );
      ptile = ( ptile && ptile->IsWithin( region ) ? ptile : 0 );
   }
   return( ptile );
}

//===========================================================================//
// Method         : FindPerIntersect_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindPerIntersect_(
      const TGS_Region_c&  region,
            TTP_TileMode_t tileMode ) const
{
   TTPT_Tile_c< T >* ptile = 0;

   if( this->IsWithin( region ))
   {
      bool findOK = true;

      if( !ptile && findOK )
      {
         TGS_Point_c point( region.x1, region.y1 );
         TTPT_Tile_c< T >* ptileLL = this->FindPerPoint_( point );
         if( ptileLL && ( ptileLL->GetMode( ) & tileMode ) &&
	     ptileLL->IsIntersecting( region ))
	 {
	    ptile = ptileLL;
         }
	 else if( ptileLL && !( ptileLL->GetMode( ) & tileMode ) &&
                  ptileLL->IsWithin( region ))
	 {
            findOK = false;
         }
      }

      if( !ptile && findOK )
      {
         TGS_Point_c point( region.x2, region.y2 );
         TTPT_Tile_c< T >* ptileUR = this->FindPerPoint_( point );
         if( ptileUR && ( ptileUR->GetMode( ) & tileMode ) &&
	     ptileUR->IsIntersecting( region ))
	 {
	    ptile = ptileUR;
         }
	 else if( ptileUR && !( ptileUR->GetMode( ) & tileMode ) &&
                  ptileUR->IsWithin( region ))
	 {
            findOK = false;
         }
      }

      if( !ptile && findOK )
      {
         TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
         ptilePlane->tilePlaneIter_.Init( *this, region );
         while( ptilePlane->tilePlaneIter_.Next( &ptile, tileMode ))
         {  
            if( ptile->IsIntersecting( region ))
	       break;
         }
      }
   }
   return( ptile );
}

//===========================================================================//
// Method         : FindNearestBySides_
// Purpose        : Find and return a pointer to the nearest "solid" tile
//                  found neighboring the given region, if any, based on the
//                  given reference region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindNearestBySides_( 
      const TGS_Region_c&      refRegion,
      const TTPT_Tile_c< T >&  searchTile,
      const TGS_OrientMode_t*  porientMode ) const
{
   TTPT_Tile_c< T >* pnearestTile = 0;
   double nearestDistance = TC_FLT_MAX;
   
   // Iterate sides, searching for nearest "solid" tile w.r.t given region
   if( TCTF_IsGT( nearestDistance, 0.0 ))
   {
      TTPT_Tile_c< T >* pleftTile = this->FindNearestBySide_( refRegion,
                                                              searchTile, 
                                                              TC_SIDE_LEFT );
      if( pleftTile )
      {
         double leftDistance = pleftTile->FindDistance( refRegion );
	 if(( porientMode ) &&
            ( *porientMode != TGS_ORIENT_UNDEFINED ) &&
            ( *porientMode != pleftTile->FindOrient( )))
	 {
	    leftDistance = TC_FLT_MAX;
	 }

         if( TCTF_IsGT( nearestDistance, leftDistance ))
         {
            nearestDistance = leftDistance;
            pnearestTile = pleftTile;
         }
      }
   }

   if( TCTF_IsGT( nearestDistance, 0.0 ))
   {
      TTPT_Tile_c< T >* prightTile = this->FindNearestBySide_( refRegion,
                                                               searchTile, 
                                                               TC_SIDE_RIGHT );
      if( prightTile )
      {
         double rightDistance = prightTile->FindDistance( refRegion );
	 if(( porientMode ) &&
            ( *porientMode != TGS_ORIENT_UNDEFINED ) &&
            ( *porientMode != prightTile->FindOrient( )))
	 {
	    rightDistance = TC_FLT_MAX;
	 }

         if( TCTF_IsGT( nearestDistance, rightDistance ))
         {
            nearestDistance = rightDistance;
            pnearestTile = prightTile;
         }
      }
   }

   if( TCTF_IsGT( nearestDistance, 0.0 ))
   {
      TTPT_Tile_c< T >* plowerTile = this->FindNearestBySide_( refRegion,
                                                               searchTile, 
                                                               TC_SIDE_LOWER );
      if( plowerTile )
      {
         double lowerDistance = plowerTile->FindDistance( refRegion );
	 if(( porientMode ) &&
            ( *porientMode != TGS_ORIENT_UNDEFINED ) &&
            ( *porientMode != plowerTile->FindOrient( )))
	 {
	    lowerDistance = TC_FLT_MAX;
	 }

         if( TCTF_IsGT( nearestDistance, lowerDistance ))
         {
            nearestDistance = lowerDistance;
            pnearestTile = plowerTile;
         }
      }
   }

   if( TCTF_IsGT( nearestDistance, 0.0 ))
   {
      TTPT_Tile_c< T >* pupperTile = this->FindNearestBySide_( refRegion,
                                                             searchTile, 
                                                             TC_SIDE_UPPER );
      if( pupperTile )
      {
         double upperDistance = pupperTile->FindDistance( refRegion );
	 if(( porientMode ) &&
            ( *porientMode != TGS_ORIENT_UNDEFINED ) &&
            ( *porientMode != pupperTile->FindOrient( )))
	 {
	    upperDistance = TC_FLT_MAX;
	 }

         if( TCTF_IsGT( nearestDistance, upperDistance ))
         {
            nearestDistance = upperDistance;
            pnearestTile = pupperTile;
         }
      }
   }
   return( pnearestTile );
}

//===========================================================================//
// Method         : FindNearestBySide_
// Purpose        : Find and return a pointer to the nearest "solid" tile
//                  found neighboring the given region side, if any, based on
//                  the given reference region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindNearestBySide_( 
      const TGS_Region_c&     refRegion,
      const TTPT_Tile_c< T >& searchTile,
            TC_SideMode_t     searchSide ) const
{
   TTPT_Tile_c< T >* pnearestTile = 0;
   double nearestDistance = TC_FLT_MAX;
   
   // Iterate side, searching for nearest "solid" tile w.r.t given region
   TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
   ptilePlane->tilePlaneIter_.Init( *this, searchTile, searchSide );
   TTPT_Tile_c< T >* psolidTile = 0;
   while( ptilePlane->tilePlaneIter_.Next( &psolidTile, TTP_TILE_SOLID ))
   {
      double solidDistance = psolidTile->FindDistance( refRegion );
      if( TCTF_IsGT( nearestDistance, solidDistance ))
      {
         nearestDistance = solidDistance;
         pnearestTile = psolidTile;
      }
   }
   return( pnearestTile );
}

//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindNearestBySide_( 
      const TGS_Region_c& refRegion,
            TC_SideMode_t searchSide,
            double        searchDistance ) const
{
   TTPT_Tile_c< T >* pnearestTile = 0;
   double nearestDistance = TC_FLT_MAX;

   if( searchSide == TC_SIDE_LEFT )
   {
      TGS_Point_c leftPoint( refRegion.x1 - this->minGrid_, refRegion.y1 );
      while( TCTF_IsLE( leftPoint.y, refRegion.y2 ))
      {
         TTPT_Tile_c< T >* pleftTile = this->Find( leftPoint );
         if( !pleftTile )
            break;

         if( TCTF_IsGT( refRegion.x1 - pleftTile->GetRegion( ).x2, searchDistance ))
            break;

         leftPoint.y = pleftTile->GetRegion( ).y2 + this->minGrid_;
         
         TTPT_Tile_c< T >* psolidTile = 0;
         if( pleftTile->IsSolid( ))
         {
            psolidTile = pleftTile;
         }
         else if( pleftTile->IsClear( ) &&
                  pleftTile->GetStitchLowerLeft( ))
         {
            psolidTile = pleftTile->GetStitchLowerLeft( );
         }
   
         if( psolidTile )
         {
            double solidDistance = psolidTile->FindDistance( refRegion );
            if( TCTF_IsGT( nearestDistance, solidDistance ))
            {
               nearestDistance = solidDistance;
               pnearestTile = psolidTile;
            }
         }
      }
   }
   
   if( searchSide == TC_SIDE_RIGHT )
   {
      TGS_Point_c rightPoint( refRegion.x2 + this->minGrid_, refRegion.y2 );
      while( TCTF_IsGE( rightPoint.y, refRegion.y1 ))
      {
         TTPT_Tile_c< T >* prightTile = this->Find( rightPoint );
         if( !prightTile )
            break;
   
         if( TCTF_IsGT( prightTile->GetRegion( ).x1 - refRegion.x2, searchDistance ))
            break;

         rightPoint.y = prightTile->GetRegion( ).y1 - this->minGrid_;
   
         TTPT_Tile_c< T >* psolidTile = 0;
         if( prightTile->IsSolid( ))
         {
            psolidTile = prightTile;
         }
         else if( prightTile->IsClear( ) &&
                  prightTile->GetStitchUpperRight( ))
         {
            psolidTile = prightTile->GetStitchUpperRight( );
         }
   
         if( psolidTile )
         {
            double solidDistance = psolidTile->FindDistance( refRegion );
            if( TCTF_IsGT( nearestDistance, solidDistance ))
            {
               nearestDistance = solidDistance;
               pnearestTile = psolidTile;
            }
         }
      }
   }
   
   if( searchSide == TC_SIDE_LOWER )
   {
      TTPT_Tile_c< T >* psolidTile = 0;
   
      TGS_Point_c lowerPoint( refRegion.x1, refRegion.y1 - this->minGrid_ );
      while( TCTF_IsLE( lowerPoint.x, refRegion.x2 ))
      {
         TTPT_Tile_c< T >* plowerTile = this->Find( lowerPoint );
         if( !plowerTile )
            break;
   
         if( TCTF_IsGT( refRegion.y1 - plowerTile->GetRegion( ).y2, searchDistance ))
            break;

         lowerPoint.x = plowerTile->GetRegion( ).x2 + this->minGrid_;
   
         if( plowerTile->IsSolid( ))
         {
            psolidTile = plowerTile;
            break;
         }
         else if( plowerTile->GetRegion( ).IsWithinDx( refRegion ) &&
                  plowerTile->GetStitchLeftLower( ))
         {
            lowerPoint.x = refRegion.x1;
            lowerPoint.y = plowerTile->GetRegion( ).y1 - this->minGrid_;
         }
         else if( plowerTile->GetRegion( ).IsWithinDx( refRegion ) &&
                  !plowerTile->GetStitchLeftLower( ))
         {
            break;
         }
      }
   
      if( psolidTile )
      {
         pnearestTile = psolidTile;
      }
   }
   
   if( searchSide == TC_SIDE_UPPER )
   {
      TTPT_Tile_c< T >* psolidTile = 0;
   
      TGS_Point_c upperPoint( refRegion.x2, refRegion.y2 + this->minGrid_ );
      while( TCTF_IsGE( upperPoint.x, refRegion.x1 ))
      {
         TTPT_Tile_c< T >* pupperTile = this->Find( upperPoint );
         if( !pupperTile )
            break;
   
         if( TCTF_IsGT( pupperTile->GetRegion( ).y1 - refRegion.y2, searchDistance ))
            break;

         upperPoint.x = pupperTile->GetRegion( ).x1 - this->minGrid_;
   
         if( pupperTile->IsSolid( ))
         {
            psolidTile = pupperTile;
            break;
         }
         else if( pupperTile->GetRegion( ).IsWithinDx( refRegion ) &&
                  pupperTile->GetStitchRightUpper( ))
         {
            upperPoint.x = refRegion.x2;
            upperPoint.y = pupperTile->GetRegion( ).y2 + this->minGrid_;
         }
         else if( pupperTile->GetRegion( ).IsWithinDx( refRegion ) &&
                  !pupperTile->GetStitchRightUpper( ))
         {
            break;
         }
      }
   
      if( psolidTile )
      {
         pnearestTile = psolidTile;
      }
   }
   return( pnearestTile );
}

//===========================================================================//
// Method         : FindNearestByRegion_
// Purpose        : Find and return a pointer to the nearest "solid" tile
//                  found within the given region, if any, based on the given 
//                  reference region.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindNearestByRegion_( 
      const TGS_Region_c&     refRegion,
      const TGS_Region_c&     searchRegion,
      const TGS_OrientMode_t* porientMode ) const
{
   TTPT_Tile_c< T >* pnearestTile = 0;
   double nearestDistance = TC_FLT_MAX;
   
   // Iterate region, searching for nearest "solid" tile w.r.t given region
   TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
   ptilePlane->tilePlaneIter_.Init( *this, searchRegion );
   TTPT_Tile_c< T >* psolidTile = 0;
   while( ptilePlane->tilePlaneIter_.Next( &psolidTile, TTP_TILE_SOLID ))
   {
      double solidDistance = psolidTile->FindDistance( refRegion );
      if(( porientMode ) &&
         ( *porientMode != TGS_ORIENT_UNDEFINED ) &&
         ( *porientMode != psolidTile->FindOrient( )))
      {
         solidDistance = TC_FLT_MAX;
      }

      const TGS_Region_c& solidRegion = psolidTile->GetRegion( );
      if( !searchRegion.IsOverlapping( solidRegion ))
      {
         solidDistance = TC_FLT_MAX;
      }

      if( TCTF_IsGT( nearestDistance, solidDistance ))
      {
         nearestDistance = solidDistance;
         pnearestTile = psolidTile;

         if( TCTF_IsEQ( nearestDistance, 0.0 ))
            break;
      }
   }
   return( pnearestTile );
}

//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::FindNearestByRegion_( 
      const TGS_Region_c&     refRegion,
            double            searchDistance,
      const TGS_OrientMode_t* porientMode ) const
{
   TTPT_Tile_c< T >* pnearestTile = 0;
   double nearestDistance = TC_FLT_MAX;
   
   TGS_Region_c searchRegion( refRegion, searchDistance + this->minGrid_, 
                              TGS_SNAP_MIN_GRID );

   // Iterate region, searching for nearest "solid" tile w.r.t given region
   TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
   ptilePlane->tilePlaneIter_.Init( *this, searchRegion );
   TTPT_Tile_c< T >* psolidTile = 0;
   while( ptilePlane->tilePlaneIter_.Next( &psolidTile, TTP_TILE_SOLID ))
   {
      double solidDistance = psolidTile->FindDistance( refRegion );
      if(( porientMode ) &&
         ( *porientMode != TGS_ORIENT_UNDEFINED ) &&
         ( *porientMode != psolidTile->FindOrient( )))
      {
         solidDistance = TC_FLT_MAX;

	 continue;
      }

      if( TCTF_IsGT( nearestDistance, solidDistance ))
      {
         nearestDistance = solidDistance;
         pnearestTile = psolidTile;

         if( TCTF_IsEQ( nearestDistance, 0.0 ))
            break;
      }
      else if( TCTF_IsGT( nearestDistance, this->minGrid_ ) &&
               TCTF_IsLT( nearestDistance * TC_SQRT2 + 10.0 * this->minGrid_, solidDistance ))
      {
         TGS_Region_c nearestRegion( refRegion, nearestDistance + this->minGrid_,                                      
                                     TGS_SNAP_MIN_GRID );

         ptilePlane->tilePlaneIter_.Init( *this, nearestRegion );
      }
   }
   return( pnearestTile );
}

//===========================================================================//
// Method         : FindCountRefresh_
// Purpose        : Counts total number of clear or solid tiles defined
//                  within this tile plane.  The current count is cached and
//                  may be valid based on whether any tiles have been added 
//                  or deleted since the last count
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::FindCountRefresh_(
      void ) const
{
   if( !this->count_.isValid )
   {
      this->FindCountRefresh_( this->region_ );

      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->count_.isValid = true;
   }
}

//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::FindCountRefresh_(
      const TGS_Region_c& region ) const
{
   TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
   ptilePlane->count_.clear = 0;
   ptilePlane->count_.solid = 0;

   ptilePlane->tilePlaneIter_.Init( *this, region );
   TTPT_Tile_c< T >* ptile = 0;
   while( ptilePlane->tilePlaneIter_.Next( &ptile ))
   {
      if( ptile->IsClear( ))
      {
         ++ptilePlane->count_.clear;
      }
      else // if( ptile->IsSolid( ))
      {
         ++ptilePlane->count_.solid;
      }
   }
   ptilePlane->count_.isValid = false;
}

//===========================================================================//
// Method         : SplitRegionCoords_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::SplitRegionCoords_( 
            TTPT_Tile_c< T >* ptileA,
            TTPT_Tile_c< T >* ptileB,
      const TGS_Line_c&       line,
            TGS_DirMode_t     dirMode,
            TGS_OrientMode_t  orientMode )
{
   TGS_Region_c regionA = ptileA->GetRegion( );
   TGS_Region_c regionB = ptileB->GetRegion( );

   if( orientMode == TGS_ORIENT_HORIZONTAL )
   {
      // Update tile (T)'s y2 and (T')'s y1 region coords
      if( dirMode == TGS_DIR_DOWN )
      {
         regionA.y2 = line.y1 - this->minGrid_;
         regionB.y1 = line.y1;
      }
      else // if( dirMode == TGS_DIR_UP )
      {
         regionA.y2 = line.y2;
         regionB.y1 = line.y2 + this->minGrid_;
      }
   }
   else // if( orientMode == TGS_ORIENT_VERTICAL )
   {
      // Update tile (T)'s x2 and (T')'s x1 region coords
      if( dirMode == TGS_DIR_LEFT )
      {
         regionA.x2 = line.x1 - this->minGrid_;
         regionB.x1 = line.x1;
      }
      else // if( dirMode == TGS_DIR_RIGHT )
      {
         regionA.x2 = line.x2;
         regionB.x1 = line.x2 + this->minGrid_;
      }
   }

   ptileA->SetRegion( regionA );
   ptileB->SetRegion( regionB );
}


//===========================================================================//
// Method         : MergeRegionCoords_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::MergeRegionCoords_( 
      TTPT_Tile_c< T >* ptileA,
      TTPT_Tile_c< T >* ptileB,
      TGS_OrientMode_t  orientMode,
      TGS_CornerMode_t  cornerMode )
{
   TGS_Region_c regionA = ptileA->GetRegion( );
   TGS_Region_c regionB = ptileB->GetRegion( );

   if( orientMode == TGS_ORIENT_HORIZONTAL )
   {
      if( cornerMode == TGS_CORNER_LOWER_LEFT )
      {
         // Update tile (T)'s y1 per tile (T')'s y1 region coord
         regionA.y1 = regionB.y1;
      }
      if( cornerMode == TGS_CORNER_UPPER_RIGHT )
      {
         // Update tile (T)'s y2 per tile (T')'s y2 region coord
         regionA.y2 = regionB.y2;
      }
   }
   else // if( orientMode == TGS_ORIENT_VERTICAL )
   {
      if( cornerMode == TGS_CORNER_LOWER_LEFT )
      {
         // Update tile (T)'s x1 per tile (T')'s x1 region coord
         regionA.x1 = regionB.x1;
      }
      if( cornerMode == TGS_CORNER_UPPER_RIGHT )
      {
         // Update tile (T)'s x2 per tile (T')'s x2 region coord
         regionA.x2 = regionB.x2;
      }
   }

   ptileA->SetRegion( regionA );
   ptileB->SetRegion( regionB );
}


//===========================================================================//
// Method         : MergeAdjacentsByExtent_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::MergeAdjacentsByExtent_(
            TGS_OrientMode_t orientMode,
            unsigned int     tileCount,
      const T*               ptileData,
            TTP_TileMode_t   tileMode,
	    TTP_MergeMode_t  mergeMode,
            TGS_Region_c*    pmergedRegion ) const
{
   bool mergedAdjacents = false;

   TGS_Region_c initialRegion( *pmergedRegion );

   if( orientMode == TGS_ORIENT_HORIZONTAL )
   {
      this->MergeAdjacentsToLeft_( tileCount, ptileData, tileMode,
                                   mergeMode, pmergedRegion );
      this->MergeAdjacentsToRight_( tileCount, ptileData, tileMode,
                                    mergeMode, pmergedRegion );
      this->MergeAdjacentsToLower_( tileCount, ptileData, tileMode, 
                                     mergeMode, pmergedRegion );
      this->MergeAdjacentsToUpper_( tileCount, ptileData, tileMode, 
                                    mergeMode, pmergedRegion );
   }
   else // if( orientMode == TGS_ORIENT_VERTICAL )
   {
      this->MergeAdjacentsToLower_( tileCount, ptileData, tileMode, 
                                    mergeMode, pmergedRegion );
      this->MergeAdjacentsToUpper_( tileCount, ptileData, tileMode, 
                                    mergeMode, pmergedRegion );
      this->MergeAdjacentsToLeft_( tileCount, ptileData, tileMode, 
                                   mergeMode, pmergedRegion );
      this->MergeAdjacentsToRight_( tileCount, ptileData, tileMode, 
                                    mergeMode, pmergedRegion );
   }

   if( initialRegion != *pmergedRegion )
   {
      mergedAdjacents = true;
   }
   return( mergedAdjacents );
}

//===========================================================================//
// Method         : MergeAdjacentsToLeft_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::MergeAdjacentsToLeft_(
            unsigned int    tileCount,
      const T*              ptileData,
            TTP_TileMode_t  tileMode,
            TTP_MergeMode_t mergeMode,
            TGS_Region_c*   pmergedRegion ) const
{
   bool mergedAdjacents = false;

   while( true )
   {
      TTPT_Tile_c< T >* pleftTile = this->Find( pmergedRegion->x1 - this->minGrid_, 
                                                pmergedRegion->y1 );
      if(( pleftTile ) && 
         ( pleftTile->GetMode( ) == tileMode ) &&
         ( TCTF_IsLE( pleftTile->GetRegion( ).y1, pmergedRegion->y1 )) &&
         ( TCTF_IsGE( pleftTile->GetRegion( ).y2, pmergedRegion->y2 )))
      {
         if( mergeMode == TTP_MERGE_REGION )
	 {
            pmergedRegion->x1 = pleftTile->GetRegion( ).x1;
            mergedAdjacents = true;
	 }
         else if(( mergeMode == TTP_MERGE_EXACT ) &&
                 ( pleftTile->IsEqualData( tileCount, ptileData )))
	 {
            pmergedRegion->x1 = pleftTile->GetRegion( ).x1;
            mergedAdjacents = true;
	 }
         else
	 {
            break;
	 }
      }
      else if(( pleftTile ) && 
              ( pleftTile->GetMode( ) == tileMode ) &&
              ( TCTF_IsLE( pleftTile->GetRegion( ).y1, pmergedRegion->y1 )) &&
              ( TCTF_IsLT( pleftTile->GetRegion( ).y2, pmergedRegion->y2 )))
      {
         double x1 = pleftTile->GetRegion( ).x1;
         double y2 = pleftTile->GetRegion( ).y2;

         TTPT_Tile_c< T >* pneighborTile = this->Find( pmergedRegion->x1 - this->minGrid_,
                                                       pleftTile->GetRegion( ).y2 + this->minGrid_ );
         while(( pneighborTile ) &&
               ( pneighborTile->GetMode( ) == tileMode ) &&
	       ( TCTF_IsLE( pneighborTile->GetRegion( ).y1, pmergedRegion->y2 )))
         {
            x1 = TCT_Max( x1, pneighborTile->GetRegion( ).x1 );
            y2 = pneighborTile->GetRegion( ).y2;

            pneighborTile = this->Find( pmergedRegion->x1 - this->minGrid_,
                                        pneighborTile->GetRegion( ).y2 + this->minGrid_ );
         }

         if(( mergeMode == TTP_MERGE_REGION ) && 
            ( TCTF_IsGE( y2, pmergedRegion->y2 )))
	 {
            pmergedRegion->x1 = x1;
            mergedAdjacents = true;
	 }
         else
	 {
            break;
	 }
      }
      else
      {
         break;
      }
   }
   return( mergedAdjacents );
}

//===========================================================================//
// Method         : MergeAdjacentsToRight_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::MergeAdjacentsToRight_(
            unsigned int    tileCount,
      const T*              ptileData,
            TTP_TileMode_t  tileMode,
            TTP_MergeMode_t mergeMode,
            TGS_Region_c*   pmergedRegion ) const
{
   bool mergedAdjacents = false;

   while( true )
   {
      TTPT_Tile_c< T >* prightTile = this->Find( pmergedRegion->x2 + this->minGrid_, 
                                                 pmergedRegion->y2 );
      if(( prightTile ) && 
         ( prightTile->GetMode( ) == tileMode ) &&
         ( TCTF_IsLE( prightTile->GetRegion( ).y1, pmergedRegion->y1 )) &&
         ( TCTF_IsGE( prightTile->GetRegion( ).y2, pmergedRegion->y2 )))
      {
         if( mergeMode == TTP_MERGE_REGION )
	 {
            pmergedRegion->x2 = prightTile->GetRegion( ).x2;
            mergedAdjacents = true;
	 }
         else if(( mergeMode == TTP_MERGE_EXACT ) &&
                 ( prightTile->IsEqualData( tileCount, ptileData )))
	 {
            pmergedRegion->x2 = prightTile->GetRegion( ).x2;
            mergedAdjacents = true;
	 }
         else
	 {
            break;
	 }
      }
      else if(( prightTile ) && 
              ( prightTile->GetMode( ) == tileMode ) &&
              ( TCTF_IsGT( prightTile->GetRegion( ).y1, pmergedRegion->y1 )) &&
              ( TCTF_IsGE( prightTile->GetRegion( ).y2, pmergedRegion->y2 )))
      {
         double x2 = prightTile->GetRegion( ).x2;
         double y1 = prightTile->GetRegion( ).y1;

         TTPT_Tile_c< T >* pneighborTile = this->Find( pmergedRegion->x2 + this->minGrid_,
                                                       prightTile->GetRegion( ).y1 - this->minGrid_ );
         while(( pneighborTile ) &&
               ( pneighborTile->GetMode( ) == tileMode ) &&
               ( TCTF_IsGE( pneighborTile->GetRegion( ).y2, pmergedRegion->y1 )))
         {
            x2 = TCT_Min( x2, pneighborTile->GetRegion( ).x2 );
            y1 = pneighborTile->GetRegion( ).y1;

            pneighborTile = this->Find( pmergedRegion->x2 + this->minGrid_,
                                        pneighborTile->GetRegion( ).y1 - this->minGrid_ );
         }

         if(( mergeMode == TTP_MERGE_REGION ) && 
            ( TCTF_IsLE( y1, pmergedRegion->y1 )))
	 {
            pmergedRegion->x2 = x2;
            mergedAdjacents = true;
	 }
         else
	 {
            break;
	 }
      }
      else
      {
         break;
      }
   }
   return( mergedAdjacents );
}

//===========================================================================//
// Method         : MergeAdjacentsToLower_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::MergeAdjacentsToLower_(
            unsigned int    tileCount,
      const T*              ptileData,
            TTP_TileMode_t  tileMode,
            TTP_MergeMode_t mergeMode,
            TGS_Region_c*   pmergedRegion ) const
{
   bool mergedAdjacents = false;

   while( true )
   {
      TTPT_Tile_c< T >* plowerTile = this->Find( pmergedRegion->x1,
                                                 pmergedRegion->y1 - this->minGrid_ );
      if(( plowerTile ) && 
         ( plowerTile->GetMode( ) == tileMode ) &&
         ( TCTF_IsLE( plowerTile->GetRegion( ).x1, pmergedRegion->x1 )) &&
         ( TCTF_IsGE( plowerTile->GetRegion( ).x2, pmergedRegion->x2 )))
      {
         if( mergeMode == TTP_MERGE_REGION )
	 {
            pmergedRegion->y1 = plowerTile->GetRegion( ).y1;
            mergedAdjacents = true;
	 }
         else if(( mergeMode == TTP_MERGE_EXACT ) &&
                 ( plowerTile->IsEqualData( tileCount, ptileData )))
	 {
            pmergedRegion->y1 = plowerTile->GetRegion( ).y1;
            mergedAdjacents = true;
	 }
         else
	 {
            break;
	 }
      }
      else if(( plowerTile ) && 
              ( plowerTile->GetMode( ) == tileMode ) &&
              ( TCTF_IsLE( plowerTile->GetRegion( ).x1, pmergedRegion->x1 )) &&
              ( TCTF_IsLT( plowerTile->GetRegion( ).x2, pmergedRegion->x2 )))
      {
         double y1 = plowerTile->GetRegion( ).y1;
         double x2 = plowerTile->GetRegion( ).x2;

         TTPT_Tile_c< T >* pneighborTile = this->Find( plowerTile->GetRegion( ).x2 + this->minGrid_, 
                                                       pmergedRegion->y1 - this->minGrid_ );
         while(( pneighborTile ) &&
               ( pneighborTile->GetMode( ) == tileMode ) &&
               ( TCTF_IsLE( pneighborTile->GetRegion( ).x1, pmergedRegion->x2 )))
         {
            y1 = TCT_Max( y1, pneighborTile->GetRegion( ).y1 );
            x2 = pneighborTile->GetRegion( ).x2;

            pneighborTile = this->Find( pneighborTile->GetRegion( ).x2 + this->minGrid_, 
                                        pmergedRegion->y1 - this->minGrid_ );
         }

         if(( mergeMode == TTP_MERGE_REGION ) && 
            ( TCTF_IsGE( x2, pmergedRegion->x2 )))
	 {
            pmergedRegion->y1 = y1;
            mergedAdjacents = true;
	 }
         else
	 {
            break;
	 }
      }
      else
      {
         break;
      }
   }
   return( mergedAdjacents );
}

//===========================================================================//
// Method         : MergeAdjacentsToUpper_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::MergeAdjacentsToUpper_(
            unsigned int    tileCount,
      const T*              ptileData,
            TTP_TileMode_t  tileMode,
            TTP_MergeMode_t mergeMode,
            TGS_Region_c*   pmergedRegion ) const
{
   bool mergedAdjacents = false;

   while( true )
   {
      TTPT_Tile_c< T >* pupperTile = this->Find( pmergedRegion->x2,
                                                 pmergedRegion->y2 + this->minGrid_ );
      if(( pupperTile ) && 
         ( pupperTile->GetMode( ) == tileMode ) &&
         ( TCTF_IsLE( pupperTile->GetRegion( ).x1, pmergedRegion->x1 )) &&
         ( TCTF_IsGE( pupperTile->GetRegion( ).x2, pmergedRegion->x2 )))
      {
         if( mergeMode == TTP_MERGE_REGION )
	 {
            pmergedRegion->y2 = pupperTile->GetRegion( ).y2;
            mergedAdjacents = true;
	 }
         else if(( mergeMode == TTP_MERGE_EXACT ) &&
                 ( pupperTile->IsEqualData( tileCount, ptileData )))
	 {
            pmergedRegion->y2 = pupperTile->GetRegion( ).y2;
            mergedAdjacents = true;
	 }
         else
	 {
            break;
	 }
      }
      else if(( pupperTile ) && 
              ( pupperTile->GetMode( ) == tileMode ) &&
              ( TCTF_IsGT( pupperTile->GetRegion( ).x1, pmergedRegion->x1 )) &&
              ( TCTF_IsGE( pupperTile->GetRegion( ).x2, pmergedRegion->x2 )))
      {
         double y2 = pupperTile->GetRegion( ).y2;
         double x1 = pupperTile->GetRegion( ).x1;

         TTPT_Tile_c< T >* pneighborTile = this->Find( pupperTile->GetRegion( ).x1 - this->minGrid_, 
                                                       pmergedRegion->y2 + this->minGrid_ );
         while(( pneighborTile ) &&
               ( pneighborTile->GetMode( ) == tileMode ) &&
               ( TCTF_IsGE( pneighborTile->GetRegion( ).x2, pmergedRegion->x1 )))
         {
            y2 = TCT_Min( y2, pneighborTile->GetRegion( ).y2 );
            x1 = pneighborTile->GetRegion( ).x1;

            pneighborTile = this->Find( pneighborTile->GetRegion( ).x1 - this->minGrid_, 
                                        pmergedRegion->y2 + this->minGrid_ );
         }

         if(( mergeMode == TTP_MERGE_REGION ) && 
            ( TCTF_IsLE( x1, pmergedRegion->x1 )))
	 {
            pmergedRegion->y2 = y2;
            mergedAdjacents = true;
	 }
         else
	 {
            break;
	 }
      }
      else
      {
         break;
      }
   }
   return( mergedAdjacents );
}

//===========================================================================//
// Method         : JoinAdjacentsPerExact
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::JoinAdjacentsPerExact_(
      const TGS_Region_c&     region,
            TGS_RegionList_t* pregionList ) const
{
   bool joinAdjacents = false;

   const TTPT_Tile_c< T >* ptile = 0;

   ptile = this->Find( region, TTP_FIND_EXACT, TTP_TILE_SOLID );
   if( !ptile )
   {
      ptile = this->Find( region, TTP_FIND_WITHIN, TTP_TILE_SOLID );
   }

   if( ptile )
   {
      joinAdjacents = this->JoinAdjacents( *ptile, pregionList );
   }
   return( joinAdjacents );
}

//===========================================================================//
// Method         : JoinAdjacentsPerIntersect
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::JoinAdjacentsPerIntersect_(
      const TGS_Region_c&     region,
            TGS_RegionList_t* pregionList ) const
{
   bool joinAdjacents = false;

   TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
   ptilePlane->tilePlaneIter_.Init( *this, region );
   TTPT_Tile_c< T >* ptile = 0;
   while( ptilePlane->tilePlaneIter_.Next( &ptile, TTP_TILE_SOLID ))
   {
      const TGS_Region_c& tileRegion = ptile->GetRegion( );
      if( region != tileRegion )
      {
         pregionList->Add( tileRegion );

         joinAdjacents = true;
      }

      TGS_RegionList_t regionList;
      if( this->JoinAdjacents( *ptile, &regionList )) 
      {
         for( size_t i = 0; i < regionList.GetLength( ); ++i )
         { 
	    if( pregionList->IsMember( *regionList[i] ))
	       continue;

            pregionList->Add( *regionList[i] );

            joinAdjacents = true;
         }
      }
   }
   return( joinAdjacents );
}

//===========================================================================//
// Method         : JoinAdjacentsBySide_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::JoinAdjacentsBySide_(
            TC_SideMode_t     side,
      const TGS_Region_c&     region,
            unsigned int      count,
      const T*                pdata,
            TTP_TileMode_t    tileMode,
            TGS_RegionList_t* pregionList ) const
{
   bool joinAdjacents = false;
   
   if( side == TC_SIDE_LEFT )
   {
      TGS_Point_c leftPoint( region.x1 - this->minGrid_, region.y1 );
      while( TCTF_IsLE( leftPoint.y, region.y2 ))
      {
         TTPT_Tile_c< T >* pleftTile = this->Find( leftPoint );
         if( !pleftTile )
            break;

         leftPoint.y = pleftTile->GetRegion( ).y2 + this->minGrid_;

         if(( pleftTile->GetMode( ) == tileMode ) &&
            ( pleftTile->IsEqualData( count, pdata )))
         {
            TGS_Region_c joinRegion;

            if( TCTF_IsLE( pleftTile->GetRegion( ).y1, region.y1 ) &&
                TCTF_IsGE( pleftTile->GetRegion( ).y2, region.y2 ))
            {
               joinRegion.ApplyAdjacent( pleftTile->GetRegion( ), region, this->minGrid_ );
               if( !joinRegion.IsValid( ))
                  continue;

               if( this->JoinAdjacentsBySide_( TC_SIDE_LEFT, joinRegion,
                                               count, pdata, tileMode, 
                                               pregionList ))
               {
                  joinAdjacents = true;
                  continue;
               }
            }
            else
            {
               joinRegion.ApplyAdjacent( pleftTile->GetRegion( ), region, this->minGrid_ );
               if( !joinRegion.IsValid( ))
                  continue;

               if( !this->JoinAdjacentsBySide_( TC_SIDE_LEFT, joinRegion,
                                                count, pdata, tileMode, 
                                                pregionList ))
               {
                  pregionList->Add( joinRegion );
               }
               joinRegion = region;
            }

            if( !pregionList->IsMember( joinRegion ))
            {
               pregionList->Add( joinRegion );
            }
            joinAdjacents = true;
         }
      }
   }  

   if( side == TC_SIDE_RIGHT )
   {
      TGS_Point_c rightPoint( region.x2 + this->minGrid_, region.y2 );
      while( TCTF_IsGE( rightPoint.y, region.y1 ))
      {
         TTPT_Tile_c< T >* prightTile = this->Find( rightPoint );
         if( !prightTile )
            break;

         rightPoint.y = prightTile->GetRegion( ).y1 - this->minGrid_;

         if(( prightTile->GetMode( ) == tileMode ) &&
            ( prightTile->IsEqualData( count, pdata )))
         {
            TGS_Region_c joinRegion;

            if( TCTF_IsLE( prightTile->GetRegion( ).y1, region.y1 ) &&
                TCTF_IsGE( prightTile->GetRegion( ).y2, region.y2 ))
            {
               joinRegion.ApplyAdjacent( prightTile->GetRegion( ), region, this->minGrid_ );
               if( !joinRegion.IsValid( ))
                  continue;

               if( this->JoinAdjacentsBySide_( TC_SIDE_RIGHT, joinRegion,
                                               count, pdata, tileMode, 
                                               pregionList ))
               {
                  joinAdjacents = true;
                  continue;
               }
            }
            else
            {
               joinRegion.ApplyAdjacent( prightTile->GetRegion( ), region, this->minGrid_ );
               if( !joinRegion.IsValid( ))
                  continue;

               if( !this->JoinAdjacentsBySide_( TC_SIDE_RIGHT, joinRegion,
                                                count, pdata, tileMode, 
                                                pregionList ))
               {
                  pregionList->Add( joinRegion );
               }
               joinRegion = region;
            }

            if( !pregionList->IsMember( joinRegion ))
            {
               pregionList->Add( joinRegion );
            }
            joinAdjacents = true;
         }
      }
   }  

   if( side == TC_SIDE_LOWER )
   {
      TGS_Point_c lowerPoint( region.x1, region.y1 - this->minGrid_ );
      while( TCTF_IsLE( lowerPoint.x, region.x2 ))
      {
         TTPT_Tile_c< T >* plowerTile = this->Find( lowerPoint );
         if( !plowerTile )
            break;

         lowerPoint.x = plowerTile->GetRegion( ).x2 + this->minGrid_;

         if(( plowerTile->GetMode( ) == tileMode ) &&
            ( plowerTile->IsEqualData( count, pdata )))
         {
            TGS_Region_c joinRegion;

            if( TCTF_IsLE( plowerTile->GetRegion( ).x1, region.x1 ) &&
                TCTF_IsGE( plowerTile->GetRegion( ).x2, region.x2 ))
            {
               joinRegion.ApplyAdjacent( plowerTile->GetRegion( ), region, this->minGrid_ );
               if( !joinRegion.IsValid( ))
                  continue;

               if( this->JoinAdjacentsBySide_( TC_SIDE_LOWER, joinRegion,
                                               count, pdata, tileMode, 
                                               pregionList ))
               {
                  joinAdjacents = true;
                  continue;
               }
            }
            else
            {
               joinRegion.ApplyAdjacent( plowerTile->GetRegion( ), region, this->minGrid_ );
               if( !joinRegion.IsValid( ))
                  continue;

               if( !this->JoinAdjacentsBySide_( TC_SIDE_LOWER, joinRegion,
                                                count, pdata, tileMode, 
                                                pregionList ))
               {
                  pregionList->Add( joinRegion );
               }
               joinRegion = region;
            }

            if( !pregionList->IsMember( joinRegion ))
            {
               pregionList->Add( joinRegion );
            }
            joinAdjacents = true;
         }
      }
   }  

   if( side == TC_SIDE_UPPER )
   {
      TGS_Point_c upperPoint( region.x2, region.y2 + this->minGrid_ );
      while( TCTF_IsGE( upperPoint.x, region.x1 ))
      {
         TTPT_Tile_c< T >* pupperTile = this->Find( upperPoint );
         if( !pupperTile )
            break;

         upperPoint.x = pupperTile->GetRegion( ).x1 - this->minGrid_;

         if(( pupperTile->GetMode( ) == tileMode ) &&
            ( pupperTile->IsEqualData( count, pdata )))
         {
            TGS_Region_c joinRegion;

            if( TCTF_IsLE( pupperTile->GetRegion( ).x1, region.x1 ) &&
                TCTF_IsGE( pupperTile->GetRegion( ).x2, region.x2 ))
            {
               joinRegion.ApplyAdjacent( pupperTile->GetRegion( ), region, this->minGrid_ );
               if( !joinRegion.IsValid( ))
                  continue;

               if( this->JoinAdjacentsBySide_( TC_SIDE_UPPER, joinRegion,
                                               count, pdata, tileMode, 
                                               pregionList ))
               {
                  joinAdjacents = true;
                  continue;
               }
            }
            else
            {
               joinRegion.ApplyAdjacent( pupperTile->GetRegion( ), region, this->minGrid_ );
               if( !joinRegion.IsValid( ))
                  continue;

               if( !this->JoinAdjacentsBySide_( TC_SIDE_UPPER, joinRegion,
                                                count, pdata, tileMode, 
                                                pregionList ))
               {
                  pregionList->Add( joinRegion );
               }
               joinRegion = region;
            }

            if( !pregionList->IsMember( joinRegion ))
            {
               pregionList->Add( joinRegion );
            }
            joinAdjacents = true;
         }
      }
   }  
   return( joinAdjacents );
}

//===========================================================================//
// Method         : CornerRegionCoords_
// Purpose        : Updates and returns a pair of region coordinates based on
//                  the given two vertically aligned, but partially mergable 
//                  regions.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::CornerRegionCoords_(
      const TGS_Region_c&    regionA,
      const TGS_Region_c&    regionB,
            TGS_Region_c*    pregionA,
            TGS_Region_c*    pregionB,
            TGS_OrientMode_t addOrient )
{
   if( pregionA && pregionB )
   {
      pregionA->Reset( );
      pregionB->Reset( );

      TC_SideMode_t side = TC_SIDE_UNDEFINED;

      if(( addOrient == TGS_ORIENT_VERTICAL ) &&
         ( this->IsAdjacent( regionA, regionB, &side )))
      {
         if(( side == TC_SIDE_UPPER ) ||
            ( side == TC_SIDE_LOWER ))
         {
            if( TCTF_IsEQ( regionA.x1, regionB.x1 ) &&
                TCTF_IsEQ( regionA.x2, regionB.x2 ))
            {
               *pregionA = regionA;
               *pregionB = regionB;

               pregionA->y1 = TCT_Min( regionA.y1, regionB.y1 );
               pregionA->y2 = TCT_Max( regionA.y2, regionB.y2 );

               pregionB->y1 = TCT_Min( regionA.y1, regionB.y1 );
               pregionB->y2 = TCT_Max( regionA.y2, regionB.y2 );
            }
            else if( TCTF_IsEQ( regionA.x1, regionB.x1 ))
            {
               *pregionA = regionA;
               *pregionB = regionB;

               if( TCTF_IsGT( regionA.GetDx( ), regionB.GetDx( )))
               {
                  pregionA->x1 = regionB.x2 + this->minGrid_;
                  pregionB->y1 = TCT_Min( regionA.y1, regionB.y1 );
                  pregionB->y2 = TCT_Max( regionA.y2, regionB.y2 );
               }
               else if( TCTF_IsLT( regionA.GetDx( ), regionB.GetDx( )))
               {
                  pregionB->x1 = regionA.x2 + this->minGrid_;
                  pregionA->y1 = TCT_Min( regionA.y1, regionB.y1 );
                  pregionA->y2 = TCT_Max( regionA.y2, regionB.y2 );
               }
            }
            else if( TCTF_IsEQ( regionA.x2, regionB.x2 ))
            {
               *pregionA = regionA;
               *pregionB = regionB;

               if( TCTF_IsGT( regionA.GetDx( ), regionB.GetDx( )))
               {
                  pregionA->x2 = regionB.x1 - this->minGrid_;
                  pregionB->y1 = TCT_Min( regionA.y1, regionB.y1 );
                  pregionB->y2 = TCT_Max( regionA.y2, regionB.y2 );
               }
               else if( TCTF_IsLT( regionA.GetDx( ), regionB.GetDx( )))
               {
                  pregionB->x2 = regionA.x1 - this->minGrid_;
                  pregionA->y1 = TCT_Min( regionA.y1, regionB.y1 );
                  pregionA->y2 = TCT_Max( regionA.y2, regionB.y2 );
               }
            }
         }
      }
      if(( addOrient == TGS_ORIENT_HORIZONTAL ) &&
         ( this->IsAdjacent( regionA, regionB, &side )))
      {
         if(( side == TC_SIDE_LEFT ) ||
            ( side == TC_SIDE_RIGHT ))
         {
            if( TCTF_IsEQ( regionA.y1, regionB.y1 ) &&
                TCTF_IsEQ( regionA.y2, regionB.y2 ))
            {
               *pregionA = regionA;
               *pregionB = regionB;

               pregionA->x1 = TCT_Min( regionA.x1, regionB.x1 );
               pregionA->x2 = TCT_Max( regionA.x2, regionB.x2 );

               pregionB->x1 = TCT_Min( regionA.x1, regionB.x1 );
               pregionB->x2 = TCT_Max( regionA.x2, regionB.x2 );
            }
            else if( TCTF_IsEQ( regionA.y1, regionB.y1 ))
            {
               *pregionA = regionA;
               *pregionB = regionB;

               if( TCTF_IsGT( regionA.GetDy( ), regionB.GetDy( )))
               {
                  pregionA->y1 = regionB.y2 + this->minGrid_;
                  pregionB->x1 = TCT_Min( regionA.x1, regionB.x1 );
                  pregionB->x2 = TCT_Max( regionA.x2, regionB.x2 );
               }
               else if( TCTF_IsLT( regionA.GetDy( ), regionB.GetDy( )))
               {
                  pregionB->y1 = regionA.y2 + this->minGrid_;
                  pregionA->x1 = TCT_Min( regionA.x1, regionB.x1 );
                  pregionA->x2 = TCT_Max( regionA.x2, regionB.x2 );
               }
            }
            else if( TCTF_IsEQ( regionA.y2, regionB.y2 ))
            {
               *pregionA = regionA;
               *pregionB = regionB;

               if( TCTF_IsGT( regionA.GetDy( ), regionB.GetDy( )))
               {
                  pregionA->y2 = regionB.y1 - this->minGrid_;
                  pregionB->x1 = TCT_Min( regionA.x1, regionB.x1 );
                  pregionB->x2 = TCT_Max( regionA.x2, regionB.x2 );
               }
               else if( TCTF_IsLT( regionA.GetDy( ), regionB.GetDy( )))
               {
                  pregionB->y2 = regionA.y1 - this->minGrid_;
                  pregionA->x1 = TCT_Min( regionA.x1, regionB.x1 );
                  pregionA->x2 = TCT_Max( regionA.x2, regionB.x2 );
               }
            }
         }
      }
   }
   return(( pregionA && pregionB ) && 
          ( pregionA->IsValid( ) && pregionB->IsValid( )) ?
           true : false );
}

//===========================================================================//
// Method         : CornerTileRemerge_
// Purpose        : Re-merges the given two vertically aligned, but partially
//                  mergable solid tiles.  This re-merge involves deleting 
//                  the existing tiles, then adding the re-merged solid 
//                  tiles.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::CornerTileRemerge_(
            TTPT_Tile_c< T >* ptileA,
            TTPT_Tile_c< T >* ptileB,
      const TGS_Region_c&     regionA,
      const TGS_Region_c&     regionB,
            TGS_OrientMode_t  addOrient )
{
   bool ok = true;
   
   TTPT_Tile_c< T > tileA( *ptileA );
   TTPT_Tile_c< T > tileB( *ptileB );
         
   if( addOrient == TGS_ORIENT_VERTICAL )
   {
      if( TCTF_IsEQ( regionA.GetDy( ), regionB.GetDy( )))
      {
         tileA.SetRegion( regionA );
         tileB.SetMode( TTP_TILE_UNDEFINED );
      }
      else if( TCTF_IsGT( regionA.GetDy( ), regionB.GetDy( )))
      {
         tileA.SetRegion( regionA );
         tileB.SetRegion( regionB );
      }
      else // if( TCTF_IsLT( regionA.GetDy( ), regionB.GetDy( )))
      {
         tileA.SetRegion( regionB );
         tileB.SetRegion( regionA );
      }
   }
   if( addOrient == TGS_ORIENT_HORIZONTAL )
   {
      if( TCTF_IsEQ( regionA.GetDx( ), regionB.GetDx( )))
      {
         tileA.SetRegion( regionA );
         tileB.SetMode( TTP_TILE_UNDEFINED );
      }
      else if( TCTF_IsGT( regionA.GetDx( ), regionB.GetDx( )))
      {
         tileA.SetRegion( regionA );
         tileB.SetRegion( regionB );
      }
      else // if( TCTF_IsLT( regionA.GetDx( ), regionB.GetDx( )))
      {
         tileA.SetRegion( regionB );
         tileB.SetRegion( regionA );
      }
   }

   if( ok && ptileA->IsValid( ))
   {
      ok = this->Delete( ptileA );
   }
   if( ok && ptileB->IsValid( ))
   {
      ok = this->Delete( ptileB );
   }
   if( ok && tileA.IsValid( ))
   {      
      ok = this->AddPerNew_( tileA, addOrient );
   }
   if( ok && tileB.IsValid( ))
   {
      ok = this->AddPerNew_( tileB, addOrient );
   }
   return( ok );
}

//===========================================================================//
// Method         : StitchToSplit_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::StitchToSplit_( 
      TTPT_Tile_c< T >* ptileA,
      TTPT_Tile_c< T >* ptileB,
      TGS_OrientMode_t  orientMode )
{
   const TGS_Region_c& regionA = ptileA->GetRegion( );
   const TGS_Region_c& regionB = ptileB->GetRegion( );

   if( orientMode == TGS_ORIENT_HORIZONTAL )
   {
      // Update tile (T) and (T')'s 'right_upper' & 'upper_right' stitches
      ptileA->SetStitchRightUpper( ptileB );
      ptileB->SetStitchLeftLower( ptileA );

      // Update tile (T')'s 'lower_left' stitch based on neighboring tile
      TGS_Point_c pointLowerLeft( regionB.x1 - this->minGrid_, regionB.y1 );
      TTPT_Tile_c< T >* ptileLowerLeft = this->Find( pointLowerLeft );
      ptileB->SetStitchLowerLeft( ptileLowerLeft );

      // Update tile (T)'s 'upper_right' stitch based on neighboring tile
      TGS_Point_c pointUpperRight( regionA.x2 + this->minGrid_, regionA.y2 );
      TTPT_Tile_c< T >* ptileUpperRight = this->Find( pointUpperRight );
      ptileA->SetStitchUpperRight( ptileUpperRight );
   }
   else // if( orientMode == TGS_ORIENT_VERTICAL )
   {
      // Update tile (T) and (T')'s 'upper_right' & 'lower_left' stitches
      ptileA->SetStitchUpperRight( ptileB );
      ptileB->SetStitchLowerLeft( ptileA );

      // Update tile (T')'s 'left_lower' stitch based on neighboring tile
      TGS_Point_c pointLeftLower( regionB.x1, regionB.y1 - this->minGrid_ );
      TTPT_Tile_c< T >* ptileLeftLower = this->Find( pointLeftLower );
      ptileB->SetStitchLeftLower( ptileLeftLower );

      // Update tile (T)'s 'right_upper' stitch based on neighboring tile
      TGS_Point_c pointRightUpper( regionA.x2, regionA.y2 + this->minGrid_ );
      TTPT_Tile_c< T >* ptileRightUpper = this->Find( pointRightUpper );
      ptileA->SetStitchRightUpper( ptileRightUpper );
   }
}

//===========================================================================//
// Method         : StitchToMerge_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::StitchToMerge_( 
      TTPT_Tile_c< T >* ptileA,
      TTPT_Tile_c< T >* ptileB,
      TGS_OrientMode_t  orientMode,
      TGS_CornerMode_t  cornerMode )
{
   if( orientMode == TGS_ORIENT_HORIZONTAL )
   {
      if( cornerMode == TGS_CORNER_LOWER_LEFT )
      {
         // Update tile (T) per tile (T') 'left_lower' & 'lower_left' stitches
         ptileA->SetStitchLeftLower( ptileB->GetStitchLeftLower( ));
         ptileA->SetStitchLowerLeft( ptileB->GetStitchLowerLeft( ));
      }
      if( cornerMode == TGS_CORNER_UPPER_RIGHT )
      {
         // Update tile (T) per tile (T') 'right_upper' & 'upper_right' stitches
         ptileA->SetStitchRightUpper( ptileB->GetStitchRightUpper( ));
         ptileA->SetStitchUpperRight( ptileB->GetStitchUpperRight( ));
      }
   }
   else // if( orientMode == TGS_ORIENT_VERTICAL )
   {
      if( cornerMode == TGS_CORNER_LOWER_LEFT )
      {
         // Update tile (T) per tile (T') 'left_lower' & 'lower_left' stitches
         ptileA->SetStitchLeftLower( ptileB->GetStitchLeftLower( ));
         ptileA->SetStitchLowerLeft( ptileB->GetStitchLowerLeft( ));
      }
      if( cornerMode == TGS_CORNER_UPPER_RIGHT )
      {
         // Update tile (T) per tile (T') 'right_upper' & 'upper_right' stitches
         ptileA->SetStitchRightUpper( ptileB->GetStitchRightUpper( ));
         ptileA->SetStitchUpperRight( ptileB->GetStitchUpperRight( ));
      }
   }
}

//===========================================================================//
// Method         : StitchFromSide_
// Purpose        : Updates the given tile's neighboring tile stitch pointers 
//                  based on the given tile side.  This may require iterating 
//                  over multiple neighboring tiles in order to completely 
//                  span the given tile side.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::StitchFromSide_( 
      TTPT_Tile_c< T >* ptile,
      TC_SideMode_t     side,
      TTPT_Tile_c< T >* ptileP )
{
   if( side != TC_SIDE_UNDEFINED )
   {
      const TGS_Region_c& tileRegion = ptile->GetRegion( );

      bool iterateSide = false;
      
      if( side == TC_SIDE_LEFT )
      {
         TTPT_Tile_c< T >* pleftTile = ptile->GetStitchLowerLeft( );
	 if( pleftTile )
	 {
            const TGS_Region_c& leftRegion = pleftTile->GetRegion( );
            if( TCTF_IsGT( leftRegion.y2, tileRegion.y2 ))
            {
               ;                                   // No need to iterate over side
            }
            else if( TCTF_IsEQ( leftRegion.y2, tileRegion.y2 ))
            {
               pleftTile->SetStitchUpperRight( ptile );   // No need to iterate over side
            }
            else // if( TCTF_IsLT( leftRegion.y2, tileRegion.y2 ))
            {
    	       iterateSide = true;                 // Need to iterate over side
            }
         }
      }
      if( side == TC_SIDE_RIGHT )
      {
         TTPT_Tile_c< T >* prightTile = ptile->GetStitchUpperRight( );
	 if( prightTile )
	 {
            const TGS_Region_c& rightRegion = prightTile->GetRegion( );
            if( TCTF_IsLT( rightRegion.y1, tileRegion.y1 ))
            {
               ;                                   // No need to iterate over side
            }
            else if( TCTF_IsEQ( rightRegion.y1, tileRegion.y1 ))
            {
               prightTile->SetStitchLowerLeft( ptile );  // No need to iterate over side
            }
            else // if( TCTF_IsGT( rightRegion.y1, tileRegion.y1 ))
            {
    	       iterateSide = true;                 // Need to iterate over side
            }
         }
      }
      if( side == TC_SIDE_LOWER )
      {
         TTPT_Tile_c< T >* plowerTile = ptile->GetStitchLeftLower( );
	 if( plowerTile )
	 {
            const TGS_Region_c& lowerRegion = plowerTile->GetRegion( );
            if( TCTF_IsGT( lowerRegion.x2, tileRegion.x2 ))
            {
               ;                                   // No need to iterate over side
            }
            else if( TCTF_IsEQ( lowerRegion.x2, tileRegion.x2 ))
            {
               plowerTile->SetStitchRightUpper( ptile ); // No need to iterate over side
            }
            else // if( TCTF_IsLT( lowerRegion.x2, tileRegion.x2 ))
            {
    	       iterateSide = true;                 // Need to iterate over side
            }
         }
      }
      if( side == TC_SIDE_UPPER )
      {
         TTPT_Tile_c< T >* pupperTile = ptile->GetStitchRightUpper( );
	 if( pupperTile )
	 {
            const TGS_Region_c& upperRegion = pupperTile->GetRegion( );
            if( TCTF_IsLT( upperRegion.x1, tileRegion.x1 ))
            {
               ;                                   // No need to iterate over side
            }
            else if( TCTF_IsEQ( upperRegion.x1, tileRegion.x1 ))
            {
               pupperTile->SetStitchLeftLower( ptile );    // No need to iterate over side
            }
            else // if( TCTF_IsGT( upperRegion.x1, tileRegion.x1 ))
            {
    	       iterateSide = true;                 // Need to iterate over side
            }
         }
      }

      if( iterateSide )
      {      
         TTPT_Tile_c< T >* piterTile = ( ptileP ? ptileP : ptile );
         this->tilePlaneIter_.Init( *this, *piterTile, side );
         TTPT_Tile_c< T >* pneighbor = 0;
         while( this->tilePlaneIter_.Next( &pneighbor ))
         {
            const TGS_Region_c& neighborRegion = pneighbor->GetRegion( );
   
            if( side == TC_SIDE_LEFT )
   	    {
               if( TCTF_IsLE( neighborRegion.y2, tileRegion.y2 ))
               {
                  pneighbor->SetStitchUpperRight( ptile );
               }
            }
            if( side == TC_SIDE_RIGHT )
   	    {
               if( TCTF_IsGE( neighborRegion.y1, tileRegion.y1 ))
               {
                  pneighbor->SetStitchLowerLeft( ptile );
               }
            }
            if( side == TC_SIDE_LOWER )
   	    {
               if( TCTF_IsLE( neighborRegion.x2, tileRegion.x2 ))
               {
                  pneighbor->SetStitchRightUpper( ptile );
               }
            }
            if( side == TC_SIDE_UPPER )
   	    {
               if( TCTF_IsGE( neighborRegion.x1, tileRegion.x1 ))
               {
                  pneighbor->SetStitchLeftLower( ptile );
               }
            }
         }
      }
   }
}

//===========================================================================//
// Method         : LoadTileList_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::LoadTileList_(
      TTPT_Tile_c< T >*                         ptile,
      TCT_OrderedVector_c< TTPT_Tile_c< T >* >* ptileList )
{
   if( ptile && 
       ptile->IsValid( ))
   {   
      ptile->SetMode( TTP_TILE_UNDEFINED );

      if( ptile->GetStitchLowerLeft( ))
      {
         TTPT_Tile_c< T >* ptileLowerLeft = ptile->GetStitchLowerLeft( );
         ptile->SetStitchLowerLeft( 0 );
   
         if( ptileLowerLeft->GetStitchUpperRight( ) == ptile )
         {
            ptileLowerLeft->SetStitchUpperRight( 0 );
         }
         this->LoadTileList_( ptileLowerLeft, ptileList );
      }
      if( ptile->GetStitchUpperRight( ))
      {
         TTPT_Tile_c< T >* ptileUpperRight = ptile->GetStitchUpperRight( );
         ptile->SetStitchUpperRight( 0 );
   
         if( ptileUpperRight->GetStitchLowerLeft( ) == ptile )
         {
            ptileUpperRight->SetStitchLowerLeft( 0 );
         }
	 this->LoadTileList_( ptileUpperRight, ptileList );
      }
      if( ptile->GetStitchLeftLower( ))
      {
         TTPT_Tile_c< T >* ptileLeftLower = ptile->GetStitchLeftLower( );
         ptile->SetStitchLeftLower( 0 );
   
         if( ptileLeftLower->GetStitchRightUpper( ) == ptile )
         {
            ptileLeftLower->SetStitchRightUpper( 0 );
         }
	 this->LoadTileList_( ptileLeftLower, ptileList );
      }
      if( ptile->GetStitchRightUpper( ))
      {
         TTPT_Tile_c< T >* ptileRightUpper = ptile->GetStitchRightUpper( );
         ptile->SetStitchRightUpper( 0 );
   
         if( ptileRightUpper->GetStitchLeftLower( ) == ptile )
         {
            ptileRightUpper->SetStitchLeftLower( 0 );
         }
	 this->LoadTileList_( ptileRightUpper, ptileList );
      }
      ptileList->Add( ptile );
   }
}

//===========================================================================//
// Method         : EstAspectRatio_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::EstAspectRatio_(
      const TGS_Region_c& newRegion,
      const TGS_Region_c& curRegion,
            double*       pminAspectRatio,
            double*       pmaxAspectRatio ) const
{
   TGS_Region_c maxRegion;
   if(( TCTF_IsLT( newRegion.x1, curRegion.x1 ) && 
        TCTF_IsGT( newRegion.x2, curRegion.x2 )) ||
      ( TCTF_IsLT( newRegion.y1, curRegion.y1 ) &&
        TCTF_IsGT( newRegion.y2, curRegion.y2 )))
   {
      maxRegion.Set( newRegion );
   }
   else if( TCTF_IsGE( newRegion.x1, curRegion.x1 ) && 
            TCTF_IsLE( newRegion.x2, curRegion.x2 ))
   {
      maxRegion.Set( newRegion.x1, TCT_Min( newRegion.y1, curRegion.y1 ),
                     newRegion.x2, TCT_Max( newRegion.y2, curRegion.y2 ));
   }
   else if( TCTF_IsGE( newRegion.y1, curRegion.y1 ) && 
            TCTF_IsLE( newRegion.y2, curRegion.y2 ))
   {
      maxRegion.Set( TCT_Min( newRegion.x1, curRegion.x1 ), newRegion.y1,
                     TCT_Max( newRegion.x2, curRegion.x2 ), newRegion.y2 );
   }
   else
   {
      maxRegion.Set( newRegion );
   }

   TGS_Region_c minRegionLL;
   if( TCTF_IsGE( curRegion.x1, maxRegion.x1 ) &&
       TCTF_IsGE( curRegion.y1, maxRegion.y1 ))
   {
      minRegionLL.Reset( );
   }
   else if( TCTF_IsLT( curRegion.x1, maxRegion.x1 ))
   {
      minRegionLL.Set( curRegion.x1, curRegion.y1, maxRegion.x1, curRegion.y2 );
   }
   else if( TCTF_IsLT( curRegion.y1, maxRegion.y1 ))
   {
      minRegionLL.Set( curRegion.x1, curRegion.y1, curRegion.x2, maxRegion.y1 );
   }
   else
   {
      minRegionLL.Reset( );
   }

   TGS_Region_c minRegionUR;
   if( TCTF_IsLE( curRegion.x2, maxRegion.x2 ) &&
       TCTF_IsLE( curRegion.y2, maxRegion.y2 ))
   {
      minRegionUR.Reset( );
   }
   else if( TCTF_IsGT( curRegion.x2, maxRegion.x2 ))
   {
      minRegionUR.Set( maxRegion.x2, curRegion.y1, curRegion.x2, curRegion.y2 );
   }
   else if( TCTF_IsGT( curRegion.y2, maxRegion.y2 ))
   {
      minRegionUR.Set( curRegion.x1, maxRegion.y2, curRegion.x2, curRegion.y2 );
   }
   else
   {
      minRegionUR.Reset( );
   }

   double maxRatio = ( maxRegion.HasArea( ) ?
                       maxRegion.FindMax( ) / maxRegion.FindMin( ) : 0.0 );
   double minRatioLL = ( minRegionLL.HasArea( ) ?
                         minRegionLL.FindMax( ) / minRegionLL.FindMin( ) : 0.0 );
   double minRatioUR = ( minRegionUR.HasArea( ) ?
                         minRegionUR.FindMax( ) / minRegionUR.FindMin( ) : 0.0 );

   if( pminAspectRatio )
   {
     *pminAspectRatio = TCT_Max( maxRatio, TCT_Min( minRatioLL, minRatioUR ));
   }
   if( pmaxAspectRatio )
   {
      *pmaxAspectRatio = TCT_Max( maxRatio, minRatioLL, minRatioUR );
   }
}

//===========================================================================//
// Method         : HasAspectRatio_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::HasAspectRatio_(
      const TGS_Region_c& newRegion,
      const TGS_Region_c& curRegion ) const
{
   double newRatioMin, newRatioMax;
   this->EstAspectRatio_( newRegion, curRegion, &newRatioMin, &newRatioMax );

   double curRatioMin, curRatioMax;
   this->EstAspectRatio_( curRegion, newRegion, &curRatioMin, &curRatioMax );

   bool hasAspectRatio = ( TCTF_IsGE( newRatioMax, curRatioMax ) ? true : false );
   if( !hasAspectRatio )
   {
      double regionMax = this->region_.FindMax( );
      double estRatioMin = ( TCTF_IsLT( regionMax, static_cast< double >( INT_MAX )) ?
                             regionMax / 100.0 : 1000.0 );
      if( TCTF_IsLT( newRatioMin, estRatioMin ))
      {
         hasAspectRatio = true;   
      }
   }
   return( hasAspectRatio );
}

//===========================================================================//
// Method         : IsClearMatch_
// Purpose        : Return true if the given point or exact region is clear.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsClearMatch_(
      const TGS_Point_c& point ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* pfind = this->Find( point );
   if( pfind && pfind->IsClear( ))
   {
      ptile = pfind;
   }
   return( ptile ? true : false );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsClearMatch_(
      const TGS_Region_c&      region,
            TTPT_Tile_c< T >** ppclearTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* pfind = this->Find( region, 
                                         TTP_FIND_EXACT, TTP_TILE_CLEAR );
   if( pfind && pfind->IsClear( ))
   {
      ptile = pfind;
   }
   
   if( ppclearTile )
   {
      *ppclearTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
// Method         : IsClearAny_
// Purpose        : Return true if any clear tiles are found within the given
//                  tile plane search region.  Optionally, returns a pointer 
//                  to the first clear tile found.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsClearAny_(
      const TGS_Region_c&      region,
            TTPT_Tile_c< T >** ppclearTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* ptileLL = this->Find( region.x1, region.y1 );
   if( ptileLL &&
       ptileLL->IsClear( ))
   {
      ptile = ptileLL;
   }
   else if( ptileLL &&
            ptileLL->IsSolid( ) &&
            ptileLL->GetRegion( ).IsWithin( region ))
   {
      ptile = 0;
   }
   else
   {
      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this, region );
      TTPT_Tile_c< T >* pnext = 0;
      while( ptilePlane->tilePlaneIter_.Next( &pnext ))
      {
         if( pnext->IsClear( ))
         {
            ptile = pnext;
            break;
         }
      }
   }

   if( ppclearTile )
   {
      *ppclearTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
// Method         : IsClearAll_
// Purpose        : Return true if only clear tiles are found within the 
//                  given tile plane search region.  Optionally, returns a 
//                  pointer to the last clear tile found.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsClearAll_( 
      const TGS_Region_c&      region, 
            TTPT_Tile_c< T >** ppclearTile ) const 
{ 
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* ptileLL = this->Find( region.x1, region.y1 );
   if( ptileLL &&
       ptileLL->IsClear( ) &&
       ptileLL->GetRegion( ).IsWithin( region ))
   {
      ptile = ptileLL;
   }
   else if( ptileLL &&
            ptileLL->IsClear( ) &&
            !this->IsSolidAny_( region ))
   {
      ptile = ptileLL;
   }
   else if( ptileLL &&
            ptileLL->IsSolid( ))
   {
      ptile = 0;
   }
   else
   {
      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this, region ); 
      TTPT_Tile_c< T >* pnext = 0; 
      while( ptilePlane->tilePlaneIter_.Next( &pnext )) 
      { 
         if( pnext->IsClear( )) 
         {
            ptile = pnext; 
         }  
         else 
         { 
            ptile = 0;
            break;
         }
      }
   }

   if( ppclearTile )
   {
      *ppclearTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
// Method         : IsSolidMatch_
// Purpose        : Return true if the given point or exact region is solid
//                  and matches the given optional data.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidMatch_(
      const TGS_Point_c& point ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* pfind = this->Find( point );
   if( pfind && pfind->IsSolid( ))
   {
      ptile = pfind;
   }
   return( ptile ? true : false );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidMatch_(
      const TGS_Point_c& point,
            unsigned int count,
      const T*           pdata ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* pfind = this->Find( point );
   if( pfind && pfind->IsSolid( ) && pfind->IsEqualData( count, pdata ))
   {
      ptile = pfind;
   }
   return( ptile ? true : false );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidMatch_(
      const TGS_Region_c&      region,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* pfind = this->Find( region, 
                                         TTP_FIND_EXACT, TTP_TILE_SOLID );
   if( pfind && pfind->IsSolid( ))
   {
      ptile = pfind;
   }
   
   if( ppsolidTile )
   {
      *ppsolidTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidMatch_(
      const TGS_Region_c&      region,
            unsigned int       count,
      const T*                 pdata,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* pfind = this->Find( region, 
                                         TTP_FIND_EXACT, TTP_TILE_SOLID );
   if( pfind && pfind->IsSolid( ) && pfind->IsEqualData( count, pdata ))
   {
      ptile = pfind;
   }
   
   if( ppsolidTile )
   {
      *ppsolidTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
// Method         : IsSolidAny_
// Purpose        : Return true if any solid tiles are found within the given
//                  tile plane search region and matches the optional given 
//                  data.  Optionally, returns a pointer to the first solid
//                  tile found.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidAny_(
      const TGS_Region_c&      region,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;

   // Apply validate to force search region within this tile plane's region
   TGS_Region_c iterRegion( region );
   iterRegion.ApplyIntersect( this->region_ );
   if( iterRegion.IsValid( ))
   {
      // Start with first tile found in upper-left corner of iteration region
      ptile = this->Find( iterRegion.x1, iterRegion.y2 );

      // Iterate until searched entire region (upper to lower) for a solid tile
      while( ptile && !ptile->IsSolid( ))
      {
         // Test for neighboring solid tile when search tile is within region
         // (Clear tiles cover left to right due to tile plane's strip property)
         const TGS_Region_c& clearRegion = ptile->GetRegion( );
         if( TCTF_IsLT( clearRegion.x2, iterRegion.x2 ))
         {
  	    ptile = this->Find( clearRegion.x2 + this->minGrid_, iterRegion.y2 );
            break;
         }

         // Iterate to next tile found within iteration search region
         iterRegion.y2 = clearRegion.y1 - this->minGrid_;
         if( TCTF_IsGE( iterRegion.y2, iterRegion.y1 ))
         {
	    // Continue with next file found in upper-left corner of region
            ptile = this->Find( iterRegion.x1, iterRegion.y2 );
         }
         else
         {
            ptile = 0;
         }
      }
   }

   if( ppsolidTile )
   {
      *ppsolidTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}


//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidAny_(
      const TGS_Region_c&      region,
            unsigned int       count,
      const T*                 pdata,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* ptileLL = this->Find( region.x1, region.y1 );
   if( ptileLL &&
       ptileLL->IsSolid( ) &&
       ptileLL->IsEqualData( count, pdata ))
   {
      ptile = ptileLL;
   }
   else if( ptileLL &&
            ptileLL->IsClear( ) &&
            ptileLL->GetRegion( ).IsWithin( region ))
   {
      ptile = 0;
   }
   else
   {
      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this, region );
      TTPT_Tile_c< T >* pnext = 0;
      while( ptilePlane->tilePlaneIter_.Next( &pnext ))
      {
         if(( pnext->IsSolid( )) &&
            ( pnext->IsEqualData( count, pdata )))
         {
            ptile = pnext;
            break;
         }
      }
   }
      
   if( ppsolidTile )
   {
      *ppsolidTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}


//===========================================================================//
// Method         : IsSolidAll_
// Purpose        : Return true if only solid tiles are found within the 
//                  given tile plane search region and matches the optional 
//                  given data.  Optionally, returns a pointer to the last 
//                  solid tile found.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidAll_(
      const TGS_Region_c&      region,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* ptileLL = this->Find( region.x1, region.y1 );
   if( ptileLL &&
       ptileLL->IsSolid( ) &&
       ptileLL->GetRegion( ).IsWithin( region ))
   {
      ptile = ptileLL;
   }
   else if( ptileLL &&
            ptileLL->IsClear( ))
   {
      ptile = 0;
   }
   else
   {
      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this, region );
      TTPT_Tile_c< T >* pnext = 0;
      while( ptilePlane->tilePlaneIter_.Next( &pnext ))
      {
         if( pnext->IsSolid( ))
         {
            ptile = pnext;
         }
         else
         { 
            ptile = 0;
            break;
         }
      }
   }

   if( ppsolidTile )
   {
      *ppsolidTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}


//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidAll_(
      const TGS_Region_c&      region,
            unsigned int       count,
      const T*                 pdata,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* ptileLL = this->Find( region.x1, region.y1 );
   if( ptileLL &&
       ptileLL->IsSolid( ) &&
       ptileLL->IsEqualData( count, pdata ) &&
       ptileLL->GetRegion( ).IsWithin( region ))
   {
      ptile = ptileLL;
   }
   else if( ptileLL &&
            ptileLL->IsClear( ))
   {
      ptile = 0;
   }
   else
   {
      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this, region );
      TTPT_Tile_c< T >* pnext = 0;
      while( ptilePlane->tilePlaneIter_.Next( &pnext ))
      {
         if(( pnext->IsSolid( )) &&
            ( pnext->IsEqualData( count, pdata )))
         {
            ptile = pnext;
         }
         else
         { 
            ptile = 0;
            break;
         }
      }
   }

   if( ppsolidTile )
   {
      *ppsolidTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
// Method         : IsSolidMax_
// Purpose        : Return true if any solid tiles are found within the given
//                  tile plane search region and matches the optional given 
//                  data.  Optionally, returns a pointer to the largest solid
//                  tile found.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidMax_(
      const TGS_Region_c&      region,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* ptileLL = this->Find( region.x1, region.y1 );
   if( ptileLL &&
       ptileLL->IsSolid( ) &&
       ptileLL->GetRegion( ).IsWithin( region ))
   {
      ptile = ptileLL;
   }
   else if( ptileLL &&
            ptileLL->IsClear( ) &&
            ptileLL->GetRegion( ).IsWithin( region ))
   {
      ptile = 0;
   }
   else
   {
      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this, region );
      TTPT_Tile_c< T >* pnext = 0;
      while( ptilePlane->tilePlaneIter_.Next( &pnext ))
      {
	 if( pnext->IsSolid( ))
         {
	    if( !ptile )
	    {
               ptile = pnext;
	    }
            else if( TCTF_IsLT( ptile->FindArea( ), pnext->FindArea( )))
	    {
               ptile = pnext;
	    }
         }
      }
   }
      
   if( ppsolidTile )
   {
      *ppsolidTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidMax_(
      const TGS_Region_c&      region,
            unsigned int       count,
      const T*                 pdata,
            TTPT_Tile_c< T >** ppsolidTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* ptileLL = this->Find( region.x1, region.y1 );
   if( ptileLL &&
       ptileLL->IsSolid( ) &&
       ptileLL->GetRegion( ).IsWithin( region ) &&
       ptileLL->IsEqualData( count, pdata ))
   {
      ptile = ptileLL;
   }
   else if( ptileLL &&
            ptileLL->IsClear( ) &&
            ptileLL->GetRegion( ).IsWithin( region ))
   {
      ptile = 0;
   }
   else
   {
      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this, region );
      TTPT_Tile_c< T >* pnext = 0;
      while( ptilePlane->tilePlaneIter_.Next( &pnext ))
      {
         if(( pnext->IsSolid( )) &&
            ( pnext->IsEqualData( count, pdata )))
         {
	    if( !ptile )
	    {
               ptile = pnext;
	    }
            else if( TCTF_IsLT( ptile->FindArea( ), pnext->FindArea( )))
	    {
               ptile = pnext;
	    }
         }
      }
   }
      
   if( ppsolidTile )
   {
      *ppsolidTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
// Method         : IsSolidNotMatch_
// Purpose        : Return true if the given point or exact region is solid
//                  and does not match the given data.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNotMatch_(
      const TGS_Point_c& point,
            unsigned int count,
      const T*           pdata ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* pfind = this->Find( point );
   if( pfind && pfind->IsSolid( ) && !pfind->IsEqualData( count, pdata ))
   {
      ptile = pfind;
   }
   return( ptile ? true : false );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNotMatch_(
      const TGS_Region_c&      region,
            unsigned int       count,
      const T*                 pdata,
            TTPT_Tile_c< T >** ppsolidNotTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* pfind = this->Find( region,
                                         TTP_FIND_EXACT, TTP_TILE_SOLID );
   if( pfind && pfind->IsSolid( ) && !pfind->IsEqualData( count, pdata ))
   {
      ptile = pfind;
   }
   
   if( ppsolidNotTile )
   {
      *ppsolidNotTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
// Method         : IsSolidNotAny_
// Purpose        : Return true if any solid tiles are found within the given
//                  tile plane search region that do not match the given 
//                  data.  Optionally, returns a pointer to the first solid
//                  not tile found.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNotAny_(
      const TGS_Region_c&      region,
            unsigned int       count,
      const T*                 pdata,
            TTPT_Tile_c< T >** ppsolidNotTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* ptileLL = this->Find( region.x1, region.y1 );
   if( ptileLL &&
       ptileLL->IsSolid( ) &&
       !ptileLL->IsEqualData( count, pdata ))
   {
      ptile = ptileLL;
   }
   else if( ptileLL &&
            ptileLL->IsClear( ) &&
            ptileLL->GetRegion( ).IsWithin( region ))
   {
      ptile = 0;
   }
   else if( ptileLL &&
            ptileLL->IsClear( ) &&
            this->IsSolidNotAny_( region ))
   {
      ptile = 0;
   }
   else
   {
      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this, region );
      TTPT_Tile_c< T >* pnext = 0;
      while( ptilePlane->tilePlaneIter_.Next( &pnext ))
      {
         if(( pnext->IsSolid( )) &&
            ( !pnext->IsEqualData( count, pdata )))
         {
            ptile = pnext;
            break;
         }
      }
   }
      
   if( ppsolidNotTile )
   {
      *ppsolidNotTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNotAny_(
      const TGS_Region_c& region ) const
{
   TTPT_Tile_c< T >* ptile = this->Find( region.x1, region.y1 );

   if( ptile )
   {
      if( !ptile->IsClear( ) ||
          !ptile->GetRegion( ).IsWithinDx( region ))
      {
         ptile = 0;
      }
   }

   while( ptile )
   {
      const TGS_Region_c& tileRegion = ptile->GetRegion( );
      if( TCTF_IsGE( tileRegion.y2, region.y2 ))
         break;

      ptile = this->Find( region.x1, tileRegion.y2 + this->minGrid_ );

      if( !ptile->IsClear( ) ||
          !ptile->GetRegion( ).IsWithinDx( region ))
      {
         ptile = 0;
      }
   }
   return( ptile ? true : false );
}

//===========================================================================//
// Method         : IsSolidNotAll_
// Purpose        : Return true if only solid tiles found within the given
//                  tile plane search region that do not match the given
//                  data.  Optionally, returns a pointer to the last solid
//                  not tile found.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::IsSolidNotAll_(
      const TGS_Region_c&      region,
            unsigned int       count,
      const T*                 pdata,
            TTPT_Tile_c< T >** ppsolidNotTile ) const
{
   TTPT_Tile_c< T >* ptile = 0;
   TTPT_Tile_c< T >* ptileLL = this->Find( region.x1, region.y1 );
   if( ptileLL &&
       ptileLL->IsSolid( ) &&
       !ptileLL->IsEqualData( count, pdata ) &&
       ptileLL->GetRegion( ).IsWithin( region ))
   {
      ptile = ptileLL;
   }
   else
   {
      TTPT_TilePlane_c< T >* ptilePlane = const_cast< TTPT_TilePlane_c< T >* >( this );
      ptilePlane->tilePlaneIter_.Init( *this, region );
      TTPT_Tile_c< T >* pnext = 0;
      while( ptilePlane->tilePlaneIter_.Next( &pnext ))
      {
         if(( pnext->IsSolid( )) &&
            ( !pnext->IsEqualData( count, pdata )))
         {
            ptile = pnext;
            break;
         }
         else
         { 
            ptile = 0;
            break;
         }
      }
   }

   if( ppsolidNotTile )
   {
      *ppsolidNotTile = ( ptile ? ptile : 0 );
   }
   return( ptile ? true : false );
}

//===========================================================================//
// Method         : Allocate_
// Purpose        : Builds and returns a new tile, based on the given tile.
//                  This method allocates memory asso. with the tile and
//                  updates the global tile plane pointers, has needed.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > TTPT_Tile_c< T >* TTPT_TilePlane_c< T >::Allocate_(
      const TTPT_Tile_c< T >& tile )
{
   TTPT_Tile_c< T >* ptile = new TC_NOTHROW TTPT_Tile_c< T >( tile );

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );
   if( printHandler.IsValidNew( ptile,
                                sizeof( TTPT_Tile_c< T > ),
                                "TTPT_TilePlane_c::Allocate_" ))
   {
      const TGS_Region_c& tileRegion = ptile->GetRegion( );
      const TGS_Region_c& thisRegion = this->GetRegion( );
      if( TCTF_IsEQ( tileRegion.x1, thisRegion.x1 ) &&
          TCTF_IsEQ( tileRegion.y1, thisRegion.y1 ))
      {
         this->ptileLL_ = ptile;
      }
   }
   return( ptile );
}

//===========================================================================//
// Method         : Deallocate_
// Purpose        : Deallocates the given tile.  This method releases memory 
//                  asso. with the given tile and updates the global tile
//                  plane pointers, as needed.
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > void TTPT_TilePlane_c< T >::Deallocate_(
      TTPT_Tile_c< T >* ptile )
{
   if( this->ptileLL_ == ptile )
   {
      const TGS_Region_c& region = this->region_;
      this->ptileLL_ = this->Find( region.x1, region.y1 );
   }
   if( this->ptileLL_ == ptile )
   {
      const TGS_Region_c& region = this->region_;
      this->ptileLL_ = this->Find( region.x1, region.y2 );
   }
   if( this->ptileLL_ == ptile )
   {
      const TGS_Region_c& region = this->region_;
      this->ptileLL_ = this->Find( region.x2, region.y2 );
   }
   if( this->ptileLL_ == ptile )
   {
      const TGS_Region_c& region = this->region_;
      this->ptileLL_ = this->Find( region.x2, region.y1 );
   }

   if( this->ptileMRC_ == ptile )
   {
      TTPT_Tile_c< T >* ptileLowerLeft = ptile->GetStitchLowerLeft( );
      TTPT_Tile_c< T >* ptileLeftLower = ptile->GetStitchLeftLower( );
      TTPT_Tile_c< T >* ptileRightUpper = ptile->GetStitchRightUpper( );
      TTPT_Tile_c< T >* ptileUpperRight = ptile->GetStitchUpperRight( );

      if( ptileLowerLeft && ptileLowerLeft->IsClear( ))
      {
         this->ptileMRC_ = ptileLowerLeft;
      }
      else if( ptileLeftLower && ptileLeftLower->IsClear( ))
      {
         this->ptileMRC_ = ptileLeftLower;
      }
      else if( ptileRightUpper && ptileRightUpper->IsClear( ))
      {
         this->ptileMRC_ = ptileRightUpper;
      }
      else if( ptileUpperRight && ptileUpperRight->IsClear( ))
      {
         this->ptileMRC_ = ptileUpperRight;
      }
      else
      {
         this->ptileMRC_ = this->ptileLL_;
      }
   }   
   if( this->ptileMRS_ == ptile )
   {
      TTPT_Tile_c< T >* ptileLowerLeft = ptile->GetStitchLowerLeft( );
      TTPT_Tile_c< T >* ptileLeftLower = ptile->GetStitchLeftLower( );
      TTPT_Tile_c< T >* ptileRightUpper = ptile->GetStitchRightUpper( );
      TTPT_Tile_c< T >* ptileUpperRight = ptile->GetStitchUpperRight( );

      if( ptileLowerLeft && ptileLowerLeft->IsSolid( ))
      {
         this->ptileMRS_ = ptileLowerLeft;
      }
      else if( ptileLeftLower && ptileLeftLower->IsSolid( ))
      {
         this->ptileMRS_ = ptileLeftLower;
      }
      else if( ptileRightUpper && ptileRightUpper->IsSolid( ))
      {
         this->ptileMRS_ = ptileRightUpper;
      }
      else if( ptileUpperRight && ptileUpperRight->IsSolid( ))
      {
         this->ptileMRS_ = ptileUpperRight;
      }
      else
      {
         this->ptileMRS_ = this->ptileLL_;
      }
   }   
   delete ptile;
}

//===========================================================================//
// Method         : ShowInternalMessage_
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/30/12 jeffr : Original
//===========================================================================//
template< class T > bool TTPT_TilePlane_c< T >::ShowInternalMessage_(
            TTP_MessageType_t messageType,
	    const char*       pszSourceMethod,
      const TGS_Region_c*     ptileRegion, 
      const TGS_Region_c*     pnextRegion ) const
{
   bool ok = true;

   string srTileRegion;
   if( ptileRegion )
   {
      ptileRegion->ExtractString( &srTileRegion );
   }
   string srNextRegion;
   if( pnextRegion )
   {
      pnextRegion->ExtractString( &srNextRegion );
   }
   string srThisRegion;

   TIO_PrintHandler_c& printHandler = TIO_PrintHandler_c::GetInstance( );

   switch( messageType )
   {
   case TTP_MESSAGE_IS_LEGAL_INVALID_REGION:

      ok = printHandler.Internal( pszSourceMethod,
                                  "Detected corrupted tile plane due to invalid tile region at (%s).\n",
                                  TIO_SR_STR( srTileRegion ));
      break;

   case TTP_MESSAGE_IS_LEGAL_ADJACENT_CLEAR_TILE:

      ok = printHandler.Internal( pszSourceMethod,
                                  "Detected corrupted tile plane due to adjacent clear tiles at (%s) and (%s).\n",
                                  TIO_SR_STR( srTileRegion ),
                                  TIO_SR_STR( srNextRegion ));
      break;

   case TTP_MESSAGE_TILE_REGION_ILLEGAL:

      ok = printHandler.Internal( pszSourceMethod,
                                  "Detected illegal tile plane region coordinates (%s).\n",
                                  TIO_SR_STR( srTileRegion ));
      break;

   case TTP_MESSAGE_ADD_PER_NEW_INVALID:

      ok = printHandler.Internal( pszSourceMethod,
                                  "Can't add new tile region (%s).\n"
                                  "%sRegion intersects with existing tile region (%s).\n",
                                  TIO_SR_STR( srTileRegion ),
                                  TIO_PREFIX_INTERNAL_SPACE,
                                  TIO_SR_STR( srThisRegion ));
      break;

   case TTP_MESSAGE_ADD_PER_NEW_EXISTS:

      ok = printHandler.Internal( pszSourceMethod,
                                  "Can't add new tile region (%s).\n"
                                  "%sRegion intersects with existing tile region (%s).\n",
                                  TIO_SR_STR( srTileRegion ),
                                  TIO_PREFIX_INTERNAL_SPACE,
                                  TIO_SR_STR( srNextRegion ));
      break;

   case TTP_MESSAGE_ADD_PER_MERGE_INVALID:

      ok = printHandler.Internal( pszSourceMethod,
                                  "Can't add merge tile region (%s).\n"
                                  "%sRegion is not within tile plane's region (%s).\n",
                                  TIO_SR_STR( srTileRegion ),
                                  TIO_PREFIX_INTERNAL_SPACE,
                                  TIO_SR_STR( srThisRegion ));
      break;

   case TTP_MESSAGE_ADD_PER_MERGE_EXISTS:

      ok = printHandler.Internal( pszSourceMethod,
                                  "Can't add merge tile region (%s).\n"
                                  "%sRegion intersects with existing tile region (%s).\n",
                                  TIO_SR_STR( srTileRegion ),
                                  TIO_PREFIX_INTERNAL_SPACE,
                                  TIO_SR_STR( srNextRegion ));
      break;

   case TTP_MESSAGE_ADD_PER_OVERLAP_INVALID:

      this->region_.ExtractString( &srThisRegion );
      ok = printHandler.Internal( pszSourceMethod,
                                  "Can't add overlap tile region (%s).\n"
                                  "%sRegion is not within tile plane's region (%s).\n",
                                  TIO_SR_STR( srTileRegion ),
                                  TIO_PREFIX_INTERNAL_SPACE,
                                  TIO_SR_STR( srThisRegion ));
      break;

   case TTP_MESSAGE_DELETE_PER_EXACT_INVALID:
   case TTP_MESSAGE_DELETE_PER_WITHIN_INVALID:
   case TTP_MESSAGE_DELETE_PER_INTERSECT_INVALID:

      this->region_.ExtractString( &srThisRegion );
      ok = printHandler.Internal( pszSourceMethod,
                                  "Can't delete existing tile region (%s).\n"
                                  "%sRegion is not within tile plane's region (%s).\n",
                                  TIO_SR_STR( srTileRegion ),
                                  TIO_PREFIX_INTERNAL_SPACE,
                                  TIO_SR_STR( srThisRegion ));
      break;

   default:
      break;
   }
   return( ok );
}

#endif
