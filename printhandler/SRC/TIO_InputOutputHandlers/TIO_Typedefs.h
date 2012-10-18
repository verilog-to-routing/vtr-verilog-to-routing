//===========================================================================//
// Purpose : Enums, typedefs, and defines for TIO_InputOutputHandler classes.
//
//===========================================================================//

#ifndef TIO_TYPEDEFS_H
#define TIO_TYPEDEFS_H

//---------------------------------------------------------------------------//
// Define file handler open modes and CPP constants
//---------------------------------------------------------------------------//

enum TIO_FileOpenMode_e 
{
   TIO_FILE_OPEN_UNDEFINED = 0,
   TIO_FILE_OPEN_READ,
   TIO_FILE_OPEN_WRITE,
   TIO_FILE_OPEN_APPEND,
   TIO_FILE_OPEN_BINARY_READ,
   TIO_FILE_OPEN_BINARY_WRITE,
   TIO_FILE_OPEN_BINARY_APPEND
};
typedef enum TIO_FileOpenMode_e TIO_FileOpenMode_t;

#if defined( SUN8 ) || defined( SUN10 )
   #define TIO_FILE_CPP_COMMAND "/usr/lib/cpp"
   #define TIO_FILE_CPP_OPTIONS "-C"
#elif defined( LINUX_I686 ) || defined( LINUX_X86_64 )
   #define TIO_FILE_CPP_COMMAND "/usr/bin/cpp"
   #define TIO_FILE_CPP_OPTIONS "-C"
#elif defined( WIN32 ) || defined( _WIN32 )
   #define TIO_FILE_CPP_COMMAND "cl.exe"
   #define TIO_FILE_CPP_OPTIONS "/nologo /E"
#else
   #define TIO_FILE_CPP_COMMAND "/usr/bin/cpp"
   #define TIO_FILE_CPP_OPTIONS ""
#endif

#if defined( SUN8 ) || defined( SUN10 ) 
   #define TIO_FILE_HIDDEN_PREFIX "."
#elif defined( LINUX_I686 ) || defined( LINUX_X86_64 )
   #define TIO_FILE_HIDDEN_PREFIX "."
#elif defined( WIN32 ) || defined( _WIN32 )
   #define TIO_FILE_HIDDEN_PREFIX "_"
#else
   #define TIO_FILE_HIDDEN_PREFIX "."
#endif

#if defined( SUN8 ) || defined( SUN10 ) || defined( LINUX_I686 ) || defined( LINUX_X86_64 )
   #define TIO_FILE_HIDDEN_PREFIX "."
#elif defined( WIN32 ) || defined( _WIN32 )
   #define TIO_FILE_HIDDEN_PREFIX "_"
#else
   #define TIO_FILE_HIDDEN_PREFIX "."
#endif

#if defined( SUN8 ) || defined( SUN10 ) || defined( LINUX_I686 ) || defined( LINUX_X86_64 )
   #define TIO_FILE_DIR_DELIMITER "/"
#elif defined( WIN32 ) || defined( _WIN32 )
   #define TIO_FILE_DIR_DELIMITER "\\"
#else
   #define TIO_FILE_DIR_DELIMITER "/"
#endif

//---------------------------------------------------------------------------//
// Define print handler severity level code constants and typedefs
//---------------------------------------------------------------------------//

enum TIO_PrintMode_e
{
   TIO_PRINT_UNDEFINED = 0,
   TIO_PRINT_INFO,
   TIO_PRINT_WARNING,
   TIO_PRINT_ERROR,
   TIO_PRINT_FATAL,
   TIO_PRINT_TRACE,
   TIO_PRINT_INTERNAL,
   TIO_PRINT_DIRECT,
   TIO_PRINT_RETURN
};
typedef enum TIO_PrintMode_e TIO_PrintMode_t;

// Define print message prefix spacing (based on print mode)
// Spacing length assumes: "INFO|ERROR(n)|WARNING(n)|INTERNAL(n): "
#define TIO_PREFIX_INFO_SPACE     "                    "
#define TIO_PREFIX_WARNING_SPACE  "            "
#define TIO_PREFIX_ERROR_SPACE    "          "
#define TIO_PREFIX_FATAL_SPACE    "       "
#define TIO_PREFIX_INTERNAL_SPACE "            "

