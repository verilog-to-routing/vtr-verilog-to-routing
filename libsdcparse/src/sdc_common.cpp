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
void sdc_create_clock_set_period(const Lexer& lexer, CreateClock& sdc_create_clock, double period) {
    if(!std::isnan(sdc_create_clock.period)) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Can only define a single clock period.\n"); 
    } else {
        sdc_create_clock.period = period;
    }
}

void sdc_create_clock_set_name(const Lexer& lexer, CreateClock& sdc_create_clock, const std::string& name) {
    if(!sdc_create_clock.name.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Can only define a single clock name.\n");
    } else {
        sdc_create_clock.name = name;
    }
}

void sdc_create_clock_set_waveform(const Lexer& lexer, CreateClock& sdc_create_clock, double rise_edge, double fall_edge) {
    if(!std::isnan(sdc_create_clock.rise_edge) || !std::isnan(sdc_create_clock.fall_edge)) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Can only define a single waveform.\n"); 
    } else {
        sdc_create_clock.rise_edge = rise_edge;
        sdc_create_clock.fall_edge = fall_edge;
    }
}

void sdc_create_clock_add_targets(const Lexer& lexer, CreateClock& sdc_create_clock, StringGroup target_group) {
    assert(target_group.group_type == StringGroupType::STRING);

    if(!sdc_create_clock.targets.strings.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Can only define a single set of targets for clock creation. "
                  "If you want to define multiple targets specify them as a list (e.g. \"{target1 target2}\".\n");
    }

    sdc_create_clock.targets = target_group;
}

std::shared_ptr<SdcCommands> add_sdc_create_clock(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, CreateClock& sdc_create_clock) {
    /*
     * Error Checking
     */
    assert(sdc_commands);

    //Must have a clock period
    if(std::isnan(sdc_create_clock.period)) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must define clock period.\n");
    }

    //Must have either a target (if a netlist clock), or a name (if a virtual clock) 
    if(sdc_create_clock.targets.strings.size() == 0 && sdc_create_clock.name.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must define either a target (for netlist clock) or a name (for virtual clock).\n");
    }

    //Currently we do not support defining clock names that differ from the netlist target name
    if(sdc_create_clock.targets.strings.size() != 0 && !sdc_create_clock.name.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Currently custom names for netlist clocks are unsupported, remove '-name' option or create a virtual clock.\n");
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
     * Set line number
     */
    sdc_create_clock.file_line_number = lexer.lineno();

    /*
     * Add command
     */
    sdc_commands->create_clock_cmds.push_back(sdc_create_clock);

    return sdc_commands;
}

/*
 * Functions for set_input_delay/set_output_delay
 */
void sdc_set_io_delay_set_clock(const Lexer& lexer, SetIoDelay& sdc_set_io_delay, const std::string& clock_name) {
    if(!sdc_set_io_delay.clock_name.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Can only specify a single clock\n"); 
    }

    sdc_set_io_delay.clock_name = clock_name;
}

void sdc_set_io_delay_set_max_value(const Lexer& lexer, SetIoDelay& sdc_set_io_delay, double max_value) {
    if(!std::isnan(sdc_set_io_delay.max_delay)) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Max delay value can only specified once.\n"); 
    }

    sdc_set_io_delay.max_delay = max_value;
}

void sdc_set_io_delay_set_ports(const Lexer& lexer, SetIoDelay& sdc_set_io_delay, StringGroup ports) {
    assert(ports.group_type == StringGroupType::PORT);

    if(!sdc_set_io_delay.target_ports.strings.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Currently only a single get_ports command is supported.\n"); 
    }

    sdc_set_io_delay.target_ports = ports;
}

std::shared_ptr<SdcCommands> add_sdc_set_io_delay(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, SetIoDelay& sdc_set_io_delay) {
    assert(sdc_commands);
    /*
     * Error checks
     */
    if(sdc_set_io_delay.clock_name.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify clock name.\n"); 
    }

    if(std::isnan(sdc_set_io_delay.max_delay)) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify max delay value.\n"); 
    }

    if(sdc_set_io_delay.target_ports.strings.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify target ports using get_ports.\n"); 
    }

    /*
     * Set line number
     */
    sdc_set_io_delay.file_line_number = lexer.lineno();

    /*
     * Add command
     */
    if(sdc_set_io_delay.io_type == IoDelayType::INPUT) {
        sdc_commands->set_input_delay_cmds.push_back(sdc_set_io_delay);
    } else {
        assert(sdc_set_io_delay.io_type == IoDelayType::OUTPUT);
        sdc_commands->set_output_delay_cmds.push_back(sdc_set_io_delay);
    }

    return sdc_commands;
}

