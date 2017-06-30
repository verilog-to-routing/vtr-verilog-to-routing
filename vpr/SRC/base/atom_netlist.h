#ifndef ATOM_NETLIST_H
#define ATOM_NETLIST_H
/*
 * Summary
 * ========
 * This file defines the AtomNetlist class used to store and manipulate the primitive (or atom) netlist.
 *
 * Overview
 * ========
 * The netlist logically consists of several different components: Blocks, Ports, Pins and Nets
 * Each component in the netlist has a unique identifier (AtomBlockId, AtomPortId, AtomPinId, AtomNetId) 
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
 *      AtomNetlist netlist;
 *
 *      //... initialize the netlist
 *
 *      //Iterate over all the blocks
 *      for(AtomBlockId blk_id : netlist.blocks()) {
 *          //Do something with each block
 *      }
 *
 *
 *      //Iterate over all the nets
 *      for(AtomNetId net_id : netlist.nets()) {
 *          //Do something with each net
 *      }
 *
 * To retrieve information about a netlist component call one of the associated member functions:
 *      
 *      //Print out each block's name
 *      for(AtomBlockId blk_id : netlist.blocks()) {
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
 * It is common to need to trace the netlist connectivity. The AtomNetlist allows this to be done 
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
 *      AtomNetId net_id;
 *
 *      //... Initialize net_id with the net of interest
 *
 *      //Iterate through each pin on the net to get the associated port
 *      for(AtomPinId pin_id : netlist.net_pins(net_id)) {
 *          
 *          //Get the port associated with the pin
 *          AtomPortId port_id = netlist.pin_port(pin_id);
 *
 *          //Get the block associated with the port
 *          AtomBlockId blk_id = netlist.port_block(port_id);
 *          
 *          //Print out the block name
 *          const std::string& block_name = netlist.block_name(blk_id);
 *          printf("Associated block: %s\n", block_name.c_str());
 *      }
 *
 * AtomNetlist also defines some convenience functions for common operations to avoid tracking the intermediate IDs
 * if they are not needed. The following produces the same result as above:
 *
 *      AtomNetId net_id;
 *
 *      //... Initialize net_id with the net of interest
 *
 *      //Iterate through each pin on the net to get the associated port
 *      for(AtomPinId pin_id : netlist.net_pins(net_id)) {
 *          
 *          //Get the block associated with the pin (bypassing the port)
 *          AtomBlockId blk_id = netlist.pin_block(pin_id);
 *          
 *          //Print out the block name
 *          const std::string& block_name = netlist.block_name(blk_id);
 *          printf("Associated block: %s\n", block_name.c_str());
 *      }
 *
 *
 * As another example, consider the inverse problem of identifying the nets connected as inputs to a particular block:
 *
 *      AtomBlkId blk_id;
 *
 *      //... Initialize blk_id with the block of interest
 *
 *      //Iterate through the ports
 *      for(AtomPortId port_id : netlist.block_input_ports(blk_id)) {
 *
 *          //Iterate through the pins
 *          for(AtomPinId pin_id : netlist.port_pins(port_id)) {
 *              //Retrieve the net
 *              AtomNetId net_id = netlist.pin_net(pin_id);
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
 *      AtomBlkId blk_id;
 *
 *      //... Initialize blk_id with the block of interest
 *
 *      //Iterate over the blocks ports directly
 *      for(AtomPinId pin_id : netlist.block_input_pins(blk_id)) {
 *
 *          //Retrieve the net
 *          AtomNetId net_id = netlist.pin_net(pin_id);
 *
 *          //Get its name
 *          const std::string& net_name = netlist.net_name(net_id);
 *          printf("Associated net: %s\n", net_name.c_str());
 *      }
 *     
 * Note the use of range-based-for loops in the above examples; it could also have written (more verbosely) 
 * using a conventional for loop and explicit iterators as follows:
 *
 *      AtomBlkId blk_id;
 *
 *      //... Initialize blk_id with the block of interest
 *
 *      //Iterate over the blocks ports directly
 *      auto pins = netlist.block_input_pins(blk_id);
 *      for(auto pin_iter = pins.begin(); pin_iter != pins.end(); ++pin_iter) {
 *
 *          //Retrieve the net
 *          AtomNetId net_id = netlist.pin_net(*pin_iter);
 *
 *          //Get its name
 *          const std::string& net_name = netlist.net_name(net_id);
 *          printf("Associated net: %s\n", net_name.c_str());
 *      }
 *
 *
 * Creating the netlist
 * --------------------
 * The netlist can be created by using the create_*() member functions to create individual Blocks/Ports/Pins/Nets
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
 *      AtomNetlist netlist("my_netlist"); //Initialize the netlist with name 'my_netlist'
 *
 *      //Create the first block
 *      AtomBlockId blk1 = netlist.create_block("block_1", blk_model);
 *
 *      //Create the first block's output port
 *      //  Note that the input/output/clock type of the port is determined
 *      //  automatically from the block model
 *      AtomPortId blk1_out = netlist.create_port(blk1, "B");
 *
 *      //Create the net
 *      AtomNetId net1 = netlist.create_net("net1");
 *
 *      //Associate the net with blk1
 *      netlist.create_pin(blk1_out, 0, net_id, AtomPinType::DRIVER);
 *
 *      //Create block 2 and hook it up to net1
 *      AtomBlockId blk2 = netlist.create_block("block_2", blk_model);
 *      AtomPortId blk2_in = netlist.create_port(blk2, "A");
 *      netlist.create_pin(blk2_in, 0, net1, AtomPinType::SINK);
 *
 *      //Create block 3 and hook it up to net1
 *      AtomBlockId blk3 = netlist.create_block("block_3", blk_model);
 *      AtomPortId blk3_in = netlist.create_port(blk3, "A");
 *      netlist.create_pin(blk3_in, 0, net1, AtomPinType::SINK);
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
 * Note that until compress() is called any 'removed' elements will have invalid IDs (e.g. AtomBlockId::INVALID()).
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
 * The AtomNetlist maintains stronger invariants if the netlist is in compressed form.
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
 * Clients of this class pass nearly-opaque IDs (AtomBlockId, AtomPortId, AtomPinId, AtomNetId, AtomStringId) to retrieve
 * information. The ID is internally converted to an index to retrieve the required value from it's associated storage.
 *
 * By using nearly-opaque IDs we can change the underlying data layout as need to optimize performance/memory, without
 * disrupting client code.
 *
 * Strings
 * -------
 * To minimize memory usage, we store each unique string only once in the netlist and give it a unique ID (AtomStringId).
 * Any references to this string then make use of the AtomStringId. 
 *
 * In particular this prevents the (potentially large) strings from begin duplicated multiple times in various look-ups,
 * instead the more space efficient AtomStringId is duplicated.
 *
 * Note that AtomStringId is an internal implementation detail and should not be exposed as part of the public interface.
 * Any public functions should take and return std::string's instead.
 *
 * Block pins/Block ports data layout
 * ----------------------------------
 * The pins/ports for each block are stored in a similar manner, for brevity we describe only pins here.
 *
 * The pins for each block (i.e. AtomPinId's) are stored in a single vector for each block (the block_pins_ member).
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
 * The AtomNetlist should contain only information directly related to the netlist state (i.e. netlist connectivity).
 * Various mappings to/from atom elements (e.g. what CLB contains an atom), and algorithmic state (e.g. if a net is routed) 
 * do NOT constitute netlist state and should NOT be stored here. 
 *
 * Such implementation state should be stored in other data structures (which may reference the AtomNetlist's IDs).
 *
 * The netlist state should be immutable (i.e. read-only) for most of the CAD flow.
 * 
 */
