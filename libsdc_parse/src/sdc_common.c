#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "sdc_common.h"

#define UNINITIALIZED_FLOAT -1.0
#define UNINITIALIZED_INT -1

/*
 * Data structure to store the SDC Commands
 */
t_sdc_commands* g_sdc_commands;

/*
 * Functions for SDC Command List
 */
t_sdc_commands* alloc_sdc_commands() {

    //Alloc and initialize to empty
    t_sdc_commands* sdc_commands = (t_sdc_commands*) calloc(1, sizeof(t_sdc_commands));

    assert(sdc_commands != NULL);
    assert(sdc_commands->has_commands == false);

    return sdc_commands;
}

void free_sdc_commands(t_sdc_commands* sdc_commands) {
    if(sdc_commands != NULL) {
        for(int i = 0; i < sdc_commands->num_create_clock_cmds; i++) {
            free_sdc_create_clock(sdc_commands->create_clock_cmds[i]);
        }
        free(sdc_commands->create_clock_cmds);

        for(int i = 0; i < sdc_commands->num_set_input_delay_cmds; i++) {
            free_sdc_set_io_delay(sdc_commands->set_input_delay_cmds[i]);
        }
        free(sdc_commands->set_input_delay_cmds);

        for(int i = 0; i < sdc_commands->num_set_output_delay_cmds; i++) {
            free_sdc_set_io_delay(sdc_commands->set_output_delay_cmds[i]);
        }
        free(sdc_commands->set_output_delay_cmds);

        for(int i = 0; i < sdc_commands->num_set_clock_groups_cmds; i++) {
            free_sdc_set_clock_groups(sdc_commands->set_clock_groups_cmds[i]);
        }
        free(sdc_commands->set_clock_groups_cmds);

        for(int i = 0; i < sdc_commands->num_set_false_path_cmds; i++) {
            free_sdc_set_false_path(sdc_commands->set_false_path_cmds[i]);
        }
        free(sdc_commands->set_false_path_cmds);

        for(int i = 0; i < sdc_commands->num_set_max_delay_cmds; i++) {
            free_sdc_set_max_delay(sdc_commands->set_max_delay_cmds[i]);
        }
        free(sdc_commands->set_max_delay_cmds);

        for(int i = 0; i < sdc_commands->num_set_multicycle_path_cmds; i++) {
            free_sdc_set_multicycle_path(sdc_commands->set_multicycle_path_cmds[i]);
        }
        free(sdc_commands->set_multicycle_path_cmds);

        free(sdc_commands);
    }
}

/*
 * Functions for create_clock
 */
t_sdc_create_clock* alloc_sdc_create_clock() {
    t_sdc_create_clock* sdc_create_clock = (t_sdc_create_clock*) malloc(sizeof(t_sdc_create_clock));
    assert(sdc_create_clock != NULL);

    //Initialize
    sdc_create_clock->period = UNINITIALIZED_FLOAT;
    sdc_create_clock->name = NULL;
    sdc_create_clock->rise_edge = UNINITIALIZED_FLOAT;
    sdc_create_clock->fall_edge = UNINITIALIZED_FLOAT;
    sdc_create_clock->targets = NULL;
    sdc_create_clock->is_virtual = false;

    sdc_create_clock->file_line_number = UNINITIALIZED_INT;

    return sdc_create_clock;
}

void free_sdc_create_clock(t_sdc_create_clock* sdc_create_clock) {
    if(sdc_create_clock != NULL) {
        free(sdc_create_clock->name);
        free_sdc_string_group(sdc_create_clock->targets);
        free(sdc_create_clock);
    }
}

t_sdc_create_clock* sdc_create_clock_set_period(t_sdc_create_clock* sdc_create_clock, double period) {
    assert(sdc_create_clock != NULL);
    if(sdc_create_clock->period != UNINITIALIZED_FLOAT) {
        sdc_error(yylineno, yytext, "Can only define a single clock period.\n", yylineno, yytext); 
    } else {
        sdc_create_clock->period = period;
    }

    return sdc_create_clock;
}

