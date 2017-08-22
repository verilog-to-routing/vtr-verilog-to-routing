create_clock -period 3 -waveform {1.25 2.75} clk
create_clock -period 2 clk2
create_clock -period 1 -name input_clk
create_clock -period 0 -name output_clk
set_clock_groups -exclusive -group [get_clocks {input_clk}] -group [get_clocks {clk2}]
set_false_path -from [get_clocks{clk}] -to [get_clocks{output_clk}]
set_max_delay 17 -from [get_clocks{input_clk}] -to [get_clocks{output_clk}]
set_multicycle_path -setup -from [get_clocks{clk}] -to [get_clocks{clk2}] 3
set_input_delay -clock input_clk 0.5 [get_ports{in1 in2 in3}]
set_output_delay -clock output_clk 1 [get_ports{out*}]
