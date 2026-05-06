/**
 * @file
 * @author  Alex Singer
 * @date    February 2026
 * @brief   Functions for reading an SDC file and updating the timing constraints
 *          on the netlist (in Tatum).
 * 
 * This file uses LibSDCParse to parse the SDC constraints provided. It uses a TCL
 * interpreter to parse each of the commands in the SDC file. This file declares a
 * callback object which provides callback functions which will be called when
 * different SDC constraints are parsed.
 * 
 * Internally, LibSDCParse maintains a database of netlist objects which may be
 * parsed. In the start_parse callback, we populate this internal database with
 * all of the netlist objects that we expect users to refer to in their SDC files.
 * 
 * If no SDC file is provided, the parser is never invoked (the callback is never
 * constructed), and default timing constraints are applied.
 */

#include "read_sdc.h"

#include <filesystem>
#include <limits>
#include <regex>
#include <unordered_map>

#include "tatum/TimingGraphFwd.hpp"
#include "vtr_log.h"
#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_math.h"

#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "sdcparse.hpp"

#include "vpr_error.h"
#include "atom_netlist.h"
#include "atom_netlist_utils.h"
#include "atom_lookup.h"

void apply_default_timing_constraints(const AtomNetlist& netlist,
                                      const AtomLookup& lookup,
                                      const LogicalModels& models,
                                      tatum::TimingConstraints& timing_constraints);

void apply_combinational_default_timing_constraints(const AtomNetlist& netlist,
                                                    const AtomLookup& lookup,
                                                    tatum::TimingConstraints& timing_constraints);

void apply_single_clock_default_timing_constraints(const AtomNetlist& netlist,
                                                   const AtomLookup& lookup,
                                                   const AtomPinId clock_driver,
                                                   tatum::TimingConstraints& timing_constraints);

void apply_multi_clock_default_timing_constraints(const AtomNetlist& netlist,
                                                  const AtomLookup& lookup,
                                                  const std::set<AtomPinId>& clock_drivers,
                                                  tatum::TimingConstraints& timing_constraints);

void mark_constant_generators(const AtomNetlist& netlist,
                              const AtomLookup& lookup,
                              tatum::TimingConstraints& tc);

void constrain_all_ios(const AtomNetlist& netlist,
                       const AtomLookup& lookup,
                       tatum::TimingConstraints& tc,
                       tatum::DomainId input_domain,
                       tatum::DomainId output_domain,
                       tatum::Time input_delay,
                       tatum::Time output_delay);

std::map<std::string, AtomPinId> find_netlist_primary_ios(const AtomNetlist& netlist);
std::string orig_blif_name(std::string name);

std::regex glob_pattern_to_regex(const std::string& glob_pattern);

class SdcParseCallback : public sdcparse::Callback {
  public:
    SdcParseCallback(const AtomNetlist& netlist,
                     const AtomLookup& lookup,
                     const LogicalModels& models,
                     tatum::TimingConstraints& timing_constraints,
                     tatum::TimingGraph& tg)
        : netlist_(netlist)
        , lookup_(lookup)
        , models_(models)
        , tc_(timing_constraints)
        , tg_(tg) {}

  public: //sdcparse::Callback interface
    //Start of parsing
    void start_parse() override {
        netlist_clock_drivers_ = find_netlist_logical_clock_drivers(netlist_, models_);
        netlist_primary_ios_ = find_netlist_primary_ios(netlist_);

        // Create the port objects in the object database.
        std::unordered_set<AtomPinId> ports;
        for (const auto& p : netlist_primary_ios_) {
            sdcparse::PortDirection port_dir;
            switch (netlist_.pin_type(p.second)) {
                case PinType::DRIVER:
                    port_dir = sdcparse::PortDirection::INPUT;
                    break;
                case PinType::SINK:
                    port_dir = sdcparse::PortDirection::OUTPUT;
                    break;
                default:
                    port_dir = sdcparse::PortDirection::UNKNOWN;
                    break;
            }

            bool is_clock_driver = false;
            if (netlist_clock_drivers_.contains(p.second))
                is_clock_driver = true;

            sdcparse::ObjectId port_id = obj_database.create_port_object(p.first, port_dir, is_clock_driver);
            object_to_port_id_[port_id] = p.second;
            ports.insert(p.second);
        }

        // Create the pin objects in the object database.
        for (AtomPinId pin : netlist_.pins()) {
            // Ports are not pins.
            if (ports.count(pin) != 0)
                continue;

            const std::string& pin_name = netlist_.pin_name(pin);

            bool is_clock_driver = false;
            if (netlist_clock_drivers_.contains(pin))
                is_clock_driver = true;

            sdcparse::ObjectId pin_id = obj_database.create_pin_object(pin_name, is_clock_driver);
            object_to_pin_id_[pin_id] = pin;
        }

        // Create the net objects in the object database.
        for (AtomNetId net : netlist_.nets()) {
            const std::string& net_name = netlist_.net_name(net);
            for (const std::string& net_alias : netlist_.net_aliases(net_name)) {
                sdcparse::ObjectId net_id = obj_database.create_net_object(net_alias);
                object_to_net_id_[net_id] = net;
            }
        }
    }

    //Sets current filename
    void filename(std::string fname) override { fname_ = fname; }

    //Sets current line number
    void lineno(int line_num) override { lineno_ = line_num; }

