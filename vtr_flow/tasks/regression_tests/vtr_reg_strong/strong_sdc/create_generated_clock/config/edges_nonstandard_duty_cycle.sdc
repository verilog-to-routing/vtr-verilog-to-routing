# Create a master clock with a standard 50% duty cycle (rise=0, fall=3).
create_clock -period 6.0 -name master_clk [get_ports clk]

# Create a generated clock with a non-standard 25% duty cycle using -edges {1 2 5}.
# The mixed odd/even indices cannot be expressed with -divide_by or -multiply_by.
# Edge 1 (odd, cycle 0) -> rise at 0.0 ns
# Edge 2 (even, cycle 0) -> fall at 3.0 ns  (fall of generated clock)
# Edge 5 (odd, cycle 2) -> rise at 12.0 ns (next rise, so period = 12.0 ns)
# Generated clock: rise=0 ns, fall=3 ns, period=12 ns (25% duty cycle).
create_generated_clock -source [get_ports clk] -edges {1 2 5} [get_ports clk2]
