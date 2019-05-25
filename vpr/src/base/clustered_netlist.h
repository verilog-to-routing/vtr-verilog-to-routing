#ifndef CLUSTERED_NETLIST_H
#define CLUSTERED_NETLIST_H
/*
 * Summary
 * ========
 * This file defines the ClusteredNetlist class in the ClusteredContext created during
 * pre-placement stages of the VTR flow (packing & clustering), and used downstream.
 *
 * Overview
 * ========
 * The ClusteredNetlist is derived from the Netlist class, and contains some
 * separate information on Blocks, Pins, and Nets. It does not make use of Ports.
 *
 * Blocks
 * ------
 * The pieces of unique block information are:
 *       block_pbs_:         Physical block describing the clustering and internal hierarchy
 *                           structure of each CLB.
 *       block_types_:       The type of physical block the block is mapped to, e.g. logic
 *                           block, RAM, DSP (Can be user-defined types).
 *       block_nets_:        Based on the block's pins (indexed from [0...num_pins - 1]),
 *                           lists which pins are used/unused with the net using it.
 *       block_pin_nets_:    Returns the index of a pin relative to the net, when given a block and a pin's
 *                           index on that block (from the type descriptor).
 *                           Differs from block_nets_.
 *
 * Differences between block_nets_ & block_pin_nets_
 * --------------------------------------------------
 *           +-----------+
 *       0-->|           |-->3
 *       1-->|   Block   |-->4
 *       2-->|           |-->5
 *           +-----------+
 *
 * block_nets_ tracks all pins on a block, and returns the ClusterNetId to which a pin is connected to.
 * If the pin is unused/open, ClusterNetId::INVALID() is stored.
 *
 * block_pin_nets_ tracks whether the nets connected to the block are drivers/receivers of that net.
 * Driver/receiver nets are determined by the pin_class of the block's pin.
 * A net connected to a driver pin in the block has a 0 is stored. A net connected to a receiver
 * has a counter (from [1...num_sinks - 1]).
 *
 * The net is connected to multiple blocks. Each block_pin_nets_ has a unique number in that net.
 *
 * E.g.
 *           +-----------+                   +-----------+
 *       0-->|           |-->3   Net A   0-->|           |-->3
 *       1-->|  Block 1  |---4---------->1-->|  Block 2  |-->4
 *       2-->|           |-->5           2-->|           |-->5
 *           +-----------+               |   +-----------+
 *                                       |
 *                                       |   +-----------+
 *                                       |   |           |-->1
 *                                       0-->|  Block 3  |
 *                                           |           |-->2
 *                                           +-----------+
 *
 * In the example, Net A is driven by Block 1, and received by Blocks 2 & 3.
 * For Block 1, block_pin_nets_ of pin 4 returns 0, as it is the driver.
 * For Block 2, block_pin_nets_ of pin 1 returns 1 (or 2), non-zero as it is a receiver.
 * For Block 3, block_pin_nets_ of pin 0 returns 2 (or 1), non-zero as it is also a receiver.
 *
 * The block_pin_nets_ data structure exists for quick indexing, rather than using a linear search
 * with the available functions from the base Netlist, into the net_delay_ structure in the
 * PostClusterDelayCalculator of inter_cluster_delay(). net_delay_ is a 2D array, where the indexing
 * scheme is [net_id] followed by [pin_index on net].
 *
 * Pins
 * ----
 * The only piece of unique pin information is:
 *       physical_pin_index_
 *
 * Example of physical_pin_index_
 * ---------------------
 * Given a ClusterPinId, physical_pin_index_ will return the index of the pin within its block
 * relative to the t_type_descriptor (physical description of the block).
 *
 *           +-----------+
 *       0-->|X         X|-->3
 *       1-->|O  Block  O|-->4
 *       2-->|O         O|-->5 (e.g. ClusterPinId = 92)
 *           +-----------+
 *
 * The index skips over unused pins, e.g. CLB has 6 pins (3 in, 3 out, numbered [0...5]), where
 * the first two ins, and last two outs are used. Indices [0,1] represent the ins, and [4,5]
 * represent the outs. Indices [2,3] are unused. Therefore, physical_pin_index_[92] = 5.
 *
 * Nets
 * ----
 * The pieces of unique net information stored are:
 *       net_global_:    Boolean mapping whether the net is global
 *       net_fixed_:     Boolean mapping whether the net is fixed (i.e. constant)
 *
 * Implementation
 * ==============
 * For all create_* functions, the ClusteredNetlist will wrap and call the Netlist's version as it contains
 * additional information that the base Netlist does not know about.
 *
 * All functions with suffix *_impl() follow the Non-Virtual Interface (NVI) idiom.
 * They are called from the base Netlist class to simplify pre/post condition checks and
 * prevent Fragile Base Class (FBC) problems.
 *
 * Refer to netlist.h for more information.
 *
 */