#include <vector>
#include <unordered_map>

#include "vtr_range.h"
#include "vtr_logic.h"
#include "vtr_vector_map.h"

#include "logic_types.h" //For t_model

#include "atom_netlist_fwd.h"

#include "base_netlist.h"

//Forward declaration for private methods
template<typename I>
class IdMap;

class AtomNetlist : public BaseNetlist {
	public:
		//Constructs a netlist
		// name: the name of the netlist (e.g. top-level module)
		// id:   a unique identifier for the netlist (e.g. a secure digest of the input file)
		AtomNetlist(std::string name = "", std::string id = "");

    public: //Public types
        typedef vtr::vector_map<AtomBlockId,AtomBlockId>::const_iterator block_iterator;
        typedef vtr::vector_map<AtomPortId,AtomPortId>::const_iterator port_iterator;

        typedef std::vector<std::vector<vtr::LogicValue>> TruthTable;

        typedef vtr::Range<block_iterator> block_range;
        typedef vtr::Range<port_iterator> port_range;

    public: //Public Accessors
        /*
         * Blocks
         */
        //Returns the name of the specified block
        const std::string&  block_name          (const AtomBlockId id) const;

        //Returns the type of the specified block
        AtomBlockType       block_type          (const AtomBlockId id) const;

        //Returns the model associated with the block
        const t_model*      block_model         (const AtomBlockId id) const;

