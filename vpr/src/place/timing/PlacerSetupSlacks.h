
#pragma once

#include "vtr_vec_id_set.h"
#include "timing_info_fwd.h"
#include "clustered_netlist_utils.h"
#include "place_delay_model.h"
#include "vpr_net_pins_matrix.h"

/**
 * @brief PlacerSetupSlacks returns the RAW setup slacks of clustered netlist connection.
 *
 * Usage
 * =====
 * This class mirrors PlacerCriticalities by both its methods and its members. The only
 * difference is that this class deals with RAW setup slacks returned by SetupTimingInfo
 * rather than criticalities. See the documentation on PlacerCriticalities for more.
 *
 * RAW setup slacks are unlike criticalities. Their values are not confined between
 * 0 and 1. Their values can be either positive or negative.
 *
 * This class also provides iterating over the clustered netlist connections/pins that
 * have modified setup slacks by the last call to update_setup_slacks(). However, this
 * utility is mainly used for incrementally committing the setup slack values into the
 * structure `connection_setup_slack` used by many placer routines.
 */
class PlacerSetupSlacks {
  public: //Types
    typedef vtr::vec_id_set<ClusterPinId>::iterator pin_iterator;
    typedef vtr::vec_id_set<ClusterNetId>::iterator net_iterator;

    typedef vtr::Range<pin_iterator> pin_range;
    typedef vtr::Range<net_iterator> net_range;

  public: //Lifetime
    ///@brief Allocates space for the timing_place_setup_slacks_ data structure.
    PlacerSetupSlacks(const ClusteredNetlist& clb_nlist,
                      const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                      std::shared_ptr<const SetupTimingInfo> timing_info);

    PlacerSetupSlacks(const PlacerSetupSlacks& clb_nlist) = delete;
    PlacerSetupSlacks& operator=(const PlacerSetupSlacks& clb_nlist) = delete;

  public: //Accessors
    ///@brief Returns the setup slack of the specified connection.
    float setup_slack(ClusterNetId net, int ipin) const { return timing_place_setup_slacks_[net][ipin]; }

    /**
     * @brief Returns the range of clustered netlist pins (i.e. ClusterPinIds)
     *        which were modified by the last call to PlacerSetupSlacks::update_setup_slacks().
     */
    pin_range pins_with_modified_setup_slack() const;

  public: //Modifiers
    /**
     * @brief Updates setup slacks based on the atom netlist setup slacks provided
     *        by timing_info_.
     *
     *  @note This function updates the setup slacks in the timing_place_setup_slacks_
     *  data structure.
     *
     * Should consistently call this method after the most recent timing analysis to
     * keep the setup slacks stored in this class in sync with the timing analyzer.
     * If out of sync, then the setup slacks cannot be incrementally updated during
     * the next timing analysis iteration.
     *
     * If the setup slacks are not updated immediately after each time we cal
     * timing_info->update(), then timing_info->pins_with_modified_setup_slack()
     * cannot accurately account for all the pins that need to be updated.
     * In this case, `recompute_required` would be true, and we update all setup slacks
     * from scratch.
     */
    void update_setup_slacks();

    ///@bried Enable the recompute_required flag to enforce from scratch update.
    void set_recompute_required() { recompute_required = true; }

    ///@brief Override the setup slack of a particular connection.
    void set_setup_slack(ClusterNetId net, int ipin, float slack_val);

    ///@brief Set `update_enabled` to true.
    void enable_update() { update_enabled = true; }

    ///@brief Set `update_enabled` to true.
    void disable_update() { update_enabled = false; }

  private: //Data
    const ClusteredNetlist& clb_nlist_;
    const ClusteredPinAtomPinsLookup& pin_lookup_;
    std::shared_ptr<const SetupTimingInfo> timing_info_;

    /**
     * @brief The matrix that stores raw setup slack values for each connection.
     *
     * Index range: [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]
     */
    ClbNetPinsMatrix<float> timing_place_setup_slacks_;

    ///@brief Set of pins with raw setup slacks modified by last call to update_setup_slacks()
    vtr::vec_id_set<ClusterPinId> cluster_pins_with_modified_setup_slack_;

    /**
     * @brief Collect the cluster pins which need to be updated based on the latest timing
     *        analysis so that incremental updates to setup slacks can be performed.
     *
     * Note we use the set of pins reported by the *timing_info* as having modified
     * setup slacks, rather than those marked as modified by the timing analyzer.
     */
    void incr_update_setup_slacks();

    /**
     * @brief Collect all the sink pins in the netlist and prepare them update.
     *
     * For the incremental version, see PlacerSetupSlacks::incr_update_setup_slacks().
     */
    void recompute_setup_slacks();

    ///@brief Flag that turns on/off the update_setup_slacks() routine.
    bool update_enabled = true;

    /**
     * @brief Flag that checks if setup slacks need to be recomputed for all connections.
     *
     * Used by the method update_setup_slacks(). They incremental update is not possible
     * if this method wasn't called updated after the previous timing info update.
     */
    bool recompute_required = true;
};