    //Individual commands
    void create_clock(const sdcparse::CreateClock& cmd) override {
        num_commands_++;

        if (cmd.add) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "-add option not supported for create_clock");
        }

        if (cmd.is_virtual) {
            if (cmd.name.empty()) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                          "clocks with no targets must have a name");
            }

            // Virtual clocks should have no targets
            if (!cmd.targets.empty()) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                          "Virtual clock definition (i.e. with '-name') should not have targets");
            }
        } else {
            // Check that all of the objects are valid types.
            bool targets_valid = check_objects(cmd.targets,
                                               {sdcparse::ObjectType::Port, sdcparse::ObjectType::Pin, sdcparse::ObjectType::Net});
            if (!targets_valid) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                          "create_clock command only supports ports, pins, and nets");
            }
        }

        if (cmd.is_virtual) {
            // Create a virtual clock
            create_clock_object(cmd.name, tatum::NodeId::INVALID(), cmd);
            return;
        }

        // Collect all of the target pins and their names. The names of the objects
        // are used for the names of the clocks when no name is provided.
        std::map<AtomPinId, std::string> target_pins = get_clock_target_pins(cmd.targets);

        // Check that if the clock is named, there is only one unique clock driver.
        if (target_pins.size() > 1 && !cmd.name.empty()) {
            for (const auto& p : target_pins) {
                VTR_LOG("Name: %s\n", p.second.c_str());
            }
            // NOTE: The reason this is not supported is because we currently
            //       create unique clock domains for each target. This may not
            //       be standard and they cannot all be named the same thing.
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "named clocks with more than 1 unique target are not supported for create_clock");
        }

        // Create a netlist clock for every target pin.
        for (const auto& p : target_pins) {
            AtomPinId clock_pin = p.first;
            const std::string& object_name = p.second;

            // Get the name of the clock. If no name is provided, the name of the port/pin is used.
            std::string clock_name;
            if (cmd.name.empty()) {
                clock_name = object_name;
            } else {
                VTR_ASSERT(target_pins.size() == 1);
                clock_name = cmd.name;
            }

            // Get the clock source associated with this pin.
            tatum::NodeId clock_source = get_clock_source(clock_pin);
            VTR_ASSERT(clock_source.is_valid());

            // Create netlist clock (a clock net which ultimately drives a
            // clock pin on some block in the design netlist)
            create_clock_object(clock_name, clock_source, cmd);
        }
    }

    void create_generated_clock(const sdcparse::CreateGeneratedClock& cmd) override {
        num_commands_++;

        // Check that the arguments to the command are valid.
        if (cmd.add) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "-add option not supported for create_generated_clock");
        }

        if (!cmd.edges.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "-edges option not supported for create_generated_clock");
        }

        if (!cmd.edge_shift.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "-edge_shift option not supported for create_generated_clock");
        }

        if (cmd.invert) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "-invert not supported for create_generated_clock");
        }

        if (!std::isnan(cmd.duty_cycle)) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "-duty_cycle option not supported for create_generated_clock");
        }

        if (cmd.sources.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "-source is required for create_generated_clock");
        }

        if (cmd.divide_by == sdcparse::UNINITIALIZED_INT && cmd.multiply_by == sdcparse::UNINITIALIZED_INT) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Either -divide_by or -multiply_by is required for create_generated_clock");
        }
        if ((cmd.divide_by != sdcparse::UNINITIALIZED_INT && cmd.divide_by <= 0) || (cmd.multiply_by != sdcparse::UNINITIALIZED_INT && cmd.multiply_by <= 0)) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "-divide_by and -multiply_by must be strictly positive for create_generated_clock");
        }

        // If no targets are provided, the generated clock is virtual.
        bool is_virtual = cmd.targets.empty();
        if (is_virtual) {
            // Virtual clocks must be named.
            if (cmd.name.empty()) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                          "clocks with no targets (i.e. virtual clocks) must have a name");
            }
        } else {
            // Check that all of the objects are valid types.
            bool targets_valid = check_objects(cmd.targets, {sdcparse::ObjectType::Port, sdcparse::ObjectType::Pin, sdcparse::ObjectType::Net});
            if (!targets_valid) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                          "create_generated_clock command only supports ports, pins, and nets");
            }
        }

        // Get the source clock.
        //  Get the clock pins referred to by the source argument.
        std::map<AtomPinId, std::string> source_clock_pins = get_clock_target_pins(cmd.sources);
        if (source_clock_pins.size() > 1) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "create_generated_clock can only have 1 source clock, multiple found.");
        }
        if (source_clock_pins.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Cannot find source pin for create_generated_clock");
        }
        //  Get the clock domain for the found pins.
        AtomPinId source_clock_pin = source_clock_pins.begin()->first;
        tatum::NodeId source_clock_tnode = get_clock_source(source_clock_pin);
        tatum::DomainId source_domain_id;
        for (tatum::DomainId domain_id : tc_.clock_domains()) {
            if (tc_.clock_domain_source_node(domain_id) == source_clock_tnode) {
                if (source_domain_id.is_valid()) {
                    vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                              "create_generated_clock source pin matches multiple clocks.");
                }
                source_domain_id = domain_id;
            }
        }
        if (!source_domain_id.is_valid()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Cannot find source clock for create_generated_clock. Make sure that the source clock has been created using create_clock.");
        }
        //  Find the command that created this clock.
        VTR_ASSERT(sdc_clocks_.contains(source_domain_id));
        const sdcparse::CreateClock& source_clock_cmd = sdc_clocks_[source_domain_id];

        // Get the generated clock period.
        double source_clock_period = source_clock_cmd.period;
        double generated_clock_period = sdcparse::UNINITIALIZED_FLOAT;
        if (cmd.divide_by != sdcparse::UNINITIALIZED_INT) {
            // Dividing the frequency means multiplying the period.
            generated_clock_period = source_clock_period * static_cast<double>(cmd.divide_by);
        } else {
            VTR_ASSERT(cmd.multiply_by != sdcparse::UNINITIALIZED_INT);
            // Similarly, multiplying the frequency means dividing the period.
            generated_clock_period = source_clock_period / static_cast<double>(cmd.multiply_by);
        }

        // Get the rise and fall edge.
        // Since we can only multiply and divide the clock, the generated clock
        // can only have a 50% duty cycle.
        // TODO: Need to add -edges support so other duty cycles can be expressed.
        if (source_clock_cmd.rise_edge != 0.0) {
            // Note: This is not supported because it shifts where the rise time
            //       of the generated clock should be. It is not clear how this
            //       shift should be performed.
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Creating a generated clock on a source with a non-zero rising edge is currently unsupported in VPR.");
        }
        double generated_rise_edge = 0.0;
        double generated_fall_edge = generated_clock_period / 2.0;

        if (is_virtual) {
            // Create a virtual clock.
            // See the note below, this cmd is only used to lookup information
            // on the clocks.
            // TODO: Make this interface better.
            sdcparse::CreateClock new_create_clock_cmd;
            new_create_clock_cmd.add = false;
            new_create_clock_cmd.is_virtual = true;
            new_create_clock_cmd.name = cmd.name;
            new_create_clock_cmd.targets = cmd.targets;
            new_create_clock_cmd.period = generated_clock_period;
            new_create_clock_cmd.rise_edge = generated_rise_edge;
            new_create_clock_cmd.fall_edge = generated_fall_edge;

            create_clock_object(cmd.name, tatum::NodeId::INVALID(), new_create_clock_cmd);
        }

        // Get the targets pins.
        std::map<AtomPinId, std::string> target_pins = get_clock_target_pins(cmd.targets);

        // Check that if the clock is named, there is only one unique clock driver.
        if (target_pins.size() > 1 && !cmd.name.empty()) {
            for (const auto& p : target_pins) {
                VTR_LOG("Name: %s\n", p.second.c_str());
            }
            // NOTE: The reason this is not supported is because we currently
            //       create unique clock domains for each target. This may not
            //       be standard and they cannot all be named the same thing.
            //       This is the same issue in create_clock.
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "named clocks with more than 1 unique target are not supported for create_generated_clock");
        }

        // Create a netlist clock for every target pin.
        for (const auto& p : target_pins) {
            AtomPinId clock_pin = p.first;
            const std::string& object_name = p.second;

            // Get the name of the clock. If no name is provided, the name of the object (clock pin, port, or net) is used.
            std::string clock_name;
            if (cmd.name.empty()) {
                clock_name = object_name;
            } else {
                VTR_ASSERT(target_pins.size() == 1);
                clock_name = cmd.name;
            }

            // Get the clock source associated with this pin.
            tatum::NodeId clock_source = get_clock_source(clock_pin);
            VTR_ASSERT(clock_source.is_valid());

            // Create equivalent create_clock commands for this generated clock.
            // NOTE: This is a bit of a hack, but the command is used throughout
            //       this code to quickly lookup information on the clock.
            // TODO: Consider using a better storage for this information instead
            //       of the command.
            sdcparse::CreateClock new_create_clock_cmd;
            new_create_clock_cmd.add = false;
            new_create_clock_cmd.is_virtual = false;
            new_create_clock_cmd.name = clock_name;
            new_create_clock_cmd.targets = cmd.targets;
            new_create_clock_cmd.period = generated_clock_period;
            new_create_clock_cmd.rise_edge = generated_rise_edge;
            new_create_clock_cmd.fall_edge = generated_fall_edge;

            // Create the netlist clock (a clock net which ultimately drives a
            // clock pin on some block in the design netlist).
            create_clock_object(clock_name, clock_source, new_create_clock_cmd);
        }
    }

    void set_io_delay(const sdcparse::SetIoDelay& cmd) override {
        num_commands_++;

        if (cmd.associated_clocks.empty()) {
            // TODO: This should be relaxed.
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_io_delay currently requires the clock to be specified");
        }

        bool clocks_valid = check_objects(cmd.associated_clocks,
                                          {sdcparse::ObjectType::Clock});
        if (!clocks_valid) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_io_delay command only supports clock objects for -clock");
        }

        //Error checks
        std::set<tatum::DomainId> domains = get_clocks(cmd.associated_clocks);
        if (domains.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Failed to find clock domain for I/O constraint");
        }

        // Verify that the targets are the correct type.
        // TODO: We may be able to support pins as well. Need to verify.
        bool targets_valid = check_objects(cmd.target_ports,
                                           {sdcparse::ObjectType::Port});
        if (!targets_valid) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_io_delay command only supports ports currently");
        }

        // Get the target ports
        std::set<AtomPinId> io_pins = get_ports(cmd.target_ports);

        if (io_pins.empty()) {
            //We treat this as a warning, since the primary I/Os in the target may have been swept away
            VTR_LOGF_WARN(fname_.c_str(), lineno_,
                          "Found no matching primary inputs or primary outputs for %s\n",
                          (cmd.type == sdcparse::IoDelayType::INPUT) ? "set_input_delay" : "set_output_delay");
        }

        bool is_max = cmd.is_max;
        bool is_min = cmd.is_min;
        if (!is_max && !is_min) {
            //Unspecified implies both
            is_max = true;
            is_min = true;
        }

        float delay = sdc_units_to_seconds(cmd.delay);

        for (AtomPinId pin : io_pins) {
            tatum::NodeId tnode = lookup_.atom_pin_tnode(pin);
            VTR_ASSERT(tnode);

            //Set i/o constraint
            if (cmd.type == sdcparse::IoDelayType::INPUT) {
                if (netlist_.pin_type(pin) == PinType::DRIVER) {
                    for (tatum::DomainId domain : domains) {
                        if (is_max) {
                            tc_.set_input_constraint(tnode, domain, tatum::DelayType::MAX, tatum::Time(delay));
                        }
                        if (is_min) {
                            tc_.set_input_constraint(tnode, domain, tatum::DelayType::MIN, tatum::Time(delay));
                        }
                    }
                } else {
                    VTR_ASSERT(netlist_.pin_type(pin) == PinType::SINK);

                    AtomBlockId blk = netlist_.pin_block(pin);
                    std::string io_name = orig_blif_name(netlist_.block_name(blk));

                    VTR_LOGF_WARN(fname_.c_str(), lineno_,
                                  "set_input_delay command matched but was not applied to primary output '%s'\n",
                                  io_name.c_str());
                }
            } else {
                VTR_ASSERT(cmd.type == sdcparse::IoDelayType::OUTPUT);

                if (netlist_.pin_type(pin) == PinType::SINK) {
                    for (tatum::DomainId domain : domains) {
                        if (is_max) {
                            tc_.set_output_constraint(tnode, domain, tatum::DelayType::MAX, tatum::Time(delay));
                        }
                        if (is_min) {
                            tc_.set_output_constraint(tnode, domain, tatum::DelayType::MIN, tatum::Time(delay));
                        }
                    }
                } else {
                    VTR_ASSERT(netlist_.pin_type(pin) == PinType::DRIVER);
                    AtomBlockId blk = netlist_.pin_block(pin);
                    std::string io_name = orig_blif_name(netlist_.block_name(blk));

                    VTR_LOGF_WARN(fname_.c_str(), lineno_,
                                  "set_output_delay command matched but was not applied to primary input '%s'\n",
                                  io_name.c_str());
                }
            }
        }
    }

    void set_clock_groups(const sdcparse::SetClockGroups& cmd) override {
        num_commands_++;

        if (cmd.type != sdcparse::ClockGroupsType::ASYNCHRONOUS) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_clock_groups only supports -asynchronous groups");
        }

        for (const auto& clock_group : cmd.clock_groups) {
            bool clock_group_valid = check_objects(clock_group,
                                                   {sdcparse::ObjectType::Clock});
            if (!clock_group_valid) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                          "set_clock_groups only supports clock targets");
            }
        }

        //FIXME: more efficient to collect per-group clocks once instead of at each iteration

        //Disable timing between each group of clock domains
        for (size_t src_group = 0; src_group < cmd.clock_groups.size(); ++src_group) {
            auto src_clocks = get_clocks(cmd.clock_groups[src_group]);

            for (size_t sink_group = 0; sink_group < cmd.clock_groups.size(); ++sink_group) {
                if (src_group == sink_group) continue;

                auto sink_clocks = get_clocks(cmd.clock_groups[sink_group]);

                for (auto src_clock : src_clocks) {
                    for (auto sink_clock : sink_clocks) {
                        //Mark this pair of domains to be disabled
                        disabled_domain_pairs_.insert({src_clock, sink_clock});
                    }
                }
            }
        }
    }

    void set_false_path(const sdcparse::SetFalsePath& cmd) override {
        num_commands_++;

        bool from_targets_valid = check_objects(cmd.from,
                                                {sdcparse::ObjectType::Clock});
        bool to_targets_valid = check_objects(cmd.to,
                                              {sdcparse::ObjectType::Clock});
        if (!(from_targets_valid && to_targets_valid)) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_false_path: False paths are currently only supported between entire clock domains");
        }

        auto from_clocks = get_clocks(cmd.from);
        auto to_clocks = get_clocks(cmd.to);

        if (from_clocks.empty() && to_clocks.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_false_path must specify at least one -from or -to clock");
        }

        if (from_clocks.empty()) {
            from_clocks = get_all_clocks();
        }

        if (to_clocks.empty()) {
            to_clocks = get_all_clocks();
        }

        for (auto from_clock : from_clocks) {
            for (auto to_clock : to_clocks) {
                //Mark this domain pair to be disabled
                disabled_domain_pairs_.insert({from_clock, to_clock});
            }
        }
    }

    void set_min_max_delay(const sdcparse::SetMinMaxDelay& cmd) override {
        num_commands_++;

        bool from_targets_valid = check_objects(cmd.from,
                                                {sdcparse::ObjectType::Clock});
        bool to_targets_valid = check_objects(cmd.to,
                                              {sdcparse::ObjectType::Clock});
        if (!(from_targets_valid && to_targets_valid)) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_min/max_delay: Max/Min delays are currently only supported between entire clock domains");
        }

        auto from_clocks = get_clocks(cmd.from);
        auto to_clocks = get_clocks(cmd.to);

        if (from_clocks.empty() && to_clocks.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_max_path must specify at least one -from or -to clock");
        }

        if (from_clocks.empty()) {
            from_clocks = get_all_clocks();
        }

        if (to_clocks.empty()) {
            to_clocks = get_all_clocks();
        }

        float constraint = cmd.value;

        for (auto from_clock : from_clocks) {
            for (auto to_clock : to_clocks) {
                //Mark this domain pair to be disabled
                auto key = std::make_pair(from_clock, to_clock);

                if (cmd.type == sdcparse::MinMaxType::MAX) {
                    setup_override_constraints_[key] = constraint;
                } else {
                    VTR_ASSERT(cmd.type == sdcparse::MinMaxType::MIN);
                    hold_override_constraints_[key] = constraint;
                }
            }
        }
    }

    void set_multicycle_path(const sdcparse::SetMulticyclePath& cmd) override {
        num_commands_++;

        if (cmd.from.empty() && cmd.to.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_multicycle_path missing both -from and -to");
        }

        bool from_targets_valid = check_objects(cmd.from,
                                                {sdcparse::ObjectType::Clock});
        if (!from_targets_valid) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_multicycle_path only supports specifying clocks for -from");
        }

        bool to_targets_valid = check_objects(cmd.to,
                                              {sdcparse::ObjectType::Clock, sdcparse::ObjectType::Pin});
        if (!to_targets_valid) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_multicycle_path only supports specifying clocks or pins for -to");
        }

        std::set<tatum::DomainId> from_clocks = get_clocks(cmd.from);
        if (from_clocks.empty()) {
            from_clocks = get_all_clocks();
        }

        std::set<tatum::DomainId> to_clocks = get_clocks(cmd.to);
        std::set<AtomPinId> to_pins = get_pins(cmd.to);

        // TODO: Added this check to match functionality of old parser.
        //       this can probably be relaxed, but needs to be tested.
        if (!to_clocks.empty() && !to_pins.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_multicycle_path -to targeting both clocks and pins not supported currently");
        }

        if (to_clocks.empty()) {
            to_clocks = get_all_clocks();
        }
        if (to_pins.empty()) {
            //Treat INVALID pin as wildcard
            to_pins = {AtomPinId::INVALID()};
        }

        int setup_mcp = cmd.mcp_value;
        int hold_mcp = cmd.mcp_value;

        bool is_setup = cmd.is_setup;
        bool is_hold = cmd.is_hold || is_setup; //Specifying a setup mcp also modifies hold
        if (!is_hold && !is_setup) {
            //Unspecified implicitly sets the setup mcp to the target value,
            //and the hold mcp to zero
            is_setup = true;
            is_hold = true;

            VTR_ASSERT(setup_mcp == cmd.mcp_value);
            hold_mcp = 0; //Default hold check is 0
        }

        for (auto from_clock : from_clocks) {
            for (auto to_clock : to_clocks) {
                for (auto to_pin : to_pins) {
                    auto node_domain = std::make_tuple(from_clock, to_clock, to_pin);

                    if (is_setup) {
                        setup_mcp_overrides_[node_domain] = setup_mcp;
                    }

                    if (is_hold) {
                        hold_mcp_overrides_[node_domain] = hold_mcp;
                    }
                }
            }
        }
    }

    void set_clock_uncertainty(const sdcparse::SetClockUncertainty& cmd) override {
        num_commands_++;

        bool from_targets_valid = check_objects(cmd.from,
                                                {sdcparse::ObjectType::Clock});
        bool to_targets_valid = check_objects(cmd.to,
                                              {sdcparse::ObjectType::Clock});
        if (!(from_targets_valid && to_targets_valid)) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_clock_uncertainty only supported between entire clock domains");
        }

        auto from_clocks = get_clocks(cmd.from);
        auto to_clocks = get_clocks(cmd.to);

        if (from_clocks.empty()) {
            from_clocks = get_all_clocks();
        }

        if (to_clocks.empty()) {
            to_clocks = get_all_clocks();
        }

        float uncertainty = sdc_units_to_seconds(cmd.value);

        bool is_setup = cmd.is_setup;
        bool is_hold = cmd.is_hold;
        if (!is_hold && !is_setup) {
            //Unspecified is implicitly both setup and hold
            is_setup = true;
            is_hold = true;
        }

        for (auto from_clock : from_clocks) {
            for (auto to_clock : to_clocks) {
                if (is_setup) {
                    tc_.set_setup_clock_uncertainty(from_clock, to_clock, tatum::Time(uncertainty));
                }

                if (is_hold) {
                    tc_.set_hold_clock_uncertainty(from_clock, to_clock, tatum::Time(uncertainty));
                }
            }
        }
    }

    void set_clock_latency(const sdcparse::SetClockLatency& cmd) override {
        num_commands_++;

        if (cmd.type != sdcparse::ClockLatencyType::SOURCE) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_clock_latency only supports specifying -source latency");
        }

        bool targets_valid = check_objects(cmd.target_clocks,
                                           {sdcparse::ObjectType::Clock});
        if (!targets_valid) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_clock_latency only supported for clock domain targets");
        }

        auto clocks = get_clocks(cmd.target_clocks);
        if (clocks.empty()) {
            clocks = get_all_clocks();
        }

        bool is_early = cmd.is_early;
        bool is_late = cmd.is_late;
        if (!is_early && !is_late) {
            //Unspecified is implicitly both early and late
            is_early = true;
            is_late = true;
        }

        float latency = sdc_units_to_seconds(cmd.value);

        for (auto clock : clocks) {
            if (is_early) {
                tc_.set_source_latency(clock, tatum::ArrivalType::EARLY, tatum::Time(latency));
            }
            if (is_late) {
                tc_.set_source_latency(clock, tatum::ArrivalType::LATE, tatum::Time(latency));
            }
        }
    }

    void set_disable_timing(const sdcparse::SetDisableTiming& cmd) override {
        num_commands_++;

        bool from_targets_valid = check_objects(cmd.from,
                                                {sdcparse::ObjectType::Pin});
        bool to_targets_valid = check_objects(cmd.to,
                                              {sdcparse::ObjectType::Pin});
        if (!(from_targets_valid && to_targets_valid)) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_disable_timing only supported between pins");
        }

        //Collect the specified pins
        auto from_pins = get_pins(cmd.from);
        auto to_pins = get_pins(cmd.to);

        if (from_pins.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Found no matching -from pins");
        }

        if (to_pins.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Found no matching -to pins");
        }

        //Disable any edges between the from and to sets
        for (auto from_pin : from_pins) {
            for (auto to_pin : to_pins) {
                tatum::NodeId from_tnode = lookup_.atom_pin_tnode(from_pin);
                VTR_ASSERT(from_tnode);

                tatum::NodeId to_tnode = lookup_.atom_pin_tnode(to_pin);
                VTR_ASSERT(to_tnode);

                //Find the edge matching these nodes
                tatum::EdgeId edge = tg_.find_edge(from_tnode, to_tnode);

                if (!edge) {
                    const auto& from_pin_name = netlist_.pin_name(from_pin);
                    const auto& to_pin_name = netlist_.pin_name(to_pin);

                    VTR_LOGF_WARN(fname_.c_str(), lineno_,
                                  "set_disable_timing no timing edge found from pin '%s' to pin '%s'\n",
                                  from_pin_name.c_str(), to_pin_name.c_str());
                }

                //Mark the edge in the timing graph as disabled
                tg_.disable_edge(edge);

                //If we have disabled all incoming edges of the to_tnode we need to mark
                //it as a constant generator to avoid causing errors during timing analysis
                //(since the node will appear in the first level of the timing graph but is not
                //a primary input).
                if (tg_.node_num_active_in_edges(to_tnode) == 0) {
                    VTR_LOGF_WARN(fname_.c_str(), lineno_,
                                  "set_disable_timing caused pin '%s' to have no active incoming edges. It is being marked as a constant generator.\n",
                                  netlist_.pin_name(to_pin).c_str());
                    tc_.set_constant_generator(to_tnode);
                }
            }
        }
    }

    void set_timing_derate(const sdcparse::SetTimingDerate& /*cmd*/) override {
        num_commands_++;
        vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_timing_derate currently unsupported");
    }

    //End of parsing
    void finish_parse() override {
        //Mark constant generator timing nodes
        mark_constant_generators(netlist_, lookup_, tc_);

        //Determine the final clock constraints
        resolve_clock_constraints();

        //Re-levelize if needed (e.g. due to set_disable_timing)
        tg_.levelize();
    }

    //Error during parsing
    void parse_error(const int /*curr_lineno*/, const std::string& near_text, const std::string& msg) override {
        vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "%s near '%s'", msg.c_str(), near_text.c_str());
    }

    // Warning during parsing.
    void parse_warning(const std::string& msg) override {
        VTR_LOGF_WARN(fname_.c_str(), lineno_,
                      "%s\n",
                      msg.c_str());
    }

    // Log error messages produced while parsing.
    void log_error_msg(const std::string& msg) override {
        // Here, we are using VTR_LOG since we want the error message to be
        // organized nicely in the log file.
        VTR_LOG("%s", msg.c_str());
    }

  public:
    size_t num_commands() { return num_commands_; }

  private:
    void resolve_clock_constraints() {
        //Set the clock constraints
        for (tatum::DomainId launch_clock : tc_.clock_domains()) {
            for (tatum::DomainId capture_clock : tc_.clock_domains()) {
                auto domain_pair = std::make_pair(launch_clock, capture_clock);

                if (disabled_domain_pairs_.count(domain_pair)) continue;

                //Setup -- default
                {
                    tatum::Time setup_constraint = calculate_setup_constraint(launch_clock, capture_clock);
                    VTR_ASSERT(setup_constraint.valid());

                    tc_.set_setup_constraint(launch_clock, capture_clock, setup_constraint);
                }

                //Setup -- capture pin overrides
                for (auto pin : setup_capture_pins_with_overrides()) {
                    tatum::Time setup_constraint = calculate_setup_constraint(launch_clock, capture_clock, pin);
                    VTR_ASSERT(setup_constraint.valid());

                    tatum::NodeId tnode = lookup_.atom_pin_tnode(pin);
                    VTR_ASSERT(tnode);

                    tc_.set_setup_constraint(launch_clock, capture_clock, tnode, setup_constraint);
                }

                //Hold -- default
                {
                    tatum::Time hold_constraint = calculate_hold_constraint(launch_clock, capture_clock);
                    VTR_ASSERT(hold_constraint.valid());

                    tc_.set_hold_constraint(launch_clock, capture_clock, hold_constraint);
                }

                //Setup -- capture pin overrides
                for (auto pin : hold_capture_pins_with_overrides()) {
                    tatum::Time hold_constraint = calculate_hold_constraint(launch_clock, capture_clock, pin);
                    VTR_ASSERT(hold_constraint.valid());

                    tatum::NodeId tnode = lookup_.atom_pin_tnode(pin);
                    VTR_ASSERT(tnode);

                    tc_.set_hold_constraint(launch_clock, capture_clock, tnode, hold_constraint);
                }
            }
        }
    }

    //Returns the setup constraint in seconds
    tatum::Time calculate_setup_constraint(tatum::DomainId launch_domain, tatum::DomainId capture_domain, AtomPinId to_pin = AtomPinId::INVALID()) const {
        //Calculate the period-based constraint, including the effect of multi-cycle paths
        float min_launch_to_capture_time = calculate_min_launch_to_capture_edge_time(launch_domain, capture_domain);

        auto iter = sdc_clocks_.find(capture_domain);
        VTR_ASSERT(iter != sdc_clocks_.end());
        float capture_period = iter->second.period;

        //The period based constraint is the minimum launch to capture edge time + the capture period * (extra_cycles)
        //
        // Since min_launch_to_capture_time is the minimum time to the first capture edge after a launch edge, it already
        // implicitly includes one capture cycle. As a result we subtract 1 from the setup capture cycle value to determine
        // how many extra capture cycles need to be added to the constraint.
        //
        // By default the setup capture cycle 1, specifying a capture 1 cycle after launch
        int extra_cycles = setup_capture_cycle(launch_domain, capture_domain, to_pin) - 1;
        float period_based_setup_constraint = min_launch_to_capture_time + capture_period * extra_cycles;

        //Warn the user if we added negative cycles
        if (extra_cycles < 0) {
            VTR_LOG_WARN(
                "Added negative (%d) additional capture clock cycles to setup constraint"
                " for clock '%s' to clock '%s' transfers; check your set_multicycle_path specifications\n",
                extra_cycles, tc_.clock_domain_name(launch_domain).c_str(), tc_.clock_domain_name(capture_domain).c_str());
        }

        //By default the period-based constraint is the constraint
        float setup_constraint = period_based_setup_constraint;

        //See if we have any other override constraints
        auto domain_pair = std::make_pair(launch_domain, capture_domain);
        auto override_iter = setup_override_constraints_.find(domain_pair);
        if (override_iter != setup_override_constraints_.end()) {
            float override_setup_constraint = override_iter->second;

            if (setup_constraint > override_setup_constraint) {
                VTR_LOG_WARN(
                    "Override setup constraint (%g) overrides a tighter default period-based constraint (%g)"
                    " for transfers from clock '%s' to clock '%s'\n",
                    override_setup_constraint, setup_constraint,
                    tc_.clock_domain_name(launch_domain).c_str(),
                    tc_.clock_domain_name(capture_domain).c_str());
            }

            //Override the constraint
            setup_constraint = override_setup_constraint;
        }

        setup_constraint = sdc_units_to_seconds(setup_constraint);

        if (setup_constraint < 0.) {
            VPR_ERROR(VPR_ERROR_SDC,
                      "Setup constraint %g for transfers from clock '%s' to clock '%s' is negative."
                      " Requires data to arrive before launch edge (No time travelling allowed!)",
                      setup_constraint,
                      tc_.clock_domain_name(launch_domain).c_str(),
                      tc_.clock_domain_name(capture_domain).c_str());
        }

        return tatum::Time(setup_constraint);
    }

    //Returns the hold constraint in seconds
    tatum::Time calculate_hold_constraint(tatum::DomainId launch_domain, tatum::DomainId capture_domain, AtomPinId to_pin = AtomPinId::INVALID()) const {
        float min_launch_to_capture_time = calculate_min_launch_to_capture_edge_time(launch_domain, capture_domain);

        auto iter = sdc_clocks_.find(capture_domain);
        VTR_ASSERT(iter != sdc_clocks_.end());
        float capture_period = iter->second.period;

        //The period based constraint is the minimum launch to capture edge time + the capture period * extra_cycles
        //
        // Since min_launch_to_capture_time is the minimum time to the first capture edge *after* a launch edge, it already
        // implicitly includes one capture cycle. As a result we subtract 1 from the hold capture cycle value to determine
        // how many extra capture cycles need to be added to the constraint.
        //
        // For the default hold check is one cycle before the setup check
        // For the default setup check (1) this means extra_cycles is -1 (i.e. the hold capture check occurs against
        // the capture edge *before* the launch edge)
        int extra_cycles = hold_capture_cycle(launch_domain, capture_domain, to_pin) - 1;
        float period_based_hold_constraint = min_launch_to_capture_time + capture_period * extra_cycles;

        //By default the period-based constraint is the constraint
        float hold_constraint = period_based_hold_constraint;

        //See if we have any other override constraints
        auto domain_pair = std::make_pair(launch_domain, capture_domain);
        auto override_iter = hold_override_constraints_.find(domain_pair);
        if (override_iter != hold_override_constraints_.end()) {
            float override_hold_constraint = override_iter->second;

            if (hold_constraint < override_hold_constraint) {
                VTR_LOG_WARN(
                    "Override hold constraint (%g) overrides tighter default period-based constraint (%g)"
                    " for transfers from clock '%s' to clock '%s'\n",
                    override_hold_constraint, hold_constraint,
                    tc_.clock_domain_name(launch_domain).c_str(),
                    tc_.clock_domain_name(capture_domain).c_str());
            }

            //Override the constraint
            hold_constraint = override_hold_constraint;
        }

        hold_constraint = sdc_units_to_seconds(hold_constraint);

        return tatum::Time(hold_constraint);
    }

    //Determine the minimum time (in SDC units) between the edges of the launch and capture clocks
    float calculate_min_launch_to_capture_edge_time(tatum::DomainId launch_domain, tatum::DomainId capture_domain) const {
        constexpr int CLOCK_SCALE = 1000;

        auto launch_iter = sdc_clocks_.find(launch_domain);
        VTR_ASSERT(launch_iter != sdc_clocks_.end());
        const sdcparse::CreateClock& launch_clock = launch_iter->second;

        auto capture_iter = sdc_clocks_.find(capture_domain);
        VTR_ASSERT(capture_iter != sdc_clocks_.end());
        const sdcparse::CreateClock& capture_clock = capture_iter->second;

        VTR_ASSERT_MSG(launch_clock.period >= 0., "Clock period must be positive");
        VTR_ASSERT_MSG(capture_clock.period >= 0., "Clock period must be positive");

        float constraint = std::numeric_limits<float>::quiet_NaN();
        if (vtr::isclose(launch_clock.period, capture_clock.period)
            && vtr::isclose(launch_clock.rise_edge, capture_clock.rise_edge)
            && vtr::isclose(launch_clock.fall_edge, capture_clock.fall_edge)) {
            //The source and sink domains have the same period and edges, the constraint is the common clock period.

            constraint = launch_clock.period;

        } else if (vtr::isclose(launch_clock.period, 0.0) || vtr::isclose(capture_clock.period, 0.0)) {
            //If either period is 0, the constraint is 0
            constraint = 0.;

        } else {
            /*
             * Use edge counting to find the minimum launch to capture edge time
             */

            //Multiply periods and edges by CLOCK_SCALE and round down to the nearest
            //integer, to avoid messy decimals.
            int launch_period = static_cast<int>(launch_clock.period * CLOCK_SCALE);
            int capture_period = static_cast<int>(capture_clock.period * CLOCK_SCALE);
            int launch_rise_edge = static_cast<int>(launch_clock.rise_edge * CLOCK_SCALE);
            int capture_rise_edge = static_cast<int>(capture_clock.rise_edge * CLOCK_SCALE);

            //Find the LCM of the two periods. This determines how long it takes before
            //the pattern of the two clock's edges starts repeating.
            int lcm_period = vtr::lcm(launch_period, capture_period);

            //Create arrays of positive edges for each clock over one LCM clock period.

            //Launch edges
            int launch_rise_time = launch_rise_edge;
            std::vector<int> launch_edges;
            int num_launch_edges = lcm_period / launch_period + 1;
            for (int i = 0; i < num_launch_edges; ++i) {
                launch_edges.push_back(launch_rise_time);
                launch_rise_time += launch_period;
            }

            //Capture edges
            int capture_rise_time = capture_rise_edge;
            int num_capture_edges = lcm_period / capture_period + 1;
            std::vector<int> capture_edges;
            for (int i = 0; i < num_capture_edges; ++i) {
                capture_edges.push_back(capture_rise_time);
                capture_rise_time += capture_period;
            }

            //Compare every edge in source_edges with every edge in sink_edges.
            //The lowest STRICTLY POSITIVE difference between a sink edge and a
            //source edge yields the setup time constraint.
            int scaled_constraint = std::numeric_limits<int>::max(); //Initialize to +inf, so any constraint will be less

            for (int launch_edge : launch_edges) {
                for (int capture_edge : capture_edges) {
                    if (capture_edge >= launch_edge) { //Positive only
                        int edge_diff = capture_edge - launch_edge;
                        VTR_ASSERT(edge_diff >= 0.);

                        scaled_constraint = std::min(scaled_constraint, edge_diff);
                    }
                }
            }

            //Rescale the constraint back to a float
            constraint = float(scaled_constraint) / CLOCK_SCALE;
        }

        return constraint;
    }

    //Returns the cycle number (after launch) where the setup check occurs
    int setup_capture_cycle(tatum::DomainId from, tatum::DomainId to, AtomPinId to_pin = AtomPinId::INVALID()) const {
        //The setup capture cycle is the setup mcp value

        //Any domain pair + pin-specific (possibly wildcard) override
        auto key = std::make_tuple(from, to, to_pin);
        auto iter = setup_mcp_overrides_.find(key);
        if (iter != setup_mcp_overrides_.end()) {
            return iter->second;
        }

        //Pin-specific override not found, look for Domain pair overrides
        key = std::make_tuple(from, to, AtomPinId::INVALID());
        iter = setup_mcp_overrides_.find(key);
        if (iter != setup_mcp_overrides_.end()) {
            return iter->second;
        }

        //Default: capture one cycle after launch
        return 1;
    }

    //Returns the cycle number (after launch) where the hold check occurs
    int hold_capture_cycle(tatum::DomainId from, tatum::DomainId to, AtomPinId to_pin = AtomPinId::INVALID()) const {
        //Default: hold captures the cycle before setup is captured
        //For the default setup mcp this implies capturing the same
        //cycle as launch
        int hold_offset = 1;

        //Any domain pair + pin-specific (possibly wildcard) override
        auto key = std::make_tuple(from, to, to_pin);
        auto iter = hold_mcp_overrides_.find(key);
        if (iter != hold_mcp_overrides_.end()) {
            //Note that we add the override to the default hold_mcp of 1 to match
            //the standard SDC behaviour (e.g. N - 1) of hold multicycles.
            //
            //For details see section 8.3 'Multicycle paths' in:
            //  J. Bhasker, R. Chadha, "Static Timing Analysis for Nanometer
            //      Designs A Practical Approach", Springer, 2009
            hold_offset += iter->second;
        } else {
            //Pin-specific override not found, look for Domain pair overrides
            key = std::make_tuple(from, to, AtomPinId::INVALID());
            iter = hold_mcp_overrides_.find(key);
            if (iter != hold_mcp_overrides_.end()) {
                hold_offset += iter->second;
            }
        }

        //The hold capture cycle is the setup capture cycle minus the hold mcp value
        return setup_capture_cycle(from, to, to_pin) - hold_offset;
    }

    /**
     * @brief Create a clock object with the given name, source, and create_clock command.
     *
     * This function handles creating the clock domain in Tatum and creating the clock
     * object in the object database.
     */
    void create_clock_object(const std::string& clock_name,
                             tatum::NodeId clock_source,
                             const sdcparse::CreateClock& cmd) {

        tatum::DomainId netlist_clock = tc_.create_clock_domain(clock_name);
        // Set the clock domain source. If the clock source is invalid, this is
        // a virtual clock.
        if (clock_source.is_valid()) {
            tc_.set_clock_domain_source(clock_source, netlist_clock);
        }

        // Update the object database.
        auto clock_obj_id = obj_database.create_clock_object(clock_name);
        object_to_clock_id_[clock_obj_id] = netlist_clock;

        if (sdc_clocks_.count(netlist_clock) != 0) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Found duplicate netlist clock definition for clock '%s'",
                      clock_name.c_str());
        }

        // Save the mapping to the sdc clock info
        sdc_clocks_[netlist_clock] = cmd;
    }

    /**
     * @brief Get the clock source for the given pin.
     *
     * In Tatum, there are some constraints on what pins can be clock sources,
     * this method helps enforce that.
     */
    tatum::NodeId get_clock_source(AtomPinId clock_pin) {
        VTR_ASSERT(clock_pin.is_valid());

        // Ensure that the pin is a driver of a clock. This may be necessary for setting
        // the source of the clock.
        if (netlist_clock_drivers_.count(clock_pin) != 1) {
            std::string pin_name = netlist_.pin_name(clock_pin);
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Pin '%s' is not a valid clock source. Clock pins currently must drive a clock net.",
                      pin_name.c_str());
        }

        // Get the clock source
        AtomNetId clock_net = netlist_.pin_net(clock_pin);
        AtomPinId clock_driver = netlist_.net_driver(clock_net);
        tatum::NodeId clock_source = lookup_.atom_pin_tnode(clock_driver);
        VTR_ASSERT(clock_source);

        return clock_source;
    }

    /**
     * @brief Get the clock target pins referred to by the given objects.
     *
     * This is used by the clock creating SDC commands to get the driver pins of the
     * netlist clocks. This allows us to unify the code such that the target objects
     * can be pins, ports, and/or nets and naturally dedupe them.
     *
     * NOTE: This method returns a pair of pins and their names. The reason is that the
     *       object contains the name information. Once the objects are parsed into pins,
     *       this information gets lost, but it is still needed. In the case of dupes, the
     *       first object found is chosen as the name (which matches other SDC
     *       implementations).
     *
     *  @param target_objects   List of objects to get the clock target pins of.
     *
     *  @return A map containing the set of clock target pins and their names.
     */
    std::map<AtomPinId, std::string> get_clock_target_pins(const std::vector<sdcparse::ObjectId>& target_objects) {
        std::map<AtomPinId, std::string> target_pins;
        for (sdcparse::ObjectId object_id : target_objects) {
            sdcparse::ObjectType object_type = obj_database.get_object_type(object_id);
            AtomPinId clock_pin;
            if (object_type == sdcparse::ObjectType::Port || object_type == sdcparse::ObjectType::Pin) {
                // When the target of the create_clock command is a pin, we implicitly are targeting
                // the driver of the net that this pin is a part of (i.e. the whole clock net).
                AtomPinId target_pin = get_port_or_pin(object_id);
                VTR_ASSERT(target_pin.is_valid());
                AtomNetId target_net = netlist_.pin_net(target_pin);
                clock_pin = netlist_.net_driver(target_net);
            } else {
                VTR_ASSERT(object_type == sdcparse::ObjectType::Net);
                // When the target of the create_clock command is a net, we implicitly are targeting
                // the driver of that net.
                AtomNetId target_net = get_net(object_id);
                VTR_ASSERT(target_net.is_valid());
                clock_pin = netlist_.net_driver(target_net);
                if (!clock_pin.is_valid()) {
                    std::string net_name = netlist_.net_name(target_net);
                    vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                              "Net '%s' has no driver and cannot be used as a clock target",
                              net_name.c_str());
                }
            }
            VTR_ASSERT(clock_pin.is_valid());

            if (!target_pins.contains(clock_pin)) {
                std::string object_name = obj_database.get_object_name(sdcparse::ObjectId(object_id));
                target_pins.insert(std::make_pair(clock_pin, object_name));
            }
        }

        return target_pins;
    }

    std::set<AtomPinId> get_ports(const std::vector<sdcparse::ObjectId>& port_group) {
        std::set<AtomPinId> pins;
        for (sdcparse::ObjectId port_id : port_group) {
            // If this object is not a port, just skip it.
            if (obj_database.get_object_type(port_id) != sdcparse::ObjectType::Port)
                continue;

            AtomPinId pin = get_port_or_pin(port_id);
            pins.insert(pin);
        }

        return pins;
    }

    std::set<tatum::DomainId> get_clocks(const std::vector<sdcparse::ObjectId>& clock_group) {
        std::set<tatum::DomainId> domains;

        if (clock_group.empty()) {
            return domains;
        }

        for (sdcparse::ObjectId clock_id : clock_group) {
            if (obj_database.get_object_type(clock_id) != sdcparse::ObjectType::Clock)
                continue;

            tatum::DomainId clock_domain = get_clock_domain(clock_id);
            domains.insert(clock_domain);
        }
        return domains;
    }

    std::set<AtomPinId> get_pins(const std::vector<sdcparse::ObjectId>& pin_group) {
        std::set<AtomPinId> pins;

        if (pin_group.empty()) {
            return pins;
        }

        for (sdcparse::ObjectId pin_id : pin_group) {
            if (obj_database.get_object_type(pin_id) != sdcparse::ObjectType::Pin)
                continue;

            AtomPinId pin = get_port_or_pin(pin_id);
            pins.insert(pin);
        }

        return pins;
    }

    AtomNetId get_net(sdcparse::ObjectId net_object_id) {
        if (obj_database.get_object_type(net_object_id) != sdcparse::ObjectType::Net) {
            return AtomNetId::INVALID();
        }

        auto it = object_to_net_id_.find(net_object_id);
        VTR_ASSERT(it != object_to_net_id_.end());
        return it->second;
    }

    /**
     * @brief Get the Atom Pin associated with the given object.
     *
     * If the given object is not a port or pin, an invalid ID is returned.
     */
    AtomPinId get_port_or_pin(sdcparse::ObjectId object_id) {
        sdcparse::ObjectType object_type = obj_database.get_object_type(object_id);

        if (object_type == sdcparse::ObjectType::Port) {
            auto it = object_to_port_id_.find(object_id);
            VTR_ASSERT(it != object_to_port_id_.end());
            return it->second;
        } else if (object_type == sdcparse::ObjectType::Pin) {
            auto it = object_to_pin_id_.find(object_id);
            VTR_ASSERT(it != object_to_pin_id_.end());
            return it->second;
        } else {
            return AtomPinId::INVALID();
        }
    }

    tatum::DomainId get_clock_domain(sdcparse::ObjectId clock_object_id) {
        if (obj_database.get_object_type(clock_object_id) != sdcparse::ObjectType::Clock) {
            return tatum::DomainId::INVALID();
        }

        auto it = object_to_clock_id_.find(clock_object_id);
        VTR_ASSERT(it != object_to_clock_id_.end());
        return it->second;
    }

    std::set<tatum::DomainId> get_all_clocks() {
        auto domains = tc_.clock_domains();
        return std::set<tatum::DomainId>(domains.begin(), domains.end());
    }

    /**
     * @brief Checks that the given string group only contains objects of the given types.
     *
     * This is used for checking the target objects of the SDC commands.
     */
    bool check_objects(const std::vector<sdcparse::ObjectId>& object_group,
                       const std::unordered_set<sdcparse::ObjectType>& expected_object_types) {

        for (sdcparse::ObjectId object_id : object_group) {
            sdcparse::ObjectType object_type = obj_database.get_object_type(object_id);
            if (!expected_object_types.contains(object_type)) {
                return false;
            }
        }

        return true;
    }

    float sdc_units_to_seconds(float val) const {
        return val * unit_scale_;
    }

    float seconds_to_sdc_units(float val) const {
        return val / unit_scale_;
    }

    std::set<AtomPinId> setup_capture_pins_with_overrides() {
        std::set<AtomPinId> pins;

        for (auto kv : setup_mcp_overrides_) {
            pins.insert(std::get<2>(kv.first));
        }

        //We use the invalid pin as a placehold for the default constraint,
        //so it should not be included in the set of pins with overrides.
        pins.erase(AtomPinId::INVALID());
        return pins;
    }

    std::set<AtomPinId> hold_capture_pins_with_overrides() {
        std::set<AtomPinId> pins;

        for (auto kv : hold_mcp_overrides_) {
            pins.insert(std::get<2>(kv.first));
        }

        //We use the invalid pin as a placehold for the default constraint,
        //so it should not be included in the set of pins with overrides.
        pins.erase(AtomPinId::INVALID());
        return pins;
    }

  private:
    const AtomNetlist& netlist_;
    const AtomLookup& lookup_;
    const LogicalModels& models_;
    tatum::TimingConstraints& tc_;
    tatum::TimingGraph& tg_;

    size_t num_commands_ = 0;
    std::string fname_;
    int lineno_ = -1;

    float unit_scale_ = 1e-9;

    std::map<tatum::DomainId, sdcparse::CreateClock> sdc_clocks_;
    std::set<AtomPinId> netlist_clock_drivers_;
    std::map<std::string, AtomPinId> netlist_primary_ios_;

    /// @brief A lookup between a LibSDCParse port object and its associated netlist pin.
    std::unordered_map<sdcparse::ObjectId, AtomPinId> object_to_port_id_;
    /// @brief A lookup between a LibSDCParse pin object and its associated netlist pin.
    std::unordered_map<sdcparse::ObjectId, AtomPinId> object_to_pin_id_;
    /// @brief A lookup between a LibSDCParse clock object and its associated Tatum timing domain.
    std::unordered_map<sdcparse::ObjectId, tatum::DomainId> object_to_clock_id_;
    /// @brief A lookup between a LibSDCParse net object and its associated netlist net.
    std::unordered_map<sdcparse::ObjectId, AtomNetId> object_to_net_id_;

    std::set<std::pair<tatum::DomainId, tatum::DomainId>> disabled_domain_pairs_;
    std::map<std::pair<tatum::DomainId, tatum::DomainId>, float> setup_override_constraints_;
    std::map<std::pair<tatum::DomainId, tatum::DomainId>, float> hold_override_constraints_;

    std::map<std::tuple<tatum::DomainId, tatum::DomainId, AtomPinId>, int> setup_mcp_overrides_;
    std::map<std::tuple<tatum::DomainId, tatum::DomainId, AtomPinId>, int> hold_mcp_overrides_;
};