        //Returns true if the block is purely combinational (i.e. no input clocks
        //and not a primary input
        bool                block_is_combinational    (const AtomBlockId id) const;

        //Returns the truth table associated with the block
        // Note that this is only non-empty for LUTs and Flip-Flops/latches.
        //
        // For LUTs the truth table stores the single-output cover representing the
        // logic function.
        //
        // For FF/Latches there is only a single entry representing the initial state
        const TruthTable&   block_truth_table   (const AtomBlockId id) const; 

        //Returns a range of all pins assoicated with the specified block
        pin_range           block_pins          (const AtomBlockId id) const;

        //Returns a range of all input pins assoicated with the specified block
        pin_range           block_input_pins    (const AtomBlockId id) const;

        //Returns a range of all output pins assoicated with the specified block
        // Note this is typically only data pins, but some blocks (e.g. PLLs) can produce outputs
        // which are clocks.
        pin_range           block_output_pins   (const AtomBlockId id) const;

        //Returns a range of all clock pins assoicated with the specified block
        pin_range           block_clock_pins    (const AtomBlockId id) const;

        //Returns a range of all ports assoicated with the specified block
        port_range          block_ports   (const AtomBlockId id) const;

        //Returns a range consisting of the input ports associated with the specified block
        port_range          block_input_ports   (const AtomBlockId id) const;

        //Returns a range consisting of the output ports associated with the specified block
        // Note this is typically only data ports, but some blocks (e.g. PLLs) can produce outputs
        // which are clocks.
        port_range          block_output_ports  (const AtomBlockId id) const;

        //Returns a range consisting of the input clock ports associated with the specified block
        port_range          block_clock_ports   (const AtomBlockId id) const;

        /*
         * Ports
         */
        //Returns the name of the specified port
        const std::string&      port_name   (const AtomPortId id) const;

        //Returns the width (number of bits) in the specified port
        BitIndex                port_width  (const AtomPortId id) const;

        //Returns the block associated with the specified port
        AtomBlockId             port_block  (const AtomPortId id) const; 

        //Returns the type of the specified port
        AtomPortType            port_type   (const AtomPortId id) const; 

        //Returns the set of valid pins associated with the port
        pin_range               port_pins   (const AtomPortId id) const;

        //Returns the pin (potentially invalid) associated with the specified port and port bit index
        //  port_id : The ID of the associated port
        //  port_bit: The bit index of the pin in the port
        //Note: this function is a synonym for find_pin()
        AtomPinId               port_pin    (const AtomPortId port_id, const BitIndex port_bit) const;

        //Returns the net (potentially invalid) associated with the specified port and port bit index
        //  port_id : The ID of the associated port
        //  port_bit: The bit index of the pin in the port
        AtomNetId               port_net    (const AtomPortId port_id, const BitIndex port_bit) const;

        //Returns the model port of the specified port or nullptr if not
        //  port_id: The ID of the port to look for
        const t_model_ports*    port_model  (const AtomPortId port_id) const;

        /*
         * Pins
         */
        //Returns the constructed name (derived from block and port) for the specified pin
        std::string  pin_name           (const AtomPinId id) const;

        //Returns the net associated with the specified pin
        AtomNetId    pin_net            (const AtomPinId id) const; 

        //Returns the pin type of the specified pin
        AtomPinType  pin_type           (const AtomPinId id) const; 

        //Returns the port associated with the specified pin
        AtomPortId   pin_port           (const AtomPinId id) const;

        //Returns the port bit index associated with the specified pin
        BitIndex     pin_port_bit       (const AtomPinId id) const;

        //Returns the port type associated with the specified pin
        AtomPortType pin_port_type      (const AtomPinId id) const;

        //Returns the block associated with the specified pin
        AtomBlockId  pin_block          (const AtomPinId id) const;

        //Returns true if the pin is a constant (i.e. its value never changes)
        bool         pin_is_constant    (const AtomPinId id) const;

        /*
         * Nets
         */
        //Returns the name of the specified net
        const std::string&  net_name        (const AtomNetId id) const; 

        //Returns a range consisting of all the pins in the net (driver and sinks)
        //The first element in the range is the driver (and may be invalid)
        //The remaining elements (potentially none) are the sinks
        pin_range           net_pins        (const AtomNetId id) const;

