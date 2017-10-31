#Latency
set_clock_latency -source -early -late 3.4 [get_clocks clk*]
set_clock_latency -source 3.4 [get_clocks clk*]

#Derate
set_timing_derate 0.9
set_timing_derate -early -late 0.9
