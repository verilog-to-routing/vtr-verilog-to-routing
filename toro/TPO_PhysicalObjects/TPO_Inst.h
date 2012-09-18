//===========================================================================//
// Purpose : Declaration and inline definitions for a TPO_Inst class.
//
//           Inline methods include:
//           - SetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetName (required for TCT_SortedNameDynamicVector_c class)
//           - GetCompare (required for TCT_BSearch and TCT_QSort classes)
//           - GetCellName
//           - GetPinList
//           - GetSource
//           - GetInputOutputType
//           - GetNamesLogicBitsList
//           - GetLatchClockType, GetLatchInitState
//           - GetSubcktPinMapList
//           - GetPackHierMapList
//           - GetPlaceStatus, GetPlaceFabricName
//           - GetPlaceRelativeList, GetPlaceRegionList
//           - SetCellName
//           - SetPinList
//           - SetSource
//           - IsValid
//
//===========================================================================//

#ifndef TPO_INST_H
#define TPO_INST_H

#include <stdio.h>

#include <string>
using namespace std;

#include "TC_Bit.h"
#include "TC_NameType.h"
#include "TC_SideName.h"

#include "TGS_Typedefs.h"
#include "TGS_Region.h"

#include "TLO_Typedefs.h"
#include "TLO_Cell.h"

#include "TPO_Typedefs.h"
#include "TPO_PinMap.h"
#include "TPO_HierMap.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
class TPO_Inst_c
{
public:

   TPO_Inst_c( void );
   TPO_Inst_c( const string& srName );
   TPO_Inst_c( const char* pszName );
   TPO_Inst_c( const string& srName,
	       const string& srCellName,
	       const TPO_PinList_t& pinList,
               TPO_InstSource_t source,
               const TPO_LogicBitsList_t& logicBitsList );
   TPO_Inst_c( const char* pszName,
	       const char* pszCellName,
	       const TPO_PinList_t& pinList,
               TPO_InstSource_t source,
               const TPO_LogicBitsList_t& logicBitsList );
   TPO_Inst_c( const string& srName,
	       const string& srCellName,
	       const TPO_PinList_t& pinList,
               TPO_InstSource_t source,
               TPO_LatchType_t clockType,
               TPO_LatchState_t initState );
   TPO_Inst_c( const char* pszName,
	       const char* pszCellName,
	       const TPO_PinList_t& pinList,
               TPO_InstSource_t source,
               TPO_LatchType_t clockType,
               TPO_LatchState_t initState );
   TPO_Inst_c( const string& srName,
	       const string& srCellName,
               TPO_InstSource_t source,
	       const TPO_PinMapList_t& pinMapList );
   TPO_Inst_c( const char* pszName,
	       const char* pszCellName,
               TPO_InstSource_t source,
	       const TPO_PinMapList_t& pinMapList );
   TPO_Inst_c( const TPO_Inst_c& inst );
   ~TPO_Inst_c( void );

   TPO_Inst_c& operator=( const TPO_Inst_c& inst );
   bool operator<( const TPO_Inst_c& inst ) const;
   bool operator==( const TPO_Inst_c& inst ) const;
   bool operator!=( const TPO_Inst_c& inst ) const;

   void Print( FILE* pfile = stdout, size_t spaceLen = 0 ) const;
   void PrintBLIF( FILE* pfile = stdout, size_t spaceLen = 0 ) const;

   void SetName( const string& srName );
   void SetName( const char* pszName );
   const char* GetName( void ) const;
   const char* GetCompare( void ) const;

   const char* GetCellName( void ) const;
   const TPO_PinList_t& GetPinList( void ) const;
   TPO_InstSource_t GetSource( void ) const;

   TC_TypeMode_t GetInputOutputType( void ) const;
   const TPO_LogicBitsList_t& GetNamesLogicBitsList( void ) const;
   TPO_LatchType_t GetLatchClockType( void ) const;
   TPO_LatchState_t GetLatchInitState( void ) const;
   const TPO_PinMapList_t& GetSubcktPinMapList( void ) const;

