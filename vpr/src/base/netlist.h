#ifndef NETLIST_H
#define NETLIST_H

/*
 * Summary
 * =======
 * This file defines the Netlist class, which stores the connectivity information
 * of the components in a netlist. It is the base class for AtomNetlist and ClusteredNetlist.
 *
 * Overview
 * ========
 * The netlist logically consists of several different components: Blocks, Ports, Pins and Nets
 * Each component in the netlist has a unique template identifier (BlockId, PortId, PinId, NetId)
 * used to retrieve information about it. In this implementation these ID's are unique throughout the
 * netlist (i.e. every port in the netlist has a unique ID, even if the ports share a common type).
 *
 * Block
 * -----
 * A Block is the primitive netlist element (a node in the netlist hyper-graph).
 * Blocks have various attributes (a name, a type etc.) and are associated with sets of
 * input/output/clock ports.
 *
 * Block related information can be retrieved using the block_*() member functions.
 *
 * Pins
 * ----
 * Pins define single-bit connections between a block and a net.
 *
 * Pin related information can be retrieved using the pin_*() member functions.
 *
 * Nets
 * ----
 * Nets represent the connections between blocks (the edges of the netlist hyper-graph).
 * Each net has a single driver pin, and a set of sink pins.
 *
 * Net related information can be retrieved using the net_*() member functions.
 *
 * Ports
 * -----
 * A Port is a (potentially multi-bit) group of pins.
 *
 * For example, the two operands and output of an N-bit adder would logically be grouped as three ports.
 * Ports have a specified bit-width which defines how many pins form the port.
 *
 * Port related information can be retrieved using the port_*() member functions.
 *
 *
 * Usage
 * =====
 *
 * The following provides usage examples for common use-cases.
 *
 * Walking the netlist
 * -------------------
 * To iterate over the whole netlist use the blocks() and/or nets() member functions:
 *
 *      Netlist netlist;
 *
 *      //... initialize the netlist
 *
 *      //Iterate over all the blocks
 *      for(BlockId blk_id : netlist.blocks()) {
 *          //Do something with each block
 *      }
 *
 *
 *      //Iterate over all the nets
 *      for(NetId net_id : netlist.nets()) {
 *          //Do something with each net
 *      }
 *
 * To retrieve information about a netlist component call one of the associated member functions:
 *
 *      //Print out each block's name
 *      for(BlockId blk_id : netlist.blocks()) {
 *
 *          //Get the block name
 *          const std::string& block_name = netlist.block_name(blk_id);
 *
 *          //Print it
 *          printf("Block: %s\n", block_name.c_str());
 *      }
 *
 * Note that the member functions are associated with the type of componenet (e.g. block_name() yields
 * the name of a block, net_name() yields the name of a net).
 *
 * Tracing cross-references
 * ------------------------
 * It is common to need to trace the netlist connectivity. The Netlist allows this to be done
 * efficiently by maintaining cross-references between the various netlist components.
 *
 * The following diagram shows the main methods and relationships between netlist components:
 *
 *            +---------+      pin_block()
 *            |         |<--------------------------+
 *            |  Block  |                           |
 *            |         |-----------------------+   |
 *            +---------+      block_pins()     |   |
 *               |   ^                          v   |
 *               |   |                       +---------+  net_pins()  +---------+
 *               |   |                       |         |<-------------|         |
 * block_ports() |   | port_block()          |   Pin   |              |   Net   |
 *               |   |                       |         |------------->|         |
 *               |   |                       +---------+  pin_net()   +---------+
 *               v   |                          ^   |
 *            +---------+      port_pins()      |   |
 *            |         |-----------------------+   |
 *            |  Port   |                           |
 *            |         |<--------------------------+
 *            +---------+      pin_port()
 *
 * Note that methods which are plurals (e.g. net_pins()) return multiple components.
 *
 *
 * As an example consider the case where we wish to find all the blocks associated with a particular net:
 *
 *      NetId net_id;
 *
 *      //... Initialize net_id with the net of interest
 *
 *      //Iterate through each pin on the net to get the associated port
 *      for(PinId pin_id : netlist.net_pins(net_id)) {
 *
 *          //Get the port associated with the pin
 *          PortId port_id = netlist.pin_port(pin_id);
 *
 *          //Get the block associated with the port
 *          BlockId blk_id = netlist.port_block(port_id);
 *
 *          //Print out the block name
 *          const std::string& block_name = netlist.block_name(blk_id);
 *          printf("Associated block: %s\n", block_name.c_str());
 *      }
 *
 * Netlist also defines some convenience functions for common operations to avoid tracking the intermediate IDs
 * if they are not needed. The following produces the same result as above:
 *
 *      NetId net_id;
 *
 *      //... Initialize net_id with the net of interest
 *
 *      //Iterate through each pin on the net to get the associated port
 *      for(PinId pin_id : netlist.net_pins(net_id)) {
 *
 *          //Get the block associated with the pin (bypassing the port)
 *          BlockId blk_id = netlist.pin_block(pin_id);
 *
 *          //Print out the block name
 *          const std::string& block_name = netlist.block_name(blk_id);
 *          printf("Associated block: %s\n", block_name.c_str());
 *      }
 *
 *
 * As another example, consider the inverse problem of identifying the nets connected as inputs to a particular block:
 *
 *      BlkId blk_id;
 *
 *      //... Initialize blk_id with the block of interest
 *
 *      //Iterate through the ports
 *      for(PortId port_id : netlist.block_input_ports(blk_id)) {
 *
 *          //Iterate through the pins
 *          for(PinId pin_id : netlist.port_pins(port_id)) {
 *              //Retrieve the net
 *              NetId net_id = netlist.pin_net(pin_id);
 *
 *              //Get its name
 *              const std::string& net_name = netlist.net_name(net_id);
 *              printf("Associated net: %s\n", net_name.c_str());
 *          }
 *      }
 *
 * Here we used the block_input_ports() method which returned an iterable range of all the input ports
 * associated with blk_id. We then used the port_pins() method to get iterable ranges of all the pins
 * associated with each port, from which we can find the associated net.
 *
 * Often port information is not relevant so this can be further simplified by iterating over a block's pins
 * directly (e.g. by calling one of the block_*_pins() functions):
 *
 *      BlkId blk_id;
 *
 *      //... Initialize blk_id with the block of interest
 *
 *      //Iterate over the blocks ports directly
 *      for(PinId pin_id : netlist.block_input_pins(blk_id)) {
 *
 *          //Retrieve the net
 *          NetId net_id = netlist.pin_net(pin_id);
 *
 *          //Get its name
 *          const std::string& net_name = netlist.net_name(net_id);
 *          printf("Associated net: %s\n", net_name.c_str());
 *      }
 *
 * Note the use of range-based-for loops in the above examples; it could also have written (more verbosely)
 * using a conventional for loop and explicit iterators as follows:
 *
 *      BlkId blk_id;
 *
 *      //... Initialize blk_id with the block of interest
 *
 *      //Iterate over the blocks ports directly
 *      auto pins = netlist.block_input_pins(blk_id);
 *      for(auto pin_iter = pins.begin(); pin_iter != pins.end(); ++pin_iter) {
 *
 *          //Retrieve the net
 *          NetId net_id = netlist.pin_net(*pin_iter);
 *
 *          //Get its name
 *          const std::string& net_name = netlist.net_name(net_id);
 *          printf("Associated net: %s\n", net_name.c_str());
 *      }
 *
 *
 * Creating the netlist
 * --------------------
 * The netlist can be created by using the create_*() member functions to create individual Blocks/Ports/Pins/Nets.
 *
 * For instance to create the following netlist (where each block is the same type, and has an input port 'A'
 * and output port 'B'):
 *
 *      -----------        net1         -----------
 *      | block_1 |-------------------->| block_2 |
 *      -----------          |          -----------
 *                           |
 *                           |          -----------
 *                           ---------->| block_3 |
 *                                      -----------
 * We could do the following:
 *
 *      const t_model* blk_model = .... //Initialize the block model appropriately
 *
 *      Netlist netlist("my_netlist"); //Initialize the netlist with name 'my_netlist'
 *
 *      //Create the first block
 *      BlockId blk1 = netlist.create_block("block_1", blk_model);
 *
 *      //Create the first block's output port
 *      //  Note that the input/output/clock type of the port is determined
 *      //  automatically from the block model
 *      PortId blk1_out = netlist.create_port(blk1, "B");
 *
 *      //Create the net
 *      NetId net1 = netlist.create_net("net1");
 *
 *      //Associate the net with blk1
 *      netlist.create_pin(blk1_out, 0, net1, PinType::DRIVER);
 *
 *      //Create block 2 and hook it up to net1
 *      BlockId blk2 = netlist.create_block("block_2", blk_model);
 *      PortId blk2_in = netlist.create_port(blk2, "A");
 *      netlist.create_pin(blk2_in, 0, net1, PinType::SINK);
 *
 *      //Create block 3 and hook it up to net1
 *      BlockId blk3 = netlist.create_block("block_3", blk_model);
 *      PortId blk3_in = netlist.create_port(blk3, "A");
 *      netlist.create_pin(blk3_in, 0, net1, PinType::SINK);
 *
 * Modifying the netlist
 * ---------------------
 * The netlist can also be modified by using the remove_*() member functions. If we wanted to remove
 * block_3 from the netlist creation example above we could do the following:
 *
 *      //Mark blk3 and any references to it invalid
 *      netlist.remove_block(blk3);
 *
 *      //Compress the netlist to actually remove the data assoicated with blk3
 *      // NOTE: This will invalidate all client held IDs (e.g. blk1, blk1_out, net1, blk2, blk2_in)
 *      netlist.compress();
 *
 * The resulting netlist connectivity now looks like:
 *
 *      -----------        net1         -----------
 *      | block_1 |-------------------->| block_2 |
 *      -----------                     -----------
 *
 * Note that until compress() is called any 'removed' elements will have invalid IDs (e.g. BlockId::INVALID()).
 * As a result after calling remove_block() (which invalidates blk3) we *then* called compress() to remove the
 * invalid IDs.
 *
 * Also note that compress() is relatively slow. As a result avoid calling compress() after every call to
 * a remove_*() function, and instead batch up calls to remove_*() and call compress() only after a set of
 * modifications have been applied.
 *
 * Verifying the netlist
 * ---------------------
 * Particularly after construction and/or modification it is a good idea to check that the netlist is in
 * a valid and consistent state. This can be done with the verify() member function:
 *
 *      netlist.verify()
 *
 * If the netlist is not valid verify() will throw an exception, otherwise it returns true.
 *
 * Invariants
 * ==========
 * The Netlist maintains stronger invariants if the netlist is in compressed form.
 *
 * Netlist is compressed ('not dirty')
 * -----------------------------------
 * If the netlist is compressed (i.e. !is_dirty(), meaning there have been NO calls to remove_*() since the last call to
 * compress()) the following invariant will hold:
 *      - Any range returned will contain only valid IDs
 *
 * In practise this means the following conditions hold:
 *      - Blocks will not contain empty ports/pins (e.g. ports with no pin/net connections)
 *      - Ports will not contain pins with no associated net
 *      - Nets will not contain invalid sink pins
 *
 * This means that no error checking for invalid IDs is needed if simply iterating through netlist (see below for
 * some exceptions).
 *
 * NOTE: you may still encounter invalid IDs in the following cases:
 *      - net_driver() will return an invalid ID if the net is undriven
 *      - port_pin()/port_net() will return an invalid ID if the bit index corresponds to an unconnected pin
 *
 * Netlist is NOT compressed ('dirty')
 * -----------------------------------
 * If the netlist is not compressed (i.e. is_dirty(), meaning there have been calls to remove_*() with no subsequent
 * calls to compress()) then the invariant above does not hold.
 *
 * Any range may return invalid IDs. In practise this means,
 *      - Blocks may contain invalid ports/pins
 *      - Ports may contain invalid pins
 *      - Pins may not have a valid associated net
 *      - Nets may contain invalid sink pins
 *
 * Implementation Details
 * ======================
 * The netlist is stored in Struct-of-Arrays format rather than the more conventional Array-of-Structs.
 * This improves cache locality by keeping component attributes of the same type in contiguous memory.
 * This prevents unneeded member data from being pulled into the cache (since most code accesses only a few
 * attributes at a time this tends to be more efficient).
 *
 * Clients of this class pass nearly-opaque IDs (BlockId, PortId, PinId, NetId, StringId) to retrieve
 * information. The ID is internally converted to an index to retrieve the required value from it's associated storage.
 *
 * By using nearly-opaque IDs we can change the underlying data layout as need to optimize performance/memory, without
 * disrupting client code.
 *
 * Strings
 * -------
 * To minimize memory usage, we store each unique string only once in the netlist and give it a unique ID (StringId).
 * Any references to this string then make use of the StringId.
 *
 * In particular this prevents the (potentially large) strings from begin duplicated multiple times in various look-ups,
 * instead the more space efficient StringId is duplicated.
 *
 * Note that StringId is an internal implementation detail and should not be exposed as part of the public interface.
 * Any public functions should take and return std::string's instead.
 *
 * Block pins/Block ports data layout
 * ----------------------------------
 * The pins/ports for each block are stored in a similar manner, for brevity we describe only pins here.
 *
 * The pins for each block (i.e. PinId's) are stored in a single vector for each block (the block_pins_ member).
 * This allows us to iterate over all pins (i.e. block_pins()), or specific subsets of pins (e.g. only inputs with
 * block_input_pins()).
 *
 * To accomplish this all pins of the same group (input/output/clock) are located next to each other. An example is
 * shown below, where the block has n input pins, m output pins and k clock pins.
 *
 *  -------------------------------------------------------------------------------------------------------------------
 *  | ipin_1 | ipin_2 | ... | ipin_n | opin_1 | opin_2 | ... | opin_m | clock_pin_1 | clock_pin_2 | ... | clock_pin_k |
 *  -------------------------------------------------------------------------------------------------------------------
 *  ^                                ^                                ^                                               ^
 *  |                                |                                |                                               |
 * begin                         opin_begin                    clock_pin_begin                                       end
 *
 * Provided we know the internal dividing points (i.e. opin_begin and clock_pin_begin) we can easily build the various
 * ranges of interest:
 *
 *      all pins   : [begin, end)
 *      input pins : [begin, opin_begin)
 *      output pins: [opin_begin, clock_pin_begin)
 *      clock pins : [clock_pin_begin, end)
 *
 * Since any reallocation would invalidate any iterators to these internal dividers, we separately store the number
 * of input/output/clock pins per block (i.e. in block_num_input_pins_, block_num_output_pins_ and
 * block_num_clock_pins_). The internal dividers can then be easily calculated (e.g. see block_output_pins()), even
 * if new pins are inserted (provided the counts are updated).
 *
 * Adding data to the netlist
 * --------------------------
 * The Netlist should contain only information directly related to the netlist state (i.e. netlist connectivity).
 * Various mappings to/from elements (e.g. what CLB contains an atom block), and algorithmic state (e.g. if a net
 * is routed) do NOT constitute netlist state and should NOT be stored here.
 *
 * Such implementation state should be stored in other data structures (which may reference the Netlist's IDs).
 *
 * The netlist state should be immutable (i.e. read-only) for most of the CAD flow.
 *
 * Interactions with other netlists
 * --------------------------------
 * Currently, the AtomNetlist and ClusteredNetlist are both derived from Netlist. The AtomNetlist has primitive
 * specific details (t_model, TruthTable), and handles all operations with the atoms. The ClusteredNetlist
 * contains information on the CLB (Clustered Logic Block) level, which includes the physical description of the
 * blocks (t_type_descriptor), as well as the internal hierarchy and wiring (t_pb/t_pb_route).
 *
 * The calling-conventions of the functions in the AtomNetlist and ClusteredNetlist is as follows:
 *
 *    Functions where the derived class (Atom/Clustered) calls the base class (Netlist)
 *       create_*()
 *
 *    Functions where the base class calls the derived class (Non-Virtual Interface idiom as described
 *    https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Non-Virtual_Interface)
 *       remove_*()
 *       clean_*()
 *       validate_*_sizes()
 *       shrink_to_fit()
 *    The derived functions based off of the virtual functions have suffix *_impl()
 *
 */
