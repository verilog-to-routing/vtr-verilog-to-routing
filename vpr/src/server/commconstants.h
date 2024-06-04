#ifndef COMMCONSTS_H
#define COMMCONSTS_H

#ifndef NO_SERVER

#include <string>

namespace comm {

inline const std::string KEY_JOB_ID{"JOB_ID"};
inline const std::string KEY_CMD{"CMD"};
inline const std::string KEY_OPTIONS{"OPTIONS"};
inline const std::string KEY_DATA{"DATA"};
inline const std::string KEY_STATUS{"STATUS"};
inline const std::string ECHO_DATA{"ECHO"};

const unsigned char ZLIB_COMPRESSOR_ID = 'z';
const unsigned char NONE_COMPRESSOR_ID = '\x0';

inline const std::string OPTION_PATH_NUM{"path_num"};
inline const std::string OPTION_PATH_TYPE{"path_type"};
inline const std::string OPTION_DETAILS_LEVEL{"details_level"};
inline const std::string OPTION_IS_FLAT_ROUTING{"is_flat_routing"};
inline const std::string OPTION_PATH_ELEMENTS{"path_elements"};
inline const std::string OPTION_HIGHLIGHT_MODE{"high_light_mode"};
inline const std::string OPTION_DRAW_PATH_CONTOUR{"draw_path_contour"};

inline const std::string KEY_SETUP_PATH_LIST{"setup"};
inline const std::string KEY_HOLD_PATH_LIST{"hold"};

enum CMD {
    CMD_GET_PATH_LIST_ID=0,
    CMD_DRAW_PATH_ID
};

} // namespace comm

#endif /* NO_SERVER */

#endif /* COMMCONSTS_H */
