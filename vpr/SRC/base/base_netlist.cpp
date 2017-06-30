#include "base_netlist.h"
#include "vtr_assert.h"

#include "vpr_error.h"

/*
*
* BaseNetlist class implementation
*
*/
BaseNetlist::BaseNetlist(std::string name, std::string id)
	: netlist_name_(name)
	, netlist_id_(id) {}

/*
*
* Netlist
*
*/
const std::string& BaseNetlist::netlist_name() const {
	return netlist_name_;
}

const std::string& BaseNetlist::netlist_id() const {
	return netlist_id_;
}


/*
*
* Pins
*
*/

NetId BaseNetlist::pin_net (const PinId id) const {
	VTR_ASSERT(valid_pin_id(id));

	return pin_nets_[id];
}

/*
*
* Nets
*
*/
const std::string& BaseNetlist::net_name(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	StringId str_id = net_names_[id];
	return strings_[str_id];
}

BaseNetlist::pin_range BaseNetlist::net_pins(const NetId id) const {
	VTR_ASSERT(valid_net_id(id));

	return vtr::make_range(net_pins_[id].begin(), net_pins_[id].end());
}

/*
*
* Aggregates
*
*/
BaseNetlist::pin_range BaseNetlist::pins() const {
	return vtr::make_range(pin_ids_.begin(), pin_ids_.end());
}

BaseNetlist::net_range BaseNetlist::nets() const {
	return vtr::make_range(net_ids_.begin(), net_ids_.end());
}

/*
*
* Lookups
*
*/
NetId BaseNetlist::find_net(const std::string& name) const {
	auto str_id = find_string(name);
	if (!str_id) {
		return NetId::INVALID();
	}
	else {
		return find_net(str_id);
	}
}

/*
*
* Mutators
*
*/
NetId BaseNetlist::create_net(const std::string name) {
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

	//Check post-conditions: size
	VTR_ASSERT(validate_net_sizes());

	//Check post-conditions: values
	VTR_ASSERT(valid_net_id(net_id));
	VTR_ASSERT(net_name(net_id) == name);
	VTR_ASSERT(find_net(name) == net_id);

	return net_id;
}

NetId BaseNetlist::add_net(const std::string name, PinId driver, std::vector<PinId> sinks) {
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
	pin_nets_[driver] = net_id;
	for (auto sink : sinks) {
		pin_nets_[sink] = net_id;
	}

	return net_id;
}



void BaseNetlist::remove_net(const NetId net_id) {
	VTR_ASSERT(valid_net_id(net_id));

	//Disassociate the pins from the net
	for (auto pin_id : net_pins(net_id)) {
		if (pin_id) {
			pin_nets_[pin_id] = NetId::INVALID();
		}
	}
	//Invalidate look-up
	StringId name_id = net_names_[net_id];
	net_name_to_net_id_.insert(name_id, NetId::INVALID());

	//Mark as invalid
	net_ids_[net_id] = NetId::INVALID();

	//Mark netlist dirty
	dirty_ = true;
}

/*
*
* Sanity Checks
*
*/

bool BaseNetlist::valid_pin_id(PinId id) const {
	if (id == PinId::INVALID()) return false;
	else if (!pin_ids_.contains(id)) return false;
	else if (pin_ids_[id] != id) return false;
	return true;
}

bool BaseNetlist::valid_net_id(NetId id) const {
	if (id == NetId::INVALID()) return false;
	else if (!net_ids_.contains(id)) return false;
	else if (net_ids_[id] != id) return false;
	return true;
}

bool BaseNetlist::valid_string_id(StringId id) const {
	if (id == StringId::INVALID()) return false;
	else if (!string_ids_.contains(id)) return false;
	else if (string_ids_[id] != id) return false;
	return true;
}


bool BaseNetlist::validate_net_sizes() const {
	if (net_names_.size() != net_ids_.size()
		|| net_pins_.size() != net_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent net data sizes");
	}
	return true;
}

bool BaseNetlist::validate_string_sizes() const {
	if (strings_.size() != string_ids_.size()) {
		VPR_THROW(VPR_ERROR_ATOM_NETLIST, "Inconsistent string data sizes");
	}
	return true;
}


/*
*
* Internal utilities
*
*/
BaseNetlist::StringId BaseNetlist::find_string(const std::string& str) const {
	auto iter = string_to_string_id_.find(str);
	if (iter != string_to_string_id_.end()) {
		StringId str_id = iter->second;

		VTR_ASSERT(str_id);
		VTR_ASSERT(strings_[str_id] == str);

		return str_id;
	}
	else {
		return StringId::INVALID();
	}
}


BaseNetlist::StringId BaseNetlist::create_string(const std::string& str) {
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


NetId BaseNetlist::find_net(const StringId name_id) const {
	VTR_ASSERT(valid_string_id(name_id));
	auto iter = net_name_to_net_id_.find(name_id);
	if (iter != net_name_to_net_id_.end()) {
		NetId net_id = *iter;

		if (net_id) {
			//Check post-conditions
			VTR_ASSERT(valid_net_id(net_id));
			VTR_ASSERT(net_names_[net_id] == name_id);
		}

		return net_id;
	}
	else {
		return NetId::INVALID();
	}
}