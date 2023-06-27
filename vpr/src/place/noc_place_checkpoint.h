#ifndef VTR_ROUTERPLACEMENTCHECKPOINT_H
#define VTR_ROUTERPLACEMENTCHECKPOINT_H

#include "vpr_types.h"
#include "place_util.h"

class RouterPlacementCheckpoint {
  private:
    std::unordered_map<ClusterBlockId, t_pl_loc> router_locations_;
    bool valid_ = false;
    double cost_;

  public:
    RouterPlacementCheckpoint();
    RouterPlacementCheckpoint(const RouterPlacementCheckpoint& other) = delete;
    RouterPlacementCheckpoint& operator=(const RouterPlacementCheckpoint& other) = delete;

    void save_checkpoint(double cost);
    void restore_checkpoint(const t_noc_opts& noc_opts,  t_placer_costs& costs);
    bool is_valid() const;
    double get_cost() const;
};

#endif //VTR_ROUTERPLACEMENTCHECKPOINT_H