#include <string>
#include <vector>
#include <unordered_map>
#include "vtr_range.h"
#include "vtr_logic.h"
#include "vtr_vector_map.h"

#include "logic_types.h"

#include "netlist_fwd.h"
#include "netlist_utils.h"

//Forward declaration for private methods
template<typename BlockId, typename PortId, typename PinId, typename NetId>
class NetlistIdRemapper {
  public:
    BlockId new_block_id(BlockId old_blk) const;
    PortId new_port_id(PortId old_port) const;
    PinId new_pin_id(PinId old_pin) const;
    NetId new_net_id(NetId old_net) const;

  private:
    friend Netlist<BlockId, PortId, PinId, NetId>;

    vtr::vector_map<BlockId, BlockId> block_id_map_;
    vtr::vector_map<PortId, PortId> port_id_map_;
    vtr::vector_map<PinId, PinId> pin_id_map_;
    vtr::vector_map<NetId, NetId> net_id_map_;
};

template<typename BlockId, typename PortId, typename PinId, typename NetId>
class Netlist {
  public: //Public Types
    typedef typename vtr::vector_map<BlockId, BlockId>::const_iterator block_iterator;
    typedef typename std::unordered_map<std::string, std::string>::const_iterator attr_iterator;
    typedef typename std::unordered_map<std::string, std::string>::const_iterator param_iterator;
    typedef typename vtr::vector_map<NetId, NetId>::const_iterator net_iterator;
    typedef typename vtr::vector_map<PinId, PinId>::const_iterator pin_iterator;
    typedef typename vtr::vector_map<PortId, PortId>::const_iterator port_iterator;

