create_clock -period 2 clk
create_clock -period 3 clk2
create_clock -period 3.5 -name virtual_io_clock
set_clock_groups -exclusive -group {clk} -group {clk2} 
set_input_delay -clock virtual_io_clock -max 0 [get_ports{*}]
set_output_delay -clock virtual_io_clock -max 0 [get_ports{*}]