#ifndef TIMING_PLACE
#define TIMING_PLACE

#include "vtr_vec_id_set.h"
#include "timing_info_fwd.h"
#include "clustered_netlist_utils.h"
#include "place_delay_model.h"

std::unique_ptr<PlaceDelayModel> alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
                                                                 const t_placer_opts& place_opts,
                                                                 const t_router_opts& router_opts,
                                                                 t_det_routing_arch* det_routing_arch,
                                                                 std::vector<t_segment_inf>& segment_inf,
                                                                 const t_direct_inf* directs,
                                                                 const int num_directs);

class PlacerCriticalities {
    public: //Types
        typedef vtr::vec_id_set<ClusterPinId>::iterator pin_iterator;
        typedef vtr::vec_id_set<ClusterNetId>::iterator net_iterator;

        typedef vtr::Range<pin_iterator> pin_range;
        typedef vtr::Range<net_iterator> net_range;

    public: //Lifetime
        PlacerCriticalities(const ClusteredNetlist& clb_nlist, const ClusteredPinAtomPinsLookup& netlist_pin_lookup);
        ~PlacerCriticalities();
        PlacerCriticalities(const PlacerCriticalities& clb_nlist) = delete;
        PlacerCriticalities& operator=(const PlacerCriticalities& clb_nlist) = delete;

    public: //Accessors
        float criticality(ClusterNetId net, int ipin) const;
        pin_range pins_with_modified_criticality() const;
        net_range nets_with_modified_criticality() const;

    public: //Modifiers
        void update_criticalities(const SetupTimingInfo& timing_info, float crit_exponent_lookup);
        void set_criticality(ClusterNetId net, int ipin, float val);

    private: //Data
        const ClusteredNetlist& clb_nlist_;
        const ClusteredPinAtomPinsLookup& pin_lookup_;
        
        vtr::t_chunk timing_place_crit_ch_;
        vtr::vector<ClusterNetId, float*> timing_place_crit_; /* [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1] */

        //The criticality exponent when update_criticalites() was last called (used to detect if incremental update can be used)
        float last_crit_exponent_ = std::numeric_limits<float>::quiet_NaN();

        //Set of pins with criticaltites modified by last call to update_criticalities()
        vtr::vec_id_set<ClusterPinId> cluster_pins_with_modified_criticality_;
        vtr::vec_id_set<ClusterNetId> cluster_nets_with_modified_criticality_;
};

#endif