/*
 * Functions for set_clock_groups
 */
void sdc_set_clock_groups_set_type(const Lexer& lexer, SetClockGroups& sdc_set_clock_groups, ClockGroupsType type) {
    if(sdc_set_clock_groups.cg_type != ClockGroupsType::NONE) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Can only specify a single clock groups relation type (e.g. '-exclusive')\n"); 
    }

    sdc_set_clock_groups.cg_type = type;
}

void sdc_set_clock_groups_add_group(const Lexer& /*lexer*/, SetClockGroups& sdc_set_clock_groups, StringGroup clock_group) {
    assert(clock_group.group_type == StringGroupType::CLOCK || clock_group.group_type == StringGroupType::STRING);

    //Duplicate and insert the clock group
    sdc_set_clock_groups.clock_groups.push_back(clock_group);
}

std::shared_ptr<SdcCommands> add_sdc_set_clock_groups(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, SetClockGroups& sdc_set_clock_groups) {
    /*
     * Error checks
     */
    if(sdc_set_clock_groups.cg_type == ClockGroupsType::NONE) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify clock relation type as '-exclusive'.\n"); 
    }

    if(sdc_set_clock_groups.clock_groups.size() < 2) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify at least 2 clock groups.\n"); 
    }

    /*
     * Set line number
     */
    sdc_set_clock_groups.file_line_number = lexer.lineno();

    /*
     * Add command
     */
    sdc_commands->set_clock_groups_cmds.push_back(sdc_set_clock_groups);

    return sdc_commands;
}

/*
 * Functions for set_false_path
 */
