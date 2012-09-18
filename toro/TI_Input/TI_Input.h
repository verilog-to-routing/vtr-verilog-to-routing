//===========================================================================//
// Purpose : Declaration and inline definitions for a TI_Input class.  This
//           class is responsible for reading and parsing the various input 
//           options, architecture, fabric, and circuit file(s).
//
//===========================================================================//
#ifndef TI_INPUT_H
#define TI_INPUT_H

#include "TCL_CommandLine.h"

#include "TOS_OptionsStore.h"

#include "TFS_FloorplanStore.h"

#include "TI_Typedefs.h"

//===========================================================================//
// Purpose        : Class declaration
// Author         : Jeff Rudolph
//---------------------------------------------------------------------------//
// Version history
// 05/01/12 jeffr : Original
//===========================================================================//
class TI_Input_c
{
public:

   TI_Input_c( void );
   ~TI_Input_c( void );

   bool Init( const TCL_CommandLine_c& commandLine,
              TOS_OptionsStore_c* poptionsStore,
              TFS_FloorplanStore_c* pfloorplanStore );

   bool Apply( const TCL_CommandLine_c& commandLine,
               TOS_OptionsStore_c* poptionsStore,
               TFS_FloorplanStore_c* pfloorplanStore );
   bool Apply( void );

   bool IsValid( void ) const;

private:

   enum TI_FileMode_e 
   { 
      TI_FILE_OPTIONS,
      TI_FILE_XML,
      TI_FILE_BLIF,
      TI_FILE_ARCHITECTURE,
      TI_FILE_FABRIC,
      TI_FILE_CIRCUIT
   };
                        
private:

   // TI_InputRead.cxx:
   bool ReadFileProcess_( TI_Input_c::TI_FileMode_e fileMode,
                          const char* pszFileType,
                          bool showInfoMessage = true );
   bool ReadFileParser_( TI_Input_c::TI_FileMode_e fileMode,
                         const char* pszFileType, 
                         const char* pszFileName,
                         bool showInfoMessage = true );
   bool GetFileNameList_( TI_Input_c::TI_FileMode_e fileMode,
                          TI_FileNameList_t* pfileNameList  ) const;

   bool LoadOptionsFiles_( void );
   void ApplyCommandLineOptions_( void );

   // TI_InputUtils.cxx:
   void DisplayProgramOwner_( void ) const;
   void DisplayProgramCommand_( void ) const;

   void BuildDefaultBaseName_( const string& srOptionsFileName,
                               string* psrDefaultBaseName ) const;

   void ApplyDefaultBaseName_( const string& srDefaultBaseName ) const;
   void ApplyDefaultBaseName_( string* psrFileName,
                               const string& srDefaultBaseName ) const;
   void ApplyDefaultBaseName_( string* psrFileName,
                               const string& srDefaultBaseName,
                               const char* pszDefaultExtName ) const;
   void ApplyDefaultBaseName_( const string& srFileName,
                               string* psrFileName,
                               const string& srDefaultBaseName,
                               const string& srDefaultExtName ) const;

   void ReplaceWildCardName_( const string& srFileName,
                              string* psrFileName,
                              const char* pszWildcardName,
                              const string& srReplaceName ) const;

   // TI_InputHandle.cxx:
   void HandleOptionsStore_( void );
   bool HandleArchitectureSpec_( void );
   void HandleFabricModel_( void );
   bool HandleCircuitDesign_( void );

   // TI_InputValidate.cxx:
   bool ValidateOptionsStore_( void );
   bool ValidateArchitectureSpec_( void );
   bool ValidateFabricModel_( void );
   bool ValidateCircuitDesign_( void );

private:

   const TCL_CommandLine_c*     pcommandLine_;

   TOS_OptionsStore_c*          poptionsStore_;
   const TOS_ControlSwitches_c* pcontrolSwitches_;
   const TOS_RulesSwitches_c*   prulesSwitches_;

   TFS_FloorplanStore_c*        pfloorplanStore_;
   TAS_ArchitectureSpec_c*      parchitectureSpec_;
   TFM_FabricModel_c*           pfabricModel_;
   TCD_CircuitDesign_c*         pcircuitDesign_;
};

#endif
