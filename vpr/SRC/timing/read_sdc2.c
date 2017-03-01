#include "read_sdc2.h"

#include "vtr_log.h"
#include "vtr_assert.h"

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

class SdcParseCallback2 : public sdcparse::Callback {
    public: //sdcparse::Callback interface
        //Start of parsing
        void start_parse() override {}

        //Sets current filename
        void filename(std::string fname) override { fname_ = fname; }

        //Sets current line number
        void lineno(int line_num) override { lineno_ = line_num; }

        //Individual commands
        void create_clock(const sdcparse::CreateClock& /*cmd*/) override { 
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "create_clock currently unsupported"); 
        }
        void set_io_delay(const sdcparse::SetIoDelay& /*cmd*/) override { 
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_input_delay/set_output_delay currently unsupported"); 
        }
        void set_clock_groups(const sdcparse::SetClockGroups& /*cmd*/) override { 
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_clock_groups currently unsupported"); 
        }
        void set_false_path(const sdcparse::SetFalsePath& /*cmd*/) override { 
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_false_path currently unsupported"); 
        }
        void set_min_max_delay(const sdcparse::SetMinMaxDelay& /*cmd*/) override { 
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_min_delay/set_max_delay currently unsupported"); 
        }
        void set_multicycle_path(const sdcparse::SetMulticyclePath& /*cmd*/) override {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_multicycle_path currently unsupported"); 
        }
        void set_clock_uncertainty(const sdcparse::SetClockUncertainty& /*cmd*/) override { 
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_clock_uncertainty currently unsupported"); 
        }
        void set_clock_latency(const sdcparse::SetClockLatency& /*cmd*/) override { 
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_clock_latency currently unsupported"); 
        }
        void set_disable_timing(const sdcparse::SetDisableTiming& /*cmd*/) override { 
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_disable_timing currently unsupported"); 
        }
        void set_timing_derate(const sdcparse::SetTimingDerate& /*cmd*/) override { 
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "set_timing_derate currently unsupported"); 
        }

        //End of parsing
        void finish_parse() override {}

        //Error during parsing
        void parse_error(const int /*curr_lineno*/, const std::string& near_text, const std::string& msg) override {
            vpr_throw(VPR_ERROR_SDC, fname_.c_str(), lineno_, "%s near '%s'", msg.c_str(), near_text.c_str());
        }

    public:
        size_t num_commands() { return num_commands_; }
    private:
        size_t num_commands_ = 0;
        std::string fname_;
        int lineno_ = -1;

};

std::unique_ptr<tatum::TimingConstraints> read_sdc2(const t_timing_inf& timing_inf, 
                                                   const AtomNetlist& netlist, 
                                                   const AtomLookup& lookup, 
                                                   tatum::TimingGraph& /*timing_graph*/) {
    auto tc = std::unique_ptr<tatum::TimingConstraints>(new tatum::TimingConstraints());

    if (!timing_inf.timing_analysis_enabled) {
		vtr::printf_info("\n");
		vtr::printf_info("Timing analysis off\n");
        apply_default_timing_constraints(netlist, lookup, *tc);
        return tc;
    }

    FILE* sdc_file = fopen(timing_inf.SDCFile, "r");
    if (sdc_file == nullptr) {
        //No SDC file
		vtr::printf_info("\n");
		vtr::printf_info("SDC file '%s' not found\n", timing_inf.SDCFile);
        apply_default_timing_constraints(netlist, lookup, *tc);
        return tc;
    }

    VTR_ASSERT(sdc_file != nullptr);

    //Parse the file
    SdcParseCallback2 callback;
    sdc_parse_file(sdc_file, callback, timing_inf.SDCFile);

    if (callback.num_commands() == 0) {
		vtr::printf_info("\n");
		vtr::printf_info("SDC file '%s' contained no SDC commands\n", timing_inf.SDCFile);
        apply_default_timing_constraints(netlist, lookup, *tc);
    } else {
		vtr::printf_info("\n");
		vtr::printf_info("Applied %zu SDC commands from %s.\n", callback.num_commands(), timing_inf.SDCFile);
    }

    return tc;
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