        //Returns the (potentially invalid) net driver pin
        AtomPinId           net_driver      (const AtomNetId id) const;

        //Returns the (potentially invalid) net driver block
        AtomBlockId         net_driver_block(const AtomNetId id) const;

        //Returns a (potentially empty) range consisting of net's sink pins
        pin_range           net_sinks       (const AtomNetId id) const;

        //Returns true if the net is driven by a constant pin (i.e. its value never changes)
        bool                net_is_constant (const AtomNetId id) const;

        /*
         * Aggregates
         */
        //Returns a range consisting of all blocks in the netlist
        block_range blocks  () const;

        //Returns a range consisting of all pins in the netlist
        pin_range   pins    () const;

        //Returns a range consisting of all nets in the netlist
        net_range   nets    () const;
        
        /*
         * Lookups
         */
        //Returns the AtomBlockId of the specified block or AtomBlockId::INVALID() if not found
        //  name: The name of the block
        AtomBlockId find_block  (const std::string& name) const;

        //Returns the AtomPortId of the specifed port if it exists or AtomPortId::INVALID() if not
        //Note that this method is typically more efficient than searching by name
        //  blk_id: The ID of the block who's ports will be checked
        //  model_port: The port model to look for
        AtomPortId  find_port   (const AtomBlockId blk_id, const t_model_ports* model_port) const;

        //Returns the AtomPortId of the specifed port if it exists or AtomPortId::INVALID() if not
        //Note that this method is typically less efficient than searching by a t_model_port
        //  blk_id: The ID of the block who's ports will be checked
        //  name  : The name of the port to look for
        AtomPortId  find_port   (const AtomBlockId blk_id, const std::string& name) const;

        //Returns the AtomPinId of the specified pin or AtomPinId::INVALID() if not found
        //  port_id : The ID of the associated port
        //  port_bit: The bit index of the pin in the port
        AtomPinId   find_pin    (const AtomPortId port_id, BitIndex port_bit) const;

        //Returns the AtomNetId of the specified net or AtomNetId::INVALID() if not found
        //  name: The name of the net
        AtomNetId   find_net    (const std::string& name) const;

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

    public: //Public Mutators
        /*
         * Note: all create_*() functions will silently return the appropriate ID if it has already been created
         */

        //Create or return an existing block in the netlist
        //  name        : The unique name of the block
        //  model       : The primitive type of the block
        //  truth_table : The single-output cover defining the block's logic function
        //                The truth_table is optional and only relevant for LUTs (where it describes the logic function)
        //                and Flip-Flops/latches (where it consists of a single entry defining the initial state).
        AtomBlockId create_block(const std::string name, const t_model* model, const TruthTable truth_table=TruthTable());

        //Create or return an existing port in the netlist
        //  blk_id      : The block the port is associated with
        //  name        : The name of the port (must match the name of a port in the block's model)
        AtomPortId  create_port (const AtomBlockId blk_id, const t_model_ports* model_port);

        //Create or return an existing pin in the netlist
        //  port_id : The port this pin is associated with
        //  port_bit: The bit index of the pin in the port
        //  net_id  : The net the pin drives/sinks
        //  type    : The type of the pin (driver/sink)
        //  is_const: Indicates whether the pin holds a constant value (e. g. vcc/gnd)
        AtomPinId   create_pin  (const AtomPortId port_id, BitIndex port_bit, const AtomNetId net_id, const AtomPinType type, bool is_const=false);

        //Create an empty, or return an existing net in the netlist
        //  name    : The unique name of the net
        AtomNetId   create_net  (const std::string name); //An empty or existing net

        //Create a completely specified net from specified driver and sinks
        //  name    : The name of the net (Note: must not already exist)
        //  driver  : The net's driver pin
        //  sinks   : The net's sink pins
        AtomNetId   add_net     (const std::string name, AtomPinId driver, std::vector<AtomPinId> sinks);

        //Mark a pin as being a constant generator.
        // There are some cases where a pin can not be identified as a is constant until after
        // the full netlist has been built; so we expose a way to mark existing pins as constants.
        //  pin_id  : The pin to be marked
        //  value   : The boolean value to set the pin_is_constant attribute
        void set_pin_is_constant(const AtomPinId pin_id, const bool value);

        //Add the specified pin to the specified net as pin_type. Automatically removes
        //any previous net connection for this pin.
        //  pin      : The pin to add
        //  pin_type : The type of the pin (i.e. driver or sink)
        //  net      : The net to add the pin to
        void set_pin_net(const AtomPinId pin, AtomPinType pin_type, const AtomNetId net);

