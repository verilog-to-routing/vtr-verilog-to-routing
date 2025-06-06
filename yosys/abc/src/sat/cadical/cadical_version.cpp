#include "global.h"

/*------------------------------------------------------------------------*/

// To simplify the build process without 'make', you can disable the
// generation of 'build.hpp' through '../scripts/make-build-header.sh' by
// defining '-DCADICAL_NBUILD'.  Then we try to guess part of the configuration.

#ifndef CADICAL_NBUILD
#if __GNUC__ > 4
#if __has_include(<build.hpp>)
#include "build.hpp"
#endif // __has_include
#else
#include "build.hpp"
#endif // __GNUC > 4
#endif // CADICAL_NBUILD

/*------------------------------------------------------------------------*/

// We prefer short 3 character version numbers made of digits and lower case
// letters only, which gives 36^3 = 46656 different versions.  The following
// macro is used for the non-standard build process and only set from
// the file '../VERSION' with '../scripts/update-version.sh'.  The standard
// build process relies on 'VERSION' to be defined in 'build.hpp'.

#ifdef CADICAL_NBUILD
#ifndef VERSION
#define VERSION "2.2.0-rc1"
#endif // VERSION
#endif // CADICAL_NBUILD

    /*------------------------------------------------------------------------*/

    // The copyright of the code is here.

    static const char *COPYRIGHT = "Copyright (c) 2016-2024";
static const char *AUTHORS =
    "A. Biere, M. Fleury, N. Froleyks, K. Fazekas, F. Pollitt, T. Faller";
static const char *AFFILIATIONS =
    "JKU Linz, University of Freiburg, TU Wien";

/*------------------------------------------------------------------------*/

// Again if we do not have 'CADICAL_NBUILD' or if something during configuration is
// broken we still want to be able to compile the solver.  In this case we
// then try our best to figure out 'COMPILER' and 'DATE', but for
// 'IDENTIFIER' and 'FLAGS' we can only use the '0' string, which marks
// those as unknown.

#ifndef COMPILER
#ifdef __clang__
#ifdef __VERSION__
#define COMPILER "clang++-" __VERSION__
#else
#define COMPILER "clang++"
#endif
#elif defined(__GNUC__)
#ifdef __VERSION__
#define COMPILER "g++-" __VERSION__
#else
#define COMPILER "g++"
#endif
#else
#define COMPILER 0
#endif
#endif

// GIT SHA2 identifier.
//
#ifndef IDENTIFIER
#define IDENTIFIER 0
#endif
#ifdef SHORTID
#define SHORTIDSTR "-" SHORTID
#else
#define SHORTIDSTR ""
#define SHORTID 0
#endif

// Compilation flags.
//
#ifndef FLAGS
#define FLAGS 0
#endif

// Build Time and operating system.
//
#ifndef DATE
#define DATE __DATE__ " " __TIME__
#endif

/*------------------------------------------------------------------------*/

#include "version.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

const char *version () { return VERSION; }
const char *copyright () { return COPYRIGHT; }
const char *authors () { return AUTHORS; }
const char *affiliations () { return AFFILIATIONS; }
const char *signature () { return "cadical-" VERSION SHORTIDSTR; }
const char *identifier () { return IDENTIFIER; }
const char *compiler () { return COMPILER; }
const char *date () { return DATE; }
const char *flags () { return FLAGS; }

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