#include "vpr_types.h"
#include "vpr_utils.h"

#include "vtr_util.h"

#include "netlist.h"
#include "clustered_netlist_fwd.h"

class ClusteredNetlist : public Netlist<ClusterBlockId, ClusterPortId, ClusterPinId, ClusterNetId> {
  public:
    //Constructs a netlist
    // name: the name of the netlist (e.g. top-level module)
    // id:   a unique identifier for the netlist (e.g. a secure digest of the input file)
    ClusteredNetlist(std::string name = "", std::string id = "");

  public: //Public Accessors
    /*
     * Blocks
     */
    //Returns the physical block
    t_pb* block_pb(const ClusterBlockId id) const;

    //Returns the type of CLB (Logic block, RAM, DSP, etc.)
    t_type_ptr block_type(const ClusterBlockId id) const;

    //Returns the net of the block attached to the specific pin index
    ClusterNetId block_net(const ClusterBlockId blk_id, const int pin_index) const;

    //Returns the count on the net of the block attached
    int block_pin_net_index(const ClusterBlockId blk_id, const int pin_index) const;

    //Returns the logical pin Id associated with the specified block and physical pin index
    ClusterPinId block_pin(const ClusterBlockId blk, const int phys_pin_index) const;

    //Returns true if the specified block contains a primary input (e.g. BLIF .input primitive)
    bool block_contains_primary_input(const ClusterBlockId blk) const;

    //Returns true if the specified block contains a primary output (e.g. BLIF .output primitive)
    bool block_contains_primary_output(const ClusterBlockId blk) const;

    /*
     * Pins
     */

    //Returns the physical pin index (i.e. pin index on the
    //t_type_descriptor) of the specified logical pin
    int pin_physical_index(const ClusterPinId id) const;

    //Finds the net_index'th net pin (e.g. the 6th pin of the net) and
    //returns the physical pin index (i.e. pin index on the t_type_descriptor)
    //of the block to which the pin belongs
    //  net_id        : The net
    //  net_pin_index : The index of the pin in the net
    int net_pin_physical_index(const ClusterNetId net_id, int net_pin_index) const;

    /*
     * Nets
     */
    //Returns whether the net is ignored i.e. not routed
    bool net_is_ignored(const ClusterNetId id) const;

    //Returns whether the net is global
    bool net_is_global(const ClusterNetId id) const;

  public: //Public Mutators
    //Create or return an existing block in the netlist
    //  name        : The unique name of the block
    //  pb          : The physical representation of the block
    //  t_type_ptr  : The type of the CLB
    ClusterBlockId create_block(const char* name, t_pb* pb, t_type_ptr type);

    //Create or return an existing port in the netlist
    //  blk_id      : The block the port is associated with
    //  name        : The name of the port (must match the name of a port in the block's model)
    //  width       : The width (number of bits) of the port
    //  type        : The type of the port (INPUT, OUTPUT, or CLOCK)
    ClusterPortId create_port(const ClusterBlockId blk_id, const std::string name, BitIndex width, PortType type);

