/* Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
 *           Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com) and
 *            Dr. Kenneth B. Kent (ken@unb.ca)
 *            for the Reconfigurable Computing Research Lab at the
 *             Univerity of New Brunswick in Fredericton, New Brunswick, Canada
 */

#ifndef RTL_UTILS_H
#define RTL_UTILS_H

#include <string>
#include <iostream>

#include <string.h>

#ifndef FILE_NAME
#define FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifndef ERR_MSG
#define ERR_MSG(errMsg) {\
    std::cout << std::endl << "ERROR: " << FILE_NAME << ":" << __LINE__ << " " << __func__ << "()" << ": " << errMsg << "!" << std::endl << std::endl;\
    }
#endif


std::string string_of_radix_to_bitstring(std::string orig_string, short radix);

inline void _assert_Werr(bool cond, const char *FUNCT, int LINE, std::string error_string)
{
    if (!cond) { 
        std::cout << std::endl << "ERROR: " << FUNCT << "::" << std::to_string(LINE) << " Assert 'assert_Werr' Failed:\t" << error_string << "!" << std::endl << std::endl;
        std::abort();
    }
}
#define assert_Werr(cond, error_string) _assert_Werr((cond),  __func__, __LINE__, std::string(error_string))

#endif
