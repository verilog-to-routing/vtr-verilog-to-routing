#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <map>

#include "vtr_error.h"

#define nint(a) ((int) floor (a + 0.5))

#define ERRTAG "ERROR:\t"
#define WARNTAG "WARNING:\t"

#include <time.h>
#ifndef CLOCKS_PER_SEC
    #ifndef CLK_PER_SEC
        #error Neither CLOCKS_PER_SEC nor CLK_PER_SEC is defined.
    #endif
    #define CLOCKS_PER_SEC CLK_PER_SEC
#endif

enum e_vpr_error {
	VPR_ERROR_UNKNOWN = 0, 
	VPR_ERROR_ARCH,
	VPR_ERROR_PACK,
	VPR_ERROR_PLACE,
	VPR_ERROR_ROUTE,
	VPR_ERROR_TIMING,
	VPR_ERROR_POWER,
	VPR_ERROR_SDC,
	VPR_ERROR_NET_F,
	VPR_ERROR_PLACE_F,
	VPR_ERROR_BLIF_F,
	VPR_ERROR_IMPL_NETLIST_WRITER,
	VPR_ERROR_OTHER
};
typedef enum e_vpr_error t_vpr_error_type;


//Returns the min of cur and max. 
// If cur > max, a warning is emitted.
int limit_value(int cur, int max, const char *name);



/* This structure is thrown back to highest level of VPR flow if an *
 * internal VPR or user input error occurs. */

class VprError : public VtrError {
    public:
        VprError(t_vpr_error_type err_type,
                 std::string msg="",
                 std::string file="",
                 size_t linenum=-1)
            : VtrError(msg, file, linenum)
            , type_(err_type) {}

        t_vpr_error_type type() const { return type_; }

    private:
        t_vpr_error_type type_;
};

#ifdef __cplusplus 
extern "C" {
#endif

extern char *out_file_prefix; /* Default prefix string for output files */

/******************* Linked list, matrix and vector utilities ****************/

/****************** File and parsing utilities *******************************/

/*********************** Portable random number generators *******************/

typedef void (*vpr_PrintHandlerInfo)( 
		const char* pszMessage, ... );
typedef void (*vpr_PrintHandlerWarning)( 
		const char* pszFileName, unsigned int lineNum,
		const char* pszMessage,	... );
typedef void (*vpr_PrintHandlerError)( 
		const char* pszFileName, unsigned int lineNum,
		const char* pszMessage,	... );
typedef void (*vpr_PrintHandlerDirect)( 
		const char* pszMessage,	... );

extern vpr_PrintHandlerInfo vpr_printf_info;
extern vpr_PrintHandlerWarning vpr_printf_warning;
extern vpr_PrintHandlerError vpr_printf_error;
extern vpr_PrintHandlerDirect vpr_printf_direct;

#ifdef __cplusplus 
}
#endif

/*********************** Error-related ***************************************/
void vpr_throw(enum e_vpr_error type, const char* psz_file_name, unsigned int line_num, const char* psz_message, ...);
void vvpr_throw(enum e_vpr_error type, const char* psz_file_name, unsigned int line_num, const char* psz_message, va_list args);

#endif
