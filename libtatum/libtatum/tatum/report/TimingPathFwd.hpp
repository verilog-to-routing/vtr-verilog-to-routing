#ifndef TATUM_TIMING_PATH_FWD_HPP
#define TATUM_TIMING_PATH_FWD_HPP

namespace tatum {

//The type of a timing path
enum class TimingPathType {
    SETUP,
    HOLD,
    UNKOWN
};

class TimingPathInfo;
class TimingPathElem;
class TimingPath;

} //namespace

#endif