std::unique_ptr<tatum::TimingConstraints> read_sdc(const t_timing_inf& timing_inf,
                                                   const AtomNetlist& netlist,
                                                   const AtomLookup& lookup,
                                                   const LogicalModels& models,
                                                   tatum::TimingGraph& timing_graph) {
    auto timing_constraints = std::make_unique<tatum::TimingConstraints>();

    if (!timing_inf.timing_analysis_enabled) {
        VTR_LOG("\n");
        VTR_LOG("Timing analysis off\n");
        apply_default_timing_constraints(netlist, lookup, models, *timing_constraints);
    } else {
        if (!std::filesystem::exists(timing_inf.SDCFile)) {
            //No SDC file
            VTR_LOG("\n");
            VTR_LOG("SDC file '%s' not found\n", timing_inf.SDCFile.c_str());
            apply_default_timing_constraints(netlist, lookup, models, *timing_constraints);
        } else {
            //Parse the file
            SdcParseCallback callback(netlist, lookup, models, *timing_constraints, timing_graph);
            sdcparse::sdc_parse_filename(timing_inf.SDCFile.c_str(), callback);

            if (callback.num_commands() == 0) {
                VTR_LOG("\n");
                VTR_LOG("SDC file '%s' contained no SDC commands\n", timing_inf.SDCFile.c_str());
                apply_default_timing_constraints(netlist, lookup, models, *timing_constraints);
            } else {
                VTR_LOG("\n");
                VTR_LOG("Applied %zu SDC commands from '%s'\n", callback.num_commands(), timing_inf.SDCFile.c_str());
            }
        }
    }
    VTR_LOG("Timing constraints created %zu clocks\n", timing_constraints->clock_domains().size());
    for (tatum::DomainId domain : timing_constraints->clock_domains()) {
        if (timing_constraints->is_virtual_clock(domain)) {
            VTR_LOG("  Constrained Clock '%s' (Virtual Clock)\n",
                    timing_constraints->clock_domain_name(domain).c_str());
        } else {
            tatum::NodeId src_tnode = timing_constraints->clock_domain_source_node(domain);
            VTR_ASSERT(src_tnode);

            AtomPinId src_pin = lookup.tnode_atom_pin(src_tnode);
            VTR_ASSERT(src_pin);

            VTR_LOG("  Constrained Clock '%s' Source: '%s'\n",
                    timing_constraints->clock_domain_name(domain).c_str(),
                    netlist.pin_name(src_pin).c_str());
        }
    }

    VTR_LOG("\n");

    return timing_constraints;
}

