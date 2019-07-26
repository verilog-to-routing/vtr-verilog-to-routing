/* Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
 *           Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com),
 *            Alexandrea Demmings (alexandrea.demmings@unb.ca, lxdemmings@gmail.com) and
 *             Dr. Kenneth B. Kent (ken@unb.ca)
 *             for the Reconfigurable Computing Research Lab at the
 *              Univerity of New Brunswick in Fredericton, New Brunswick, Canada
 */

#ifndef RTL_UTILS_H
#define RTL_UTILS_H

#include <string>
#include <iostream>

#include <string.h>

#ifndef FILE_NAME
#define FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

/* Enable Debug Messages for libRTLNumber: Un-Comment to Enable Debug Messages:
 *                                          Comment-out to Disable Debug Messages: */
// #define ENABLE_DEBUG_MESSAGES

#ifdef ENABLE_DEBUG_MESSAGES
#define DEBUG_MSG(debugMsg) std::cerr << "DEBUG: " << FILE_NAME << ":" << __LINE__ << " " << __func__ << "()" << ": " << debugMsg << std::endl
#else
#define DEBUG_MSG(debugMsg) /* No-Op */
#endif

#ifndef WARN_MSG
#define WARN_MSG(warnMSG) std::cerr << "WARNING: " << FILE_NAME << ":" << __LINE__ << " " << __func__ << "()" << ": " << warnMSG << "!" << std::endl
#endif

#ifndef ERR_MSG
#define ERR_MSG(errMsg) std::cerr << std::endl << "ERROR: " << FILE_NAME << ":" << __LINE__ << " " << __func__ << "()" << ": " << errMsg << "!" << std::endl << std::endl
#endif


std::string string_of_radix_to_bitstring(std::string orig_string, size_t radix);

inline void _assert_Werr(bool cond, const char *FUNCT, int LINE, std::string error_string)
{
    if (!cond) { 
        std::cerr << std::endl << "ERROR: " << FUNCT << "::" << std::to_string(LINE) << " Assert 'assert_Werr' Failed:\t" << error_string << "!" << std::endl << std::endl;
        std::abort();
    }
}
#define assert_Werr(cond, error_string) _assert_Werr((cond),  __func__, __LINE__, std::string(error_string))

#endif
