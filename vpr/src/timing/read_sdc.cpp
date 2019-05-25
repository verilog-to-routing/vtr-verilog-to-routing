#include "read_sdc.h"

#include <regex>

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
                     tatum::TimingConstraints& timing_constraints,
                     tatum::TimingGraph& tg)
        : netlist_(netlist)
        , lookup_(lookup)
        , tc_(timing_constraints)
        , tg_(tg) {}

  public: //sdcparse::Callback interface
    //Start of parsing
    void start_parse() override {
        netlist_clock_drivers_ = find_netlist_logical_clock_drivers(netlist_);
        netlist_primary_ios_ = find_netlist_primary_ios(netlist_);
    }

    //Sets current filename
    void filename(std::string fname) override { fname_ = fname; }

    //Sets current line number
    void lineno(int line_num) override { lineno_ = line_num; }

    //Individual commands
    void create_clock(const sdcparse::CreateClock& cmd) override {
        ++num_commands_;

        if (cmd.is_virtual) {
            //Create a virtual clock
            tatum::DomainId virtual_clk = tc_.create_clock_domain(cmd.name);

            if (sdc_clocks_.count(virtual_clk)) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                          "Found duplicate virtual clock definition for clock '%s'",
                          cmd.name.c_str());
            }

            //Virtual clocks should have no targets
            if (!cmd.targets.strings.empty()) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                          "Virtual clock definition (i.e. with '-name') should not have targets");
            }

            //Save the mapping to the sdc clock info
            sdc_clocks_[virtual_clk] = cmd;
        } else {
            //Create a netlist clock for every matching netlist clock
            for (const std::string& clock_name_glob_pattern : cmd.targets.strings) {
                bool found = false;

                //We interpret each SDC target as glob-style pattern matches, which we
                //convert to a regex
                auto clock_name_regex = glob_pattern_to_regex(clock_name_glob_pattern);

                //Look for matching netlist clocks
                for (AtomPinId clock_pin : netlist_clock_drivers_) {
                    AtomNetId clock_net = netlist_.pin_net(clock_pin);
                    const auto& clock_name = netlist_.net_name(clock_net);

                    if (std::regex_match(clock_name, clock_name_regex)) {
                        found = true;
                        //Create netlist clock
                        tatum::DomainId netlist_clk = tc_.create_clock_domain(clock_name);

                        if (sdc_clocks_.count(netlist_clk)) {
                            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                                      "Found duplicate netlist clock definition for clock '%s' matching target pattern '%s'",
                                      clock_name.c_str(), clock_name_glob_pattern.c_str());
                        }

                        //Set the clock source
                        AtomPinId clock_driver = netlist_.net_driver(clock_net);
                        tatum::NodeId clock_source = lookup_.atom_pin_tnode(clock_driver);
                        VTR_ASSERT(clock_source);
                        tc_.set_clock_domain_source(clock_source, netlist_clk);

                        //Save the mapping to the clock info
                        sdc_clocks_[netlist_clk] = cmd;
                    }
                }

                if (!found) {
                    vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                              "Clock name or pattern '%s' does not correspond to any nets."
                              " To create a virtual clock, use the '-name' option.",
                              clock_name_glob_pattern.c_str());
                }
            }
        }
    }

    void set_io_delay(const sdcparse::SetIoDelay& cmd) override {
        ++num_commands_;

        tatum::DomainId domain;

        if (cmd.clock_name == "*") {
            if (netlist_clock_drivers_.size() == 1) {
                //Support non-standard wildcard clock name for set_input_delay/set_output_delay
                //commands, provided it is unambiguous (i.e. there is only one netlist clock)

                AtomNetId clock_net = netlist_.pin_net(*netlist_clock_drivers_.begin());
                std::string clock_name = netlist_.net_name(clock_net);
                domain = tc_.find_clock_domain(clock_name);
            } else {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                          "Wildcard clock domain '%s' is ambiguous in multi-clock circuits, explicitly specify the target clock",
                          cmd.clock_name.c_str());
            }
        } else {
            //Regular look-up
            domain = tc_.find_clock_domain(cmd.clock_name);
        }

        //Error checks
        if (!domain) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Failed to find clock domain '%s' for I/O constraint",
                      cmd.clock_name.c_str());
        }

        //Find all matching I/Os
        auto io_pins = get_ports(cmd.target_ports);

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
                    if (is_max) {
                        tc_.set_input_constraint(tnode, domain, tatum::DelayType::MAX, tatum::Time(delay));
                    }
                    if (is_min) {
                        tc_.set_input_constraint(tnode, domain, tatum::DelayType::MIN, tatum::Time(delay));
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
                    if (is_max) {
                        tc_.set_output_constraint(tnode, domain, tatum::DelayType::MAX, tatum::Time(delay));
                    }
                    if (is_min) {
                        tc_.set_output_constraint(tnode, domain, tatum::DelayType::MIN, tatum::Time(delay));
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
        ++num_commands_;

        if (cmd.type != sdcparse::ClockGroupsType::EXCLUSIVE) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_clock_groups only supports -exclusive groups");
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
        ++num_commands_;

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
        ++num_commands_;

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
        ++num_commands_;

        auto from_clocks = get_clocks(cmd.from);
        auto to_clocks = get_clocks(cmd.to);

        if (from_clocks.empty() && to_clocks.empty()) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "set_multicycle_path must specify at least one -from or -to clock");
        }

        if (from_clocks.empty()) {
            from_clocks = get_all_clocks();
        }

        if (to_clocks.empty()) {
            to_clocks = get_all_clocks();
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
                auto domain_pair = std::make_pair(from_clock, to_clock);

                if (is_setup) {
                    setup_mcp_overrides_[domain_pair] = setup_mcp;
                }

                if (is_hold) {
                    hold_mcp_overrides_[domain_pair] = hold_mcp;
                }
            }
        }
    }

    void set_clock_uncertainty(const sdcparse::SetClockUncertainty& cmd) override {
        ++num_commands_;

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
        ++num_commands_;

        if (cmd.type != sdcparse::ClockLatencyType::SOURCE) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_clock_latency only supports specifying -source latency");
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
        ++num_commands_;

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
            for (auto to_pin : from_pins) {
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
            }
        }
    }

    void set_timing_derate(const sdcparse::SetTimingDerate& /*cmd*/) override {
        ++num_commands_;
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

  public:
    size_t num_commands() { return num_commands_; }

  private:
    void resolve_clock_constraints() {
        //Set the clock constraints
        for (tatum::DomainId launch_clock : tc_.clock_domains()) {
            for (tatum::DomainId capture_clock : tc_.clock_domains()) {
                auto domain_pair = std::make_pair(launch_clock, capture_clock);

                if (disabled_domain_pairs_.count(domain_pair)) continue;

                //Setup
                tatum::Time setup_constraint = calculate_setup_constraint(launch_clock, capture_clock);
                VTR_ASSERT(setup_constraint.valid());

                tc_.set_setup_constraint(launch_clock, capture_clock, setup_constraint);

                //Hold
                tatum::Time hold_constraint = calculate_hold_constraint(launch_clock, capture_clock);
                VTR_ASSERT(hold_constraint.valid());

                tc_.set_hold_constraint(launch_clock, capture_clock, hold_constraint);
            }
        }
    }

    //Returns the setup constraint in seconds
    tatum::Time calculate_setup_constraint(tatum::DomainId launch_domain, tatum::DomainId capture_domain) const {
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
        int extra_cycles = setup_capture_cycle(launch_domain, capture_domain) - 1;
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

            //Override the constarint
            setup_constraint = override_setup_constraint;
        }

        setup_constraint = sdc_units_to_seconds(setup_constraint);

        if (setup_constraint < 0.) {
            VPR_THROW(VPR_ERROR_SDC,
                      "Setup constraint %g for transfers from clock '%s' to clock '%s' is negative."
                      " Requires data to arrive before launch edge (No time travelling allowed!)",
                      setup_constraint,
                      tc_.clock_domain_name(launch_domain).c_str(),
                      tc_.clock_domain_name(capture_domain).c_str());
        }

        return tatum::Time(setup_constraint);
    }

    //Returns the hold constraint in seconds
    tatum::Time calculate_hold_constraint(tatum::DomainId launch_domain, tatum::DomainId capture_domain) const {
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
        int extra_cycles = hold_capture_cycle(launch_domain, capture_domain) - 1;
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

            //Override the constarint
            hold_constraint = override_hold_constraint;
        }

        hold_constraint = sdc_units_to_seconds(hold_constraint);

        return tatum::Time(hold_constraint);
    }

    //Determine the minumum time (in SDC units) between the edges of the launch and capture clocks
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

        if (std::fabs(launch_clock.period - capture_clock.period) < EPSILON && std::fabs(launch_clock.rise_edge - capture_clock.rise_edge) < EPSILON && std::fabs(launch_clock.fall_edge - capture_clock.fall_edge) < EPSILON) {
            //The source and sink domains have the same period and edges, the constraint is the common clock period.

            constraint = launch_clock.period;

        } else if (launch_clock.period < EPSILON || capture_clock.period < EPSILON) {
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
            //source edge yeilds the setup time constraint.
            int scaled_constraint = std::numeric_limits<int>::max(); //Initialize to +inf, so any constraint will be less

            for (int launch_edge : launch_edges) {
                for (int capture_edge : capture_edges) {
                    if (capture_edge >= launch_edge) { //Postive only
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
    int setup_capture_cycle(tatum::DomainId from, tatum::DomainId to) const {
        //Default: capture one cycle after launch
        int setup_path_mult = 1;

        //Any overrides
        auto key = std::make_pair(from, to);
        auto iter = setup_mcp_overrides_.find(key);
        if (iter != setup_mcp_overrides_.end()) {
            setup_path_mult = iter->second;
        }

        //The setup capture cycle is the setup mcp value
        return setup_path_mult;
    }

    //Returns the cycle number (after launch) where the hold check occurs
    int hold_capture_cycle(tatum::DomainId from, tatum::DomainId to) const {
        //Default: hold captures the cycle before setup is captured
        //For the default setup mcp this implies capturing the same
        //cycle as launch
        int hold_offset = 1;

        //Any overrides?
        auto key = std::make_pair(from, to);
        auto iter = hold_mcp_overrides_.find(key);
        if (iter != hold_mcp_overrides_.end()) {
            //Note that we add the override to the default hold_mcp of 1 to match
            //the standard SDC behaviour (e.g. N - 1) of hold multicycles.
            //
            //For details see section 8.3 'Multicycle paths' in:
            //  J. Bhasker, R. Chadha, "Static Timing Analysis for Nanometer
            //      Designs A Practical Approach", Springer, 2009
            hold_offset += iter->second;
        }

        //The hold capture cycle is the setup capture cycle minus the hold mcp value
        return setup_capture_cycle(from, to) - hold_offset;
    }

    std::set<AtomPinId> get_ports(const sdcparse::StringGroup& port_group) {
        if (port_group.type != sdcparse::StringGroupType::PORT) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Expected port collection via get_ports");
        }

        std::set<AtomPinId> pins;
        for (const auto& port_pattern : port_group.strings) {
            std::regex port_regex = glob_pattern_to_regex(port_pattern);

            bool found = false;
            for (const auto& kv : netlist_primary_ios_) {
                const std::string& io_name = kv.first;
                if (std::regex_match(io_name, port_regex)) {
                    found = true;

                    AtomPinId pin = kv.second;

                    pins.insert(pin);
                }
            }

            if (!found) {
                VTR_LOGF_WARN(fname_.c_str(), lineno_,
                              "get_ports target name or pattern '%s' matched no ports\n",
                              port_pattern.c_str());
            }
        }
        return pins;
    }

    std::set<tatum::DomainId> get_clocks(const sdcparse::StringGroup& clock_group) {
        std::set<tatum::DomainId> domains;

        if (clock_group.strings.empty()) {
            return domains;
        }

        if (clock_group.type != sdcparse::StringGroupType::CLOCK
            && clock_group.type != sdcparse::StringGroupType::STRING) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Expected clock names or collection via get_clocks");
        }

        for (const auto& clock_glob_pattern : clock_group.strings) {
            std::regex clock_regex = glob_pattern_to_regex(clock_glob_pattern);

            bool found = false;
            for (tatum::DomainId domain : tc_.clock_domains()) {
                const std::string& clock_name = tc_.clock_domain_name(domain);
                if (std::regex_match(clock_name, clock_regex)) {
                    found = true;

                    domains.insert(domain);
                }
            }

            if (!found) {
                VTR_LOGF_WARN(fname_.c_str(), lineno_,
                              "get_clocks target name or pattern '%s' matched no clocks\n",
                              clock_glob_pattern.c_str());
            }
        }

        return domains;
    }

    std::set<AtomPinId> get_pins(const sdcparse::StringGroup& pin_group) {
        std::set<AtomPinId> pins;

        if (pin_group.strings.empty()) {
            return pins;
        }

        if (pin_group.type != sdcparse::StringGroupType::PIN) {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                      "Expected pin collection via get_pins");
        }

        for (const auto& pin_pattern : pin_group.strings) {
            std::regex pin_regex = glob_pattern_to_regex(pin_pattern);

            bool found = false;
            for (AtomPinId pin : netlist_.pins()) {
                const std::string& pin_name = netlist_.pin_name(pin);

                if (std::regex_match(pin_name, pin_regex)) {
                    found = true;

                    pins.insert(pin);
                }
            }

            if (!found) {
                VTR_LOGF_WARN(fname_.c_str(), lineno_,
                              "get_pins target name or pattern '%s' matched no pins\n",
                              pin_pattern.c_str());
            }
        }

        return pins;
    }

    std::set<tatum::DomainId> get_all_clocks() {
        auto domains = tc_.clock_domains();
        return std::set<tatum::DomainId>(domains.begin(), domains.end());
    }

    float sdc_units_to_seconds(float val) const {
        return val * unit_scale_;
    }

    float seconds_to_sdc_units(float val) const {
        return val / unit_scale_;
    }

  private:
    const AtomNetlist& netlist_;
    const AtomLookup& lookup_;
    tatum::TimingConstraints& tc_;
    tatum::TimingGraph& tg_;

    size_t num_commands_ = 0;
    std::string fname_;
    int lineno_ = -1;

    float unit_scale_ = 1e-9;

    std::map<tatum::DomainId, sdcparse::CreateClock> sdc_clocks_;
    std::set<AtomPinId> netlist_clock_drivers_;
    std::map<std::string, AtomPinId> netlist_primary_ios_;

    std::set<std::pair<tatum::DomainId, tatum::DomainId>> disabled_domain_pairs_;
    std::map<std::pair<tatum::DomainId, tatum::DomainId>, float> setup_override_constraints_;
    std::map<std::pair<tatum::DomainId, tatum::DomainId>, float> hold_override_constraints_;

    std::map<std::pair<tatum::DomainId, tatum::DomainId>, int> setup_mcp_overrides_;
    std::map<std::pair<tatum::DomainId, tatum::DomainId>, int> hold_mcp_overrides_;
};

