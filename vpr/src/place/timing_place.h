#ifndef TIMING_PLACE
#define TIMING_PLACE

#include "vtr_vec_id_set.h"
#include "timing_info_fwd.h"
#include "clustered_netlist_utils.h"
#include "place_delay_model.h"
#include "vpr_net_pins_matrix.h"

std::unique_ptr<PlaceDelayModel> alloc_lookups_and_criticalities(t_chan_width_dist chan_width_dist,
                                                                 const t_placer_opts& place_opts,
                                                                 const t_router_opts& router_opts,
                                                                 t_det_routing_arch* det_routing_arch,
                                                                 std::vector<t_segment_inf>& segment_inf,
                                                                 const t_direct_inf* directs,
                                                                 const int num_directs);
/* Usage
 * =====
 * PlacerCriticalities returns the clustered netlist connection criticalities used by 
 * the placer ('sharpened' by a criticality exponent). This also serves to map atom 
 * netlist level criticalites (i.e. on AtomPinIds) to the clustered netlist (i.e. 
 * ClusterPinIds) used during placement.
 *
 * Criticalities are calculated by calling update_criticalities(), which will 
 * update criticalities based on the atom netlist connection criticalities provided by
 * the passed in SetupTimingInfo. This is done incrementally, based on the modified
 * connections/AtomPinIds returned by SetupTimingInfo.
 *
 * The criticalities of individual connections can then be queried by calling the 
 * criticality() member function.
 *
 * It also supports iterating via pins_with_modified_criticalities() through the 
 * clustered netlist pins/connections which have had their criticality modified by 
 * the last call to update_criticalities(), which is useful for incrementally 
 * re-calculating timing costs.
 *
 * Implementation
 * ==============
 * To support incremental re-calculation the class saves the last criticality exponent
 * passed to update_criticalites(). If the next update uses the same exponent criticalities
 * can be incrementally updated. Otherwise they must be re-calculated from scratch, since
 * a change in exponent changes *all* criticalities.
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
    //Returns the criticality of the specified connection
    float criticality(ClusterNetId net, int ipin) const { return timing_place_crit_[net][ipin]; }

    //Returns the range of clustered netlist pins (i.e. ClusterPinIds) which were modified
    //by the last call to update_criticalities()
    pin_range pins_with_modified_criticality() const;

  public: //Modifiers
    //Incrementally updates criticalities based on the atom netlist criticalitites provied by
    //timing_info and the provided criticality_exponent.
    void update_criticalities(const SetupTimingInfo* timing_info, float criticality_exponent);

    //Override the criticality of a particular connection
    void set_criticality(ClusterNetId net, int ipin, float val);

  private: //Data
    const ClusteredNetlist& clb_nlist_;
    const ClusteredPinAtomPinsLookup& pin_lookup_;

    ClbNetPinsMatrix<float> timing_place_crit_; /* [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1] */

    //The criticality exponent when update_criticalites() was last called (used to detect if incremental update can be used)
    float last_crit_exponent_ = std::numeric_limits<float>::quiet_NaN();

    //Set of pins with criticaltites modified by last call to update_criticalities()
    vtr::vec_id_set<ClusterPinId> cluster_pins_with_modified_criticality_;
};