//Apply the default timing constraints (i.e. if there are no user specified constraints)
//appropriate to the type of circuit.
void apply_default_timing_constraints(const AtomNetlist& netlist,
                                      const AtomLookup& lookup,
                                      const LogicalModels& models,
                                      tatum::TimingConstraints& tc) {
    std::set<AtomPinId> netlist_clock_drivers = find_netlist_logical_clock_drivers(netlist, models);

    if (netlist_clock_drivers.size() == 0) {
        apply_combinational_default_timing_constraints(netlist, lookup, tc);

    } else if (netlist_clock_drivers.size() == 1) {
        apply_single_clock_default_timing_constraints(netlist, lookup, *netlist_clock_drivers.begin(), tc);

    } else {
        VTR_ASSERT(netlist_clock_drivers.size() > 1);

        apply_multi_clock_default_timing_constraints(netlist, lookup, netlist_clock_drivers, tc);
    }
}

//Apply the default timing constraints for purely combinational circuits which have
//no explicit netlist clock
void apply_combinational_default_timing_constraints(const AtomNetlist& netlist,
                                                    const AtomLookup& lookup,
                                                    tatum::TimingConstraints& tc) {
    std::string clock_name = "virtual_io_clock";

    VTR_LOG("Setting default timing constraints:\n");
    VTR_LOG("   * constrain all primary inputs and primary outputs on a virtual external clock '%s'\n", clock_name.c_str());
    VTR_LOG("   * optimize virtual clock to run as fast as possible\n");

    //Create a virtual clock, with 0 period
    tatum::DomainId domain = tc.create_clock_domain(clock_name);
    tc.set_setup_constraint(domain, domain, tatum::Time(0.));
    tc.set_hold_constraint(domain, domain, tatum::Time(0.));

    //Constrain all I/Os with zero input/output delay
    constrain_all_ios(netlist, lookup, tc, domain, domain, tatum::Time(0.), tatum::Time(0.));

    //Mark constant generator timing nodes
    mark_constant_generators(netlist, lookup, tc);
}

