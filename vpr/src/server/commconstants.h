#ifndef COMMCONSTS_H
#define COMMCONSTS_H

#include <string_view>

namespace comm {

constexpr const std::string_view KEY_JOB_ID = "JOB_ID";
constexpr const std::string_view KEY_CMD = "CMD";
constexpr const std::string_view KEY_OPTIONS = "OPTIONS";
constexpr const std::string_view KEY_DATA = "DATA";
constexpr const std::string_view KEY_STATUS = "STATUS";
constexpr const std::string_view ECHO_DATA{"ECHO"};

const unsigned char ZLIB_COMPRESSOR_ID = 'z';
const unsigned char NONE_COMPRESSOR_ID = '\x0';

constexpr const std::string_view OPTION_PATH_NUM = "path_num";
constexpr const std::string_view OPTION_PATH_TYPE = "path_type";
constexpr const std::string_view OPTION_DETAILS_LEVEL = "details_level";
constexpr const std::string_view OPTION_IS_FLAT_ROUTING = "is_flat_routing";
constexpr const std::string_view OPTION_PATH_ELEMENTS = "path_elements";
constexpr const std::string_view OPTION_HIGHLIGHT_MODE = "high_light_mode";
constexpr const std::string_view OPTION_DRAW_PATH_CONTOUR = "draw_path_contour";

// please don't change values as they are involved in socket communication
constexpr const std::string_view KEY_SETUP_PATH_LIST = "setup";
constexpr const std::string_view KEY_HOLD_PATH_LIST = "hold";
//

enum CMD {
    CMD_GET_PATH_LIST_ID=0,
    CMD_DRAW_PATH_ID
};

} // namespace comm

#endif /* COMMCONSTS_H */
