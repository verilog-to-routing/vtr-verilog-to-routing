#ifndef COMMCONSTS_H
#define COMMCONSTS_H

namespace comm {

constexpr const char* KEY_JOB_ID = "JOB_ID";
constexpr const char* KEY_CMD = "CMD";
constexpr const char* KEY_OPTIONS = "OPTIONS";
constexpr const char* KEY_DATA = "DATA";
constexpr const char* KEY_STATUS = "STATUS";
constexpr const char* ECHO_DATA = "ECHO";

const unsigned char ZLIB_COMPRESSOR_ID = 'z';
const unsigned char NONE_COMPRESSOR_ID = '\x0';

constexpr const char* OPTION_PATH_NUM = "path_num";
constexpr const char* OPTION_PATH_TYPE = "path_type";
constexpr const char* OPTION_DETAILS_LEVEL = "details_level";
constexpr const char* OPTION_IS_FLAT_ROUTING = "is_flat_routing";
constexpr const char* OPTION_PATH_ELEMENTS = "path_elements";
constexpr const char* OPTION_HIGHTLIGHT_MODE = "hight_light_mode";
constexpr const char* OPTION_DRAW_PATH_CONTOUR = "draw_path_contour";

constexpr const char* CRITICAL_PATH_ITEMS_SELECTION_NONE = "none";

// please don't change values as they are involved in socket communication
constexpr const char* KEY_SETUP_PATH_LIST = "setup";
constexpr const char* KEY_HOLD_PATH_LIST = "hold";
//

enum CMD {
    CMD_GET_PATH_LIST_ID=0,
    CMD_DRAW_PATH_ID
};

} // namespace comm

#endif 