//Apply the default timing constraints for circuits with a single netlist clock
void apply_single_clock_default_timing_constraints(const AtomNetlist& netlist,
                                                   const AtomLookup& lookup,
                                                   const AtomPinId clock_driver,
                                                   tatum::TimingConstraints& tc) {
    AtomNetId clock_net = netlist.pin_net(clock_driver);
    const std::string& clock_name = netlist.net_name(clock_net);

    VTR_LOG("Setting default timing constraints:\n");
    VTR_LOG("   * constrain all primary inputs and primary outputs on netlist clock '%s'\n", clock_name.c_str());
    VTR_LOG("   * optimize netlist clock to run as fast as possible\n");

    //Create the netlist clock with period 0
    tatum::DomainId domain = tc.create_clock_domain(clock_name);
    tc.set_setup_constraint(domain, domain, tatum::Time(0.));
    tc.set_hold_constraint(domain, domain, tatum::Time(0.));

    //Mark the clock domain source
    AtomPinId clock_driver_pin = netlist.net_driver(clock_net);
    tatum::NodeId clock_source = lookup.atom_pin_tnode(clock_driver_pin);
    VTR_ASSERT(clock_source);
    tc.set_clock_domain_source(clock_source, domain);

    //Constrain all I/Os with zero input/output delay
    constrain_all_ios(netlist, lookup, tc, domain, domain, tatum::Time(0.), tatum::Time(0.));

    //Mark constant generator timing nodes
    mark_constant_generators(netlist, lookup, tc);
}

