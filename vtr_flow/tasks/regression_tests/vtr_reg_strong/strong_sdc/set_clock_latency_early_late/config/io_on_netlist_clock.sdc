# Netlist clocks
create_clock -period 3 clk
create_clock -period 2 clk2

# Different early and late source latencies, so launch and capture clocks see different latencies
set_clock_latency -source -early 0.3 [get_clocks {clk}]
set_clock_latency -source -late 0.7 [get_clocks {clk}]

# The I/Os are constrained on the netlist clock, so it captures the primary outputs
set_input_delay -clock clk 0.25 [get_ports {in*}]
set_output_delay -clock clk 1 [get_ports {out*}]
