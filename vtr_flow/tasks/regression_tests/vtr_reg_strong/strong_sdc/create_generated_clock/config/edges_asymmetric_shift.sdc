# Create a master clock with a standard 50% duty cycle (rise=0, fall=3).
create_clock -period 6.0 -name master_clk [get_ports clk]

# Create a generated clock using -edges with an asymmetric -edge_shift.
# Each shift value is applied independently to its corresponding edge, changing the duty cycle.
# Edge 1 (odd, cycle 0) -> 0.0 + 0.0 = 0.0 ns  (rise of generated clock)
# Edge 3 (odd, cycle 1) -> 6.0 + (-1.0) = 5.0 ns  (fall of generated clock)
# Edge 5 (odd, cycle 2) -> 12.0 + 0.0 = 12.0 ns (next rise, so period = 12.0 ns)
# Generated clock: rise=0 ns, fall=5 ns, period=12 ns (~41.7% duty cycle).
create_generated_clock -source [get_ports clk] -edges {1 3 5} -edge_shift {0.0 -1.0 0.0} [get_ports clk2]
