#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cassert>
#include <cmath>
#include <memory>

#include "sdc_common.hpp"
#include "sdc_error.hpp"

namespace sdcparse {

/*
 * Functions for create_clock
 */
void sdc_create_clock_set_period(Callback& callback, const Lexer& lexer, CreateClock& sdc_create_clock, double period) {
    if(!std::isnan(sdc_create_clock.period)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Can only define a single clock period.\n"); 
    } else {
        sdc_create_clock.period = period;
    }
}

void sdc_create_clock_set_name(Callback& callback, const Lexer& lexer, CreateClock& sdc_create_clock, const std::string& name) {
    if(!sdc_create_clock.name.empty()) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Can only define a single clock name.\n");
    } else {
        sdc_create_clock.name = name;
    }
}

void sdc_create_clock_set_waveform(Callback& callback, const Lexer& lexer, CreateClock& sdc_create_clock, double rise_edge, double fall_edge) {
    if(!std::isnan(sdc_create_clock.rise_edge) || !std::isnan(sdc_create_clock.fall_edge)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Can only define a single waveform.\n"); 
    } else {
        sdc_create_clock.rise_edge = rise_edge;
        sdc_create_clock.fall_edge = fall_edge;
    }
}

void sdc_create_clock_add_targets(Callback& callback, const Lexer& lexer, CreateClock& sdc_create_clock, StringGroup target_group) {
    assert(target_group.type == StringGroupType::STRING);

    if(!sdc_create_clock.targets.strings.empty()) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Can only define a single set of targets for clock creation. "
                  "If you want to define multiple targets specify them as a list (e.g. \"{target1 target2}\").\n");
    }

    sdc_create_clock.targets = target_group;
}

void add_sdc_create_clock(Callback& callback, const Lexer& lexer, CreateClock& sdc_create_clock) {
    /*
     * Error Checking
     */

    //Must have a clock period
    if(std::isnan(sdc_create_clock.period)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must define clock period.\n");
    }

    //Must have either a target (if a netlist clock), or a name (if a virtual clock) 
    if(sdc_create_clock.targets.strings.size() == 0 && sdc_create_clock.name.empty()) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must define either a target (for netlist clock) or a name (for virtual clock).\n");
    }

    //Currently we do not support defining clock names that differ from the netlist target name
    if(sdc_create_clock.targets.strings.size() != 0 && !sdc_create_clock.name.empty()) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Currently custom names for netlist clocks are unsupported, remove '-name' option or create a virtual clock.\n");
    }

    /*
     * Set defaults
     */
    //Determine default rise/fall time for waveform
    if(std::isnan(sdc_create_clock.rise_edge) && std::isnan(sdc_create_clock.fall_edge)) {
        sdc_create_clock.rise_edge = 0.0;
        sdc_create_clock.fall_edge = sdc_create_clock.period / 2;
    }
    assert(!std::isnan(sdc_create_clock.rise_edge));
    assert(!std::isnan(sdc_create_clock.fall_edge));
    
    //Determine if clock is virtual or not
    if(sdc_create_clock.targets.strings.size() == 0 && !sdc_create_clock.name.empty()) {
        //Have a virtual target if there is a name AND no target strings
        sdc_create_clock.is_virtual = true;
    } else {
        assert(sdc_create_clock.targets.strings.size() > 0);
        //Have a real target so this is not a virtual clock
        sdc_create_clock.is_virtual = false;
    }

    /*
     * Add command
     */
    callback.create_clock(sdc_create_clock);

}

/*
 * Functions for set_input_delay/set_output_delay
 */
void sdc_set_io_delay_set_clock(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay, const std::string& clock_name) {
    if(!sdc_set_io_delay.clock_name.empty()) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Can only specify a single clock\n"); 
    }

    sdc_set_io_delay.clock_name = clock_name;
}

void sdc_set_io_delay_set_value(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay, double value) {
    if(!std::isnan(sdc_set_io_delay.delay)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Delay value can only specified once.\n"); 
    }

    sdc_set_io_delay.delay = value;
}

void sdc_set_io_delay_set_max(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay) {
    if (sdc_set_io_delay.is_max) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "-max can only be specified once.\n"); 
    }

    sdc_set_io_delay.is_max = true;
}

