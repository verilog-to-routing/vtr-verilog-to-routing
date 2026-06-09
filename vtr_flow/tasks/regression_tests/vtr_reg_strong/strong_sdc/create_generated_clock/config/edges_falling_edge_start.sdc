# Create a master clock with a standard 50% duty cycle (rise=0, fall=3).
create_clock -period 6.0 -name master_clk [get_ports clk]

# Create a generated clock whose first edge index is even (a falling edge).
# This exercises the even-index -> fall_edge path for edges[0].
# Edge 2 (even, cycle 0) -> fall at 3.0 ns  (rise of generated clock)
# Edge 3 (odd, cycle 1)  -> rise at 6.0 ns  (fall of generated clock)
# Edge 4 (even, cycle 1) -> fall at 9.0 ns  (next rise, so period = 9-3 = 6.0 ns)
# Generated clock: rise=3 ns, fall=6 ns, period=6 ns (phase-shifted by half a master period).
create_generated_clock -source [get_ports clk] -edges {2 3 4} [get_ports clk2]
