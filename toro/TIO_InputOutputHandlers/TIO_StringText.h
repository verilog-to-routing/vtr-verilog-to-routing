//===========================================================================//
// Purpose : Miscellaneous string text defines.
//
//===========================================================================//

#ifndef TIO_STRING_TEXT_H
#define TIO_STRING_TEXT_H

//---------------------------------------------------------------------------//
// Define text used to format and display various program version strings
//---------------------------------------------------------------------------//

#define TIO_SZ_PROGRAM_MAJOR "0"
#define TIO_SZ_PROGRAM_MINOR "A15"
#define TIO_SZ_PROGRAM_PATCH "0"

#if defined( SUN8 )
   #define TIO_SZ_PROGRAM_OPSYS " (Sun8)"
#elif defined( SUN10 )
   #define TIO_SZ_PROGRAM_OPSYS " (Sun10)"
#elif defined( LINUX_I686 )
   #define TIO_SZ_PROGRAM_OPSYS " (Linux I686)"
#elif defined( LINUX_X86_64 )
   #define TIO_SZ_PROGRAM_OPSYS " (Linux X86/64)"
#elif defined( WIN32 ) || defined( _WIN32 )
   #define TIO_SZ_PROGRAM_OPSYS " (Windows/32)"
#else
   #define TIO_SZ_PROGRAM_OPSYS ""
#endif

#if defined( DEBUG )
   #define TIO_SZ_PROGRAM_CCOPT " (D)"
#else
   #define TIO_SZ_PROGRAM_CCOPT ""
#endif

#define TIO_SZ_PROGRAM_VERSION TIO_SZ_PROGRAM_MAJOR "." TIO_SZ_PROGRAM_MINOR "." TIO_SZ_PROGRAM_PATCH \
        TIO_SZ_PROGRAM_CCOPT TIO_SZ_PROGRAM_OPSYS

#define TIO_SZ_BUILD_VERSION    "Version " TIO_SZ_PROGRAM_VERSION
#define TIO_SZ_BUILD_TIME_STAMP __DATE__ ", " __TIME__

//---------------------------------------------------------------------------//
// Define text used to format and display various program skin strings
//---------------------------------------------------------------------------//

#define TIO_SZ_VPR_PROGRAM_NAME   "VPR FPGA Placement and Routing\n" \
                                  "Original VPR by V. Betz\n" \
                                  "Timing-driven placement enhancements by A. Marquardt\n" \
                                  "Single-drivers enhancements by Andy Ye with additions by Mark Fang, Jason Luu, Ted Campbell\n" \
                                  "Heterogeneous stucture support by Jason Luu and Ted Campbell\n" \
                                  "T-VPack clustering integration by Jason Luu\n" \
                                  "Area-driven AAPack added by Jason Luu" \
                                  ""
#define TIO_SZ_VPR_PROGRAM_TITLE  "VPR - The Next Generation"
#define TIO_SZ_VPR_BINARY_NAME    "vpr"
#define TIO_SZ_VPR_SOURCE_NAME    "VPR"
#define TIO_SZ_VPR_COPYRIGHT      "This is free open source code under MIT license."
#define TIO_SZ_VPR_PREFIX         "[vpr] "

#define TIO_SZ_TORO_PROGRAM_NAME  "Toro"
#define TIO_SZ_TORO_PROGRAM_TITLE "Toro - A VPR Front-End"
#define TIO_SZ_TORO_BINARY_NAME   "toro"
#define TIO_SZ_TORO_SOURCE_NAME   "TORO"
#define TIO_SZ_TORO_COPYRIGHT     "(c) Copyright 2012 Texas Instruments"

//---------------------------------------------------------------------------//
// Define strings used to display brief help summary
//---------------------------------------------------------------------------//

#define TIO_SZ_PROGRAM_HELP \
   "Usage: %s\n" \
   "       -h[elp]                   Shows program help\n" \
   "       -v[ersion]                Shows program build version\n" \
   "       -o[ptions]      <file(s)> Input options file name(s)\n" \
   "       -x[ml]          <file>    Input architecture file (VPR's XML)\n" \
   "       -b[lif]         <file>    Input circuit file (VPR's BLIF)\n" \
   "       -a[rchitecture] <file>    Input architecture file\n" \
   "       -f[abric]       <file>    Input fabric file\n" \
   "       -c[ircuit]      <file>    Input circuit file\n" \
   "       +a[rchitecture] <file>    Output architecture file\n" \
   "       +f[abric]       <file>    Output fabric file\n" \
   "       +c[ircuit]      <file>    Output circuit file\n" \
   "       +l[og]          <file>    Output log file name\n" \
   "       -e[xecute]      ( pack + place + route ) | all | none"

//---------------------------------------------------------------------------//
// Define strings used for default file descriptions and extensions
//---------------------------------------------------------------------------//

#define TIO_SZ_INPUT_OPTIONS_DEF_TYPE        "options"
#define TIO_SZ_INPUT_XML_DEF_TYPE            "xml"
#define TIO_SZ_INPUT_BLIF_DEF_TYPE           "blif"
#define TIO_SZ_INPUT_ARCHITECTURE_DEF_TYPE   "architecture"
#define TIO_SZ_INPUT_FABRIC_DEF_TYPE         "fabric"
#define TIO_SZ_INPUT_CIRCUIT_DEF_TYPE        "circuit"

#define TIO_SZ_OUTPUT_OPTIONS_DEF_TYPE       "options"
#define TIO_SZ_OUTPUT_XML_DEF_TYPE           "xml"
#define TIO_SZ_OUTPUT_BLIF_DEF_TYPE          "blif"
#define TIO_SZ_OUTPUT_ARCHITECTURE_DEF_TYPE  "architecture"
#define TIO_SZ_OUTPUT_FABRIC_DEF_TYPE        "fabric"
#define TIO_SZ_OUTPUT_CIRCUIT_DEF_TYPE       "circuit"
#define TIO_SZ_OUTPUT_LAFF_DEF_TYPE          "laff"
#define TIO_SZ_OUTPUT_METRICS_DEF_TYPE       "metrics"
#define TIO_SZ_OUTPUT_LOG_DEF_TYPE           "log"

#define TIO_SZ_INPUT_OPTIONS_DEF_EXT         "opt"
#define TIO_SZ_INPUT_XML_DEF_EXT             "xml"
#define TIO_SZ_INPUT_BLIF_DEF_EXT            "blif"
#define TIO_SZ_INPUT_ARCHITECTURE_DEF_EXT    "arch"
#define TIO_SZ_INPUT_FABRIC_DEF_EXT          "fabric"
#define TIO_SZ_INPUT_CIRCUIT_DEF_EXT         "circuit"

#define TIO_SZ_OUTPUT_OPTIONS_DEF_EXT        "snoitpo"
#define TIO_SZ_OUTPUT_XML_DEF_EXT            "lmx"
#define TIO_SZ_OUTPUT_BLIF_DEF_EXT           "filb"
#define TIO_SZ_OUTPUT_ARCHITECTURE_DEF_EXT   "hcra"
#define TIO_SZ_OUTPUT_FABRIC_DEF_EXT         "cirbaf"
#define TIO_SZ_OUTPUT_CIRCUIT_DEF_EXT        "tiucric"
#define TIO_SZ_OUTPUT_RC_DELAYS_DEF_EXT      "rc"
#define TIO_SZ_OUTPUT_LAFF_DEF_EXT           "laff"
#define TIO_SZ_OUTPUT_METRICS_DEF_EXT        "metrics"
#define TIO_SZ_OUTPUT_LOG_DEF_EXT            "log"

#endif