   const TPO_HierMapList_t& GetPackHierMapList( void ) const;
   TPO_StatusMode_t GetPlaceStatus( void ) const;
   const char* GetPlaceFabricName( void ) const;
   const TPO_RelativeList_t& GetPlaceRelativeList( void ) const;
   const TGS_RegionList_t& GetPlaceRegionList( void ) const;
   
   void SetCellName( const string& srCellName );
   void SetCellName( const char* pszCellName );
   void SetPinList( const TPO_PinList_t& pinList );
   void SetInputOutputType( TC_TypeMode_t type );
   void SetNamesLogicBitsList( const TPO_LogicBitsList_t& logicBitsList );
   void SetLatchClockType( TPO_LatchType_t clockType );
   void SetLatchInitState( TPO_LatchState_t initState );
   void SetSubcktPinMapList( const TPO_PinMapList_t& pinMapList );
   void SetSource( TPO_InstSource_t source );

   void SetPackHierMapList( const TPO_HierMapList_t& hierMapList );
   void SetPlaceStatus( TPO_StatusMode_t status );
   void SetPlaceFabricName( const string& srFabricName );
   void SetPlaceFabricName( const char* pszFabricName );
   void SetPlaceRelativeList( const TPO_RelativeList_t& relativeList );
   void SetPlaceRegionList( const TGS_RegionList_t& regionList );

   void AddPin( const TPO_Pin_t& pin );

   const TPO_Pin_t* FindPin( TC_TypeMode_t type ) const;

   size_t FindPinCount( TC_TypeMode_t type,
                        const TLO_CellList_t& cellList ) const;

   void Clear( void );

   bool IsValid( void ) const;

private:

   string           srName_;      // Defines instance name
   string           srCellName_;  // Defines instance's master cell name
   TPO_PinList_t    pinList_;     // List of instance's pins

   TPO_InstSource_t source_;      // Defines instance source (optional)
                                  // For example: ".input", ".names", or ".latch"
   class TPO_InstInputOutput_c
   {
   public:

      TC_TypeMode_t type;         // Defines port type (optional)
                                  // For example: ".input" or ".output"
   } inputOutput_;

   class TPO_InstNames_c
   {
   public:

      TPO_LogicBitsList_t logicBitsList;  
                                  // Defines PLA logic bits (optional for ".names")
   } names_;                      // For example: "1--1 1", "-1-1 1", or "0-11 1"

   class TPO_InstLatch_c
   {
   public:
   
      TPO_LatchType_t clockType;  // Defines clocking type (optional for ".latch")
                                  // For example: FE, RE, AH, AL, or AS
      TPO_LatchState_t initState; // Defines initial state (optional for ".latch")
                                  // For example: true, false, or don't care
   } latch_;

   class TPO_InstSubckt_c
   {
   public:

      TPO_PinMapList_t pinMapList;// Defines ".subckt" pin-to-pin map table
   } subckt_;

   class TPO_InstPack_c
   {
   public:

      TPO_HierMapList_t hierMapList;
                                  // Defines packing (instance to PB architecture)
   } pack_;

   class TPO_InstPlace_c
   {
   public:

      TPO_StatusMode_t status;    // Defines placement status mode (optional)
      string srFabricName;        // Defines placement fabric name (optional)
      TPO_RelativeList_t relativeList;
                                  // Defines placement relative(s) (optional)
      TGS_RegionList_t regionList;// Defines placement region(s) (optional)
   } place_;

private:

   enum TPO_DefCapacity_e 
   { 
      TPO_PIN_LIST_DEF_CAPACITY = 64,
      TPO_LOGIC_BITS_LIST_DEF_CAPACITY = 8,
      TPO_PIN_MAP_LIST_DEF_CAPACITY = 64,
      TPO_HIER_MAP_LIST_DEF_CAPACITY = 1,
      TPO_RELATIVE_LIST_DEF_CAPACITY = 1,
      TPO_REGION_LIST_DEF_CAPACITY = 1
   };
};

