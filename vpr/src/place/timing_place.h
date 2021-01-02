/**
 * @file timing_place.h
 * @brief Interface used by the VPR placer to query information
 *        from the Tatum timing analyzer.
 *
 *   @class PlacerSetupSlacks
 *              Queries connection **RAW** setup slacks, which can
 *              range from negative to positive values. Also maps
 *              atom pin setup slacks to clb pin setup slacks.
 *   @class PlacerCriticalities
 *              Query connection criticalities, which are calculuated
 *              based on the raw setup slacks and ranges from 0 to 1.
 *              Also maps atom pin crit. to clb pin crit.
 *   @class PlacerTimingCosts
 *              Hierarchical structure used by update_td_costs() to
 *              maintain the order of addition operation of float values
 *              (to avoid round-offs) while doing incremental updates.
 *
 * Calculating criticalities:
 *      All the raw setup slack values across a single clock domain are gathered
 *      and rated from the best to the worst in terms of criticalities. In order
 *      to calculate criticalities, all the slack values need to be non-negative.
 *      Hence, if the worst slack is negative, all the slack values are shifted
 *      by the value of the worst slack so that the value is at least 0. If the
 *      worst slack is positive, then no shift happens.
 *
 *      The best (shifted) slack (the most positive one) will have a criticality of 0.
 *      The worst (shifted) slack value will have a criticality of 1.
 *
 *      Criticalities are used to calculated timing costs for each connection.
 *      The formula is cost = delay * criticality.
 *
 *      For a more detailed description on how criticalities are calculated, see
 *      calc_relaxed_criticality() in `timing_util.cpp`.
 */

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
    PlacerCriticalities(const ClusteredNetlist& clb_nlist, const ClusteredPinAtomPinsLookup& netlist_pin_lookup);
    PlacerCriticalities(const PlacerCriticalities& clb_nlist) = delete;
    PlacerCriticalities& operator=(const PlacerCriticalities& clb_nlist) = delete;

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
    void update_criticalities(const SetupTimingInfo* timing_info, const PlaceCritParams& crit_params);

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
    void incr_update_criticalities(const SetupTimingInfo* timing_info);

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
    PlacerSetupSlacks(const ClusteredNetlist& clb_nlist, const ClusteredPinAtomPinsLookup& netlist_pin_lookup);
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
     *        by timing_info.
     *
     * Should consistently call this method after the most recent timing analysis to
     * keep the setup slacks stored in this class in sync with the timing analyzer.
     * If out of sync, then the setup slacks cannot be incrementally updated on
     * during the next timing analysis iteration.
     */
    void update_setup_slacks(const SetupTimingInfo* timing_info);

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

    /**
     * @brief The matrix that stores raw setup slack values for each connection.
     *
     * Index range: [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]
     */
    ClbNetPinsMatrix<float> timing_place_setup_slacks_;

    ///@brief Set of pins with raw setup slacks modified by last call to update_setup_slacks()
    vtr::vec_id_set<ClusterPinId> cluster_pins_with_modified_setup_slack_;

    ///@brief Incremental update. See timing_place.cpp for more.
    void incr_update_setup_slacks(const SetupTimingInfo* timing_info);

    ///@brief Incremental update. See timing_place.cpp for more.
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

/**
 * @brief PlacerTimingCosts mimics a 2D array of connection timing costs running from:
 *        [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1].
 *
 * It can be used similar to:
 *
 *      PlacerTimingCosts connection_timing_costs(cluster_ctx.clb_nlist); //Construct
 *
 *      //...
 *
 *      //Modify a connection cost
 *      connection_timing_costs[net_id][ipin] = new_cost;
 *
 *      //Potentially other modifications...
 *
 *      //Calculate the updated timing cost, of all connections,
 *      //incrementally based on modifications
 *      float total_timing_cost = connection_timing_costs.total_cost();
 *
 * However behind the scenes PlacerTimingCosts tracks when connection costs are modified,
 * and efficiently re-calculates the total timing cost incrementally based on the connections
 * which have had their cost modified.
 *
 * Implementation
 * ==============
 * Internally, PlacerTimingCosts stores all connection costs in a flat array in the last part
 * of connection_costs_.  To mimic 2d-array like access PlacerTimingCosts also uses two proxy
 * classes which allow indexing in the net and pin dimensions (NetProxy and ConnectionProxy
 * respectively).
 *
 * The first part of connection_costs_ stores intermediate sums of the connection costs for
 * efficient incremental re-calculation. More concretely, connection_costs_ stores a binary
 * tree, where leaves correspond to individual connection costs and intermediate nodes the
 * partial sums of the connection costs. (The binary tree is stored implicitly in the
 * connection_costs_  vector, using Eytzinger's/BFS layout.) By summing the entire binary
 * tree we calculate the total timing cost over all connections.
 *
 * Using a binary tree allows us to efficiently re-calculate the timing costs when only a subset
 * of connections are changed. This is done by 'invalidating' intermediate nodes (from leaves up
 * to the root) which have ancestors (leaves) with modified connection costs. When the
 * total_cost() method is called, it recursively walks the binary tree to re-calculate the cost.
 * Only invalidated nodes are traversed, with valid nodes just returning their previously
 * calculated (and unchanged) value.
 *
 * For a circuit with 'K' connections, of which 'k' have changed (typically k << K), this can
 * be done in O(k log K) time.
 *
 * It is important to note that due to limited floating point precision, floating point
 * arithmetic has an order dependence (due to round-off). Using a binary tree to total
 * the timing connection costs allows us to incrementally update the total timing cost while
 * maintianing the *same order of operations* as if it was re-computed from scratch. This
 * ensures we *always* get consistent results regardless of what/when connections are changed.
 *
 * Proxy Classes
 * =============
 * NetProxy is returned by PlacerTimingCost's operator[], and stores a pointer to the start of
 * internal storage of that net's connection costs.
 *
 * ConnectionProxy is returned by NetProxy's operator[], and holds a reference to a particular
 * element of the internal storage pertaining to a specific connection's cost. ConnectionProxy
 * supports assignment, allowing clients to modify the connection cost. It also detects if the
 * assigned value differs from the previous value and if so, calls PlacerTimingCosts's
 * invalidate() method on that connection cost.
 *
 * PlacerTimingCosts's invalidate() method marks the cost element's ancestors as invalid (NaN)
 * so they will be re-calculated by PlacerTimingCosts' total_cost() method.
 */