    typedef typename vtr::Range<block_iterator> block_range;
    typedef typename vtr::Range<attr_iterator> attr_range;
    typedef typename vtr::Range<param_iterator> param_range;
    typedef typename vtr::Range<net_iterator> net_range;
    typedef typename vtr::Range<pin_iterator> pin_range;
    typedef typename vtr::Range<port_iterator> port_range;

    typedef NetlistIdRemapper<BlockId, PortId, PinId, NetId> IdRemapper;

  public:
    Netlist(std::string name = "", std::string id = "");
    virtual ~Netlist();

  public: //Public Accessors
    /*
     * Netlist
     */
    //Retrieve the name of the netlist
    const std::string& netlist_name() const;

    //Retrieve the unique identifier for this netlist
    //This is typically a secure digest of the input file.
    const std::string& netlist_id() const;

    /*
     * Utility
     */
    //Sanity check for internal consistency (throws an exception on failure)
    bool verify() const;

    //Returns true if the netlist has invalid entries due to modifications (e.g. from remove_*() calls)
    bool is_dirty() const;

    //Returns true if the netlist has *no* invalid entries due to modifications (e.g. from remove_*() calls)
    //Note that this is a convenience method which is the logical inverse of is_dirty()
    bool is_compressed() const;

    //Item counts and container info (for debugging)
    void print_stats() const;

