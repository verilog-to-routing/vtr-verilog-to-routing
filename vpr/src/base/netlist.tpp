#include <algorithm>
#include <numeric>

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vpr_error.h"
/*
 *
 * NetlistIdRemapper class implementation
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId NetlistIdRemapper<BlockId, PortId, PinId, NetId>::new_block_id(BlockId old_id) const {
    return block_id_map_[old_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PortId NetlistIdRemapper<BlockId, PortId, PinId, NetId>::new_port_id(PortId old_id) const {
    return port_id_map_[old_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinId NetlistIdRemapper<BlockId, PortId, PinId, NetId>::new_pin_id(PinId old_id) const {
    return pin_id_map_[old_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId NetlistIdRemapper<BlockId, PortId, PinId, NetId>::new_net_id(NetId old_id) const {
    return net_id_map_[old_id];
}

/*
 *
 * Netlist class implementation
 *
 */

template<typename BlockId, typename PortId, typename PinId, typename NetId>
Netlist<BlockId, PortId, PinId, NetId>::Netlist(std::string name, std::string id)
    : netlist_name_(name)
    , netlist_id_(id) {}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
Netlist<BlockId, PortId, PinId, NetId>::~Netlist() = default;

/*
 *
 * Netlist
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
const std::string& Netlist<BlockId, PortId, PinId, NetId>::netlist_name() const {
    return netlist_name_;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
const std::string& Netlist<BlockId, PortId, PinId, NetId>::netlist_id() const {
    return netlist_id_;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::is_dirty() const {
    return dirty_;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::is_compressed() const {
    return !is_dirty();
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::print_stats() const {
    VTR_LOG("Blocks  %zu capacity/size: %.2f\n", block_ids_.size(), float(block_ids_.capacity()) / block_ids_.size());
    VTR_LOG("Ports   %zu capacity/size: %.2f\n", port_ids_.size(), float(port_ids_.capacity()) / port_ids_.size());
    VTR_LOG("Pins    %zu capacity/size: %.2f\n", pin_ids_.size(), float(pin_ids_.capacity()) / pin_ids_.size());
    VTR_LOG("Nets    %zu capacity/size: %.2f\n", net_ids_.size(), float(net_ids_.capacity()) / net_ids_.size());
    VTR_LOG("Strings %zu capacity/size: %.2f\n", string_ids_.size(), float(string_ids_.capacity()) / string_ids_.size());
}

/*
 *
 * Blocks
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
const std::string& Netlist<BlockId, PortId, PinId, NetId>::block_name(const BlockId blk_id) const {
    StringId str_id = block_names_[blk_id];
    return strings_[str_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::block_is_combinational(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    return block_clock_pins(blk_id).size() == 0;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::attr_range Netlist<BlockId, PortId, PinId, NetId>::block_attrs(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    return vtr::make_range(block_attrs_[blk_id].begin(), block_attrs_[blk_id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::param_range Netlist<BlockId, PortId, PinId, NetId>::block_params(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    return vtr::make_range(block_params_[blk_id].begin(), block_params_[blk_id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::block_pins(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    return vtr::make_range(block_pins_[blk_id].begin(), block_pins_[blk_id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::block_input_pins(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    auto begin = block_pins_[blk_id].begin();

    auto end = block_pins_[blk_id].begin() + block_num_input_pins_[blk_id];

    return vtr::make_range(begin, end);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::block_output_pins(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    auto begin = block_pins_[blk_id].begin() + block_num_input_pins_[blk_id];

    auto end = begin + block_num_output_pins_[blk_id];

    return vtr::make_range(begin, end);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::block_clock_pins(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    auto begin = block_pins_[blk_id].begin()
                 + block_num_input_pins_[blk_id]
                 + block_num_output_pins_[blk_id];

    auto end = begin + block_num_clock_pins_[blk_id];

    VTR_ASSERT_SAFE(end == block_pins_[blk_id].end());

    return vtr::make_range(begin, end);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::port_range Netlist<BlockId, PortId, PinId, NetId>::block_ports(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    return vtr::make_range(block_ports_[blk_id].begin(), block_ports_[blk_id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::port_range Netlist<BlockId, PortId, PinId, NetId>::block_input_ports(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    auto begin = block_ports_[blk_id].begin();

    auto end = block_ports_[blk_id].begin() + block_num_input_ports_[blk_id];

    return vtr::make_range(begin, end);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::port_range Netlist<BlockId, PortId, PinId, NetId>::block_output_ports(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    auto begin = block_ports_[blk_id].begin() + block_num_input_ports_[blk_id];

    auto end = begin + block_num_output_ports_[blk_id];

    return vtr::make_range(begin, end);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::port_range Netlist<BlockId, PortId, PinId, NetId>::block_clock_ports(const BlockId blk_id) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    auto begin = block_ports_[blk_id].begin()
                 + block_num_input_ports_[blk_id]
                 + block_num_output_ports_[blk_id];

    auto end = begin + block_num_clock_ports_[blk_id];

    VTR_ASSERT_SAFE(end == block_ports_[blk_id].end());

    return vtr::make_range(begin, end);
}

/*
 *
 * Ports
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
const std::string& Netlist<BlockId, PortId, PinId, NetId>::port_name(const PortId port_id) const {
    VTR_ASSERT_SAFE(valid_port_id(port_id));

    StringId str_id = port_names_[port_id];
    return strings_[str_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::port_block(const PortId port_id) const {
    VTR_ASSERT_SAFE(valid_port_id(port_id));

    return port_blocks_[port_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::port_pins(const PortId port_id) const {
    VTR_ASSERT_SAFE(valid_port_id(port_id));

    return vtr::make_range(port_pins_[port_id].begin(), port_pins_[port_id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinId Netlist<BlockId, PortId, PinId, NetId>::port_pin(const PortId port_id, const BitIndex port_bit) const {
    //Convenience look-up bypassing port
    return find_pin(port_id, port_bit);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::port_net(const PortId port_id, const BitIndex port_bit) const {
    //port_pin() will validate that port_bit and port_id are valid so don't
    //check redundently here

    //Convenience look-up bypassing port and pin
    PinId pin_id = port_pin(port_id, port_bit);
    if (pin_id) {
        return pin_net(pin_id);
    } else {
        return NetId::INVALID();
    }
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BitIndex Netlist<BlockId, PortId, PinId, NetId>::port_width(const PortId port_id) const {
    VTR_ASSERT_SAFE(valid_port_id(port_id));

    return port_widths_[port_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PortType Netlist<BlockId, PortId, PinId, NetId>::port_type(const PortId port_id) const {
    VTR_ASSERT_SAFE(valid_port_id(port_id));

    return port_types_[port_id];
}

/*
 *
 * Pins
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
std::string Netlist<BlockId, PortId, PinId, NetId>::pin_name(const PinId pin_id) const {
    BlockId blk = pin_block(pin_id);
    PortId port = pin_port(pin_id);

    return block_name(blk) + "." + port_name(port) + "[" + std::to_string(pin_port_bit(pin_id)) + "]";
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinType Netlist<BlockId, PortId, PinId, NetId>::pin_type(const PinId pin_id) const {
    auto port_id = pin_port(pin_id);

    PinType type = PinType::OPEN;
    switch (port_type(port_id)) {
        case PortType::INPUT: /*fallthrough */;
        case PortType::CLOCK:
            type = PinType::SINK;
            break;
        case PortType::OUTPUT:
            type = PinType::DRIVER;
            break;
        default:
            VTR_ASSERT_OPT_MSG(false, "Valid port type");
    }
    return type;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::pin_net(const PinId pin_id) const {
    VTR_ASSERT_SAFE(valid_pin_id(pin_id));

    return pin_nets_[pin_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
int Netlist<BlockId, PortId, PinId, NetId>::pin_net_index(const PinId pin_id) const {
    VTR_ASSERT_SAFE(valid_pin_id(pin_id));

    return pin_net_indices_[pin_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PortId Netlist<BlockId, PortId, PinId, NetId>::pin_port(const PinId pin_id) const {
    VTR_ASSERT_SAFE(valid_pin_id(pin_id));

    return pin_ports_[pin_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BitIndex Netlist<BlockId, PortId, PinId, NetId>::pin_port_bit(const PinId pin_id) const {
    VTR_ASSERT_SAFE(valid_pin_id(pin_id));

    return pin_port_bits_[pin_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::pin_block(const PinId pin_id) const {
    //Convenience lookup bypassing the port
    VTR_ASSERT_SAFE(valid_pin_id(pin_id));

    PortId port_id = pin_port(pin_id);
    return port_block(port_id);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PortType Netlist<BlockId, PortId, PinId, NetId>::pin_port_type(const PinId pin_id) const {
    PortId port_id = pin_port(pin_id);

    return port_type(port_id);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::pin_is_constant(const PinId pin_id) const {
    VTR_ASSERT_SAFE(valid_pin_id(pin_id));

    return pin_is_constant_[pin_id];
}

/*
 *
 * Nets
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
const std::string& Netlist<BlockId, PortId, PinId, NetId>::net_name(const NetId net_id) const {
    VTR_ASSERT_SAFE(valid_net_id(net_id));

    StringId str_id = net_names_[net_id];
    return strings_[str_id];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::net_pins(const NetId net_id) const {
    VTR_ASSERT_SAFE(valid_net_id(net_id));

    return vtr::make_range(net_pins_[net_id].begin(), net_pins_[net_id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinId Netlist<BlockId, PortId, PinId, NetId>::net_pin(const NetId net_id, int net_pin_index) const {
    VTR_ASSERT_SAFE(valid_net_id(net_id));
    VTR_ASSERT_SAFE_MSG(net_pin_index >= 0 && size_t(net_pin_index) < net_pins_[net_id].size(), "Pin index must be in range");

    return net_pins_[net_id][net_pin_index];
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::net_pin_block(const NetId net_id, int net_pin_index) const {
    auto pin_id = net_pin(net_id, net_pin_index);

    return pin_block(pin_id);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinId Netlist<BlockId, PortId, PinId, NetId>::net_driver(const NetId net_id) const {
    VTR_ASSERT_SAFE(valid_net_id(net_id));

    if (net_pins_[net_id].size() > 0) {
        return net_pins_[net_id][0];
    } else {
        return PinId::INVALID();
    }
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::net_driver_block(const NetId net_id) const {
    auto driver_pin_id = net_driver(net_id);
    if (driver_pin_id) {
        return pin_block(driver_pin_id);
    }
    return BlockId::INVALID();
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::net_sinks(const NetId net_id) const {
    VTR_ASSERT_SAFE(valid_net_id(net_id));

    return vtr::make_range(++net_pins_[net_id].begin(), net_pins_[net_id].end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::net_is_constant(const NetId net_id) const {
    VTR_ASSERT_SAFE(valid_net_id(net_id));

    //Look-up the driver
    auto driver_pin_id = net_driver(net_id);
    if (driver_pin_id) {
        //Valid driver, see it is constant
        return pin_is_constant(driver_pin_id);
    }

    //No valid driver so can't be const
    return false;
}

/*
 *
 * Aggregates
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::block_range Netlist<BlockId, PortId, PinId, NetId>::blocks() const {
    return vtr::make_range(block_ids_.begin(), block_ids_.end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::port_range Netlist<BlockId, PortId, PinId, NetId>::ports() const {
    return vtr::make_range(port_ids_.begin(), port_ids_.end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::pin_range Netlist<BlockId, PortId, PinId, NetId>::pins() const {
    return vtr::make_range(pin_ids_.begin(), pin_ids_.end());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::net_range Netlist<BlockId, PortId, PinId, NetId>::nets() const {
    return vtr::make_range(net_ids_.begin(), net_ids_.end());
}

/*
 *
 * Lookups
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::find_block(const std::string& name) const {
    auto str_id = find_string(name);
    if (!str_id) {
        return BlockId::INVALID();
    } else {
        return find_block(str_id);
    }
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PortId Netlist<BlockId, PortId, PinId, NetId>::find_port(const BlockId blk_id, const std::string& name) const {
    VTR_ASSERT_SAFE(valid_block_id(blk_id));

    //Since we only know the port name, we must search all the ports
    for (auto port_id : block_ports(blk_id)) {
        if (port_name(port_id) == name) {
            return port_id;
        }
    }
    return PortId::INVALID();
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::find_net(const std::string& name) const {
    auto str_id = find_string(name);
    if (!str_id) {
        return NetId::INVALID();
    } else {
        return find_net(str_id);
    }
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinId Netlist<BlockId, PortId, PinId, NetId>::find_pin(const PortId port_id, BitIndex port_bit) const {
    VTR_ASSERT_SAFE(valid_port_id(port_id));
    VTR_ASSERT_SAFE(valid_port_bit(port_id, port_bit));

    //Pins are stored in ascending order of bit index,
    //so we can binary search for the specific bit
    auto port_bit_cmp = [&](const PinId pin_id, BitIndex bit_index) {
        return pin_port_bit(pin_id) < bit_index;
    };

    auto pins_rng = port_pins(port_id);

    //Finds the location where the pin with bit index port_bit should be located (if it exists)
    auto iter = std::lower_bound(pins_rng.begin(), pins_rng.end(), port_bit, port_bit_cmp);

    if (iter == pins_rng.end() || pin_port_bit(*iter) != port_bit) {
        //Either the end of the pins (i.e. not found), or
        //the value does not match (indicating a gap in the indicies, so also not found)
        return PinId::INVALID();
    } else {
        //Found it
        VTR_ASSERT_SAFE(pin_port_bit(*iter) == port_bit);
        return *iter;
    }
}

/*
 *
 * Validation
 *
 */
//Top-level verification method, checks that the sizes
//references, and lookups are all consistent.
template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::verify() const {
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
template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::verify_sizes() const {
    bool valid = true;
    valid &= validate_block_sizes();
    valid &= validate_port_sizes();
    valid &= validate_pin_sizes();
    valid &= validate_net_sizes();
    valid &= validate_string_sizes();
    return valid;
}

//Checks that all cross-references are consistent.
//Should take linear time.
template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::verify_refs() const {
    bool valid = true;
    valid &= validate_block_port_refs();
    valid &= validate_port_pin_refs();
    valid &= validate_block_pin_refs();
    valid &= validate_net_pin_refs();
    valid &= validate_string_refs();

    return valid;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::verify_lookups() const {
    //Verify that fast look-ups are consistent

    //Blocks
    for (auto blk_id : blocks()) {
        const auto& name = block_name(blk_id);
        if (find_block(name) != blk_id) {
            VPR_THROW(VPR_ERROR_NETLIST, "Block lookup by name mismatch");
        }
    }

    //Ports
    for (auto port_id : port_ids_) {
        auto blk_id = port_block(port_id);
        const auto& name = port_name(port_id);
        if (find_port(blk_id, name) != port_id) {
            VPR_THROW(VPR_ERROR_NETLIST, "Port lookup by name mismatch");
        }
    }

    //Pins
    for (auto pin_id : pin_ids_) {
        auto port_id = pin_port(pin_id);
        auto bit = pin_port_bit(pin_id);
        if (find_pin(port_id, bit) != pin_id) {
            VPR_THROW(VPR_ERROR_NETLIST, "Pin lookup by name mismatch");
        }
    }

    //Nets
    for (auto net_id : nets()) {
        const auto& name = net_name(net_id);
        if (find_net(name) != net_id) {
            VPR_THROW(VPR_ERROR_NETLIST, "Net lookup by name mismatch");
        }
    }

    //Strings
    for (auto str_id : string_ids_) {
        const auto& name = strings_[str_id];
        if (find_string(name) != str_id) {
            VPR_THROW(VPR_ERROR_NETLIST, "String lookup by name mismatch");
        }
    }
    return true;
}

/*
 *
 * Mutators
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::create_block(const std::string name) {
    //Must have a non-empty name
    VTR_ASSERT_MSG(!name.empty(), "Non-Empty block name");

    //Check if the block has already been created
    StringId name_id = create_string(name);
    BlockId blk_id = find_block(name_id);

    if (blk_id == BlockId::INVALID()) {
        //Not found, create it

        //Reserve an id
        blk_id = BlockId(block_ids_.size());
        block_ids_.push_back(blk_id);

        //Initialize the data
        block_names_.push_back(name_id);
        block_attrs_.emplace_back();
        block_params_.emplace_back();

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

    //Check post-conditions: values
    VTR_ASSERT(valid_block_id(blk_id));
    VTR_ASSERT(block_name(blk_id) == name);
    VTR_ASSERT_SAFE(find_block(name) == blk_id);

    return blk_id;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PortId Netlist<BlockId, PortId, PinId, NetId>::create_port(const BlockId blk_id, const std::string name, BitIndex width, PortType type) {
    //Check pre-conditions
    VTR_ASSERT_MSG(valid_block_id(blk_id), "Valid block id");

    //See if the port already exists
    StringId name_id = create_string(name);
    PortId port_id = find_port(blk_id, name);
    if (!port_id) { //Not found, create it
                    //Reserve an id
        port_id = PortId(port_ids_.size());
        port_ids_.push_back(port_id);

        //Initialize the per-port-instance data
        port_blocks_.push_back(blk_id);
        port_names_.push_back(name_id);
        port_widths_.push_back(width);
        port_types_.push_back(type);

        //Allocate the pins, initialize to invalid Ids
        port_pins_.emplace_back();
    }

    //Check post-conditions: values
    VTR_ASSERT(valid_port_id(port_id));
    VTR_ASSERT(port_block(port_id) == blk_id);
    VTR_ASSERT(port_width(port_id) == width);

    return port_id;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
PinId Netlist<BlockId, PortId, PinId, NetId>::create_pin(const PortId port_id, BitIndex port_bit, const NetId net_id, const PinType type, bool is_const) {
    //Check pre-conditions (valid ids)
    VTR_ASSERT_MSG(valid_port_id(port_id), "Valid port id");
    VTR_ASSERT_MSG(valid_port_bit(port_id, port_bit), "Valid port bit");
    VTR_ASSERT_MSG(valid_net_id(net_id), "Valid net id");

    //See if the pin already exists
    PinId pin_id = find_pin(port_id, port_bit);
    if (!pin_id) {
        //Not found, create it

        //Reserve an id
        pin_id = PinId(pin_ids_.size());
        pin_ids_.push_back(pin_id);

        //Initialize the pin data
        pin_ports_.push_back(port_id);
        pin_port_bits_.push_back(port_bit);
        pin_nets_.push_back(net_id);
        pin_is_constant_.push_back(is_const);

        //Add the pin to the net
        int pins_net_index = associate_pin_with_net(pin_id, type, net_id);

        //Save the net index of the pin
        pin_net_indices_.push_back(pins_net_index);

        //Add the pin to the port
        associate_pin_with_port(pin_id, port_id);

        //Add the pin to the block
        associate_pin_with_block(pin_id, port_type(port_id), port_block(port_id));
    }

    //Check post-conditions: values
    VTR_ASSERT(valid_pin_id(pin_id));
    VTR_ASSERT(pin_port(pin_id) == port_id);
    VTR_ASSERT(pin_port_bit(pin_id) == port_bit);
    VTR_ASSERT(pin_net(pin_id) == net_id);
    VTR_ASSERT(pin_is_constant(pin_id) == is_const);
    VTR_ASSERT_SAFE(find_pin(port_id, port_bit) == pin_id);

    return pin_id;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::create_net(const std::string name) {
    //Creates an empty net (or returns an existing one)
    VTR_ASSERT_MSG(!name.empty(), "Valid net name");

    //Check if the net has already been created
    StringId name_id = create_string(name);
    NetId net_id = find_net(name_id);
    if (net_id == NetId::INVALID()) {
        //Not found, create it

        //Reserve an id
        net_id = NetId(net_ids_.size());
        net_ids_.push_back(net_id);

        //Initialize the data
        net_names_.push_back(name_id);

        //Initialize the look-ups
        net_name_to_net_id_.insert(name_id, net_id);

        //Initialize with no driver
        net_pins_.emplace_back();
        net_pins_[net_id].emplace_back(PinId::INVALID());

        VTR_ASSERT(net_pins_[net_id].size() == 1);
        VTR_ASSERT(net_pins_[net_id][0] == PinId::INVALID());
    }

    //Check post-conditions: values
    VTR_ASSERT(valid_net_id(net_id));
    VTR_ASSERT(net_name(net_id) == name);
    VTR_ASSERT(find_net(name) == net_id);

    return net_id;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::add_net(const std::string name, PinId driver, std::vector<PinId> sinks) {
    //Creates a net with a full set of pins
    VTR_ASSERT_MSG(!find_net(name), "Net should not exist");

    //Create the empty net
    NetId net_id = create_net(name);

    //Set the driver and sinks of the net
    auto& dest_pins = net_pins_[net_id];
    dest_pins[0] = driver;
    dest_pins.insert(dest_pins.end(),
                     std::make_move_iterator(sinks.begin()),
                     std::make_move_iterator(sinks.end()));

    //Associate each pin with the net
    int net_index = 0;
    pin_nets_[driver] = net_id;
    pin_net_indices_[driver] = net_index++;
    for (auto sink : sinks) {
        pin_nets_[sink] = net_id;
        pin_net_indices_[sink] = net_index++;
    }

    return net_id;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::set_pin_is_constant(const PinId pin_id, const bool value) {
    VTR_ASSERT(valid_pin_id(pin_id));

    pin_is_constant_[pin_id] = value;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::set_pin_net(const PinId pin, PinType type, const NetId net) {
    VTR_ASSERT(valid_pin_id(pin));

    VTR_ASSERT((type == PinType::DRIVER && pin_port_type(pin) == PortType::OUTPUT)
               || (type == PinType::SINK && (pin_port_type(pin) == PortType::INPUT || pin_port_type(pin) == PortType::CLOCK)));

    NetId orig_net = pin_net(pin);
    if (orig_net) {
        //Clean up the pin reference on the original net
        remove_net_pin(orig_net, pin);
    }

    //Mark the pin's net
    pin_nets_[pin] = net;

    //Add the pin to the net
    int pins_net_index = associate_pin_with_net(pin, type, net);

    //Save the pin's index within the net
    pin_net_indices_[pin] = pins_net_index;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::set_block_name(const BlockId blk_id, const std::string new_name) {
    VTR_ASSERT(valid_block_id(blk_id));

    //Names must be unique -- no duplicates allowed
    BlockId existing_blk_id = find_block(new_name);
    if (existing_blk_id) {
        VPR_THROW(VPR_ERROR_NETLIST, "Can not re-name block '%s' to '%s' (a block named '%s' already exists).",
                  block_name(blk_id).c_str(),
                  new_name.c_str(),
                  new_name.c_str());
    }

    //Remove old name-look-up
    {
        StringId old_string = find_string(block_name(blk_id));
        block_name_to_block_id_[old_string] = BlockId::INVALID();
    }

    //Re-name the block
    StringId new_string = create_string(new_name);
    block_names_[blk_id] = new_string;

    //Update name-look-up
    block_name_to_block_id_.insert(new_string, blk_id);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::set_block_attr(const BlockId blk_id, const std::string& name, const std::string& value) {
    VTR_ASSERT(valid_block_id(blk_id));

    block_attrs_[blk_id][name] = value;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::set_block_param(const BlockId blk_id, const std::string& name, const std::string& value) {
    VTR_ASSERT(valid_block_id(blk_id));

    block_params_[blk_id][name] = value;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::merge_nets(const NetId driver_net, const NetId sink_net) {
    VTR_ASSERT(valid_net_id(driver_net));
    VTR_ASSERT(valid_net_id(sink_net));

    //Sink net must not have a driver pin
    PinId sink_driver = net_driver(sink_net);
    if (sink_driver) {
        VPR_THROW(VPR_ERROR_NETLIST, "Can not merge nets '%s' and '%s' (sink net '%s' should have no driver, but is driven by pin '%s')",
                  net_name(driver_net).c_str(),
                  net_name(sink_net).c_str(),
                  net_name(sink_net).c_str(),
                  pin_name(sink_driver).c_str());
    }

    //We allow the driver net to (potentially) have no driver yet,
    //so we don't check to ensure it exists

    //
    //Merge the nets
    //

    //Move the sinks to the driver net
    for (PinId sink_pin : net_sinks(sink_net)) {
        //Update pin -> net references, also adds pins to driver_net
        set_pin_net(sink_pin, pin_type(sink_pin), driver_net);
    }

    //Remove the sink net
    // Note that we drop the sink net's pin references first,
    // this ensures remove_net() will only clean-up the net
    // data, and will not modify the (already moved) pins
    net_pins_[sink_net].clear(); //Drop sink_net's pin references
    remove_net(sink_net);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_block(const BlockId blk_id) {
    VTR_ASSERT(valid_block_id(blk_id));

    //Remove the ports
    for (PortId block_port : block_ports(blk_id)) {
        remove_port(block_port);
    }

    //Invalidate look-up
    StringId name_id = block_names_[blk_id];
    block_name_to_block_id_.insert(name_id, BlockId::INVALID());

    //Mark as invalid
    block_ids_[blk_id] = BlockId::INVALID();

    //Call derived class' remove()
    remove_block_impl(blk_id);

    //Mark netlist dirty
    dirty_ = true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_port(const PortId port_id) {
    VTR_ASSERT(valid_port_id(port_id));

    //Remove the pins
    for (auto pin : port_pins(port_id)) {
        if (valid_pin_id(pin)) {
            remove_pin(pin);
        }
    }

    //Mark as invalid
    port_ids_[port_id] = PortId::INVALID();

    //Call derived class' remove()
    remove_port_impl(port_id);

    //Mark netlist dirty
    dirty_ = true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_pin(const PinId pin_id) {
    VTR_ASSERT(valid_pin_id(pin_id));

    //Find the associated net
    NetId net = pin_net(pin_id);

    //Remove the pin from the associated net/port/block
    remove_net_pin(net, pin_id);

    //Mark as invalid
    pin_ids_[pin_id] = PinId::INVALID();

    //Call derived class' remove()
    remove_pin_impl(pin_id);

    //Mark netlist dirty
    dirty_ = true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_net(const NetId net_id) {
    VTR_ASSERT(valid_net_id(net_id));

    //Disassociate the pins from the net
    for (auto pin_id : net_pins(net_id)) {
        if (pin_id) {
            pin_nets_[pin_id] = NetId::INVALID();
            pin_net_indices_[pin_id] = INVALID_INDEX;
        }
    }
    //Invalidate look-up
    StringId name_id = net_names_[net_id];
    net_name_to_net_id_.insert(name_id, NetId::INVALID());

    //Mark as invalid
    net_ids_[net_id] = NetId::INVALID();

    //Call derived class' remove()
    remove_net_impl(net_id);

    //Mark netlist dirty
    dirty_ = true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_net_pin(const NetId net_id, const PinId pin_id) {
    //Remove a net-pin connection
    //
    //Note that during sweeping either the net or pin could be invalid (i.e. already swept)
    //so we check before trying to use them

    if (valid_net_id(net_id)) {
        //Warning: this is slow!
        auto iter = std::find(net_pins_[net_id].begin(), net_pins_[net_id].end(), pin_id); //Linear search
        VTR_ASSERT(iter != net_pins_[net_id].end());

        if (net_driver(net_id) == pin_id) {
            //Mark no driver
            net_pins_[net_id][0] = PinId::INVALID();
        } else {
            //Remove sink
            net_pins_[net_id].erase(iter); //Linear remove
        }

        //Note: since we fully update the net we don't need to mark the netlist dirty_
    }

    //Disassociate the pin with the net
    if (valid_pin_id(pin_id)) {
        pin_nets_[pin_id] = NetId::INVALID();
        pin_net_indices_[pin_id] = INVALID_INDEX;

        //Mark netlist dirty, since we are leaving an invalid net id
        dirty_ = true;
    }
}

/*
 *
 * Internal utilities
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::IdRemapper Netlist<BlockId, PortId, PinId, NetId>::remove_and_compress() {
    remove_unused();
    return compress();
}

//Compress the various netlist components to remove invalid entries
// Note: this invalidates all Ids
template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::IdRemapper Netlist<BlockId, PortId, PinId, NetId>::compress() {
    //Build the mappings from old to new id's, potentially
    //re-ordering for improved cache locality
    //
    //The vectors passed as parameters are initialized as a mapping from old to new index
    // e.g. block_id_map[old_id] == new_id
    IdRemapper id_remapper = build_id_maps();

    clean_nets(id_remapper.net_id_map_);
    clean_pins(id_remapper.pin_id_map_);
    clean_ports(id_remapper.port_id_map_);
    clean_blocks(id_remapper.block_id_map_);
    //TODO: clean strings
    //TODO: iterative cleaning?

    //Now we re-build all the cross references
    // Note: net references must be rebuilt (to remove pins) before
    //       the pin references can be rebuilt (to account for index changes
    //       due to pins being removed from the net)
    rebuild_block_refs(id_remapper.pin_id_map_, id_remapper.port_id_map_);
    rebuild_port_refs(id_remapper.block_id_map_, id_remapper.pin_id_map_);
    rebuild_net_refs(id_remapper.pin_id_map_);
    rebuild_pin_refs(id_remapper.port_id_map_, id_remapper.net_id_map_);

    //Re-build the lookups
    rebuild_lookups();

    //Resize containers to exact size
    shrink_to_fit();

    //Netlist is now clean
    dirty_ = false;

    return id_remapper;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::remove_unused() {
    //Mark any nets/pins/ports/blocks which are not in use as invalid
    //so they will be removed

    bool found_unused;
    do {
        found_unused = false;
        //Nets with no connected pins
        for (auto net_id : net_ids_) {
            if (net_id
                && !net_driver(net_id)
                && net_sinks(net_id).size() == 0) {
                remove_net(net_id);
                found_unused = true;
            }
        }

        //Pins with no connected nets
        for (auto pin_id : pin_ids_) {
            if (pin_id && !pin_net(pin_id)) {
                remove_pin(pin_id);
                found_unused = true;
            }
        }

        //Empty ports
        for (auto port_id : port_ids_) {
            if (port_id && port_pins(port_id).size() == 0) {
                remove_port(port_id);
                found_unused = true;
            }
        }

        //Blocks with no pins
        for (auto blk_id : block_ids_) {
            if (blk_id && block_pins(blk_id).size() == 0) {
                remove_block(blk_id);
                found_unused = true;
            }
        }

    } while (found_unused);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::IdRemapper Netlist<BlockId, PortId, PinId, NetId>::build_id_maps() {
    IdRemapper id_remapper;
    id_remapper.block_id_map_ = compress_ids(block_ids_);
    id_remapper.port_id_map_ = compress_ids(port_ids_);
    id_remapper.pin_id_map_ = compress_ids(pin_ids_);
    id_remapper.net_id_map_ = compress_ids(net_ids_);
    return id_remapper;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::clean_blocks(const vtr::vector_map<BlockId, BlockId>& block_id_map) {
    //Update all the block values
    block_ids_ = clean_and_reorder_ids(block_id_map);
    block_names_ = clean_and_reorder_values(block_names_, block_id_map);

    block_pins_ = clean_and_reorder_values(block_pins_, block_id_map);
    block_num_input_pins_ = clean_and_reorder_values(block_num_input_pins_, block_id_map);
    block_num_output_pins_ = clean_and_reorder_values(block_num_output_pins_, block_id_map);
    block_num_clock_pins_ = clean_and_reorder_values(block_num_clock_pins_, block_id_map);

    block_ports_ = clean_and_reorder_values(block_ports_, block_id_map);
    block_num_input_ports_ = clean_and_reorder_values(block_num_input_ports_, block_id_map);
    block_num_output_ports_ = clean_and_reorder_values(block_num_output_ports_, block_id_map);
    block_num_clock_ports_ = clean_and_reorder_values(block_num_clock_ports_, block_id_map);

    block_attrs_ = clean_and_reorder_values(block_attrs_, block_id_map);
    block_params_ = clean_and_reorder_values(block_params_, block_id_map);

    clean_blocks_impl(block_id_map);

    VTR_ASSERT(validate_block_sizes());

    VTR_ASSERT_MSG(are_contiguous(block_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(block_ids_), "All Ids should be valid");
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::clean_ports(const vtr::vector_map<PortId, PortId>& port_id_map) {
    //Update all the port values
    port_ids_ = clean_and_reorder_ids(port_id_map);
    port_names_ = clean_and_reorder_values(port_names_, port_id_map);
    port_blocks_ = clean_and_reorder_values(port_blocks_, port_id_map);
    port_widths_ = clean_and_reorder_values(port_widths_, port_id_map);
    port_types_ = clean_and_reorder_values(port_types_, port_id_map);
    port_pins_ = clean_and_reorder_values(port_pins_, port_id_map);

    clean_ports_impl(port_id_map);

    VTR_ASSERT(validate_port_sizes());

    VTR_ASSERT_MSG(are_contiguous(port_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(port_ids_), "All Ids should be valid");
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::clean_pins(const vtr::vector_map<PinId, PinId>& pin_id_map) {
    //Update all the pin values
    pin_ids_ = clean_and_reorder_ids(pin_id_map);
    pin_ports_ = clean_and_reorder_values(pin_ports_, pin_id_map);

    //It is sufficient to merely clean and re-order the pin_port_bits_, since
    //the size of a port (and hence the pin's bit index in the port) does not change
    //during cleaning
    pin_port_bits_ = clean_and_reorder_values(pin_port_bits_, pin_id_map);
    pin_nets_ = clean_and_reorder_values(pin_nets_, pin_id_map);

    //Note that clean and re-order serves only to resize pin_net_indices_, the
    //actual net references are wrong (since net size may have changed during
    //cleaning). They will be updated in rebuild_pin_refs()
    pin_net_indices_ = clean_and_reorder_values(pin_net_indices_, pin_id_map);
    pin_is_constant_ = clean_and_reorder_values(pin_is_constant_, pin_id_map);

    clean_pins_impl(pin_id_map);

    VTR_ASSERT(validate_pin_sizes());

    VTR_ASSERT_MSG(are_contiguous(pin_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(pin_ids_), "All Ids should be valid");
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::clean_nets(const vtr::vector_map<NetId, NetId>& net_id_map) {
    //Update all the net values
    net_ids_ = clean_and_reorder_ids(net_id_map);
    net_names_ = clean_and_reorder_values(net_names_, net_id_map);
    net_pins_ = clean_and_reorder_values(net_pins_, net_id_map); //Note: actual pin refs are updated in rebuild_net_refs()

    clean_nets_impl(net_id_map);

    VTR_ASSERT(validate_net_sizes());

    VTR_ASSERT_MSG(are_contiguous(net_ids_), "Ids should be contiguous");
    VTR_ASSERT_MSG(all_valid(net_ids_), "All Ids should be valid");
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::rebuild_lookups() {
    //We iterate through the reverse-lookups and update the values (i.e. ids)
    //to the new id values

    //Blocks
    block_name_to_block_id_.clear();
    for (auto blk_id : blocks()) {
        const auto& key = block_names_[blk_id];
        block_name_to_block_id_.insert(key, blk_id);
    }

    //Nets
    net_name_to_net_id_.clear();
    for (auto net_id : nets()) {
        const auto& key = net_names_[net_id];
        net_name_to_net_id_.insert(key, net_id);
    }
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::rebuild_block_refs(const vtr::vector_map<PinId, PinId>& pin_id_map,
                                                                const vtr::vector_map<PortId, PortId>& port_id_map) {
    //Update the pin id references held by blocks
    for (auto blk_id : blocks()) {
        //Before update the references, we need to know how many are valid,
        //so we can also update the numbers of input/output/clock pins

        //Note that we take special care to not modify the pin counts until after
        //they have all been counted (since the block_*_pins() functions depend on
        //the counts).
        size_t num_input_pins = count_valid_refs(block_input_pins(blk_id), pin_id_map);
        size_t num_output_pins = count_valid_refs(block_output_pins(blk_id), pin_id_map);
        size_t num_clock_pins = count_valid_refs(block_clock_pins(blk_id), pin_id_map);

        std::vector<PinId>& pin_collection = block_pins_[blk_id];

        pin_collection = update_valid_refs(pin_collection, pin_id_map);
        block_num_input_pins_[blk_id] = num_input_pins;
        block_num_output_pins_[blk_id] = num_output_pins;
        block_num_clock_pins_[blk_id] = num_clock_pins;

        VTR_ASSERT_SAFE_MSG(all_valid(pin_collection), "All Ids should be valid");
        VTR_ASSERT(pin_collection.size() == size_t(block_num_input_pins_[blk_id] + block_num_output_pins_[blk_id] + block_num_clock_pins_[blk_id]));

        //Similarily for ports
        size_t num_input_ports = count_valid_refs(block_input_ports(blk_id), port_id_map);
        size_t num_output_ports = count_valid_refs(block_output_ports(blk_id), port_id_map);
        size_t num_clock_ports = count_valid_refs(block_clock_ports(blk_id), port_id_map);

        std::vector<PortId>& blk_ports = block_ports_[blk_id];

        blk_ports = update_valid_refs(blk_ports, port_id_map);
        block_num_input_ports_[blk_id] = num_input_ports;
        block_num_output_ports_[blk_id] = num_output_ports;
        block_num_clock_ports_[blk_id] = num_clock_ports;

        VTR_ASSERT_SAFE_MSG(all_valid(blk_ports), "All Ids should be valid");
        VTR_ASSERT(blk_ports.size() == size_t(block_num_input_ports_[blk_id] + block_num_output_ports_[blk_id] + block_num_clock_ports_[blk_id]));
    }

    rebuild_block_refs_impl(pin_id_map, port_id_map);

    VTR_ASSERT(validate_block_sizes());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::rebuild_port_refs(const vtr::vector_map<BlockId, BlockId>& block_id_map,
                                                               const vtr::vector_map<PinId, PinId>& pin_id_map) {
    //Update block and pin references held by ports
    port_blocks_ = update_valid_refs(port_blocks_, block_id_map);
    VTR_ASSERT_SAFE_MSG(all_valid(port_blocks_), "All Ids should be valid");

    VTR_ASSERT(port_blocks_.size() == port_ids_.size());

    for (auto& pin_collection : port_pins_) {
        pin_collection = update_valid_refs(pin_collection, pin_id_map);
        VTR_ASSERT_SAFE_MSG(all_valid(pin_collection), "All Ids should be valid");
    }

    rebuild_port_refs_impl(block_id_map, pin_id_map);

    VTR_ASSERT(validate_port_sizes());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::rebuild_pin_refs(const vtr::vector_map<PortId, PortId>& port_id_map,
                                                              const vtr::vector_map<NetId, NetId>& net_id_map) {
    //Update port and net references held by pins
    pin_ports_ = update_all_refs(pin_ports_, port_id_map);
    VTR_ASSERT_SAFE_MSG(all_valid(pin_ports_), "All Ids should be valid");

    //NOTE: we do not need to update pin_port_bits_ since a pin's index within it's
    //port will not change (since the port width's don't change when cleaned)

    pin_nets_ = update_all_refs(pin_nets_, net_id_map);
    VTR_ASSERT_SAFE_MSG(all_valid(pin_nets_), "All Ids should be valid");

    //We must carefully re-build the pin net indices from scratch, since cleaning
    //may have changed the index of the pin within the net (e.g. if invalid pins
    //were removed)
    //
    //Note that for this to work correctly, the net references must have already been re-built!
    for (auto net : nets()) {
        int i = 0;
        for (auto pin : net_pins(net)) {
            pin_net_indices_[pin] = i;
            ++i;
        }
    }

    rebuild_pin_refs_impl(port_id_map, net_id_map);

    VTR_ASSERT(validate_pin_sizes());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::rebuild_net_refs(const vtr::vector_map<PinId, PinId>& pin_id_map) {
    //Update pin references held by nets
    for (auto& pin_collection : net_pins_) {
        //We take special care to preserve the driver index, since an INVALID id is used
        //to indicate an undriven net it should not be dropped during the update
        pin_collection = update_valid_refs(pin_collection, pin_id_map, {NET_DRIVER_INDEX});

        VTR_ASSERT_SAFE_MSG(all_valid(pin_collection), "All sinks should be valid");
    }

    rebuild_net_refs_impl(pin_id_map);

    VTR_ASSERT(validate_net_sizes());
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::shrink_to_fit() {
    //Block data
    block_ids_.shrink_to_fit();
    block_names_.shrink_to_fit();

    block_pins_.shrink_to_fit();
    for (std::vector<PinId>& pin_collection : block_pins_) {
        pin_collection.shrink_to_fit();
    }
    block_num_input_pins_.shrink_to_fit();
    block_num_output_pins_.shrink_to_fit();
    block_num_clock_pins_.shrink_to_fit();

    block_ports_.shrink_to_fit();
    for (auto& blk_ports : block_ports_) {
        blk_ports.shrink_to_fit();
    }
    block_num_input_ports_.shrink_to_fit();
    block_num_output_ports_.shrink_to_fit();
    block_num_clock_ports_.shrink_to_fit();

    VTR_ASSERT(validate_block_sizes());

    //Port data
    port_ids_.shrink_to_fit();
    port_blocks_.shrink_to_fit();
    port_widths_.shrink_to_fit();
    port_types_.shrink_to_fit();
    port_pins_.shrink_to_fit();
    for (auto& pin_collection : port_pins_) {
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
template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_block_id(BlockId block_id) const {
    if (block_id == BlockId::INVALID()
        || !block_ids_.contains(block_id)
        || block_ids_[block_id] != block_id) {
        return false;
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_port_id(PortId port_id) const {
    if (port_id == PortId::INVALID()
        || !port_ids_.contains(port_id)
        || port_ids_[port_id] != port_id) {
        return false;
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_port_bit(PortId port_id, BitIndex port_bit) const {
    VTR_ASSERT_SAFE(valid_port_id(port_id));
    if (port_bit >= port_width(port_id)) {
        return false;
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_pin_id(PinId pin_id) const {
    if (pin_id == PinId::INVALID()
        || !pin_ids_.contains(pin_id)
        || pin_ids_[pin_id] != pin_id) {
        return false;
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_net_id(NetId net_id) const {
    if (net_id == NetId::INVALID()
        || !net_ids_.contains(net_id)
        || net_ids_[net_id] != net_id) {
        return false;
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::valid_string_id(typename Netlist<BlockId, PortId, PinId, NetId>::StringId string_id) const {
    if (string_id == StringId::INVALID()
        || !string_ids_.contains(string_id)
        || string_ids_[string_id] != string_id) {
        return false;
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_block_sizes() const {
    size_t num_blocks = blocks().size();
    if (block_names_.size() != num_blocks
        || block_pins_.size() != num_blocks
        || block_num_input_pins_.size() != num_blocks
        || block_num_output_pins_.size() != num_blocks
        || block_num_clock_pins_.size() != num_blocks
        || block_ports_.size() != num_blocks
        || block_num_input_ports_.size() != num_blocks
        || block_num_output_ports_.size() != num_blocks
        || block_num_clock_ports_.size() != num_blocks
        || block_attrs_.size() != num_blocks
        || block_params_.size() != num_blocks
        || !validate_block_sizes_impl(num_blocks)) {
        VPR_THROW(VPR_ERROR_NETLIST, "Inconsistent block data sizes");
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_port_sizes() const {
    size_t num_ports = ports().size();
    if (port_names_.size() != num_ports
        || port_blocks_.size() != num_ports
        || port_pins_.size() != num_ports
        || !validate_port_sizes_impl(num_ports)) {
        VPR_THROW(VPR_ERROR_NETLIST, "Inconsistent port data sizes");
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_pin_sizes() const {
    size_t num_pins = pins().size();
    if (pin_ports_.size() != num_pins
        || pin_port_bits_.size() != num_pins
        || pin_nets_.size() != num_pins
        || pin_net_indices_.size() != num_pins
        || pin_is_constant_.size() != num_pins
        || !validate_pin_sizes_impl(num_pins)) {
        VPR_THROW(VPR_ERROR_NETLIST, "Inconsistent pin data sizes");
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_net_sizes() const {
    size_t num_nets = nets().size();
    if (net_names_.size() != num_nets
        || net_pins_.size() != num_nets
        || !validate_net_sizes_impl(num_nets)) {
        VPR_THROW(VPR_ERROR_NETLIST, "Inconsistent net data sizes");
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_string_sizes() const {
    if (strings_.size() != string_ids_.size()) {
        VPR_THROW(VPR_ERROR_NETLIST, "Inconsistent string data sizes");
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_block_port_refs() const {
    //Verify that all block <-> port references are consistent

    //Track how many times we've seen each port from the blocks
    vtr::vector_map<PortId, unsigned> seen_port_ids(port_ids_.size());

    for (auto blk_id : blocks()) {
        for (auto in_port_id : block_input_ports(blk_id)) {
            if (blk_id != port_block(in_port_id)) {
                VPR_THROW(VPR_ERROR_NETLIST, "Block-input port cross-reference does not match");
            }
            ++seen_port_ids[in_port_id];
        }
        for (auto out_port_id : block_output_ports(blk_id)) {
            if (blk_id != port_block(out_port_id)) {
                VPR_THROW(VPR_ERROR_NETLIST, "Block-output port cross-reference does not match");
            }
            ++seen_port_ids[out_port_id];
        }
        for (auto clock_port_id : block_clock_ports(blk_id)) {
            if (blk_id != port_block(clock_port_id)) {
                VPR_THROW(VPR_ERROR_NETLIST, "Block-clock port cross-reference does not match");
            }
            ++seen_port_ids[clock_port_id];
        }
    }

    //Check that we have neither orphaned ports (i.e. that aren't referenced by a block)
    //nor shared ports (i.e. referenced by multiple blocks)
    auto is_one = [](unsigned val) {
        return val == 1;
    };
    if (!std::all_of(seen_port_ids.begin(), seen_port_ids.end(), is_one)) {
        VPR_THROW(VPR_ERROR_NETLIST, "Port not referenced by a single block");
    }

    if (std::accumulate(seen_port_ids.begin(), seen_port_ids.end(), 0u) != port_ids_.size()) {
        VPR_THROW(VPR_ERROR_NETLIST, "Found orphaned port (not referenced by a block)");
    }

    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_block_pin_refs() const {
    //Verify that block pin refs are consistent
    //(e.g. inputs are inputs, outputs are outputs etc.)
    for (auto blk_id : blocks()) {
        //Check that only input pins are found when iterating through inputs
        for (auto pin_id : block_input_pins(blk_id)) {
            auto port_id = pin_port(pin_id);

            auto type = port_type(port_id);
            if (type != PortType::INPUT) {
                VPR_THROW(VPR_ERROR_NETLIST, "Non-input pin in block input pins");
            }
        }

        //Check that only output pins are found when iterating through outputs
        for (auto pin_id : block_output_pins(blk_id)) {
            auto port_id = pin_port(pin_id);

            auto type = port_type(port_id);
            if (type != PortType::OUTPUT) {
                VPR_THROW(VPR_ERROR_NETLIST, "Non-output pin in block output pins");
            }
        }

        //Check that only clock pins are found when iterating through clocks
        for (auto pin_id : block_clock_pins(blk_id)) {
            auto port_id = pin_port(pin_id);

            auto type = port_type(port_id);
            if (type != PortType::CLOCK) {
                VPR_THROW(VPR_ERROR_NETLIST, "Non-clock pin in block clock pins");
            }
        }
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_port_pin_refs() const {
    //Check that port <-> pin references are consistent

    //Track how many times we've seen each pin from the ports
    vtr::vector_map<PinId, unsigned> seen_pin_ids(pin_ids_.size());

    for (auto port_id : port_ids_) {
        bool first_bit = true;
        BitIndex prev_bit_index = 0;
        for (auto pin_id : port_pins(port_id)) {
            if (pin_port(pin_id) != port_id) {
                VPR_THROW(VPR_ERROR_NETLIST, "Port-pin cross-reference does not match");
            }
            if (pin_port_bit(pin_id) >= port_width(port_id)) {
                VPR_THROW(VPR_ERROR_NETLIST, "Out-of-range port bit index");
            }
            ++seen_pin_ids[pin_id];

            BitIndex port_bit_index = pin_port_bit(pin_id);

            //Verify that the port bit index is legal
            if (!valid_port_bit(port_id, port_bit_index)) {
                VPR_THROW(VPR_ERROR_NETLIST, "Invalid pin bit index in port");
            }

            //Verify that the pins are listed in increasing order of port bit index,
            //we rely on this property to perform fast binary searches for pins with specific bit
            //indicies
            if (first_bit) {
                prev_bit_index = port_bit_index;
                first_bit = false;
            } else if (prev_bit_index >= port_bit_index) {
                VPR_THROW(VPR_ERROR_NETLIST, "Port pin indicies are not in ascending order");
            }
        }
    }

    //Check that we have neither orphaned pins (i.e. that aren't referenced by a port)
    //nor shared pins (i.e. referenced by multiple ports)
    auto is_one = [](unsigned val) {
        return val == 1;
    };
    if (!std::all_of(seen_pin_ids.begin(), seen_pin_ids.end(), is_one)) {
        VPR_THROW(VPR_ERROR_NETLIST, "Pins referenced by zero or multiple ports");
    }

    if (std::accumulate(seen_pin_ids.begin(), seen_pin_ids.end(), 0u) != pin_ids_.size()) {
        VPR_THROW(VPR_ERROR_NETLIST, "Found orphaned pins (not referenced by a port)");
    }

    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_net_pin_refs() const {
    //Check that net <-> pin references are consistent

    //Track how many times we've seen each pin from the ports
    vtr::vector_map<PinId, unsigned> seen_pin_ids(pin_ids_.size());

    for (auto net_id : nets()) {
        auto pins_rng = net_pins(net_id);
        for (auto iter = pins_rng.begin(); iter != pins_rng.end(); ++iter) {
            int pin_index = std::distance(pins_rng.begin(), iter);
            auto pin_id = *iter;

            if (iter == pins_rng.begin()) {
                //The first net pin is the driver, which may be invalid
                //if there is no driver.
                if (pin_id) {
                    VTR_ASSERT(pin_index == NET_DRIVER_INDEX);
                    if (pin_type(pin_id) != PinType::DRIVER) {
                        VPR_THROW(VPR_ERROR_NETLIST, "Driver pin not found at expected index in net");
                    }
                }
            } else {
                //Any non-driver (i.e. sink) pins must be valid
                if (!pin_id) {
                    VPR_THROW(VPR_ERROR_NETLIST, "Invalid pin found in net sinks");
                }

                if (pin_type(pin_id) != PinType::SINK) {
                    VPR_THROW(VPR_ERROR_NETLIST, "Invalid pin type found in net sinks");
                }
            }

            if (pin_id) {
                //Verify the cross reference if the pin_id is valid (i.e. a sink or a valid driver)
                if (pin_net(pin_id) != net_id) {
                    VPR_THROW(VPR_ERROR_NETLIST, "Net-pin cross-reference does not match");
                }

                if (pin_net_index(pin_id) != pin_index) {
                    VPR_THROW(VPR_ERROR_NETLIST, "Pin's net index cross-reference does not match actual net index");
                }

                //We only record valid seen pins since we may see multiple undriven nets with invalid IDs
                ++seen_pin_ids[pin_id];
            }
        }
    }

    //Check that we have neither orphaned pins (i.e. that aren't referenced by a port)
    //nor shared pins (i.e. referenced by multiple ports)
    auto is_one = [](unsigned val) {
        return val == 1;
    };
    if (!std::all_of(seen_pin_ids.begin(), seen_pin_ids.end(), is_one)) {
        VPR_THROW(VPR_ERROR_NETLIST, "Found pins referenced by zero or multiple nets");
    }

    if (std::accumulate(seen_pin_ids.begin(), seen_pin_ids.end(), 0u) != pin_ids_.size()) {
        VPR_THROW(VPR_ERROR_NETLIST, "Found orphaned pins (not referenced by a net)");
    }

    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::validate_string_refs() const {
    for (const auto& str_id : block_names_) {
        if (!valid_string_id(str_id)) {
            VPR_THROW(VPR_ERROR_NETLIST, "Invalid block name string reference");
        }
    }
    for (const auto& str_id : port_names_) {
        if (!valid_string_id(str_id)) {
            VPR_THROW(VPR_ERROR_NETLIST, "Invalid port name string reference");
        }
    }
    for (const auto& str_id : net_names_) {
        if (!valid_string_id(str_id)) {
            VPR_THROW(VPR_ERROR_NETLIST, "Invalid net name string reference");
        }
    }
    return true;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
bool Netlist<BlockId, PortId, PinId, NetId>::verify_block_invariants() const {
    for (auto blk_id : blocks()) {
        //Check block type
        //Find any connected clock
        NetId clk_net_id;
        for (auto pin_id : block_clock_pins(blk_id)) {
            if (pin_id) {
                clk_net_id = pin_net(pin_id);
                break;
            }
        }

        if (block_is_combinational(blk_id)) {
            //Non-sequential types must not have a clock
            if (clk_net_id) {
                VPR_THROW(VPR_ERROR_NETLIST, "Block '%s' is a non-sequential type but has a clock '%s'",
                          block_name(blk_id).c_str(), net_name(clk_net_id).c_str());
            }

        } else {
            //Sequential types must have a clock
            if (!clk_net_id) {
                VPR_THROW(VPR_ERROR_NETLIST, "Block '%s' is sequential type but has no clock",
                          block_name(blk_id).c_str());
            }
        }

        //Check that block pins and ports are consistent (i.e. same number of pins)
        size_t num_block_pins = block_pins(blk_id).size();

        size_t total_block_port_pins = 0;
        for (auto port_id : block_ports(blk_id)) {
            total_block_port_pins += port_pins(port_id).size();
        }

        if (num_block_pins != total_block_port_pins) {
            VPR_THROW(VPR_ERROR_NETLIST, "Block pins and port pins do not match on block '%s'",
                      block_name(blk_id).c_str());
        }
    }

    return true;
}

/*
 *
 * Internal utilities
 *
 */
template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::StringId Netlist<BlockId, PortId, PinId, NetId>::find_string(const std::string& str) const {
    auto iter = string_to_string_id_.find(str);
    if (iter != string_to_string_id_.end()) {
        StringId str_id = iter->second;

        VTR_ASSERT_SAFE(str_id);
        VTR_ASSERT_SAFE(strings_[str_id] == str);

        return str_id;
    } else {
        return StringId::INVALID();
    }
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
BlockId Netlist<BlockId, PortId, PinId, NetId>::find_block(const typename Netlist<BlockId, PortId, PinId, NetId>::StringId name_id) const {
    VTR_ASSERT_SAFE(valid_string_id(name_id));
    auto iter = block_name_to_block_id_.find(name_id);
    if (iter != block_name_to_block_id_.end()) {
        BlockId blk_id = *iter;

        //Check post-conditions
        if (blk_id) {
            VTR_ASSERT_SAFE(valid_block_id(blk_id));
            VTR_ASSERT_SAFE(block_names_[blk_id] == name_id);
        }

        return blk_id;
    } else {
        return BlockId::INVALID();
    }
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::associate_pin_with_block(const PinId pin_id, const PortType type, const BlockId blk_id) {
    //Get an interator pointing to where we want to insert
    pin_iterator iter;
    if (type == PortType::INPUT) {
        iter = block_input_pins(blk_id).end();
        ++block_num_input_pins_[blk_id];
    } else if (type == PortType::OUTPUT) {
        iter = block_output_pins(blk_id).end();
        ++block_num_output_pins_[blk_id];
    } else {
        VTR_ASSERT(type == PortType::CLOCK);
        iter = block_clock_pins(blk_id).end();
        ++block_num_clock_pins_[blk_id];
    }

    //Insert the element just before iter
    auto new_iter = block_pins_[blk_id].insert(iter, pin_id);

    //Inserted value should be the last valid range element
    if (type == PortType::INPUT) {
        VTR_ASSERT(new_iter + 1 == block_input_pins(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_output_pins(blk_id).begin());
    } else if (type == PortType::OUTPUT) {
        VTR_ASSERT(new_iter + 1 == block_output_pins(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_clock_pins(blk_id).begin());
    } else {
        VTR_ASSERT(type == PortType::CLOCK);
        VTR_ASSERT(new_iter + 1 == block_clock_pins(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_pins(blk_id).end());
    }
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
int Netlist<BlockId, PortId, PinId, NetId>::associate_pin_with_net(const PinId pin_id, const PinType type, const NetId net_id) {
    //Add the pin to the net
    if (type == PinType::DRIVER) {
        VTR_ASSERT_MSG(net_pins_[net_id].size() > 0, "Must be space for net's pin");
        VTR_ASSERT_MSG(net_pins_[net_id][0] == PinId::INVALID(), "Must be no existing net driver");

        net_pins_[net_id][0] = pin_id; //Set driver

        return 0; //Driver always at index 0
    } else {
        VTR_ASSERT(type == PinType::SINK);

        net_pins_[net_id].emplace_back(pin_id); //Add sink

        return net_pins_[net_id].size() - 1; //Index of inserted pin
    }
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::associate_pin_with_port(const PinId pin_id, const PortId port_id) {
    //A ports pins are stored in ascending order of port bit index, to allow for fast look-ups.
    //As a result we need to ensure that even if pins are created out-of-order (with respect
    //to port bit) they are still inserted in the port in sorted order.
    //
    //As a result we find the correct position using std::lower_bound and insert there.
    //Note that in the typical case (when port pins are created in order) this will be
    //the end of the port's pins and the insertion will be efficient. Only in the (more
    //unusual) case where the pins are created out-of-order will we need to do a slow
    //insert in the middle of the vector.
    auto port_bit_cmp = [&](const PinId pin, BitIndex bit_index) {
        return pin_port_bit(pin) < bit_index;
    };
    auto iter = std::lower_bound(port_pins_[port_id].begin(), port_pins_[port_id].end(), pin_port_bit(pin_id), port_bit_cmp);
    port_pins_[port_id].insert(iter, pin_id);
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
void Netlist<BlockId, PortId, PinId, NetId>::associate_port_with_block(const PortId port_id, const PortType type, const BlockId blk_id) {
    //Associate the port with the blocks inputs/outputs/clocks

    //Get an interator pointing to where we want to insert
    port_iterator iter;
    if (type == PortType::INPUT) {
        iter = block_input_ports(blk_id).end();
        ++block_num_input_ports_[blk_id];
    } else if (type == PortType::OUTPUT) {
        iter = block_output_ports(blk_id).end();
        ++block_num_output_ports_[blk_id];
    } else {
        VTR_ASSERT(type == PortType::CLOCK);
        iter = block_clock_ports(blk_id).end();
        ++block_num_clock_ports_[blk_id];
    }

    //Insert the element just before iter
    auto new_iter = block_ports_[blk_id].insert(iter, port_id);

    //Inserted value should be the last valid range element
    if (type == PortType::INPUT) {
        VTR_ASSERT(new_iter + 1 == block_input_ports(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_output_ports(blk_id).begin());
    } else if (type == PortType::OUTPUT) {
        VTR_ASSERT(new_iter + 1 == block_output_ports(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_clock_ports(blk_id).begin());
    } else {
        VTR_ASSERT(type == PortType::CLOCK);
        VTR_ASSERT(new_iter + 1 == block_clock_ports(blk_id).end());
        VTR_ASSERT(new_iter + 1 == block_ports(blk_id).end());
    }
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
typename Netlist<BlockId, PortId, PinId, NetId>::StringId Netlist<BlockId, PortId, PinId, NetId>::create_string(const std::string& str) {
    StringId str_id = find_string(str);
    if (!str_id) {
        //Not found, create

        //Reserve an id
        str_id = StringId(string_ids_.size());
        string_ids_.push_back(str_id);

        //Store the reverse look-up
        auto key = str;
        string_to_string_id_[key] = str_id;

        //Initialize the data
        strings_.emplace_back(str);
    }

    //Check post-conditions: sizes
    VTR_ASSERT(string_to_string_id_.size() == string_ids_.size());
    VTR_ASSERT(strings_.size() == string_ids_.size());

    //Check post-conditions: values
    VTR_ASSERT(strings_[str_id] == str);
    VTR_ASSERT_SAFE(find_string(str) == str_id);

    return str_id;
}

template<typename BlockId, typename PortId, typename PinId, typename NetId>
NetId Netlist<BlockId, PortId, PinId, NetId>::find_net(const typename Netlist<BlockId, PortId, PinId, NetId>::StringId name_id) const {
    VTR_ASSERT_SAFE(valid_string_id(name_id));
    auto iter = net_name_to_net_id_.find(name_id);
    if (iter != net_name_to_net_id_.end()) {
        NetId net_id = *iter;

        if (net_id) {
            //Check post-conditions
            VTR_ASSERT_SAFE(valid_net_id(net_id));
            VTR_ASSERT_SAFE(net_names_[net_id] == name_id);
        }

        return net_id;
    } else {
        return NetId::INVALID();
    }
}
