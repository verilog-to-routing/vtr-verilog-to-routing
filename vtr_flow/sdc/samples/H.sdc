create_clock -period 0 -name whatever 

set_input_delay -clock whatever 0 [get_ports{*}] 

set_output_delay -clock whatever 0 [get_ports{*}]

set_min_delay 0 -from [get_clocks{*}] -to [get_clocks{*}]

set_max_delay 0 -from [get_clocks{*}] -to [get_clocks{*}]

#CHEAP FIX? MENTION IN MEETING
set_false_path -from[get_clocks{*}] -to [get_clocks{*}]

