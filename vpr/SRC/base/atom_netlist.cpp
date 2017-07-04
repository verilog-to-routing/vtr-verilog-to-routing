#include <algorithm>
#include <unordered_set>
#include <queue>
#include <numeric>

#include "atom_netlist.h"

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_error.h"

/*
 *
 *
 * Utility templates
 *
 *
 */

//Returns true if all elements are contiguously ascending values (i.e. equal to their index)
template<typename T>
bool are_contiguous(vtr::vector_map<T,T>& values) {
    size_t i = 0;
    for(T val : values) {
        if (val != T(i)) {
            return false;
        }
        ++i;
    }
    return true;
}

//Returns true if all elements in the vector 'values' evaluate true
template<typename Container>
bool all_valid(const Container& values) {
    for(auto val : values) {
        if(!val) {
            return false;
        }
    }
    return true;
}

//Builds a mapping from old to new ids by skipping values marked invalid
template<typename Id>
vtr::vector_map<Id,Id> compress_ids(const vtr::vector_map<Id,Id>& ids) {
    vtr::vector_map<Id,Id> id_map(ids.size());
    size_t i = 0;
    for(auto id : ids) {
        if(id) {
            //Valid
            id_map.insert(id, Id(i));
            ++i;
        }
    }

    return id_map;
}

//Returns a vector based on 'values', which has had entries dropped and 
//re-ordered according according to 'id_map'.
//
// Each entry in id_map corresponds to the assoicated element in 'values'.
// The value of the id_map entry is the new ID of the entry in values.
//
// If it is an invalid ID, the element in values is dropped.
// Otherwise the element is moved to the new ID location.
template<typename Id, typename T>
vtr::vector_map<Id,T> clean_and_reorder_values(const vtr::vector_map<Id,T>& values, const vtr::vector_map<Id,Id>& id_map) {
    VTR_ASSERT(values.size() == id_map.size());

    //Allocate space for the values that will not be dropped
    vtr::vector_map<Id,T> result;

    //Move over the valid entries to their new locations
    for(size_t cur_idx = 0; cur_idx < values.size(); ++cur_idx) {
        Id old_id = Id(cur_idx);

        Id new_id = id_map[old_id];
        if (new_id) {
            //There is a valid mapping
            result.insert(new_id, std::move(values[old_id]));
        }
    }

    return result;
}

//Returns the set of new valid Ids defined by 'id_map'
//TOOD: merge with clean_and_reorder_values
template<typename Id>
vtr::vector_map<Id,Id> clean_and_reorder_ids(const vtr::vector_map<Id,Id>& id_map) {
    //For IDs, the values are the new id's stored in the map

    //Allocate a new vector to store the values that have been not dropped
    vtr::vector_map<Id,Id> result;

    //Move over the valid entries to their new locations
    for(size_t cur_idx = 0; cur_idx < id_map.size(); ++cur_idx) {
        Id old_id = Id(cur_idx);

        Id new_id = id_map[old_id];
        if (new_id) {
            result.insert(new_id, new_id);
        }
    }

    return result;
}

//Count how many of the Id's referenced in 'range' have a valid
//new mapping in 'id_map'
template<typename R, typename Id>
size_t count_valid_refs(R range, const vtr::vector_map<Id,Id>& id_map) {
    size_t valid_count = 0;

    for(Id old_id : range) {
        if(id_map[old_id]) {
            ++valid_count;
        }
    }

    return valid_count;
}

//Updates the Ids in 'values' based on id_map, even if the original or new mapping is not valid
template<typename Container, typename ValId>
Container update_all_refs(const Container& values, const vtr::vector_map<ValId,ValId>& id_map) {
    Container updated;

    for(ValId orig_val : values) {
        //The original item was valid
        ValId new_val = id_map[orig_val]; 
        //The original item exists in the new mapping
        updated.emplace_back(new_val);
    }

    return updated;
}

template<typename Container, typename ValId>
Container update_valid_refs(const Container& values, const vtr::vector_map<ValId,ValId>& id_map) {
    Container updated;

    for(ValId orig_val : values) {
        if(orig_val) {
            //Original item valid

            ValId new_val = id_map[orig_val];
            if(new_val) {
                //The original item exists in the new mapping
                updated.emplace_back(new_val);
            }
        }
    }
    return updated;
}