    /*
     * Blocks
     */
    //Returns the name of the specified block
    const std::string& block_name(const BlockId blk_id) const;

    //Returns true if the block is purely combinational (i.e. no input clocks
    //and not a primary input
    bool block_is_combinational(const BlockId blk_id) const;

    // Returns a range of all attributes associated with the specified block
    attr_range block_attrs(const BlockId blk_id) const;

    // Returns a range of all parameters associated with the specified block
    param_range block_params(const BlockId blk_id) const;

    //Returns a range of all pins associated with the specified block
    pin_range block_pins(const BlockId blk_id) const;

    //Returns a range of all input pins associated with the specified block
    pin_range block_input_pins(const BlockId blk_id) const;

    //Returns a range of all output pins associated with the specified block
    // Note this is typically only data pins, but some blocks (e.g. PLLs) can produce outputs
    // which are clocks.
    pin_range block_output_pins(const BlockId blk_id) const;

    //Returns a range of all clock pins associated with the specified block
    pin_range block_clock_pins(const BlockId blk_id) const;

    //Returns a range of all ports associated with the specified block
    port_range block_ports(const BlockId blk_id) const;

    //Returns a range consisting of the input ports associated with the specified block
    port_range block_input_ports(const BlockId blk_id) const;

    //Returns a range consisting of the output ports associated with the specified block
    // Note this is typically only data ports, but some blocks (e.g. PLLs) can produce outputs
    // which are clocks.
    port_range block_output_ports(const BlockId blk_id) const;

