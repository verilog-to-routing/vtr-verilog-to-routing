# clk2 has no create_clock, so its clock pins are not timed. Its clock net
# should be routed without RCV, while clk is grouped with the virtual IO clock.
create_clock -period 10 clk
create_clock -period 10 -name virtual_io_clock
set_clock_latency -source 10.0 [get_clocks {virtual_io_clock}]
set_input_delay -clock virtual_io_clock -max 0 [get_ports {in1 in2 in3}]
set_output_delay -clock virtual_io_clock -max 0 [get_ports {out*}]
