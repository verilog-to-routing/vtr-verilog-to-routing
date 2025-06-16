#pragma once

#ifndef NO_SERVER

namespace comm {

enum class CMD : int {
    NONE = -1,
    GET_PATH_LIST_ID = 0,
    DRAW_PATH_ID = 1
};

} // namespace comm

#endif /* NO_SERVER */
