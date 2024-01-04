#ifndef SERVERCONSTS_H
#define SERVERCONSTS_H

namespace server {

constexpr const char* KEY_JOB_ID = "JOB_ID";
constexpr const char* KEY_CMD = "CMD";
constexpr const char* KEY_OPTIONS = "OPTIONS";
constexpr const char* KEY_DATA = "DATA";
constexpr const char* KEY_STATUS = "STATUS";
constexpr const unsigned char TELEGRAM_FRAME_DELIMETER{0x17}; //  0x17 - End of Transmission Block

constexpr const char* OPTION_PATH_NUM = "path_num";
constexpr const char* OPTION_PATH_TYPE = "path_type";
constexpr const char* OPTION_DETAILS_LEVEL = "details_level";
constexpr const char* OPTION_IS_FLOAT_ROUTING = "is_flat_routing";
constexpr const char* OPTION_PATH_INDEX = "path_index";
constexpr const char* OPTION_HIGHTLIGHT_MODE = "hight_light_mode";

enum CMD {
    CMD_GET_PATH_LIST_ID=0,
    CMD_DRAW_PATH_ID
};

} // namespace server

#endif 