/*
 *
 *
 * AtomNetlist Class Implementation
 *
 *
 */

AtomNetlist::AtomNetlist(std::string name, std::string id)
    : BaseNetlist(name, id)
    , dirty_(false) {}

/*
 *
 * Netlist
 *
 */

bool AtomNetlist::is_dirty() const {
    return dirty_;
}

bool AtomNetlist::is_compressed() const {
    return !is_dirty();
}

void AtomNetlist::print_stats() const {
    vtr::printf_info("Blocks  %zu capacity/size: %.2f\n", block_ids_.size(), float(block_ids_.capacity()) / block_ids_.size());
    vtr::printf_info("Ports   %zu capacity/size: %.2f\n", port_ids_.size(), float(port_ids_.capacity()) / port_ids_.size());
    vtr::printf_info("Pins    %zu capacity/size: %.2f\n", pin_ids_.size(), float(pin_ids_.capacity()) / pin_ids_.size());
    vtr::printf_info("Nets    %zu capacity/size: %.2f\n", net_ids_.size(), float(net_ids_.capacity()) / net_ids_.size());
    vtr::printf_info("Strings %zu capacity/size: %.2f\n", string_ids_.size(), float(string_ids_.capacity()) / string_ids_.size());
}

/*
 *
 * Blocks
 *
 */
const std::string& AtomNetlist::block_name (const AtomBlockId id) const { 
	return BaseNetlist::block_name(id);
}

AtomBlockType AtomNetlist::block_type (const AtomBlockId id) const {
	return (AtomBlockType) BaseNetlist::block_type(id);
}

const t_model* AtomNetlist::block_model (const AtomBlockId id) const {
	return BaseNetlist::block_model(id);
}

const AtomNetlist::TruthTable& AtomNetlist::block_truth_table (const AtomBlockId id) const {
    VTR_ASSERT(valid_block_id(id));

    return block_truth_tables_[id];
}

/*
bool  AtomNetlist::block_is_combinational (const AtomBlockId id) const {
	return BaseNetlist::block_is_combinational(id);
}
*/

AtomNetlist::pin_range AtomNetlist::block_pins (const AtomBlockId id) const {
	return BaseNetlist::block_pins(id);
}

AtomNetlist::pin_range AtomNetlist::block_input_pins (const AtomBlockId id) const {
	return BaseNetlist::block_input_pins(id);
}

AtomNetlist::pin_range AtomNetlist::block_output_pins (const AtomBlockId id) const {
	return BaseNetlist::block_output_pins(id);
}

AtomNetlist::pin_range AtomNetlist::block_clock_pins (const AtomBlockId id) const {
	return BaseNetlist::block_clock_pins(id);
}

AtomNetlist::port_range AtomNetlist::block_ports (const AtomBlockId id) const {
	return BaseNetlist::block_ports(id);
}

AtomNetlist::port_range AtomNetlist::block_input_ports (const AtomBlockId id) const {
	return BaseNetlist::block_input_ports(id);
}

AtomNetlist::port_range AtomNetlist::block_output_ports (const AtomBlockId id) const {
	return BaseNetlist::block_output_ports(id);
}

AtomNetlist::port_range AtomNetlist::block_clock_ports (const AtomBlockId id) const {
	return BaseNetlist::block_clock_ports(id);
}

/*
 *
 * Ports
 *
 */
const std::string& AtomNetlist::port_name (const AtomPortId id) const {
	return BaseNetlist::port_name(id);
}

BitIndex AtomNetlist::port_width (const AtomPortId id) const {
	return BaseNetlist::port_width(id);
}

AtomPortType AtomNetlist::port_type (const AtomPortId id) const {
	return (AtomPortType) BaseNetlist::port_type(id);
}

AtomBlockId AtomNetlist::port_block (const AtomPortId id) const {
	return BaseNetlist::port_block(id);
}

AtomNetlist::pin_range AtomNetlist::port_pins (const AtomPortId id) const {
    return BaseNetlist::port_pins(id);
}

AtomPinId AtomNetlist::port_pin (const AtomPortId port_id, const BitIndex port_bit) const {
	return BaseNetlist::port_pin(port_id, port_bit);
}

AtomNetId AtomNetlist::port_net (const AtomPortId port_id, const BitIndex port_bit) const {
	return BaseNetlist::port_net(port_id, port_bit);
}

