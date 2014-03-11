#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "sdc.h"

void print_string_group(t_sdc_string_group* group) {
    const char *start_token, *end_token;
    if(group->group_type == SDC_STRING) {
        start_token = "{";
        end_token   = "}";

    } else if (group->group_type == SDC_CLOCK) {
        start_token = "[get_clocks {";
        end_token   = "}]";

    } else if (group->group_type == SDC_PORT) {
        start_token = "[get_ports {";
        end_token   = "}]";

    } else {
        printf("Unsupported sdc string group type\n");
        exit(1);
    }

    printf("%s", start_token);
    for(int i = 0; i < group->num_strings; i++) {
        printf("%s ", group->strings[i]);
    }
    printf("%s", end_token);
}

void print_from_to_group(t_sdc_string_group* from, t_sdc_string_group* to) {
    printf("-from ");
    print_string_group(from);
    printf(" -to ");
    print_string_group(to);
}

/*
 * Error handling
 */
#ifdef SDC_CUSTOM_ERROR_REPORT
void sdc_error(const int line_no, const char* near_text, const char* fmt, ...) {
    fprintf(stderr, "Custom SDC Error line %d near '%s': ", line_no, near_text);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}
#endif

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s filename.sdc\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "Reads in an SDC file into internal data structures\n");
        fprintf(stderr, "and then prints it out\n");
        exit(1);
    }

    //Parse the file
    t_sdc_commands* sdc_commands = sdc_parse_filename(argv[1]);



    //Print out the commands
    printf("File: %s\n", argv[1]);
    for(int i = 0; i < sdc_commands->num_create_clock_cmds; i++) {
        t_sdc_create_clock* sdc_create_clock = sdc_commands->create_clock_cmds[i];
        printf("Line %3d: ", sdc_create_clock->file_line_number);
        if(sdc_create_clock->is_virtual) {
            printf("create_clock -period %f -waveform {%f %f} -name %s\n",
                    sdc_create_clock->period,
                    sdc_create_clock->rise_edge,
                    sdc_create_clock->fall_edge,
                    sdc_create_clock->name);
        } else {
            printf("create_clock -period %f -waveform {%f %f} ",
                    sdc_create_clock->period,
                    sdc_create_clock->rise_edge,
                    sdc_create_clock->fall_edge);
            print_string_group(sdc_create_clock->targets);
            printf("\n");
        }
    }

    for(int i = 0; i < sdc_commands->num_set_input_delay_cmds; i++) {
        t_sdc_set_io_delay* sdc_set_input_delay = sdc_commands->set_input_delay_cmds[i];
        printf("Line %3d: ", sdc_set_input_delay->file_line_number);
        printf("set_input_delay -clock %s -max %f ", 
                    sdc_set_input_delay->clock_name,
                    sdc_set_input_delay->max_delay);
        print_string_group(sdc_set_input_delay->target_ports);
        printf("\n");
    }

    for(int i = 0; i < sdc_commands->num_set_output_delay_cmds; i++) {
        t_sdc_set_io_delay* sdc_set_output_delay = sdc_commands->set_output_delay_cmds[i];
        printf("Line %3d: ", sdc_set_output_delay->file_line_number);
        printf("set_output_delay -clock %s -max %f ", 
                    sdc_set_output_delay->clock_name,
                    sdc_set_output_delay->max_delay);
        print_string_group(sdc_set_output_delay->target_ports);
        printf("\n");
    }

    for(int i = 0; i < sdc_commands->num_set_clock_groups_cmds; i++) {
        t_sdc_set_clock_groups* sdc_set_clock_groups = sdc_commands->set_clock_groups_cmds[i];
        printf("Line %3d: ", sdc_set_clock_groups->file_line_number);
        printf("set_clock_groups ");
        if(sdc_set_clock_groups->type == SDC_CG_EXCLUSIVE) {
            printf(" -exclusive");
        }
        for(int j = 0; j < sdc_set_clock_groups->num_clock_groups; j++) {
            printf(" -group ");
            print_string_group(sdc_set_clock_groups->clock_groups[j]);
        }
        printf("\n");
    }

    for(int i = 0; i < sdc_commands->num_set_false_path_cmds; i++) {
        t_sdc_set_false_path* sdc_set_false_path = sdc_commands->set_false_path_cmds[i];
        printf("Line %3d: ", sdc_set_false_path->file_line_number);
        printf("set_false_path ");
        print_from_to_group(sdc_set_false_path->from, sdc_set_false_path->to);
        printf("\n");
    }

    for(int i = 0; i < sdc_commands->num_set_max_delay_cmds; i++) {
        t_sdc_set_max_delay* sdc_set_max_delay = sdc_commands->set_max_delay_cmds[i];
        printf("Line %3d: ", sdc_set_max_delay->file_line_number);
        printf("set_max_delay %f ", sdc_set_max_delay->max_delay);
        print_from_to_group(sdc_set_max_delay->from, sdc_set_max_delay->to);
        printf("\n");
    }

    for(int i = 0; i < sdc_commands->num_set_multicycle_path_cmds; i++) {
        t_sdc_set_multicycle_path* sdc_set_multicycle_path = sdc_commands->set_multicycle_path_cmds[i];
        printf("Line %3d: ", sdc_set_multicycle_path->file_line_number);
        printf("set_multicycle_path %d ", sdc_set_multicycle_path->mcp_value);
        if(sdc_set_multicycle_path->type == SDC_MCP_SETUP) {
            printf("-setup ");
        }
        print_from_to_group(sdc_set_multicycle_path->from, sdc_set_multicycle_path->to);
        printf("\n");
    }

    //Free sdc_parse related data, including g_sdc_commands
    sdc_parse_cleanup();

    return 0;
}
