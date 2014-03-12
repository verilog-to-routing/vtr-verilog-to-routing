#ifndef SDC_COMMON_H
#define SDC_COMMON_H

#include "sdc.h"

/*
 * Parser info
 */
extern int yylineno;
extern char* yytext;

extern t_sdc_commands* g_sdc_commands;

/*
 * Function Declarations
 */
//Error reporting
extern void sdc_error(const int line_number, const char* near_text, const char* fmt, ...);

//SDC Command List manipulation
t_sdc_commands* alloc_sdc_commands();
void free_sdc_commands(t_sdc_commands* sdc_commands);

//create_clock
t_sdc_create_clock* alloc_sdc_create_clock();
void free_sdc_create_clock(t_sdc_create_clock* sdc_create_clock);
t_sdc_create_clock* sdc_create_clock_set_period(t_sdc_create_clock* sdc_create_clock, double period);
t_sdc_create_clock* sdc_create_clock_set_name(t_sdc_create_clock* sdc_create_clock, char* name);
t_sdc_create_clock* sdc_create_clock_set_waveform(t_sdc_create_clock* sdc_create_clock, double rise_time, double fall_time);
t_sdc_create_clock* sdc_create_clock_add_targets(t_sdc_create_clock* sdc_create_clock, t_sdc_string_group* target_group);
t_sdc_commands* add_sdc_create_clock(t_sdc_commands* sdc_commands, t_sdc_create_clock* sdc_create_clock);

//set_input_delay & set_output_delay
t_sdc_set_io_delay* alloc_sdc_set_io_delay(t_sdc_io_delay_type cmd_type);
void free_sdc_set_io_delay(t_sdc_set_io_delay* sdc_set_io_delay);
t_sdc_set_io_delay* sdc_set_io_delay_set_clock(t_sdc_set_io_delay* sdc_set_io_delay, char* clock_name);
t_sdc_set_io_delay* sdc_set_io_delay_set_max_value(t_sdc_set_io_delay* sdc_set_io_delay, double max_value);
t_sdc_set_io_delay* sdc_set_io_delay_set_ports(t_sdc_set_io_delay* sdc_set_io_delay, t_sdc_string_group* ports);
t_sdc_commands* add_sdc_set_io_delay(t_sdc_commands* sdc_commands, t_sdc_set_io_delay* sdc_set_io_delay);

//set_clock_groups
t_sdc_set_clock_groups* alloc_sdc_set_clock_groups();
void free_sdc_set_clock_groups(t_sdc_set_clock_groups* sdc_set_clock_groups);
t_sdc_set_clock_groups* sdc_set_clock_groups_set_type(t_sdc_set_clock_groups* sdc_set_clock_groups, t_sdc_clock_groups_type type);
t_sdc_set_clock_groups* sdc_set_clock_groups_add_group(t_sdc_set_clock_groups* sdc_set_clock_groups, t_sdc_string_group* clock_group);
t_sdc_commands* add_sdc_set_clock_groups(t_sdc_commands* sdc_commands, t_sdc_set_clock_groups* sdc_set_clock_groups);

//set_false_path
t_sdc_set_false_path* alloc_sdc_set_false_path();
void free_sdc_set_false_path(t_sdc_set_false_path* sdc_set_false_path);
t_sdc_set_false_path* sdc_set_false_path_add_to_from_group(t_sdc_set_false_path* sdc_set_false_path, t_sdc_string_group* group, t_sdc_to_from_dir to_from_dir);
t_sdc_commands* add_sdc_set_false_path(t_sdc_commands* sdc_commands, t_sdc_set_false_path* sdc_set_false_path);

//set_max_delay
t_sdc_set_max_delay* alloc_sdc_set_max_delay();
void free_sdc_set_max_delay(t_sdc_set_max_delay* sdc_set_max_delay);
t_sdc_set_max_delay* sdc_set_max_delay_set_max_delay_value(t_sdc_set_max_delay* sdc_set_max_delay, double max_delay);
t_sdc_set_max_delay* sdc_set_max_delay_add_to_from_group(t_sdc_set_max_delay* sdc_set_max_delay, t_sdc_string_group* group, t_sdc_to_from_dir to_from_dir);
t_sdc_commands* add_sdc_set_max_delay(t_sdc_commands* sdc_commands, t_sdc_set_max_delay* sdc_set_max_delay);

//set_multicycle_path
t_sdc_set_multicycle_path* alloc_sdc_set_multicycle_path();
void free_sdc_set_multicycle_path(t_sdc_set_multicycle_path* sdc_set_multicycle_path);
t_sdc_set_multicycle_path* sdc_set_multicycle_path_set_type(t_sdc_set_multicycle_path* sdc_set_multicycle_path, t_sdc_mcp_type type);
t_sdc_set_multicycle_path* sdc_set_multicycle_path_set_mcp_value(t_sdc_set_multicycle_path* sdc_set_multicycle_path, int mcp_value);
t_sdc_set_multicycle_path* sdc_set_multicycle_path_add_to_from_group(t_sdc_set_multicycle_path* sdc_set_multicycle_path, t_sdc_string_group* group, t_sdc_to_from_dir to_from_dir);
t_sdc_commands* add_sdc_set_multicycle_path(t_sdc_commands* sdc_commands, t_sdc_set_multicycle_path* sdc_set_multicycle_path);


//string_group
t_sdc_string_group* alloc_sdc_string_group(t_sdc_string_group_type type);
t_sdc_string_group* make_sdc_string_group(t_sdc_string_group_type type, char* string);
t_sdc_string_group* duplicate_sdc_string_group(t_sdc_string_group* string_group);
void free_sdc_string_group(t_sdc_string_group* sdc_string_group);
t_sdc_string_group* sdc_string_group_add_string(t_sdc_string_group* sdc_string_group, char* string);
t_sdc_string_group* sdc_string_group_add_strings(t_sdc_string_group* sdc_string_group, t_sdc_string_group* string_group_to_add);

//utility
char* sdc_strdup(const char* src);
char* sdc_strndup(const char* src, size_t len);
#endif
