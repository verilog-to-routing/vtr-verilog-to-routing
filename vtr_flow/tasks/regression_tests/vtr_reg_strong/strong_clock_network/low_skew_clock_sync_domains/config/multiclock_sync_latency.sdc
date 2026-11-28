# Two synchronous clocks, where clk has a source latency. The source latency is
# part of clk's insertion delay, so clk2 should target clk's (larger) delay.
create_clock -period 10 clk
create_clock -period 10 clk2
set_clock_latency -source 1.0 [get_clocks {clk}]
