#ifndef EXPAT_CONFIG_H
#define EXPAT_CONFIG_H

#ifdef _MSC_VER
#  include <libstudxml/details/config-vc.h>
#else
#  include <libstudxml/details/config.h>
#endif

#define BYTEORDER LIBSTUDXML_BYTEORDER

#define XML_NS            1
#define XML_DTD           1
#define XML_CONTEXT_BYTES 1024
/* #define XML_FREESTANDING  1 */

#define UNUSED(x) (void)x;

/* Specific for Windows.
 */
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN

#  define COMPILED_FROM_DSP 1
#endif

/* Common for all supported OSes/compilers.
 */
#define HAVE_MEMMOVE 1

#endif /* EXPAT_CONFIG_H */