//===========================================================================//
// Purpose        : Class inline definition(s)
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/15/12 jeffr : Original
//===========================================================================//
inline void TPO_Inst_c::SetName( 
      const string& srName )
{
   this->srName_ = srName;
}

//===========================================================================//
inline void TPO_Inst_c::SetName( 
      const char* pszName )
{
   this->srName_ = TIO_PSZ_STR( pszName );
}

//===========================================================================//
inline const char* TPO_Inst_c::GetName( 
      void ) const
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TPO_Inst_c::GetCompare( 
      void ) const 
{
   return( TIO_SR_STR( this->srName_ ));
}

//===========================================================================//
inline const char* TPO_Inst_c::GetCellName( 
      void ) const 
{
   return( TIO_SR_STR( this->srCellName_ ));
}

//===========================================================================//
inline const TPO_PinList_t& TPO_Inst_c::GetPinList( 
      void ) const
{
   return( this->pinList_ );
}

//===========================================================================//
inline TPO_InstSource_t TPO_Inst_c::GetSource( 
      void ) const
{
   return( this->source_ );
}

//===========================================================================//
inline TC_TypeMode_t TPO_Inst_c::GetInputOutputType( 
      void ) const
{
   return( this->inputOutput_.type );
}

//===========================================================================//
inline const TPO_LogicBitsList_t& TPO_Inst_c::GetNamesLogicBitsList( 
      void ) const
{
   return( this->names_.logicBitsList );
}

//===========================================================================//
inline TPO_LatchType_t TPO_Inst_c::GetLatchClockType( 
      void ) const
{
   return( this->latch_.clockType );
}

//===========================================================================//
inline TPO_LatchState_t TPO_Inst_c::GetLatchInitState( 
      void ) const
{
   return( this->latch_.initState );
}

//===========================================================================//
inline const TPO_PinMapList_t& TPO_Inst_c::GetSubcktPinMapList( 
      void ) const
{
   return( this->subckt_.pinMapList );
}

//===========================================================================//
inline const TPO_HierMapList_t& TPO_Inst_c::GetPackHierMapList( 
      void ) const
{
   return( this->pack_.hierMapList );
}

//===========================================================================//
inline TPO_StatusMode_t TPO_Inst_c::GetPlaceStatus(
      void ) const
{
   return( this->place_.status );
}

//===========================================================================//
inline const char* TPO_Inst_c::GetPlaceFabricName( 
      void ) const
{
   return( TIO_SR_STR( this->place_.srFabricName ));
}

//===========================================================================//
inline const TPO_RelativeList_t& TPO_Inst_c::GetPlaceRelativeList( 
      void ) const
{
   return( this->place_.relativeList );
}

//===========================================================================//
inline const TGS_RegionList_t& TPO_Inst_c::GetPlaceRegionList( 
      void ) const
{
   return( this->place_.regionList );
}

//===========================================================================//
inline void TPO_Inst_c::SetCellName( 
      const string& srCellName )
{
   this->srCellName_ = srCellName;
}

//===========================================================================//
inline void TPO_Inst_c::SetCellName( 
      const char* pszCellName )
{
   this->srCellName_ = TIO_PSZ_STR( pszCellName );
}

//===========================================================================//
inline void TPO_Inst_c::SetPinList(
      const TPO_PinList_t& pinList )
{
   this->pinList_ = pinList;
}

//===========================================================================//
inline void TPO_Inst_c::SetSource(
      TPO_InstSource_t source )
{
   this->source_ = source;
}

//===========================================================================//
inline bool TPO_Inst_c::IsValid( 
      void ) const
{
   return( this->srName_.length( ) ? true : false );
}

#endif
