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

struct t_conn_cost_params; //Forward declaration

/* To modularize the timing-driven router:
 * The class is named after TimingDriven
 * The upstream functions in timing-driven router should be adapted to use this 
 */

class TimingDrivenRouterLookahead {
  public:
    virtual float get_expected_cost(RRNodeId node, RRNodeId target_node, 
                                    const t_conn_cost_params& params, float R_upstream) const = 0;

    virtual ~TimingDrivenRouterLookahead() {}
};


std::unique_ptr<TimingDrivenRouterLookahead> make_timing_driven_router_lookahead(e_router_lookahead router_lookahead_type);


class TimingDrivenClassicLookahead : public TimingDrivenRouterLookahead {
  public:
    float get_expected_cost(RRNodeId node_id, RRNodeId target_node_id, 
                            const t_conn_cost_params& params, float R_upstream) const override;
  private:
    float classic_wire_lookahead_cost(RRNodeId node_id, RRNodeId target_node_id, 
                                      float criticality, float R_upstream) const;
};

class TimingDrivenMapLookahead : public TimingDrivenRouterLookahead {
  protected:
    float get_expected_cost(RRNodeId node_id, RRNodeId target_node_id, 
                            const t_conn_cost_params& params, float R_upstream) const override;
};

class TimingDrivenNoOpLookahead : public TimingDrivenRouterLookahead {
  protected:
    float get_expected_cost(RRNodeId node_id, RRNodeId target_node_id, 
                            const t_conn_cost_params& params, float R_upstream) const override;
};

#endif
