set_clock_latency -source -late 3.4 [get_clocks clk1]
set_clock_latency -source -late 3.4 [get_clocks clk*]
set_clock_latency -source -early 3.4 [get_clocks {clk2}]
set_clock_latency -source 3.4 [get_clocks {clk3}]
