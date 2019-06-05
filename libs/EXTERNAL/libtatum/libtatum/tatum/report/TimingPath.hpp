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
                       EdgeId incomming_edge_v) noexcept
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

//One sub-path of a timing path (e.g. clock launch path, data path, clock capture path)
class TimingSubPath {
    public:
        typedef std::vector<TimingPathElem>::const_iterator path_elem_iterator;
        typedef util::Range<path_elem_iterator> path_elem_range;

    public:
        TimingSubPath() = default;
        TimingSubPath(std::vector<TimingPathElem> elems)
            : elements_(elems) {}

        path_elem_range elements() const { 
            return util::make_range(elements_.begin(),
                                    elements_.end());
        }

    private:
        std::vector<TimingPathElem> elements_;
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
                   TimingSubPath clock_launch,
                   TimingSubPath data_arrival,
                   TimingSubPath clock_capture,
                   const TimingPathElem& data_required_elem,
                   const TimingTag& slack)
            : path_info_(info)
            , clock_launch_path_(clock_launch)
            , data_arrival_path_(data_arrival)
            , clock_capture_path_(clock_capture)
            , data_required_element_(data_required_elem)
            , slack_tag_(slack) {
            //pass
        }

    public:
        const TimingPathInfo& path_info() const { return path_info_; }

        const TimingSubPath& clock_launch_path() const {
            return clock_launch_path_;
        }

        const TimingSubPath& data_arrival_path() const {
            return data_arrival_path_;
        }

        const TimingSubPath& clock_capture_path() const {
            return clock_capture_path_;
        }

        TimingPathElem data_required_element() const {
            return data_required_element_;
        }

        const TimingTag& slack_tag() const { return slack_tag_; }

    private:
        TimingPathInfo path_info_;

        TimingSubPath clock_launch_path_;
        TimingSubPath data_arrival_path_;
        TimingSubPath clock_capture_path_;
        TimingPathElem data_required_element_;
        TimingTag slack_tag_;
};

} //namespace

#endif