class PlacerTimingCosts {
  public:
    PlacerTimingCosts() = default;

    PlacerTimingCosts(const ClusteredNetlist& nlist) {
        auto nets = nlist.nets();

        net_start_indicies_.resize(nets.size());

        //Walk through the netlist to determine how many connections there are.
        size_t iconn = 0;
        for (ClusterNetId net : nets) {
            //The placer always skips 'ignored' nets so they don't effect timing
            //costs, so we also skip them here
            if (nlist.net_is_ignored(net)) {
                net_start_indicies_[net] = OPEN;
                continue;
            }

            //Save the startind index of the current net's connections.
            // We use a -1 offset, since sinks indexed from [1..num_net_pins-1]
            // (there is no timing cost associated with net drivers)
            net_start_indicies_[net] = iconn - 1;

            //Reserve space for all this net's connections
            iconn += nlist.net_sinks(net).size();
        }

        size_t num_connections = iconn;

        //Determine how many binary tree levels we need to have a leaf
        //for each connection cost
        size_t ilevel = 0;
        while (num_nodes_in_level(ilevel) < num_connections) {
            ++ilevel;
        }
        num_levels_ = ilevel + 1;

        size_t num_leaves = num_nodes_in_level(ilevel);
        size_t num_level_before_leaves = num_nodes_in_level(ilevel - 1);

        VTR_ASSERT_MSG(num_leaves >= num_connections, "Need at least as many leaves as connections");
        VTR_ASSERT_MSG(
            num_connections == 0 || num_level_before_leaves < num_connections,
            "Level before should have fewer nodes than connections (to ensure using the smallest binary tree)");

        //We don't need to store all possible leaves if we have fewer connections
        //(i.e. bottom-right of tree is empty)
        size_t last_level_unused_nodes = num_nodes_in_level(ilevel) - num_connections;
        size_t num_nodes = num_nodes_up_to_level(ilevel) - last_level_unused_nodes;

        //Reserve space for connection costs and intermediate node values
        connection_costs_ = std::vector<double>(num_nodes, std::numeric_limits<double>::quiet_NaN());

        //The net start indicies we calculated earlier didn't account for intermediate binary tree nodes
        //Shift the start indicies after the intermediate nodes
        size_t num_intermediate_nodes = num_nodes_up_to_level(ilevel - 1);
        for (ClusterNetId net : nets) {
            if (nlist.net_is_ignored(net)) continue;

            net_start_indicies_[net] = net_start_indicies_[net] + num_intermediate_nodes;
        }
    }

    /**
     * @brief Proxy class representing a connection cost.
     *
     * Supports modification of connection cost while detecting
     * changes and reporting them up to PlacerTimingCosts.
     */
    class ConnectionProxy {
      public:
        ConnectionProxy(PlacerTimingCosts* timing_costs, double& connection_cost)
            : timing_costs_(timing_costs)
            , connection_cost_(connection_cost) {}

        ///@brief Allow clients to modify the connection cost via assignment.
        ConnectionProxy& operator=(double new_cost) {
            if (new_cost != connection_cost_) {
                //If connection cost changed, update it, and mark it
                //as invalidated
                connection_cost_ = new_cost;
                timing_costs_->invalidate(&connection_cost_);
            }
            return *this;
        }

        /**
         * @brief Support getting the current connection cost as a double.
         *
         * Useful for client code operating on the cost values (e.g. difference between costs).
         */
        operator double() {
            return connection_cost_;
        }

      private:
        PlacerTimingCosts* timing_costs_;
        double& connection_cost_;
    };

    /**
     * @brief Proxy class representing the connection costs of a net.
     *
     * Supports indexing by pin index to retrieve the ConnectionProxy for that pin/connection.
     */
    class NetProxy {
      public:
        NetProxy(PlacerTimingCosts* timing_costs, double* net_sink_costs)
            : timing_costs_(timing_costs)
            , net_sink_costs_(net_sink_costs) {}

