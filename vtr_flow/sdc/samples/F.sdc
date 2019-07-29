create_clock -period 3 -waveform {1.25 2.75} clk
create_clock -period 2 clk2
create_clock -period 1 -name input_clk
create_clock -period 0 -name output_clk
set_clock_latency -source 1.0 [get_clocks{clk}] 
#if neither early nor late is specified then the latency applies to early paths
set_clock_groups -exclusive -group input_clk -group clk2
set_false_path -from [get_clocks{clk}] -to [get_clocks{output_clk}]
set_input_delay -clock input_clk -max 0.5 [get_ports{in1 in2 in3}]
set_output_delay -clock output_clk -min 1 [get_ports{out*}]
set_max_delay 17 -from [get_clocks{input_clk}] -to [get_clocks{output_clk}]
set_min_delay 2 -from [get_clocks{input_clk}] -to [get_clocks{output_clk}]
set_multicycle_path -setup -from [get_clocks{clk}] -to [get_clocks{clk2}] 3 
#For multicycle_path, if setup is specified then hold is also implicity specified
set_clock_uncertainty -from [get_clocks{clk}] -to [get_clocks{clk2}] 0.75 
#For set_clock_uncertainty, if neither setup nor hold is unspecified then uncertainty is applied to both
set_disable_timing -from [get_pins {FFA.Q\[0\]}] -to [get_pins {to_FFD.in\[0\]}]
