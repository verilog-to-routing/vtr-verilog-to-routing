#ifndef COMMCONSTS_H
#define COMMCONSTS_H

#include <string>

namespace comm {

extern const std::string KEY_JOB_ID;
extern const std::string KEY_CMD;
extern const std::string KEY_OPTIONS;
extern const std::string KEY_DATA;
extern const std::string KEY_STATUS;
extern const std::string ECHO_DATA;

const unsigned char ZLIB_COMPRESSOR_ID = 'z';
const unsigned char NONE_COMPRESSOR_ID = '\x0';

extern const std::string OPTION_PATH_NUM;
extern const std::string OPTION_PATH_TYPE;
extern const std::string OPTION_DETAILS_LEVEL;
extern const std::string OPTION_IS_FLAT_ROUTING;
extern const std::string OPTION_PATH_ELEMENTS;
extern const std::string OPTION_HIGHLIGHT_MODE;
extern const std::string OPTION_DRAW_PATH_CONTOUR;

extern const std::string KEY_SETUP_PATH_LIST;
extern const std::string KEY_HOLD_PATH_LIST;

enum CMD {
    CMD_GET_PATH_LIST_ID=0,
    CMD_DRAW_PATH_ID
};

} // namespace comm

#endif /* COMMCONSTS_H */
