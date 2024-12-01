
#pragma once

#include "vtr_vec_id_set.h"
#include "timing_info_fwd.h"
#include "clustered_netlist_utils.h"
#include "place_delay_model.h"
#include "vpr_net_pins_matrix.h"

/**
 * @brief Saves the placement criticality parameters
 *
 * crit_exponent: The criticality exponent used to sharpen the criticalities
 * crit_limit:    The limit to consider a pin as timing critical
 */
struct PlaceCritParams {
    float crit_exponent;
    float crit_limit;
};

/**
 * @brief PlacerCriticalities returns the clustered netlist connection criticalities
 *        used by the placer ('sharpened' by a criticality exponent).
 *
 * Usage
 * =====
 * This class also serves to map atom netlist level criticalites (i.e. on AtomPinIds)
 * to the clustered netlist (i.e. ClusterPinIds) used during placement.
 *
 * Criticalities are updated by update_criticalities(), given that `update_enabled` is
 * set to true. It will update criticalities based on the atom netlist connection
 * criticalities provided by the passed in SetupTimingInfo.
 *
 * This process can be done incrementally, based on the modified connections/AtomPinIds
 * returned by SetupTimingInfo. However, the set returned only reflects the connections
 * changed by the last call to the timing info update.
 *
 * Therefore, if SetupTimingInfo is updated twice in succession without criticalities
 * getting updated (update_enabled = false), the returned set cannot account for all
 * the connections that have been modified. In this case, we flag `recompute_required`
 * as false, and we recompute the criticalities for every connection to ensure that
 * they are all up to date. Hence, each time update_setup_slacks_and_criticalities()
 * is called, we assign `recompute_required` the opposite value of `update_enabled`.
 *
 * This class also maps/transforms the modified atom connections/pins returned by the
 * timing info into modified clustered netlist connections/pins after calling
 * update_criticalities(). The interface then enables users to iterate over this range
 * via pins_with_modified_criticalities(). This is useful for incrementally re-calculating
 * the timing costs.
 *
 * The criticalities of individual connections can then be queried by calling the
 * criticality() member function.
 *
 * Implementation
 * ==============
 * To support incremental re-calculation, the class saves the last criticality exponent
 * passed to PlacerCriticalities::update_criticalites(). If the next update uses the same
 * exponent, criticalities can be incrementally updated. Otherwise, they must be re-calculated
 * from scratch, since a change in exponent changes *all* criticalities.
 */
class PlacerCriticalities {
  public: //Types
    typedef vtr::vec_id_set<ClusterPinId>::iterator pin_iterator;
    typedef vtr::vec_id_set<ClusterNetId>::iterator net_iterator;

    typedef vtr::Range<pin_iterator> pin_range;
    typedef vtr::Range<net_iterator> net_range;

  public: //Lifetime
    PlacerCriticalities(const ClusteredNetlist& clb_nlist,
                        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                        std::shared_ptr<const SetupTimingInfo> timing_info);
    PlacerCriticalities(const PlacerCriticalities&) = delete;
    PlacerCriticalities& operator=(const PlacerCriticalities&) = delete;

  public: //Accessors
    ///@brief Returns the criticality of the specified connection.
    float criticality(ClusterNetId net, int ipin) const { return timing_place_crit_[net][ipin]; }

    /**
     * @brief Returns the range of clustered netlist pins (i.e. ClusterPinIds) which
     *        were modified by the last call to PlacerCriticalities::update_criticalities().
     */
    pin_range pins_with_modified_criticality() const;

  public: //Modifiers
    /**
     * @brief Updates criticalities based on the atom netlist criticalitites
     *        provided by timing_info and the provided criticality_exponent.
     *
     * Should consistently call this method after the most recent timing analysis to
     * keep the criticalities stored in this class in sync with the timing analyzer.
     * If out of sync, then the criticalities cannot be incrementally updated on
     * during the next timing analysis iteration.
     */
    void update_criticalities(const PlaceCritParams& crit_params,
                              PlacerState& placer_state);

    ///@bried Enable the recompute_required flag to enforce from scratch update.
    void set_recompute_required();

    ///@brief From scratch update. See timing_place.cpp for more.
    void recompute_criticalities();

    ///@brief Override the criticality of a particular connection.
    void set_criticality(ClusterNetId net, int ipin, float crit_val);

    ///@brief Set `update_enabled` to true.
    void enable_update() { update_enabled = true; }

    ///@brief Set `update_enabled` to true.
    void disable_update() { update_enabled = false; }

  private: //Data
    ///@brief The clb netlist in the placement context.
    const ClusteredNetlist& clb_nlist_;

    ///@brief The lookup table that maps atom pins to clb pins.
    const ClusteredPinAtomPinsLookup& pin_lookup_;

    ///@brief A pointer to the setup timing analyzer
    std::shared_ptr<const SetupTimingInfo> timing_info_;

    /**
     * @brief The matrix that stores criticality value for each connection.
     *
     * Index range: [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]
     */
    ClbNetPinsMatrix<float> timing_place_crit_;

    /**
     * The criticality exponent when update_criticalites() was last called
     * (used to detect if incremental update can be used).
     */
    float last_crit_exponent_ = std::numeric_limits<float>::quiet_NaN();

    ///@brief Set of pins with criticaltites modified by last call to update_criticalities().
    vtr::vec_id_set<ClusterPinId> cluster_pins_with_modified_criticality_;

    ///@brief Incremental update. See timing_place.cpp for more.
    void incr_update_criticalities();

    ///@brief Flag that turns on/off the update_criticalities() routine.
    bool update_enabled = true;

    /**
     * @brief Flag that checks if criticalities need to be recomputed for all connections.
     *
     * Used by the method update_criticalities(). They incremental update is not possible
     * if this method wasn't called updated after the previous timing info update.
     */
    bool recompute_required = true;

    /**
     * @brief if this is first time to call update_criticality
     *
     * This can be used for incremental criticality update and also incrementally update the highly critical pins
     */
    bool first_time_update_criticality = true;
};