    //Returns a range consisting of the input clock ports associated with the specified block
    port_range block_clock_ports(const BlockId blk_id) const;

    //Removes a block from the netlist. This will also remove the associated ports and pins.
    //  blk_id  : The block to be removed
    void remove_block(const BlockId blk_id);

    /*
     * Ports
     */
    //Returns the name of the specified port
    const std::string& port_name(const PortId port_id) const;

    //Returns the block associated with the specified port
    BlockId port_block(const PortId port_id) const;

    //Returns the set of valid pins associated with the port
    pin_range port_pins(const PortId port_id) const;

    //Returns the pin (potentially invalid) associated with the specified port and port bit index
    //  port_id : The ID of the associated port
    //  port_bit: The bit index of the pin in the port
    //Note: this function is a synonym for find_pin()
    PinId port_pin(const PortId port_id, const BitIndex port_bit) const;

    //Returns the net (potentially invalid) associated with the specified port and port bit index
    //  port_id : The ID of the associated port
    //  port_bit: The bit index of the pin in the port
    NetId port_net(const PortId port_id, const BitIndex port_bit) const;

    //Returns the width (number of bits) in the specified port
    BitIndex port_width(const PortId port_id) const;

    //Returns the type of the specified port
    PortType port_type(const PortId port_id) const;

    //Removes a port from the netlist.
    //The port's pins are also marked invalid and removed from any associated nets
    //  port_id: The ID of the port to be removed
    void remove_port(const PortId port_id);

    /*
     * Pins
     */
    //Returns the constructed name (derived from block and port) for the specified pin
    std::string pin_name(const PinId pin_id) const;

    //Returns the type of the specified pin
    PinType pin_type(const PinId pin_id) const;

    //Returns the net associated with the specified pin
    NetId pin_net(const PinId pin_id) const;

    //Returns the index of the specified pin within it's connected net
    int pin_net_index(const PinId pin_id) const;

    //Returns the port associated with the specified pin
    PortId pin_port(const PinId pin_id) const;

    //Returns the port bit index associated with the specified pin
    BitIndex pin_port_bit(const PinId pin_id) const;

    //Returns the block associated with the specified pin
    BlockId pin_block(const PinId pin_id) const;

    //Returns the port type associated with the specified pin
    PortType pin_port_type(const PinId pin_id) const;

    //Returns true if the pin is a constant (i.e. its value never changes)
    bool pin_is_constant(const PinId pin_id) const;

    //Removes a pin from the netlist.
    //The pin is marked invalid, and removed from any assoicated nets
    //  pin_id: The pin_id of the pin to be removed
    void remove_pin(const PinId pin_id);

    /*
     * Nets
     */
    //Returns the name of the specified net
    const std::string& net_name(const NetId net_id) const;

    //Returns a range consisting of all the pins in the net (driver and sinks)
    //The first element in the range is the driver (and may be invalid)
    //The remaining elements (potentially none) are the sinks
    pin_range net_pins(const NetId net_id) const;

    //Returns the net_pin_index'th pin of the specified net
    PinId net_pin(const NetId net_id, int net_pin_index) const;

    //Returns the block associated with the net_pin_index'th pin of the specified net
    BlockId net_pin_block(const NetId net_id, int net_pin_index) const;

    //Returns the (potentially invalid) net driver pin
    PinId net_driver(const NetId net_id) const;

    //Returns the (potentially invalid) net driver block
    BlockId net_driver_block(const NetId net_id) const;

    //Returns a (potentially empty) range consisting of net's sink pins
    pin_range net_sinks(const NetId net_id) const;

    //Returns true if the net is driven by a constant pin (i.e. its value never changes)
    bool net_is_constant(const NetId net_id) const;

    //Removes a net from the netlist.
    //This will mark the net's pins as having no associated.
    //  net_id  : The net to be removed
    void remove_net(const NetId net_id);

    //Removes a connection betwen a net and pin. The pin is removed from the net and the pin
    //will be marked as having no associated net
    //  net_id  : The net from which the pin is to be removed
    //  pin_id  : The pin to be removed from the net
    void remove_net_pin(const NetId net_id, const PinId pin_id);