/* Usage
 * =====
 * PlacerTimingCosts mimics a 2D array of connection timing costs running from:
 *      [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]
 *
 * So it can be used similar to:
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
 *      //Calculate the updated timing cost, of all connections, incrementally based 
 *      //on modifications
 *      float total_timing_cost = connection_timing_costs.total_cost();
 *      
 * However behind the scenes PlacerTimingCosts tracks when connection costs are modified,
 * and efficiently re-calculates the total timing cost incrementally based on the connections
 * which have had their cost modified.
 *
 * Implementaion
 * =============
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
 * the timing connection costs allows us to incrementally update the total timign cost while
 * maintianing the *same order of operations* as if it was re-computed from scratch. This 
 * ensures we *always* get consistent results regardless of what/when connections are changed.
 *
 * Proxy Classes
 * -------------
 * NetProxy is returned by PlacerTimingCost's operator[], and stores a pointer to the start of
 * internal storage of that net's connection costs.
 *
 * ConnectionProxy is returnd by NetProxy's operator[], and holds a reference to a particular 
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
        VTR_ASSERT_MSG(num_connections == 0 || num_level_before_leaves < num_connections, "Level before should have fewer nodes than connections (to ensure using the smallest binary tree)");

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

    //Proxy class representing a connection cost
    // Supports modification of connection cost while detecting changes and
    // reporting them up to PlacerTimingCosts
    class ConnectionProxy {
      public:
        ConnectionProxy(PlacerTimingCosts* timing_costs, double& connection_cost)
            : timing_costs_(timing_costs)
            , connection_cost_(connection_cost) {}

        //Allow clients to modify the connection cost via assignment
        ConnectionProxy& operator=(double new_cost) {
            if (new_cost != connection_cost_) {
                //If connection cost changed, update it, and mark it
                //as invalidated
                connection_cost_ = new_cost;
                timing_costs_->invalidate(&connection_cost_);
            }
            return *this;
        }

        //Support getting the current connection cost as a double
        // Useful for client code operating on the cost values (e.g.
        // difference between costs)
        operator double() {
            return connection_cost_;
        }

      private:
        PlacerTimingCosts* timing_costs_;
        double& connection_cost_;
    };

    //Proxy class representing the connection costs of a net
    // Supports indexing by pin index to retrieve the ConnectionProxy for that pin/connection
    class NetProxy {
      public:
        NetProxy(PlacerTimingCosts* timing_costs, double* net_sink_costs)
            : timing_costs_(timing_costs)
            , net_sink_costs_(net_sink_costs) {}

        //Indexes into the specific net pin/connection
        ConnectionProxy operator[](size_t ipin) {
            return ConnectionProxy(timing_costs_, net_sink_costs_[ipin]);
        }

      private:
        PlacerTimingCosts* timing_costs_;
        double* net_sink_costs_;
    };

    //Indexes into the specific net
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

    //Calculates the total cost of all connections efficiently
    //in the face of modified connection costs
    double total_cost() {
        float cost = total_cost_recurr(0); //Root

        VTR_ASSERT_DEBUG_MSG(cost == total_cost_from_scratch(0),
                             "Expected incremental and from-scratch costs to be consistent");

        return cost;
    }

  private:
    //Recursively calculate and update the timing cost rooted at inode
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

    friend ConnectionProxy; //So it can call invalidate()

    void invalidate(double* invalidated_cost) {
        //Check pointer within range of internal storage
        VTR_ASSERT_SAFE_MSG(invalidated_cost >= &connection_costs_[0], "Connection cost pointer should be after start of internal storage");
        VTR_ASSERT_SAFE_MSG(invalidated_cost <= &connection_costs_[connection_costs_.size() - 1], "Connection cost pointer should be before end of internal storage");

        size_t icost = invalidated_cost - &connection_costs_[0];

        VTR_ASSERT_SAFE(icost >= num_nodes_up_to_level(num_levels_ - 2));

        //Invalidate parent intermediate costs up to root or first
        //already-invalidated parent
        size_t iparent = parent(icost);
        ;
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

    //Returns the number of nodes in ilevel'th level
    //If ilevel is negative, return 0, since the root shouldn't be counted
    //as a leaf node candidate
    size_t num_nodes_in_level(int ilevel) const {
        return ilevel < 0 ? 0 : (2 << (ilevel));
    }

    //Returns the total number of nodes in levels [0..ilevel] (inclusive)
    size_t num_nodes_up_to_level(int ilevel) const {
        return (2 << (ilevel + 1)) - 1;
    }

  private:
    //Vector storing the implicit binary tree of connection costs
    // The actual connections are stored at the end of the vector
    // (last level of the binary tree). The earlier portions of
    // the tree are the intermediate nodes.
    //
    // The methods left_child()/right_child()/parent() can be used
    // to traverse the tree by indicies into this vector
    std::vector<double> connection_costs_;

    //Vector storing the indicies of the first connection for
    //each net in the netlist, used for indexing by net.
    vtr::vector<ClusterNetId, int> net_start_indicies_;

    //Number of levels in the binary tree
    size_t num_levels_ = 0;
};

#endif
