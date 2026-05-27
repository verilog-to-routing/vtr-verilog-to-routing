# Basic clock crossing where the launch is 2x divided relative to the capture clock.
create_clock -period 12.0 clk
create_clock -period 6.0 clk2

set_input_delay -clock clk 1.0 [get_ports in]
set_output_delay -clock clk2 1.0 [get_ports out]

set_multicycle_path -from [get_clocks clk] -to [get_clocks clk2] 2
