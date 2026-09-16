create_clock -period 6.0 -name master_clk clk
create_generated_clock -source clk -divide_by 2 [get_pins {div_clk.Q[0]}] -name div_clk

# Source latency on the master clock sets the traversal starting point in
# update_generated_clock_source_latencies(). Since this shifts every derived
# clock by the same amount, all slacks should be identical to the no-latency
# baseline — any deviation indicates the latency is being ignored or double-counted.
set_clock_latency -source 1.0 [get_clocks {master_clk}]

set_input_delay -clock master_clk 1.0 [get_ports in]
set_output_delay -clock div_clk 1.0 [get_ports out]
