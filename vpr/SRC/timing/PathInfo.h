#ifndef VPR_PATH_INFO_H
#define VPR_PATH_INFO_H

struct PathInfo {
    PathInfo() = default;
    PathInfo(float delay, float slack_val, tatum::NodeId launch_n, tatum::NodeId capture_n, tatum::DomainId launch_d, tatum::DomainId capture_d)
        : path_delay(delay)
        , slack(slack_val)
        , launch_node(launch_n)
        , capture_node(capture_n)
        , launch_domain(launch_d)
        , capture_domain(capture_d) {}

    float path_delay = NAN;
    float slack = NAN;

    //The timing source and sink which launched,
    //and captured the data
    tatum::NodeId launch_node;
    tatum::NodeId capture_node;

    //The launching clock domain
    tatum::DomainId launch_domain;

    //The capture clock domain
    tatum::DomainId capture_domain;
};

#endif
