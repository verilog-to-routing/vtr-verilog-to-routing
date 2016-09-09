#ifndef VPR_ERROR_H
#define VPR_ERROR_H

#include "vtr_error.h"

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

/* This structure is thrown back to highest level of VPR flow if an *
 * internal VPR or user input error occurs. */

class VprError : public vtr::VtrError {
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

void vpr_throw(enum e_vpr_error type, const char* psz_file_name, unsigned int line_num, const char* psz_message, ...);
void vvpr_throw(enum e_vpr_error type, const char* psz_file_name, unsigned int line_num, const char* psz_message, va_list args);

#endif