std::unique_ptr<tatum::TimingConstraints> read_sdc(const t_timing_inf& timing_inf,
                                                   const AtomNetlist& netlist,
                                                   const AtomLookup& lookup,
                                                   tatum::TimingGraph& timing_graph) {
    auto timing_constraints = std::make_unique<tatum::TimingConstraints>();

    if (!timing_inf.timing_analysis_enabled) {
        VTR_LOG("\n");
        VTR_LOG("Timing analysis off\n");
        apply_default_timing_constraints(netlist, lookup, *timing_constraints);
    } else {
        FILE* sdc_file = fopen(timing_inf.SDCFile.c_str(), "r");
        if (sdc_file == nullptr) {
            //No SDC file
            VTR_LOG("\n");
            VTR_LOG("SDC file '%s' not found\n", timing_inf.SDCFile.c_str());
            apply_default_timing_constraints(netlist, lookup, *timing_constraints);
        } else {
            VTR_ASSERT(sdc_file != nullptr);

            //Parse the file
            SdcParseCallback callback(netlist, lookup, *timing_constraints, timing_graph);
            sdc_parse_file(sdc_file, callback, timing_inf.SDCFile.c_str());
            fclose(sdc_file);

            if (callback.num_commands() == 0) {
                VTR_LOG("\n");
                VTR_LOG("SDC file '%s' contained no SDC commands\n", timing_inf.SDCFile.c_str());
                apply_default_timing_constraints(netlist, lookup, *timing_constraints);
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
                                      tatum::TimingConstraints& tc) {
    std::set<AtomPinId> netlist_clock_drivers = find_netlist_logical_clock_drivers(netlist);

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
    VTR_LOG("   * constrain all primay inputs and primary outputs on a virtual external clock '%s'\n", clock_name.c_str());
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
    std::string clock_name = netlist.net_name(clock_net);

    VTR_LOG("Setting default timing constraints:\n");
    VTR_LOG("   * constrain all primay inputs and primary outputs on netlist clock '%s'\n", clock_name.c_str());
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
    VTR_LOG("   * constrain all primay inputs and primary outputs on a virtual external clock '%s'\n", virtual_clock_name.c_str());
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
        std::string clock_name = netlist.net_name(clock_net);
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

//Constrain all primary inputs and primary outputs to the specifed clock domains and delays
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
    //   '*' matches zero or more of the preceeding character
    //   '.' matches any character
    //
    //To convert a glob to a regex we need to:
    //   Convert '.' to "\.", so literal '.' in glob is treated as literal in the regex
    //   Convert '*' to ".*" so literal '*' in glob matches any sequence

    std::string regex_str = vtr::replace_all(glob_pattern, ".", "\\.");
    regex_str = vtr::replace_all(regex_str, "*", ".*");

    return std::regex(regex_str);
}