const t_model_ports* AtomNetlist::port_model (const AtomPortId port_id) const {
	return BaseNetlist::port_model(port_id);
}

/*
 *
 * Pins
 *
 */
std::string AtomNetlist::pin_name (const AtomPinId id) const {
	return BaseNetlist::pin_name(id);
}

AtomNetId AtomNetlist::pin_net (const AtomPinId id) const { 
    return BaseNetlist::pin_net(id);
}

AtomPinType AtomNetlist::pin_type (const AtomPinId id) const { 
	return (AtomPinType) BaseNetlist::pin_type(id);
}

AtomPortId AtomNetlist::pin_port (const AtomPinId id) const { 
	return BaseNetlist::pin_port(id);
}

BitIndex AtomNetlist::pin_port_bit (const AtomPinId id) const {
	return BaseNetlist::pin_port_bit(id);
}

AtomPortType AtomNetlist::pin_port_type (const AtomPinId id) const {
	return (AtomPortType) BaseNetlist::pin_port_type(id);
}

AtomBlockId AtomNetlist::pin_block (const AtomPinId id) const { 
	return BaseNetlist::pin_block(id);
}

bool AtomNetlist::pin_is_constant (const AtomPinId id) const {
	return BaseNetlist::pin_is_constant(id);
}


/*
 *
 * Nets
 *
 */
const std::string& AtomNetlist::net_name (const AtomNetId id) const { 
	return BaseNetlist::net_name(id);
}

AtomNetlist::pin_range AtomNetlist::net_pins (const AtomNetId id) const {
	return BaseNetlist::net_pins(id);
}

AtomPinId AtomNetlist::net_driver (const AtomNetId id) const {
	return BaseNetlist::net_driver(id);
}

AtomBlockId AtomNetlist::net_driver_block (const AtomNetId id) const {
	return BaseNetlist::net_driver_block(id);
}

AtomNetlist::pin_range AtomNetlist::net_sinks (const AtomNetId id) const {
	return BaseNetlist::net_sinks(id);
}

bool AtomNetlist::net_is_constant (const AtomNetId id) const {
	return BaseNetlist::net_is_constant(id);
}

/*
 *
 * Aggregates
 *
 */
AtomNetlist::block_range AtomNetlist::blocks () const {
	return BaseNetlist::blocks();
}

AtomNetlist::pin_range AtomNetlist::pins () const {
	return BaseNetlist::pins();
}

AtomNetlist::net_range AtomNetlist::nets () const {
	return BaseNetlist::nets();
}


/*
 *
 * Lookups
 *
 */
AtomPortId AtomNetlist::find_port (const AtomBlockId blk_id, const t_model_ports* model_port) const {
	return BaseNetlist::find_port((BlockId) blk_id, model_port);
}

AtomPortId AtomNetlist::find_port (const AtomBlockId blk_id, const std::string& name) const {
	return BaseNetlist::find_port((BlockId) blk_id, name);
}

AtomPinId AtomNetlist::find_pin (const AtomPortId port_id, BitIndex port_bit) const {
	return BaseNetlist::find_pin(port_id, port_bit);
}

AtomNetId AtomNetlist::find_net (const std::string& name) const {
	return BaseNetlist::find_net(name);
}

/*
 *
 * Validation
 *
 */
bool AtomNetlist::verify() const {
    bool valid = true;

    //Verify data structure consistency
    valid &= verify_sizes();
    valid &= verify_refs();
    valid &= verify_lookups();

    //Verify logical consistency
    valid &= verify_block_invariants();

    return valid;
}

//Checks that the sizes of internal data structures
//are consistent. Should take constant time.
bool AtomNetlist::verify_sizes() const {
    bool valid = true;
    valid &= validate_block_sizes();
    valid &= validate_port_sizes();
    valid &= validate_pin_sizes();
    valid &= validate_net_sizes();
    valid &= validate_string_sizes();
    return valid;
}

