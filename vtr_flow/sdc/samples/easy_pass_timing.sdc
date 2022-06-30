create_clock -period 10000 -name easy 

set_input_delay -clock easy 0 [get_ports{*}] 

set_output_delay -clock easy 0 [get_ports{*}]

set_min_delay 0 -from [get_clocks{*}] -to [get_clocks{*}]

set_max_delay 0 -from [get_clocks{*}] -to [get_clocks{*}]

set_false_path -from[get_clocks{*}] -to [get_clocks{*}]

