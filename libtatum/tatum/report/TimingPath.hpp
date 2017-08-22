#ifndef TATUM_TIMING_PATH_HPP
#define TATUM_TIMING_PATH_HPP
#include <vector>

#include "tatum/report/TimingPathFwd.hpp"
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/base/TimingType.hpp"
#include "tatum/Time.hpp"
#include "tatum/tags/TimingTag.hpp"

#include "tatum/util/tatum_assert.hpp"
#include "tatum/util/tatum_range.hpp"

namespace tatum {


//High level information about a timing path
class TimingPathInfo {
    public:
        TimingPathInfo() = default;
        TimingPathInfo(TimingType path_type, Time path_delay, Time path_slack, NodeId launch_n, NodeId capture_n, DomainId launch_d, DomainId capture_d)
            : path_type_(path_type)
            , delay_(path_delay)
            , slack_(path_slack)
            , startpoint_(launch_n)
            , endpoint_(capture_n)
            , launch_domain_(launch_d)
            , capture_domain_(capture_d) {}

        TimingType type() { return path_type_; }

        Time delay() const { return delay_; }
        Time slack() const { return slack_; }

        NodeId startpoint() const { return startpoint_; } //Note may be NodeId::INVALID() for functions which don't fully trace the timing path
        NodeId endpoint() const { return endpoint_; }

        DomainId launch_domain() const { return launch_domain_; }
        DomainId capture_domain() const { return capture_domain_; }

    private:
        TimingType path_type_ = TimingType::UNKOWN;

        Time delay_;
        Time slack_;

        //The timing source and sink which launched,
        //and captured the data
        NodeId startpoint_;
        NodeId endpoint_;

        //The clock domains
        DomainId launch_domain_;
        DomainId capture_domain_;
};

//A component/point along a timing path
class TimingPathElem {
    public:
        TimingPathElem() = default;
        TimingPathElem(TimingTag tag_v,
                       NodeId node_v,
                       EdgeId incomming_edge_v)
            : tag_(tag_v)
            , node_(node_v)
            , incomming_edge_(incomming_edge_v) {
            //pass
        }
    public: //Accessors
        const TimingTag& tag() const { return tag_; }
        NodeId node() const { return node_; }
        EdgeId incomming_edge() const { return incomming_edge_; }

    public: //Mutators
        void set_incomming_edge(EdgeId edge) { incomming_edge_ = edge; }
    private:
        TimingTag tag_;
        NodeId node_;
        EdgeId incomming_edge_;
};


//A collection of timing path elements which form
//a timing path.
class TimingPath {
    public: 
        typedef std::vector<TimingPathElem>::const_iterator path_elem_iterator;

        typedef util::Range<path_elem_iterator> path_elem_range;
    public:
        TimingPath() = default;
        TimingPath(const TimingPathInfo& info,
                   std::vector<TimingPathElem> clock_launch_elems,
                   std::vector<TimingPathElem> data_arrival_elems,
                   std::vector<TimingPathElem> clock_capture_elems,
                   const TimingPathElem& data_required_elem,
                   const TimingTag& slack)
            : path_info_(info)
            , clock_launch_elements_(clock_launch_elems)
            , data_arrival_elements_(data_arrival_elems)
            , clock_capture_elements_(clock_capture_elems)
            , data_required_element_(data_required_elem)
            , slack_tag_(slack) {
            //pass
        }

    public:
        const TimingPathInfo& path_info() const { return path_info_; }

        path_elem_range clock_launch_elements() const {
            return util::make_range(clock_launch_elements_.begin(),
                                    clock_launch_elements_.end());
        }

        path_elem_range data_arrival_elements() const {
            return util::make_range(data_arrival_elements_.begin(),
                                    data_arrival_elements_.end());
        }

        path_elem_range clock_capture_elements() const {
            return util::make_range(clock_capture_elements_.begin(),
                                    clock_capture_elements_.end());
        }

        TimingPathElem data_required_element() const {
            return data_required_element_;
        }

        const TimingTag& slack_tag() const { return slack_tag_; }

    private:
        TimingPathInfo path_info_;

        std::vector<TimingPathElem> clock_launch_elements_;
        std::vector<TimingPathElem> data_arrival_elements_;
        std::vector<TimingPathElem> clock_capture_elements_;
        TimingPathElem data_required_element_;
        TimingTag slack_tag_;
};

} //namespace

#endif
