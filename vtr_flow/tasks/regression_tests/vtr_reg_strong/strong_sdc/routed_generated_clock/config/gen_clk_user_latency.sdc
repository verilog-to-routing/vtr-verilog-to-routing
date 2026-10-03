create_clock -period 6.0 -name master_clk clk
create_generated_clock -source clk -divide_by 2 [get_pins {div_clk.Q[0]}] -name div_clk

# User-specified source latency on the generated clock.
# update_generated_clock_source_latencies() is called on every timing update during
# place-and-route. The 0.5 ns must be stacked once on top of the routed network
# delay (via user_source_latency), not accumulated on each call. If source_latency
# were used instead of user_source_latency, repeated calls would compound the value.
set_clock_latency -source 0.5 [get_clocks {div_clk}]

set_input_delay -clock master_clk 1.0 [get_ports in]
set_output_delay -clock div_clk 1.0 [get_ports out]