void sdc_set_io_delay_set_min(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay) {
    if (sdc_set_io_delay.is_min) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "-min can only be specified once.\n"); 
    }

    sdc_set_io_delay.is_min = true;
}

void sdc_set_io_delay_set_ports(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay, StringGroup ports) {
    assert(ports.type == StringGroupType::PORT);

    if(!sdc_set_io_delay.target_ports.strings.empty()) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Currently only a single get_ports command is supported.\n"); 
    }

    sdc_set_io_delay.target_ports = ports;
}

void add_sdc_set_io_delay(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay) {
    /*
     * Error checks
     */
    if(sdc_set_io_delay.clock_name.empty()) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify clock name.\n"); 
    }

    if(std::isnan(sdc_set_io_delay.delay)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify delay value.\n"); 
    }

    if(sdc_set_io_delay.target_ports.strings.empty()) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify target ports using get_ports.\n"); 
    }

    /*
     * Add command
     */
    callback.set_io_delay(sdc_set_io_delay);
}

/*
 * Functions for set_clock_groups
 */
void sdc_set_clock_groups_set_type(Callback& callback, const Lexer& lexer, SetClockGroups& sdc_set_clock_groups, ClockGroupsType type) {
    if(sdc_set_clock_groups.type != ClockGroupsType::NONE) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Can only specify a single clock groups relation type (e.g. '-exclusive')\n"); 
    }

    sdc_set_clock_groups.type = type;
}

void sdc_set_clock_groups_add_group(Callback& /*callback*/, const Lexer& /*lexer*/, SetClockGroups& sdc_set_clock_groups, StringGroup clock_group) {
    assert(clock_group.type == StringGroupType::CLOCK || clock_group.type == StringGroupType::STRING);

    //Duplicate and insert the clock group
    sdc_set_clock_groups.clock_groups.push_back(clock_group);
}

void add_sdc_set_clock_groups(Callback& callback, const Lexer& lexer, SetClockGroups& sdc_set_clock_groups) {
    /*
     * Error checks
     */
    if(sdc_set_clock_groups.type == ClockGroupsType::NONE) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify clock relation type as '-exclusive'.\n"); 
    }

    if(sdc_set_clock_groups.clock_groups.size() < 2) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify at least 2 clock groups.\n"); 
    }

    /*
     * Add command
     */
    callback.set_clock_groups(sdc_set_clock_groups);
}

/*
 * Functions for set_false_path
 */
void sdc_set_false_path_add_to_from_group(Callback& callback, const Lexer& lexer, 
                                                            SetFalsePath& sdc_set_false_path, 
                                                            StringGroup group, 
                                                            FromToType to_from_dir) {
    assert(group.type == StringGroupType::CLOCK || group.type == StringGroupType::STRING);

    //Error checking
    if(to_from_dir == FromToType::FROM) {
        //Check that we haven't already defined the from path    
        if(!sdc_set_false_path.from.strings.empty()) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == FromToType::TO);
        //Check that we haven't already defined the from path    
        if(!sdc_set_false_path.to.strings.empty()) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single '-to' option is supported.\n"); 
        }
    }

    //Add the clock group
    if(to_from_dir == FromToType::FROM) {
        sdc_set_false_path.from = group;
    } else {
        assert(to_from_dir == FromToType::TO);
        sdc_set_false_path.to = group;
    }
}

void add_sdc_set_false_path(Callback& callback, const Lexer& /*lexer*/, SetFalsePath& sdc_set_false_path) {
    /*
     * Error checks
     */

    /*
     * Add command
     */
    callback.set_false_path(sdc_set_false_path);

}

/*
 * Functions for set_max_delay/set_min_delay
 */
void sdc_set_min_max_delay_set_value(Callback& callback, const Lexer& lexer, SetMinMaxDelay& sdc_set_min_max_delay, double min_max_delay) {
    if(!std::isnan(sdc_set_min_max_delay.value)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify max delay value only once.\n"); 
    }
    sdc_set_min_max_delay.value = min_max_delay;
}