        /*
         * Note: all remove_*() will mark the associated items as invalid, but the items
         * will not be removed until compress() is called.
         */

        //Removes a block from the netlist. This will also remove the associated ports and pins.
        //  blk_id  : The block to be removed
        void remove_block   (const AtomBlockId blk_id);

        //Removes a connection betwen a net and pin. The pin is removed from the net and the pin
        //will be marked as having no associated net
        //  net_id  : The net from which the pin is to be removed
        //  pin_id  : The pin to be removed from the net
        void remove_net_pin (const AtomNetId net_id, const AtomPinId pin_id);

        //Compresses the netlist, removing any invalid and/or unreferenced
        //blocks/ports/pins/nets.
        //
        //This should be called after completing a series of netlist modifications 
        //(e.g. removing blocks/ports/pins/nets).
        //
        //NOTE: this invalidates all existing IDs!
        void compress();

    private: //Private types
        //A unique identifier for a string in the atom netlist
        typedef vtr::StrongId<BaseNetlist::string_id_tag> AtomStringId;

    private: //Private members
        /*
         * Lookups
         */
        //Returns the AtomStringId of the specifed string if it exists or AtomStringId::INVALID() if not
        //  str : The string to look for
        AtomStringId find_string(const std::string& str) const;

        //Returns the AtomBlockId of the specifed block if it exists or AtomBlockId::INVALID() if not
        //  name_id : The block name to look for
        AtomBlockId find_block(const AtomStringId name_id) const;

        /*
         * Mutators
         */
        //Updates net cross-references for the specified pin
        void associate_pin_with_net(const AtomPinId pin_id, const AtomPinType type, const AtomNetId net_id); 

        //Updates port cross-references for the specified pin
        void associate_pin_with_port(const AtomPinId pin_id, const AtomPortId port_id); 

        //Updates block cross-references for the specified pin
        void associate_pin_with_block(const AtomPinId pin_id, const AtomPortType type, const AtomBlockId blk_id); 

        //Updates block cross-references for the specified port
        void associate_port_with_block(const AtomPortId port_id, const AtomBlockId blk_id);

        //Removes a port from the netlist.
        //The port's pins are also marked invalid and removed from any associated nets
        //  port_id: The ID of the port to be removed
        void remove_port(const AtomPortId port_id);

        //Removes a pin from the netlist.
        //The pin is marked invalid, and removed from any assoicated nets
        //  pin_id: The ID of the pin to be removed
        void remove_pin(const AtomPinId pin_id);

        //Marks netlist components which have become redundant due to other removals
        //(e.g. ports with only invalid pins) as invalid so they will be destroyed during
        //compress()
        void remove_unused();

        /*
         * Netlist compression/optimization
         */
        //Builds the new mappings from old to new IDs.
        //The various IdMap's should be initialized with invalid mappings
        //for all current ID's before being called.
        void build_id_maps(vtr::vector_map<AtomBlockId,AtomBlockId>& block_id_map, 
                           vtr::vector_map<AtomPortId,AtomPortId>& port_id_map, 
                           vtr::vector_map<AtomPinId,AtomPinId>& pin_id_map, 
                           vtr::vector_map<AtomNetId,AtomNetId>& net_id_map);

        //Removes invalid and reorders blocks
        void clean_blocks(const vtr::vector_map<AtomBlockId,AtomBlockId>& block_id_map);

        //Removes invalid and reorders ports
        void clean_ports(const vtr::vector_map<AtomPortId,AtomPortId>& port_id_map);

        //Removes invalid and reorders pins
        void clean_pins(const vtr::vector_map<AtomPinId,AtomPinId>& pin_id_map);

        //Removes invalid and reorders nets
        void clean_nets(const vtr::vector_map<AtomNetId,AtomNetId>& net_id_map);

        //Re-builds cross-references held by blocks
        void rebuild_block_refs(const vtr::vector_map<AtomPinId,AtomPinId>& pin_id_map, 
                                const vtr::vector_map<AtomPortId,AtomPortId>& port_id_map);

        //Re-builds cross-references held by ports
        void rebuild_port_refs(const vtr::vector_map<AtomBlockId,AtomBlockId>& block_id_map, 
                               const vtr::vector_map<AtomPinId,AtomPinId>& pin_id_map);

