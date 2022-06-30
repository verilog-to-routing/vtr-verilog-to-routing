create_clock -period 0.1 -name impossible 

set_input_delay -clock impossible 0 [get_ports{*}] 

set_output_delay -clock impossible 0 [get_ports{*}]

set_min_delay 0 -from [get_clocks{*}] -to [get_clocks{*}]

set_max_delay 0 -from [get_clocks{*}] -to [get_clocks{*}]