    /*
     * Aggregates
     */
    //Returns a range consisting of all blocks in the netlist
    block_range blocks() const;

    //Returns a range consisting of all ports in the netlist
    port_range ports() const;

    //Returns a range consisting of all nets in the netlist
    net_range nets() const;

    //Returns a range consisting of all pins in the netlist
    pin_range pins() const;

    /*
     * Lookups
     */
    //Returns the BlockId of the specified block or BlockId::INVALID() if not found
    //  name: The name of the block
    BlockId find_block(const std::string& name) const;

    //Returns the PortId of the specifed port if it exists or PortId::INVALID() if not
    //Note that this method is typically less efficient than searching by a t_model_port
    //With the overloaded AtomNetlist method
    //  blk_id: The ID of the block who's ports will be checked
    //  name  : The name of the port to look for
    PortId find_port(const BlockId blk_id, const std::string& name) const;

    //Returns the NetId of the specified net or NetId::INVALID() if not found
    //  name: The name of the net
    NetId find_net(const std::string& name) const;

    //Returns the PinId of the specified pin or PinId::INVALID() if not found
    //  port_id : The ID of the associated port
    //  port_bit: The bit index of the pin in the port
    PinId find_pin(const PortId port_id, BitIndex port_bit) const;

  public: //Public Mutators
    //Add the specified pin to the specified net as pin_type. Automatically removes
    //any previous net connection for this pin.
    //  pin      : The pin to add
    //  pin_type : The type of the pin (i.e. driver or sink)
    //  net      : The net to add the pin to
    void set_pin_net(const PinId pin, PinType pin_type, const NetId net);

    //Mark a pin as being a constant generator.
    // There are some cases where a pin can not be identified as a is constant until after
    // the full netlist has been built; so we expose a way to mark existing pins as constants.
    //  pin_id  : The pin to be marked
    //  value   : The boolean value to set the pin_is_constant attribute
    void set_pin_is_constant(const PinId pin_id, const bool value);

    //Re-name a block
    //  blk_id   : The block to be renamed
    //  new_name : The new name for the specified block
    void set_block_name(const BlockId blk_id, const std::string new_name);

    //Set a block attribute
    //  blk_id   : The block to which the attribute is attached
    //  name     : The name of the attribute to set
    //  value    : The new value for the specified attribute on the specified block
    void set_block_attr(const BlockId blk_id, const std::string& name, const std::string& value);

    //Set a block parameter
    //  blk_id   : The block to which the parameter is attached
    //  name     : The name of the parameter to set
    //  value    : The new value for the specified parameter on the specified block
    void set_block_param(const BlockId blk_id, const std::string& name, const std::string& value);

    //Merges sink_net into driver_net
    //After merging driver_net will contain all the sinks of sink_net
    //  driver_net: The net which includes the driver pin
    //  sink_net: The target net to be merged into driver_net (must have no driver pin)
    void merge_nets(const NetId driver_net, const NetId sink_net);

    /*
     * Note: all remove_*() will mark the associated items as invalid, but the items
     * will not be removed until compress() is called.
     */
    //Wrapper for remove_unused() & compress()
    // This function should be used in the case where a netlist is fully modified
    IdRemapper remove_and_compress();

    //This should be called after completing a series of netlist modifications
    //(e.g. removing blocks/ports/pins/nets).
    //
    //Marks netlist components which have become redundant due to other removals
    //(e.g. ports with only invalid pins) as invalid so they will be destroyed during
    //compress()
    void remove_unused();

    //Compresses the netlist, removing any invalid and/or unreferenced
    //blocks/ports/pins/nets.
    //
    //NOTE: this invalidates all existing IDs!
    IdRemapper compress();

  protected: //Protected Mutators
    //Create or return an existing block in the netlist
    //  name        : The unique name of the block
    BlockId create_block(const std::string name);

    //Create or return an existing port in the netlist
    //  blk_id      : The block the port is associated with
    //  name        : The name of the port (must match the name of a port in the block's model)
    //  width       : The width (number of bits) of the port
    //  type        : The type of the port (INPUT, OUTPUT, CLOCK)
    PortId create_port(const BlockId blk_id, const std::string name, BitIndex width, PortType type);

    //Create or return an existing pin in the netlist
    //  port_id    : The port this pin is associated with
    //  port_bit   : The bit index of the pin in the port
    //  net_id     : The net the pin drives/sinks
    //  pin_type   : The type of the pin (driver/sink)
    //  is_const   : Indicates whether the pin holds a constant value (e. g. vcc/gnd)
    PinId create_pin(const PortId port_id, BitIndex port_bit, const NetId net_id, const PinType pin_type, bool is_const = false);

    //Create an empty, or return an existing net in the netlist
    //  name    : The unique name of the net
    NetId create_net(const std::string name); //An empty or existing net

