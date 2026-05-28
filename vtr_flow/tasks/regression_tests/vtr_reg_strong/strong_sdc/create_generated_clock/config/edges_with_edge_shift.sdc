# Create a master clock with a standard 50% duty cycle (rise=0, fall=3).
create_clock -period 6.0 -name master_clk [get_ports clk]

# Create a generated clock using -edges with a uniform -edge_shift.
# A uniform shift is equivalent to a pure phase offset on a divide-by-2 clock.
# Edge 1 (odd, cycle 0) -> 0.0 + 0.5 = 0.5 ns  (rise of generated clock)
# Edge 3 (odd, cycle 1) -> 6.0 + 0.5 = 6.5 ns  (fall of generated clock)
# Edge 5 (odd, cycle 2) -> 12.0 + 0.5 = 12.5 ns (next rise, so period = 12.0 ns)
# Generated clock: rise=0.5 ns, fall=6.5 ns, period=12 ns.
create_generated_clock -source [get_ports clk] -edges {1 3 5} -edge_shift {0.5 0.5 0.5} [get_ports clk2]