    //Create or return an existing pin in the netlist
    //  port_id    : The port this pin is associated with
    //  port_bit   : The bit index of the pin in the port
    //  net_id     : The net the pin drives/sinks
    //  pin_type   : The type of the pin (driver/sink)
    //  pin_index  : The index of the pin relative to its block, excluding OPEN pins)
    //  is_const   : Indicates whether the pin holds a constant value (e. g. vcc/gnd)
    ClusterPinId create_pin(const ClusterPortId port_id, BitIndex port_bit, const ClusterNetId net_id, const PinType pin_type, int pin_index, bool is_const = false);

    //Sets the mapping of a ClusterPinId to the block's type descriptor's pin index
    //  pin_id   : The pin to be set
    //  index    : The new index to set the pin to
    void set_pin_physical_index(const ClusterPinId pin_id, const int index);

    //Create an empty, or return an existing net in the netlist
    //  name     : The unique name of the net
    ClusterNetId create_net(const std::string name);

    //Sets the flag in net_ignored_ = state
    void set_net_is_ignored(ClusterNetId net_id, bool state);

    //Sets the flag in net_is_global_ = state
    void set_net_is_global(ClusterNetId net_id, bool state);

  private: //Private Members
    /*
     * Netlist compression/optimization
     */
    //Removes invalid components and reorders them
    void clean_blocks_impl(const vtr::vector_map<ClusterBlockId, ClusterBlockId>& block_id_map) override;
    void clean_ports_impl(const vtr::vector_map<ClusterPortId, ClusterPortId>& port_id_map) override;
    void clean_pins_impl(const vtr::vector_map<ClusterPinId, ClusterPinId>& pin_id_map) override;
    void clean_nets_impl(const vtr::vector_map<ClusterNetId, ClusterNetId>& net_id_map) override;

    //Shrinks internal data structures to required size to reduce memory consumption
    void shrink_to_fit_impl() override;

    /*
     * Component removal
     */
    //Removes a block from the netlist. This will also remove the associated ports and pins.
    //  blk_id  : The block to be removed
    void remove_block_impl(const ClusterBlockId blk_id) override;
    void remove_port_impl(const ClusterPortId port_id) override;
    void remove_pin_impl(const ClusterPinId pin_id) override;
    void remove_net_impl(const ClusterNetId net_id) override;

    void rebuild_block_refs_impl(const vtr::vector_map<ClusterPinId, ClusterPinId>& pin_id_map, const vtr::vector_map<ClusterPortId, ClusterPortId>& port_id_map) override;
    void rebuild_port_refs_impl(const vtr::vector_map<ClusterBlockId, ClusterBlockId>& block_id_map, const vtr::vector_map<ClusterPinId, ClusterPinId>& pin_id_map) override;
    void rebuild_pin_refs_impl(const vtr::vector_map<ClusterPortId, ClusterPortId>& port_id_map, const vtr::vector_map<ClusterNetId, ClusterNetId>& net_id_map) override;
    void rebuild_net_refs_impl(const vtr::vector_map<ClusterPinId, ClusterPinId>& pin_id_map) override;

    /*
     * Sanity Checks
     */
    //Verify the internal data structure sizes match
    bool validate_block_sizes_impl(size_t num_blocks) const override;
    bool validate_port_sizes_impl(size_t num_ports) const override;
    bool validate_pin_sizes_impl(size_t num_pins) const override;
    bool validate_net_sizes_impl(size_t num_nets) const override;

  private: //Private Data
    //Blocks
    vtr::vector_map<ClusterBlockId, t_pb*> block_pbs_;                              //Physical block representing the clustering & internal hierarchy of each CLB
    vtr::vector_map<ClusterBlockId, t_type_ptr> block_types_;                       //The type of physical block this user circuit block is mapped to
    vtr::vector_map<ClusterBlockId, std::vector<ClusterPinId>> block_logical_pins_; //The logical pin associated with each physical block pin

    //Pins
    vtr::vector_map<ClusterPinId, int> pin_physical_index_; //The physical pin index (i.e. pin index
                                                            //in t_type_descriptor) of logical pins

    //Nets
    vtr::vector_map<ClusterNetId, bool> net_is_ignored_; //Boolean mapping indicating if the net is ignored
    vtr::vector_map<ClusterNetId, bool> net_is_global_;  //Boolean mapping indicating if the net is global
};

#endif
