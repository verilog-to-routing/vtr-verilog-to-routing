#ifndef SDC_COMMON_HPP
#define SDC_COMMON_HPP

#include "sdcparse.hpp"
#include "sdc_lexer.hpp"

namespace sdcparse {

enum class TimingDerateTargetType {
    NET,
    CELL
};

/*
 * Function Declarations
 */

//create_clock
void sdc_create_clock_set_period(Callback& callback, const Lexer& lexer, CreateClock& sdc_create_clock, double period);
void sdc_create_clock_set_name(Callback& callback, const Lexer& lexer, CreateClock& sdc_create_clock, const std::string& name);
void sdc_create_clock_set_waveform(Callback& callback, const Lexer& lexer, CreateClock& sdc_create_clock, double rise_time, double fall_time);
void sdc_create_clock_add_targets(Callback& callback, const Lexer& lexer, CreateClock& sdc_create_clock, StringGroup target_group);
void add_sdc_create_clock(Callback& callback, const Lexer& lexer, CreateClock& sdc_create_clock);

//set_input_delay & set_output_delay
void sdc_set_io_delay_set_clock(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay, const std::string& clock_name);
void sdc_set_io_delay_set_value(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay, double value);
void sdc_set_io_delay_set_max(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay);
void sdc_set_io_delay_set_min(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay);
void sdc_set_io_delay_set_ports(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay, StringGroup ports);
void add_sdc_set_io_delay(Callback& callback, const Lexer& lexer, SetIoDelay& sdc_set_io_delay);

//set_clock_groups
void sdc_set_clock_groups_set_type(Callback& callback, const Lexer& lexer, SetClockGroups& sdc_set_clock_groups, ClockGroupsType type);
void sdc_set_clock_groups_add_group(Callback& callback, const Lexer& lexer, SetClockGroups& sdc_set_clock_groups, StringGroup clock_group);
void add_sdc_set_clock_groups(Callback& callback, const Lexer& lexer, SetClockGroups& sdc_set_clock_groups);

//set_false_path
void sdc_set_false_path_add_to_from_group(Callback& callback, const Lexer& lexer, SetFalsePath& sdc_set_false_path, StringGroup group, FromToType to_from_dir);
void add_sdc_set_false_path(Callback& callback, const Lexer& lexer, SetFalsePath& sdc_set_false_path);

//set_max_delay
void sdc_set_min_max_delay_set_type(Callback& callback, const Lexer& lexer, SetMinMaxDelay& sdc_set_max_delay, MinMaxType type);
void sdc_set_min_max_delay_set_value(Callback& callback, const Lexer& lexer, SetMinMaxDelay& sdc_set_max_delay, double max_delay);
void sdc_set_min_max_delay_add_to_from_group(Callback& callback, const Lexer& lexer, SetMinMaxDelay& sdc_set_max_delay, StringGroup group, FromToType to_from_dir);
void add_sdc_set_min_max_delay(Callback& callback, const Lexer& lexer, SetMinMaxDelay& sdc_set_max_delay);

//set_multicycle_path
void sdc_set_multicycle_path_set_setup(Callback& callback, const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path);
void sdc_set_multicycle_path_set_hold(Callback& callback, const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path);
void sdc_set_multicycle_path_set_mcp_value(Callback& callback, const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path, int mcp_value);
void sdc_set_multicycle_path_add_to_from_group(Callback& callback, const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path, StringGroup group, FromToType to_from_dir);
void add_sdc_set_multicycle_path(Callback& callback, const Lexer& lexer, SetMulticyclePath& sdc_set_multicycle_path);

//set_clock_uncertainty
void sdc_set_clock_uncertainty_set_setup(Callback& callback, const Lexer& lexer, SetClockUncertainty& sdc_set_clock_uncertainty);
void sdc_set_clock_uncertainty_set_hold(Callback& callback, const Lexer& lexer, SetClockUncertainty& sdc_set_clock_uncertainty);
void sdc_set_clock_uncertainty_set_value(Callback& callback, const Lexer& lexer, SetClockUncertainty& sdc_set_clock_uncertainty, float value);
void sdc_set_clock_uncertainty_add_to_from_group(Callback& callback, const Lexer& lexer, SetClockUncertainty& sdc_set_clock_uncertainty, StringGroup group, FromToType to_from_dir);
void add_sdc_set_clock_uncertainty(Callback& callback, const Lexer& lexer, SetClockUncertainty& sdc_set_clock_uncertainty);

//set_clock_latency
void sdc_set_clock_latency_set_type(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency, ClockLatencyType type);
void sdc_set_clock_latency_early(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency);
void sdc_set_clock_latency_late(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency);
void sdc_set_clock_latency_set_value(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency, float value);
void sdc_set_clock_latency_set_clocks(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency, StringGroup clock_targets);
void add_sdc_set_clock_latency(Callback& callback, const Lexer& lexer, SetClockLatency& sdc_set_clock_latency);

//set_disable_timing
void sdc_set_disable_timing_add_to_from_group(Callback& callback, const Lexer& lexer, SetDisableTiming& sdc_set_disable_timing, StringGroup group, FromToType to_from_dir);
void add_sdc_set_disable_timing(Callback& callback, const Lexer& lexer, SetDisableTiming& sdc_set_disable_timing);

//set_timing_derate
void sdc_set_timing_derate_early(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate);
void sdc_set_timing_derate_late(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate);
void sdc_set_timing_derate_target_type(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate, TimingDerateTargetType target_type);
void sdc_set_timing_derate_value(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate, float value);
void sdc_set_timing_derate_targets(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_timing_derate, StringGroup targets);
void add_sdc_set_timing_derate(Callback& callback, const Lexer& lexer, SetTimingDerate& sdc_set_multicycle_path);

//string_group
StringGroup make_sdc_string_group(StringGroupType type, const std::string& string);
void sdc_string_group_add_string(StringGroup& sdc_string_group, const std::string& string);
void sdc_string_group_add_strings(StringGroup& sdc_string_group, const StringGroup& string_group_to_add);

//utility
char* strdup(const char* src);
char* strndup(const char* src, size_t len);

} //namespace
#endif