    //Create a completely specified net from specified driver and sinks
    //  name    : The name of the net (Note: must not already exist)
    //  driver  : The net's driver pin
    //  sinks   : The net's sink pins
    NetId add_net(const std::string name, PinId driver, std::vector<PinId> sinks);

  protected: //Protected Base Types
    struct string_id_tag;

    //A unique identifier for a string in the netlist
    typedef vtr::StrongId<string_id_tag> StringId;

  protected: //Protected Base Members
    /*
     * Lookups
     */
    //Returns the StringId of the specifed string if it exists or StringId::INVALID() if not
    //  str : The string to look for
    StringId find_string(const std::string& str) const;

    //Returns the BlockId of the specifed block if it exists or BlockId::INVALID() if not
    //  name_id : The block name to look for
    BlockId find_block(const StringId name_id) const;

    //Returns the NetId of the specifed port if it exists or NetId::INVALID() if not
    //  name_id: The string ID of the net name to look for
    NetId find_net(const StringId name_id) const;

    /*
     * Mutators
     */
    //Create or return the ID of the specified string
    //  str: The string whose ID is requested
    StringId create_string(const std::string& str);

    //Updates net cross-references for the specified pin
    //Returns the pin's index within the net
    int associate_pin_with_net(const PinId pin_id, const PinType type, const NetId net_id);

    //Updates port cross-references for the specified pin
    void associate_pin_with_port(const PinId pin_id, const PortId port_id);

    //Updates block cross-references for the specified pin
    void associate_pin_with_block(const PinId pin_id, const PortType type, const BlockId blk_id);

    //Updates block cross-references for the specified port
    void associate_port_with_block(const PortId port_id, const PortType type, const BlockId blk_id);

    /*
     * Netlist compression/optimization
     */
    //Builds the new mappings from old to new IDs.
    //The various IdMap's should be initialized with invalid mappings
    //for all current ID's before being called.
    IdRemapper build_id_maps();

    //Removes invalid and reorders blocks
    void clean_blocks(const vtr::vector_map<BlockId, BlockId>& block_id_map);

    //Removes invalid and reorders ports
    void clean_ports(const vtr::vector_map<PortId, PortId>& port_id_map);

    //Removes invalid and reorders pins
    void clean_pins(const vtr::vector_map<PinId, PinId>& pin_id_map);

    //Removes invalid and reorders nets
    void clean_nets(const vtr::vector_map<NetId, NetId>& net_id_map);

    //Re-builds fast look-ups
    void rebuild_lookups();

    //Re-builds cross-references held by blocks
    void rebuild_block_refs(const vtr::vector_map<PinId, PinId>& pin_id_map,
                            const vtr::vector_map<PortId, PortId>& port_id_map);

    //Re-builds cross-references held by ports
    void rebuild_port_refs(const vtr::vector_map<BlockId, BlockId>& block_id_map,
                           const vtr::vector_map<PinId, PinId>& pin_id_map);

    //Re-builds cross-references held by pins
    void rebuild_pin_refs(const vtr::vector_map<PortId, PortId>& port_id_map,
                          const vtr::vector_map<NetId, NetId>& net_id_map);

    //Re-builds cross-references held by nets
    void rebuild_net_refs(const vtr::vector_map<PinId, PinId>& pin_id_map);

    void shrink_to_fit();

    /*
     * Sanity Checks
     */
    //Verify the internal data structure sizes match
    bool verify_sizes() const;
    bool validate_block_sizes() const;
    bool validate_port_sizes() const;
    bool validate_pin_sizes() const;
    bool validate_net_sizes() const;
    bool validate_string_sizes() const;

    //Verify that internal data structure cross-references are consistent
    bool verify_refs() const; //All cross-references
    bool validate_block_port_refs() const;
    bool validate_block_pin_refs() const;
    bool validate_port_pin_refs() const;
    bool validate_net_pin_refs() const;
    bool validate_string_refs() const;

    //Verify that block invariants hold (i.e. logical consistency)
    bool verify_block_invariants() const;

    //Verify that fast-lookups are consistent with internal data structures
    bool verify_lookups() const;

    //Validates that the specified ID is valid in the current netlist state
    bool valid_block_id(BlockId block_id) const;
    bool valid_port_id(PortId port_id) const;
    bool valid_port_bit(PortId port_id, BitIndex port_bit) const;
    bool valid_pin_id(PinId pin_id) const;
    bool valid_net_id(NetId net_id) const;
    bool valid_string_id(StringId string_id) const;

  protected: //Protected virtual functions implemented in derived classes
    //The functions follow the Non-Virtual Interface (NVI) idiom, and
    //are called from this class in their respective non-impl() functions.
    virtual void shrink_to_fit_impl() = 0;

    virtual bool validate_block_sizes_impl(size_t num_blocks) const = 0;
    virtual bool validate_port_sizes_impl(size_t num_ports) const = 0;
    virtual bool validate_pin_sizes_impl(size_t num_pins) const = 0;
    virtual bool validate_net_sizes_impl(size_t num_nets) const = 0;

