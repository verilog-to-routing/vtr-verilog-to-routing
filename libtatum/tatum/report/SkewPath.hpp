#ifndef TATUM_SKEW_PATH_HPP
#define TATUM_SKEW_PATH_HPP

namespace tatum {

struct SkewPath {
    DomainId launch_domain;
    DomainId capture_domain;

    TimingSubPath clock_launch_path;
    TimingSubPath clock_capture_path;

    NodeId data_launch_node;
    NodeId data_capture_node;

    Time clock_launch_arrival;
    Time clock_capture_arrival;
    Time clock_constraint;
    Time clock_skew;
};

}
#endif