void sdc_set_min_max_delay_add_to_from_group(Callback& callback, const Lexer& lexer, SetMinMaxDelay& sdc_set_min_max_delay, StringGroup group, FromToType to_from_dir) {
    assert(group.type == StringGroupType::CLOCK || group.type == StringGroupType::STRING);

    //Error checking
    if(to_from_dir == FromToType::FROM) {
        //Check that we haven't already defined the from path    
        if(!sdc_set_min_max_delay.from.strings.empty()) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == FromToType::TO);
        //Check that we haven't already defined the from path    
        if(!sdc_set_min_max_delay.to.strings.empty()) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single '-to' option is supported.\n"); 
        }
    }

    //Add the clock group
    if(to_from_dir == FromToType::FROM) {
        sdc_set_min_max_delay.from = group;
    } else {
        assert(to_from_dir == FromToType::TO);
        sdc_set_min_max_delay.to = group;
    }
}

void add_sdc_set_min_max_delay(Callback& callback, const Lexer& lexer, SetMinMaxDelay& sdc_set_min_max_delay) {
    /*
     * Error checks
     */
    if(std::isnan(sdc_set_min_max_delay.value)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify the max delay value.\n"); 
    }

    /*
     * Add command
     */
    callback.set_min_max_delay(sdc_set_min_max_delay);

}

/*
 * Functions for set_multicycle_path
 */
void sdc_set_multicycle_path_set_setup(Callback& callback, const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path) {
    if(sdc_set_multicycle_path.is_setup) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "'-setup' should only be specified once.\n"); 
    }
    sdc_set_multicycle_path.is_setup = true;
}

void sdc_set_multicycle_path_set_hold(Callback& callback, const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path) {
    if(sdc_set_multicycle_path.is_hold) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "'-hold' should only be specified once.\n"); 
    }
    sdc_set_multicycle_path.is_hold = true;
}

void sdc_set_multicycle_path_set_mcp_value(Callback& callback, const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path, int mcp_value) {
    if(sdc_set_multicycle_path.mcp_value != UNINITIALIZED_INT) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify multicycle path value only once.\n"); 
    }
    sdc_set_multicycle_path.mcp_value = mcp_value;
}

void sdc_set_multicycle_path_add_to_from_group(Callback& callback, const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path, 
                                                                     StringGroup group, 
                                                                     FromToType to_from_dir) {
    assert(group.type == StringGroupType::CLOCK || group.type == StringGroupType::STRING);

    //Error checking
    if(to_from_dir == FromToType::FROM) {
        //Check that we haven't already defined the from path    
        if(!sdc_set_multicycle_path.from.strings.empty()) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == FromToType::TO);
        //Check that we haven't already defined the from path    
        if(!sdc_set_multicycle_path.to.strings.empty()) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single '-to' option is supported.\n"); 
        }
    }

    //Add the clock group
    if(to_from_dir == FromToType::FROM) {
        sdc_set_multicycle_path.from = group;
    } else {
        assert(to_from_dir == FromToType::TO);
        sdc_set_multicycle_path.to = group;
    }
}

void add_sdc_set_multicycle_path(Callback& callback, const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path) {
    /*
     * Error checks
     */
    if(sdc_set_multicycle_path.mcp_value == UNINITIALIZED_INT) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify the multicycle path value.\n"); 
    }

    /*
     * Add command
     */
    callback.set_multicycle_path(sdc_set_multicycle_path);

}

/*
 * Functions for set_clock_uncertainty
 */
void sdc_set_clock_uncertainty_set_setup(Callback& callback, const Lexer& lexer, SetClockUncertainty& sdc_set_clock_uncertainty) {
    if(sdc_set_clock_uncertainty.is_setup) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "'-setup' should only be specified once.\n"); 
    }
    sdc_set_clock_uncertainty.is_setup = true;
}

void sdc_set_clock_uncertainty_set_hold(Callback& callback, const Lexer& lexer, SetClockUncertainty& sdc_set_clock_uncertainty) {
    if(sdc_set_clock_uncertainty.is_hold) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "'-hold' should only be specified once.\n"); 
    }
    sdc_set_clock_uncertainty.is_hold = true;
}

void sdc_set_clock_uncertainty_set_value(Callback& callback, const Lexer& lexer, SetClockUncertainty& sdc_set_clock_uncertainty, float value) {
    if(!std::isnan(sdc_set_clock_uncertainty.value)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify clock uncertainty value only once.\n"); 
    }
    sdc_set_clock_uncertainty.value = value;
}

