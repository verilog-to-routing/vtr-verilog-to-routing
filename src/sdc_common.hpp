#ifndef SDC_COMMON_HPP
#define SDC_COMMON_HPP

#include "sdc.hpp"
#include "sdc_lexer.hpp"

/*
 * Parser info
 */
extern int sdcparse_lineno;
extern char* sdcparse_text;

namespace sdcparse {

/*
 * Function Declarations
 */

//create_clock
void sdc_create_clock_set_period(const Lexer& lexer, CreateClock& sdc_create_clock, double period);
void sdc_create_clock_set_name(const Lexer& lexer, CreateClock& sdc_create_clock, const std::string& name);
void sdc_create_clock_set_waveform(const Lexer& lexer, CreateClock& sdc_create_clock, double rise_time, double fall_time);
void sdc_create_clock_add_targets(const Lexer& lexer, CreateClock& sdc_create_clock, StringGroup target_group);
std::shared_ptr<SdcCommands> add_sdc_create_clock(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, CreateClock& sdc_create_clock);

//set_input_delay & set_output_delay
void sdc_set_io_delay_set_clock(const Lexer& lexer, SetIoDelay& sdc_set_io_delay, const std::string& clock_name);
void sdc_set_io_delay_set_max_value(const Lexer& lexer, SetIoDelay& sdc_set_io_delay, double max_value);
void sdc_set_io_delay_set_ports(const Lexer& lexer, SetIoDelay& sdc_set_io_delay, StringGroup ports);
std::shared_ptr<SdcCommands> add_sdc_set_io_delay(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, SetIoDelay& sdc_set_io_delay);

//set_clock_groups
void sdc_set_clock_groups_set_type(const Lexer& lexer, SetClockGroups& sdc_set_clock_groups, ClockGroupsType type);
void sdc_set_clock_groups_add_group(const Lexer& lexer, SetClockGroups& sdc_set_clock_groups, StringGroup clock_group);
std::shared_ptr<SdcCommands> add_sdc_set_clock_groups(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, SetClockGroups& sdc_set_clock_groups);

//set_false_path
void sdc_set_false_path_add_to_from_group(const Lexer& lexer, SetFalsePath& sdc_set_false_path, StringGroup group, FromToType to_from_dir);
std::shared_ptr<SdcCommands> add_sdc_set_false_path(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, SetFalsePath& sdc_set_false_path);

//set_max_delay
void sdc_set_max_delay_set_max_delay_value(const Lexer& lexer, SetMaxDelay& sdc_set_max_delay, double max_delay);
void sdc_set_max_delay_add_to_from_group(const Lexer& lexer, SetMaxDelay& sdc_set_max_delay, StringGroup group, FromToType to_from_dir);
std::shared_ptr<SdcCommands> add_sdc_set_max_delay(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, SetMaxDelay& sdc_set_max_delay);

//set_multicycle_path
void sdc_set_multicycle_path_set_type(const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path, McpType type);
void sdc_set_multicycle_path_set_mcp_value(const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path, int mcp_value);
void sdc_set_multicycle_path_add_to_from_group(const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path, StringGroup group, FromToType to_from_dir);
std::shared_ptr<SdcCommands> add_sdc_set_multicycle_path(const Lexer& lexer, std::shared_ptr<SdcCommands> sdc_commands, SetMulticyclePath& sdc_set_multicycle_path);


//string_group
StringGroup make_sdc_string_group(StringGroupType type, const std::string& string);
void sdc_string_group_add_string(StringGroup& sdc_string_group, const std::string& string);
void sdc_string_group_add_strings(StringGroup& sdc_string_group, const StringGroup& string_group_to_add);

//utility
char* strdup(const char* src);
char* strndup(const char* src, size_t len);

} //namespace
#endif