t_sdc_create_clock* sdc_create_clock_set_name(t_sdc_create_clock* sdc_create_clock, char* name) {
    assert(sdc_create_clock != NULL);
    if(sdc_create_clock->name != NULL) {
        sdc_error(yylineno, yytext, "Can only define a single clock name.\n");
    } else {
        sdc_create_clock->name = sdc_strdup(name);
    }

    return sdc_create_clock;
}

t_sdc_create_clock* sdc_create_clock_set_waveform(t_sdc_create_clock* sdc_create_clock, double rise_edge, double fall_edge) {
    assert(sdc_create_clock != NULL);
    if(sdc_create_clock->rise_edge != UNINITIALIZED_FLOAT || sdc_create_clock->fall_edge != UNINITIALIZED_FLOAT) {
        sdc_error(yylineno, yytext, "Can only define a single waveform.\n"); 
    } else {
        sdc_create_clock->rise_edge = rise_edge;
        sdc_create_clock->fall_edge = fall_edge;
    }

    return sdc_create_clock;
}

t_sdc_create_clock* sdc_create_clock_add_targets(t_sdc_create_clock* sdc_create_clock, t_sdc_string_group* target_group) {
    assert(sdc_create_clock != NULL);

    assert(target_group->group_type == SDC_STRING);

    if(sdc_create_clock->targets != NULL) {
        sdc_error(yylineno, yytext, "Can only define a single set of targets for clock creation. "
                  "If you want to define multiple targets specify them as a list (e.g. \"{target1 target2}\".\n");
    }

    sdc_create_clock->targets = duplicate_sdc_string_group(target_group);

    return sdc_create_clock;
}

t_sdc_commands* add_sdc_create_clock(t_sdc_commands* sdc_commands, t_sdc_create_clock* sdc_create_clock) {
    /*
     * Error Checking
     */
    assert(sdc_commands != NULL);
    assert(sdc_create_clock != NULL);

    //Allocate a default (empty) target if none was defined, since this clock may be virtual
    if(sdc_create_clock->targets == NULL) {
        sdc_create_clock->targets = alloc_sdc_string_group(SDC_STRING);
        assert(sdc_create_clock->targets != NULL);
    }

    //Must have a clock period
    if(sdc_create_clock->period == UNINITIALIZED_FLOAT) {
        sdc_error(yylineno, yytext, "Must define clock period.\n");
    }

    //Must have either a target (if a netlist clock), or a name (if a virtual clock) 
    if(sdc_create_clock->targets->num_strings == 0 && sdc_create_clock->name == NULL) {
        sdc_error(yylineno, yytext, "Must define either a target (for netlist clock) or a name (for virtual clock).\n");
    }

    //Currently we do not support defining clock names that differ from the netlist target name
    if(sdc_create_clock->targets->num_strings != 0 && sdc_create_clock->name != NULL) {
        sdc_error(yylineno, yytext, "Currently custom names for netlist clocks are unsupported, remove '-name' option or create a virtual clock.\n");
    }

    /*
     * Set defaults
     */
    //Determine default rise/fall time for waveform
    if(sdc_create_clock->rise_edge == UNINITIALIZED_FLOAT && sdc_create_clock->fall_edge == UNINITIALIZED_FLOAT) {
        sdc_create_clock->rise_edge = 0.0;
        sdc_create_clock->fall_edge = sdc_create_clock->period / 2;
    }
    assert(sdc_create_clock->rise_edge != UNINITIALIZED_FLOAT);
    assert(sdc_create_clock->fall_edge != UNINITIALIZED_FLOAT);
    
    //Determine if clock is virtual or not
    if(sdc_create_clock->targets->num_strings == 0 && sdc_create_clock->name != NULL) {
        //Have a virtual target if there is a name AND no target strings
        sdc_create_clock->is_virtual = true;
    } else {
        assert(sdc_create_clock->targets->num_strings > 0);
        //Have a real target so this is not a virtual clock
        sdc_create_clock->is_virtual = false;
    }

    /*
     * Set line number
     */
    sdc_create_clock->file_line_number = yylineno;

    /*
     * Add command
     */
    sdc_commands->has_commands = true;
    sdc_commands->num_create_clock_cmds++;
    sdc_commands->create_clock_cmds = (t_sdc_create_clock**) realloc(sdc_commands->create_clock_cmds, sdc_commands->num_create_clock_cmds*sizeof(*sdc_commands->create_clock_cmds));
    sdc_commands->create_clock_cmds[sdc_commands->num_create_clock_cmds-1] = sdc_create_clock;

    return sdc_commands;
}

