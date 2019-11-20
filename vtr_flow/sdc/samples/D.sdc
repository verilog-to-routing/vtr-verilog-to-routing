create_clock -period 3 -waveform {1.25 2.75} clk # rising edge at 1.25, falling at 2.75
create_clock -period 2 clk2
create_clock -period 2.5 -name virtual_io_clock
set_input_delay -clock virtual_io_clock -max 1 [get_ports{*}]
set_output_delay -clock virtual_io_clock -max 0.5 [get_ports{*}]
