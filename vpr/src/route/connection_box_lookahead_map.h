#ifndef CONNECTION_BOX_LOOKAHEAD_H_
#define CONNECTION_BOX_LOOKAHEAD_H_

#include <vector>
#include "physical_types.h"
#include "router_lookahead.h"

class ConnectionBoxMapLookahead : public RouterLookahead {
  public:
    float get_expected_cost(int node, int target_node, const t_conn_cost_params& params, float R_upstream) const override;
    void compute(const std::vector<t_segment_inf>& segment_inf) override;

    void read(const std::string& file) override;
    void write(const std::string& file) const override;
};

#endif
