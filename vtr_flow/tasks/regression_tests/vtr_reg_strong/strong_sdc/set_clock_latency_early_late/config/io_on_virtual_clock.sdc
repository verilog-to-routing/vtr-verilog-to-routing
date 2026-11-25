# Netlist clocks
create_clock -period 3 clk
create_clock -period 2 clk2

# Virtual clock for the I/Os
create_clock -period 3 -name io_clk

# Different early and late source latencies on every clock, so launch and capture clocks see different latencies
set_clock_latency -source -early 0.2 [get_clocks {*}]
set_clock_latency -source -late 0.5 [get_clocks {*}]

# The I/Os are constrained on the virtual clock, so it captures the primary outputs
set_input_delay -clock io_clk 0.25 [get_ports {in*}]
set_output_delay -clock io_clk 1 [get_ports {out*}]
