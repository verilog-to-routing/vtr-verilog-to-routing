# clk and clk2 are asynchronous to each other, but both have analyzed transfers
# with a virtual IO clock which has a large source latency. Grouping is
# transitive, so all three clocks should be in one low-skew clock group whose
# target is the virtual IO clock's source latency.
create_clock -period 10 clk
create_clock -period 10 clk2
create_clock -period 10 -name virtual_io_clock
set_clock_groups -asynchronous -group {clk} -group {clk2}
set_clock_latency -source 10.0 [get_clocks {virtual_io_clock}]
set_input_delay -clock virtual_io_clock -max 0 [get_ports {in1 in2 in3}]
set_output_delay -clock virtual_io_clock -max 0 [get_ports {out*}]
