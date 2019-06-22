#ifndef VPR_ROUTER_LOOKAHEAD_H
#define VPR_ROUTER_LOOKAHEAD_H
#include <memory>
#include "vpr_types.h"

struct t_conn_cost_params; //Forward declaration

class RouterLookahead {
  public:
    virtual float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const = 0;

    virtual ~RouterLookahead() {}
};

std::unique_ptr<RouterLookahead> make_router_lookahead(e_router_lookahead router_lookahead_type);

class ClassicLookahead : public RouterLookahead {
  public:
    float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const override;

  private:
    float classic_wire_lookahead_cost(int node, int target_node, float criticality, float R_upstream) const;
};

class MapLookahead : public RouterLookahead {
  protected:
    float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const override;
};

class NoOpLookahead : public RouterLookahead {
  protected:
    float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const override;
};

#endif