void sdc_set_clock_uncertainty_add_to_from_group(Callback& callback, const Lexer& lexer, SetClockUncertainty& sdc_set_clock_uncertainty, 
                                                                     StringGroup group, 
                                                                     FromToType to_from_dir) {
    assert(group.type == StringGroupType::CLOCK || group.type == StringGroupType::STRING);

    //Error checking
    if(to_from_dir == FromToType::FROM) {
        //Check that we haven't already defined the from path    
        if(!sdc_set_clock_uncertainty.from.strings.empty()) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == FromToType::TO);
        //Check that we haven't already defined the from path    
        if(!sdc_set_clock_uncertainty.to.strings.empty()) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single '-to' option is supported.\n"); 
        }
    }

    //Add the clock group
    if(to_from_dir == FromToType::FROM) {
        sdc_set_clock_uncertainty.from = group;
    } else {
        assert(to_from_dir == FromToType::TO);
        sdc_set_clock_uncertainty.to = group;
    }
}

void add_sdc_set_clock_uncertainty(Callback& callback, const Lexer& lexer, SetClockUncertainty& sdc_set_clock_uncertainty) {
    /*
     * Error checks
     */
    if(std::isnan(sdc_set_clock_uncertainty.value)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify the clock uncertainty value.\n"); 
    }

    /*
     * Add command
     */
    callback.set_clock_uncertainty(sdc_set_clock_uncertainty);

}

/*
 * Functions for set_clock_latency
 */
void sdc_set_clock_latency_set_type(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency, ClockLatencyType type) {
    //Error checking
    if(sdc_set_clock_latency.type != ClockLatencyType::NONE) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "The latency type (e.g. '-source') can only be specified once.\n"); 
    }

    sdc_set_clock_latency.type = type;
}

void sdc_set_clock_latency_early(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency) {
    //Error checking
    if(sdc_set_clock_latency.is_early) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "The '-early' option can only be specified once.\n"); 
    }

    sdc_set_clock_latency.is_early = true;
}

void sdc_set_clock_latency_late(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency) {
    //Error checking
    if(sdc_set_clock_latency.is_late) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "The '-late' option can only be specified once.\n"); 
    }

    sdc_set_clock_latency.is_late = true;
}

void sdc_set_clock_latency_set_value(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency, float value) {
    if(!std::isnan(sdc_set_clock_latency.value)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "The clock latency value can only be specified once.\n"); 
    }
    
    sdc_set_clock_latency.value = value;
}

void sdc_set_clock_latency_set_clocks(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency, StringGroup target_clocks) {
    if(target_clocks.type != StringGroupType::CLOCK) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Target clocks must be specified with 'get_clocks'.\n"); 
    }

    sdc_set_clock_latency.target_clocks = target_clocks;
}

void add_sdc_set_clock_latency(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency) {
    /*
     * Error checks
     */
    if(sdc_set_clock_latency.type != ClockLatencyType::SOURCE) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify the clock latency type (e.g. '-source').\n"); 
    }

    if(std::isnan(sdc_set_clock_latency.value)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify the clock latency value.\n"); 
    }

    if(sdc_set_clock_latency.target_clocks.strings.empty()) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify target clocks.\n"); 
    }

    /*
     * Add command
     */
    callback.set_clock_latency(sdc_set_clock_latency);
}

/*
 * Functions for set_disable_timing
 */
void sdc_set_disable_timing_add_to_from_group(Callback& callback, const Lexer& lexer, 
                                                            SetDisableTiming& sdc_set_disable_timing, 
                                                            StringGroup group, 
                                                            FromToType to_from_dir) {

    //Error checking
    if (group.type != StringGroupType::PIN && group.type != StringGroupType::STRING) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only get_pins supported with set_disable_timing.\n"); 
    }

    if(to_from_dir == FromToType::FROM) {
        //Check that we haven't already defined the from path    
        if(!sdc_set_disable_timing.from.strings.empty()) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == FromToType::TO);
        //Check that we haven't already defined the from path    
        if(!sdc_set_disable_timing.to.strings.empty()) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single '-to' option is supported.\n"); 
        }
    }

    //Add the clock group
    if(to_from_dir == FromToType::FROM) {
        sdc_set_disable_timing.from = group;
    } else {
        assert(to_from_dir == FromToType::TO);
        sdc_set_disable_timing.to = group;
    }
}

