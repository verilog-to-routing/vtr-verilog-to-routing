# Create a master clock on the clk port.
create_clock -period 6.0 -name master_clk [get_ports clk]

# Create a generated clock using -edges {1 3 5}, which is equivalent to -divide_by 2.
# Edge 1 (odd, cycle 0) -> rise at 0.0 ns
# Edge 3 (odd, cycle 1) -> rise at 6.0 ns  (fall of generated clock)
# Edge 5 (odd, cycle 2) -> rise at 12.0 ns (next rise, so period = 12.0 ns)
# Generated clock: rise=0 ns, fall=6 ns, period=12 ns.
create_generated_clock -source [get_ports clk] -edges {1 3 5} [get_ports clk2]