/*
 * Functions for set_input_delay/set_output_delay
 */
t_sdc_set_io_delay* alloc_sdc_set_io_delay(t_sdc_io_delay_type cmd_type) {
    //Allocate
    t_sdc_set_io_delay* sdc_set_io_delay = (t_sdc_set_io_delay*) malloc(sizeof(t_sdc_set_io_delay));
    assert(sdc_set_io_delay != NULL);

    //Initialize
    sdc_set_io_delay->cmd_type = cmd_type;
    sdc_set_io_delay->clock_name = NULL;
    sdc_set_io_delay->max_delay = UNINITIALIZED_FLOAT;
    sdc_set_io_delay->target_ports = NULL;

    sdc_set_io_delay->file_line_number = UNINITIALIZED_INT;

    return sdc_set_io_delay;
}

void free_sdc_set_io_delay(t_sdc_set_io_delay* sdc_set_io_delay) {
    if(sdc_set_io_delay != NULL) {
        free(sdc_set_io_delay->clock_name);
        free_sdc_string_group(sdc_set_io_delay->target_ports);
        free(sdc_set_io_delay);
    }
}

t_sdc_set_io_delay* sdc_set_io_delay_set_clock(t_sdc_set_io_delay* sdc_set_io_delay, char* clock_name) {
    assert(sdc_set_io_delay != NULL);

    if(sdc_set_io_delay->clock_name != NULL) {
        sdc_error(yylineno, yytext, "Can only specify a single clock\n"); 
    }

    sdc_set_io_delay->clock_name = sdc_strdup(clock_name);
    return sdc_set_io_delay;
}

t_sdc_set_io_delay* sdc_set_io_delay_set_max_value(t_sdc_set_io_delay* sdc_set_io_delay, double max_value) {
    assert(sdc_set_io_delay != NULL);

    if(sdc_set_io_delay->max_delay != UNINITIALIZED_FLOAT) {
        sdc_error(yylineno, yytext, "Max delay value can only specified once.\n"); 
    }

    sdc_set_io_delay->max_delay = max_value;
    return sdc_set_io_delay;
}

t_sdc_set_io_delay* sdc_set_io_delay_set_ports(t_sdc_set_io_delay* sdc_set_io_delay, t_sdc_string_group* ports) {
    assert(sdc_set_io_delay != NULL);
    assert(ports != NULL);
    assert(ports->group_type == SDC_PORT);

    if(sdc_set_io_delay->target_ports != NULL) {
        sdc_error(yylineno, yytext, "Currently only a single get_ports command is supported.\n"); 
    }

    sdc_set_io_delay->target_ports = duplicate_sdc_string_group(ports);
    return sdc_set_io_delay;
}

t_sdc_commands* add_sdc_set_io_delay(t_sdc_commands* sdc_commands, t_sdc_set_io_delay* sdc_set_io_delay) {
    assert(sdc_commands != NULL);
    assert(sdc_set_io_delay != NULL);
    /*
     * Error checks
     */
    if(sdc_set_io_delay->clock_name == NULL) {
        sdc_error(yylineno, yytext, "Must specify clock name.\n"); 
    }

    if(sdc_set_io_delay->max_delay == UNINITIALIZED_FLOAT) {
        sdc_error(yylineno, yytext, "Must specify max delay value.\n"); 
    }

    if(sdc_set_io_delay->target_ports == NULL) {
        sdc_error(yylineno, yytext, "Must specify target ports using get_ports.\n"); 
    }

    /*
     * Set line number
     */
    sdc_set_io_delay->file_line_number = yylineno;

    /*
     * Add command
     */
    if(sdc_set_io_delay->cmd_type == SDC_INPUT_DELAY) {
        sdc_commands->has_commands = true;
        sdc_commands->num_set_input_delay_cmds++;
        sdc_commands->set_input_delay_cmds = (t_sdc_set_io_delay**) realloc(sdc_commands->set_input_delay_cmds, sdc_commands->num_set_input_delay_cmds*sizeof(*sdc_commands->set_input_delay_cmds));
        sdc_commands->set_input_delay_cmds[sdc_commands->num_set_input_delay_cmds-1] = sdc_set_io_delay;
    } else {
        assert(sdc_set_io_delay->cmd_type == SDC_OUTPUT_DELAY);
        sdc_commands->has_commands = true;
        sdc_commands->num_set_output_delay_cmds++;
        sdc_commands->set_output_delay_cmds = (t_sdc_set_io_delay**) realloc(sdc_commands->set_output_delay_cmds, sdc_commands->num_set_output_delay_cmds*sizeof(*sdc_commands->set_output_delay_cmds));
        sdc_commands->set_output_delay_cmds[sdc_commands->num_set_output_delay_cmds-1] = sdc_set_io_delay;
    }

    return sdc_commands;
}

