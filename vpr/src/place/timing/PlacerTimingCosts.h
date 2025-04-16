
#pragma once
#include "vtr_vec_id_set.h"
#include "timing_info_fwd.h"
#include "clustered_netlist_utils.h"
#include "place_delay_model.h"
#include "vpr_net_pins_matrix.h"

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
 * connection_costs_ vector, using Eytzinger's/BFS layout.) By summing the entire binary
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
 * maintaining the *same order of operations* as if it was re-computed from scratch. This
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

    PlacerTimingCosts(const ClusteredNetlist& nlist);

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
        operator double() const {
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

        const ConnectionProxy operator[](size_t ipin) const {
            return ConnectionProxy(timing_costs_, net_sink_costs_[ipin]);
        }

      private:
        PlacerTimingCosts* timing_costs_;
        double* net_sink_costs_;
    };

    ///@brief Indexes into the specific net.
    NetProxy operator[](ClusterNetId net_id) {
        VTR_ASSERT_SAFE(net_start_indices_[net_id] >= 0);

        double* net_connection_costs = &connection_costs_[net_start_indices_[net_id]];
        return NetProxy(this, net_connection_costs);
    }

    NetProxy operator[](ClusterNetId net_id) const {
        VTR_ASSERT_SAFE(net_start_indices_[net_id] >= 0);

        const double* net_connection_costs = &connection_costs_[net_start_indices_[net_id]];
        return NetProxy(const_cast<PlacerTimingCosts*>(this), const_cast<double*>(net_connection_costs));
    }

    void clear() {
        connection_costs_.clear();
        net_start_indices_.clear();
    }

    void swap(PlacerTimingCosts& other) {
        std::swap(connection_costs_, other.connection_costs_);
        std::swap(net_start_indices_, other.net_start_indices_);
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
    double total_cost_recurr(size_t inode);

    double total_cost_from_scratch(size_t inode) const;

    ///@brief Friend-ed so it can call invalidate().
    friend ConnectionProxy;

    void invalidate(const double* invalidated_cost);

    static size_t left_child(size_t i) {
        return 2 * i + 1;
    }

    static size_t right_child(size_t i) {
        return 2 * i + 2;
    }

    static size_t parent(size_t i) {
        return (i - 1) / 2;
    }

    /**
     * @brief Returns the number of nodes in ilevel'th level.
     *
     * If ilevel is negative, return 0, since the root shouldn't
     * be counted as a leaf node candidate.
     */
    static size_t num_nodes_in_level(int ilevel) {
        return ilevel < 0 ? 0 : (2 << (ilevel));
    }

    ///@brief Returns the total number of nodes in levels [0..ilevel] (inclusive).
    static size_t num_nodes_up_to_level(int ilevel) {
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
     * to traverse the tree by indices into this vector.
     */
    std::vector<double> connection_costs_;

    /**
     * @brief Vector storing the indices of the first connection
     *        for each net in the netlist, used for indexing by net.
     */
    vtr::vector<ClusterNetId, int> net_start_indices_;

    ///@brief Number of levels in the binary tree.
    size_t num_levels_ = 0;
};
