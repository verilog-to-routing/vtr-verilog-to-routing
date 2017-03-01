#include "read_sdc2.h"

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
#include "atom_lookup.h"

void apply_default_timing_constraints(const AtomNetlist& netlist, 
                                      const AtomLookup& lookup, 
                                      tatum::TimingConstraints& timing_constraints);

void apply_combinational_default_timing_constraints(const AtomNetlist& netlist,
                                                    const AtomLookup& lookup,
                                                    tatum::TimingConstraints& timing_constraints);

void apply_single_clock_default_timing_constraints(const AtomNetlist& netlist,
                                                   const AtomLookup& lookup,
                                                   const AtomNetId clock_net,
                                                   tatum::TimingConstraints& timing_constraints);

void apply_multi_clock_default_timing_constraints(const AtomNetlist& netlist,
                                                  const AtomLookup& lookup,
                                                  const std::set<AtomNetId>& clock_nets,
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

std::set<AtomNetId> find_netlist_clocks(const AtomNetlist& netlist);
std::map<std::string,AtomPinId> find_netlist_primary_ios(const AtomNetlist& netlist);
std::string orig_blif_name(std::string name);

std::regex glob_pattern_to_regex(const std::string& glob_pattern);

class SdcParseCallback2 : public sdcparse::Callback {
    public:
        SdcParseCallback2(const AtomNetlist& netlist,
                          const AtomLookup& lookup,
                          tatum::TimingConstraints& timing_constraints,
                          tatum::TimingGraph& tg)
            : netlist_(netlist)
            , lookup_(lookup)
            , tc_(timing_constraints)
            , tg_(tg)
            {}
    public: //sdcparse::Callback interface
        //Start of parsing
        void start_parse() override {
            netlist_clock_nets_ = find_netlist_clocks(netlist_);        
            netlist_primary_ios_ = find_netlist_primary_ios(netlist_);
        }

        //Sets current filename
        void filename(std::string fname) override { fname_ = fname; }

        //Sets current line number
        void lineno(int line_num) override { lineno_ = line_num; }

        //Individual commands
        void create_clock(const sdcparse::CreateClock& cmd) override { 
            ++num_commands_;

            if(cmd.is_virtual) {
                //Create a virtual clock
                tatum::DomainId virtual_clk = tc_.create_clock_domain(cmd.name);

                if(sdc_clocks_.count(virtual_clk)) {
                    vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                              "Found duplicate virtual clock definition for clock '%s'",
                              cmd.name.c_str());
                }

                //Virtual clocks should have no targets
                if(!cmd.targets.strings.empty()) {
                    vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                              "Virtual clock definition (i.e. with '-name') should not have targets");
                }

                //Save the mapping to the sdc clock info
                sdc_clocks_[virtual_clk] = cmd;
            } else {
                //Create a netlist clock for every matching netlist clock
                for(const std::string& clock_name_glob_pattern : cmd.targets.strings) {
                    bool found = false;

                    //We interpret each SDC target as glob-style pattern matches, which we
                    //convert to a regex
                    auto clock_name_regex = glob_pattern_to_regex(clock_name_glob_pattern);

                    //Look for matching netlist clocks
                    for(AtomNetId clock_net : netlist_clock_nets_) {
                        const auto& clock_name = netlist_.net_name(clock_net);

                        if(std::regex_match(clock_name, clock_name_regex)) {
                            found = true;
                            //Create netlist clock
                            tatum::DomainId netlist_clk = tc_.create_clock_domain(clock_name);

                            if(sdc_clocks_.count(netlist_clk)) {
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

                    if(!found) {
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
                if(netlist_clock_nets_.size() == 1) {
                    //Support non-standard wildcard clock name for set_input_delay/set_output_delay
                    //commands, provided it is unambiguous (i.e. there is only one netlist clock)

                    std::string clock_name = netlist_.net_name(*netlist_clock_nets_.begin());
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

            if(io_pins.empty()) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_,
                          "Found no matching primary inputs or primary outputs");
            }

            float max_delay = sdc_units_to_seconds(cmd.max_delay);

            for(AtomPinId pin : io_pins) {
                tatum::NodeId tnode = lookup_.atom_pin_tnode(pin);
                VTR_ASSERT(tnode);

                //Set i/o constraint
                if (cmd.type == sdcparse::IoDelayType::INPUT) {

                    tc_.set_input_constraint(tnode, domain, tatum::Time(max_delay));

                    if(netlist_.pin_type(pin) == AtomPinType::SINK) {
                        AtomBlockId blk = netlist_.pin_block(pin);
                        std::string io_name = orig_blif_name(netlist_.block_name(blk));

                        vtr::printf_warning(fname_.c_str(), lineno_, 
                                            "set_input_delay command applied to primary output '%s'\n",
                                            io_name.c_str());
                    }
                } else {
                    VTR_ASSERT(cmd.type == sdcparse::IoDelayType::OUTPUT);

                    tc_.set_output_constraint(tnode, domain, tatum::Time(max_delay));

                    if(netlist_.pin_type(pin) == AtomPinType::DRIVER) {
                        AtomBlockId blk = netlist_.pin_block(pin);
                        std::string io_name = orig_blif_name(netlist_.block_name(blk));

                        vtr::printf_warning(fname_.c_str(), lineno_, 
                                            "set_output_delay command applied to primary input '%s'\n",
                                            io_name.c_str());
                    }
                }
            }
        }

        void set_clock_groups(const sdcparse::SetClockGroups& /*cmd*/) override { 
            ++num_commands_;
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_clock_groups currently unsupported"); 
        }

        void set_false_path(const sdcparse::SetFalsePath& /*cmd*/) override { 
            ++num_commands_;
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_false_path currently unsupported"); 
        }

        void set_min_max_delay(const sdcparse::SetMinMaxDelay& /*cmd*/) override { 
            ++num_commands_;
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_min_delay/set_max_delay currently unsupported"); 
        }

        void set_multicycle_path(const sdcparse::SetMulticyclePath& /*cmd*/) override {
            ++num_commands_;
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_multicycle_path currently unsupported"); 
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

            for (auto from_clock : from_clocks) {
                for (auto to_clock : to_clocks) {

                    if (cmd.type == sdcparse::SetupHoldType::SETUP) {
                        tc_.set_setup_clock_uncertainty(from_clock, to_clock, tatum::Time(uncertainty));

                    } else {
                        VTR_ASSERT(cmd.type == sdcparse::SetupHoldType::HOLD);

                        tc_.set_hold_clock_uncertainty(from_clock, to_clock, tatum::Time(uncertainty));
                    }
                }
            }
        }

        void set_clock_latency(const sdcparse::SetClockLatency& cmd) override { 
            ++num_commands_;

            if (cmd.type != sdcparse::ClockLatencyType::SOURCE) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_clock_latency only support specifying -source latency"); 
            }

            if (cmd.early_late == sdcparse::EarlyLateType::EARLY) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_clock_latency currently does not support -early latency"); 
            }

            VTR_ASSERT(cmd.early_late == sdcparse::EarlyLateType::LATE || cmd.early_late == sdcparse::EarlyLateType::NONE);

            auto clocks = get_clocks(cmd.target_clocks);
            if (clocks.empty()) {
                clocks = get_all_clocks();
            }

            float latency = sdc_units_to_seconds(cmd.value);

            for (auto clock : clocks) {
                tc_.set_source_latency(clock, tatum::Time(latency)); 
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

                        vtr::printf_warning(fname_.c_str(), lineno_, 
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

        std::set<AtomPinId> get_ports(const sdcparse::StringGroup& port_group) {
            if(port_group.type != sdcparse::StringGroupType::PORT) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, 
                         "Expected port collection via get_ports"); 
            }

            std::set<AtomPinId> pins;
            for (const auto& port_pattern : port_group.strings) {
                std::regex port_regex = glob_pattern_to_regex(port_pattern);

                bool found = false;
                for(const auto& kv : netlist_primary_ios_) {
                    
                    const std::string& io_name = kv.first;
                    if(std::regex_match(io_name, port_regex)) {
                        found = true;

                        AtomPinId pin = kv.second;
                        
                        pins.insert(pin);
                    }
                }

                if(!found) {
                    vtr::printf_warning(fname_.c_str(), lineno_, 
                                        "get_ports target name or pattern '%s' matched no clocks\n",
                                        port_pattern.c_str());
                }
            }
            return pins;
        }

        std::set<tatum::DomainId> get_clocks(const sdcparse::StringGroup& clock_group) {
            if(clock_group.type != sdcparse::StringGroupType::CLOCK) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, 
                         "Expected clock collection via get_clocks"); 
            }

            std::set<tatum::DomainId> domains;
            for (const auto& clock_glob_pattern : clock_group.strings) {
                std::regex clock_regex = glob_pattern_to_regex(clock_glob_pattern);

                bool found = false;
                for(tatum::DomainId domain : tc_.clock_domains()) {
                    
                    const std::string& clock_name = tc_.clock_domain_name(domain);
                    if(std::regex_match(clock_name, clock_regex)) {
                        found = true;
                        
                        domains.insert(domain);
                    }
                }

                if(!found) {
                    vtr::printf_warning(fname_.c_str(), lineno_, 
                                        "get_clocks target name or pattern '%s' matched no clocks\n",
                                        clock_glob_pattern.c_str());
                }
            }
            
            return domains;
        }

        std::set<AtomPinId> get_pins(const sdcparse::StringGroup& pin_group) {
            if(pin_group.type != sdcparse::StringGroupType::PIN) {
                vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, 
                         "Expected pin collection via get_pins"); 
            }

            std::set<AtomPinId> pins;
            for (const auto& pin_pattern : pin_group.strings) {
                std::regex pin_regex = glob_pattern_to_regex(pin_pattern);

                bool found = false;
                for(AtomPinId pin : netlist_.pins()) {

                    const std::string& pin_name = netlist_.pin_name(pin);

                    if(std::regex_match(pin_name, pin_regex)) {
                        found = true;
                        
                        pins.insert(pin);
                    }
                }

                if(!found) {
                    vtr::printf_warning(fname_.c_str(), lineno_, 
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

        void resolve_clock_constraints() {
            //Set the default clock constraints
            for(tatum::DomainId launch_clock : tc_.clock_domains()) {
                for(tatum::DomainId capture_clock : tc_.clock_domains()) {
                    float constraint = calculate_default_setup_constraint(launch_clock, capture_clock);                    

                    //Convert to seconds
                    constraint = sdc_units_to_seconds(constraint);

                    tc_.set_setup_constraint(launch_clock, capture_clock, tatum::Time(constraint));
                }
            }

            //TODO: deal with override constraints
        }

        float calculate_default_setup_constraint(tatum::DomainId launch_domain, tatum::DomainId capture_domain) const {
            constexpr int CLOCK_SCALE = 1000;

            auto launch_iter = sdc_clocks_.find(launch_domain);
            VTR_ASSERT(launch_iter != sdc_clocks_.end());
            const sdcparse::CreateClock& launch_clock = launch_iter->second;

            auto capture_iter = sdc_clocks_.find(capture_domain);
            VTR_ASSERT(capture_iter != sdc_clocks_.end());
            const sdcparse::CreateClock& capture_clock = capture_iter->second;

            VTR_ASSERT_MSG(launch_clock.period > 0., "Clock period must be positive");
            VTR_ASSERT_MSG(capture_clock.period > 0., "Clock period must be positive");

            //If the source and sink domains have the same period and edges, the constraint is the common clock period. 
            if (std::fabs(launch_clock.period - capture_clock.period) < EPSILON && 
                std::fabs(launch_clock.rise_edge - capture_clock.rise_edge) < EPSILON &&
                std::fabs(launch_clock.fall_edge - capture_clock.fall_edge) < EPSILON) {
                return launch_clock.period; 
            }

            //If either period is 0, the constraint is 0
            if (launch_clock.period < EPSILON || capture_clock.period < EPSILON) {
                return 0.;
            }

            /*
             * Use edge counting to find the period
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
            int num_launch_edges = lcm_period/launch_period + 1; 
            for(int i = 0; i < num_launch_edges; ++i) {
                launch_edges.push_back(launch_rise_time);
                launch_rise_time += launch_period;
            }

            //Capture edges
            int capture_rise_time = capture_rise_edge;
            int num_capture_edges = lcm_period/capture_period + 1;
            std::vector<int> capture_edges;
            for(int i = 0; i < num_capture_edges; ++i) {
                capture_edges.push_back(capture_rise_time);
                capture_rise_time += capture_period;
            }

            //Compare every edge in source_edges with every edge in sink_edges. 
            //The lowest STRICTLY POSITIVE difference between a sink edge and a 
            //source edge yeilds the setup time constraint.
            int scaled_constraint = std::numeric_limits<int>::max(); //Initialize to +inf, so any constraint will be less

            for(int launch_edge : launch_edges) {
                for(int capture_edge : capture_edges) {
                    if(capture_edge > launch_edge) { //Postive only
                        int edge_diff = capture_edge - launch_edge;
                        VTR_ASSERT(edge_diff >= 0.);

                        scaled_constraint = std::min(scaled_constraint, edge_diff);
                    }
                }
            }

            //Rescale the constraint back to a float
            return float(scaled_constraint) / CLOCK_SCALE;
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

        std::map<tatum::DomainId,sdcparse::CreateClock> sdc_clocks_;
        std::set<AtomNetId> netlist_clock_nets_;
        std::map<std::string,AtomPinId> netlist_primary_ios_;
};

std::unique_ptr<tatum::TimingConstraints> read_sdc2(const t_timing_inf& timing_inf, 
                                                   const AtomNetlist& netlist, 
                                                   const AtomLookup& lookup, 
                                                   tatum::TimingGraph& timing_graph) {
    auto timing_constraints = std::unique_ptr<tatum::TimingConstraints>(new tatum::TimingConstraints());

    if (!timing_inf.timing_analysis_enabled) {
		vtr::printf("\n");
		vtr::printf("Timing analysis off\n");
        apply_default_timing_constraints(netlist, lookup, *timing_constraints);
        return timing_constraints;
    }

    FILE* sdc_file = fopen(timing_inf.SDCFile, "r");
    if (sdc_file == nullptr) {
        //No SDC file
		vtr::printf("\n");
		vtr::printf("SDC file '%s' not found\n", timing_inf.SDCFile);
        apply_default_timing_constraints(netlist, lookup, *timing_constraints);
        return timing_constraints;
    }

    VTR_ASSERT(sdc_file != nullptr);

    //Parse the file
    SdcParseCallback2 callback(netlist, lookup, *timing_constraints, timing_graph);
    sdc_parse_file(sdc_file, callback, timing_inf.SDCFile);
    fclose(sdc_file);

    if (callback.num_commands() == 0) {
		vtr::printf("\n");
		vtr::printf("SDC file '%s' contained no SDC commands\n", timing_inf.SDCFile);
        apply_default_timing_constraints(netlist, lookup, *timing_constraints);
    } else {
		vtr::printf("\n");
		vtr::printf("Applied %zu SDC commands from '%s'\n", callback.num_commands(), timing_inf.SDCFile);
    }
    vtr::printf("\n");

    return timing_constraints;
}

//Apply the default timing constraints (i.e. if there are no user specified constraints)
//appropriate to the type of circuit.
void apply_default_timing_constraints(const AtomNetlist& netlist, 
                                      const AtomLookup& lookup, 
                                      tatum::TimingConstraints& tc) {
    std::set<AtomNetId> netlist_clocks = find_netlist_clocks(netlist); 

    if(netlist_clocks.size() == 0) {
        apply_combinational_default_timing_constraints(netlist, lookup, tc);

    } else if (netlist_clocks.size() == 1) {
        apply_single_clock_default_timing_constraints(netlist, lookup, *netlist_clocks.begin(), tc);

    } else {
        VTR_ASSERT(netlist_clocks.size() > 1);

        apply_multi_clock_default_timing_constraints(netlist, lookup, netlist_clocks, tc);
    }

}

//Apply the default timing constraints for purely combinational circuits which have
//no explicit netlist clock
void apply_combinational_default_timing_constraints(const AtomNetlist& netlist,
                                                    const AtomLookup& lookup,
                                                    tatum::TimingConstraints& tc) {
    std::string clock_name = "virtual_io_clock";

    vtr::printf("Setting default timing constraints:\n");
    vtr::printf("   * constrain all primay inputs and primary outputs on a virtual external clock '%s'\n", clock_name.c_str());
    vtr::printf("   * optimize virtual clock to run as fast as possible\n");

    //Create a virtual clock, with 0 period
    tatum::DomainId domain = tc.create_clock_domain(clock_name);
    tc.set_setup_constraint(domain, domain, tatum::Time(0.));

    //Constrain all I/Os with zero input/output delay
    constrain_all_ios(netlist, lookup, tc, domain, domain, tatum::Time(0.), tatum::Time(0.));

    //Mark constant generator timing nodes
    mark_constant_generators(netlist, lookup, tc);
}

//Apply the default timing constraints for circuits with a single netlist clock
void apply_single_clock_default_timing_constraints(const AtomNetlist& netlist,
                                                   const AtomLookup& lookup,
                                                   const AtomNetId clock_net,
                                                   tatum::TimingConstraints& tc) {
    std::string clock_name = netlist.net_name(clock_net);

    vtr::printf("Setting default timing constraints:\n");
    vtr::printf("   * constrain all primay inputs and primary outputs on netlist clock '%s'\n", clock_name.c_str());
    vtr::printf("   * optimize netlist clock to run as fast as possible\n");

    //Create the netlist clock with period 0
    tatum::DomainId domain = tc.create_clock_domain(clock_name);
    tc.set_setup_constraint(domain, domain, tatum::Time(0.));

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
                                                  const std::set<AtomNetId>& clock_nets,
                                                  tatum::TimingConstraints& tc) {
    std::string virtual_clock_name = "virtual_io_clock";
    vtr::printf("Setting default timing constraints:\n");
    vtr::printf("   * constrain all primay inputs and primary outputs on a virtual external clock '%s'\n", virtual_clock_name.c_str());
    vtr::printf("   * optimize all netlist and virtual clocks to run as fast as possible\n");
    vtr::printf("   * ignore cross netlist clock domain timing paths\n");

    //Create a virtual clock, with 0 period
    tatum::DomainId virtual_clock = tc.create_clock_domain(virtual_clock_name);
    tc.set_setup_constraint(virtual_clock, virtual_clock, tatum::Time(0.));

    //Constrain all I/Os with zero input/output delay t the virtual clock
    constrain_all_ios(netlist, lookup, tc, virtual_clock, virtual_clock, tatum::Time(0.), tatum::Time(0.));

    //Create each of the netlist clocks, and constrain it to period 0. Do not analyze cross-domain paths
    for(AtomNetId clock_net : clock_nets) {

        //Create the clock
        std::string clock_name = netlist.net_name(clock_net);
        tatum::DomainId clock = tc.create_clock_domain(clock_name);

        //Mark the clock domain source
        AtomPinId clock_driver_pin = netlist.net_driver(clock_net);
        tatum::NodeId clock_source = lookup.atom_pin_tnode(clock_driver_pin);
        VTR_ASSERT(clock_source);
        tc.set_clock_domain_source(clock_source, clock);

        //Do not analyze cross-domain timing paths (except to/from virtual clock)
        tc.set_setup_constraint(clock, clock, tatum::Time(0.)); //Intra-domain
        tc.set_setup_constraint(clock, virtual_clock, tatum::Time(0.)); //netlist to virtual
        tc.set_setup_constraint(virtual_clock, clock, tatum::Time(0.)); //virtual to netlist
    }

    //Mark constant generator timing nodes
    mark_constant_generators(netlist, lookup, tc);
}

//Look through the netlist to find any constant generators, and mark them as
//constant generators in the timing constraints
void mark_constant_generators(const AtomNetlist& netlist, 
                              const AtomLookup& lookup, 
                              tatum::TimingConstraints& tc) {

    for(AtomPinId pin : netlist.pins()) {
        if(netlist.pin_is_constant(pin)) {
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
            VTR_ASSERT(netlist.block_pins(blk).size() == 1);
            AtomPinId pin = *netlist.block_pins(blk).begin();

            //Find the associated tnode
            tatum::NodeId tnode = lookup.atom_pin_tnode(pin);

            //Constrain it
            if (type == AtomBlockType::INPAD) {
                tc.set_input_constraint(tnode, input_domain, input_delay);
            } else {
                VTR_ASSERT(type == AtomBlockType::OUTPAD);
                tc.set_output_constraint(tnode, output_domain, output_delay);
            }
        }
    }
}

//Find all the clock nets in the specified netlist
std::set<AtomNetId> find_netlist_clocks(const AtomNetlist& netlist) {

    std::set<AtomNetId> clock_nets; //The clock nets

    std::map<const t_model*,std::vector<const t_model_ports*>> clock_gen_ports; //Records info about clock generating ports

    //Look through all the blocks (except I/Os) to find sink clock pins, or
    //clock generators
    //
    //Since we don't have good information about what pins are clock generators we build a lookup as we go
    for(auto blk_id : netlist.blocks()) {
        AtomBlockType type = netlist.block_type(blk_id);
        if(type != AtomBlockType::BLOCK) continue;

        //Save any clock generating ports on this model type
        const t_model* model = netlist.block_model(blk_id);
        VTR_ASSERT(model);
        auto iter = clock_gen_ports.find(model);
        if(iter == clock_gen_ports.end()) { 
            //First time we've seen this model, intialize it
            clock_gen_ports[model] = {};

            //Look at all the ports to find clock generators
            for(const t_model_ports* model_port = model->outputs; model_port; model_port = model_port->next) {
                VTR_ASSERT(model_port->dir == OUT_PORT);
                if(model_port->is_clock) {
                    //Clock generator
                    clock_gen_ports[model].push_back(model_port);
                }
            }
        }

        //Look for connected input clocks
        for(auto pin_id : netlist.block_clock_pins(blk_id)) {
            AtomNetId clk_net_id = netlist.pin_net(pin_id);
            VTR_ASSERT(clk_net_id);

            clock_nets.insert(clk_net_id);
        }

        //Look for any generated clocks
        if(!clock_gen_ports[model].empty()) {
            //This is a clock generator

            //Check all the clock generating ports
            for(const t_model_ports* model_port : clock_gen_ports[model]) {
                AtomPortId clk_gen_port = netlist.find_port(blk_id, model_port);

                for(AtomPinId pin_id : netlist.port_pins(clk_gen_port)) {
                    AtomNetId clk_net_id = netlist.pin_net(pin_id);
                    VTR_ASSERT(clk_net_id);
                    clock_nets.insert(clk_net_id);
                }
            }
        }
    }

    return clock_nets;
}

std::map<std::string,AtomPinId> find_netlist_primary_ios(const AtomNetlist& netlist) {
    std::map<std::string,AtomPinId> primary_inputs;

    for(AtomBlockId blk : netlist.blocks()) {
        auto type = netlist.block_type(blk);
        if(type == AtomBlockType::INPAD || type == AtomBlockType::OUTPAD) {
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

    if(name.find(BLIF_UNIQ_PREFIX) == 0) { //Starts with prefix
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
    //   Convert '*' to ".*" so literal '*' in regex matches any sequence

    std::string regex_str = vtr::replace_all(glob_pattern, ".", "\\.");
    regex_str = vtr::replace_all(regex_str, "*", ".*");

    return std::regex(regex_str, std::regex::grep);
}