/*
 * Functions for set_clock_groups
 */
t_sdc_set_clock_groups* alloc_sdc_set_clock_groups() {
    //Allocate
    t_sdc_set_clock_groups* sdc_set_clock_groups = (t_sdc_set_clock_groups*) malloc(sizeof(t_sdc_set_clock_groups));
    assert(sdc_set_clock_groups != NULL);

    //Initialize
    sdc_set_clock_groups->type = SDC_CG_NONE;
    sdc_set_clock_groups->num_clock_groups = 0;
    sdc_set_clock_groups->clock_groups = NULL;

    sdc_set_clock_groups->file_line_number = UNINITIALIZED_INT;

    return sdc_set_clock_groups;
}

void free_sdc_set_clock_groups(t_sdc_set_clock_groups* sdc_set_clock_groups) {
    if(sdc_set_clock_groups != NULL) {
        for(int i = 0; i < sdc_set_clock_groups->num_clock_groups; i++) {
            free_sdc_string_group(sdc_set_clock_groups->clock_groups[i]);
        }
        free(sdc_set_clock_groups->clock_groups);
        free(sdc_set_clock_groups);
    }
}

t_sdc_set_clock_groups* sdc_set_clock_groups_set_type(t_sdc_set_clock_groups* sdc_set_clock_groups, t_sdc_clock_groups_type type) {
    assert(sdc_set_clock_groups != NULL);

    if(sdc_set_clock_groups->type != SDC_CG_NONE) {
        sdc_error(yylineno, yytext, "Can only specify a single clock groups relation type (e.g. '-exclusive')\n"); 
    }

    sdc_set_clock_groups->type = type;
    return sdc_set_clock_groups;
}

t_sdc_set_clock_groups* sdc_set_clock_groups_add_group(t_sdc_set_clock_groups* sdc_set_clock_groups, t_sdc_string_group* clock_group) {
    assert(sdc_set_clock_groups != NULL);
    assert(clock_group != NULL);

    assert(clock_group->group_type == SDC_CLOCK || clock_group->group_type == SDC_STRING);

    //Allocate space
    sdc_set_clock_groups->num_clock_groups++;
    sdc_set_clock_groups->clock_groups = (t_sdc_string_group**) realloc(sdc_set_clock_groups->clock_groups, sdc_set_clock_groups->num_clock_groups*sizeof(*sdc_set_clock_groups->clock_groups));
    assert(sdc_set_clock_groups->clock_groups != NULL);

    //Duplicate and insert the clock group
    sdc_set_clock_groups->clock_groups[sdc_set_clock_groups->num_clock_groups-1] = duplicate_sdc_string_group(clock_group);

    return sdc_set_clock_groups;
}

t_sdc_commands* add_sdc_set_clock_groups(t_sdc_commands* sdc_commands, t_sdc_set_clock_groups* sdc_set_clock_groups) {
    /*
     * Error checks
     */
    if(sdc_set_clock_groups->type == SDC_CG_NONE) {
        sdc_error(yylineno, yytext, "Must specify clock relation type as '-exclusive'.\n"); 
    }

    if(sdc_set_clock_groups->num_clock_groups < 2) {
        sdc_error(yylineno, yytext, "Must specify at least 2 clock groups.\n"); 
    }

    /*
     * Set line number
     */
    sdc_set_clock_groups->file_line_number = yylineno;

    /*
     * Add command
     */
    sdc_commands->has_commands = true;
    sdc_commands->num_set_clock_groups_cmds++;
    sdc_commands->set_clock_groups_cmds = (t_sdc_set_clock_groups**) realloc(sdc_commands->set_clock_groups_cmds, sdc_commands->num_set_clock_groups_cmds*sizeof(*sdc_commands->set_clock_groups_cmds));
    sdc_commands->set_clock_groups_cmds[sdc_commands->num_set_clock_groups_cmds-1] = sdc_set_clock_groups;

    return sdc_commands;
}