void add_sdc_set_disable_timing(Callback& callback, const Lexer& /*lexer*/, SetDisableTiming& sdc_set_disable_timing) {
    /*
     * Error checks
     */

    /*
     * Add command
     */
    callback.set_disable_timing(sdc_set_disable_timing);
}

/*
 * Functions for set_timing_derate
 */
void sdc_set_timing_derate_early(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate) {
    if(sdc_set_timing_derate.is_early) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "-early should only be specified once.\n"); 
    }

    sdc_set_timing_derate.is_early = true; 
}

void sdc_set_timing_derate_late(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate) {
    if(sdc_set_timing_derate.is_late) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "-late should only be specified once.\n"); 
    }

    sdc_set_timing_derate.is_late = true; 
}

void sdc_set_timing_derate_target_type(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate, TimingDerateTargetType target_type) {
    if(target_type == TimingDerateTargetType::NET) {
        if(sdc_set_timing_derate.derate_nets) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "-net_delay should only be specified once.\n"); 
        } else {
            sdc_set_timing_derate.derate_nets = true;
        }
    } else if (target_type == TimingDerateTargetType::CELL) {
        if(sdc_set_timing_derate.derate_cells) {
            sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "-cell_delay should only be specified once.\n"); 
        } else {
            sdc_set_timing_derate.derate_cells = true;
        }
    }
}

void sdc_set_timing_derate_value(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate, float value) {
    if(!std::isnan(sdc_set_timing_derate.value)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single derate value per set_timing_derate command is allowed.\n"); 
    }

    if(sdc_set_timing_derate.value < 0) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Timing derate values must be positive.\n"); 
    }

    sdc_set_timing_derate.value = value;
}

void sdc_set_timing_derate_targets(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate, StringGroup targets) {
    if(targets.type != StringGroupType::CELL) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only get_cells is supported with set_timing_derate.\n"); 
    }

    if(!sdc_set_timing_derate.cell_targets.strings.empty()) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Only a single get_cells call is supported with set_timing_derate.\n"); 
    }

    sdc_set_timing_derate.cell_targets = targets;
}

void add_sdc_set_timing_derate(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate) {
    /*
     * Error checks
     */
    if(std::isnan(sdc_set_timing_derate.value)) {
        sdc_error_wrap(callback, lexer.lineno(), lexer.text(), "Must specify timing derate value\n"); 
    }

    /*
     * Defaults
     */
    if(!sdc_set_timing_derate.derate_nets && !sdc_set_timing_derate.derate_cells) {
        //If unspecified, both cells and nets are derated
        sdc_set_timing_derate.derate_nets = true;
        sdc_set_timing_derate.derate_cells = true;
    }

    /*
     * Add command
     */
    callback.set_timing_derate(sdc_set_timing_derate);
}

/*
 * Functions for string_group
 */
StringGroup make_sdc_string_group(StringGroupType type, const std::string& string) {
    //Convenience function for converting a single string into a group
    StringGroup sdc_string_group(type);
    sdc_string_group.strings.push_back(string);

    return sdc_string_group;
}

void sdc_string_group_add_string(StringGroup& sdc_string_group, const std::string& string) {
    //Insert the new string
    sdc_string_group.strings.push_back(string);
}

void sdc_string_group_add_strings(StringGroup& sdc_string_group, const StringGroup& string_group_to_add) {
    for(const auto& string : string_group_to_add.strings) {
        sdc_string_group_add_string(sdc_string_group, string);
    }
}

char* strdup(const char* src) {
    if(src == NULL) {
        return NULL;
    }

    size_t len = std::strlen(src); //Number of char's excluding null terminator

    //Space for [0..len] chars
    char* new_str = (char*) std::malloc((len+1)*sizeof(*src));
    assert(new_str != NULL);

    //Copy chars from [0..len], i.e. src[len] should be null terminator
    std::memcpy(new_str, src, len+1);
    
    return new_str;
}

char* strndup(const char* src, size_t len) {
    if(src == NULL) {
        return NULL;
    }

    //Space for [0..len] chars
    char* new_str = (char*) std::malloc((len+1)*sizeof(*src));
    assert(new_str != NULL);

    //Copy chars from [0..len-1]
    std::memcpy(new_str, src, len);
    
    //Add the null terminator
    new_str[len] = '\0';

    return new_str;
}

} //namespace
