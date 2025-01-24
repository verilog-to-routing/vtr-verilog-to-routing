
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
 * changed by the last call to the timing info update (update_setup() method of SetupTimingInfo).
 *
 * Therefore, if SetupTimingInfo is updated twice in a row without criticalities
 * getting updated after the first update of SetupTimingInfo (PlacerCriticalities::update_enabled = false),
 * the returned set of modified connections/AtomPinIds by SetupTimingInfo after its second update does not
 * account for all the connections that have been modified.
 * To address this issue, whenever update_criticalities() is called with flag update_enabled = false,
 * we don't update criticalities and set flag recompute_required to true to remember that criticalities
 * need to be recomputed from scratch in the first call to update_criticalities() with update_enabled = true.
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
 *
 * Calculating criticalities:
 * All the raw setup slack values across a single clock domain are gathered
 * and rated from the best to the worst in terms of criticalities. In order
 * to calculate criticalities, all the slack values need to be non-negative.
 * Hence, if the worst slack is negative, all the slack values are shifted
 * by the value of the worst slack so that the value is at least 0. If the
 * worst slack is positive, then no shift happens.
 *
 * The best (shifted) slack (the most positive one) will have a criticality of 0.
 * The worst (shifted) slack value will have a criticality of 1.
 *
 * Criticalities are used to calculated timing costs for each connection.
 * The formula is cost = delay * criticality.
 *
 * For a more detailed description on how criticalities are calculated, see
 * calc_relaxed_criticality() in `timing_util.cpp`.
 */
class PlacerCriticalities {
  public: //Types
    typedef vtr::vec_id_set<ClusterPinId>::iterator pin_iterator;
    typedef vtr::vec_id_set<ClusterNetId>::iterator net_iterator;

    typedef vtr::Range<pin_iterator> pin_range;
    typedef vtr::Range<net_iterator> net_range;

  public: //Lifetime

    /**
     * @brief Allocates space for the timing_place_crit_ data structure.
     * @param clb_nlist Used to lookup and iterate clustered netlist connections.
     * @param netlist_pin_lookup Used to lookup Atom/Clustered pins connected to a Clustered/Atom pin.
     * @param timing_info Holds setup timing info.
     *
     * @note timing_info may be shared by multiple objects with different lifetimes.
     * To ensure timing_info is destroyed only after all its user object are destructed,
     * each user object should hold a shared_ptr to it.
     */
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

    /// @brief Returns a constant reference to highly critical pins
    const std::vector<std::pair<ClusterNetId, int>>& get_highly_critical_pins() const { return highly_crit_pins; }

  public: //Modifiers
    /**
     * @brief Updates criticalities based on the atom netlist criticalities
     *        provided by timing_info and the provided criticality_exponent.
     *
     * Should consistently call this method after the most recent timing analysis to
     * keep the criticalities stored in this class in sync with the timing analyzer.
     * If out of sync, then the criticalities cannot be incrementally updated on
     * during the next timing analysis iteration.
     *
     * If the criticalities are not updated immediately after each time we call
     * timing_info->update(), then timing_info->pins_with_modified_setup_criticality()
     * cannot accurately account for all the pins that need to be updated. In this case,
     * `recompute_required` would be true, and we update all criticalities from scratch.
     * If the criticality exponent has changed, we also need to update from scratch.
     */
    void update_criticalities(const PlaceCritParams& crit_params);

    ///@bried Enable the recompute_required flag to enforce from scratch update.
    void set_recompute_required();

    /**
     * @brief Collect all the sink pins in the netlist and prepare them update.
     *
     * For the incremental version, see PlacerCriticalities::incr_update_criticalities().
     */
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

    ///@brief Set of pins with criticalities modified by last call to update_criticalities().
    vtr::vec_id_set<ClusterPinId> cluster_pins_with_modified_criticality_;

    /**
     * @brief Collect the cluster pins which need to be updated based on the latest timing
     *        analysis so that incremental updates to criticalities can be performed.
     *
     * Note we use the set of pins reported by the *timing_info* as having modified
     * criticality, rather than those marked as modified by the timing analyzer.
     *
     * Since timing_info uses shifted/relaxed criticality (which depends on max required
     * time and worst case slacks), additional nodes may be modified when updating the
     * atom pin criticalities.
     */
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

    /// @brief Saves the highly critical pins (higher than a timing criticality limit set by commandline option)
    std::vector<std::pair<ClusterNetId, int>> highly_crit_pins;
};