void sdc_set_false_path_add_to_from_group(const Lexer& lexer, 
                                                            SetFalsePath& sdc_set_false_path, 
                                                            StringGroup group, 
                                                            FromToType to_from_dir) {
    assert(group.group_type == StringGroupType::CLOCK || group.group_type == StringGroupType::STRING);

    //Error checking
    if(to_from_dir == FromToType::FROM) {
        //Check that we haven't already defined the from path    
        if(!sdc_set_false_path.from.strings.empty()) {
            sdc_error_wrap(lexer.lineno(), lexer.text(), "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == FromToType::TO);
        //Check that we haven't already defined the from path    
        if(!sdc_set_false_path.to.strings.empty()) {
            sdc_error_wrap(lexer.lineno(), lexer.text(), "Only a single '-to' option is supported.\n"); 
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

std::shared_ptr<SdcCommands> add_sdc_set_false_path(const Lexer& lexer, 
                                                    std::shared_ptr<SdcCommands> sdc_commands, 
                                                    SetFalsePath& sdc_set_false_path) {
    /*
     * Error checks
     */
    if(sdc_set_false_path.from.strings.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify source clock(s)/object(s) with the '-from' option.\n"); 
    }

    if(sdc_set_false_path.to.strings.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify target clock(s)/objects(s) with the '-to' option.\n"); 
    }

    /*
     * Set line number
     */
    sdc_set_false_path.file_line_number = lexer.lineno();

    /*
     * Add command
     */
    sdc_commands->set_false_path_cmds.push_back(sdc_set_false_path);

    return sdc_commands;
}

/*
 * Functions for set_max_delay
 */
void sdc_set_max_delay_set_max_delay_value(const Lexer& lexer, SetMaxDelay& sdc_set_max_delay, double max_delay) {
    if(!std::isnan(sdc_set_max_delay.max_delay)) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify max delay value only once.\n"); 
    }
    sdc_set_max_delay.max_delay = max_delay;
}

void sdc_set_max_delay_add_to_from_group(const Lexer& lexer, SetMaxDelay& sdc_set_max_delay, StringGroup group, FromToType to_from_dir) {
    assert(group.group_type == StringGroupType::CLOCK || group.group_type == StringGroupType::STRING);

    //Error checking
    if(to_from_dir == FromToType::FROM) {
        //Check that we haven't already defined the from path    
        if(!sdc_set_max_delay.from.strings.empty()) {
            sdc_error_wrap(lexer.lineno(), lexer.text(), "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == FromToType::TO);
        //Check that we haven't already defined the from path    
        if(!sdc_set_max_delay.to.strings.empty()) {
            sdc_error_wrap(lexer.lineno(), lexer.text(), "Only a single '-to' option is supported.\n"); 
        }
    }

    //Add the clock group
    if(to_from_dir == FromToType::FROM) {
        sdc_set_max_delay.from = group;
    } else {
        assert(to_from_dir == FromToType::TO);
        sdc_set_max_delay.to = group;
    }
}

std::shared_ptr<SdcCommands> add_sdc_set_max_delay(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, SetMaxDelay& sdc_set_max_delay) {
    /*
     * Error checks
     */
    if(std::isnan(sdc_set_max_delay.max_delay)) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify the max delay value.\n"); 
    }

    if(sdc_set_max_delay.from.strings.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify source clock(s) with the '-from' option.\n"); 
    }

    if(sdc_set_max_delay.to.strings.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify source clock(s) with the '-to' option.\n"); 
    }

    /*
     * Set line number
     */
    sdc_set_max_delay.file_line_number = lexer.lineno();

    /*
     * Add command
     */
    sdc_commands->set_max_delay_cmds.push_back(sdc_set_max_delay);

    return sdc_commands;
}

/*
 * Functions for set_multicycle_path
 */
void sdc_set_multicycle_path_set_type(const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path, McpType type) {
    if(sdc_set_multicycle_path.mcp_type != McpType::NONE) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify the type (e.g. '-setup') only once.\n"); 
    }
    sdc_set_multicycle_path.mcp_type = type;
}

void sdc_set_multicycle_path_set_mcp_value(const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path, int mcp_value) {
    if(sdc_set_multicycle_path.mcp_value != UNINITIALIZED_INT) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify multicycle path value only once.\n"); 
    }
    sdc_set_multicycle_path.mcp_value = mcp_value;
}

void sdc_set_multicycle_path_add_to_from_group(const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path, 
                                                                     StringGroup group, 
                                                                     FromToType to_from_dir) {
    assert(group.group_type == StringGroupType::CLOCK || group.group_type == StringGroupType::STRING);

    //Error checking
    if(to_from_dir == FromToType::FROM) {
        //Check that we haven't already defined the from path    
        if(!sdc_set_multicycle_path.from.strings.empty()) {
            sdc_error_wrap(lexer.lineno(), lexer.text(), "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == FromToType::TO);
        //Check that we haven't already defined the from path    
        if(!sdc_set_multicycle_path.to.strings.empty()) {
            sdc_error_wrap(lexer.lineno(), lexer.text(), "Only a single '-to' option is supported.\n"); 
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

std::shared_ptr<SdcCommands> add_sdc_set_multicycle_path(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, SetMulticyclePath& sdc_set_multicycle_path) {
    /*
     * Error checks
     */
    if(sdc_set_multicycle_path.mcp_type != McpType::SETUP) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify the multicycle path type as '-setup'.\n"); 
    }

    if(sdc_set_multicycle_path.mcp_value == UNINITIALIZED_INT) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify the multicycle path value.\n"); 
    }

    if(sdc_set_multicycle_path.from.strings.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify source clock(s) with the '-from' option.\n"); 
    }

    if(sdc_set_multicycle_path.to.strings.empty()) {
        sdc_error_wrap(lexer.lineno(), lexer.text(), "Must specify source clock(s) with the '-to' option.\n"); 
    }

    /*
     * Set line number
     */
    sdc_set_multicycle_path.file_line_number = lexer.lineno();

    /*
     * Add command
     */
    sdc_commands->set_multicycle_path_cmds.push_back(sdc_set_multicycle_path);

    return sdc_commands;
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

/*
 * Error reporting
 */

} //namespace
