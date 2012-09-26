//===========================================================================//
// Purpose : Declaration and inline definitions for a TO_Output class.  This
//           class is responsible for formatting and writing various data and
//           report file(s).
//
//===========================================================================//

#ifndef TO_OUTPUT_H
#define TO_OUTPUT_H

#include <cstdio>
using namespace std;

#include "TCL_CommandLine.h"

#include "TOS_OptionsStore.h"

#include "TFS_FloorplanStore.h"

#include "TO_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TO_Output_c
{
public:

   TO_Output_c( void );
   ~TO_Output_c( void );

   bool Init( const TCL_CommandLine_c& commandLine,
              const TOS_OptionsStore_c& optionsStore,
              const TFS_FloorplanStore_c& floorplanStore );

   bool Apply( const TCL_CommandLine_c& commandLine,
               const TOS_OptionsStore_c& optionsStore,
               const TFS_FloorplanStore_c& floorplanStore );
   bool Apply( void );

   bool IsValid( void ) const;

private:

   // See TO_Output.cxx
   void WriteFileHeader_( FILE* pfile,
                          const char* pszFileType,
                          const char* pszPrefix,
                          const char* pszPostfix,
                          const char* pszMessage = 0 ) const;
   void WriteFileFooter_( FILE* pfile,
                          const char* pszPrefix,
                          const char* pszPostfix ) const;

   // See TO_OutputWriteOptions.cxx
   bool WriteOptionsFile_( const TOS_OptionsStore_c& optionsStore,
                           const TOS_OutputOptions_c& outputOptions ) const;

   // See TO_OutputWriteXml.cxx
   bool WriteXmlFile_( const TAS_ArchitectureSpec_c& architectureSpec,
                       const TOS_OutputOptions_c& outputOptions ) const;

   // See TO_OutputWriteBlif.cxx
   bool WriteBlifFile_( const TCD_CircuitDesign_c& circuitDesign,
                        const TOS_OutputOptions_c& outputOptions ) const;

   // See TO_OutputWriteArchitecture.cxx
   bool WriteArchitectureFile_( const TAS_ArchitectureSpec_c& architectureSpec,
                                const TOS_OutputOptions_c& outputOptions ) const;

   // See TO_OutputWriteFabric.cxx
   bool WriteFabricFile_( const TFM_FabricModel_c& fabricModel,
                          const TOS_OutputOptions_c& outputOptions ) const;

   // See TO_OutputWriteCircuit.cxx
   bool WriteCircuitFile_( const TCD_CircuitDesign_c& circuitDesign,
                           const TOS_OutputOptions_c& outputOptions ) const;

   // See TO_OutputWriteLaff.cxx
   bool WriteLaffFile_( const TFV_FabricView_c& fabricView,
                        const TOS_OutputOptions_c& outputOptions ) const;

   void WriteLaffBarInfo_( void ) const;
   void WriteLaffHeader_( void ) const;
   void WriteLaffTrailer_( void ) const;

   void WriteLaffBoundingBox_( const TFV_FabricView_c& fabricView ) const;
   void WriteLaffInternalGrid_( const TFV_FabricView_c& fabricView ) const;
   void WriteLaffFabricView_( const TFV_FabricView_c& fabricView ) const;
   void WriteLaffFabricPlane_( const TFV_FabricView_c& fabricView,
                               TGS_Layer_t layer,
                               const TFV_FabricPlane_c& fabricPlane ) const;

   void WriteLaffFabricMapTable_( const TGS_Region_c& region,
			 	  const TFV_FabricData_c& fabricData ) const;
   void WriteLaffFabricMapTable_( const TGS_Region_c& region,
		 		  const TFV_FabricData_c& fabricData,
                                  TC_SideMode_t side,
                                  const TC_MapSideList_t& mapSideList ) const;
   void WriteLaffFabricMapTable_( const TGS_Region_c& region,
	 			  const TFV_FabricData_c& fabricData,
                                  TC_SideMode_t side,
                                  unsigned int index,
                                  const TC_SideList_t& sideList ) const;

   void WriteLaffFabricPinList_( const TGS_Region_c& region,
                                 const TFV_FabricPinList_t& pinList ) const;

   void WriteLaffFabricConnectionList_( const TFV_FabricView_c& fabricView,
                                        const TGS_Region_c& region,
                                        const TFV_ConnectionList_t& connectionList ) const;

   void WriteLaffBoundary_( TO_LaffLayerId_t layerId,
                            const TGS_Region_c& region ) const;
   void WriteLaffMarker_( TO_LaffLayerId_t layerId,
                          const TGS_Point_c& point,
                          double length ) const;
   void WriteLaffLine_( TO_LaffLayerId_t layerId,
                        const TGS_Line_c& line,
                        const char* pszText = 0 ) const;
   void WriteLaffLine_( TO_LaffLayerId_t layerId,
                        const TGS_Line_c& line,
                        const string& srText ) const;
   void WriteLaffRegion_( TO_LaffLayerId_t layerId,
                          const TGS_Region_c& region,
                          const char* pszText = 0 ) const;
   void WriteLaffRegion_( TO_LaffLayerId_t layerId,
                          const TGS_Region_c& region,
                          const string& srText ) const;
   void WriteLaffRegion_( TO_LaffLayerId_t layerId,
                          const TGS_Point_c& point,
                          double scale,
                          const char* pszText = 0 ) const;
   void WriteLaffRegion_( TO_LaffLayerId_t layerId,
                          const TGS_Point_c& point,
                          double scale,
                          const string& srText ) const;

   TO_LaffLayerId_t WriteLaffMapLayerId_( TGS_Layer_t layer ) const;

   // See TO_OutputWriteMetrics.cxx
   bool WriteMetricsFile_( void ) const;
 
   // See TO_OutputEmailMetrics.cxx
   bool SendMetricsEmail_( const TOS_InputOptions_c& inputOptions,
                           const TOS_OutputOptions_c& outputOptions ) const;
   void BuildMetricsEmailSubject_( const char* pszFileName,
                                   string* psrSubject ) const;
   void BuildMetricsEmailBody_( const string& srCommandLine,
                                string* psrBody ) const;
   bool MailMetricsEmailMessage_( const string& srAddress,
                                  const string& srSubject,
                                  const string& srBody ) const;
private:

   const TCL_CommandLine_c*      pcommandLine_;

   const TOS_OptionsStore_c*     poptionsStore_;
   const TOS_ControlSwitches_c*  pcontrolSwitches_;
   const TOS_RulesSwitches_c*    prulesSwitches_;


   const TFS_FloorplanStore_c*   pfloorplanStore_;
   const TAS_ArchitectureSpec_c* parchitectureSpec_;
   const TFM_FabricModel_c*      pfabricModel_;
   const TCD_CircuitDesign_c*    pcircuitDesign_;
};

#endif 