        //Re-builds cross-references held by pins
        void rebuild_pin_refs(const vtr::vector_map<AtomPortId,AtomPortId>& port_id_map, 
                              const vtr::vector_map<AtomNetId,AtomNetId>& net_id_map);

        //Re-builds cross-references held by nets
        void rebuild_net_refs(const vtr::vector_map<AtomPinId,AtomPinId>& pin_id_map);

        //Re-builds fast look-ups
        void rebuild_lookups();

        //Shrinks internal data structures to required size to reduce memory consumption
        void shrink_to_fit();

        /*
         * Sanity Checks
         */
        //Verify the internal data structure sizes match
        bool verify_sizes() const; //All data structures
        bool validate_block_sizes() const;
        bool validate_port_sizes() const;
        bool validate_pin_sizes() const;

        //Verify that internal data structure cross-references are consistent
        bool verify_refs() const; //All cross-references
        bool validate_block_port_refs() const;
        bool validate_block_pin_refs() const;
        bool validate_port_pin_refs() const;
        bool validate_net_pin_refs() const;
		bool validate_string_refs() const;

        //Verify that fast-lookups are consistent with internal data structures
        bool verify_lookups() const;
        
        //Verify that block invariants hold (i.e. logical consistency)
        bool verify_block_invariants() const;

        //Validates that the specified ID is valid in the current netlist state
        bool valid_block_id(AtomBlockId id) const;
        bool valid_port_id(AtomPortId id) const;
        bool valid_port_bit(AtomPortId id, BitIndex port_bit) const;
        bool valid_pin_id(AtomPinId id) const;
        bool valid_net_id(AtomNetId id) const;
        bool valid_string_id(AtomStringId id) const;

    private: //Private data

        //Netlist data
        bool                        dirty_;         //Indicates the netlist has invalid entries from remove_*() functions

        //Block data
        vtr::vector_map<AtomBlockId,AtomBlockId>             block_ids_;                //Valid block ids
        vtr::vector_map<AtomBlockId,AtomStringId>            block_names_;              //Name of each block
        vtr::vector_map<AtomBlockId,const t_model*>          block_models_;             //Architecture model of each block
        vtr::vector_map<AtomBlockId,TruthTable>              block_truth_tables_;       //Truth tables of each block

        vtr::vector_map<AtomBlockId,std::vector<AtomPinId>>  block_pins_;               //Pins of each block
        vtr::vector_map<AtomBlockId,unsigned>                block_num_input_pins_;     //Number of input pins on each block
        vtr::vector_map<AtomBlockId,unsigned>                block_num_output_pins_;    //Number of output pins on each block
        vtr::vector_map<AtomBlockId,unsigned>                block_num_clock_pins_;     //Number of clock pins on each block

        vtr::vector_map<AtomBlockId,std::vector<AtomPortId>> block_ports_;              //Ports of each block
        vtr::vector_map<AtomBlockId,unsigned>                block_num_input_ports_;    //Input ports of each block
        vtr::vector_map<AtomBlockId,unsigned>                block_num_output_ports_;   //Output ports of each block
        vtr::vector_map<AtomBlockId,unsigned>                block_num_clock_ports_;    //Clock ports of each block

        //Port data
        vtr::vector_map<AtomPortId,AtomPortId>             port_ids_;      //Valid port ids
        vtr::vector_map<AtomPortId,AtomStringId>           port_names_;    //Name of each port
        vtr::vector_map<AtomPortId,AtomBlockId>            port_blocks_;   //Block associated with each port
        vtr::vector_map<AtomPortId,const t_model_ports*>   port_models_;   //Architecture port models of each port
        vtr::vector_map<AtomPortId,std::vector<AtomPinId>> port_pins_;     //Pins associated with each port

        //Pin data
//        vtr::vector_map<AtomPinId,AtomPinId>  pin_ids_;           //Valid pin ids
        vtr::vector_map<AtomPinId,AtomPortId> pin_ports_;         //Type of each pin
        vtr::vector_map<AtomPinId,BitIndex>   pin_port_bits_;     //The ports bit position in the port
//        vtr::vector_map<AtomPinId,AtomNetId>  pin_nets_;          //Net associated with each pin
//        vtr::vector_map<AtomPinId,bool>       pin_is_constant_;   //Indicates if the pin always keeps a constant value

    private: //Fast lookups
        vtr::vector_map<AtomStringId,AtomBlockId>       block_name_to_block_id_;
};

#include "atom_lookup.h"

#endif