//---------------------------------------------------------------------------//
// Define print handler output enable/disable options
//---------------------------------------------------------------------------//

enum TIO_PrintOutputMask_e
{
   TIO_PRINT_OUTPUT_UNDEFINED = 0x00,
   TIO_PRINT_OUTPUT_NONE      = 0x00,
   TIO_PRINT_OUTPUT_STDIO     = 0x01,
   TIO_PRINT_OUTPUT_CUSTOM    = 0x02,
   TIO_PRINT_OUTPUT_LOG_FILE  = 0x04,
   TIO_PRINT_OUTPUT_USER_FILE = 0x08,
   TIO_PRINT_OUTPUT_ANY       = 0xFF,
   TIO_PRINT_OUTPUT_ALL       = 0xFF
};
typedef enum TIO_PrintOutputMask_e TIO_PrintOutputMask_t;

//---------------------------------------------------------------------------//
// Define format string lengths (for miscellaneous output strings)
//---------------------------------------------------------------------------//

enum TIO_FormatLengths_e              // Lengths used for formatted output
{ 
   TIO_FORMAT_STRING_LEN_MAX       = 8192,  // Define max message length allowed
   TIO_FORMAT_STRING_LEN_EMAIL     = 16384, // Format: (body,subject,address)
   TIO_FORMAT_STRING_LEN_POINT     = 48,    // Format: (x,y,z)
   TIO_FORMAT_STRING_LEN_BOX       = 80,    // Format: (x,y,z)-(x,y,z)
   TIO_FORMAT_STRING_LEN_REGION    = 64,    // Format: (x,y)-(x,y)
   TIO_FORMAT_STRING_LEN_RECT      = 80,    // Format: z (x,y)-(x,y)
   TIO_FORMAT_STRING_LEN_PATH      = 80,    // Format: (x,y)-(x,y) z width
   TIO_FORMAT_STRING_LEN_LINE      = 80,    // Format: (x,y)-(x,y) z
   TIO_FORMAT_STRING_LEN_VALUE     = 16,    // Format: bool/int/uint/float
   TIO_FORMAT_STRING_LEN_DATA      = 256,   // Format: int/uint/long/float/string
   TIO_FORMAT_STRING_LEN_DATE_TIME = 80,    // Format: [mm/dd/yy hh:mm:ss]
   TIO_FORMAT_STRING_LEN_HEAD_FOOT = 80     // Format: file header|footer
};

//---------------------------------------------------------------------------//
// Define format banner length (for miscellaneous output strings)
//---------------------------------------------------------------------------//

enum TIO_FormatBannerLength_e
{ 
   TIO_FORMAT_BANNER_LEN_DEF = 72
};

//---------------------------------------------------------------------------//
// Define command lengths (for miscellaneous command strings)
//---------------------------------------------------------------------------//

enum TIO_CommandLengths_e
{ 
   TIO_COMMAND_USERNAME_LEN   = 64,   // Define 'whoami' command file length
   TIO_COMMAND_HOSTNAME_LEN   = 64,   // Define 'hostname' command file length
   TIO_COMMAND_WORKINGDIR_LEN = 512   // Define 'pwd' command file length
};

//---------------------------------------------------------------------------//
// Define inline/macros used for format string convenience
//---------------------------------------------------------------------------//

#define TIO_SR_STR( _sr_ )     (( _sr_ ).length( ) ? ( _sr_ ).data( ) : "" )
#define TIO_PSR_STR( _psr_ )   (( _psr_ ) ? ( _psr_ )->data( ) : "" )
#define TIO_SZ_STR( _sz_ )     (( _sz_ ) ? ( _sz_ ) : "" )
#define TIO_PSZ_STR( _psz_ )   (( _psz_ ) ? ( _psz_ ) : "" )
#define TIO_BOOL_STR( _bool_ ) (( _bool_ ) ? "TRUE" : "FALSE" )
#define TIO_BOOL_VAL( _bool_ ) (( _bool_ ) ? "true" : "false" )

#endif
