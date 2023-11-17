#pragma once

constexpr const char* KEY_JOB_ID = "JOB_ID";
constexpr const char* KEY_CMD = "CMD";
constexpr const char* KEY_OPTIONS = "OPTIONS";
constexpr const char* KEY_DATA = "DATA";
constexpr const char* KEY_STATUS = "STATUS";
constexpr const unsigned char TELEGRAM_FRAME_DELIMETER{0x17}; //  0x17 - End of Transmission Block

enum CMD {
    CMD_GET_PATH_LIST_ID=0,
    CMD_DRAW_PATH_ID
};
