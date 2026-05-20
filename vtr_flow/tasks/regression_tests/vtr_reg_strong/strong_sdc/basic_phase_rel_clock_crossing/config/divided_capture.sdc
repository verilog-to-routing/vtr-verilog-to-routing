# Basic clock crossing where the capture is 2x divided relative to the launch clock.
create_clock -period 6.0 clk
create_clock -period 12.0 clk2

set_input_delay -clock clk 1.0 [get_ports in]
set_output_delay -clock clk2 1.0 [get_ports out]