//Apply the default timing constraints for circuits with multiple netlist clocks
void apply_multi_clock_default_timing_constraints(const AtomNetlist& netlist,
                                                  const AtomLookup& lookup,
                                                  const std::set<AtomPinId>& clock_drivers,
                                                  tatum::TimingConstraints& tc) {
    std::string virtual_clock_name = "virtual_io_clock";
    VTR_LOG("Setting default timing constraints:\n");
    VTR_LOG("   * constrain all primary inputs and primary outputs on a virtual external clock '%s'\n", virtual_clock_name.c_str());
    VTR_LOG("   * optimize all netlist and virtual clocks to run as fast as possible\n");
    VTR_LOG("   * ignore cross netlist clock domain timing paths\n");

    //Create a virtual clock, with 0 period
    tatum::DomainId virtual_clock = tc.create_clock_domain(virtual_clock_name);
    tc.set_setup_constraint(virtual_clock, virtual_clock, tatum::Time(0.));
    tc.set_hold_constraint(virtual_clock, virtual_clock, tatum::Time(0.));

    //Constrain all I/Os with zero input/output delay t the virtual clock
    constrain_all_ios(netlist, lookup, tc, virtual_clock, virtual_clock, tatum::Time(0.), tatum::Time(0.));

    //Create each of the netlist clocks, and constrain it to period 0. Do not analyze cross-domain paths
    for (AtomPinId clock_driver : clock_drivers) {
        AtomNetId clock_net = netlist.pin_net(clock_driver);

        //Create the clock
        const std::string& clock_name = netlist.net_name(clock_net);
        tatum::DomainId clock = tc.create_clock_domain(clock_name);

        //Mark the clock domain source
        tatum::NodeId clock_source = lookup.atom_pin_tnode(clock_driver);
        VTR_ASSERT(clock_source);
        tc.set_clock_domain_source(clock_source, clock);

        //Do not analyze cross-domain timing paths (except to/from virtual clock)
        tc.set_setup_constraint(clock, clock, tatum::Time(0.));         //Intra-domain
        tc.set_setup_constraint(clock, virtual_clock, tatum::Time(0.)); //netlist to virtual
        tc.set_setup_constraint(virtual_clock, clock, tatum::Time(0.)); //virtual to netlist

        tc.set_hold_constraint(clock, clock, tatum::Time(0.));         //Intra-domain
        tc.set_hold_constraint(clock, virtual_clock, tatum::Time(0.)); //netlist to virtual
        tc.set_hold_constraint(virtual_clock, clock, tatum::Time(0.)); //virtual to netlist
    }

    //Mark constant generator timing nodes
    mark_constant_generators(netlist, lookup, tc);
}

