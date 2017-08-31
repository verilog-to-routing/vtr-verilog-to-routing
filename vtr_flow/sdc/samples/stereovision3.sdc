create_clock -period 10 *
set_clock_latency -source 2 -late [get_clocks {clk}]