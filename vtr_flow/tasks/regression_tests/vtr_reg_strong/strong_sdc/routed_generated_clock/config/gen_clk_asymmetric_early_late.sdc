create_clock -period 6.0 -name master_clk clk
create_generated_clock -source clk -divide_by 2 [get_pins {div_clk.Q[0]}] -name div_clk

# Asymmetric EARLY and LATE user latency on the generated clock, set via separate
# calls. EARLY and LATE must be stored and applied independently:
#   source_latency(LATE)  = routed_network_MAX + 0.7
#   source_latency(EARLY) = routed_network_MIN + 0.3
# If one overwrites the other, or both collapse to the same value, the intra-domain
# div_clk critical path delay changes from its expected value.
set_clock_latency -source -late 0.7 [get_clocks {div_clk}]
set_clock_latency -source -early 0.3 [get_clocks {div_clk}]

set_input_delay -clock master_clk 1.0 [get_ports in]
set_output_delay -clock div_clk 1.0 [get_ports out]