bool AtomNetlist::verify_block_invariants() const {
    for(auto blk_id : blocks()) {

        {
            //Check block type

            //Find any connected clock
            AtomNetId clk_net_id;
            for(auto pin_id : block_clock_pins(blk_id)) {
                if(pin_id) {
                    clk_net_id = pin_net(pin_id);
                    break;
                }
            }

            if(block_is_combinational(blk_id)) {
                //Non-sequential types must not have a clock
                if(clk_net_id) {
                    VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Atom block '%s' is a non-sequential type but has a clock '%s'", 
                              block_name(blk_id).c_str(), net_name(clk_net_id).c_str());
                }

            } else {
                //Sequential types must have a clock
                if(!clk_net_id) {
                    VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Atom block '%s' is sequential type but has no clock", 
                              block_name(blk_id).c_str());
                }

            }
        }
        {
            //Check that block pins and ports are consistent (i.e. same number of pins)
            size_t num_block_pins = block_pins(blk_id).size();

            size_t total_block_port_pins = 0;
            for(auto port_id : block_ports(blk_id)) {
                total_block_port_pins += port_pins(port_id).size(); 
            }

            if(num_block_pins != total_block_port_pins) {
                VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Block pins and port pins do not match on atom block '%s'",
                        block_name(blk_id).c_str());
            }
        }
    }
    return true;
}

/*
 *
 * Mutators
 *
 */
AtomBlockId AtomNetlist::create_block(const std::string name, const t_model* model, const TruthTable truth_table) {
    //Must have a non-empty name
    VTR_ASSERT_MSG(!name.empty(), "Non-Empty block name");

    //Check if the block has already been created
    AtomStringId name_id = BaseNetlist::create_string(name);
    AtomBlockId blk_id = BaseNetlist::find_block(name_id);

    if(blk_id == AtomBlockId::INVALID()) {
        //Not found, create it

        //Reserve an id
        blk_id = AtomBlockId(block_ids_.size());
        block_ids_.push_back(blk_id);

        //Initialize the data
        block_names_.push_back(name_id);
        block_models_.push_back(model);
        block_truth_tables_.push_back(truth_table);

        //Initialize the look-ups
        block_name_to_block_id_.insert(name_id, blk_id);

        block_pins_.emplace_back();
        block_num_input_pins_.push_back(0);
        block_num_output_pins_.push_back(0);
        block_num_clock_pins_.push_back(0);

        block_ports_.emplace_back();
        block_num_input_ports_.push_back(0);
        block_num_output_ports_.push_back(0);
        block_num_clock_ports_.push_back(0);
    }

    //Check post-conditions: size
    VTR_ASSERT(validate_block_sizes());

    //Check post-conditions: values
    VTR_ASSERT(valid_block_id(blk_id));
    VTR_ASSERT(block_name(blk_id) == name);
    VTR_ASSERT(block_model(blk_id) == model);
    VTR_ASSERT(block_truth_table(blk_id) == truth_table);
    VTR_ASSERT_SAFE(find_block(name) == blk_id);

    return blk_id;
}

AtomPortId AtomNetlist::create_port (const AtomBlockId blk_id, const t_model_ports* model_port) {
	return BaseNetlist::create_port(blk_id, model_port);
}

AtomPinId AtomNetlist::create_pin (const AtomPortId port_id, BitIndex port_bit, const AtomNetId net_id, const AtomPinType type, bool is_const) {
	return BaseNetlist::create_pin(port_id, port_bit, net_id, (PinType) type, is_const);
}

AtomNetId AtomNetlist::create_net (const std::string name) {
	return BaseNetlist::create_net(name);
}

AtomNetId AtomNetlist::add_net (const std::string name, AtomPinId driver, std::vector<AtomPinId> sinks) {
	return BaseNetlist::add_net(name, driver, sinks);
}

void AtomNetlist::set_pin_is_constant(const AtomPinId pin_id, const bool value) {
	return BaseNetlist::set_pin_is_constant(pin_id, value);
}

void AtomNetlist::set_pin_net (const AtomPinId pin, AtomPinType type, const AtomNetId net) {
	BaseNetlist::set_pin_net(pin, (PinType) type, net);
}

void AtomNetlist::remove_block(const AtomBlockId blk_id) {
	BaseNetlist::remove_block(blk_id);
}

void AtomNetlist::remove_port(const AtomPortId port_id) {
	BaseNetlist::remove_port(port_id);
}

void AtomNetlist::remove_pin(const AtomPinId pin_id) {
	BaseNetlist::remove_pin(pin_id);
}

void AtomNetlist::remove_net_pin(const AtomNetId net_id, const AtomPinId pin_id) {
	BaseNetlist::remove_net_pin(net_id, pin_id);
}

void AtomNetlist::remove_unused() {
	BaseNetlist::remove_unused();
}

