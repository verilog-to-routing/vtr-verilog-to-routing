/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */
#ifndef ROUTER_LOOKAHEAD_H
#define ROUTER_LOOKAHEAD_H

/*
 * Notes in include header files in a head file 
 * Only include the neccessary header files 
 * that is required by the data types in the function/class declarations!
 */
#include <memory>
#include "vpr_types.h"

#include "rr_graph_fwd.h"

/* TODO: the cost params may be moved to the namespace later */
struct t_conn_cost_params; //Forward declaration


/* Categorize all the functions in the specific name space of this router */
namespace router {


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

}// end namespace timing_driven_router

#endif
