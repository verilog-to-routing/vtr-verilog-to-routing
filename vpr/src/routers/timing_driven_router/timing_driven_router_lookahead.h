/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef TIMING_DRIVEN_ROUTER_LOOKAHEAD_H
#define TIMING_DRIVEN_ROUTER_LOOKAHEAD_H

/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
#include <memory>
#include "vpr_types.h"

#include "rr_graph_fwd.h"


/* Namespace declaration */
/* All the routers should be placed in the namespace of router
 * A specific router may have it own namespace under the router namespace
 * To call a function in a router, function need a prefix of router::<your_router_namespace>::
 * This will ease the development in modularization.
 * The concern is to minimize/avoid conflicts between data type names as much as possible
 * Also, this will keep function names as short as possible.
 */
/* Categorize all the functions in the specific name space of this router */
namespace router {

namespace timing_driven {

  /* TODO: the cost params may be moved to the namespace later */
  struct t_conn_cost_params; //Forward declaration


/* To modularize the timing-driven router:
 * The class is named after TimingDriven
 * The upstream functions in timing-driven router should be adapted to use this 
 */

class RouterLookahead {
  public:
    virtual float get_expected_cost(RRNodeId node, RRNodeId target_node, 
                                    const t_conn_cost_params& params, float R_upstream) const = 0;

    virtual ~RouterLookahead() {}
};


std::unique_ptr<RouterLookahead> make_router_lookahead(e_router_lookahead router_lookahead_type);


class ClassicLookahead : public RouterLookahead {
  public:
    float get_expected_cost(RRNodeId node_id, RRNodeId target_node_id, 
                            const t_conn_cost_params& params, float R_upstream) const override;
  private:
    float classic_wire_lookahead_cost(RRNodeId node_id, RRNodeId target_node_id, 
                                      float criticality, float R_upstream) const;
};

class MapLookahead : public RouterLookahead {
  protected:
    float get_expected_cost(RRNodeId node_id, RRNodeId target_node_id, 
                            const t_conn_cost_params& params, float R_upstream) const override;
};

class NoOpLookahead : public RouterLookahead {
  protected:
    float get_expected_cost(RRNodeId node_id, RRNodeId target_node_id, 
                            const t_conn_cost_params& params, float R_upstream) const override;
};

}// end namespace timing_driven

}// end namespace router

#endif