void AtomNetlist::compress() {
    //Compress the various netlist components to remove invalid entries
    // Note: this invalidates all Ids

    //Walk the netlist to invalidate any unused items
    remove_unused();

    vtr::vector_map<AtomBlockId,AtomBlockId> block_id_map(block_ids_.size());
    vtr::vector_map<AtomPortId,AtomPortId> port_id_map(port_ids_.size());
    vtr::vector_map<AtomPinId,AtomPinId> pin_id_map(pin_ids_.size());
    vtr::vector_map<AtomNetId,AtomNetId> net_id_map(net_ids_.size());

    //Build the mappings from old to new id's, potentially
    //re-ordering for improved cache locality
    //
    //The vecotrs passed as parameters are initialized as a mapping from old to new index
    // e.g. block_id_map[old_id] == new_id
    build_id_maps(block_id_map, port_id_map, pin_id_map, net_id_map);

    //The clean_*() functions return a vector which maps from old to new index
    // e.g. block_id_map[old_id] == new_id
    clean_nets(net_id_map);
    clean_pins(pin_id_map);
    clean_ports(port_id_map);
    clean_blocks(block_id_map);
    //TODO: clean strings
    //TODO: iterative cleaning?

    //Now we re-build all the cross references
    rebuild_block_refs(pin_id_map, port_id_map);
    rebuild_port_refs(block_id_map, pin_id_map);
    rebuild_pin_refs(port_id_map, net_id_map);
    rebuild_net_refs(pin_id_map);

    //Re-build the lookups
    rebuild_lookups();

    //Resize containers to exact size
    shrink_to_fit();

    //Netlist is now clean
    dirty_ = false;
}

void AtomNetlist::build_id_maps(vtr::vector_map<AtomBlockId,AtomBlockId>& block_id_map, 
                                vtr::vector_map<AtomPortId,AtomPortId>& port_id_map, 
                                vtr::vector_map<AtomPinId,AtomPinId>& pin_id_map, 
                                vtr::vector_map<AtomNetId,AtomNetId>& net_id_map) {
    block_id_map = compress_ids(block_ids_);
    port_id_map = compress_ids(port_ids_);
    pin_id_map = compress_ids(pin_ids_);
    net_id_map = compress_ids(net_ids_);
}