//Look through the netlist to find any constant generators, and mark them as
//constant generators in the timing constraints
void mark_constant_generators(const AtomNetlist& netlist,
                              const AtomLookup& lookup,
                              tatum::TimingConstraints& tc) {
    for (AtomPinId pin : netlist.pins()) {
        if (netlist.pin_is_constant(pin)) {
            tatum::NodeId tnode = lookup.atom_pin_tnode(pin);
            VTR_ASSERT(tnode);

            tc.set_constant_generator(tnode);
        }
    }
}

//Constrain all primary inputs and primary outputs to the specified clock domains and delays
void constrain_all_ios(const AtomNetlist& netlist,
                       const AtomLookup& lookup,
                       tatum::TimingConstraints& tc,
                       tatum::DomainId input_domain,
                       tatum::DomainId output_domain,
                       tatum::Time input_delay,
                       tatum::Time output_delay) {
    for (AtomBlockId blk : netlist.blocks()) {
        AtomBlockType type = netlist.block_type(blk);

        if (type == AtomBlockType::INPAD || type == AtomBlockType::OUTPAD) {
            //Get the pin
            if (netlist.block_pins(blk).size() == 1) {
                AtomPinId pin = *netlist.block_pins(blk).begin();

                //Find the associated tnode
                tatum::NodeId tnode = lookup.atom_pin_tnode(pin);

                //Constrain it
                if (type == AtomBlockType::INPAD) {
                    tc.set_input_constraint(tnode, input_domain, tatum::DelayType::MAX, input_delay);
                    tc.set_input_constraint(tnode, input_domain, tatum::DelayType::MIN, input_delay);
                } else {
                    VTR_ASSERT(type == AtomBlockType::OUTPAD);
                    tc.set_output_constraint(tnode, output_domain, tatum::DelayType::MAX, output_delay);
                    tc.set_output_constraint(tnode, output_domain, tatum::DelayType::MIN, output_delay);
                }
            } else {
                VTR_ASSERT_MSG(netlist.block_pins(blk).size() == 0, "Unconnected I/O");
            }
        }
    }
}