    virtual void clean_blocks_impl(const vtr::vector_map<BlockId, BlockId>& block_id_map) = 0;
    virtual void clean_ports_impl(const vtr::vector_map<PortId, PortId>& port_id_map) = 0;
    virtual void clean_pins_impl(const vtr::vector_map<PinId, PinId>& pin_id_map) = 0;
    virtual void clean_nets_impl(const vtr::vector_map<NetId, NetId>& net_id_map) = 0;

    virtual void remove_block_impl(const BlockId blk_id) = 0;
    virtual void remove_port_impl(const PortId port_id) = 0;
    virtual void remove_pin_impl(const PinId pin_id) = 0;
    virtual void remove_net_impl(const NetId net_id) = 0;

    virtual void rebuild_block_refs_impl(const vtr::vector_map<PinId, PinId>& pin_id_map, const vtr::vector_map<PortId, PortId>& port_id_map) = 0;
    virtual void rebuild_port_refs_impl(const vtr::vector_map<BlockId, BlockId>& block_id_map, const vtr::vector_map<PinId, PinId>& pin_id_map) = 0;
    virtual void rebuild_pin_refs_impl(const vtr::vector_map<PortId, PortId>& port_id_map, const vtr::vector_map<NetId, NetId>& net_id_map) = 0;
    virtual void rebuild_net_refs_impl(const vtr::vector_map<PinId, PinId>& pin_id_map) = 0;

  protected:
    constexpr static int INVALID_INDEX = -1;
    constexpr static int NET_DRIVER_INDEX = 0;

  private:                     //Data
    std::string netlist_name_; //Name of the top-level netlist
    std::string netlist_id_;   //Unique identifier for the netlist
    bool dirty_ = false;       //Indicates the netlist has invalid entries from remove_*() functions

    //Block data
    vtr::vector_map<BlockId, BlockId> block_ids_;    //Valid block ids
    vtr::vector_map<BlockId, StringId> block_names_; //Name of each block

    vtr::vector_map<BlockId, std::vector<PortId>> block_ports_; //Ports of each block
    vtr::vector_map<BlockId, unsigned> block_num_input_ports_;  //Input ports of each block
    vtr::vector_map<BlockId, unsigned> block_num_output_ports_; //Output ports of each block
    vtr::vector_map<BlockId, unsigned> block_num_clock_ports_;  //Clock ports of each block

    vtr::vector_map<BlockId, std::vector<PinId>> block_pins_;  //Pins of each block
    vtr::vector_map<BlockId, unsigned> block_num_input_pins_;  //Number of input pins on each block
    vtr::vector_map<BlockId, unsigned> block_num_output_pins_; //Number of output pins on each block
    vtr::vector_map<BlockId, unsigned> block_num_clock_pins_;  //Number of clock pins on each block

    vtr::vector_map<BlockId, std::unordered_map<std::string, std::string>> block_params_; //Parameters of each block
    vtr::vector_map<BlockId, std::unordered_map<std::string, std::string>> block_attrs_;  //Attributes of each block

    //Port data
    vtr::vector_map<PortId, PortId> port_ids_;              //Valid port ids
    vtr::vector_map<PortId, StringId> port_names_;          //Name of each port
    vtr::vector_map<PortId, BlockId> port_blocks_;          //Block associated with each port
    vtr::vector_map<PortId, std::vector<PinId>> port_pins_; //Pins associated with each port
    vtr::vector_map<PortId, BitIndex> port_widths_;         //Width (in bits) of each port
    vtr::vector_map<PortId, PortType> port_types_;          //Type of the port (INPUT, OUTPUT, CLOCK)

    //Pin data
    vtr::vector_map<PinId, PinId> pin_ids_;          //Valid pin ids
    vtr::vector_map<PinId, PortId> pin_ports_;       //Type of each pin
    vtr::vector_map<PinId, BitIndex> pin_port_bits_; //The pins bit position in the port
    vtr::vector_map<PinId, NetId> pin_nets_;         //Net associated with each pin
    vtr::vector_map<PinId, int> pin_net_indices_;    //Index of the specified pin within it's associated net
    vtr::vector_map<PinId, bool> pin_is_constant_;   //Indicates if the pin always keeps a constant value

    //Net data
    vtr::vector_map<NetId, NetId> net_ids_;               //Valid net ids
    vtr::vector_map<NetId, StringId> net_names_;          //Name of each net
    vtr::vector_map<NetId, std::vector<PinId>> net_pins_; //Pins associated with each net

    //String data
    // We store each unique string once, and reference it by an StringId
    // This avoids duplicating the strings in the fast look-ups (i.e. the look-ups
    // only store the Ids)
    vtr::vector_map<StringId, StringId> string_ids_; //Valid string ids
    vtr::vector_map<StringId, std::string> strings_; //Strings

  private: //Fast lookups
    vtr::vector_map<StringId, BlockId> block_name_to_block_id_;
    vtr::vector_map<StringId, NetId> net_name_to_net_id_;
    std::unordered_map<std::string, StringId> string_to_string_id_;
};

#include "netlist.tpp"
#endif