/*
 * Functions for set_false_path
 */
t_sdc_set_false_path* alloc_sdc_set_false_path() {
    //Allocate
    t_sdc_set_false_path* sdc_set_false_path = (t_sdc_set_false_path*) malloc(sizeof(t_sdc_set_false_path));

    //Initialize
    sdc_set_false_path->from = NULL;
    sdc_set_false_path->to = NULL;

    sdc_set_false_path->file_line_number = UNINITIALIZED_INT;

    return sdc_set_false_path;
}

void free_sdc_set_false_path(t_sdc_set_false_path* sdc_set_false_path) {
    if(sdc_set_false_path != NULL) {
        free_sdc_string_group(sdc_set_false_path->from);
        free_sdc_string_group(sdc_set_false_path->to);

        free(sdc_set_false_path);
    }
}

t_sdc_set_false_path* sdc_set_false_path_add_to_from_group(t_sdc_set_false_path* sdc_set_false_path, 
                                                           t_sdc_string_group* group, 
                                                           t_sdc_to_from_dir to_from_dir) {
    assert(sdc_set_false_path != NULL);
    assert(group != NULL);
    assert(group->group_type == SDC_CLOCK || group->group_type == SDC_STRING);

    //Error checking
    if(to_from_dir == SDC_FROM) {
        //Check that we haven't already defined the from path    
        if(sdc_set_false_path->from != NULL) {
            sdc_error(yylineno, yytext, "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == SDC_TO);
        //Check that we haven't already defined the from path    
        if(sdc_set_false_path->to != NULL) {
            sdc_error(yylineno, yytext, "Only a single '-to' option is supported.\n"); 
        }
    }

    //Add the clock group
    if(to_from_dir == SDC_FROM) {
        sdc_set_false_path->from = duplicate_sdc_string_group(group);
    } else {
        assert(to_from_dir == SDC_TO);
        sdc_set_false_path->to = duplicate_sdc_string_group(group);
    }

    return sdc_set_false_path;
}

t_sdc_commands* add_sdc_set_false_path(t_sdc_commands* sdc_commands, t_sdc_set_false_path* sdc_set_false_path) {
    /*
     * Error checks
     */
    if(sdc_set_false_path->from == NULL) {
        sdc_error(yylineno, yytext, "Must specify source clock(s)/object(s) with the '-from' option.\n"); 
    }

    if(sdc_set_false_path->to == NULL) {
        sdc_error(yylineno, yytext, "Must specify target clock(s)/objects(s) with the '-to' option.\n"); 
    }

    /*
     * Set line number
     */
    sdc_set_false_path->file_line_number = yylineno;

    /*
     * Add command
     */
    sdc_commands->has_commands = true;
    sdc_commands->num_set_false_path_cmds++;
    sdc_commands->set_false_path_cmds = (t_sdc_set_false_path**) realloc(sdc_commands->set_false_path_cmds, sdc_commands->num_set_false_path_cmds*sizeof(*sdc_commands->set_false_path_cmds));
    sdc_commands->set_false_path_cmds[sdc_commands->num_set_false_path_cmds-1] = sdc_set_false_path;

    return sdc_commands;
}

/*
 * Functions for set_max_delay
 */
t_sdc_set_max_delay* alloc_sdc_set_max_delay() {
    //Allocate
    t_sdc_set_max_delay* sdc_set_max_delay = (t_sdc_set_max_delay*) malloc(sizeof(t_sdc_set_max_delay));

    //Initialize
    sdc_set_max_delay->max_delay = UNINITIALIZED_FLOAT;
    sdc_set_max_delay->from = NULL;
    sdc_set_max_delay->to = NULL;

    sdc_set_max_delay->file_line_number = UNINITIALIZED_INT;

    return sdc_set_max_delay;
}

void free_sdc_set_max_delay(t_sdc_set_max_delay* sdc_set_max_delay) {
    if(sdc_set_max_delay != NULL) {
        free_sdc_string_group(sdc_set_max_delay->from);
        free_sdc_string_group(sdc_set_max_delay->to);
        free(sdc_set_max_delay);
    }
}

t_sdc_set_max_delay* sdc_set_max_delay_set_max_delay_value(t_sdc_set_max_delay* sdc_set_max_delay, double max_delay) {
    if(sdc_set_max_delay->max_delay != UNINITIALIZED_FLOAT) {
        sdc_error(yylineno, yytext, "Must specify max delay value only once.\n"); 
    }
    sdc_set_max_delay->max_delay = max_delay;
    return sdc_set_max_delay;
}

t_sdc_set_max_delay* sdc_set_max_delay_add_to_from_group(t_sdc_set_max_delay* sdc_set_max_delay, t_sdc_string_group* group, t_sdc_to_from_dir to_from_dir) {
    assert(sdc_set_max_delay != NULL);
    assert(group != NULL);
    assert(group->group_type == SDC_CLOCK || group->group_type == SDC_STRING);

    //Error checking
    if(to_from_dir == SDC_FROM) {
        //Check that we haven't already defined the from path    
        if(sdc_set_max_delay->from != NULL) {
            sdc_error(yylineno, yytext, "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == SDC_TO);
        //Check that we haven't already defined the from path    
        if(sdc_set_max_delay->to != NULL) {
            sdc_error(yylineno, yytext, "Only a single '-to' option is supported.\n"); 
        }
    }

    //Add the clock group
    if(to_from_dir == SDC_FROM) {
        sdc_set_max_delay->from = duplicate_sdc_string_group(group);
    } else {
        assert(to_from_dir == SDC_TO);
        sdc_set_max_delay->to = duplicate_sdc_string_group(group);
    }

    return sdc_set_max_delay;
}

t_sdc_commands* add_sdc_set_max_delay(t_sdc_commands* sdc_commands, t_sdc_set_max_delay* sdc_set_max_delay) {
    /*
     * Error checks
     */
    if(sdc_set_max_delay->max_delay == UNINITIALIZED_FLOAT) {
        sdc_error(yylineno, yytext, "Must specify the max delay value.\n"); 
    }

    if(sdc_set_max_delay->from == NULL) {
        sdc_error(yylineno, yytext, "Must specify source clock(s) with the '-from' option.\n"); 
    }

    if(sdc_set_max_delay->to == NULL) {
        sdc_error(yylineno, yytext, "Must specify source clock(s) with the '-to' option.\n"); 
    }

    /*
     * Set line number
     */
    sdc_set_max_delay->file_line_number = yylineno;

    /*
     * Add command
     */
    sdc_commands->has_commands = true;
    sdc_commands->num_set_max_delay_cmds++;
    sdc_commands->set_max_delay_cmds = (t_sdc_set_max_delay**) realloc(sdc_commands->set_max_delay_cmds, sdc_commands->num_set_max_delay_cmds*sizeof(*sdc_commands->set_max_delay_cmds));
    sdc_commands->set_max_delay_cmds[sdc_commands->num_set_max_delay_cmds-1] = sdc_set_max_delay;

    return sdc_commands;

}
/*
 * Functions for set_multicycle_path
 */
t_sdc_set_multicycle_path* alloc_sdc_set_multicycle_path() {
    //Allocate
    t_sdc_set_multicycle_path* sdc_set_multicycle_path = (t_sdc_set_multicycle_path*) malloc(sizeof(t_sdc_set_multicycle_path));
    assert(sdc_set_multicycle_path != NULL);

    //Initialize
    sdc_set_multicycle_path->type = SDC_MCP_NONE;
    sdc_set_multicycle_path->mcp_value = UNINITIALIZED_INT;
    sdc_set_multicycle_path->from = NULL;
    sdc_set_multicycle_path->to = NULL;

    sdc_set_multicycle_path->file_line_number = UNINITIALIZED_INT;

    return sdc_set_multicycle_path;

}
void free_sdc_set_multicycle_path(t_sdc_set_multicycle_path* sdc_set_multicycle_path) {
    if(sdc_set_multicycle_path != NULL) {
        free_sdc_string_group(sdc_set_multicycle_path->from);
        free_sdc_string_group(sdc_set_multicycle_path->to);
        free(sdc_set_multicycle_path);
    }
}

t_sdc_set_multicycle_path* sdc_set_multicycle_path_set_type(t_sdc_set_multicycle_path* sdc_set_multicycle_path, t_sdc_mcp_type type) {
    if(sdc_set_multicycle_path->type != SDC_MCP_NONE) {
        sdc_error(yylineno, yytext, "Must specify the type (e.g. '-setup') only once.\n"); 
    }
    sdc_set_multicycle_path->type = type;
    return sdc_set_multicycle_path;
}

t_sdc_set_multicycle_path* sdc_set_multicycle_path_set_mcp_value(t_sdc_set_multicycle_path* sdc_set_multicycle_path, int mcp_value) {
    if(sdc_set_multicycle_path->mcp_value != UNINITIALIZED_INT) {
        sdc_error(yylineno, yytext, "Must specify multicycle path value only once.\n"); 
    }
    sdc_set_multicycle_path->mcp_value = mcp_value;
    return sdc_set_multicycle_path;
}

t_sdc_set_multicycle_path* sdc_set_multicycle_path_add_to_from_group(t_sdc_set_multicycle_path* sdc_set_multicycle_path, 
                                                                     t_sdc_string_group* group, 
                                                                     t_sdc_to_from_dir to_from_dir) {
    assert(sdc_set_multicycle_path != NULL);
    assert(group != NULL);
    assert(group->group_type == SDC_CLOCK || group->group_type == SDC_STRING);

    //Error checking
    if(to_from_dir == SDC_FROM) {
        //Check that we haven't already defined the from path    
        if(sdc_set_multicycle_path->from != NULL) {
            sdc_error(yylineno, yytext, "Only a single '-from' option is supported.\n"); 
        }
    } else {
        assert(to_from_dir == SDC_TO);
        //Check that we haven't already defined the from path    
        if(sdc_set_multicycle_path->to != NULL) {
            sdc_error(yylineno, yytext, "Only a single '-to' option is supported.\n"); 
        }
    }

    //Add the clock group
    if(to_from_dir == SDC_FROM) {
        sdc_set_multicycle_path->from = duplicate_sdc_string_group(group);
    } else {
        assert(to_from_dir == SDC_TO);
        sdc_set_multicycle_path->to = duplicate_sdc_string_group(group);
    }

    return sdc_set_multicycle_path;
}

t_sdc_commands* add_sdc_set_multicycle_path(t_sdc_commands* sdc_commands, t_sdc_set_multicycle_path* sdc_set_multicycle_path) {
    /*
     * Error checks
     */
    if(sdc_set_multicycle_path->type != SDC_MCP_SETUP) {
        sdc_error(yylineno, yytext, "Must specify the multicycle path type as '-setup'.\n"); 
    }

    if(sdc_set_multicycle_path->mcp_value == UNINITIALIZED_FLOAT) {
        sdc_error(yylineno, yytext, "Must specify the multicycle path value.\n"); 
    }

    if(sdc_set_multicycle_path->from == NULL) {
        sdc_error(yylineno, yytext, "Must specify source clock(s) with the '-from' option.\n"); 
    }

    if(sdc_set_multicycle_path->to == NULL) {
        sdc_error(yylineno, yytext, "Must specify source clock(s) with the '-to' option.\n"); 
    }

    /*
     * Set line number
     */
    sdc_set_multicycle_path->file_line_number = yylineno;

    /*
     * Add command
     */
    sdc_commands->has_commands = true;
    sdc_commands->num_set_multicycle_path_cmds++;
    sdc_commands->set_multicycle_path_cmds = (t_sdc_set_multicycle_path**) realloc(sdc_commands->set_multicycle_path_cmds, sdc_commands->num_set_multicycle_path_cmds*sizeof(*sdc_commands->set_multicycle_path_cmds));
    sdc_commands->set_multicycle_path_cmds[sdc_commands->num_set_multicycle_path_cmds-1] = sdc_set_multicycle_path;

    return sdc_commands;
}

/*
 * Functions for string_group
 */
t_sdc_string_group* alloc_sdc_string_group(t_sdc_string_group_type type) {
    //Allocate and initialize
    t_sdc_string_group* sdc_string_group = (t_sdc_string_group*) calloc(1, sizeof(t_sdc_string_group)); 
    assert(sdc_string_group != NULL);

    sdc_string_group->group_type = type;

    return sdc_string_group;
}

t_sdc_string_group* make_sdc_string_group(t_sdc_string_group_type type, char* string) {
    //Convenience function for converting a single string into a group
    t_sdc_string_group* sdc_string_group = alloc_sdc_string_group(type);

    sdc_string_group_add_string(sdc_string_group, string);

    return sdc_string_group;
}

t_sdc_string_group* duplicate_sdc_string_group(t_sdc_string_group* string_group) {
    //Allocate
    t_sdc_string_group* new_sdc_string_group = (t_sdc_string_group*) malloc(sizeof(t_sdc_string_group));
    assert(new_sdc_string_group != NULL);

    //Deep Copy
    new_sdc_string_group->num_strings = string_group->num_strings;
    new_sdc_string_group->group_type = string_group->group_type;

    new_sdc_string_group->strings = (char**) calloc(new_sdc_string_group->num_strings, sizeof(*new_sdc_string_group->strings));
    assert(new_sdc_string_group->strings != NULL);
    for(int i = 0; i < new_sdc_string_group->num_strings; i++) {
        new_sdc_string_group->strings[i] = sdc_strdup(string_group->strings[i]);
    }

    return new_sdc_string_group;
}

void free_sdc_string_group(t_sdc_string_group* sdc_string_group) {
    if(sdc_string_group != NULL) {

        for(int i = 0; i < sdc_string_group->num_strings; i++) {
            free(sdc_string_group->strings[i]);
        }
        free(sdc_string_group->strings);
        free(sdc_string_group);
    }
}

t_sdc_string_group* sdc_string_group_add_string(t_sdc_string_group* sdc_string_group, char* string) {
    assert(sdc_string_group != NULL);

    //Allocate space
    sdc_string_group->num_strings++;
    sdc_string_group->strings = (char**) realloc(sdc_string_group->strings, sdc_string_group->num_strings*sizeof(*sdc_string_group->strings));
    assert(sdc_string_group->strings != NULL);

    //Insert the new string
    sdc_string_group->strings[sdc_string_group->num_strings-1] = sdc_strdup(string);
    
    return sdc_string_group;
}

t_sdc_string_group* sdc_string_group_add_strings(t_sdc_string_group* sdc_string_group, t_sdc_string_group* string_group_to_add) {
    for(int i = 0; i < string_group_to_add->num_strings; i++) {
        sdc_string_group_add_string(sdc_string_group, string_group_to_add->strings[i]);
    }
    return sdc_string_group;
}

char* sdc_strdup(const char* src) {
    if(src == NULL) {
        return NULL;
    }

    size_t len = strlen(src); //Number of char's excluding null terminator

    //Space for [0..len] chars
    char* new_str = (char*) malloc((len+1)*sizeof(*src));
    assert(new_str != NULL);

    //Copy chars from [0..len], i.e. src[len] should be null terminator
    memcpy(new_str, src, len+1);
    
    return new_str;
}

char* sdc_strndup(const char* src, size_t len) {
    if(src == NULL) {
        return NULL;
    }

    //Space for [0..len] chars
    char* new_str = (char*) malloc((len+1)*sizeof(*src));
    assert(new_str != NULL);

    //Copy chars from [0..len-1]
    memcpy(new_str, src, len);
    
    //Add the null terminator
    new_str[len] = '\0';

    return new_str;
}

/*
 * Error reporting
 */
#ifndef SDC_CUSTOM_ERROR_REPORT
void sdc_error(const int line_no, const char* near_text, const char* fmt, ...) {
    fprintf(stderr, "SDC Error line %d near '%s': ", line_no, near_text);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}
#endif