std::map<std::string, AtomPinId> find_netlist_primary_ios(const AtomNetlist& netlist) {
    std::map<std::string, AtomPinId> primary_inputs;

    for (AtomBlockId blk : netlist.blocks()) {
        auto type = netlist.block_type(blk);
        if (type == AtomBlockType::INPAD || type == AtomBlockType::OUTPAD) {
            VTR_ASSERT(netlist.block_pins(blk).size() == 1);
            AtomPinId pin = *netlist.block_pins(blk).begin();

            std::string orig_name = orig_blif_name(netlist.block_name(blk));

            VTR_ASSERT(!primary_inputs.count(orig_name));

            primary_inputs[orig_name] = pin;
        }
    }

    return primary_inputs;
}

//Trim off the prefix added to blif names to make outputs unique
std::string orig_blif_name(std::string name) {
    constexpr const char* BLIF_UNIQ_PREFIX = "out:";

    if (name.find(BLIF_UNIQ_PREFIX) == 0) {                    //Starts with prefix
        name = vtr::replace_first(name, BLIF_UNIQ_PREFIX, ""); //Remove prefix
    }

    return name;
}

//Converts a glob pattern to a std::regex
std::regex glob_pattern_to_regex(const std::string& glob_pattern) {
    //In glob (i.e. unix-shell style):
    //   '*' is a wildcard match of zero or more instances of any characters
    //
    //In regex:
    //   '*' matches zero or more of the preceding character
    //   '.' matches any character
    //
    //To convert a glob to a regex we need to:
    //   Convert '.' to "\.", so literal '.' in glob is treated as literal in the regex
    //   Convert '*' to ".*" so literal '*' in glob matches any sequence

    std::string regex_str = vtr::replace_all(glob_pattern, ".", "\\.");
    regex_str = vtr::replace_all(regex_str, "*", ".*");

    return std::regex(regex_str);
}