        ///@brief Indexes into the specific net pin/connection.
        ConnectionProxy operator[](size_t ipin) {
            return ConnectionProxy(timing_costs_, net_sink_costs_[ipin]);
        }

      private:
        PlacerTimingCosts* timing_costs_;
        double* net_sink_costs_;
    };

    ///@brief Indexes into the specific net.
    NetProxy operator[](ClusterNetId net_id) {
        VTR_ASSERT_SAFE(net_start_indicies_[net_id] >= 0);

        double* net_connection_costs = &connection_costs_[net_start_indicies_[net_id]];
        return NetProxy(this, net_connection_costs);
    }

    void clear() {
        connection_costs_.clear();
        net_start_indicies_.clear();
    }

    void swap(PlacerTimingCosts& other) {
        std::swap(connection_costs_, other.connection_costs_);
        std::swap(net_start_indicies_, other.net_start_indicies_);
        std::swap(num_levels_, other.num_levels_);
    }

    /**
     * @brief Calculates the total cost of all connections efficiently
     *        in the face of modified connection costs.
     */
    double total_cost() {
        float cost = total_cost_recurr(0); //Root

        VTR_ASSERT_DEBUG_MSG(cost == total_cost_from_scratch(0),
                             "Expected incremental and from-scratch costs to be consistent");

        return cost;
    }

  private:
    ///@brief Recursively calculate and update the timing cost rooted at inode.
    double total_cost_recurr(size_t inode) {
        //Prune out-of-tree
        if (inode > connection_costs_.size() - 1) {
            return 0.;
        }

        //Valid pre-calculated intermediate result or valid leaf
        if (!std::isnan(connection_costs_[inode])) {
            return connection_costs_[inode];
        }

        //Recompute recursively
        double node_cost = total_cost_recurr(left_child(inode))
                           + total_cost_recurr(right_child(inode));

        //Save intermedate cost at this node
        connection_costs_[inode] = node_cost;

        return node_cost;
    }

    double total_cost_from_scratch(size_t inode) const {
        //Prune out-of-tree
        if (inode > connection_costs_.size() - 1) {
            return 0.;
        }

        //Recompute recursively
        double node_cost = total_cost_from_scratch(left_child(inode))
                           + total_cost_from_scratch(right_child(inode));

        return node_cost;
    }

    ///@brief Friend-ed so it can call invalidate().
    friend ConnectionProxy;

    void invalidate(double* invalidated_cost) {
        //Check pointer within range of internal storage
        VTR_ASSERT_SAFE_MSG(
            invalidated_cost >= &connection_costs_[0],
            "Connection cost pointer should be after start of internal storage");

        VTR_ASSERT_SAFE_MSG(
            invalidated_cost <= &connection_costs_[connection_costs_.size() - 1],
            "Connection cost pointer should be before end of internal storage");

        size_t icost = invalidated_cost - &connection_costs_[0];

        VTR_ASSERT_SAFE(icost >= num_nodes_up_to_level(num_levels_ - 2));

        //Invalidate parent intermediate costs up to root or first
        //already-invalidated parent
        size_t iparent = parent(icost);

        while (!std::isnan(connection_costs_[iparent])) {
            //Invalidate
            connection_costs_[iparent] = std::numeric_limits<double>::quiet_NaN();

            if (iparent == 0) {
                break; //At root
            } else {
                //Next parent
                iparent = parent(iparent);
            }
        }

        VTR_ASSERT_SAFE_MSG(std::isnan(connection_costs_[0]), "Invalidating any connection should have invalidated the root");
    }

    size_t left_child(size_t i) const {
        return 2 * i + 1;
    }

    size_t right_child(size_t i) const {
        return 2 * i + 2;
    }

    size_t parent(size_t i) const {
        return (i - 1) / 2;
    }

    /**
     * @brief Returns the number of nodes in ilevel'th level.
     *
     * If ilevel is negative, return 0, since the root shouldn't
     * be counted as a leaf node candidate.
     */
    size_t num_nodes_in_level(int ilevel) const {
        return ilevel < 0 ? 0 : (2 << (ilevel));
    }

    ///@brief Returns the total number of nodes in levels [0..ilevel] (inclusive).
    size_t num_nodes_up_to_level(int ilevel) const {
        return (2 << (ilevel + 1)) - 1;
    }

  private:
    /**
     * @brief Vector storing the implicit binary tree of connection costs.
     *
     * The actual connections are stored at the end of the vector
     * (last level of the binary tree). The earlier portions of
     * the tree are the intermediate nodes.
     *
     * The methods left_child()/right_child()/parent() can be used
     * to traverse the tree by indicies into this vector.
     */
    std::vector<double> connection_costs_;

    /**
     * @brief Vector storing the indicies of the first connection
     *        for each net in the netlist, used for indexing by net.
     */
    vtr::vector<ClusterNetId, int> net_start_indicies_;

    ///@brief Number of levels in the binary tree.
    size_t num_levels_ = 0;
};