void AtomNetlist::clean_blocks(const vtr::vector_map<AtomBlockId,AtomBlockId>& block_id_map) {
    //Clean the blocks

    //Update all the block values
    block_ids_ = clean_and_reorder_ids(block_id_map);
    block_names_ = clean_and_reorder_values(block_names_, block_id_map);
    block_models_ = clean_and_reorder_values(block_models_, block_id_map);
    block_truth_tables_ = clean_and_reorder_values(block_truth_tables_, block_id_map);

    block_pins_ = clean_and_reorder_values(block_pins_, block_id_map);
    block_num_input_pins_ = clean_and_reorder_values(block_num_input_pins_, block_id_map);
    block_num_output_pins_ = clean_and_reorder_values(block_num_output_pins_, block_id_map);
    block_num_clock_pins_ = clean_and_reorder_values(block_num_clock_pins_, block_id_map);

    block_ports_ = clean_and_reorder_values(block_ports_, block_id_map);
    block_num_input_ports_ = clean_and_reorder_values(block_num_input_ports_, block_id_map);
    block_num_output_ports_ = clean_and_reorder_values(block_num_output_ports_, block_id_map);
    block_num_clock_ports_ = clean_and_reorder_values(block_num_clock_ports_, block_id_map);

    VTR_ASSERT(validate_block_sizes());

    VTR_ASSERT_MSG(are_contiguous(block_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(block_ids_), "All Ids should be valid");
}

void AtomNetlist::clean_ports(const vtr::vector_map<AtomPortId,AtomPortId>& port_id_map) {
    //Clean the ports

    //Update all the port values
    port_ids_ = clean_and_reorder_ids(port_id_map);
    port_names_ = clean_and_reorder_values(port_names_, port_id_map);
    port_blocks_ = clean_and_reorder_values(port_blocks_, port_id_map);
    port_models_ = clean_and_reorder_values(port_models_, port_id_map);
    port_pins_ = clean_and_reorder_values(port_pins_, port_id_map);

    VTR_ASSERT(validate_port_sizes());

    VTR_ASSERT_MSG(are_contiguous(port_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(port_ids_), "All Ids should be valid");
}

void AtomNetlist::clean_pins(const vtr::vector_map<AtomPinId,AtomPinId>& pin_id_map) {
    //Clean the pins

    //Update all the pin values
    pin_ids_ = clean_and_reorder_ids(pin_id_map);
    pin_ports_ = clean_and_reorder_values(pin_ports_, pin_id_map);
    pin_port_bits_ = clean_and_reorder_values(pin_port_bits_, pin_id_map);
    pin_nets_ = clean_and_reorder_values(pin_nets_, pin_id_map);
    pin_is_constant_ = clean_and_reorder_values(pin_is_constant_, pin_id_map);

    VTR_ASSERT(validate_pin_sizes());

    VTR_ASSERT_MSG(are_contiguous(pin_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(pin_ids_), "All Ids should be valid");
}

void AtomNetlist::clean_nets(const vtr::vector_map<AtomNetId,AtomNetId>& net_id_map) {
    //Clean the nets

    //Update all the net values
    net_ids_ = clean_and_reorder_ids(net_id_map);
    net_names_ = clean_and_reorder_values(net_names_, net_id_map);
    net_pins_ = clean_and_reorder_values(net_pins_, net_id_map);

    VTR_ASSERT(validate_net_sizes());

    VTR_ASSERT_MSG(are_contiguous(net_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(net_ids_), "All Ids should be valid");
}

void AtomNetlist::rebuild_block_refs(const vtr::vector_map<AtomPinId,AtomPinId>& pin_id_map, 
                                     const vtr::vector_map<AtomPortId,AtomPortId>& port_id_map) {
    //Update the pin id references held by blocks
    for(auto blk_id : blocks()) {
        //Before update the references, we need to know how many are valid,
        //so we can also update the numbers of input/output/clock pins

        //Note that we take special care to not modify the pin counts until after
        //they have all been counted (since the block_*_pins() functions depend on
        //the counts).
        size_t num_input_pins = count_valid_refs(block_input_pins(blk_id), pin_id_map);
        size_t num_output_pins = count_valid_refs(block_output_pins(blk_id), pin_id_map);
        size_t num_clock_pins = count_valid_refs(block_clock_pins(blk_id), pin_id_map);

        std::vector<AtomPinId>& pin_collection = block_pins_[blk_id];

        pin_collection = update_valid_refs(pin_collection, pin_id_map);
        block_num_input_pins_[blk_id] = num_input_pins;
        block_num_output_pins_[blk_id] = num_output_pins;
        block_num_clock_pins_[blk_id] = num_clock_pins;

        VTR_ASSERT_SAFE_MSG(all_valid(pin_collection), "All Ids should be valid");
        VTR_ASSERT(pin_collection.size() == (size_t) block_num_input_pins_[blk_id]
                                                   + block_num_output_pins_[blk_id]
                                                   + block_num_clock_pins_[blk_id]);

        //Similarily for ports
        size_t num_input_ports = count_valid_refs(block_input_ports(blk_id), port_id_map);
        size_t num_output_ports = count_valid_refs(block_output_ports(blk_id), port_id_map);
        size_t num_clock_ports = count_valid_refs(block_clock_ports(blk_id), port_id_map);

        std::vector<AtomPortId>& ports = block_ports_[blk_id];

        ports = update_valid_refs(ports, port_id_map);
        block_num_input_ports_[blk_id] = num_input_ports;
        block_num_output_ports_[blk_id] = num_output_ports;
        block_num_clock_ports_[blk_id] = num_clock_ports;

        VTR_ASSERT_SAFE_MSG(all_valid(ports), "All Ids should be valid");
        VTR_ASSERT(ports.size() == (size_t) block_num_input_ports_[blk_id]
                                           + block_num_output_ports_[blk_id]
                                           + block_num_clock_ports_[blk_id]);
    }

    VTR_ASSERT(validate_block_sizes());
}

void AtomNetlist::rebuild_port_refs(const vtr::vector_map<AtomBlockId,AtomBlockId>& block_id_map, 
                                    const vtr::vector_map<AtomPinId,AtomPinId>& pin_id_map) {
    //Update block and pin references held by ports
    port_blocks_ = update_valid_refs(port_blocks_, block_id_map); 
    VTR_ASSERT_SAFE_MSG(all_valid(port_blocks_), "All Ids should be valid");

    VTR_ASSERT(port_blocks_.size() == port_ids_.size());

    for(auto& pin_collection : port_pins_) {
        pin_collection = update_valid_refs(pin_collection, pin_id_map);
        VTR_ASSERT_SAFE_MSG(all_valid(pin_collection), "All Ids should be valid");
    }
    VTR_ASSERT(validate_port_sizes());
}

void AtomNetlist::rebuild_pin_refs(const vtr::vector_map<AtomPortId,AtomPortId>& port_id_map, 
                                   const vtr::vector_map<AtomNetId,AtomNetId>& net_id_map) {
    //Update port and net references held by pins
    pin_ports_ = update_all_refs(pin_ports_, port_id_map);
    VTR_ASSERT_SAFE_MSG(all_valid(pin_ports_), "All Ids should be valid");

    pin_nets_ = update_all_refs(pin_nets_, net_id_map);
    VTR_ASSERT_SAFE_MSG(all_valid(pin_nets_), "All Ids should be valid");

    VTR_ASSERT(validate_pin_sizes());
}

void AtomNetlist::rebuild_net_refs(const vtr::vector_map<AtomPinId,AtomPinId>& pin_id_map) {
    //Update pin references held by nets
    for(auto& pin_collection : net_pins_) {
        pin_collection = update_valid_refs(pin_collection, pin_id_map);

        VTR_ASSERT_SAFE_MSG(all_valid(pin_collection), "All sinks should be valid");
    }
    VTR_ASSERT(validate_net_sizes());
}

void AtomNetlist::rebuild_lookups() {
    //We iterate through the reverse-lookups and update the values (i.e. ids)
    //to the new id values

    //Blocks
    block_name_to_block_id_.clear();
    for(auto blk_id : blocks()) {
        const auto& key = block_names_[blk_id];
        block_name_to_block_id_.insert(key, blk_id);
    }

    //Nets
    net_name_to_net_id_.clear();
    for(auto net_id : nets()) {
        const auto& key = net_names_[net_id];
        net_name_to_net_id_.insert(key, net_id);
    }
}

void AtomNetlist::shrink_to_fit() {
    //Block data
    block_ids_.shrink_to_fit();
    block_names_.shrink_to_fit();
    block_models_.shrink_to_fit();
    block_truth_tables_.shrink_to_fit();

    block_pins_.shrink_to_fit();
    for(std::vector<AtomPinId>& pin_collection : block_pins_) {
        pin_collection.shrink_to_fit();
    }
    block_num_input_pins_.shrink_to_fit();
    block_num_output_pins_.shrink_to_fit();
    block_num_clock_pins_.shrink_to_fit();

    block_ports_.shrink_to_fit();
    for(auto& ports : block_ports_) {
        ports.shrink_to_fit();
    }
    block_num_input_ports_.shrink_to_fit();
    block_num_output_ports_.shrink_to_fit();
    block_num_clock_ports_.shrink_to_fit();

    VTR_ASSERT(validate_block_sizes());

    //Port data
    port_ids_.shrink_to_fit();
    port_blocks_.shrink_to_fit();
    port_models_.shrink_to_fit();
    port_pins_.shrink_to_fit();
    for(auto& pin_collection : port_pins_) {
        pin_collection.shrink_to_fit();
    }
    VTR_ASSERT(validate_port_sizes());

    //Pin data
    pin_ids_.shrink_to_fit();
    pin_ports_.shrink_to_fit();
    pin_port_bits_.shrink_to_fit();
    pin_nets_.shrink_to_fit();
    pin_is_constant_.shrink_to_fit();
    VTR_ASSERT(validate_pin_sizes());

    //Net data
    net_ids_.shrink_to_fit();
    net_names_.shrink_to_fit();
    net_pins_.shrink_to_fit();
    VTR_ASSERT(validate_net_sizes());

    //String data
    string_ids_.shrink_to_fit();
    strings_.shrink_to_fit();
    VTR_ASSERT(validate_string_sizes());
}

/*
 *
 * Sanity Checks
 *
 */
bool AtomNetlist::validate_block_sizes() const {
	if(block_truth_tables_.size() != block_ids_.size()) {
        VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent block data sizes");
    }
    return BaseNetlist::validate_block_sizes();
}
